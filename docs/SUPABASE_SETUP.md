# Supabase account setup

For the new cloud persistence and device connection code, also follow [CONNECTION_SETUP.md](CONNECTION_SETUP.md). The account document/device migrations and Edge Functions must be deployed before signed-in cloud loading and saving can work. Browser URL/publishable-key configuration alone is insufficient. Guest mode remains available.

1. For this team, use the existing Okanegotchi Supabase project. Ask the project owner for its Project URL and publishable key; do not create a separate project just to run a teammate's checkout. Only create a new project at https://supabase.com/dashboard when intentionally setting up a separate environment. Keep database passwords private.
2. Enable the Email provider under Authentication and keep Confirm Email enabled.
3. Under Authentication → URL Configuration, set the local Site URL to `http://127.0.0.1:5173/` and allow the same Redirect URL. Add deployment URLs separately when deployment is authorized.
4. Copy the Project URL and publishable key from the project's Connect dialog.
5. Copy `.env.example` to `.env.local` in the app root and fill in both values. Never put a service-role/secret key in a `VITE_*` variable: those variables are exposed in the browser bundle. `.env.local` is ignored by Git.
6. Run `npm ci` and `npm run dev`. Restart Vite after changing environment variables.

## Refresh a collaborator's local preview

From the actual Okanegotchi Git clone, run `git status` and preserve any local work before switching branches. Then update main:

```sh
git fetch origin
git switch main
git pull --ff-only origin main
npm ci
npm run dev
```

If Git reports local changes or divergent history, stop and reconcile that work rather than resetting or overwriting it. Stop any older preview first so the browser does not keep showing another checkout on port 5173. Use the exact URL printed by the newly started server, then refresh the browser.

`http://127.0.0.1:5173/` points to each person's own computer. Sharing that address does not share a running preview. A downloaded ZIP/snapshot does not update when GitHub changes, and a hosted website changes only after its separate deployment workflow runs.

The repository includes signup and **Continue in demo mode → Financial accounts**. The financial demo works without Supabase configuration. Actual signup/login requires each checkout's own ignored `.env.local`; Git never transfers that file. Use the same team's Project URL and publishable key to access the same Auth project. Demo account selections are stored per browser and do not sync between collaborators.

## Account verification

Create a fresh test account with your own email. With Confirm Email enabled, signup should show a confirmation notice and should not issue a signed-in session. Attempt login before confirmation and verify rejection. Follow the confirmation email and log in. Refresh to check session restoration, then sign out and refresh again. Also test a wrong password.

Seeing SIGNED IN establishes that the UI has a Supabase session; it does not establish that all confirmation and authorization checks passed. If Confirm Email was disabled, Supabase may already have automatically confirmed the account. Use a fresh account when testing enforcement after changing that setting.

The SDK manages access/refresh tokens and persistent browser sessions. Passwords are sent to the configured Supabase Auth service and cleared from React state after submission. Demo navigation does not create a session. Future companion APIs must verify JWTs and derive ownership server-side; browser session checks are presentation only.

## Integration boundary

Signup does not yet provision a saved pet or device. The financial-account screen uses synthetic local demo data, not a deployed bank or companion API. Device sync, goals, appearance persistence, registration/revocation and authenticated demo reactions still need backend implementation and tests. The old food-only patch remains an unverified draft and must be reviewed before reuse. Plaid and AI remain later milestones.

References:

- https://supabase.com/docs/guides/getting-started/quickstarts/reactjs
- https://supabase.com/docs/guides/getting-started/api-keys
- https://supabase.com/docs/guides/auth/general-configuration
