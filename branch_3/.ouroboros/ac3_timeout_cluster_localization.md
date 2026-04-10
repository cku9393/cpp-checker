# Strong-Gate Zero-Progress Corridor Localization

This note exists to leave the next solver retry with the current `attempt_013`
trust-boundary slices, not the stale `attempt_011` solver-only reread order and
not the mislocalized wrapper snippets that currently point at env-root or stale
cleanup lines.

Current failure basis:
- Failed attempt: `attempt_013`
- Failed AC: `3`
- Failure body: `Selected model is at capacity. Please try a different model.`
- Guard signal: `latest_attempt_guard.md` rejects `AC3` on
  `missing_direct_gate_evidence`
- Row artifact:
  `artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_probe_v3.latest_failure/certify_rows.csv`
- Row freshness: `predates attempt_013 start`; use it only as carried-forward
  background, not as fresh same-worktree gate evidence
- Primary axis: `zero_span_fastpath`
- Secondary axis: `none`

Why this is narrower than before:
- The current generated AC3 breakdown labels useful retry anchors, but several
  rendered snippets still point at unrelated lines such as `local env_path=""`,
  `"$HOME"`, and `shopt -u nullglob`.
- That means the next retry should not start from `assert_runtime_environment`
  or `clear_stale_state`; the live reread target is the exact wrapper/certify
  zero-progress corridor that would create the first trustworthy `time.txt` and
  published run.
- The latest failure produced no fresh same-worktree strong-gate artifact, so
  exact wrapper/certify handoff statements now outrank older solver-side
  zero-span counters. Solver rereads stay secondary until direct gate evidence
  exists again.

## Why the primary axis should not broaden yet

- `boj28350_resume/current_state_summary.md`,
  `boj28350_progress40_layout_signature_reuse_gate_report.md`, and
  `boj28350_progress40_results_merged.json` still agree on the same pivot:
  `zero-span eligibility and fastpath commit` remains the largest residual at
  `49.9983%`.
- `latest_attempt_guard.md` only downgrades closure credibility. It does not
  provide fresh solver/runtime/profile evidence for `retain_compaction`,
  `state_materialization`, or any other competing axis.
- `latest_git_repo_health.md` only shows timed-out git inspection plus reflog
  noise; that degrades git-backed inspection but does not explain the missing
  strong-gate output.
- `latest_next_probe_result.md` is still the last solver-facing probe, but it
  predates `attempt_013`; keep it as background context only until a new AC3
  rerun recreates direct gate evidence.

## Exact retry anchors

Start here before any wider wrapper or solver reread.

1. `outer_suite_wrappers/lca_strong_gate.sh [404-405]`
   - Published-run witness inside `count_completed_cases()`.

```text
404:   if [[ -n "${WORKDIR:-}" && -d "$WORKDIR/runs" ]]; then
405:     published_count="$(find "$WORKDIR/runs" -type f -name 'time.txt' 2>/dev/null | wc -l | tr -d '[:space:]')"
```

2. `outer_suite_wrappers/lca_strong_gate.sh [408-409]`
   - Active-run witness under `$TMP_PARENT/case_runs`.

```text
408:   if [[ -f "$LOCK_PID_FILE" && -d "$TMP_PARENT/case_runs" ]]; then
409:     active_count="$(find "$TMP_PARENT/case_runs" -type f -name 'time.txt' -newer "$LOCK_PID_FILE" 2>/dev/null | wc -l | tr -d '[:space:]')"
```

3. `outer_suite_wrappers/lca_strong_gate.sh [447-449]`
   - Actual subprocess launch into `branch_certify_suite.py`.

```text
447:   BRANCH_CERTIFY_REPORT_OUTDIR="$OUTROOT" \
448:     python3 "$CERTIFY_HELPER" --solver "$SOLVER_SNAPSHOT" --preset "$PRESET" --out "$WORKDIR" --limit-scale "$LIMIT_SCALE" &
449:   CERTIFY_PID=$!
```

4. `outer_suite_wrappers/lca_strong_gate.sh [458-459]`
   - Heartbeat that samples and emits `completed_cases`.

```text
458:     completed="$(count_completed_cases)"
459:     echo "[lca_strong_gate] heartbeat elapsed=${elapsed}s completed_cases=${completed} workdir=$WORKDIR" >&2
```

5. `branch_certify_suite.py [431-431]`
   - Exact case-local timing artifact declaration.

```text
431:         time_path = work_dir / "time.txt"
```

6. `branch_certify_suite.py [450-459]`
   - Actual solver timing handoff that should write `time.txt`.

```text
450:         rc_sol, to_sol, sec, rss = branch_run_solver_with_time(
451:             solver,
452:             in_path,
453:             out_path,
454:             time_path,
455:             solver_stderr,
456:             timeout,
457:             env=solver_env,
458:             cwd=work_dir,
459:         )
```

7. `branch_certify_suite.py [467-467]`
   - Exact published-run handoff.

```text
467:         _publish_case_dir(work_dir, case_dir)
```

## Retry start order

1. Regenerate direct AC3 evidence with
   `LCA_STAGE_FILTER=correctness_fuzz ./lca_strong_gate.sh`.
2. If the rerun still shows `completed_cases=0`, start from the four wrapper
   anchors above, in order: published count, active count, certify launch,
   heartbeat sample.
3. If the wrapper counts or heartbeat move, jump immediately to
   `branch_certify_suite.py [431]`, `[450-459]`, and `[467]` before rereading
   any solver-side zero-span counter family.
4. Only after a fresh same-worktree rerun recreates direct gate output should
   the older solver-side `zero_span_fastpath` anchors regain priority over this
   zero-progress corridor.
