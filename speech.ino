#include <Arduino.h>
#include <Avatar.h>
#include <M5Unified.h>
#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <std_msgs/msg/string.h>

extern m5avatar::Avatar avatar;

rcl_subscription_t speech_sub;
std_msgs__msg__String speech_msg;

const int MAX_SPEECH_LENGTH = 256;
char speech_buffer[MAX_SPEECH_LENGTH];

// Audio configuration
const int SAMPLE_RATE = 16000;
const int BUFFER_SIZE = 512;
int16_t audio_buffer[BUFFER_SIZE];

float pitch = 500.0;             // voice pitch
float volume_gain = 12000.0;     // voice volume
const int VOWEL_FRAMES = 5;      // duration of each vowel
const int CONSONANT_FRAMES = 2;  // duration of silence for consonants

// reference:
// https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html
class BiquadFilter {
 private:
  float b0, b1, b2, a1, a2;
  float x1, x2, y1, y2;

 public:
  BiquadFilter() : x1(0), x2(0), y1(0), y2(0) {}
  void setBandpass(float freq, float q) {
    float w0 = 2.0 * PI * freq / SAMPLE_RATE;
    float alpha = sin(w0) / (2.0 * q);
    float a0 = 1.0 + alpha;
    b0 = alpha / a0;
    b1 = 0.0;
    b2 = -alpha / a0;
    a1 = -2.0 * cos(w0) / a0;
    a2 = (1.0 - alpha) / a0;
  }
  float process(float x) {
    float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1;
    x1 = x;
    y2 = y1;
    y1 = y;
    return y;
  }
};

BiquadFilter f1, f2;
float phase = 0.0;

// Japanese vowel formant frequencies [F1, F2]
// reference:
// https://speech-lab.org/proceedings/ASJmeeting/ASJ2006S/pdf/0124_2-4-13.pdf
const float VOWEL_FREQ[5][2] = {
    {800.0, 1200.0},  // a
    {300.0, 2300.0},  // i
    {300.0, 1200.0},  // u
    {500.0, 1900.0},  // e
    {500.0, 800.0}    // o
};

TaskHandle_t speechTaskHandle = NULL;
char current_speech_text[MAX_SPEECH_LENGTH];

void speechTask(void *pvParameters) {
  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // wait text
    avatar.setSpeechText(current_speech_text);
    int len = strlen(current_speech_text);
    // iterate through each character
    for (int idx = 0; idx < len; idx++) {
      char c = tolower(current_speech_text[idx]);
      int vowel_idx = -1;
      if (c == 'a')
        vowel_idx = 0;
      else if (c == 'i')
        vowel_idx = 1;
      else if (c == 'u')
        vowel_idx = 2;
      else if (c == 'e')
        vowel_idx = 3;
      else if (c == 'o')
        vowel_idx = 4;
      // generate vowel
      if (vowel_idx != -1) {
        f1.setBandpass(VOWEL_FREQ[vowel_idx][0], 8.0);
        f2.setBandpass(VOWEL_FREQ[vowel_idx][1], 10.0);
        for (int frame = 0; frame < VOWEL_FRAMES; frame++) {
          float phaseIncrement = pitch / SAMPLE_RATE;
          for (int i = 0; i < BUFFER_SIZE; i++) {
            phase += phaseIncrement;
            if (phase >= 1.0) phase -= 1.0;
            float source = (phase < 0.3) ? 1.0 : -1.0;
            float out1 = f1.process(source);
            float out2 = f2.process(source);
            float amp = 1.0;
            // fade-in
            if (frame == 0) {
              if (i < BUFFER_SIZE / 2) {
                amp = (float)i / (BUFFER_SIZE / 2);
              }
            }
            // fade-out
            if (frame == VOWEL_FRAMES - 1) {
              if (i > BUFFER_SIZE / 2) {
                amp = 1.0 - (float)(i - BUFFER_SIZE / 2) / (BUFFER_SIZE / 2);
              }
            }
            float mixed = (out1 + out2 * 0.6) * amp;
            audio_buffer[i] = (int16_t)(mixed * volume_gain);
          }
          M5.Speaker.playRaw(audio_buffer, BUFFER_SIZE, SAMPLE_RATE, false, 1, 0);
        }
      } else {  // generate consonants
        memset(audio_buffer, 0, sizeof(audio_buffer));
        for (int frame = 0; frame < CONSONANT_FRAMES; frame++) {
          M5.Speaker.playRaw(audio_buffer, BUFFER_SIZE, SAMPLE_RATE, false, 1, 0);
        }
      }
    }
    avatar.setSpeechText("");
  }
}

void speech_subscription_callback(const void *msgin) {
  const std_msgs__msg__String *msg = (const std_msgs__msg__String *)msgin;
  if (msg->data.size > 0) {
    Serial.printf("[DEBUG] Received Text: %s\n", msg->data.data);
    strncpy(current_speech_text, msg->data.data, MAX_SPEECH_LENGTH - 1);
    current_speech_text[MAX_SPEECH_LENGTH - 1] = '\0';
    if (speechTaskHandle != NULL) {
      xTaskNotifyGive(speechTaskHandle);
    }
  }
}

void setup_speech(rcl_node_t *node, rclc_support_t *support, rclc_executor_t *executor) {
  Serial.println("[DEBUG] Registering Custom Speech Node...");
  // create background speech task
  xTaskCreatePinnedToCore(speechTask, "SpeechTask", 4096, NULL, 1, &speechTaskHandle, 0);
  speech_msg.data.data = speech_buffer;
  speech_msg.data.capacity = MAX_SPEECH_LENGTH;
  speech_msg.data.size = 0;
  rclc_subscription_init_default(&speech_sub, node,
                                 ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
                                 "stackchan/speech_text");
  rclc_executor_add_subscription(executor, &speech_sub, &speech_msg, &speech_subscription_callback,
                                 ON_NEW_DATA);
  Serial.println("[DEBUG] Custom Speech Node Registered!");
}