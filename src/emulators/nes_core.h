#ifndef NES_CORE_H
#define NES_CORE_H

#include "emulator_interface.h"

class Cartridge;
class Bus;

class NesCore : public IEmulator {
public:
    NesCore();
    ~NesCore() override;

    const char* getName() override { return "Nintendo Entertainment System"; }
    const char* getExtension() override { return ".nes"; }

    bool loadRom(const char* path) override;
    void reset() override;
    void stepFrame() override;
    void handleInput(uint16_t buttons) override;
    void render(TFT_eSPI& tft) override;
    void stop() override;
    uint32_t getFrameTimeUs() override { return 16639; } // 60.098 FPS NTSC

private:
    Cartridge* _cart;
    Bus* _bus;
    bool _isRunning;
    uint8_t _controllerState;
};

#endif // NES_CORE_H

