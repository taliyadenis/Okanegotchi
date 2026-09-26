import schema from './packet.schema.json' with { type: 'json' }
export const assets = { pet: ['gator', 'robot', 'duck'] } as const
// Legacy IDs remain readable in immutable saved letters; setup only offers current assets.
export type Companion = { name: string; pet: typeof assets.pet[number] | 'piggy' | 'cat' | 'dragon' }
export type Preferences = { timezone: string; weeklyBudgetMinor: number | null; currency: 'USD' }
export function validatePreferences(value: unknown): asserts value is Preferences {
  validate(value, schema.properties.preferences as Rule, 'Preferences')
  try { new Intl.DateTimeFormat('en-US', { timeZone: (value as Preferences).timezone }).format() } catch { throw Error('Choose a valid IANA timezone.') }
}
export type Packet = { schema: 'okanegotchi.demo-letter.v4'; kind: 'configuration'; messageId: string; revision: number; createdAt: string; sourceAt: string; source: 'demo'; destination: 'demo-egg'; companion: Companion; preferences: Preferences; goal: { id: 'primary-savings-goal'; revision: number; name: string; targetMinor: number; savedMinor: number; currency: 'USD'; basis: 'full-savings-balance' } }
type Rule = { type: string | string[]; enum?: unknown[]; minLength?: number; maxLength?: number; pattern?: string; minimum?: number; maximum?: number; required?: string[]; properties?: Record<string, Rule>; additionalProperties?: boolean }
function validate(value: unknown, rule: Rule, path: string) {
  if (Array.isArray(rule.type)) {
    if (value === null && rule.type.includes('null')) return
    return validate(value, {...rule, type: 'integer'}, path)
  }
  if (rule.type === 'object') {
    if (!value || typeof value !== 'object' || Array.isArray(value)) throw Error(`${path}: expected an object.`)
    const obj = value as Record<string, unknown>
    if (Object.keys(obj).some(k => !Object.hasOwn(rule.properties!, k)) || rule.required!.some(k => !Object.hasOwn(obj, k))) throw Error(`${path}: missing or unsupported fields.`)
    for (const [key, r] of Object.entries(rule.properties!)) validate(obj[key], r, `${path}.${key}`)
  } else if (rule.type === 'string') {
    if (typeof value !== 'string' || !value.trim() || [...value].length < (rule.minLength ?? 0) || [...value].length > (rule.maxLength ?? Infinity) || (rule.pattern && !new RegExp(rule.pattern).test(value))) throw Error(`${path}: invalid text or length.`)
  } else if (rule.type === 'integer' && (typeof value !== 'number' || !Number.isSafeInteger(value) || value < rule.minimum! || value > rule.maximum!)) throw Error(`${path}: invalid integer.`)
  if (rule.enum && !rule.enum.includes(value)) throw Error(`${path}: unsupported value.`)
}
export function validateCompanion(value: unknown): asserts value is Companion { validate(value, schema.properties.companion as Rule, 'Companion') }
export function validatePacket(value: unknown): asserts value is Packet {
  validate(value, schema as Rule, 'Packet')
  const p = value as Packet
  validatePreferences(p.preferences)
  for (const d of [p.createdAt, p.sourceAt]) if (!Number.isFinite(Date.parse(d)) || new Date(d).toISOString() !== d) throw Error('Packet has an invalid UTC date.')
  if (p.goal.revision !== p.revision) throw Error('Goal snapshot revision must match configuration revision.')
  if (new TextEncoder().encode(JSON.stringify(p)).byteLength > 2048) throw Error('Packet exceeds proposed 2048-byte limit.')
}
export function serializePacket(value: unknown) { validatePacket(value); return JSON.stringify(value) }
export function parsePacket(raw: string): Packet {
  if (new TextEncoder().encode(raw).byteLength > 2048) throw Error('Packet exceeds proposed 2048-byte limit.')
  const value: unknown = JSON.parse(raw); validatePacket(value); return value
}
export type Receipt = { messageId: string; destination: 'demo-egg'; appliedRevision: number; outcome: 'applied' }
export function validateReceipt(value: unknown, p: Packet): asserts value is Receipt {
  const r = value as Receipt
  if (!r || typeof r !== 'object' || Object.keys(r).sort().join(',') !== 'appliedRevision,destination,messageId,outcome' || r.messageId !== p.messageId || r.destination !== p.destination || r.appliedRevision !== p.revision || r.outcome !== 'applied') throw Error('Receipt does not match this letter.')
}
