import { accounts, transactions, referenceDate } from './fixtures.ts'
import { readState, writeState } from './storage.ts'
import { FinanceError } from './types.ts'
import type { FinancialProvider, Snapshot, StorageLike } from './types.ts'
export class DemoProvider implements FinancialProvider {
  private state
  private revision = 0
  private active = true
  private failRefresh = false
  private failConnect = false
  storageAvailable = true
  constructor(private identity: string, private storage?: StorageLike, private clock = () => referenceDate, private delay = () => new Promise<void>(r => setTimeout(r, 450))) {
    this.state = readState(storage, identity)
    this.storageAvailable = !!storage
  }
  snapshot(): Snapshot {
    const ids = this.state.connections.flatMap(c => c.accountIds)
    return structuredClone({ connections: this.state.connections, accounts: accounts.filter(a => ids.includes(a.id)), transactions: transactions.filter(t => ids.includes(t.accountId)) })
  }
  async list() { return this.snapshot() }
  private save() { this.storageAvailable = writeState(this.storage, this.identity, this.state) }
  private async wait(signal?: AbortSignal) {
    const revision = this.revision
    await this.delay()
    if (!this.active || signal?.aborted || revision !== this.revision) throw new FinanceError('cancelled', 'Operation cancelled. Please try again.')
  }
  async connect(institutionId: string, ids: string[], signal?: AbortSignal) {
    await this.wait(signal)
    if (this.failConnect) { this.failConnect = false; throw new FinanceError('network', 'Demo connection failed. Your selections are safe; try again.') }
    const unique = [...new Set(ids)]
    if (!unique.length || !unique.every(id => accounts.some(a => a.id === id && a.institutionId === institutionId))) throw new FinanceError('invalid', 'Select at least one available account.')
    const existing = this.state.connections.find(c => c.institutionId === institutionId)
    if (existing) existing.accountIds = [...new Set([...existing.accountIds, ...unique])]
    else this.state.connections.push({ institutionId, accountIds: unique, status: 'connected', refreshedAt: this.clock() })
    this.save()
  }
  async refresh(id: string) {
    await this.wait()
    const c = this.state.connections.find(c => c.institutionId === id)
    if (!c) throw new FinanceError('cancelled', 'Connection no longer exists.')
    if (c.status === 'attention') throw new FinanceError('attention', 'Reconnect this demo institution first.')
    if (this.failRefresh) { this.failRefresh = false; c.status = 'stale'; this.save(); throw new FinanceError('network', 'Demo refresh failed. Showing saved data from the last successful refresh.') }
    c.status = 'connected'; c.refreshedAt = this.clock(); this.save()
  }
  async reconnect(id: string) {
    await this.wait()
    const c = this.state.connections.find(c => c.institutionId === id)
    if (!c) throw new FinanceError('cancelled', 'Connection no longer exists.')
    c.status = 'connected'; c.refreshedAt = this.clock(); this.save()
  }
  async disconnect(id: string) { this.revision++; this.state.connections = this.state.connections.filter(c => c.institutionId !== id); this.save() }
  async reset() { this.revision++; this.state.connections = []; this.failRefresh = false; this.failConnect = false; this.save() }
  simulateRefreshFailure() { this.failRefresh = true }
  simulateConnectFailure() { this.failConnect = true }
  attention(id: string) { this.revision++; const c = this.state.connections.find(c => c.institutionId === id); if (c) { c.status = 'attention'; this.save() } }
  activate() { this.active = true }
  dispose() { this.active = false; this.revision++ }
}
