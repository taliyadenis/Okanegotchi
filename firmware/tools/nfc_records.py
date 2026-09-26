"""Restricted NDEF Text/Type2 TLV reference + fixture builder. Never writes hardware."""
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PREFIX = 'okanegachi:demo:v1:'
ACTIONS = ('food', 'ride', 'savings', 'review')

def encode_text(text):
    payload = b'\x02en' + text.encode('utf-8')
    if len(payload) > 127:
        raise ValueError('text exceeds demo profile')
    ndef = bytes((0xD1, 1, len(payload), 0x54)) + payload
    tlv = bytes((3, len(ndef))) + ndef + b'\xfe'
    if len(tlv) > 144:
        raise ValueError('exceeds NTAG213 user-memory cap')
    return tlv

def parse(data):
    """Return allowlisted action or unrelated. Malformed/unsupported raises ValueError."""
    if not data or len(data) > 144:
        raise ValueError('invalid memory length')
    i, message, terminated = 0, None, False
    while i < len(data):
        typ = data[i]
        i += 1
        if typ == 0:
            continue
        if typ == 0xFE:
            # A rewritten tag may retain old bytes beyond the terminator. Ignore them.
            terminated = True
            break
        if i >= len(data):
            raise ValueError('missing TLV length')
        size = data[i]
        i += 1
        if size == 0xFF or i + size > len(data):
            raise ValueError('unsupported or truncated TLV length')
        value = data[i:i+size]
        i += size
        if typ in (1, 2):
            if size != 3:
                raise ValueError('invalid control TLV')
        elif typ == 3 and message is None:
            message = value
        else:
            raise ValueError('unknown or multiple TLV')
    if message is None or not terminated:
        raise ValueError('missing NDEF or terminator')
    if len(message) < 7 or message[0] != 0xD1 or message[1] != 1 or message[3] != 0x54:
        raise ValueError('requires one short UTF8 Text record')
    if len(message) != 4 + message[2]:
        raise ValueError('record length mismatch')
    payload = message[4:]
    status = payload[0]
    langlen = status & 0x3F
    if status & 0xC0 or not 1 <= langlen <= 8 or 1+langlen >= len(payload):
        raise ValueError('invalid text status/language')
    language = payload[1:1+langlen]
    if not all(65 <= b <= 90 or 97 <= b <= 122 or b == 45 for b in language):
        raise ValueError('invalid language code')
    try:
        text = payload[1+langlen:].decode('utf-8', errors='strict')
    except UnicodeDecodeError as exc:
        raise ValueError('invalid UTF8') from exc
    if '\x00' in text:
        raise ValueError('embedded NUL')
    if text.startswith('okanegachi:'):
        if not text.startswith(PREFIX) or text[len(PREFIX):] not in ACTIONS:
            raise ValueError('unsupported Okanegachi command')
        return text[len(PREFIX):]
    return 'unrelated'

