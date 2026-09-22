#include "gui_settings.h"
#include <EEPROM.h>
#include "keypad.h"
#include "terminal_manager.h"
#include "status_bar.h"

extern TFT_eSPI tft;

#define EEPROM_GUI_ADDR 10

// Определения глобальных переменных
GuiSettings guiSettings;
uint8_t globalTextSize = 1;
uint16_t globalTextColor = 0x9300;         // #ffc003
uint16_t globalTextBgColor = TFT_BLACK;
uint8_t cmdTextSize = 1;
uint16_t cmdTextColor = 0xFE60;           // #ffc003
uint16_t cmdTextBgColor = TFT_BLACK;

// ===== Загрузка настроек =====
void loadGuiSettings() {
  EEPROM.get(EEPROM_GUI_ADDR, guiSettings);
  if (guiSettings.globalTextSize < 1 || guiSettings.globalTextSize > 4 ||
      guiSettings.cmdTextSize < 1 || guiSettings.cmdTextSize > 4) {
    guiSettings.globalTextSize = 1;
    guiSettings.globalTextColor = 0x9300;
    guiSettings.globalTextBgColor = TFT_BLACK;
    guiSettings.cmdTextSize = 1;
    guiSettings.cmdTextColor = 0xFE60;
    guiSettings.cmdTextBgColor = TFT_BLACK;
    saveGuiSettings();
  }
  globalTextSize = guiSettings.globalTextSize;
  globalTextColor = guiSettings.globalTextColor;
  globalTextBgColor = guiSettings.globalTextBgColor;
  cmdTextSize = guiSettings.cmdTextSize;
  cmdTextColor = guiSettings.cmdTextColor;
  cmdTextBgColor = guiSettings.cmdTextBgColor;
}

void saveGuiSettings() {
  guiSettings.globalTextSize = globalTextSize;
  guiSettings.globalTextColor = globalTextColor;
  guiSettings.globalTextBgColor = globalTextBgColor;
  guiSettings.cmdTextSize = cmdTextSize;
  guiSettings.cmdTextColor = cmdTextColor;
  guiSettings.cmdTextBgColor = cmdTextBgColor;
  EEPROM.put(EEPROM_GUI_ADDR, guiSettings);
  EEPROM.commit();
}

// ===== РИСОВАНИЕ РАМКИ =====
void drawBorder() {
  tft.drawRect(0, 0, tft.width(), tft.height(), tft.color565(144, 97, 0));
}

// ===== Ввод цвета =====
bool inputColor(uint16_t &color) {
  const int MAX_DIGITS = 4;
  char hexStr[MAX_DIGITS + 1] = {0};
  int pos = 0;
  uint16_t oldColor = color;

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("=== ENTER COLOR ===");
  tft.setTextSize(1);
  tft.setCursor(10, 50);
  tft.println("Type hex digits (0-9, A-F)");
  tft.println("4 digits max, then ENTER");
  tft.println("BACK to delete, ESC to cancel");

  int fieldX = 10, fieldY = 120;
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(fieldX, fieldY);
  tft.print("0x");

  while (true) {
    tft.fillRect(fieldX + 30, fieldY, 100, 30, TFT_BLACK);
    tft.setCursor(fieldX + 30, fieldY);
    for (int i = 0; i < pos; i++) {
      tft.print(hexStr[i]);
    }
    tft.drawChar('_', fieldX + 30 + pos * 12, fieldY + 4, TFT_WHITE, TFT_BLACK, 2);

    char ch = Keypad.getChar();
    if (ch != 0) {
      if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f')) {
        if (pos < MAX_DIGITS) {
          if (ch >= 'a' && ch <= 'f') ch = ch - 'a' + 'A';
          hexStr[pos++] = ch;
          hexStr[pos] = 0;
        }
      }
      else if (ch == '\r' || ch == '\n') {
        if (pos == 0) {
          color = oldColor;
          return false;
        }
        uint32_t val = strtoul(hexStr, NULL, 16);
        color = (uint16_t)val;
        return true;
      }
    }

    KeyCode key = Keypad.getKey();
    if (key == KEY_ESC) {
      color = oldColor;
      return false;
    } else if (key == KEY_BACK) {
      if (pos > 0) {
        pos--;
        hexStr[pos] = 0;
      }
    } else if (key == KEY_ENTER) {
      if (pos == 0) {
        color = oldColor;
        return false;
      }
      uint32_t val = strtoul(hexStr, NULL, 16);
      color = (uint16_t)val;
      return true;
    }
    delay(20);
  }
}

// ===== Меню настроек =====
void guiSettingsMenu() {
  bool guiActive = true;
  int menuIndex = 0;
  const char* menuItems[] = {
    "Global text size",
    "Global text color",
    "Command text size",
    "Command text color"
  };
  const int menuCount = 4;

  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 10);
  tft.println("=== GUI SETTINGS ===");
  tft.setTextSize(1);
  tft.setCursor(10, 45);
  tft.println("S/X: change value");
  tft.println("Enter: next / edit color");
  tft.println("Esc: save & exit");
  tft.println();

  int startY = tft.getCursorY();
  int itemHeight = 20;

  while (guiActive) {
    for (int i = 0; i < menuCount; i++) {
      int y = startY + i * itemHeight;
      if (i == menuIndex) {
        tft.fillRect(0, y, tft.width(), itemHeight, TFT_DARKGREY);
        tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
      } else {
        tft.fillRect(0, y, tft.width(), itemHeight, TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
      }
      tft.setTextSize(1);
      tft.setCursor(10, y + 4);
      tft.print(menuItems[i]);
      tft.print(": ");

      switch (i) {
        case 0: tft.print(globalTextSize); break;
        case 1: tft.print("0x"); tft.print(globalTextColor, HEX); break;
        case 2: tft.print(cmdTextSize); break;
        case 3: tft.print("0x"); tft.print(cmdTextColor, HEX); break;
      }
    }

    KeyCode key = Keypad.getKey();

    if (key == KEY_S) {
      if (menuIndex == 0) {
        if (globalTextSize > 1) globalTextSize--;
      } else if (menuIndex == 2) {
        if (cmdTextSize > 1) cmdTextSize--;
      } else if (menuIndex == 1) {
        if (globalTextColor > 0) globalTextColor--;
      } else if (menuIndex == 3) {
        if (cmdTextColor > 0) cmdTextColor--;
      }
    } else if (key == KEY_X) {
      if (menuIndex == 0) {
        if (globalTextSize < 4) globalTextSize++;
      } else if (menuIndex == 2) {
        if (cmdTextSize < 4) cmdTextSize++;
      } else if (menuIndex == 1) {
        if (globalTextColor < 0xFFFF) globalTextColor++;
      } else if (menuIndex == 3) {
        if (cmdTextColor < 0xFFFF) cmdTextColor++;
      }
    } else if (key == KEY_ENTER) {
      if (menuIndex == 1) {
        uint16_t newColor = globalTextColor;
        if (inputColor(newColor)) {
          globalTextColor = newColor;
        }
      } else if (menuIndex == 3) {
        uint16_t newColor = cmdTextColor;
        if (inputColor(newColor)) {
          cmdTextColor = newColor;
        }
      } else {
        menuIndex = (menuIndex + 1) % menuCount;
      }
    } else if (key == KEY_ESC) {
      guiActive = false;
      saveGuiSettings();
      terminalPrint("Settings saved.");
      drawTerminal();
      drawStatusBar();
      drawBorder();   // Рамка после возврата
    }
    delay(20);
  }
}