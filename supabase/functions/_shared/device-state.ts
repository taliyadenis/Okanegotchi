import {accounts,transactions,referenceDate} from '../../../src/finance/fixtures.ts'
import {uuid} from './http.ts'
type RequestBody={api_version:1;request_id:string;device_id:string;epoch:string|null;ack_command_seq:number;events:unknown[];telemetry:{firmware:string;asset_version:number;uptime_s:number;rssi_dbm:number}}
export function validateSync(v:unknown):asserts v is RequestBody {
  const r=v as RequestBody
  const keys=(o:unknown,expected:string[])=>!!o&&typeof o==='object'&&!Array.isArray(o)&&Object.keys(o).sort().join()===expected.sort().join()
  const integer=(n:unknown,min:number,max:number)=>Number.isSafeInteger(n)&&Number(n)>=min&&Number(n)<=max
  if(!keys(r,['api_version','request_id','device_id','epoch','ack_command_seq','events','telemetry'])||r.api_version!==1||!uuid(r.request_id)||!uuid(r.device_id)||!(r.epoch===null||uuid(r.epoch))||!integer(r.ack_command_seq,0,2147483647)||!Array.isArray(r.events)||r.events.length>8||
    !keys(r.telemetry,['firmware','asset_version','uptime_s','rssi_dbm'])||typeof r.telemetry.firmware!=='string'||!/^[A-Za-z0-9][A-Za-z0-9._+-]{0,31}$/.test(r.telemetry.firmware)||!integer(r.telemetry.asset_version,1,2147483647)||!integer(r.telemetry.uptime_s,0,4294967295)||!integer(r.telemetry.rssi_dbm,-127,0)||(r.epoch===null&&(r.events.length!==0||r.ack_command_seq!==0)))throw Error('Invalid sync request')
}
export function deviceState(request:RequestBody,result:any,now=new Date()) {
  const setup=result.documents.setup?.value
  if(!setup?.companion||!setup?.preferences)throw Error('Setup missing')
  const timezone=setup.preferences.timezone
  const date=new Intl.DateTimeFormat('en-CA',{timeZone:timezone,year:'numeric',month:'2-digit',day:'2-digit'}).format(now)
  const day=new Date(date+'T00:00:00Z'),weekday=day.getUTCDay()
  day.setUTCDate(day.getUTCDate()-((weekday+6)%7));const start=day.toISOString().slice(0,10);day.setUTCDate(day.getUTCDate()+7);const end=day.toISOString().slice(0,10)
  const connections=result.documents.finance?.value?.connections ?? []
  const ids=new Set(connections.flatMap((c:any)=>Array.isArray(c.accountIds)?c.accountIds.filter((id:unknown)=>accounts.some(a=>a.id===id&&a.institutionId===c.institutionId)):[]))
  const spend=transactions.filter(t=>ids.has(t.accountId)&&t.status==='posted'&&t.kind==='out'&&t.date>=start&&t.date<end).reduce((sum,t)=>sum+t.amount,0)
  const budget=setup.preferences.weeklyBudgetMinor
  const savedGoal=result.documents.goal?.value?.goal
  const account=accounts.find(a=>a.id===savedGoal?.accountId&&a.type==='savings'&&ids.has(a.id))
  const goal=account&&typeof savedGoal.name==='string'&&Number.isSafeInteger(savedGoal.target)&&savedGoal.target>0&&savedGoal.target<=2147483647?
    {name:savedGoal.name.slice(0,60),target_minor:savedGoal.target,saved_minor:account.balance,currency:'USD'}:null
  return {
    api_version:1,request_id:request.request_id,epoch:result.epoch,server_time:now.toISOString(),state_version:result.state_version,next_sync_ms:30000,demo_mode:false,
    pet:{pet_id:setup.companion.pet,name:setup.companion.name,palette:'mint',accessory:'none',asset_version:1},
    care:{stage:'content',elapsed_connected_ms:0,streak_days:0,timezone,local_date:date,current_window:Number(new Intl.DateTimeFormat('en-US',{timeZone:timezone,hour:'numeric',hourCycle:'h23'}).format(now))<12?'AM':'PM',am_complete:false,pm_complete:false},
    finance:{snapshot_version:result.state_version,data_source:'demo',as_of:referenceDate,currency:'USD',period_start:start,period_end:end,spend_minor:spend,budget_minor:budget,budget_status:budget===null?'no_budget':spend>budget?'over_budget':'on_track',summary:'Sample accounts only. '+(spend/100).toFixed(2)+' USD spent this week.'},
    goal,review:null,event_results:[],commands:[],
  }
}
