# Okanegachi firmware

Arduino C++ for **Waveshare ESP32-S3-Zero (4 MB flash, 2 MB QSPI PSRAM)**, SPI ST7789 240×240 LCD, SPI PN532, three grounded buttons and a passive piezo through **1 kΩ**. Confirm the actual delivered variants before powering peripherals. The pet lives on this display.

This is the production implementation folder. The old `hardware/firmware-mvp` branch's loop stub is not this runtime. Start with [BUILD_AND_TEST.md](../docs/firmware/BUILD_AND_TEST.md), then [INTEGRATION.md](../docs/firmware/INTEGRATION.md). Imported earlier design documents are background; the runtime guide and executable tests record the current behavior.

## Run before the hardware arrives

```sh
python -m pip install -r firmware/requirements-dev.txt
python firmware/tools/import_shared_assets.py
python firmware/tools/generate_schema.py
# ArduinoJson must be version 7.2.1; point at its library folder.
python firmware/tools/build_native.py --arduinojson /path/to/ArduinoJson
python firmware/tools/test_runtime.py
python firmware/tools/demo_console.py
```

`import_shared_assets.py` verifies every uploaded pixel against `firmware/generated/okanegotchi_assets.h`, then generates the renderer registry. The 96 frames are 32×32, stored in flash and displayed at 4× nearest-neighbor scale. It does not need 96 separate files. Source sheets and timing are in `assets/manifest.json`; regenerate the shared header with `npm run assets:build` after changing them. The runtime includes that header in **one translation unit**.

## Build the real device

Install Arduino CLI 1.5.1, Espressif Arduino **3.3.12**, ArduinoJson **7.2.1**, Adafruit GFX **1.12.6**, ST7735/ST7789 **1.11.0**, BusIO **1.17.2**. Install these with `--no-deps` after their dependencies are present so the library manager does not silently replace pinned versions.

```sh
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.3.12 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install --no-deps 'ArduinoJson@7.2.1' 'Adafruit BusIO@1.17.2' 'Adafruit GFX Library@1.12.6' 'Adafruit ST7735 and ST7789 Library@1.11.0'
python firmware/tools/build_esp32.py
python firmware/tools/build_esp32.py --demo
```

Cloud is the default. The **LOCAL DEMO** build runs a fake ledger on the device, labels the screen, and uses a separate NVS namespace; it makes no server requests. Reset/reboot of the local demo starts a fresh ledger. It is for rehearsals while the website backend is completed.

For this workspace's downloaded tools on Windows:

```powershell
python firmware/tools/build_esp32.py --workspace-tools ../.tools --short-path
```

The temporary drive mapping avoids Xtensa GCC's long-path header failure and is removed afterward. Builds do **not** upload. Use a verified board port, USB-only power for first bring-up, and the FQBN in `tools/build_esp32.py`. This build uses a 3 MB application partition, 4 MB flash, QSPI PSRAM and native USB CDC. It has no OTA updater.

## Setup and controls

```sh
python firmware/tools/provision.py --list
python firmware/tools/provision.py --port COM7 --configure
python firmware/tools/provision.py --port COM7
```

The website backend must issue a device UUID and **opaque device token**. Supply those, Wi-Fi credentials and the Supabase project ref through the USB helper. Never put the Supabase service-role key on the device. Configuration is saved in NVS and the board restarts. TLS verifies the hostname and CA bundle; the clock must synchronize before requests. Use a 2.4 GHz network without a captive portal.

| Button | Short press | Hold ≥1 second |
|---|---|---|
| A | Previous page/menu option; move left in game | Return home |
| B | Pat on home; select; request review on Check-in; start/pause game | Request review from home; confirm the displayed review |
| C | Next page/menu option; move right in game | Start game from home |

Pages: pet, savings goal, check-in, menu. Menu: review, game, connection diagnostics, mute. Check-ins are pending until the server accepts them. A ghost is revived through a confirmed spending review; it does not delete money or a savings goal. The game is cosmetic.

For LOCAL DEMO only, the USB helper also accepts `--scenario food|ride|savings|neglect|reset` or `--pet gator|robot|duck`. NFC demo stickers use the same runtime event path. A supported generic Type A card requests a review; no card account, PAN or payment data is read. See the integration guide for supported sticker type and exact text.

## Evidence boundaries

Native tests exercise the actual portable C++ engine, schema decoder, NDEF parser and compositor. An ESP32 cross-compile verifies target APIs and link/flash size. Neither proves wiring, RF sensitivity, display colors, USB, NVS power-loss behavior or TLS heap headroom on a powered board. Live Supabase integration requires the website team's device endpoint. See the bench checklist before the demo.
