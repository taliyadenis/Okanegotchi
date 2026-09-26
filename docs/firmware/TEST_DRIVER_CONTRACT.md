> Historical design reference from the earlier firmware branch. For the current runtime, pet IDs, supported NFC tags, commands and executable tests, read `firmware/README.md` and `docs/firmware/BUILD_AND_TEST.md`.

# Black-box driver for production C++

Cursor must implement `firmware_test_driver` as a native C++ executable linked against the same domain/codec code used by the ESP32. The provided Python runner runs one process per case, sends **one JSON line** to stdin and expects **one JSON line** on stdout. Logs go to stderr. Exit0 only when input executed normally; assertions belong to independent runner/tests. No driver exists in this initial pack.

Input: `{ "seed": { ... }, "steps": [ ... ] }`. Output: `{ "snapshot": {...}, "trace": [...], "requests": [...] }`. Do not use case IDs in logic; they are not sent to the driver. All clocks/RNG/transport/storage are fake adapters, all decisions and JSON serialization/parsing are production code. Host adapter may use the actual ArduinoJson portable library. No Internet or real secrets.

## Seed

Default fixture state from `mvp/examples/sync-response.json`, UTC pinned to its server_time, monotonic0, demo=true. Persist fixture epoch, cursor4, empty outbox and cached fixture state; clear its event_results, commands and review at initialization. Set initial state_version18. Default config valid using example device ID and a fake token; transport never sends it externally. No spontaneous action or animation. `paired:false` seeds no epoch/cursor. `ack:0` overrides cursor, `ghost:true` sets stageghost, `buttons_held:["B"]` specifies boot-held raw input, `monotonic_ms` overrides startup clock, `outbox` seeds validated persisted events. Boot initializes the actual app/store but does not pump the scheduler: first sync is due, not yet sent. The same rule applies to reboot. Enqueue calls the production durable action boundary without pumping the scheduler; advance/button/nfc/sync/http may pump it. Tests using historical fixture dates must never use the machine wall clock. An event result must correspond to the immutable active request's submitted events, not merely an event currently in the outbox.

## Operations

| op | Fields / semantics |
|---|---|
| advance | `ms`: advance fake clock and pump real timer/UI/scheduler in <=5ms increments, no real sleep. Fake transport captures due requests and keeps at most one pending; it does not auto-complete. |
| button | `name:A/B/C`, `down:boolean`: change raw pin then pump one iteration; debounce only with later advance. |
| nfc | `present:boolean`: fake detection observation delivered to production presence reducer. No identifier. Advance pumps periodic observations at the implementation's polling cadence, all consistent with last present level. |
| sync | `patch` optional: pump due/queued request then complete it with full fixture response, request_id taken from that real request, server_time using fake UTC; default commands/event_results empty, reviewnull; merge patch recursively, arrays replace. Default command expiry dynamically fake UTC+120s, reviewexpiry+300s unless explicitly supplied. If bootstrapping commands empty; return seeded epoch. Every successful full fixture passes actual codec. A pending request must exist or this op fails. |
| http | `status`, `body` optional object/string, `headers` optional: complete pending request through same network result adapter. `body_fixture:"reset"` produces schema-valid409 with a different fixed epoch; `body_fixture:"unauthorized"` valid401 error. If no pending request pump scheduler, otherwise fail. |
| enqueue | `event`: call production outbox action-enqueue boundary with validated event; exercise persistence/overflow, not a private vector shortcut. |
| fail_store | `writes`: next N writes fail (default1); failures must affect production durable adapter and safe handling. |
| reboot | discard volatile objects, retain fake durable store, recreate production app; no elapsed wall time unless separate advance. Pending requests discarded. Reboot isn't factory reset. |

For `sync`, an explicit patch request_id bypasses correlation replacement; patch epoch overrides the fake server's current epoch. This allows invalid envelope testing. The fake server begins with the fixture epoch; reset-error switches its epoch to 22222222-2222-4222-8222-222222222222, retained across client reboot, and subsequent bootstrap returns this new epoch. Patch state_version19 when changing seeded authoritative state; equal18 can still supply command/event results without changing cached state. `http` does not imply schema-valid body. Body caps, chunk handling and real TLS need additional adapter tests outside this simple driver.

## Observable output

Snapshot fields required: `page` (Pet/Goal/Check-in/Menu/Review/Game), `active_animation` (idle or protocol animation), `care_stage`, `ack_command_seq`, `outbox_size`, `epoch` (nullable), `state_version`, `storage_fault`, `network_status` (ready/backoff/unauthorized/forbidden/update_required/error), `queued_commands` (pending count excluding active), `mute`.

Trace records emitted at production dependency boundaries: `animation_start` with `epoch`, `seq` and `animation`; `cursor_saved` with `epoch` and `seq` (only after successful durable commit); `button_short`/`button_long` with `button`; `event_queued` with `event_type` and `event_id`; `response_rejected` with diagnostic reason; `storage_error`; `reboot`. Optional extra traces allowed; never include secrets. Every server-command start must have an earlier successful cursor_saved for that epoch+sequence; the runner enforces this globally. Local cosmetic feedback uses a different trace type. Requests contains decoded real outgoing wire JSON in emission order, without Authorization/config secrets.

Runner expectations: `equals` maps dotted object paths (array numeric indexes supported) to values; `counts` selects `trace` records by partial `where` object; `ordered` checks matching trace subsequence; `request_count` checks emitted requests. This is a public test interface, not production networking. Add production-unit tests for edge cases not represented by these operations. Do not replace the runner by code that merely prints expected traces.

## Revision2: content and demo-tag driver extensions

The same firmware_test_driver must also run tests/nfc/routing-cases.json. `nfc` accepts optional `tag_type:"ntag213"` and `ndef_hex` (verified Type2 user-memory bytes supplied by fake hardware). Execute the actual C++ NDEF parser/router. Default absence of these fields retains generic card-review behavior. `nfc` with `observation:"error"` means an indeterminate read/presence failure, NOT absence; preserve latched presentation and RESET the continuous confirmed-absence window. A later no-target starts a fresh500ms window. Fake observations remain at their last status during advance.

New `connectivity` operation sets `online:boolean`, delivers actual network health change to the application, and fails/discards any pending fake request on disconnect through the production failure path. Do not bypass demo creation policy in tests. Initial cached demo=true is not a fresh live authorization; only a completed valid sync establishes freshness. Capture trace event_queued from the production durable boundary, not the driver. Scenario tests initially sync to establish live mode; a stale pending intent is processed only after fresh response, keeping original presentation consumed.

Additionally implement native `content_test_driver`, linked to production C++ NDEF parser and AnimationPlayer. One input JSON line/output JSON line per process; no fixture IDs sent. `{"op":"parse_ndef","hex":"..."}` returns exactly `{"result":"food|ride|savings|review|unrelated|invalid"}` (one enum value). `{"op":"sample_clip","durations_ms":[125,125,125,125],"mode":"loop","elapsed_ms":500}` returns exactly `{"frame":0}`. Use actual production parser/player; no Python reference delegation or hardcoded case results. Debug logs stderr only.

Run `python tools/run_content_acceptance.py --driver <content_test_driver>` for24 NDEF+22 timing samples. The sample at8000ms tests clip sampling itself; a separate ReactionScheduler test must prove the8000ms activity ends before another frame is rendered. Add renderer/asset converter/gating/error tests beyond these fixtures per ACCEPTANCE.md.
