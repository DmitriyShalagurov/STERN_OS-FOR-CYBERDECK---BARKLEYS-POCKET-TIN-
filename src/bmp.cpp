#include "bmp.h"
#include <TFT_eSPI.h>
#include "hal/storage_hal.h"
#include "keypad.h"
#include "status_bar.h"
#include "file_browser.h"

extern TFT_eSPI tft;

// =============================================
// НАСТРОЙКИ (меняйте их для подбора цветов)
// =============================================
#define COLOR_MODE 0   // 0..5 (перестановка каналов)
#define SWAP_BYTES 1   // 0 = выкл, 1 = вкл (меняет байты местами в 16-битном слове)
// =============================================

static const uint8_t perm[6][3] = {
    {0,1,2}, // RGB
    {0,2,1}, // RBG
    {1,0,2}, // GRB
    {1,2,0}, // GBR
    {2,0,1}, // BRG
    {2,1,0}  // BGR
};

#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BMPFileHeader;

typedef struct {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BMPInfoHeader;
#pragma pack(pop)

// Вспомогательная функция для отображения одного BMP-файла
static bool displayBMPFile(const char* filename) {
    File file = Storage.openFile(filename);
    if (!file) return false;

    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;
    if (file.read((uint8_t*)&fileHeader, sizeof(fileHeader)) != sizeof(fileHeader)) return false;
    if (file.read((uint8_t*)&infoHeader, sizeof(infoHeader)) != sizeof(infoHeader)) return false;

    if (fileHeader.bfType != 0x4D42) return false;
    if (infoHeader.biBitCount != 24 || infoHeader.biCompression != 0) return false;
    if (infoHeader.biWidth != 320 || abs(infoHeader.biHeight) != 240) return false;

    bool topDown = (infoHeader.biHeight < 0);
    int height = abs(infoHeader.biHeight);
    int width = infoHeader.biWidth;
    int stride = (width * 3 + 3) & ~3;

    file.seek(fileHeader.bfOffBits);

    uint16_t lineBuffer[320];
    const uint8_t* p = perm[COLOR_MODE];

    for (int y = 0; y < height; y++) {
        int row = topDown ? y : (height - 1 - y);
        uint8_t bgrRow[320 * 3];
        size_t bytesRead = file.read(bgrRow, stride);
        if (bytesRead != stride) break;

        for (int x = 0; x < width; x++) {
            uint8_t b = bgrRow[x * 3];
            uint8_t g = bgrRow[x * 3 + 1];
            uint8_t r = bgrRow[x * 3 + 2];
            uint8_t channels[3] = {r, g, b};
            uint8_t r2 = channels[p[0]];
            uint8_t g2 = channels[p[1]];
            uint8_t b2 = channels[p[2]];
            uint16_t rgb565 = ((r2 & 0xF8) << 8) | ((g2 & 0xFC) << 3) | (b2 >> 3);
#if SWAP_BYTES
            rgb565 = ((rgb565 >> 8) & 0x00FF) | ((rgb565 & 0x00FF) << 8);
#endif
            lineBuffer[x] = rgb565;
        }
        tft.pushImage(0, row, width, 1, lineBuffer);
    }

    file.close();
    return true;
}

bool showBMPImage(const char* filename) {
    if (!displayBMPFile(filename)) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setCursor(10, 100);
        tft.println("Cannot open BMP file");
        delay(1000);
        return false;
    }

    // Цикл ожидания клавиш с навигацией
    while (true) {
        KeyCode key = Keypad.getKey();
        if (key == KEY_ESC) break;

        if (key == KEY_Z) {
            if (selectedIndex > 0) {
                selectedIndex--;
                String newFile = fileList[selectedIndex];
                if (newFile.endsWith(".bmp") || newFile.endsWith(".BMP")) {
                    tft.fillScreen(TFT_BLACK);
                    displayBMPFile(newFile.c_str());
                }
            }
        } else if (key == KEY_C) {
            if (selectedIndex < (int)fileList.size() - 1) {
                selectedIndex++;
                String newFile = fileList[selectedIndex];
                if (newFile.endsWith(".bmp") || newFile.endsWith(".BMP")) {
                    tft.fillScreen(TFT_BLACK);
                    displayBMPFile(newFile.c_str());
                }
            }
        }
        delay(20);
    }

    drawStatusBar();
    setRawImageJustViewed();
    return true;
}