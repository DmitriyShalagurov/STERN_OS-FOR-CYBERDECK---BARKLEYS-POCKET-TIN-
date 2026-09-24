#include "terminal_manager.h"
#include "gui_settings.h"
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

String outputBuffer[20];
int outputCount = 0;
String inputLine = "barkleys_pc:> ";
const int TERMINAL_START_Y = 15;
const int MAX_HISTORY = 20;
const int LINE_HEIGHT = 10;

#define LEFT_MARGIN 10

// === БАННЕР ===
const char* BANNER[] = {
    "   ___  ___   ___  __ ____   ______  __  ___  _____",
    "  / _ )/ _ | / _ \\/ //_/ /  / __/\\ \\/ / / _ \\/ ___/",
    " / _  / __ |/ , _/ ,< / /__/ _/   \\  / / ___/ /__  ",
    "/____/_/ |_/_/|_/_/|_/____/___/   /_/ /_/   \\___/  "
};
const int BANNER_LINES = sizeof(BANNER) / sizeof(BANNER[0]); // = 4

void terminalPrint(String text) {
  if (outputCount < MAX_HISTORY) {
    outputBuffer[outputCount++] = text;
  } else {
    for (int i = 0; i < MAX_HISTORY - 1; i++) {
      outputBuffer[i] = outputBuffer[i + 1];
    }
    outputBuffer[MAX_HISTORY - 1] = text;
  }
}

void terminalClear() {
  outputCount = 0;
}

void drawTerminal() {
  int lineHeight = tft.fontHeight();
  int bannerHeight = BANNER_LINES * lineHeight;

  // ===== ГЛАВНОЕ ИСПРАВЛЕНИЕ =====
  // Очищаем ВСЮ область терминала от TERMINAL_START_Y и ниже,
  // включая область баннера. Иначе после меню/игр на месте баннера
  // остаются артефакты (куски рамки, полосы курсора меню и т.п.).
  tft.fillRect(0, TERMINAL_START_Y, 320, 240 - TERMINAL_START_Y, TFT_BLACK);

  // ===== БАННЕР – ПРИГЛУШЁННЫЙ ЖЁЛТЫЙ (globalTextColor) =====
  tft.setTextSize(globalTextSize);
  tft.setTextColor(globalTextColor, TFT_BLACK);
  int y = TERMINAL_START_Y;
  for (int i = 0; i < BANNER_LINES; i++) {
    tft.setCursor(LEFT_MARGIN, y);
    tft.print(BANNER[i]);
    y += lineHeight;
  }

  // ===== История команд =====
  tft.setTextSize(globalTextSize);
  tft.setTextColor(globalTextColor, globalTextBgColor);

  y = TERMINAL_START_Y + bannerHeight + 2;
  for (int i = 0; i < outputCount; i++) {
    tft.setCursor(LEFT_MARGIN, y);
    tft.print(outputBuffer[i]);
    y += lineHeight;
    if (y >= 240 - lineHeight) break;
  }

  // ===== Строка ввода (яркий жёлтый) =====
  tft.setTextSize(cmdTextSize);
  tft.setTextColor(cmdTextColor, cmdTextBgColor);

  int fontHeight = tft.fontHeight();
  if (y >= 240 - fontHeight) {
    y = 240 - fontHeight;
  }
  tft.setCursor(LEFT_MARGIN, y);
  tft.print(inputLine);

  int cursorX = LEFT_MARGIN + tft.textWidth(inputLine);
  int cursorY = y + fontHeight - 2;
  tft.drawFastHLine(cursorX, cursorY, 6, cmdTextColor);

  drawBorder();
}