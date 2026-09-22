#include "text_editor.h"
#include <TFT_eSPI.h>
#include "hal/storage_hal.h"
#include "keypad.h"
#include "gui_settings.h"

extern TFT_eSPI tft;
extern bool sdCardOK;
extern void terminalPrint(String text);
extern void drawTerminal();

// Подключаем глобальные настройки цвета
extern uint16_t globalTextColor;
extern uint16_t cmdTextColor;

#define TERMINAL_START_Y 15
#define LINE_HEIGHT 10
#define MAX_LINES_ON_SCREEN 19
#define LEFT_MARGIN 10   // <-- новый отступ слева

TextEditor::TextEditor() {
    active = false;
    fileName = "/editor.txt";
    filenameInputMode = false;
    filenameInput = "";
    currentFileName = fileName;
    editMode = false;
    modified = false;
    lastInsertRow = 0;
    lastInsertCol = 0;
    lastInsertValid = false;
}

void TextEditor::init() {
    lines.clear();
    lines.push_back("");
    cursorRow = 0;
    cursorCol = 0;
    scrollOffset = 0;
    active = true;
    filenameInputMode = false;
    currentFileName = generateDefaultFilename();
    editMode = true;
    modified = false;
    lastInsertRow = 0;
    lastInsertCol = 0;
    lastInsertValid = false;
    render();
}

void TextEditor::handleKey(KeyCode code, bool ctrl, bool shift) {
    if (!active) return;

    if (filenameInputMode) {
        handleFilenameInput(code, ctrl, shift);
        return;
    }

    // Обработка Ctrl+комбинаций (только сохранить и переключить режим)
    if (ctrl) {
        switch (code) {
            case KEY_S:   // Ctrl+S – сохранить
                saveCurrentFile();
                return;
            case KEY_E:   // Ctrl+E – переключить в режим редактирования
                if (!editMode) {
                    enterEditMode();
                } else {
                    terminalPrint("Already in edit mode");
                    drawTerminal();
                }
                return;
            default:
                break;
        }
    }

    // ESC – выход
    if (code == KEY_ESC) {
        if (confirmExit()) {
            active = false;
            drawTerminal();
        }
        return;
    }

    // ===== Режим просмотра: перемещение без изменений =====
    if (!editMode) {
        switch (code) {
            case KEY_S:      // вверх
                moveCursor(-1, 0);
                render();
                break;
            case KEY_X:      // вниз
                moveCursor(1, 0);
                render();
                break;
            default:
                break;
        }
        return;
    }

    // ===== Режим редактирования =====
    // Если Fn активна, используем S/X/Z/C для перемещения курсора
    if (Keypad.isFnPressed()) {
        switch (code) {
            case KEY_S:   // вверх
                moveCursor(-1, 0);
                render();
                return;
            case KEY_X:   // вниз
                moveCursor(1, 0);
                render();
                return;
            case KEY_Z:   // влево
                moveCursor(0, -1);
                render();
                return;
            case KEY_C:   // вправо
                moveCursor(0, 1);
                render();
                return;
            default:
                break;
        }
    }

    // Обычный ввод символов (включая S, X, Z, C)
    char ch = Keypad.getChar();
    if (ch != 0) {
        if (Keypad.isMultiTap()) {
            // Удаляем предыдущий символ только если курсор не был перемещён
            if (lastInsertValid && lastInsertRow == cursorRow && lastInsertCol == cursorCol) {
                deleteChar();
            }
        }
        insertChar(ch);
        modified = true;
        // Запоминаем позицию вставленного символа для корректного multi-tap
        lastInsertRow = cursorRow;
        lastInsertCol = cursorCol;
        lastInsertValid = true;
        render();
        return;
    }

    // Специальные клавиши
    switch (code) {
        case KEY_ENTER:
            newLine();
            modified = true;
            lastInsertValid = false;  // сбрасываем multi-tap после новой строки
            render();
            break;
        case KEY_BACK:
            deleteChar();
            modified = true;
            lastInsertValid = false;  // сбрасываем multi-tap после удаления
            render();
            break;
        case KEY_TAB:
            for (int i = 0; i < 4; i++) insertChar(' ');
            modified = true;
            lastInsertValid = false;  // сбрасываем multi-tap после табуляции
            render();
            break;
        default:
            break;
    }
}

// ========== Новые методы ==========

void TextEditor::enterEditMode() {
    editMode = true;
    cursorRow = lines.size() - 1;
    cursorCol = lines[cursorRow].length();
    if (cursorRow >= scrollOffset + MAX_LINES_ON_SCREEN) {
        scrollOffset = cursorRow - MAX_LINES_ON_SCREEN + 1;
    }
    if (cursorRow < scrollOffset) scrollOffset = cursorRow;
    render();
    terminalPrint("Edit mode ON");
    drawTerminal();
}

