# Firmware lane status — September 26, 2026

Implementation branch: `codex/firmware-runtime`. Based on main `17df40f` (shared Gator/Robot/Duck artwork). Website source and generated shared artwork were preserved. No remote push, hardware upload or backend deployment performed in this task.

Implemented: ESP32-S3 Arduino entry point; shared hardware SPI LCD/PN532 driver; sliced scene/UI renderer; all 96 authored frames; variable clip timing; buttons, menu, review/check-in receipt, goal, ghost/revive, cosmetic game and piezo/mute; USB setup and recovery; NVS CRC record and account binding; Wi-Fi/TLS worker; strict v1 JSON; durable events and command cursor; retry/auth/reset handling; bounded NFC NDEF decoding; separately labeled local fake-server demo; build/test tools and GitHub Actions workflow.

Verified locally:

| Check | Result |
|---|---|
| Actual C++ runtime scenarios | 29 passed |
| Actual C++ schema decoder fixtures | 44 passed |
| Independent JSON Schema validator | Same 44 fixtures passed |
| Actual NDEF parser fixtures | 24 passed |
| Actual compositor timing fixtures | 22 passed |
| PN532 SPI transcript scenarios | 8 passed |
| Source artwork vs shared arrays/masks | All 98,304 pixels matched |
| Actual C++ scene raster vs PNG sources | All 96 frames / 5,529,600 output pixels matched |
| ESP32 cloud cross-compile | 1,335,458 bytes flash / 3,145,728; 57,052 bytes static RAM / 327,680 |
| ESP32 LOCAL DEMO cross-compile | 648,739 bytes flash / 3,145,728; 37,460 bytes static RAM / 327,680 |

The compiler's remaining RAM is not measured free heap during TLS, and PSRAM has not been detected on a powered board. No RF, display-color, battery, USB provisioning, NVS power-interruption, deployed authentication or end-to-end website/device test has been performed. The new CI workflow is configured but has not run on GitHub.

Next: website team implements the authenticated device endpoint and pairing/token issuance in `docs/firmware/INTEGRATION.md`; hardware team follows `BUILD_AND_TEST.md` when the board arrives. Current device IDs are gator/robot/duck, palette original, accessory none. Use the amended schemas together. The browser-local v4 demo letter remains separate from v1 device sync.

The LOCAL DEMO server is a test double, not a production backend. Its ledger resets on reboot, receipts/history are bounded, care transitions are manually triggered for rehearsal, and it does not enforce the live server's timezone/lease/authorization rules. Do not deploy it as the real API.
