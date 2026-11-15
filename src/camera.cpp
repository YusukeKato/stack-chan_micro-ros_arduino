// reference:
// https://docs.m5stack.com/ja/arduino/stackchan/camera

#include "camera.h"

#include <M5StackChan.h>
#include <rmw_microros/rmw_microros.h>
#include <sensor_msgs/msg/compressed_image.h>

#include "display_log.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "ros_utils.h"

// Tuning
// - One frame must fit in the reliable output stream of micro_ros_arduino 3.0.0-iron
//   (UDP MTU 512 B x history 4 = about 2 KB). Frames close to that limit (about 1.9 KB)
//   produced broken fragments that crashed the micro-ROS Agent.
// - JPEG size depends on the scene, so the quality is adjusted per frame to stay below
//   MAX_JPEG_BYTES (between MIN_JPEG_QUALITY and MAX_JPEG_QUALITY). Larger frames are dropped.
// - The screen shows the size and quality (Camera line). Check the rate with ros2 topic hz.
static const int CAMERA_PERIOD_MS = 200;  // 5 Hz
static const int MAX_JPEG_QUALITY = 30;   // 1-100, higher = better and larger
static const int MIN_JPEG_QUALITY = 5;
static const size_t MAX_JPEG_BYTES = 1700;  // drop larger frames
static const int PUBLISH_TIMEOUT_MS = 0;    // do not wait for delivery (1000 ms by default)

static rcl_publisher_t camera_pub;
static sensor_msgs__msg__CompressedImage image_msg;

static camera_config_t camera_config = {
    .pin_pwdn = -1,
    .pin_reset = -1,
    .pin_xclk = -1,
    .pin_sscb_sda = -1,
    .pin_sscb_scl = -1,
    .pin_d7 = 47,
    .pin_d6 = 48,
    .pin_d5 = 16,
    .pin_d4 = 15,
    .pin_d3 = 42,
    .pin_d2 = 41,
    .pin_d1 = 40,
    .pin_d0 = 39,
    .pin_vsync = 46,
    .pin_href = 38,
    .pin_pclk = 45,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_RGB565,
    .frame_size = FRAMESIZE_QQVGA,  // 160 x 120
    .jpeg_quality = 0,
    // 2 buffers + GRAB_LATEST: always publish the newest frame (lower latency than 1 buffer +
    // GRAB_WHEN_EMPTY, which returns a frame captured one period earlier)
    .fb_count = 2,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_LATEST,
    .sccb_i2c_port = -1,
};

bool init_camera_hardware() {
  camera_config.sccb_i2c_port = M5.In_I2C.getPort();
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    display_log("[ERROR] Camera: init failed (0x%x)", err);
    return false;
  }
  display_log("Camera: hardware ready");
  return true;
}

// capture, convert to JPEG and publish
void update_camera() {
  static uint32_t last_ms = 0;
  if (!every_ms(&last_ms, CAMERA_PERIOD_MS)) return;
  camera_fb_t *fb = esp_camera_fb_get();
  if (fb == NULL) return;
  static int quality = MAX_JPEG_QUALITY;
  uint8_t *jpg = NULL;
  size_t jpg_len = 0;
  if (fmt2jpg(fb->buf, fb->len, fb->width, fb->height, fb->format, quality, &jpg, &jpg_len)) {
    if (jpg_len <= MAX_JPEG_BYTES) {
      image_msg.data.data = jpg;
      image_msg.data.size = jpg_len;
      image_msg.data.capacity = jpg_len;
      set_stamp(&image_msg.header.stamp);
      publish(&camera_pub, &image_msg);  // false only means "not confirmed yet"
    }
    // adjust quality for the next frame: down fast when too large, up slowly when small
    if (jpg_len > MAX_JPEG_BYTES) {
      quality = max(MIN_JPEG_QUALITY, quality - 5);
    } else if (jpg_len < MAX_JPEG_BYTES * 3 / 4) {
      quality = min(MAX_JPEG_QUALITY, quality + 1);
    }
    display_status(STATUS_CAMERA, "Camera  %.1f KB  q%d", jpg_len / 1024.0f, quality);
  } else {
    static uint32_t last_error_ms = 0;
    if (every_ms(&last_error_ms, 1000)) display_log("[ERROR] Camera: JPEG conversion failed");
  }
  free(jpg);
  esp_camera_fb_return(fb);
}

void setup_camera(rcl_node_t *node) {
  rcl_ret_t ret = rclc_publisher_init_default(
      &camera_pub, node, ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, CompressedImage),
      "stackchan/camera/image/compressed");
  if (!check(ret, "Camera publisher")) return;
  rmw_uros_set_publisher_session_timeout(rcl_publisher_get_rmw_handle(&camera_pub),
                                         PUBLISH_TIMEOUT_MS);
  set_string(&image_msg.header.frame_id, "camera_link");
  set_string(&image_msg.format, "jpeg");
  display_log("Camera: ready");
}
