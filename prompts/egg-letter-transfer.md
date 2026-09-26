# Implementation prompt: send a little letter to your Okanegotchi egg

Build the next Okanegotchi experience: after completing setup, the user reviews their companion configuration and savings goal, then presses a button that visually packs the information into a paper letter, seals it inside an envelope, and sends it off screen toward their egg. Make this a delightful, functional JSON handoff with a testable delivery lifecycle. Deliver the website experience and a simulator that can later be paired with the physical egg through an agreed firmware contract.

## Read the existing context

Read `README.md`, `docs/PROJECT_STATUS.md`, `docs/SUPABASE_SETUP.md`, `docs/FINANCIAL_ACCOUNTS.md`, `src/main.tsx`, `src/auth.ts`, `src/styles.css`, and the complete `src/finance/` feature, particularly the savings-goal screen, models, fixtures and persistence. Inspect the working tree, follow applicable AGENTS.md instructions, and preserve teammate changes and ignored local environment configuration.

Search the workspace for `MVP_BUILD_PLAN.md`, `mvp/DEVICE_PROTOCOL_V1.md`, a hardware handoff archive, or newly supplied firmware/schema files. The README identifies the first two documents as authoritative, but they were absent when this prompt was written. PROJECT_STATUS mentions a protocol read in another checkout/archive; that statement is not access to its contents. If the contract is present, read it and follow it. Otherwise proceed with a clearly marked proposed contract, document unresolved firmware questions, and do not claim physical compatibility or invent official endpoints, asset IDs or acknowledgements.

Known design context:

- React, TypeScript, Vite and Tailwind frontend; Supabase Auth/Postgres and Edge Functions are the planned backend.
- The physical egg uses an ESP32-S3, a 240 × 240 display, PN532 NFC reader, three buttons and Arduino C++.
- The planned flow is website → backend → authenticated device polling → acknowledgement. Normal polling is every 30 seconds. A separately enabled temporary demo session may use 2-second polling for at most 15 minutes.
- The egg runs independently of an open website and keeps its last state while offline. NFC opens a review or an explicitly enabled demo scenario; it is not a file-transfer or payment mechanism.
- Planned pets: piggy, cat, dragon; palettes: mint, coral, lavender; accessories: none, cap, scarf. The current decorative artwork is not a finalized firmware asset. Do not substitute an unapproved gator or new sprite identifier.
- Financial accounts and goals currently use local demo data. Goals use the full linked savings balance. The $50 celebration preview is visual-only and must never enter an outgoing financial snapshot as an actual contribution.
- Care reflects check-ins, not spending or savings amounts. Sending a letter must not feed, heal, revive, or level up the pet unless a future explicit product rule authorizes that separately.

## Product outcome and scope

Add a final setup/review screen called “A little letter for your egg.” Reach it through a clear “Continue to your egg” action after the savings-goal screen, with navigation back to edit setup. Keep a coherent route or view structure so this does not become a collection of disconnected demo pages.

Support two explicitly separate destinations:

1. **Demo egg simulator**, available without authentication or hardware and clearly labeled throughout.
2. **Paired physical egg**, available only when a real authorized device association and transport implementation exist. Until then show an honest unavailable/setup-needed state and the simulator alternative. Do not enable a physical send button that calls the demo adapter.

An additional “Download demo JSON” action is a developer/hardware handoff aid. Exporting a file is not delivery to an egg and must not show a received confirmation. Do not implement USB, Bluetooth, Web Serial or local-network upload based on assumptions. Choose a different transport only if the actual firmware context specifies it.

Build the functional demo now; prepare clear integration interfaces and documentation for the hardware teammate. Do not deploy a backend, alter Supabase Auth settings, invent a real pairing claim, or connect real financial accounts as part of this task.

## Setup readiness and review

Check actual state instead of assuming earlier screens are complete. Present a short, friendly readiness list with edit actions:

- Companion appearance and name.
- Financial data source, visibly identified as demo.
- Named savings goal, target and linked savings account.
- Timezone and optional weekly budget, only where the existing scope/configuration supports them.
- Destination: simulator or an authorized paired egg.

The pet editor is currently a placeholder. If it remains so, add a compact setup form for the documented presets, name, palette and accessory, persist those choices per identity, and validate them. Do not silently fabricate a configured pet. Any proposed preset-to-firmware ID mapping stays in one shared contract module awaiting hardware approval. An explicitly labeled default demo preset is acceptable if the user can see and change it.

