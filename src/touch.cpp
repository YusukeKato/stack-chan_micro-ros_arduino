#include "touch.h"

#include <M5StackChan.h>
#include <std_msgs/msg/int32_multi_array.h>

#include "display_log.h"
#include "ros_utils.h"

static rcl_publisher_t touch_pub;
static std_msgs__msg__Int32MultiArray touch_msg;
static int32_t touch_data[3];  // front, middle, back (0: no touch, 1-3: intensity)
static rcl_timer_t touch_timer;

static void touch_timer_callback(rcl_timer_t *timer, int64_t last_call_time) {
  if (timer == NULL) return;
  auto intensities = M5StackChan.TouchSensor.getIntensities();
  for (int i = 0; i < 3; i++) touch_data[i] = intensities[i];
  publish(&touch_pub, &touch_msg);
  display_status(STATUS_TOUCH, "Touch  front %d  middle %d  back %d", (int)touch_data[0],
                 (int)touch_data[1], (int)touch_data[2]);
}

void setup_touch(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor) {
  // best effort: no delivery wait (reliable blocks up to 1 s per publish)
  rclc_publisher_init_best_effort(&touch_pub, node,
                                  ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32MultiArray),
                                  "stackchan/touch_intensities");
  touch_msg.data.data = touch_data;
  touch_msg.data.size = 3;
  touch_msg.data.capacity = 3;
  rclc_timer_init_default(&touch_timer, support, RCL_MS_TO_NS(100), touch_timer_callback);
  rclc_executor_add_timer(executor, &touch_timer);
  display_log("Touch: ready");
}
