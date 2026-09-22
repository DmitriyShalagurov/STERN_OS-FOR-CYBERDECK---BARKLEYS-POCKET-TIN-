#ifndef UI_LIST_H
#define UI_LIST_H

#include <Arduino.h>
#include <vector>
#include <TFT_eSPI.h>
#include "keypad.h"
#include "display_config.h"

// =============================================================================
// UiListView: Wiederverwendbare, flicker-freie Listenansicht (DRY)
// Wird für Dateibrowser, Emulator-ROM-Auswahl und Systemmenüs verwendet
// =============================================================================

class UiListView {
public:
    UiListView();

    // Setzt den Inhalt der Liste
    void setItems(const std::vector<String>& items);

    // Setzt die Fußzeilen-Tastenbelegung (z.B. "S:X:Move  ENTER:Select  ESC:Back")
    void setHintText(const String& hint);

    // Zeichnet die Liste (nur geänderte Bereiche zur Vermeidung von Flimmern)
    void render(TFT_eSPI& tft, uint16_t textColor, uint16_t highlightColor, uint8_t textSize = 1);

    // Führt die interaktive Navigation aus. Gibt den Index des ausgewählten Elements zurück oder -1 bei ESC.
    // Ein optionaler Tastatur-Callback kann Spezialtasten verarbeiten (z.B. BACK für Löschen).
    typedef bool (*SpecialKeyCallback)(KeyCode code, int currentIndex, const String& currentItem);
    int runInteractive(TFT_eSPI& tft, KeypadManager& keypad, uint16_t textColor, uint16_t highlightColor, SpecialKeyCallback onSpecialKey = nullptr);

    int getSelectedIndex() const { return _selectedIndex; }
    void setSelectedIndex(int idx);

    void moveUp();
    void moveDown();

private:
    std::vector<String> _items;
    int _selectedIndex;
    String _hintText;
    int _lastRenderedSelected;
    int _lastStartIdx;

    void drawRows(TFT_eSPI& tft, uint16_t textColor, uint16_t highlightColor, uint8_t textSize, int startIdx, int maxLines);
};

#endif // UI_LIST_H

