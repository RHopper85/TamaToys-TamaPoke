// TamaToys on-device Settings and Wi-Fi UI.
// The emulator uses sample networks; only the physical ESP32 scans/connects.
// No real Wi-Fi password is ever shown in serial output.

#define WIFI_ROWS_PER_PAGE 4
#define WIFI_ROW_X 76
#define WIFI_ROW_W 314
#define WIFI_ROW_H 48
#define WIFI_ROW_Y(i) (119 + (i) * 62)

static bool wifiHit(int x, int y, int bx, int by, int bw, int bh) {
  return x >= bx && x <= bx + bw && y >= by && y <= by + bh;
}

static void wifiLabel(const char *label, int y, uint8_t size = 2,
                      uint16_t color = UI_INK) {
  gfx->setTextSize(size);
  gfx->setTextColor(color);
  gfx->setCursor(CX - (int)strlen(label) * 3 * size, y);
  gfx->print(label);
}

// Shared GBA styling for Settings and Wi-Fi. All touch regions remain defined
// in their existing tap handlers; drawing changes must not alter them.
static void wifiButton(int x, int y, int w, int h, const char *label,
                       bool selected = false, bool active = true,
                       uint8_t size = 2) {
  const uint16_t accent = !active ? GBA_SHADOW : selected ? GBA_GREEN : GBA_BLUE;
  const uint16_t fill = !active ? GBA_DISABLED : selected ? GBA_MIST : GBA_CREAM;
  gbaFrame(gfx, x, y, w, h, fill, accent);
  // Keep long localized strings within the button, without changing the
  // layout or the button's hit area.
  while (size > 1 && (int)strlen(label) * 6 * size > w - 20) --size;
  gfx->setTextColor(active ? GBA_NAVY : GBA_SHADOW);
  gfx->setTextSize(size);
  gfx->setCursor(x + (w - (int)strlen(label) * 6 * size) / 2,
                 y + (h - 8 * size) / 2);
  gfx->print(label);
}

static void wifiFrame(const char *title) {
  gbaBackdrop(gfx, GBA_WATER, GBA_MIST);
  // Text is centered by the common ribbon helper, not a hardcoded offset.
  const uint8_t scale = strlen(title) > 23 ? 1 : 2;
  gbaRibbon(gfx, title, 98, 30, 270, 46, GBA_BLUE, scale);
}

static void wifiBackButton() {
  // Finger-sized on the 466x466 display, consistent across Wi-Fi screens.
  wifiButton(132, 389, 202, 46, "BACK");
}

static void wifiNotice(const char *msg) {
  strncpy(wifiMessage, msg, sizeof(wifiMessage) - 1);
  wifiMessage[sizeof(wifiMessage) - 1] = 0;
  wifiMessageUntil = millis() + 4000;
}

// Both reset paths block the shutdown handler from restoring the old creature
// after NVS has been erased. New Game deliberately keeps Wi-Fi and tt_online.
bool startNewGame() {
  if (!TamaOnline::rememberName(pet.trainerName[0] ? pet.trainerName : "TRAINER"))
    return false;
  if (!pet.factoryReset()) return false;
  saveInhibited = true;
  delay(100);
  ESP.restart();
  return true; // only reached by an emulator/test restart stub
}

TamaOnline::DeleteResult startFactoryReset() {
  TamaOnline::DeleteResult deleted = TamaOnline::deleteAccount();
  if (deleted != TamaOnline::DeleteResult::DELETED &&
      deleted != TamaOnline::DeleteResult::NO_ACCOUNT) return deleted;
  // Server deletion is already complete (or no account ever existed).
  // Only now erase local identity, saved networks and the entire game save.
  if (!TamaOnline::clearIdentity() || !TamaWifi::clearAll() || !pet.factoryReset())
    return TamaOnline::DeleteResult::STORAGE_ERROR;
  saveInhibited = true;
  delay(100);
  ESP.restart();
  return deleted;
}

