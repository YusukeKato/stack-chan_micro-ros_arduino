#ifndef STACKCHAN_TOUCH_H
#define STACKCHAN_TOUCH_H

#include <rclc/rclc.h>

// Create the touch publisher.
void setup_touch(rcl_node_t *node);

// Publish when the period has passed. Call every loop().
void update_touch();

#endif  // STACKCHAN_TOUCH_H
