# Failure Analysis Playbook

This file is an auxiliary analysis target for the branch_3 retry loop.

Purpose:
- Force each post-failure analysis mini-session to leave behind a concrete refinement to the analysis process.
- Keep the next solver retry anchored to narrower failure localization than the previous attempt.

Mandatory refinement rule:
- After each failed solver workflow, the analysis-only mini-session must update either this playbook or `.ouroboros/capture_failure_context.py` before the next solver retry is allowed to start.

Retry-gate freshness rule:
- Before any solver retry resumes, `.ouroboros/failure_analysis_state.json` must be present and explicitly marked current for the latest captured failure with a matching attempt/session/timestamp/signature tuple taken from the newest failure report and breakdown.
- Treat a mere mtime bump as insufficient. If the refreshed state cannot prove it is tied to the newest captured failure, the retry loop must stop instead of starting another blind solver retry.

Refinement ladder:
1. Identify the failed AC and blocked downstream ACs from the last workflow.
2. Map the failure to dominant execution phases.
3. Narrow the issue from file-level to symbol-level.
4. Narrow the issue from symbol-level to line-range and code-excerpt evidence.
5. If repeated failures still hit the same file/symbol/range, add a sharper analysis rule so the next capture distinguishes subcases instead of repeating the same broad diagnosis.

Expected evidence in each new failure breakdown:
- Repeated failed AC detection.
- Repeated hotspot files.
- Repeated enclosing symbols.
- Repeated line ranges.
- Trace lines that actually mention the hotspot.
- Code excerpts from the implicated region.
- A `primary_axis` and, when justified, a `secondary_axis` from the progress40 performance vocabulary.
- The last visible `profile_mode`, `release_diag` phase, and `progress checkpoint` phase when they can be inferred.
- A concrete `next_probe_command` for the next solver retry.
- Any `latest_attempt_guard.md` finding that rejects a nominal PASS as not credible.
- Any `latest_next_probe_result.md` signal that narrows the next retry to a specific axis or wrapper family.
- Any `latest_git_repo_health.md` finding that explains why git-backed inspection should be treated as degraded rather than authoritative.

Progress40 axis vocabulary:
- `watch_diff`
- `retain_compaction`
- `state_materialization`
- `carry_writeback`
- `pointer_rebind`
- `slot_owner_patch`
- `layout_gate`
- `zero_span_fastpath`

Gate-specific interpretation:
- `strong_gate` failures should prefer the correctness/proof lane first, then use progress40 axes to localize the expensive or unstable region.
- `boj3s_gate` failures should prefer the performance/profile lane first, using the current progress40 residual pivot as the default fallback when trace evidence is weak.

Axis-broadening guard:
- Do not broaden away from the pinned primary axis just because `failure_history.json` or a coarse failed-AC trace still mentions older hotspots or multiple candidate axes.
- Keep the current primary axis when all of the following remain true at the same time:
  1. `boj28350_resume/current_state_summary.md` and the bundled progress40 report still name the same largest residual or safest next pivot.
  2. `latest_next_probe_result.md` exits before meaningful solver work starts, especially on sub-second wrapper or lock quick-fails.
  3. `latest_attempt_guard.md` reports no suspicious PASS findings that would force reinterpretation of the last solver outcome.
  4. `latest_git_repo_health.md` only degrades git-backed inspection and does not show the gate or probe itself failing on git commands.
- Under those conditions, treat the new evidence as structural narrowing inside the same axis, not as permission to widen into unrelated axes such as `watch_diff`, `retain_compaction`, `carry_writeback`, or other legacy branches that the latest probe never reached.
- Current branch_3 precedent: if the authoritative summary still points to `zero-span eligibility and fastpath commit`, but `latest_next_probe_result.md` quick-fails on `another lca_strong_gate.sh run is active`, keep `zero_span_fastpath` as the only retry axis. Do not reintroduce `watch_diff`, `layout_gate`, or `state_materialization` until a later probe survives past wrapper/lock handling and reaches solver/runtime/profile evidence for those axes.
- Current branch_3 axis-comparison rule:
  1. Treat `layout_gate` as a completed predecessor axis when `current_state_summary.md` or the bundled progress40 report says `next pivot after layout-gate round: zero-span eligibility and fastpath commit`; do not broaden backward unless a new runtime/profile trace disproves that pivot.
  2. Treat `state_materialization` as fallback context only when it appears via summary-derived residual lists or coarse breakdown defaults; do not promote it to an active retry axis unless fresh solver/profile evidence reaches materialization-specific counters or phases.
  3. Treat `watch_diff` in older `latest_next_probe_result.md` or `latest_analysis_session.md` entries as historical metadata, not active guidance, when that same probe exited before meaningful solver/runtime work. A sub-second quick-fail cannot outvote the authoritative progress40 pivot.
- Only broaden to a different axis when new gate or probe evidence survives into solver/runtime/profile work and directly names that axis, or when the authoritative progress40 baseline changes.

When repeated failures stay too coarse:
- Tighten `capture_failure_context.py` to add better path extraction, symbol parsing, snippet capture, or repeated-hotspot comparison.
- If a closure AC trace only preserves truncated wrapper commands such as `sed -n '260,420p' outer_suite_wrappers/lca_...`, recover the exact wrapper path from the failed AC itself instead of leaving the hotspot at the root `lca_*_gate.sh` shim.
- When the same trace also inspects `.locks/.../pid` or prints `strong pid` / `boj3s pid`, pin the wrapper's `acquire_lock` symbol together with the observed wrapper-tail range so the next breakdown names a concrete function and line range instead of a whole wrapper file.
- Current branch_3 closure precedent: AC4 should collapse to `outer_suite_wrappers/lca_strong_gate.sh::acquire_lock [144-179]` plus the tail range around `cleanup` / `acquire_lock` / `clear_stale_state` at `275-336`; AC6 should collapse to `outer_suite_wrappers/lca_boj3s_gate.sh::acquire_lock [107-142]` plus the launch tail at `263-310`.
- Update this playbook with the newly added refinement rule so the next analysis session starts from it.

Quick-fail probe refinement:
- If `latest_next_probe_result.md` exits in under a second and the paired stderr log says `another lca_*_gate.sh run is active`, treat that as wrapper lock contention, not fresh solver-axis evidence.
- In that case keep exactly one primary progress40 axis from `current_state_summary.md` or the bundled progress40 report, and only keep a secondary axis when the failure trace independently supports it.
- Pin `outer_suite_wrappers/lca_*_gate.sh` lock handling (`release_lock`, `acquire_lock`) plus the matching `.locks/.../pid` artifact into `failure_analysis_state.json` so the next retry starts from a concrete wrapper section.
- When the stderr includes a concrete holder pid, narrow further than the whole lock helper: capture the exact fail-string line and the surrounding `acquire_lock` active-holder branch (`read -r holder`, `kill -0`, `fail`) as the primary wrapper focus range.
- Prefer a lock-focused next probe that reads `.locks/.../pid`, prints `ps -p <pid> -o pid,ppid,etime,command`, and only reruns the heavy gate after the holder process has been identified or cleared.
- If `latest_git_repo_health.md` shows pack corruption or timed-out `git status`/`git fsck`, record git-backed inspection as degraded evidence rather than treating repo health as the direct cause unless the gate/probe itself failed on git commands.
