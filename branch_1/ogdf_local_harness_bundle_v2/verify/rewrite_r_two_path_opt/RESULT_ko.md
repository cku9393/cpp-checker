rewrite-r TS_TWO_PATH local synthetic mini optimization completed.

Campaign result:
- rewrite-r random correctness stayed green across the full staged campaign

Baseline vs new stats:
- rewriteCalls: 111100 -> 111100
- compactReadyCount: 63667 (57.31%) -> 63667 (57.31%)
- compactRejectedFallbackCount: 29972 (26.98%) -> 9543 (8.59%)
- compactTooSmallHandledCount: 20429
- compactTooSmallTwoPathHandledCount: 20429
- rewriteFallbackWholeCoreCount: 29972 -> 9543
- rewriteFallbackSpecialCaseCount: 17461 -> 37890
- backendBuildRawDirectCount: 63667 -> 63667
- backendBuildRawFallbackCount: 0 -> 0

Reject breakdown:
- NOT_BICONNECTED: 9543 -> 9543
- EMPTY_AFTER_DELETE: 0 -> 0
- OTHER: 0 -> 0
- OWNER_NOT_R: 0 -> 0
- SELF_LOOP: 0 -> 0
- TOO_SMALL_FOR_SPQR: 20429 -> 0
- X_INCIDENT_VIRTUAL_UNSUPPORTED: 0 -> 0
- X_NOT_PRESENT_IN_R: 0 -> 0

Too-small subtype breakdown:
- TS_TWO_PATH: 20429 -> 20429
- TS_EMPTY: 0 -> 0
- TS_ONE_EDGE: 0 -> 0
- TS_OTHER: 0 -> 0
- TS_TWO_DISCONNECTED: 0 -> 0
- TS_TWO_OTHER: 0 -> 0
- TS_TWO_PARALLEL: 0 -> 0

Tiny subtype first dumps:
- TS_TWO_PATH: dumps/rewrite_r_tiny_rejects/reject_TS_TWO_PATH_seed1_tc3.txt

Conclusion:
- correctness stayed green
- TS_TWO_PATH special handling is active
- fallback ratio decreased relative to the 26.98% baseline
