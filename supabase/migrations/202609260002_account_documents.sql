-- Website-only persistence. These documents are not authoritative device/bank state.
begin;
create table if not exists public.account_documents (
  user_id uuid not null references auth.users(id) on delete cascade,
  kind text not null check (kind in ('setup','finance','goal')),
  value text not null check (octet_length(value) <= 131072),
  revision bigint not null default 1 check (revision > 0),
  updated_at timestamptz not null default now(),
  primary key (user_id,kind)
);
alter table public.account_documents enable row level security;
revoke all on public.account_documents from anon, authenticated;
grant select on public.account_documents to authenticated;
create policy account_documents_owner_read on public.account_documents
  for select to authenticated using ((select auth.uid()) = user_id);

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
  select revision into next_revision from public.account_documents where user_id=owner_id and kind=document_kind;
  if coalesce(next_revision,0)<>expected_revision then
    if exists(select 1 from public.account_documents where user_id=owner_id and kind=document_kind and value=document_value) then
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
commit;
