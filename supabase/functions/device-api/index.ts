import {createClient} from 'npm:@supabase/supabase-js@2'
import {boundedJson,hashToken,json,uuid} from '../_shared/http.ts'
import {deviceState,validateSync} from '../_shared/device-state.ts'
const admin=createClient(Deno.env.get('SUPABASE_URL')!,Deno.env.get('SUPABASE_SERVICE_ROLE_KEY')!,{auth:{persistSession:false,autoRefreshToken:false}})
Deno.serve(async req=>{
  let requestId:string|null=null
  const fail=(code:string,message:string,status:number,epoch:string|null=null)=>json({api_version:1,request_id:requestId,error:{code,message,current_epoch:epoch,retry_after_ms:null}},status)
  if(req.method!=='POST'||!new URL(req.url).pathname.endsWith('/v1/sync'))return fail('BAD_REQUEST','POST /v1/sync required',400)
  const token=req.headers.get('authorization')?.match(/^Bearer ([A-Za-z0-9_-]{43})$/)?.[1]
  if(!token)return fail('UNAUTHORIZED','Invalid device credentials',401)
  let body:unknown
  try{body=await boundedJson(req);if(uuid((body as any)?.request_id))requestId=(body as any).request_id;validateSync(body)}catch{return fail('BAD_REQUEST','Invalid or oversized sync request',400)}
  validateSync(body)
  if(body.telemetry.asset_version!==1)return fail('UPDATE_REQUIRED','Asset version 1 required',426)
  try{
    const secretHash=await hashToken(token)
    // Authenticate before reporting feature availability, and before domain access.
    const {data:device,error:authError}=await admin.from('registered_devices').select('id').eq('token_hash',secretHash).eq('id',body.device_id).is('revoked_at',null).maybeSingle()
    if(authError)throw authError
    if(!device)return fail('UNAUTHORIZED','Invalid device credentials',401)
    if(body.events.length)return fail('INTERNAL_ERROR','Device actions are not enabled in this state-sync milestone; retain the outbox.',501)
    const {data,error}=await admin.rpc('read_device_sync',{secret_hash:secretHash,requested_device:body.device_id,requested_epoch:body.epoch,ack:body.ack_command_seq,firmware_version:body.telemetry.firmware,supported_assets:body.telemetry.asset_version})
    if(error)throw error
    if(data.error)return fail(data.error,data.error==='RESET_REQUIRED'?'Bootstrap with current epoch':'Sync rejected',data.error==='UNAUTHORIZED'?401:409,data.error==='RESET_REQUIRED'?data.epoch:null)
    const response=deviceState(body,data)
    if(new TextEncoder().encode(JSON.stringify(response)).length>8192)throw Error('Response too large')
    return json(response)
  }catch{return fail('INTERNAL_ERROR','Sync temporarily unavailable',503)}
})
