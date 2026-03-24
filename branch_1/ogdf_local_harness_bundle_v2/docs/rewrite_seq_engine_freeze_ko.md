# Rewrite Sequence Engine Freeze

현재 freeze 기준:

- `rewrite-r` single-step: green
- `rewrite-r-seq`: green
- `seqFallbackCaseCount = 0`
- `seqRewriteWholeCoreFallbackCount = 0`
- tracked whole-core rebuild fallback trigger: `NONE`
- freeze bench aggregate 기준 `rewriteSeqCalls = 110000`
  - `full_s1_10_r1000 + full_s1_20_r5000` 합산값
  - standalone `s1_r1000`은 smoke run이라 dedup aggregate에서 제외

freeze gate:

- regression manifest:
  - `regressions/rewrite_seq_cases.json`
- diagnostic manifest:
  - `diagnostics/rewrite_seq_diagnostic_cases.json`
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
- `xincident_oneedge_tc33_step1`

hard-gate policy:

- hard gate:
  - 현재 6개 fixed case만 사용
  - manifest는 generator 좌표뿐 아니라 `inputExplicit` snapshot도 함께 들고 있어 immutable input으로 취급
- diagnostic-only:
  - exploratory replay/random 케이스는 `diagnostics/` 아래 별도 manifest로 분리
  - 추적/분석용이고 hard regression pass/fail 기준에는 포함하지 않음

solver integration freeze rule:

- feature flag name: `USE_REWRITE_SEQ_ENGINE`
- first integration step is compare-only
- do not replace the legacy solver path until regression + bench + solver compare are all green
