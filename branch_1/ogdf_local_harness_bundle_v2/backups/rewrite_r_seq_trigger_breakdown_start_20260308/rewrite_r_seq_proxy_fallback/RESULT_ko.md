# rewrite-r-seq proxy fallback

## 결과
- returncode: 0
- completed tc: 50
- rewriteCalls: 73
- compactRejectedFallbackCount: 21
- rewriteFallbackWholeCoreCount: 22
- seqProxyMetadataFallbackCount: 0
- seqGraftRewireFallbackCount: 1
- seqRewriteWholeCoreFallbackCount: 1

## 해석
- 기존 `SEQ_REWRITE_R_FAIL / graft: rewireArcEndpoint returned false` 상황은 hard fail 대신 whole-core rebuild fallback으로 흡수됐다.
- 이번 `seed=1, rounds=50` 실행에서는 sequence가 끝까지 통과했고 failure bundle은 생성되지 않았다.

## 산출물
- run.log
- summary.json