static void renderResetConfirm() {
  const bool factory = resetView == RESET_FACTORY;
  wifiFrame(resetView == RESET_FAILED ? "RESET FAILED" :
            factory ? "FACTORY RESET?" : "NEW GAME?");
  if (resetView == RESET_FAILED) {
    wifiLabel(resetMessage, 177, 1, UI_BAR_BAD);
    wifiLabel("Your game was not restarted", 214, 1, UI_INK_SOFT);
    wifiButton(112, 301, 242, 54, "BACK");
    gfx->flush();
    return;
  }
  wifiLabel("THIS CANNOT BE UNDONE", 112, 2, UI_BAR_BAD);
  if (factory) {
    wifiLabel("ERASE ALL GAME PROGRESS", 157, 2);
    wifiLabel("Remove account and friends", 190, 1);
    wifiLabel("Erase trainer + saved Wi-Fi", 211, 1);
    wifiLabel(TamaOnline::hasToken() ?
              (TamaWifi::state() == TamaWifi::State::CONNECTED ?
                "Online deletion required" : "CONNECT WI-FI FIRST") :
              "No online account on device", 245, 1, UI_BAR_BAD);
  } else {
    wifiLabel("ERASE ALL GAME PROGRESS", 157, 2);
    wifiLabel("Keep trainer and friend code", 190, 1);
    wifiLabel("Keep friends and saved Wi-Fi", 211, 1);
    wifiLabel("Choose a new starter afterward", 245, 1, UI_INK_SOFT);
  }
  wifiButton(93, 288, 130, 57, "NO", true);
  wifiButton(243, 288, 130, 57, "YES");
  wifiLabel("Please confirm", 375, 1, UI_INK_SOFT);
  gfx->flush();
}

// Shared coordinates are used by both drawing and hit-testing below.
#define SETTINGS_ROW_TOP 104
#define SETTINGS_ROW_BOTTOM 373
#define SETTINGS_ROW_STEP 86
#define SETTINGS_ROW_H 74
#define SETTINGS_ROW_COUNT 4
void renderSettingsHub() {
  if (resetView != RESET_NONE) { renderResetConfirm(); return; }
  wifiFrame("SETTINGS");
  wifiLabel("SWIPE TO SCROLL", 85, 1, GBA_TEAL);
  static const char *const labels[SETTINGS_ROW_COUNT] = {
    "TIME / SOUND", "WI-FI", "NEW GAME", "FACTORY RESET"
  };
  settingsScroll = (int16_t)uiListClamp(settingsScroll);
  for (int i = 0; i < SETTINGS_ROW_COUNT; ++i) {
    const int y = SETTINGS_ROW_TOP + i * SETTINGS_ROW_STEP - settingsScroll;
    if (y < SETTINGS_ROW_TOP || y + SETTINGS_ROW_H > SETTINGS_ROW_BOTTOM) continue;
    wifiButton(99, y, 268, SETTINGS_ROW_H, labels[i]);
  }
  uiListDrawBar(SETTINGS_ROW_TOP, SETTINGS_ROW_BOTTOM,
                SETTINGS_ROW_COUNT, SETTINGS_ROW_STEP, settingsScroll);
  wifiButton(132, 389, 202, 46, "BACK");
  gfx->flush();
}

