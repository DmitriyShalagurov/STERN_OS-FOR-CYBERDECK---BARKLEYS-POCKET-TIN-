#ifndef MENU_H
#define MENU_H

#include <Arduino.h>

// Открывает интерактивное меню команд.
// Возвращает выбранную команду (String) или пустую строку, если нажали ESC.
String runMenu();

#endif