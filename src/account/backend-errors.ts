import { SaveConflict } from './cloud-store.ts'

export class AccountSchemaError extends Error {
  constructor() {
    super('You’re signed in, but saved account data is temporarily unavailable while we update the account service. Please try again after the update.')
    this.name = 'AccountSchemaError'
  }
}

export class AccountSessionError extends Error {
  constructor() {
    super('Your account session has expired or changed. Please log out and sign in again.')
    this.name = 'AccountSessionError'
  }
}

/** Classify by stable server codes; never surface document values or raw SQL errors. */
export function accountRequestError(status: number, body: unknown): Error {
  const code = body && typeof body === 'object' && 'code' in body ? body.code : undefined
  if (code === '40001') return new SaveConflict('Another device has saved newer data.')
  if (status === 401 || code === 'PGRST301' || code === 'PGRST303') return new AccountSessionError()
  if (['42703', '42P01', '42883', 'PGRST202', 'PGRST204', 'PGRST205'].includes(String(code))) {
    return new AccountSchemaError()
  }
  return new Error('Cloud account request failed.')
}

export function accountLoadMessage(error: unknown): string {
  if (error instanceof AccountSchemaError || error instanceof AccountSessionError) return error.message
  return 'We could not load your saved account. Check your connection and try again. If the problem continues, contact the project team.'
}
