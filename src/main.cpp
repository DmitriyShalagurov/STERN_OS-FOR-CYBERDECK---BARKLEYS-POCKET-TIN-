#include <TFT_eSPI.h>
#include <EEPROM.h>
#include <Wire.h>
#include <vector>
#include <Adafruit_GFX.h>

#include "hardware_config.h"
#include "display_config.h"
#include "hal/storage_hal.h"
#include "ui/ui_list.h"
#include "ui/desktop.h"
#include "emulators/emulator_manager.h"
#include "shell/linux_shell.h"

#include "game_of_life.h"
#include "dino_game.h"
#include "text_editor.h"
#include "keypad.h"
#include "boot_animation.h"
#include "terminal_manager.h"
#include "status_bar.h"
#include "oled_display.h"
#include "file_browser.h"
#include "gui_settings.h"
#include "RAW.h"
#include "bmp.h"

// ========== Глобальные объекты ==========
TFT_eSPI tft;
TextEditor textEditor;

// ========== Глобальные флаги ==========
bool sdCardOK = false;
bool keypadOK = false;
bool editorActive = false;
bool editorFromBrowser = false;
bool gameRunning = false;

// Прототип
void processCommand(String cmd);

// -------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("Starting SternOS on " BOARD_NAME);

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

  Shell.begin();
  terminalInitInput();

  drawStatusBar();
  terminalClear();
  terminalPrint("SternOS v2.0 Modular Linux Cyberdeck");
  terminalPrint("Hardware: " BOARD_NAME);
  terminalPrint("Type 'help' or 'man' for POSIX / Linux tools");
  terminalPrint("Type 'emu' for Games | 'startx' or ESC for Desktop");
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
    display.println("SternOS v2.0");
    display.display();
    delay(400);
  } else {
    terminalPrint("OLED: not detected");
    drawTerminal();
  }

  // Storage HAL initialisieren (SD-Karte und internes SPIFFS/LittleFS)
  Storage.begin();
  sdCardOK = Storage.isSdMounted();
  if (sdCardOK) {
    Serial.println("SD Card ready.");
    terminalPrint("Storage: SD Card mounted");
  } else if (Storage.isFlashMounted()) {
    terminalPrint("Storage: Internal Flash ready (No SD)");
  } else {
  drawTerminal();
  delay(300);

  // Desktop-Umgebung automatisch beim Systemstart laden
  openDesktop();
}

