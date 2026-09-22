#ifndef DISPLAY_CONFIG_H
#define DISPLAY_CONFIG_H

#include <Arduino.h>

// =============================================================================
// STERN OS Display Konfiguration & Abstraktion
// Unterstützt: ST7789, ST7735, ILI9341, ILI9488, SSD1306
// =============================================================================

#if defined(DISPLAY_TYPE_ST7735_128x160)
    #define SCREEN_WIDTH  160
    #define SCREEN_HEIGHT 128
    #define UI_LINE_HEIGHT 9
    #define UI_LEFT_MARGIN 4
    #define DISPLAY_DRIVER_NAME "ST7735 160x128"

#elif defined(DISPLAY_TYPE_ST7735_128x128)
    #define SCREEN_WIDTH  128
    #define SCREEN_HEIGHT 128
    #define UI_LINE_HEIGHT 8
    #define UI_LEFT_MARGIN 2
    #define DISPLAY_DRIVER_NAME "ST7735 128x128"

#elif defined(DISPLAY_TYPE_ST7789_240x240)
    #define SCREEN_WIDTH  240
    #define SCREEN_HEIGHT 240
    #define UI_LINE_HEIGHT 10
    #define UI_LEFT_MARGIN 6
    #define DISPLAY_DRIVER_NAME "ST7789 240x240"

#elif defined(DISPLAY_TYPE_SSD1306_128x64)
    #define SCREEN_WIDTH  128
    #define SCREEN_HEIGHT 64
    #define UI_LINE_HEIGHT 8
    #define UI_LEFT_MARGIN 2
    #define DISPLAY_DRIVER_NAME "SSD1306 128x64"

#elif defined(DISPLAY_TYPE_ILI9341)
    #define SCREEN_WIDTH  320
    #define SCREEN_HEIGHT 240
    #define UI_LINE_HEIGHT 10
    #define UI_LEFT_MARGIN 10
    #define DISPLAY_DRIVER_NAME "ILI9341 320x240"

#else
    // Standard Barkleys Tin Display: ST7789 240x320 (im Querformat: 320x240)
    #ifndef SCREEN_WIDTH
    #define SCREEN_WIDTH  320
    #endif
    #ifndef SCREEN_HEIGHT
    #define SCREEN_HEIGHT 240
    #endif
    #ifndef UI_LINE_HEIGHT
    #define UI_LINE_HEIGHT 10
    #endif
    #ifndef UI_LEFT_MARGIN
    #define UI_LEFT_MARGIN 10
    #endif
    #define DISPLAY_DRIVER_NAME "ST7789 320x240 (Default)"
#endif

// Gemeinsame GUI Layout-Parameter
#define STATUS_BAR_HEIGHT 15
#define USABLE_SCREEN_HEIGHT (SCREEN_HEIGHT - STATUS_BAR_HEIGHT)

#endif // DISPLAY_CONFIG_H

