# Okanegotchi project status

**Rebase integration update:** The teammate’s `account_documents` revision-checked CloudStore and AccountGate now provide signed-in persistence. The earlier companion_workspaces migration and snapshot-sync adapter were superseded and removed. Onboarding completion is stored within the setup document and survives pet edits. Guest state remains local. Device registration/status uses the teammate’s app-api foundation; physical letter delivery still awaits verified firmware compatibility and acknowledgement. See `docs/CONNECTION_SETUP.md` (or `CONNECTION_SETUP.md` from this folder) for current backend deployment steps. Earlier persistence notes below are historical.

## September 26 — current onboarding/navigation update

The website now has permanent Dashboard · Pet · Banks · Goals · Letters navigation, separate onboarding completion, named gator/robot/duck previews, and a physical-only letter screen. The header/favicon use the supplied gator and the requested copy/Japanese footer is applied. Developer account controls and manual goal celebration are removed.

Local persistence works; the account-sync adapter and owner-scoped SQL migration are prepared but not deployed. Physical sending is disabled because firmware species/asset versions and receipt/provisioning contracts remain incompatible or unverified. See [onboarding/device handoff](ONBOARDING_DEVICE_HANDOFF.md) for exact fields, source revision, verification and blockers. Earlier implementation notes below describe historical stages and do not establish current physical connectivity.


Updated September 26, 2026. Local main incorporates Ayira’s `1702fd2` plus the Gator/Robot/Duck asset integration. No deployment performed.

## Current implementation

The latest remote app already includes Supabase signup/login, confirmation messaging, session restoration, sign-out, and a financial-account demo. Those newer implementations are preserved. The older desktop snapshot's overlapping authentication code was not copied over them.

The user created a Supabase project and configured its URL and publishable key in ignored `.env.local` in both the older snapshot and current Git checkout. The active preview was switched to the current checkout after main reached `107ccae`. No service-role key was supplied or added to browser source. Collaborators need their own local configuration using the existing team project; Git does not transfer ignored environment files.

The financial-account screen is a local synthetic demo. The companion screen now previews Gator, Robot and Duck with eight animations each. Color/accessory customization has been removed at the user's request. Selection and setup preferences now save locally for the demo-letter flow; signup does not provision a pet/profile/device. No owner-scoped companion API or device sync backend is deployed.

## Changes in this integration

- Add `docs/SUPABASE_SETUP.md` with local configuration, confirmation/session checks and backend boundaries.
- Add this current status report and link both documents from README.
- Preserve existing frontend, financial demo, dependency versions, and teammate changes from main.

## Verification

Latest combined integration: TypeScript and the production build pass (91 modules), along with 38 application tests and 28 asset tests. New pet IDs round-trip through saved setup and demo simulator receipts. The browser was reopened with both the animation selector and saved setup controls present. Vite reports a non-blocking bundle-size warning. Earlier checks below are historical.

On the older desktop snapshot, TypeScript and production build passed. The preview returned HTTP 200 after restart, and browser inspection showed SIGNED IN, the user's email, and Sign out. This was an observed SDK session, not proof of complete authentication or backend authorization coverage.

Email-confirmation enforcement remains unresolved: the user reported getting through without fully verifying their account. Confirm Email configuration, account confirmation state, and whether a confirmation link had already been followed were not verified. No authentication setting was changed by the assistant.

Fresh-account rejection before confirmation, wrong-password rejection, session persistence after refresh, and sign-out followed by refresh still require explicit verification against the configured project.

Integrated checkout: `node node_modules/typescript/bin/tsc --noEmit` passed; `node node_modules/vite/bin/vite.js build` passed (78 modules); `node --experimental-transform-types --test tests/finance.test.ts` passed all 15 tests. npm reported zero vulnerabilities during installation. The existing lockfile already matched package.json; installation-only peer metadata changes were discarded. `git diff --check` passed. These checks do not exercise hosted authentication or a physical device.

## Remaining milestones

The supplied v1 protocol and data model were read from the handoff archive; schemas are available there but not yet integrated. The unpublished food-only patch remains unapplied and unverified. SQL, authentication/ownership, idempotency/concurrency, device events, receipt/reset/care and hardware behavior need implementation and testing. Do not treat the draft as a completed backend.

