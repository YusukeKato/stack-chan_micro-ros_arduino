#ifndef STACKCHAN_MOTOR_H
#define STACKCHAN_MOTOR_H

#include <rclc/executor.h>
#include <rclc/rclc.h>

// Create the joint_states publisher and add the joint_commands subscriber to the executor.
void setup_motor(rcl_node_t *node, rclc_executor_t *executor);

// Publish joint_states when the period has passed. Call every loop().
void update_motor();

#endif  // STACKCHAN_MOTOR_H
