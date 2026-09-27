import {readFile,writeFile,mkdir,readdir} from 'node:fs/promises'
import {fileURLToPath} from 'node:url'
import {resolve} from 'node:path'

const root = new URL('../../',import.meta.url)
const read = path => readFile(new URL(path,root),'utf8')
const unwrap = sql => sql.replace(/^begin;\s*$/gmi,'').replace(/^commit;\s*$/gmi,'')
function after(sql,marker) {
  if(sql.split(marker).length!==2)throw Error(`Baseline changed: review ${marker}`)
  return sql.slice(sql.indexOf(marker))
}
export async function upgradeSql() {
  const files=(await readdir(new URL('supabase/migrations/',root))).filter(f=>f.endsWith('.sql')).sort()
  if(files.length!==4 || files[0]!=='202609260002_account_documents.sql' || files[1]!=='202609260003_device_connection.sql')
    throw Error('Migration list changed: review upgrade assembly before continuing')
  const account=after(await read('supabase/migrations/'+files[0]),'alter table public.account_documents enable row level security;')
  // Skip historical CREATE TABLE and its active-device index; the final migration installs
  // the index using preserved is_revoked status instead of invented revocation timestamps.
  const device=after(await read('supabase/migrations/'+files[1]),'alter table public.registered_devices enable row level security;')
  const parts=[await read('tools/backend/align-observed-schema.sql'),account,device,
    ...await Promise.all(files.slice(2).map(f=>read('supabase/migrations/'+f)))]
  return '-- REVIEWED UPGRADE CANDIDATE. No remote execution by this generator.\n' +
    '-- Requires private backup, current inventory, client cutover and deployment review.\n' +
    'begin;\nset local lock_timeout = \'5s\';\n' + parts.map(unwrap).join('\n')+'\ncommit;\n'
}
if(process.argv[1] && resolve(process.argv[1])===fileURLToPath(import.meta.url)) {
  const output=new URL('.local-backend/upgrade.sql',root)
  await mkdir(new URL('.local-backend/',root),{recursive:true})
  await writeFile(output,await upgradeSql())
  console.log('Prepared local upgrade candidate: '+fileURLToPath(output))
}
