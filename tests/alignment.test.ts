import {test} from 'node:test'
import assert from 'node:assert/strict'
import {readFile,readdir} from 'node:fs/promises'
import {PGlite} from '@electric-sql/pglite'
import {observedSchemaSql} from './fixtures/backend/observed-schema.mjs'
import {upgradeSql} from '../tools/backend/prepare-upgrade.mjs'

const a='11111111-1111-4111-8111-111111111111',b='22222222-2222-4222-8222-222222222222'
const device='33333333-3333-4333-8333-333333333333',rowId='44444444-4444-4444-8444-444444444444'
const oldDevice='55555555-5555-4555-8555-555555555555'
const hash='ab'.repeat(32),oldHash='cd'.repeat(32)
const setup={version:4,companion:{pet:'duck',name:'Test Duck'},preferences:{timezone:'America/New_York',weeklyBudgetMinor:12000,currency:'USD'},onboardingComplete:true,history:[{synthetic:true}],revision:6,applied:null}
async function base() {
  const db=new PGlite()
  await db.exec(`create role anon;create role authenticated;create role service_role;
    create schema auth;create table auth.users(id uuid primary key);
    create function auth.uid() returns uuid language sql as $$select nullif(current_setting('request.jwt.claim.sub',true),'')::uuid$$;
    grant usage on schema auth to anon,authenticated,service_role;
    grant execute on function auth.uid() to anon,authenticated,service_role;`)
  await db.query('insert into auth.users values ($1),($2)',[a,b])
  return db
}
async function legacy() {
  const db=await base();await db.exec(await observedSchemaSql())
  for(const [kind,content] of Object.entries({setup,finance:{version:1,connections:[{institutionId:'fixture',accountIds:['sample']} ]},goal:{version:1,name:'Synthetic fund',history:['keep']}}))
    await db.query("insert into account_documents(owner,document_key,revision,content,created_at,updated_at) values($1,$2,7,$3,'2026-01-02','2026-01-03')",[a,kind,JSON.stringify(content)])
  await db.query("insert into registered_devices(id,owner,device_id,device_token_sha256,last_seen_at) values($1,$2,$3,decode($4,'hex'),'2026-01-04')",[rowId,a,device,hash])
  await db.query("insert into registered_devices(owner,device_id,device_token_sha256,is_revoked) values($1,$2,decode($3,'hex'),true)",[a,oldDevice,oldHash])
  return db
}
async function identity(db:PGlite,id:string) {
  await db.exec('set role authenticated');await db.query("select set_config('request.jwt.claim.sub',$1,false)",[id])
}
async function checkApi(db:PGlite,revision:number) {
  await identity(db,a)
  const save=(value:unknown,rev:number)=>db.query('select public.save_account_document($1,$2,$3) as revision',['setup',JSON.stringify(value),rev])
  assert.equal((await save(setup,revision)).rows[0].revision,revision+1)
  // JSONB normalization changes whitespace/key order; retry remains the same operation.
  const reordered=Object.fromEntries(Object.entries(setup).reverse())
  assert.equal((await save(reordered,revision)).rows[0].revision,revision+1)
  await assert.rejects(save({...setup,companion:{pet:'gator',name:'Stale'}},revision),/another device/)
  await assert.rejects(db.exec("update account_documents set revision=99"),/permission denied/)
  await assert.rejects(db.exec('truncate account_documents cascade'),/permission denied/)
  await assert.rejects(db.exec('select token_hash from registered_devices'),/permission denied/)
  await identity(db,b)
  assert.equal((await db.query('select * from account_documents')).rows.length,0)
  await save({...setup,companion:{pet:'robot',name:'Other owner'}},0)
  await identity(db,a)
  assert.deepEqual(JSON.parse((await db.query("select value from account_documents where kind='setup'")).rows[0].value as string),setup)
  await db.exec('set role anon')
  await assert.rejects(db.query('select * from account_documents'),/permission denied/)
  await assert.rejects(save(setup,0),/permission denied/)
  await db.exec('reset role')
}
test('fresh migrations preserve account API and normalized retry semantics',async()=>{
  const db=await base()
  try {
    for(const f of (await readdir(new URL('../supabase/migrations/',import.meta.url))).filter(f=>f.endsWith('.sql')).sort())
      await db.exec(await readFile(new URL('../supabase/migrations/'+f,import.meta.url),'utf8'))
    await checkApi(db,0)
  }finally{await db.close()}
})
test('observed-schema upgrade preserves documents, identities, timestamps and revocation',async()=>{
  const db=await legacy()
  try {
    const before=(await db.query('select id,document_key,revision,content,created_at,updated_at from account_documents order by document_key')).rows
    await db.exec(await upgradeSql())
    const after=(await db.query('select id,kind as document_key,revision,value::jsonb as content,created_at,updated_at from account_documents order by kind')).rows
    assert.deepEqual(after,before)
    const devices=(await db.query('select id,legacy_row_id,token_hash,is_revoked,revoked_at,last_seen from registered_devices order by id')).rows
    assert.equal(devices[0].id,device);assert.equal(devices[0].legacy_row_id,rowId);assert.equal(devices[0].token_hash,hash)
    assert.equal(devices[1].is_revoked,true);assert.equal(devices[1].revoked_at,null,'do not invent historical revocation time')
    const sync=await db.query("select public.read_device_sync($1,$2,null,0,'0.1.0',1) as result",[oldHash,oldDevice])
    assert.equal((sync.rows[0].result as any).error,'UNAUTHORIZED')
    await assert.rejects(db.query('update registered_devices set is_revoked=false where id=$1',[oldDevice]),/cannot be reactivated/)
    await checkApi(db,7)
    await db.exec('set role authenticated')
    await assert.rejects(db.query("select backend_legacy.register_device($1,decode($2,'hex'),true)",[oldDevice,oldHash]),/permission denied/)
    await db.exec('reset role')
    await assert.rejects(db.exec(await upgradeSql()),/Unexpected account schema/)
    await db.exec('rollback')
    assert.equal((await db.query('select count(*)::int as count from account_documents')).rows[0].count,4)
  }finally{await db.close()}
})
test('unmapped documents and invalid credentials abort the upgrade without losing rows',async()=>{
  for(const invalid of ['key','hash','duplicate']) {
    const db=await legacy()
    try {
      if(invalid==='key')await db.exec("update account_documents set document_key='unknown-client-key' where document_key='goal'")
      if(invalid==='hash')await db.exec("update registered_devices set device_token_sha256=decode('aa','hex') where not is_revoked")
      if(invalid==='duplicate')await db.query("insert into registered_devices(owner,device_id,device_token_sha256) values($1,$2,decode($3,'hex'))",[b,device,'ef'.repeat(32)])
      const before=(await db.query('select * from account_documents order by document_key')).rows
      await assert.rejects(db.exec(await upgradeSql()),/Unmapped|SHA-256|Duplicate/)
      await db.exec('rollback')
      assert.deepEqual((await db.query('select * from account_documents order by document_key')).rows,before)
    }finally{await db.close()}
  }
})
