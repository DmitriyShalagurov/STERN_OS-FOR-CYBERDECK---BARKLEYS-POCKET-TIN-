#include "desktop.h"
#include <TFT_eSPI.h>
#include <WiFi.h>
#include "status_bar.h"
#include "gui_settings.h"
#include "keypad.h"
#include "terminal_manager.h"
#include "file_browser.h"
#include "text_editor.h"
#include "dino_game.h"
#include "game_of_life.h"
#include "../emulators/emulator_manager.h"
#include "../hal/storage_hal.h"
#include "../shell/linux_shell.h"

extern TFT_eSPI tft;
extern KeypadManager Keypad;
extern TextEditor textEditor;
extern bool editorActive;
extern bool editorFromBrowser;
extern String lastKeyMessage;

struct DesktopApp {
    const char* title;
    const char* description;
    uint16_t accentColor;
};

static const DesktopApp DESKTOP_APPS[9] = {
    { "Terminal",  "Linux POSIX Shell & CLI Tools",      0x07E0 }, // Grün
    { "Files",     "File Browser (SD & Flash)",          0xFD20 }, // Orange
    { "Editor",    "Text Editor & Notes (Nano/Txt)",     0x07FF }, // Cyan
    { "Emulators", "Retro Games: NES, GB, CHIP-8",       0xF800 }, // Rot
    { "Dino Game", "Arcade Runner Game",                 0xFFE0 }, // Gelb
    { "Game Life", "Conway's Game of Life Simulation",   0x93FF }, // Violett
    { "WiFi / Net","WLAN Scanner & Network Status",      0x2CFF }, // Hellblau
    { "Settings",  "Colors, Fonts & GUI Options",        0xCE79 }, // Grau
    { "Sys Info",  "CPU, RAM, PSRAM & Storage Status",   0xFD70 }  // Amber
};

// -----------------------------------------------------------------------------
// Icon Renderer (Vektor-Pixel-Icons für jedes Display)
// -----------------------------------------------------------------------------

static void drawIconTerminal(int cx, int cy, uint16_t col) {
    tft.drawRoundRect(cx - 12, cy - 10, 24, 20, 2, col);
    // ">_"
    tft.drawLine(cx - 8, cy - 5, cx - 4, cy - 1, col);
    tft.drawLine(cx - 4, cy - 1, cx - 8, cy + 3, col);
    tft.drawFastHLine(cx - 1, cy + 4, 6, col);
}

static void drawIconFiles(int cx, int cy, uint16_t col) {
    // Ordner-Form mit Reiter
    tft.drawRect(cx - 12, cy - 8, 10, 4, col);
    tft.drawRoundRect(cx - 12, cy - 5, 24, 16, 2, col);
    tft.drawFastHLine(cx - 9, cy + 2, 18, col);
}

static void drawIconEditor(int cx, int cy, uint16_t col) {
    // Dokument mit Linien
    tft.drawRect(cx - 8, cy - 10, 16, 20, col);
    tft.drawFastHLine(cx - 5, cy - 5, 10, col);
    tft.drawFastHLine(cx - 5, cy - 1, 10, col);
    tft.drawFastHLine(cx - 5, cy + 3, 10, col);
    tft.drawFastHLine(cx - 5, cy + 7, 6, col);
}

static void drawIconGamepad(int cx, int cy, uint16_t col) {
    // Controller Body
    tft.drawRoundRect(cx - 12, cy - 7, 24, 15, 3, col);
    // D-Pad '+' links
    tft.drawFastHLine(cx - 8, cy, 5, col);
    tft.drawFastVLine(cx - 6, cy - 2, 5, col);
    // Tasten A / B rechts
    tft.drawPixel(cx + 4, cy + 2, col);
    tft.drawPixel(cx + 7, cy - 1, col);
}

static void drawIconDino(int cx, int cy, uint16_t col) {
    // Pixel-Dino Silhouette
    tft.fillRect(cx - 3, cy - 9, 8, 5, col);  // Kopf
    tft.fillRect(cx - 7, cy - 4, 12, 8, col); // Körper
    tft.fillRect(cx - 6, cy + 4, 2, 4, col);  // Bein 1
    tft.fillRect(cx - 1, cy + 4, 2, 4, col);  // Bein 2
    tft.drawPixel(cx + 2, cy - 8, TFT_BLACK); // Auge
}

