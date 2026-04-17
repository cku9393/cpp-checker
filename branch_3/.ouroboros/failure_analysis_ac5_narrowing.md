# AC5 Narrowing Note

- Timestamp: `2026-04-10 14:27:29 KST`
- Current failure signature: `attempt_024|orch_8b5bba834347|2026-04-10 14:12:03 KST|2,3,8`
- Pinned AC subset: `2`
- Excluded ACs: `3, 8`
- Primary axis: `zero_span_fastpath`
- Secondary axis: `none`
- Next probe command: `./lca_smoke.sh`

## Narrowed Reread Order

1. `artifacts/lca_tree_stress_v5/retry_loop/latest_failure_report.md [70-87]`
2. `artifacts/lca_tree_stress_v5/retry_loop/latest_failure_report.md [136-140]`
3. `artifacts/lca_tree_stress_v5/retry_loop/latest_failure_report.md [248-249]`
4. `artifacts/lca_tree_stress_v5/retry_loop/attempt_024_20260410_130747/workflow.log [459-483]`
5. `artifacts/lca_tree_stress_v5/retry_loop/latest_attempt_guard.md [8-20]`
6. `boj28350_resume/current_state_summary.md [44-53]`

## AC Subset Reasons

- `AC2` is the narrowest live ingress because the same attempt published a fresh smoke PASS and escalation handoff before the session stalled.
- `AC3` failed without any fresh strong-gate status bundle, and `latest_attempt_guard.md` still downgrades it to `missing_direct_gate_evidence`.
- `AC8` stalled only on artifact-path inspection and does not carry any direct solver or progress40-axis evidence that outranks the AC2 handoff mismatch.

## Historical Metadata To Suppress

- `latest_next_probe_result.md` still points at attempt `022` and its `watch_diff` / `retain_compaction` probe, so it is historical only for attempt `024`.
- `latest_failure_breakdown.md` still reuses attempt `023` transport anchors and stale AC3/AC8 secondary axes under the attempt `024` header.
- `latest_git_repo_health.md` timed out on git inspection, but that degrades trust only; it does not establish a competing axis or a git-root cause for the current failure.
