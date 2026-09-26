import { test } from 'node:test'
import assert from 'node:assert/strict'
import example from '../docs/examples/demo-letter.json' with { type:'json' }
import { parsePacket,serializePacket,validatePacket,validateReceipt } from '../src/egg/contract.ts'
import type { Packet } from '../src/egg/contract.ts'
import { DemoMailbox,mailboxKey } from '../src/egg/transport.ts'
import { sendLetter,bounded } from '../src/egg/send.ts'
const packet=(revision=1):Packet=>({...structuredClone(example),messageId:`test-${revision}`,revision,goal:{...example.goal,revision}} as Packet)
const memory=()=>{const map=new Map<string,string>();return {getItem:(k:string)=>map.get(k)??null,setItem:(k:string,v:string)=>{map.set(k,v)}}}
test('sample JSON matches runtime schema and round-trips Unicode and quotes',()=>{validatePacket(example);const p=packet();p.companion.name='小猫 "Penny"';assert.deepEqual(parsePacket(serializePacket(p)),p)})
test('rejects unknown/private fields, versions, assets, fractions, nonfinite and malformed dates',()=>{for(const change of [(p:any)=>p.secret='no',(p:any)=>p.schema='v9',(p:any)=>p.companion.pet='gator',(p:any)=>p.goal.savedMinor=1.2,(p:any)=>p.goal.targetMinor=NaN,(p:any)=>p.createdAt='2026-02-30T00:00:00.000Z',(p:any)=>p.companion.name=' '.repeat(3),(p:any)=>p.goal.currency='EUR']){const p=packet();change(p);assert.throws(()=>serializePacket(p))}})
test('UTF-8 bytes are bounded independently of character count',()=>{assert.throws(()=>parsePacket(' '.repeat(2049)));const p=packet();p.goal.name='🐱'.repeat(61);assert.throws(()=>serializePacket(p));p.goal.name='🐱'.repeat(60);assert.doesNotThrow(()=>serializePacket(p))})
test('queued is distinct from validated applied receipt; source packet cannot mutate saved data',async()=>{const m=new DemoMailbox('guest',memory());const p=packet();assert.equal((await m.submit(p,'online')).status,'queued');assert.equal(m.snapshot().applied,null);p.goal.savedMinor=1;const d=await m.status('test-1');assert.equal(d.status,'received');assert.equal(m.snapshot().applied?.goal.savedMinor,380000)})
test('duplicate submissions and receipts are idempotent',async()=>{const m=new DemoMailbox('guest',memory());await Promise.all([m.submit(packet(),'duplicate'),m.submit(packet(),'duplicate')]);await m.status('test-1');await m.status('test-1');assert.equal(m.snapshot().history.length,1);assert.equal(m.snapshot().applied?.revision,1)})
test('same ID cannot change payload and old revision cannot replace applied state',async()=>{const m=new DemoMailbox('g',memory());await m.submit(packet(),'online');await m.status('test-1');const changed=packet();changed.goal.name='changed';await assert.rejects(m.submit(changed,'online'));await m.submit(packet(2),'online');await m.status('test-2');assert.throws(()=>m.acceptReceipt({messageId:'test-1',destination:'demo-egg',appliedRevision:1,outcome:'applied'},m.snapshot().history[1]));assert.equal(m.snapshot().applied?.revision,2)})
test('receipts must match message, destination, revision and outcome',()=>{const p=packet();const r={messageId:p.messageId,destination:'demo-egg',appliedRevision:1,outcome:'applied'};validateReceipt(r,p);for(const change of [{messageId:'other'},{destination:'physical'},{appliedRevision:0},{outcome:'queued'},{extra:true}])assert.throws(()=>validateReceipt({...r,...change},p))})
test('offline and timeout survive reload; explicit reconnect resumes validation',async()=>{for(const mode of ['offline','timeout'] as const){const s=memory();let m=new DemoMailbox('g',s);await m.submit(packet(),mode);await m.status('test-1');assert.equal(m.snapshot().applied,null);await assert.rejects(m.submit(packet(2),'online'));m=new DemoMailbox('g',s);assert.equal((await m.status('test-1',true)).status,'received')}})
test('failed enqueue/schema rejection retry with same immutable ID',async()=>{for(const mode of ['failure','reject'] as const){const m=new DemoMailbox('g',memory());await m.submit(packet(),mode);await m.status('test-1');assert.equal(m.snapshot().history[0].status,'failed');await m.submit(packet(),'online');await m.status('test-1');assert.equal(m.snapshot().history.length,1);assert.equal(m.snapshot().applied?.messageId,'test-1')}})
test('history bounded and identities isolated',async()=>{const s=memory();const m=new DemoMailbox('g',s);for(let i=1;i<=10;i++){await m.submit(packet(i),'online');await m.status(`test-${i}`)}assert.equal(m.snapshot().history.length,8);assert.equal(new DemoMailbox('user',s).snapshot().history.length,0)})
test('malformed storage is discarded and unavailable storage remains functional',async()=>{const s=memory();s.setItem(mailboxKey('g'),'{');assert.equal(new DemoMailbox('g',s).snapshot().revision,0);const m=new DemoMailbox('g',{getItem:()=>{throw Error()},setItem:()=>{throw Error()}});await m.submit(packet(),'online');await m.status('test-1');assert.equal(m.persistent,false);assert.equal(m.snapshot().applied?.revision,1)})
test('cancel packing never submits, leaving after dispatch retains queued letter',async()=>{const m=new DemoMailbox('g',memory());const c=new AbortController();await assert.rejects(sendLetter(m,packet(),'online',c.signal,()=>{},false,async()=>{c.abort()}));assert.equal(m.snapshot().history.length,0);const c2=new AbortController();let waits=0;await sendLetter(m,packet(),'online',c2.signal,()=>{},false,async()=>{if(++waits===2)c2.abort()});assert.equal(m.snapshot().history[0].status,'queued')})
test('animation phases are separate from applied validation and no source mutation',async()=>{const p=packet();const before=serializePacket(p);const phases:string[]=[];const m=new DemoMailbox('g',memory());const result=await sendLetter(m,p,'online',new AbortController().signal,x=>phases.push(x),true,async()=>{});assert.deepEqual(phases,['packing','submitting','flying']);assert.equal(result.status,'received');assert.equal(serializePacket(p),before)})
test('transport timeout is bounded and reported unknown',async()=>{await assert.rejects(bounded(new Promise(()=>{}),5),/unknown/)})


