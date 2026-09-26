import { test } from 'node:test'
import assert from 'node:assert/strict'
import { parseTarget, goalProgress, goalKey, readGoal, saveGoal } from '../src/finance/goals.ts'
import { accounts } from '../src/finance/fixtures.ts'
const goal = { name: 'New bike', target: 500000, accountId: 'clover-savings' }
test('goal amounts parse as exact positive cents', () => { assert.equal(parseTarget('100.01'),10001); assert.equal(parseTarget(' 0.50 '),50); for(const v of ['0','-3','1.999','NaN','1e4','1,000','1000000000']) assert.equal(parseTarget(v),null) })
test('goal progress uses savings balance and caps completed percentage', () => { assert.deepEqual(goalProgress(goal,accounts),{saved:380000,remaining:120000,percent:76,complete:false}); assert.deepEqual(goalProgress({...goal,target:100000},accounts),{saved:380000,remaining:0,percent:100,complete:true}) })
test('missing or non-savings account has unavailable progress', () => { assert.equal(goalProgress(goal,[]),null); assert.equal(goalProgress({...goal,accountId:'clover-checking'},accounts),null) })
test('goal persistence isolates identities and removal leaves accounts alone', () => { const map=new Map<string,string>(); const storage={getItem:(k:string)=>map.get(k)??null,setItem:(k:string,v:string)=>{map.set(k,v)}}; saveGoal(storage,'guest',goal); assert.deepEqual(readGoal(storage,'guest'),goal); assert.equal(readGoal(storage,'user:one'),null); saveGoal(storage,'guest',null); assert.equal(readGoal(storage,'guest'),null); assert.equal(map.size,1) })
test('invalid saved goals and unavailable storage recover safely', () => { const storage={getItem:()=>'{',setItem:()=>{throw Error()}}; assert.equal(readGoal(storage,'guest'),null); assert.equal(saveGoal(storage,'guest',goal),false); assert.equal(saveGoal(undefined,'guest',goal),false); assert.equal(readGoal({getItem:()=>JSON.stringify({version:1,goal:{...goal,target:-1}}),setItem:()=>{}},'guest'),null); assert.ok(goalKey('user:one').includes('v1:')) })
