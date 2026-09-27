import { supabase } from '../auth'
import { SaveConflict } from './cloud-store'
import type { CloudBackend, CloudRow, DocumentKind } from './cloud-store'

export class AccountSchemaError extends Error {
  constructor() { super('You’re signed in, but saved accounts are temporarily unavailable because the database update is pending. Please try again after the update.'); this.name='AccountSchemaError' }
}
export class AccountSaveError extends Error {
  constructor(message:string) { super(message); this.name='AccountSaveError' }
}

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
    if(!response.ok){
      if(data.code==='40001')throw new SaveConflict('Another device has saved newer data.')
      if(data.code==='42703'||data.code==='PGRST202')throw new AccountSchemaError()
      if(body!==undefined && data.code==='22023')throw new AccountSaveError('The account setup was rejected by the hosted database. Check the pet name, budget, and timezone, then try again.')
      throw Error('Cloud account request failed.')
    }
    return data
  }
  return {
    async load() {
      // The hosted project currently uses owner/document_key/content. Keep this
      // adapter until the reviewed alignment migration is deployed.
      const data=await request('account_documents?select=document_key,content,revision&owner=eq.'+encodeURIComponent(userId))
      return (data as Array<{document_key:string,content:unknown,revision:number}>).map(row=>({
        kind:row.document_key as DocumentKind,
        value:typeof row.content==='string'?row.content:JSON.stringify(row.content),
        revision:Number(row.revision),
      } satisfies CloudRow))
    },
    async save(kind,value,revision) {
      // Ownership comes from the JWT in Postgres, never from a submitted owner ID.
      // The hosted RPC uses its live p_* contract and returns a one-row table.
      const data=await request('rpc/save_account_document',{p_document_key:kind,p_content:JSON.parse(value),p_expected_revision:revision})
      const row=Array.isArray(data)?data[0]:data
      return Number(row?.revision ?? row)
    },
  }
}
