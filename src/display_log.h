#ifndef STACKCHAN_DISPLAY_LOG_H
#define STACKCHAN_DISPLAY_LOG_H

// Log output to Serial and (optionally) the StackChan screen.
//
// Screen layout:
//   top lines : sensor status (fixed, one line each, updated in place)
//   below     : log lines (scroll up when full)
//
// Call only from the Arduino loop task (setup(), loop() and executor callbacks).

// Fixed status lines, from the top of the screen.
enum StatusLine {
  STATUS_JOINT,
  STATUS_BATTERY,
  STATUS_IMU,
  STATUS_TOUCH,
  STATUS_LINE_COUNT,
};

// Call once after M5StackChan.begin().
// enable_screen: true = show logs on the screen, false = keep the screen blank.
void display_log_init(bool enable_screen);

// Print one log line to Serial and the screen. printf-style.
void display_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

// Overwrite one fixed status line on the screen (not printed to Serial). printf-style.
void display_status(StatusLine line, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

#endif  // STACKCHAN_DISPLAY_LOG_H
