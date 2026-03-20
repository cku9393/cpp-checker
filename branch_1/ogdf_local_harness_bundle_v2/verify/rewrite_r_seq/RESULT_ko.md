# rewrite-r-seq campaign

## 결과
- `rewrite-r-seq --manual-only`: green
- 첫 random sequence failure: `s1_r50`
- earliest stage: `SEQ_REWRITE_R_FAIL`
- reason: `graft: rewireArcEndpoint returned false`
- first bundle: `dumps/rewrite_r_seq/s1_r50/SEQ_REWRITE_R_FAIL_seed1_tc40_step1.txt`

## 실패 위치
- `tc=40`
- `stepIndex=1`
- `sequenceLengthSoFar=2`
- `chosenR=0`
- `chosenX=1`

## 참고
- manual sequence 실행 중에는 `rewriteCalls=4`, `compactRejectedFallbackCount=1` 이었고 전체 case는 통과했다.
- random sequence는 첫 캠페인 `seed=1, rounds=50` 에서 바로 중단했다.
- 이번 단계에서는 earliest-stage 수집까지만 수행했고, 더 뒤 단계는 보지 않았다.

## 산출물
- `summary.json`
- `manual_run.log`
- `logs/s1_r50.log`
- `dumps/rewrite_r_seq/s1_r50/SEQ_REWRITE_R_FAIL_seed1_tc40_step1.txt`
