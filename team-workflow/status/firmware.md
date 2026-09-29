# Firmware lane status — September 26, 2026

Review branch: `codex/firmware-usb-bringup-noel`, continuing base implementation commit `5256752` from local `codex/firmware-runtime`. Website source and generated shared artwork were preserved. No backend deployment performed. A spare USB-only board on COM6 has now been flashed and tested with the local demo plus the bench diagnostics in this branch; see the latest checkpoint below.

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
| ESP32 cloud cross-compile (USB diagnostics update) | 1,335,526 bytes flash / 3,145,728; 57,052 bytes static RAM / 327,680 |
| ESP32 LOCAL DEMO cross-compile (USB diagnostics update) | 650,215 bytes flash / 3,145,728; 37,484 bytes static RAM / 327,680 |

The compiler's remaining RAM is not measured free heap during TLS. PSRAM is now confirmed on the spare board. No RF, display-color, battery, cloud credential provisioning, NVS power-interruption, deployed authentication or end-to-end website/device test has been performed. The new CI workflow is configured but has not run on GitHub.

Next: website team implements the authenticated device endpoint and pairing/token issuance in `docs/firmware/INTEGRATION.md`; hardware team follows `BUILD_AND_TEST.md` when the board arrives. Current device IDs are gator/robot/duck, palette original, accessory none. Use the amended schemas together. The browser-local v4 demo letter remains separate from v1 device sync.

The LOCAL DEMO server is a test double, not a production backend. Its ledger resets on reboot, receipts/history are bounded, care transitions are manually triggered for rehearsal, and it does not enforce the live server's timezone/lease/authorization rules. Do not deploy it as the real API.

## Hardware-arrival review checkpoint

User reports hardware received and soldering underway; no passing bench measurements supplied yet. Fetched remote main `f52a7c5`: only README changed after the artwork commit; local implementation `5256752` is still absent from all remote branches, and remote hardware `d17cdd7` retains the empty Arduino loop. Reran 127 firmware executable scenarios using the existing host binaries and 38 browser logic tests: all passed. No target rebuild, flash, physical test or hosted-backend check in this review.

Next: use this local runtime for USB-only bring-up, then connect to the backend team's implemented sync endpoint. Cross-lane review is saved in the planning workspace at `repository-audit/MVP_REVIEW_2026-09-26.md`. Pending firmware presentation improvements: last-sync/data-as-of display, readable financial summary, fitting long names and reassuring ghost copy. The website's local configuration receipt is not a device acknowledgement; coordinate that interface before claiming physical delivery.

## Spare-board physical runtime checkpoint

User confirmed COM6 is the spare Waveshare board, USB only, no battery/peripherals. Esptool identified ESP32-S3 rev 0.2 with embedded 4 MB flash / 2 MB PSRAM. Flashed LOCAL DEMO; all four written images passed esptool hash verification. Firmware reports 4,194,304 flash bytes and 2,097,152 PSRAM bytes.

Added local-demo-only USB button injection with bounded auto-release and non-secret state diagnostics, plus `firmware/tools/bench_smoke.py`. Both target variants compile. Python syntax and diff checks pass. The on-device script passed **25 runtime checks**: baseline reset, all three pets, food and ordered ride reactions, timed return to idle, savings without extra spending, ghost/game blocking, receipt review and confirmed revival, page navigation, game controls, petting, event drain and final reset. These execute the actual firmware on the board; switches are simulated over USB. The cloud protocol is unchanged and cloud firmware rejects the diagnostic button operation.

Measured local-demo final free heap: **317,808 bytes**; minimum free heap: **301,644 bytes**. No Wi-Fi/TLS load was active. NFC reports `reader unavailable`, expected with no attached reader. No display pixels, sound, physical button wiring or RF behavior was observed. NVS writes/readback occurred without a storage fault; reboot/power-cut durability was not tested.

Evidence: `docs/firmware/bench/2026-09-26-COM6-local-demo.json`. The board is left running LOCAL DEMO with gator, $25 spent and $50 saved. Next: connect verified peripherals for electrical/visual tests, then test the cloud build with the live backend. User requested publication so the website team can integrate; this review branch contains the prior implementation and new bench work together. The older remote hardware branch is not the firmware entry point for this implementation.

## Software-branch coordination checkpoint

A subsequent fetch found new `origin/software` at `0d504d3`, adding cloud document persistence, owner-bound device registration/revocation and a state-only sync endpoint. This supersedes the earlier audit's repository-wide absence-of-backend finding. That branch's docs state deployment is still required. Its device handler returns HTTP 501 when the firmware submits events, supplies no commands/review, fixes care at content and disables demo mode. Thus registration/basic polling can be integrated first, but NFC reviews, check-ins and financial reactions are not connected yet. Keep the device's outbox intact; implement server event handling rather than discarding events to make polling appear healthy. See the updated integration guide for cross-branch field constraints.
