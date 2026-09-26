# Official implementation references checked 2026-09-26

These links support library/tool usage, not a claim of passing hardware validation. Inspect pinned library source and actual board at implementation time.

- Cursor subagents, definitions and isolated worktrees: https://cursor.com/docs/subagents
- Cursor project rules: https://cursor.com/docs/rules
- ESP32 Arduino SPI: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/spi.html
- ESP32 Preferences/NVS: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/preferences.html
- ESP32 LEDC API (match installed core version): https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ledc.html
- Adafruit PN532 API/source: https://github.com/adafruit/Adafruit-PN532/blob/master/Adafruit_PN532.h
- NTAG213/215/216 capacity, page layout and Type2 information: https://www.nxp.com/docs/en/data-sheet/NTAG213_215_216.pdf
- GFX RGB565 bitmap/mask API; indexed pixels need expansion: https://adafruit.github.io/Adafruit-GFX-Library/html/class_adafruit___g_f_x.html
- Original board vendor reference, previously checked in project: https://www.waveshare.com/wiki/ESP32-S3-Zero

The repository's copied mvp files remain the agreed project protocol; vendor docs do not supersede the project's electrical pin map. Cursor agent/tool permissions and account limits still apply to an autonomous request.
