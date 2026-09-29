# Using the firmware package

For ongoing team work, use review branch `codex/firmware-usb-bringup-noel` and its PR rather than re-importing the older ZIP. The ZIP/patch instructions below describe the original pre-board handoff and do not include the newer USB bench diagnostics or measured results. Website/backend reviewers should start at `INTEGRATION.md` and `../../team-workflow/status/firmware.md`.

1. Keep the team's current repository. Create a hardware feature branch from current main. Do not replace the website folder with an older project snapshot.
2. Prefer applying the accompanying Git patch with `git am <path-to-Okanegachi-Firmware-Implementation.patch>`. It adds the firmware implementation, v1 contracts and documentation; the original shared art already on main is not replaced. Resolve any genuine same-file team changes before proceeding; do not force an overwrite.
3. Alternatively, extract the source ZIP into a separate folder for inspection. It includes `firmware/`, the required `assets/`, `mvp/`, hardware pin reference and firmware docs, but not a new website implementation or dependency installation. Start at `firmware/README.md`.
4. Website developers read `docs/firmware/INTEGRATION.md` and the JSON schemas. Firmware developers read `firmware/AGENTS.md`, build/tests and the arrival checklist. `team-workflow/status/firmware.md` records exactly what passed and what remains untested.
5. Build and run the native tests before flashing. The Windows native executable is local build output, not included as a portable binary. For ESP32 builds, install the pinned Arduino tools/libraries; choose cloud or LOCAL DEMO explicitly. Use the verified delivered board/port and USB-only initial bring-up.

The separate 96-PNG ZIP is optional convenience artwork. The firmware already consumes the horizontal sheets through the shared generated header; manual cutting is unnecessary. `runtime-preview.png` is rendered by the C++ scene compositor, without the physical LCD UI overlay. It is a software preview, not a photograph of a tested device.

The local implementation commit is authored by Codex because this checkout has no human Git identity configured. This does not change your global Git settings. No secrets, cloud credentials, installed toolchains or build caches are included in the packages.
