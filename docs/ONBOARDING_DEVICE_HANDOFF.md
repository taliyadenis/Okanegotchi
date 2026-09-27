# Onboarding and physical-device handoff

**Rebase integration update:** The teammate’s `account_documents` revision-checked CloudStore and AccountGate now provide signed-in persistence. The earlier companion_workspaces migration and snapshot-sync adapter were superseded and removed. Onboarding completion is stored within the setup document and survives pet edits. Guest state remains local. Device registration/status uses the teammate’s app-api foundation; physical letter delivery still awaits verified firmware compatibility and acknowledgement. See `docs/CONNECTION_SETUP.md` (or `CONNECTION_SETUP.md` from this folder) for current backend deployment steps. Earlier persistence notes below are historical.

## Implemented locally

The requested gator icon is in `public/gator-icon.webp` and used in the centered header, favicon and app icon. Login/signup copy and the Japanese footer follow the supplied message. Existing login art and authentication remain intact. Latest website changes through `f52a7c5` were reconciled into the working tree, including all 24 gator/robot/duck sprite sheets and their build pipeline. Earlier uncommitted copy cleanup was preserved.

Dashboard, Pet, Banks, Goals and Letters are permanent navigation. Onboarding completion has a separate per-identity flag; finishing the walkthrough opens Dashboard. Returning completed users open Dashboard after refresh/re-entry. Pet edits do not reset completion; save leads to a named confirmation with connection next steps. The character preview uses the entered name (or okanegotchi) and ordered Idle, Walking, Eating, Celebrating, Sleepy, Neglected, Ghost, Revive controls. Reduced motion starts paused and responds to preference changes.

Bank developer controls and Maybe later are removed. The savings action requires a linked savings account. Goal editing and full-savings-balance progress remain; the manual celebration and concept badge are removed. Letters show only pet name/type/fund in the summary, numbered 5.1–5.3 subsections and a physical destination. No simulator submit, export or local edit button appears in that flow. The old simulator modules/tests remain as development tooling and preserve legacy saved history; simulator receipts are not shown as physical receipts.

## Persistence and deployment boundary

Local per-identity keys persist pet/preferences, fictional bank connections, goals, old letter history and onboarding completion. Guest data stays on that browser. Signed-in users have an account-sync adapter for `public.companion_workspaces`, with an owner-only RLS migration in `supabase/migrations/202609260001_companion_workspaces.sql`. It stores only this UI workspace, never passwords, auth tokens or device credentials. Its contents are not authoritative financial/device state.

**Migration not applied, backend not deployed, cross-browser hosted behavior not verified.** The UI says account sync is unavailable if the table/service is missing. Local edits remain stored with an unsynced marker for retry. Successful account writes are explicitly reported; a second browser hydrates that saved row after login. Unsynced local edits take precedence over a remote snapshot on retry. Concurrent browser edits use last completed snapshot write wins; this is not a collaborative merge. Pending local changes survive logout on that browser, but are not available elsewhere until sync succeeds. Existing guests are intentionally not silently merged into authenticated accounts.

## Hardware inspected

Branch `origin/hardware/firmware-mvp`, commit `d17cdd7f56b376917cbd430876d88fd1d86535ed`. Exact protocol, request/response schemas, historical response example, website handoff, JSON codec and sketch are copied under `docs/hardware-reference`. No firmware source or protocol on the hardware branch was changed.

Protocol: device POST `/functions/v1/device-api/v1/sync` with an opaque owner-bound device Bearer token; browser `/functions/v1/app-api/v1` uses the user JWT. Registration returns a secret once, then USB provisioning and verified last_seen are required. The provided sketch only initializes pins and Serial and delays in loop; it has no implemented serial provisioning command or network/display integration. No exact provisioning command can be responsibly supplied from that sketch.

A real response requires **api_version, request_id, epoch, server_time, state_version, next_sync_ms, demo_mode, pet, care, finance, goal, review, event_results and commands**. Nullable goal/review still appear as keys. Pet contains pet_id/name/palette/accessory/asset_version. Care contains stage, elapsed_connected_ms, streak_days, timezone, local_date, current_window, am_complete, pm_complete. Finance contains snapshot_version/data_source/as_of/currency/period_start/period_end/spend_minor/budget_minor/budget_status/summary. Goal contains name/target_minor/saved_minor/currency. Review carries an expiring frozen financial snapshot; events have stable IDs; commands have increasing seq, UUID, animation, duration and expiry. Schemas contain the full nested limits and exact names. The historical fixture is not a current user's letter.

This response is server-produced, **not** a website-exported setup letter. The browser must never invent epoch, authoritative care, review receipts, event results or command sequence values. Normal polling is 30s; the separately authorized 15-minute lease allows 2s. Responses max 8KiB, requests max 4KiB. No sprite bytes or credentials belong in JSON exports.

## Unresolved compatibility

- Website: gator/robot/duck; firmware v1 schema: piggy/cat/dragon. No silent species mapping was added.
- Website has no palettes/accessories; firmware v1 requires both. Agree a revised version or documented fixed defaults jointly; the UI does not restore these selectors.
- Both manifests currently call themselves asset_version 1 despite incompatible species sets. A coordinated asset/version release is required.
- Website progress uses full linked savings balance; firmware contract uses normalized goal-contribution events. Do not pass balances as new savings events.
- Codec validates request echo/epoch/nondecreasing state_version and some finance values, but does not fully enforce the schema or apply appearance fields. Sketch and codec do not establish complete physical functionality.
- ack_command_seq acknowledges consumed reaction commands, not this application's configuration-letter receipt. An authenticated application-status mapping still needs definition. last_seen alone is not proof a letter was applied.
- Account dashboard response shape, registration response and actual deployed endpoints are unverified. Connect device explains next steps and can check authenticated service availability; HTTP 200 never turns into a connected/applied claim.

Until these are resolved, physical Send remains disabled, no pack/send animation suggests success, and setup can finish without a paired device. The connection status remains actionable and explicit.

## Validation

41 application tests and 28 asset tests pass; production build passes with the existing large-chunk warning. Browser checks: creation confirmation, ordered animation controls, preserved saved accounts/76% goal, physical-only letters, finish-to-dashboard, refresh returning to Dashboard and direct pet rename/save without onboarding reset. Original name restored after test. Narrow sidebar checked. Live sign-in across two browsers, hosted RLS isolation, device registration/provisioning, physical rendering and authenticated acknowledgement remain unverified. No commit, push or deployment performed.
