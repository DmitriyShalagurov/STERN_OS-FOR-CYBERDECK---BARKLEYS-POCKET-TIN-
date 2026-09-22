#ifndef CHIP8_CORE_H
#define CHIP8_CORE_H

#include "emulator_interface.h"
#include <vector>

struct BuiltInRom {
    const char* name;
    const uint8_t* data;
    size_t size;
};

class Chip8Core : public IEmulator {
public:
    Chip8Core();
    ~Chip8Core() override;

    const char* getName() override { return "CHIP-8 / SuperChip"; }
    const char* getExtension() override { return ".ch8"; }

    bool loadRom(const char* path) override;
    bool loadRomFromMemory(const uint8_t* data, size_t size) override;

    void reset() override;
    void stepFrame() override;
    void handleInput(uint16_t buttons) override;
    void render(TFT_eSPI& tft) override;
    void stop() override;
    uint32_t getFrameTimeUs() override { return 16666; } // 60 Hz

    // Zugriff auf vorinstallierte Flash-ROMs
    static const std::vector<BuiltInRom>& getBuiltInRoms();

private:
    uint8_t memory[4096];
    uint8_t V[16];
    uint16_t I;
    uint16_t pc;
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint16_t stack[16];
    uint8_t sp;
    uint8_t keypad[16];
    uint8_t gfx[64 * 32];
    bool drawFlag;
    bool isRunning;

    void executeOpcode();
    void initFont();
};

#endif // CHIP8_CORE_H

