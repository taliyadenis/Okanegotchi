import { accountStorage } from '../account/cloud-store'
import { useRef, useState } from 'react'
import type { FormEvent } from 'react'
import { DemoMailbox } from './transport'
import { browserStorage } from '../finance/storage'
import { assets } from './contract'
import { CharacterCreator } from '../character/CharacterCreator'
import type { Pet } from '../character/CharacterCreator'
import type { Companion } from './contract'
import { parseTarget } from '../finance/goals'
import './letter.css'
export function CompanionSetup({identity,onContinue,editing=false}:{identity:string;onContinue:()=>void;editing?:boolean}) {
  const [mailbox]=useState(()=>new DemoMailbox(identity,browserStorage(identity)))
  const [pet,setPet]=useState<Companion>(()=>{ const saved=mailbox.snapshot().companion; return saved && assets.pet.includes(saved.pet as Pet) ? saved : {name:saved?.name ?? '',pet:'gator'} })
  const [timezone,setTimezone]=useState(()=>mailbox.snapshot().preferences?.timezone ?? Intl.DateTimeFormat().resolvedOptions().timeZone ?? 'UTC')
  const [budget,setBudget]=useState(()=>{const amount=mailbox.snapshot().preferences?.weeklyBudgetMinor;return amount == null ? '' : (amount/100).toFixed(2)})
  const [error,setError]=useState('')
  const [saving,setSaving]=useState(false)
  const savingLock=useRef(false)
  async function save(e:FormEvent) {e.preventDefault();if(savingLock.current)return;savingLock.current=true;setSaving(true);setError('');try{const amount=budget.trim() ? parseTarget(budget) : null;if(budget.trim() && amount===null) throw Error('Enter a positive weekly budget with up to two decimal places, or leave it blank.');mailbox.setSetup({...pet,name:pet.name.trim()},{timezone:timezone.trim(),weeklyBudgetMinor:amount,currency:'USD'});await accountStorage(identity)?.flush();onContinue()}catch(e){setError((e as Error).message)}finally{savingLock.current=false;setSaving(false)}}
  return <form className="companion-setup" onSubmit={save}><div className="field"><label htmlFor="companion-name">Companion name</label><input id="companion-name" maxLength={24} required value={pet.name} onChange={e=>setPet({...pet,name:e.target.value})}/></div><h2>{pet.name.trim() || 'okanegotchi'}</h2><CharacterCreator name={pet.name} pet={pet.pet as Pet} onPetChange={p=>setPet({...pet,pet:p})}/><div className="field"><label htmlFor="setup-timezone">Timezone</label><input id="setup-timezone" required maxLength={100} value={timezone} onChange={e=>setTimezone(e.target.value)} aria-describedby="timezone-help"/><p id="timezone-help" className="mail-note">Detected from your browser. Use an IANA timezone such as America/New_York or Europe/London. This will guide future daily check-ins.</p></div><div className="field"><label htmlFor="weekly-budget">Weekly spending budget (USD, optional)</label><input id="weekly-budget" inputMode="decimal" placeholder="e.g. 150.00" value={budget} onChange={e=>setBudget(e.target.value)} aria-describedby="budget-help"/><p id="budget-help" className="mail-note">Your budget provides financial context; it does not affect pet care.</p></div>{error&&<p role="alert" className="auth-error">{error}</p>}<button type="submit" disabled={saving} className="primary-button">{saving?'Saving…':editing?'Save changes':'Create character ↗'}</button><p className="mail-note">These choices will be included in your letter.</p></form>
}
