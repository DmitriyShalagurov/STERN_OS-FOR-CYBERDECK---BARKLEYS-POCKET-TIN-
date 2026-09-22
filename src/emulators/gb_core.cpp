#include "gb_core.h"
#include "../hal/storage_hal.h"
#include "display_config.h"
#include "hardware_config.h"
#include "peanut_gb.h"

GbCore* GbCore::_instance = nullptr;
TFT_eSPI* GbCore::_currentTft = nullptr;

// Klassische Game Boy Farbpalette (DMG Green)
static const uint16_t gb_palette[4] = {
    0x9E66, // Sehr helles Grün (Hintergrund)
    0x868B, // Helles Olivgrün
    0x3344, // Dunkelgrün
    0x09E1  // Fast Schwarz / Tiefdunkelgrün
};

static uint8_t gb_rom_read_cb(struct gb_s *gb, const uint_fast32_t addr) {
    return GbCore::_instance ? GbCore::_instance->readRomByte(addr) : 0xFF;
}

static uint8_t gb_cart_ram_read_cb(struct gb_s *gb, const uint_fast32_t addr) {
    return GbCore::_instance ? GbCore::_instance->readCartRamByte(addr) : 0xFF;
}

static void gb_cart_ram_write_cb(struct gb_s *gb, const uint_fast32_t addr, const uint8_t val) {
    if (GbCore::_instance) GbCore::_instance->writeCartRamByte(addr, val);
}

static void gb_error_cb(struct gb_s *gb, const enum gb_error_e err, const uint16_t val) {
    Serial.printf("[GB ERROR] Code: %d, Val: 0x%04X\n", (int)err, val);
}

static void gb_lcd_draw_line_cb(struct gb_s *gb, const uint_fast8_t pixels[], const uint_fast8_t line) {
    if (GbCore::_instance) {
        GbCore::_instance->drawLcdLine(pixels, line);
    }
}

GbCore::GbCore()
    : _gbContext(nullptr),
      _romData(nullptr),
      _romSize(0),
      _cartRam(nullptr),
      _cartRamSize(32768),
      _useFilePaging(false),
      _isRunning(false),
      _cachedBank(-1)
{
    _instance = this;
    _gbContext = malloc(sizeof(struct gb_s));
}

GbCore::~GbCore() {
    stop();
    freeBuffers();
    if (_gbContext) {
        free(_gbContext);
        _gbContext = nullptr;
    }
    if (_instance == this) {
        _instance = nullptr;
    }
}

void GbCore::freeBuffers() {
    if (_romData) {
        free(_romData);
        _romData = nullptr;
    }
    if (_cartRam) {
        free(_cartRam);
        _cartRam = nullptr;
    }
    if (_romFile) {
        _romFile.close();
    }
    _romSize = 0;
    _cachedBank = -1;
}

bool GbCore::loadRom(const char* path) {
    freeBuffers();
    _instance = this;

    _romFile = Storage.openFile(path);
    if (!_romFile) {
        Serial.println("[GB] Failed to open ROM file.");
        return false;
    }

    _romSize = _romFile.size();
    if (_romSize < 32768) {
        Serial.println("[GB] File too small for GB ROM.");
        _romFile.close();
        return false;
    }

    // Speicherzuweisung für ROM (Priorität: PSRAM -> interner Heap -> File-Paging)
    if (hasPSRAM()) {
        _romData = (uint8_t*)ps_malloc(_romSize);
        if (_romData) {
            _romFile.read(_romData, _romSize);
            _useFilePaging = false;
            Serial.printf("[GB] Loaded %u bytes into PSRAM.\n", (unsigned int)_romSize);
        }
    }

    if (!_romData) {
        if (_romSize <= 262144 && ESP.getFreeHeap() > (_romSize + 50000)) {
            _romData = (uint8_t*)malloc(_romSize);
            if (_romData) {
                _romFile.read(_romData, _romSize);
                _useFilePaging = false;
                Serial.printf("[GB] Loaded %u bytes into Internal RAM.\n", (unsigned int)_romSize);
            }
        }
    }

    if (!_romData) {
        _useFilePaging = true;
        Serial.println("[GB] Using 16KB bank caching from file.");
    }

    // Cartridge RAM
    if (hasPSRAM()) {
        _cartRam = (uint8_t*)ps_calloc(_cartRamSize, 1);
    } else {
        _cartRam = (uint8_t*)calloc(_cartRamSize, 1);
    }

    struct gb_s* gb = (struct gb_s*)_gbContext;
    enum gb_init_error_e err = gb_init(gb,
                                      gb_rom_read_cb,
                                      gb_cart_ram_read_cb,
                                      gb_cart_ram_write_cb,
                                      gb_error_cb,
                                      nullptr);

    if (err != GB_INIT_NO_ERROR) {
        Serial.printf("[GB] Init error code: %d\n", (int)err);
        freeBuffers();
        return false;
    }

    gb_init_lcd(gb, gb_lcd_draw_line_cb);
    _isRunning = true;
    return true;
}

