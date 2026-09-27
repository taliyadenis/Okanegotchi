import type { StorageLike } from '../finance/types.ts'
import { parsePacket, serializePacket, validateCompanion, validatePreferences, validateReceipt } from './contract.ts'
import type { Companion, Preferences, Packet, Receipt } from './contract.ts'
export type Scenario = 'online' | 'offline' | 'failure' | 'reject' | 'timeout' | 'duplicate'
export type Delivery = { packet: Packet; status: 'queued' | 'received' | 'failed' | 'unknown'; scenario: Scenario; note: string }
export interface EggTransport { submit(packet: Packet, scenario: Scenario): Promise<Delivery>; status(messageId: string, reconnect?: boolean): Promise<Delivery> }
type Mailbox = { onboardingComplete?: boolean; version: 4; revision: number; applied: Packet | null; companion: Companion | null; preferences: Preferences | null; history: Delivery[] }
const sessionMailboxes = new Map<string, Mailbox>()
const empty = (): Mailbox => ({version:4,revision:0,applied:null,companion:null,preferences:null,history:[]})
export const mailboxKey = (identity: string) => `okanegotchi:demo-mail:v4:${encodeURIComponent(identity)}`
export class DemoMailbox implements EggTransport {
  private state = empty()
  persistent: boolean
  constructor(private identity: string, private storage?: StorageLike) {
    this.persistent = !!storage
    let serialized: string | null = null
    try { serialized = storage?.getItem(mailboxKey(identity)) ?? null }
    catch { this.persistent = false; this.state = structuredClone(sessionMailboxes.get(identity) ?? empty()); return }
    if (!storage) { this.state = structuredClone(sessionMailboxes.get(identity) ?? empty()); return }
    try {
      const raw = JSON.parse(serialized ?? 'null')
      if (!raw) {
        // Preserve the previously chosen pet, but never rewrite frozen legacy letters.
        const legacy=JSON.parse(storage?.getItem(mailboxKey(identity).replace(':v4:',':v3:')) ?? storage?.getItem(mailboxKey(identity).replace(':v4:',':v2:')) ?? storage?.getItem(mailboxKey(identity).replace(':v4:',':v1:')) ?? 'null')
        if(legacy?.companion) {
          const { name, pet }=legacy.companion
          const companion={name,pet}; validateCompanion(companion)
          this.state.companion=companion; this.save()
        }
        return
      }
      if (raw.version !== 4 || !Number.isSafeInteger(raw.revision) || raw.revision < 0 || raw.revision > 2147483647 || !Array.isArray(raw.history) || raw.history.length > 8) throw Error()
      if (raw.preferences) validatePreferences(raw.preferences)
      if (raw.companion) validateCompanion(raw.companion)
      if (raw.applied) parsePacket(serializePacket(raw.applied))
      const ids = new Set()
      for (const d of raw.history) {
        parsePacket(serializePacket(d.packet))
        if (d.packet.revision > raw.revision || ids.has(d.packet.messageId) || !['queued','received','failed','unknown'].includes(d.status) || !['online','offline','failure','reject','timeout','duplicate'].includes(d.scenario) || typeof d.note !== 'string' || d.note.length > 300) throw Error()
        ids.add(d.packet.messageId)
      }
      if (raw.applied && raw.applied.revision > raw.revision) throw Error()
      this.state = {version:4,onboardingComplete:raw.onboardingComplete===true,revision:raw.revision,companion:raw.companion ?? null,preferences:raw.preferences ?? null,applied:raw.applied ?? null,history:raw.history}
    } catch { this.state = empty() }
  }
  snapshot() { return structuredClone(this.state) }
  private save() {
    try { this.storage?.setItem(mailboxKey(this.identity),JSON.stringify(this.state)); this.persistent=!!this.storage } catch { this.persistent=false }
    if (!this.persistent) sessionMailboxes.set(this.identity,structuredClone(this.state))
  }
  setCompanion(c: Companion) { validateCompanion(c); this.state.companion=structuredClone(c); this.save() }
  setSetup(c: Companion, preferences: Preferences) { validateCompanion(c); validatePreferences(preferences); this.state.companion=structuredClone(c); this.state.preferences=structuredClone(preferences); this.save() }
  nextRevision() { return this.state.revision + 1 }
  async submit(packet: Packet, scenario: Scenario) {
    const frozen = parsePacket(serializePacket(packet))
    let d = this.state.history.find(x=>x.packet.messageId===packet.messageId)
    if (d && serializePacket(d.packet)!==serializePacket(frozen)) throw Error('An existing message ID cannot contain changed data.')
    if (d && ['received','queued','unknown'].includes(d.status)) return structuredClone(d)
    if (!d && frozen.revision <= this.state.revision) throw Error('A new letter requires a newer revision.')
    if (this.state.applied && frozen.revision < this.state.applied.revision) throw Error('This old letter has been superseded.')
    if (!d) {
      if (this.state.history.some(x=>x.status==='queued'||x.status==='unknown')) throw Error('Resolve the pending letter before sending a new one.')
      d={packet:frozen,status:'queued',scenario,note:'Queued for your demo egg.'}
      this.state.history=[d,...this.state.history].slice(0,8); this.state.revision=frozen.revision
    }
    d.scenario=scenario
    if (scenario==='failure') { d.status='failed'; d.note='Your letter could not be queued. Retry the same letter.' }
    else { d.status='queued'; d.note='Your letter is waiting for your demo egg.' }
    this.save(); return structuredClone(d)
  }
  acceptReceipt(raw: unknown, d: Delivery) {
    const stored=this.state.history.find(x=>x.packet.messageId===d.packet.messageId)
    if(!stored || serializePacket(stored.packet)!==serializePacket(d.packet)) throw Error('No matching submitted letter.')
    d=stored
    validateReceipt(raw,d.packet)
    if (this.state.applied && this.state.applied.revision > d.packet.revision) throw Error('An older receipt cannot overwrite the current configuration.')
    this.state.applied=structuredClone(d.packet); d.status='received'; d.note='Received and applied by the demo egg.'; this.save()
  }
  async status(id: string, reconnect=false) {
    const d=this.state.history.find(x=>x.packet.messageId===id)
    if (!d) throw Error('No saved letter found.')
    if (d.status==='received'||d.status==='failed') return structuredClone(d)
    if (d.scenario==='offline'&&!reconnect) return structuredClone(d)
    if (d.scenario==='timeout'&&!reconnect) { d.status='unknown'; d.note='Receipt timed out. Delivery is unknown; check again before sending a new letter.'; this.save(); return structuredClone(d) }
    try {
      const wire=d.scenario==='reject' ? JSON.stringify({...d.packet,schema:'unsupported'}) : serializePacket(d.packet)
      const accepted=parsePacket(wire)
      const receipt: Receipt={messageId:accepted.messageId,destination:accepted.destination,appliedRevision:accepted.revision,outcome:'applied'}
      this.acceptReceipt(receipt,d)
      if(d.scenario==='duplicate') this.acceptReceipt(receipt,d)
    } catch(e) { d.status='failed'; d.note=e instanceof Error?e.message:'Demo egg rejected this packet.'; this.save() }
    return structuredClone(d)
  }
}
