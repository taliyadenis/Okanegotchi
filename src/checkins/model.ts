import type { Snapshot, StorageLike } from '../finance/types.ts'
export type WindowName='AM'|'PM'
export function localDay(now:Date,timezone:string){const parts=new Intl.DateTimeFormat('en-US',{timeZone:timezone,year:'numeric',month:'2-digit',day:'2-digit'}).formatToParts(now);const value=(key:string)=>parts.find(p=>p.type===key)!.value;return `${value('year')}-${value('month')}-${value('day')}`}
export function localWindow(now:Date,timezone:string):WindowName{return Number(new Intl.DateTimeFormat('en-US',{timeZone:timezone,hour:'numeric',hourCycle:'h23'}).format(now))<12?'AM':'PM'}
export function shiftDay(date:string,offset:number){const d=new Date(date+'T12:00:00Z');d.setUTCDate(d.getUTCDate()+offset);return d.toISOString().slice(0,10)}
export function weekDays(date:string){const day=new Date(date+'T12:00:00Z').getUTCDay();const monday=shiftDay(date,-((day+6)%7));return Array.from({length:7},(_,i)=>shiftDay(monday,i))}
export type Summary={weekStart:string;weekEnd:string;localDate:string;timezone:string;spentMinor:number;refundMinor:number;pendingMinor:number;budgetMinor:number|null;accountCount:number;asOf:string|null;stale:boolean;categories:{name:string;amount:number}[]}
export function weeklySummary(data:Snapshot,timezone:string,budget:number|null,now=new Date()):Summary{
 const date=localDay(now,timezone),days=weekDays(date),end=shiftDay(days[6],1),ids=new Set(data.accounts.map(a=>a.id)),seen=new Set<string>(),categories=new Map<string,number>()
 let spent=0,refund=0,pending=0
 for(const t of data.transactions){if(!ids.has(t.accountId)||seen.has(t.id)||t.date<days[0]||t.date>=end||t.date>date)continue;seen.add(t.id)
  if(t.kind==='out'){if(t.status==='pending')pending+=t.amount;else{spent+=t.amount;categories.set(t.category,(categories.get(t.category)??0)+t.amount)}}else if(t.kind==='refund'&&t.status==='posted')refund+=t.amount
 }
 const dates=data.connections.map(c=>c.refreshedAt).filter(d=>Number.isFinite(Date.parse(d))).sort((a,b)=>Date.parse(a)-Date.parse(b));const asOf=dates[0]??null
 return {weekStart:days[0],weekEnd:days[6],localDate:date,timezone,spentMinor:spent,refundMinor:refund,pendingMinor:pending,budgetMinor:budget,accountCount:ids.size,asOf,stale:!asOf||now.getTime()-Date.parse(asOf)>86400000||data.connections.some(c=>c.status!=='connected'),categories:[...categories].map(([name,amount])=>({name,amount})).sort((a,b)=>b.amount-a.amount)}
}
export type Review={id:string;createdAt:string;expiresAt:string;localDate:string;window:WindowName;timezone:string;snapshot:Summary}
export type Checkin={localDate:string;window:WindowName;timezone:string;confirmedAt:string;reviewId:string}
export const checkinKey=(identity:string)=>`okanegotchi:checkins:v1:${encodeURIComponent(identity)}`
export function readCheckins(storage:StorageLike|undefined,identity:string):Checkin[]{
 const raw=storage?.getItem(checkinKey(identity));if(!raw)return []
 try{const value=JSON.parse(raw);if(value.version!==1||!Array.isArray(value.entries)||value.entries.length>730)throw Error();const seen=new Set<string>();for(const e of value.entries){const key=e.localDate+e.window;if(!/^\d{4}-\d{2}-\d{2}$/.test(e.localDate)||!['AM','PM'].includes(e.window)||typeof e.timezone!=='string'||typeof e.reviewId!=='string'||!Number.isFinite(Date.parse(e.confirmedAt))||seen.has(key))throw Error();seen.add(key)}return value.entries}catch{throw Error('Saved check-ins could not be read. Your history has not been overwritten.')}
}
export function createReview(snapshot:Summary,now=new Date(),id=crypto.randomUUID()):Review{if(!snapshot.accountCount)throw Error('Connect an account before checking in.');return {id,createdAt:now.toISOString(),expiresAt:new Date(now.getTime()+300000).toISOString(),localDate:localDay(now,snapshot.timezone),window:localWindow(now,snapshot.timezone),timezone:snapshot.timezone,snapshot:structuredClone(snapshot)}}
export function confirmReview(entries:Checkin[],review:Review,now=new Date()):Checkin[]{
 if(entries.some(e=>e.reviewId===review.id))return entries
 if(now.getTime()>=Date.parse(review.expiresAt)||now.getTime()<Date.parse(review.createdAt)||localDay(now,review.timezone)!==review.localDate||localWindow(now,review.timezone)!==review.window)throw Error('This review expired or the check-in window changed. Open a fresh review.')
 if(entries.some(e=>e.localDate===review.localDate&&e.window===review.window))return entries
 return [...entries,{localDate:review.localDate,window:review.window,timezone:review.timezone,confirmedAt:now.toISOString(),reviewId:review.id}].sort((a,b)=>a.confirmedAt.localeCompare(b.confirmedAt)).slice(-730)
}
