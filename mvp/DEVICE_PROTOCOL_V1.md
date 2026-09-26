# Okanegachi protocol v1

Implementation specification, September 25, 2026. This replaces old `/api/device/state` and `/api/device/events` sketches. Finalized for the recommended MVP, not an already running service. JSON schemas and example payloads in this folder are the structural contract; this document supplies stateful rules they cannot encode.

## Transport and routes

HTTPS, TLS server certificate validation, UTF-8 `application/json`, `Cache-Control: no-store`. No MQTT, WebSockets or inbound device port. API version is integer 1. Server timestamps are UTC RFC3339 ending Z. Weekly totals use Monday00:00 to the next Monday00:00 in the configured timezone, counting normalized expenses once and excluding transfers. All money in v1 is integer US cents with currency USD; future multi-currency support needs explicit conversion/account rules. No floating-point dollars in API state.

Device URL: `https://<project-ref>.supabase.co/functions/v1/device-api/v1/sync`.

Browser API base: `https://<project-ref>.supabase.co/functions/v1/app-api/v1`.

| Function/route | Authentication | Behavior |
|---|---|---|
| `device-api` POST `/v1/sync` | Opaque device bearer token | Actions, command acknowledgement, state and next queued reactions in one round trip |
| `app-api` GET `/v1/dashboard` | User JWT | Owner's pet, goal, bounded history, last seen and current demo session |
| `app-api` PUT `/v1/appearance` | User JWT | `{pet_id,name,palette,accessory,asset_version}`; validate built-in enums |
| `app-api` PUT `/v1/preferences` | User JWT | `{timezone,weekly_budget_minor}`; validate IANA zone, budget positive cents or null |
| `app-api` PUT `/v1/goal` | User JWT | `{name,target_minor}`; saved amount comes from normalized goal events, not arbitrary device input |
| `app-api` POST `/v1/devices` | User JWT | Register owner's one device, return random bearer secret once |
| `app-api` DELETE `/v1/devices/{id}` | User JWT | Revoke owner's device token |
| `app-api` POST `/v1/demo/session` | User JWT + demo-profile flag | Enable a 15-minute demo lease; returns expires_at |
| `app-api` POST `/v1/demo/scenarios` | User JWT + active demo lease | `{event_id,scenario}`; food, ride, savings or neglect; idempotent by owner/epoch/event ID |
| `app-api` POST `/v1/demo/reset` | User JWT + active demo lease | `{request_id}`; reset fixtures and create a new epoch; repeated same request returns same reset result |

Device `verify_jwt=false` is necessary for a non-Supabase opaque token; it does **not** make the handler public. Hash and look up the token, check active device/owner before domain access. Do not log Authorization. Device supplies its ID for consistency; auth identity is the token record, never the supplied ID alone. Registration token: cryptographically random 32 bytes encoded base64url; store SHA-256, show once, rotate by revoking/reprovisioning.

Browser function retains Supabase JWT verification and checks current user/owner. Browser uses the public Supabase client configuration and session; backend has service secrets in hosted environment variables. Do not send those secrets to ESP32. Restrict browser CORS to actual deployment/development origins; CORS is not authentication. Privileged SQL RPCs must be callable only by the backend, not anon/authenticated browser roles.

## POST sync request

See `device-sync-request.schema.json` and `examples/sync-request.json`.

- `api_version`, `request_id`, `device_id`, `epoch`, `ack_command_seq`, `events`, `telemetry` required.
- request_id is a UUID for correlation, **not** a promise of byte-identical cached responses. A retry can receive newer state.
- epoch is the server reset generation UUID. Only bootstrap uses null.
- ack_command_seq is the greatest command sequence consumed in ascending order, persisted before each reaction starts. It is not the last command merely received.
- events contains at most 8 stable-ID actions. Retain each event unchanged until a terminal result. Event types: `review_requested`, `checkin_completed`, `play_completed`, `demo_trigger`.
- telemetry includes firmware version, supported asset version, uptime seconds and RSSI only. No battery percentage or real card identifier.
- Bootstrap: epoch null, ack 0, events empty. The server returns state/current epoch, commands empty. Persist epoch then make a normal sync. Bootstrap never submits cached financial actions under a guessed epoch.

