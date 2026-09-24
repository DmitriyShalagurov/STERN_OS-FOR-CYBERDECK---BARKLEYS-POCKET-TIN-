#include "keypad.h"
#include <Wire.h>
#include <string.h>

static const char* keySymbols[KEY_COUNT] = {
  "",                        // 0:  KEY_NONE
  "",                        // 1:  KEY_ESC
  "qQ1", "wW2", "eE3", "rR4", "tT5", "yY6", "uU7", "iI8", "oO9", // 2-10
  "",                        // 11: KEY_TAB
  "aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL{}", // 12-20
  "",                        // 21: KEY_SHIFT
  "zZ", "xX", "cC", "vV", "bB", "nN", "mM", // 22-28
  "<>",                      // 29: KEY_ANGLE
  ":;",                      // 30: KEY_SEMICOLON
  "",                        // 31: KEY_CTRL
  "",                        // 32: KEY_ALT
  "",                        // 33: KEY_FN
  "",                        // 34: KEY_CAPS_LOCK
  " ",                       // 35: KEY_SPACE
  "!*%",                     // 36: KEY_SPECIAL1
  "()",                      // 37: KEY_PARENTHESIS
  "\"",                      // 38: KEY_QUOTE
  "",                        // 39: KEY_WIFI_TOGGLE
  "pP0",                     // 40: KEY_P
  "[]",                      // 41: KEY_BRACKETS
  "?/\\",                    // 42: KEY_QUESTION_SLASH
  ".,",                      // 43: KEY_DOT_COMMA  <-- ИСПРАВЛЕНО
  "",                        // 44: KEY_BACK
  "",                        // 45: KEY_ENTER
  "$",                       // 46: KEY_DOLLAR
  "#@",                      // 47: KEY_HASH_AT
};

KeypadManager::KeypadManager() : _rows(KEYPAD_ROWS), _cols(KEYPAD_COLS),
  _shift(false), _ctrl(false), _alt(false), _fn(false), _capsLock(false),
  _lastKeyCode(KEY_NONE), _lastKeyTime(0), _tapIndex(0), _lastChar(0), _isMultiTap(false)
{
    for(int i=0;i<KEY_COUNT;i++) _pressed[i]=false;
    initMap();
}

bool KeypadManager::begin() {
  Wire.begin();
  if (!_keypad.begin(TCA8418_DEFAULT_ADDR, &Wire)) {
    if (!_keypad.begin(0x35, &Wire)) return false;
  }
  _keypad.matrix(_rows, _cols);
  _keypad.flush();
  return true;
}

void KeypadManager::flush() { _keypad.flush(); }
int KeypadManager::available() { return _keypad.available(); }
int KeypadManager::getEvent() { return _keypad.getEvent(); }

void KeypadManager::initMap() {
  for (int r = 0; r < _rows; r++)
    for (int c = 0; c < _cols; c++)
      _map[r][c] = KEY_NONE;

  _map[0][0] = KEY_ESC;
  _map[0][1] = KEY_Q;  _map[0][2] = KEY_W;  _map[0][3] = KEY_E;  _map[0][4] = KEY_R;
  _map[0][5] = KEY_T;  _map[0][6] = KEY_Y;  _map[0][7] = KEY_U;  _map[0][8] = KEY_I;  _map[0][9] = KEY_O;

  _map[1][0] = KEY_TAB;
  _map[1][1] = KEY_A;  _map[1][2] = KEY_S;  _map[1][3] = KEY_D;  _map[1][4] = KEY_F;
  _map[1][5] = KEY_G;  _map[1][6] = KEY_H;  _map[1][7] = KEY_J;  _map[1][8] = KEY_K;  _map[1][9] = KEY_L;

  _map[2][0] = KEY_SHIFT;
  _map[2][1] = KEY_Z;  _map[2][2] = KEY_X;  _map[2][3] = KEY_C;  _map[2][4] = KEY_V;
  _map[2][5] = KEY_B;  _map[2][6] = KEY_N;  _map[2][7] = KEY_M;
  _map[2][8] = KEY_ANGLE;
  _map[2][9] = KEY_SEMICOLON;

  _map[3][0] = KEY_CTRL;
  _map[3][1] = KEY_ALT;
  _map[3][2] = KEY_FN;
  _map[3][3] = KEY_CAPS_LOCK;
  _map[3][4] = KEY_NONE;
  _map[3][5] = KEY_SPACE;
  _map[3][6] = KEY_NONE;
  _map[3][7] = KEY_SPECIAL1;
  _map[3][8] = KEY_PARENTHESIS;
  _map[3][9] = KEY_QUOTE;

  _map[4][0] = KEY_WIFI_TOGGLE;
  _map[4][1] = KEY_MENU;              // <-- ИСПРАВЛЕНО (было KEY_NONE)
  _map[4][2] = KEY_P;
  _map[4][3] = KEY_BRACKETS;
  _map[4][4] = KEY_QUESTION_SLASH;
  _map[4][5] = KEY_NONE;
  _map[4][6] = KEY_BACK;
  _map[4][7] = KEY_ENTER;
  _map[4][8] = KEY_DOLLAR;
  _map[4][9] = KEY_HASH_AT;
}

