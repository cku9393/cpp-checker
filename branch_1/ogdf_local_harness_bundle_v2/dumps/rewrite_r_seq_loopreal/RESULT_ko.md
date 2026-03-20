# rewrite-r-seq loopreal result

- correctness: green
- aggregate strategy: s1_r100 + full_s1_10_r1000(seed1..10) + full_s1_20_r5000(seed1..20), excluding duplicate s1_r1000
- implementation note: preserve-clear + same-node rehome direct apply succeeded, and the final actual shape stays flattened on oldNode (`loopSharedChildNode=-1`) because the current actual invariant forbids proxy-only relay nodes.

## Replay
- replay target: seed=1 tc=56 step=1
- replay green: 1
- inPlaceLoopSharedApplied: 1
- sameNodeRehomeSucceeded: 1
- postcheckSubtype: GPS_OTHER
- replay bundle: dumps/rewrite_r_seq_loopreal/replay/SEQ_REPLAY_CAPTURE_seed1_tc56_step1.txt

## Current aggregate
- rewriteSeqCalls: 110100
- seqFallbackCaseCount: 824
- seqRewriteWholeCoreFallbackCount: 1507
- RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED: 121
- XSR_HAFTER_LOOP_SHARED: 0
- XSR_HAFTER_SPQR_READY: 121
- seqXSharedLoopSharedProxyLoopRealAttemptCount: 1235
- seqXSharedLoopSharedProxyLoopRealHandledCount: 1235
- seqXSharedLoopSharedProxyLoopRealFallbackCount: 0

## Delta vs previous xshared residual2 baseline
- seqFallbackCaseCount: 2059 -> 824 (-1235)
- seqRewriteWholeCoreFallbackCount: 3977 -> 1507 (-2470)
- RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED: 1356 -> 121 (-1235)
- XSR_HAFTER_LOOP_SHARED: 1235 -> 0 (-1235)
- XLSB_GRAFT_FAIL: 1235 -> 0 (-1235)

## Next target
- current dominant remaining trigger: RFT_COMPACT_TOO_SMALL_UNHANDLED (824)
- next safe fix target: RFT_COMPACT_TOO_SMALL_UNHANDLED / TS_ONE_EDGE
