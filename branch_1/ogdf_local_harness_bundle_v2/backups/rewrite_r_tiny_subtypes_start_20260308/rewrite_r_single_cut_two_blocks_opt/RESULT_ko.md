rewrite-r NB_SINGLE_CUT_TWO_BLOCKS local synthetic mini optimization completed.

Campaign result:
- rewrite-r random correctness stayed green across the full staged campaign

Baseline vs new stats:
- rewriteCalls: 111100 -> 111100
- compactReadyCount: 63667 (57.31%) -> 63667 (57.31%)
- compactRejectedFallbackCount: 47433 (42.69%) -> 29972 (26.98%)
- compactSingleCutTwoBlocksHandled: 17461
- compactPathOfBlocksHandled: 0
- rewriteFallbackWholeCoreCount: 29972
- rewriteFallbackSpecialCaseCount: 17461
- backendBuildRawDirectCount: 63667 -> 63667
- backendBuildRawFallbackCount: 0 -> 0

Reject breakdown:
- TOO_SMALL_FOR_SPQR: 20429 -> 20429
- NOT_BICONNECTED: 27004 -> 9543
- EMPTY_AFTER_DELETE: 0 -> 0
- OTHER: 0 -> 0
- OWNER_NOT_R: 0 -> 0
- SELF_LOOP: 0 -> 0
- X_INCIDENT_VIRTUAL_UNSUPPORTED: 0 -> 0
- X_NOT_PRESENT_IN_R: 0 -> 0

NOT_BICONNECTED subtype breakdown:
- NB_SINGLE_CUT_TWO_BLOCKS: 17461 -> 17461
- NB_PATH_OF_BLOCKS: 9543 -> 9543
- NB_BLOCKS_ALL_TINY: 0 -> 0
- NB_COMPLEX_MULTI_CUT: 0 -> 0
- NB_DISCONNECTED: 0 -> 0
- NB_OTHER: 0 -> 0
- NB_STAR_AROUND_ONE_CUT: 0 -> 0

Subtype first dumps:
- NB_PATH_OF_BLOCKS: dumps/rewrite_r_rejects/reject_NB_PATH_OF_BLOCKS_seed1_tc14.txt
- NB_SINGLE_CUT_TWO_BLOCKS: dumps/rewrite_r_rejects/reject_NB_SINGLE_CUT_TWO_BLOCKS_seed1_tc2.txt

Conclusion:
- correctness stayed green
- NB_SINGLE_CUT_TWO_BLOCKS special handling is active
- fallback ratio decreased relative to the 42.69% baseline
