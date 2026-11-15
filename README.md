# stack-chan_micro-ros_arduino
Base firmware to use [M5Stack StackChan](https://docs.m5stack.com/en/StackChan) from ROS 2 via micro-ROS (Arduino).
It only bridges the hardware (motors, camera, sensors, speaker) to ROS 2 topics. Apps (face, speech, behaviors) run on the PC side.

<img width="320" height="240" alt="stack-chan_micro-ros_arduino_servo-motor" src="https://github.com/user-attachments/assets/99937d38-68b9-4965-a578-3f4c2f4793d5" />

## Topics
Node: `stackchan_node`

| Topic | Type | Direction | Rate |
|---|---|---|---|
| `/stackchan/joint_states` | `sensor_msgs/msg/JointState` | publish | 10 Hz |
| `/stackchan/joint_commands` | `sensor_msgs/msg/JointState` | subscribe | - |
| `/stackchan/camera/image/compressed` | `sensor_msgs/msg/CompressedImage` | publish | 5 Hz |
| `/stackchan/battery_state` | `sensor_msgs/msg/BatteryState` | publish | 1 Hz |
| `/stackchan/touch_intensities` | `std_msgs/msg/Int32MultiArray` | publish | 10 Hz |
| `/stackchan/imu` | `sensor_msgs/msg/Imu` | publish | 10 Hz |
| `/stackchan/tone` | `std_msgs/msg/Float32MultiArray` | subscribe | - |

Each feature can be turned on/off in `config.h`.

## Development Environment
Verified on the following versions.
Other versions (e.g. Ubuntu 26.04 / ROS 2 Lyrical, or newer board packages and libraries) may not work.

- Ubuntu 24.04 LTS
- ROS 2 Jazzy
- Docker 29.9.3
- Arduino IDE 2.3.9 / arduino-cli 1.6.0

| Type | Name | Version |
|---|---|---|
| Board package | M5Stack (`m5stack:esp32`) | 3.3.7 |
| Board | M5CoreS3 (`m5stack:esp32:m5stack_cores3`) | default options |
| Library | micro_ros_arduino | 3.0.0-iron |
| Library | M5StackChan | 1.0.1 |
| Library | M5Unified | 0.2.16 |
| Library | M5GFX | 0.2.22 |

References:
- [ROS 2 Jazzy Installation](https://docs.ros.org/en/jazzy/Installation.html)
- [StackChan Arduino Sample Program Build and Flash](https://docs.m5stack.com/en/arduino/stackchan/program)
- [micro-ROS/micro_ros_arduino](https://github.com/micro-ROS/micro_ros_arduino)

## Usage
1. Clone this repository.

```sh
git clone https://github.com/YusukeKato/stack-chan_micro-ros_arduino.git
```

2. Copy `config.h.example` to `config.h` and edit it (Wi-Fi, micro-ROS Agent address, feature on/off).
3. Upload the program to your StackChan with Arduino IDE, or with arduino-cli:

```sh
arduino-cli compile -b m5stack:esp32:m5stack_cores3 .
arduino-cli upload -b m5stack:esp32:m5stack_cores3 -p <PORT> .
```

Replace `<PORT>` with the serial port of your StackChan (e.g. `/dev/ttyACM0`). You can find it with `arduino-cli board list`.

4. Start micro-ROS Agent.

```sh
docker run -it --rm --net=host microros/micro-ros-agent:jazzy udp4 --port 8888 -v6
```

5. Press the reset button on your StackChan to restart it.

## Screen
- Lines 1-5 (fixed, updated in place; values are rounded to fit one line):
  - `Joint  yaw 12.3  pitch 45.0 deg`
  - `Battery  93%  4.21 V  charging`
  - `IMU accel  x -0.3  y 0.1  z -9.7 m/s^2`
  - `IMU gyro  x 0.1  y 0.1  z -0.0 rad/s`
  - `Touch  front 3  middle 0  back 1`
- Line 6 and below (scrolling): boot progress (Wi-Fi, micro-ROS Agent), ready features, received commands, and `[WARN]` / `[ERROR]` messages

The scrolling log is also printed to Serial (115200 baud). The fixed lines are screen only.

## Features
### Motor
Joint angles are in [rad].

| Joint | Range [rad] | Positive direction |
|---|---|---|
| `yaw_joint` | -2.23 to 2.23 (±128°) | left |
| `pitch_joint` | 0.0 to 1.57 (0° to 90°) | up |

Out-of-range angles are clamped. With `name`, only the named joints move. Without `name`, `position` is `[yaw, pitch]`.

```sh
ros2 topic pub --once /stackchan/joint_commands sensor_msgs/msg/JointState "{name: [yaw_joint, pitch_joint], position: [0.5, 0.3]}"
ros2 topic pub --once /stackchan/joint_commands sensor_msgs/msg/JointState "{name: [pitch_joint], position: [0.0]}"
ros2 topic pub --once /stackchan/joint_commands sensor_msgs/msg/JointState "{position: [0.0, 0.0]}"
ros2 topic echo /stackchan/joint_states
```

### Camera
JPEG, 160 x 120.

```sh
ros2 topic hz /stackchan/camera/image/compressed
ros2 run rqt_image_view rqt_image_view
```

> **Note:**
> If `rqt_image_view` doesn't work properly on Wayland, please switch to Xorg.

<img width="333" height="306" alt="rqt_image_view stack-chan camera image topic" src="https://github.com/user-attachments/assets/4afe2943-41d6-4adb-8c30-17c6df6c2393" />

### Battery
`voltage` [V], `current` [A], `percentage` [0.0-1.0], `power_supply_status` (1: charging, 2: discharging).

```sh
ros2 topic echo /stackchan/battery_state
```

### Touch
`data` is `[front, middle, back]`. Each value is 0 (no touch) or 1-3 (intensity).

```sh
ros2 topic echo /stackchan/touch_intensities
```

<img width="341" height="192" alt="stack-chan_micro-ros_arduino_touch" src="https://github.com/user-attachments/assets/5ee1efc9-c074-4b66-aa36-51406f1d2235" />

### IMU
`linear_acceleration` [m/s^2] and `angular_velocity` [rad/s], `frame_id: imu_link`.

> **Note1:**
>  M5StackChan(X-right, Y-down, Z-backward)
>  -> ROS 2(X-forward, Y-left, Z-up)

> **Note2:**
> Same axis mapping for linear and angular, but different sign inversions.

```sh
ros2 topic echo /stackchan/imu
```

<img width="340" height="192" alt="stackchan_microros_imu" src="https://github.com/user-attachments/assets/856985cd-acbd-402e-aae8-f8a1eef8115a" />

### Speaker
Play a tone with `[frequency Hz, duration ms]` (non-blocking). The volume is set by `SPEAKER_VOLUME` in `config.h`.
Range: 0-20000 Hz, 0-10000 ms (out of range is ignored with `[WARN]`). Frequency `0` stops the sound.

```sh
ros2 topic pub --once /stackchan/tone std_msgs/msg/Float32MultiArray "{data: [440.0, 500.0]}"
```

## File Structure
```
stack-chan_micro-ros_arduino.ino  # main: Wi-Fi, micro-ROS node, executor
config.h.example                  # copy to config.h
src/
  motor.h/.cpp        # joint_states, joint_commands
  camera.h/.cpp       # camera/image/compressed
  battery.h/.cpp      # battery_state
  touch.h/.cpp        # touch_intensities
  imu.h/.cpp          # imu
  speaker.h/.cpp      # tone
  display_log.h/.cpp  # logs to Serial and the screen
  ros_utils.h/.cpp    # stamp, string and publish helpers
```

## Format
```sh
clang-format -i -style=file *.ino src/*.cpp src/*.h
```
