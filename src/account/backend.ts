import { supabase } from '../auth'
import { accountRequestError, AccountSessionError } from './backend-errors'
import type { CloudBackend, CloudRow } from './cloud-store'

export function accountBackend(userId:string):CloudBackend {
  if(!supabase)throw Error('Account service is not configured.')
  const client=supabase
  async function request(path:string,body?:unknown) {
    const {data:{session}}=await client.auth.getSession()
    if(!session||session.user.id!==userId)throw new AccountSessionError()
    const response=await fetch(import.meta.env.VITE_SUPABASE_URL.replace(/\/$/,'')+'/rest/v1/'+path,{
      method:body===undefined?'GET':'POST',
      headers:{apikey:import.meta.env.VITE_SUPABASE_PUBLISHABLE_KEY,Authorization:'Bearer '+session.access_token,'Content-Type':'application/json'},
      body:body===undefined?undefined:JSON.stringify(body),
      signal:AbortSignal.timeout(15000),
    })
    // Proxies can return a non-JSON failure; keep it in the safe error path.
    const data=await response.json().catch(()=>null)
    if(!response.ok)throw accountRequestError(response.status,data)
    if(data===null)throw Error('Cloud account response was invalid.')
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
