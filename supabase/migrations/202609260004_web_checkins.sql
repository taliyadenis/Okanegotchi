-- Web review/check-ins; separate from device review receipts and pet care.
begin;
create table public.web_financial_reviews (
 user_id uuid not null references auth.users(id) on delete cascade,
 id uuid not null, snapshot jsonb not null,
 timezone text not null, local_date date not null,
 "window" text not null check("window" in ('AM','PM')),
 created_at timestamptz not null default now(), expires_at timestamptz not null,
 consumed_at timestamptz, primary key(user_id,id)
);
create table public.web_checkins (
 user_id uuid not null references auth.users(id) on delete cascade,
 local_date date not null, "window" text not null check("window" in ('AM','PM')),
 timezone text not null, confirmed_at timestamptz not null default now(), review_id uuid not null,
 primary key(user_id,local_date,"window"),
 foreign key(user_id,review_id) references public.web_financial_reviews(user_id,id)
);
alter table public.web_financial_reviews enable row level security;
alter table public.web_checkins enable row level security;
revoke all on public.web_financial_reviews,public.web_checkins from anon,authenticated;
grant select on public.web_checkins to authenticated;
create policy own_web_checkins on public.web_checkins for select to authenticated using ((select auth.uid())=user_id);
create or replace function public.create_web_financial_review(owner_id uuid, request_id uuid, financial_snapshot jsonb)
returns jsonb language plpgsql security definer set search_path='' as $$
declare r public.web_financial_reviews; zone text; local_now timestamp; doc jsonb;
begin
 select * into r from public.web_financial_reviews where user_id=owner_id and id=request_id;
 if not found then
  select value::jsonb into doc from public.account_documents where user_id=owner_id and kind='setup';
  zone:=doc->'preferences'->>'timezone';
  if zone is null or not exists(select 1 from pg_catalog.pg_timezone_names where name=zone) then raise exception 'Save a timezone first'; end if;
  local_now:=now() at time zone zone;
  if financial_snapshot->>'timezone' is distinct from zone or financial_snapshot->>'localDate' is distinct from local_now::date::text or coalesce((financial_snapshot->>'accountCount')::int,0)<1 then raise exception 'Financial review must be refreshed'; end if;
  insert into public.web_financial_reviews(user_id,id,snapshot,timezone,local_date,"window",expires_at)
   values(owner_id,request_id,financial_snapshot,zone,local_now::date,case when extract(hour from local_now)<12 then 'AM' else 'PM' end,now()+interval '5 minutes')
   on conflict(user_id,id) do nothing;
  select * into r from public.web_financial_reviews where user_id=owner_id and id=request_id;
 end if;
 return jsonb_build_object('id',r.id,'createdAt',r.created_at,'expiresAt',r.expires_at,'localDate',r.local_date,'window',r."window",'timezone',r.timezone,'snapshot',r.snapshot);
end $$;
revoke all on function public.create_web_financial_review(uuid,uuid,jsonb) from public,anon,authenticated;
grant execute on function public.create_web_financial_review(uuid,uuid,jsonb) to service_role;
create or replace function public.confirm_web_checkin(review_id uuid)
returns jsonb language plpgsql security definer set search_path='' as $$
declare owner_id uuid:=auth.uid(); r public.web_financial_reviews; c public.web_checkins; local_now timestamp; zone text;
begin
 if owner_id is null then raise exception 'Sign in required' using errcode='42501'; end if;
 select * into r from public.web_financial_reviews where user_id=owner_id and id=review_id for update;
 if not found then raise exception 'Review not found' using errcode='22023'; end if;
 -- A lost response retries successfully even after its receipt expires.
 if r.consumed_at is null then
  select value::jsonb->'preferences'->>'timezone' into zone from public.account_documents where user_id=owner_id and kind='setup';
  local_now:=now() at time zone r.timezone;
  if zone is distinct from r.timezone or now()>=r.expires_at or local_now::date<>r.local_date or (case when extract(hour from local_now)<12 then 'AM' else 'PM' end)<>r."window" then raise exception 'Review expired or window changed; review again' using errcode='22023'; end if;
  insert into public.web_checkins(user_id,local_date,"window",timezone,review_id) values(owner_id,r.local_date,r."window",r.timezone,r.id) on conflict(user_id,local_date,"window") do nothing;
  update public.web_financial_reviews set consumed_at=now() where user_id=owner_id and id=r.id;
 end if;
 select * into c from public.web_checkins where user_id=owner_id and local_date=r.local_date and "window"=r."window";
 return jsonb_build_object('localDate',c.local_date,'window',c."window",'timezone',c.timezone,'confirmedAt',c.confirmed_at,'reviewId',c.review_id);
end $$;
revoke all on function public.confirm_web_checkin(uuid) from public,anon;
grant execute on function public.confirm_web_checkin(uuid) to authenticated;
commit;