void settingsHubTap(int16_t x, int16_t y) {
  if (resetView != RESET_NONE) {
    if (resetView == RESET_FAILED) {
      if (wifiHit(x, y, 112, 301, 242, 54)) resetView = RESET_NONE;
      return;
    }
    if (wifiHit(x, y, 93, 288, 130, 57)) {
      resetView = RESET_NONE;
      sfxPlay(SFX_TAP);
    } else if (wifiHit(x, y, 243, 288, 130, 57)) {
      if (resetView == RESET_NEW_GAME) {
        if (!startNewGame()) {
          strcpy(resetMessage, "Could not erase game save");
          resetView = RESET_FAILED;
          sfxPlay(SFX_DENY);
        }
      } else {
        wifiFrame("FACTORY RESET");
        wifiLabel("Removing online account...", 198, 1);
        gfx->flush();
        TamaOnline::DeleteResult outcome = startFactoryReset();
        switch (outcome) {
          case TamaOnline::DeleteResult::WIFI_REQUIRED:
            strcpy(resetMessage, "Connect to Wi-Fi and retry"); break;
          case TamaOnline::DeleteResult::STORAGE_ERROR:
            strcpy(resetMessage, "NVS erase failed. Retry reset"); break;
          default:
            strcpy(resetMessage, "Server deletion failed. Retry"); break;
        }
        resetView = RESET_FAILED;
        sfxPlay(SFX_DENY);
      }
    }
    return;
  }
  if (wifiHit(x, y, 132, 389, 202, 46)) {
    settingsOpen = false;
    sfxPlay(SFX_TAP);
    return;
  }
  const int choice = uiListIndexAt(y, SETTINGS_ROW_TOP, SETTINGS_ROW_BOTTOM,
                                   SETTINGS_ROW_STEP, SETTINGS_ROW_H,
                                   settingsScroll, SETTINGS_ROW_COUNT);
  if (choice < 0 || !wifiHit(x, y, 99, SETTINGS_ROW_TOP, 268,
                              SETTINGS_ROW_BOTTOM - SETTINGS_ROW_TOP)) return;
  switch (choice) {
    case 0: openClock(); break;
    case 1:
      wifiOpen = true;
      wifiView = WIFI_HOME;
      wifiPage = 0;
      wifiHomeScroll = 0;
      break;
    case 2: resetView = RESET_NEW_GAME; break;
    case 3: resetView = RESET_FACTORY; break;
  }
  sfxPlay(SFX_TAP);
}

uint8_t wifiPages() {
  int n = wifiView == WIFI_NEARBY ? TamaWifi::networkCount()
         : wifiView == WIFI_SAVED ? TamaWifi::savedCount() : 0;
  int pages = (n + WIFI_ROWS_PER_PAGE - 1) / WIFI_ROWS_PER_PAGE;
  return (uint8_t)(pages > 0 ? pages : 1);
}

void wifiBack() {
  wifiForgetConfirm = false;
  if (wifiView == WIFI_PASSWORD) {
    memset(wifiPassword, 0, sizeof(wifiPassword));
    wifiPassLen = 0;
    wifiView = WIFI_NEARBY;
  } else if (wifiView == WIFI_SAVED_DETAIL) {
    wifiView = WIFI_SAVED;
  } else if (wifiView == WIFI_NEARBY || wifiView == WIFI_SAVED) {
    wifiView = WIFI_HOME;
  } else {
    wifiOpen = false;
  }
  wifiPage = 0;wifiScroll=0;
}

static const char *wifiStatusText() {
  switch (TamaWifi::state()) {
    case TamaWifi::State::OFF: return "Wi-Fi is off";
    case TamaWifi::State::SCANNING: return "Scanning...";
    case TamaWifi::State::CONNECTING: return "Connecting...";
    case TamaWifi::State::CONNECTED: return "Connected";
    case TamaWifi::State::FAILED: return "Connection failed";
    case TamaWifi::State::LAN: return "LAN battle active";
    default: return "Not connected";
  }
}

#define WIFI_HOME_ROW_TOP 140
#define WIFI_HOME_ROW_BOTTOM 375
#define WIFI_HOME_ROW_STEP 80
#define WIFI_HOME_ROW_H 68
static void renderWifiHome() {
  wifiFrame("WI-FI SETTINGS");
  gbaFrame(gfx, 87, 82, 292, 48, GBA_CREAM,
           TamaWifi::state() == TamaWifi::State::CONNECTED ? GBA_GREEN : GBA_TEAL);
  wifiLabel(wifiStatusText(), 92, 2,
            TamaWifi::state() == TamaWifi::State::CONNECTED ? GBA_GREEN : GBA_NAVY);
  if (TamaWifi::state() == TamaWifi::State::CONNECTED) {
    char name[25];
    snprintf(name, sizeof(name), "%.20s", TamaWifi::connectedName());
    wifiLabel(name, 114, 1, GBA_TEAL);
  }
  char saved[25];
  snprintf(saved, sizeof(saved), "SAVED NETWORKS (%d)", TamaWifi::savedCount());
  const char *const labels[4] = {
    TamaWifi::enabled() ? "WI-FI: ON" : "WI-FI: OFF",
    TamaWifi::autoConnect() ? "AUTO: ON" : "AUTO: OFF",
    "AVAILABLE NETWORKS", saved
  };
  wifiHomeScroll = (int16_t)uiListClamp(wifiHomeScroll);
  for (int i = 0; i < 4; ++i) {
    const int y = WIFI_HOME_ROW_TOP + i * WIFI_HOME_ROW_STEP - wifiHomeScroll;
    if (y < WIFI_HOME_ROW_TOP || y + WIFI_HOME_ROW_H > WIFI_HOME_ROW_BOTTOM) continue;
    wifiButton(89, y, 288, WIFI_HOME_ROW_H, labels[i],
               i == 0 ? TamaWifi::enabled() : i == 1 ? TamaWifi::autoConnect() : false,
               i != 2 || TamaWifi::enabled());
  }
  uiListDrawBar(WIFI_HOME_ROW_TOP, WIFI_HOME_ROW_BOTTOM, 4,
                WIFI_HOME_ROW_STEP, wifiHomeScroll);
  wifiBackButton();
#ifndef ARDUINO_ARCH_ESP32
  wifiLabel("EMULATOR: SAMPLE WI-FI", 443, 1, UI_INK_SOFT);
#endif
}

