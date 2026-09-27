# Backend integration handoff

## 2026-09-27 — account loading failure confirmed

- A read-only hosted REST request selecting `kind,value,revision` with `limit=0` returned HTTP 400 / PostgreSQL `42703` (missing column). No account documents or credentials were retrieved. The website's current document contract remains incompatible with the hosted table.
- `src/account/backend.ts` now classifies missing-column/missing-RPC errors; `src/account/AccountGate.tsx` explains that sign-in succeeded but the database update is pending instead of suggesting password or connection retries will repair the schema. Other errors retain the generic retry message.
- This is an error-message fix only. The user confirmed no backup exists; backup creation and restoration verification remain prerequisites for the live upgrade. No hosted schema change or successful account hydration is claimed.

## 2026-09-27 — resumed after website changes reached main

- Refreshed all published remote branches. `main` and `codex/backend-integration` are at `9f586a2`; current firmware remains `65149a8`, historical hardware remains `d17cdd7`, and software remains `0d504d3`. Preserve the website changes now on main.
- The earlier inventory-in-progress entries below are historical. Commit `cb73cdd` added the observed-schema fixture, atomic upgrade generator, persistence alignment migration and migration regression tests. This is local implementation, not evidence of hosted deployment.
- Re-ran `node --experimental-transform-types --test tests/alignment.test.ts tests/database.test.ts`: all four tests passed. Coverage includes fresh installation, synthetic observed-schema upgrade, ownership/revision isolation, preservation of document data and device identity/revocation, and invalid-data rollback. These tests do not establish a private live backup or restoration after a committed hosted migration.
- Regenerated `.local-backend/upgrade.sql` with `node tools/backend/prepare-upgrade.mjs`. It remains ignored and is a review candidate only; do not execute before the backup, current inventory and deployment gates.
- Inspected the authenticated hosted migration page: it still shows “Run your first migration.” This establishes absence of recorded migration history, not absence of manually applied changes. No hosted writes performed in this continuation.
- Requested the user's backup location and any intervening manual deployment status. No `pg_dump`, `psql` or `supabase` command is currently on PATH; the local private-artifact folder contains only the generated upgrade candidate.

Next action: confirm or create a private recoverable database backup, refresh schema/aggregate validation and rehearse restoration before reviewing the live upgrade. Task 1's hosted save/reload/isolation gate remains open. Do not treat the subsequent state projection, event processing, device transport or physical acceptance gates as complete.

## 2026-09-27 — inventory in progress

User requested the eight tasks in the MVP execution plan in order, inspection of all branches, and a change/reason record for the next agent. No hosted database changes, deployment, firmware flash, commit or push have occurred in this session.

### Branch review

Fetched all remote branches. Integration work is on `codex/backend-integration`, based on the latest `origin/main` (`2c7162b`, newer than the plan's `165a536`). Original local `software` branch is preserved.

| Published branch | Commit | Role |
| --- | --- | --- |
| main | 2c7162b | Current website layout and cloud interfaces; preserve these |
| software | 0d504d3 | Earlier cloud foundation, already included in main |
| codex/firmware-usb-bringup-noel | 65149a8 | Current firmware and authoritative MVP contracts |
| hardware/firmware-mvp | d17cdd7 | Historical prototype; inspected for handoff, not an implementation baseline |

`BACKEND_ALIGNMENT_HANDOFF.md` and the plan's `codex/backend-alignment-plan-noel` branch are absent from all fetched remote branches and local branches. No alignment-named file was found in all-ref Git history. Asked the user for the unpublished file/path. Do not invent its contents or replace this missing team decision record with this log.

Follow-up: user supplied the complete alignment handoff in chat. Read and accepted its requirements: preserve current API/CloudStore/UI, audit actual keys and distinct device identities, private backup, synthetic upgrade and rollback rehearsal, reconcile migration history, and explicit reviewed deployment. The missing document no longer blocks investigation. User also authorized confirming Supabase's generic warning for read-only schema/aggregate queries.

### Changes and reasons

- Added `tools/backend/schema-inventory.sql`: repeatable, read-only metadata inventory to establish the actual database before proposing a data-preserving migration. It does not select user documents, Auth rows or device credentials.
- Added this log: preserve exact branch findings, edits, reasons, validation and blockers for the next agent.
- Extended the metadata inventory with triggers, views, schema ACL and relation dependencies: renames can affect more than table columns and RPC bodies.
- Added `tools/backend/validate-observed-schema.sql`: aggregate-only checks for document shape/revisions, recognized keys, orphan ownership, distinct device identifiers, hash lengths and duplicates. Unknown document keys are grouped as `[unmapped]`; this query intentionally does not guess a mapping or disclose their values.
- Updated `docs/CONNECTION_SETUP.md` deployment section: the old directions could apply conflicting CREATE TABLE migrations to the known incompatible hosted schema. Marked them fresh-install-only, removed the project's ID from that recipe, removed manual SQL bypass advice, and documented the required backup/upgrade/history gate. Refreshed official Supabase migration and backup documentation.

Validation: the existing `tests/database.test.ts` passes locally (PGlite, one test) on the latest main baseline. This proves only the old fresh-install contract, not compatibility with the observed live tables. New SQL helpers have not yet been fully executed against the live database. Supabase's generic SQL warning needs browser confirmation before further metadata queries; user approval requested.

### Live read-only findings

Authenticated Supabase dashboard access works. A SELECT-only `information_schema.columns` query returned exactly two public tables:

- `account_documents`: `id uuid`, `owner uuid`, `document_key text`, `revision bigint`, `content jsonb`, `created_at timestamptz`, `updated_at timestamptz`.
- `registered_devices`: `id uuid`, `owner uuid`, `device_id uuid`, `device_token_sha256 bytea`, `is_revoked boolean`, `last_seen_at timestamptz`, `created_at timestamptz`, `updated_at timestamptz`.

This conflicts with the website's expected `user_id/kind/value` document contract and old device migration. Do not run the old CREATE TABLE migrations over it, rename identifiers by assumption, or discard content/revisions. Need document-key mapping, constraints, policies, RPC signatures/definitions/grants, aggregate validity checks, migration history, and private recoverable backup first.

### Gate status / next action

Task 1 remains in progress. Tasks 2–8 remain pending; no live save, cross-account, TLS, device or physical display success is claimed. Continue read-only inventory, obtain required team handoff, then reproduce the observed database with synthetic data and rehearse alignment/rollback before any live upgrade. Firmware instructions are at `firmware/AGENTS.md` on the current firmware branch; read before editing its source. Keep physical letter Send disabled.
## 2026-09-27 — hosted-schema compatibility adapter

- Updated `src/account/backend.ts` to read the currently hosted `account_documents` columns (`owner`, `document_key`, `content`) and call the hosted `save_account_document(p_document_key, p_content, p_expected_revision)` contract.
- Kept the adapter narrow and reversible; no hosted schema, migration, RPC, or device deployment was changed.
- This should restore website account hydration and saves for the observed hosted schema after the updated frontend is deployed. Device communication still requires the reviewed alignment migration and Edge Function deployment.
- Validation: TypeScript check and the four alignment/database tests pass. Browser/hosted account save verification remains outstanding.
