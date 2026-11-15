// reference:
// https://docs.m5stack.com/ja/arduino/stackchan/battery

#include "battery.h"

#include <M5StackChan.h>
#include <sensor_msgs/msg/battery_state.h>

#include "display_log.h"
#include "ros_utils.h"

static rcl_publisher_t battery_pub;
static sensor_msgs__msg__BatteryState battery_msg;

// publish voltage, current, level and charge state
void update_battery() {
  static uint32_t last_ms = 0;
  if (!every_ms(&last_ms, 1000)) return;
  int32_t level = M5.Power.getBatteryLevel();                    // [%]
  battery_msg.voltage = M5.Power.getBatteryVoltage() / 1000.0f;  // [mV] -> [V]
  battery_msg.current = M5.Power.getBatteryCurrent() / 1000.0f;  // [mA] -> [A]
  battery_msg.percentage = level / 100.0f;                       // [0.0-1.0]

  const char *status = "unknown";
  battery_msg.power_supply_status = sensor_msgs__msg__BatteryState__POWER_SUPPLY_STATUS_UNKNOWN;
  auto charge_state = M5.Power.isCharging();
  if (charge_state == m5::Power_Class::is_charging) {
    status = "charging";
    battery_msg.power_supply_status = sensor_msgs__msg__BatteryState__POWER_SUPPLY_STATUS_CHARGING;
  } else if (charge_state == m5::Power_Class::is_discharging) {
    status = "discharging";
    battery_msg.power_supply_status =
        sensor_msgs__msg__BatteryState__POWER_SUPPLY_STATUS_DISCHARGING;
  }

  set_stamp(&battery_msg.header.stamp);
  publish(&battery_pub, &battery_msg);
  display_status(STATUS_BATTERY, "Battery  %d%%  %.2f V  %s", (int)level, battery_msg.voltage,
                 status);
}

void setup_battery(rcl_node_t *node) {
  // best effort: no delivery wait (reliable blocks up to 1 s per publish)
  rcl_ret_t ret = rclc_publisher_init_best_effort(
      &battery_pub, node, ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, BatteryState),
      "stackchan/battery_state");
  if (!check(ret, "Battery publisher")) return;
  set_string(&battery_msg.header.frame_id, "base_link");
  display_log("Battery: ready");
}
