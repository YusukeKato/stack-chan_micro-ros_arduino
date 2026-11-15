#include "motor.h"

#include <Arduino.h>
#include <M5StackChan.h>
#include <sensor_msgs/msg/joint_state.h>

#include "display_log.h"
#include "ros_utils.h"

// Joints: yaw (positive = left), pitch (positive = up). Angles in ROS are [rad].
// M5StackChan uses [0.1 deg]: yaw -1280..1280, pitch 0..900.
static const int JOINT_COUNT = 2;
static const int YAW = 0;
static const int PITCH = 1;
static const char *JOINT_NAMES[JOINT_COUNT] = {"yaw_joint", "pitch_joint"};
static const int ANGLE_MIN[JOINT_COUNT] = {-1280, 0};   // [0.1 deg]
static const int ANGLE_MAX[JOINT_COUNT] = {1280, 900};  // [0.1 deg]

static rcl_publisher_t joint_pub;
static rcl_subscription_t joint_sub;
static rcl_timer_t motor_timer;

// publish message buffers
static sensor_msgs__msg__JointState pub_msg;
static rosidl_runtime_c__String pub_names[JOINT_COUNT];
static double pub_position[JOINT_COUNT];

// subscribe message buffers (up to JOINT_COUNT joints, name length < NAME_CAPACITY)
static const int NAME_CAPACITY = 32;
static sensor_msgs__msg__JointState sub_msg;
static char sub_frame_id[NAME_CAPACITY];
static rosidl_runtime_c__String sub_names[JOINT_COUNT];
static char sub_name_data[JOINT_COUNT][NAME_CAPACITY];
static double sub_position[JOINT_COUNT];
static double sub_velocity[JOINT_COUNT];
static double sub_effort[JOINT_COUNT];

static int rad_to_servo(double rad) { return (int)lround(rad * 1800.0 / PI); }
static double servo_to_rad(int angle) { return angle * PI / 1800.0; }

static int find_joint(const rosidl_runtime_c__String &name) {
  for (int j = 0; j < JOINT_COUNT; j++) {
    if (strcmp(name.data, JOINT_NAMES[j]) == 0) return j;
  }
  return -1;
}

// subscribe target joint angles [rad] & move
// - with names: only the named joints move (e.g. {name: [pitch_joint], position: [0.3]})
// - without names: position = [yaw, pitch]
static void joint_subscription_callback(const void *msgin) {
  const sensor_msgs__msg__JointState *msg = (const sensor_msgs__msg__JointState *)msgin;
  bool has_target[JOINT_COUNT] = {false, false};
  double target_rad[JOINT_COUNT] = {0.0, 0.0};

  if (msg->name.size == 0) {
    if (msg->position.size < JOINT_COUNT) {
      display_log("[WARN] Motor: need %d positions without names, got %u", JOINT_COUNT,
                  (unsigned)msg->position.size);
      return;
    }
    for (int j = 0; j < JOINT_COUNT; j++) {
      has_target[j] = true;
      target_rad[j] = msg->position.data[j];
    }
  } else {
    if (msg->position.size != msg->name.size) {
      display_log("[WARN] Motor: name and position sizes differ (%u, %u)", (unsigned)msg->name.size,
                  (unsigned)msg->position.size);
      return;
    }
    for (size_t i = 0; i < msg->name.size; i++) {
      int j = find_joint(msg->name.data[i]);
      if (j < 0) {
        display_log("[WARN] Motor: unknown joint '%s'", msg->name.data[i].data);
        continue;
      }
      has_target[j] = true;
      target_rad[j] = msg->position.data[i];
    }
  }

  char cmd_text[JOINT_COUNT][16] = {"-", "-"};
  for (int j = 0; j < JOINT_COUNT; j++) {
    if (!has_target[j]) continue;
    int angle = rad_to_servo(target_rad[j]);
    int clamped = constrain(angle, ANGLE_MIN[j], ANGLE_MAX[j]);
    if (clamped != angle) {
      display_log("[WARN] Motor: %s %.2f rad out of range", JOINT_NAMES[j], target_rad[j]);
    }
    snprintf(cmd_text[j], sizeof(cmd_text[j]), "%.2f", servo_to_rad(clamped));
    if (j == YAW) {
      M5StackChan.Motion.moveYaw(clamped);
    } else {
      M5StackChan.Motion.movePitch(clamped);
    }
  }
  display_log("Motor: cmd yaw %s pitch %s rad", cmd_text[YAW], cmd_text[PITCH]);
}

// publish current joint angles [rad]
static void motor_timer_callback(rcl_timer_t *timer, int64_t last_call_time) {
  if (timer == NULL) return;
  int current[JOINT_COUNT] = {M5StackChan.Motion.getCurrentYawAngle(),
                              M5StackChan.Motion.getCurrentPitchAngle()};
  for (int j = 0; j < JOINT_COUNT; j++) pub_position[j] = servo_to_rad(current[j]);
  set_stamp(&pub_msg.header.stamp);
  publish(&joint_pub, &pub_msg, "Motor");
  // [0.1 deg] -> [deg]
  display_status(STATUS_JOINT, "Joint  yaw %.1f  pitch %.1f deg", current[YAW] / 10.0f,
                 current[PITCH] / 10.0f);
}

void setup_motor(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor) {
  // publisher
  rclc_publisher_init_default(&joint_pub, node,
                              ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, JointState),
                              "stackchan/joint_states");
  // subscriber
  rclc_subscription_init_default(&joint_sub, node,
                                 ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, JointState),
                                 "stackchan/joint_commands");

  // publish msg buffer setup (velocity/effort are empty)
  for (int j = 0; j < JOINT_COUNT; j++) set_string(&pub_names[j], JOINT_NAMES[j]);
  set_string(&pub_msg.header.frame_id, "");
  pub_msg.name.data = pub_names;
  pub_msg.name.size = JOINT_COUNT;
  pub_msg.name.capacity = JOINT_COUNT;
  pub_msg.position.data = pub_position;
  pub_msg.position.size = JOINT_COUNT;
  pub_msg.position.capacity = JOINT_COUNT;

  // subscribe msg buffer setup
  sub_msg.header.frame_id.data = sub_frame_id;
  sub_msg.header.frame_id.size = 0;
  sub_msg.header.frame_id.capacity = NAME_CAPACITY;
  for (int j = 0; j < JOINT_COUNT; j++) {
    sub_names[j].data = sub_name_data[j];
    sub_names[j].size = 0;
    sub_names[j].capacity = NAME_CAPACITY;
  }
  sub_msg.name.data = sub_names;
  sub_msg.name.size = 0;
  sub_msg.name.capacity = JOINT_COUNT;
  sub_msg.position.data = sub_position;
  sub_msg.position.size = 0;
  sub_msg.position.capacity = JOINT_COUNT;
  sub_msg.velocity.data = sub_velocity;
  sub_msg.velocity.size = 0;
  sub_msg.velocity.capacity = JOINT_COUNT;
  sub_msg.effort.data = sub_effort;
  sub_msg.effort.size = 0;
  sub_msg.effort.capacity = JOINT_COUNT;

  // timer setup
  rclc_timer_init_default(&motor_timer, support, RCL_MS_TO_NS(100), motor_timer_callback);
  rclc_executor_add_subscription(executor, &joint_sub, &sub_msg, &joint_subscription_callback,
                                 ON_NEW_DATA);
  rclc_executor_add_timer(executor, &motor_timer);
  display_log("Motor: ready");
}
