# Plan and ownership

These stages are execution order, not promises of elapsed time.

| Stage | Owner | Exit condition |
|---|---|---|
| 0: inspect/build interfaces | Parent | Existing code mapped; pins/profile isolated; portable types/interfaces committed; safe Git baseline |
| 1A: state and inputs | firmware-core | Button/UI/game/reaction/outbox/review tests pass with fake clock/store |
| 1B: device adapters/assets | firmware-io | SPI/display/NFC/piezo, asset converter/registry/compositor, bounded NDEF reader cross-compile; hardware tests pending |
| 1C: protocol/connectivity | firmware-network | Real codec, fake transport cases, HTTPS adapter, provisioning/NVS compile |
| 2: integration | Parent | Main loop/tasks wired; driver runs real core; no duplicate ownership of state/SPI/persistence |
| 3: independent review | firmware-verifier | Acceptance/failure tests run; findings fixed; separate build modes verified |
| 4: publish/handoff | Parent | Private repo/PR where authorized/authenticated; accurate report and bench steps |

Parent creates and owns `firmware/src/shared/` types, `firmware/src/app/` orchestration, build files, `tests/driver/`, Git integration and docs/EXECUTION_STATUS.md. Freeze contracts for Clock, DurableStore, Transport, InputEvents, RenderModel, AssetProvider, NfcEvents and SyncResult. Headers state queue ownership and lifetimes. Implementers propose interface changes back to parent, not silently edit each other's files.

Revision2 ownership: core implements pure AnimationPlayer/ReactionScheduler and DemoTagRouter/presentation latch/pending-intent logic. IO implements AssetRegistry/SceneComposer/DisplayAdapter, catalog converter/preview and bounded Type2/NDEF reader/parser. Network supplies validated demo freshness/health and unchanged demo_trigger transport. Parent integrates registry interfaces, schemas and both test-driver suites. If IO becomes bottleneck, complete converter then adapters sequentially or delegate an additional isolated asset subtask within available slots; never concurrent editing of its files.

Suggested module ownership: core owns `firmware/src/core/` + tests/core; IO owns `firmware/src/hal/` + rendering + tests/io; network owns `firmware/src/network/` and provisioning/storage adapters + tests/network. Parent resolves exact storage/core ownership before dispatch. Verifier initially read-only; test additions assigned a separate path/worktree. Run only useful parallel tasks with explicit handoffs.

Do not wait for final art. A state label, colored block and geometric overlay prove event flow. First integrated slice: fixture food command -> real parser -> core -> placeholder eating -> one persisted cursor -> return idle. Then ride ordering, offline responsiveness, NFC review, server-confirmed check-in/revival, customization, game and provisioning polish.

Use one canonical repo for both team pairs when one exists. Website code is out of this task's scope. This run's explicit authorization permits Git creation/push/PR; it does not authorize team-main merge, adding collaborators or deploying financial infrastructure.
