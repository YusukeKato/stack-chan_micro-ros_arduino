#include "speaker.h"

#include <Arduino.h>
#include <M5StackChan.h>
#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <std_msgs/msg/float32_multi_array.h>

#include "display_log.h"

static const float MAX_FREQUENCY = 20000.0f;  // [Hz]
static const float MAX_DURATION = 10000.0f;   // [ms]

static rcl_subscription_t tone_sub;
static std_msgs__msg__Float32MultiArray tone_msg;
static float tone_data[2];

// subscribe [frequency_hz, duration_ms] & play a tone (non-blocking)
// frequency 0 stops the sound.
static void tone_subscription_callback(const void *msgin) {
  const std_msgs__msg__Float32MultiArray *msg = (const std_msgs__msg__Float32MultiArray *)msgin;
  if (msg->data.size != 2) {
    display_log("[WARN] Speaker: tone needs [Hz, ms], got %u values", (unsigned)msg->data.size);
    return;
  }
  float frequency = msg->data.data[0];
  float duration = msg->data.data[1];
  if (frequency <= 0.0f) {
    M5.Speaker.stop();
    display_log("Speaker: stop");
    return;
  }
  if (frequency > MAX_FREQUENCY || duration <= 0.0f || duration > MAX_DURATION) {
    display_log("[WARN] Speaker: out of range (0-%.0f Hz, 0-%.0f ms)", MAX_FREQUENCY, MAX_DURATION);
    return;
  }
  M5.Speaker.tone(frequency, (uint32_t)duration);
  display_log("Speaker: tone %.0f Hz %.0f ms", frequency, duration);
}

void setup_speaker(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor,
                   uint8_t volume) {
  M5.Speaker.setVolume(volume);
  rclc_subscription_init_default(&tone_sub, node,
                                 ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32MultiArray),
                                 "stackchan/tone");
  tone_msg.data.data = tone_data;
  tone_msg.data.size = 0;
  tone_msg.data.capacity = 2;
  rclc_executor_add_subscription(executor, &tone_sub, &tone_msg, &tone_subscription_callback,
                                 ON_NEW_DATA);
  display_log("Speaker: ready (volume %u)", (unsigned)volume);
}
