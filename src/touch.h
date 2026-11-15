#ifndef STACKCHAN_TOUCH_H
#define STACKCHAN_TOUCH_H

#include <rclc/executor.h>
#include <rclc/rclc.h>

// Register touch publishers/subscribers/timers to the executor.
void setup_touch(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor);

#endif  // STACKCHAN_TOUCH_H
