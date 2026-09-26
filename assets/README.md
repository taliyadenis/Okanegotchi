# Shared character assets

The approved presets are **gator, robot, duck**, with original colors and no accessories or color customization. This replaces earlier Piggy/Cat/Dragon planning. The UI previews animations and saves the selected pet with companion setup locally for the demo-letter flow. Physical appearance sync is not implemented.

`assets/manifest.json` is the canonical mapping for IDs, palettes used internally for indexed storage, common 32×32 canvas, anchors, frame counts, and timing. Original PNG exports are preserved in `assets/source/<pet>/`. No editable Piskel sources were supplied for these final sets.

Run `npm run assets:build` to generate:

- `public/assets/v1/*.png`: original-color horizontal sheets for the website.
- `public/assets/v1/*.bin`: one palette index per pixel, frame-major then row-major.
- `firmware/generated/okanegotchi_assets.h`: indexed frames, per-pet RGB565 lookup palettes, 1-bit masks, timing constants, and optional direct RGB565 arrays.
- `firmware/generated/asset-report.json`: byte totals and source validation report.

`npm run dev` and `npm run build` regenerate assets automatically. Keep source and generated outputs together when sharing this change. `npm run test:assets` compares every source pixel and alpha value against web output and firmware data.

## Animation mapping

| Uploaded name | Shared ID | Frames | Frame delay |
| --- | --- | --- | --- |
| idle | idle (also blink/neutral fallback) | 4 | 240 ms |
| eating | eating | 6 | 160 ms |
| walking | traveling | 4 | 180 ms |
| celebrate | celebrate | 4 | 180 ms |
| sleepy | sleepy | 4 | 400 ms |
| ghost | ghost | 4 | 320 ms |
| neglected | needs_checkin | 2 | 500 ms |
| revive / revived | revive | 4 | 180 ms |

These delays are adjustable defaults because PNG files do not contain playback timing. Eating/traveling/celebrate return to idle after 8 seconds; revive after 2 seconds. Other states loop. Reduced-motion preference starts playback paused.

The generator validates sheet dimensions, declared colors, CRC through the PNG decoder, and binary alpha. Some supplied PNGs contain 1–2 trailing bytes after IEND; these are ignored during decoding, recorded in the report, and excluded from generated PNGs. No visible pixels are changed.

## Firmware handoff

`asset_version` is 1. Common anchors are center (16,16) and feet (16,31); firmware must confirm desired screen placement. Draw source pixels at integer scale with nearest-neighbor sampling; the web preview uses 4× = 128×128.

Each pixel index uses one byte (not packed nibbles). Index 0 is transparent. The separate mask packs eight pixels per byte, most-significant bit first, 1 for opaque. RGB565 values are numeric words in R5/G6/B5 layout, with no byte swapping. Use the display driver's required byte order at transfer time.

Default firmware usage is indices + the pet's palette. Defining `OKANEGOTCHI_INCLUDE_RGB565_REFERENCE` also includes direct RGB565 arrays for drivers that need them; do not enable both storage representations unnecessarily. Include the header from a single translation unit to avoid duplicating static arrays. Rendering can decode a scanline or a single sprite, rather than allocating a full-screen frame.

There are 96 frames: 98,304 bytes of indices + 12,288 bytes of masks + 62 bytes of palettes (excluding metadata/alignment). Direct RGB565 frames alone are 196,608 bytes. A 240×240 RGB565 framebuffer adds 115,200 bytes. These are generated data sizes, not measured flash/heap/PSRAM headroom. C++ target compilation, display byte order, flash partition, PSRAM and TLS memory use have not been verified.

The old protocol schemas still describe different preset IDs and palette/accessory fields. Hardware/backend must adopt gator/robot/duck and agree how deprecated fields are handled before appearance saving is enabled. No PNGs or frame data belong in v1 sync. Keep the last valid appearance for unknown IDs or incompatible assets. A working web preview does not prove physical rendering.
