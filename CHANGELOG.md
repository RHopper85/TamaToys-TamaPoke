# TamaToys TamaPoke changelog

## Unreleased (`dev`)

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
