// reference:
// https://docs.m5stack.com/ja/arduino/stackchan/battery

#include <Arduino.h>
#include <Avatar.h>
#include <M5StackChan.h>
#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <sensor_msgs/msg/battery_state.h>

extern m5avatar::Avatar avatar;

rcl_publisher_t battery_pub;
sensor_msgs__msg__BatteryState battery_msg;
rcl_timer_t battery_timer;

// publish voltage and current
void battery_timer_callback(rcl_timer_t * timer, int64_t last_call_time) {
  if (timer != NULL) {
    // get voltage and current
    int32_t vol_mv = M5.Power.getBatteryVoltage(); // [mV]
    int32_t cur_ma = M5.Power.getBatteryCurrent(); // [mA]
    int32_t level = M5.Power.getBatteryLevel();  // [%]
    battery_msg.voltage = vol_mv / 1000.0f; // [V]
    battery_msg.current = cur_ma / 1000.0f; // [A]
    battery_msg.percentage = level / 100.0f;  // [0.0-1.0]
    // get charge state
    auto charge_state = M5.Power.isCharging();
    if (charge_state == m5::Power_Class::is_charging) {
      battery_msg.power_supply_status = sensor_msgs__msg__BatteryState__POWER_SUPPLY_STATUS_CHARGING;
    } else if (charge_state == m5::Power_Class::is_discharging) {
      battery_msg.power_supply_status = sensor_msgs__msg__BatteryState__POWER_SUPPLY_STATUS_DISCHARGING;
    } else {
      battery_msg.power_supply_status = sensor_msgs__msg__BatteryState__POWER_SUPPLY_STATUS_UNKNOWN;
    }
    // header setup
    int64_t time = rmw_uros_epoch_millis();
    battery_msg.header.stamp.sec = (int32_t)(time / 1000);
    battery_msg.header.stamp.nanosec = (uint32_t)((time % 1000) * 1000000);
    battery_msg.header.frame_id.data = (char*)"base_link";
    battery_msg.header.frame_id.size = strlen("base_link");
    battery_msg.header.frame_id.capacity = battery_msg.header.frame_id.size + 1;
    // publish
    rcl_publish(&battery_pub, &battery_msg, NULL);
    // debug
    static char speech_buf[64];
    snprintf(speech_buf, sizeof(speech_buf), "Battery : %d%%", level);
    avatar.setSpeechText(speech_buf);
    /*
    M5StackChan.Display().setCursor(0, 140);
    M5StackChan.Display().setTextColor(TFT_WHITE, TFT_BLACK); 
    M5StackChan.Display().printf("  Volt  : %4.2f V \n", battery_msg.voltage);
    M5StackChan.Display().printf("  Level : %4d %% \n", level);
    M5StackChan.Display().printf("  Status: %s    \n", 
      (charge_state == m5::Power_Class::is_charging) ? "Charging " : 
      (charge_state == m5::Power_Class::is_discharging) ? "Discharge" : "Unknown  ");
    */
  }
}

void setup_battery(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor) {
  Serial.println("[DEBUG] Registering Battery Node...");
  // publisher
  rclc_publisher_init_default(
    &battery_pub, node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, BatteryState),
    "stackchan/battery_state");
  // timer setup
  rclc_timer_init_default(&battery_timer, support, RCL_MS_TO_NS(1000), battery_timer_callback);
  rclc_executor_add_timer(executor, &battery_timer);

  Serial.println("[DEBUG] Battery Node Registered!");
}