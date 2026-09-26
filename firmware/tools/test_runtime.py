"""Exercise actual C++ JSON decoding, event persistence and runtime state transitions.

No HTTP, GPIO or storage timing is simulated as proof of physical behavior.
"""
import copy
import json
import os
from pathlib import Path
import subprocess
import unittest
ROOT=Path(__file__).resolve().parents[1]
BASE=json.loads((ROOT.parent/'mvp/examples/sync-response.json').read_text(encoding="utf-8"))
BASE['commands']=[];BASE['event_results']=[];BASE['review']=None
BASE['pet'].update(pet_id='gator',palette='original',accessory='none')
def uid(n):return f'11111111-1111-4111-8111-{n:012d}'
def command(n,animation='eating',expiry='2026-09-25T18:32:00Z'):
    return dict(seq=n,command_id=uid(n),animation=animation,duration_ms=8000,expires_at=expiry)
class Driver:
    def __init__(self):
        env=os.environ.copy()
        if os.name=='nt':env['PATH']='C:/msys64/ucrt64/bin;'+env['PATH']
        self.p=subprocess.Popen([str(ROOT/'build'/('host_driver.exe' if os.name=='nt' else 'host_driver'))],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=env,encoding='utf-8')
    def call(self,op,**kwargs):
        self.p.stdin.write(json.dumps(dict(op=op,**kwargs))+'\n');self.p.stdin.flush();line=self.p.stdout.readline()
        if not line:raise RuntimeError('driver exited: '+self.p.stderr.read())
        return json.loads(line)
    def close(self):self.p.stdin.close();self.p.wait(timeout=10);self.p.stdout.close();self.p.stderr.close()
