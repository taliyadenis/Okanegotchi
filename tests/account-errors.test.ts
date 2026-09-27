import { test } from 'node:test'
import assert from 'node:assert/strict'
import { accountLoadMessage, accountRequestError, AccountSchemaError, AccountSessionError } from '../src/account/backend-errors.ts'
import { CloudStore, SaveConflict } from '../src/account/cloud-store.ts'

test('missing columns, tables and RPCs explain the account-service update without exposing server details', () => {
  for (const code of ['42703','42P01','42883','PGRST202','PGRST204','PGRST205']) {
    const error = accountRequestError(400, { code, message: 'private document content', details: 'private SQL details' })
    assert.ok(error instanceof AccountSchemaError)
    assert.match(accountLoadMessage(error), /signed in/)
    assert.doesNotMatch(accountLoadMessage(error), /private|migration|SQL/)
  }
})

test('session, stale-write, permission and network failures remain distinct', () => {
  assert.ok(accountRequestError(401, {}) instanceof AccountSessionError)
  assert.ok(accountRequestError(409, { code: '40001' }) instanceof SaveConflict)
  assert.match(accountLoadMessage(accountRequestError(403, { code: '42501' })), /contact the project team/)
  assert.match(accountLoadMessage(accountRequestError(502, null)), /connection/)
  assert.match(accountLoadMessage(new TypeError('Failed to fetch')), /connection/)
})

test('schema failure prevents hydration and never submits an empty account save', async () => {
  let saves = 0
  const store = new CloudStore('user:synthetic', {
    load: async () => { throw accountRequestError(400, { code: '42703' }) },
    save: async () => { saves++; return 1 },
  })
  await assert.rejects(store.hydrate(), AccountSchemaError)
  assert.notEqual(store.status, 'saved')
  assert.equal(saves, 0)
  store.dispose()
})
