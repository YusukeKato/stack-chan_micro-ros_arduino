# stack-chan_micro-ros_arduino
StackChan + micro-ROS + Arduino

<img width="320" height="240" alt="stack-chan_micro-ros_arduino_servo-motor" src="https://github.com/user-attachments/assets/99937d38-68b9-4965-a578-3f4c2f4793d5" />

## Target StackChan
- [M5Stack/StackChan](https://docs.m5stack.com/en/StackChan)

## Development Environment
Verified on the following versions.
Other versions (e.g. Ubuntu 26.04 / ROS 2 Lyrical, or newer board packages and libraries) may not work.

- Ubuntu 24.04 LTS
- ROS 2 Jazzy
- Docker 29.5.0
- Arduino IDE 2.3.8, 2.3.9 / arduino-cli 1.6.0

| Type | Name | Version |
|---|---|---|
| Board package | M5Stack (`m5stack:esp32`) | 3.3.7 |
| Board | M5CoreS3 (`m5stack:esp32:m5stack_cores3`) | default options |
| Library | micro_ros_arduino | 3.0.0-iron |
| Library | M5StackChan | 1.0.1 |
| Library | M5Unified | 0.2.16 |
| Library | M5GFX | 0.2.22 |

## Setup
- [ROS 2 Jazzy Installation](https://docs.ros.org/en/jazzy/Installation.html)
- [StackChan Arduino Sample Program Build and Flash](https://docs.m5stack.com/en/arduino/stackchan/program)
- [micro-ROS/micro_ros_arduino](https://github.com/micro-ROS/micro_ros_arduino)

## Usage
1. Clone this repository.

```sh
git clone https://github.com/YusukeKato/stack-chan_micro-ros_arduino.git
```

2. Copy `config.h.example` to `config.h` and edit it to match your environment.
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

### Display Log
The screen shows logs: boot progress (Wi-Fi, micro-ROS Agent), ready topics, received commands, touch clicks, battery changes, and warnings/errors (yellow/red).
The same logs (plus debug logs) are always printed to Serial.

To turn off the screen log, set `ENABLE_DISPLAY_LOG = false` in `config.h`.

### Motor
Control the motor by specifying the angle.

```sh
ros2 topic pub --once /stackchan/joint_commands std_msgs/msg/Int32MultiArray "{data: [200, 500]}"
```

<img width="320" height="240" alt="control stack-chan motor" src="https://github.com/user-attachments/assets/99937d38-68b9-4965-a578-3f4c2f4793d5" />

<br>  
<br>

Get the current motor angle.

```sh
ros2 topic echo /stackchan/joint_states
```

```sh
# output
layout:
  dim: []
  data_offset: 0
data:
- 331
- 300
```

<img width="340" height="192" alt="get stack-chan motor angle" src="https://github.com/user-attachments/assets/8355e25b-2884-4712-9fd0-0706c9e662dc" />

### Camera
Get the publish rate of the camera image topic.

```sh
ros2 topic hz /stackchan/camera/image/compressed
```

View the camera image topic.

```sh
ros2 run rqt_image_view rqt_image_view
```

> **Note:**
> If `rqt_image_view` doesn't work properly on Wayland, please switch to Xorg.

<img width="333" height="306" alt="rqt_image_view stack-chan camera image topic" src="https://github.com/user-attachments/assets/4afe2943-41d6-4adb-8c30-17c6df6c2393" />

### Battery
Get the battery voltage, remaining level (%), and charging status.

```sh
ros2 topic echo /stackchan/battery_state
```

```sh
# output
header:
  stamp:
    sec: 1778980809
    nanosec: 0
  frame_id: base_link
voltage: 4.205999851226807  # Voltage [V]
temperature: 0.0
current: 0.0
charge: 0.0
capacity: 0.0
design_capacity: 0.0
percentage: 0.9300000071525574  # remaining level [0.0-1.0]
power_supply_status: 1  # 1:charging, 2:discharging
power_supply_health: 0
power_supply_technology: 0
present: false
cell_voltage: []
cell_temperature: []
location: ''
serial_number: ''
```

### Touch
Get touch sensor values.

```sh
ros2 topic echo /stackchan/touch_intensities
```

```sh
# output
---
layout:
  dim: []
  data_offset: 0
data:
- 3  # Front
- 0  # Middle
- 1  # Back
---
```

<img width="341" height="192" alt="stack-chan_micro-ros_arduino_touch" src="https://github.com/user-attachments/assets/5ee1efc9-c074-4b66-aa36-51406f1d2235" />

### IMU
Get the IMU data.

> **Note1:**
>  M5StackChan(X-right, Y-down, Z-backward)
>  -> ROS 2(X-forward, Y-left, Z-up)

> **Note2:**
> Same axis mapping for linear and angular, but different sign inversions.

```sh
ros2 topic echo /stackchan/imu
```

```sh
# output
header:
  stamp:
    sec: 1780211663
    nanosec: 838000000
  frame_id: imu_link
orientation:
...
orientation_covariance:
...
angular_velocity:
  x: 0.059654808417774936
  y: 0.08096009713840883
  z: -0.0010652644360316954
angular_velocity_covariance:
...
linear_acceleration:
  x: -0.3208230137825012
  y: 0.1412578970193863
  z: -9.68694019317627
linear_acceleration_covariance:
...
```

<img width="340" height="192" alt="stackchan_microros_imu" src="https://github.com/user-attachments/assets/856985cd-acbd-402e-aae8-f8a1eef8115a" />

## Note
- You can check all logs, including debug logs, in the Arduino IDE Serial Monitor (115200 baud).

## File Structure
```
stack-chan_micro-ros_arduino.ino  # main: Wi-Fi, micro-ROS node, executor
config.h.example                  # copy to config.h (Wi-Fi, agent, topic/log on/off)
src/
  display_log.h/.cpp  # logs to Serial and the screen
  motor.h/.cpp        # /stackchan/joint_states, /stackchan/joint_commands
  camera.h/.cpp       # /stackchan/camera/image/compressed
  battery.h/.cpp      # /stackchan/battery_state
  touch.h/.cpp        # /stackchan/touch_intensities
  imu.h/.cpp          # /stackchan/imu
```

## Format
```sh
clang-format -i -style=file *.ino src/*.cpp src/*.h
```
