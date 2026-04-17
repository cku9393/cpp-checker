# Next Retry Handoff

- Timestamp: `2026-04-11 15:34:39 KST`
- Current failure attempt: `attempt_032`
- Current failure signature: `attempt_032|orch_4092594ca006|2026-04-11 14:55:26 KST|3`
- Branch-local refreshed assets: `.ouroboros/ac3_timeout_cluster_localization.md`, `.ouroboros/failure_analysis_iteration.md`, `.ouroboros/failure_analysis_state.json`
- Primary axis: `zero_span_fastpath`
- Secondary axis: `none`
- Next probe command: `LCA_STAGE_FILTER=correctness_fuzz ./lca_strong_gate.sh`

## Read Order

1. `artifacts/lca_tree_stress_v5/retry_loop/latest_failure_report.md`
2. `artifacts/lca_tree_stress_v5/retry_loop/latest_failure_breakdown.md`
3. `.ouroboros/ac3_timeout_cluster_localization.md`
4. `.ouroboros/failure_analysis_iteration.md`
5. `.ouroboros/failure_analysis_state.json`

## Narrowed Diagnosis

- The live blocker is still `AC 3` only: the newest same-worktree failure is a
  `strong_gate_timeout_cluster`, not a smoke pre-dispatch issue and not a
  credible downstream closure.
- Start from the helper-side timeout publication corridor in
  `branch_certify_suite.py [543-556, 563-614]`, because that is the smallest
  path that turns the solver call into the `timeout=120` / `solver_rc=-9`
  certify-row plateau.
- Pair that helper reread immediately with the active solver zero-span slice at
  `boj28350_resume/boj28350_branch_3_solver.cpp [9528-9600]`, where
  `time_cnorm_zero_span_elision_ns`,
  `time_lgate_zero_span_eligibility_gate_ns`, and
  `time_lgate_fastpath_commit_core_ns` are emitted together.
- Keep `applyPieceNativeReuseForClass(...) [14761-14852]` and
  `materializeSupportMetadataFromCollector(...)` /
  `materializeSupportMetadataFromPieceState(...) [11376-11608]` as fallback
  section references only. They are not live axes unless a fresh retry names
  them directly.
- Treat `latest_attempt_guard.md` as a credibility filter, not as a competing
  diagnosis: it blocks formal closure for missing same-worktree gate evidence.
- Treat `latest_git_repo_health.md` as a warning only: the post-failure git
  probes timed out, but they do not outweigh attempt-local certify-row timeout
  evidence.

## Do Not Broaden

- Keep exactly one primary progress40 axis: `zero_span_fastpath`.
- Keep at most one secondary axis: `none`.
- Do not reuse the stale `attempt_029` smoke-probe `retain_compaction` /
  `state_materialization` pair as live retry guidance for `attempt_032`.
- Do not reopen wrapper-wide lock or snapshot-cleanup rereads unless a new
  same-worktree failure disproves the current helper-plus-solver timeout
  corridor.
