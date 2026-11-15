#ifndef STACKCHAN_MOTOR_H
#define STACKCHAN_MOTOR_H

#include <rclc/executor.h>
#include <rclc/rclc.h>

// Register motor publishers/subscribers/timers to the executor.
void setup_motor(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor);

#endif  // STACKCHAN_MOTOR_H
