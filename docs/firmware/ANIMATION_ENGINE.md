> Historical design reference from the earlier firmware branch. For the current runtime, pet IDs, supported NFC tags, commands and executable tests, read `firmware/README.md` and `docs/firmware/BUILD_AND_TEST.md`.

# How animations execute

The server returns an instruction such as eating for8000ms. Firmware already contains the current character's frames. It receives no video/GIF and does no per-frame HTTP request. PNGs become compiled pixel data at build time; C++ selects and draws frames via SPI.

Example: four eating frames at125ms each form a500ms cycle. An8000ms reaction loops that clip16 times. On start, persist command cursor, record monotonic start time and resolve character clip/scene. Every UI tick calculates elapsed time and chooses the frame; at8000ms start next queued activity or return to care-state idle.

```text
validated command -> persist cursor -> start reaction budget
  -> character's semantic clip -> elapsed = now - start
  -> loop: phase = elapsed modulo sum(frame durations)
  -> frame = first cumulative end strictly greater than phase
  -> compose background + body + anchored clothing + props + UI
  -> flush changed tiles -> budget ends -> next command/care idle
```

For once clips, clamp to last frame when clip ends and hold until reaction budget ends. Do not change server duration to fit art. If a frame is late, jump to the frame appropriate to elapsed time, never block to replay missed frames. No delay loops, per-frame storage writes or per-frame allocation. UI target20fps is independent of sprite frame timing. Tests include0,124,125,499,500,7999,8000ms boundaries, stalled rendering and timer wrap.

Each character maps idle/blink/pat/needs_checkin/sleepy/ghost/eating/traveling/celebrate/neutral/revive to clips. Idle required; others may explicitly map to idle plus a scene/prop/text fallback. Different pets may have different frame counts/timing. No recursive fallback cycles. Unsupported wire animation still follows protocol skip rules; a local clip name isn't a new API enum.

Composition order: background -> behind-body props/accessories -> body -> clothing -> front props/effects -> UI. Character root transform (screen placement, integer scale, facing mirror, local bob/slide) applies to pixels AND anchors. Align accessory origin with current frame anchor plus offset. Sprite changes redraw union of old/new bounds including clothing to restore background and avoid trails. Clip every draw at screen edges. Accessory/motion phases use the same clock, not independent delay loops.

Appearance update during a reaction is staged until it ends, latest update wins; never reset financial timer or show half of two characters. Idle applies updates atomically next render. Ghost/reset may cancel activity immediately per protocol. Menus/review do not pause reaction time; a full-screen reaction must not cover a review being read. Keep Review visible while activity clock advances; returning home shows current frame/idle, not replay. Presses apply to visible page. Runtime scene selection follows semantic state, not pet name.

Required separable modules:

- AssetRegistry: lookup validated const descriptors/fallback status.
- AnimationPlayer: start(clip,start_ms), pure sample(now_ms) -> frame/anchors; no storage/network/SPI.
- ReactionScheduler: duration/order/epoch/persistence; no PNG filenames.
- SceneComposer: transforms, layering, palette expansion, dirty bounds/tiles.
- DisplayAdapter: short SPI flush operations cooperating with NFC.

Required runtime tests: loop/once boundaries, state fallback, per-frame cap motion, mirror/clip, staged appearance, background restoration after removing clothing, scene switch, no replay on poll, slow frame skips, hidden-view activity expiry, ghost/reset cancellation and missing PSRAM fallback. Converter tests cannot substitute for actual C++ player/compositor tests.
