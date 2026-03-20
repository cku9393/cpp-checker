# remaining fallback trigger decision

- current dominant remaining trigger = `RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED`
- trigger count = `1356`
- representative subtype = `XSR_HAFTER_LOOP_SHARED`
- representative subtype count = `1235`
- next safe fix target = `x-incident residual / XSR_HAFTER_LOOP_SHARED`
- 이번 단계에서는 behavior 변경 없음

## supporting counts

- `seqFallbackCaseCount = 2059`
- `seqRewriteWholeCoreFallbackCount = 3977`
- `rewriteFallbackTriggerCounts`
  - `RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED = 1356`
  - `RFT_COMPACT_EMPTY_AFTER_DELETE = 1235`
  - `RFT_COMPACT_TOO_SMALL_UNHANDLED = 824`
  - `RFT_COMPACT_BUILD_FAIL = 562`
- `seqXIncidentVirtualSubtypeCounts`
  - `XIV_SHARED_WITH_LOOP = 1356`
- `seqXSharedResidualSubtypeCounts`
  - `XSR_HAFTER_LOOP_SHARED = 1235`
  - `XSR_HAFTER_SPQR_READY = 121`

## interpretation

- 남은 fallback 1위는 이제 graft/postcheck 계열이 아니라 `x-incident virtual unsupported`다.
- 다음 단계는 새 handler 추가가 아니라, optimized baseline 기준 `XSR_HAFTER_LOOP_SHARED` 케이스를 재현하고 safe handling gate를 설계하는 쪽으로 가면 된다.
