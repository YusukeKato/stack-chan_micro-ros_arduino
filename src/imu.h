#ifndef STACKCHAN_IMU_H
#define STACKCHAN_IMU_H

#include <rclc/rclc.h>

// Create the imu publisher.
void setup_imu(rcl_node_t *node);

// Publish when the period has passed. Call every loop().
void update_imu();

#endif  // STACKCHAN_IMU_H
