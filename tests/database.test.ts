import {test} from 'node:test'
import assert from 'node:assert/strict'
import {readFile} from 'node:fs/promises'
import {PGlite} from '@electric-sql/pglite'

test('database enforces ownership, revisions, device tokens and revocation',async()=>{
  const db=new PGlite()
  try {
    await db.exec(`create role anon;create role authenticated;create role service_role;
      create schema auth;create table auth.users(id uuid primary key);
      create function auth.uid() returns uuid language sql as $$ select nullif(current_setting('request.jwt.claim.sub',true),'')::uuid $$;
      grant usage on schema auth to authenticated,anon;grant execute on function auth.uid() to authenticated,anon;`)
    for(const file of ['202609260002_account_documents.sql','202609260003_device_connection.sql'])await db.exec(await readFile(new URL('../supabase/migrations/'+file,import.meta.url),'utf8'))
    const a='11111111-1111-4111-8111-111111111111',b='22222222-2222-4222-8222-222222222222'
    await db.query('insert into auth.users values($1),($2)',[a,b])
    await db.exec('set role authenticated');await db.query("select set_config('request.jwt.claim.sub',$1,false)",[a])
    const setup=JSON.stringify({version:4,companion:{pet:'gator',name:'Mint'},preferences:{timezone:'America/New_York',weeklyBudgetMinor:null,currency:'USD'},history:[],revision:0,applied:null})
    const save=(value:string,revision:number)=>db.query('select public.save_account_document($1,$2,$3) as revision',['setup',value,revision])
    assert.equal((await save(setup,0)).rows[0].revision,1)
    assert.equal((await save(setup,0)).rows[0].revision,1,'lost response retry is idempotent')
    await assert.rejects(save(setup.replace('Mint','Other'),0),/another device/)
    await assert.rejects(db.query("insert into public.account_documents(user_id,kind,value) values($1,'goal','{}')",[b]),/permission denied/)
    await db.query("select set_config('request.jwt.claim.sub',$1,false)",[b])
    assert.equal((await db.query('select * from public.account_documents')).rows.length,0)
    await save(setup.replace('Mint','Second'),0)
    assert.equal((await db.query('select * from public.account_documents')).rows.length,1)
    await db.exec('set role anon')
    await assert.rejects(db.query('select * from public.account_documents'),/permission denied/)
    await assert.rejects(save(setup,0),/permission denied/)
    await db.exec('reset role')
    const hash='a'.repeat(64),otherHash='b'.repeat(64)
    const id=(await db.query('select public.register_owned_device($1,$2) as id',[a,hash])).rows[0].id
    await assert.rejects(db.query('select public.register_owned_device($1,$2)',[a,otherHash]),/unique/)
    const sync=(secret:string,epoch:string|null=null)=>db.query('select public.read_device_sync($1,$2,$3,0,$4,1) as data',[secret,id,epoch,'0.1.0'])
    assert.equal((await sync(otherHash)).rows[0].data.error,'UNAUTHORIZED')
    const first=(await sync(hash)).rows[0].data
    assert.equal(first.documents.setup.value.companion.name,'Mint')
    assert.equal((await sync(hash,b)).rows[0].data.error,'RESET_REQUIRED')
    assert.equal((await sync(hash,first.epoch)).rows[0].data.state_version,2)
    await db.query('update public.registered_devices set revoked_at=now() where id=$1',[id])
    assert.equal((await sync(hash)).rows[0].data.error,'UNAUTHORIZED')
    await db.exec('set role authenticated')
    await assert.rejects(db.query('select token_hash from public.registered_devices'),/permission denied/)
    await assert.rejects(db.query('select public.register_owned_device($1,$2)',[a,otherHash]),/permission denied/)
  }finally{await db.close()}
})
