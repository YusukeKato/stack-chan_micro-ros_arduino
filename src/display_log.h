#ifndef STACKCHAN_DISPLAY_LOG_H
#define STACKCHAN_DISPLAY_LOG_H

// Log output to Serial and (optionally) the StackChan screen.
//
// - log_debug: Serial only
// - log_info / log_warn / log_error: Serial, and the screen when enabled
//
// The screen shows the latest lines and scrolls up when full.
// Call only from the Arduino loop task (setup(), loop() and executor callbacks).

// Initialize logging. Call after M5StackChan.begin().
// enable_screen: true = show logs on the screen, false = keep the screen blank.
void display_log_init(bool enable_screen);

void log_debug(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void log_info(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void log_warn(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void log_error(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif  // STACKCHAN_DISPLAY_LOG_H
