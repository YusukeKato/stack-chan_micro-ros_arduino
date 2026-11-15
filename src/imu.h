#ifndef STACKCHAN_IMU_H
#define STACKCHAN_IMU_H

#include <rclc/executor.h>
#include <rclc/rclc.h>

// Register imu publishers/subscribers/timers to the executor.
void setup_imu(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor);

#endif  // STACKCHAN_IMU_H
