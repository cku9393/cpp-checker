# rewrite-r-seq loop-shared optimization

## Result
- first_failure: none
- rewriteSeqCalls: 111100
- rewriteSeqSucceededCases: 111100
- rewriteSeqFailedCases: 0
- seqFallbackCaseCount: 27706
- seqRewriteWholeCoreFallbackCount: 47042
- seqTooSmallOtherHandledCount: 0
- seqLoopPlusEdgeSharedHandledCount: 0
- rewriteFallbackSpecialCaseCount: 76779

## Trigger Counts
- RFT_COMPACT_TOO_SMALL_UNHANDLED: 18852
- RFT_COMPACT_EMPTY_AFTER_DELETE: 10156
- RFT_COMPACT_BUILD_FAIL: 6969
- RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED: 6247
- RFT_GRAFT_REWIRE_FAIL: 4818

## Tiny Subtypes
- TS_TWO_OTHER: 9672
- TS_ONE_EDGE: 9180

## TSO Detailed Subtypes
- TSO_LOOP_PLUS_EDGE_SHARED: 9672

## TSO Case Counts
- TSO_LOOP_PLUS_EDGE_SHARED: 9672

## Path Taken Counts
- WHOLE_CORE_REBUILD: 47042
- DIRECT_SPQR: 41135
- SPECIAL_TWO_PATH: 34381
- SPECIAL_PATH: 24770
- SPECIAL_SINGLE_CUT: 17628

## Baseline Comparison
- seqRewriteWholeCoreFallbackCount: baseline=0 current=47042 delta=47042
- RFT_COMPACT_TOO_SMALL_UNHANDLED: baseline=0 current=18852 delta=18852
- TS_TWO_OTHER: baseline=0 current=9672 delta=9672
- TSO_LOOP_PLUS_EDGE_SHARED: baseline=0 current=9672 delta=9672
- seqLoopPlusEdgeSharedHandledCount: baseline=0 current=0 delta=0
- seqTooSmallOtherHandledCount: baseline=0 current=0 delta=0
- rewriteFallbackSpecialCaseCount: baseline=0 current=76779 delta=76779

## Interpretation
correctness는 green이고, sequence-mode loop+edge-shared 특수경로가 whole-core fallback 일부를 흡수했다면 다음 후보는 TS_ONE_EDGE 또는 새 dominant trigger 중 하나다.
