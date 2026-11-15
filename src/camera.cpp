// reference:
// https://docs.m5stack.com/ja/arduino/stackchan/camera

#include "camera.h"

#include <M5StackChan.h>
#include <sensor_msgs/msg/compressed_image.h>

#include "display_log.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "ros_utils.h"

// Tuning
// - Frame size is limited by micro-ROS: one message must fit in the output buffer
//   (UDP MTU 512 B x history 4 = about 2 KB in micro_ros_arduino 3.0.0-iron).
//   QQVGA (160 x 120) JPEG fits; QVGA (320 x 240) does not.
// - Higher JPEG_QUALITY = larger frames. Check the rate and size on the screen (Camera line).
static const int CAMERA_PERIOD_MS = 200;  // 5 Hz
static const int JPEG_QUALITY = 30;       // 1-100

static rcl_publisher_t camera_pub;
static sensor_msgs__msg__CompressedImage image_msg;
static rcl_timer_t camera_timer;

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
static void camera_timer_callback(rcl_timer_t *timer, int64_t last_call_time) {
  if (timer == NULL) return;
  camera_fb_t *fb = esp_camera_fb_get();
  if (fb == NULL) return;
  uint8_t *jpg = NULL;
  size_t jpg_len = 0;
  if (fmt2jpg(fb->buf, fb->len, fb->width, fb->height, fb->format, JPEG_QUALITY, &jpg, &jpg_len)) {
    image_msg.data.data = jpg;
    image_msg.data.size = jpg_len;
    image_msg.data.capacity = jpg_len;
    set_stamp(&image_msg.header.stamp);
    static int published = 0;
    if (publish(&camera_pub, &image_msg)) published++;
    // show publish rate and frame size once per second
    static uint32_t last_status_ms = 0;
    static uint32_t start_ms = millis();
    if (every_ms(&last_status_ms, 1000)) {
      float fps = published * 1000.0f / max(1UL, (unsigned long)(millis() - start_ms));
      display_status(STATUS_CAMERA, "Camera  %.1f fps  %.1f KB", fps, jpg_len / 1024.0f);
      published = 0;
      start_ms = millis();
    }
  } else {
    static uint32_t last_error_ms = 0;
    if (every_ms(&last_error_ms, 1000)) display_log("[ERROR] Camera: JPEG conversion failed");
  }
  free(jpg);
  esp_camera_fb_return(fb);
}

void setup_camera(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor) {
  rclc_publisher_init_default(&camera_pub, node,
                              ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, CompressedImage),
                              "stackchan/camera/image/compressed");
  set_string(&image_msg.header.frame_id, "camera_link");
  set_string(&image_msg.format, "jpeg");
  rclc_timer_init_default(&camera_timer, support, RCL_MS_TO_NS(CAMERA_PERIOD_MS),
                          camera_timer_callback);
  rclc_executor_add_timer(executor, &camera_timer);
  display_log("Camera: ready");
}