The user's final Gator, Robot and Duck PNG sets are imported under assets/source. The canonical manifest generates 24 PNG sheets, indexed frames, RGB565 lookup palettes, optional direct RGB565 arrays and transparency masks. All 96 frames preserve source colors and binary transparency. Shared IDs are now gator/robot/duck; the old backend/firmware contract must be coordinated before device saving. Plaid and AI remain later milestones.

**Next single action:** verify Confirm Email and test a fresh account before email confirmation.

## Local letter-transfer extension

The website now includes savings goals and a final demo letter flow: review of the previously selected companion, immutable validated JSON snapshot, pack/seal/fly animation, local simulator application and receipts, replay/download, bounded history and recovery controls. Physical delivery and pairing remain unavailable. See `EGG_TRANSFER_HANDOFF.md` for the proposed contract and hardware questions. The initial setup screen now saves the pet and name; the letter step only reviews those choices. Accessories are not supported. No deployment or external authentication settings changed in this work.

Setup now includes a reviewed timezone and optional weekly USD budget. The demo letter v4 includes these preferences, with integer cents or null for no budget. Historical letters are preserved; legacy pet setups require selecting a current pet before saving. These fields do not yet implement check-in scheduling, budget calculations, care or reaction events.

## Character preview update

Fixed artwork unavailable: the previous interrupted implementation referenced generated files that did not exist. Generation now runs before development and production builds. Website playback uses generated PNG sheets with dimension checks, loading/error/retry states and integer 4× Canvas rendering. All three pet selectors are enabled; color and accessory controls are removed. Ayira’s name, timezone, budget and save controls surround the animated selector. Animation selections can replay; timed activities return to idle. Motion preference is respected initially and playback can be paused.

Changed files: assets/manifest.json, assets/source (24 original sheets), assets/README.md, tools/assets/build.mjs and build.test.mjs, public/assets/v1 generated files, firmware/generated header/report, src/character component/styles, src/main.tsx, package.json/lockfile, README and this status. Earlier local Supabase setup documentation changes are preserved.

Checks: asset generator passed; TypeScript check passed; production build passed (81 modules); all 28 asset tests and 15 finance tests passed. Asset tests compare source/web RGBA pixels, indexed output, masks and RGB565 arrays for all frames. Browser confirmed no artwork error, three enabled presets, robot and duck eating selection, and duck revive selection. Preview server restarted at http://127.0.0.1:5173/.

Generated data: indices 98,304 bytes, masks 12,288 bytes, palettes 62 bytes. These are data sizes only. No C++ compiler/board test was performed; byte order, flash partition, PSRAM/internal heap under TLS and physical display still need testing. Default animation timing is provisional because PNGs omit frame delays. Originals remain unchanged; harmless trailing bytes after PNG IEND are recorded and omitted from generated copies.

**Next single action:** review the three character previews and coordinate the new IDs with the hardware team before appearance API/device integration.

## Collaborator preview investigation

After fetching origin, main remained at `107ccae` and the active checkout's application source, package.json and lockfile had no content differences from origin/main. Signup and the financial demo are already in collaborator-authored commit `f851ccd`; the merged desktop handoff commit added documentation. There is no evidence of an unpushed application-source change in the current preview.

Corrected the setup guide to reuse the team Supabase project and explain Git updates, restarting the correct checkout, local-only URLs, ignored `.env.local`, and browser-local demo data. The collaborator's exact URL/checkout and missing behavior have not yet been supplied, so the cause on her machine remains unconfirmed.

### Connection milestone — deployment pending

Added owner-scoped cloud documents, revision-conflict handling, acknowledged setup saves and routing of returning users to Financial accounts. Supabase Auth supplies unique user IDs/session credentials; no shared account secret is generated. Guests remain local. Device registration returns a random token once, hashes it at rest, enforces one active device per user and supports revocation. Device state sync reads saved setup and normalized sample finance/goal state.

Both SQL migrations run successfully in PGlite with ownership/permission, retry/conflict, device-token, epoch and revocation assertions. All 48 application/database tests and 28 asset tests pass; frontend TypeScript, production build (95 modules) and Deno Edge Function checks pass. The local preview was restarted and guest setup remained available. Hosted migration/function deployment, two-browser acceptance and physical provisioning/display are still pending; no CLI deployment credentials were available. This does not complete the full v1 action/reaction/care protocol: nonempty device event outboxes currently receive 501 without consumption. See CONNECTION_SETUP.md for exact scope and deployment commands.

