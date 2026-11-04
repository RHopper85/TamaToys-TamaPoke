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

- [x] Prompt for trainer name after first starter choice, including after a pet factory wipe; offline play still works.
- [ ] Register a named player silently when they first connect to the TamaToys API.
- [ ] Create a server-unique, permanent friend code and a separate secret device token.
- [x] Friends menu interface on Player screen: pending code, code-entry keyboard, lists, request and removal-confirmation screens; emulator-only demo interactions.
- [ ] Wire those interfaces to the real authenticated API (requests, codes, last-seen and challenges).
- [ ] Friends-only last-seen information; allow players to opt out.
- [ ] Keep online identity separate from local Pokemon/game resets; provide account recovery.

## Phase 3: Asynchronous internet battles

- [ ] Server snapshots both teams when a battle begins.
- [ ] Each player submits hidden moves; resolve a turn only after both arrive.
- [ ] One new challenge per day, seven-day inactivity expiration.
- [ ] Server-authoritative move resolution with parity tests against the existing battle engine.
- [ ] Preserve current LAN and offline modes.

## Hosting

- [ ] Implement an HTTPS PHP API in a `server/` folder in this same repository.
- [ ] Create database migrations for profiles, devices, friendships, battles and turns.
- [ ] Deploy API to existing IONOS shared hosting using its MariaDB database.
- [ ] Configure secrets only on the host, never in firmware or GitHub.
