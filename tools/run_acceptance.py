"""Independent black-box runner. Self-test validates this runner, never firmware."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]

def path_get(obj, path):
    for part in path.split('.'):
        obj = obj[int(part)] if isinstance(obj, list) else obj[part]
    return obj

def matches(record, subset):
    return isinstance(record, dict) and all(k in record and record[k] == v for k, v in subset.items())

def validate_result(result, expect):
    failures = []
    if not isinstance(result, dict):
        return ['driver result must be object']
    for key, typ in [('snapshot', dict), ('trace', list), ('requests', list)]:
        if not isinstance(result.get(key), typ):
            failures.append(f'missing/invalid {key}')
    if failures:
        return failures
    saved = set()
    for record in result['trace']:
        if not isinstance(record, dict):
            failures.append('trace entries must be objects')
            continue
        if record.get('type') == 'cursor_saved':
            if not isinstance(record.get('epoch'), str) or type(record.get('seq')) is not int:
                failures.append('cursor_saved must identify epoch and integer sequence')
            else:
                saved.add((record['epoch'], record['seq']))
        if record.get('type') == 'animation_start' and (type(record.get('seq')) is not int or not isinstance(record.get('epoch'), str) or (record['epoch'], record['seq']) not in saved):
            failures.append('animation_start without earlier successful cursor_saved')
    for path, value in expect.get('equals', {}).items():
        try:
            actual = path_get(result, path)
            if type(actual) is not type(value) or actual != value:
                failures.append(f'{path}: expected {value!r}, got {actual!r}')
        except (KeyError, IndexError, TypeError, ValueError):
            failures.append(f'{path}: missing')
    for item in expect.get('counts', []):
        count = sum(matches(record, item['where']) for record in result['trace'])
        if count != item['eq']:
            failures.append(f'trace {item["where"]}: expected {item["eq"]}, got {count}')
    pos = 0
    for wanted in expect.get('ordered', []):
        while pos < len(result['trace']) and not matches(result['trace'][pos], wanted):
            pos += 1
        if pos == len(result['trace']):
            failures.append(f'missing ordered trace {wanted}')
            break
        pos += 1
    if 'request_count' in expect and len(result['requests']) != expect['request_count']:
        failures.append(f'expected {expect["request_count"]} requests, got {len(result["requests"])}')
    return failures

def self_test():
    good = {'snapshot': {'x': 2}, 'trace': [{'type':'saved'}, {'type':'start'}], 'requests':[{'ack':2}]}
    check = {'equals': {'snapshot.x':2, 'requests.0.ack':2}, 'counts':[{'where':{'type':'start'}, 'eq':1}], 'ordered':[{'type':'saved'}, {'type':'start'}], 'request_count':1}
    assert validate_result(good, check) == []
    assert validate_result({'snapshot':{}, 'requests':[], 'trace':[
        {'type':'cursor_saved','epoch':'test-epoch','seq':5},
        {'type':'animation_start','epoch':'test-epoch','seq':5}]}, {}) == []
    mutations = [({}, check), (good, {'equals':{'snapshot.x': True}}),
        (good, {'equals':{'snapshot.missing':0}}), (good, {'request_count':2}),
        (good, {'counts':[{'where':{'type':'start'},'eq':2}]}),
        (good, {'ordered':[{'type':'start'}, {'type':'saved'}]}),
        ({'snapshot':{},'trace':'wrong','requests':[]}, {}),
        ({'snapshot':{},'trace':[{'type':'animation_start','seq':5}], 'requests':[]}, {})]
    for result, expected in mutations:
        assert validate_result(result, expected), 'runner failed to reject mutation'
    print(f'PASS runner self-test: 2 valid and {len(mutations)} invalid result checks; NOT firmware validation')

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--self-test', action='store_true')
    ap.add_argument('--driver', help='path to compiled C++ driver, not a shell command')
    ap.add_argument('--cases', type=Path, default=ROOT/'tests/cases.json')
    ap.add_argument('--timeout', type=float, default=15)
    args = ap.parse_args()
    if args.self_test:
        self_test()
        return 0
    if not args.driver:
        ap.error('--driver is required; no firmware implementation is bundled')
    cases = json.loads(args.cases.read_text(encoding='utf-8'))
    if not isinstance(cases, list) or not cases:
        ap.error('cases must be a nonempty array; empty suite cannot pass')
    failed = 0
    for case in cases:
        try:
            payload = {'seed':case.get('seed', {}), 'steps':case['steps']}
            proc = subprocess.run([args.driver], input=json.dumps(payload)+'\n', text=True,
                encoding='utf-8', capture_output=True, timeout=args.timeout, cwd=ROOT)
            if proc.returncode:
                problems = [f'driver exited {proc.returncode}; inspect bounded local stderr']
            else:
                lines = [line for line in proc.stdout.splitlines() if line.strip()]
                if len(lines) != 1:
                    problems = ['driver must output exactly one JSON line']
                else:
                    problems = validate_result(json.loads(lines[0]), case['expect'])
        except (OSError, ValueError, subprocess.TimeoutExpired) as exc:
            problems = [f'{type(exc).__name__}: {exc}']
        print(('FAIL ' if problems else 'PASS ') + case['id'])
        for problem in problems:
            print('  ' + problem)
        failed += bool(problems)
    print(f'{len(cases)-failed}/{len(cases)} cases passed; {failed} failed')
    return 1 if failed else 0

if __name__ == '__main__':
    sys.exit(main())
