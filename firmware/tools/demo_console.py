"""Run the real firmware core and local fake server before hardware arrives."""
from test_runtime import Driver
import json
def main():
    d=Driver();c=d.call;c('demo_start');c('online',value=True);c('demo_sync');c('advance',ms=1000)
    print('LOCAL SIMULATION. Commands: food ride savings review confirm neglect reset gator robot duck offline online status quit')
    try:
        while True:
            command=input('pet> ').strip()
            if command=='quit':break
            if command in ('food','ride','savings','neglect','reset'):c('demo_scenario',scenario=command);c('retry')
            elif command=='review':c('review')
            elif command=='confirm':c('button',id='B',down=True);c('advance',ms=1100);c('button',id='B',down=False);c('advance',ms=30)
            elif command in ('gator','robot','duck'):c('demo_pet',pet=command);c('retry')
            elif command in ('online','offline'):c('online',value=command=='online')
            c('advance',ms=2000);out=c('demo_sync')
            print(json.dumps({k:out[k] for k in ('status','notice','pet','stage','animation','spend','outbox_count','ack')},indent=2))
    finally:d.close()
if __name__=='__main__':main()
