// TamaToys Friends UI, Phase 2. This is an offline shell, NOT an online account.
// The eventual HTTPS API will own real friend codes, requests, activity and
// challenge validation. Nothing here claims to create a registered identity.
// The emulator alone supplies a few clearly labeled, session-only demo friends.

struct FriendPreview {
  char code[13];
  char trainer[12];
  uint16_t hoursAway;
};

#define FRIEND_DEMO_CAP 4
#ifndef ARDUINO_ARCH_ESP32
static FriendPreview friendPreview[FRIEND_DEMO_CAP] = {
  {"DEMO0001", "MISTY", 2},
  {"DEMO0002", "BROCK", 30},
};
uint8_t friendPreviewCount = 2;
static bool friendDemoRequest = true;
#else
static FriendPreview friendPreview[FRIEND_DEMO_CAP] = {};
uint8_t friendPreviewCount = 0;
static bool friendDemoRequest = false;
#endif

static void friendNotice(const char *message) {
  strncpy(friendMessage, message, sizeof(friendMessage) - 1);
  friendMessage[sizeof(friendMessage) - 1] = 0;
}

static void friendActivity(uint16_t hours, char *out, size_t len) {
  if (!hours) snprintf(out, len, "Active now");
  else if (hours < 24) snprintf(out, len, "%uh ago", (unsigned)hours);
  else snprintf(out, len, "%ud ago", (unsigned)(hours / 24));
}

void friendsBack() {
  friendMessage[0] = 0;
  switch (friendsView) {
    case FRIEND_LIST:
    case FRIEND_ADD:
    case FRIEND_REQUESTS:
      friendsView = FRIEND_HOME;
      break;
    case FRIEND_DETAIL:
      friendsView = FRIEND_LIST;
      break;
    case FRIEND_REMOVE:
      friendsView = FRIEND_DETAIL;
      break;
    default:
      friendsOpen = false;
      break;
  }
}

static void friendPageFrame(const char *title) {
  wifiFrame(title);
#ifndef ARDUINO_ARCH_ESP32
  wifiLabel("EMULATOR / DEMO DATA ONLY", 441, 1, UI_INK_SOFT);
#endif
}

static void friendPageHome() {
  friendPageFrame("FRIENDS");
  gbaFrame(gfx, 82, 82, 302, 95, GBA_CREAM, GBA_TEAL);
  wifiLabel("YOUR FRIEND CODE", 91, 2, GBA_TEAL);
  wifiLabel(TamaOnline::registered() ? TamaOnline::friendCode() : "NOT ASSIGNED", 119, 3);
  wifiLabel(TamaOnline::registered() ? "Your trainer account is saved" :
            TamaOnline::status(), 154, 1, UI_INK_SOFT);
  wifiButton(89, 188, 288, 51, "FRIENDS LIST");
  wifiButton(89, 250, 288, 51, "ADD FRIEND");
  char requests[28];
  snprintf(requests, sizeof(requests), "REQUESTS (%u)",
           (unsigned)(friendDemoRequest ? 1 : 0));
  wifiButton(89, 312, 288, 51, requests);
  wifiBackButton();
}

static void friendPageList() {
  friendPageFrame("MY FRIENDS");
  wifiLabel("SELECT A TRAINER", 84, 1, GBA_TEAL);
  if (!friendPreviewCount) wifiLabel("No friends yet", 205, 2, UI_INK_SOFT);
  for (uint8_t i = 0; i < friendPreviewCount; ++i) {
    int y = 103 + i * 54;
    gbaFrame(gfx, 81, y, 304, 48, GBA_CREAM,
             (i % 2) ? GBA_TEAL : GBA_BLUE);
    gfx->fillRect(89, y + 8, 6, 30,
                  friendPreview[i].hoursAway < 24 ? GBA_GREEN : GBA_PURPLE);
    gfx->setTextColor(GBA_NAVY);
    gfx->setTextSize(2);
    gfx->setCursor(104, y + 5);
    gfx->print(friendPreview[i].trainer);
    char activity[24];
    friendActivity(friendPreview[i].hoursAway, activity, sizeof(activity));
    gfx->setTextColor(GBA_SHADOW);
    gfx->setTextSize(1);
    gfx->setCursor(104, y + 30);
    gfx->print(activity);
    gfx->setTextSize(2);
    gfx->setCursor(352, y + 14);
    gfx->print(">");
  }
  wifiBackButton();
}

