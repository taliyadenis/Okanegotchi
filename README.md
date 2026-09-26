# Okanegachi firmware MVP

Portable C++17 domain core, host JSONL test drivers, NFC demo sticker routing, animation sampling, catalog asset tools, and an ESP32-S3 bring-up sketch (Waveshare ESP32-S3-Zero profile).

## Quick start (host)

```bash
pip install -r requirements-dev.txt
cmake -S . -B build -DCMAKE_CXX_COMPILER=g++-12
cmake --build build
python3 tools/check_contract.py
python3 tools/run_acceptance.py --driver build/firmware_test_driver
python3 tools/run_content_acceptance.py --driver build/content_test_driver
```

## ESP32

Install [arduino-cli](https://arduino.github.io/arduino-cli/) and the `esp32:esp32` core, then:

```bash
scripts/build_esp32.sh
```

Pins and assumptions: `hardware/firmware_pins.h`, `HARDWARE.md`. Protocol: `mvp/`. Status: `docs/EXECUTION_STATUS.md`, `docs/VERIFICATION_REPORT.md`.
