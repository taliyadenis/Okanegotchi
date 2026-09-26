# Supabase account setup

1. Create a Supabase project at https://supabase.com/dashboard. Keep the database password private.
2. Enable the Email provider under Authentication and keep Confirm Email enabled.
3. Under Authentication → URL Configuration, set the local Site URL to `http://127.0.0.1:5173/` and allow the same Redirect URL. Add deployment URLs separately when deployment is authorized.
4. Copy the Project URL and publishable key from the project's Connect dialog.
5. Copy `.env.example` to `.env.local` in the app root and fill in both values. Never put a service-role/secret key in a `VITE_*` variable: those variables are exposed in the browser bundle. `.env.local` is ignored by Git.
6. Run `npm ci` and `npm run dev`. Restart Vite after changing environment variables.

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
