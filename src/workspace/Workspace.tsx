import { useEffect, useState } from 'react'
import { CompanionSetup } from '../egg/CompanionSetup'
import { DemoMailbox } from '../egg/transport'
import { DeviceConnection } from '../egg/DeviceConnection'
import { FinanceScreen } from '../finance/FinanceScreen'
import { DemoProvider } from '../finance/provider'
import { readGoal, goalProgress } from '../finance/goals'
import { money } from '../finance/fixtures'
import { browserStorage } from '../finance/storage'
import { completed, completionKey } from './state'
import { AccountGate } from '../account/AccountGate'
import { accountStorage } from '../account/cloud-store'
import './workspace.css'
type Page='Dashboard'|'Pet'|'Banks'|'Goals'|'Letters'
export function Workspace({identity,onLogout}:{identity:string;onLogout:()=>void}) {
 return identity.startsWith('user:')?<AccountGate userId={identity.slice(5)} onReady={()=>{}} onLogout={onLogout}><WorkspaceContent identity={identity} onLogout={onLogout}/></AccountGate>:<WorkspaceContent identity={identity} onLogout={onLogout}/>
}
function WorkspaceContent({identity,onLogout}:{identity:string;onLogout:()=>void}) {
 const storage=browserStorage(identity)
 const [done,setDone]=useState(()=>completed(storage,identity))
 const [page,setPage]=useState<Page>(done?'Dashboard':'Pet')
 const [revision,setRevision]=useState(0)
 const [created,setCreated]=useState(false)
 const [saveError,setSaveError]=useState('')
 const [finishing,setFinishing]=useState(false)
 const box=new DemoMailbox(identity,storage).snapshot(),name=box.companion?.name || 'okanegotchi'
 const data=new DemoProvider(identity,storage).snapshot(),goal=readGoal(storage,identity)
 const progress=goal?goalProgress(goal,data.accounts):null
 useEffect(()=>{const update=()=>setRevision(n=>n+1);window.addEventListener('workspace-change',update);return()=>window.removeEventListener('workspace-change',update)},[])
 useEffect(()=>{const h=document.querySelector<HTMLElement>('main h1');if(h){h.tabIndex=-1;h.focus()}},[page,created])
 async function finish(){if(finishing)return;setFinishing(true);setSaveError('');try{storage?.setItem(completionKey(identity),'true');await accountStorage(identity)?.flush();setDone(true);setPage('Dashboard')}catch{setSaveError('Setup completion could not be saved. Retry the cloud save before leaving.')}finally{setFinishing(false)}}
 function leave(){const store=accountStorage(identity);if(!store?.hasUnsaved()||window.confirm('Changes are not saved yet. Sign out anyway?'))onLogout()}
 function navigate(next:Page){setCreated(false);setPage(next)}
 return <><nav className="permanent-nav" aria-label="Main navigation">{(['Dashboard','Pet','Banks','Goals','Letters'] as Page[]).map(p=><button key={p} aria-current={page===p?'page':undefined} onClick={()=>navigate(p)}>{p}</button>)}<button onClick={leave}>{identity==='guest'?'Exit guest':'Sign out'}</button></nav>{identity==='guest'&&<p className="sync-status">Saved on this browser. Sign in for account sync.</p>}{saveError&&<p role="alert">{saveError}</p>}{finishing&&<p role="status">Saving setup completion…</p>}

 {!done&&<p className="onboarding-note">First-time setup · Pet → Banks → Goals → Letters. You can edit each area later.</p>}
 {page==='Dashboard'?<main className="finance-page dashboard"><span className="section-number">YOUR DASHBOARD</span><h1>Hello, {name}.</h1><p>Everything you need, in one place.</p>{!done&&<button className="primary-button" onClick={()=>navigate(box.companion?'Banks':'Pet')}>Continue setup ↗</button>}<div className="dashboard-grid"><section className="dashboard-pet-card"><h2>{box.companion ? `${name} the ${box.companion.pet}` : 'Choose your pet'}</h2><button className="small-button" onClick={()=>navigate('Pet')}>Edit pet</button></section><section><h2>Your banks</h2><p>{data.accounts.length} accounts linked</p><button className="small-button" onClick={()=>navigate('Banks')}>Manage banks</button></section><section><h2>{goal?.name || 'Your savings goal'}</h2><p>{progress&&goal?`${money(progress.saved)} of ${money(goal.target)} · ${progress.percent}%`:'Connect a savings account and set a goal.'}</p><button className="small-button" onClick={()=>navigate('Goals')}>Manage goals</button></section></div>{box.companion&&<DeviceConnection identity={identity} name={name}/>}</main>:page==='Pet'?<main className="finance-page pet-page"><span className="section-number">{done?'YOUR PET':'02 / A NEW LITTLE FRIEND'}</span>{created?<><h1>{name} is saved.</h1><DeviceConnection identity={identity} name={name}/><button className="primary-button" onClick={()=>navigate(done?'Dashboard':'Banks')}>{done?'Back to dashboard':'Continue to banks ↗'}</button></>:<><h1>{done?`Edit ${name}`:'Create your okanegotchi'}</h1><CompanionSetup identity={identity} editing={done} onContinue={()=>{setRevision(revision+1);setCreated(true)}}/></>}</main>:<FinanceScreen key={page} identity={identity} initialView={page==='Banks'?'accounts':page==='Goals'?'goal':'letter'} onBack={()=>navigate('Pet')} onNavigate={view=>navigate(view==='accounts'?'Banks':view==='goal'?'Goals':'Letters')} onFinish={finish}/>}
 </>
}
