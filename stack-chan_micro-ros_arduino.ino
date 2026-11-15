#include <Arduino.h>
#include <M5StackChan.h>
#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>

#include "config.h"  // include only here: it defines variables
#include "src/battery.h"
#include "src/camera.h"
#include "src/display_log.h"
#include "src/imu.h"
#include "src/motor.h"
#include "src/ros_utils.h"
#include "src/speaker.h"
#include "src/touch.h"
#include "src/wifi_transport.h"

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
  set_wifi_transport(ssid, password, agent_ip_address, agent_port);
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

  // executor only handles subscriptions; publishers are driven by update_*() in loop()
  // (rcl timers are not used: micro-ROS has only 4 guard conditions, one per timer)
  if (ENABLE_MOTOR) num_handles += 1;    // joint_commands
  if (ENABLE_SPEAKER) num_handles += 1;  // tone
  if (num_handles > 0) {
    rcl_ret_t ret = rclc_executor_init(&executor, &support.context, num_handles, &allocator);
    if (!check(ret, "Executor init")) num_handles = 0;
  }

  if (ENABLE_MOTOR) setup_motor(&node, &executor);
  if (ENABLE_CAMERA) setup_camera(&node);
  if (ENABLE_BATTERY) setup_battery(&node);
  if (ENABLE_TOUCH) setup_touch(&node);
  if (ENABLE_IMU) setup_imu(&node);
  if (ENABLE_SPEAKER) setup_speaker(&node, &executor, SPEAKER_VOLUME);

  display_log("ROS 2 ready! (node: stackchan_node)");
}

void loop() {
  M5StackChan.update();
  if (num_handles > 0) {
    // timeout 0: with any non-zero timeout, the micro-XRCE session loop ran until the next
    // second boundary (about 1 s per call), so every topic dropped to 1 Hz
    rcl_ret_t ret = rclc_executor_spin_some(&executor, 0);
    if (ret != RCL_RET_OK && ret != RCL_RET_TIMEOUT) {
      static uint32_t last_error_ms = 0;
      if (every_ms(&last_error_ms, 1000)) display_log("[ERROR] Executor spin failed: %d", (int)ret);
    }
  }
  delay(1);
  if (ENABLE_MOTOR) update_motor();
  if (ENABLE_CAMERA) update_camera();
  if (ENABLE_BATTERY) update_battery();
  if (ENABLE_TOUCH) update_touch();
  if (ENABLE_IMU) update_imu();
}
