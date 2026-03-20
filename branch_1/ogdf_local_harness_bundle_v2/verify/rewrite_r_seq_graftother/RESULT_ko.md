# rewrite-r-seq graft-other breakdown

- correctness: green
- run count: 31
- baseline: `verify/rewrite_r_seq_clearpreserve/summary.json`
- current: `verify/rewrite_r_seq_graftother/summary.json`

## 핵심 결과
- `rewriteSeqCalls = 110100`
- `seqFallbackCaseCount = 6088`
- `seqRewriteWholeCoreFallbackCount = 8006`
- `dominant remaining trigger = RFT_GRAFT_REWIRE_FAIL (4029)`
- `dominant graft subtype = GRB_OTHER`
- `dominant graft-other subtype = GOS_POSTCHECK_ADJ_MISMATCH (4029)`

## Graft Other Breakdown
- `GOS_POSTCHECK_ADJ_MISMATCH = 4029`
- `GOS_OTHER = 0`
- first sample: `dumps/rewrite_r_seq_graftother/seq_graft_other_GOS_POSTCHECK_ADJ_MISMATCH_seed1_tc40_step1.txt`

## 비교
- `GRB_OTHER`: baseline 4029 -> current 4029 (delta 0)
- `seqFallbackCaseCount`: baseline 6088 -> current 6088 (delta 0)
- `seqRewriteWholeCoreFallbackCount`: baseline 8006 -> current 8006 (delta 0)

## 다음 타겟
- `GOS_POSTCHECK_ADJ_MISMATCH`가 전부를 차지하므로, 다음 safe fix 타겟은 sequence preserve-clear 이후 `adjacent same-type S/P nodes` postcheck를 유발하는 same-node rehome / adjacency stabilization 경로입니다.
