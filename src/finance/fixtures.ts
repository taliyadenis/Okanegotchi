import type { Account, Transaction } from './types.ts'
export const referenceDate = '2026-09-26T12:00:00.000Z'
export const institutions = [
  { id: 'clover', name: 'Clover Bank', symbol: '✿', description: 'A little room for everyday life.' },
  { id: 'pocket', name: 'Pocket Credit Union', symbol: '◈', description: 'Small steps, shared possibilities.' },
  { id: 'sunny', name: 'Sunny Savings', symbol: '☀', description: 'Something good to look forward to.' },
]
export const accounts: Account[] = [
  { id: 'clover-checking', institutionId: 'clover', name: 'Everyday pocket', type: 'checking', mask: '1042', currency: 'USD', balance: 245080, available: 241830 },
  { id: 'clover-savings', institutionId: 'clover', name: 'Rainy day fund', type: 'savings', mask: '2086', currency: 'USD', balance: 380000, available: 380000 },
  { id: 'pocket-checking', institutionId: 'pocket', name: 'Weekend wallet', type: 'checking', mask: '3310', currency: 'USD', balance: 82525, available: null },
  { id: 'pocket-credit', institutionId: 'pocket', name: 'Little adventures card', type: 'credit', mask: '4498', currency: 'USD', balance: 18450, available: 231550 },
  { id: 'sunny-savings', institutionId: 'sunny', name: 'Next chapter savings', type: 'savings', mask: '5021', currency: 'USD', balance: 125000, available: 125000 },
]
const samples: Array<[string, number, Transaction['kind'], string]> = [
  ['Little Leaf Café', 1250, 'out', 'Food'], ['Metro ride', 290, 'out', 'Transportation'],
  ['Sample paycheck', 120000, 'in', 'Income'], ['Bookshop refund', 1800, 'refund', 'Shopping'],
  ['Neighborhood market', 3250, 'out', 'Groceries'],
]
export const transactions: Transaction[] = accounts.flatMap((account, a) => samples.map(([description, amount, kind, category], i) => ({
  id: `demo-${account.id}-${i}`, accountId: account.id, description,
  date: `2026-09-${String(25 - i - a).padStart(2, '0')}`, amount, kind, category,
  status: i === 4 ? 'pending' as const : 'posted' as const, source: 'demo' as const,
})))
// Savings activity is deposits and interest; credit activity includes a payment,
// rather than treating a credit card as an income account.
for (const t of transactions) {
  const account = accounts.find(a => a.id === t.accountId)!
  if (account.type === 'savings') {
    t.description = t.id.endsWith('-0') ? 'Monthly interest' : 'Savings contribution'
    t.amount = t.id.endsWith('-0') ? 425 : 5000
    t.kind = t.id.endsWith('-0') ? 'in' : 'transfer'
    t.category = t.id.endsWith('-0') ? 'Interest' : 'Savings transfer'
  }
  if (account.type === 'credit' && t.kind === 'in') {
    t.description = 'Sample card payment'; t.amount = 15000; t.kind = 'transfer'; t.category = 'Card payment'
  }
}
transactions[1] = { ...transactions[1], description: 'To rainy day fund', amount: 10000, kind: 'transfer', category: 'Savings transfer', transferRef: 'demo-transfer-1' }
transactions[6] = { ...transactions[6], description: 'From everyday pocket', date: transactions[1].date, amount: 10000, kind: 'transfer', category: 'Savings transfer', transferRef: 'demo-transfer-1' }
export const money = (minor: number) => new Intl.NumberFormat('en-US', { style: 'currency', currency: 'USD' }).format(minor / 100)
export function totals(selected: Account[], activity: Transaction[]) {
  return { cash: selected.filter(a => a.type !== 'credit').reduce((s, a) => s + a.balance, 0), owed: selected.filter(a => a.type === 'credit').reduce((s, a) => s + a.balance, 0), spending: activity.filter(t => t.status === 'posted' && t.kind === 'out').reduce((s, t) => s + t.amount, 0) }
}
