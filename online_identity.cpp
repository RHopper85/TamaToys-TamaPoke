#include "online_identity.h"

#include <Preferences.h>
#include <cstring>
#include "netwifi.h"

#ifdef ARDUINO_ARCH_ESP32
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <esp_system.h>
#include <time.h>
#include "tamapoke_ca.h"
#endif

namespace TamaOnline {
namespace {

char token[65] = "";
char code[11] = "";
char name[12] = "";
char state[35] = "Connect Wi-Fi to register";
bool namePending = false;

static void copyText(char *dest, size_t cap, const char *src) {
  if (!src) src = "";
  if (!cap) return;
  strncpy(dest, src, cap - 1);
  dest[cap - 1] = 0;
}

static bool validToken(const char *value) {
  if (strlen(value) != 64) return false;
  for (int i = 0; i < 64; ++i)
    if (!((value[i] >= 'a' && value[i] <= 'f') ||
          (value[i] >= '0' && value[i] <= '9'))) return false;
  return true;
}

static bool storeString(const char *key, const char *value) {
  Preferences prefs;
  if (!prefs.begin("tt_online", false)) return false;
  size_t wrote = prefs.putString(key, value);
  prefs.end();
  return wrote == strlen(value) || wrote == strlen(value) + 1;
}

#ifdef ARDUINO_ARCH_ESP32
constexpr char API_BASE[] = "https://tamapokeapi.astraobscura.com/index.php?route=";
uint32_t retryAt = 0;
bool timeStarted = false;

static bool jsonField(const String &json, const char *field, char *dest, size_t cap) {
  String search = String('"') + field + '"';
  int pos = json.indexOf(search);
  if (pos < 0) return false;
  pos = json.indexOf(':', pos + search.length());
  if (pos < 0) return false;
  ++pos;
  while (pos < (int)json.length() && json[pos] == ' ') ++pos;
  if (pos >= (int)json.length() || json[pos++] != '"') return false;
  size_t len = 0;
  while (pos < (int)json.length() && json[pos] != '"') {
    char c = json[pos++];
    // Expected API values are simple uppercase code / ASCII trainer name.
    if ((unsigned char)c < 32 || c == '\\' || len + 1 >= cap) return false;
    dest[len++] = c;
  }
  if (pos >= (int)json.length()) return false;
  dest[len] = 0;
  return true;
}

// An HTTP error never falls through to a local wipe. The explicit X- header
// survives IONOS's CGI proxy, which can strip Authorization headers.
static int request(const char *method, const char *route,
                   const char *body, String &response, bool authorized) {
  if (TamaWifi::state() != TamaWifi::State::CONNECTED) return -1;
  WiFiClientSecure client;
  client.setCACert(TAMAPOKE_CA_PEM);
  client.setTimeout(4500);
  HTTPClient http;
  String url = String(API_BASE) + route;
  if (!http.begin(client, url)) return -1;
  http.setConnectTimeout(4500);
  http.setTimeout(4500);
  http.addHeader("Accept", "application/json");
  if (authorized) http.addHeader("X-TamaPoke-Token", token);
  int result;
  if (body) {
    http.addHeader("Content-Type", "application/json");
    result = http.POST((uint8_t *)body, strlen(body));
  } else {
    result = http.sendRequest(method);
  }
  if (result > 0) response = http.getString();
  http.end();
  return result;
}

static bool freshToken() {
  uint8_t random[32];
  esp_fill_random(random, sizeof(random));
  static const char HEX[] = "0123456789abcdef";
  char fresh[65];
  for (int i = 0; i < 32; ++i) {
    fresh[i * 2] = HEX[random[i] >> 4];
    fresh[i * 2 + 1] = HEX[random[i] & 15];
  }
  fresh[64] = 0;
  // Persist BEFORE POST. Retries send this same secret so a dropped response
  // cannot create duplicate server accounts (the API is idempotent by token).
  if (!storeString("token", fresh)) return false;
  copyText(token, sizeof(token), fresh);
  return true;
}

static void tryRegister(const char *trainerName) {
  // Older API versions generated their own token and did not understand
  // idempotency. Never register there: a mismatched token would create an
  // account the device could not later delete.
  String health;
  int healthCode = request("GET", "/v1/health", nullptr, health, false);
  int versionAt = health.indexOf("\"version\":");
  if (healthCode != 200 || versionAt < 0 ||
      health.substring(versionAt + 10).toInt() < 2) {
    copyText(state, sizeof(state), healthCode == 200 ?
             "Update API server first" : "Online service unreachable");
    return;
  }
  if (!validToken(token) && !freshToken()) {
    copyText(state, sizeof(state), "Cannot save online token");
    return;
  }
  char body[125];
  // Trainer names are controlled by the game's uppercase alphanumeric keypad.
  snprintf(body, sizeof(body),
           "{\"trainer_name\":\"%.10s\",\"device_token\":\"%s\"}",
           trainerName, token);
  copyText(state, sizeof(state), "Registering...");
  String response;
  int httpCode = request("POST", "/v1/register", body, response, false);
  char received[11] = "";
  char echoedToken[65] = "";
  if ((httpCode == 200 || httpCode == 201) &&
      jsonField(response, "friend_code", received, sizeof(received)) &&
      jsonField(response, "device_token", echoedToken, sizeof(echoedToken)) &&
      strcmp(echoedToken, token) == 0 &&
      strlen(received) == 10 && received[0] == 'T' && received[1] == 'P' &&
      storeString("code", received)) {
    copyText(code, sizeof(code), received);
    copyText(state, sizeof(state), "Registered");
  } else {
    copyText(state, sizeof(state), httpCode == 429 ? "Try again later" :
             httpCode < 0 ? "Online service unreachable" : "Registration failed");
  }
}
#endif

}  // namespace

void begin() {
  token[0] = code[0] = name[0] = 0;
  namePending = false;
  Preferences prefs;
  if (prefs.begin("tt_online", true)) {
    prefs.getString("token", token, sizeof(token));
    prefs.getString("code", code, sizeof(code));
    prefs.getString("name", name, sizeof(name));
    namePending = prefs.getBool("needs_name", false);
    prefs.end();
  }
  if (!validToken(token)) token[0] = code[0] = 0;
  copyText(state, sizeof(state), registered() ? "Registered" :
           "Connect Wi-Fi to register");
}

bool hasToken() { return validToken(token); }
bool registered() { return hasToken() && strlen(code) == 10; }
const char *friendCode() { return registered() ? code : ""; }
const char *status() { return state; }
const char *savedName() { return name; }
bool needsNameOnboarding() { return namePending; }

bool setNameOnboarding(bool required) {
  Preferences prefs;
  if (!prefs.begin("tt_online", false)) return false;
  bool saved = prefs.putBool("needs_name", required);
  prefs.end();
  if (saved) namePending = required;
  return saved;
}

bool rememberName(const char *trainerName) {
  if (!trainerName || !trainerName[0] || strlen(trainerName) > 11) return false;
  if (!storeString("name", trainerName)) return false;
  copyText(name, sizeof(name), trainerName);
  return true;
}

void poll(const char *trainerName, bool nameReady) {
#ifdef ARDUINO_ARCH_ESP32
  if (registered() || namePending || !nameReady || !trainerName || !trainerName[0]) return;
  if (TamaWifi::state() != TamaWifi::State::CONNECTED) {
    copyText(state, sizeof(state), "Connect Wi-Fi to register");
    return;
  }
  if (!timeStarted) {
    configTime(0, 0, "pool.ntp.org", "time.cloudflare.com");
    timeStarted = true;
  }
  // The ESP32's system clock can be 1970 after boot. Do not weaken TLS date
  // validation to make registration work; wait for NTP instead.
  if (time(nullptr) < 1735689600) {
    copyText(state, sizeof(state), "Waiting for network time");
    return;
  }
  if ((int32_t)(millis() - retryAt) < 0) return;
  retryAt = millis() + 300000; // no more than one attempt every five minutes
  tryRegister(trainerName);
#else
  (void)trainerName;
  (void)nameReady; // Emulator deliberately never talks to IONOS.
#endif
}

DeleteResult deleteAccount() {
  if (!hasToken()) return DeleteResult::NO_ACCOUNT;
#ifdef ARDUINO_ARCH_ESP32
  if (TamaWifi::state() != TamaWifi::State::CONNECTED) return DeleteResult::WIFI_REQUIRED;
  if (time(nullptr) < 1735689600) return DeleteResult::WIFI_REQUIRED;
  String response;
  int httpCode = request("DELETE", "/v1/me", nullptr, response, true);
  if (httpCode == 200 || httpCode == 204) return DeleteResult::DELETED;
  // A registration whose POST never completed may have a token but no record.
  if (httpCode == 401 && !registered()) return DeleteResult::NO_ACCOUNT;
  return DeleteResult::SERVER_ERROR;
#else
  return DeleteResult::SERVER_ERROR; // Fake Wi-Fi can never delete real accounts.
#endif
}

bool clearIdentity() {
  Preferences prefs;
  if (!prefs.begin("tt_online", false)) return false;
  bool ok = prefs.clear();
  prefs.end();
  if (!ok) return false;
  token[0] = code[0] = name[0] = 0;
  namePending = false;
  copyText(state, sizeof(state), "Connect Wi-Fi to register");
  return true;
}

}  // namespace TamaOnline
