#include <Arduino.h>
#include <Avatar.h>
#include <M5StackChan.h>
#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include "config.h"

rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

m5avatar::Avatar avatar;

// motor: pub(timer), sub
// camera: pub(timer)
// battery: pub(timer)
const unsigned int num_handles = 4;

static char speech_buf[64];

extern bool init_camera_hardware();
extern void setup_motor(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor);
extern void setup_camera(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor);
extern void setup_battery(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor);

void setup() {
  // serial setup
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n--- StackChan Booting ---");

  // init stack-chan
  M5StackChan.begin();
  M5StackChan.Display().setTextSize(2);
  M5StackChan.Motion.goHome();
  delay(1000);

  // init avatar
  avatar.init();
  avatar.setExpression(m5avatar::Expression::Happy);

  if (!init_camera_hardware()) {
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

  snprintf(speech_buf, sizeof(speech_buf), "ROS 2 Ready!");
  avatar.setSpeechText(speech_buf);

  // init micro-ROS core
  allocator = rcl_get_default_allocator();
  rclc_support_init(&support, 0, NULL, &allocator);
  rclc_node_init_default(&node, "stackchan_node", "", &support);

  // set micro-ROS executor
  rclc_executor_init(&executor, &support.context, num_handles, &allocator);

  // init node
  setup_motor(&node, &support, &executor);
  setup_camera(&node, &support, &executor);
  setup_battery(&node, &support, &executor);

  Serial.println("[DEBUG] Setup completely finished! Entering loop().");
}

void loop() {
  M5StackChan.update();
  rcl_ret_t ret = rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));
  if (ret != RCL_RET_OK && ret != RCL_RET_TIMEOUT) {
    Serial.printf("[ERROR] Executor spin failed! Error code: %d\n", ret);
    delay(100);
  }
}