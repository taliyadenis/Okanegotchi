import { supabase } from '../auth'
import { SaveConflict } from './cloud-store'
import type { CloudBackend, CloudRow } from './cloud-store'

export function accountBackend(userId:string):CloudBackend {
  if(!supabase)throw Error('Account service is not configured.')
  const client=supabase
  async function request(path:string,body?:unknown) {
    const {data:{session}}=await client.auth.getSession()
    if(!session||session.user.id!==userId)throw Error('Account session changed.')
    const response=await fetch(import.meta.env.VITE_SUPABASE_URL.replace(/\/$/,'')+'/rest/v1/'+path,{
      method:body===undefined?'GET':'POST',
      headers:{apikey:import.meta.env.VITE_SUPABASE_PUBLISHABLE_KEY,Authorization:'Bearer '+session.access_token,'Content-Type':'application/json'},
      body:body===undefined?undefined:JSON.stringify(body),
      signal:AbortSignal.timeout(15000),
    })
    const data=await response.json()
    if(!response.ok){if(data.code==='40001')throw new SaveConflict('Another device has saved newer data.');throw Error('Cloud account request failed.')}
    return data
  }
  return {
    async load() {
      const data=await request('account_documents?select=kind,value,revision&user_id=eq.'+encodeURIComponent(userId))
      return (data as CloudRow[]).map(row=>({...row,revision:Number(row.revision)}))
    },
    async save(kind,value,revision) {
      // Ownership comes from the JWT in Postgres, never from a submitted owner ID.
      const data=await request('rpc/save_account_document',{document_kind:kind,document_value:value,expected_revision:revision})
      return Number(data)
    },
  }
}
