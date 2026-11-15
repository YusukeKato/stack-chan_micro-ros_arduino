#include "imu.h"

#include <Arduino.h>
#include <M5StackChan.h>
#include <sensor_msgs/msg/imu.h>

#include "display_log.h"
#include "ros_utils.h"

static rcl_publisher_t imu_pub;
static sensor_msgs__msg__Imu imu_msg;

void update_imu() {
  static uint32_t last_ms = 0;
  if (!every_ms(&last_ms, 100)) return;
  M5.Imu.update();
  auto data = M5.Imu.getImuData();

  // M5Unified axes (looking at the screen): X right, Y up, Z out of the screen.
  // ROS axes (REP-103, robot's view): X forward, Y left, Z up.
  // -> ROS (x, y, z) = M5 (z, x, y). Same mapping for accel and gyro.
  // Accel is proper acceleration: about +9.8 m/s^2 on z when the head is level (REP-145).

  // [G] -> [m/s^2]
  imu_msg.linear_acceleration.x = data.accel.z * 9.80665f;
  imu_msg.linear_acceleration.y = data.accel.x * 9.80665f;
  imu_msg.linear_acceleration.z = data.accel.y * 9.80665f;
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

void setup_imu(rcl_node_t *node) {
  // best effort: no delivery wait (reliable blocks up to 1 s per publish)
  if (!check(
          rclc_publisher_init_best_effort(
              &imu_pub, node, ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Imu), "stackchan/imu"),
          "IMU publisher"))
    return;
  set_string(&imu_msg.header.frame_id, "imu_link");
  // no orientation estimate: tell subscribers to ignore orientation (sensor_msgs/Imu spec)
  imu_msg.orientation_covariance[0] = -1.0;
  display_log("IMU: ready");
}