static void drawIconLife(int cx, int cy, uint16_t col) {
    // Conway Glider Pattern
    int s = 3;
    tft.fillRect(cx - s, cy - s * 2, s, s, col);
    tft.fillRect(cx, cy - s, s, s, col);
    tft.fillRect(cx - s * 2, cy, s, s, col);
    tft.fillRect(cx - s, cy, s, s, col);
    tft.fillRect(cx, cy, s, s, col);
}

static void drawIconWifi(int cx, int cy, uint16_t col) {
    tft.fillCircle(cx, cy + 6, 2, col); // Basis
    // Bögen
    tft.drawCircle(cx, cy + 6, 6, col);
    tft.drawCircle(cx, cy + 6, 10, col);
    // Untere Hälfte abdecken
    tft.fillRect(cx - 12, cy + 7, 24, 6, TFT_BLACK);
}

static void drawIconSettings(int cx, int cy, uint16_t col) {
    tft.drawCircle(cx, cy, 6, col);
    tft.drawCircle(cx, cy, 2, col);
    // Zähne
    tft.drawFastVLine(cx, cy - 9, 3, col);
    tft.drawFastVLine(cx, cy + 7, 3, col);
    tft.drawFastHLine(cx - 9, cy, 3, col);
    tft.drawFastHLine(cx + 7, cy, 3, col);
}

static void drawIconInfo(int cx, int cy, uint16_t col) {
    tft.drawCircle(cx, cy, 9, col);
    tft.drawPixel(cx, cy - 5, col); // i-Punkt
    tft.drawFastVLine(cx, cy - 2, 6, col);
    tft.drawFastHLine(cx - 2, cy - 2, 3, col);
    tft.drawFastHLine(cx - 2, cy + 4, 5, col);
}

static void drawAppIcon(int index, int cx, int cy, uint16_t color) {
    switch (index) {
        case 0: drawIconTerminal(cx, cy, color); break;
        case 1: drawIconFiles(cx, cy, color); break;
        case 2: drawIconEditor(cx, cy, color); break;
        case 3: drawIconGamepad(cx, cy, color); break;
        case 4: drawIconDino(cx, cy, color); break;
        case 5: drawIconLife(cx, cy, color); break;
        case 6: drawIconWifi(cx, cy, color); break;
        case 7: drawIconSettings(cx, cy, color); break;
        case 8: drawIconInfo(cx, cy, color); break;
    }
}

// -----------------------------------------------------------------------------
// Desktop Grid & UI Rendering
// -----------------------------------------------------------------------------

static void drawTile(int index, int selectedIndex, int x, int y, int w, int h) {
    bool selected = (index == selectedIndex);
    const DesktopApp& app = DESKTOP_APPS[index];

    uint16_t bg = selected ? tft.color565(30, 40, 50) : tft.color565(12, 15, 20);
    uint16_t borderCol = selected ? app.accentColor : tft.color565(60, 60, 60);

    tft.fillRoundRect(x, y, w, h, 4, bg);
    tft.drawRoundRect(x, y, w, h, 4, borderCol);

    if (selected) {
        // Doppelter Rahmen für Fokus-Effekt
        tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 3, borderCol);
    }

    // Icon rendern
    int iconCX = x + w / 2;
    int iconCY = y + h / 2 - 6;
    drawAppIcon(index, iconCX, iconCY, selected ? app.accentColor : TFT_WHITE);

    // Label unter dem Icon
    tft.setTextSize(1);
    tft.setTextColor(selected ? TFT_WHITE : tft.color565(180, 180, 180), bg);
    int textW = tft.textWidth(app.title);
    tft.setCursor(x + (w - textW) / 2, y + h - 12);
    tft.print(app.title);
}

