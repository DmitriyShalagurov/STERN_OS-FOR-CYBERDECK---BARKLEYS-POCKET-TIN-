#include "nes_emulator.h"
#include "emulators/emulator_manager.h"

void startNESGame(String romPath) {
    EmuManager.launchRom(romPath);
}

void runNES() {
    EmuManager.showConsoleMenu(&EmuManager.getNesCore());
}