# Pack preparation validation — 2026-09-26

## Revision2 additions

Modular characters/scenes/backgrounds/clothing/frame playback and mandatory NFC Text sticker requirements added. Final art, converter/player and actual C++ sticker implementation still belong to Cursor's task.

- PASS placeholder catalog semantic checks and11 deliberately invalid reference/geometry/path/palette mutations.
- PASS24 Python NDEF reference-fixture cases (four scenario texts, padding/control TLV and rejection cases). No physical tag or C++ parser tested.
- PASS asset catalog Draft2020-12 schema validation, example catalog and unknown-property rejection.
- PASS new Python utility syntax. Content runner rejects a missing C++ executable (0/46, nonzero exit), rather than falsely passing.
- Supplied22 animation timing samples and13 NFC routing cases for future real C++ testing. They are NOT RUN against firmware.39 original state/protocol cases remain; total main/routing scenarios52, plus46 parser/player samples.
- Independent NFC review required positive Type2 classification, bounded page reads, real library API path, volatile refresh intent and errors-vs-absence distinction; these are specified. A pinned/licensed library adapter patch is allowed when public APIs lack metadata/readiness.
- Independent revision2 review corrected terminator handling for rewritten tags, feet-anchor scene positioning, canonical import palette, scenario-payload assertions and continuous absence/error reset. Tag bytes after the terminator are ignored; schema/import checks distinguish canonical palette from display variants.

Additional pack-only checks: `python tools/check_assets.py --self-test`, `python tools/nfc_records.py --self-test`. Later firmware checks: run_acceptance.py with both tests/cases.json and tests/nfc/routing-cases.json, plus run_content_acceptance.py against the production-linked content driver. Do not count reference tests as implementation verification.

This report covers the handoff/test infrastructure only. **No ESP32 firmware implementation or firmware driver is bundled or claimed tested.** Cursor's task is to write that implementation using this pack.

| Check | Actual result |
|---|---|
| Full Draft2020-12 schema/format fixtures | PASS44/44 with jsonschema4.26.0 + format-nongpl dependencies |
| Acceptance-runner self-test | PASS2 valid results and8 deliberately invalid results |
| Empty-suite failure behavior | PASS; runner exits2 instead of false0/0 success |
| C++ test-support helpers | PASS; built with GCC15.2.0, C++17, -Wall -Wextra -Werror; executable passed |
| Python utility syntax | PASS py_compile for runner, schema validator, pack checker and delivery generator |
| Independent pack review | Completed; corrected request timing, submitted-event correlation, state versions, reset epoch and cursor-save invariant |
| ZIP CRC/file hashes | Verified by delivery generator; MANIFEST.json records SHA256 of delivered files |
| C++ firmware behavior scenarios |39 supplied, NOT RUN: production driver does not exist yet |
| ESP32 cross-compile/flash/memory | NOT RUN: implementation/toolchain profile required |
| Physical SPI/NFC/display/buttons/piezo | NOT RUN: hardware required |
| Live website/TLS integration | NOT RUN: deployment/config/hardware required |

Commands (from extracted pack root after installing requirements-dev.txt in the chosen Python environment):

```text
python tools/check_contract.py
python tools/run_acceptance.py --self-test
g++ -std=c++17 -Wall -Wextra -Werror tests/support/harness_smoke.cpp -o build/harness_smoke
build/harness_smoke
python tools/check_pack.py
```

Create build/ before compiling; Windows executable may use .exe. Preparation used WindowsPython3.14 and GCC15.2.0. Validator dependencies were installed in an ignored build/devdeps directory; local permission boundaries required an elevated validation invocation with that directory added to sys.path. Dependencies/binaries are excluded from ZIP. Initial validation exposed absent optional date-time validation; requirements and a fail-closed validator preflight were fixed, then44/44 passed. This history is not a firmware defect.

`check_pack.py` is a delivery-integrity check and will intentionally fail after implementation edits alter delivered files; it is not a permanent source-code CI requirement. Do not regenerate acceptance expectations to make failing firmware appear correct.
