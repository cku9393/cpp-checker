# rewrite-r-seq random campaign

## Result
- first_failure: none
- rewriteSeqCalls: 111100
- rewriteSeqSucceededCases: 111100
- rewriteSeqFailedCases: 0
- rewriteSeqMaxStepReachedCount: 0
- seqProxyMetadataFallbackCount: 0
- seqGraftRewireFallbackCount: 4818
- seqRewriteWholeCoreFallbackCount: 4818
- seqFallbackCaseCount: 4818
- seqFallbackCaseRate: 4.337%
- seqFallbackEventRatePerRewriteCall: 4.337%

## Fallback Reason Counts
- GRAFT_REWIRE: 4818

## Fallback Step Counts
- step1: 4818

## Sequence Length Histogram
- len1: 81440
- len2: 5464
- len3: 24196

## Interpretation
correctness는 green이지만 fallback 빈도가 유의미합니다. 다음 단계는 proxy metadata 누적 불일치 root-cause 분석이 우선입니다.
