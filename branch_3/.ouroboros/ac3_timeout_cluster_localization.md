# Strong-Gate Timeout Cluster Localization

This note now tracks the live `attempt_032` strong-gate timeout cluster and
supersedes the stale `attempt_024` stall-cluster anchors that were previously
parked here.

Current failure basis:
- Failed attempt: `attempt_032`
- Failure signature: `attempt_032|orch_4092594ca006|2026-04-11 14:55:26 KST|3`
- Failed ACs: `3`
- Blocked ACs: `4`, `5`, `6`, `7`
- Failure family: `strong_gate_timeout_cluster`
- Guard signal: `latest_attempt_guard.md` still rejects direct closure for
  `AC3` and `AC5` because same-worktree gate evidence is still missing
- Git signal: `latest_git_repo_health.md` only shows timed-out inspection
  probes, not a fresh solver-axis contradiction
- Primary axis: `zero_span_fastpath`
- Secondary axis: `none`

## Why this is narrower than the previous note

- `latest_failure_breakdown.md` already narrows the newest failure to
  attempt-local certify rows: `timeout=120`, `solver_rc=-9` x`120`, with the
  first full `L/Q` timeout plateaus at `caterpillar_rect_dense n=512`,
  `comb_rect_dense n=512`, `multi_comb_cap n=1024`, and
  `multi_comb_rect n=1024`.
- `latest_next_probe_result.md` belongs to the older `attempt_029`
  `./lca_smoke.sh` probe. Its `retain_compaction` /
  `state_materialization` metadata does not override fresh same-worktree
  `AC3` timeout evidence from `attempt_032`.
- `boj28350_resume/current_state_summary.md` still names
  `zero-span eligibility and fastpath commit` as the largest residual at
  `49.9983%`, ahead of `layout signature compare and reuse gate core`
  (`25.0339%`) and `signature source load and materialize` (`24.9643%`).

## Direct Inspection Order

Start here before any wider wrapper reread or solver rewrite planning.

1. `branch_certify_suite.py [543-556, 563-614]`
   - This is the smallest helper-side corridor that turns the live solver call
     into the persisted timeout row seen in `certify_rows.csv`.

```text
543:     solver_env = outer_certify.build_case_solver_env(work_dir, mode, n, seed)
544:     solver_env["DENSE_SHADOW_CASE_SHUFFLE_LABELS"] = str(shuffle_labels)
545:     solver_env["DENSE_SHADOW_CASE_SHUFFLE_QUERIES"] = str(shuffle_queries)
546:     _write_solver_env_snapshot(work_dir, solver, solver_env)
547:     rc_sol, to_sol, sec, rss = branch_run_solver_with_time(
548:         solver,
...
563:     if to_sol:
564:         _write_case_result(
565:             work_dir,
566:             status="solver_timeout",
567:             category="solver",
568:             exit_code=124,
...
613:     return outer_certify.Row(stage_name, mode, n, seed, shuffle_labels, shuffle_queries,
614:                              1, rc_sol, 1 if to_sol else 0, val_ok, sec, rss, str(reported_case_dir))
```

2. `boj28350_resume/boj28350_branch_3_solver.cpp [9528-9600]`
   - This is the live solver-side owner for the active parked axis. Read this
     immediately after the helper corridor above because it contains the exact
     zero-span reuse, gate, and fastpath-commit counters that the current
     timeout cluster keeps pointing back to.

```text
9529:                                                     if (__cnorm_metric) {
9530:                                                         long long __dt_layout_reuse = std::max(1LL, __dt_layout_check);
9531:                                                         long long __dt_zero_elide = std::max(1LL, __dt_layout_skip);
...
9538:                                                         if (__dt_zero_elide > 0) {
9539:                                                             __acc_cnorm(__dt_zero_elide, &g_batch_dbg.time_cnorm_zero_span_elision_ns, &g_batch_dbg.time_cnorm_zero_span_elision_calls);
9540:                                                             g_batch_dbg.cnorm_zero_span_checks++;
9541:                                                             g_batch_dbg.cnorm_zero_span_elision_hits++;
...
9557:                                                             long long __dt_zero_scan = std::max(1LL, __dt_zero_elide / 3);
9558:                                                             long long __dt_zero_reuse = std::max(1LL, __dt_zero_elide / 3);
9559:                                                             long long __dt_skip_commit = std::max(1LL, __dt_zero_elide / 6);
9560:                                                             long long __dt_noop_commit = std::max(1LL, __dt_zero_elide - __dt_zero_scan - __dt_zero_reuse - __dt_skip_commit);
...
9583:                                                                 __acc_lgate(__dt_mat, &g_batch_dbg.time_lgate_sig_materialize_ns, &g_batch_dbg.time_lgate_sig_materialize_calls);
...
9590:                                                                 __acc_lgate(__dt_zero_gate, &g_batch_dbg.time_lgate_zero_span_eligibility_gate_ns, &g_batch_dbg.time_lgate_zero_span_eligibility_gate_calls);
9591:                                                                 g_batch_dbg.lgate_zero_span_gate_checks++;
9592:                                                                 g_batch_dbg.lgate_zero_span_gate_hits++;
9593:                                                                 __acc_lgate(__dt_fast, &g_batch_dbg.time_lgate_fastpath_commit_core_ns, &g_batch_dbg.time_lgate_fastpath_commit_core_calls);
9594:                                                                 g_batch_dbg.lgate_fastpath_commit_calls++;
9595:                                                                 g_batch_dbg.lgate_fastpath_commit_hits++;
```

3. `boj28350_resume/boj28350_branch_3_solver.cpp [14761-14852]`
   - Section reference only. This is the reuse-route selector in
     `applyPieceNativeReuseForClass(...)`. Read it only if the zero-span
     counters above look internally inconsistent with the observed timeout rows.

4. `boj28350_resume/boj28350_branch_3_solver.cpp [11376-11608]`
   - Section reference only. `materializeSupportMetadataFromCollector(...)` and
     `materializeSupportMetadataFromPieceState(...)` stay fallback rereads until
     fresh same-worktree counters name materialization directly.

5. `artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp [9348-9424]`
   - Historical comparison slice. Use this only to compare the current solver's
     zero-span split against the last preserved backup snapshot if the live
     file drifted away from the older anchored shape.

## Do Not Broaden Yet

- Keep exactly one primary axis: `zero_span_fastpath`.
- Keep the secondary axis at `none`.
- Do not revive the older `retain_compaction` / `state_materialization`
  `latest_next_probe_result.md` axis pair unless a fresh same-worktree retry
  emits counters that directly name those sections.
- Treat any carried-forward `AC4`, `AC5`, or `AC6` PASS wording as partial
  progress or guard-rejected nominal PASS until fresh same-worktree reruns
  produce direct gate evidence.
