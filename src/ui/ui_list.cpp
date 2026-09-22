#include "ui_list.h"
#include "status_bar.h"
#include "gui_settings.h"

UiListView::UiListView()
    : _selectedIndex(0),
      _hintText("S:X:Move  ENTER:Select  ESC:Back"),
      _lastRenderedSelected(-1),
      _lastStartIdx(-1)
{
}

void UiListView::setItems(const std::vector<String>& items) {
    _items = items;
    if (_selectedIndex >= (int)_items.size()) {
        _selectedIndex = _items.empty() ? 0 : (int)_items.size() - 1;
    }
    _lastRenderedSelected = -1;
    _lastStartIdx = -1;
}

void UiListView::setHintText(const String& hint) {
    _hintText = hint;
}

void UiListView::setSelectedIndex(int idx) {
    if (idx >= 0 && idx < (int)_items.size()) {
        _selectedIndex = idx;
    }
}

void UiListView::moveUp() {
    if (_selectedIndex > 0) {
        _selectedIndex--;
    }
}

void UiListView::moveDown() {
    if (_selectedIndex < (int)_items.size() - 1) {
        _selectedIndex++;
    }
}

void UiListView::drawRows(TFT_eSPI& tft, uint16_t textColor, uint16_t highlightColor, uint8_t textSize, int startIdx, int maxLines) {
    int screenW = tft.width();
    int screenH = tft.height();
    int y = STATUS_BAR_HEIGHT + 2;
    int count = (int)_items.size();

    tft.setTextSize(textSize);

    // Berechne maximale Zeichenanzahl anhand von Bildschirmbreite
    int maxChars = (screenW - UI_LEFT_MARGIN * 2) / (6 * textSize);
    if (maxChars < 10) maxChars = 10;

    for (int i = 0; i < maxLines; i++) {
        int idx = startIdx + i;
        if (idx >= count) {
            // Leere Zeilen löschen
            tft.fillRect(UI_LEFT_MARGIN, y, screenW - UI_LEFT_MARGIN * 2, UI_LINE_HEIGHT, TFT_BLACK);
            y += UI_LINE_HEIGHT;
            continue;
        }

        String line = _items[idx];
        if ((int)line.length() > maxChars) {
            line = line.substring(0, maxChars - 3) + "...";
        }

        bool isSelected = (idx == _selectedIndex);
        uint16_t fgColor = isSelected ? TFT_BLACK : textColor;
        uint16_t bgColor = isSelected ? highlightColor : TFT_BLACK;

        tft.fillRect(UI_LEFT_MARGIN, y, screenW - UI_LEFT_MARGIN * 2, UI_LINE_HEIGHT, bgColor);
        tft.setTextColor(fgColor, bgColor);
        tft.setCursor(UI_LEFT_MARGIN + 4, y);
        tft.print(line);
        y += UI_LINE_HEIGHT;
    }

    // Untere Tasten-Hinweisleiste
    tft.fillRect(0, screenH - UI_LINE_HEIGHT, screenW, UI_LINE_HEIGHT, TFT_BLACK);
    tft.setTextColor(textColor, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(UI_LEFT_MARGIN, screenH - UI_LINE_HEIGHT);
    tft.print(_hintText);
}

void UiListView::render(TFT_eSPI& tft, uint16_t textColor, uint16_t highlightColor, uint8_t textSize) {
    int screenH = tft.height();
    int maxLines = (screenH - STATUS_BAR_HEIGHT) / UI_LINE_HEIGHT - 1;
    if (maxLines <= 0) maxLines = 1;

    int count = (int)_items.size();
    int startIdx = 0;

    if (count > maxLines) {
        int half = maxLines / 2;
        if (_selectedIndex > half) {
            startIdx = _selectedIndex - half;
            if (startIdx > count - maxLines) {
                startIdx = count - maxLines;
            }
        }
    }

    tft.startWrite();
    drawRows(tft, textColor, highlightColor, textSize, startIdx, maxLines);
    tft.endWrite();

    drawBorder();
    _lastRenderedSelected = _selectedIndex;
    _lastStartIdx = startIdx;
}

int UiListView::runInteractive(TFT_eSPI& tft, KeypadManager& keypad, uint16_t textColor, uint16_t highlightColor, SpecialKeyCallback onSpecialKey) {
    bool running = true;
    bool needsRedraw = true;

    static unsigned long lastRepeat = 0;
    static bool repeatActive = false;
    static KeyCode repeatKey = KEY_NONE;

    while (running) {
        if (needsRedraw) {
            render(tft, textColor, highlightColor);
            needsRedraw = false;
        }

        KeyCode code = keypad.getKey();

        if (code != KEY_NONE) {
            if (code == KEY_S) {
                if (_selectedIndex > 0) {
                    moveUp();
                    needsRedraw = true;
                }
            } else if (code == KEY_X) {
                if (_selectedIndex < (int)_items.size() - 1) {
                    moveDown();
                    needsRedraw = true;
                }
            } else if (code == KEY_ENTER) {
                return _selectedIndex;
            } else if (code == KEY_ESC) {
                return -1;
            } else if (onSpecialKey != nullptr) {
                String current = (_selectedIndex >= 0 && _selectedIndex < (int)_items.size()) ? _items[_selectedIndex] : "";
                if (onSpecialKey(code, _selectedIndex, current)) {
                    needsRedraw = true;
                }
            }
        }

        // Tasten-Autorepeat für flüssiges Scrollen
        bool sPressed = keypad.isPressed(KEY_S);
        bool xPressed = keypad.isPressed(KEY_X);

        if (sPressed || xPressed) {
            KeyCode currentKey = sPressed ? KEY_S : KEY_X;
            if (!repeatActive || currentKey != repeatKey) {
                repeatActive = true;
                repeatKey = currentKey;
                lastRepeat = millis() + 300;
            } else if (millis() > lastRepeat) {
                if (sPressed && _selectedIndex > 0) {
                    moveUp();
                    needsRedraw = true;
                } else if (xPressed && _selectedIndex < (int)_items.size() - 1) {
                    moveDown();
                    needsRedraw = true;
                }
                lastRepeat = millis() + 70;
            }
        } else {
            repeatActive = false;
            repeatKey = KEY_NONE;
        }

        delay(10);
    }

    return -1;
}

