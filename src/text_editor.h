#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

#include <Arduino.h>
#include <vector>
#include <FS.h>
#include "keypad.h"

class TextEditor {
public:
    TextEditor();
    void init();
    void handleKey(KeyCode code, bool ctrl, bool shift);
    void render();
    void renderCursor();
    void clearArea();
    void drawStatusBarEditor();
    void insertChar(char ch);
    void deleteChar();
    void newLine();
    void moveCursor(int dRow, int dCol);
    void loadFile(String filename);
    void saveToSD(String filename);
    bool isActive() const { return active; }

private:
    bool active;
    String fileName;
    std::vector<String> lines;
    int cursorRow;
    int cursorCol;
    int scrollOffset;

    bool editMode = false;
    bool modified = false;
    bool filenameInputMode = false;
    String filenameInput = "";
    String currentFileName = "";

    // Для корректного multi-tap при перемещении каретки
    int lastInsertRow = 0;
    int lastInsertCol = 0;
    bool lastInsertValid = false;

    void startFilenameInput();
    void handleFilenameInput(KeyCode code, bool ctrl, bool shift);
    void enterEditMode();
    bool confirmExit();
    void saveCurrentFile();
    String generateDefaultFilename();
};

#endif