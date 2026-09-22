#include "status_bar.h"
#include "gui_settings.h"
#include <TFT_eSPI.h>
#include <string.h>

extern TFT_eSPI tft;

unsigned long lastSecond = 0;
int currentHour = 0, currentMinute = 0, currentSecond = 0;
char oldTimeStr[9] = "00:00:00";
bool wifiConnected = false;
int timeStartX = 0, timeWidth = 0, wifiIconX = 0;

void drawStatusBar() {
  int screenW = tft.width();
  tft.fillRect(0, 0, screenW, 20, TFT_BLACK);
  tft.setTextColor(globalTextColor, globalTextBgColor);
  tft.setTextSize(globalTextSize);

  timeWidth = tft.textWidth("00:00:00");
  timeStartX = screenW - timeWidth - 10;
  wifiIconX = timeStartX - 20 - 20;

  sprintf(oldTimeStr, "%02d:%02d:%02d", currentHour, currentMinute, currentSecond);
  tft.setCursor(timeStartX, 4);
  tft.print(oldTimeStr);
  updateWifiIcon();

  drawBorder();
}

void updateTimeOnStatusBar() {
  char newTimeStr[9];
  sprintf(newTimeStr, "%02d:%02d:%02d", currentHour, currentMinute, currentSecond);
  if (strcmp(newTimeStr, oldTimeStr) == 0) return;

  tft.fillRect(timeStartX, 4, timeWidth, tft.fontHeight(), TFT_BLACK);
  tft.setTextColor(globalTextColor, globalTextBgColor);
  tft.setTextSize(globalTextSize);
  tft.setCursor(timeStartX, 4);
  tft.print(newTimeStr);
  strcpy(oldTimeStr, newTimeStr);

  drawBorder();
}

void updateWifiIcon() {
  static int lastBars = -2;
  int iconX = wifiIconX, iconY = 3;
  int bars = wifiConnected ? 3 : -1;
  if (bars == lastBars) return;
  lastBars = bars;

  tft.fillRect(iconX, iconY, 20, 12, TFT_BLACK);

  if (bars == -1) {
    tft.drawLine(iconX, iconY+2, iconX+12, iconY+10, TFT_RED);
    tft.drawLine(iconX+12, iconY+2, iconX, iconY+10, TFT_RED);
  } else {
    tft.fillRect(iconX, iconY+8, 4, 2, TFT_GREEN);
    tft.fillRect(iconX+5, iconY+6, 4, 2, TFT_GREEN);
    tft.fillRect(iconX+10, iconY+4, 4, 2, TFT_GREEN);
  }

  drawBorder();
}

void updateTimeDisplay() {
  currentSecond++;
  if (currentSecond >= 60) {
    currentSecond = 0;
    currentMinute++;
    if (currentMinute >= 60) {
      currentMinute = 0;
      currentHour++;
      if (currentHour >= 24) currentHour = 0;
    }
  }
}