static void friendPageDetail() {
  friendPageFrame("TRAINER");
  if (friendSelected >= friendPreviewCount) { friendsView = FRIEND_LIST; return; }
  const FriendPreview &friendData = friendPreview[friendSelected];
  gbaFrame(gfx, 82, 98, 302, 103, GBA_CREAM, GBA_BLUE);
  wifiLabel(friendData.trainer, 108, 3, GBA_NAVY);
  wifiLabel(friendData.code, 144, 2, UI_INK_SOFT);
  char activity[24];
  friendActivity(friendData.hoursAway, activity, sizeof(activity));
  wifiLabel(activity, 176, 2, UI_INK_SOFT);
  wifiButton(90, 212, 286, 51, "CHALLENGE", false, false);
  wifiButton(90, 280, 286, 51, "REMOVE FRIEND");
  if (friendMessage[0]) wifiLabel(friendMessage, 355, 1, UI_INK_SOFT);
  wifiBackButton();
}

static void friendPageRemove() {
  friendPageFrame("REMOVE FRIEND?");
  gbaFrame(gfx, 88, 129, 290, 79, GBA_CREAM, GBA_RED);
  if (friendSelected < friendPreviewCount)
    wifiLabel(friendPreview[friendSelected].trainer, 145, 3, GBA_NAVY);
  wifiLabel("This is a demo preview", 190, 1, UI_INK_SOFT);
  wifiButton(98, 236, 270, 55, "YES, REMOVE", false);
  wifiButton(98, 305, 270, 55, "CANCEL", true);
}

static void friendPageRequests() {
  friendPageFrame("REQUESTS");
  if (!friendDemoRequest) {
    wifiLabel("No pending requests", 188, 2, GBA_SHADOW);
  } else {
    gbaFrame(gfx, 82, 103, 302, 91, GBA_CREAM, GBA_GREEN);
    wifiLabel("GARY (DEMO)", 126, 3);
    wifiLabel("Wants to be your friend", 171, 1, UI_INK_SOFT);
    wifiButton(98, 215, 270, 54, "ACCEPT", true);
    wifiButton(98, 285, 270, 54, "REJECT");
  }
  wifiBackButton();
}

static const char FRIEND_CODE_KEYS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
#define FKEY_X 77
#define FKEY_Y 146
#define FKEY_W 52
#define FKEY_H 37

static void friendPageAdd() {
  friendPageFrame("ADD FRIEND");
  wifiLabel("Enter a friend's code", 78, 1, UI_INK_SOFT);
  wifiButton(329, 65, 65, 34, "BACK", false, true, 1);
  gbaFrame(gfx, 90, 91, 286, 44, GBA_CREAM, GBA_TEAL);
  wifiLabel(friendCodeEntryLen ? friendCodeEntry : "_", 102, 2, GBA_NAVY);
  for (int i = 0; i < 36; ++i) {
    int x = FKEY_X + (i % 6) * FKEY_W;
    int y = FKEY_Y + (i / 6) * FKEY_H;
    char key[2] = { FRIEND_CODE_KEYS[i], 0 };
    gbaFrame(gfx, x, y, 48, 33, GBA_CREAM,
             (i / 6) % 2 ? GBA_TEAL : GBA_BLUE);
    ui7Centered(x + 24, y + 9, key, 34, 2, GBA_NAVY);
  }
  wifiButton(104, 376, 122, 49, "DELETE", false, true, 1);
  wifiButton(241, 376, 122, 49, "ADD", true);
  if (friendMessage[0]) wifiLabel(friendMessage, 430, 1, UI_INK_SOFT);
}

void renderFriends() {
  switch (friendsView) {
    case FRIEND_LIST: friendPageList(); break;
    case FRIEND_ADD: friendPageAdd(); break;
    case FRIEND_REQUESTS: friendPageRequests(); break;
    case FRIEND_DETAIL: friendPageDetail(); break;
    case FRIEND_REMOVE: friendPageRemove(); break;
    default: friendPageHome(); break;
  }
  gfx->flush();
}

static void friendSubmitCode() {
  if (friendCodeEntryLen < 6) {
    friendNotice("Enter at least 6 characters");
    sfxPlay(SFX_DENY);
    return;
  }
#ifndef ARDUINO_ARCH_ESP32
  for (uint8_t i = 0; i < friendPreviewCount; ++i) {
    if (!strcmp(friendPreview[i].code, friendCodeEntry)) {
      friendNotice("Already in your friends list");
      sfxPlay(SFX_DENY);
      return;
    }
  }
  if (friendPreviewCount >= FRIEND_DEMO_CAP) {
    friendNotice("Demo friends list is full");
    sfxPlay(SFX_DENY);
    return;
  }
  FriendPreview &friendData = friendPreview[friendPreviewCount++];
  strncpy(friendData.code, friendCodeEntry, sizeof(friendData.code) - 1);
  friendData.code[sizeof(friendData.code) - 1] = 0;
  strcpy(friendData.trainer, "DEMO FRIEND");
  friendData.hoursAway = 0;
  friendsView = FRIEND_LIST;
  friendNotice("Demo friend added (not saved)");
  sfxPlay(SFX_TAP);
#else
  // Do not forge a local friend relationship: only the future IONOS HTTPS API
  // can verify the code and accept the recipient's permission.
  friendNotice("Online service not ready");
  sfxPlay(SFX_DENY);
#endif
}

