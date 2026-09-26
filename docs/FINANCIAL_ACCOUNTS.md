# Financial accounts: integration handoff

## Delivered demo

Entry: Continue in demo mode → Financial accounts. Authenticated companion screens expose the same entry, with financial data still explicitly marked Demo data. Supabase configuration is not needed.

- `src/finance/types.ts`: normalized account, transaction, connection, snapshot, error and provider contracts. Amounts are integer USD cents. Transaction amounts are unsigned; kind determines their meaning. Transfers have no implied purchase/income direction; paired savings transfers share a reference.
- `fixtures.ts`: three fictional institutions, five accounts, 25 stable transactions, and a fixed September 26, 2026 clock. Snapshot balances are illustrative and are not derived from this partial recent history. No device events are dispatched.
- `provider.ts`: asynchronous demo provider. Revisions invalidate late operations after disconnect/reset/disposal. AbortSignal cancels a connection before mutation. UI serializes repeated actions. Read snapshots are defensive copies.
- `storage.ts`: validates versioned state and separates guest from user IDs. Only connection selections/status/timestamps are persisted; fixtures are reconstructed. Storage failures fall back to memory. This is not an authorization boundary.
- `FinanceScreen.tsx`: presentation, connection dialog, account filter, feedback and demo controls. Keying by identity remounts the feature and invalidates old provider work.

The provider clock stays fixed deliberately: refresh means a successful simulated fetch, not newly generated activity. One-shot failure switches reset after use; the demo reset clears them. Leaving the feature preserves saved accounts but clears transient controls. Unsupported/invalid saved state starts empty.

## Proposed real-provider boundary (not implemented)

The teammate owns Supabase project configuration. Keep the existing auth integration intact. Implement a backend financial provider only after consulting the current official Plaid documentation; the following describes intent, not finalized endpoint contracts.

1. An authenticated Edge Function authorizes the requesting owner and creates a Plaid Link token.
2. Frontend launches Link. The backend exchanges the public token, stores sensitive access tokens server-side, and associates the Item with the authenticated owner.
3. Backend returns normalized connections, accounts and transactions using the domain models. Add provider/source provenance for live records; never relabel demo records as real ones. Real linking needs a Link-specific initiation/completion flow in place of the demo account picker; it is not a drop-in API-key switch.
4. Postgres tables need owner-scoped RLS. Edge Functions must validate the session and ownership for every read/change. Never trust a client-supplied owner ID. Browser UI and localStorage do not enforce authorization.
5. Implement incremental transaction sync (including updates/removals and pending-to-posted replacement), verified webhooks, relink/update flows, real data freshness, error mapping, and disconnect/revocation semantics. Avoid repeat effects from repeated webhook delivery or refresh.
6. Map balances, null availability, account types, transaction signs, categories and transfer identities explicitly. Keep debt separate from cash. Expand currency support deliberately rather than summing currencies.
7. Keep Plaid secrets/access tokens and privileged Supabase keys out of VITE variables, browser bundles, browser storage and device firmware. Replace demo persistence with owner-scoped server storage for real data.
8. Verify in Plaid Sandbox before planning production access. Acceptance checks include Link cancellation, duplicate institution connections, expired authorization, partial sync failure, repeated webhook delivery, deleted transactions, user isolation and revocation.

Food, transportation and savings can later map to README pet reactions using stable source/event IDs and idempotent delivery. Refresh itself must not award care or replay reactions. Spending is never required for care or revival.

## Context and validation

`README.md` is the available product source. `MVP_BUILD_PLAN.md` and `mvp/DEVICE_PROTOCOL_V1.md` remain absent; this feature does not claim conformance with unseen device payloads.

`npm test` runs Node's built-in test runner with TypeScript transformation (use a recent Node 22 release or newer). Fifteen tests cover validation, duplicates, cancellation, persistence and isolation, corrupted/unavailable storage, failure/retry, disconnect/reset, stale asynchronous operations and money rules. `npm run build` checks application types and produces the Vite bundle.

Browser verification covered guest entry, connection of multiple accounts/institutions, filtering, reload persistence, refresh failure and retry, attention/reconnect, disconnect cancellation and completion, reset, keyboard operation, and narrow layout. Login/sign-up navigation was checked; live authentication/logout remains unverified without Supabase configuration. No real financial provider was contacted.

## Savings goals extension

`goals.ts` owns versioned goal persistence, integer-cent target parsing, and pure progress calculation; `SavingsGoalScreen.tsx` owns the create/edit/view/remove flow and illustrative egg preview. One goal is stored per demo identity separately from financial connections. Goal progress uses the complete linked savings balance. Missing connections show unavailable progress, while stale connections retain labeled snapshot progress. Removing a goal never changes financial accounts.

The $50 contribution button is visual-only and never mutates provider balances, creates transactions, queues hardware events, or awards care. Real contribution tracking needs a product decision on baseline/allocation and transaction provenance before implementing server-side goal updates and idempotent device reactions. The displayed egg and celebration are concepts, not agreed firmware assets or payloads.
