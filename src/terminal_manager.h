#ifndef TERMINAL_MANAGER_H
#define TERMINAL_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "gui_settings.h"

extern String outputBuffer[20];
extern int outputCount;
extern String inputLine;
extern const int TERMINAL_START_Y;
extern const int MAX_HISTORY;
extern const int LINE_HEIGHT;

void terminalInitInput();
void terminalPrint(String text);
void terminalClear();
void drawTerminal();

#endif