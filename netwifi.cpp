#include "netwifi.h"

#include <cstring>
#include <cstdio>

#ifdef ARDUINO_ARCH_ESP32

#include <WiFi.h>
#include <Preferences.h>

namespace TamaWifi {
namespace {

struct SavedNetwork {
  char ssid[33] = "";
  char password[65] = "";
};

SavedNetwork saved[MAX_SAVED];
Network visible[MAX_NETWORKS];
int visibleCount = 0;
bool masterOn = false;
bool reconnectOn = true;
bool lanOwnsRadio = false;
bool scanRunning = false;
bool connectRunning = false;
bool autoScan = false;
bool saveOnSuccess = false;
uint32_t connectStarted = 0;
uint32_t nextAutoScan = 0;
State current = State::OFF;
char pendingSsid[33] = "";
char pendingPassword[65] = "";
char activeSsid[33] = "";

void copyText(char *dest, size_t size, const char *src) {
  if (!size) return;
  if (!src) src = "";
  strncpy(dest, src, size - 1);
  dest[size - 1] = '\0';
}

int savedIndex(const char *ssid) {
  if (!ssid || !ssid[0]) return -1;
  for (int i = 0; i < MAX_SAVED; ++i)
    if (strcmp(saved[i].ssid, ssid) == 0) return i;
  return -1;
}

void persist() {
  Preferences prefs;
  if (!prefs.begin("tt_wifi", false)) return;
  prefs.putBool("enabled", masterOn);
  prefs.putBool("auto", reconnectOn);
  for (int i = 0; i < MAX_SAVED; ++i) {
    char skey[8], pkey[8];
    snprintf(skey, sizeof(skey), "ssid%d", i);
    snprintf(pkey, sizeof(pkey), "pass%d", i);
    prefs.putString(skey, saved[i].ssid);
    prefs.putString(pkey, saved[i].password);
  }
  prefs.end();
}

void remember(const char *ssid, const char *password) {
  int i = savedIndex(ssid);
  if (i < 0) {
    for (i = 0; i < MAX_SAVED && saved[i].ssid[0]; ++i) {}
    if (i == MAX_SAVED) i = MAX_SAVED - 1;
  }
  copyText(saved[i].ssid, sizeof(saved[i].ssid), ssid);
  copyText(saved[i].password, sizeof(saved[i].password), password);
  persist();
}

bool beginConnection(const char *ssid, const char *password, bool shouldSave) {
  if (!masterOn || lanOwnsRadio || !ssid || !ssid[0] || connectRunning) return false;
  if (scanRunning) {
    WiFi.scanDelete();
    scanRunning = false;
  }
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
  copyText(pendingSsid, sizeof(pendingSsid), ssid);
  copyText(pendingPassword, sizeof(pendingPassword), password);
  saveOnSuccess = shouldSave;
  connectStarted = millis();
  connectRunning = true;
  current = State::CONNECTING;
  WiFi.begin(pendingSsid, pendingPassword);
  return true;
}

void connectBestSaved() {
  int best = -1;
  int rssi = -200;
  for (int i = 0; i < visibleCount; ++i) {
    int si = savedIndex(visible[i].ssid);
    if (si >= 0 && visible[i].rssi > rssi) {
      best = si;
      rssi = visible[i].rssi;
    }
  }
  if (best >= 0)
    beginConnection(saved[best].ssid, saved[best].password, false);
}

void completeScan(int count) {
  visibleCount = 0;
  for (int i = 0; i < count && visibleCount < MAX_NETWORKS; ++i) {
    String ssid = WiFi.SSID(i);
    if (ssid.length() < 1 || ssid.length() > 32) continue;
    bool duplicate = false;
    for (int k = 0; k < visibleCount; ++k)
      if (strcmp(visible[k].ssid, ssid.c_str()) == 0) duplicate = true;
    if (duplicate) continue;
    Network &n = visible[visibleCount++];
    copyText(n.ssid, sizeof(n.ssid), ssid.c_str());
    n.rssi = WiFi.RSSI(i);
    n.secure = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    n.saved = savedIndex(n.ssid) >= 0;
  }
  WiFi.scanDelete();
  scanRunning = false;
  current = WiFi.status() == WL_CONNECTED ? State::CONNECTED : State::IDLE;
  if (autoScan) connectBestSaved();
  autoScan = false;
}

}  // namespace

void begin() {
  Preferences prefs;
  if (prefs.begin("tt_wifi", true)) {
    masterOn = prefs.getBool("enabled", false);
    reconnectOn = prefs.getBool("auto", true);
    for (int i = 0; i < MAX_SAVED; ++i) {
      char skey[8], pkey[8];
      snprintf(skey, sizeof(skey), "ssid%d", i);
      snprintf(pkey, sizeof(pkey), "pass%d", i);
      prefs.getString(skey, saved[i].ssid, sizeof(saved[i].ssid));
      prefs.getString(pkey, saved[i].password, sizeof(saved[i].password));
    }
    prefs.end();
  }
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  if (masterOn) {
    WiFi.mode(WIFI_STA);
    current = State::IDLE;
    nextAutoScan = millis() + 3000;
  } else {
    WiFi.mode(WIFI_OFF);
    current = State::OFF;
  }
}

void poll() {
  if (!masterOn || lanOwnsRadio) return;
  uint32_t now = millis();
  if (connectRunning) {
    if (WiFi.status() == WL_CONNECTED) {
      connectRunning = false;
      current = State::CONNECTED;
      copyText(activeSsid, sizeof(activeSsid), pendingSsid);
      if (saveOnSuccess) remember(pendingSsid, pendingPassword);
      saveOnSuccess = false;
      pendingPassword[0] = '\0';
    } else if (now - connectStarted > 16000) {
      connectRunning = false;
      saveOnSuccess = false;
      pendingPassword[0] = '\0';
      WiFi.disconnect(false, false);
      current = State::FAILED;
      nextAutoScan = now + 120000;
    }
    return;
  }
  if (scanRunning) {
    int done = WiFi.scanComplete();
    if (done >= 0) completeScan(done);
    else if (done == WIFI_SCAN_FAILED) {
      scanRunning = false;
      autoScan = false;
      current = State::FAILED;
      nextAutoScan = now + 120000;
    }
    return;
  }
  if (WiFi.status() == WL_CONNECTED) {
    current = State::CONNECTED;
    return;
  }
  if (current == State::CONNECTED) {
    activeSsid[0] = '\0';
    current = State::IDLE;
    nextAutoScan = now + 10000;
  }
  if (reconnectOn && savedCount() && (int32_t)(now - nextAutoScan) >= 0) {
    nextAutoScan = now + 120000;
    autoScan = true;
    scan();
  }
}

bool enabled() { return masterOn; }
void setEnabled(bool on) {
  if (masterOn == on) return;
  masterOn = on;
  scanRunning = connectRunning = autoScan = false;
  pendingPassword[0] = activeSsid[0] = '\0';
  visibleCount = 0;
  if (!lanOwnsRadio) {
    WiFi.scanDelete();
    WiFi.disconnect(false, false);
    WiFi.mode(on ? WIFI_STA : WIFI_OFF);
  }
  current = on ? State::IDLE : State::OFF;
  nextAutoScan = millis() + 3000;
  persist();
}
bool autoConnect() { return reconnectOn; }
void setAutoConnect(bool on) { reconnectOn = on; persist(); }
State state() { return current; }
const char *connectedName() { return current == State::CONNECTED ? activeSsid : ""; }

void scan() {
  if (!masterOn || lanOwnsRadio || scanRunning || connectRunning) return;
  WiFi.mode(WIFI_STA);
  visibleCount = 0;
  int result = WiFi.scanNetworks(true, false);
  if (result == WIFI_SCAN_FAILED) {
    current = State::FAILED;
    return;
  }
  scanRunning = true;
  current = State::SCANNING;
  if (result >= 0) completeScan(result);
}
int networkCount() { return visibleCount; }
Network networkAt(int index) {
  return index >= 0 && index < visibleCount ? visible[index] : Network();
}
int savedCount() {
  int n = 0;
  for (auto &s : saved) if (s.ssid[0]) ++n;
  return n;
}
const char *savedName(int index) {
  for (auto &s : saved) {
    if (!s.ssid[0]) continue;
    if (index-- == 0) return s.ssid;
  }
  return "";
}
bool savedSecure(int index) {
  for (auto &s : saved) {
    if (!s.ssid[0]) continue;
    if (index-- == 0) return s.password[0] != '\0';
  }
  return true;
}
bool isSaved(const char *ssid) { return savedIndex(ssid) >= 0; }
void forget(const char *ssid) {
  int i = savedIndex(ssid);
  if (i < 0) return;
  for (int j = i; j < MAX_SAVED - 1; ++j) saved[j] = saved[j + 1];
  saved[MAX_SAVED - 1] = SavedNetwork();
  if (strcmp(activeSsid, ssid) == 0) {
    WiFi.disconnect(false, false);
    activeSsid[0] = '\0';
    current = masterOn ? State::IDLE : State::OFF;
  }
  persist();
}
bool connect(const char *ssid, const char *password) {
  if (!ssid || !ssid[0]) return false;
  if (!password) {
    int i = savedIndex(ssid);
    if (i < 0) return false;
    return beginConnection(ssid, saved[i].password, false);
  }
  return beginConnection(ssid, password, true);
}
void suspendForLan() {
  lanOwnsRadio = true;
  scanRunning = connectRunning = autoScan = false;
  WiFi.scanDelete();
  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_OFF);
  current = State::LAN;
}
void resumeAfterLan() {
  if (!lanOwnsRadio) return;
  lanOwnsRadio = false;
  activeSsid[0] = '\0';
  if (masterOn) WiFi.mode(WIFI_STA);
  current = masterOn ? State::IDLE : State::OFF;
  nextAutoScan = millis() + 3000;
}

}  // namespace TamaWifi