class RuntimeTests(unittest.TestCase):
    def setUp(self):self.d=Driver();self.c=self.d.call
    def tearDown(self):self.d.close()
    def boot(self):
        self.c('online',value=True);r=self.c('next')['request'];self.assertIsNone(r['epoch']);self.assertEqual(r['events'],[]);return self.respond(r)
    def respond(self,req,**changes):
        body=copy.deepcopy(BASE);body.update(request_id=req['request_id'],**changes)
        return self.c('complete',id=req['request_id'],body=body)
    def request(self,ms=2000):self.c('advance',ms=ms);self.c('retry');return self.c('next')['request']
    def press(self,button,ms=80):
        self.c('button',id=button,down=True);self.c('advance',ms=ms);self.c('button',id=button,down=False);return self.c('advance',ms=30)
    def test_bootstrap_and_no_flash_writes_on_unchanged_polls(self):
        first=self.boot();self.assertTrue(first['accepted']);r=self.request();second=self.respond(r);self.assertEqual(first['writes'],second['writes'])
    def test_bootstrap_rejects_commands(self):
        self.c('online',value=True);r=self.c('next')['request'];self.assertFalse(self.respond(r,commands=[command(1)])['accepted'])
    def test_persistent_cache_replaced_and_valid_after_several_updates(self):
        self.boot()
        for version in range(19,24):self.assertTrue(self.respond(self.request(),state_version=version)['accepted'])
        reboot=self.c('reboot');self.assertFalse(reboot['storage_fault']);self.assertEqual(reboot['version'],23)
    def test_event_retry_and_reboot_preserve_id(self):
        self.boot();self.c('review');r=self.request();event=r['events'][0];self.c('complete',id=r['request_id'],status=503,body='');self.c('reboot');self.c('online',value=True);r2=self.c('next')['request'];self.assertEqual(r2['events'][0],event)
    def test_outbox_capacity_and_corrupt_store(self):
        self.boot()
        for _ in range(8):self.assertTrue(self.c('review')['accepted'])
        self.assertFalse(self.c('review')['accepted']);self.assertTrue(self.c('corrupt')['storage_fault']);self.c('online',value=True);self.assertIsNone(self.c('next')['request'])
    def test_invalid_result_does_not_drop_event(self):
        self.boot();self.c('review');r=self.request();bad=[dict(event_id=r['events'][0]['event_id'],status='pending',reason=None)]
        out=self.respond(r,event_results=bad);self.assertFalse(out['accepted']);self.assertEqual(out['outbox_count'],1)
    def test_unsolicited_result_rejected(self):
        self.boot();out=self.respond(self.request(),event_results=[dict(event_id=uid(22),status='accepted',reason=None)]);self.assertFalse(out['accepted'])
    def test_authentication_stops_retries(self):
        for status in (401,403,426):
            self.c('reset');self.boot();r=self.request();self.c('complete',id=r['request_id'],status=status,body='');self.c('advance',ms=70000);self.c('retry');self.assertIsNone(self.c('next')['request'])
    def test_backoff_and_rate_limit(self):
        self.boot();r=self.request();self.c('complete',id=r['request_id'],status=429,body='',retry_ms=20000);self.c('advance',ms=19999);self.assertIsNone(self.c('next')['request']);self.c('advance',ms=1);self.assertIsNotNone(self.c('next')['request'])
    def test_single_in_flight_and_unrelated_completion(self):
        self.boot();r=self.request();self.assertIsNone(self.c('next')['request']);self.assertFalse(self.c('complete',id=uid(88),body=BASE)['accepted']);self.assertTrue(self.respond(r)['accepted'])
    def test_retry_after_seconds_and_http_date(self):
        for value,expected in [('45',45000),('Fri, 25 Sep 2026 18:30:10 GMT',5000),('Thu, 24 Sep 2026 18:30:10 GMT',0),('not a date',0),('4294967295',4294967295000)]:
            self.assertEqual(self.c('retry_header',value=value)['delay_ms'],expected)
    def test_missing_oversized_and_trailing_json_rejected(self):
        self.boot()
        for invalid in ('{}',json.dumps(BASE)+' '*8192,json.dumps(BASE)+' {}'):
            r=self.request(70000);o=self.c('complete',id=r['request_id'],body=invalid);self.assertFalse(o['accepted']);self.assertEqual(o['ack'],0)
    def test_command_cursor_persisted_before_animation_and_reboot_no_replay(self):
        self.boot();out=self.respond(self.request(),commands=[command(1),command(2)])
        self.assertEqual(out['ack'],1);self.assertEqual(out['queued'],1)
        types=[t['type'] for t in out['trace']];self.assertLess(types.index('cursor_saved'),types.index('animation_start'))
        self.c('reboot');self.c('online',value=True);req=self.c('next')['request'];self.assertEqual(req['ack_command_seq'],1);out=self.respond(req,commands=[command(1),command(2)]);self.assertEqual(out['ack'],2)
    def test_pending_duplicates_filtered_before_capacity(self):
        self.boot();self.respond(self.request(),commands=[command(i) for i in range(1,5)])
        out=self.respond(self.request(),commands=[command(i) for i in range(2,6)]);self.assertTrue(out['accepted']);self.assertEqual(out['queued'],4)
        out=self.respond(self.request(),commands=[command(i) for i in range(3,7)]);self.assertFalse(out['accepted']);self.assertEqual(out['queued'],4)
    def test_expired_and_ghost_commands_consumed_without_animation(self):
        self.boot();out=self.respond(self.request(),commands=[command(1,expiry='2026-09-24T00:00:00Z')]);self.assertEqual(out['ack'],1);self.assertEqual(out['animation'],'idle')
        care=copy.deepcopy(BASE['care']);care['stage']='ghost';out=self.respond(self.request(),care=care,commands=[command(2)]);self.assertEqual(out['ack'],2);self.assertEqual(out['animation'],'ghost')
    def test_failed_cursor_save_never_starts_effect(self):
        self.boot();r=self.request();self.c('storage_fail',value=True);out=self.respond(r,commands=[command(1)]);self.assertTrue(out['storage_fault']);self.assertEqual(out['ack'],0);self.assertFalse(any(t['type']=='animation_start' for t in out['trace']))
    def test_reset_409_wipes_old_epoch_and_bootstraps(self):
        self.boot();self.c('review');r=self.request();body=json.loads((ROOT/'tests/fixtures/reset-error.json').read_text(encoding="utf-8"));body['request_id']=r['request_id'];out=self.c('complete',id=r['request_id'],status=409,body=body);self.assertTrue(out['accepted']);self.assertEqual(out['outbox_count'],0);self.assertEqual(out['epoch'],'');r=self.request(1000);self.assertIsNone(r['epoch']);self.assertEqual(r['events'],[])
    def test_short_button_and_5ms_bounce_after_idle(self):
        self.c('advance',ms=100);self.c('button',id='C',down=True);self.c('advance',ms=5);self.c('button',id='C',down=False);o=self.c('advance',ms=30);self.assertEqual(o['page'],0);self.assertEqual(self.press('C')['page'],1)
    def test_boot_held_button_is_ignored_until_release(self):
        self.c('button',id='C',down=True,boot=True);self.c('advance',ms=1500);self.c('button',id='C',down=False);out=self.c('advance',ms=40);self.assertEqual(out['page'],0);self.assertEqual(self.press('C')['page'],1)
    def test_long_press_does_not_also_fire_short(self):
        self.press('C');out=self.press('A',1100);self.assertEqual(out['page'],0);self.assertFalse(any(t['type']=='button_short' for t in out['trace']))
    def test_nfc_one_per_presentation_and_continuous_absence(self):
        self.boot();out=self.c('nfc',state='present',parsed='food');self.assertEqual(out['outbox_count'],1)
        for _ in range(5):self.c('advance',ms=200);self.assertEqual(self.c('nfc',state='present',parsed='food')['outbox_count'],1)
        self.c('nfc',state='absent');self.c('advance',ms=600);self.c('nfc',state='error');self.c('nfc',state='absent');self.assertEqual(self.c('nfc',state='present',parsed='food')['outbox_count'],1)
        self.c('nfc',state='absent');self.c('advance',ms=501);self.c('nfc',state='absent');self.assertEqual(self.c('nfc',state='present',parsed='food')['outbox_count'],2)
    def test_stale_nfc_intent_expires_before_slow_response(self):
        self.boot();self.c('advance',ms=11000);self.c('nfc',state='present',parsed='food');r=self.c('next')['request'];self.c('advance',ms=10001);out=self.respond(r);self.assertEqual(out['outbox_count'],0)
    def test_offline_demo_cannot_fake_financial_animation(self):
        self.boot();self.c('online',value=False);out=self.c('nfc',state='present',parsed='food');self.assertEqual(out['outbox_count'],0);self.assertEqual(out['animation'],'idle')
    def test_generic_card_only_requests_review(self):
        self.boot();out=self.c('nfc',state='present',parsed='unrelated');self.assertEqual(out['outbox'][0]['type'],'review_requested');self.assertEqual(out['outbox'][0]['payload'],{'source':'card'})
    def test_review_frozen_and_long_press_sends_receipt_id(self):
        self.boot();self.c('review');req=self.request();review=json.loads((ROOT.parent/'mvp/examples/sync-response.json').read_text(encoding="utf-8"))['review'];results=[dict(event_id=req['events'][0]['event_id'],status='accepted',reason=None)]
        out=self.respond(req,review=review,event_results=results);self.assertEqual(out['page'],4)
        other=copy.deepcopy(review);other['review_id']=uid(77);self.respond(self.request(),review=other)
        out=self.press('B',1100);self.assertEqual(out['outbox'][0]['payload']['review_id'],review['review_id'])
    def test_game_emits_one_cosmetic_result(self):
        self.boot();self.press('C',1100);self.assertEqual(self.press('B')['page'],5);out=self.c('advance',ms=15000);events=[e for e in out['outbox'] if e['type']=='play_completed'];self.assertEqual(len(events),1);self.assertTrue(0<=events[0]['payload']['score']<=15);self.assertEqual(out['spend'],BASE['finance']['spend_minor'])
    def test_actual_pets_render_and_variable_timing(self):
        hashes=[]
        for pet in ('gator','robot','duck'):
            a=self.c('render',pet=pet,ms=0);b=self.c('render',pet=pet,ms=239);self.assertEqual(a['hash'],b['hash']);self.assertEqual(a['pixel_bytes'],98366);hashes.append(a['hash'])
        self.assertEqual(len(set(hashes)),3)
    def test_local_demo_boot_food_review_checkin_reset(self):
        self.c('demo_start');self.c('online',value=True);self.assertTrue(self.c('demo_sync')['accepted']);self.c('advance',ms=1000)
        self.c('nfc',state='present',parsed='food');out=self.c('demo_sync');self.assertTrue(out['accepted']);self.assertEqual(out['spend'],3750);self.assertEqual(out['animation'],'eating')
        self.c('review');self.c('advance',ms=1000);out=self.c('demo_sync');self.assertTrue(out['accepted']);self.assertEqual(out['page'],4)
        self.press('B',1100);out=self.c('demo_sync');self.assertTrue(out['accepted']);self.assertEqual(out['outbox_count'],0);self.assertEqual(out['notice'],'Check-in confirmed')
        self.c('demo_scenario',scenario='reset');self.c('advance',ms=2000);out=self.c('demo_sync');self.assertEqual(out['http_status'],409);self.assertTrue(out['accepted']);self.c('advance',ms=1000);self.assertTrue(self.c('demo_sync')['accepted'])
    def test_ghost_confirm_visibly_returns_to_reviving_pet(self):
        self.c('demo_start');self.c('online',value=True);self.assertTrue(self.c('demo_sync')['accepted']);self.c('advance',ms=1000)
        self.c('demo_scenario',scenario='neglect');self.c('retry');out=self.c('demo_sync');self.assertTrue(out['accepted']);self.assertEqual(out['stage'],'ghost')
        self.c('review');self.c('advance',ms=1000);self.assertEqual(self.c('demo_sync')['page'],4)
        self.press('B',1100);out=self.c('demo_sync');self.assertTrue(out['accepted']);self.assertEqual(out['animation'],'revive');self.assertEqual(out['page'],0);self.assertEqual(out['stage'],'content')
        self.assertEqual(out['response']['finance']['spend_minor'],2500);self.assertEqual(out['response']['goal']['saved_minor'],5000)

