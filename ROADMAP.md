# TamaToys TamaPoke roadmap

## Phase 1: Wi-Fi settings

- [x] Settings hub that keeps the existing time, sound and language controls.
- [x] On-device Wi-Fi On/Off, auto-connect, scan, saved networks and forget confirmation.
- [x] Case-sensitive touchscreen password keyboard; no passwords in source or serial logs.
- [x] Separate home Wi-Fi manager from the existing ESP-NOW LAN radio.
- [x] Emulator sample networks and a headless UI regression test.
- [ ] Verify an actual WPA connection, reconnect and saved credentials on Waveshare hardware.
- [ ] Test LAN battles before and after an internet Wi-Fi connection on two boards.
- [ ] Localize the new Wi-Fi UI into the six supported languages.

## Phase 2: Player profiles and friends

- [x] Prompt for trainer name after first starter choice or Factory Reset; New Game preserves the previous name.
- [x] Implement automatic registration via HTTPS on first Wi-Fi connection (hardware test pending).
- [x] Assign a server-unique friend code and a separate secret device token; store token only on device and hashed on server.
- [x] Friends menu interface on Player screen: pending code, code-entry keyboard, lists, request and removal-confirmation screens; emulator-only demo interactions.
- [ ] Wire those interfaces to the real authenticated API (requests, codes, last-seen and challenges).
- [ ] Friends-only last-seen information; allow players to opt out.
- [x] New Game clears pet progress but preserves name, online token/code/friendships and saved Wi-Fi.
- [x] Factory Reset confirms Yes/No, deletes the server trainer first (Wi-Fi required for an existing online token), then clears game, device identity and Wi-Fi.
- [ ] Verify automatic registration and Factory Reset against a real ESP32 over Wi-Fi with valid TLS certificate and system time.

## Phase 3: Asynchronous internet battles

- [ ] Server snapshots both teams when a battle begins.
- [ ] Each player submits hidden moves; resolve a turn only after both arrive.
- [ ] One new challenge per day, seven-day inactivity expiration.
- [ ] Server-authoritative move resolution with parity tests against the existing battle engine.
- [ ] Preserve current LAN and offline modes.

## Hosting

- [x] Implement a minimal HTTPS PHP API in a `server/` folder in this same repository (registration/profile/deletion).
- [ ] Create database migrations for profiles, devices, friendships, battles and turns.
- [x] Deploy initial registration/profile API to IONOS MariaDB; deploy the deletion update next.
- [ ] Configure secrets only on the host, never in firmware or GitHub.
