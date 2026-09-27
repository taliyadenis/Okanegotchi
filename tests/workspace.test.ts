import {test} from 'node:test'
import assert from 'node:assert/strict'
import {completed,completionKey,capture,restore,workspaceKeys} from '../src/workspace/state.ts'
const memory=()=>{const m=new Map<string,string>();return {getItem:(k:string)=>m.get(k)??null,setItem:(k:string,v:string)=>{m.set(k,v)}}}
test('onboarding completion stays independent from edits and identities',()=>{const s=memory();assert.equal(completed(s,'a'),false);s.setItem(completionKey('a'),'true');s.setItem(workspaceKeys('a')[1],JSON.stringify({goal:{name:'New name'}}));assert.equal(completed(s,'a'),true);assert.equal(completed(s,'b'),false)})
test('cloud snapshots restore the exact workspace only for the signed in identity',()=>{const a=memory(),b=memory();workspaceKeys('user:one').forEach((k,i)=>a.setItem(k,String(i)));restore(b,'user:one',capture(a,'user:one'));assert.deepEqual(capture(b,'user:one'),capture(a,'user:one'));assert.deepEqual(capture(b,'user:two'),[null,null,null,null])})
test('malformed cloud snapshots fail before modifying local state',()=>{const s=memory();s.setItem(completionKey('a'),'true');assert.throws(()=>restore(s,'a',['x',5,null,null]));assert.equal(completed(s,'a'),true);assert.throws(()=>restore(s,'a',[]))})
