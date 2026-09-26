# Egg letter transfer handoff

## Delivered, and what is not connected

Open Demo mode → Financial accounts → Your savings goal → Continue to your egg. Use the companion saved on the initial setup screen, review the linked savings goal, select Demo egg simulator, and pack/send. Supabase auth remains separate. Physical pairing is deliberately unavailable; no backend queue, firmware endpoint, USB/Bluetooth/Web Serial transport, or real device acknowledgement is implemented.

The README and local status/handoff documents were reviewed. `MVP_BUILD_PLAN.md`, `mvp/DEVICE_PROTOCOL_V1.md`, firmware sources and a handoff archive were not found in this workspace. Everything below is a proposed demo application contract, not an assertion of firmware compatibility.

## Contract and modules

- `src/egg/packet.schema.json`: machine-readable JSON Schema, with strict required fields and no unknown fields. The runtime validator consumes this same schema, avoiding a separate hand-maintained field whitelist.
- `src/egg/contract.ts`: exported types, preset IDs, runtime validation, serialization, parse and receipt matching.
- `docs/examples/demo-letter.json`: valid sample packet; also validated in automated tests.
- `src/egg/transport.ts`: EggTransport interface and local DemoMailbox implementation.
- `src/egg/send.ts`: cancellable packing/flying sequence and bounded transport waits.
- `src/egg/LetterScreen.tsx` / `letter.css`: review, review of the previously selected companion, envelope animation, status, frozen history, simulator display and JSON download.

Proposed wire encoding: compact UTF-8 JSON, at most 2048 bytes (the parser also rejects oversized input before parsing). Names are nonblank, max 24 Unicode code points for companion and 60 for goal; the browser companion field uses a conservative 24 UTF-16-unit limit. Message IDs are 1–64 ASCII letters/digits/hyphens. Revisions are positive integers through 2147483647. Amounts are integer USD cents, target 1–99999999999, saved 0–99999999999. Timestamps are canonical real UTC ISO strings with milliseconds. Unknown fields, unknown versions/assets, nonfinite/fractional numbers and invalid dates are rejected, never silently truncated. JSON Schema describes structural limits; canonical-date and total UTF-8 byte checks are additional runtime requirements.

The only supported envelope is `okanegotchi.demo-letter.v4`, kind `configuration`, source `demo`, destination `demo-egg`. A future live adapter needs a deliberately versioned/validated extension; changing a provenance label is insufficient. Companion IDs are proposed matching application presets, not confirmed firmware IDs. The goal ID is the single primary goal slot; its revision identifies the snapshot and matches the configuration revision. Saved progress uses the full linked savings account balance, including no pretend $50 contribution from the goal preview. Progress percent is derived, not separately transmitted. Timezone and optional weekly budget are not yet collected by the app, so are omitted rather than invented.

Packets contain only companion selection, goal summary, provenance, timestamps and routing/version metadata. They exclude auth credentials, email, bank numbers and raw transactions. User-authored names are still personal content: show them in the review and treat exported files accordingly. Download validates the draft and creates a file without submitting or generating a receipt. Creation timestamp means draft capture time, not receipt time.

## State and delivery rules

Readiness requires a saved valid companion, saved goal, connected savings account, valid packet and demo destination. Stale/attention account data requires an explicit checkbox acknowledging the displayed last-known balance and timestamp. In-flight controls lock submission; packing can be cancelled before dispatch. Leaving during packing aborts before submission. After dispatch, the local saved queue persists; leaving does not revoke it.

The presentation transitions ready → packing (1100 ms) → submitting → flying (700 ms) → delivery status. Reduced motion bypasses those delays for send and uses static visual changes. Animation never creates a receipt. A received status requires packet parsing, validation, revision checks and a matching simulator receipt. Replay never invokes the transport. No goal/account/care state changes and no reaction events are emitted.

Transport submission records acceptance as queued. A separate status operation makes the simulator parse/apply and produce a receipt. Scenarios: online, offline, submission failure, schema rejection, timeout/unknown and duplicate receipt. Offline and unknown remain pending until an explicit reconnect/status check; this button represents the simulator coming back online. Schema rejection deliberately changes the wire version, not the stored reviewed packet. Select Online then Retry same letter to recover. Real adapters must not treat timeout as proof that a request was cancelled.

