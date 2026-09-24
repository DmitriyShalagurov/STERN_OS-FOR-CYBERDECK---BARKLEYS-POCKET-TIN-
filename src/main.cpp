#include <TFT_eSPI.h>
#include <EEPROM.h>
#include <Wire.h>
#include <vector>
#include "game_of_life.h"
#include <Adafruit_GFX.h>
#include <SD.h>
#include "dino_game.h"
#include "text_editor.h"
#include "keypad.h"
#include "nes_emulator.h"
#include "boot_animation.h"
#include "terminal_manager.h"
#include "status_bar.h"
#include "oled_display.h"
#include "file_browser.h"
#include "gui_settings.h"
#include "RAW.h"
#include "bmp.h"
#include "donat.h"
#include "oscilloscope.h"
#include "mouth.h"
#include "menu.h"

#define SD_CS   27
#define SD_MISO 26
#define SD_MOSI 13
#define SD_SCK  14

TFT_eSPI tft;
TextEditor textEditor;

bool nesActive = false;
bool sdCardOK = false;
bool keypadOK = false;
bool editorActive = false;
bool editorFromBrowser = false;
extern bool gameRunning;

extern void startNESGame(String romPath);

void processCommand(String cmd);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting SternOS with TCA8418");

  EEPROM.begin(128);
  loadGuiSettings();

  globalTextColor = tft.color565(144, 97, 0);
  saveGuiSettings();

  tft.init();
  tft.initDMA();
  tft.setRotation(1);

  showBootAnimation();

  currentHour = 0; currentMinute = 0; currentSecond = 0;
  sprintf(oldTimeStr, "%02d:%02d:%02d", currentHour, currentMinute, currentSecond);
  wifiConnected = false;

  drawStatusBar();
  terminalClear();
  terminalPrint("SternOS v1.0");
  terminalPrint("WiFi: Disabled");
  terminalPrint("Time: local counter");
  terminalPrint("");
  terminalPrint("Type 'help' for commands");
  drawTerminal();

  keypadOK = Keypad.begin();
  if (!keypadOK) {
    terminalPrint("ERROR: Keypad not found!");
    drawTerminal();
  } else {
    terminalPrint("Keypad ready.");
    drawTerminal();
  }

  if (initOLED()) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("SternOS");
    display.display();
    delay(500);
  } else {
    terminalPrint("OLED not detected!");
    drawTerminal();
  }

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SPI, 4000000)) {
    Serial.println("SD Card initialization failed!");
    sdCardOK = false;
    terminalPrint("SD: not found");
    drawTerminal();
  } else {
    sdCardOK = true;
    Serial.println("SD Card ready.");
    terminalPrint("SD: mounted");
    drawTerminal();
  }
}

