import { StrictMode, useEffect, useRef, useState } from 'react'
import { createRoot } from 'react-dom/client'
import type { FormEvent } from 'react'
import './styles.css'

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
  const [page, setPage] = useState<'login' | 'setup'>('login')
  const [email, setEmail] = useState('')
  const [password, setPassword] = useState('')
  const [visible, setVisible] = useState(false)
  const [errors, setErrors] = useState<{ email?: string; password?: string }>({})
  const [authError, setAuthError] = useState('')
  const [loading, setLoading] = useState(false)
  const heading = useRef<HTMLHeadingElement>(null)
  const firstRender = useRef(true)

  useEffect(() => {
    document.title = `${page === 'login' ? 'Welcome' : 'Your companion'} · Okanegotchi`
    if (firstRender.current) { firstRender.current = false; return }
    heading.current?.focus()
  }, [page])

  async function login(event: FormEvent<HTMLFormElement>) {
    event.preventDefault()
    const next = {
      email: !email.trim() ? 'Please enter your email address.' : !/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email.trim()) ? 'Enter a valid email address.' : undefined,
      password: !password ? 'Please enter your password.' : undefined,
    }
    setErrors(next)
    setAuthError('')
    if (next.email || next.password) {
      document.getElementById(next.email ? 'email' : 'password')?.focus()
      return
    }
    setLoading(true)
    // No credentials are sent or persisted. Authentication is not connected yet.
    await new Promise(resolve => setTimeout(resolve, 250))
    setPassword('')
    setLoading(false)
    setAuthError('Account login is not connected yet. Use demo mode to take a look around.')
  }

  function navigate(next: 'login' | 'setup') {
    setEmail(''); setPassword(''); setVisible(false); setErrors({}); setAuthError(''); setPage(next)
  }

  return <div className="site-shell">
    <header className="flex items-center justify-between gap-4">
      <a className="brand flex items-center gap-2" href="#" onClick={event => { event.preventDefault(); navigate('login') }} aria-label="Okanegotchi home"><PixelPet small /><span>okanegotchi<span className="brand-dot">.</span></span></a>
      <span className="header-note">Small steps. More heart.</span>
    </header>

    <main className="main-layout">
      <section className="story-panel">
        <div className="eyebrow"><span className="tiny-star">✦</span> YOUR EVERYDAY LITTLE COMPANION</div>
        <h1>Little pet.<br /><span>Big<br />personality.</span></h1>
        <p className="story-copy">A little companion for your financial journey.</p>
        <DeviceIllustration />
        <div className="story-footnote"><span>♡</span> Made for progress, one small step at a time.</div>
      </section>

      <section className="form-side" aria-label={page === 'login' ? 'Login' : 'Character setup'}>
        {page === 'login' ? <div className="form-card">
          <span className="section-number">01 / WELCOME HOME</span>
          <h2 ref={heading} tabIndex={-1}>Hey, you.</h2>
          <p className="card-intro">Good to see you. Your next little step starts here.</p>
          <form noValidate onSubmit={login}>
            <div className="field"><label htmlFor="email">Email address</label><input id="email" type="email" autoComplete="username" placeholder="you@example.com" value={email} disabled={loading} onChange={event => setEmail(event.target.value)} aria-invalid={!!errors.email} aria-describedby={errors.email ? 'email-error' : undefined} />{errors.email && <p id="email-error" className="field-error">{errors.email}</p>}</div>
            <div className="field"><label htmlFor="password">Password</label><div className="password-wrap"><input id="password" type={visible ? 'text' : 'password'} autoComplete="current-password" placeholder="Enter your password" value={password} disabled={loading} onChange={event => setPassword(event.target.value)} aria-invalid={!!errors.password} aria-describedby={errors.password ? 'password-error' : undefined} /><button className="password-toggle" type="button" disabled={loading} onClick={() => setVisible(!visible)} aria-label={visible ? 'Hide password' : 'Show password'} aria-pressed={visible}>{visible ? 'Hide' : 'Show'}</button></div>{errors.password && <p id="password-error" className="field-error">{errors.password}</p>}</div>
            {authError && <p className="auth-error" role="alert">{authError}</p>}
            <button className="primary-button" type="submit" disabled={loading}>{loading ? 'Checking…' : 'Log in'}<span aria-hidden="true">↗</span></button>
          </form>
          <div className="divider"><span />just looking around?<span /></div>
          <button className="demo-button" disabled={loading} onClick={() => navigate('setup')}>Continue in demo mode <span aria-hidden="true">→</span></button>
          <p className="demo-note"><span className="note-dot" /> A little preview. No account needed.</p>
          <p className="connection-note">Account login is coming soon. Try the demo for now.</p>
        </div> : <div className="form-card setup-card">
          <span className="section-number">02 / A NEW LITTLE FRIEND</span>
          <span className="demo-badge">DEMO PREVIEW</span>
          <h2 ref={heading} tabIndex={-1}>Let’s create your companion</h2>
          <p className="card-intro">Your companion will live on your Okanegotchi device. Here’s what you’ll choose when character creation is ready.</p>
          <ol className="steps">
            <li><span className="step-number">01</span><div><h3>Choose your pet</h3><p>Piggy, Cat, or Dragon.</p></div></li>
            <li><span className="step-number">02</span><div><h3>Give it a name</h3><p>Pick something that feels personal.</p></div></li>
            <li><span className="step-number">03</span><div><h3>Make it yours</h3><p>Choose a mint, coral, or lavender palette and an optional cap or scarf.</p></div></li>
          </ol>
          <div className="coming-soon"><span aria-hidden="true">✦</span> Character creation coming soon</div>
          <button className="demo-button back-button" onClick={() => navigate('login')}><span aria-hidden="true">←</span> Back to login</button>
        </div>}
        <p className="below-card">YOUR NEXT CHAPTER STARTS SMALL.</p>
      </section>
    </main>
    <footer className="flex flex-wrap items-center justify-between gap-3"><span>© {new Date().getFullYear()} Okanegotchi</span><span>A little companion. A lot of possibility. <span className="footer-star">✦</span></span></footer>
  </div>
}

createRoot(document.getElementById('root')!).render(<StrictMode><App /></StrictMode>)
