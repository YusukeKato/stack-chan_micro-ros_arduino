#include "imu.h"

#include <Arduino.h>
#include <M5StackChan.h>
#include <sensor_msgs/msg/imu.h>

#include "display_log.h"
#include "ros_utils.h"

static rcl_publisher_t imu_pub;
static sensor_msgs__msg__Imu imu_msg;
static rcl_timer_t imu_timer;

static void imu_timer_callback(rcl_timer_t *timer, int64_t last_call_time) {
  if (timer == NULL) return;
  M5.Imu.update();
  auto data = M5.Imu.getImuData();

  // [G] -> [m/s^2]
  imu_msg.linear_acceleration.x = -data.accel.z * 9.80665f;
  imu_msg.linear_acceleration.y = -data.accel.x * 9.80665f;
  imu_msg.linear_acceleration.z = -data.accel.y * 9.80665f;
  // [deg/s] -> [rad/s]
  imu_msg.angular_velocity.x = data.gyro.z * DEG_TO_RAD;
  imu_msg.angular_velocity.y = data.gyro.x * DEG_TO_RAD;
  imu_msg.angular_velocity.z = data.gyro.y * DEG_TO_RAD;

  set_stamp(&imu_msg.header.stamp);
  publish(&imu_pub, &imu_msg);
  display_status(STATUS_IMU_ACCEL, "IMU accel  x %.1f  y %.1f  z %.1f m/s^2",
                 imu_msg.linear_acceleration.x, imu_msg.linear_acceleration.y,
                 imu_msg.linear_acceleration.z);
  display_status(STATUS_IMU_GYRO, "IMU gyro  x %.1f  y %.1f  z %.1f rad/s",
                 imu_msg.angular_velocity.x, imu_msg.angular_velocity.y,
                 imu_msg.angular_velocity.z);
}

void setup_imu(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor) {
  rclc_publisher_init_default(&imu_pub, node, ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Imu),
                              "stackchan/imu");
  set_string(&imu_msg.header.frame_id, "imu_link");
  rclc_timer_init_default(&imu_timer, support, RCL_MS_TO_NS(100), imu_timer_callback);
  rclc_executor_add_timer(executor, &imu_timer);
  display_log("IMU: ready");
}
