import { useEffect, useState } from 'react'
import type { ReactNode } from 'react'
import { accountBackend } from './backend'
import { attachStorage, CloudStore, detachStorage } from './cloud-store'
import { DemoMailbox } from '../egg/transport'

export function AccountGate({userId,onReady,onLogout,children}:{userId:string;onReady:(complete:boolean)=>void;onLogout:()=>void;children:ReactNode}) {
  const [store,setStore]=useState<CloudStore|null>(null)
  const [status,setStatus]=useState('loading')
  const [attempt,setAttempt]=useState(0)
  const [error,setError]=useState('')
  useEffect(()=>{
    let active=true
    const next=new CloudStore('user:'+userId,accountBackend(userId))
    setStore(null);setStatus('loading');setError('')
    const unsubscribe=next.subscribe(()=>{if(active)setStatus(next.status)})
    void next.hydrate().then(()=>{
      if(!active)return
      attachStorage(next);setStore(next)
      const saved=new DemoMailbox(next.identity,next).snapshot()
      onReady(!!saved.companion&&!!saved.preferences)
    }).catch(()=>{if(active){setStatus('error');setError('We could not load your saved account. Check your connection and try again. If this is the first deployment, apply the account database migration.')}})
    return()=>{active=false;unsubscribe();detachStorage(next)}
  },[userId,attempt])
  useEffect(()=>{
    const preventLoss=(e:BeforeUnloadEvent)=>{if(store?.hasUnsaved()){e.preventDefault();e.returnValue=''}}
    const reconnect=()=>{if(store?.status==='error')void store.flush().catch(()=>{})}
    window.addEventListener('beforeunload',preventLoss);window.addEventListener('online',reconnect)
    return()=>{window.removeEventListener('beforeunload',preventLoss);window.removeEventListener('online',reconnect)}
  },[store])
  const reload=()=>{
    if(store?.hasUnsaved()&&!window.confirm('Discard unsaved changes from this tab and load the latest cloud copy?'))return
    setAttempt(n=>n+1)
  }
  if(!store)return <main className="form-card"><h2>Your Account</h2><p role="status">{error||'Loading your saved companion…'}</p>{error&&<button className="primary-button" onClick={()=>setAttempt(n=>n+1)}>Retry loading</button>}<button className="text-button" onClick={onLogout}>Log out</button></main>
  return <><aside className="account-sync" aria-label="Account sync"><p role="status">{status==='saved'?'Saved to your account':status==='saving'?'Saving to your account…':status==='conflict'?'Another browser saved newer data. Your changes have not been saved.':'Cloud save failed. Keep this tab open and retry.'}</p>{status==='error'&&<button className="small-button" onClick={()=>void store.flush().catch(()=>{})}>Retry save</button>}<button className="text-button" onClick={reload}>Load latest saved data</button><button className="text-button" onClick={()=>{if(!store.hasUnsaved()||window.confirm('Some changes are not saved. Log out anyway?'))onLogout()}}>Log out</button></aside><div key={attempt}>{children}</div></>
}