void loop() {
  if (nesActive) {
    delay(1);
    return;
  }

  if (millis() - lastSecond >= 1000) {
    lastSecond = millis();
    updateTimeDisplay();
    updateTimeOnStatusBar();
    drawStandardOled();
  }

  // --- Отложенная перерисовка терминала (убирает мерцание) ---
  static bool termDirty = false;
  static unsigned long lastTermRedraw = 0;

  KeyCode code = Keypad.getKey();
  if (code != KEY_NONE) {
    if (code == KEY_WIFI_TOGGLE) {
      wifiConnected = !wifiConnected;
      terminalPrint(wifiConnected ? "WiFi: Enabled" : "WiFi: Disabled");
      drawTerminal();
      drawStatusBar();
      if (oledOK) drawStandardOled();
      lastKeyMessage = wifiConnected ? "WiFi ON" : "WiFi OFF";
      return;
    }

    // ===== Кнопка вызова графического меню =====
    if (code == KEY_MENU) {
      static unsigned long lastMenuOpen = 0;
      if (millis() - lastMenuOpen < 300) return;
      lastMenuOpen = millis();

      String sel = runMenu();
      if (sel.length() > 0 && sel != "__no_ram__") {
        processCommand(sel);
      } else {
        drawTerminal();
        drawStatusBar();
      }
      return;
    }
    // ==========================================

    if (editorActive) {
      bool ctrl = Keypad.isCtrlPressed();
      bool shift = Keypad.isShiftPressed();
      textEditor.handleKey(code, ctrl, shift);
      if (!textEditor.isActive()) {
        editorActive = false;
        if (editorFromBrowser) {
          editorFromBrowser = false;
          drawFileBrowser();
        } else {
          drawTerminal();
        }
        drawStatusBar();
        lastKeyMessage = "Editor closed";
      }
      return;
    }

    if (fileBrowserActive) {
      switch (code) {
        case KEY_S:
          if (selectedIndex > 0) { selectedIndex--; drawFileBrowser(); }
          break;
        case KEY_X:
          if (selectedIndex < (int)fileList.size() - 1) { selectedIndex++; drawFileBrowser(); }
          break;
        case KEY_ENTER: {
          if (fileList.size() > 0 && selectedIndex < (int)fileList.size()) {
            String filename = fileList[selectedIndex];
            if (filename.endsWith(".nes")) {
              fileBrowserActive = false;
              closeFileBrowser();
              terminalPrint("Starting NES: " + filename);
              drawTerminal();
              if (oledOK) {
                display.clearDisplay();
                display.setTextColor(SSD1306_WHITE);
                display.setTextSize(1);
                display.setCursor(0, 12);
                display.print("LOADING NES...");
                display.display();
              }
              nesActive = true;
              startNESGame(filename);
              nesActive = false;
              if (oledOK) drawStandardOled();
              terminalPrint("NES stopped.");
              drawTerminal();
              drawStatusBar();
              lastKeyMessage = "NES OFF";
            } else if (filename.endsWith(".txt")) {
              fileBrowserActive = false;
              closeFileBrowser();
              textEditor.loadFile(filename);
              editorActive = true;
              editorFromBrowser = true;
              lastKeyMessage = "Editing: " + filename;
            } else if (filename.endsWith(".raw") || filename.endsWith(".RAW")) {
              showRAWImage(filename.c_str());
              drawFileBrowser();
              drawStatusBar();
            } else if (filename.endsWith(".bmp") || filename.endsWith(".BMP")) {
              showBMPImage(filename.c_str());
              drawFileBrowser();
              drawStatusBar();
            } else {
              terminalPrint("Cannot open file: " + filename);
              drawTerminal();
            }
          }
          break;
        }
        case KEY_BACK:
          confirmAndDeleteFile();
          break;
        case KEY_ESC:
          closeFileBrowser();
          drawTerminal();
          drawStatusBar();
          break;
        case KEY_TAB:
          inputLine += "    ";
          drawTerminal();
          break;
        default:
          break;
      }
      return;
    }

    switch (code) {
      case KEY_ENTER:
        if (inputLine.length() > 11) {
          terminalPrint(inputLine);
          String cmdOnly = inputLine.substring(inputLine.indexOf(":> ") + 3);
          processCommand(cmdOnly);
          inputLine = "barkleys_pc:> ";
          if (!fileBrowserActive && !editorActive) {
            drawTerminal();
          }
        } else {
          terminalPrint("");
          drawTerminal();
        }
        break;

      case KEY_BACK:
        if (inputLine.length() > 13) {
          inputLine.remove(inputLine.length() - 1);
          termDirty = true;          // <-- отложенная перерисовка
        }
        break;

      case KEY_ESC:
        break;

      default: {
        char ch = Keypad.getChar();
        if (ch != 0) {
          inputLine += ch;
          termDirty = true;          // <-- отложенная перерисовка
          lastKeyMessage = String(ch);
        }
        break;
      }
    }
  }

  // --- Одна перерисовка не чаще чем раз в 30 мс ---
  if (termDirty && (millis() - lastTermRedraw >= 30)) {
    lastTermRedraw = millis();
    termDirty = false;
    tft.startWrite();
    drawTerminal();
    tft.endWrite();
  }

  if (fileBrowserActive) {
    static unsigned long lastRepeat = 0;
    static bool repeatActive = false;
    static KeyCode repeatKey = KEY_NONE;

    bool sPressed = Keypad.isPressed(KEY_S);
    bool xPressed = Keypad.isPressed(KEY_X);

    if (sPressed || xPressed) {
      KeyCode currentKey = sPressed ? KEY_S : KEY_X;
      if (!repeatActive || currentKey != repeatKey) {
        repeatActive = true;
        repeatKey = currentKey;
        lastRepeat = millis() + 300;
      } else if (millis() > lastRepeat) {
        if (sPressed && selectedIndex > 0) {
          selectedIndex--;
          drawFileBrowser();
        } else if (xPressed && selectedIndex < (int)fileList.size() - 1) {
          selectedIndex++;
          drawFileBrowser();
        }
        lastRepeat = millis() + 80;
      }
    } else {
      repeatActive = false;
      repeatKey = KEY_NONE;
    }
  }

  delay(20);
}