Receipt shape: messageId, destination, appliedRevision, outcome=applied. All must exactly match a submitted immutable packet; extra fields are rejected. Duplicate receipts assign the same configuration with no one-time effects. Older revisions cannot overwrite a newer applied configuration. One pending queued/unknown letter blocks new letters until reconciled. Failed letters may be retried with their original ID/contents, or a deliberate new send captures new contents with a higher revision. History retains eight letters; revision high-water and last applied packet survive pruning. Backend delivery will require durable ordering/idempotency instead of relying on this local high-water mark.

Versioned storage: `okanegotchi:demo-mail:v2:<identity>`, separate for guest and signed-in users. Stored companion, frozen history, revision and applied packet are validated on load. Malformed/obsolete records reset to an empty mailbox. Storage errors fall back to screen-local memory with a visible warning; reload/navigation may lose that memory. Identity remounts and AbortController cleanup stop the old screen's animation/status continuation. The local mailbox is not an authorization boundary or a multi-tab durable queue. Run one simulator tab per identity; cross-tab writes are not coordinated.

Reload does not replay animation or pretend a simulator ran in the background. Return through setup and explicitly check a pending receipt to resume. A real backend must continue durable delivery independent of the browser. Transport calls are bounded to 5 seconds with an unknown-status message; real-adapter reconciliation must persist attempts before network dispatch. The shipped local adapter is synchronous before its promise resolves, so submission records are available before the UI awaits them.

## Required physical pairing work

Agree with the hardware teammate on schema/version and sample vectors, asset IDs, display-name lengths/character support, maximum bytes, goal semantics, device provisioning and credential revocation, proof-of-possession pairing, registry ownership checks, endpoints, receipt authentication, revision persistence across reboot, queue expiration and reconnect behavior.

Planned architecture: authenticated website request → backend derives user and validates owned device + authoritative setup → durable per-device queue → authenticated egg fetch → firmware validates and idempotently applies newer revisions → durable authenticated receipt → owner-scoped status shown in website. A serial number or client device ID alone cannot authorize delivery. Never embed privileged backend credentials in the browser, packet or firmware. Queue/receipt access needs owner-scoped RLS/server checks. Real receipts must be authenticated by the backend; local JSON cannot prove that a physical egg applied anything.

Keep normal polling at 30 seconds. Only the explicitly enabled temporary demo session may use 2-second polling for up to 15 minutes. Envelope timing does not change polling cadence. The egg retains its last configuration when offline; PN532 NFC is not a file-transfer mechanism.

Sending setup is not a savings-contribution event. Any later food/ride/savings reaction requires separate stable event identity, provenance, duplicate suppression and agreed firmware handling. Sending/replaying letters cannot award care or revive the pet.

## Verification

Run `npm test` and `npm run build`. Tests include sample/schema validation, Unicode and escaping, unsupported fields/assets/versions, numeric/date errors, bytes/length limits, queue-vs-application separation, immutable retries, duplicate receipts, bad receipt identity, revision ordering, offline/unknown recovery, rejection recovery, history bound, storage isolation/failure, cancellation before dispatch, leaving after dispatch and bounded timeout. Existing financial and goal tests remain included.

Physical receipt, live owner authorization, pairing/revocation and firmware rendering are not verified. The simulator is the delivered integration test harness, not a production delivery service.

### Companion selection correction

The initial companion screen owns pet and name selection. The letter page reuses these saved choices. Palettes and accessories are not supported and are excluded from the v3 schema. Existing v1/v2 companion choices migrate using only name and pet. Frozen legacy letters remain untouched under their previous storage keys; the v3 mailbox starts a new revision sequence. This remains a proposed demo contract, not a firmware protocol. When localStorage is unavailable, a per-identity memory fallback carries setup and mailbox state across screens until reload.

## Timezone and weekly budget

The v4 configuration requires `preferences`: `timezone` (a valid Intl/IANA timezone, max 100 characters), `weeklyBudgetMinor` (positive integer USD cents up to 99999999999, or null for no budget), and `currency` (`USD`). Setup suggests the browser timezone and requires saving/reviewing preferences. No budget is inferred. The letter shows both values and freezes them at dispatch. Backend check-in windows, weekly spending calculations, care state and reaction events remain future work; no week boundary or check-in schedule is invented here. Firmware must agree to these proposed fields before physical use.

The v4 mailbox preserves only the companion from legacy v1–v3 setup; new preferences start unreviewed. Old frozen letters remain unchanged in their legacy keys. The new mailbox has its own revision sequence. Tests cover validation, identity isolation, preference persistence, immutable queued snapshots and migration.