#else  // Desktop emulator: demonstrate menus without using the PC's real Wi-Fi.

namespace TamaWifi {
namespace {
bool masterOn = false;
bool reconnectOn = true;
bool lanOwnsRadio = false;
bool linked = false;
bool rememberedSecure = true;
char activeSsid[33] = "";
char remembered[33] = "";
State current = State::OFF;
bool scanDone = false;
Network sample[] = {
  {"TamaToys Demo", -43, true, false},
  {"Guest WiFi", -67, false, false},
  {"Workshop", -76, true, false}
};
}  // namespace

void begin() {}
void poll() {}
bool enabled() { return masterOn; }
void setEnabled(bool on) {
  masterOn = on;
  linked = false;
  current = on ? State::IDLE : State::OFF;
}
bool autoConnect() { return reconnectOn; }
void setAutoConnect(bool on) { reconnectOn = on; }
State state() { return current; }
const char *connectedName() { return linked ? activeSsid : ""; }
void scan() { if (masterOn && !lanOwnsRadio) scanDone = true; }
int networkCount() { return scanDone && masterOn ? 3 : 0; }
Network networkAt(int index) {
  if (index < 0 || index >= networkCount()) return Network();
  Network n = sample[index];
  n.saved = strcmp(n.ssid, remembered) == 0;
  return n;
}
int savedCount() { return remembered[0] ? 1 : 0; }
const char *savedName(int index) { return index == 0 ? remembered : ""; }
bool savedSecure(int index) { return index == 0 ? rememberedSecure : true; }
bool isSaved(const char *ssid) { return ssid && strcmp(ssid, remembered) == 0; }
void forget(const char *ssid) {
  if (!isSaved(ssid)) return;
  remembered[0] = '\0';
  if (strcmp(activeSsid, ssid) == 0) {
    linked = false;
    activeSsid[0] = '\0';
    current = masterOn ? State::IDLE : State::OFF;
  }
}
bool connect(const char *ssid, const char *password) {
  if (!masterOn || lanOwnsRadio || !ssid || !ssid[0]) return false;
  bool secure = true;
  for (const auto &n : sample) if (strcmp(n.ssid, ssid) == 0) secure = n.secure;
  if (secure && password && strlen(password) < 8) {
    current = State::FAILED;
    return false;
  }
  if (!password && !isSaved(ssid)) return false;
  strncpy(activeSsid, ssid, sizeof(activeSsid) - 1);
  activeSsid[sizeof(activeSsid) - 1] = '\0';
  if (password) {
    strncpy(remembered, ssid, sizeof(remembered) - 1);
    remembered[sizeof(remembered) - 1] = '\0';
    rememberedSecure = secure;
  }
  linked = true;
  current = State::CONNECTED;
  return true;
}
void suspendForLan() { lanOwnsRadio = true; linked = false; current = State::LAN; }
void resumeAfterLan() { lanOwnsRadio = false; current = masterOn ? State::IDLE : State::OFF; }
}  // namespace TamaWifi

#endif
