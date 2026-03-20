# rewrite-r-seq TS_TWO_OTHER breakdown

## Result
- first_failure: none
- rewriteSeqCalls: 111100
- seqFallbackCaseCount: 27706
- seqRewriteWholeCoreFallbackCount: 47042

## rewriteFallbackTriggerCounts
- RFT_COMPACT_TOO_SMALL_UNHANDLED: 18852
- RFT_COMPACT_EMPTY_AFTER_DELETE: 10156
- RFT_COMPACT_BUILD_FAIL: 6969
- RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED: 6247
- RFT_GRAFT_REWIRE_FAIL: 4818

## seqTooSmallSubtypeCounts
- TS_TWO_OTHER: 9672
- TS_ONE_EDGE: 9180

## seqTooSmallOtherSubtypeCounts
- TSO_LOOP_PLUS_EDGE_SHARED: 9672

## seqTooSmallCaseCountsBySubtype
- TSO_LOOP_PLUS_EDGE_SHARED: 9672

## TSO Sample Dumps
- TSO_LOOP_PLUS_EDGE_SHARED: dumps/rewrite_r_seq_tso/reject_TSO_LOOP_PLUS_EDGE_SHARED_seed1_tc2_step1.txt

## Interpretation
correctness는 green이고 TS_TWO_OTHER 상세 subtype 중 TSO_LOOP_PLUS_EDGE_SHARED가 최다다. 다음 safe tiny handling 타겟은 이 subtype 하나로 좁히는 것이 맞다.
