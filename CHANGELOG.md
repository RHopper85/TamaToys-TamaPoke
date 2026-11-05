# TamaToys TamaPoke changelog

## Unreleased (`dev`)

### Online identity and reset controls

- Added first Wi-Fi registration to the IONOS HTTPS API, with device-generated private token persisted before the request so network retries do not create duplicate accounts. The assigned friend code appears only inside Friends.
- Added `NEW GAME` to Settings: confirmation erases local Pokemon, party/box, Dex, badges, items, currency and game progress, preserving the trainer name, token, friend code, server friendships and Wi-Fi profiles.
- Added `FACTORY RESET` with a separate irreversible Yes/No warning. A registered device must connect to Wi-Fi and successfully delete its IONOS account before device data is wiped. Server deletion cascades to friend requests and friendships.
- Added TLS root validation, network-time synchronization, a fail-closed deletion path and a headless reset UI regression test. Real hardware TLS and Wi-Fi testing is still required.
- Changed serial `WIPE` to the safe New Game semantics so a debugging action cannot accidentally orphan an IONOS account.

### Trainer onboarding and Friends UI (Phase 2)

- Added a Back button on the first-run starter-selection screen so players can change their starting region before choosing a Pokemon; returning preserves the current region-list page (including pages 2 and 3).
- Added the missing mandatory trainer-name prompt immediately after choosing the first starter, including after a full pet wipe.
- Reused the trainer-name keyboard and fixed player-card input routing so tapping the rename keyboard actually enters letters.
- Added Friends on the Player card: code-pending home, friend list, details, friend-code entry, requests and removal confirmation.
- Limited mock friend addition, request acceptance and last-active previews to the emulator. The real device does not manufacture accounts or relationships without the future HTTPS service.
- Added a headless onboarding/Friends regression suite and updated the touch test for the extra first-run step.

### Wi-Fi Settings foundation

- Introduced a Settings hub; the existing clock/sound/language editor remains available.
- Added on-device Wi-Fi On/Off, auto-connect toggle, network scanning, saved networks, connection status and forgetting a saved network.
- Added a case-sensitive Wi-Fi password keyboard with lowercase, uppercase, numbers, symbols, masking and a show/hide control.
- Stored up to five successful network credentials in a dedicated `tt_wifi` NVS namespace, away from pet saves.
- Coordinated the Wi-Fi manager with ESP-NOW LAN radio ownership.
- Added simulated Wi-Fi networks to the desktop emulator and a headless UI test.

### Baseline

- Removed duplicate `z_battle.cpp` and corrected the release badge.
- Verified the original ESP32-S3 firmware builds with appropriate flash/PSRAM/partition settings.

IONOS registration and deletion are implemented. Friend request/list endpoints and WAN battles are not yet implemented.
