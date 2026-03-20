# RESULT

- correctness: green
- representative replay: `seed=2, tc=444, step=1` green
- replay result: `actualInvariantOk=1`, `oracleEquivalentOk=1`
- rewriteSeqCalls: `110100`
- seqSelfLoopRemainderOneEdgeAttemptCount: `148`
- seqSelfLoopRemainderOneEdgeHandledCount: `148`
- seqSelfLoopRemainderOneEdgeFallbackCount: `0`
- RFT_COMPACT_BUILD_FAIL: `148 -> 0`
- CBF_SELF_LOOP_PRECHECK: `148 -> 0`
- SL_PROXY_ONLY_REMAINDER_OTHER_NOT_BICONNECTED: `148 -> 0`
- seqFallbackCaseCount: `269 -> 121`
- seqRewriteWholeCoreFallbackCount: `269 -> 121`

Observed dominant sample for this stage was not a single stripped loop. The representative `seed=2, tc=444, step=1` case strips two PROXY self-loops and leaves one REAL non-loop remainder edge. The implemented path keeps the remainder strict at `TS_ONE_EDGE / SOE_REAL_NONLOOP` and only generalizes reattachment across one-or-more stripped PROXY loops whose poles lie on the remainder edge endpoints.

Current dominant remaining trigger is `RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED=121`. Current representative residual breakdown is `XIV_SHARED_WITH_LOOP / XSR_HAFTER_SPQR_READY / XSB_UNSUPPORTED_HAFTER_SUBTYPE`. Next safe fix target is `x-incident residual / XSR_HAFTER_SPQR_READY`.
