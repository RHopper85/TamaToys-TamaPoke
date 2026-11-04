# TamaToys TamaPoke project notes

- This is a Pokemon-only virtual-pet firmware fork, currently developed on `dev`.
- Preserve the core pet experience and the existing ESP-NOW LAN battle protocol.
- Online services will use an HTTPS PHP API on shared IONOS hosting; ESP32 devices must never connect directly to MariaDB.
- Keep real Wi-Fi credentials and API secrets out of source control. Home Wi-Fi preferences live in the device's `tt_wifi` NVS namespace.
- The emulator's `TamaToys Demo`, `Guest WiFi` and `Workshop` networks are simulated; they do not exercise the PC's wireless hardware.
- Run `bash tools/emu/build.sh` and `bash tools/emu/tests/run.sh` for emulator validation. Verify the firmware with Arduino IDE using the ESP32-S3 board, 16 MB flash, OPI PSRAM and a 3 MB application partition.
- Do not treat emulator success as real-device proof of Wi-Fi, power management or ESP-NOW coexistence.
- Keep `README.md`, `ROADMAP.md` and `CHANGELOG.md` current when adding significant features.
