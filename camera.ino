// reference:
// https://docs.m5stack.com/ja/arduino/stackchan/camera

#include <Arduino.h>
#include <M5StackChan.h>
#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <sensor_msgs/msg/compressed_image.h>

#include "esp_camera.h"
#include "img_converters.h"

rcl_publisher_t camera_pub;
sensor_msgs__msg__CompressedImage image_msg;
rcl_timer_t camera_timer;

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
    // image setup
    .pixel_format = PIXFORMAT_RGB565,  // color
    // .pixel_format = PIXFORMAT_GRAYSCALE,  // grayscale
    .frame_size = FRAMESIZE_QQVGA,  // 160 x 120
    // .frame_size   = FRAMESIZE_96X96,  // 96 x 96
    .jpeg_quality = 0,
    .fb_count = 1,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    .sccb_i2c_port = -1,
};

bool init_camera_hardware() {
  Serial.println("[DEBUG] Initializing Camera Hardware...");
  camera_config.sccb_i2c_port = M5.In_I2C.getPort();
  Serial.printf("[DEBUG] Shared I2C Port: %d\n", camera_config.sccb_i2c_port);
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    Serial.printf("[ERROR] Camera Init Failed! Error code: 0x%x\n", err);
    return false;
  }
  Serial.println("[DEBUG] Camera Hardware Init Success!");
  return true;
}

// publish camera image
void camera_timer_callback(rcl_timer_t *timer, int64_t last_call_time) {
  if (timer != NULL) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb) {
      uint8_t *out_jpg = NULL;
      size_t out_jpg_len = 0;
      bool jpeg_converted =
          fmt2jpg(fb->buf, fb->len, fb->width, fb->height, fb->format, 30, &out_jpg, &out_jpg_len);
      if (jpeg_converted && out_jpg != NULL) {
        int64_t time = rmw_uros_epoch_millis();
        image_msg.header.stamp.sec = (int32_t)(time / 1000);
        image_msg.header.stamp.nanosec = (uint32_t)((time % 1000) * 1000000);
        image_msg.format.data = (char *)"jpeg";
        image_msg.format.size = 4;
        image_msg.data.data = out_jpg;
        image_msg.data.size = out_jpg_len;
        image_msg.data.capacity = out_jpg_len;
        rcl_publish(&camera_pub, &image_msg, NULL);
        free(out_jpg);
      } else {
        Serial.println("[ERROR] JPEG conversion failed!");
      }
      esp_camera_fb_return(fb);
    }
  }
}

void setup_camera(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor) {
  Serial.println("[DEBUG] Registering Camera Node...");
  // init publisher
  rclc_publisher_init_default(&camera_pub, node,
                              ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, CompressedImage),
                              "stackchan/camera/image/compressed");
  image_msg.header.frame_id.data = (char *)"camera_link";
  image_msg.header.frame_id.size = strlen("camera_link");
  image_msg.header.frame_id.capacity = image_msg.header.frame_id.size + 1;
  // timer setup
  rclc_timer_init_default(&camera_timer, support, RCL_MS_TO_NS(200), camera_timer_callback);
  rclc_executor_add_timer(executor, &camera_timer);
  Serial.println("[DEBUG] Camera Node Registered!");
}