For this savings-focused flow, require a valid goal and connected savings account before sending a savings snapshot. Missing state gets a specific “Finish this step” action. A disconnected account must not become a zero balance. Stale/attention data must be labeled with its timestamp; define a deliberate policy for sending a last-known snapshot, and require review of that state instead of silently treating it as fresh.

Display a readable letter preview: pet name/appearance, goal name, target, saved amount, progress, currency and data freshness. Explain which fields are sent. Keep raw JSON behind an optional “View packet” details control, not in the main product flow. Never include raw bank transactions merely because they are available.

## The send interaction and animation

Match the existing yellow background, pink panels, blue primary actions, green accents, dark outlines, offset shadows, bold display type and monospace micro-labels. Create the animation with CSS and lightweight inline SVG; reuse existing design tokens and avoid a heavy animation library or generated raster assets.

Create a cream paper letter with pixel-like decorations, a small approved companion mark and a stamp. Keep the financial review readable until the user presses **“Pack & send to demo egg”** or the genuinely supported physical equivalent.

Suggested sequence (roughly 1.5–2 seconds total; tune for clarity):

1. Validate and capture the exact reviewed configuration as an immutable send snapshot. Show “Packing your little update…”
2. Miniature representations of the pet, goal and progress settle into the letter; use decorative duplicates so accessible review content does not disappear unpredictably.
3. Fold the paper, slide it into a pink/cream envelope, close its flap and add a green seal or pixel stamp.
4. At the dispatch point, enqueue/send the captured packet through the transport adapter. On accepted dispatch, let the envelope tilt and travel off the viewport toward a small egg/mailbox marker, leaving a brief dotted trail or a few restrained stars. Clip decorative motion locally so it never creates horizontal scrolling.
5. Show a persistent delivery card that reports the actual transport state. “Packed,” “Queued for your egg,” “Waiting for your egg,” and “Received by demo egg” are different states.

The animation is presentation, not proof of network success. Do not mark received on animationend or after a timer in the real adapter. A failed enqueue leaves the sealed envelope with “Your letter couldn’t be sent” and a retry action. An accepted queue with an offline egg says “Your letter is waiting for your egg,” not “Delivered.”

Do not replay the full animation on React rerenders, refreshed auth tokens, page reloads, receipt polling, or duplicate acknowledgements. Offer “Replay animation” only as a visual action with no second submission. Disable repeated submission while an attempt is in flight. If users edit setup while an earlier letter is pending, preserve that letter’s immutable contents and show that new changes have not been sent.

Provide a reduced-motion version: immediate/static transitions and the same useful status text. Use polite live announcements at major state changes, keyboard-accessible controls, predictable focus, readable contrast and mobile layouts. Do not force focus into decorative animation or make users wait through it to navigate elsewhere.

## JSON contract and transfer boundary

First use the real protocol if supplied. If unavailable, implement a versioned **proposed demo envelope**, with runtime validation and exported TypeScript types, not just unchecked JSON.stringify calls. Include a sample JSON file and machine-readable JSON Schema. Keep UI models, the application envelope, firmware mapping and transport separate so the eventual protocol can replace the proposal without rewriting the animation.

The proposed model should cover:

- Schema version, message ID/idempotency identity, configuration revision, creation timestamp, source freshness timestamp and explicit demo/live provenance.
- A destination reference resolved by the transport; a client-written device ID never proves ownership.
- Approved companion appearance identifiers and a validated display name.
- A goal identifier/revision, name, target and saved values in integer minor currency units, currency, and an explicit progress basis such as full linked savings balance. Do not count transfer pairs or celebration previews as contributions.
- Relevant timezone/budget preferences only if implemented and agreed for device consumption.
- A separation between a configuration/state snapshot and a one-time reaction event. Sending the same savings balance repeatedly must not repeatedly celebrate it. This feature sends setup/state; it must not manufacture a financial reaction event.

Specify field bounds, unknown-field handling, schema-version rejection, timestamp format, string encoding, maximum UTF-8 byte size and numeric limits. Treat unconfirmed hardware bounds as proposal values, not device facts. Include tests for multibyte names, escaping, invalid values, unsupported asset IDs and oversized packets. Avoid floats, NaN, undefined and silent truncation. Keep payloads compact for the device, and display long names safely in the web review.

