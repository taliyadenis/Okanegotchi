import type { StorageLike } from '../finance/types.ts'

export type DocumentKind = 'setup' | 'finance' | 'goal'
export type CloudRow = { kind: DocumentKind; value: string; revision: number }
export interface CloudBackend {
  load(): Promise<CloudRow[]>
  save(kind: DocumentKind, value: string, revision: number): Promise<number>
}
export class SaveConflict extends Error {}
export type SyncStatus = 'loading' | 'saved' | 'saving' | 'error' | 'conflict'

/** A hydrated, identity-bound adapter for the existing synchronous demo models.
 * Writes are queued per document and acknowledged before setup can complete.
 * Revision checks prevent another browser's updates being silently overwritten.
 */
export class CloudStore implements StorageLike {
  private rows = new Map<DocumentKind, CloudRow>()
  private pending = new Map<DocumentKind, string>()
  private running: Promise<void> | null = null
  private listeners = new Set<() => void>()
  private closed = false
  status: SyncStatus = 'loading'
  constructor(readonly identity: string, private backend: CloudBackend) {}
  private keys(): Record<DocumentKind, string> {
    const id=encodeURIComponent(this.identity)
    return {setup:`okanegotchi:demo-mail:v4:${id}`,finance:`okanegotchi:finance-demo:v1:${id}`,goal:`okanegotchi:savings-goal:v1:${id}`}
  }
  private kind(key: string) { return (Object.entries(this.keys()).find(([,v])=>v===key)?.[0] ?? null) as DocumentKind|null }
  subscribe = (fn:()=>void) => { this.listeners.add(fn); return ()=>{this.listeners.delete(fn)} }
  private notify(status: SyncStatus) { if(this.closed)return;this.status=status;this.listeners.forEach(fn=>fn()) }
  async hydrate() {
    const rows=await this.backend.load()
    if(this.closed)return
    this.rows=new Map(rows.map(row=>[row.kind,row]));this.notify('saved')
  }
  private completionKey(){return `okanegotchi:onboarding:v1:${encodeURIComponent(this.identity)}`}
  getItem(key:string): string | null {
    if(key===this.completionKey()){
      const setup=this.getItem(this.keys().setup)
      try{return JSON.parse(setup??'null')?.onboardingComplete===true?'true':null}catch{return null}
    }
    const kind=this.kind(key)
    if(this.closed||!kind)return null
    return this.pending.get(kind) ?? this.rows.get(kind)?.value ?? null
  }
  setItem(key:string,value:string) {
    if(key===this.completionKey()){
      if(value!=='true'&&value!=='false')throw Error('Invalid onboarding state.')
      const setup=JSON.parse(this.getItem(this.keys().setup)??'null')
      if(!setup)throw Error('Save your pet setup first.')
      this.setItem(this.keys().setup,JSON.stringify({...setup,onboardingComplete:value==='true'}));return
    }
    const kind=this.kind(key)
    if(this.closed||!kind)throw Error('Account storage is unavailable.')
    if(new TextEncoder().encode(value).length>131072)throw Error('Saved data is too large.')
    JSON.parse(value)
    this.pending.set(kind,value)
    if(this.status!=='error'&&this.status!=='conflict')void this.flush().catch(()=>{})
  }
  async flush(): Promise<void> {
    if(this.closed)throw Error('Account session ended.')
    if(this.running) { await this.running; if(this.pending.size)return this.flush();return }
    if(this.status==='conflict')throw new SaveConflict('Another device saved newer data. Reload to review it.')
    if(!this.pending.size)return
    this.notify('saving')
    this.running=(async()=>{
      while(this.pending.size&&!this.closed) {
        const [kind,value]=this.pending.entries().next().value!
        const revision=await this.backend.save(kind,value,this.rows.get(kind)?.revision ?? 0)
        if(this.closed)return
        this.rows.set(kind,{kind,value,revision})
        if(this.pending.get(kind)===value)this.pending.delete(kind)
      }
      this.notify('saved')
    })().catch(error=>{this.notify(error instanceof SaveConflict?'conflict':'error');throw error})
    try {await this.running} finally {this.running=null}
  }
  hasUnsaved() {return this.pending.size>0}
  dispose() {this.closed=true;this.listeners.clear();this.pending.clear();this.rows.clear()}
}

const stores=new Map<string,CloudStore>()
export function accountStorage(identity:string) {return stores.get(identity)}
export function attachStorage(store:CloudStore) {stores.set(store.identity,store)}
export function detachStorage(store:CloudStore) {if(stores.get(store.identity)===store)stores.delete(store.identity);store.dispose()}

