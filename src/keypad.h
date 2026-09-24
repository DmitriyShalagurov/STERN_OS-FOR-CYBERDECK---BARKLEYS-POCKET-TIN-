#ifndef KEYPAD_H
#define KEYPAD_H

#include <Arduino.h>
#include <Adafruit_TCA8418.h>

#define KEYPAD_ROWS 5
#define KEYPAD_COLS 10
#define MULTITAP_TIMEOUT 500   // мс

enum KeyCode {
  KEY_NONE = 0,

  // row0
  KEY_ESC,
  KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O,
  // row1
  KEY_TAB,
  KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L,
  // row2
  KEY_SHIFT,
  KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M,
  KEY_ANGLE,        // < >
  KEY_SEMICOLON,    // : ;
  // row3
  KEY_CTRL,
  KEY_ALT,
  KEY_FN,
  KEY_CAPS_LOCK,
  KEY_SPACE,        // col5
  KEY_SPECIAL1,     // ! * %
  KEY_PARENTHESIS,  // ( )
  KEY_QUOTE,        // "
  // row4
  KEY_WIFI_TOGGLE,
  KEY_P,
  KEY_BRACKETS,     // [ ]
  KEY_QUESTION_SLASH, // ? / \
  KEY_DOT_COMMA,    // . ,
  KEY_BACK,
  KEY_ENTER,
  KEY_DOLLAR,
  KEY_HASH_AT,      // # @

  KEY_MENU,         // <-- НОВОЕ: кнопка вызова меню (row4, col1)

  KEY_COUNT         // обязательно последним!
};

class KeypadManager {
public:
  KeypadManager();
  bool begin();
  void flush();
  KeyCode getKey();
  bool isPressed(KeyCode key);
  char getChar();
  int available();
  int getEvent();

  bool isShiftPressed() const { return _shift; }
  bool isCtrlPressed()  const { return _ctrl; }
  bool isAltPressed()   const { return _alt; }
  bool isFnPressed()    const { return _fn; }
  bool isCapsLockOn()   const { return _capsLock; }
  bool isMultiTap()     const { return _isMultiTap; }

private:
  Adafruit_TCA8418 _keypad;
  uint8_t _rows;
  uint8_t _cols;
  KeyCode _map[KEYPAD_ROWS][KEYPAD_COLS];

  bool _pressed[KEY_COUNT];

  bool _shift, _ctrl, _alt, _fn, _capsLock;
  KeyCode _lastKeyCode;
  unsigned long _lastKeyTime;
  int _tapIndex;
  char _lastChar;
  bool _isMultiTap;

  void initMap();
  void updateModifiers(KeyCode code, bool pressed);
  int getSymbolIndex(KeyCode code);
  char getSymbolForCode(KeyCode code, int index);
};

extern KeypadManager Keypad;

#endif