#include <Arduino.h>
#include <M5StackChan.h>
#include <micro_ros_arduino.h>
#include <rcl/error_handling.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>

#include "config.h"  // include only here: it defines variables
#include "src/battery.h"
#include "src/camera.h"
#include "src/display_log.h"
#include "src/imu.h"
#include "src/motor.h"
#include "src/touch.h"

rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

unsigned int num_handles = 0;

void setup() {
  // serial setup
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n--- StackChan Booting ---");

  // init stack-chan
  M5StackChan.begin();
  display_log_init(ENABLE_DISPLAY_LOG);
  display_log("StackChan micro-ROS booting");
  M5StackChan.Motion.goHome();
  delay(1000);

  if (ENABLE_CAMERA) init_camera_hardware();  // logs the result

  display_log("Wi-Fi: connecting to %s", ssid);
  set_microros_wifi_transports(ssid, password, agent_ip_address, agent_port);
  display_log("Wi-Fi: connected, IP %s", WiFi.localIP().toString().c_str());
  delay(2000);

  display_log("Agent: waiting %s:%u", agent_ip_address, (unsigned)agent_port);
  while (rmw_uros_ping_agent(100, 1) != RMW_RET_OK) {
    M5StackChan.update();
    delay(100);
  }
  display_log("Agent: connected");

  // init micro-ROS core
  allocator = rcl_get_default_allocator();
  rclc_support_init(&support, 0, NULL, &allocator);
  rmw_uros_sync_session(1000);  // time sync
  rclc_node_init_default(&node, "stackchan_node", "", &support);

  // count num_handles
  if (ENABLE_MOTOR) num_handles += 2;    // pub(timer), sub
  if (ENABLE_CAMERA) num_handles += 1;   // pub(timer)
  if (ENABLE_BATTERY) num_handles += 1;  // pub(timer)
  if (ENABLE_TOUCH) num_handles += 1;    // pub(timer)
  if (ENABLE_IMU) num_handles += 1;      // pub(timer)

  if (num_handles > 0) {
    rclc_executor_init(&executor, &support.context, num_handles, &allocator);
    if (ENABLE_MOTOR) setup_motor(&node, &support, &executor);
    if (ENABLE_CAMERA) setup_camera(&node, &support, &executor);
    if (ENABLE_BATTERY) setup_battery(&node, &support, &executor);
    if (ENABLE_TOUCH) setup_touch(&node, &support, &executor);
    if (ENABLE_IMU) setup_imu(&node, &support, &executor);
  } else {
    display_log("[WARN] All topics are disabled in config.h");
  }

  display_log("ROS 2 ready! (node: stackchan_node)");
}

void loop() {
  M5StackChan.update();
  if (num_handles > 0) {
    rcl_ret_t ret = rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));
    if (ret != RCL_RET_OK && ret != RCL_RET_TIMEOUT) {
      // limit to once per second so the screen is not flooded
      static uint32_t last_error_ms = 0;
      if (millis() - last_error_ms > 1000) {
        display_log("[ERROR] Executor spin failed: %d", (int)ret);
        last_error_ms = millis();
      }
      delay(100);
    }
  }
}
