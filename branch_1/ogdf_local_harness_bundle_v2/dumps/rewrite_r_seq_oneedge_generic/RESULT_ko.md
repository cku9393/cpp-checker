# RESULT_ko

## Outcome
- rewriteSeqCalls = 110100
- seqFallbackCaseCount = 683 (baseline 824 대비 -141)
- seqRewriteWholeCoreFallbackCount = 683 (baseline 1507 대비 -824)
- RFT_COMPACT_TOO_SMALL_UNHANDLED = 0 (baseline 824 대비 -824)
- seqTooSmallOneEdgeHandledCount = 824
- seqTooSmallOneEdgeRealNonLoopHandledCount = 824
- seqTooSmallOneEdgeFallbackCount = 0

## Notes
- correctness는 s1_r100, s1_r1000, seed=1..10 x 1000, seed=1..20 x 5000 모두 green이다.
- sequence one-edge subtype은 SOE_REAL_NONLOOP만 관측됐고 누적 824건 모두 local synthetic 1-node mini path로 처리됐다.
- 남은 dominant trigger는 RFT_COMPACT_BUILD_FAIL=562 이고, 대표 residual은 SL_PROXY_ONLY_REMAINDER_SPQR_READY=414 이다.
