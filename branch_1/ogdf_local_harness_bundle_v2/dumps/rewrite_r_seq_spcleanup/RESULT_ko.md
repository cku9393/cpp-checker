# rewrite-r-seq sequence-only local S/P cleanup 결과

## 요약

- sequence mode 전용 local same-type S/P cleanup 을 추가했다.
- replay `seed=1 tc=40 step=1` 는 이제 `SEQ_ACTUAL_INVARIANT_FAIL` 없이 green 이다.
- replay bundle 기준:
  - `postcheckSubtype=GPS_OTHER`
  - `preCleanupPostcheckSubtype=GPS_SAME_TYPE_SP_ONLY`
  - `postCleanupPostcheckSubtype=GPS_OTHER`
  - `sameTypeSPCleanupMergeCount=1`
  - merged pair = `(1,0) -> keep 0`
- 소규모 random:
  - `seed=1 rounds=100` green
  - `seed=1 rounds=1000` green
- full campaign:
  - `seed=1..10 rounds=1000` green
  - `seed=1..20 rounds=5000` green

## dedup 비교 기준

- 현재 비교는 기존 baseline 과 동일하게 dedup aggregate 기준으로 계산했다.
- 기준 집합:
  - `s1_r100`
  - `seed=1..10 rounds=1000`
  - `seed=1..20 rounds=5000`
- 이 기준에서 `rewriteSeqCalls=110100` 이다.

## baseline 대비 변화

- baseline
  - `rewriteSeqCalls=110100`
  - `seqFallbackCaseCount=6088`
  - `seqRewriteWholeCoreFallbackCount=8006`
  - `GOS_POSTCHECK_ADJ_MISMATCH=4029`
- current
  - `rewriteSeqCalls=110100`
  - `seqFallbackCaseCount=2059`
  - `seqRewriteWholeCoreFallbackCount=3977`
  - `GOS_POSTCHECK_ADJ_MISMATCH=0`
  - `seqSameTypeSPCleanupAttemptCount=4029`
  - `seqSameTypeSPCleanupMergeCount=4029`
  - `seqSameTypeSPCleanupSuccessCount=4029`
  - `seqSameTypeSPCleanupFailCount=0`
  - `seqSameTypeSPCleanupCaseCount=4029`
- delta
  - `seqFallbackCaseCount=-4029`
  - `seqRewriteWholeCoreFallbackCount=-4029`
  - `GOS_POSTCHECK_ADJ_MISMATCH=-4029`

## 해석

- 기존 dominant bucket 이던 `GOS_POSTCHECK_ADJ_MISMATCH=4029` 는 이번 단계에서 cleanup `4029`회로 정확히 흡수됐다.
- replay 와 full campaign 모두 correctness 는 green 이다.
- 이번 단계의 주 타깃이었던 sequence preserve-clear 이후 local same-type S/P adjacency 는 cleanup 으로 해소된 상태다.
