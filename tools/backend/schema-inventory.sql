-- Read-only metadata inventory. No user documents, Auth records or device tokens.
begin transaction read only;
select 'columns' as section, to_jsonb(c) as detail
from (select table_name,column_name,data_type,udt_name,is_nullable,column_default
      from information_schema.columns where table_schema='public' order by table_name,ordinal_position) c
union all
select 'constraints', jsonb_build_object('table',c.relname,'name',co.conname,'definition',pg_get_constraintdef(co.oid))
from pg_constraint co join pg_class c on c.oid=co.conrelid join pg_namespace n on n.oid=c.relnamespace where n.nspname='public'
union all
select 'indexes',to_jsonb(i) from pg_indexes i where schemaname='public'
union all
select 'policies',to_jsonb(p) from pg_policies p where schemaname='public'
union all
select 'rls',jsonb_build_object('table',c.relname,'enabled',c.relrowsecurity,'forced',c.relforcerowsecurity)
from pg_class c join pg_namespace n on n.oid=c.relnamespace where n.nspname='public' and c.relkind in ('r','p')
union all
select 'table_grants',to_jsonb(g) from information_schema.role_table_grants g where table_schema='public'
union all
select 'column_grants',to_jsonb(g) from information_schema.role_column_grants g where table_schema='public'
union all
select 'functions',jsonb_build_object('name',p.proname,'arguments',pg_get_function_identity_arguments(p.oid),'result',pg_get_function_result(p.oid),'security_definer',p.prosecdef,'settings',p.proconfig,'acl',p.proacl,'definition',pg_get_functiondef(p.oid))
from pg_proc p join pg_namespace n on n.oid=p.pronamespace where n.nspname='public' and p.prokind='f'
union all
select 'migration_history_table',jsonb_build_object('exists',to_regclass('supabase_migrations.schema_migrations') is not null);
-- Additional dependencies matter when considering a rename or compatibility view.
select 'triggers' as section, jsonb_build_object('table',c.relname,'name',t.tgname,'definition',pg_get_triggerdef(t.oid)) as detail
from pg_trigger t join pg_class c on c.oid=t.tgrelid join pg_namespace n on n.oid=c.relnamespace
where n.nspname='public' and not t.tgisinternal
union all
select 'views',jsonb_build_object('name',viewname,'definition',definition) from pg_views where schemaname='public'
union all
select 'schema_acl',jsonb_build_object('name',nspname,'acl',nspacl) from pg_namespace where nspname='public'
union all
select 'relation_dependencies',jsonb_build_object('table',c.relname,'type',d.deptype,'dependent',pg_describe_object(d.classid,d.objid,d.objsubid))
from pg_depend d join pg_class c on c.oid=d.refobjid join pg_namespace n on n.oid=c.relnamespace
where d.refclassid='pg_class'::regclass and n.nspname='public' and c.relkind in ('r','p');
commit;
