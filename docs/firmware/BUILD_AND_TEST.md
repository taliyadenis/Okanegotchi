# Build, verification and first power-up

Start with `firmware/README.md` for pinned dependencies and commands. Run from the repository root. Python 3.10+ and a C++17 compiler are required for host tests; the hardware build uses Espressif Arduino 3.3.12. Generated files are committed so the ESP32 build does not require Node, Pillow, PNG decoding or internet asset downloads at runtime.

## What the automated checks exercise

`tools/build_native.py` compiles the real engine/codec/renderer and a separate PN532 transcript executable. `tools/test_runtime.py` drives the compiled binary over JSON lines, testing retries, reboot, persistent IDs, invalid responses, queue bounds, save-before-play, button timing, NFC presentation/freshness, review receipts, the game, actual pets and the local fake server. `tools/check_contract.py` independently validates the same JSON fixtures with JSON Schema. `tools/import_shared_assets.py` compares all 98,304 source pixels and alpha bits to the shared generated artwork.

`firmware/tests/cases.json` and `tests/nfc/routing-cases.json` are retained design/reference scenarios from the older branch, not claims that the new engine has passed an obsolete driver interface. The current executable suite is `test_runtime.py`, `pn532_test.cpp` and the schema/NDEF fixtures. Do not present a runner's self-test as firmware acceptance.

## Arrival checklist: USB only first

1. Verify the delivered board is the assumed ESP32-S3-Zero variant. Check flash/PSRAM and the labeled GPIOs. This pinout is not for a classic ESP32 WROOM development board. Disconnect battery/boost while USB powers initial tests; never combine unverified 5 V sources.
2. Check the exact screen labels: SCK→12, MOSI/SDA→11, CS→10, DC→2, VCC→3V3, GND→system ground. Six-pin display assumption has no exposed reset pin. **Wire colors do not identify pins.** Check the delivered PCB labels before power.
3. PN532: SPI selector position per the actual board, SCK→12, MOSI→11, MISO→13, CS→1; verify that module's supply and logic ratings. All signal grounds are common on the load side of the battery protection. Pull both chip selects high before bus initialization (the firmware does).
4. Buttons: GPIO4/5/6 each to ground through a normally-open contact pair, `INPUT_PULLUP`. On a four-leg tactile switch, use opposite electrical sides, not two internally shorted legs. Piezo: GPIO7 → **1 kΩ** → piezo → ground. No motor driver is implied.
5. Build/flash **LOCAL DEMO** first using the verified serial port. Confirm USB CDC output and the screen label. Check RGB colors, image orientation and all three pets. Adjust `OKANEGACHI_LCD_ROTATION` (0–3) only after observing the mounted panel. If bus noise occurs with long jumpers, reduce LCD SPI frequency in `platform/device.cpp` and shorten wiring.
6. Test A/B/C short and long presses, boot with a held button, ghost/revive, game, mute, each sticker, and removal/re-tap. Hold a sticker still for 10 seconds: one event. Disconnect/reconnect the reader: the buttons/animation loop must remain responsive. Check PN532 status through the USB helper/Connection menu.
7. Record `flash_bytes`, `psram_bytes`, `heap_free`, `heap_min` using the helper. Run an extended animation + NFC + Wi-Fi session. Static compiler RAM is **not** free heap under TLS load. A missing PSRAM report or brownout must be investigated, not dismissed as a software test passing.
8. Flash the **cloud** build, provision using the website team's real device token/endpoint, and use a 2.4 GHz non-captive network. Check a transaction from the website reaches the physical pet, then a physical review/check-in appears confirmed in the web app. Test loss/reconnect, server 429 and invalid token recovery. Real TLS/auth/device registration cannot be proven by the local demo.
9. Test power interruption during a pending event and command start. NVS should retain a consistent record; cursor replay is prohibited, while an effect may be lost in the documented save-before-start window. `--factory-reset` requires holding all three buttons for 2 seconds and clears local configuration/state; it does not reset the server ledger.
10. Only after USB operation is stable, resume the separate battery/charger/boost wiring guide. This firmware does not measure battery voltage, manage charging, provide load sharing or make simultaneous USB/boost power safe.

## Reproducing a website-team transaction without hardware

Run `python firmware/tools/demo_console.py`. Try `food`, `review`, `confirm`, `neglect`, `review`, `confirm`, and `reset`. The actual C++ engine exchanges validated request/response JSON with the local fake server. `offline` keeps actions pending; `online` resumes sync. This tests software flow, not ESP32 electrical behavior or the deployed backend.

To export the horizontal sheets as 96 individual PNGs, run `python firmware/tools/import_shared_assets.py --export-frames`. Outputs go to ignored `firmware/build/assets/frames/<pet>/<animation>/`. They are convenience exports; the runtime reads compiled flash arrays.

## Deployment boundary

Changes remain on `codex/firmware-runtime` until reviewed/merged. No device has been flashed and no Supabase service has been deployed by this implementation task. The website team must adopt the amended pet IDs and implement the device endpoint in `INTEGRATION.md`; a working browser-local preview does not establish connectivity.