void processCommand(String cmd) {
  cmd.trim();
  if (cmd == "txt") {
    terminalPrint("Starting Text Editor...");
    drawTerminal();
    textEditor.init();
    editorActive = true;
    editorFromBrowser = false;
    lastKeyMessage = "Editor ON";
  } else if (cmd == "gameoflive") {
    terminalPrint("Starting Game of Life...");
    drawTerminal();
    runGameOfLife();
    terminalPrint("Game finished.");
    drawTerminal();
    drawStatusBar();
    lastKeyMessage = "Game OFF";
  } else if (cmd == "dino") {
    terminalPrint("Starting Dino Game...");
    drawTerminal();
    runDinoGame();
    terminalPrint("Dino Game finished.");
    drawTerminal();
    drawStatusBar();
    lastKeyMessage = "Dino OFF";
  } else if (cmd == "donat" || cmd == "donut") {
    terminalPrint("Starting Donat...");
    drawTerminal();
    runDonat();
    terminalPrint("Donat stopped.");
    drawTerminal();
    drawStatusBar();
    lastKeyMessage = "Donat OFF";
  } else if (cmd == "osc") {
    terminalPrint("Starting Oscilloscope...");
    drawTerminal();
    runOscilloscope();
    terminalPrint("Oscilloscope stopped.");
    drawTerminal();
    drawStatusBar();
    lastKeyMessage = "OSC OFF";
  } else if (cmd == "mouth") {
    tft.fillScreen(TFT_BLACK);
    runMouth();
    terminalPrint("Mouth stopped.");
    drawTerminal();
    drawStatusBar();
    lastKeyMessage = "Mouth OFF";
  } else if (cmd == "menu" || cmd == "main") {
    String sel = runMenu();
    if (sel.length() > 0 && sel != "__no_ram__") {
      processCommand(sel);
    } else {
      drawTerminal();
      drawStatusBar();
    }
  } else if (cmd == "ls") {
    terminalPrint("Opening file browser...");
    drawTerminal();
    openFileBrowser();
  } else if (cmd == "clear") {
    terminalClear();
    drawTerminal();
  } else if (cmd == "help") {
    terminalPrint("        GAMES");
    terminalPrint("  gameoflive - Run Game of Life");
    terminalPrint("  dino       - Run Dino Game");
    terminalPrint("  donat      - Run Donut animation");
    terminalPrint("  nes        - Run NES Emulator");
    terminalPrint("  osc        - Run Oscilloscope (TFT)");
    terminalPrint("  mouth      - Mouth on OLED (TFT off)");
    terminalPrint("");
    terminalPrint("      BASIC COMMANDS");
    terminalPrint("  menu       - Interactive menu");
    terminalPrint("  txt        - Open text editor");
    terminalPrint("  ls         - List files on SD card");
    terminalPrint("  clear      - Clear terminal history");
    terminalPrint("  status     - Show system status");
    terminalPrint("  info       - Show hardware info");
    terminalPrint("  gui        - GUI settings");
    terminalPrint("  help       - Show this help");
    drawTerminal();
  } else if (cmd == "status") {
    terminalPrint("=== System Status ===");
    terminalPrint("  Time: " + String(currentHour) + ":" + String(currentMinute) + ":" + String(currentSecond));
    terminalPrint("  SD Card: " + String(sdCardOK ? "Mounted" : "Not found"));
    terminalPrint("  OLED: " + String(oledOK ? "OK" : "Error"));
    terminalPrint("  Keyboard: " + String(keypadOK ? "Connected" : "Not found"));
    terminalPrint("  WiFi: " + String(wifiConnected ? "Enabled" : "Disabled"));
    terminalPrint("  Bluetooth: Disabled (not implemented)");
    terminalPrint("  Game running: " + String(gameRunning ? "Yes" : "No"));
    terminalPrint("  Editor active: " + String(editorActive ? "Yes" : "No"));
    drawTerminal();
  } else if (cmd == "info") {
    terminalPrint("=== Hardware Info ===");
    terminalPrint("Chip: " + String(ESP.getChipModel()));
    terminalPrint("Cores: " + String(ESP.getChipCores()));
    terminalPrint("CPU Freq: " + String(ESP.getCpuFreqMHz()) + " MHz");
    terminalPrint("Flash size: " + String(ESP.getFlashChipSize()) + " bytes (" + String(ESP.getFlashChipSize() / (1024.0*1024.0), 2) + " MB)");
    terminalPrint("Heap total: " + String(ESP.getHeapSize()) + " bytes (" + String(ESP.getHeapSize()/1024.0, 2) + " KB)");
    terminalPrint("Heap free: " + String(ESP.getFreeHeap()) + " bytes (" + String(ESP.getFreeHeap()/1024.0, 2) + " KB)");
    if (psramFound()) {
        terminalPrint("PSRAM total: " + String(ESP.getPsramSize()) + " bytes (" + String(ESP.getPsramSize()/(1024.0*1024.0), 2) + " MB)");
        terminalPrint("PSRAM free: " + String(ESP.getFreePsram()) + " bytes (" + String(ESP.getFreePsram()/(1024.0*1024.0), 2) + " MB)");
    } else {
        terminalPrint("PSRAM: not found");
    }
    terminalPrint("=== End Info ===");
    drawTerminal();
  } else if (cmd == "gui") {
    guiSettingsMenu();
  } else if (cmd == "nes") {
    terminalPrint("Starting NES Emulator...");
    drawTerminal();
    if (oledOK) {
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      display.setCursor(0, 12);
      display.print("EMULATOR_.NES_TRUE");
      display.display();
    }
    nesActive = true;
    runNES();
    nesActive = false;
    if (oledOK) drawStandardOled();
    terminalPrint("NES Emulator finished.");
    drawTerminal();
    drawStatusBar();
    lastKeyMessage = "NES OFF";
  } else {
    terminalPrint("Unknown command: " + cmd);
    drawTerminal();
  }
}