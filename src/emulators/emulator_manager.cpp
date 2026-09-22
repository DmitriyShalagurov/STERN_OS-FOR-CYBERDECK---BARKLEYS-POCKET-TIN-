#include "emulator_manager.h"
#include <esp_timer.h>
#include "../ui/ui_list.h"
#include "../hal/storage_hal.h"
#include "oled_display.h"
#include "status_bar.h"
#include "terminal_manager.h"
#include "gui_settings.h"

extern TFT_eSPI tft;
extern KeypadManager Keypad;
extern bool gameRunning;
extern bool oledOK;
extern Adafruit_SSD1306 display;

EmulatorManager EmuManager;

EmulatorManager::EmulatorManager() {
    _emulators.push_back(&_nesCore);
    _emulators.push_back(&_gbCore);
    _emulators.push_back(&_chip8Core);
}

EmulatorManager::~EmulatorManager() {
}

uint16_t EmulatorManager::pollInput() {
    uint16_t buttons = BTN_NONE;

    // Standardisierte Tastenbelegung für das Barkleys-Cyberdeck:
    if (Keypad.isPressed(KEY_SEMICOLON))      buttons |= BTN_A;          // A (Aktion / Springen)
    if (Keypad.isPressed(KEY_QUESTION_SLASH)) buttons |= BTN_B;          // B (Schießen / Sekundär)
    if (Keypad.isPressed(KEY_ENTER))          buttons |= BTN_SELECT;     // SELECT
    if (Keypad.isPressed(KEY_SPACE))          buttons |= BTN_START;      // START
    if (Keypad.isPressed(KEY_S))              buttons |= BTN_UP;         // OBEN
    if (Keypad.isPressed(KEY_X))              buttons |= BTN_DOWN;       // UNTEN
    if (Keypad.isPressed(KEY_Z))              buttons |= BTN_LEFT;       // LINKS
    if (Keypad.isPressed(KEY_C))              buttons |= BTN_RIGHT;      // RECHTS

    return buttons;
}

void EmulatorManager::runEmulator(IEmulator* emu) {
    if (!emu) return;

    gameRunning = true;
    tft.fillScreen(TFT_BLACK);
    emu->render(tft);

    // Status-OLED aktualisieren
    if (oledOK) {
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor(0, 8);
        display.print("GAME RUNNING:");
        display.setCursor(0, 20);
        display.print(emu->getName());
        display.display();
    }

    const uint32_t frameTimeUs = emu->getFrameTimeUs();
    uint64_t nextFrame = esp_timer_get_time();
    bool running = true;

    while (running) {
        // Tasten abfragen
        uint16_t buttons = pollInput();

        // ESC zum Beenden prüfen
        KeyCode code = Keypad.getKey();
        if (code == KEY_ESC) {
            running = false;
            break;
        }

        emu->handleInput(buttons);
        emu->stepFrame();
        emu->render(tft);

        // Frame-Rate Pacing
        uint64_t now = esp_timer_get_time();
        int64_t diff = (int64_t)nextFrame - (int64_t)now;
        if (diff > 0 && diff < 100000) {
            ets_delay_us(diff);
        }
        nextFrame += frameTimeUs;
    }

    emu->stop();
    gameRunning = false;

    // Nach Spielende Bildschirm wiederherstellen
    tft.fillScreen(TFT_BLACK);
    drawStatusBar();
    terminalPrint(String(emu->getName()) + " closed.");
    drawTerminal();
    if (oledOK) drawStandardOled();
}

bool EmulatorManager::launchRom(const String& path) {
    String lower = path;
    lower.toLowerCase();

    IEmulator* target = nullptr;
    if (lower.endsWith(".nes")) {
        target = &_nesCore;
    } else if (lower.endsWith(".gb") || lower.endsWith(".gbc")) {
        target = &_gbCore;
    } else if (lower.endsWith(".ch8")) {
        target = &_chip8Core;
    }

    if (!target) {
        terminalPrint("No emulator for: " + path);
        drawTerminal();
        return false;
    }

    terminalPrint("Loading: " + path);
    drawTerminal();

    if (!target->loadRom(path.c_str())) {
        terminalPrint("Failed to load ROM!");
        drawTerminal();
        return false;
    }

    runEmulator(target);
    return true;
}

bool EmulatorManager::launchBuiltInRom(int index) {
    const auto& roms = Chip8Core::getBuiltInRoms();
    if (index < 0 || index >= (int)roms.size()) return false;

    terminalPrint("Starting built-in: " + String(roms[index].name));
    drawTerminal();

    if (!_chip8Core.loadRomFromMemory(roms[index].data, roms[index].size)) {
        terminalPrint("Error starting built-in ROM!");
        drawTerminal();
        return false;
    }

    runEmulator(&_chip8Core);
    return true;
}

void EmulatorManager::showConsoleMenu(IEmulator* emu) {
    if (!emu) return;

    std::vector<String> menuItems;
    bool isChip8 = (emu == &_chip8Core);

    // Bei CHIP-8: Eingebaute ROMs zuerst auflisten
    if (isChip8) {
        const auto& builtins = Chip8Core::getBuiltInRoms();
        for (const auto& r : builtins) {
            menuItems.push_back(String("[ROM] ") + r.name);
        }
    }

    // ROMs von SD-Karte oder LittleFS laden
    std::vector<String> files = Storage.listFiles("/", emu->getExtension());
    for (const auto& f : files) {
        menuItems.push_back(f);
    }

    if (menuItems.empty()) {
        terminalPrint("No games found for " + String(emu->getName()));
        drawTerminal();
        return;
    }

    tft.fillScreen(TFT_BLACK);
    drawStatusBar();

    UiListView listView;
    listView.setItems(menuItems);
    listView.setHintText("S:X:Move  ENTER:Run  ESC:Back");

    int selected = listView.runInteractive(tft, Keypad, cmdTextColor, globalTextColor);
    if (selected >= 0) {
        if (isChip8 && selected < (int)Chip8Core::getBuiltInRoms().size()) {
            launchBuiltInRom(selected);
        } else {
            launchRom(menuItems[selected]);
        }
    } else {
        tft.fillScreen(TFT_BLACK);
        drawStatusBar();
        drawTerminal();
    }
}

void EmulatorManager::showEmulatorMenu() {
    std::vector<String> menu = {
        "1. NES (Nintendo)",
        "2. Game Boy (DMG)",
        "3. CHIP-8 (Built-in + SD)"
    };

    tft.fillScreen(TFT_BLACK);
    drawStatusBar();

    UiListView listView;
    listView.setItems(menu);
    listView.setHintText("S:X:Select Console  ENTER:Open  ESC:Back");

    int selected = listView.runInteractive(tft, Keypad, cmdTextColor, globalTextColor);
    if (selected == 0) {
        showConsoleMenu(&_nesCore);
    } else if (selected == 1) {
        showConsoleMenu(&_gbCore);
    } else if (selected == 2) {
        showConsoleMenu(&_chip8Core);
    } else {
        tft.fillScreen(TFT_BLACK);
        drawStatusBar();
        drawTerminal();
    }
}

