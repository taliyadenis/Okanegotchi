import {useEffect,useState} from 'react'
import {supabase} from '../auth'
type Device={id:string;last_seen:string|null;firmware:string|null}
type Credentials={device_id:string;token:string;api_base:string;sync_url:string}
export function DeviceConnection({userId,name='okanegotchi'}:{userId:string;name?:string}) {
  const [devices,setDevices]=useState<Device[]>([])
  const [credentials,setCredentials]=useState<Credentials|null>(null)
  const [busy,setBusy]=useState(false),[error,setError]=useState('')
  async function request(method:string,path='') {
    const {data:{session}}=await supabase!.auth.getSession()
    if(!session||session.user.id!==userId)throw Error('Sign in again.')
    const response=await fetch(import.meta.env.VITE_SUPABASE_URL+'/functions/v1/app-api/v1/devices'+path,{method,headers:{Authorization:'Bearer '+session.access_token,apikey:import.meta.env.VITE_SUPABASE_PUBLISHABLE_KEY},signal:AbortSignal.timeout(15000)})
    const data=await response.json()
    if(!response.ok)throw Error(data.error??'Device service is unavailable.')
    return data
  }
  async function refresh(){const data=await request('GET');setDevices(data.devices)}
  async function run(action:()=>Promise<void>){setBusy(true);setError('');try{await action()}catch(e){setError(e instanceof Error?e.message:'Device request failed.')}finally{setBusy(false)}}
  useEffect(()=>{void run(refresh)},[userId])
  return <section className="device-connection"><h2>{name} · device registration</h2>
    <p>Register one device for your account, then provision it over USB with your hardware teammate. State sync requires the deployed API and compatible firmware.</p>
    <p>Connection milestone: saved companion, preferences and sample financial summary. Device actions, care and reaction commands are not enabled yet.</p>
    {error&&<p role="alert" className="auth-error">{error}</p>}
    {devices.map(d=><div key={d.id}><p>Device ID: <code>{d.id}</code></p><p>Last contact: {d.last_seen?new Date(d.last_seen).toLocaleString():'Not connected yet'} · Firmware: {d.firmware??'Unknown'}</p><button className="small-button" disabled={busy} onClick={()=>{if(window.confirm('Revoke this device’s access? It will need new credentials to reconnect.'))void run(async()=>{await request('DELETE','/'+d.id);setCredentials(null);await refresh()})}}>Revoke device</button></div>)}
    {!devices.length&&<button className="small-button" disabled={busy} onClick={()=>void run(async()=>{setCredentials(await request('POST'));await refresh()})}>Register my device</button>}
    <button className="text-button" disabled={busy} onClick={()=>void run(refresh)}>Refresh device status</button>
    {credentials&&<div className="auth-notice"><strong>Provisioning credentials — shown once</strong><p>Keep the token private. It grants access to this device’s state. If lost, revoke and register again.</p><label>Device ID<input readOnly value={credentials.device_id}/></label><label>API base<input readOnly value={credentials.api_base}/></label><label>Device token<input type="password" readOnly value={credentials.token} onFocus={e=>e.currentTarget.select()}/></label><button className="text-button" onClick={()=>setCredentials(null)}>I’ve stored the credentials — hide them</button></div>}
  </section>
}
