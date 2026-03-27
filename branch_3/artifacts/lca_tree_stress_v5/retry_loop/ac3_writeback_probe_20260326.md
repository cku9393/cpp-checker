# AC3 Writeback Probe 2026-03-26

## Scope

- Task lane: AC3 strong-gate recovery inside `branch_3`
- Pinned primary axis: `zero_span_fastpath`
- Pinned secondary axis: `state_materialization`

## Safe branch-local change kept

- `boj28350_resume/solve` now executes a private linked/copied binary from branch-local tmp instead of depending on the shared `artifacts/boj28350_resume/build/solve` path surviving until `exec`.
- This targets the reproducibility issue seen in `strong_gate.latest_failure` rows where `boj28350_resume/solve` reached line 31 and the shared build output had disappeared.

## Key runtime findings

### 1. Current release defaults do not actually enter the progress40 writeback/layout path

- With the normal release env stack and `ENABLE_PREV_STATE_WRITEBACK_OPT=0`, sampled LOCAL probes showed:
  - `cnorm_same_layout_reuse_checks=0`
  - `lreuse_same_layout_hits=0`
  - `lgate_same_layout_gate_hits=0`
- This means the pinned progress40 `zero-span eligibility and fastpath commit` line is not active under the default branch-local runtime.

Representative artifact:

- `artifacts/lca_tree_stress_v5/retry_loop/ac3_local_rel_comb256_both_on/solver_stderr.txt`

### 2. Re-enabling writeback activates the intended progress40 chain and gives major wins on several hard families

- With `ENABLE_PREV_STATE_WRITEBACK_OPT=1`, sampled LOCAL probe on `comb_rect_dense 256` showed nonzero:
  - `cnorm_same_layout_reuse_checks=15719`
  - `lreuse_same_layout_hits=15719`
  - `lgate_same_layout_gate_hits=15719`

Representative artifact:

- `artifacts/lca_tree_stress_v5/retry_loop/ac3_local_rel_writeback_on_comb256/solver_stderr.txt`

Representative wrapper timings:

- `multi_comb_rect 512 seed1 L1 Q1`
  - default release env: `8.226190s`
  - writeback on: `1.912738s`
  - writeback on + connector_only: `1.490577s`
- `multi_comb_cap 512 seed1 L1 Q1`
  - writeback on + connector_only: `0.993565s`
- `broom_mixed 1024 seed1 L1 Q1`
  - writeback on + connector_only: `1.377572s`
- `star_pairs 512 seed1 L1 Q1`
  - writeback on + connector_only: `0.509373s`

Representative artifacts:

- `artifacts/lca_tree_stress_v5/retry_loop/ac3_wrapper_default_multi512`
- `artifacts/lca_tree_stress_v5/retry_loop/ac3_wrapper_writeback_on_multi512`
- `artifacts/lca_tree_stress_v5/retry_loop/ac3_wrapper_writeback_on_connector_only_multi512`
- `artifacts/lca_tree_stress_v5/retry_loop/ac3_combo_multicap512`
- `artifacts/lca_tree_stress_v5/retry_loop/ac3_combo_broom1024`
- `artifacts/lca_tree_stress_v5/retry_loop/ac3_combo_starpairs512`

### 3. The same writeback activation also introduces a deterministic release-build correctness regression on `chain_unary`

- Case:
  - `chain_unary 1024 seed1 L1 Q1`
- Observed deterministic failure under release build with writeback on:
  - `query #52 mismatch: lca(217, 961)=828, expected 961`
- This persisted across:
  - `connector_only`
  - `preserved_only`
  - `both_off`
  - `ENABLE_STATE_LOAD_MATERIALIZATION_OPT=1`
  - `ENABLE_PREV_STATE_CARRY_REUSE_OPT=0`
  - `ENABLE_CARRY_REUSE_FASTPATH_OPT=0`
  - `ENABLE_DIRECT_PREBIND_OPT=0`

Representative artifacts:

- `artifacts/lca_tree_stress_v5/retry_loop/ac3_recheck_default_chain1024`
- `artifacts/lca_tree_stress_v5/retry_loop/ac3_chain_writeback_preserved_only`
- `artifacts/lca_tree_stress_v5/retry_loop/ac3_chain_writeback_both_off`
- `artifacts/lca_tree_stress_v5/retry_loop/ac3_chain_sload_on`
- `artifacts/lca_tree_stress_v5/retry_loop/ac3_chain_creuse_off`
- `artifacts/lca_tree_stress_v5/retry_loop/ac3_chain_fastpath_off`
- `artifacts/lca_tree_stress_v5/retry_loop/ac3_chain_dprebind_off`

### 4. LOCAL vs release diverge on the same writeback-enabled chain case

- The `LOCAL` build compiled via:
  - `./build.sh --define LOCAL --out artifacts/lca_tree_stress_v5/retry_loop/ac3_local_probe_build/solve`
- With the same runtime stack and `LOCAL_SKIP_SELF_TEST=1 PROFILE_MODE=PROFILE_NONE`, the LOCAL binary passed:
  - `chain_unary 1024 seed1 L1 Q1` in `1.534844s`
- Release build still failed on the same case.
- Output trees differ at least at node 4:
  - release parent: `828`
  - LOCAL parent: `427`

Representative artifacts:

- `artifacts/lca_tree_stress_v5/retry_loop/ac3_recheck_default_chain1024/out.txt`
- `artifacts/lca_tree_stress_v5/retry_loop/ac3_local_noprof_chain1024/out.txt`

## Remaining blockers after the best safe-looking probe stack

Even with `writeback on + connector_only`, dense comb-family cases remain far over the strong-gate target:

- `comb_rect_dense 512 seed1 L0 Q0`: `9.638768s`
- `caterpillar_rect_dense 512 seed1 L0 Q0`: `7.417573s`

Representative artifacts:

- `artifacts/lca_tree_stress_v5/retry_loop/ac3_wrapper_writeback_on_connector_only_comb512`
- `artifacts/lca_tree_stress_v5/retry_loop/ac3_combo_caterpillar512`

## Recommended next retry focus

1. Treat `ENABLE_PREV_STATE_WRITEBACK_OPT=1` as a diagnostic signal, not a safe default, until the release-vs-LOCAL chain regression is fixed.
2. Investigate the release-vs-LOCAL divergence first:
   - same input
   - same runtime env
   - only parent assignment difference observed so far is node `4`
3. Search for release-only behavior in the writeback path, or for LOCAL-only validation/reference code whose side effects accidentally stabilize state.
4. After correctness is restored under writeback, revisit dense comb/caterpillar scaling, which still dominates AC3 even under the faster runtime stack.
