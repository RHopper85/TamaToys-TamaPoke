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

static void wifiButton(int x, int y, int w, int h, const char *label,
                       bool selected = false, bool active = true,
                       uint8_t size = 2) {
  uint16_t bg = !active ? UI_TRACK : selected ? UI_BAR_OK : UI_WHITE;
  gfx->fillRoundRect(x, y, w, h, 11, bg);
  gfx->drawRoundRect(x, y, w, h, 11, UI_INK);
  gfx->setTextColor(selected && active ? UI_BG_DAY : UI_INK);
  gfx->setTextSize(size);
  gfx->setCursor(x + (w - (int)strlen(label) * 6 * size) / 2,
                 y + (h - 8 * size) / 2);
  gfx->print(label);
}

static void wifiFrame(const char *title) {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  wifiLabel(title, 46, 3);
}

static void wifiBackButton() {
  wifiButton(132, 397, 202, 34, "BACK");
}

static void wifiNotice(const char *msg) {
  strncpy(wifiMessage, msg, sizeof(wifiMessage) - 1);
  wifiMessage[sizeof(wifiMessage) - 1] = 0;
  wifiMessageUntil = millis() + 4000;
}

void renderSettingsHub() {
  wifiFrame("SETTINGS");
  wifiLabel("Device options", 99, 2, UI_INK_SOFT);
  wifiButton(89, 142, 288, 62, "TIME / SOUND");
  wifiButton(89, 218, 288, 62, "WI-FI");
  wifiButton(89, 294, 288, 62, "BACK");
  wifiLabel("TamaToys", 409, 2, UI_INK_SOFT);
  gfx->flush();
}

