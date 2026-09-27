import { useEffect, useRef, useState } from 'react'
import manifest from '../../assets/manifest.json'
import './character.css'

export type Pet = keyof typeof manifest.presets
type Animation = keyof typeof manifest.animations
export function CharacterCreator({pet,onPetChange,name='okanegotchi'}:{pet:Pet;onPetChange:(pet:Pet)=>void;name?:string}) {
  const [animation,setAnimation]=useState<Animation>('idle')
  const [playing,setPlaying]=useState(()=>!window.matchMedia('(prefers-reduced-motion: reduce)').matches)
  const [retry,setRetry]=useState(0)
  const [replay,setReplay]=useState(0)
  const [assets,setAssets]=useState<Record<string,HTMLImageElement>>({})
  const [error,setError]=useState('')
  const [frame,setFrame]=useState(0)
  const canvas=useRef<HTMLCanvasElement>(null)
  const ready=Object.keys(assets).length===Object.keys(manifest.presets).length*Object.keys(manifest.animations).length

  useEffect(()=>{const query=window.matchMedia('(prefers-reduced-motion: reduce)');const changed=()=>{if(query.matches)setPlaying(false)};query.addEventListener('change',changed);return()=>query.removeEventListener('change',changed)},[])

  useEffect(()=>{
    let active=true
    setError('')
    const images:HTMLImageElement[]=[]
    const jobs=Object.keys(manifest.presets).flatMap(p=>Object.entries(manifest.animations).map(([state,a])=>new Promise<[string,HTMLImageElement]>((resolve,reject)=>{
      const image=new Image();images.push(image)
      image.onload=()=>image.naturalWidth===32*a.frames&&image.naturalHeight===32?resolve([p+'_'+state,image]):reject(Error('Incompatible artwork'))
      image.onerror=()=>reject(Error('Missing artwork'))
      image.src=import.meta.env.BASE_URL+'assets/v1/'+p+'_'+state+'.png'+(retry?'?retry='+retry:'')
    })))
    void Promise.all(jobs).then(entries=>{if(active)setAssets(Object.fromEntries(entries))}).catch(()=>{if(active)setError('The artwork could not load. Try loading it again.')})
    return()=>{active=false;images.forEach(image=>{image.onload=null;image.onerror=null})}
  },[retry])

  useEffect(()=>{
    setFrame(0)
    if(!playing||!ready) return
    const a=manifest.animations[animation], start=performance.now()
    const timer=window.setInterval(()=>{
      const elapsed=performance.now()-start
      if(a.duration_ms!==null&&elapsed>=a.duration_ms){setAnimation('idle');return}
      setFrame(Math.floor(elapsed/a.frame_ms)%a.frames)
    },Math.min(a.frame_ms,100))
    return()=>clearInterval(timer)
  },[pet,animation,playing,ready,replay])

  useEffect(()=>{
    const image=assets[pet+'_'+animation], target=canvas.current?.getContext('2d')
    if(!image||!target) return // Keep the previous valid image until replacement assets are ready.
    target.imageSmoothingEnabled=false
    target.clearRect(0,0,128,128)
    target.drawImage(image,(frame%manifest.animations[animation].frames)*32,0,32,32,0,0,128,128)
  },[assets,pet,animation,frame])

  return <div className="character-creator">
    <p className="card-intro">Three little personalities. Choose a pet and try their moves.</p>
    <div className="pet-options" role="group" aria-label="Choose a companion">
      {(Object.keys(manifest.presets) as Pet[]).map(p=><button type="button" key={p} aria-pressed={pet===p} onClick={()=>{onPetChange(p);setFrame(0);setReplay(r=>r+1)}}>{manifest.presets[p].label}</button>)}
    </div>
    <div className="pet-stage">
      <span className="pet-stage-label">{manifest.animations[animation].label}</span>
      <canvas ref={canvas} width={128} height={128} role="img" aria-label={(name.trim() || 'okanegotchi')+': '+manifest.animations[animation].label} />
      <strong>{name.trim() || 'okanegotchi'}</strong>
      <button type="button" className="play-toggle" disabled={!ready} onClick={()=>setPlaying(p=>!p)}>{playing?'Pause animation':'Play animation'}</button>
      {!ready&&!error&&<span role="status">Loading your companions…</span>}
    </div>
    {error&&<div role="alert" className="auth-error">{error} <button type="button" onClick={()=>setRetry(r=>r+1)}>Retry artwork</button></div>}
    <fieldset><legend>Try an animation</legend><div className="animation-options">{(['idle','traveling','eating','celebrate','sleepy','needs_checkin','ghost','revive'] as Animation[]).map(a=><button key={a} type="button" disabled={!ready} aria-pressed={animation===a} onClick={()=>{setAnimation(a);setFrame(0);setReplay(r=>r+1)}}>{a==='idle'?'Idle':a==='celebrate'?'Celebrating':manifest.animations[a].label}</button>)}</div></fieldset>

  </div>
}