Firmware caps request JSON at 4 KiB and response JSON at 8 KiB. Reject oversized/chunked bodies after the same cumulative byte limit. Server emits responses under 8 KiB; short strings and <=4 commands normally keep them smaller. The prior 4 KiB response target is superseded to accommodate frozen review content and event results. No PNG/frame/base64 assets in sync.

## Response

See `device-sync-response.schema.json` and `examples/sync-response.json`.

- Echo request_id; return epoch, server_time, monotonically increasing state_version, next_sync_ms and demo_mode.
- Return appearance IDs; care stage and daily window progress; a minimal financial summary; optional goal; optional active frozen review receipt; event_results; up to 4 ascending commands.
- `finance.data_source` is `demo`, `plaid_sandbox`, or `plaid_trial` (optional eligible live-data trial). `as_of` reports financial data freshness; server_time is not proof the bank data is current.
- `care.stage`: content, needs_checkin, sleepy, ghost. Care is attention, not financial worth. `budget_status`: no_budget, on_track, over_budget based on user budget and normalized totals, not AI.
- September 26 prerelease appearance amendment: send pet_id gator/robot/duck, palette original, accessory none, asset_version 1, matching the uploaded shared art. Schemas retain earlier piggy/cat/dragon and color/accessory values for transitional validation; the live UI must offer only compiled current presets. Older firmware must be updated together with this amendment.
- command animation: eating, traveling, celebrate, neutral, revive. Default activity 8,000 ms; revive 2,000 ms. Duration allowed 2,000–10,000 ms. Sequence is increasing but gaps are allowed.
- reaction command expires 120 seconds after creation. Server omits expired commands. Max 32 live commands/device; if full, discard oldest undelivered command with a diagnostic count, keep latest events in financial history. These overload semantics intentionally favor recent reactions over playing an hour of backlog.
- Hardware holds at most four pending commands plus one active command. A response still contains at most four commands. Filter already active/queued/consumed duplicates before checking capacity. Do not replace a running eating loop when a travel event arrives. Keep ascending order, then idle.

## Atomic processing and reset

One active device per pet in v1. One sync in flight on the device. Server authenticates, validates schema and performs one DB transaction/RPC that locks the pet/device, checks epoch, evaluates idempotent events/ack, updates connected care/last-seen, and reads consistent state/queue. Do not emulate a transaction with a series of independent HTTP table updates.

Check epoch before any event, acknowledgement or telemetry writes. Old epoch returns HTTP409 `RESET_REQUIRED` with current_epoch and no mutations. Firmware stops old reaction playback, durably clears old commands/events/cursor/epoch, then sends an empty null-epoch bootstrap. It persists the epoch from the successful bootstrap response. Never relabel a stale event with a new epoch. Only a current authenticated owner can reset a demo profile; reset cancels active review receipts and starts the fixture scene. Keep reset request IDs to make retries idempotent.

For each input event return `{event_id,status,reason}`. status is accepted, duplicate or rejected; reason is null on success and a fixed reason on rejection. Device removes terminal entries and tells the user when an action was rejected. Transient DB failure aborts the transaction with5xx so inputs are retained/retried. Malformed request returns400 before processing any events.

Unique `(device_id,epoch,event_id)` prevents duplicate effects. Store event payload digest and result. Reuse of an event ID with different payload rejects `EVENT_ID_REUSE`, not another action. Lookup existing successful result before checking receipt expiry again: a successful action cannot fail simply because its first response was lost.

Server checks ack <= highest sequence delivered to that device in this epoch and never decreases its own ack. Persist `last_delivered_seq` transactionally when forming a response; a dropped response can be redelivered. Device persists cursor immediately **before starting** each reaction; then animates and eventually acknowledges on sync. This is at-most-once local start across reboot, with possible loss if power fails after saving cursor but before display. It is not guaranteed exactly-once completion. Deliberately skipping expired/unsupported commands also consumes their sequence; never acknowledge later commands before earlier returned commands are handled. Confirmed financial effects remain idempotent independently of animation playback.

