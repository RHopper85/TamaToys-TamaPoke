// Emulator-only UI regression test. The Wi-Fi backend supplies sample networks,
// so this never scans the computer's real radio or persists real credentials.
#include "Arduino.h"
#include "Arduino_GFX_Library.h"
#include "pet.h"
#include "netwifi.h"
#include <cstdio>
#include <cstring>

uint32_t g_seed = 7;
FakeSerial Serial;
FakeESP ESP;
FakeWire Wire;
volatile int g_touchX = 0, g_touchY = 0;
volatile bool g_touchDown = false;
void FakeESP::restart() { exit(0); }
int FakeSerial::available() { return 0; }
String FakeSerial::readStringUntil(char) { return String(""); }

extern Pet pet;
extern Arduino_Canvas *gfx;
extern bool menuOpen, settingsOpen, wifiOpen;
extern uint8_t wifiView;
void setup();
void render();
void onTap(int16_t x, int16_t y);
void onSwipeV(int dir);
int uiMenuRowCenterY(int i);

static int failures = 0;
static void expect(bool condition, const char *label) {
  printf("%s: %s\n", condition ? "PASS" : "FAIL", label);
  if (!condition) ++failures;
}
static void checkFrame(const char *label) {
  gfx->frameReady = false;
  render();
  expect(gfx->frameReady, label);
}

int main() {
  setup();
  if (pet.awaitingStarter()) pet.chooseStarter(4);

  menuOpen = true;
  onTap(233, uiMenuRowCenterY(3));
  expect(settingsOpen, "Settings menu opens a hub");
  checkFrame("Settings hub renders");
  onTap(233, 193); // Wi-Fi row moved up to make room for reset options
  expect(wifiOpen, "Wi-Fi opens from Settings");
  checkFrame("Wi-Fi home renders");

  onTap(233, 162);
  expect(TamaWifi::enabled(), "Wi-Fi master switch turns on");
  expect(TamaWifi::autoConnect(), "Auto-connect defaults on");
  onTap(233, 281);
  expect(wifiView == 1 && TamaWifi::networkCount() == 3,
         "Nearby list shows emulator sample networks");
  checkFrame("Nearby network list renders");
  onTap(233, 143);
  expect(wifiView == 3, "Secured network opens password keyboard");
  checkFrame("Password keyboard renders");
  for (int i = 0; i < 8; ++i) onTap(70, 177); // 'q' eight times
  onTap(314, 414);
  expect(wifiView == 0 && TamaWifi::savedCount() == 1,
         "Joining saves the Wi-Fi network");
  expect(TamaWifi::state() == TamaWifi::State::CONNECTED,
         "Emulated Wi-Fi connects");

  onTap(233, 342);
  expect(wifiView == 2, "Saved networks are accessible");
  onTap(233, 143);
  expect(wifiView == 4, "Saved network has a detail screen");
  onTap(233, 290);
  checkFrame("Forget confirmation renders");
  onTap(300, 280);
  expect(TamaWifi::savedCount() == 0, "Forget removes a saved network");
  onSwipeV(1);
  onSwipeV(1);
  expect(!wifiOpen && settingsOpen, "Back returns to Settings");
  onSwipeV(1);
  expect(!settingsOpen, "Back leaves Settings without changing gameplay");
  return failures ? 1 : 0;
}
