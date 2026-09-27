import { weeklySummary } from '../../../src/checkins/model.ts'
import { accounts, transactions } from '../../../src/finance/fixtures.ts'
import {createClient} from 'npm:@supabase/supabase-js@2'
import {boundedJson,hashToken,json,newToken,uuid} from '../_shared/http.ts'
const url=Deno.env.get('SUPABASE_URL')!
const admin=createClient(url,Deno.env.get('SUPABASE_SERVICE_ROLE_KEY')!,{auth:{persistSession:false,autoRefreshToken:false}})
Deno.serve(async req=>{
  const origin=req.headers.get('origin')??''
  const allowed=(Deno.env.get('ALLOWED_ORIGINS')??'http://127.0.0.1:5173,http://localhost:5173').split(',')
  const headers:Record<string,string>=allowed.includes(origin)?{'Access-Control-Allow-Origin':origin,'Vary':'Origin','Access-Control-Allow-Headers':'authorization,apikey,content-type','Access-Control-Allow-Methods':'GET,POST,DELETE,OPTIONS'}:{}
  if(origin&&!allowed.includes(origin))return json({error:'Origin not allowed'},403)
  if(req.method==='OPTIONS')return new Response(null,{status:204,headers})
  const respond=(body:unknown,status=200)=>json(body,status,headers)
  try {
    const token=req.headers.get('authorization')?.match(/^Bearer (.+)$/i)?.[1]
    if(!token)return respond({error:'Sign in required'},401)
    const {data,error}=await admin.auth.getUser(token)
    if(error||!data.user)return respond({error:'Sign in required'},401)
    const owner=data.user.id,path=new URL(req.url).pathname
    if(req.method==='POST'&&path.endsWith('/v1/checkins/review')) {
      let body:any
      try {body=await boundedJson(req);if(!body||Object.keys(body).join()!=='request_id'||!uuid(body.request_id))throw Error()}catch{return respond({error:'A valid review request ID is required.'},400)}
      const {data:rows,error}=await admin.from('account_documents').select('kind,value').eq('user_id',owner).in('kind',['setup','finance']);if(error)throw error
      const setup=JSON.parse(rows?.find(row=>row.kind==='setup')?.value??'null'),finance=JSON.parse(rows?.find(row=>row.kind==='finance')?.value??'null')
      if(!setup?.preferences)return respond({error:'Save your timezone and preferences first.'},409)
      const connections=Array.isArray(finance?.connections)?finance.connections:[]
      const ids=new Set(connections.flatMap((c:any)=>Array.isArray(c.accountIds)?c.accountIds.filter((id:unknown)=>accounts.some(a=>a.id===id&&a.institutionId===c.institutionId)):[]))
      const selected=accounts.filter(a=>ids.has(a.id));if(!selected.length)return respond({error:'Connect an account before reviewing.'},409)
      const snapshot=weeklySummary({accounts:selected,transactions:transactions.filter(t=>ids.has(t.accountId)),connections},setup.preferences.timezone,setup.preferences.weeklyBudgetMinor)
      const {data:review,error:reviewError}=await admin.rpc('create_web_financial_review',{owner_id:owner,request_id:body.request_id,financial_snapshot:snapshot});if(reviewError)throw reviewError
      return respond(review)
    }
    if(req.method==='POST'&&path.endsWith('/v1/devices')) {
      const secret=newToken()
      const {data:id,error}=await admin.rpc('register_owned_device',{owner_id:owner,secret_hash:await hashToken(secret)})
      if(error)return respond({error:error.code==='23505'?'Revoke your existing device before registering another.':'Complete setup before registering a device.'},409)
      return respond({device_id:id,token:secret,api_base:url+'/functions/v1/device-api',sync_url:url+'/functions/v1/device-api/v1/sync'},201)
    }
    if(req.method==='GET'&&path.endsWith('/v1/devices')) {
      const {data,error}=await admin.from('registered_devices').select('id,created_at,last_seen,firmware,asset_version').eq('user_id',owner).eq('is_revoked',false).is('revoked_at',null)
      if(error)throw error
      return respond({devices:data})
    }
    const id=path.split('/').pop()
    if(req.method==='DELETE'&&path.includes('/v1/devices/')&&uuid(id)) {
      const {error}=await admin.from('registered_devices').update({is_revoked:true,revoked_at:new Date().toISOString()}).eq('id',id).eq('user_id',owner).eq('is_revoked',false).is('revoked_at',null)
      if(error)throw error
      return respond({revoked:true})
    }
    return respond({error:'Not found'},404)
  }catch{return respond({error:'Account service unavailable'},503)}
})
