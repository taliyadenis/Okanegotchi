# Push firmware MVP to GitHub (hardware team)

Target repo: **https://github.com/taliyadenis/Okanegotchi**  
Branch for hardware: **`hardware/firmware-mvp`** (commit `25aef6d` — full MVP with host tests passing).

This cloud environment cannot authenticate to GitHub (`gh` is not logged in). Run the steps below on your machine after `gh auth login` or with a PAT.

## Option A — From this Cursor project (if synced locally)

```bash
git fetch origin
git checkout hardware/firmware-mvp
git remote add github https://github.com/taliyadenis/Okanegotchi.git   # once
git fetch github main
git push -u github hardware/firmware-mvp
```

Open a PR on GitHub: `hardware/firmware-mvp` → `main`.

## Option B — Fresh clone of GitHub, pull branch from Cursor origin

If your local folder only has the GitHub repo:

```bash
git clone https://github.com/taliyadenis/Okanegotchi.git
cd Okanegotchi
git remote add cursor <your-cursor-project-git-url>
git fetch cursor hardware/firmware-mvp
git checkout -b hardware/firmware-mvp cursor/hardware/firmware-mvp
git push -u origin hardware/firmware-mvp
```

Use the git URL from the Cursor agent / project settings for `<your-cursor-project-git-url>`.

## What’s on the branch

- `firmware/` — C++ core, NDEF, drivers, ESP32 sketch  
- `tests/driver/` — `firmware_test_driver`, `content_test_driver`  
- `CMakeLists.txt`, `tools/build_assets.py`, `scripts/build_esp32.sh`  
- `docs/EXECUTION_STATUS.md`, `docs/VERIFICATION_REPORT.md`
