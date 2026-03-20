# rewrite-r-seq fallback trigger breakdown

## Result
- first_failure: none
- rewriteSeqCalls: 111100
- rewriteSeqSucceededCases: 111100
- rewriteSeqFailedCases: 0
- seqFallbackCaseCount: 27706
- seqProxyMetadataFallbackCount: 0
- seqGraftRewireFallbackCount: 4818
- seqRewriteWholeCoreFallbackCount: 47042
- seqFallbackCaseRate: 24.938%
- seqFallbackEventRatePerRewriteAttempt: 28.518%
- dominantTrigger: RFT_COMPACT_TOO_SMALL_UNHANDLED

## rewriteFallbackTriggerCounts
- RFT_COMPACT_TOO_SMALL_UNHANDLED: 18852 (40.075%)
- RFT_COMPACT_EMPTY_AFTER_DELETE: 10156 (21.589%)
- RFT_COMPACT_BUILD_FAIL: 6969 (14.814%)
- RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED: 6247 (13.280%)
- RFT_GRAFT_REWIRE_FAIL: 4818 (10.242%)

## rewriteFallbackCaseCountsByTrigger
- RFT_COMPACT_TOO_SMALL_UNHANDLED: 15884
- RFT_COMPACT_EMPTY_AFTER_DELETE: 10156
- RFT_COMPACT_BUILD_FAIL: 6969
- RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED: 6247
- RFT_GRAFT_REWIRE_FAIL: 4818

## rewriteFallbackTriggerAtStepCounts
- step2_RFT_COMPACT_EMPTY_AFTER_DELETE: 10156
- step1_RFT_COMPACT_TOO_SMALL_UNHANDLED: 9672
- step2_RFT_COMPACT_TOO_SMALL_UNHANDLED: 9180
- step1_RFT_COMPACT_BUILD_FAIL: 6969
- step1_RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED: 6247
- step1_RFT_GRAFT_REWIRE_FAIL: 4818

## rewritePathTakenCounts
- WHOLE_CORE_REBUILD: 47042
- DIRECT_SPQR: 41135
- SPECIAL_TWO_PATH: 34381
- SPECIAL_PATH: 24770
- SPECIAL_SINGLE_CUT: 17628

## seqFallbackReasonCounts
- OTHER: 42224
- GRAFT_REWIRE: 4818

## sequenceLengthHistogram
- len1: 81440
- len2: 5464
- len3: 24196

## Fallback Sample Dumps By Trigger
- RFT_COMPACT_BUILD_FAIL: dumps/rewrite_r_seq_fallbacks/seq_fallback_RFT_COMPACT_BUILD_FAIL_seed1_tc57_step1.txt
- RFT_COMPACT_EMPTY_AFTER_DELETE: dumps/rewrite_r_seq_fallbacks/seq_fallback_RFT_COMPACT_EMPTY_AFTER_DELETE_seed1_tc0_step2.txt
- RFT_COMPACT_TOO_SMALL_UNHANDLED: dumps/rewrite_r_seq_fallbacks/seq_fallback_RFT_COMPACT_TOO_SMALL_UNHANDLED_seed1_tc2_step1.txt
- RFT_GRAFT_REWIRE_FAIL: dumps/rewrite_r_seq_fallbacks/seq_fallback_RFT_GRAFT_REWIRE_FAIL_seed1_tc40_step1.txt
- RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED: dumps/rewrite_r_seq_fallbacks/seq_fallback_RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED_seed1_tc0_step1.txt

## Fallback Sample Dumps By Reason
- GRAFT_REWIRE: dumps/rewrite_r_seq_fallbacks/seq_fallback_GRAFT_REWIRE_seed1_tc40_step1.txt
- OTHER: dumps/rewrite_r_seq_fallbacks/seq_fallback_OTHER_seed1_tc0_step1.txt

## Interpretation
correctness는 green이고 fallback trigger는 RFT_COMPACT_TOO_SMALL_UNHANDLED가 최다다. 다음 safe fix 타겟은 이 trigger 하나로 좁히는 것이 맞다.
