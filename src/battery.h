#ifndef STACKCHAN_BATTERY_H
#define STACKCHAN_BATTERY_H

#include <rclc/executor.h>
#include <rclc/rclc.h>

// Register battery publishers/subscribers/timers to the executor.
void setup_battery(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor);

#endif  // STACKCHAN_BATTERY_H
