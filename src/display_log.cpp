#include "display_log.h"

#include <Arduino.h>
#include <M5StackChan.h>
#include <stdarg.h>

enum class LogLevel { Debug, Info, Warn, Error };

static bool screen_enabled = false;

void display_log_init(bool enable_screen) {
  screen_enabled = enable_screen;
  auto &display = M5.Display;
  display.fillScreen(TFT_BLACK);
  if (!screen_enabled) return;
  display.setFont(&fonts::Font2);  // 16px height, ~40 chars x 15 lines
  display.setTextSize(1);
  display.setTextWrap(true);
  display.setTextScroll(true);
  display.setScrollRect(0, 0, display.width(), display.height());
  display.setCursor(0, 0);
}

static void vlog(LogLevel level, const char *fmt, va_list args) {
  char buf[128];
  vsnprintf(buf, sizeof(buf), fmt, args);

  const char *tag = "";
  uint16_t color = TFT_WHITE;
  switch (level) {
    case LogLevel::Debug:
      tag = "[DEBUG]";
      break;
    case LogLevel::Info:
      tag = "[INFO]";
      color = TFT_WHITE;
      break;
    case LogLevel::Warn:
      tag = "[WARN]";
      color = TFT_YELLOW;
      break;
    case LogLevel::Error:
      tag = "[ERROR]";
      color = TFT_RED;
      break;
  }
  Serial.printf("%s %s\n", tag, buf);

  if (!screen_enabled || level == LogLevel::Debug) return;
  auto &display = M5.Display;
  display.setTextColor(color, TFT_BLACK);
  if (level == LogLevel::Info) {
    display.println(buf);
  } else {
    display.printf("%s %s\n", tag, buf);
  }
}

#define DEFINE_LOG_FUNC(name, level) \
  void name(const char *fmt, ...) {  \
    va_list args;                    \
    va_start(args, fmt);             \
    vlog(level, fmt, args);          \
    va_end(args);                    \
  }

DEFINE_LOG_FUNC(log_debug, LogLevel::Debug)
DEFINE_LOG_FUNC(log_info, LogLevel::Info)
DEFINE_LOG_FUNC(log_warn, LogLevel::Warn)
DEFINE_LOG_FUNC(log_error, LogLevel::Error)
