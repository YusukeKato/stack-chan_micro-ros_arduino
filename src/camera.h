#ifndef STACKCHAN_CAMERA_H
#define STACKCHAN_CAMERA_H

#include <rclc/rclc.h>

// Initialize camera hardware. Call before Wi-Fi setup.
bool init_camera_hardware();

// Create the camera publisher.
void setup_camera(rcl_node_t *node);

// Capture and publish when the period has passed. Call every loop().
void update_camera();

#endif  // STACKCHAN_CAMERA_H