// -------------------------------------------------------------------
void loop() {
  if (gameRunning) {
    delay(1);
    return;
  }

  if (millis() - lastSecond >= 1000) {
    lastSecond = millis();
    updateTimeDisplay();
    updateTimeOnStatusBar();
    drawStandardOled();
  }

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
            String lower = filename;
            lower.toLowerCase();

            // Automatische Erkennung und Start für alle Emulatoren (.nes, .gb, .gbc, .ch8)
            if (lower.endsWith(".nes") || lower.endsWith(".gb") || lower.endsWith(".gbc") || lower.endsWith(".ch8")) {
              fileBrowserActive = false;
              closeFileBrowser();
              EmuManager.launchRom(filename);
              lastKeyMessage = "Game Exited";
            } else if (filename.endsWith(".txt")) {
              fileBrowserActive = false;
              closeFileBrowser();
              textEditor.loadFile(filename);
              editorActive = true;
              editorFromBrowser = true;
              lastKeyMessage = "Editing: " + filename;
            } else if (lower.endsWith(".raw")) {
              showRAWImage(filename.c_str());
              drawFileBrowser();
              drawStatusBar();
            } else if (lower.endsWith(".bmp")) {
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

    // ---- Режим терминала ----
    switch (code) {
      case KEY_ENTER: {
        String prompt = Shell.getPrompt();
        if (inputLine.length() > prompt.length()) {
          terminalPrint(inputLine);
          String cmdOnly = inputLine.substring(prompt.length());
          processCommand(cmdOnly);
          inputLine = Shell.getPrompt();
          if (!fileBrowserActive && !editorActive) {
            drawTerminal();
          }
        } else {
          terminalPrint(inputLine);
          inputLine = Shell.getPrompt();
          drawTerminal();
        }
        break;
      }
      case KEY_BACK: {
        String prompt = Shell.getPrompt();
        if (inputLine.length() > prompt.length()) {
          inputLine.remove(inputLine.length() - 1);
          drawTerminal();
        }
        break;
      }
      case KEY_ESC:
        openDesktop();
        break;
      default: {
        char ch = Keypad.getChar();
        if (ch != 0) {
          inputLine += ch;
          tft.startWrite();
          drawTerminal();
          tft.endWrite();
          lastKeyMessage = String(ch);
        }
        break;
      }
    }
  }

  // Автоповтор для файлового браузера
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

// -------------------------------------------------------------------
void processCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  // 1. Zuerst native Linux / POSIX Shell-Befehle prüfen
  // (cd, pwd, ls, cat, touch, mkdir, rm, cp, mv, grep, head, df, echo, uname, uptime, free, ps, ifconfig, scan, wifi, ping, wget, curl, sh)
  if (Shell.execute(cmd)) {
    return;
  }

  // 2. STERN OS Anwendungen & Emulatoren
  if (cmd == "exit" || cmd == "desktop" || cmd == "startx" || cmd == "gui") {
    openDesktop();
    return;
  } else if (cmd == "txt" || cmd == "editor" || cmd == "nano") {
    terminalPrint("Starting Text Editor...");
    drawTerminal();
    textEditor.init();
    editorActive = true;
    editorFromBrowser = false;
    lastKeyMessage = "Editor ON";
  } else if (cmd == "emu" || cmd == "games") {
    EmuManager.showEmulatorMenu();
  } else if (cmd == "nes") {
    EmuManager.showConsoleMenu(&EmuManager.getNesCore());
  } else if (cmd == "gb") {
    EmuManager.showConsoleMenu(&EmuManager.getGbCore());
  } else if (cmd == "chip8") {
    EmuManager.showConsoleMenu(&EmuManager.getChip8Core());
  } else if (cmd == "browser" || cmd == "files") {
    terminalPrint("Opening GUI file browser...");
    drawTerminal();
    openFileBrowser();
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
  } else if (cmd == "clear") {
    terminalClear();
    drawTerminal();
  } else if (cmd == "help") {
    terminalPrint("=== STERN OS LINUX CYBERDECK ===");
    terminalPrint("POSIX / LINUX CLI:");
    terminalPrint("  cd <dir>, pwd, ls [-l], cat <file>, touch <file>");
    terminalPrint("  mkdir <dir>, rm <file>, cp <src> <dst>, mv <src> <dst>");
    terminalPrint("  grep <pat> <file>, head <file>, df -h, echo txt > file");
    terminalPrint("SYSTEM:");
    terminalPrint("  uname -a, uptime, free, ps, whoami, date, reboot");
    terminalPrint("NETWORK:");
    terminalPrint("  ifconfig, scan, wifi connect <ssid> <pass>, ping <host>");
    terminalPrint("  wget <url> [file], curl <url>");
    terminalPrint("EMULATORS & APPS:");
    terminalPrint("  emu, nes, gb, chip8, dino, gameoflive, txt, browser, gui");
    drawTerminal();
  } else if (cmd == "status") {
    terminalPrint("=== System Status ===");
    terminalPrint("  Time: " + String(currentHour) + ":" + String(currentMinute) + ":" + String(currentSecond));
    terminalPrint("  SD Card: " + String(Storage.isSdMounted() ? "Mounted" : "Not found"));
    terminalPrint("  Flash FS: " + String(Storage.isFlashMounted() ? "Mounted" : "Not found"));
    terminalPrint("  OLED: " + String(oledOK ? "OK" : "Error"));
    terminalPrint("  Keyboard: " + String(keypadOK ? "Connected" : "Not found"));
    terminalPrint("  WiFi: " + String(wifiConnected ? "Enabled" : "Disabled"));
    drawTerminal();
  } else if (cmd == "info") {
    terminalPrint("=== Hardware Info ===");
    terminalPrint("Platform: " BOARD_NAME);
    terminalPrint("Display: " DISPLAY_DRIVER_NAME);
    terminalPrint("Res: " + String(tft.width()) + "x" + String(tft.height()));
    terminalPrint("Chip: " + String(ESP.getChipModel()) + " (" + String(ESP.getChipCores()) + " Cores)");
    terminalPrint("CPU Freq: " + String(ESP.getCpuFreqMHz()) + " MHz");
    terminalPrint("Flash size: " + String(ESP.getFlashChipSize() / (1024.0*1024.0), 1) + " MB");
    terminalPrint("Free Heap: " + String(ESP.getFreeHeap() / 1024.0, 1) + " KB");
    if (hasPSRAM()) {
        terminalPrint("PSRAM: " + String(ESP.getPsramSize() / (1024.0*1024.0), 1) + " MB (Free: " + String(ESP.getFreePsram() / (1024.0*1024.0), 1) + " MB)");
    } else {
        terminalPrint("PSRAM: Not installed");
    }
    uint32_t totKB = 0, usedKB = 0;
    Storage.getStorageInfo(totKB, usedKB);
    if (totKB > 0) {
        terminalPrint("Storage: " + String(usedKB / 1024.0, 1) + "/" + String(totKB / 1024.0, 1) + " MB used");
    }
    terminalPrint("=== End Info ===");
    drawTerminal();
  } else if (cmd == "gui") {
    guiSettingsMenu();
  } else {
    terminalPrint("Unknown command: " + cmd);
    drawTerminal();
  }
}