void friendsTap(int16_t x, int16_t y) {
  if (friendsView == FRIEND_ADD) {
    if (wifiHit(x, y, 329, 65, 65, 34)) { friendsBack(); return; }
    if (wifiHit(x, y, 104, 376, 122, 49)) {
      if (friendCodeEntryLen) friendCodeEntry[--friendCodeEntryLen] = 0;
      friendMessage[0] = 0;
      return;
    }
    if (wifiHit(x, y, 241, 376, 122, 49)) { friendSubmitCode(); return; }
    if (x >= FKEY_X && x < FKEY_X + 6 * FKEY_W &&
        y >= FKEY_Y && y < FKEY_Y + 6 * FKEY_H) {
      int col = (x - FKEY_X) / FKEY_W;
      int row = (y - FKEY_Y) / FKEY_H;
      int index = row * 6 + col;
      if (index >= 0 && index < 36 && friendCodeEntryLen < sizeof(friendCodeEntry) - 1) {
        friendCodeEntry[friendCodeEntryLen++] = FRIEND_CODE_KEYS[index];
        friendCodeEntry[friendCodeEntryLen] = 0;
        friendMessage[0] = 0;
        sfxPlay(SFX_TAP);
      }
    }
    return;
  }

  if (friendsView == FRIEND_HOME) {
    if (wifiHit(x, y, 89, 188, 288, 51)) friendsView = FRIEND_LIST;
    else if (wifiHit(x, y, 89, 250, 288, 51)) {
      friendsView = FRIEND_ADD;
      friendCodeEntry[0] = 0;
      friendCodeEntryLen = 0;
    } else if (wifiHit(x, y, 89, 312, 288, 51)) friendsView = FRIEND_REQUESTS;
    else if (wifiHit(x, y, 132, 397, 202, 34)) friendsOpen = false;
    else return;
  } else if (friendsView == FRIEND_LIST) {
    for (uint8_t i = 0; i < friendPreviewCount; ++i) {
      if (wifiHit(x, y, 81, 103 + i * 54, 304, 48)) {
        friendSelected = i;
        friendMessage[0] = 0;
        friendsView = FRIEND_DETAIL;
        sfxPlay(SFX_TAP);
        return;
      }
    }
    if (wifiHit(x, y, 132, 397, 202, 34)) friendsView = FRIEND_HOME;
    else return;
  } else if (friendsView == FRIEND_DETAIL) {
    if (wifiHit(x, y, 90, 212, 286, 51)) {
      friendNotice("Online battles coming soon");
      sfxPlay(SFX_DENY);
      return;
    }
    else if (wifiHit(x, y, 90, 280, 286, 51)) friendsView = FRIEND_REMOVE;
    else if (wifiHit(x, y, 132, 397, 202, 34)) friendsView = FRIEND_LIST;
    else return;
  } else if (friendsView == FRIEND_REMOVE) {
    if (wifiHit(x, y, 98, 236, 270, 55)) {
      if (friendSelected < friendPreviewCount) {
        for (uint8_t i = friendSelected + 1; i < friendPreviewCount; ++i)
          friendPreview[i - 1] = friendPreview[i];
        friendPreviewCount--;
      }
      friendsView = FRIEND_LIST;
    } else if (wifiHit(x, y, 98, 305, 270, 55)) friendsView = FRIEND_DETAIL;
    else return;
  } else if (friendsView == FRIEND_REQUESTS) {
    if (friendDemoRequest && wifiHit(x, y, 98, 215, 270, 54)) {
      friendDemoRequest = false;
      if (friendPreviewCount < FRIEND_DEMO_CAP) {
        friendPreview[friendPreviewCount++] = {"DEMO0003", "GARY", 0};
      }
      friendsView = FRIEND_LIST;
    } else if (friendDemoRequest && wifiHit(x, y, 98, 285, 270, 54)) {
      friendDemoRequest = false;
    } else if (wifiHit(x, y, 132, 397, 202, 34)) friendsView = FRIEND_HOME;
    else return;
  }
  friendMessage[0] = 0;
  sfxPlay(SFX_TAP);
}
