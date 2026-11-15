#ifndef STACKCHAN_SPEAKER_H
#define STACKCHAN_SPEAKER_H

#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <stdint.h>

// Register speaker subscribers to the executor.
// volume: speaker volume (0-255)
void setup_speaker(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor,
                   uint8_t volume);

#endif  // STACKCHAN_SPEAKER_H
