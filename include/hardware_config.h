#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include <Arduino.h>

// =============================================================================
// STERN OS Hardware Configuration
// Automatische oder konfigurierbare Pin-Zuordnung für verschiedene ESP32 Chips
// =============================================================================

#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_ESP32S3_DEV)
    // -------------------------------------------------------------------------
    // ESP32-S3 Konfiguration
    // -------------------------------------------------------------------------
    #define BOARD_NAME "ESP32-S3 Cyberdeck"

    // MicroSD SPI Pins (ESP32-S3 Standard)
    #ifndef SD_CS
    #define SD_CS   10
    #endif
    #ifndef SD_MISO
    #define SD_MISO 13
    #endif
    #ifndef SD_MOSI
    #define SD_MOSI 11
    #endif
    #ifndef SD_SCK
    #define SD_SCK  12
    #endif

    // Tastatur TCA8418 I2C Pins
    #ifndef KEYBOARD_SDA
    #define KEYBOARD_SDA 8
    #endif
    #ifndef KEYBOARD_SCL
    #define KEYBOARD_SCL 9
    #endif

    // Sekundäres Status-OLED I2C Pins (falls vorhanden)
    #ifndef OLED_SDA
    #define OLED_SDA 17
    #endif
    #ifndef OLED_SCL
    #define OLED_SCL 18
    #endif

#elif defined(CONFIG_IDF_TARGET_ESP32C3) || defined(ARDUINO_ESP32C3_DEV)
    // -------------------------------------------------------------------------
    // ESP32-C3 Konfiguration (RISC-V)
    // -------------------------------------------------------------------------
    #define BOARD_NAME "ESP32-C3 Cyberdeck"

    // MicroSD SPI Pins
    #ifndef SD_CS
    #define SD_CS   7
    #endif
    #ifndef SD_MISO
    #define SD_MISO 5
    #endif
    #ifndef SD_MOSI
    #define SD_MOSI 6
    #endif
    #ifndef SD_SCK
    #define SD_SCK  4
    #endif

    // I2C Pins (Shared oder TCA8418)
    #ifndef KEYBOARD_SDA
    #define KEYBOARD_SDA 8
    #endif
    #ifndef KEYBOARD_SCL
    #define KEYBOARD_SCL 9
    #endif

    #ifndef OLED_SDA
    #define OLED_SDA 8
    #endif
    #ifndef OLED_SCL
    #define OLED_SCL 9
    #endif

#else
    // -------------------------------------------------------------------------
    // ESP32-WROOM-32 / ESP32-WROVER (Barkleys Tin Standard-PCB)
    // -------------------------------------------------------------------------
    #define BOARD_NAME "ESP32 Barkleys Pocket Tin"

    // MicroSD SPI Pins
    #ifndef SD_CS
    #define SD_CS   27
    #endif
    #ifndef SD_MISO
    #define SD_MISO 26
    #endif
    #ifndef SD_MOSI
    #define SD_MOSI 13
    #endif
    #ifndef SD_SCK
    #define SD_SCK  14
    #endif

    // Tastatur TCA8418 I2C Pins (Standard Wire)
    #ifndef KEYBOARD_SDA
    #define KEYBOARD_SDA 21
    #endif
    #ifndef KEYBOARD_SCL
    #define KEYBOARD_SCL 22
    #endif

    // Sekundäres Status-OLED I2C Pins (Wire1)
    #ifndef OLED_SDA
    #define OLED_SDA 32
    #endif
    #ifndef OLED_SCL
    #define OLED_SCL 25
    #endif

#endif

// Audio / Buzzer Pin (optional)
#ifndef BUZZER_PIN
#define BUZZER_PIN 25
#endif

// Speicher-Erkennung
inline bool hasPSRAM() {
#if defined(BOARD_HAS_PSRAM)
    return psramFound();
#else
    return false;
#endif
}

#endif // HARDWARE_CONFIG_H

