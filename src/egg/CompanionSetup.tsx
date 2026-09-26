import { useState } from 'react'
import type { FormEvent } from 'react'
import { DemoMailbox } from './transport'
import { browserStorage } from '../finance/storage'
import { assets } from './contract'
import type { Companion } from './contract'
import { parseTarget } from '../finance/goals'
import './letter.css'
export function CompanionSetup({identity,onContinue}:{identity:string;onContinue:()=>void}) {
  const [mailbox]=useState(()=>new DemoMailbox(identity,browserStorage()))
  const [pet,setPet]=useState<Companion>(()=>mailbox.snapshot().companion ?? {name:'Penny',pet:'piggy'})
  const [timezone,setTimezone]=useState(()=>mailbox.snapshot().preferences?.timezone ?? Intl.DateTimeFormat().resolvedOptions().timeZone ?? 'UTC')
  const [budget,setBudget]=useState(()=>{const amount=mailbox.snapshot().preferences?.weeklyBudgetMinor;return amount == null ? '' : (amount/100).toFixed(2)})
  const [error,setError]=useState('')
  function save(e:FormEvent) {e.preventDefault();try{const amount=budget.trim() ? parseTarget(budget) : null;if(budget.trim() && amount===null) throw Error('Enter a positive weekly budget with up to two decimal places, or leave it blank.');mailbox.setSetup({...pet,name:pet.name.trim()},{timezone:timezone.trim(),weeklyBudgetMinor:amount,currency:'USD'});onContinue()}catch(e){setError((e as Error).message)}}
  return <form className="companion-setup" onSubmit={save}><div className="field"><label htmlFor="companion-name">Companion name</label><input id="companion-name" maxLength={24} required value={pet.name} onChange={e=>setPet({...pet,name:e.target.value})}/></div><fieldset><legend>Choose your pet</legend><div className="companion-picks">{assets.pet.map(p=><label key={p}><input type="radio" name="pet" value={p} checked={pet.pet===p} onChange={()=>setPet({...pet,pet:p})}/><span>{p==='piggy'?'♙':p==='cat'?'♧':'♢'}</span>{p}</label>)}</div></fieldset><div className="field"><label htmlFor="setup-timezone">Timezone</label><input id="setup-timezone" required maxLength={100} value={timezone} onChange={e=>setTimezone(e.target.value)} aria-describedby="timezone-help"/><p id="timezone-help" className="mail-note">Detected from your browser. Use an IANA timezone such as America/New_York or Europe/London. This will guide future daily check-ins.</p></div><div className="field"><label htmlFor="weekly-budget">Weekly spending budget (USD, optional)</label><input id="weekly-budget" inputMode="decimal" placeholder="e.g. 150.00" value={budget} onChange={e=>setBudget(e.target.value)} aria-describedby="budget-help"/><p id="budget-help" className="mail-note">Leave blank for no budget. Your budget provides financial context; it does not affect your companion’s care.</p></div>{error&&<p role="alert" className="auth-error">{error}</p>}<button type="submit" className="primary-button">Save & continue to accounts ↗</button><p className="mail-note">Demo companion. These choices will be included in your letter to the demo egg.</p></form>
}
