#ifndef STACKCHAN_DISPLAY_LOG_H
#define STACKCHAN_DISPLAY_LOG_H

// Log output to Serial and (optionally) the StackChan screen.
//
// Screen layout:
//   line 1   : joint angles (fixed, updated in place)
//   line 2.. : log lines (scroll up when full)
//
// Call only from the Arduino loop task (setup(), loop() and executor callbacks).

// Call once after M5StackChan.begin().
// enable_screen: true = show logs on the screen, false = keep the screen blank.
void display_log_init(bool enable_screen);

// Print one log line to Serial and the screen. printf-style.
void display_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

// Update the fixed joint angle line on the screen (not printed to Serial). [deg]
void display_joint_angles(float yaw_deg, float pitch_deg);

#endif  // STACKCHAN_DISPLAY_LOG_H
