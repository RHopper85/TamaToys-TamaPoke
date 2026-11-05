#pragma once

#include <Arduino.h>

// IONOS owns online identity. Nothing in this module changes the pet save or
// fabricates friends in the emulator. The device token is private NVS state.
namespace TamaOnline {

enum class DeleteResult : uint8_t {
  DELETED, NO_ACCOUNT, WIFI_REQUIRED, SERVER_ERROR, STORAGE_ERROR
};

void begin();
void poll(const char *trainerName, bool nameReady);
bool hasToken();
bool registered();
const char *friendCode();
const char *status();

// A New Game leaves this namespace untouched, including the saved name.
bool rememberName(const char *trainerName);
const char *savedName();

// First-run name entry survives an unexpected reboot before confirmation.
bool needsNameOnboarding();
bool setNameOnboarding(bool required);

// Call deleteAccount BEFORE clearing NVS for a factory reset. A failure must
// leave the local save and token untouched so the user can retry safely.
DeleteResult deleteAccount();
bool clearIdentity();

}  // namespace TamaOnline
