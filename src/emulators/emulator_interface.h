#ifndef EMULATOR_INTERFACE_H
#define EMULATOR_INTERFACE_H

#include <Arduino.h>
#include <TFT_eSPI.h>

// Virtuelle Tastenbelegung für alle Emulatoren
enum VirtualButton {
    BTN_NONE    = 0x0000,
    BTN_A       = 0x0001,
    BTN_B       = 0x0002,
    BTN_SELECT  = 0x0004,
    BTN_START   = 0x0080,
    BTN_UP      = 0x0010,
    BTN_DOWN    = 0x0020,
    BTN_LEFT    = 0x0040,
    BTN_RIGHT   = 0x0080,
    BTN_EXIT    = 0x0100
};

// =============================================================================
// IEmulator: Einheitliche Schnittstelle für alle Emulator-Kerne
// Ermöglicht das Hinzufügen weiterer Konsolen mit minimalem Codeaufwand
// =============================================================================
class IEmulator {
public:
    virtual ~IEmulator() {}

    // Name des Systems (z.B. "Nintendo Entertainment System", "Game Boy", "CHIP-8")
    virtual const char* getName() = 0;

    // Dateiendung (z.B. ".nes", ".gb", ".ch8")
    virtual const char* getExtension() = 0;

    // ROM von Datei oder Speicher laden
    virtual bool loadRom(const char* path) = 0;
    virtual bool loadRomFromMemory(const uint8_t* data, size_t size) { return false; }

    // Steuerung und Frame-Zyklus
    virtual void reset() = 0;
    virtual void stepFrame() = 0;
    virtual void handleInput(uint16_t buttons) = 0;
    virtual void render(TFT_eSPI& tft) = 0;
    virtual void stop() = 0;

    // Ziel-Framerate in Mikrosekunden (z.B. 16666 us für 60 Hz)
    virtual uint32_t getFrameTimeUs() { return 16666; }
};

#endif // EMULATOR_INTERFACE_H