Exclude passwords, Supabase session/refresh tokens, publishable/secret/service-role keys, Plaid tokens, raw transactions, account numbers, email addresses and unnecessary personal information from JSON exports and device packets. Authentication belongs in the transport, not inside an exportable letter. Compute derived values from validated state; a server-backed implementation must validate ownership and authoritative values again.

Define an asynchronous transport interface for submission and receipt/status lookup. Model submission acceptance separately from device acceptance/application. A matching receipt should identify the message and intended destination, report a protocol-defined outcome and, where supported, the applied revision. Reject mismatched, stale or malformed receipts. Document how real receipts will be authenticated by the backend; a local JSON object cannot establish delivery trust.

Retries of an uncertain or failed delivery reuse the same immutable packet and message ID. A deliberate new send after changed setup receives a new ID/revision. Serialize or otherwise order updates per device so an older delayed packet cannot overwrite a newer applied configuration. Do not claim exactly-once delivery; use idempotent application with durable acknowledgement in the eventual backend/firmware.

## Demo simulator and delivery states

Provide a deterministic simulator that validates the same proposed packet contract and displays what it accepted. Keep its display visibly labeled as a simulation, including the 240 × 240 egg display concept if shown.

Exercise these states: draft/not ready, ready, packing, submitting, queued/waiting, received/applied, failed, and unknown/timed out. Define transitions explicitly. Use a seeded/injectable clock and controllable delays for tests.

Compact demo controls should cover immediate receipt, offline/queued delivery, reconnect and delayed receipt, submission failure, invalid/schema-rejected packet, timeout with unknown delivery, and duplicate acknowledgement. Simulator acceptance should follow actual parse/validation/application code rather than a success timeout. Keep bounded delivery history with the frozen letter summary and status. Make persistence/versioning and storage-failure behavior explicit; separate demo identities and clear active subscriptions on logout/identity change.

Pending delivery must remain understandable when the user navigates away or reloads. In a real integration, the backend owns durable queue/status; browser state must not pretend to provide that guarantee. In the simulator, resume only the documented local simulation behavior and label it accordingly. Do not leave users in an eternal “Sending…” state if a callback never arrives.

## Physical pairing handoff

Provide `docs/EGG_TRANSFER_HANDOFF.md` describing the concrete contract, adapter boundary, test vectors and simulator usage. List remaining hardware decisions: firmware schema/version, allowed asset IDs and name lengths, packet-size limit, pairing mechanism, transport endpoints, credentials/provisioning, device registry/owner checks, receipt format, revision ordering and offline/expiration behavior.

For the planned HTTPS polling architecture, outline an authenticated backend enqueue operation, owner-scoped device registry and status reads, authenticated device fetch, schema validation and idempotent application on firmware, and a durable device receipt. Resolve ownership server-side, support revocation, and never ship privileged keys to the browser or firmware. Actual pairing should use a teammate-approved proof-of-possession flow; a typed serial number or local “paired” flag alone must not authorize control.

Keep normal device polling at the documented cadence; the envelope animation must not force rapid device polling or depend on the webpage staying open. Fast polling requires the explicitly enabled, bounded demo session described in the context.

## Acceptance criteria and delivery

- Existing authentication, financial account demo, goals and identity isolation continue to work.
- Incomplete setup has specific recovery links; complete setup produces a validated immutable packet that matches the review.
- A single press runs one pack/fold/seal/send sequence and makes one logical submission.
- Demo and physical destinations are never confused. Download, queue acceptance and device receipt have distinct copy/statuses.
- Offline, retry, timeout, schema rejection and duplicate receipts have tested behavior without duplicate state application or pet reactions.
- Goal balances, financial transactions and care state are unchanged by animation, replay or JSON download.
- Cover double-clicks, cancellation before dispatch, leaving after dispatch, identity changes, stale async responses, reload recovery, malformed storage and out-of-order revisions.
- Validate the JSON examples/schema, run focused automated tests and `npm run build`, and exercise the complete flow in the sidebar browser at desktop and narrow widths, with keyboard and reduced motion.
- Update README and project status to separate shipped demo capabilities from pending backend/pairing/firmware work. Avoid claiming a physical send unless a real paired egg returns a verified receipt.
- Leave a working preview open on the final review/delivery screen. Report what was built, verified, and still required from the hardware teammate. Do not deploy or change external account settings.

Deliver a polished working demo and a concrete hardware handoff. Treat missing firmware documentation as a limit on physical compatibility, not a reason to stop building the review, JSON validation, simulator and letter animation.
