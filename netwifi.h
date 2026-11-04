#pragma once

#include <Arduino.h>

// Home Wi-Fi is deliberately separate from the existing ESP-NOW battle radio.
// All calls are made from the main loop, never from Wi-Fi event callbacks.
namespace TamaWifi {

constexpr int MAX_NETWORKS = 16;
constexpr int MAX_SAVED = 5;

struct Network {
  char ssid[33] = "";
  int16_t rssi = -100;
  bool secure = true;
  bool saved = false;
};

enum class State : uint8_t {
  OFF, IDLE, SCANNING, CONNECTING, CONNECTED, FAILED, LAN
};

void begin();
void poll();
bool enabled();
void setEnabled(bool on);
bool autoConnect();
void setAutoConnect(bool on);
State state();
const char *connectedName();

void scan();
int networkCount();
Network networkAt(int index);
int savedCount();
const char *savedName(int index);
bool savedSecure(int index);
bool isSaved(const char *ssid);
void forget(const char *ssid);

// password == nullptr means use the password of a previously saved network.
// The password is stored only after the connection has actually succeeded.
bool connect(const char *ssid, const char *password = nullptr);

// ESP-NOW uses a fixed radio channel, so it owns the radio for LAN battles.
// Do not reconnect or scan until the LAN session has ended.
void suspendForLan();
void resumeAfterLan();

}  // namespace TamaWifi
