# Okanegachi: Cursor firmware execution pack

Prepared 2026-09-26. This is an implementation brief and executable acceptance harness, **not completed firmware**. No GitHub repository is created by unpacking it. No board, live API, or production TLS path was tested while preparing this pack.

**Revision2:** explicit modular asset engine/backgrounds/clothes/animation playback and mandatory NFC demo stickers. Start with docs/MODULAR_ASSETS.md, docs/ANIMATION_ENGINE.md, docs/ADDING_CONTENT.md and docs/NFC_DEMO_STICKERS.md. Includes placeholder catalog/schema, animation samples and byte/routing fixtures. If Cursor already started revision1, use UPDATE_PROMPT.md to apply changes without restarting or replacing its code.

## Start

1. Extract the ZIP. In Cursor open your existing team repository if available; otherwise open a new empty project folder and copy the contents of this pack into it. In an existing repository merge instructions and files deliberately; do not overwrite existing AGENTS.md, rules, source or secrets. Place this pack's files at the repository root (not inside an extra nested folder), or adjust paths once.
2. Use Cursor Agent mode with terminal access. Sign into GitHub CLI (`gh auth login`) yourself if you want unattended repository creation. No bank, Supabase, AI or paid-service API key is needed for offline development. Toolchain/dependency downloads and GitHub publishing require Internet access.
3. Paste **MASTER_PROMPT.md** as the task. It explicitly authorizes implementation, isolated subagents, tests, private GitHub repository creation and pushes of reviewed project files. Runtime approval prompts, account login, usage limits and hardware cannot be bypassed by this text.
4. The agent should leave working source, build commands, test reports, a GitHub URL when publishing succeeds, and a short bench checklist. If interrupted, paste RESUME_PROMPT.md; it resumes from files rather than recreating the project.

## Included

- MASTER_PROMPT.md: complete long-running instruction; WORK_PLAN.md: stages and ownership.
- FIRMWARE_SPEC.md: hardware, app, transport, storage, onboarding, assets and build requirements.
- ACCEPTANCE.md: mandatory behavioral tests and separate physical gates.
- TEST_DRIVER_CONTRACT.md + tools/run_acceptance.py + tests/cases.json: black-box C++ driver acceptance interface and executable runner. Driver implementation is Cursor's task and must invoke production C++ modules.
- tests/support/fakes.hpp and harness_smoke.cpp: host-buildable deterministic clock, store-failure and transport-recording helpers. These are test infrastructure, not firmware.
- tools/check_contract.py + contract_cases.json: full Draft 2020-12 schema validator and valid/invalid fixture cases.
- tools/check_pack.py: package integrity/structure checks; tools/run_acceptance.py --self-test checks the runner itself, not firmware.
- .cursor/agents/: four specialist definitions; .cursor/rules/: project guidance.
- mvp/: unchanged v1 device protocol, schemas and example JSON copied from the team contract.
- hardware/firmware_pins.h + HARDWARE.md: current pins and critical assumptions.
- docs/: asset contract, website integration checklist, verification report and durable execution status.
- assets/catalog.example.json and catalog.schema.json: extensible placeholder character/clip/background/clothing registries. tools/check_assets.py checks semantic references; final converter/player implementation remains Cursor's task.
- tests/nfc/:24 byte fixtures plus13 routing cases, four tag-memory hex examples. tools/nfc_records.py builds/tests reference bytes without touching hardware.
- tests/animation/samples.json:22 loop/once frame-selection cases. tools/run_content_acceptance.py will run the real C++ content driver Cursor builds.
- UPDATE_PROMPT.md: apply this revision to an already-running implementation without restarting it.

## No service credentials required to build

The firmware still implements an **API client**. Offline fake transports exercise the same serializer/parser/state engine without cloud accounts. Real Wi-Fi + website integration later requires the website team's deployed sync endpoint and an opaque device token. Device registration is performed by the companion/backend, not by embedding a Supabase administrative key in firmware.

Run `python tools/run_acceptance.py --self-test` now. Run `python tools/check_contract.py` after installing `requirements-dev.txt`. Cursor must build its C++ JSONL test driver before `python tools/run_acceptance.py --driver build/firmware_test_driver` can validate firmware.

Do not label passing package checks or test-support smoke tests as passing firmware tests. Read VALIDATION.md for what was actually run for this delivery.