void settingsHubTap(int16_t x, int16_t y) {
  if (wifiHit(x, y, 89, 142, 288, 62)) {
    openClock();
    sfxPlay(SFX_TAP);
  } else if (wifiHit(x, y, 89, 218, 288, 62)) {
    wifiOpen = true;
    wifiView = WIFI_HOME;
    wifiPage = 0;
    sfxPlay(SFX_TAP);
  } else if (wifiHit(x, y, 89, 294, 288, 62)) {
    settingsOpen = false;
    sfxPlay(SFX_TAP);
  }
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
  wifiPage = 0;
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

static void renderWifiHome() {
  wifiFrame("WI-FI SETTINGS");
  wifiLabel(wifiStatusText(), 93, 2,
            TamaWifi::state() == TamaWifi::State::CONNECTED ? UI_BAR_OK : UI_INK_SOFT);
  if (TamaWifi::state() == TamaWifi::State::CONNECTED) {
    char name[25];
    snprintf(name, sizeof(name), "%.20s", TamaWifi::connectedName());
    wifiLabel(name, 115, 1);
  }
  wifiButton(89, 137, 288, 49,
             TamaWifi::enabled() ? "WI-FI: ON" : "WI-FI: OFF",
             TamaWifi::enabled());
  wifiButton(89, 198, 288, 49,
             TamaWifi::autoConnect() ? "AUTO: ON" : "AUTO: OFF",
             TamaWifi::autoConnect());
  wifiButton(89, 259, 288, 49, "AVAILABLE NETWORKS",
             false, TamaWifi::enabled(), 2);
  char saved[25];
  snprintf(saved, sizeof(saved), "SAVED NETWORKS (%d)", TamaWifi::savedCount());
  wifiButton(89, 320, 288, 49, saved);
  wifiBackButton();
#ifndef ARDUINO_ARCH_ESP32
  wifiLabel("EMULATOR: SAMPLE WI-FI", 441, 1, UI_INK_SOFT);
#endif
}

static void wifiNetworkRow(int row, const char *ssid, bool secured,
                           bool saved, int rssi) {
  int y = WIFI_ROW_Y(row);
  gfx->fillRoundRect(WIFI_ROW_X, y, WIFI_ROW_W, WIFI_ROW_H, 9, UI_WHITE);
  gfx->drawRoundRect(WIFI_ROW_X, y, WIFI_ROW_W, WIFI_ROW_H, 9, UI_INK);
  char shown[23];
  snprintf(shown, sizeof(shown), "%.19s", ssid);
  gfx->setTextSize(2);
  gfx->setTextColor(UI_INK);
  gfx->setCursor(WIFI_ROW_X + 10, y + 8);
  gfx->print(shown);
  gfx->setTextSize(1);
  gfx->setCursor(WIFI_ROW_X + 10, y + 31);
  if (saved) gfx->print("SAVED  ");
  gfx->print(secured ? "SECURED" : "OPEN");
  if (rssi != 0) {
    char signal[13];
    snprintf(signal, sizeof(signal), "%d dBm", rssi);
    gfx->setCursor(WIFI_ROW_X + WIFI_ROW_W - 67, y + 33);
    gfx->print(signal);
  }
  gfx->setTextSize(2);
  gfx->setCursor(WIFI_ROW_X + WIFI_ROW_W - 24, y + 8);
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
  uint8_t pages = wifiPages();
  if (wifiPage >= pages) wifiPage = pages - 1;
  if (!total) {
    wifiLabel(nearby && TamaWifi::state() == TamaWifi::State::SCANNING ?
              "Searching..." : "No networks found", 224, 2, UI_INK_SOFT);
  }
  for (int row = 0; row < WIFI_ROWS_PER_PAGE; ++row) {
    int i = (int)wifiPage * WIFI_ROWS_PER_PAGE + row;
    if (i >= total) break;
    if (nearby) {
      TamaWifi::Network n = TamaWifi::networkAt(i);
      wifiNetworkRow(row, n.ssid, n.secure, n.saved, n.rssi);
    } else {
      wifiNetworkRow(row, TamaWifi::savedName(i), TamaWifi::savedSecure(i), true, 0);
    }
  }
  if (pages > 1) {
    char info[24];
    snprintf(info, sizeof(info), "Page %d/%d - drag rim",
             wifiPage + 1, pages);
    wifiLabel(info, 377, 1, UI_INK_SOFT);
  }
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
#define WIFI_KEY_X 53
#define WIFI_KEY_Y 157
#define WIFI_KEY_STEP_X 45
#define WIFI_KEY_STEP_Y 47

static void renderWifiPassword() {
  wifiFrame("PASSWORD");
  char ssid[27];
  snprintf(ssid, sizeof(ssid), "%.23s", wifiSelectedSsid);
  wifiLabel(ssid, 77, 2);
  gfx->fillRoundRect(77, 98, 312, 44, 10, UI_WHITE);
  gfx->drawRoundRect(77, 98, 312, 44, 10, UI_INK);
  char shown[23];
  size_t start = wifiPassLen > 18 ? wifiPassLen - 18 : 0;
  for (size_t i = start; i < wifiPassLen; ++i)
    shown[i - start] = wifiShowPassword ? wifiPassword[i] : '*';
  shown[wifiPassLen - start] = '\0';
  gfx->setTextSize(2);
  gfx->setTextColor(UI_INK);
  gfx->setCursor(88, 111);
  gfx->print(wifiPassLen ? shown : "_");
  wifiButton(315, 102, 68, 36, wifiShowPassword ? "HIDE" : "SHOW",
             false, true, 1);
  char length[24];
  snprintf(length, sizeof(length), "%u / 64 chars", wifiPassLen);
  wifiLabel(length, 143, 1, UI_INK_SOFT);
  const char *keys = WIFI_KEYS[wifiKeyPage % 4];
  size_t keyCount = strlen(keys);
  for (int i = 0; i < 32; ++i) {
    if ((size_t)i >= keyCount) break;
    int x = WIFI_KEY_X + (i % 8) * WIFI_KEY_STEP_X;
    int y = WIFI_KEY_Y + (i / 8) * WIFI_KEY_STEP_Y;
    gfx->fillRoundRect(x, y, 41, 42, 5, UI_WHITE);
    gfx->drawRoundRect(x, y, 41, 42, 5, UI_INK);
    gfx->setTextColor(UI_INK);
    gfx->setTextSize(2);
    gfx->setCursor(x + 14, y + 13);
    char k[] = { keys[i], 0 };
    gfx->print(k);
  }
  for (int i = 0; i < 4; ++i)
    wifiButton(78 + i * 79, 353, 73, 34, WIFI_MODES[i],
               wifiKeyPage == i, true, 1);
  wifiButton(111, 397, 75, 31, "SPACE", false, true, 1);
  wifiButton(196, 397, 75, 31, "DEL", false, true, 1);
  wifiButton(281, 397, 75, 31, "JOIN", true, true, 1);
  if (wifiMessageUntil > millis())
    wifiLabel(wifiMessage, 438, 1, UI_BAR_BAD);
  else
    wifiLabel("Swipe down to cancel", 438, 1, UI_INK_SOFT);
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
  if (y >= WIFI_KEY_Y && y < WIFI_KEY_Y + 4 * WIFI_KEY_STEP_Y &&
      x >= WIFI_KEY_X && x < WIFI_KEY_X + 8 * WIFI_KEY_STEP_X) {
    int col = (x - WIFI_KEY_X) / WIFI_KEY_STEP_X;
    int row = (y - WIFI_KEY_Y) / WIFI_KEY_STEP_Y;
    int i = row * 8 + col;
    const char *keys = WIFI_KEYS[wifiKeyPage % 4];
    if (i < (int)strlen(keys) && wifiPassLen < sizeof(wifiPassword) - 1) {
      wifiPassword[wifiPassLen++] = keys[i];
      wifiPassword[wifiPassLen] = '\0';
    }
    return;
  }
  if (y >= 353 && y <= 387) {
    for (int i = 0; i < 4; ++i)
      if (wifiHit(x, y, 78 + i * 79, 353, 73, 34)) {
        wifiKeyPage = i;
        return;
      }
  }
  if (wifiHit(x, y, 111, 397, 75, 31) &&
      wifiPassLen < sizeof(wifiPassword) - 1) {
    wifiPassword[wifiPassLen++] = ' ';
    wifiPassword[wifiPassLen] = '\0';
  } else if (wifiHit(x, y, 196, 397, 75, 31)) {
    if (wifiPassLen) wifiPassword[--wifiPassLen] = '\0';
  } else if (wifiHit(x, y, 281, 397, 75, 31)) {
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
  if (wifiHit(x, y, 132, 397, 202, 34)) {
    wifiBack();
    sfxPlay(SFX_TAP);
    return;
  }
  if (wifiView == WIFI_HOME) {
    if (wifiHit(x, y, 89, 137, 288, 49)) {
      TamaWifi::setEnabled(!TamaWifi::enabled());
      sfxPlay(SFX_TAP);
    } else if (wifiHit(x, y, 89, 198, 288, 49)) {
      TamaWifi::setAutoConnect(!TamaWifi::autoConnect());
      sfxPlay(SFX_TAP);
    } else if (wifiHit(x, y, 89, 259, 288, 49)) {
      if (!TamaWifi::enabled()) { sfxPlay(SFX_DENY); return; }
      wifiView = WIFI_NEARBY;
      wifiPage = 0;
      TamaWifi::scan();
      sfxPlay(SFX_TAP);
    } else if (wifiHit(x, y, 89, 320, 288, 49)) {
      wifiView = WIFI_SAVED;
      wifiPage = 0;
      sfxPlay(SFX_TAP);
    }
    return;
  }
  if (wifiView == WIFI_NEARBY || wifiView == WIFI_SAVED) {
    if (wifiView == WIFI_NEARBY && wifiHit(x, y, 317, 76, 73, 32)) {
      TamaWifi::scan();
      wifiPage = 0;
      return;
    }
    for (int row = 0; row < WIFI_ROWS_PER_PAGE; ++row) {
      if (!wifiHit(x, y, WIFI_ROW_X, WIFI_ROW_Y(row),
                   WIFI_ROW_W, WIFI_ROW_H)) continue;
      int i = (int)wifiPage * WIFI_ROWS_PER_PAGE + row;
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
