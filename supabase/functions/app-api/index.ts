import {createClient} from 'npm:@supabase/supabase-js@2'
import {hashToken,json,newToken,uuid} from '../_shared/http.ts'
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
    if(req.method==='POST'&&path.endsWith('/v1/devices')) {
      const secret=newToken()
      const {data:id,error}=await admin.rpc('register_owned_device',{owner_id:owner,secret_hash:await hashToken(secret)})
      if(error)return respond({error:error.code==='23505'?'Revoke your existing device before registering another.':'Complete setup before registering a device.'},409)
      return respond({device_id:id,token:secret,api_base:url+'/functions/v1/device-api',sync_url:url+'/functions/v1/device-api/v1/sync'},201)
    }
    if(req.method==='GET'&&path.endsWith('/v1/devices')) {
      const {data,error}=await admin.from('registered_devices').select('id,created_at,last_seen,firmware,asset_version').eq('user_id',owner).is('revoked_at',null)
      if(error)throw error
      return respond({devices:data})
    }
    const id=path.split('/').pop()
    if(req.method==='DELETE'&&path.includes('/v1/devices/')&&uuid(id)) {
      const {error}=await admin.from('registered_devices').update({revoked_at:new Date().toISOString()}).eq('id',id).eq('user_id',owner).is('revoked_at',null)
      if(error)throw error
      return respond({revoked:true})
    }
    return respond({error:'Not found'},404)
  }catch{return respond({error:'Account service unavailable'},503)}
})
