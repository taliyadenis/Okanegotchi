# Firmware lane

Read `README.md`, `../docs/firmware/INTEGRATION.md` and `../docs/firmware/BUILD_AND_TEST.md` before changing runtime behavior. The current implementation is this folder, not the old hardware branch's app stub. Historical planning files are explicitly marked.

- The pet lives on the physical display. Current authored presets are gator, robot and duck in original colors, with no clothing. Reuse the shared `../assets` manifest and generated header; never replace designer pixels with placeholders by running the experimental catalog compiler over production assets.
- `../mvp/*.schema.json` is the wire contract. Regenerate checks after a coordinated schema change. Do not silently translate the browser-local demo-letter packet into device v1 or invent a deployed backend.
- Preserve the verified GPIO plan in `src/board.hpp`. Both peripherals use SPI. One loop task owns the bus; the HTTP worker owns networking. Do not access SPI, mutate Engine, or access NVS from the network worker.
- Keep LOCAL DEMO separate from cloud configuration, NVS, credentials and claims. No insecure TLS fallback or real API keys in source/logs. USB setup is the current provisioning path.
- Preserve durable event IDs, save-before-start command cursor, receipt-based check-in, queue/body bounds, and reader-error versus confirmed-absence distinction.
- Run the relevant native tests and both target builds for runtime changes. Generated source must remain reproducible. Do not describe a cross-compile or fake SPI transcript as a powered-board, RF or deployed-backend test.
- Keep web implementation files with the website team. Update the firmware status file and integration contract when behavior changes. Never force-push shared branches.
