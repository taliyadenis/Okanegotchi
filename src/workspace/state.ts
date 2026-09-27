import type { StorageLike } from '../finance/types.ts'
export const completionKey=(identity:string)=>`okanegotchi:onboarding:v1:${encodeURIComponent(identity)}`
export function completed(storage:StorageLike|undefined,identity:string){try{return storage?.getItem(completionKey(identity))==='true'}catch{return false}}
export function workspaceKeys(identity:string){return ['okanegotchi:finance-demo:v1:','okanegotchi:savings-goal:v1:','okanegotchi:demo-mail:v4:','okanegotchi:onboarding:v1:'].map(prefix=>prefix+encodeURIComponent(identity))}
export function capture(storage:StorageLike|undefined,identity:string){return workspaceKeys(identity).map(key=>storage?.getItem(key)??null)}
export function restore(storage:StorageLike|undefined,identity:string,values:unknown){
 if(!Array.isArray(values)||values.length!==4||values.some(v=>v!==null&&(typeof v!=='string'||v.length>100000)))throw Error('Invalid saved workspace')
 workspaceKeys(identity).forEach((key,i)=>{if(values[i]!==null)storage?.setItem(key,values[i]);else storage?.setItem(key,'null')})
}
