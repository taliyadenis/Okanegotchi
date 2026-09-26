# Okanegotchi project status

Updated September 26, 2026. Integration branch: `codex/supabase-auth-handoff`, based on remote main `f851ccd6c494e23dce5824cea55486c01fa139b8`. No deployment performed.

## Current implementation

The latest remote app already includes Supabase signup/login, confirmation messaging, session restoration, sign-out, and a financial-account demo. Those newer implementations are preserved. The older desktop snapshot's overlapping authentication code was not copied over them.

The user created a Supabase project and configured its URL and publishable key in the older local preview's ignored `.env.local`. No service-role key was supplied or added to browser source. The new checkout needs its own local configuration; no environment values are included in this commit.

The financial-account screen is a local synthetic demo. Character customization is still a placeholder; signup does not provision a pet/profile/device. No owner-scoped companion API or device sync backend is deployed.

## Changes in this integration

- Add `docs/SUPABASE_SETUP.md` with local configuration, confirmation/session checks and backend boundaries.
- Add this current status report and link both documents from README.
- Preserve existing frontend, financial demo, dependency versions, and teammate changes from main.

## Verification

On the older desktop snapshot, TypeScript and production build passed. The preview returned HTTP 200 after restart, and browser inspection showed SIGNED IN, the user's email, and Sign out. This was an observed SDK session, not proof of complete authentication or backend authorization coverage.

Email-confirmation enforcement remains unresolved: the user reported getting through without fully verifying their account. Confirm Email configuration, account confirmation state, and whether a confirmation link had already been followed were not verified. No authentication setting was changed by the assistant.

Fresh-account rejection before confirmation, wrong-password rejection, session persistence after refresh, and sign-out followed by refresh still require explicit verification against the configured project.

Integrated checkout: `node node_modules/typescript/bin/tsc --noEmit` passed; `node node_modules/vite/bin/vite.js build` passed (78 modules); `node --experimental-transform-types --test tests/finance.test.ts` passed all 15 tests. npm reported zero vulnerabilities during installation. The existing lockfile already matched package.json; installation-only peer metadata changes were discarded. `git diff --check` passed. These checks do not exercise hosted authentication or a physical device.

## Remaining milestones

The supplied v1 protocol and data model were read from the handoff archive; schemas are available there but not yet integrated. The unpublished food-only patch remains unapplied and unverified. SQL, authentication/ownership, idempotency/concurrency, device events, receipt/reset/care and hardware behavior need implementation and testing. Do not treat the draft as a completed backend.

Awaiting the user's selected 32×32 sprite export. The earlier gator test in Downloads was explicitly excluded. Shared asset generation and physical rendering remain untested. Existing appearance IDs remain piggy/cat/dragon pending hardware-team agreement on the gator. Plaid and AI remain later milestones.

**Next single action:** verify Confirm Email and test a fresh account before email confirmation.
