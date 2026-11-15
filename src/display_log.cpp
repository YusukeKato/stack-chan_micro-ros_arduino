#include "display_log.h"

#include <Arduino.h>
#include <M5StackChan.h>
#include <stdarg.h>

static const int LINE_HEIGHT = 16;  // Font2
static const int STATUS_HEIGHT = LINE_HEIGHT * STATUS_LINE_COUNT;

static bool screen_enabled = false;
static char last_status[STATUS_LINE_COUNT][64];

void display_log_init(bool enable_screen) {
  screen_enabled = enable_screen;
  M5.Display.fillScreen(TFT_BLACK);
  if (!screen_enabled) return;
  M5.Display.setFont(&fonts::Font2);  // ~40 chars x 15 lines
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextScroll(true);
  // top lines are reserved for status, logs scroll below them
  M5.Display.setScrollRect(0, STATUS_HEIGHT, M5.Display.width(),
                           M5.Display.height() - STATUS_HEIGHT);
  M5.Display.setCursor(0, STATUS_HEIGHT);
}

void display_log(const char *fmt, ...) {
  char buf[128];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  Serial.println(buf);
  if (screen_enabled) M5.Display.println(buf);
}

void display_status(StatusLine line, const char *fmt, ...) {
  if (!screen_enabled || line < 0 || line >= STATUS_LINE_COUNT) return;
  char buf[sizeof(last_status[0])];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  // redraw only when the text changes
  if (strcmp(buf, last_status[line]) == 0) return;
  strcpy(last_status[line], buf);
  int y = LINE_HEIGHT * line;
  M5.Display.fillRect(0, y, M5.Display.width(), LINE_HEIGHT, TFT_BLACK);
  M5.Display.drawString(buf, 0, y);  // does not move the log cursor
}
