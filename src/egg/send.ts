import type { Packet } from './contract.ts'
import { parsePacket, serializePacket } from './contract.ts'
import type { EggTransport, Scenario } from './transport.ts'
export type Phase = 'idle' | 'packing' | 'submitting' | 'flying'
export function pause(ms: number, signal: AbortSignal) {
  return new Promise<void>((resolve,reject)=>{
    if(signal.aborted) { reject(new Error('Cancelled')); return }
    const abort=()=>{clearTimeout(timer); reject(new Error('Cancelled'))}
    const timer=setTimeout(()=>{signal.removeEventListener('abort',abort); resolve()},ms)
    signal.addEventListener('abort',abort,{once:true})
  })
}
// UI timeouts do not prove cancellation of a remote submission. Callers retain
// the message ID and must reconcile status before attempting another letter.
export async function bounded<T>(operation: Promise<T>, ms=5000): Promise<T> {
  let timer: ReturnType<typeof setTimeout>
  try { return await Promise.race([operation,new Promise<T>((_,reject)=>{timer=setTimeout(()=>reject(Error('Delivery status is unknown. Check the saved letter before sending again.')),ms)})]) } finally { clearTimeout(timer!) }
}
export async function sendLetter(transport: EggTransport, packet: Packet, scenario: Scenario, signal: AbortSignal, phase: (p:Phase)=>void, reduced=false, wait=pause) {
  const frozen=parsePacket(serializePacket(packet))
  phase('packing'); await wait(reduced?0:1100,signal)
  if(signal.aborted) throw Error('Cancelled')
  phase('submitting'); const queued=await bounded(transport.submit(frozen,scenario))
  if(signal.aborted || queued.status==='failed') return queued
  phase('flying'); await wait(reduced?0:700,signal)
  if(signal.aborted) return queued
  return bounded(transport.status(frozen.messageId))
}
