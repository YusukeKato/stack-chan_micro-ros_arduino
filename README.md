# stack-chan_micro-ros_arduino
<img width="320" height="240" alt="stack-chan_micro-ros_arduino_servo-motor" src="https://github.com/user-attachments/assets/99937d38-68b9-4965-a578-3f4c2f4793d5" />

## Target StackChan
- [M5Stack/StackChan](https://docs.m5stack.com/en/StackChan)

## Development Environment
- Ubuntu 24.04 LTS
- ROS 2 Jazzy
- Docker 29.5.0
- Arduino IDE 2.3.8
  - micro_ros_arduino 2.0.7-humble
  - M5StackChan 1.0.1

## Setup
- [ROS 2 Jazzy Installation](https://docs.ros.org/en/jazzy/Installation.html)
- [StackChan Arduino Sample Program Build and Flash](https://docs.m5stack.com/en/arduino/stackchan/program)
- [micro-ROS/micro_ros_arduino](https://github.com/micro-ROS/micro_ros_arduino)

## Usage
1. `git clone https://github.com/YusukeKato/stack-chan_micro-ros_arduino.git`
2. Copy `config.h.example` to `config.h` and edit it to match your environment.
3. Upload the program to your StackChan.
4. `docker run -it --rm --net=host microros/micro-ros-agent:jazzy udp4 --port 8888 -v6`
5. Press the reset button on your StackChan to restart it.

### Motor
Control the motor by specifying the angle.

```sh
ros2 topic pub --once /stackchan/joint_commands std_msgs/msg/Int32MultiArray "{data: [200, 500]}"
```

<img width="320" height="240" alt="control stack-chan motor" src="https://github.com/user-attachments/assets/99937d38-68b9-4965-a578-3f4c2f4793d5" />

Get the current motor angle.

```sh
ros2 topic echo /stackchan/joint_states
```

<img width="340" height="192" alt="get stack-chan motor angle" src="https://github.com/user-attachments/assets/8355e25b-2884-4712-9fd0-0706c9e662dc" />

### Camera
Check the publish rate of the camera image topic.

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

## Note
- You can check the debug logs in the Arduino IDE Serial Monitor.
- StackChan may repeatedly reboot if the micro-ROS agent is not running.
