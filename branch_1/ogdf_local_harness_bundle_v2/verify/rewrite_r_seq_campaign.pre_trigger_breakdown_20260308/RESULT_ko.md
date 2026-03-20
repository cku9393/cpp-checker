# rewrite-r-seq random campaign

## Result
- first_failure: none
- rewriteSeqCalls: 111100
- rewriteSeqSucceededCases: 111100
- rewriteSeqFailedCases: 0
- rewriteSeqMaxStepReachedCount: 0
- rewriteCalls: 164956
- seqProxyMetadataFallbackCount: 0
- seqGraftRewireFallbackCount: 4818
- seqRewriteWholeCoreFallbackCount: 47042
- seqFallbackCaseCount: 27706
- seqFallbackCaseRate: 24.938%
- seqFallbackEventRatePerRewriteAttempt: 28.518%

## Fallback Reason Counts
- GRAFT_REWIRE: 4818 (10.242%)
- OTHER: 42224 (89.758%)

## Fallback Step Counts
- step1: 27706
- step2: 19336

## Sequence Length Histogram
- len1: 81440
- len2: 5464
- len3: 24196

## Fallback Sample Dumps
- GRAFT_REWIRE: dumps/rewrite_r_seq_fallbacks/seq_fallback_GRAFT_REWIRE_seed1_tc40_step1.txt
- OTHER: dumps/rewrite_r_seq_fallbacks/seq_fallback_OTHER_seed1_tc0_step1.txt

## Interpretation
correctness는 green이지만 fallback 빈도가 유의미하다. 다음 단계는 proxy metadata/rewire 누적 불일치 root-cause 분석이 우선이다.
