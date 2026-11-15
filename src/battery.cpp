// reference:
// https://docs.m5stack.com/ja/arduino/stackchan/battery

#include "battery.h"

#include <Arduino.h>
#include <M5StackChan.h>
#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <sensor_msgs/msg/battery_state.h>

#include "display_log.h"

static rcl_publisher_t battery_pub;
static sensor_msgs__msg__BatteryState battery_msg;
static rcl_timer_t battery_timer;

// publish voltage and current
static void battery_timer_callback(rcl_timer_t *timer, int64_t last_call_time) {
  if (timer != NULL) {
    // get voltage and current
    int32_t vol_mv = M5.Power.getBatteryVoltage();  // [mV]
    int32_t cur_ma = M5.Power.getBatteryCurrent();  // [mA]
    int32_t level = M5.Power.getBatteryLevel();     // [%]
    battery_msg.voltage = vol_mv / 1000.0f;         // [V]
    battery_msg.current = cur_ma / 1000.0f;         // [A]
    battery_msg.percentage = level / 100.0f;        // [0.0-1.0]
    // get charge state
    auto charge_state = M5.Power.isCharging();
    if (charge_state == m5::Power_Class::is_charging) {
      battery_msg.power_supply_status =
          sensor_msgs__msg__BatteryState__POWER_SUPPLY_STATUS_CHARGING;
    } else if (charge_state == m5::Power_Class::is_discharging) {
      battery_msg.power_supply_status =
          sensor_msgs__msg__BatteryState__POWER_SUPPLY_STATUS_DISCHARGING;
    } else {
      battery_msg.power_supply_status = sensor_msgs__msg__BatteryState__POWER_SUPPLY_STATUS_UNKNOWN;
    }
    // header setup
    int64_t time = rmw_uros_epoch_millis();
    battery_msg.header.stamp.sec = (int32_t)(time / 1000);
    battery_msg.header.stamp.nanosec = (uint32_t)((time % 1000) * 1000000);
    battery_msg.header.frame_id.data = (char *)"base_link";
    battery_msg.header.frame_id.size = strlen("base_link");
    battery_msg.header.frame_id.capacity = battery_msg.header.frame_id.size + 1;
    // publish
    rcl_publish(&battery_pub, &battery_msg, NULL);
    // log only when level or charge state changes
    static int32_t last_level = -1;
    static uint8_t last_status = 0xFF;
    if (level != last_level || battery_msg.power_supply_status != last_status) {
      const char *status = "unknown";
      if (charge_state == m5::Power_Class::is_charging) {
        status = "charging";
      } else if (charge_state == m5::Power_Class::is_discharging) {
        status = "discharging";
      }
      log_info("Battery: %d%% %.2fV %s", (int)level, battery_msg.voltage, status);
      last_level = level;
      last_status = battery_msg.power_supply_status;
    }
  }
}

void setup_battery(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor) {
  log_debug("Registering Battery Node...");
  // publisher
  rclc_publisher_init_default(&battery_pub, node,
                              ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, BatteryState),
                              "stackchan/battery_state");
  // timer setup
  rclc_timer_init_default(&battery_timer, support, RCL_MS_TO_NS(1000), battery_timer_callback);
  rclc_executor_add_timer(executor, &battery_timer);

  log_info("Battery: ready");
}
