#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FQBN="${FQBN:-esp32:esp32:esp32s3:FlashSize=4M,PSRAM=opi,USBMode=default,CDCOnBoot=cdc}"
arduino-cli compile --fqbn "$FQBN" "$ROOT/firmware/sketch/okanegachi" "$@"
