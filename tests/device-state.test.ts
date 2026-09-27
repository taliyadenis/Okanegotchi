import {test} from 'node:test'
import assert from 'node:assert/strict'
import {deviceState,validateSync} from '../supabase/functions/_shared/device-state.ts'
import {boundedJson,hashToken,newToken} from '../supabase/functions/_shared/http.ts'
const req=()=>({api_version:1,request_id:'11111111-1111-4111-8111-111111111111',device_id:'22222222-2222-4222-8222-222222222222',epoch:null,ack_command_seq:0,events:[],telemetry:{firmware:'0.1.0',asset_version:1,uptime_s:10,rssi_dbm:-40}})
test('bootstrap validation rejects extra fields, mixed epochs and invalid telemetry',()=>{
  validateSync(req())
  for(const change of [(r:any)=>r.extra=true,(r:any)=>r.ack_command_seq=1,(r:any)=>r.events=[{}],(r:any)=>r.telemetry.rssi_dbm=1,(r:any)=>r.device_id='other']){
    const r=req();change(r);assert.throws(()=>validateSync(r))
  }
})
test('device snapshot uses selected sample accounts and current saved companion',()=>{
  const r=req();validateSync(r)
  const result={epoch:r.device_id,state_version:3,documents:{setup:{value:{companion:{pet:'duck',name:'Sunny'},preferences:{timezone:'America/New_York',weeklyBudgetMinor:null}}},finance:{value:{connections:[{institutionId:'clover',accountIds:['clover-savings']}]}},goal:{value:{goal:{name:'Trip',target:500000,accountId:'clover-savings'}}}}}
  const response=deviceState(r,result,new Date('2026-09-26T15:00:00Z'))
  assert.equal(response.pet.pet_id,'duck');assert.equal(response.goal?.saved_minor,380000)
  assert.equal(response.finance.spend_minor,0);assert.equal(response.finance.data_source,'demo')
  assert.equal(response.care.current_window,'AM');assert.equal(response.next_sync_ms,30000)
  assert.deepEqual(response.commands,[]);assert.equal(response.review,null)
  assert.ok(new TextEncoder().encode(JSON.stringify(response)).length<8192)
})
test('device credentials are random 32-byte secrets stored as SHA256',async()=>{
  const a=newToken(),b=newToken();assert.match(a,/^[A-Za-z0-9_-]{43}$/);assert.notEqual(a,b)
  assert.match(await hashToken(a),/^[a-f0-9]{64}$/);assert.notEqual(await hashToken(a),await hashToken(b))
})
test('request reader bounds chunked bodies',async()=>{
  const request=new Request('https://example.test',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({large:'x'.repeat(4200)})})
  await assert.rejects(boundedJson(request),/too large/)
})
