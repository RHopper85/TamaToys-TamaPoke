# TamaToys TamaPoke changelog

## 2026-11-12 — Patch 7: Final GBA UI consistency pass
- Restyled Friends home, list, trainer details, requests, friend-code keyboard and removal confirmation using the shared GBA palette; emulator demo data remains clearly marked as demo.
- Updated PC Box header, page indicator and fixed Back button while keeping its six-slot ring and existing transfer/release behavior.
- Reframed Pokémon detail sheets, move rows, stat summary, focus/withdraw and release controls; preserved data and touch coordinates.
- Replaced generic confirmation overlays with GBA-style framed choices while preserving warning text and modal hit regions.
- Matched the starter picker, shared name keyboard, training hub, minigame HUDs and results to the GBA window system.
- No save-format, combat, online-identity, training-reward or tap-handler changes. ESP32 and complete desktop suite still require laptop/device verification.

## 2026-11-12 — Patch 6.1: Gym layout, GBA battle UI, items
- Made every Gym Leader card a fixed 292px width (including while scrolled) and reserved a separate right-hand scrollbar gutter. Kept leader selection hit areas aligned with the new geometry.
- Restyled wild/trainer/LAN battle status plates, command buttons, move/type cards, dialogue panel, bag controls, Back and trainer victory views with the shared GBA palette.
- Fixed the battle bag's inaccessible pages: four items are displayed per page, and arrows or horizontal swipes now reach later consumables. New fights reset that page to zero.
- Potions and cure items now show their effects before the enemy's responding turn; Potion healing updates displayed HP immediately and reports the exact amount restored. As before, battle HP is temporary rather than stored on the pet.
- Added `battle_items_test` for Gym card width, battle bag paging, Potion healing/consumption, guaranteed Master Ball capture with Party/Box storage, trainer-battle ball blocking, and full-HP Potion rejection.


## 2026-11-12 — Patch 6: Trainer, Settings, Time & Wi-Fi UI
- Repainted Trainer card, badges, medals and Friends control with GBA-style frames and ribbons.
- Repainted Settings hub and reset confirmation controls with consistent GBA styling.
- Repainted Time / Sound controls, including the time display, volume and language selector.
- Repainted Wi-Fi connection, saved networks, and WPA password keyboard without changing interaction regions or credentials handling.
- Preserved save data, network behavior, and existing touch targets.

## Unreleased (`dev`)

### Expeditions clock correction and GBA interface

- Initialize the SDL emulator RTC from the host wall clock at each launch so saved expedition timers continue after the emulator is closed (works on Windows, macOS and Linux). The actual ESP32 RTC remains unchanged.
- Redesign destination selection, active adventure, journal and reward summary using the existing `gba_ui.h` frames, ribbons and RGB565 palette.
- Show the dispatched Pokemon's escrowed sprite/name/level even while absent from the Party/Box. All expedition touch hit regions, journal records, reward settlement and save layout remain unchanged.


### GBA-era menu UI foundation (visual-only)

- Introduced `gba_ui.h` with an original RGB565 GBA-inspired palette, double-line panels, ribbon headings, and tiled screen backgrounds.
- Restyled Main Menu, six-member Party (active included), and Pokémon battle stats with consistent compact pixel UI. Existing touch areas and party/save mechanics are unchanged.
- Prevented underlying tile-navigation labels from drawing across modal Party and Summary controls; the main pet screen retains its navigation hints.

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
