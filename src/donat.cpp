#include "donat.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <math.h>
#include <string.h>
#include "keypad.h"
#include "terminal_manager.h"
#include "status_bar.h"
#include "oled_display.h"

extern TFT_eSPI tft;

#define DONAT_COLS    80
#define DONAT_ROWS    60
#define DONAT_CELL    4

#define COLOR_COUNT   8
#define SPEED_COUNT   10

struct DonutColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  const char* name;
};

static const DonutColor donutPalette[COLOR_COUNT] = {
  { 255, 255, 255, "WHITE"   },
  { 255,  90,  90, "RED"     },
  {  90, 255,  90, "GREEN"   },
  {  90, 150, 255, "BLUE"    },
  { 255, 255,  90, "YELLOW"  },
  {  90, 255, 255, "CYAN"    },
  { 255,  90, 255, "MAGENTA" },
  { 255, 165,  40, "ORANGE"  }
};

static const float donutSpeeds[SPEED_COUNT] = {
  1.0f, 2.0f, 3.0f, 4.0f, 5.0f,
  6.0f, 7.0f, 8.0f, 9.0f, 10.0f
};

static float    d_zbuf    [DONAT_COLS * DONAT_ROWS];
static uint16_t d_framebuf[DONAT_COLS * DONAT_ROWS];
static uint16_t d_prevbuf [DONAT_COLS * DONAT_ROWS];

static int d_colorIndex = 0;
static int d_speedIndex = 0;

static void donatRender(float A, float B) {
  memset(d_zbuf,     0, sizeof(d_zbuf));
  memset(d_framebuf, 0, sizeof(d_framebuf));

  const uint8_t cr = donutPalette[d_colorIndex].r;
  const uint8_t cg = donutPalette[d_colorIndex].g;
  const uint8_t cb = donutPalette[d_colorIndex].b;

  const float cosA = cosf(A), sinA = sinf(A);
  const float cosB = cosf(B), sinB = sinf(B);
  const float CX   = DONAT_COLS * 0.5f;
  const float CY   = DONAT_ROWS * 0.5f;

  for (float phi = 0.0f; phi < TWO_PI; phi += 0.07f) {
    const float cosphi = cosf(phi);
    const float sinphi = sinf(phi);
    const float h      = cosphi + 2.0f;
    const float hsinA  = h * sinA;
    const float hcosA  = h * cosA;

    for (float theta = 0.0f; theta < TWO_PI; theta += 0.02f) {
      const float sintheta = sinf(theta);
      const float costheta = cosf(theta);

      const float D = 1.0f / (sintheta * hsinA + sinphi * cosA + 5.0f);
      const float t = sintheta * hcosA - sinphi * sinA;

      const int x = (int)(CX + 29.0f * D * (costheta * h * cosB - t * sinB));
      const int y = (int)(CY + 29.0f * D * (costheta * h * sinB + t * cosB));

      if ((unsigned)x >= DONAT_COLS || (unsigned)y >= DONAT_ROWS) continue;
      const int o = x + DONAT_COLS * y;

      if (D > d_zbuf[o]) {
        d_zbuf[o] = D;

        int N = (int)(8.0f * (
            (sinphi * sinA - sintheta * cosphi * cosA) * cosB
            -  sintheta * cosphi * sinA
            -  sinphi * cosA
            -  costheta * cosphi * sinB
        ));
        if (N < 0)  N = 0;
        if (N > 11) N = 11;

        const uint8_t v = (uint8_t)(30 + N * 20);

        const uint8_t r = (uint8_t)((v * cr) / 255);
        const uint8_t g = (uint8_t)((v * cg) / 255);
        const uint8_t b = (uint8_t)((v * cb) / 255);

        d_framebuf[o] = tft.color565(r, g, b);
      }
    }
  }
}

static void donatDrawOled() {
  if (!oledOK) return;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Z / C  change color");

  display.print("col - ");
  display.println(donutPalette[d_colorIndex].name);

  display.print("spd - ");
  display.print((int)donutSpeeds[d_speedIndex]);
  display.println("x");

  display.display();
  delay(3);
}

void runDonat() {
  while (Keypad.getKey() != KEY_NONE) { delay(1); }

  tft.fillScreen(TFT_BLACK);
  tft.startWrite();
  tft.endWrite();

  d_colorIndex = 0;
  d_speedIndex = 0;
  donatDrawOled();

  memset(d_prevbuf, 0, sizeof(d_prevbuf));

  float A = 0.0f;
  float B = 0.0f;

  while (true) {
    KeyCode k = Keypad.getKey();
    if (k == KEY_ESC) break;

    if (k == KEY_Z) {
      d_colorIndex--;
      if (d_colorIndex < 0) d_colorIndex = COLOR_COUNT - 1;
      memset(d_prevbuf, 0, sizeof(d_prevbuf));
      tft.fillScreen(TFT_BLACK);
      donatDrawOled();
    } else if (k == KEY_C) {
      d_colorIndex++;
      if (d_colorIndex >= COLOR_COUNT) d_colorIndex = 0;
      memset(d_prevbuf, 0, sizeof(d_prevbuf));
      tft.fillScreen(TFT_BLACK);
      donatDrawOled();
    } else if (k == KEY_SPACE) {
      d_speedIndex++;
      if (d_speedIndex >= SPEED_COUNT) d_speedIndex = 0;
      donatDrawOled();
    }

    donatRender(A, B);

    for (int i = 0; i < DONAT_COLS * DONAT_ROWS; ++i) {
      if (d_framebuf[i] != d_prevbuf[i]) {
        const int x = i % DONAT_COLS;
        const int y = i / DONAT_COLS;
        const uint16_t col = d_framebuf[i] ? d_framebuf[i] : (uint16_t)TFT_BLACK;
        tft.fillRect(x * DONAT_CELL,
                     y * DONAT_CELL,
                     DONAT_CELL, DONAT_CELL, col);
        d_prevbuf[i] = d_framebuf[i];
      }
    }

    const float mult = donutSpeeds[d_speedIndex];
    A += 0.04f * mult;
    B += 0.02f * mult;

    delay(1);
  }

  while (Keypad.getKey() != KEY_NONE) { delay(1); }

  tft.fillScreen(TFT_BLACK);
}