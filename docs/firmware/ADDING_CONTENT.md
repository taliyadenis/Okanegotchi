> Historical design reference from the earlier firmware branch. For the current runtime, pet IDs, supported NFC tags, commands and executable tests, read `firmware/README.md` and `docs/firmware/BUILD_AND_TEST.md`.

# Adding future artwork

The following workflow must be implemented by Cursor. Final art comes from website/design team; existing example placeholders are not final sprites.

1. Artist supplies transparent32x32 PNGs/sheet, palette, frame timings and feet/head/neck/body anchors. Start one pet's idle/eating/traveling; reuse frames with props for remaining states until dedicated art exists.
2. Put sources under assets/characters/<id>/, assets/accessories/<id>/ or assets/backgrounds/<id>/. Copy catalog.example.json to catalog.json if missing. Change frame source from placeholder to png with explicit path/crop. Do not touch generated C++ arrays.
3. Add ordered frames/timings to clips and map each semantic state. Set per-frame anchors so clothes follow the body. Add accessory origin/compatibility/variants and scene background/props separately.
4. Run catalog validation, Cursor's converter and preview/contact-sheet command. Verify transparency, timing, clipping, palette and clothing anchors.
5. Rebuild/flash and test on screen; give generated normalized manifest/PNGs to website team for identical preview. New online-selectable IDs need coordinated protocol changes; existing IDs' new artwork does not.

| Addition | Data changed | Expected code changes |
|---|---|---|
| Cat eating frames | frame sources + cat.eating clip | None |
| New room | background entry + scene binding | None |
| Cap follows bobbing head | frame head anchor + cap origin | None |
| Dragon-specific cap | variant art under cap ID, explicit compatibility | None |
| New Rabbit character | new frames/clips/character entry | None in engine; coordinate API enums for online choice |
| Website-selected background/multiple clothes | scene/outfit registry | Versioned backend/website/firmware protocol work |

Cursor must deliver exact converter/preview commands, actual manifest syntax, coordinate illustration, memory report, and one worked synthetic content addition that proves engine code did not change. Keep final artwork pending; do not ask artist to reverse engineer firmware.
