#include "boot_animation.h"
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

// Функция для получения цвета по кругу (0-255) в формате RGB565
uint16_t rainbowColor(int pos) {
    byte r, g, b;
    if (pos < 85) {
        r = pos * 3;
        g = 255 - pos * 3;
        b = 0;
    } else if (pos < 170) {
        pos -= 85;
        r = 255 - pos * 3;
        g = 0;
        b = pos * 3;
    } else {
        pos -= 170;
        r = 0;
        g = pos * 3;
        b = 255 - pos * 3;
    }
    return tft.color565(r, g, b);
}

void showBootAnimation() {
    // ========================================================
    // СТАРАЯ АНИМАЦИЯ ЗВЁЗД – ЗАКОММЕНТИРОВАНА
    // ========================================================
    /*
    (закомментированный код)
    */

    // === БАННЕР (4 строки) ===
    const char* banner[] = {
        "   ___  ___   ___  __ ____   ______  __       ___  _____",
        "  / _ )/ _ | / _ \\/ //_/ /  / __/\\ \\/ / _/|  / _ \\/ ___/",
        " / _  / __ |/ , _/ ,< / /__/ _/   \\  / > _< / ___/ /__  ",
        "/____/_/ |_/_/|_/_/|_/____/___/   /_/  |/  /_/   \\___/ "
    };
    int lines = sizeof(banner) / sizeof(banner[0]);

    tft.setTextSize(1);
    unsigned long start = millis();

    while (millis() - start < 3000) {
        tft.fillScreen(TFT_BLACK);

        int hue = (millis() / 10) % 256;
        uint16_t color = rainbowColor(hue);

        int lineHeight = tft.fontHeight();
        int totalHeight = lines * lineHeight;
        // Смещаем блок вверх на 15 пикселей
        int y = (tft.height() - totalHeight) / 2 - 8;

        if (tft.width() >= 280) {
            for (int i = 0; i < lines; i++) {
                int x = (tft.width() - tft.textWidth(banner[i])) / 2;
                if (x < 0) x = 0;
                tft.setCursor(x, y);
                tft.setTextColor(color, TFT_BLACK);
                tft.print(banner[i]);
                y += lineHeight;
            }
        } else {
            tft.setTextSize(2);
            tft.setCursor((tft.width() - tft.textWidth("STERN OS")) / 2, (tft.height() - 20) / 2);
            tft.setTextColor(color, TFT_BLACK);
            tft.print("STERN OS");
            tft.setTextSize(1);
        }

        delay(30);
    }

    tft.fillScreen(TFT_BLACK);
}