import { useEffect, useRef, useState } from 'react'
import type { FormEvent } from 'react'
import type { Snapshot } from './types'
import { money } from './fixtures'
import { browserStorage } from './storage'
import { goalProgress, parseTarget, readGoal, saveGoal } from './goals'
import type { SavingsGoal } from './goals'
import './goals.css'

export function SavingsGoalScreen({ identity, data, onBack, onContinue }: { identity: string; data: Snapshot; onBack: () => void; onContinue: (goal: SavingsGoal | null) => void }) {
  const [storage] = useState(()=>browserStorage(identity))
  const [goal, setGoal] = useState(() => readGoal(storage, identity))
  const [editing, setEditing] = useState(!goal)
  const [name, setName] = useState(goal?.name ?? '')
  const [target, setTarget] = useState(goal ? (goal.target / 100).toFixed(2) : '')
  const savings = data.accounts.filter(a => a.type === 'savings')
  const [accountId, setAccountId] = useState(goal?.accountId ?? savings[0]?.id ?? '')
  const [error, setError] = useState('')
  const [notice, setNotice] = useState('')
  const [persistent, setPersistent] = useState(!!storage)
  const [preview, setPreview] = useState(false)
  const [confirmRemove, setConfirmRemove] = useState(false)
  const heading = useRef<HTMLHeadingElement>(null)
  useEffect(() => { heading.current?.focus() }, [editing])
  const progress = goal ? goalProgress(goal, data.accounts) : null
  const account = data.accounts.find(a => a.id === goal?.accountId)
  const connection = data.connections.find(c => c.institutionId === account?.institutionId)
  function persist(next: SavingsGoal | null) { setGoal(next); setPersistent(saveGoal(storage, identity, next)); setPreview(false) }
  function submit(e: FormEvent) {
    e.preventDefault()
    const cents = parseTarget(target)
    if (!name.trim() || name.trim().length > 60) { setError('Give your goal a name between 1 and 60 characters.'); document.getElementById('goal-name')?.focus(); return }
    if (cents === null) { setError('Enter a positive dollar amount with no more than two decimal places.'); document.getElementById('goal-target')?.focus(); return }
    if (!savings.some(a => a.id === accountId)) { setError('Choose a connected demo savings account.'); document.getElementById('goal-account')?.focus(); return }
    persist({ name: name.trim(), target: cents, accountId }); setEditing(false); setError(''); setNotice('Your goal is updated. For signed-in accounts, check the cloud save status above.'); setConfirmRemove(false)
  }
  return <main className="finance-page goal-page">
    <button className="text-button" onClick={onBack}>← Financial accounts</button>
    <div className="finance-heading"><div><span className="section-number">04 / SOMETHING TO LOOK FORWARD TO</span><h1 ref={heading} tabIndex={-1}>Small steps.<br /><em>Your next chapter.</em></h1></div><span className="finance-badge">✦ DEMO DATA</span></div>
    <p className="finance-intro">A little goal for you. A little cheer from your companion.</p>
    <p role="status" className="finance-message">{notice}</p>
    {!persistent && <p className="finance-warning">This goal is available for this visit only. Browser storage is unavailable.</p>}
    <div className="goal-layout"><section className="goal-card">
      {editing ? <><span className="section-number">{goal ? 'MAKE IT YOURS' : 'YOUR FIRST SAVINGS GOAL'}</span><h2>{goal ? 'Edit your goal' : 'What are you saving for?'}</h2><p>A rainy day, a new bike, or something all your own.</p>
      {!savings.length ? <div className="goal-empty"><h3>First, give your goal a pocket.</h3><p>Connect a demo savings account at Clover Bank or Sunny Savings to track your progress.</p><button className="primary-button" onClick={onBack}>Connect a savings account ↗</button>{goal && <button className="text-button" onClick={() => setEditing(false)}>Back to saved goal</button>}</div> : <form onSubmit={submit} noValidate>
        <div className="field"><label htmlFor="goal-name">Goal name</label><input id="goal-name" value={name} maxLength={60} onChange={e => setName(e.target.value)} placeholder="A rainy day fund" required /></div>
        <div className="field"><label htmlFor="goal-target">Target amount (USD)</label><input id="goal-target" inputMode="decimal" value={target} onChange={e => setTarget(e.target.value)} placeholder="5000.00" required /></div>
        <div className="field"><label htmlFor="goal-account">Linked demo savings account</label><select id="goal-account" value={accountId} onChange={e => setAccountId(e.target.value)} required><option value="">Choose an account</option>{savings.map(a => <option key={a.id} value={a.id}>{a.name} · ••{a.mask} · {money(a.balance)}</option>)}</select></div>
        <p className="goal-rule">Progress uses this account’s entire current demo balance. Linking a goal doesn’t set money aside or move funds.</p>
        {error && <p role="alert" className="auth-error">{error}</p>}<button className="primary-button" type="submit">Save savings goal ↗</button>{goal && <button type="button" className="text-button" onClick={() => { setEditing(false); setError('') }}>Cancel edits</button>}
      </form>}</> : goal && <><span className="section-number">YOUR LITTLE AMBITION</span><h2 className="goal-name">{goal.name}</h2><p>Target: <strong>{money(goal.target)}</strong> USD</p>
        {progress ? <><div className="goal-amount"><strong>{money(progress.saved)}</strong><span>of {money(goal.target)}</span></div><progress value={progress.percent} max={100} aria-label={`${goal.name} progress`}>{progress.percent}%</progress><div className="goal-progress-label"><strong>{progress.percent}% of target</strong><span>{progress.complete ? 'Target reached ✦' : `${money(progress.remaining)} to go`}</span></div><p className="goal-rule">Using the full balance of <strong>{account?.name} · ••{account?.mask}</strong>. This is sample money; nothing has been transferred or reserved.</p>{connection?.status !== 'connected' && <p className="finance-warning">Your linked account needs a refresh or reconnect. This progress uses its last saved demo balance.</p>}</> : <div className="goal-empty"><h3>Your goal is still here.</h3><p>The linked savings account is disconnected. Reconnect it or edit your goal to choose another savings account. Progress is unavailable until then.</p><button className="demo-button" onClick={onBack}>Manage demo accounts</button></div>}
        <div className="goal-actions"><button className="small-button" onClick={() => { setName(goal.name); setTarget((goal.target / 100).toFixed(2)); setAccountId(goal.accountId); setEditing(true); setPreview(false); setNotice(''); setConfirmRemove(false) }}>Edit goal</button><button className="text-button" onClick={() => setConfirmRemove(true)}>Remove goal</button></div>
        {confirmRemove && <div className="goal-remove" role="group" aria-label="Confirm goal removal"><p>Remove this demo goal? Your connected accounts and their balances will stay as they are.</p><button className="small-button" onClick={() => { persist(null); setName(''); setTarget(''); setAccountId(savings[0]?.id ?? ''); setEditing(true); setConfirmRemove(false); setNotice('Demo goal removed.') }}>Remove saved goal</button><button className="text-button" onClick={() => setConfirmRemove(false)}>Keep goal</button></div>}
      </>}
    </section><aside className="goal-companion"><span className="section-number">A LITTLE CHEER, JUST FOR YOU</span><div className={`goal-egg ${preview ? 'celebrating' : ''}`} aria-hidden="true"><span className="egg-spark left">✦</span><div className="egg-screen"><span>{preview ? 'NICE LITTLE STEP!' : 'ONE STEP AT A TIME'}</span><div className="egg-face">{preview ? '⌃ ▿ ⌃' : '• ▿ •'}</div><span>{preview ? '✦ +$50 DEMO ✦' : 'YOU + YOUR NEXT CHAPTER'}</span></div><div className="egg-buttons">● ● ●</div><span className="egg-spark right">✦</span></div><h2>{preview ? 'A little progress. A little party.' : 'Your egg is cheering you on.'}</h2><p>{preview ? 'This previews how your companion could celebrate a $50 savings contribution.' : 'Savings contributions can give your companion a reason to celebrate.'}</p><span className="finance-badge">ON-SCREEN CONCEPT PREVIEW</span><p className="goal-rule">No physical device is connected. This preview changes neither your goal progress nor your account balance. Care comes from check-ins, not spending or saving money.</p><button className="demo-button" disabled={!progress || editing} onClick={() => { setPreview(!preview); setNotice(preview ? 'Celebration preview ended.' : 'Simulated $50 contribution: celebration preview only. Balances and goal progress are unchanged.') }}>{preview ? 'End celebration preview' : 'Preview a $50 savings celebration'}</button></aside></div>
    <section className="finance-goal-entry"><div><h2>A little hello for your egg.</h2><p>Review your setup, pack a letter, and try a demo delivery.</p></div><button className="primary-button" disabled={editing} onClick={() => onContinue(goal)}>Continue to your egg ↗</button></section>
  </main>
}
