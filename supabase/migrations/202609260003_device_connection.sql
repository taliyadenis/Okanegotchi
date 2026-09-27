begin;
create table public.registered_devices (
  id uuid primary key default gen_random_uuid(),
  user_id uuid not null references auth.users(id) on delete cascade,
  token_hash text not null unique check(token_hash ~ '^[a-f0-9]{64}$'),
  epoch uuid not null default gen_random_uuid(),
  state_version integer not null default 0,
  created_at timestamptz not null default now(),
  revoked_at timestamptz,
  last_seen timestamptz,
  firmware text,
  asset_version integer
);
create unique index one_active_device_per_user on public.registered_devices(user_id) where revoked_at is null;
alter table public.registered_devices enable row level security;
revoke all on public.registered_devices from anon,authenticated;
grant select,update on public.registered_devices to service_role;
grant select(id,user_id,created_at,revoked_at,last_seen,firmware,asset_version) on public.registered_devices to authenticated;
create policy devices_owner_read on public.registered_devices for select to authenticated using ((select auth.uid())=user_id);

create function public.register_owned_device(owner_id uuid,secret_hash text) returns uuid
language plpgsql security definer set search_path='' as $$
declare device uuid;
begin
  if not exists(select 1 from public.account_documents where user_id=owner_id and kind='setup') then
    raise exception 'Complete account setup first' using errcode='22023';
  end if;
  insert into public.registered_devices(user_id,token_hash) values(owner_id,secret_hash) returning id into device;
  return device;
end $$;
revoke all on function public.register_owned_device(uuid,text) from public,anon,authenticated;
grant execute on function public.register_owned_device(uuid,text) to service_role;

create function public.read_device_sync(secret_hash text,requested_device uuid,requested_epoch uuid,ack integer,firmware_version text,supported_assets integer)
returns jsonb language plpgsql security definer set search_path='' as $$
declare d public.registered_devices; documents jsonb;
begin
  select * into d from public.registered_devices where token_hash=secret_hash and revoked_at is null for update;
  if d.id is null or d.id<>requested_device then return jsonb_build_object('error','UNAUTHORIZED'); end if;
  if requested_epoch is not null and requested_epoch<>d.epoch then
    return jsonb_build_object('error','RESET_REQUIRED','epoch',d.epoch);
  end if;
  -- This milestone delivers state, not queued actions. No command has been delivered.
  if ack<>0 then return jsonb_build_object('error','ACK_OUT_OF_RANGE'); end if;
  update public.registered_devices set last_seen=now(),firmware=firmware_version,asset_version=supported_assets,state_version=state_version+1
    where id=d.id returning * into d;
  select coalesce(jsonb_object_agg(kind,jsonb_build_object('value',value::jsonb,'revision',revision,'updated_at',updated_at)),'{}'::jsonb)
    into documents from public.account_documents where user_id=d.user_id;
  return jsonb_build_object('epoch',d.epoch,'state_version',d.state_version,'documents',documents);
end $$;
revoke all on function public.read_device_sync(text,uuid,uuid,integer,text,integer) from public,anon,authenticated;
grant execute on function public.read_device_sync(text,uuid,uuid,integer,text,integer) to service_role;
commit;
