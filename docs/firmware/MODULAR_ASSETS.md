> Historical design reference from the earlier firmware branch. For the current runtime, pet IDs, supported NFC tags, commands and executable tests, read `firmware/README.md` and `docs/firmware/BUILD_AND_TEST.md`.

# Required modular asset system

Implement now with geometric placeholders, then accept final artwork from the website/design team. No final sprites are required to build/test the engine. The catalog is data, not executable code.

| Registry | Meaning | Example |
|---|---|---|
| Frames | PNG crop or placeholder, dimensions, named anchors | cat.eat.0 |
| Clips | Ordered frame IDs and duration per frame, loop/once | cat.eating |
| Characters | ID, palette binding and semantic-state -> clip mapping | cat.eating for eating |
| Accessories | Independent art, head/neck/body slot, anchor/origin, compatibility | cap follows head |
| Backgrounds | Static/tiled image or solid fill, optional scroll | room/city |
| Scenes | Background, character placement/scale and layered props | traveling scene |
| Scene bindings | Semantic activity -> scene | eating -> kitchen |

No if(cat) animation logic or one class per character. New catalog entries regenerate registries without changing the pet/network logic. Engine effects such as bob/slide/sparkle are named reusable functions; manifests cannot contain scripts. IDs unique within registry and references must resolve.

## Sources and anchors

Character frames32x32RGBA, transparent outside body, no antialiasing, consistent facing. Feet/head/neck/body anchors are per frame in unscaled coordinates, top-left origin, x right/y down. A bobbing head moves the cap through its anchor rather than an independent guessed animation. Anchor coordinates stay within frame bounds.

Accessories may be8-64px per dimension (default16x16), have an attachment origin and offset. Cap position = current frame head anchor + offset - cap origin, transformed with the body. Compatibility lists explicit character IDs. Character-specific variants under one cap/scarf product ID let Dragon wear a different cap shape without new wire enums. Unsupported variant hides with a developer warning; never draw detached clothing or duplicate all body frames for every outfit.

Background defaults60x60 tile scaled4x to240x240; solid-fill scene works immediately. Allow full240x240 static RGB565 images where budget permits. Props use small8-64px frames. Share scenery across characters, do not store one full background per frame. Explicit PNG paths/crop rectangles; no guessed filename sequence. Reject out-of-bounds crops, wrong size, partial alpha and missing files. A per-frame PNG simply uses its full-image crop.

Use a16-entry palette including transparent index0. Default compiled format indexed4, two pixels per byte (high nibble first; pad odd last low nibble0). Opaque colors must match palette entries1-15 exactly; alpha0->index0, partial alpha/unmapped color rejected unless artist quantized before import. Palette variants change the RGB565 lookup table, not all frame pixels. Palette entry0's color is ignored. Build-time synthetic tests verify red0xF800/green0x07E0/blue0x001F and transparency. Display adapter is responsible for correct byte order. GFX drawRGBBitmap does not accept indexed pixels; expand bounded tiles/scanlines before display calls.

Catalog import_palette names the ONE canonical PNG import palette (example mint). Its15 opaque RGB entries must be unique. All PNG/body/accessory colors map against that palette once; coral/lavender only replace runtime lookup values and may intentionally contain duplicate colors. Never re-quantize source pixels independently for each palette or guess palette from character name.

Scene character_root is the SCREEN TARGET of the current frame feet anchor, not the body top-left. For unmirrored character pixel p, screen=root+scale*(p-feet). For horizontal mirror in width w, reflect every local point with M(x,y)=(w-1-x,y) before subtracting M(feet). Accessory anchor target=root+scale*(M(anchor)-M(feet)); transform accessory pixels/origin in its own width and apply mirrored authored offset, then align origin to that target. Unmirrored accessory top-left=root+scale*(anchor-feet+offset-origin). This explicit anchor placement keeps [120,190] at the pet's feet with4x scale and keeps hats attached when facing changes. Props with screen_xy are absolute screen top-left, scaled independently.

## Runtime and tooling

`AssetRegistry` maps IDs to const flash descriptors; `AnimationPlayer` selects current frame; `SceneComposer` combines layers into bounded tiles; `DisplayAdapter` flushes SPI. Interfaces injectable for native tests, no network in renderer and no SPI in player. PNG decode runs on development computer, not every device frame. Baseline uses generated const C++ arrays, no runtime filesystem or HTTP asset downloader.

Reuse buffers; no per-frame malloc.32x32 indexed4 costs512 bytes/frame;72 frames ~36KiB plus palettes/metadata.240x240 RGB565 costs112.5KiB and240x16 tile7.5KiB. These are budgets, not measurements. Produce actual generated flash/working RAM report and measure TLS heap separately. Tile rendering yields shared SPI between short transfers so NFC remains responsive; don't hold a bus lock for a whole animation.

Cursor implements:

```text
python tools/build_assets.py --catalog assets/catalog.json --output firmware/src/assets/generated --web-output build/web-assets
python tools/preview_assets.py --catalog assets/catalog.json --output build/asset-preview
```

Emit deterministic C++ registry/indexed arrays, normalized shared manifest, browser-readable PNG previews/contact sheet and size report. The example catalog contains placeholder primitives, so the first build needs no image files. Actual PNG import can use pinned Pillow. Verify source paths stay inside assets/ (including symlinks), crop/alpha/IDs/anchors/clips and aggregate size. Preserve artist inputs; don't edit generated arrays manually. Preview every character/state/outfit/background with nearest-neighbor scale. Test synthetic squares/checkerboards, not finished art.

`assets/catalog.schema.json` and `tools/check_assets.py` validate metadata and references now; they do not implement converter or prove rendered pixels. Cursor implements production converter/player/compositor and tests actual behavior.

## Compatibility and future additions

Catalog schema_version1 describes local format; asset_version1 describes current wire compatibility. Existing JSON v1 allows piggy/cat/dragon, mint/coral/lavender, none/cap/scarf and current animation names. New local registry entries must work in the engine and preview without C++ edits. New companion-selectable IDs require coordinated firmware/backend/website enum/capability updates; never silently accept unknown protocol values.

v1 has no background_id or multi-item outfit fields and rejects extra properties. Implement backgrounds NOW through local semantic scene bindings (idle room/eating kitchen/traveling city/ghost night), independent of character. Internally support head/neck/body layers; wire accessory currently selects one item. Selecting backgrounds or multiple clothes from the website is a later versioned protocol change. Replacing art/adding frames/backgrounds under compatible IDs requires generation/rebuild/flash and matching website preview assets; compiled content is not remotely downloaded by selecting an outfit.
