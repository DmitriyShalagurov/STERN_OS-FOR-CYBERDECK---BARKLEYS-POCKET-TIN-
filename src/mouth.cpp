#include "mouth.h"
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "keypad.h"
#include <math.h>

extern Adafruit_SSD1306 display;
extern bool oledOK;

static float mouthEnergy = 0.0f;
static float mouthPhase  = 0.0f;
static bool  mouthReady  = false;

// ---------- один кадр ----------
static void mouthDrawFrame() {
    const int w = display.width();
    const int h = display.height();
    const int cy = h / 2;
    const int maxAmp = (h / 2) - 2;
    if (maxAmp < 2) return;

    mouthPhase = fmodf(mouthPhase + 0.62f, 1000.0f);

    display.clearDisplay();

    int prevY = -1;
    for (int x = 0; x < w; x++) {
        float t   = (float)x / (float)(w - 1);
        float env = sinf(t * PI);            // огибающая: края плоские

        // idle "дыхание" — всегда есть, даже без нажатий
        float s = sinf(mouthPhase * 0.5f + t * 3.0f) * 0.12f;

        // реакция на нажатия
        if (mouthEnergy > 0.01f) {
            s += sinf(mouthPhase + t * 6.5f)          * mouthEnergy * 0.75f;
            s += sinf(mouthPhase * 2.3f + t * 13.0f)  * mouthEnergy * 0.25f;
        }

        if (s >  1.0f) s =  1.0f;
        if (s < -1.0f) s = -1.0f;

        int y = cy - (int)(s * env * maxAmp);
        if (y < 1)     y = 1;
        if (y > h - 2) y = h - 2;

        if (prevY < 0) {
            display.drawPixel(x, y, SSD1306_WHITE);
        } else {
            int y0 = (prevY < y) ? prevY : y;
            int y1 = (prevY < y) ? y : prevY;
            display.drawFastVLine(x, y0, y1 - y0 + 1, SSD1306_WHITE);
        }
        prevY = y;
    }

    display.display();

    mouthEnergy *= 0.94f;
    if (mouthEnergy < 0.002f) mouthEnergy = 0.0f;
}

// ---------- публичное API ----------
void mouthBegin() {
    if (!oledOK) return;
    mouthEnergy = 0.0f;
    mouthPhase  = 0.0f;
    mouthReady  = true;
    display.clearDisplay();
    display.display();
}

void mouthUpdate() {
    if (!mouthReady || !oledOK) return;
    mouthDrawFrame();
}

void mouthTrigger(float amount) {
    if (!mouthReady) return;
    mouthEnergy += amount;
    if (mouthEnergy > 1.5f) mouthEnergy = 1.5f;
}

void mouthEnd() {
    if (!mouthReady || !oledOK) return;
    mouthReady = false;
    display.clearDisplay();
    display.display();
}

// ---------- одиночный режим (команда mouth) ----------
void runMouth() {
    if (!oledOK) return;

    mouthBegin();

    unsigned long lastFrame = 0;
    const unsigned long frameMs = 40;

    while (true) {
        KeyCode code = Keypad.getKey();
        if (code != KEY_NONE) {
            if (code == KEY_ESC) break;
            mouthTrigger(0.85f);
        }

        unsigned long now = millis();
        if (now - lastFrame >= frameMs) {
            lastFrame = now;
            mouthUpdate();
        }
        delay(1);
    }

    mouthEnd();
}