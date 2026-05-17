#include <Arduino.h>
#include <M5StackChan.h>
#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32_multi_array.h>

rcl_publisher_t angle_pub;
rcl_subscription_t angle_sub;
std_msgs__msg__Int32MultiArray pub_msg;
std_msgs__msg__Int32MultiArray sub_msg;
int32_t pub_data[2] = {0, 0};
int32_t sub_data[2] = {0, 0};
rcl_timer_t motor_timer;

// subscribe target angle & move
void angle_subscription_callback(const void * msgin) {
  const std_msgs__msg__Int32MultiArray * msg = (const std_msgs__msg__Int32MultiArray *)msgin;
  if (msg->data.size >= 2) {
    int32_t target_yaw = msg->data.data[0];
    int32_t target_pitch = msg->data.data[1];
    Serial.printf("[DEBUG] Received joint_commands -> Yaw: %d, Pitch: %d\n", target_yaw, target_pitch);
    target_yaw = max(-1280, min(1280, (int)target_yaw));
    target_pitch = max(0, min(900, (int)target_pitch));
    M5StackChan.Motion.move(target_yaw, target_pitch);
  } else {
    Serial.println("[WARN] Received joint_commands with invalid data size!");
  }
}

// publish current angle
void motor_timer_callback(rcl_timer_t * timer, int64_t last_call_time) {
  if (timer != NULL) {
    int32_t current_yaw = M5StackChan.Motion.getCurrentXAngle();
    int32_t current_pitch = M5StackChan.Motion.getCurrentYAngle();
    pub_msg.data.data[0] = current_yaw;
    pub_msg.data.data[1] = current_pitch;
    rcl_publish(&angle_pub, &pub_msg, NULL);
    /*
    rcl_ret_t ret = rcl_publish(&angle_pub, &pub_msg, NULL);
    if (ret != RCL_RET_OK) {
      Serial.printf("[WARN] Motor publish failed. Error: %d\n", ret);
    }
    */
    
    // print log
    M5StackChan.Display().setCursor(0, 80);
    M5StackChan.Display().setTextColor(TFT_WHITE, TFT_BLACK); 
    M5StackChan.Display().printf("  Yaw   : %4d    \n", current_yaw);
    M5StackChan.Display().printf("  Pitch : %4d    \n", current_pitch);
  }
}

void setup_motor(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor) {
  Serial.println("[DEBUG] Registering Motor Node...");
  // publisher
  rclc_publisher_init_default(
    &angle_pub, node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32MultiArray),
    "stackchan/joint_states");
  // subscriber
  rclc_subscription_init_default(
    &angle_sub, node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32MultiArray),
    "stackchan/joint_commands");
  // msg buffer setup
  pub_msg.data.capacity = 2;
  pub_msg.data.size = 2;
  pub_msg.data.data = pub_data;
  sub_msg.data.capacity = 2;
  sub_msg.data.size = 0;
  sub_msg.data.data = sub_data;
  // timer setup
  rclc_timer_init_default(&motor_timer, support, RCL_MS_TO_NS(100), motor_timer_callback);
  rclc_executor_add_subscription(executor, &angle_sub, &sub_msg, &angle_subscription_callback, ON_NEW_DATA);
  rclc_executor_add_timer(executor, &motor_timer);
  Serial.println("[DEBUG] Motor Node Registered!");
}