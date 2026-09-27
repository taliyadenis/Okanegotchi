import { useEffect, useRef, useState } from 'react'
import { accounts, institutions, money, totals } from './fixtures'
import { DemoProvider } from './provider'
import { browserStorage } from './storage'
import './finance.css'
import { PhysicalLetterScreen } from '../egg/PhysicalLetterScreen'
import { DemoMailbox } from '../egg/transport'
import { readGoal } from './goals'
import type { SavingsGoal } from './goals'
import { SavingsGoalScreen } from './SavingsGoalScreen'

export function FinanceScreen({ identity, onBack, initialView='accounts', onNavigate, onFinish }: { identity: string; onBack: () => void; initialView?: 'accounts'|'goal'|'letter'; onNavigate: (view:'accounts'|'goal'|'letter')=>void; onFinish:()=>void }) {
  const [view] = useState<'accounts' | 'goal' | 'letter'>(initialView)
  const [letterGoal, setLetterGoal] = useState<SavingsGoal | null>(()=>readGoal(browserStorage(identity),identity))
  const [provider] = useState(() => new DemoProvider(identity, browserStorage(identity)))
  const [data, setData] = useState(() => provider.snapshot())
  const [busy, setBusy] = useState(false)
  const lock = useRef(false)
  const alive = useRef(true)
  const [message, setMessage] = useState('')
  const [error, setError] = useState('')
  const [filter, setFilter] = useState('all')
  const [modal, setModal] = useState<'connect' | 'reset' | 'disconnect' | null>(null)
  const [institution, setInstitution] = useState('')
  const [search, setSearch] = useState('')
  const [selected, setSelected] = useState<string[]>([])
  const [target, setTarget] = useState('')
  const controller = useRef<AbortController | null>(null)
  const pageHeading = useRef<HTMLHeadingElement>(null)
  const dialog = useRef<HTMLDialogElement>(null)
  const trigger = useRef<HTMLElement | null>(null)
  useEffect(() => {
    alive.current = true
    provider.activate()
    pageHeading.current?.focus()
    return () => { alive.current = false; controller.current?.abort(); provider.dispose() }
  }, [])
  useEffect(() => {
    if (modal) dialog.current?.showModal()
    else { dialog.current?.close(); (trigger.current?.isConnected ? trigger.current : pageHeading.current)?.focus() }
  }, [modal])
  function open(type: typeof modal, id = '') {
    trigger.current = document.activeElement as HTMLElement
    setError(''); setMessage(''); setInstitution(''); setSelected([]); setSearch(''); setTarget(id); setModal(type)
  }
  function close() { controller.current?.abort(); setModal(null); setError('') }
  async function run(action: () => Promise<void>, success: string, closeOnSuccess = false) {
    if (lock.current) return
    lock.current = true; setBusy(true); setError(''); setMessage('')
    try {
      await action()
      if (!alive.current) return
      setMessage(success)
      if (closeOnSuccess) setModal(null)
    } catch (e) {
      if (alive.current && !(e instanceof Error && 'code' in e && e.code === 'cancelled')) setError(e instanceof Error ? e.message : 'Something went wrong. Try again.')
    } finally {
      lock.current = false
      if (alive.current) { setBusy(false); setData(provider.snapshot()) }
    }
  }
  const summary = totals(data.accounts, data.transactions)
  const connected = data.accounts.map(a => a.id)
  const available = accounts.filter(a => a.institutionId === institution && !connected.includes(a.id))
  const effectiveFilter = data.accounts.some(a => a.id === filter) ? filter : 'all'
  const activity = data.transactions.filter(t => effectiveFilter === 'all' || t.accountId === effectiveFilter).sort((a, b) => b.date.localeCompare(a.date) || a.id.localeCompare(b.id))
  if (view === 'letter') return <PhysicalLetterScreen identity={identity} goal={letterGoal} onFinish={onFinish} />
  if (view === 'goal') return <SavingsGoalScreen onContinue={goal => { setLetterGoal(goal); onNavigate('letter') }} identity={identity} data={data} onBack={() => onNavigate('accounts')} />
  return <main className="finance-page">
    <button className="text-button" onClick={onBack}>← {new DemoMailbox(identity,browserStorage(identity)).snapshot().companion?.name || 'okanegotchi'}</button>
    <div className="finance-heading"><div><span className="section-number">03 / YOUR MONEY, A LITTLE CLEARER</span><h1 ref={pageHeading} tabIndex={-1}>Little pockets.<br /><em>Big possibilities.</em></h1></div></div>
    <p className="finance-intro">No real money. Just practice.</p>
    {!modal && error && <p className="auth-error" role="alert">{error}</p>}
    <p className="finance-message" role="status">{message}</p>
    {!provider.storageAvailable && <p className="finance-warning">Browser storage is unavailable. Your demo works here, but changes may not survive a reload.</p>}
    {data.accounts.length === 0 ? <section className="finance-empty"><div className="pocket-art" aria-hidden="true">✿<span>$</span></div><span className="section-number">A FRESH LITTLE START</span><h2>Give your money a home.</h2><p>Explore sample balances and everyday moments.<br />No bank login. No real money. Just a little practice.</p><button className="primary-button" onClick={() => open('connect')}>Connect a demo account ↗</button></section> : <>
      <div className="finance-summary"><article><span>Cash in sample accounts</span><strong>{money(summary.cash)}</strong><small>Checking + savings · USD</small></article><article><span>Credit card amount owed</span><strong>{money(summary.owed)}</strong><small>Shown separately from your cash · USD</small></article></div>
      <div className="finance-section-title"><h2>Your little pockets <span>({data.accounts.length})</span></h2><button className="demo-button" disabled={busy} onClick={() => open('connect')}>Add demo account +</button></div>
      <div className="institution-list">{data.connections.map(connection => {
        const bank = institutions.find(i => i.id === connection.institutionId)!
        return <section className="institution-card" key={bank.id} aria-label={bank.name}><div className="institution-top"><div className="bank-name"><span className="bank-symbol" aria-hidden="true">{bank.symbol}</span><div><h3>{bank.name}</h3><span className={`connection-state ${connection.status}`}>{connection.status === 'connected' ? '● Demo connected' : connection.status === 'attention' ? '⚠ Needs attention' : '⚠ Saved data · stale'}</span></div></div><button className="text-button" disabled={busy} onClick={() => open('disconnect', bank.id)}>Disconnect</button></div>
          <div className="account-grid">{data.accounts.filter(a => a.institutionId === bank.id).map(account => <article className="account-tile" key={account.id}><span className="account-type">{account.type} · ••{account.mask}</span><h4>{account.name}</h4><strong>{money(account.balance)}</strong><span>{account.type === 'credit' ? 'Amount owed' : 'Current balance'} · USD</span><small>{account.type === 'credit' ? 'Available credit' : 'Available balance'}: {account.available === null ? 'Unavailable' : money(account.available)}</small></article>)}</div>
          <div className="institution-bottom"><span>Last simulated refresh: {new Date(connection.refreshedAt).toLocaleString('en-US', { timeZone: 'UTC' })} UTC</span><button className="small-button" disabled={busy} onClick={() => void run(() => connection.status === 'attention' ? provider.reconnect(bank.id) : provider.refresh(bank.id), connection.status === 'attention' ? 'Demo connection restored.' : 'Demo data refreshed. The fixed sample balances are unchanged.')}>{busy ? 'Please wait…' : connection.status === 'attention' ? 'Reconnect demo' : connection.status === 'stale' ? 'Retry refresh' : 'Refresh demo data'}</button></div>
          {connection.status !== 'connected' && <p className="finance-warning">{connection.status === 'attention' ? 'This simulated connection needs your attention. Reconnect to resume demo refreshes.' : 'Showing saved data from the last successful refresh.'}</p>}
        </section>
      })}</div>
      <section className="activity-panel"><div className="finance-section-title"><div><span className="section-number">THE EVERYDAY MOMENTS</span><h2>Recent activity</h2></div><label>Account<select value={data.accounts.some(a => a.id === filter) ? filter : 'all'} onChange={e => setFilter(e.target.value)}><option value="all">All accounts</option>{data.accounts.map(a => <option value={a.id} key={a.id}>{a.name} · {a.mask}</option>)}</select></label></div><p className="activity-note">Fixed sample history · September 2026. Transfers and pending activity are separate from posted purchases.</p><ul className="transaction-list">{activity.map(t => <li key={t.id}><span className="transaction-icon" aria-hidden="true">{t.kind === 'transfer' ? '⇄' : t.kind === 'out' ? '↗' : '↙'}</span><div><strong>{t.description}</strong><small>{t.date} · {data.accounts.find(a => a.id === t.accountId)?.name}</small><span>{t.category} · {t.kind === 'out' ? 'Money out' : t.kind === 'in' ? 'Money in' : t.kind === 'refund' ? 'Refund' : 'Transfer'} · {t.status}</span></div><b>{t.kind === 'out' ? '−' : t.kind === 'transfer' ? '' : '+'}{money(t.amount)}</b></li>)}</ul>{!activity.length && <p>No sample transactions for this account.</p>}</section>
    </>}
    <section className="finance-goal-entry"><div><h2>What’s your next little goal?</h2><p>{data.accounts.some(a=>a.type==='savings') ? 'Give your savings a purpose.' : 'Connect a savings account to unlock your goal.'}</p></div><button className="primary-button" disabled={!data.accounts.some(a=>a.type==='savings')} onClick={() => onNavigate('goal')}>Your savings goal ↗</button></section>

    <dialog className="finance-dialog" ref={dialog} onCancel={e => { e.preventDefault(); close() }} aria-labelledby="dialog-heading"><div className="dialog-top"><button className="text-button" onClick={close} aria-label="Cancel connection dialog">✕</button></div>
      {modal === 'connect' ? <><span className="section-number">{institution ? '02 / CHOOSE YOUR POCKETS' : '01 / FIND A SAMPLE BANK'}</span><h2 id="dialog-heading">{institution ? institutions.find(i => i.id === institution)?.name : 'A little connection.'}</h2><p>Explore fictional balances and transactions. No bank credentials needed.</p>{!institution ? <><label className="search-label">Search demo institutions<input value={search} onChange={e => setSearch(e.target.value)} placeholder="Find your sample bank" /></label><div className="bank-options">{institutions.filter(i => i.name.toLowerCase().includes(search.toLowerCase())).map(i => <button key={i.id} onClick={() => { setInstitution(i.id); setSelected([]); setError('') }}><span className="bank-symbol">{i.symbol}</span><span><strong>{i.name}</strong><small>{i.description}</small></span><span>→</span></button>)}</div>{!institutions.some(i => i.name.toLowerCase().includes(search.toLowerCase())) && <p>No sample banks match. Try another name.</p>}</> : <><fieldset disabled={busy}><legend>Select the sample accounts to display</legend>{available.map(a => <label className="account-choice" key={a.id}><input type="checkbox" checked={selected.includes(a.id)} onChange={e => setSelected(e.target.checked ? [...selected, a.id] : selected.filter(id => id !== a.id))} /><span><strong>{a.name}</strong><small>{a.type} · ••{a.mask} · {money(a.balance)} {a.type === 'credit' ? 'owed' : ''}</small></span></label>)}</fieldset>{!available.length && <p>All accounts at this institution are already connected.</p>}<button className="primary-button" disabled={busy || !selected.length} onClick={() => { controller.current = new AbortController(); void run(() => provider.connect(institution, selected, controller.current!.signal), 'Your sample accounts are ready.', true) }}>{busy ? 'Connecting demo accounts…' : 'Connect demo accounts'}</button><button className="text-button" disabled={busy} onClick={() => { setInstitution(''); setSelected([]); setError('') }}>← Back to institutions</button></>}</> : <><h2 id="dialog-heading">{modal === 'reset' ? 'A fresh start?' : 'Disconnect sample bank?'}</h2><p>{modal === 'reset' ? 'This removes all sample connections and transactions from this demo. You can connect them again any time.' : 'This removes this institution’s sample accounts and transactions from your demo. No real bank is affected.'}</p><button className="primary-button" disabled={busy} onClick={() => { setFilter('all'); void run(() => modal === 'reset' ? provider.reset() : provider.disconnect(target), modal === 'reset' ? 'Demo data reset.' : 'Sample bank disconnected.', true) }}>{modal === 'reset' ? 'Reset demo data' : 'Disconnect sample bank'}</button></>}
      {error && <p className="auth-error" role="alert">{error}</p>}<button className="text-button" onClick={close}>Cancel</button>
    </dialog>
  </main>
}
