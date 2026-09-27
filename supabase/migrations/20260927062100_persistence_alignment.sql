-- Same final persistence contract after fresh install or observed-schema upgrade.
begin;
alter table public.registered_devices add column if not exists is_revoked boolean not null default false;
update public.registered_devices set is_revoked=true where revoked_at is not null and not is_revoked;
-- Preserve legacy boolean revocation without fabricating its unknown timestamp.
create or replace function public.keep_device_revocation() returns trigger
language plpgsql set search_path='' as $$
begin
  if new.revoked_at is not null then new.is_revoked:=true; end if;
  if old.is_revoked and not new.is_revoked then
    raise exception 'Revoked credentials cannot be reactivated; register a new device' using errcode='22023';
  end if;
  return new;
end $$;
revoke all on function public.keep_device_revocation() from public,anon,authenticated;
create trigger keep_device_revocation before update on public.registered_devices
for each row execute function public.keep_device_revocation();
drop index if exists public.one_active_device_per_user;
create unique index one_active_device_per_user on public.registered_devices(user_id)
where not is_revoked and revoked_at is null;
-- Deny all direct browser writes, including revision bypass and token changes.
revoke all on public.account_documents,public.registered_devices from anon,authenticated;
grant select on public.account_documents to authenticated;
grant select(id,user_id,created_at,revoked_at,last_seen,firmware,asset_version) on public.registered_devices to authenticated;
create or replace function public.save_account_document(
  document_kind text, document_value text, expected_revision bigint
) returns bigint language plpgsql security definer set search_path = '' as $$
declare owner_id uuid := auth.uid(); parsed jsonb; next_revision bigint;
begin
  if owner_id is null then raise exception 'Authentication required' using errcode='42501'; end if;
  if document_kind is null or document_kind not in ('setup','finance','goal')
    or document_value is null or octet_length(document_value)>131072
    or expected_revision is null or expected_revision<0 then
    raise exception 'Invalid document' using errcode='22023';
  end if;
  parsed := document_value::jsonb;
  if jsonb_typeof(parsed)<>'object' then raise exception 'Expected object' using errcode='22023'; end if;
  if document_kind='setup' then
    if parsed->>'version' is distinct from '4'
      or jsonb_typeof(parsed->'companion') is distinct from 'object'
      or coalesce(parsed->'companion'->>'pet','') not in ('gator','robot','duck')
      or length(btrim(coalesce(parsed->'companion'->>'name',''))) not between 1 and 24
      or jsonb_typeof(parsed->'preferences') is distinct from 'object'
      or parsed->'preferences'->>'currency' is distinct from 'USD'
      or not exists(select 1 from pg_catalog.pg_timezone_names where name=parsed->'preferences'->>'timezone')
      or not (parsed->'preferences' ? 'weeklyBudgetMinor')
      or (parsed->'preferences'->'weeklyBudgetMinor'<>'null'::jsonb and
        (coalesce(parsed->'preferences'->>'weeklyBudgetMinor','') !~ '^[0-9]+$'
         or (parsed->'preferences'->>'weeklyBudgetMinor')::numeric not between 1 and 99999999999))
    then raise exception 'Invalid setup' using errcode='22023'; end if;
  elsif parsed->>'version' is distinct from '1' then
    raise exception 'Invalid document version' using errcode='22023';
  end if;
  -- Serialize compare-and-swap even for the first write; a lost response can retry safely.
  perform pg_catalog.pg_advisory_xact_lock(pg_catalog.hashtextextended(owner_id::text || ':' || document_kind,0));
  select ad.revision into next_revision
    from public.account_documents as ad
    where ad.user_id=owner_id and ad.kind=document_kind;
  if coalesce(next_revision,0)<>expected_revision then
    if exists(select 1 from public.account_documents where user_id=owner_id and kind=document_kind and value::jsonb=parsed) then
      return next_revision;
    end if;
    raise exception 'Saved on another device; reload first' using errcode='40001';
  end if;
  next_revision := coalesce(next_revision,0)+1;
  insert into public.account_documents(user_id,kind,value,revision) values(owner_id,document_kind,document_value,next_revision)
    on conflict(user_id,kind) do update set value=excluded.value,revision=excluded.revision,updated_at=now();
  return next_revision;
end $$;
revoke all on function public.save_account_document(text,text,bigint) from public,anon;
grant execute on function public.save_account_document(text,text,bigint) to authenticated;

create or replace function public.read_device_sync(secret_hash text,requested_device uuid,requested_epoch uuid,ack integer,firmware_version text,supported_assets integer)
returns jsonb language plpgsql security definer set search_path='' as $$
declare d public.registered_devices; documents jsonb;
begin
  select * into d from public.registered_devices where token_hash=secret_hash and not is_revoked and revoked_at is null for update;
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

