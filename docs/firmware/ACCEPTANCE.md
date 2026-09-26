> Historical design reference from the earlier firmware branch. For the current runtime, pet IDs, supported NFC tags, commands and executable tests, read `firmware/README.md` and `docs/firmware/BUILD_AND_TEST.md`.

# Required acceptance and evidence

Revision2 adds mandatory modular asset pipeline/playback and demo NDEF sticker tests. Read docs/MODULAR_ASSETS.md, docs/ANIMATION_ENGINE.md and docs/NFC_DEMO_STICKERS.md. Run tests/nfc/routing-cases.json through the same real C++ driver; tests/nfc/cases.json through actual C++ parser; tests/animation/samples.json through actual C++ player. Python references/catalog metadata checks are separate evidence. Verify adding a new local character/background/clothing variant requires only catalog/art changes and generation, while online enums remain unchanged.

The supplied vector suite is a starting integration gate, not exhaustive proof. Add executable tests below using production modules and deterministic dependencies. Every omitted/unexecutable check is reported, never auto-passed. Final source must not retain feature stubs hidden behind mock builds.

## Host software gates

1. Schema examples validate with Draft2020-12 plus format checking. Invalid fixtures reject. Firmware codec matches types, nulls, ranges, unknown keys, 64-bit care, UTF-8 character vs byte bounds, integer overflow/fractions, truncated/malformed/oversize/chunked bodies, deep nesting and unexpected MIME/content. Whole response validated before mutation.
2. Button bounce25ms, hold1s once, no short after long, simultaneous inputs, release after page change, boot-held ignored, monotonic/millis wrap and nonblocking audio. UI pages, null goal/budget, integer money, long text fallback, game pause/exit/15s/scorecap. Frozen review content held constant despite new top-level finance.
3. Bootstrap null/zero/empty, saved-epoch reboot retains pending actions, response correlation and late pre-reprovision results ignored. Lower state version rejected; equal version does not suppress valid outstanding event results. Bad epoch doesn't mutate state. Reset cancels playback and clears review/outbox/queue/version; durable reset intent survives interrupted bootstrap.
4. Duplicate commands start once; gaps allowed; reordered/conflicting sequences reject; queue bounded4; arriving food+ride order preserved. Cursor saved before start. Expired, unsupported animation and ghost-suppressed commands consumed in order. Ghost no ordinary activity or game; server-confirmed revival. Lost power after save may lose animation, never promise exactly-once completion.
5. Stable event UUID/payload through retries; outbox max8 no eviction; missing/unknown result IDs don't drop actions; accepted/duplicate/rejected terminal handling and rejection UX. Write failures before action, before cursor and after terminal result produce safe retry/recovery. Corrupt/torn NVS/config generation records. Reprovision discards old device identity state and in-flight results. Cached state cannot leak across device ownership.
6. Single HTTP in flight, input actions coalesce, <=1request/s. Retry2/4/8/16/30/60+jitter,429 header/error delay,401stop,403stop,400/422diagnostic,426fallback,5xxretry. Actions cannot defeat backoff.30s regular/2s demo. Lost network does not change care. Valid fake transport must still use actual serializer/parser.
7. Bounded NFC polling and presence debounce/removal gate. No storing/logging/transmitting UID/PAN. Reader missing gracefully degrades. SPI transactions use correct settings and locks; no overlapping CS or held lock across card/network waits. Fake call recording checks orchestration; physical bus validation separate.
8. Serial provisioning rejects bad/oversize input without partial save; no secrets in logs/errors/process args/Git. Validate host, device identity and update generation. Timeout/incomplete serial line doesn't hang UI. All device config examples contain only conspicuous placeholders.
9. Asset converter synthetic tests:32x32 dimensions, transparency, allowed manifest IDs/frame limits, deterministic output, malformed assets rejected. No final art required. Missing asset uses fallback. Build production and local-demo with same core, storage separation and visibly distinct local demo.
10. Native compiler warnings reviewed; sanitizers when supported. Build from clean checkout using pinned toolchain/library configuration; record command, revision, flash partition/sketch size and static RAM. Measure runtime later; don't replace measurements with theoretical SRAM totals.

## Must remain pending until actually exercised

- Actual ESP32 flash/boot, actual flash and PSRAM detection, display colors/orientation/flicker.
- PN532/card/sticker detection in final enclosure, 3.3V logic/power verification, shared bus stability.
- Buttons <=100ms response and target20fps during missing card and slow Wi-Fi/HTTP; measured task stacks/free/min/largest heap under TLS and repeated reconnections.
- HTTPS valid and invalid CA/hostname, bad clock, production redirect rejection. A mock cannot validate certificates.
- Companion registration/token -> serial provision -> bootstrap -> food once -> ride next -> appearance -> tap/frozen review/Bhold -> accepted check-in -> ghost/revive -> reset. Record server-to-screen latency; no guaranteed number before measurement.
- Actual battery/boost voltage, charging current, temperatures/runtime are engineering bench work, not code tests. No firmware battery percentage.

Report statuses as PASS / FAIL / BLOCKED / NOT RUN and label evidence layer (pack / host / cross-compile / hardware / live). Never count BLOCKED as PASS. Keep unresolved bugs in docs/VERIFICATION_REPORT.md.