def fixture_tests():
    cases=json.loads((ROOT/'tests/contract_cases.json').read_text(encoding="utf-8"));driver=Driver();passed=0
    try:
        for c in cases:
            path=(ROOT.parent if c['fixture'].startswith('mvp/') else ROOT)/c['fixture'];obj=json.loads(path.read_text(encoding="utf-8"))
            for change in c.get('changes',[]):
                p=obj;parts=change['path'].split('.')
                for key in parts[:-1]:p=p[int(key)] if isinstance(p,list) else p[key]
                key=int(parts[-1]) if isinstance(p,list) else parts[-1]
                if change.get('delete'):del p[key]
                else:p[key]=change['value']
            kind='error' if 'error' in c['schema'] else 'request' if 'request' in c['schema'] else 'response'
            actual=driver.call('validate',schema=kind,body=obj)['valid']
            if actual!=c['valid']:raise AssertionError(f'Firmware schema case {c["id"]}: {actual} != {c["valid"]}')
            passed+=1
        nfc=json.loads((ROOT/'tests/nfc/cases.json').read_text(encoding="utf-8"))
        for c in nfc:
            actual=driver.call('parse_nfc',hex=c['hex'])['parsed']
            if actual!=c['expected']:raise AssertionError(c['id']+': '+actual)
        clips=json.loads((ROOT/'tests/animation/samples.json').read_text(encoding='utf-8'))
        for c in clips:
            actual=driver.call('sample_clip',durations_ms=c['durations_ms'],mode=c['mode'],elapsed_ms=c['elapsed_ms'])['frame']
            if actual!=c['expected_frame']:raise AssertionError(c['id']+': wrong displayed frame')
        print(f'PASS actual firmware: {passed} schema fixtures; {len(nfc)} NDEF fixtures; {len(clips)} compositor timing fixtures',flush=True)
    finally:driver.close()
if __name__=='__main__':fixture_tests();unittest.main(verbosity=2)
