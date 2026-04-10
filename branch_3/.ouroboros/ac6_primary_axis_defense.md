# AC6 Primary Axis Defense

This note explains why the next solver retry should keep exactly one active
primary progress40 axis, `state_materialization`, with only
`layout_gate` as secondary instead of broadening immediately into unrelated or
older axes after `attempt_016`.

## Current failure basis

- Failed attempt: `attempt_016`
- Failed AC: `5`
- Failure family: `boj3s_gate_unspecified`
- Failure lane:
  `latest_failure_breakdown.md` places the newest failure in the
  `performance-profile` lane with `PROFILE_BASE`
- Fresh AC5-local anchors:
  the rendered wrapper ranges are stale, and the live solver body now sits in
  `artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp`
  at the route fork `[14544-14618]`, with the primary corroborating metric at
  `9364-9370` (`__dt_sig_load` ->
  `time_lreuse_layout_sig_load_ns`) and the only allowed secondary corroboration
  at `9365-9372` (`__dt_sig_cmp` ->
  `time_lreuse_layout_sig_compare_ns`)
- Guard result:
  `latest_attempt_guard.md` rejects `AC3` and `AC5` on
  `missing_direct_gate_evidence`
- Latest probe:
  `latest_next_probe_result.md` is still the older `attempt_015`
  `./lca_smoke.sh` timeout probe, so it is not fresher than the current AC5
  failure
- Git health:
  `latest_git_repo_health.md` only degrades git-backed inspection
  (`head_commit`, `status`, and `fsck_connectivity` timed out); it does not
  show the gate or probe itself failing on git

## Decision

- Primary axis: `state_materialization`
- Secondary axis: `layout_gate`

## Why `state_materialization` is the most defensible primary axis

1. The freshest same-worktree failure evidence now points there. Both
   `latest_failure_breakdown.md` and `failure_history.json` now converge on the
   newest same-worktree AC5 attempt itself, not on an older broad baseline.
   The current failure is `attempt_016`, its failed-AC set is only `5`, and the
   latest history entry records `top_axes = ["state_materialization",
   "layout_gate"]`. That is fresher than the carried `attempt_015` smoke probe
   and fresher than the older AC3-centered `attempt_013` defense.
2. The fresh solver-local evidence is attached to the signature-load corridor,
   not to a broad historical axis bucket. The included-body route fork at
   `[14544-14618]` is the live ingress, and the first direct metric published
   behind that fork is the signature-source load pair at `[9364-9370]`:
   `__dt_sig_load` feeding `time_lreuse_layout_sig_load_ns`. That makes
   `state_materialization` the first defensible reread target inside the current
   AC5 failure corridor.
3. The secondary `layout_gate` evidence is adjacent and supportive, not more
   direct. The compare-side metric at `[9365-9372]` is the paired follow-on
   witness for `time_lreuse_layout_sig_compare_ns`, so it remains the only
   allowed secondary cross-check. It corroborates the same corridor, but the
   current narrowing still begins one step earlier at signature-source load and
   materialization.
4. The branch-local build/runtime envelope also still names the materialization
   switch directly. `latest_failure_breakdown.md` surfaces `build.sh` lines `9`,
   `16`, and `17`, where `ENABLE_STATE_LOAD_MATERIALIZATION_OPT` is unset and
   then defaulted to `0`. That is attempt-local materialization evidence; it is
   stronger than importing a different axis from older retry shapes.

## Why the deferred axes should stay deferred

| Axis | Why it stays deferred on `attempt_016` |
| --- | --- |
| `layout_gate` | Keep it as the only secondary axis, not a broadened replacement. The fresh attempt-local evidence names the compare-side metric only after the signature-load metric, so `layout_gate` is corroboration inside the same corridor rather than a better first pivot. |
| `zero_span_fastpath` | `boj28350_resume/current_state_summary.md`, the bundled report, and `boj28350_progress40_results_merged.json` still keep `zero-span eligibility and fastpath commit` as the largest global residual at `49.9983%`. But that is package-level baseline context, while the freshest same-worktree failure moved into an AC5 performance-profile corridor that already names `state_materialization`. Broadening back to zero-span now would discard the fresher attempt-local lane. |
| `watch_diff`, `retain_compaction`, `carry_writeback`, `pointer_rebind`, `slot_owner_patch` | None of these axes acquire fresh attempt-local counters, route anchors, profile phases, or current-attempt probe ownership in `attempt_016`. Reopening them now would be history-driven or grep-driven, not failure-driven. |

## Why coarse history is not enough to broaden

- `failure_history.json` is still useful for recurrence, but it is too coarse to
  choose the next active axis by itself. Older entries still preserve
  `zero_span_fastpath` and other legacy axes from different failure shapes.
- The newest history entry matters because it agrees with the latest failure
  breakdown on `state_materialization` plus `layout_gate`, but it still keeps
  stale wrapper ranges. That makes history supporting evidence only; it cannot
  justify widening beyond the live attempt-local corridor already pinned in the
  branch-local analysis notes.

## Why this is narrower than broadening now

- The next reread starts from one included-body route corridor,
  `applyPieceNativeReuseForClass [14517-14620]`, not from the stale wrapper
  ranges in `boj28350_resume/boj28350_branch_3_solver.cpp:14034-14107`.
- Inside that corridor, the next solver retry should first test the exact route
  exits at `[14544-14549]`, `[14553-14557]`, `[14573-14588]`,
  `[14592-14608]`, and `[14610-14618]`, then confirm the materialization-owned
  metric at `[9364-9370]` before using the compare-side `[9365-9372]` as the
  only secondary check.
- That keeps the diagnosis narrower than broadening to unrelated axes because it
  starts from the freshest failing AC, the freshest route fork, and the first
  direct attempt-local counter family behind that fork.

## Formal-closure credibility reminder

- `AC4`, `AC5`, and `AC6` still require fresh same-worktree rerun evidence.
- Any carried-forward PASS wording remains downgraded to partial progress or a
  guard-rejected nominal PASS until the direct gate artifacts are recreated on
  the current working tree.