def build_cases():
    cases = []
    for action in ACTIONS:
        cases.append({'id':'valid-'+action,'hex':encode_text(PREFIX+action).hex(),'expected':action})
    food = encode_text(PREFIX+'food')
    cases += [
        {'id':'valid-padding','hex':(b'\x00'+food+b'\x00'*20).hex(),'expected':'food'},
        {'id':'valid-control','hex':(b'\x01\x03\xa0\x10\x44'+food).hex(),'expected':'food'},
        {'id':'unrelated-text','hex':encode_text('Hello').hex(),'expected':'unrelated'},
        {'id':'valid-old-data-after-terminator','hex':(food+b'\x01old-data').hex(),'expected':'food'},
    ]
    bad = {
        'empty':b'', 'over-cap':food+b'\x00'*144, 'truncated':food[:-3],
        'no-terminator':food[:-1], 'double-ndef':food[:-1]+food,
        'unknown-version':encode_text('okanegachi:demo:v2:food'),
        'unknown-action':encode_text(PREFIX+'reset'),
        'trailing-newline':encode_text(PREFIX+'food\n'),
        'nul':encode_text(PREFIX+'food\x00'),
        'bad-control':b'\x01\x02\x00\x00'+food,
    }
    for name,index,value in [('utf16',6,0x82),('huge-language',6,0x3f),('uri-record',5,0x55),
                             ('chained-record',2,0x91),('wrong-record-length',4,0x7f),('long-tlv',1,0xff)]:
        changed=bytearray(food); changed[index]=value; bad[name]=bytes(changed)
    cases.extend({'id':'reject-'+name,'hex':data.hex(),'expected':'invalid'} for name,data in bad.items())
    return cases

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--write-fixtures', action='store_true')
    ap.add_argument('--self-test', action='store_true')
    args=ap.parse_args()
    cases=build_cases()
    if args.write_fixtures:
        folder=ROOT/'tests/nfc'; folder.mkdir(parents=True,exist_ok=True)
        (folder/'cases.json').write_text(json.dumps(cases,indent=2)+'\n',encoding='utf-8')
        for action in ACTIONS:
            (folder/(action+'.hex')).write_text(encode_text(PREFIX+action).hex(' ')+'\n',encoding='utf-8')
        routing=[]
        def tag(action):
            return {'op':'nfc','present':True,'tag_type':'ntag213','ndef_hex':encode_text(PREFIX+action).hex()}
        def advance(ms): return {'op':'advance','ms':ms}
        def add(name,steps,demo,review):
            routing.append({'id':name,'seed':{},'steps':steps,'expect':{
                'counts':[{'where':{'type':'event_queued','event_type':'demo_trigger'},'eq':demo},
                          {'where':{'type':'event_queued','event_type':'review_requested'},'eq':review}]}})
        for action in ('food','ride','savings'):
            add('demo-'+action,[{'op':'sync'},tag(action),advance(1200)],1,0)
            routing[-1]['expect']['equals']={'requests.1.events.0.type':'demo_trigger', 'requests.1.events.0.payload.scenario':action}
        add('review-sticker',[{'op':'sync'},tag('review'),advance(1200)],0,1)
        routing[-1]['expect']['equals']={'requests.1.events.0.type':'review_requested', 'requests.1.events.0.payload.source':'sticker'}
        add('normal-food-review',[{'op':'sync','patch':{'demo_mode':False,'next_sync_ms':30000}},tag('food'),advance(200)],0,1)
        add('offline-no-new-demo',[{'op':'sync'},{'op':'connectivity','online':False},tag('food'),advance(200)],0,0)
        add('stale-demo-pending-refresh',[{'op':'sync'},advance(11000),tag('food'),advance(200)],0,0)
        add('stale-demo-fresh-confirmation',[{'op':'sync'},advance(11000),tag('food'),advance(200),{'op':'sync'}],1,0)
        add('held-demo-one-event',[{'op':'sync'},tag('food'),advance(5000)],1,0)
        add('intentional-retap',[{'op':'sync'},tag('food'),advance(200),{'op':'nfc','present':False},advance(600),tag('food'),advance(200)],2,0)
        add('read-error-not-removal',[{'op':'sync'},tag('food'),advance(200),{'op':'nfc','observation':'error'},advance(600),tag('food'),advance(200)],1,0)
        add('absence-must-be-continuous',[{'op':'sync'},tag('food'),advance(200),{'op':'nfc','present':False},advance(300),{'op':'nfc','observation':'error'},advance(100),{'op':'nfc','present':False},advance(300),tag('food'),advance(200)],1,0)
        add('malformed-tag-no-action',[{'op':'sync'},{'op':'nfc','present':True,'tag_type':'ntag213','ndef_hex':'0308d1'},advance(200)],0,0)
        (folder/'routing-cases.json').write_text(json.dumps(routing,indent=2)+'\n',encoding='utf-8')
    if args.self_test:
        for case in cases:
            try: actual=parse(bytes.fromhex(case['hex']))
            except ValueError: actual='invalid'
            assert actual==case['expected'], (case['id'],actual,case['expected'])
        print(f'PASS {len(cases)} Python NFC fixture checks; C++ parser/hardware NOT tested')
    if not (args.write_fixtures or args.self_test):
        ap.print_help()

if __name__=='__main__': main()
