// Phase 2: first-run trainer naming plus the intentionally offline Friends UI.
#include "Arduino.h"
#include "Arduino_GFX_Library.h"
#include "pet.h"
#include <cstdio>
#include <cstring>

uint32_t g_seed = 29;
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
extern bool kbOpen, kbOnboarding, playerOpen, friendsOpen;
extern uint8_t playerPage, friendsView, friendPreviewCount;
extern char friendCodeEntry[13];
extern uint8_t friendCodeEntryLen;
extern const char *const SCREEN_NAME[];
void setup();
void render();
void onTap(int16_t x, int16_t y);
void onSwipeV(int dir);
uint8_t uiCurrentScreen();

static int errors = 0;
static void expect(bool cond, const char *what) {
  printf("%s: %s\n", cond ? "PASS" : "FAIL", what);
  if (!cond) ++errors;
}
static void frame(const char *what) {
  gfx->frameReady = false;
  render();
  expect(gfx->frameReady, what);
}

int main() {
  setup();
  expect(pet.awaitingStarter(), "clean save requires a starter");
  onTap(233, 140); // Kanto region
  onTap(233, 140); // first starter
  expect(!pet.awaitingStarter(), "starter is chosen");
  expect(kbOpen && kbOnboarding, "mandatory trainer-name keyboard appears");
  frame("trainer onboarding renders");

  onTap(380, 379); // OK without a name
  expect(kbOpen, "blank onboarding name cannot be submitted");
  onTap(384, 278); // R
  onTap(192, 278); // O
  onTap(128, 174); // B
  onTap(380, 379); // OK
  expect(!kbOpen && !kbOnboarding, "trainer-name keyboard closes after naming");
  expect(!strcmp(pet.trainerName, "ROB"), "entered trainer name is saved");

  playerOpen = true;
  playerPage = 0;
  frame("player card and Friends button render");
  onTap(233, 380);
  expect(friendsOpen && friendsView == 0, "Friends opens from Player card");
  expect(!strcmp(SCREEN_NAME[uiCurrentScreen()], "friends"), "Friends has a screen id");
  frame("Friends home renders without a fake friend code");

  onTap(233, 213); // friends list
  expect(friendsView == 1 && friendPreviewCount == 2,
         "emulator lists two clearly simulated friends");
  frame("friends list renders");
  onTap(200, 126); // MISTY
  expect(friendsView == 4, "friend detail opens");
  frame("friend detail renders");
  onTap(233, 302); // remove
  expect(friendsView == 5, "removing requires confirmation");
  frame("confirmation renders");
  onTap(233, 330); // cancel
  expect(friendsView == 4 && friendPreviewCount == 2,
         "cancel preserves the friend");
  onTap(233, 302);
  onTap(233, 260); // confirm
  expect(friendsView == 1 && friendPreviewCount == 1,
         "confirm removes the demo friend");

  onSwipeV(1); // list -> home
  onTap(233, 275); // add friend
  expect(friendsView == 2, "friend-code keyboard opens");
  frame("friend-code keyboard renders");
  for (int i = 0; i < 8; ++i) onTap(101, 164); // AAAAAAAA
  expect(friendCodeEntryLen == 8 && !strcmp(friendCodeEntry, "AAAAAAAA"),
         "friend code is typed as uppercase letters");
  onTap(300, 397); // add
  expect(friendsView == 1 && friendPreviewCount == 2,
         "emulator adds a nonpersistent demo friend");

  onSwipeV(1); // list -> home
  onTap(233, 338); // requests
  expect(friendsView == 3, "requests screen opens");
  frame("pending requests render");
  onTap(233, 239); // accept mock GARY
  expect(friendsView == 1 && friendPreviewCount == 3,
         "accepting mock request adds a friend");
  onSwipeV(1); // list -> home
  onSwipeV(1); // home -> player
  expect(!friendsOpen && playerOpen, "back returns to Player card");
  return errors ? 1 : 0;
}
