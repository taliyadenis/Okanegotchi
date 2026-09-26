export type Account = { id: string; institutionId: string; name: string; type: 'checking' | 'savings' | 'credit'; mask: string; currency: 'USD'; balance: number; available: number | null }
export type Transaction = { id: string; accountId: string; description: string; date: string; amount: number; kind: 'out' | 'in' | 'refund' | 'transfer'; category: string; status: 'posted' | 'pending'; transferRef?: string; source: 'demo' }
export type Connection = { institutionId: string; accountIds: string[]; status: 'connected' | 'attention' | 'stale'; refreshedAt: string }
export type Snapshot = { connections: Connection[]; accounts: Account[]; transactions: Transaction[] }
export type SavedState = { version: 1; connections: Connection[] }
export type StorageLike = Pick<Storage, 'getItem' | 'setItem'>
export class FinanceError extends Error {
  constructor(public code: 'cancelled' | 'invalid' | 'attention' | 'network', message: string) { super(message) }
}
export interface FinancialProvider {
  list(): Promise<Snapshot>
  connect(institutionId: string, accountIds: string[], signal?: AbortSignal): Promise<void>
  refresh(institutionId: string): Promise<void>
  reconnect(institutionId: string): Promise<void>
  disconnect(institutionId: string): Promise<void>
}
