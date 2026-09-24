#include "menu.h"
#include <TFT_eSPI.h>
#include "keypad.h"
#include "mouth.h"
#include <math.h>

extern TFT_eSPI tft;

// ---------- Список команд ----------
static const char* menuItems[] = {
    "help", "txt", "ls", "clear", "status", "info", "gui",
    "gameoflive", "dino", "donat", "nes", "osc", "mouth"
};
static const int MENU_COUNT = sizeof(menuItems) / sizeof(menuItems[0]);

#define SIDE_ROWS       3
#define VISIBLE         (SIDE_ROWS * 2 + 1)
#define ROW_H           30
#define LEFT_PAD        30
#define ARC_R           150.0f
#define FRAME_MS        30

// --- Параметры автоповтора ---
#define REPEAT_DELAY    400     // мс до первого повтора после нажатия
#define REPEAT_START    100     // стартовый интервал повторов
#define REPEAT_MIN      30      // минимальный интервал (максимальное ускорение)
#define REPEAT_ACCEL    8       // на сколько мс уменьшать интервал каждый шаг

// ---------- Цвета ----------
static uint16_t colInactive = 0;
static uint16_t colActive   = 0;
static uint16_t colBorder   = 0;

// ---------- Пружина ----------
static float scrollPos = 0.0f;
static float scrollVel = 0.0f;
static int   cursor    = 0;

// ---------- Спрайт ----------
static TFT_eSprite spr = TFT_eSprite(&tft);
static bool sprReady = false;
static bool useSprite = false;

// ---------- Рамка ----------
static void drBorder(TFT_eSPI* g) {
    g->drawRect(0, 0, tft.width(), tft.height(), colBorder);
    g->drawRect(1, 1, tft.width() - 2, tft.height() - 2, colBorder);
}

// ---------- Неактивная строка ----------
static void drInactive(TFT_eSPI* g, const char* text, int x, int y) {
    g->setTextSize(2);
    g->setTextColor(colInactive, TFT_BLACK);
    g->setCursor(x, y);
    g->print(text);
}

// ---------- Активная строка ----------
static void drActive(TFT_eSPI* g, const char* text, int y) {
    const int SIDE_CUT = 20;
    const int V_CUT    = 3;

    int boxX = 8 + SIDE_CUT;
    int boxW = tft.width() - 16 - 2 * SIDE_CUT;
    int boxH = 42 - 2 * V_CUT;
    int boxY = y - boxH / 2;

    g->fillRect(boxX, boxY, boxW, boxH, colActive);
    g->setTextSize(3);
    g->setTextColor(TFT_BLACK, colActive);

    int w  = g->textWidth(text);
    int th = g->fontHeight();
    g->setCursor(boxX + (boxW - w) / 2, boxY + (boxH - th) / 2);
    g->print(text);
}

// ---------- Отрисовка кадра ----------
static void drawFrame(TFT_eSPI* g, bool clearFirst) {
    if (clearFirst) {
        g->fillScreen(TFT_BLACK);
    }

    int w  = tft.width();
    int h  = tft.height();
    int cy = h / 2;

    float visualOffset = scrollPos - (float)cursor;

    for (int k = 0; k < VISIBLE; k++) {
        int off = k - SIDE_ROWS;
        int i = (cursor + off + MENU_COUNT * 100) % MENU_COUNT;

        float dy     = (off - visualOffset) * ROW_H;
        int   y_cent = cy + (int)dy;

        if (y_cent < -ROW_H || y_cent > h + ROW_H) continue;

        if (off == 0) {
            drActive(g, menuItems[i], y_cent);
        } else {
            float xshift = (dy * dy) / (2.0f * ARC_R);
            int x = LEFT_PAD - (int)xshift;
            if (x < 2) x = 2;
            drInactive(g, menuItems[i], x, y_cent - 8);
        }
    }

    g->setTextSize(1);
    g->setTextColor(colInactive, TFT_BLACK);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d/%d", cursor + 1, MENU_COUNT);
    g->setCursor(w - 40, h - 14);
    g->print(buf);

    drBorder(g);
}

// ---------- Шаг пружины ----------
static bool springStep() {
    float target = (float)cursor;
    float d = target - scrollPos;
    scrollVel += d * 0.30f;
    scrollVel *= 0.55f;
    scrollPos += scrollVel;

    if (fabsf(scrollPos - target) < 0.004f && fabsf(scrollVel) < 0.004f) {
        scrollPos = target;
        scrollVel = 0.0f;
        return false;
    }
    return true;
}