uint8_t GbCore::readRomByte(uint32_t addr) {
    if (addr >= _romSize) return 0xFF;

    if (_romData) {
        return _romData[addr];
    }

    // File-Paging mit 16KB Cache
    if (_useFilePaging && _romFile) {
        int32_t bank = addr / 16384;
        uint32_t offsetInBank = addr % 16384;

        if (_cachedBank != bank) {
            _cachedBank = bank;
            _romFile.seek(bank * 16384);
            _romFile.read(_bankCache, 16384);
        }
        return _bankCache[offsetInBank];
    }

    return 0xFF;
}

uint8_t GbCore::readCartRamByte(uint32_t addr) {
    if (_cartRam && addr < _cartRamSize) {
        return _cartRam[addr];
    }
    return 0xFF;
}

void GbCore::writeCartRamByte(uint32_t addr, uint8_t val) {
    if (_cartRam && addr < _cartRamSize) {
        _cartRam[addr] = val;
    }
}

void GbCore::drawLcdLine(const uint8_t pixels[], uint8_t line) {
    if (!_currentTft || line >= 144) return;

    uint16_t lineBuf[160];
    for (int x = 0; x < 160; ++x) {
        // 2-bit DMG Farbe (0..3)
        uint8_t shade = pixels[x] & 0x03;
        lineBuf[x] = gb_palette[shade];
    }

    int screenW = _currentTft->width();
    int screenH = _currentTft->height() - STATUS_BAR_HEIGHT;

    int offsetX = (screenW - 160) / 2;
    if (offsetX < 0) offsetX = 0;
    int offsetY = STATUS_BAR_HEIGHT + (screenH - 144) / 2;
    if (offsetY < STATUS_BAR_HEIGHT) offsetY = STATUS_BAR_HEIGHT;

    _currentTft->pushImage(offsetX, offsetY + line, 160, 1, lineBuf);
}

void GbCore::reset() {
    if (_gbContext && _isRunning) {
        gb_reset((struct gb_s*)_gbContext);
    }
}

void GbCore::stepFrame() {
    if (!_gbContext || !_isRunning) return;
    struct gb_s* gb = (struct gb_s*)_gbContext;
    gb_run_frame(gb);
}

void GbCore::handleInput(uint16_t buttons) {
    if (!_gbContext) return;
    struct gb_s* gb = (struct gb_s*)_gbContext;

    // Peanut-GB Tasten sind invertiert: 0 = gedrückt, 1 = losgelassen
    gb->direct.joypad_bits.up     = (buttons & BTN_UP)     ? 0 : 1;
    gb->direct.joypad_bits.down   = (buttons & BTN_DOWN)   ? 0 : 1;
    gb->direct.joypad_bits.left   = (buttons & BTN_LEFT)   ? 0 : 1;
    gb->direct.joypad_bits.right  = (buttons & BTN_RIGHT)  ? 0 : 1;
    gb->direct.joypad_bits.a      = (buttons & BTN_A)      ? 0 : 1;
    gb->direct.joypad_bits.b      = (buttons & BTN_B)      ? 0 : 1;
    gb->direct.joypad_bits.start  = (buttons & BTN_START)  ? 0 : 1;
    gb->direct.joypad_bits.select = (buttons & BTN_SELECT) ? 0 : 1;
}

void GbCore::render(TFT_eSPI& tft) {
    _currentTft = &tft;
}

void GbCore::stop() {
    _isRunning = false;
    _currentTft = nullptr;
}

