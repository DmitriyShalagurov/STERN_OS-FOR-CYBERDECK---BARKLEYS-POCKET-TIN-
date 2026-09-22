#include "file_browser.h"
#include <TFT_eSPI.h>
#include "hal/storage_hal.h"
#include "ui/ui_list.h"
#include "terminal_manager.h"
#include "keypad.h"
#include "gui_settings.h"
#include "RAW.h"
#include "status_bar.h"

extern TFT_eSPI tft;
extern KeypadManager Keypad;

bool fileBrowserActive = false;
std::vector<String> fileList;
int selectedIndex = 0;
bool rawImageJustViewed = false;

static UiListView fileListView;

void setRawImageJustViewed() {
    rawImageJustViewed = true;
}

void loadFileList() {
    fileList = Storage.listFiles("/");
    if (fileList.empty()) {
        fileList.push_back("[NO FILES]");
    }
    selectedIndex = 0;
    fileListView.setItems(fileList);
    fileListView.setHintText("S:X:Move  ENTER:Open  BACK:Del  ESC:Exit");
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

    fileListView.setSelectedIndex(selectedIndex);
    fileListView.render(tft, cmdTextColor, globalTextColor, globalTextSize);
}

void closeFileBrowser() {
    fileBrowserActive = false;
    fileList.clear();
    tft.fillScreen(TFT_BLACK);
    drawStatusBar();
    drawTerminal();
    drawBorder();
}

void confirmAndDeleteFile() {
    if (selectedIndex < 0 || selectedIndex >= (int)fileList.size()) return;
    String filename = fileList[selectedIndex];
    if (filename.startsWith("[")) return;

    int screenH = tft.height();
    tft.fillRect(0, screenH - 40, tft.width(), 40, TFT_BLACK);
    tft.setTextColor(globalTextColor, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(5, screenH - 30);
    tft.print("Delete: " + filename + "?");
    tft.setCursor(5, screenH - 15);
    tft.print("Y: Yes   ESC: Cancel");

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
            Storage.remove(filename);
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