void KeypadManager::updateModifiers(KeyCode code, bool pressed) {
  switch (code) {
    case KEY_SHIFT:     _shift = pressed; break;
    case KEY_CTRL:      _ctrl  = pressed; break;
    case KEY_ALT:       _alt   = pressed; break;
    case KEY_FN:
      if (pressed) _fn = !_fn;
      break;
    case KEY_CAPS_LOCK:
      if (pressed) _capsLock = !_capsLock;
      break;
    default: break;
  }
}

int KeypadManager::getSymbolIndex(KeyCode code) {
  if (_fn)    return 2;
  if (_shift) return 1;
  if (_ctrl)  return 0;
  return (_capsLock ? 1 : 0) + (_tapIndex * 2);
}

char KeypadManager::getSymbolForCode(KeyCode code, int index) {
  if (code <= KEY_NONE || code >= KEY_COUNT) return 0;
  const char* str = keySymbols[code];
  if (!str || *str == '\0') return 0;
  int len = strlen(str);
  if (len == 0) return 0;
  if (index >= len) return 0;
  char ch = str[index];
  if (_capsLock && isalpha(ch)) ch = toupper(ch);
  return ch;
}

KeyCode KeypadManager::getKey() {
  if (_keypad.available() > 0) {
    int k = _keypad.getEvent();
    bool pressed = k & 0x80;
    k &= 0x7F;
    k--;

    uint8_t row = k / _cols;
    uint8_t col = k % _cols;

    if (row < _rows && col < _cols) {
      KeyCode code = _map[row][col];
      if (code != KEY_NONE) {
        _pressed[code] = pressed;

        if (code == KEY_SHIFT || code == KEY_CTRL || code == KEY_ALT ||
            code == KEY_FN || code == KEY_CAPS_LOCK) {
          updateModifiers(code, pressed);
        }

        if (pressed) {
          unsigned long now = millis();
          bool isMultiTap = false;

          if (!_shift && !_fn && !_ctrl && code != KEY_CAPS_LOCK) {
            if (code == _lastKeyCode && (now - _lastKeyTime) < MULTITAP_TIMEOUT) {
              _tapIndex = (_tapIndex == 0) ? 1 : 0;
              isMultiTap = true;
            } else {
              _tapIndex = 0;
            }
            _lastKeyCode = code;
            _lastKeyTime = now;
          } else {
            _tapIndex = 0;
          }

          _isMultiTap = isMultiTap;

          int idx = getSymbolIndex(code);
          _lastChar = getSymbolForCode(code, idx);
          return code;
        }
      }
    }
  }
  _isMultiTap = false;
  return KEY_NONE;
}

char KeypadManager::getChar() { return _lastChar; }

bool KeypadManager::isPressed(KeyCode key) {
  if (key >= KEY_COUNT) return false;
  return _pressed[key];
}

KeypadManager Keypad;