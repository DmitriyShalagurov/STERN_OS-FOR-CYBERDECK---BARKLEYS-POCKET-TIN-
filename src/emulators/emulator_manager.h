#ifndef EMULATOR_MANAGER_H
#define EMULATOR_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "emulator_interface.h"
#include "nes_core.h"
#include "gb_core.h"
#include "chip8_core.h"
#include "keypad.h"

// =============================================================================
// EmulatorManager: Zentraler Verwalter & Ausführer aller Emulatoren (DRY)
// Verbindet einheitliches Tasten-Mapping, 60Hz-Timing und Menüführung
// =============================================================================

class EmulatorManager {
public:
    EmulatorManager();
    ~EmulatorManager();

    // Startet eine beliebige ROM-Datei anhand der Dateiendung (.nes, .gb, .ch8)
    bool launchRom(const String& path);

    // Startet ein vorinstalliertes Flash-ROM (CHIP-8)
    bool launchBuiltInRom(int index);

    // Zeigt das Haupt-Spielemenü mit allen Konsolen
    void showEmulatorMenu();

    // Zeigt die Liste der ROMs für eine bestimmte Konsole
    void showConsoleMenu(IEmulator* emu);

    // Zentraler Ausführungs-Loop für jeden Emulator
    void runEmulator(IEmulator* emu);

    NesCore& getNesCore() { return _nesCore; }
    GbCore& getGbCore() { return _gbCore; }
    Chip8Core& getChip8Core() { return _chip8Core; }

private:
    NesCore _nesCore;
    GbCore _gbCore;
    Chip8Core _chip8Core;
    std::vector<IEmulator*> _emulators;

    uint16_t pollInput();
};

extern EmulatorManager EmuManager;

#endif // EMULATOR_MANAGER_H

