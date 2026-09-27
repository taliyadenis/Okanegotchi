-- READ ONLY. For the observed owner/document_key/content schema only.
-- Aggregate validation: no owner IDs, document contents or credential hashes.
begin transaction read only;
select count(*) as document_count,
  count(distinct owner) as document_owner_count,
  count(*) filter(where owner is null) as missing_owners,
  count(*) filter(where revision is null or revision < 1) as invalid_revisions,
  count(*) filter(where content is null or jsonb_typeof(content) <> 'object') as non_object_documents,
  count(*) filter(where octet_length(content::text)>131072) as oversized_documents,
  count(*) filter(where document_key not in ('setup','finance','goal') or document_key is null) as unmapped_document_keys,
  count(*) filter(where not exists(select 1 from auth.users u where u.id=owner)) as orphan_owners
from public.account_documents;

select case when document_key in ('setup','finance','goal') then document_key else '[unmapped]' end as known_document_kind,
  count(*) as documents,
  count(*) filter(where content->>'version' in ('1','4')) as recognized_version_count
from public.account_documents group by 1 order by 1;

select count(*) as duplicate_owner_key_groups from
  (select owner,document_key from public.account_documents group by owner,document_key having count(*)>1) duplicates;

select count(*) as device_count,
  count(*) filter(where id is distinct from device_id) as distinct_identifier_count,
  count(*) filter(where device_id is null) as missing_device_ids,
  count(*) filter(where device_token_sha256 is null or octet_length(device_token_sha256)<>32) as invalid_hash_lengths,
  count(*) filter(where not is_revoked) as active_devices,
  count(*) filter(where is_revoked is null) as missing_revocation_flags,
  count(*) filter(where not exists(select 1 from auth.users u where u.id=owner)) as orphan_owners
from public.registered_devices;

select count(*) as multiple_active_device_owners from
  (select owner from public.registered_devices where not is_revoked group by owner having count(*)>1) duplicates;
select count(*) as duplicate_device_id_groups from
  (select device_id from public.registered_devices group by device_id having count(*)>1) duplicates;
select count(*) as duplicate_token_hash_groups from
  (select device_token_sha256 from public.registered_devices group by device_token_sha256 having count(*)>1) duplicates;
commit;
