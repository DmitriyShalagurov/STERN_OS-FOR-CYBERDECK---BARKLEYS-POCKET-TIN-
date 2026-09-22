#include "nes_core.h"
#include "core/Bus.h"
#include "core/Cartridge.h"
#include "../hal/storage_hal.h"

NesCore::NesCore()
    : _cart(nullptr), _bus(nullptr), _isRunning(false), _controllerState(0)
{
}

NesCore::~NesCore() {
    stop();
}

void NesCore::stop() {
    _isRunning = false;
    if (_bus) {
        delete _bus;
        _bus = nullptr;
    }
    if (_cart) {
        delete _cart;
        _cart = nullptr;
    }
}

bool NesCore::loadRom(const char* path) {
    stop();

    File romFile = Storage.openFile(path, FILE_READ);
    if (!romFile || romFile.size() < 16) {
        if (romFile) romFile.close();
        return false;
    }

    uint8_t header[16];
    romFile.read(header, 16);
    romFile.close();

    // NES Header Prüfen: "NES\x1A"
    if (header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 0x1A) {
        return false;
    }

    _cart = new Cartridge(path);
    if (!_cart) return false;

    _bus = new Bus();
    if (!_bus) {
        delete _cart;
        _cart = nullptr;
        return false;
    }

    _bus->insertCartridge(_cart);
    _isRunning = true;
    return true;
}

void NesCore::render(TFT_eSPI& tft) {
    if (_bus) {
        _bus->connectScreen(&tft);
        tft.setSwapBytes(true);
    }
}

void NesCore::reset() {
    if (_bus) {
        _bus->reset();
    }
}

void NesCore::stepFrame() {
    if (_bus && _isRunning) {
        _bus->controller = _controllerState;
        _bus->clock();
    }
}

void NesCore::handleInput(uint16_t buttons) {
    uint8_t nesButtons = 0;

    if (buttons & BTN_A)      nesButtons |= 0x01;
    if (buttons & BTN_B)      nesButtons |= 0x02;
    if (buttons & BTN_SELECT) nesButtons |= 0x04;
    if (buttons & BTN_START)  nesButtons |= 0x08;
    if (buttons & BTN_UP)     nesButtons |= 0x10;
    if (buttons & BTN_DOWN)   nesButtons |= 0x20;
    if (buttons & BTN_LEFT)   nesButtons |= 0x40;
    if (buttons & BTN_RIGHT)  nesButtons |= 0x80;

    _controllerState = nesButtons;
}

