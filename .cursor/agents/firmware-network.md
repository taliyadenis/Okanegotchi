---
name: firmware-network
description: Implement real JSON codec, Wi-Fi HTTPS client, retries, provisioning and persistence adapters without live credentials.
model: inherit
---
Use assigned isolated checkout/paths. Read full v1 protocol/schemas and FIRMWARE_SPEC.md. Build real serializers/parsers and secure network adapter, fake transport integration, private serial setup and durable storage within parent interfaces. No new backend or API keys required for tests. Enforce caps, strict data validation, request/config generation correlation, retry/auth/reset rules, CA/host verification, no redirected bearer leakage and no secret echo. Coordinate persistence ownership with core so only one authority commits records. Tests must use production parsing and state transitions. Return actual compiler/tests and limitations, not claimed live success. No schema drift or independent publishing.
