import { DeviceConnection as RegisteredDevice } from '../account/DeviceConnection'
import { useState } from 'react'
import { supabase } from '../auth'
export function DeviceConnection({name='okanegotchi',identity='guest'}:{name?:string;identity?:string}) {
  const [open,setOpen]=useState(false),[busy,setBusy]=useState(false),[status,setStatus]=useState('Not connected — no verified device acknowledgement.')
  async function check(){setBusy(true);try{
    const session=(await supabase?.auth.getSession())?.data.session
    if(!session){setStatus('Sign in to register a device. Your saved character is ready.');return}
    const response=await fetch(`${import.meta.env.VITE_SUPABASE_URL?.replace(/\/$/,'')}/functions/v1/app-api/v1/dashboard`,{headers:{Authorization:`Bearer ${session.access_token}`,apikey:import.meta.env.VITE_SUPABASE_PUBLISHABLE_KEY ?? ''},signal:AbortSignal.timeout(10000)})
    setStatus(response.ok?'Account service reached. Device registration and a compatible firmware acknowledgement still need verification.':response.status===401?'Your session could not authorize device access. Sign in again.':'Device service is unavailable. The integration team must deploy and verify registration first.')
  }catch{setStatus('Unable to reach the device service. Check your connection or try again after deployment.')}finally{setBusy(false)}}
  return <section className="device-connection"><h2>Connect {name}</h2><p role="status">{status}</p><button className="small-button" onClick={()=>setOpen(true)}>Connect device</button>{open&&identity.startsWith('user:')?<RegisteredDevice userId={identity.slice(5)} name={name}/>:open&&<div><p>Device connection is not ready yet. Check the service when you’re ready.</p><button className="small-button" disabled={busy} onClick={()=>void check()}>{busy?'Checking service…':'Check connection service'}</button></div>}</section>
}
