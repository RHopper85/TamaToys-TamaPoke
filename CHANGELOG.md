# TamaToys TamaPoke changelog

## Unreleased (`dev`)

### Trainer onboarding and Friends UI (Phase 2)

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

No live API, friend-code registration or WAN battles have been implemented yet.
