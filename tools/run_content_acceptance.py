"""Run actual C++ content driver against independent byte/timing expectations."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[1]
def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--driver',required=True,help='compiled content_test_driver executable path')
    args=ap.parse_args()
    cases=[]
    for c in json.loads((ROOT/'tests/nfc/cases.json').read_text()):
        cases.append((c['id'],{'op':'parse_ndef','hex':c['hex']}, {'result':c['expected']}))
    for c in json.loads((ROOT/'tests/animation/samples.json').read_text()):
        cases.append((c['id'],{'op':'sample_clip','durations_ms':c['durations_ms'],
            'mode':c['mode'],'elapsed_ms':c['elapsed_ms']},{'frame':c['expected_frame']}))
    if not cases: raise SystemExit('FAIL empty content fixture suite')
    failed=0
    for name,payload,expected in cases:
        try:
            proc=subprocess.run([args.driver],input=json.dumps(payload)+'\n',capture_output=True,text=True,encoding='utf-8',timeout=10,cwd=ROOT)
            lines=[x for x in proc.stdout.splitlines() if x.strip()]
            actual=json.loads(lines[0]) if proc.returncode==0 and len(lines)==1 else None
            ok=isinstance(actual,dict) and set(actual)==set(expected) and all(type(actual[k]) is type(v) and actual[k]==v for k,v in expected.items())
        except (OSError,ValueError,subprocess.TimeoutExpired): ok=False
        failed+=not ok
        print(('PASS ' if ok else 'FAIL ')+name)
    print(f'{len(cases)-failed}/{len(cases)} content driver cases passed; hardware not tested')
    return int(bool(failed))
if __name__=='__main__':sys.exit(main())
