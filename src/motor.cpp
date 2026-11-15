#include "motor.h"

#include <Arduino.h>
#include <M5StackChan.h>
#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <std_msgs/msg/int32_multi_array.h>

#include "display_log.h"

static rcl_publisher_t angle_pub;
static rcl_subscription_t angle_sub;
static std_msgs__msg__Int32MultiArray pub_msg;
static std_msgs__msg__Int32MultiArray sub_msg;
static int32_t pub_data[2] = {0, 0};
static int32_t sub_data[2] = {0, 0};
static rcl_timer_t motor_timer;

// subscribe target angle & move
static void angle_subscription_callback(const void *msgin) {
  const std_msgs__msg__Int32MultiArray *msg = (const std_msgs__msg__Int32MultiArray *)msgin;
  if (msg->data.size >= 2) {
    int32_t target_yaw = msg->data.data[0];
    int32_t target_pitch = msg->data.data[1];
    display_log("Motor: cmd yaw %d pitch %d", (int)target_yaw, (int)target_pitch);
    target_yaw = max(-1280, min(1280, (int)target_yaw));
    target_pitch = max(0, min(900, (int)target_pitch));
    M5StackChan.Motion.move(target_yaw, target_pitch);
  } else {
    display_log("[WARN] Motor: joint_commands needs 2 values, got %u", (unsigned)msg->data.size);
  }
}

// publish current angle
static void motor_timer_callback(rcl_timer_t *timer, int64_t last_call_time) {
  if (timer != NULL) {
    int32_t current_yaw = M5StackChan.Motion.getCurrentXAngle();
    int32_t current_pitch = M5StackChan.Motion.getCurrentYAngle();
    pub_msg.data.data[0] = current_yaw;
    pub_msg.data.data[1] = current_pitch;
    rcl_publish(&angle_pub, &pub_msg, NULL);
    display_joint_angles(current_yaw / 10.0f, current_pitch / 10.0f);  // [0.1 deg] -> [deg]
  }
}

void setup_motor(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor) {
  // publisher
  rclc_publisher_init_default(&angle_pub, node,
                              ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32MultiArray),
                              "stackchan/joint_states");
  // subscriber
  rclc_subscription_init_default(&angle_sub, node,
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
  rclc_executor_add_subscription(executor, &angle_sub, &sub_msg, &angle_subscription_callback,
                                 ON_NEW_DATA);
  rclc_executor_add_timer(executor, &motor_timer);
  display_log("Motor: ready");
}
