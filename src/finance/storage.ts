import { accounts } from './fixtures.ts'
import type { SavedState, StorageLike } from './types.ts'
export const storageKey = (identity: string) => `okanegotchi:finance-demo:v1:${encodeURIComponent(identity)}`
export function readState(storage: StorageLike | undefined, identity: string): SavedState {
  const empty: SavedState = { version: 1, connections: [] }
  try {
    const raw = storage?.getItem(storageKey(identity))
    if (!raw) return empty
    const parsed = JSON.parse(raw)
    if (parsed.version !== 1 || !Array.isArray(parsed.connections)) return empty
    const seen = new Set<string>()
    for (const c of parsed.connections) {
      if (!c || typeof c.institutionId !== 'string' || seen.has(c.institutionId) || !['connected', 'attention', 'stale'].includes(c.status) || typeof c.refreshedAt !== 'string' || !Number.isFinite(Date.parse(c.refreshedAt)) || !Array.isArray(c.accountIds) || !c.accountIds.length || new Set(c.accountIds).size !== c.accountIds.length || !c.accountIds.every((id: unknown) => accounts.some(a => a.id === id && a.institutionId === c.institutionId))) return empty
      seen.add(c.institutionId)
    }
    return { version: 1, connections: parsed.connections }
  } catch { return empty }
}
export function writeState(storage: StorageLike | undefined, identity: string, state: SavedState) {
  try { storage?.setItem(storageKey(identity), JSON.stringify(state)); return !!storage } catch { return false }
}
export function browserStorage(): StorageLike | undefined { try { return window.localStorage } catch { return undefined } }
