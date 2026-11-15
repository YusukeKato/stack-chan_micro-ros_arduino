#ifndef STACKCHAN_WIFI_TRANSPORT_H
#define STACKCHAN_WIFI_TRANSPORT_H

#include <stdint.h>

// Connect Wi-Fi and use UDP to the micro-ROS Agent.
// Same as set_microros_wifi_transports() of micro_ros_arduino, but with a fixed read():
// the library version waits for the next packet (about 1 s) when the timeout is negative,
// and never receives when the timeout is 0.
void set_wifi_transport(const char *ssid, const char *password, const char *agent_ip,
                        uint16_t agent_port);

#endif  // STACKCHAN_WIFI_TRANSPORT_H
