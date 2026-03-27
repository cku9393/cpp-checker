# AC4 Current Wrapper Probe Summary

- Timestamp: `2026-03-26 KST`
- Scope: branch-local AC4 investigation after reading the required retry-loop and progress40 materials.
- Goal: determine whether `./lca_strong_gate.sh` closure is currently blocked by wrapper reproducibility or by a still-live solver runtime failure.

## Launcher hardening

- Patched `boj28350_resume/solve` to retry snapshotting the built binary before exec.
- Motivation: preserved strong-gate rows included intermittent `No such file or directory` launcher failures against `artifacts/boj28350_resume/build/solve`.
- Verification:
  - `bash -n boj28350_resume/solve`
  - `python3 branch_run_case.py multi_comb_rect 512 1 0 0 boj28350_resume/solve artifacts/lca_tree_stress_v5/retry_loop/ac4_postpatch_multi512 --timeout 2.0`
  - `python3 branch_run_case.py comb_rect_dense 256 1 0 0 boj28350_resume/solve artifacts/lca_tree_stress_v5/retry_loop/ac4_postpatch_comb256 --timeout 2.0`

## Current wrapper probes

- PASS: `comb_rect_dense n256 seed1 L0 Q0` in `0.886972s`
  - Artifact: `artifacts/lca_tree_stress_v5/retry_loop/ac4_probe_current_wrapper_comb256`
- FAIL: `comb_rect_dense n512 seed1 L0 Q0` timed out at `2.0s`
  - Artifact: `artifacts/lca_tree_stress_v5/retry_loop/ac4_probe_current_wrapper_comb512`
- FAIL: same `comb_rect_dense n512 seed1 L0 Q0` timed out again at `12.0s`
  - Artifact: `artifacts/lca_tree_stress_v5/retry_loop/ac4_comb512_default12`
- PASS: `multi_comb_rect n512 seed1 L0 Q0` in `1.222110s`
  - Artifact: `artifacts/lca_tree_stress_v5/retry_loop/ac4_probe_current_wrapper_multi512`
- PASS: `multi_comb_cap n512 seed1 L0 Q0` in `0.705011s`
  - Artifact: `artifacts/lca_tree_stress_v5/retry_loop/ac4_probe_current_wrapper_multicap512`
- PASS: `chain_unary n1024 seed5 L1 Q1` in `0.841113s`
  - Artifact: `artifacts/lca_tree_stress_v5/retry_loop/ac4_probe_current_wrapper_chain1024`
- PASS: `broom_mixed n1024 seed5 L0 Q1` in `1.847676s`
  - Artifact: `artifacts/lca_tree_stress_v5/retry_loop/ac4_probe_current_wrapper_broom1024`

## Comb-family axis checks

The representative `comb_rect_dense n512 seed1 L0 Q0` row remained well above the strong-gate `2.0s` correctness-fuzz budget.

- FAIL: `ENABLE_PREV_STATE_WRITEBACK_OPT=1`, timed out at `12.0s`
  - Artifact: `artifacts/lca_tree_stress_v5/retry_loop/ac4_comb512_writeback12`
- FAIL: `ENABLE_STATE_LOAD_MATERIALIZATION_OPT=1`, timed out at `12.0s`
  - Artifact: `artifacts/lca_tree_stress_v5/retry_loop/ac4_comb512_sload12`
- FAIL: both `ENABLE_STATE_LOAD_MATERIALIZATION_OPT=1` and `ENABLE_PREV_STATE_WRITEBACK_OPT=1`, timed out at `12.0s`
  - Artifact: `artifacts/lca_tree_stress_v5/retry_loop/ac4_comb512_statewrite12`
- FAIL: `ENABLE_LAYOUT_SIGNATURE_GATE_OPT=0`, timed out at `12.0s`
  - Artifact: `artifacts/lca_tree_stress_v5/retry_loop/ac4_comb512_lgateoff12`
- FAIL: `ENABLE_LAYOUT_REUSE_ZERO_ELISION_OPT=0`, timed out at `12.0s`
  - Artifact: `artifacts/lca_tree_stress_v5/retry_loop/ac4_comb512_lreuseoff12`
- FAIL: both `ENABLE_LAYOUT_SIGNATURE_GATE_OPT=0` and `ENABLE_LAYOUT_REUSE_ZERO_ELISION_OPT=0`, timed out at `12.0s`
  - Artifact: `artifacts/lca_tree_stress_v5/retry_loop/ac4_comb512_lgate_lreuse_off12`

## Conclusion

- AC4 is still blocked by solver runtime on the current working tree, not just by wrapper repeatability.
- The blocker is now sharply localized: the current wrapper clears representative `multi_comb_rect`, `multi_comb_cap`, `chain_unary`, and `broom_mixed` rows, but `comb_rect_dense n512 seed1 L0 Q0` remains far above budget even with fastpath-related env variants toggled.
- This points the next solver retry back to comb-family performance work inside the pinned progress40 line rather than another closure-only rerun.
