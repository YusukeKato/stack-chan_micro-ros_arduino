#ifndef STACKCHAN_CAMERA_H
#define STACKCHAN_CAMERA_H

#include <rclc/executor.h>
#include <rclc/rclc.h>

// Initialize camera hardware. Call before Wi-Fi setup.
bool init_camera_hardware();

// Register camera publishers/subscribers/timers to the executor.
void setup_camera(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor);

#endif  // STACKCHAN_CAMERA_H