bool TextEditor::confirmExit() {
    if (!modified) return true;

    tft.fillRect(0, tft.height() - 60, tft.width(), 60, TFT_BLACK);
    tft.setTextColor(globalTextColor, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(5, tft.height() - 50);
    tft.print("Save changes? (Y/N/C)");
    tft.setCursor(5, tft.height() - 35);
    tft.print("Y=Yes   N=No   C=Cancel");

    while (true) {
        KeyCode key = Keypad.getKey();
        if (key == KEY_NONE) continue;

        char ch = Keypad.getChar();

        if (ch == 'y' || ch == 'Y' || key == KEY_Y) {
            saveCurrentFile();
            return true;
        }
        else if (ch == 'n' || ch == 'N' || key == KEY_N) {
            return true;
        }
        else if (ch == 'c' || ch == 'C' || key == KEY_C || key == KEY_ESC) {
            render();
            return false;
        }
        delay(20);
    }
}

void TextEditor::saveCurrentFile() {
    if (!Storage.isSdMounted() && !Storage.isFlashMounted()) {
        terminalPrint("Storage not mounted!");
        drawTerminal();
        return;
    }
    if (currentFileName.length() == 0) {
        startFilenameInput();
        return;
    }
    saveToSD(currentFileName);
}

void TextEditor::startFilenameInput() {
    filenameInputMode = true;
    filenameInput = "";
    tft.fillRect(0, tft.height() - 60, tft.width(), 60, TFT_BLACK);
    tft.setTextColor(globalTextColor, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(5, tft.height() - 50);
    tft.print("Enter filename:");
    tft.setCursor(5, tft.height() - 35);
    tft.print("ENTER=Save ESC=Cancel");
    tft.fillRect(5, 220, 310, 12, TFT_BLACK);
    tft.setCursor(5, 220);
    tft.print(filenameInput);
    tft.drawFastHLine(5 + tft.textWidth(filenameInput), 228, 6, globalTextColor);
}

void TextEditor::handleFilenameInput(KeyCode code, bool ctrl, bool shift) {
    if (code == KEY_ENTER) {
        if (filenameInput.length() == 0) {
            filenameInput = generateDefaultFilename();
        } else {
            if (!filenameInput.endsWith(".txt")) filenameInput += ".txt";
            if (!filenameInput.startsWith("/")) filenameInput = "/" + filenameInput;
        }
        currentFileName = filenameInput;
        filenameInputMode = false;
        saveToSD(currentFileName);
        render();
        return;
    }
    else if (code == KEY_ESC) {
        filenameInputMode = false;
        active = false;
        drawTerminal();
        return;
    }
    else if (code == KEY_BACK) {
        filenameInputMode = false;
        if (SD.exists(currentFileName)) {
            SD.remove(currentFileName);
        }
        active = false;
        drawTerminal();
        return;
    }
    else if (code == KEY_ANGLE) {
        if (filenameInput.length() > 0) {
            filenameInput.remove(filenameInput.length() - 1);
        }
    }
    else {
        char ch = Keypad.getChar();
        if (ch != 0) {
            filenameInput += ch;
        }
    }

    tft.fillRect(5, 220, 310, 12, TFT_BLACK);
    tft.setCursor(5, 220);
    tft.setTextColor(globalTextColor, TFT_BLACK);
    tft.print(filenameInput);
    tft.drawFastHLine(5 + tft.textWidth(filenameInput), 228, 6, globalTextColor);
}

// ========== Остальные методы ==========

void TextEditor::render() {
    clearArea();
    int y = TERMINAL_START_Y + 2;
    int lineCount = lines.size();
    int start = scrollOffset;
    int end = min(start + MAX_LINES_ON_SCREEN, lineCount);
    tft.setTextColor(cmdTextColor, TFT_BLACK);
    tft.setTextSize(1);

    for (int i = start; i < end; i++) {
        tft.setCursor(LEFT_MARGIN, y);   // <-- исправлено
        tft.print(lines[i]);
        y += LINE_HEIGHT;
    }

    renderCursor();
    drawStatusBarEditor();
}

void TextEditor::renderCursor() {
    int rowOnScreen = cursorRow - scrollOffset;
    if (rowOnScreen < 0 || rowOnScreen >= MAX_LINES_ON_SCREEN) return;
    int y = TERMINAL_START_Y + 2 + rowOnScreen * LINE_HEIGHT;
    String currentLine = lines[cursorRow];
    int col = cursorCol;
    if (col > currentLine.length()) col = currentLine.length();
    int x = LEFT_MARGIN + tft.textWidth(currentLine.substring(0, col));   // <-- исправлено
    tft.drawFastHLine(x, y + 8, 6, cmdTextColor);
}

void TextEditor::clearArea() {
    tft.fillRect(0, TERMINAL_START_Y, tft.width(), tft.height() - TERMINAL_START_Y, TFT_BLACK);
}

void TextEditor::drawStatusBarEditor() {
    int screenH = tft.height();
    tft.fillRect(0, screenH - 18, tft.width(), 18, TFT_BLACK);
    tft.setTextColor(globalTextColor, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(LEFT_MARGIN, screenH - 14);   // <-- исправлено
    tft.print("Ln:");
    tft.print(cursorRow + 1);
    tft.print(" Col:");
    tft.print(cursorCol + 1);
    if (editMode) {
        tft.print("  EDIT");
    } else {
        tft.print("  VIEW");
    }
    tft.print("  Ctrl+S:Save Ctrl+E:Edit Fn+S/X/Z/C:Move ESC:Exit");
}

void TextEditor::insertChar(char ch) {
    String &line = lines[cursorRow];
    if (cursorCol > line.length()) cursorCol = line.length();
    line = line.substring(0, cursorCol) + ch + line.substring(cursorCol);
    cursorCol++;
}

void TextEditor::deleteChar() {
    if (cursorRow == 0 && cursorCol == 0) return;
    String &line = lines[cursorRow];
    if (cursorCol > 0) {
        line = line.substring(0, cursorCol - 1) + line.substring(cursorCol);
        cursorCol--;
    } else {
        if (cursorRow > 0) {
            String prevLine = lines[cursorRow - 1];
            int prevLen = prevLine.length();
            lines[cursorRow - 1] = prevLine + line;
            lines.erase(lines.begin() + cursorRow);
            cursorRow--;
            cursorCol = prevLen;
            if (cursorRow < scrollOffset) scrollOffset = cursorRow;
        }
    }
}

void TextEditor::newLine() {
    String &line = lines[cursorRow];
    String rest = line.substring(cursorCol);
    line = line.substring(0, cursorCol);
    lines.insert(lines.begin() + cursorRow + 1, rest);
    cursorRow++;
    cursorCol = 0;
    if (cursorRow >= scrollOffset + MAX_LINES_ON_SCREEN) {
        scrollOffset = cursorRow - MAX_LINES_ON_SCREEN + 1;
    }
}

void TextEditor::moveCursor(int dRow, int dCol) {
    int newRow = cursorRow + dRow;
    int newCol = cursorCol + dCol;
    if (newRow < 0) newRow = 0;
    if (newRow >= (int)lines.size()) newRow = lines.size() - 1;
    String &line = lines[newRow];
    if (newCol < 0) newCol = 0;
    if (newCol > (int)line.length()) newCol = line.length();
    cursorRow = newRow;
    cursorCol = newCol;
    if (cursorRow < scrollOffset) scrollOffset = cursorRow;
    if (cursorRow >= scrollOffset + MAX_LINES_ON_SCREEN) {
        scrollOffset = cursorRow - MAX_LINES_ON_SCREEN + 1;
    }
}

void TextEditor::loadFile(String filename) {
    lines.clear();
    currentFileName = filename;
    editMode = false;
    modified = false;

    if (!Storage.exists(filename)) {
        lines.push_back("");
        editMode = true;
    } else {
        File file = Storage.openFile(filename, FILE_READ);
        if (!file) {
            lines.push_back("Failed to open file");
        } else {
            while (file.available()) {
                String line = file.readStringUntil('\n');
                line.trim();
                lines.push_back(line);
            }
            file.close();
        }
        if (lines.size() == 0) lines.push_back("");
    }

    active = true;
    cursorRow = 0;
    cursorCol = 0;
    scrollOffset = 0;
    render();
}

String TextEditor::generateDefaultFilename() {
    int maxNum = 0;
    std::vector<String> files = Storage.listFiles("/", ".txt");
    for (const auto& name : files) {
        String base = name;
        if (base.startsWith("/")) base = base.substring(1);
        if (base.startsWith("note_")) {
            int dotIndex = base.indexOf('.');
            if (dotIndex > 5) {
                String numStr = base.substring(5, dotIndex);
                int num = numStr.toInt();
                if (num > maxNum) maxNum = num;
            }
        }
    }

    char buf[32];
    sprintf(buf, "/note_%d.txt", maxNum + 1);
    return String(buf);
}

void TextEditor::saveToSD(String filename) {
    if (!Storage.isSdMounted() && !Storage.isFlashMounted()) {
        terminalPrint("No storage mounted!");
        drawTerminal();
        return;
    }
    File file = Storage.openFile(filename, FILE_WRITE);
    if (!file) {
        terminalPrint("Failed to open file for writing");
        drawTerminal();
        return;
    }
    for (size_t i = 0; i < lines.size(); i++) {
        file.print(lines[i]);
        if (i < lines.size() - 1) file.println();
    }
    file.close();
    modified = false;
    terminalPrint("File saved: " + filename);
    drawTerminal();
    int screenH = tft.height();
    tft.fillRect(0, screenH - 20, tft.width(), 10, TFT_BLACK);
    tft.setTextColor(globalTextColor, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN, screenH - 20);
    tft.print("Saved!");
    delay(500);
    render();
}