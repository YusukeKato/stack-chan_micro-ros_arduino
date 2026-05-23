#include <Arduino.h>
#include <M5StackChan.h>
#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <std_msgs/msg/int32_multi_array.h>

rcl_publisher_t touch_pub;
std_msgs__msg__Int32MultiArray touch_msg;
int32_t touch_data[3] = {0, 0, 0};
rcl_timer_t touch_timer;

uint32_t last_touch_time = 0;
bool is_expression_changed = false;

void touch_timer_callback(rcl_timer_t *timer, int64_t last_call_time) {
  if (timer != NULL) {
    // intensities[0]: Front, intensities[1]: Middle, intensities[2]: Back
    auto intensities = M5StackChan.TouchSensor.getIntensities();
    touch_msg.data.data[0] = intensities[0];
    touch_msg.data.data[1] = intensities[1];
    touch_msg.data.data[2] = intensities[2];
    rcl_publish(&touch_pub, &touch_msg, NULL);

    if (M5StackChan.TouchSensor.wasClicked()) {
      avatar.setExpression(m5avatar::Expression::Happy);
      last_touch_time = millis();
      is_expression_changed = true;
    }
    if (is_expression_changed && (millis() - last_touch_time > 3000)) {
      avatar.setExpression(m5avatar::Expression::Neutral);
      is_expression_changed = false;
    }
  }
}

void setup_touch(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor) {
  Serial.println("[DEBUG] Registering Touch Node...");
  rclc_publisher_init_default(&touch_pub, node,
                              ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32MultiArray),
                              "stackchan/touch_intensities");
  touch_msg.data.capacity = 3;
  touch_msg.data.size = 3;
  touch_msg.data.data = touch_data;
  rclc_timer_init_default(&touch_timer, support, RCL_MS_TO_NS(100), touch_timer_callback);
  rclc_executor_add_timer(executor, &touch_timer);
  Serial.println("[DEBUG] Touch Node Registered!");
}