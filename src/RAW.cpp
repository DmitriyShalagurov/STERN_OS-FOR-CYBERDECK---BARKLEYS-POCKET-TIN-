#include "RAW.h"
#include <TFT_eSPI.h>
#include "hal/storage_hal.h"
#include "keypad.h"
#include "status_bar.h"
#include "file_browser.h"

extern TFT_eSPI tft;


#define COLOR_MODE 0   
#define SWAP_BYTES 1  

static const uint8_t perm[6][3] = {
    {0,1,2}, // RGB
    {0,2,1}, // RBG
    {1,0,2}, // GRB
    {1,2,0}, // GBR
    {2,0,1}, // BRG
    {2,1,0}  // BGR
};

// Вспомогательная функция для отображения одного RAW-файла
static bool displayRAWFile(const char* filename) {
    File file = Storage.openFile(filename);
    if (!file) return false;

    if (file.size() != 320UL * 240UL * 2UL) {
        file.close();
        return false;
    }

    uint8_t rawLine[640];
    uint16_t pixelLine[320];
    const uint8_t* p = perm[COLOR_MODE];

    for (int y = 0; y < 240; y++) {
        if (file.read(rawLine, 640) != 640) break;
        for (int x = 0; x < 320; x++) {
            uint16_t color = ((uint16_t)rawLine[x * 2 + 1] << 8) | rawLine[x * 2];
            uint8_t r = (color >> 11) & 0x1F;
            uint8_t g = (color >> 5) & 0x3F;
            uint8_t b = color & 0x1F;
            uint8_t channels[3] = {r, g, b};
            uint8_t r2 = channels[p[0]];
            uint8_t g2 = channels[p[1]];
            uint8_t b2 = channels[p[2]];
            uint16_t rgb565 = ((r2 & 0x1F) << 11) | ((g2 & 0x3F) << 5) | (b2 & 0x1F);
#if SWAP_BYTES
            rgb565 = ((rgb565 >> 8) & 0x00FF) | ((rgb565 & 0x00FF) << 8);
#endif
            pixelLine[x] = rgb565;
        }
        tft.pushImage(0, y, 320, 1, pixelLine);
    }

    file.close();
    return true;
}

bool showRAWImage(const char* filename) {
   
    if (!displayRAWFile(filename)) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setCursor(10, 100);
        tft.println("Cannot open RAW file");
        delay(1000);
        return false;
    }

    
    while (true) {
        KeyCode key = Keypad.getKey();
        if (key == KEY_ESC) break;

      
        if (key == KEY_Z) {
            if (selectedIndex > 0) {
                selectedIndex--;
                String newFile = fileList[selectedIndex];
          
                if (newFile.endsWith(".raw") || newFile.endsWith(".RAW")) {
                    tft.fillScreen(TFT_BLACK);
                    displayRAWFile(newFile.c_str());
                }
            }
        } else if (key == KEY_C) {
            if (selectedIndex < (int)fileList.size() - 1) {
                selectedIndex++;
                String newFile = fileList[selectedIndex];
                if (newFile.endsWith(".raw") || newFile.endsWith(".RAW")) {
                    tft.fillScreen(TFT_BLACK);
                    displayRAWFile(newFile.c_str());
                }
            }
        }
        delay(20);
    }

    drawStatusBar();
    setRawImageJustViewed();
    return true;
}