static void wifiNetworkRow(int y, const char *ssid, bool secured,
                           bool saved, int rssi) {
  gbaFrame(gfx, WIFI_ROW_X, y, WIFI_ROW_W, WIFI_ROW_H,
           GBA_CREAM, saved ? GBA_GREEN : GBA_BLUE);
  char shown[21];
  snprintf(shown, sizeof(shown), "%.18s", ssid);
  gfx->setTextSize(2);
  gfx->setTextColor(GBA_NAVY);
  gfx->setCursor(WIFI_ROW_X + 13, y + 7);
  gfx->print(shown);
  gfx->setTextSize(1);
  gfx->setCursor(WIFI_ROW_X + 13, y + 31);
  gfx->setTextColor(GBA_TEAL);
  if (saved) gfx->print("SAVED  ");
  gfx->print(secured ? "SECURED" : "OPEN");
  if (rssi != 0) {
    char signal[13];
    snprintf(signal, sizeof(signal), "%d dBm", rssi);
    gfx->setCursor(WIFI_ROW_X + WIFI_ROW_W - 80, y + 31);
    gfx->print(signal);
  }
  gfx->setTextColor(GBA_BLUE);
  gfx->setTextSize(2);
  gfx->setCursor(WIFI_ROW_X + WIFI_ROW_W - 25, y + 7);
  gfx->print(">");
}

static void renderWifiList() {
  bool nearby = wifiView == WIFI_NEARBY;
  wifiFrame(nearby ? "NEARBY WI-FI" : "SAVED WI-FI");
  if (nearby) {
    wifiLabel(TamaWifi::state() == TamaWifi::State::SCANNING ?
              "SCANNING..." : "Tap a network to join", 89, 1, UI_INK_SOFT);
    wifiButton(317, 76, 73, 32, "SCAN", false, true, 1);
  } else {
    wifiLabel("Tap to connect or forget", 88, 1, UI_INK_SOFT);
  }
  int total = nearby ? TamaWifi::networkCount() : TamaWifi::savedCount();
  wifiScroll=(int16_t)uiListClamp(wifiScroll);
  if (!total) {
    wifiLabel(nearby && TamaWifi::state() == TamaWifi::State::SCANNING ?
              "Searching..." : "No networks found", 224, 2, UI_INK_SOFT);
  }
  for (int i=0;i<total;i++) {
    const int y=WIFI_ROW_Y(i)-wifiScroll;
    if(y<119||y+WIFI_ROW_H>370)continue;
    if (nearby) {
      TamaWifi::Network n = TamaWifi::networkAt(i);
      wifiNetworkRow(y, n.ssid, n.secure, n.saved, n.rssi);
    } else {
      wifiNetworkRow(y, TamaWifi::savedName(i), TamaWifi::savedSecure(i), true, 0);
    }
  }
  uiListDrawBar(119,370,total,62,wifiScroll);
  wifiBackButton();
  uiChrome();
}

