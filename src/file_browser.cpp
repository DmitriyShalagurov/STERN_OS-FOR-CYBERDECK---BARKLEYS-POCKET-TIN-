#include "file_browser.h"
#include <TFT_eSPI.h>
#include <SD.h>
#include "terminal_manager.h"
#include "keypad.h"
#include "gui_settings.h"
#include "RAW.h"
#include "status_bar.h"

extern TFT_eSPI tft;
extern bool sdCardOK;

bool fileBrowserActive = false;
std::vector<String> fileList;
int selectedIndex = 0;

#define LEFT_MARGIN 10


bool rawImageJustViewed = false;

void setRawImageJustViewed() {
    rawImageJustViewed = true;
}

void loadFileList() {
  fileList.clear();

  if (!sdCardOK) {
    fileList.push_back("[SD NOT MOUNTED]");
    return;
  }

  File root = SD.open("/");
  if (!root) {
    fileList.push_back("[FAILED TO OPEN ROOT]");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String name = file.name();
      if (!name.startsWith("/")) name = "/" + name;
      fileList.push_back(name);
    }
    file = root.openNextFile();
  }
  root.close();

  if (fileList.size() == 0) {
    fileList.push_back("[NO FILES]");
  }
  selectedIndex = 0;
}

void openFileBrowser() {
  loadFileList();
  fileBrowserActive = true;
 
  tft.fillScreen(TFT_BLACK);
  drawStatusBar();
  drawBorder();
  drawFileBrowser(); 
}

void drawFileBrowser() {

  if (rawImageJustViewed) {
    tft.fillScreen(TFT_BLACK);
    drawStatusBar();
    drawBorder();
    rawImageJustViewed = false;
  }


  tft.startWrite();


  tft.setTextSize(globalTextSize);

  int y = TERMINAL_START_Y + 2;
  int count = fileList.size();
  int startIdx = 0;
  int maxLines = (240 - TERMINAL_START_Y) / LINE_HEIGHT - 1;

  if (count > maxLines) {
    int half = maxLines / 2;
    if (selectedIndex > half) {
      startIdx = selectedIndex - half;
      if (startIdx > count - maxLines) startIdx = count - maxLines;
    }
  }

  for (int i = 0; i < maxLines; i++) {
    int idx = startIdx + i;
    if (idx >= count) break;

    String line = fileList[idx];
    if (line.length() > 30) line = line.substring(0, 28) + "...";

    bool isSelected = (idx == selectedIndex);
    uint16_t textColor = isSelected ? TFT_BLACK : cmdTextColor;
    uint16_t bgColor = isSelected ? cmdTextColor : TFT_BLACK;

    tft.fillRect(LEFT_MARGIN, y, 320 - LEFT_MARGIN * 2, LINE_HEIGHT, bgColor);
    tft.setTextColor(textColor, bgColor);
    tft.setCursor(LEFT_MARGIN + 4, y);
    tft.print(line);
    y += LINE_HEIGHT;
  }


  tft.fillRect(0, 240 - LINE_HEIGHT, 320, LINE_HEIGHT, TFT_BLACK);
  tft.setTextColor(globalTextColor, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(LEFT_MARGIN, 240 - LINE_HEIGHT);
  tft.print("S:X:Move  ENTER:Open  BACK:Delete  ESC:Exit");

  tft.endWrite();


  drawBorder();
}

void closeFileBrowser() {
  fileBrowserActive = false;
  fileList.clear();
  tft.fillRect(0, TERMINAL_START_Y, 320, 240 - TERMINAL_START_Y, TFT_BLACK);
  drawTerminal();
  drawBorder();
}

void confirmAndDeleteFile() {
  if (selectedIndex < 0 || selectedIndex >= (int)fileList.size()) return;
  String filename = fileList[selectedIndex];
  if (filename.startsWith("[")) return;


  tft.fillRect(0, 200, 320, 40, TFT_BLACK);
  tft.setTextColor(globalTextColor, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(5, 210);
  tft.print("Delete file?");
  tft.setCursor(5, 225);
  tft.print("Y:Yes   ESC:No");

  while (true) {
    KeyCode key = Keypad.getKey();
    if (key == KEY_NONE) continue;

    if (key == KEY_ESC) {

      tft.fillScreen(TFT_BLACK);
      drawStatusBar();
      drawBorder();
      drawFileBrowser();
      return;
    }

    char ch = Keypad.getChar();
    if (key == KEY_Y || ch == 'y' || ch == 'Y') {
      if (SD.exists(filename)) {
        SD.remove(filename);
      }
      loadFileList();

      tft.fillScreen(TFT_BLACK);
      drawStatusBar();
      drawBorder();
      drawFileBrowser();
      return;
    }

    delay(20);
  }
}