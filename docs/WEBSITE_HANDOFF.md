# Website team dependencies (no server code in this task)

Implement the existing mvp contract unchanged. Provide hosted project sync URL, companion login, device registration endpoint returning owner-bound device UUID and token once, revoke endpoint, and matching fixtures. Never send service-role/bank secrets to firmware. The device submits its opaque Bearer credential, not website JWT. It initiates outgoing HTTPS; no public device IP/port forwarding.

Pairing sequence: login companion -> create device -> copy returned credentials privately -> USB serial provisioning helper -> device bootstrap -> website last_seen. API URL and token are later deployment inputs, not required for offline compilation. Firmware implementation must document exact serial keys/CLI command and error handling.

Test common flow together: food12.50 -> eating8s; ride18.00 -> traveling8s; savings10.00 -> celebrate; change preset/palette/accessory IDs; tap->review receipt frozen snapshot; Bhold->checkin result; demo neglect->ghost; valid review->revive2s. Demo lease15minutes2s polling then30s. Website owns authenticated scenario/reset controls and backend care rules. Device never fabricates authoritative financial state.

Send shared assets using docs/ASSET_CONTRACT.md. Deliver one pet first. Existing mvp examples are historical fixtures: mock clock must be pinned or all timestamps rebased coherently, not treated as live data. Full manual/end-to-end result remains pending until both deployed service and hardware are available.

Revision2: use docs/MODULAR_ASSETS.md and docs/ADDING_CONTENT.md plus assets/catalog.example.json. Website artist supplies frames, anchors, clothes and backgrounds; firmware converter exports normalized preview assets for the website. Backgrounds are selected by device scene bindings in v1; a website background selector or multi-item outfit/new pet ID requires coordinated versioned contract work. Do not add fields to unchanged v1 responses.

Required demo stickers encode exact NDEF Text food/ride/savings/review commands in docs/NFC_DEMO_STICKERS.md. Website must enable the existing15min demo lease and accept device demo_trigger events through sync, with the same idempotent fixture rules as website scenario controls. Confirm food12.50/ride18.00/savings10.00. Neglect/reset remain website-only controls. Firmware does not animate spending optimistically; website and physical pet converge on server result. No NFC tag UID or PAN in API.