static void renderWifiSavedDetail() {
  wifiFrame("SAVED NETWORK");
  char shown[26];
  snprintf(shown, sizeof(shown), "%.22s", wifiSelectedSsid);
  wifiLabel(shown, 125, 2);
  if (!wifiForgetConfirm) {
    wifiButton(99, 192, 268, 56, "CONNECT",
               false, TamaWifi::enabled());
    wifiButton(99, 260, 268, 56, "FORGET NETWORK");
  } else {
    wifiLabel("Remove this saved Wi-Fi?", 181, 2, UI_BAR_BAD);
    wifiButton(99, 254, 127, 56, "CANCEL");
    wifiButton(240, 254, 127, 56, "REMOVE", true);
  }
  wifiBackButton();
}

// A separate password keyboard is needed: the existing trainer keyboard only
// accepts short uppercase names, not case-sensitive WPA passwords.
static const char *const WIFI_KEYS[] = {
  "qwertyuiopasdfghjklzxcvbnm",
  "QWERTYUIOPASDFGHJKLZXCVBNM",
  "0123456789!@#$%^&*()[]{}+-=_:;.,?",
  "~\\|/'\"<>?;:,.@#%^&*()-_+=[]{}!"
};
static const char *const WIFI_MODES[] = { "abc", "ABC", "123", "#+=" };
// Seven columns make keys ~30% wider than the previous eight-column board.
// Five rows still cover all 32 punctuation keys without changing their order.
#define WIFI_KEY_X 51
#define WIFI_KEY_Y 151
#define WIFI_KEY_COLS 7
#define WIFI_KEY_ROWS 5
#define WIFI_KEY_STEP_X 52
#define WIFI_KEY_STEP_Y 43
#define WIFI_KEY_W 48
#define WIFI_KEY_H 40

static void renderWifiPassword() {
  wifiFrame("PASSWORD");
  char ssid[27];
  snprintf(ssid, sizeof(ssid), "%.23s", wifiSelectedSsid);
  wifiLabel(ssid, 82, 1, GBA_TEAL);
  gbaFrame(gfx, 77, 98, 312, 44, GBA_WHITE, GBA_YELLOW);
  char shown[23];
  size_t start = wifiPassLen > 18 ? wifiPassLen - 18 : 0;
  for (size_t i = start; i < wifiPassLen; ++i)
    shown[i - start] = wifiShowPassword ? wifiPassword[i] : '*';
  shown[wifiPassLen - start] = '\0';
  gfx->setTextSize(2);
  gfx->setTextColor(GBA_NAVY);
  gfx->setCursor(88, 111);
  gfx->print(wifiPassLen ? shown : "_");
  wifiButton(315, 102, 68, 36, wifiShowPassword ? "HIDE" : "SHOW",
             false, true, 1);
  char length[24];
  snprintf(length, sizeof(length), "%u / 64 chars", wifiPassLen);
  wifiLabel(length, 143, 1, UI_INK_SOFT);
  const char *keys = WIFI_KEYS[wifiKeyPage % 4];
  const size_t keyCount = strlen(keys);
  for (size_t i = 0; i < keyCount; ++i) {
    const int x = WIFI_KEY_X + (int)(i % WIFI_KEY_COLS) * WIFI_KEY_STEP_X;
    const int y = WIFI_KEY_Y + (int)(i / WIFI_KEY_COLS) * WIFI_KEY_STEP_Y;
    gfx->fillRoundRect(x, y + 2, WIFI_KEY_W, WIFI_KEY_H, 5, GBA_NAVY);
    gfx->fillRoundRect(x + 2, y, WIFI_KEY_W - 4, WIFI_KEY_H - 2, 4, GBA_CREAM);
    gfx->drawRoundRect(x + 2, y, WIFI_KEY_W - 4, WIFI_KEY_H - 2, 4, GBA_TEAL);
    gfx->setTextColor(GBA_NAVY);
    gfx->setTextSize(2);
    gfx->setCursor(x + (WIFI_KEY_W - 12) / 2, y + 12);
    char k[] = { keys[i], 0 };
    gfx->print(k);
  }
  for (int i = 0; i < 4; ++i)
    wifiButton(78 + i * 79, 372, 73, 34, WIFI_MODES[i],
               wifiKeyPage == i, true, 1);
  wifiButton(134, 410, 64, 34, "SPACE", false, true, 1);
  wifiButton(202, 410, 64, 34, "DEL", false, true, 1);
  wifiButton(270, 410, 64, 34, "JOIN", true, true, 1);
  if (wifiMessageUntil > millis())
    wifiLabel(wifiMessage, 448, 1, GBA_RED);
  else
    wifiLabel("Swipe down to cancel", 448, 1, UI_INK_SOFT);
}

