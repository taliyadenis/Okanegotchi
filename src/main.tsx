import { StrictMode, useEffect, useRef, useState } from 'react'
import { createRoot } from 'react-dom/client'
import type { FormEvent } from 'react'
import './styles.css'
import { Workspace } from './workspace/Workspace'

import { supabase } from './auth'
import type { Session } from '@supabase/supabase-js'

function PixelPet({ small = false }: { small?: boolean }) {
  return <svg className={small ? 'pixel-pet small' : 'pixel-pet'} viewBox="0 0 32 32" aria-hidden="true" shapeRendering="crispEdges">
    <path fill="#191919" d="M7 7h6v4h6V7h6v6h3v12h-4v3H8v-3H4V13h3z" />
    <path fill="#00875A" d="M8 9h3v5h10V9h3v6h2v9h-4v2H10v-2H6v-9h2z" />
    <path fill="#2448FF" d="M6 21h3v3h14v-3h3v3h-4v2H10v-2H6z" />
    <path fill="#191919" d="M11 16h2v3h-2zm8 0h2v3h-2zm-5 5h4v2h-4z" />
    <path fill="#FF90C2" d="M8 20h3v2H8zm13 0h3v2h-3z" />
    <path fill="#FFD600" d="M15 12h2v2h-2z" />
  </svg>
}

function DeviceIllustration() {
  return <div className="illustration" role="img" aria-label="A bright green handheld device with a friendly pixel pet on its screen">
    <span className="spark spark-one">✦</span><span className="spark spark-two">+</span>
    <div className="device">
      <div className="device-top"><span>OKANEGOTCHI</span><span className="indicator" /></div>
      <div className="screen-frame"><div className="screen">
        <div className="screen-header"><span>HELLO, FRIEND</span><span>♥</span></div>
        <PixelPet />
        <div className="ground" />
        <span className="screen-caption">little steps, together.</span>
      </div></div>
      <div className="device-buttons"><i /><i /><i /></div>
      <div className="speaker"><i /><i /><i /><i /></div>
    </div>
    <span className="illustration-note">a small friend. a fresh start.</span>
  </div>
}

