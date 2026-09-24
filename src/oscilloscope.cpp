#include "oscilloscope.h"
#include <TFT_eSPI.h>
#include "keypad.h"
#include <string.h>
#include <math.h>

extern TFT_eSPI tft;

// ----- Цвета -----
#define OSC_BG        0x0000   // чёрный фон
#define OSC_GRID      0x0320   // тёмно-зелёная сетка
#define OSC_GRID_HI   0x05E0   // яркая центральная линия
#define OSC_WAVE      0x07E0   // ярко-зелёная волна
#define OSC_TEXT      0x07E0
#define OSC_SUB       0x7BEF
#define OSC_BAR       0xFFE0   // жёлтый индикатор уровня

// ----- Параметры -----
#define OSC_MAX_W     320
#define OSC_FRAME_MS  33       // ~30 fps

static float waveBuf[OSC_MAX_W];
static int   waveLen = 0;

static float energy = 0.0f;   // «громкость» от нажатий
static float phase  = 0.0f;   // фаза для синусоид

// ------------------ Статичный фон (сетка + подписи) ------------------
static void drawStaticBackground(int w, int h) {
    tft.fillScreen(OSC_BG);

    // сетка
    for (int x = 0; x < w; x += 32) tft.drawFastVLine(x, 0, h, OSC_GRID);
    for (int y = 0; y < h; y += 32) tft.drawFastHLine(0, y, w, OSC_GRID);

    // центральные линии
    tft.drawFastHLine(0, h / 2, w, OSC_GRID_HI);
    tft.drawFastVLine(w / 2, 0, h, OSC_GRID_HI);

    // шапка
    tft.setTextSize(1);
    tft.setTextColor(OSC_TEXT, OSC_BG);
    tft.setCursor(4, 4);
    tft.print("OSCILLOSCOPE");

    tft.setTextColor(OSC_SUB, OSC_BG);
    if (w > 60) {
        tft.setCursor(w - 52, 4);
        tft.print("30 fps");
    }

    tft.setCursor(4, h - 10);
    tft.print("Keys = signal    ESC = exit");
}

// ------------------ Перерисовка волны (только полоса вокруг центра) ------------------
static void redrawWaveBand(int w, int h) {
    int cy  = h / 2;
    int amp = (h / 2) - 18;
    if (amp < 8) amp = 8;

    int top = cy - amp - 4;
    int bot = cy + amp + 4;
    if (top < 0) top = 0;
    if (bot >= h) bot = h - 1;
    int bandH = bot - top + 1;

    // 1) очищаем полосу
    tft.fillRect(0, top, w, bandH, OSC_BG);

    // 2) горизонтальные линии сетки внутри полосы
    int yStart = ((top + 31) / 32) * 32;
    for (int y = yStart; y <= bot; y += 32) {
        uint16_t c = (y == h / 2) ? OSC_GRID_HI : OSC_GRID;
        tft.drawFastHLine(0, y, w, c);
    }
    if (h / 2 >= top && h / 2 <= bot) {
        tft.drawFastHLine(0, h / 2, w, OSC_GRID_HI);
    }

    // 3) вертикальные линии сетки
    for (int x = 0; x < w; x += 32) {
        tft.drawFastVLine(x, top, bandH, OSC_GRID);
    }

    // 4) сама волна
    if (waveLen < 2) return;

    int n = waveLen;
    if (n > w) n = w;
    int offset = waveLen - n;   // самый старый сэмпл, который влез

    int prevX = -1, prevY = 0;
    for (int i = 0; i < n; i++) {
        int x = w - n + i;      // новые сэмплы — справа
        int y = cy - (int)(waveBuf[offset + i] * amp);
        if (y < top + 1) y = top + 1;
        if (y > bot - 1) y = bot - 1;

        if (prevX >= 0) {
            tft.drawLine(prevX, prevY, x, y, OSC_WAVE);
        }
        prevX = x;
        prevY = y;
    }
}

// ------------------ Главная функция ------------------
void runOscilloscope() {
    int w = tft.width();
    int h = tft.height();
    if (w > OSC_MAX_W) w = OSC_MAX_W;

    waveLen = 0;
    energy  = 0.0f;
    phase   = 0.0f;
    memset(waveBuf, 0, sizeof(waveBuf));

    tft.startWrite();
    drawStaticBackground(w, h);
    tft.endWrite();

    unsigned long lastFrame = 0;
    bool running = true;

    while (running) {
        // --- Клавиатура ---
        KeyCode code = Keypad.getKey();
        if (code != KEY_NONE) {
            if (code == KEY_ESC) {
                running = false;
                break;
            }
            // любая другая клавиша — «триггер сигнала»
            energy += 0.85f;
            if (energy > 1.5f) energy = 1.5f;
        }

        // --- Кадр ---
        unsigned long now = millis();
        if (now - lastFrame >= OSC_FRAME_MS) {
            lastFrame = now;

            // фаза
            phase = fmodf(phase + 0.55f, 1000.0f);

            // генерируем сэмпл (смесь синусоид + шум)
            float s = 0.0f;
            if (energy > 0.02f) {
                s  = sinf(phase)         * energy * 0.85f;
                s += sinf(phase * 2.31f) * energy * 0.25f;
                s += sinf(phase * 5.77f) * energy * 0.12f;
            }
            s += ((float)random(-1000, 1001) / 1000.0f) * 0.02f;

            if (s >  1.0f) s =  1.0f;
            if (s < -1.0f) s = -1.0f;

            // затухание
            energy *= 0.94f;
            if (energy < 0.002f) energy = 0.0f;

            // сдвиг влево + новый сэмпл
            if (waveLen < w) {
                waveBuf[waveLen++] = s;
            } else {
                memmove(waveBuf, waveBuf + 1, (w - 1) * sizeof(float));
                waveBuf[w - 1] = s;
            }

            // --- Рисуем ---
            tft.startWrite();
            redrawWaveBand(w, h);

            // индикатор уровня
            int barX    = 6;
            int barY    = h - 20;
            int barMaxW = w - 12;
            int fill    = (int)((energy / 1.5f) * barMaxW);
            if (fill > barMaxW) fill = barMaxW;

            tft.fillRect(barX,      barY,     barMaxW, 8, OSC_BG);
            tft.drawRect(barX,      barY,     barMaxW, 8, OSC_GRID);
            if (fill > 0)
                tft.fillRect(barX + 1, barY + 1, fill, 6, OSC_BAR);

            tft.endWrite();
        }

        delay(1);
    }

    // очищаем экран перед возвратом в терминал
    tft.fillScreen(TFT_BLACK);
}