Apply only current-epoch responses from the active request; never roll back state_version. Poll state changes do not restart an animation; only unseen queued commands do. NVS cursor writes happen per consumed command, not per frame. Preserve pet/config/cursor/event outbox across reboot with bounded records; if storage fails, stay idle and report an error rather than claim durable delivery.

## Review, check-in and care

Contactless card or menu sends `review_requested` with a source. Server creates/reuses a persisted receipt bound to the device, epoch and a frozen **financial snapshot version**, valid for five minutes. Response carries that snapshot's short summary, goal progress and as_of. Retry of the same review event returns the same receipt even if the original response was lost. An expired unused receipt requires a new review_requested event ID; an existing duplicate result does not generate a fresh valid receipt silently.

Device displays summary, then B-hold sends `checkin_completed` with review_id. Server validates owner/device/epoch and receipt expiry, marks receipt consumed, credits current local AM/PM window at most once, and resets elapsed care. A second new event using an already consumed receipt rejects REVIEW_USED; retry of the original accepted event remains duplicate success. This encourages review but is not cryptographic proof of attention.

Ghost check-in resets care and queues revive. Existing old activity commands are suppressed while ghost and not resurrected later. Game result is cosmetic only; no check-in credit or money mutation. Demo triggers require both demo-profile flag and active lease. NFC financial scenario tags are not normal-mode behavior.

Authoritative care: at each authenticated regular sync, add server-time delta since last accepted regular sync only if <=90sec. Retry cannot double-count because stored last_sync_at advances transactionally. Bootstrap never advances care. Long gaps add zero. After valid check-in reset elapsed to zero; derive stage from thresholds12h/24h/48h and set care streak to0 on first ghost transition. Streak counts consecutive server-confirmed days with both windows completed; offline grace does not award missing windows, and no claims that the streak has an outage guarantee. Keep `care.stage` and AM/PM goal separate.

No ghost merely because the server is unreachable. Device keeps cached stage/idle; offline confirmation is provisional. If a queued confirmation arrives after its receipt expires, reject REVIEW_EXPIRED and request a fresh summary. Display freshness and pending state clearly.

## Retry and polling

Normal next_sync_ms=30000; active15min demo lease=2000. Server controls lease expiry. Immediate review/action requests may bypass poll timer but coalesce and cap to at most one request/second. Keep animation/buttons outside blocking HTTP calls (separate networking task/queue).

Network/5xx: exponential backoff 2,4,8,16,30,60 sec plus up to20% jitter. Respect429 Retry-After.401: stop network retries until token replaced.400/422: show/log bounded diagnostic, do not loop rapidly.409: reset handling above. Unsupported api_version/asset_version: return426 and keep existing local pet/fallback, show update-required. Never disable TLS to bypass errors.

## Browser scenario contract

POST demo/scenarios example: `{"event_id":"d03cff08-cba8-40fc-975a-84b3b02e1381","scenario":"food"}`. Active lease required. Final fixtures: food = USD12.50 food expense; ride = USD18.00 transport expense; savings = USD10.00 goal contribution (a transfer, not an expense); neglect = set connected care to48h only on demo profile. Timestamps are injection time; IDs stable per retry. Keep a deterministic reset base: goal USD50 saved of USD100, optional weekly discretionary budget USD100 and pre-scene spend USD25.00. These are demo fiction, not user account facts.

Suggested success: `{"event_id":"...","status":"accepted","state_version":18}`; duplicate preserves the action's original result and does not enqueue another reaction. Appearance updates are idempotent PUTs. All browser reads/writes enforce owner identity derived from JWT. Optional Plaid handlers are separate from these fixtures and must normalize into the same rule engine.

## Files and checks

Schema validation validates shape/enums/limits, not auth, transaction isolation, ownership, TLS, care clocks or live hardware. Implement behavioral tests for all those boundaries. Included examples exercise the schema. Use the provided verification script and a standard Draft2020-12 validator when integrating; inspect each tool report for the actual verification scope. No API implementation is included by this document.