function App() {
  const [page, setPage] = useState<'login' | 'setup' | 'finance'>(()=>{try{return sessionStorage.getItem('okanegotchi:guest-active')==='true'?'setup':'login'}catch{return 'login'}})
  const [mode, setMode] = useState<'login' | 'signup'>('login')
  const [session, setSession] = useState<Session | null>(null)
  const [initializing, setInitializing] = useState(!!supabase)
  const [notice, setNotice] = useState('')
  const [confirmPassword, setConfirmPassword] = useState('')
  const [email, setEmail] = useState('')
  const [password, setPassword] = useState('')
  const [visible, setVisible] = useState(false)
  const [errors, setErrors] = useState<{ email?: string; password?: string; confirmPassword?: string }>({})
  const [authError, setAuthError] = useState('')
  const [loading, setLoading] = useState(false)
  const heading = useRef<HTMLHeadingElement>(null)
  const firstRender = useRef(true)

  useEffect(() => {
    document.title = `${page === 'login' ? 'Welcome' : 'Dashboard'} · Okanegotchi`
    if (firstRender.current) { firstRender.current = false; return }
    heading.current?.focus()
  }, [page])

  useEffect(() => {
    if (!supabase) return
    const { data: { subscription } } = supabase.auth.onAuthStateChange((event, nextSession) => {
      setSession(nextSession)
      setInitializing(false)
      if (nextSession) {
        setPage('setup')
        setPassword('')
        setConfirmPassword('')
        setAuthError('')
      } else if (event === 'SIGNED_OUT') {
        setPage('login')
      }
    })
    return () => subscription.unsubscribe()
  }, [])

  async function logout() {
    if (!supabase) return
    setLoading(true)
    setAuthError('')
    try {
      const { error } = await supabase.auth.signOut({ scope: 'local' })
      if (error) throw error
      navigate('login')
    } catch {
      setAuthError('Unable to log out. Please try again.')
    } finally { setLoading(false) }
  }

  async function login(event: FormEvent<HTMLFormElement>) {
    event.preventDefault()
    if (loading || initializing) return
    const next = {
      email: !email.trim() ? 'Please enter your email address.' : !/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email.trim()) ? 'Enter a valid email address.' : undefined,
      password: !password ? 'Please enter your password.' : mode === 'signup' && password.length < 8 ? 'Use at least 8 characters.' : undefined,
      confirmPassword: mode === 'signup' && password !== confirmPassword ? 'Passwords do not match.' : undefined,
    }
    setErrors(next)
    setAuthError('')
    setNotice('')
    if (next.email || next.password || next.confirmPassword) {
      document.getElementById(next.email ? 'email' : next.password ? 'password' : 'confirm-password')?.focus()
      return
    }
    setLoading(true)
    try {
      if (!supabase) {
        setAuthError('Accounts are not available yet. Please try the demo while we finish connecting them.')
        return
      }
      const credentials = { email: email.trim(), password }
      const { data, error } = mode === 'signup'
        ? await supabase.auth.signUp({ ...credentials, options: { emailRedirectTo: window.location.origin } })
        : await supabase.auth.signInWithPassword(credentials)
      if (error) {
        setAuthError(error.code === 'invalid_credentials' ? 'Email or password is incorrect.'
          : error.code === 'email_not_confirmed' ? 'Please confirm your email before logging in.'
          : error.message)
      } else if (!data.session && mode === 'signup') {
        setNotice('Check your email for a confirmation link, then return to log in. If you already have an account, log in instead.')
      } else if (data.session) {
        setSession(data.session)
        setPage('setup')
      }
    } catch {
      setAuthError('Unable to reach the account service. Please check your connection and try again.')
    } finally {
      setPassword('')
      setConfirmPassword('')
      setVisible(false)
      setLoading(false)
    }
  }

  function navigate(next: 'login' | 'setup' | 'finance') {
    try {sessionStorage.setItem('okanegotchi:guest-active',next==='login'?'false':'true')} catch {}
    setEmail(''); setPassword(''); setVisible(false); setErrors({}); setAuthError(''); setNotice(''); setConfirmPassword(''); setPage(next)
  }

  return <div className="site-shell">
    <header className="flex items-center justify-between gap-4">
      <a className="brand flex items-center gap-2" href="#" onClick={event => { event.preventDefault(); if (!loading) navigate(session ? 'setup' : 'login') }} aria-label="Okanegotchi home"><img className="brand-icon" src="/gator-icon.webp" alt=""/><span>Okanegotchi</span></a>

    </header>

    {page !== 'login' && !initializing ? <Workspace key={session?.user.id ?? 'guest'} identity={session ? `user:${session.user.id}` : 'guest'} onLogout={()=>session?void logout():navigate('login')} /> : <main className="main-layout">
      <section className="story-panel">

        <div className="eyebrow">Your friendly reminder.</div><h1>Little pet.<br /><span>Big<br />responsibility.</span></h1>
        <p className="story-copy">A mini companion for your financial journey.</p>
        <DeviceIllustration />
        <div className="story-footnote"><span>♡</span> Made for progress, one small step at a time.</div>
      </section>

      <section className="form-side" aria-label={page === 'login' ? 'Login' : 'Character setup'}>
        {initializing ? <div className="form-card" role="status">Restoring your session…</div> : page === 'login' ? <div className="form-card">
          <span className="section-number">01 / WELCOME HOME</span>
          <h2 ref={heading} tabIndex={-1}>{mode === 'login' ? 'Hello again!' : 'Join us!'}</h2>
          <p className="card-intro">{mode === 'login' ? 'Good to see you. Your next little step starts here.' : 'Create an account and start building better financial habits.'}</p>
          <form noValidate onSubmit={login}>
            <div className="field"><label htmlFor="email">Email address</label><input id="email" type="email" autoComplete="username" placeholder="you@example.com" value={email} disabled={loading} onChange={event => setEmail(event.target.value)} aria-invalid={!!errors.email} aria-describedby={errors.email ? 'email-error' : undefined} />{errors.email && <p id="email-error" className="field-error">{errors.email}</p>}</div>
            <div className="field"><label htmlFor="password">Password</label><div className="password-wrap"><input id="password" type={visible ? 'text' : 'password'} autoComplete={mode === 'signup' ? 'new-password' : 'current-password'} placeholder="Enter your password" value={password} disabled={loading} onChange={event => setPassword(event.target.value)} aria-invalid={!!errors.password} aria-describedby={errors.password ? 'password-error' : undefined} /><button className="password-toggle" type="button" disabled={loading} onClick={() => setVisible(!visible)} aria-label={visible ? 'Hide password' : 'Show password'} aria-pressed={visible}>{visible ? 'Hide' : 'Show'}</button></div>{errors.password && <p id="password-error" className="field-error">{errors.password}</p>}</div>
            {mode === 'signup' && <div className="field"><label htmlFor="confirm-password">Confirm password</label><input id="confirm-password" type={visible ? 'text' : 'password'} autoComplete="new-password" value={confirmPassword} disabled={loading} onChange={event => setConfirmPassword(event.target.value)} aria-invalid={!!errors.confirmPassword} aria-describedby={errors.confirmPassword ? 'confirm-error' : undefined} />{errors.confirmPassword && <p id="confirm-error" className="field-error">{errors.confirmPassword}</p>}<p className="connection-note">Use at least 8 characters for your password.</p></div>}
            {notice && <p className="auth-notice" role="status">{notice}</p>}
            {authError && <p className="auth-error" role="alert">{authError}</p>}
            <button className="primary-button" type="submit" disabled={loading}>{loading ? 'Please wait…' : mode === 'signup' ? 'Sign up' : 'Log in'}<span aria-hidden="true">↗</span></button>
          </form>
          <button className="auth-switch" disabled={loading} onClick={() => { setMode(mode === 'login' ? 'signup' : 'login'); setErrors({}); setAuthError(''); setNotice(''); setPassword(''); setConfirmPassword(''); setVisible(false) }}>{mode === 'login' ? 'New here? Create an account' : 'Already have an account? Log in'}</button>
          <div className="divider"><span />just looking around?<span /></div>
          <button className="demo-button" disabled={loading} onClick={() => navigate('setup')}>Continue in demo mode <span aria-hidden="true">→</span></button>
          <p className="demo-note"><span className="note-dot" /> A little preview. No account needed.</p>
          {!supabase && <p className="connection-note">Accounts are being connected. The demo is available now.</p>}
        </div> : null}

      </section>
    </main>}
    <footer><span lang="ja">お金ごっち</span><small className="footer-japanese" lang="ja">「小さな一歩を、いっしょに。」</small></footer>
  </div>
}

createRoot(document.getElementById('root')!).render(<StrictMode><App /></StrictMode>)