static void wifiSelectNetwork(const TamaWifi::Network &network) {
  strncpy(wifiSelectedSsid, network.ssid, sizeof(wifiSelectedSsid) - 1);
  wifiSelectedSsid[sizeof(wifiSelectedSsid) - 1] = 0;
  if (network.saved) {
    if (TamaWifi::connect(network.ssid, nullptr)) {
      wifiView = WIFI_HOME;
      return;
    }
  } else if (!network.secure) {
    if (TamaWifi::connect(network.ssid, "")) {
      wifiView = WIFI_HOME;
      return;
    }
  } else {
    wifiView = WIFI_PASSWORD;
    wifiPassLen = 0;
    wifiPassword[0] = 0;
    wifiKeyPage = 0;
    wifiShowPassword = false;
    wifiMessageUntil = 0;
    return;
  }
  wifiNotice("Unable to connect");
}

static void wifiPasswordTap(int16_t x, int16_t y) {
  if (wifiHit(x, y, 315, 102, 68, 36)) {
    wifiShowPassword = !wifiShowPassword;
    return;
  }
  if (y >= WIFI_KEY_Y && y < WIFI_KEY_Y + WIFI_KEY_ROWS * WIFI_KEY_STEP_Y &&
      x >= WIFI_KEY_X && x < WIFI_KEY_X + WIFI_KEY_COLS * WIFI_KEY_STEP_X) {
    const int col = (x - WIFI_KEY_X) / WIFI_KEY_STEP_X;
    const int row = (y - WIFI_KEY_Y) / WIFI_KEY_STEP_Y;
    const int keyX = WIFI_KEY_X + col * WIFI_KEY_STEP_X;
    const int keyY = WIFI_KEY_Y + row * WIFI_KEY_STEP_Y;
    const int i = row * WIFI_KEY_COLS + col;
    const char *keys = WIFI_KEYS[wifiKeyPage % 4];
    // Gap between keys is deliberately not clickable.
    if (x < keyX + WIFI_KEY_W && y < keyY + WIFI_KEY_H &&
        i < (int)strlen(keys) && wifiPassLen < sizeof(wifiPassword) - 1) {
      wifiPassword[wifiPassLen++] = keys[i];
      wifiPassword[wifiPassLen] = '\0';
    }
    return;
  }
  if (y >= 372 && y <= 406) {
    for (int i = 0; i < 4; ++i)
      if (wifiHit(x, y, 78 + i * 79, 372, 73, 34)) {
        wifiKeyPage = i;
        return;
      }
  }
  if (wifiHit(x, y, 134, 410, 64, 34) &&
      wifiPassLen < sizeof(wifiPassword) - 1) {
    wifiPassword[wifiPassLen++] = ' ';
    wifiPassword[wifiPassLen] = '\0';
  } else if (wifiHit(x, y, 202, 410, 64, 34)) {
    if (wifiPassLen) wifiPassword[--wifiPassLen] = '\0';
  } else if (wifiHit(x, y, 270, 410, 64, 34)) {
    if (wifiPassLen < 8) {
      wifiNotice("Password needs 8+ chars");
      return;
    }
    if (TamaWifi::connect(wifiSelectedSsid, wifiPassword)) {
      memset(wifiPassword, 0, sizeof(wifiPassword));
      wifiPassLen = 0;
      wifiView = WIFI_HOME;
      sfxPlay(SFX_TAP);
    } else {
      wifiNotice("Unable to connect");
    }
  }
}