test('preferences reject invalid zones and budgets while allowing no budget',()=>{
  for(const preferences of [{timezone:'Mars/Olympus',weeklyBudgetMinor:100,currency:'USD'},{timezone:'UTC',weeklyBudgetMinor:0,currency:'USD'},{timezone:'UTC',weeklyBudgetMinor:1.5,currency:'USD'}]) {
    assert.throws(()=>serializePacket({...packet(),preferences}))
  }
  assert.doesNotThrow(()=>serializePacket({...packet(),preferences:{timezone:'UTC',weeklyBudgetMinor:null,currency:'USD'}}))
})
test('setup preferences persist by identity and edits do not change a queued letter',async()=>{
  const s=memory();const m=new DemoMailbox('preferences',s);const p=packet();
  m.setSetup(p.companion,p.preferences);await m.submit(p,'offline');
  m.setSetup(p.companion,{timezone:'Europe/London',weeklyBudgetMinor:null,currency:'USD'});
  const restored=new DemoMailbox('preferences',s).snapshot();
  assert.equal(restored.preferences?.timezone,'Europe/London');
  assert.equal(restored.history[0].packet.preferences.timezone,'America/New_York');
  assert.equal(new DemoMailbox('other',s).snapshot().preferences,null)
})
test('legacy setup keeps pet but requires reviewing new preferences without rewriting old letters',()=>{
  const s=memory();const legacy=JSON.stringify({version:3,companion:{name:'Mochi',pet:'cat'},history:[{old:true}]});
  const key=mailboxKey('legacy').replace(':v4:',':v3:');s.setItem(key,legacy);
  const m=new DemoMailbox('legacy',s);
  assert.deepEqual(m.snapshot().companion,{name:'Mochi',pet:'cat'});
  assert.equal(m.snapshot().preferences,null);assert.equal(m.snapshot().history.length,0);assert.equal(s.getItem(key),legacy)
})
