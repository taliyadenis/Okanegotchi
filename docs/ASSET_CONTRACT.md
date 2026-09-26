# Asset contract entry point — revision 2

**Required detailed specifications:** read [MODULAR_ASSETS.md](MODULAR_ASSETS.md), [ANIMATION_ENGINE.md](ANIMATION_ENGINE.md), [ADDING_CONTENT.md](ADDING_CONTENT.md) and assets/catalog.example.json. They supersede the brief planning notes below where more specific. The engine, import pipeline, scene/clothing support and tests are required now; only final artwork is deferred. Indexed4 pixels are the default runtime representation, with index0 transparent; earlier RGB565+mask sizes below are comparison estimates.

One website/design owner supplies canonical PNGs and a manifest. Firmware owns deterministic conversion and rendering. Source frame32x32RGBA, nearest-neighbor integer scaling, transparency mask, shared anchors for feet/hat/scarf. Art provider should expose frame lookup by pet, visual state, frame index, palette and accessory; no network asset download in sync v1.

IDs: piggy/cat/dragon; mint/coral/lavender; none/cap/scarf; version1. Renderer supports idle/blink/eating/traveling/celebrate/neutral/needs_checkin/sleepy/ghost/revive. A visual state may reuse a base frame plus overlay/transform; do not force the artist to hand-draw every state. Unknown/missing local frames fallback clearly, no out-of-bounds indexing. Timings driven by app, not by blocking sprite playback.

Cursor must define a versioned manifest and converter using a small Python utility (Pillow dev dependency acceptable) and test synthetic transparency/dimensions/IDs. Document RGB565 endianness and GFX call expectations, palette representation and masks. Prefer indexed colors so alternate palettes don't triple flash storage. Generated source goes in isolated assets/generated; final artist files untouched. Placeholder geometry requires no final PNGs.

Target planning budget:72 tiny RGB565 frames ~144KiB plus9KiB masks; this excludes accessories/code/TLS. Record actual final counts/compiled sizes rather than claiming the estimate as measured. No full-screen animation frame sequences. Public website preview and ESP32 must consume the same manifest/IDs.
