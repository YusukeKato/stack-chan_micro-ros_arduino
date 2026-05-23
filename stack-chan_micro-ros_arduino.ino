#include <Arduino.h>
#include <Avatar.h>
#include <M5StackChan.h>
#include <micro_ros_arduino.h>
#include <rcl/error_handling.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>

#include "config.h"

rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

m5avatar::Avatar avatar;

unsigned int num_handles = 0;

static char speech_buf[64];

extern bool init_camera_hardware();
extern void setup_motor(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor);
extern void setup_camera(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor);
extern void setup_battery(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor);
extern void setup_speech(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor);
extern void setup_touch(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor);

void setup() {
  // serial setup
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n--- StackChan Booting ---");

  // init stack-chan
  M5StackChan.begin();
  M5StackChan.Display().setTextSize(2);
  M5.Speaker.setVolume(255);  // max: 255
  M5StackChan.Motion.goHome();
  delay(1000);

  // init avatar
  avatar.init();
  avatar.setExpression(m5avatar::Expression::Neutral);

  if (ENABLE_CAMERA && !init_camera_hardware()) {
    Serial.println("[ERROR] Failed to initialize camera hardware!");
  }

  snprintf(speech_buf, sizeof(speech_buf), "Connecting Wi-Fi...");
  avatar.setSpeechText(speech_buf);
  set_microros_wifi_transports(ssid, password, agent_ip_address, agent_port);
  delay(2000);

  snprintf(speech_buf, sizeof(speech_buf), "Wait micro-ROS Agent...");
  avatar.setSpeechText(speech_buf);
  Serial.println("[INFO] Waiting for micro-ROS Agent...");
  while (rmw_uros_ping_agent(100, 1) != RMW_RET_OK) {
    M5StackChan.update();
    delay(100);
  }

  // init micro-ROS core
  allocator = rcl_get_default_allocator();
  rclc_support_init(&support, 0, NULL, &allocator);
  rclc_node_init_default(&node, "stackchan_node", "", &support);

  // count num_handles
  if (ENABLE_MOTOR) num_handles += 2;    // pub(timer), sub
  if (ENABLE_CAMERA) num_handles += 1;   // pub(timer)
  if (ENABLE_BATTERY) num_handles += 1;  // pub(timer)
  if (ENABLE_SPEECH) num_handles += 1;   // sub
  if (ENABLE_TOUCH) num_handles += 1;    // pub(timer)

  if (num_handles > 0) {
    rclc_executor_init(&executor, &support.context, num_handles, &allocator);
    if (ENABLE_MOTOR) setup_motor(&node, &support, &executor);
    if (ENABLE_CAMERA) setup_camera(&node, &support, &executor);
    if (ENABLE_BATTERY) setup_battery(&node, &support, &executor);
    if (ENABLE_SPEECH) setup_speech(&node, &support, &executor);
    if (ENABLE_TOUCH) setup_touch(&node, &support, &executor);
  } else {
    Serial.println("[WARN] All topics are disabled in config.h");
  }

  snprintf(speech_buf, sizeof(speech_buf), "ROS 2 Ready!");
  avatar.setSpeechText(speech_buf);

  Serial.println("[DEBUG] Setup completely finished! Entering loop().");
}

void loop() {
  M5StackChan.update();
  if (num_handles > 0) {
    rcl_ret_t ret = rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));
    if (ret != RCL_RET_OK && ret != RCL_RET_TIMEOUT) {
      Serial.printf("[ERROR] Executor spin failed! Error code: %d\n", ret);
      delay(100);
    }
  }
}