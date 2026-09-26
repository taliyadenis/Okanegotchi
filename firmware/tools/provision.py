"""USB setup/status for the physical device. Secrets are prompted, never saved or logged."""
import argparse
from getpass import getpass
import json
import re
import sys
import time
import uuid
import serial
from serial.tools import list_ports

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--list',action='store_true');ap.add_argument('--port');ap.add_argument('--configure',action='store_true');ap.add_argument('--factory-reset',action='store_true');ap.add_argument('--scenario',choices=['food','ride','savings','neglect','reset']);ap.add_argument('--pet',choices=['gator','robot','duck']);a=ap.parse_args()
    if a.list:
        for p in list_ports.comports():print(p.device,p.description)
        return
    if not a.port:ap.error('Use --list, then supply the board port with --port. No port is selected automatically.')
    if sum(bool(x) for x in (a.configure,a.factory_reset,a.scenario,a.pet))>1:ap.error('Choose one action')
    command={'op':'status'}
    if a.configure:
        ref=input('Supabase project ref (20 lowercase letters/digits): ').strip()
        if not re.fullmatch('[a-z0-9]{20}',ref):raise ValueError('Invalid project ref')
        device=input('Device UUID issued by the backend: ').strip();uuid.UUID(device)
        ssid=input('2.4 GHz Wi-Fi SSID: ');password=getpass('Wi-Fi password (blank for open network): ');token=getpass('Opaque device token issued by backend: ')
        if not 1<=len(ssid.encode())<=32 or len(password.encode())>63 or (password and len(password.encode())<8):raise ValueError('Invalid SSID/password length')
        if not re.fullmatch('[A-Za-z0-9_-]{32,256}',token):raise ValueError('Expected an opaque base64url/hex device token, not a Supabase service-role key')
        command={'op':'configure','ssid':ssid,'password':password,'url':f'https://{ref}.supabase.co/functions/v1/device-api/v1/sync','device_id':device,'device_token':token}
    elif a.factory_reset:command={'op':'factory_reset'};print('Hold all three device buttons for at least 2 seconds now.')
    elif a.scenario:command={'op':'demo','scenario':a.scenario}
    elif a.pet:command={'op':'pet','id':a.pet}
    payload=json.dumps(command,separators=(',',':'),ensure_ascii=False).encode()+b'\n'
    if len(payload)>1025:raise ValueError('Configuration exceeds serial limit')
    with serial.Serial(a.port,115200,timeout=0.25,write_timeout=3) as port:
        time.sleep(3);port.reset_input_buffer();port.write(payload);port.flush();deadline=time.monotonic()+10
        while time.monotonic()<deadline:
            raw=port.readline(2048)
            try:result=json.loads(raw)
            except (ValueError,UnicodeError):continue
            # Only display known safe response fields, even if the attached board is wrong.
            safe={k:result[k] for k in ('ok','error','restarting','firmware','mode','configured','status','nfc','heap_free','heap_min','psram_bytes','flash_bytes','pending') if k in result}
            if safe:print(json.dumps(safe,indent=2));return 0 if result.get('ok',True) else 1
        raise TimeoutError('No response. Confirm board, USB data cable, CDC-on-boot setting and port.')
if __name__=='__main__':
    try:sys.exit(main())
    except (ValueError,TimeoutError,serial.SerialException) as e:print(type(e).__name__+': '+str(e),file=sys.stderr);sys.exit(1)
