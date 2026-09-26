# Implementation prompt: demo financial account connections

Implement the financial account connection portion of Okanegotchi using deterministic dummy data. Deliver a working, polished flow in the existing app, with a clear integration boundary for a later Plaid implementation. A teammate owns connecting the existing Supabase project; this task must work without Supabase configuration or real bank access.

## Read the project context first

- `README.md`: product purpose, MVP scope, planned stack, financial reaction rules, demo behavior, and team coordination.
- `src/main.tsx`: current login/sign-up flow, session handling, demo entry point, and character-setup placeholder.
- `src/auth.ts`, `.env.example`, and `package.json`: authentication boundary, configuration, dependencies, and build commands.
- `src/styles.css`: existing visual language and responsive behavior.
- Follow any applicable `AGENTS.md` instructions.
- The README references `MVP_BUILD_PLAN.md` and `mvp/DEVICE_PROTOCOL_V1.md` as authoritative handoff documents that supersede older drafts. They were absent when this prompt was written. Look for them in the workspace; read them if now available. Otherwise explicitly note their absence, proceed from the README, and do not invent device payloads or claim compliance with unseen documents.

Inspect the working tree before editing and preserve existing work. This request authorizes implementation of the demo feature, not real financial connections or deployment.

## Product goals and scope

Okanegotchi is a friendly companion for financial awareness. The physical pet reacts to financial activity, while the website helps people understand their finances and their pet. For this task, build the account-connection experience and a focused account overview with recent transactions. Do not expand into a complete dashboard, pet editor, banking backend, or hardware integration.

Keep the existing login/sign-up and session behavior intact. Do not require credentials, change Supabase settings, or block progress on the teammate's setup. Add a clear “Financial accounts” entry from the existing companion/setup screen, accessible in both demo mode and an authenticated session. Keep navigation back to the companion screen. A signed-in user must still see that these financial accounts are simulated.

## User experience

1. **Initial account screen**
   - Start with an empty state and a prominent “Connect a demo account” action.
   - Explain simply: “Try sample accounts to see how Okanegotchi works. No real bank is connected.”
   - Keep a visible “Demo data” indicator throughout the feature.
   - Offer a way to skip or return; connecting an account is optional.

2. **Simulated connection flow**
   - Open an accessible dialog or clearly structured step-by-step screen.
   - Let users search and choose among three fictional institutions, such as Clover Bank, Pocket Credit Union, and Sunny Savings. Avoid implying partnerships with real banks or Plaid.
   - Show a short explanation of the sample balances and transactions the demo will display.
   - Let users select one or more available sample accounts, with names, types, and masked account numbers.
   - Require at least one account selection before continuing.
   - Provide back, cancel, loading, success, and recoverable failure states. Cancel must not create a connection or leave half-saved state.
   - Never ask for real bank credentials, account numbers, API keys, or personal financial information.
   - Use “Connect demo accounts” for the final action and an honest confirmation such as “Your sample accounts are ready.”

3. **Connected account overview**
   - Group accounts by institution. Show account name, checking/savings/credit type, masked number, currency, current balance, available balance when provided, connection state, and last simulated refresh time.
   - Label credit card balances as amounts owed. Show cash balances and credit owed separately; do not add debts to a positive cash total.
   - Use currency formatting and integer minor units for calculations. Represent unavailable balances as unavailable, not zero. Keep this demo in USD.
   - Include recent transactions with merchant/description, date, amount, category, pending/posted status, and account association. Allow filtering by connected account.
   - Distinguish money in, money out, refunds, and transfers. Pending transactions must not silently count as posted spending. Transfers between sample accounts must not count as purchases or income.
   - Provide “Add demo account,” “Refresh demo data,” and “Disconnect” actions. Explain that disconnect removes the institution's sample accounts and transactions from this demo. Allow canceling the confirmation.
   - Prevent duplicate connections and accounts. Reconnecting the same institution should expose only accounts not already connected or clearly report that they are already connected.

4. **Recoverable demo states**
   - Add a compact, clearly labeled demo controls area for simulating refresh failure and a connection requiring attention.
   - Provide retry/reconnect flows. Failed refreshes retain the last successful data and timestamp, with an explicit stale-data message.
   - Refresh must not duplicate transactions or invent newly updated balances on every click.
   - Provide “Reset demo data” with an explanation and confirmation, returning to the empty state.

## Dummy data and persistence

