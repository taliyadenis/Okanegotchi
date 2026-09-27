# Okanegotchi

<img width="380" height="188" alt="Image" src="https://github.com/user-attachments/assets/3c5f443f-89be-4cef-9c81-d24853284e95" />

A retro pixel-pet companion designed to make checking your finances feel more approachable and personal.

The pet lives on a small physical handheld device and reacts to financial activity. The companion website helps users customize their pet, set savings goals, review spending, and understand what their pet is responding to.

## Project status

This repository contains the **companion website**, scaffolded with React, Vite, TypeScript, and Tailwind CSS. The first screens provide login, sign-up, and a character selector with animated Gator, Robot, and Duck previews.

The features and architecture below describe the planned MVP. Supabase authentication is implemented but requires project configuration and live verification. Demo mode remains available without configuration. Companion names, pet choices, timezone and optional budgets save locally per demo identity. Fictional financial accounts, savings goals and demo egg-letter delivery are implemented. A deployed API, real bank connections, and physical-device integration remain future work.

## The experience

- **A pet with personality:** preview Gator, Robot, or Duck in their original colors. Color and accessory customization have been removed from the MVP.

- **Financial reactions:** food expenses trigger an eating animation, rides trigger traveling, and savings contributions trigger a celebration.
- **Savings goals:** see progress toward a named goal alongside clear spending context.
- **Gentle care mechanics:** attention affects the pet's care state. A faded or ghost pet can be revived through a confirmed review without losing customization, savings, or history.

Spending money is never required to care for or revive the pet. Care reflects check-in activity, while budget status reflects financial facts; these are separate concepts.

## Planned website features

| Area | MVP scope |
| --- | --- |
| Account setup | Sign-in and owner-scoped access to a pet and device |
| Dashboard | Spending summary, savings goal, check-in progress, and device connection status |
| Pet selector | Gator, Robot, or Duck; eight animation previews with original artwork |

| Pet preview | A pixel-art preview using the same asset IDs as the device |
| Goals and preferences | Savings target, optional weekly budget, and timezone |
| Demo controls | Clearly labeled food, ride, savings, and accelerated care scenarios |
| Integration simulator | A software client for testing the device API before physical integration |

Arbitrary pixel-art uploads, AI-generated financial advice, and live production banking are outside the initial MVP. Deterministic demo data comes first; Plaid Sandbox is an optional later integration.

## Planned technology stack

| Layer | Technology |
| --- | --- |
| Frontend | React, Vite, and TypeScript |
| Styling and UI | Tailwind CSS and shadcn/ui |
| Pet preview | HTML Canvas with shared pixel-art assets |
| Authentication and database | Supabase Auth and Postgres |
| Backend API | Supabase Edge Functions |
| Website hosting | Cloudflare Pages |
| Optional financial data | Plaid Sandbox |

These are the selections from the project handoff. React, Vite, TypeScript, and Tailwind are installed; the remaining services and deployment configuration are future work.

## How the website connects to the pet

1. A labeled demo scenario, or a future financial-data integration, supplies an event to the backend.
2. The backend updates financial state and queues a compact pet reaction.
3. The physical device requests updates over authenticated HTTPS and plays the animation locally.
4. The website displays the resulting state, its explanation, and data freshness.

The device normally checks for updates every 30 seconds. A temporary demo session permits 2-second polling for up to 15 minutes. Stable event IDs and acknowledgements prevent retries from creating duplicate financial effects or repeatedly starting the same reaction.

The pet keeps animating with its last known state when disconnected. The website may mirror the pet, but the device does not depend on an open browser.

The hardware team's planned platform is an ESP32-S3 with a 240 × 240 display, a PN532 NFC reader, and three buttons, using Arduino C++. NFC opens a financial review or triggers an explicitly enabled demo scenario; a tap does not make a payment or retrieve bank details.

## Local development

Use Node.js 22 or newer and npm. From the repository folder:

```sh
npm install
npm run dev
```

