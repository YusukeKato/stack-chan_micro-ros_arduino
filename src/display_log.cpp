#include "display_log.h"

#include <Arduino.h>
#include <M5StackChan.h>
#include <stdarg.h>

static const int LINE_HEIGHT = 16;  // Font2

static bool screen_enabled = false;

void display_log_init(bool enable_screen) {
  screen_enabled = enable_screen;
  M5.Display.fillScreen(TFT_BLACK);
  if (!screen_enabled) return;
  M5.Display.setFont(&fonts::Font2);  // ~40 chars x 15 lines
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextScroll(true);
  // first line is reserved for joint angles, logs scroll below it
  M5.Display.setScrollRect(0, LINE_HEIGHT, M5.Display.width(), M5.Display.height() - LINE_HEIGHT);
  M5.Display.setCursor(0, LINE_HEIGHT);
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

void display_joint_angles(float yaw_deg, float pitch_deg) {
  if (!screen_enabled) return;
  char buf[48];
  snprintf(buf, sizeof(buf), "Joint yaw %6.1f  pitch %6.1f [deg]", yaw_deg, pitch_deg);
  // redraw only when the text changes
  static char last[48] = "";
  if (strcmp(buf, last) == 0) return;
  strcpy(last, buf);
  M5.Display.fillRect(0, 0, M5.Display.width(), LINE_HEIGHT, TFT_BLACK);
  M5.Display.drawString(buf, 0, 0);  // does not move the log cursor
}
