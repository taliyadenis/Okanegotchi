# Demo sticker support is required

This supersedes earlier optional-scenario wording. Implement normal card review and four public NTAG213 NDEF Text stickers. Stickers hold no credentials/account IDs/payment data and are not authentication.

| Label | Exact UTF-8 Text, language en | Live demo action |
|---|---|---|
| FOOD | `okanegachi:demo:v1:food` | demo_trigger food -> backend12.50USD fixture -> eating8s |
| RIDE | `okanegachi:demo:v1:ride` | demo_trigger ride -> backend18.00USD fixture -> traveling8s |
| SAVE | `okanegachi:demo:v1:savings` | demo_trigger savings -> goal+10.00USD -> celebrate8s |
| CHECK IN | `okanegachi:demo:v1:review` | review_requested source sticker -> frozen summary -> Bhold |

No tag neglect/reset action exists in v1: use authenticated website controls. No tap auto-completes check-in/revival. Outside demo mode, recognized scenario stickers open normal financial review with a Demo off hint.

## Write and verify

Use an NFC-writing-capable phone/app. Add ONE **Text** record (not URL), UTF-8/en, paste exact string, write blank team-owned sticker and read back record type/text. No whitespace/newline, extra Android application record, password protection or permanent locking. Label tags and test each on the reader. Do not write to a credit card. App-specific menu wording is not part of firmware requirements.

`tools/nfc_records.py` and tests/nfc/*.hex provide developer fixtures: NDEF TLV bytes for tag user memory, not full card dumps or phone transfer files. NTAG213 has144 user bytes; these texts fit [NXP](https://www.nxp.com/docs/en/data-sheet/NTAG213_215_216.pdf). A phone writer is sufficient. An optional separate USB maintenance writer may target explicitly selected, positively identified writable NTAG213 test tags, user pages only, with readback; never auto-format unidentified cards, alter CC/lock/password/config/manufacturer pages or write in normal runtime.

## Identify and read correctly

Bounded presence -> latch presentation -> classify Type2 -> bounded NDEF read -> parse exact command -> gate demo -> enqueue stable event -> HTTPS -> server validates lease -> returned command -> local animation. For non-Type2/ambiguous/contactless card, generic review source card; no payment APDUs. Not all credit cards guaranteed readable.

Do not identify Type2/NTAG213 by UID length alone. Verify selection/protocol metadata and supported tag identification before page commands. Standard Adafruit public readPassiveTargetID exposes UID/length, not every needed classification/readiness field. Inspect pinned source; a narrowly scoped licensed local adapter/library patch exposing ATQA/SAK/readiness is allowed if necessary, with tests. Do not invent public getters or call private methods. ISO14443-4/payment cards must not get speculative Type2 reads. NTAG-specific identification/CC checks only on supported non-payment path. Missing reliable classification degrades to generic review and is a reported sticker feature blocker, not falsely marked completed.

Verified NTAG213: CC page3; user pages4..39 (144 bytes). A four-page READ should finish at start page36; never cross into configuration. Obey actual library read-buffer size and verified capacity. Other supported Type2 tags use same144-byte message cap with verified bounds; ambiguous tags generic review. Read incrementally/yield SPI between short operations, decode once per presentation, not each render frame. No UID/PAN in storage/logs/HTTP; library may temporarily hold UID for selection only.

Parser restricted profile matches tools/nfc_records.py: null TLVs, valid length3 lock/memory-control TLVs skipped, one NDEF TLV and terminator. Stop at0xFE; ignore remaining bounded storage bytes, which may contain old data from a longer previous record. Never parse that stale data as an extra command. One short unchunked well-known Text record with MB/ME, UTF-8, no ID, ASCII language1-8 bytes. Strip status/language before exact case-sensitive text matching. Reject truncation/overflow/>144bytes/multiple records before terminator/UTF-16/URI/unknown version/extra suffix/newline/NUL. Well-formed unrelated text may request review. Malformed or application-prefixed unsupported text shows Unrecognized sticker, no synthetic action. Never execute tag text or accept tag-supplied URLs/tokens/amounts.

## Gating and retries

- Scenario creation requires connected, nonfaulted network and validated latest sync demo_mode=true no older than10s. Website's15min lease is still enforced server-side; client flag alone is not authorization.
- Stale/unknown state: retain at most one VOLATILE pending intent <=10s, request fresh sync, mark presentation consumed immediately. Removal may occur while waiting. Recheck device/epoch/generation, health and demo flag before creating one event; reset/reprovision/auth failure/timeout cancels intent. Fresh false flag falls back to normal review. No stale tap should fire minutes later or after reconnect while tag stays held.
- Offline scenario: Connect to run live demo, no NEW synthetic outbox action. An already created durable action still retries with identical ID/payload; DEMO_DISABLED after lease expiry is terminal and shown, not circumvented by a new ID.
- One event per presentation. Rearm only after continuous confirmed no-target observations for>=500ms; read errors/timeouts reset the absence window and are not absence. Two300ms absence intervals separated by an error do not rearm. New deliberate removal/tap creates a new event. Do not use UID as permanent identity/dedup key.
- No optimistic financial animation in live mode. Tap/pending cue first; backend command starts eating/traveling. LOCAL DEMO build can route same tags to isolated synthetic state and labeled local animation, never production events/cache.

Judge flow: website enables demo -> FOOD/eat -> remove -> RIDE/travel -> SAVE/goal -> CHECK IN/review/Bhold -> website neglect -> ghost -> CHECK IN/Bhold/revive. Disable demo to show scenario tag becomes normal review. Keep button fallback and clearly labeled offline rehearsal build.

Test actual C++ parser against byte fixtures; also stale/false/fresh demo, refresh timeout/reset/epoch changes, auth failure, read error vs absence, held tag, offline no-new-action, retry stable ID, server rejection, unknown Type2/credit-card fallback. Phone/reader writes and final enclosure readability remain physical checks.
