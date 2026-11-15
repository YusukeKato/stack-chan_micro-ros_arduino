#include "imu.h"

#include <Arduino.h>
#include <M5StackChan.h>
#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <sensor_msgs/msg/imu.h>

#include "display_log.h"

static rcl_publisher_t imu_pub;
static sensor_msgs__msg__Imu imu_msg;
static rcl_timer_t imu_timer;

static void imu_timer_callback(rcl_timer_t *timer, int64_t last_call_time) {
  if (timer != NULL) {
    M5.Imu.update();
    auto data = M5.Imu.getImuData();

    // G -> m/s^2
    imu_msg.linear_acceleration.x = -data.accel.z * 9.80665f;
    imu_msg.linear_acceleration.y = -data.accel.x * 9.80665f;
    imu_msg.linear_acceleration.z = -data.accel.y * 9.80665f;

    // deg/s -> rad/s
    imu_msg.angular_velocity.x = data.gyro.z * (PI / 180.0f);
    imu_msg.angular_velocity.y = data.gyro.x * (PI / 180.0f);
    imu_msg.angular_velocity.z = data.gyro.y * (PI / 180.0f);

    // timestamp
    int64_t time = rmw_uros_epoch_millis();
    int32_t sec = (int32_t)(time / 1000);
    uint32_t nanosec = (uint32_t)((time % 1000) * 1000000);

    // imu header
    imu_msg.header.stamp.sec = sec;
    imu_msg.header.stamp.nanosec = nanosec;
    imu_msg.header.frame_id.data = (char *)"imu_link";
    imu_msg.header.frame_id.size = strlen("imu_link");
    imu_msg.header.frame_id.capacity = imu_msg.header.frame_id.size + 1;

    rcl_publish(&imu_pub, &imu_msg, NULL);
  }
}

void setup_imu(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor) {
  log_debug("Registering IMU Node...");

  rclc_publisher_init_default(&imu_pub, node, ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Imu),
                              "stackchan/imu");

  rclc_timer_init_default(&imu_timer, support, RCL_MS_TO_NS(100), imu_timer_callback);
  rclc_executor_add_timer(executor, &imu_timer);

  log_info("IMU: ready");
}
