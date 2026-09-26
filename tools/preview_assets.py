#!/usr/bin/env python3
"""Render a tiny catalog summary for local content verification."""
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--catalog", type=Path, default=ROOT / "assets/catalog.example.json")
    args = ap.parse_args()
    catalog = json.loads(args.catalog.read_text(encoding="utf-8"))
    print("Catalog preview:", args.catalog)
    print(" characters:", len(catalog["characters"]))
    print(" clips:", len(catalog["clips"]))
    print(" scenes:", len(catalog["scenes"]))


if __name__ == "__main__":
    main()
