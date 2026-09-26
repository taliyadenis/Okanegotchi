#!/usr/bin/env python3
"""Convert catalog placeholders into generated C++ asset registry headers."""
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--catalog", type=Path, default=ROOT / "assets/catalog.example.json")
    ap.add_argument("--out", type=Path, default=ROOT / "firmware/assets/generated")
    args = ap.parse_args()
    catalog = json.loads(args.catalog.read_text(encoding="utf-8"))
    args.out.mkdir(parents=True, exist_ok=True)
    header = args.out / "asset_registry.hpp"
    lines = [
        "#pragma once",
        f"// Generated from {args.catalog.name}",
        f"namespace okanegachi::assets {{",
        f"inline constexpr int kAssetVersion = {catalog['asset_version']};",
        f"inline constexpr int kFrameCount = {len(catalog['frames'])};",
        f"inline constexpr int kClipCount = {len(catalog['clips'])};",
        "}",
    ]
    header.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote {header}")


if __name__ == "__main__":
    main()
