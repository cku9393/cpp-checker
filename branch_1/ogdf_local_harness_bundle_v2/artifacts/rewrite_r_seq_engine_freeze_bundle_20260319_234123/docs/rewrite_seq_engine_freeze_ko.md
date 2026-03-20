# Rewrite Sequence Engine Freeze

현재 freeze 기준:

- `rewrite-r` single-step: green
- `rewrite-r-seq`: green
- `seqFallbackCaseCount = 0`
- `seqRewriteWholeCoreFallbackCount = 0`
- tracked whole-core rebuild fallback trigger: `NONE`

freeze gate:

- regression manifest:
  - `regressions/rewrite_seq_cases.json`
- regression mode:
  - `--mode rewrite-r-seq-regression`
- bench mode:
  - `--mode rewrite-r-seq-bench`
- regression success:
  - `failedCases == 0`
  - every manifest case has `actualInvariantOk = true`
  - every manifest case has `oracleEquivalentOk = true`
- bench success:
  - `seqFallbackCaseCount == 0`
  - `seqRewriteWholeCoreFallbackCount == 0`
  - `rewritePathTakenCounts.WHOLE_CORE_REBUILD == 0`

representative replay corpus:

- `same_type_sp_cleanup_tc40_step1`
- `loopshared_proxy_loop_real_tc56_step1`
- `selfloop_remainder_spqrready_tc328_step1`
- `selfloop_remainder_oneedge_tc444_step1`
- `xshared_spqrready_tc851_step1`

solver integration freeze rule:

- feature flag name: `USE_REWRITE_SEQ_ENGINE`
- first integration step is compare-only
- do not replace the legacy solver path until regression + bench + solver compare are all green
