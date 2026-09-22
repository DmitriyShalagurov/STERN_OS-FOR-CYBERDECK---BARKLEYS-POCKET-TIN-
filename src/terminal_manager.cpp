#include "terminal_manager.h"
#include "gui_settings.h"
#include "shell/linux_shell.h"
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

String outputBuffer[20];
int outputCount = 0;
String inputLine = "root@stern:~# ";
const int TERMINAL_START_Y = 15;
const int MAX_HISTORY = 20;
const int LINE_HEIGHT = 10;

#define LEFT_MARGIN 10

void terminalInitInput() {
  inputLine = Shell.getPrompt();
}

// === ASCII BANNER ===
const char* BANNER[] = {
    "   ___  ___   ___  __ ____   ______  __  ___  _____",
    "  / _ )/ _ | / _ \\/ //_/ /  / __/\\ \\/ / / _ \\/ ___/",
    " / _  / __ |/ , _/ ,< / /__/ _/   \\  / / ___/ /__  ",
    "/____/_/ |_/_/|_/_/|_/____/___/   /_/ /_/   \\___/  "
};
const int BANNER_LINES = sizeof(BANNER) / sizeof(BANNER[0]);

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
  int screenW = tft.width();
  int screenH = tft.height();
  int lineHeight = tft.fontHeight();

  // Banner anzeigen, wenn Bildschirm breit genug ist (>= 280px), sonst kompakte Zeile
  bool showFullBanner = (screenW >= 280);
  int bannerHeight = showFullBanner ? (BANNER_LINES * lineHeight) : (lineHeight + 2);

  // Oeffne Bereich unterhalb der Statusleiste
  tft.fillRect(0, TERMINAL_START_Y, screenW, screenH - TERMINAL_START_Y, TFT_BLACK);

  // ===== BANNER =====
  tft.setTextSize(globalTextSize);
  tft.setTextColor(globalTextColor, TFT_BLACK);
  int y = TERMINAL_START_Y;

  if (showFullBanner) {
    for (int i = 0; i < BANNER_LINES; i++) {
      tft.setCursor(LEFT_MARGIN, y);
      tft.print(BANNER[i]);
      y += lineHeight;
    }
  } else {
    tft.setCursor(LEFT_MARGIN, y);
    tft.print("=== STERN OS CYBERDECK ===");
    y += lineHeight + 2;
  }

  // ===== HISTORY =====
  tft.setTextSize(globalTextSize);
  tft.setTextColor(globalTextColor, globalTextBgColor);

  y = TERMINAL_START_Y + bannerHeight + 2;
  for (int i = 0; i < outputCount; i++) {
    tft.setCursor(LEFT_MARGIN, y);
    tft.print(outputBuffer[i]);
    y += lineHeight;
    if (y >= screenH - lineHeight) break;
  }

  // ===== PROMPT / INPUT =====
  tft.setTextSize(cmdTextSize);
  tft.setTextColor(cmdTextColor, cmdTextBgColor);

  int fontHeight = tft.fontHeight();
  if (y >= screenH - fontHeight) {
    y = screenH - fontHeight;
  }
  tft.setCursor(LEFT_MARGIN, y);
  tft.print(inputLine);

  int cursorX = LEFT_MARGIN + tft.textWidth(inputLine);
  int cursorY = y + fontHeight - 2;
  tft.drawFastHLine(cursorX, cursorY, 6, cmdTextColor);

  drawBorder();
}