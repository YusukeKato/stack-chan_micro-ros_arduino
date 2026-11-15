#ifndef STACKCHAN_ROS_UTILS_H
#define STACKCHAN_ROS_UTILS_H

#include <builtin_interfaces/msg/time.h>
#include <rcl/rcl.h>
#include <rosidl_runtime_c/string.h>

// Set a stamp from the clock synchronized with the micro-ROS Agent.
void set_stamp(builtin_interfaces__msg__Time *stamp);

// Point a message string at a constant C string (no allocation).
void set_string(rosidl_runtime_c__String *str, const char *value);

// rcl_publish(). Returns true on success.
bool publish(rcl_publisher_t *publisher, const void *msg);

// Log "[ERROR] <what> failed (<ret>)" if ret is not RCL_RET_OK. Returns true on success.
bool check(rcl_ret_t ret, const char *what);

// Returns true at most once per period_ms for each last_ms (for rate-limited logs).
bool every_ms(uint32_t *last_ms, uint32_t period_ms);

#endif  // STACKCHAN_ROS_UTILS_H
