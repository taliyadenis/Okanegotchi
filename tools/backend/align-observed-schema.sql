-- PRELUDE ONLY: assembled atomically with the baseline migrations by prepare-upgrade.mjs.
-- Never run this fragment or the historical migrations independently on the hosted project.
-- Preserves every row, original row UUID, revision, timestamp and original revoked flag.
lock table public.account_documents, public.registered_devices in access exclusive mode;
do $$
begin
  if (select array_agg(column_name::text order by ordinal_position) from information_schema.columns
      where table_schema='public' and table_name='account_documents') is distinct from
      array['id','owner','document_key','revision','content','created_at','updated_at'] then
    raise exception 'Unexpected account schema; repeat inventory before upgrade';
  end if;
  if (select array_agg(column_name::text order by ordinal_position) from information_schema.columns
      where table_schema='public' and table_name='registered_devices') is distinct from
      array['id','owner','device_id','device_token_sha256','is_revoked','last_seen_at','created_at','updated_at'] then
    raise exception 'Unexpected device schema; repeat inventory before upgrade';
  end if;
  if exists(select 1 from public.account_documents where document_key not in ('setup','finance','goal')) then
    raise exception 'Unmapped document_key: investigate client meaning before upgrade' using errcode='22023';
  end if;
  if exists(select 1 from public.account_documents where revision<1 or jsonb_typeof(content)<>'object'
    or octet_length(content::text)>131072
    or content->>'version' is distinct from case when document_key='setup' then '4' else '1' end) then
    raise exception 'Incompatible document revision, size, shape or version; preserve and resolve before upgrade' using errcode='22023';
  end if;
  if exists(select 1 from public.registered_devices where octet_length(device_token_sha256)<>32) then
    raise exception 'Invalid device SHA-256 length; coordinate reprovisioning before upgrade' using errcode='22023';
  end if;
  if exists(select 1 from public.registered_devices group by device_id having count(*)>1)
    or exists(select 1 from public.registered_devices group by device_token_sha256 having count(*)>1)
    or exists(select 1 from public.registered_devices where not is_revoked group by owner having count(*)>1) then
    raise exception 'Duplicate device identifier, hash or active owner; resolve without discarding records' using errcode='22023';
  end if;
end $$;

-- The inspected RPC accepts/returns device_id, while id is the internal row UUID.
-- Preserve the internal UUID separately; never substitute it for the external device identity.
alter table public.registered_devices drop constraint registered_devices_pkey;
alter table public.registered_devices rename column id to legacy_row_id;
alter table public.registered_devices add unique(legacy_row_id);
alter table public.registered_devices rename column device_id to id;
alter table public.registered_devices add primary key(id);
alter table public.registered_devices alter column id set default gen_random_uuid();
alter table public.registered_devices rename column owner to user_id;
alter table public.registered_devices rename column device_token_sha256 to token_hash;
alter table public.registered_devices alter column token_hash type text using encode(token_hash,'hex');
alter table public.registered_devices add unique(token_hash);
alter table public.registered_devices add check(token_hash ~ '^[a-f0-9]{64}$');
alter table public.registered_devices rename column last_seen_at to last_seen;
alter table public.registered_devices add column epoch uuid not null default gen_random_uuid(),
  add column state_version integer not null default 0,
  add column revoked_at timestamptz, add column firmware text, add column asset_version integer;
-- Legacy revoked_at stays NULL because its actual time was never recorded.
-- is_revoked remains authoritative and the aligned API filters both fields.

alter table public.account_documents rename column owner to user_id;
alter table public.account_documents rename column document_key to kind;
alter table public.account_documents rename column content to value;
alter table public.account_documents alter column value drop default;
alter table public.account_documents alter column value type text using value::text;
alter table public.account_documents alter column revision set default 1;
alter table public.account_documents add check(revision>0),
  add check(kind in ('setup','finance','goal')), add check(octet_length(value)<=131072);
alter table public.account_documents drop constraint account_documents_pkey;
alter table public.account_documents add unique(id);
alter table public.account_documents add primary key(user_id,kind);

-- Keep historical RPC definitions privately for recovery, with no callable public aliases.
-- This is an explicit API cutover; don't deploy if another client still uses these signatures.
create schema backend_legacy;
revoke all on schema backend_legacy from public,anon,authenticated,service_role;
alter function public.load_account_document(text) set schema backend_legacy;
alter function public.save_account_document(text,bigint,jsonb) set schema backend_legacy;
alter function public.register_device(uuid,bytea,boolean) set schema backend_legacy;
alter function public.revoke_device(uuid) set schema backend_legacy;
revoke all on all functions in schema backend_legacy from public,anon,authenticated,service_role;

drop policy account_documents_owner_read on public.account_documents;
drop policy account_documents_owner_update on public.account_documents;
drop policy account_documents_owner_upsert on public.account_documents;
drop policy registered_devices_owner_read on public.registered_devices;
drop policy registered_devices_owner_insert on public.registered_devices;
drop policy registered_devices_owner_update on public.registered_devices;
drop policy registered_devices_owner_delete on public.registered_devices;
