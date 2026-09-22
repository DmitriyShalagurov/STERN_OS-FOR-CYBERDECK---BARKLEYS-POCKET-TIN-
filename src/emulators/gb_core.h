#ifndef GB_CORE_H
#define GB_CORE_H

#include "emulator_interface.h"
#include <FS.h>

class GbCore : public IEmulator {
public:
    GbCore();
    ~GbCore() override;

    const char* getName() override { return "Game Boy (Peanut-GB)"; }
    const char* getExtension() override { return ".gb"; }

    bool loadRom(const char* path) override;
    void reset() override;
    void stepFrame() override;
    void handleInput(uint16_t buttons) override;
    void render(TFT_eSPI& tft) override;
    void stop() override;
    uint32_t getFrameTimeUs() override { return 16742; } // ~59.7 FPS Game Boy timing

    // ROM-Read Callback für Peanut-GB
    uint8_t readRomByte(uint32_t addr);
    uint8_t readCartRamByte(uint32_t addr);
    void writeCartRamByte(uint32_t addr, uint8_t val);
    void drawLcdLine(const uint8_t pixels[], uint8_t line);

private:
    void* _gbContext;
    uint8_t* _romData;
    size_t _romSize;
    uint8_t* _cartRam;
    size_t _cartRamSize;
    File _romFile;
    bool _useFilePaging;
    bool _isRunning;

    // Line buffer / Framebuffer rendering
    static TFT_eSPI* _currentTft;
    static GbCore* _instance;

    // Cache für File-Paging wenn kein PSRAM vorhanden ist
    uint8_t _bankCache[16384];
    int32_t _cachedBank;

    void freeBuffers();
};

#endif // GB_CORE_H

