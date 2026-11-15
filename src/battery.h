#ifndef STACKCHAN_BATTERY_H
#define STACKCHAN_BATTERY_H

#include <rclc/rclc.h>

// Create the battery publisher.
void setup_battery(rcl_node_t *node);

// Publish when the period has passed. Call every loop().
void update_battery();

#endif  // STACKCHAN_BATTERY_H