void renderWifi() {
  switch (wifiView) {
    case WIFI_NEARBY:
    case WIFI_SAVED: renderWifiList(); break;
    case WIFI_PASSWORD: renderWifiPassword(); break;
    case WIFI_SAVED_DETAIL: renderWifiSavedDetail(); break;
    default: renderWifiHome(); break;
  }
  gfx->flush();
}

void wifiTap(int16_t x, int16_t y) {
  if (wifiView == WIFI_PASSWORD) { wifiPasswordTap(x, y); return; }
  if (wifiHit(x, y, 132, 389, 202, 46)) {
    wifiBack();
    sfxPlay(SFX_TAP);
    return;
  }
  if (wifiView == WIFI_HOME) {
    const int choice = uiListIndexAt(y, WIFI_HOME_ROW_TOP, WIFI_HOME_ROW_BOTTOM,
                                     WIFI_HOME_ROW_STEP, WIFI_HOME_ROW_H,
                                     wifiHomeScroll, 4);
    if (choice < 0 || !wifiHit(x, y, 89, WIFI_HOME_ROW_TOP, 288,
                                WIFI_HOME_ROW_BOTTOM - WIFI_HOME_ROW_TOP)) return;
    if (choice == 0) {
      TamaWifi::setEnabled(!TamaWifi::enabled());
      sfxPlay(SFX_TAP);
    } else if (choice == 1) {
      TamaWifi::setAutoConnect(!TamaWifi::autoConnect());
      sfxPlay(SFX_TAP);
    } else if (choice == 2) {
      if (!TamaWifi::enabled()) { sfxPlay(SFX_DENY); return; }
      wifiView = WIFI_NEARBY;
      wifiPage = 0;wifiScroll=0;
      TamaWifi::scan();
      sfxPlay(SFX_TAP);
    } else if (choice == 3) {
      wifiView = WIFI_SAVED;
      wifiPage = 0;wifiScroll=0;
      sfxPlay(SFX_TAP);
    }
    return;
  }
  if (wifiView == WIFI_NEARBY || wifiView == WIFI_SAVED) {
    if (wifiView == WIFI_NEARBY && wifiHit(x, y, 317, 76, 73, 32)) {
      TamaWifi::scan();
      wifiPage = 0;wifiScroll=0;
      return;
    }
    int total=wifiView==WIFI_NEARBY?TamaWifi::networkCount():TamaWifi::savedCount();
    int i=uiListIndexAt(y,119,370,62,WIFI_ROW_H,wifiScroll,total);
    if(i>=0&&wifiHit(x,y,WIFI_ROW_X,119,WIFI_ROW_W,251)) {
      if (wifiView == WIFI_NEARBY && i < TamaWifi::networkCount()) {
        wifiSelectNetwork(TamaWifi::networkAt(i));
        sfxPlay(SFX_TAP);
      } else if (wifiView == WIFI_SAVED && i < TamaWifi::savedCount()) {
        strncpy(wifiSelectedSsid, TamaWifi::savedName(i),
                sizeof(wifiSelectedSsid) - 1);
        wifiSelectedSsid[sizeof(wifiSelectedSsid) - 1] = 0;
        wifiForgetConfirm = false;
        wifiView = WIFI_SAVED_DETAIL;
        sfxPlay(SFX_TAP);
      }
      return;
    }
    return;
  }
  if (wifiView == WIFI_SAVED_DETAIL) {
    if (wifiForgetConfirm) {
      if (wifiHit(x, y, 99, 254, 127, 56)) {
        wifiForgetConfirm = false;
      } else if (wifiHit(x, y, 240, 254, 127, 56)) {
        TamaWifi::forget(wifiSelectedSsid);
        wifiForgetConfirm = false;
        wifiView = WIFI_SAVED;
        wifiPage = 0;
        sfxPlay(SFX_TAP);
      }
    } else if (wifiHit(x, y, 99, 192, 268, 56)) {
      if (TamaWifi::connect(wifiSelectedSsid, nullptr)) wifiView = WIFI_HOME;
      else wifiNotice("Turn Wi-Fi on first");
    } else if (wifiHit(x, y, 99, 260, 268, 56)) {
      wifiForgetConfirm = true;
    }
  }
}
