#include <Arduino.h>
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

// motor: pub, sub
// camera: pub
const unsigned int num_handles = 3;

extern bool init_camera_hardware();
extern void setup_motor(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor);
extern void setup_camera(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor);

void setup() {
  // serial setup
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n--- StackChan Booting ---");

  // init stack-chan
  M5StackChan.begin();
  M5StackChan.Display().setTextSize(3);
  M5StackChan.Motion.goHome();
  delay(1000);

  if (!init_camera_hardware()) {
    Serial.println("[ERROR] Failed to initialize camera hardware!");
  }

  M5StackChan.Display().printf("> Connecting Wi-Fi...\n");
  set_microros_wifi_transports(ssid, password, agent_ip_address, agent_port);
  delay(2000);

  M5StackChan.Display().clear();
  M5StackChan.Display().setCursor(0, 0);
  M5StackChan.Display().setTextColor(TFT_GREENYELLOW);
  M5StackChan.Display().printf("> ROS 2 Ready!\n");
  M5StackChan.Display().setTextColor(TFT_WHITE);

  // init micro-ROS core
  allocator = rcl_get_default_allocator();
  rclc_support_init(&support, 0, NULL, &allocator);
  rclc_node_init_default(&node, "stackchan_node", "", &support);

  // set micro-ROS executor
  rclc_executor_init(&executor, &support.context, num_handles, &allocator);

  // init node
  setup_motor(&node, &support, &executor);
  setup_camera(&node, &support, &executor);

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