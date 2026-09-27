// Reconstruct only the schema exported read-only on 2026-09-27. No hosted row data.
import {readFile} from 'node:fs/promises'
const json=async name=>JSON.parse((await readFile(new URL(name,import.meta.url),'utf8')).replace(/^\uFEFF/,''))
const quote=name=>'"'+name.replaceAll('"','""')+'"'
export async function observedSchemaSql() {
  const meta=await json('observed-metadata.json'),functions=await json('observed-functions.json')
  const statements=[]
  for(const table of ['account_documents','registered_devices']) {
    const columns=meta.columns.filter(c=>c.table_name===table).sort((a,b)=>a.ordinal_position-b.ordinal_position)
    statements.push(`create table public.${quote(table)} (${columns.map(c=>
      `${quote(c.column_name)} ${c.udt_name}${c.column_default?' default '+c.column_default:''}${c.is_nullable==='NO'?' not null':''}`
    ).join(',')});`)
  }
  for(const c of meta.constraints)statements.push(`alter table public.${quote(c.table)} add constraint ${quote(c.name)} ${c.definition};`)
  for(const f of functions)statements.push(f.definition+';')
  for(const t of meta.triggers)statements.push(t.definition+';')
  for(const r of meta.rls)if(r.enabled)statements.push(`alter table public.${quote(r.table)} enable row level security;`)
  for(const p of meta.policies)statements.push(`create policy ${quote(p.policyname)} on public.${quote(p.tablename)} as ${p.permissive} for ${p.cmd} to ${p.roles.map(quote).join(',')}${p.qual?' using ('+p.qual+')':''}${p.with_check?' with check ('+p.with_check+')':''};`)
  for(const g of meta.table_grants)if(g.grantee!=='postgres')statements.push(`grant ${g.privilege_type} on public.${quote(g.table_name)} to ${quote(g.grantee)};`)
  // Exported non-trigger functions revoked PUBLIC but grant these three Supabase roles.
  for(const signature of ['load_account_document(text)','save_account_document(text,bigint,jsonb)','register_device(uuid,bytea,boolean)','revoke_device(uuid)']) {
    statements.push(`revoke all on function public.${signature} from public;`)
    statements.push(`grant execute on function public.${signature} to anon,authenticated,service_role;`)
  }
  return statements.join('\n')
}
