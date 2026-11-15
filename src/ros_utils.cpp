#include "ros_utils.h"

#include <Arduino.h>
#include <rmw_microros/rmw_microros.h>
#include <string.h>

void set_stamp(builtin_interfaces__msg__Time *stamp) {
  int64_t ms = rmw_uros_epoch_millis();
  stamp->sec = (int32_t)(ms / 1000);
  stamp->nanosec = (uint32_t)((ms % 1000) * 1000000);
}

void set_string(rosidl_runtime_c__String *str, const char *value) {
  str->data = (char *)value;
  str->size = strlen(value);
  str->capacity = str->size + 1;
}

bool publish(rcl_publisher_t *publisher, const void *msg) {
  return rcl_publish(publisher, msg, NULL) == RCL_RET_OK;
}

bool every_ms(uint32_t *last_ms, uint32_t period_ms) {
  uint32_t now = millis();
  if (*last_ms != 0 && now - *last_ms < period_ms) return false;
  *last_ms = now;
  return true;
}
