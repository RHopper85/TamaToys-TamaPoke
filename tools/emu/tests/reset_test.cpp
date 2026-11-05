// Drive the actual Settings reset UI and NVS code, not a reimplementation.
// ESP.restart() is intercepted so the test can inspect the erased storage.
#include "Arduino.h"
#include "Arduino_GFX_Library.h"
#include "Preferences.h"
#include "pet.h"
#include "online_identity.h"
#include "netwifi.h"
#include <cstdio>
#include <cstring>

uint32_t g_seed = 29;
FakeSerial Serial;
FakeESP ESP;
FakeWire Wire;
volatile int g_touchX = 0, g_touchY = 0;
volatile bool g_touchDown = false;
struct Restarted {};
void FakeESP::restart() { throw Restarted(); }
int FakeSerial::available() { return 0; }
String FakeSerial::readStringUntil(char) { return String(""); }

extern Pet pet;
extern bool settingsOpen, saveInhibited;
extern uint8_t resetView;
void setup();
void onTap(int16_t x, int16_t y);
void render();

static int failures = 0;
static void expect(bool yes, const char *reason) {
  printf("%s %s\n", yes ? "PASS" : "FAIL", reason);
  if (!yes) ++failures;
}

int main() {
  setup();
  pet.chooseStarter(1);
  pet.renameTrainer("ROB");
  TamaWifi::setEnabled(true);
  TamaWifi::connect("TamaToys Demo", "password");
  expect(TamaWifi::savedCount() == 1, "Wi-Fi profile exists before resetting");
  settingsOpen = true;
  render();

  onTap(233, 250); // NEW GAME button
  expect(resetView != 0, "New Game shows explicit confirmation");
  onTap(150, 315); // NO
  expect(resetView == 0 && !pet.awaitingStarter() &&
         !strcmp(pet.trainerName, "ROB"), "No preserves the complete save");

  onTap(233, 310); // FACTORY RESET button
  expect(resetView != 0, "Factory Reset shows separate confirmation");
  onTap(150, 315); // NO
  expect(resetView == 0 && !pet.awaitingStarter(), "No cancels factory reset");

  onTap(233, 250);
  bool restarted = false;
  try { onTap(290, 315); } catch (const Restarted &) { restarted = true; }
  expect(restarted && saveInhibited, "New Game inhibits autosave and restarts");
  TamaOnline::begin();
  expect(!strcmp(TamaOnline::savedName(), "ROB"), "New Game preserves trainer name");
  expect(TamaWifi::savedCount() == 1, "New Game preserves saved Wi-Fi");
  Pet newPet;
  newPet.begin();
  expect(newPet.awaitingStarter(), "New Game erases game progress");

  // A registered account cannot be factory-wiped when the real server cannot
  // confirm deletion. The emulator intentionally never sends network traffic.
  Preferences online;
  online.begin("tt_online", false);
  online.putString("token", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  online.putString("code", "TPABCDEFGH");
  online.end();
  TamaOnline::begin();
  expect(TamaOnline::registered(), "Test account is marked registered");
  resetView = 0;
  settingsOpen = true;
  onTap(233, 310);
  onTap(290, 315);
  expect(resetView == 3 && TamaOnline::registered(),
         "Failed online deletion keeps token and cancels factory wipe");
  Preferences progress;
  progress.begin("tamapoke", true);
  expect(progress.getBool("init", false), "Failed deletion keeps current game data");
  progress.end();

  // An unregistered/offline-only device has no remote account to delete.
  expect(TamaOnline::clearIdentity(), "Remove synthetic test account");
  saveInhibited = false;
  resetView = 0; // A real ESP.restart() reinitializes this UI state.
  settingsOpen = true;
  onTap(233, 310); // FACTORY RESET
  restarted = false;
  try { onTap(290, 315); } catch (const Restarted &) { restarted = true; }
  expect(restarted && saveInhibited, "Factory Reset restarts after local wipe");
  TamaOnline::begin();
  expect(!TamaOnline::savedName()[0] && !TamaOnline::hasToken(),
         "Factory Reset clears online identity and retained trainer name");
  expect(TamaWifi::savedCount() == 0 && !TamaWifi::enabled(),
         "Factory Reset removes saved Wi-Fi and turns radio off");
  return failures ? 1 : 0;
}
