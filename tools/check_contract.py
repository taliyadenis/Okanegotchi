"""Standards validator; does not test firmware or server semantics."""
import copy
import json
from pathlib import Path
import sys
try:
    from jsonschema import Draft202012Validator, FormatChecker
except ImportError:
    raise SystemExit('BLOCKED: install requirements-dev.txt; no fallback pretending full schema validation')

ROOT = Path(__file__).resolve().parents[1]

def main():
    checker = FormatChecker()
    if 'date-time' not in checker.checkers or checker.conforms('2026-02-30T18:30:00Z', 'date-time'):
        raise SystemExit('BLOCKED: date-time format checker unavailable; install jsonschema[format-nongpl] from requirements-dev.txt')
    cases = json.loads((ROOT/'tests/contract_cases.json').read_text(encoding='utf-8'))
    failed = 0
    for case in cases:
        schema = json.loads((ROOT/'mvp'/case['schema']).read_text(encoding='utf-8'))
        Draft202012Validator.check_schema(schema)
        obj = json.loads((ROOT/case['fixture']).read_text(encoding='utf-8'))
        obj = copy.deepcopy(obj)
        for op in case.get('changes', []):
            parent = obj
            parts = op['path'].split('.')
            for part in parts[:-1]:
                parent = parent[int(part)] if isinstance(parent, list) else parent[part]
            key = int(parts[-1]) if isinstance(parent, list) else parts[-1]
            if op.get('delete'):
                del parent[key]
            else:
                parent[key] = op['value']
        errors = list(Draft202012Validator(schema, format_checker=checker).iter_errors(obj))
        ok = (not errors) == case['valid']
        failed += not ok
        print(('PASS ' if ok else 'FAIL ')+case['id'])
        if not ok:
            print('  '+ ('unexpectedly valid' if not errors else errors[0].message[:160]))
    print(f'{len(cases)-failed}/{len(cases)} schema cases passed; this is not firmware or backend validation')
    return int(bool(failed))

if __name__ == '__main__':
    sys.exit(main())
