# rewrite-r-seq loop+edge-shared opt

- correctness: green
- rewriteSeqCalls=111100
- rewriteSeqSucceededCases=111100
- rewriteSeqFailedCases=0
- seqFallbackCaseCount=18034
- seqRewriteWholeCoreFallbackCount=27698
- seqTooSmallOtherHandledCount=9968
- seqLoopPlusEdgeSharedHandledCount=9968
- rewriteFallbackSpecialCaseCount=86747

## Baseline Compare
- seqFallbackCaseCount: baseline=27706 current=18034 delta=-9672
- seqRewriteWholeCoreFallbackCount: baseline=47042 current=27698 delta=-19344
- seqTooSmallOtherHandledCount: baseline=0 current=9968 delta=9968
- seqLoopPlusEdgeSharedHandledCount: baseline=0 current=9968 delta=9968
- rewriteFallbackSpecialCaseCount: baseline=76779 current=86747 delta=9968

## Trigger Compare
- RFT_COMPACT_BUILD_FAIL: baseline=6969 current=6969 delta=0
- RFT_COMPACT_EMPTY_AFTER_DELETE: baseline=10156 current=3452 delta=-6704
- RFT_COMPACT_TOO_SMALL_UNHANDLED: baseline=18852 current=6212 delta=-12640
- RFT_GRAFT_REWIRE_FAIL: baseline=4818 current=4818 delta=0
- RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED: baseline=6247 current=6247 delta=0

## Path Totals
- DIRECT_SPQR=41135
- SPECIAL_LOOP_SHARED=9968
- SPECIAL_PATH=24770
- SPECIAL_SINGLE_CUT=17628
- SPECIAL_TWO_PATH=34381
- WHOLE_CORE_REBUILD=27698
