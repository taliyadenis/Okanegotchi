# Firmware execution checkpoint

Status: MVP implementation complete on branch `codex/firmware-mvp` (host + cross-compile verified in cloud agent).

- Board profile: Waveshare ESP32-S3-Zero FH4R2 (4MB flash, 2MB QSPI PSRAM), pins from `hardware/firmware_pins.h`.
- Completed: portable C++17 core (state/sync/outbox/commands/buttons/NFC demo routing), JSON codec (ArduinoJson), NDEF parser, animation sampler, host drivers, asset tools, minimal ESP32 sketch.
- Host gates: `python3 tools/check_contract.py` 44/44; `run_acceptance.py` 39/39; `run_content_acceptance.py` 46/46; NFC routing 13/13; CMake `ctest` harness_smoke PASS.
- ESP32: `arduino-cli compile` FQBN `esp32:esp32:esp32s3:FlashSize=4M,PSRAM=opi` — production ~356 KiB flash / ~39 KiB RAM (sketch shell).
- Hardware flash/NFC/display bench: NOT RUN (no device in agent).
- Next: flash identified board, run USB provisioning smoke, validate SPI wiring on received Waveshare unit.

Build (host):

```bash
cmake -S . -B build -DCMAKE_CXX_COMPILER=g++-12
cmake --build build
python3 tools/run_acceptance.py --driver build/firmware_test_driver
python3 tools/run_content_acceptance.py --driver build/content_test_driver
```

Build (ESP32):

```bash
export PATH="/workspace/bin:$PATH"  # arduino-cli
scripts/build_esp32.sh
```
