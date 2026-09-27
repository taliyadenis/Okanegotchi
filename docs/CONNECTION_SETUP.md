# Account persistence and physical connection

## Deployment status

Implemented and tested locally. The SQL migrations and Edge Functions must be deployed to the team's Supabase project before account saving or physical sync works. A public project key cannot deploy these resources. No service-role secret belongs in the website or firmware.

This is the **connection/state-sync portion** of build-plan section 3, not the full reaction/check-in protocol. Website account persistence includes companion setup/preferences, selected fictional accounts, savings goal and demo-letter history. Logged-in returning users with completed cloud setup open Financial accounts instead of setup. Guest data stays in its browser. Older browser-only signed-in data remains untouched; it is not automatically uploaded. Review and save setup again when adopting cloud persistence.

## Deploy

**Existing project upgrade blocked pending reconciliation (2026-09-27).** Live read-only inspection confirms `account_documents` uses `owner/document_key/content`, while this application expects `user_id/kind/value`. The device table also differs. Do not run `db push`, database reset, or paste the old migrations into this existing project. The `CREATE TABLE IF NOT EXISTS` statement does not align an existing table. See [integration change record](BACKEND_INTEGRATION_CHANGELOG.md) for inventory and progress.

Before an existing-project deployment: obtain a private recoverable backup, audit schema/functions/grants and data mappings, rehearse a data-preserving upgrade and rollback locally, and reconcile migration history. Do not mark old migrations applied while the live definitions remain incompatible. Current official guidance: [Supabase migrations](https://supabase.com/docs/guides/deployment/database-migrations) and [backups](https://supabase.com/docs/guides/platform/backups).

The following historical commands describe **fresh installation only**, after confirming an empty target database and reviewing all migrations. They are not the upgrade procedure for `ynutfftknaieisbaioaz`:

```sh
npx supabase login
npx supabase link --project-ref <reviewed-empty-project-ref>
npx supabase db push
npx supabase functions deploy app-api --project-ref <reviewed-empty-project-ref>
npx supabase functions deploy device-api --project-ref <reviewed-empty-project-ref>
```

Review db push's migration list before applying. Track schema changes through migrations; do not bypass the history by applying the old SQL files manually to a nonempty project.

Set ALLOWED_ORIGINS in Edge Function secrets to the deployed website origin plus local development origins, comma-separated. The default permits only http://127.0.0.1:5173 and http://localhost:5173.

Both functions disable gateway JWT verification in config.toml because app-api explicitly validates the current user with Supabase Auth and device-api uses an opaque token. This is not anonymous authorization: missing/invalid credentials are rejected by each handler. Hosted SUPABASE_SERVICE_ROLE_KEY is used only inside the functions.

## User identity and synchronization

- Supabase assigns a stable Auth UUID and individual session tokens to each email/password account. No shared user password or per-user public API key is created.
- RLS exposes only the current user's documents. The save RPC derives ownership from auth.uid(); callers cannot submit a different owner.
- Browser writes use compare-and-swap revisions per document. Conflicts stop saves and ask the user to load the latest cloud copy. Identical retries after lost replies return the existing revision.
- Setup continues only after its save is acknowledged. Other changes show Saving / Saved / failed at the top. Unsaved work is kept in memory for retry and warns before reload; it is not an offline durable queue.
- Each login/reload reads cloud data first. Use Load latest saved data in an already-open browser to pick up changes from another device; no realtime subscription runs.
- The data is still synthetic demo data. Bank credentials and live transactions are not collected.

## Physical device registration

After setup, Register my device creates one owner-bound device ID and a cryptographically random 32-byte bearer token. The token is returned once, displayed masked, never persisted by the website, and stored only as SHA-256 on the server. Lost tokens require revocation/re-registration. Revoke immediately denies future polls. Device status displays last successful sync, not a guarantee the screen updated.

Provision privately over USB using the hardware team's tool: Wi-Fi SSID/password, device ID, API base and token. Use the exact deployed project URL with TLS certificate validation. Do not send the user's password, browser JWT or service-role key to the ESP32.

API base: https://ynutfftknaieisbaioaz.supabase.co/functions/v1/device-api

POST /v1/sync uses the build-plan bootstrap request (epoch null, ack_command_seq 0, events []). A valid response supplies the epoch; subsequent polls use it every 30 seconds. Requests are capped at 4 KiB and responses at 8 KiB. Token/device mismatch or revocation returns 401; an old epoch returns RESET_REQUIRED before writes. No commands are issued, so ack remains 0.

## Firmware compatibility and remaining work

Use `origin/codex/firmware-usb-bringup-noel` (reviewed at `65149a8`), its `mvp/*.schema.json`, `firmware/src/protocol.cpp`, and `docs/firmware/INTEGRATION.md` for alignment. The copied `hardware/firmware-mvp` prototype is not authoritative. Current firmware expects gator/robot/duck in original artwork, `palette=original`, `accessory=none`, and `asset_version=1`. The state-only backend still emits `palette=mint`; its goal-name, monetary bounds, goal semantics and state-version updates also need reconciliation against the actual schema/parser. This documentation correction does not establish compatibility or authorize device deployment.

State currently supplies setup, sample weekly spending and the linked sample savings goal. Care fields are initial placeholders (content, zero elapsed/streak, no completed windows). Demo lease, review/check-in processing, reaction queue/ack delivery, reset and financial scenario ingestion are **not implemented**. Requests containing events return 501 without consuming the outbox; do not enable NFC/actions against this endpoint yet. Old firmware accepting an HTTP response is not proof of correct rendering.

## Acceptance

1. Deploy migrations/functions, then save setup under account A; wait for Saved.
2. Sign into A in another browser/computer. It must open Financial accounts and recover setup, selected sample accounts, goal and letter history.
3. Account B must see none of A's data. Anonymous requests must fail.
4. Edit the same document in two browsers; the stale writer must show a conflict and preserve the server's newer copy.
5. Register A's device, bootstrap and poll with its token. B cannot revoke it; an incorrect token/device pair fails. Revoke and verify 401.
6. Provision the compatible physical board, verify last-seen updates and exact pet/goal display after website edits. Record board/firmware version and test result.

Local checks: PGlite executes both migrations and verifies RLS, revision conflicts/retries, token lookup, epoch handling and revocation. Unit tests check save queues, account isolation, bounded requests and state normalization. These checks do not replace hosted Auth, gateway/CORS, two-browser or physical-board tests.
