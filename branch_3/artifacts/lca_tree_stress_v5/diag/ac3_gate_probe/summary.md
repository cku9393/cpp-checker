# AC3 Hard-Scaling Exact Probe Summary

Timestamp: `2026-03-26`

Context:
- Target sub-AC: `LCA_STAGE_FILTER=hard_scaling ./lca_strong_gate.sh`
- Exact row used for repeatable diagnosis: `comb_dense n4096 seed1 L1 Q1`
- Probe driver: `branch_certify_suite.py` with a one-row preset under `artifacts/lca_tree_stress_v5/diag/ac3_gate_probe/presets/`

Key findings:
- The stable published artifact under `artifacts/lca_tree_stress_v5/strong_gate/runs/hard_scaling/comb_dense/n4096/seed1_L1_Q1/time.txt` still claims a prior validated runtime of `0.025121 9168`.
- The current tracked branch state does not reproduce that row. The exact one-row certify probe times out at the gate's `12.0s` cap.
- The bundled retry note `artifacts/lca_tree_stress_v5/retry_loop/ac3_progress40_investigation_20260325.md` already warned that the previously published PASS artifact does not reproduce from the raw bundled `progress40` snapshot and likely depended on a missing uncommitted progress40-derived optimizer layer.

Recovered runtime-path improvement:
- `solver_release_env.sh` now keeps the archived progress39/progress40 cumulative optimizer line enabled by default except for `ENABLE_PREV_STATE_WRITEBACK_OPT`, which remains `0` because enabling it regressed the exact gate probe on the current source.
- This recovers a materially better exact-row execution path than the original branch-local defaults that had the later delta/pointer/normalize/layout flags disabled.

Exact probe evidence:
- Default branch-local path before recovery:
  - `artifacts/lca_tree_stress_v5/diag/ac3_gate_probe/default_env_diag_comb_dense_n4096_seed1_L1_Q1/.../solver_stderr.txt`
  - Timed out after logging only the earliest erase checkpoints.
- Recovered branch-local default after env repair:
  - `artifacts/lca_tree_stress_v5/diag/ac3_gate_probe/post_patch_v2_default_diag_comb_dense_n4096_seed1_L1_Q1/.../solver_stderr.txt`
  - Still timed out on the exact gate row, so the required strong-gate hard-scaling PASS was not restored.

Current interpretation:
- The env repair fixes a real branch-local drift and should be retained.
- The remaining gap is not explained by generator mismatch or artifact routing.
- Passing `hard_scaling` from the currently tracked source likely requires recovering an optimizer layer that is not present in the bundled `progress40` source or elsewhere in tracked branch artifacts.