Open the local URL printed by Vite (normally http://127.0.0.1:5173). Saved source changes update the preview automatically.

```sh
npm run build    # Type-check and produce the production bundle in dist/
npm run preview  # Serve that bundle locally
```

Demo mode requires no environment variables. To connect your existing Supabase project:

1. Copy `.env.example` to `.env.local`.
2. Set `VITE_SUPABASE_URL` to the project URL and `VITE_SUPABASE_PUBLISHABLE_KEY` to its publishable key (a legacy anon key also works). Never use a secret or service-role key.
3. Enable the email/password provider in Supabase Authentication. Set its minimum password length to at least 8, matching the form.
4. Add `http://127.0.0.1:5173` to Authentication URL Configuration's allowed redirect URLs. Add your production origin before deploying and set the production Site URL there.
5. Restart `npm run dev` after changing environment variables.

Sign-up supports email confirmation: when confirmation is enabled, the UI asks the user to check their inbox; when Supabase returns a session immediately, it opens the companion screen. Existing accounts can log in with email and password. Sessions are restored after refresh, and Log out ends the current browser session. Supabase manages session tokens; the app does not store passwords. Demo navigation never creates an authenticated session.

Live acceptance checks (require a configured project and a test inbox): create an account, follow its confirmation link, log out, reject an incorrect password, log in successfully, refresh and retain the session, then log out and refresh to verify the login screen. Also verify expired confirmation links and the project's rate limits. Until those checks are performed, successful real authentication is unverified.

Choose **Continue in demo mode** to preview Gator, Robot, and Duck without an account. Select a pet and try its eight animations, or pause playback. Save your companion name, selected pet, timezone and optional budget to continue to demo accounts. Setup is stored locally and reused by the demo letter; it does not update a physical device. Future owner-scoped database tables and APIs must enforce authorization with RLS/server checks; the companion screen is not a data-access security boundary.

The static handheld illustration on the login screen remains decorative concept art. The character selector uses the user's 32×32 sprites at 4× nearest-neighbor scale. See [the shared asset handoff](assets/README.md) for generated PNGs, indexed frames, RGB565 palettes/arrays and masks. Run `npm run test:assets` for pixel-level checks. Assets regenerate automatically with `npm run dev` and `npm run build`; physical-device rendering remains unverified.

Keep credentials out of source control. Future `.env.example` files should contain placeholder values only; privileged backend keys and financial-provider secrets must never be included in browser code or device firmware.

## Team coordination

See [current project status](docs/PROJECT_STATUS.md) for verified behavior and remaining checks, and [Supabase setup](docs/SUPABASE_SETUP.md) to configure and test website accounts locally.

The project has two hardware/firmware contributors and two website/integration contributors. Both pairs need to agree on API payloads, pet asset IDs, and changes that affect the device.

The supplied handoff's `MVP_BUILD_PLAN.md` and `mvp/DEVICE_PROTOCOL_V1.md` define the current MVP and supersede older drafts. Those reference files have not yet been imported into this repository. As implementation progresses, document what is working, how it was checked, and the next integration milestone here.

## Financial account demo

Choose **Continue in demo mode → Save & continue to accounts** on the companion screen. The same entry is available after sign-in, and all financial data stays explicitly labeled **Demo data**. No Supabase configuration or real bank access is required.

Connect accounts from three fictional institutions, review cash and credit balances separately, filter 25 sample transactions by account, and refresh or disconnect institutions. The demo controls simulate connection failures, failed refreshes, and attention/reconnect states. Failed refreshes retain the previous snapshot. **Reset demo data** removes all saved demo connections after confirmation.

Selections and simulated statuses are saved under versioned `okanegotchi:finance-demo:v1:` localStorage keys, separately for guests and signed-in identities. No passwords, real bank information or tokens are saved by this feature. Invalid storage starts empty; unavailable storage uses memory. Demo dates and balances are fixed; refresh does not generate new transactions. The guest navigation returns to login on reload, but saved financial accounts reappear when reopening the feature.

Run `npm test` for financial, savings-goal and demo-letter tests and `npm run build` for the production build. Browser checks covered connection, filtering, reload persistence, retry/reconnect, cancellation, disconnect/reset, and keyboard/narrow layout. Live authentication and live financial connections remain unverified. See [the financial integration handoff](docs/FINANCIAL_ACCOUNTS.md) for implementation boundaries and the teammate's future Plaid/Supabase work.
## Savings goal demo

From **Financial accounts**, select **Your savings goal**. Connect a savings account first if needed. Create one named goal per demo identity, choose a positive USD target, and link a connected savings account. Progress uses the account's entire current balance, not contributions since goal creation; it does not reserve or transfer money. This tracking rule is an initial demo choice for the team to review.

Goals can be edited or removed and use a separate versioned `okanegotchi:savings-goal:v1:` localStorage key. Removing/resetting financial connections preserves the named goal and shows unavailable progress until its savings account is reconnected or replaced. Stale account data is explicitly identified. With unavailable browser storage, goal state lasts only while the goal screen stays mounted.

The egg illustration is a proposed on-screen concept. **Preview a $50 savings celebration** changes only the illustration and message; it neither changes the goal/account nor sends hardware events. The preview respects reduced-motion settings. Future real savings events and device presentation need agreement with the hardware team and the missing device protocol. Care continues to depend on check-ins, not contributions.

Savings-goal validation covers amount parsing, goal progress/completion, missing accounts, persistence isolation and invalid storage. Production build passes. Browser checks verified the savings prerequisite, invalid target handling, create/edit, 76% progress, completed target, persistence after reload, and the celebration with unchanged balances. Narrow sidebar layout has no horizontal overflow. Live Supabase, Plaid, and egg communication remain unimplemented or unverified.

## Send a little letter to your egg

After your saved savings goal, choose **Continue to your egg**. Review your previously saved companion and the letter, and press **Pack & send to demo egg**. The paper folds into an envelope, seals and flies away; a separate simulator validation/application step determines the receipt. The screen supports demo offline queueing, reconnect, failures, unknown receipt status and retry with the same frozen letter. History and companion selection are saved per identity. JSON download and animation replay never submit a letter.

**Physical pairing is unavailable.** The packet schema and appearance identifiers are proposals pending the missing firmware contract. No real device or delivery backend is connected. See [Egg transfer handoff](docs/EGG_TRANSFER_HANDOFF.md) for schema limits, simulator behavior, cancellation/persistence rules and hardware-team requirements. Savings, transactions and care are unchanged by sending. Companion choices are saved on the initial setup screen; the letter screen reuses them. Accessories are not supported.

Setup includes a reviewed timezone and optional weekly USD budget. The demo letter v4 includes these preferences, with integer cents or null for no budget. Historical letters retain their original pet IDs. Legacy Piggy/Cat/Dragon setups show Gator as the default for review before saving again; their names remain intact. These fields do not yet implement check-in scheduling, budget calculations, care or reaction events.

Combined integration validation: TypeScript, the production build, 38 application tests and 28 asset tests pass. The browser shows the animated selector alongside companion name, timezone, budget and Save & continue. Physical delivery remains unverified.

**Connection milestone (local implementation, deployment pending):** signed-in accounts load and save setup, sample-account selections, goals and demo-letter history through owner-protected Supabase storage. Returning users open Financial accounts after completed cloud setup. Device registration/revocation and authenticated state-sync endpoints are implemented; physical rendering and device actions remain unverified/unimplemented. See [connection setup and deployment](docs/CONNECTION_SETUP.md). Earlier local-storage descriptions below describe guest mode and the previous demo implementation.

