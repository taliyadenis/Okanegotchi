import {test} from 'node:test'
import assert from 'node:assert/strict'
import {CloudStore,SaveConflict} from '../src/account/cloud-store.ts'
import type {CloudBackend,CloudRow,DocumentKind} from '../src/account/cloud-store.ts'
const key=(user:string)=>'okanegotchi:demo-mail:v4:'+encodeURIComponent(user)
function backend(){
  const rows=new Map<DocumentKind,CloudRow>()
  return {rows,load:async()=>structuredClone([...rows.values()]),save:async(kind:DocumentKind,value:string,revision:number)=>{
    const existing=rows.get(kind)
    if((existing?.revision??0)!==revision)throw new SaveConflict()
    rows.set(kind,{kind,value,revision:revision+1});return revision+1
  }}
}
test('saved data hydrates on another browser and identities cannot use each other’s keys',async()=>{
  const server=backend(),first=new CloudStore('user:a',server);await first.hydrate()
  first.setItem(key('user:a'),'{"name":"Gator"}');await first.flush()
  const second=new CloudStore('user:a',server);await second.hydrate()
  assert.equal(second.getItem(key('user:a')),'{"name":"Gator"}')
  assert.equal(second.getItem(key('user:b')),null);assert.throws(()=>second.setItem(key('user:b'),'{}'))
})
test('concurrent browsers do not silently overwrite each other',async()=>{
  const server=backend(),a=new CloudStore('user:a',server),b=new CloudStore('user:a',server)
  await a.hydrate();await b.hydrate();a.setItem(key('user:a'),'{"v":1}');await a.flush()
  b.setItem(key('user:a'),'{"v":2}');await assert.rejects(b.flush(),SaveConflict)
  assert.equal(b.status,'conflict');assert.equal(server.rows.get('setup')?.value,'{"v":1}')
})
test('failed save stays pending and retry acknowledges it',async()=>{
  const server=backend();let offline=true
  const store=new CloudStore('user:a',{...server,save:async(...args)=>{if(offline)throw Error('offline');return server.save(...args)}})
  await store.hydrate();store.setItem(key('user:a'),'{}');await assert.rejects(store.flush())
  assert.equal(store.status,'error');assert.equal(store.hasUnsaved(),true)
  offline=false;await store.flush();assert.equal(store.status,'saved');assert.equal(store.hasUnsaved(),false)
})
test('edits during a save are serialized and the latest value survives',async()=>{
  const server=backend();let release:()=>void=()=>{};let first=true
  const store=new CloudStore('user:a',{...server,save:async(...args)=>{if(first){first=false;await new Promise<void>(r=>release=r)}return server.save(...args)}})
  await store.hydrate();store.setItem(key('user:a'),'{"v":1}');store.setItem(key('user:a'),'{"v":2}');release();await store.flush()
  assert.equal(server.rows.get('setup')?.value,'{"v":2}');assert.equal(store.status,'saved')
})
test('ending a session prevents further queued writes',async()=>{
  const server=backend(),store=new CloudStore('user:a',server);await store.hydrate();store.dispose()
  assert.equal(store.getItem(key('user:a')),null);assert.throws(()=>store.setItem(key('user:a'),'{}'));await assert.rejects(store.flush())
})


test('onboarding completion travels with setup and survives later pet edits',async()=>{
 const server=backend(),first=new CloudStore('user:a',server);await first.hydrate()
 const setupKey=key('user:a'),doneKey='okanegotchi:onboarding:v1:'+encodeURIComponent('user:a')
 first.setItem(setupKey,JSON.stringify({version:4,revision:0,companion:{name:'Penny',pet:'gator'},preferences:{timezone:'UTC',weeklyBudgetMinor:null,currency:'USD'},applied:null,history:[]}));await first.flush()
 first.setItem(doneKey,'true');await first.flush()
 const second=new CloudStore('user:a',server);await second.hydrate();assert.equal(second.getItem(doneKey),'true')
 const {DemoMailbox}=await import('../src/egg/transport.ts');const mailbox=new DemoMailbox('user:a',second);mailbox.setCompanion({name:'Mochi',pet:'duck'});await second.flush()
 const third=new CloudStore('user:a',server);await third.hydrate();assert.equal(third.getItem(doneKey),'true');assert.equal(new DemoMailbox('user:a',third).snapshot().companion?.name,'Mochi')
})
