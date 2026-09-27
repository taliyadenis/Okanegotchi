import { useEffect, useMemo, useRef, useState } from 'react'
import type { Snapshot } from '../finance/types'
import type { SavingsGoal } from '../finance/goals'
import { goalProgress } from '../finance/goals'
import { browserStorage } from '../finance/storage'
import { money } from '../finance/fixtures'
import { serializePacket } from './contract'
import type { Packet } from './contract'
import { DemoMailbox } from './transport'
import type { Scenario } from './transport'
import { sendLetter, pause, bounded } from './send'
import type { Phase } from './send'
import './letter.css'

export function LetterScreen({identity,data,goal,onBack,onAccounts,onCompanion}:{identity:string;data:Snapshot;goal:SavingsGoal|null;onBack:()=>void;onAccounts:()=>void;onCompanion:()=>void}) {
  const [mailbox]=useState(()=>new DemoMailbox(identity,browserStorage(identity)))
  const [box,setBox]=useState(()=>mailbox.snapshot())
  const [still,setStill]=useState(()=>window.matchMedia('(prefers-reduced-motion: reduce)').matches)
  const [scenario,setScenario]=useState<Scenario>('online')
  const [destination,setDestination]=useState('demo')
  const [phase,setPhase]=useState<Phase>('idle')
  const [animation,setAnimation]=useState(0)
  const [busy,setBusy]=useState(false)
  const [error,setError]=useState('')
  const [notice,setNotice]=useState('')
  const [acceptStale,setAcceptStale]=useState(false)
  const controller=useRef<AbortController|null>(null)
  const lock=useRef(false)
  const heading=useRef<HTMLHeadingElement>(null)
  useEffect(()=>{heading.current?.focus();return()=>{controller.current?.abort()}},[])
  const account=data.accounts.find(a=>a.id===goal?.accountId && a.type==='savings')
  const connection=data.connections.find(c=>c.institutionId===account?.institutionId)
  const progress=goal?goalProgress(goal,data.accounts):null
  const stale=connection?.status!=='connected'
  const pending=box.history.find(d=>d.status==='queued'||d.status==='unknown')
  const latest=box.history[0]
  const candidate=useMemo<Packet|null>(()=> {
    if(!goal||!progress||!box.companion||!box.preferences||!connection) return null
    return {schema:'okanegotchi.demo-letter.v4',kind:'configuration',messageId:crypto.randomUUID(),revision:box.revision+1,createdAt:new Date().toISOString(),sourceAt:new Date(connection.refreshedAt).toISOString(),source:'demo',destination:'demo-egg',companion:box.companion,preferences:box.preferences,goal:{id:'primary-savings-goal',revision:box.revision+1,name:goal.name,targetMinor:goal.target,savedMinor:progress.saved,currency:'USD',basis:'full-savings-balance'}}
  },[goal,box.preferences,box.companion,box.revision,connection,progress?.saved])
  let packetError=''
  if(candidate) { try { serializePacket(candidate) } catch(e) { packetError=(e as Error).message } }
  const ready=!!candidate&&!packetError&&(!stale||acceptStale)&&destination==='demo'&&!pending
  async function act(action:(signal:AbortSignal)=>Promise<unknown>) {
    if(lock.current) return
    lock.current=true;setBusy(true);setError('');setNotice('')
    const c=new AbortController();controller.current=c
    try { await action(c.signal) } catch(e) { if(!c.signal.aborted)setError((e as Error).message) }
    finally {if(controller.current===c)lock.current=false;if(!c.signal.aborted){setBox(mailbox.snapshot());setPhase('idle');setBusy(false)}}
  }
  function dispatch(p:Packet) {setAnimation(n=>n+1);void act(signal=>sendLetter(mailbox,p,scenario,signal,setPhase,still||window.matchMedia('(prefers-reduced-motion: reduce)').matches))}
  function download() {
    if(!candidate)return
    try {const raw=serializePacket(candidate);const url=URL.createObjectURL(new Blob([raw],{type:'application/json'}));const a=document.createElement('a');a.href=url;a.download='okanegotchi-demo-letter.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);setNotice('Demo JSON downloaded. This is an export, not a delivery.')}catch(e){setError((e as Error).message)}
  }
  function cancel() {controller.current?.abort();lock.current=false;setBusy(false);setPhase('idle');setNotice('Packing cancelled. Nothing was submitted.')}
  const statusText=phase==='packing'?'Packing your little update…':phase==='submitting'?'Submitting your sealed letter…':phase==='flying'?'Queued. Sending your letter on its way…':latest?.note ?? (ready?'Your letter is ready when you are.':'Finish the steps below to prepare your letter.')
  return <main className={`finance-page letter-page ${still ? 'still-mail' : ''}`}>
    <button className="text-button" onClick={onBack}>← Your savings goal</button>
    <div className="finance-heading"><div><span className="section-number">05 / SPECIAL DELIVERY</span><h1 ref={heading} tabIndex={-1}>A little letter<br /><em>for your egg.</em></h1></div><span className="finance-badge">DEMO POST OFFICE</span></div>
    <p className="finance-intro">Your companion. Your next chapter. All wrapped up in a little hello.</p>
    <div className="mail-layout"><div>
      <section className="mail-setup"><span className="section-number">01 / GET READY</span><h2>Who’s getting your letter?</h2>
        <p className="mail-note">Your companion comes from the setup you already completed.</p><button className="small-button" disabled={busy} onClick={onCompanion}>Edit companion setup</button>
        <ul className="mail-readiness"><li><span>{box.companion?'✓':'○'}</span>{box.companion?`${box.companion.name} · ${box.companion.pet}`:'Finish companion setup before sending.'}</li><li><span>{box.preferences?'✓':'○'}</span>{box.preferences?'Timezone and budget reviewed':'Review timezone and optional budget in companion setup.'}</li><li><span>{progress?'✓':'○'}</span>{progress?'Demo savings account connected':'Connect a demo savings account'}<button className="text-button" disabled={busy} onClick={onAccounts}>Edit accounts</button></li><li><span>{goal?'✓':'○'}</span>{goal?goal.name:'Set your savings goal'}<button className="text-button" disabled={busy} onClick={onBack}>Edit goal</button></li></ul>
        <label className="mail-destination">Destination<select value={destination} disabled={busy} onChange={e=>setDestination(e.target.value)}><option value="demo">Demo egg simulator</option><option value="physical">Paired physical egg — unavailable</option></select></label>
        {destination==='physical'&&<p className="finance-warning">Physical pairing needs an authorized device registry and the firmware contract. Choose the demo simulator to try delivery today.</p>}
        {connection&&<p className="mail-note">Sample data as of {new Date(connection.refreshedAt).toLocaleString('en-US',{timeZone:'UTC'})} UTC.</p>}
        {connection&&stale&&<label className="stale-check"><input type="checkbox" checked={acceptStale} onChange={e=>setAcceptStale(e.target.checked)} disabled={busy}/>I reviewed the stale/attention status and want to send this last-known sample balance.</label>}
        {!mailbox.persistent&&<p className="finance-warning">Storage is unavailable. This mailbox lasts until this page reloads.</p>}
      </section>
      <section className="letter-review"><span className="letter-stamp" aria-hidden="true">✦<small>DEMO<br/>POST</small></span><span className="section-number">02 / YOUR LETTER</span><h2>Dear {box.companion?.name ?? 'little companion'},</h2><p>Here’s what we’re looking forward to.</p><dl><dt>Our goal</dt><dd>{goal?.name ?? 'Choose a goal'}</dd><dt>Saved / target</dt><dd>{progress&&goal?`${money(progress.saved)} / ${money(goal.target)} USD`:'Waiting for a linked savings account'}</dd><dt>Progress</dt><dd>{progress?`${progress.percent}% · full savings balance`:'Unavailable'}</dd><dt>Companion</dt><dd>{box.companion?box.companion.pet:'Finish companion setup'}</dd><dt>Timezone</dt><dd>{box.preferences?.timezone ?? 'Review in companion setup'}</dd><dt>Weekly budget</dt><dd>{box.preferences ? box.preferences.weeklyBudgetMinor === null ? 'Not set' : `${money(box.preferences.weeklyBudgetMinor)} USD` : 'Review in companion setup'}</dd></dl><p className="mail-signature">Little steps, together. &lt;3</p><p className="mail-note">Only this setup, savings summary, and timestamps travel in your letter. No bank transactions, account numbers, or login details.</p>
      {candidate&&<details><summary>View proposed demo packet</summary><pre>{JSON.stringify(candidate,null,2)}</pre></details>}{packetError&&<p role="alert" className="auth-error">{packetError}</p>}</section>
    </div><div className="mail-right">
      <section className="mail-stage-card"><span className="section-number">03 / PACK A LITTLE POSSIBILITY</span><div className={`mail-stage ${phase}`} key={animation} aria-hidden="true"><div className="mail-trail">· · · ✦ · · ·</div><div className="mail-envelope"><div className="mail-paper"><span>♡</span><span>YOUR LITTLE GOAL</span><i/><i/><b>✦</b></div><div className="envelope-back"/><div className="envelope-front"/><div className="envelope-flap"/><span className="envelope-seal">♥</span><span className="envelope-address">TO: YOUR DEMO EGG</span></div><span className="mail-destination-egg">◉<small>DEMO EGG</small></span></div>
      <p className="mail-status" role="status" aria-live="polite">{statusText}</p>{error&&<p className="auth-error" role="alert">{error}</p>}{notice&&<p className="finance-message" role="status">{notice}</p>}
      <button className="primary-button" disabled={!ready||busy} onClick={()=>candidate&&dispatch(candidate)}>{busy?'Packing & sending…':'Pack & send to demo egg ↗'}</button>{phase==='packing'&&<button className="text-button" onClick={cancel}>Cancel before dispatch</button>}
      {pending&&<p className="mail-note">A frozen letter is still pending. Resolve its receipt before sending new changes. You can leave this page; saved demo delivery resumes when you check its status.</p>}
      <button className="text-button" disabled={!candidate||!!packetError||busy} onClick={download}>Download demo JSON</button><p className="mail-note">Demo only. No physical egg is paired. Sending does not move money or change your pet’s care.</p>
      </section>
      <section className="mail-receipts"><span className="section-number">THE LITTLE POSTBOX</span><h2>Delivery receipts</h2>{!box.history.length&&<p>No letters sent yet. Your first hello is waiting.</p>}{box.history.map(d=><article key={d.packet.messageId}><div className="receipt-title"><strong>{d.status==='received'?'✓ Received by demo egg':d.status==='queued'?'◷ Queued for demo egg':d.status==='unknown'?'? Delivery unknown':'! Delivery failed'}</strong><small>Letter #{d.packet.revision}</small></div><p>{d.packet.companion.name} · {d.packet.goal.name}<br/>{money(d.packet.goal.savedMinor)} of {money(d.packet.goal.targetMinor)}</p><small>{d.note}</small>{(d.status==='queued'||d.status==='unknown')&&<button className="small-button" disabled={busy} onClick={()=>void act(()=>bounded(mailbox.status(d.packet.messageId,true)))}>Reconnect simulator & check receipt</button>}{d.status==='failed'&&d.packet.revision===box.revision&&<button className="small-button" disabled={busy||!!pending||destination!=='demo'} onClick={()=>dispatch(d.packet)}>Retry same letter</button>}<details><summary>Frozen letter details</summary><pre>{JSON.stringify(d.packet,null,2)}</pre></details></article>)}</section>
      {box.applied&&<section className="simulated-display"><span className="section-number">SIMULATOR · LAST APPLIED CONFIGURATION</span><h3>{box.applied.companion.name}</h3><p>{box.applied.companion.pet}</p><strong>{box.applied.goal.name}</strong><p>{money(box.applied.goal.savedMinor)} / {money(box.applied.goal.targetMinor)}</p><small>Revision {box.applied.revision} · no reaction event emitted</small></section>}
    </div></div>
    <details className="demo-controls"><summary>Demo delivery controls</summary><p>Local simulation only. Queued letters survive reload when storage works; checking the receipt resumes the simulator. Nothing runs on hardware.</p><label className="stale-check"><input type="checkbox" checked={still} disabled={busy} onChange={e=>setStill(e.target.checked)} />Use still transitions (reduced motion)</label><label>Next send outcome<select value={scenario} disabled={busy} onChange={e=>setScenario(e.target.value as Scenario)}><option value="online">Online — validate and apply</option><option value="offline">Offline — keep queued</option><option value="failure">Submission failure</option><option value="reject">Schema rejection at demo egg</option><option value="timeout">Receipt timeout — unknown</option><option value="duplicate">Duplicate receipt — apply once</option></select></label><button className="small-button" disabled={busy} onClick={()=>{setAnimation(n=>n+1);void act(async signal=>{setPhase('packing');await pause(still||window.matchMedia('(prefers-reduced-motion: reduce)').matches?0:1100,signal);setPhase('flying');await pause(still||window.matchMedia('(prefers-reduced-motion: reduce)').matches?0:700,signal);setNotice('Animation replay only. No letter submitted.')})}}>Replay animation only</button></details>
  </main>
}
