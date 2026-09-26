# Verification report

| Gate | Status | Evidence layer |
|---|---|---|
| JSON schema contract (`check_contract.py`) | PASS | pack |
| Catalog semantics (`check_assets.py`) | PASS | pack |
| Host behavioral driver (`tests/cases.json`) | PASS 39/39 | host |
| NFC routing driver (`tests/nfc/routing-cases.json`) | PASS 13/13 | host |
| Content driver NDEF + animation (`run_content_acceptance.py`) | PASS 46/46 | host |
| CMake unit smoke (`harness_smoke`) | PASS | host |
| ESP32 cross-compile production sketch | PASS | cross-compile |
| ESP32 cross-compile `OKANEGACHI_LOCAL_DEMO` | PASS | cross-compile |
| Physical flash / PN532 / ST7789 | NOT RUN | hardware |
| Live HTTPS / companion backend | NOT RUN | live |

Unresolved / follow-up:

- Full HAL display/NFC/piezo integration on-device (sketch is bring-up shell wiring pins only).
- PSRAM detection and task stack measurements on hardware.
- Final artwork pipeline uses placeholders; converter writes `firmware/assets/generated/asset_registry.hpp` from catalog.
