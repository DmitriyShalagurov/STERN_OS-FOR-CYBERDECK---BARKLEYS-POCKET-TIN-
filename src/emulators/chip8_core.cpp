#include "chip8_core.h"
#include "../hal/storage_hal.h"
#include <cstring>

// Standard CHIP-8 Font (0-F)
static const uint8_t chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

// -----------------------------------------------------------------------------
// Vorinstallierte Klassiker im Flash-Speicher (PROGMEM)
// -----------------------------------------------------------------------------
// Pong (1-Spieler Wand-Pong / 2-Spieler Pong)
static const uint8_t ROM_PONG[] PROGMEM = {
    0x6A, 0x02, 0x6B, 0x0C, 0x6C, 0x3F, 0x6D, 0x0C, 0xA2, 0xEA, 0xDA, 0xB6, 0xDC, 0xD6, 0x6E, 0x00,
    0x22, 0xD4, 0x66, 0x03, 0x68, 0x02, 0x60, 0x60, 0xF0, 0x15, 0xF0, 0x07, 0x30, 0x00, 0x12, 0x1A,
    0xC7, 0x17, 0x77, 0x08, 0x69, 0xFF, 0xA2, 0xF0, 0xD6, 0x71, 0xA2, 0xEA, 0xDA, 0xB6, 0xDC, 0xD6,
    0x60, 0x01, 0xE0, 0xA1, 0x7B, 0xFE, 0x60, 0x04, 0xE0, 0xA1, 0x7B, 0x02, 0x60, 0x1F, 0x8B, 0x02,
    0xDA, 0xB6, 0x60, 0x0C, 0xE0, 0xA1, 0x7D, 0xFE, 0x60, 0x0D, 0xE0, 0xA1, 0x7D, 0x02, 0x60, 0x1F,
    0x8D, 0x02, 0xDC, 0xD6, 0xA2, 0xF0, 0xD6, 0x71, 0x86, 0x84, 0x87, 0x94, 0x60, 0x3F, 0x86, 0x02,
    0x61, 0x1F, 0x87, 0x12, 0x3F, 0x01, 0x12, 0x6E, 0x22, 0xDA, 0x70, 0x01, 0x36, 0x3F, 0x12, 0x78,
    0x40, 0x01, 0x12, 0x6E, 0x70, 0xFF, 0x36, 0x00, 0x12, 0x86, 0x22, 0xD4, 0x70, 0x01, 0x37, 0x1F,
    0x12, 0x90, 0x41, 0x01, 0x12, 0x6E, 0x70, 0xFF, 0x37, 0x00, 0x12, 0x9E, 0x61, 0x01, 0x12, 0x6E,
    0x71, 0xFF, 0x41, 0x01, 0x12, 0xAA, 0x70, 0x01, 0x12, 0x6E, 0x60, 0x00, 0x86, 0x02, 0x60, 0x00,
    0x87, 0x02, 0x12, 0x28, 0x12, 0xD4, 0x60, 0x20, 0xF0, 0x18, 0x22, 0xD4, 0x8E, 0x30, 0x22, 0xD4,
    0x66, 0x2E, 0x67, 0x08, 0x68, 0xFE, 0x69, 0xFF, 0x12, 0x24, 0x00, 0xE0, 0x00, 0xEE, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Brix (Breakout / Arkanoid Klon)
static const uint8_t ROM_BRIX[] PROGMEM = {
    0x12, 0x1E, 0x80, 0x40, 0x20, 0x10, 0x20, 0x40, 0x80, 0x10, 0x20, 0x40, 0x80, 0x10, 0x20, 0x40,
    0x20, 0x10, 0x20, 0x40, 0x80, 0x10, 0x20, 0x40, 0x80, 0x10, 0x20, 0x40, 0x20, 0x10, 0x60, 0x00,
    0x61, 0x00, 0x62, 0x00, 0xA2, 0x02, 0xD0, 0x1F, 0x70, 0x08, 0x30, 0x40, 0x12, 0x26, 0x60, 0x00,
    0x71, 0x0F, 0x31, 0x1E, 0x12, 0x26, 0x64, 0x1C, 0x65, 0x1E, 0x62, 0x05, 0xA2, 0x1A, 0xD4, 0x51,
    0x66, 0x00, 0x67, 0x00, 0x68, 0x01, 0x69, 0xFF, 0x6A, 0x04, 0x63, 0x00, 0x60, 0x04, 0xE0, 0xA1,
    0x76, 0xFE, 0x60, 0x06, 0xE0, 0xA1, 0x76, 0x02, 0x60, 0x3C, 0x86, 0x02, 0x60, 0x00, 0x86, 0x04,
    0xDA, 0x41, 0x84, 0x60, 0xDA, 0x41, 0x00, 0xE0, 0x12, 0x00
};

Chip8Core::Chip8Core() {
    reset();
}

Chip8Core::~Chip8Core() {
}

const std::vector<BuiltInRom>& Chip8Core::getBuiltInRoms() {
    static std::vector<BuiltInRom> roms = {
        { "Pong (1-2 Player)", ROM_PONG, sizeof(ROM_PONG) },
        { "Brix (Arkanoid)",   ROM_BRIX, sizeof(ROM_BRIX) }
    };
    return roms;
}

void Chip8Core::initFont() {
    for (int i = 0; i < 80; ++i) {
        memory[i] = chip8_fontset[i];
    }
}

void Chip8Core::reset() {
    pc = 0x200;
    I = 0;
    sp = 0;
    delay_timer = 0;
    sound_timer = 0;
    drawFlag = true;
    isRunning = false;

    memset(memory, 0, sizeof(memory));
    memset(V, 0, sizeof(V));
    memset(stack, 0, sizeof(stack));
    memset(keypad, 0, sizeof(keypad));
    memset(gfx, 0, sizeof(gfx));

    initFont();
}

bool Chip8Core::loadRomFromMemory(const uint8_t* data, size_t size) {
    reset();
    if (size > (4096 - 0x200)) return false;

    for (size_t i = 0; i < size; ++i) {
        memory[0x200 + i] = pgm_read_byte(data + i);
    }
    isRunning = true;
    return true;
}

bool Chip8Core::loadRom(const char* path) {
    reset();
    File file = Storage.openFile(path);
    if (!file) return false;

    size_t size = file.size();
    if (size > (4096 - 0x200)) {
        file.close();
        return false;
    }

    file.read(memory + 0x200, size);
    file.close();
    isRunning = true;
    return true;
}

void Chip8Core::handleInput(uint16_t buttons) {
    memset(keypad, 0, sizeof(keypad));

    // Mapping von VirtualButton auf Standard CHIP-8 Hex-Tasten
    // Pfeiltasten / WASD: Up(2), Down(8), Left(4), Right(6)
    if (buttons & BTN_UP)    keypad[0x2] = 1;
    if (buttons & BTN_DOWN)  keypad[0x8] = 1;
    if (buttons & BTN_LEFT)  keypad[0x4] = 1;
    if (buttons & BTN_RIGHT) keypad[0x6] = 1;

    // A/B Action Tasten: 5, E, F
    if (buttons & BTN_A)     keypad[0x5] = 1;
    if (buttons & BTN_B)     keypad[0x0] = 1;
    if (buttons & BTN_START) keypad[0x1] = 1;
}

void Chip8Core::executeOpcode() {
    uint16_t opcode = (memory[pc] << 8) | memory[pc + 1];
    pc += 2;

    uint16_t nnn = opcode & 0x0FFF;
    uint8_t n   = opcode & 0x000F;
    uint8_t x   = (opcode & 0x0F00) >> 8;
    uint8_t y   = (opcode & 0x00F0) >> 4;
    uint8_t kk  = opcode & 0x00FF;

    switch (opcode & 0xF000) {
        case 0x0000:
            if (opcode == 0x00E0) {
                memset(gfx, 0, sizeof(gfx));
                drawFlag = true;
            } else if (opcode == 0x00EE) {
                if (sp > 0) pc = stack[--sp];
            }
            break;

        case 0x1000: pc = nnn; break;
        case 0x2000:
            if (sp < 16) {
                stack[sp++] = pc;
                pc = nnn;
            }
            break;

        case 0x3000: if (V[x] == kk) pc += 2; break;
        case 0x4000: if (V[x] != kk) pc += 2; break;
        case 0x5000: if (V[x] == V[y]) pc += 2; break;
        case 0x6000: V[x] = kk; break;
        case 0x7000: V[x] += kk; break;

        case 0x8000:
            switch (n) {
                case 0x0: V[x] = V[y]; break;
                case 0x1: V[x] |= V[y]; break;
                case 0x2: V[x] &= V[y]; break;
                case 0x3: V[x] ^= V[y]; break;
                case 0x4: {
                    uint16_t sum = V[x] + V[y];
                    V[0xF] = (sum > 0xFF) ? 1 : 0;
                    V[x] = sum & 0xFF;
                    break;
                }
                case 0x5:
                    V[0xF] = (V[x] >= V[y]) ? 1 : 0;
                    V[x] -= V[y];
                    break;
                case 0x6:
                    V[0xF] = V[x] & 0x1;
                    V[x] >>= 1;
                    break;
                case 0x7:
                    V[0xF] = (V[y] >= V[x]) ? 1 : 0;
                    V[x] = V[y] - V[x];
                    break;
                case 0xE:
                    V[0xF] = (V[x] >> 7) & 0x1;
                    V[x] <<= 1;
                    break;
            }
            break;

        case 0x9000: if (V[x] != V[y]) pc += 2; break;
        case 0xA000: I = nnn; break;
        case 0xB000: pc = nnn + V[0]; break;
        case 0xC000: V[x] = (rand() % 256) & kk; break;

        case 0xD000: { // Draw Sprite
            uint8_t vx = V[x] % 64;
            uint8_t vy = V[y] % 32;
            V[0xF] = 0;

            for (int row = 0; row < n; ++row) {
                if (vy + row >= 32) break;
                uint8_t spriteByte = memory[I + row];

                for (int col = 0; col < 8; ++col) {
                    if (vx + col >= 64) break;
                    if ((spriteByte & (0x80 >> col)) != 0) {
                        int pixelIdx = (vy + row) * 64 + (vx + col);
                        if (gfx[pixelIdx] == 1) V[0xF] = 1;
                        gfx[pixelIdx] ^= 1;
                    }
                }
            }
            drawFlag = true;
            break;
        }

        case 0xE000:
            if (kk == 0x9E) {
                if (keypad[V[x] & 0x0F]) pc += 2;
            } else if (kk == 0xA1) {
                if (!keypad[V[x] & 0x0F]) pc += 2;
            }
            break;

        case 0xF000:
            switch (kk) {
                case 0x07: V[x] = delay_timer; break;
                case 0x0A: {
                    bool keyPressed = false;
                    for (int k = 0; k < 16; ++k) {
                        if (keypad[k]) {
                            V[x] = k;
                            keyPressed = true;
                            break;
                        }
                    }
                    if (!keyPressed) pc -= 2; // Warten auf Taste
                    break;
                }
                case 0x15: delay_timer = V[x]; break;
                case 0x18: sound_timer = V[x]; break;
                case 0x1E: I += V[x]; break;
                case 0x29: I = (V[x] & 0x0F) * 5; break;
                case 0x33:
                    memory[I]     = V[x] / 100;
                    memory[I + 1] = (V[x] / 10) % 10;
                    memory[I + 2] = V[x] % 10;
                    break;
                case 0x55:
                    for (int i = 0; i <= x; ++i) memory[I + i] = V[i];
                    break;
                case 0x65:
                    for (int i = 0; i <= x; ++i) V[i] = memory[I + i];
                    break;
            }
            break;
    }
}

void Chip8Core::stepFrame() {
    if (!isRunning) return;

    // Führe ca. 10 Instruktionen pro 60Hz-Frame aus (~600 Hz CPU-Takt)
    for (int i = 0; i < 11; ++i) {
        executeOpcode();
    }

    if (delay_timer > 0) delay_timer--;
    if (sound_timer > 0) sound_timer--;
}

void Chip8Core::render(TFT_eSPI& tft) {
    if (!drawFlag) return;
    drawFlag = false;

    int screenW = tft.width();
    int screenH = tft.height() - STATUS_BAR_HEIGHT;

    // Dynamischer Skalierungsfaktor angepasst an Bildschirmauflösung
    int scale = screenW / 64;
    if (screenH / 32 < scale) scale = screenH / 32;
    if (scale < 1) scale = 1;

    int offsetX = (screenW - (64 * scale)) / 2;
    int offsetY = STATUS_BAR_HEIGHT + (screenH - (32 * scale)) / 2;

    uint16_t onColor = tft.color565(0, 255, 128); // Retro Cyberpunk-Grün
    uint16_t offColor = TFT_BLACK;

    tft.startWrite();
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 64; ++x) {
            uint16_t color = gfx[y * 64 + x] ? onColor : offColor;
            tft.fillRect(offsetX + x * scale, offsetY + y * scale, scale, scale, color);
        }
    }
    tft.endWrite();
}

void Chip8Core::stop() {
    isRunning = false;
}

