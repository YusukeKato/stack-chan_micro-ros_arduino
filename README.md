# stack-chan_micro-ros_arduino
<img width="320" height="240" alt="stack-chan_micro-ros_arduino_servo-motor" src="https://github.com/user-attachments/assets/99937d38-68b9-4965-a578-3f4c2f4793d5" />

## Target StackChan
- [M5Stack/StackChan](https://docs.m5stack.com/en/StackChan)

## Development Environment
- Ubuntu 24.04 LTS
- ROS 2 Jazzy
- Arduino IDE 2.3.8

## Setup
- [ROS 2 Jazzy Installation](https://docs.ros.org/en/jazzy/Installation.html)
- [StackChan Arduino Sample Program Build and Flash](https://docs.m5stack.com/en/arduino/stackchan/program)
- [micro-ROS/micro_ros_arduino](https://github.com/micro-ROS/micro_ros_arduino)

## Usage
1. Copy `config.h.example` to `config.h` and edit it to match your environment.
2. Upload the program to your StackChan.
3. Run the following command on your PC: `docker run -it --rm --net=host microros/micro-ros-agent:jazzy udp4 --port 8888 -v6`
4. Press the reset button on your StackChan to restart it.

### Motor
Control the motor by specifying the angle.

```sh
ros2 topic pub --once /stackchan/joint_commands std_msgs/msg/Int32MultiArray "{data: [200, 500]}"
```

Get the current motor angle.

```sh
ros2 topic echo /stackchan/joint_states
```
