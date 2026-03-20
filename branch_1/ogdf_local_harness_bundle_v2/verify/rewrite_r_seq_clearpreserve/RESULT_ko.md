# rewrite-r-seq clear preserve 결과

- allRunsGreen: True
- rewriteSeqCalls: 110100
- seqFallbackCaseCount: 6894 -> 6088 (delta -806)
- seqRewriteWholeCoreFallbackCount: 8829 -> 8006 (delta -823)
- GRB_OLDARC_DEAD: 4818 -> 0 (delta -4818)
- PAL_AFTER_CLEAR_KEEP_DEAD: 11221 -> 0 (delta -11221)
- seqClearPreserveRequestedCount: 11134
- seqClearPreserveArcCount: 11134
- seqClearPreserveCrossNodeRewireCount: 0
- seqClearPreserveSameNodeRehomeCount: 11134
- seqClearPreserveFallbackCount: 0
- dominantRemainingTrigger: RFT_GRAFT_REWIRE_FAIL (4029)
- dominantGraftSubtype: GRB_OTHER

## 해석

sequence mode preserve-clear repair는 correctness를 깨지 않고 유지됐다.

핵심 효과는 clear 직후 external proxy arc가 죽지 않도록 만든 것이다.
- dominant lifecycle first bad phase였던 `PAL_AFTER_CLEAR_KEEP_DEAD`는 완전히 사라졌다.
- dominant graft bailout이었던 `GRB_OLDARC_DEAD`도 완전히 사라졌다.
- preserve 경로는 전부 same-node rehome로 처리됐고, cross-node rewire는 이번 campaign에서 0회였다.

남은 fallback은 더 이상 clear 단계 arc death가 원인이 아니다. 현재 기준 다음 gate 대상은 `dominantRemainingTrigger`를 다시 좁히는 쪽이다.