Create typed fixtures for three institutions and at least five accounts across checking, savings, and credit. Include approximately 20–30 plausible fictional transactions with stable IDs. Cover food, transportation, income, a savings transfer, a refund, and a pending transaction. Include an unavailable available-balance value so that state is exercised.

Use a fixed dataset and an injectable demo clock or explicit reference date. Do not use random balances, random IDs, or changing transaction dates that make tests unreliable. Model paired transfers with a shared transfer reference so a later summary can avoid double counting.

Persist only demo selections and simulated state in versioned, namespaced localStorage. Recover safely from malformed or obsolete storage, and continue in memory if browser storage is unavailable. Keep guest demo state separate from signed-in users' demo state, and clear in-memory state when the active identity changes so data does not flash across users. This is demo organization, not a substitute for backend authorization. Do not store passwords, real bank data, or provider tokens.

## Architecture and future Plaid integration

Keep domain types, fixtures, provider logic, persistence, and UI components separate. Avoid adding the entire feature to `src/main.tsx`; make only the navigation and composition changes needed there. Prefer the existing stack and avoid unnecessary dependencies.

Define a small asynchronous financial-account provider interface that supports listing connections/accounts/transactions, connecting selected demo accounts, refreshing a connection, reconnecting, and disconnecting. Use explicit typed statuses and errors. Have the UI consume normalized application models rather than provider-specific response payloads. Implement only the demo provider now. Handle rapid repeated actions and stale asynchronous results, including a refresh completing after disconnect/reset or an identity change.

Write a short future-integration handoff explaining where a Plaid-backed provider would replace the demo provider. Real Plaid integration will require a backend and authentication, not merely a frontend API key. Treat the following as a proposed boundary to verify against official Plaid documentation when implementing it:

- Authenticated backend creates a Link token; the frontend launches Plaid Link.
- Backend exchanges the resulting public token and stores sensitive access tokens server-side.
- Backend syncs and normalizes accounts/transactions, handles provider updates and removals, and exposes owner-scoped data to the frontend.
- Supabase Edge Functions and Postgres are the planned backend; the teammate will configure Supabase. Future tables require owner-scoped authorization/RLS and secure server-side secrets.
- Later implementation must address update/relink flows, webhook verification, incremental transaction sync, pending-to-posted changes, disconnect/revocation, and Sandbox acceptance tests.
- Never put Plaid secrets or access tokens in `VITE_*` variables, browser storage, frontend bundles, or firmware. Do not add fake production endpoints or unused Plaid dependencies now.

Keep stable event IDs and provenance in the data model for future pet integration. Document that food, rides, and savings may later map to the reactions described in the README. Do not dispatch device commands, award care, or emit repeated reactions when transactions are refreshed. Spending is never required to care for or revive the pet; financial state and care state remain separate.

## Visual design and accessibility

Extend the current playful design: yellow background, pink panels, blue primary actions, green accents, bold typography, dark borders, and offset shadows. Match existing spacing and controls while giving account and transaction information a clear hierarchy. Use friendly, concise copy and avoid financial advice or guilt-based messaging.

Support phone and desktop layouts without horizontal overflow. Use semantic labels, visible keyboard focus, meaningful status announcements, and readable contrast. Dialogs must manage focus, support Escape/cancel when appropriate, and return focus to their trigger. Do not convey connection status through color alone. Respect reduced-motion preferences.

## Verification and deliverables

Implement the feature, run `npm run build`, and add focused tests for meaningful behavior: duplicate prevention; cancel without mutation; disconnect/reset; corrupted/unavailable storage; identity isolation; retry retaining old data; refresh idempotency; and ignoring late results after reset/disconnect. Verify money calculations and transfer/pending exclusions wherever summaries are shown.

Exercise the user flow in the browser: enter demo mode without environment variables, navigate to financial accounts, connect multiple accounts, inspect/filter transactions, reload to confirm persistence, add another institution, simulate a refresh failure, retry, disconnect, and reset. Check keyboard navigation and a narrow viewport. Verify that login, sign-up, logout, and companion navigation remain intact; do not claim live authentication was tested without a configured project.

Update `README.md` with the actual entry point, supported demo behavior, storage/reset behavior, testing results, and remaining integration work. Add a concise financial-account integration handoff document for the teammate. Clearly separate verified demo behavior from unimplemented real banking features.

Finish with a summary of what changed, how to access it in the sidebar preview, which checks passed, and any concrete limitations. Keep the working preview available. Do not deploy, connect real banks, or modify the teammate's Supabase configuration.