// ---------- Один шаг вверх (круговой) ----------
static void stepUp() {
    if (cursor == 0) {
        cursor = MENU_COUNT - 1;
        scrollPos = (float)MENU_COUNT;
    } else {
        cursor--;
    }
    scrollVel -= 0.55f;
    mouthTrigger(0.75f);
}

// ---------- Один шаг вниз (круговой) ----------
static void stepDown() {
    if (cursor == MENU_COUNT - 1) {
        cursor = 0;
        scrollPos = -1.0f;
    } else {
        cursor++;
    }
    scrollVel += 0.55f;
    mouthTrigger(0.75f);
}

// ---------- Инициализация спрайта ----------
static bool initSprite() {
    if (sprReady) return useSprite;

    spr.setColorDepth(8);
    if (spr.createSprite(tft.width(), tft.height()) != nullptr) {
        sprReady = true;
        useSprite = true;
        return true;
    }

    spr.setColorDepth(4);
    if (spr.createSprite(tft.width(), tft.height()) != nullptr) {
        sprReady = true;
        useSprite = true;
        return true;
    }

    sprReady = true;
    useSprite = false;
    return false;
}

// ---------- Запуск меню ----------
String runMenu() {
    colInactive = tft.color565(144, 97, 0);
    colActive   = 0xFE60;
    colBorder   = tft.color565(144, 97, 0);

    cursor    = 0;
    scrollPos = 0.0f;
    scrollVel = 0.0f;

    initSprite();
    mouthBegin();

    // ----- Первый кадр -----
    if (useSprite) {
        spr.fillSprite(TFT_BLACK);
        drawFrame(&spr, false);
        spr.pushSprite(0, 0);
    } else {
        drawFrame(&tft, true);
    }

    unsigned long lastFrame = 0;
    bool needRedraw = true;

    // --- Состояние автоповтора ---
    KeyCode repeatKey       = KEY_NONE;   // какая клавиша сейчас повторяется
    unsigned long repeatNext = 0;         // когда следующий шаг
    int repeatInterval      = REPEAT_START;

    while (true) {
        // ----- событие нажатия -----
        KeyCode code = Keypad.getKey();
        if (code != KEY_NONE) {
            if (code == KEY_ESC) {
                break;

            } else if (code == KEY_S) {
                stepUp();
                needRedraw = true;
                repeatKey       = KEY_S;
                repeatNext      = millis() + REPEAT_DELAY;
                repeatInterval  = REPEAT_START;

            } else if (code == KEY_X) {
                stepDown();
                needRedraw = true;
                repeatKey       = KEY_X;
                repeatNext      = millis() + REPEAT_DELAY;
                repeatInterval  = REPEAT_START;

            } else if (code == KEY_ENTER) {
                mouthEnd();
                return String(menuItems[cursor]);
            }
        }

        // ----- автоповтор при удержании -----
        bool sHeld = Keypad.isPressed(KEY_S);
        bool xHeld = Keypad.isPressed(KEY_X);

        if (!sHeld && !xHeld) {
            repeatKey = KEY_NONE;
        } else {
            // какую клавишу считаем удерживаемой
            KeyCode held = sHeld ? KEY_S : KEY_X;

            // если удерживаемая сменилась — сбросим задержку
            if (held != repeatKey) {
                repeatKey      = held;
                repeatNext     = millis() + REPEAT_DELAY;
                repeatInterval = REPEAT_START;
            } else if (millis() >= repeatNext) {
                if (held == KEY_S) stepUp();
                else               stepDown();

                needRedraw    = true;
                repeatNext    = millis() + repeatInterval;

                // ускорение: уменьшаем интервал до минимума
                if (repeatInterval > REPEAT_MIN) {
                    repeatInterval -= REPEAT_ACCEL;
                    if (repeatInterval < REPEAT_MIN) repeatInterval = REPEAT_MIN;
                }
            }
        }

        // ----- анимация -----
        unsigned long now = millis();
        if (now - lastFrame >= FRAME_MS) {
            lastFrame = now;

            bool moving = springStep();
            if (moving) needRedraw = true;

            if (needRedraw) {
                if (useSprite) {
                    spr.fillSprite(TFT_BLACK);
                    drawFrame(&spr, false);
                    spr.pushSprite(0, 0);
                } else {
                    drawFrame(&tft, true);
                }
                needRedraw = moving;
            }

            mouthUpdate();
        }

        delay(1);
    }

    mouthEnd();
    return String();
}