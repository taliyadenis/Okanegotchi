# Okanegotchi

A retro pixel-pet companion designed to make checking your finances feel more approachable and personal.

The pet lives on a small physical handheld device and reacts to financial activity. The companion website helps users customize their pet, set savings goals, review spending, and understand what their pet is responding to.

## Project status

This repository is the starting point for the **companion website and its backend integration**. It currently contains project documentation and a `.gitignore`; the application has not been scaffolded yet.

The features and architecture below describe the planned MVP. A running website, deployed API, bank connection, and tested connection to the physical device are not yet available in this repository.

## The experience

- **A pet with personality:** choose Piggy, Cat, or Dragon, then customize its name, color palette, and accessory.
- **Financial reactions:** food expenses trigger an eating animation, rides trigger traveling, and savings contributions trigger a celebration.
- **Savings goals:** see progress toward a named goal alongside clear spending context.
- **Daily check-ins:** review a financial summary and confirm a check-in, with one target in the morning and one in the afternoon/evening.
- **Gentle care mechanics:** attention affects the pet's care state. A faded or ghost pet can be revived through a confirmed review without losing customization, savings, or history.

Spending money is never required to care for or revive the pet. Care reflects check-in activity, while budget status reflects financial facts; these are separate concepts.

## Planned website features

| Area | MVP scope |
| --- | --- |
| Account setup | Sign-in and owner-scoped access to a pet and device |
| Dashboard | Spending summary, savings goal, check-in progress, and device connection status |
| Pet creator | Three preset pets; mint, coral, or lavender palettes; no accessory, cap, or scarf |
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

These are the selections from the project handoff; dependencies and deployment configuration have not been added yet.

## How the website connects to the pet

1. A labeled demo scenario, or a future financial-data integration, supplies an event to the backend.
2. The backend updates financial state and queues a compact pet reaction.
3. The physical device requests updates over authenticated HTTPS and plays the animation locally.
4. The website displays the resulting state, its explanation, and data freshness.

The device normally checks for updates every 30 seconds. A temporary demo session permits 2-second polling for up to 15 minutes. Stable event IDs and acknowledgements prevent retries from creating duplicate financial effects or repeatedly starting the same reaction.

The pet keeps animating with its last known state when disconnected. The website may mirror the pet, but the device does not depend on an open browser.

The hardware team's planned platform is an ESP32-S3 with a 240 × 240 display, a PN532 NFC reader, and three buttons, using Arduino C++. NFC opens a financial review or triggers an explicitly enabled demo scenario; a tap does not make a payment or retrieve bank details.

## Development roadmap

- [ ] Scaffold the React/Vite/TypeScript application and local preview.
- [ ] Build the dashboard and pet creator against typed demo fixtures.
- [ ] Implement authentication, database migrations, and owner-scoped APIs.
- [ ] Verify one queued reaction with a software device simulator.
- [ ] Connect the physical device and verify financial reactions and appearance updates.
- [ ] Add review/check-in, goal, and ghost/revival flows.
- [ ] Rehearse a complete demo and deploy the companion website.

## Local development

There is no runnable application or development command yet. Installation steps, required environment variables, and preview commands will be added when the app is scaffolded.

Keep credentials out of source control. Future `.env.example` files should contain placeholder values only; privileged backend keys and financial-provider secrets must never be included in browser code or device firmware.

## Team coordination

The project has two hardware/firmware contributors and two website/integration contributors. Both pairs need to agree on API payloads, pet asset IDs, and changes that affect the device.

The supplied handoff's `MVP_BUILD_PLAN.md` and `mvp/DEVICE_PROTOCOL_V1.md` define the current MVP and supersede older drafts. Those reference files have not yet been imported into this repository. As implementation progresses, document what is working, how it was checked, and the next integration milestone here.
