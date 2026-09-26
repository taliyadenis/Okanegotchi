import type { Account, StorageLike } from './types.ts'
export type SavingsGoal = { name: string; target: number; accountId: string }
export const goalKey = (identity: string) => `okanegotchi:savings-goal:v1:${encodeURIComponent(identity)}`
export function parseTarget(value: string): number | null {
  if (!/^\d{1,9}(\.\d{1,2})?$/.test(value.trim())) return null
  const [whole, decimal = ''] = value.trim().split('.')
  const cents = Number(whole) * 100 + Number(decimal.padEnd(2, '0'))
  return cents > 0 ? cents : null
}
export function readGoal(storage: StorageLike | undefined, identity: string): SavingsGoal | null {
  try {
    const saved = JSON.parse(storage?.getItem(goalKey(identity)) ?? 'null')
    const g = saved?.goal
    if (saved?.version !== 1 || !g || typeof g.name !== 'string' || !g.name.trim() || g.name.length > 60 || !Number.isSafeInteger(g.target) || g.target <= 0 || g.target > 99999999999 || typeof g.accountId !== 'string') return null
    return { name: g.name, target: g.target, accountId: g.accountId }
  } catch { return null }
}
export function saveGoal(storage: StorageLike | undefined, identity: string, goal: SavingsGoal | null) {
  try { storage?.setItem(goalKey(identity), JSON.stringify({ version: 1, goal })); return !!storage } catch { return false }
}
export function goalProgress(goal: SavingsGoal, accounts: Account[]) {
  const account = accounts.find(a => a.id === goal.accountId && a.type === 'savings')
  if (!account) return null
  const saved = Math.max(0, account.balance)
  return { saved, remaining: Math.max(0, goal.target - saved), percent: Math.min(100, Math.floor(saved / goal.target * 100)), complete: saved >= goal.target }
}
