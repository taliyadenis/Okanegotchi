"""Exercise LOCAL DEMO on an explicitly selected USB board, without peripherals.

This changes its fictional ledger/pet and injects button inputs via USB. It does
not test physical switches, LCD pixels, RF, Wi-Fi/TLS or a deployed backend.
"""
import argparse
import json
from pathlib import Path
import time

import serial


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--port', required=True)
    ap.add_argument('--report', type=Path)
    args = ap.parse_args()
    checks = []
    with serial.Serial(args.port, 115200, timeout=0.2, write_timeout=3) as port:
        time.sleep(2)
        port.reset_input_buffer()

        def request(command):
            port.write(json.dumps(command).encode() + b'\n')
            port.flush()
            deadline = time.monotonic() + 5
            while time.monotonic() < deadline:
                raw = port.readline(8192)
                try:
                    reply = json.loads(raw)
                except (ValueError, UnicodeError):
                    continue
                if command['op'] == 'status' and 'mode' in reply:
                    return reply
                if command['op'] != 'status' and 'ok' in reply:
                    if not reply['ok']:
                        raise RuntimeError(reply.get('error', 'Command rejected'))
                    return reply
            raise TimeoutError(f"No reply to {command['op']}")

        def status():
            return request({'op': 'status'})

        def expect(name, predicate, timeout=6):
            deadline = time.monotonic() + timeout
            while True:
                state = status()
                if predicate(state):
                    checks.append({'check': name, 'status': state})
                    print('PASS', name, flush=True)
                    return state
                if time.monotonic() >= deadline:
                    raise AssertionError(f'{name}: {json.dumps(state)}')
                time.sleep(0.12)

        def scenario(name):
            request({'op': 'demo', 'scenario': name})

        def button(name, hold=False):
            duration = 1200 if hold else 150
            request({'op': 'demo_button', 'button': name, 'duration_ms': duration})
            time.sleep(duration / 1000 + 0.12)

        initial = status()
        if initial.get('mode') != 'LOCAL_DEMO' or 'demo' not in initial:
            raise RuntimeError('Flash the LOCAL DEMO build with USB bench diagnostics first.')
        expect('4 MB flash / 2 MB PSRAM detected',
               lambda s: s['flash_bytes'] == 4194304 and s['psram_bytes'] == 2097152)
        scenario('reset')
        expect('Bootstrap/reset baseline', lambda s: s['demo']['spend_minor'] == 2500
               and s['demo']['saved_minor'] == 5000 and s['demo']['care_stage'] == 'content'
               and s['demo']['ack_command_seq'] == 0 and not s['demo']['storage_fault'])
        for pet in ('gator', 'robot', 'duck'):
            request({'op': 'pet', 'id': pet})
            expect('Select ' + pet, lambda s: s['demo']['pet'] == pet)
        scenario('food')
        expect('Food: $12.50 + eating', lambda s: s['demo']['spend_minor'] == 3750
               and s['demo']['animation'] == 'eating')
        scenario('ride')
        expect('Ride queued behind eating', lambda s: s['demo']['spend_minor'] == 5550
               and s['demo']['animation'] == 'eating' and s['demo']['queued_commands'] >= 1)
        expect('Queued ride starts traveling', lambda s: s['demo']['animation'] == 'traveling', 10)
        expect('Timed reaction returns to idle', lambda s: s['demo']['animation'] == 'idle', 10)
        scenario('savings')
        expect('Savings: +$10 without extra expense', lambda s: s['demo']['saved_minor'] == 6000
               and s['demo']['spend_minor'] == 5550 and s['demo']['animation'] == 'celebrate')
        scenario('neglect')
        expect('Neglect shows ghost', lambda s: s['demo']['care_stage'] == 'ghost'
               and s['demo']['animation'] == 'ghost')
        button('A', True)
        button('C', True)
        expect('Ghost blocks game', lambda s: s['demo']['page'] == 'pet')
        button('B', True)
        expect('Review requested by long B', lambda s: s['demo']['page'] == 'review'
               and s['demo']['review_available'])
        button('B', True)
        expect('Confirmed review revives without changing money',
               lambda s: s['demo']['care_stage'] == 'content' and s['demo']['pm_complete']
               and s['demo']['page'] == 'pet' and s['demo']['animation'] == 'revive'
               and s['demo']['spend_minor'] == 5550 and s['demo']['saved_minor'] == 6000)
        expect('Revive finishes', lambda s: s['demo']['animation'] == 'idle')
        button('C')
        expect('Short C opens goal', lambda s: s['demo']['page'] == 'goal')
        button('A', True)
        button('C', True)
        expect('Long C opens game', lambda s: s['demo']['page'] == 'game')
        button('B')
        expect('B starts game', lambda s: s['demo']['game_started'] and not s['demo']['game_paused'])
        button('A')
        expect('A moves left', lambda s: s['demo']['game_lane'] == 0)
        button('C')
        expect('C moves right', lambda s: s['demo']['game_lane'] == 1)
        button('B')
        expect('B pauses game', lambda s: s['demo']['game_paused'])
        button('A', True)
        expect('Long A exits game', lambda s: s['demo']['page'] == 'pet')
        button('B')
        expect('B pets character', lambda s: s['demo']['animation'] == 'pat')
        expect('No pending events/storage fault', lambda s: s['pending'] == 0 and not s['demo']['storage_fault'])
        scenario('reset')
        expect('Final reset baseline', lambda s: s['demo']['spend_minor'] == 2500
               and s['demo']['saved_minor'] == 5000 and s['demo']['pet'] == 'gator'
               and s['demo']['ack_command_seq'] == 0)
        final = status()
    report = {'port': args.port, 'scope': 'USB LOCAL DEMO; simulated inputs; no peripherals or network',
              'initial': initial, 'checks': checks, 'final': final}
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(f'{len(checks)} board-runtime checks passed. Minimum free heap: {final["heap_min"]} bytes.')


if __name__ == '__main__':
    main()
