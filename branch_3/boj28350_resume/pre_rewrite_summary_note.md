# Pre-Rewrite Summary Note

Date: `2026-04-12`

This note is the compact branch-local pre-rewrite summary for the current
`branch_3` working tree. It extracts the live solver hypothesis, the most
recent retry-loop failure state, and the locked rewrite constraints from the
branch notes and retry artifacts so the next solver session does not restart
from scratch.

## Sources Reviewed For This Summary

- `boj28350_resume/current_state_summary.md`
- `boj28350_resume/next_session_briefing.md`
- `boj28350_resume/pre_rewrite_checkpoint.md`
- `boj28350_resume/pre_rewrite_synthesis_note.md`
- `boj28350_resume/progress40_derived_reference.md`
- `boj28350_resume/boj28350_branch_3_solver.cpp`
- `boj28350_literature_progress40_layout_signature_reuse_gate.cpp`
- `.ouroboros/failure_analysis_state.json`
- `.ouroboros/failure_analysis_iteration.md`
- `.ouroboros/failure_analysis_playbook.md`
- `artifacts/lca_tree_stress_v5/retry_loop/latest_failure_report.md`
- `artifacts/lca_tree_stress_v5/retry_loop/latest_failure_breakdown.md`
- `artifacts/lca_tree_stress_v5/retry_loop/latest_analysis_session.md`
- `artifacts/lca_tree_stress_v5/retry_loop/latest_next_probe_result.md`
- `artifacts/lca_tree_stress_v5/retry_loop/latest_attempt_guard.md`
- `artifacts/lca_tree_stress_v5/retry_loop/latest_git_repo_health.md`

## Current Solver Hypotheses

1. The next solver-side change must stay inside the bundled `progress40`
   family. The safe corridor remains
   `pack/normalize -> same-layout reuse -> layout-signature gate -> zero-span eligibility -> fastpath commit`,
   not a new algorithm family.
2. The current primary optimization axis is still
   `zero-span eligibility and fastpath commit`. The authoritative residual
   ordering remains:
   `zero-span eligibility and fastpath commit` `49.9983%`,
   `layout signature compare and reuse gate core` `25.0339%`,
   `signature source load and materialize` `24.9643%`,
   `connector hotpath normalize reuse` `0.0036%`.
3. Secondary solver-side follow-up axes remain
   `signature source load and materialize` and
   `layout signature compare and reuse gate core`, but only after the current
   zero-span/fastpath residual is revisited on the same semantics.
4. The active solver still exposes the progress40 route-aware
   `time_lgate_*` / `lgate_*` surfaces and zero-span counters, so later edits
   should preserve that attribution rather than delete it.
5. The active solver file has obvious drift markers. Its top-of-file state
   still points at the bundled `progress40` source through the disabled include
   path, but the file header describes a reconstructed `progress11` artifact
   and the branch notes explicitly classify the current working copy as
   separator-decomposition drift. The next rewrite should reduce that drift,
   not widen it.

## Prior Failures And Rejected Directions

1. Hard-family history still rejects `shared backbone`, `owner-local exact
   oracle`, and `BC local-surgery`. These approaches already failed to cut cost
   on the representative adversarial families and should not be revived as the
   next main branch_3 direction.
2. The latest retry-loop failure is `attempt_038`
   (`2026-04-12 10:29:34 KST`) with failed ACs `2`, `3`, and `8`.
3. The newest concrete failure is not a solver proof that the progress40 axis
   is wrong. It is an `AC 2` `./lca_smoke.sh` `generic_retry_failure` in the
   `pre-gate-stability` lane. The pinned retry axis therefore stays
   `zero_span_fastpath`, with `secondary_axis = none` and
   `next_probe_command = ./lca_smoke.sh`.
4. The latest smoke localization is wrapper-side, not solver-core:
   `lca_smoke.sh` around `write_launcher_status_bundle`,
   `publish_launcher_smoke_summary_bundle`,
   `capture_original_launcher_context`,
   `write_launcher_run_source_failure_snapshot_manifest`, and
   `launcher_backfill_source_failure_details`.
5. `AC 3` and `AC 5` are still credibility failures, not closure wins.
   `latest_attempt_guard.md` rejects both because fresh same-attempt direct
   `./lca_strong_gate.sh` and `./lca_boj3s_gate.sh` evidence is missing.
6. The retry loop already confirmed that a same-worktree `./lca_smoke.sh`
   probe can pass in isolation, so the current failure history should be read
   as unstable wrapper/gate progression plus missing direct heavy-gate
   evidence, not as permission to abandon the progress40 solver line.

## Locked Rewrite Constraints

1. Keep `./boj28350_resume/boj28350_branch_3_solver.cpp` as the primary solver
   target.
2. Preserve the literature-grade structure: BC-tree flavored explicit child
   lattice, `closeByBCPath(...)`, `buildClosedHandleFromWitness(...)`, exact
   strict-child testing, and the no-owner-exact-rebuild line.
3. Do not widen the current separator-decomposition drift and do not replace
   the active solver with a heuristic-only or different algorithm family.
4. Preserve the progress40 instrumentation and gate-stack semantics. The
   layout-signature gate must remain attached to canonical normalize and
   same-layout reuse; zero-span fastpath work must preserve the same acceptance
   meaning rather than loosen the gate.
5. Read branch-local gate failures through the documented lanes first:
   `./lca_strong_gate.sh` failures should first be interpreted as
   correctness/proof-preservation issues, while `./lca_boj3s_gate.sh` failures
   should first be interpreted as performance/profile issues.
6. Reproducibility is part of the rewrite contract. Bundled `progress40`
   evidence is still only `partial`; dense `1024` repeat, dense `4096`, multi
   `4096`, and long-run terminal-row persistence still require fresh
   branch-local evidence.
7. Formal closure still requires fresh same-working-tree reruns of
   `./lca_strong_gate.sh` and `./lca_boj3s_gate.sh` with no manual artifact
   cleanup between runs.
8. Build only with `./build.sh`, keep artifacts under `branch_3/artifacts/...`,
   and prefer the smallest meaningful validation step before escalating.
9. Git-backed inspection is currently unreliable in this checkout.
   `latest_git_repo_health.md` still reports timed-out `git` checks, so branch
   notes and branch-local artifacts should remain the primary state source.

## Working Conclusion

The next rewrite should not begin as a broad solver-family pivot. The current
evidence says to keep branch_3 anchored to bundled `progress40`, preserve the
literature-grade structure, treat `zero_span_fastpath` as the active primary
axis, and clear the smoke-to-gate reproducibility path before trusting any
nominal closure claims.
