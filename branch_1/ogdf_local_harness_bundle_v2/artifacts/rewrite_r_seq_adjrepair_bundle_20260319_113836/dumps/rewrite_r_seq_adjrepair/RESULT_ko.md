# rewrite-r-seq adjacency repair 결과

## 구현 범위
- sequence mode preserve-clear 경로에 authoritative `core.arcs` full-scan 기반 adjacency repair 추가
- `collectAuthoritativeLiveAdjArcs`, `rebuildAdjacencyForNodeFromArcs`, `rebuildAdjacencyForAffectedNodesAfterGraft`, `collectAffectedNodesForAdjRepair` 추가
- `seqAdjRepair*` stats 추가
- `GraftTrace`에 affected node / oldNode adj before-after / first bad node / expectedAdj / actualAdj 추가
- same-node rehome의 직접 `oldNode.adjArcs` patch 제거

## 검증
- clean rebuild 성공
- 소규모: `seed=1, rounds=100` green
- 소규모: `seed=1, rounds=1000` green
- full campaign: `seed=1..10, rounds=1000` + `seed=1..20, rounds=5000` 완료

## dedup 비교 기준
baseline: `verify/rewrite_r_seq_graftother/summary.json`
current: `dumps/rewrite_r_seq_adjrepair/summary_dedup_compare.json`

- `rewriteSeqCalls`: 110100 -> 110100
- `seqFallbackCaseCount`: 6088 -> 6088
- `seqRewriteWholeCoreFallbackCount`: 8006 -> 8006
- `GOS_POSTCHECK_ADJ_MISMATCH`: 4029 -> 4029
- `seqAdjRepairUsedCount`: 11134
- `seqAdjRepairAffectedNodeCount`: 34976
- `seqAdjRepairOldNodeCount`: 11134
- `seqAdjRepairOutsideNodeCount`: 11134

## 해석
- correctness는 유지됐다.
- adjacency repair는 실제로 11134회 사용됐다.
- 그러나 baseline 대비 dominant subtype과 fallback 계수는 유의미하게 줄지 않았다.
- 대표 샘플 reason은 여전히 `graft: sequence rewrite produced adjacent same-type S/P nodes on arc 0 (0,1)`로 관찰된다.
- 즉 현재 `GOS_POSTCHECK_ADJ_MISMATCH` 버킷에는 authoritative adjacency metadata mismatch 외에 same-type S/P structural adjacency 케이스가 함께 포함되어 있다.
