"""Check pack structure/hashes. Not a code/hardware validation."""
import hashlib
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
    paths=['MASTER_PROMPT.md','FIRMWARE_SPEC.md','HARDWARE.md','ACCEPTANCE.md','TEST_DRIVER_CONTRACT.md',
      'tests/cases.json','tests/contract_cases.json','hardware/firmware_pins.h','mvp/DEVICE_PROTOCOL_V1.md',
      'docs/MODULAR_ASSETS.md','docs/ANIMATION_ENGINE.md','docs/NFC_DEMO_STICKERS.md',
      'assets/catalog.schema.json','assets/catalog.example.json','tests/nfc/routing-cases.json',
      'tests/animation/samples.json','UPDATE_PROMPT.md']
    for name in paths:
        assert (ROOT/name).is_file(), name
    for name in ['tests/cases.json','tests/contract_cases.json','tests/nfc/cases.json',
                 'tests/nfc/routing-cases.json','tests/animation/samples.json']:
        cases=json.loads((ROOT/name).read_text(encoding='utf-8'))
        assert cases and len({case['id'] for case in cases})==len(cases), name
    for p in ROOT.glob('mvp/*.schema.json'):
        assert json.loads(p.read_text())['$schema'].endswith('2020-12/schema')
    manifest=json.loads((ROOT/'MANIFEST.json').read_text())
    for path,digest in manifest.items():
        assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==digest, f'modified since packaging: {path}'
    print(f'PASS pack structure and {len(manifest)} SHA256 hashes; NOT firmware verification')
if __name__=='__main__':
    main()
