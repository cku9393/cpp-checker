rewrite-r TOO_SMALL_FOR_SPQR tiny subtype instrumentation completed.

Campaign result:
- rewrite-r random correctness stayed green across the full staged campaign

Aggregate stats:
- rewriteCalls: 111100
- compactReadyCount: 63667 (57.31%)
- compactRejectedFallbackCount: 29972 (26.98%)
- compactTooSmallHandledCount: 0
- backendBuildRawDirectCount: 63667
- backendBuildRawFallbackCount: 0

Reject breakdown:
- TOO_SMALL_FOR_SPQR: 20429
- NOT_BICONNECTED: 9543
- EMPTY_AFTER_DELETE: 0
- OTHER: 0
- OWNER_NOT_R: 0
- SELF_LOOP: 0
- X_INCIDENT_VIRTUAL_UNSUPPORTED: 0
- X_NOT_PRESENT_IN_R: 0

TOO_SMALL_FOR_SPQR tiny subtype breakdown:
- TS_TWO_PATH: 20429
- TS_EMPTY: 0
- TS_ONE_EDGE: 0
- TS_OTHER: 0
- TS_TWO_DISCONNECTED: 0
- TS_TWO_OTHER: 0
- TS_TWO_PARALLEL: 0

Tiny subtype first dumps:
- TS_TWO_PATH: dumps/rewrite_r_tiny_rejects/reject_TS_TWO_PATH_seed1_tc3.txt

Next candidate:
- TS_TWO_PATH (20429)
