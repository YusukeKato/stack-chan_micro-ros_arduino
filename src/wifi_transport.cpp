#include "wifi_transport.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <rmw_microros/rmw_microros.h>

static WiFiUDP udp;
static IPAddress agent_address;
static uint16_t agent_udp_port;

static bool transport_open(struct uxrCustomTransport *transport) {
  udp.begin(agent_udp_port);
  return true;
}

static bool transport_close(struct uxrCustomTransport *transport) {
  udp.stop();
  return true;
}

static size_t transport_write(struct uxrCustomTransport *transport, const uint8_t *buf, size_t len,
                              uint8_t *errcode) {
  udp.beginPacket(agent_address, agent_udp_port);
  size_t sent = udp.write(buf, len);
  udp.endPacket();
  return sent;
}

static size_t transport_read(struct uxrCustomTransport *transport, uint8_t *buf, size_t len,
                             int timeout, uint8_t *errcode) {
  uint32_t wait_ms = timeout > 0 ? (uint32_t)timeout : 0;  // negative = do not wait
  uint32_t start = millis();
  while (udp.parsePacket() == 0) {  // check at least once, even with timeout 0
    if (millis() - start >= wait_ms) return 0;
    delay(1);
  }
  int read = udp.read(buf, len);
  return read > 0 ? (size_t)read : 0;
}

void set_wifi_transport(const char *ssid, const char *password, const char *agent_ip,
                        uint16_t agent_port) {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  agent_address.fromString(agent_ip);
  agent_udp_port = agent_port;
  rmw_uros_set_custom_transport(false, NULL, transport_open, transport_close, transport_write,
                                transport_read);
}