static void renderDesktop(int selectedIndex) {
    int screenW = tft.width();
    int screenH = tft.height();

    int topBarH = 20;
    int bottomBarH = 22;
    int usableH = screenH - topBarH - bottomBarH;

    const int cols = 3;
    const int rows = 3;

    int padX = 10;
    int padY = 4;
    int tileW = (screenW - padX * 2 - (cols - 1) * 6) / cols;
    int tileH = (usableH - padY * 2 - (rows - 1) * 4) / rows;

    tft.startWrite();

    // 1. Hintergrund
    tft.fillRect(0, topBarH, screenW, screenH - topBarH, TFT_BLACK);

    // 2. Kacheln zeichnen
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int idx = r * cols + c;
            int x = padX + c * (tileW + 6);
            int y = topBarH + padY + r * (tileH + 4);
            drawTile(idx, selectedIndex, x, y, tileW, tileH);
        }
    }

    // 3. Untere Statusleiste (App-Beschreibung & Tasten-Hinweis)
    int botY = screenH - bottomBarH;
    tft.fillRect(0, botY, screenW, bottomBarH, tft.color565(15, 20, 25));
    tft.drawFastHLine(0, botY, screenW, tft.color565(50, 70, 90));

    tft.setTextSize(1);
    tft.setTextColor(DESKTOP_APPS[selectedIndex].accentColor, tft.color565(15, 20, 25));
    tft.setCursor(padX, botY + 3);
    tft.print(DESKTOP_APPS[selectedIndex].description);

    tft.setTextColor(tft.color565(140, 140, 140), tft.color565(15, 20, 25));
    tft.setCursor(padX, botY + 13);
    tft.print("S/X/Z/C: Navigate  |  ENTER: Launch  |  ESC: Terminal");

    tft.endWrite();

    drawStatusBar();
    drawBorder();
}

// -----------------------------------------------------------------------------
// App Launcher
// -----------------------------------------------------------------------------

static void launchDesktopApp(int index) {
    switch (index) {
        case 0: // Terminal
            tft.fillScreen(TFT_BLACK);
            drawStatusBar();
            drawTerminal();
            break;

        case 1: // File Manager
            openFileBrowser();
            break;

        case 2: // Text Editor
            textEditor.init();
            editorActive = true;
            editorFromBrowser = false;
            break;

        case 3: // Emulators
            EmuManager.showEmulatorMenu();
            break;

        case 4: // Dino Game
            runDinoGame();
            break;

        case 5: // Game of Life
            runGameOfLife();
            break;

        case 6: // WiFi Manager / Scanner
            tft.fillScreen(TFT_BLACK);
            drawStatusBar();
            terminalClear();
            terminalPrint("=== WiFi Network Manager ===");
            Shell.execute("scan");
            terminalPrint("");
            terminalPrint("To connect: wifi connect <SSID> <PASSWORD>");
            drawTerminal();
            break;

        case 7: // Settings
            guiSettingsMenu();
            break;

        case 8: // System Info
            tft.fillScreen(TFT_BLACK);
            drawStatusBar();
            terminalClear();
            Shell.execute("uname -a");
            Shell.execute("uptime");
            Shell.execute("free -h");
            Shell.execute("df -h");
            drawTerminal();
            break;
    }
}

// -----------------------------------------------------------------------------
// Haupt-Desktop Loop
// -----------------------------------------------------------------------------

void openDesktop() {
    int selectedIndex = 0;
    bool inDesktop = true;
    bool redraw = true;

    while (inDesktop) {
        if (redraw) {
            renderDesktop(selectedIndex);
            redraw = false;
        }

        KeyCode key = Keypad.getKey();

        if (key != KEY_NONE) {
            const int cols = 3;
            const int rows = 3;
            int r = selectedIndex / cols;
            int c = selectedIndex % cols;

            switch (key) {
                case KEY_Z: // Links
                    if (c > 0) { selectedIndex--; redraw = true; }
                    break;
                case KEY_C: // Rechts
                    if (c < cols - 1) { selectedIndex++; redraw = true; }
                    break;
                case KEY_S: // Oben
                    if (r > 0) { selectedIndex -= cols; redraw = true; }
                    break;
                case KEY_X: // Unten
                    if (r < rows - 1) { selectedIndex += cols; redraw = true; }
                    break;
                case KEY_ENTER:
                    launchDesktopApp(selectedIndex);
                    // Nach App-Ende Desktop wiederherstellen (außer bei Terminal)
                    if (selectedIndex == 0) {
                        inDesktop = false;
                    } else {
                        redraw = true;
                    }
                    break;
                case KEY_ESC:
                    // ESC verlässt den Desktop und kehrt zum Terminal zurück
                    inDesktop = false;
                    tft.fillScreen(TFT_BLACK);
                    drawStatusBar();
                    drawTerminal();
                    break;
                default:
                    break;
            }
        }

        delay(15);
    }
}
