# branch_3 Outer Suite Wrappers

These wrappers run the shared `lca_tree_stress_v5` harness against the
branch-local solver in `../boj28350_resume/solve`.

Outputs stay under `../artifacts/lca_tree_stress_v5/`.

Formal acceptance in this branch is driven by `./lca_smoke.sh`,
`./lca_strong_gate.sh`, and `./lca_boj3s_gate.sh`.

`./lca_smoke_target.sh --list` prints the deterministic smoke-case selectors
derived from `../boj28350_resume/smoke_cases.tsv`. Running
`./lca_smoke_target.sh <case-index-or-tag> [artifact_subpath]` replays one of
those manifest rows with the same `branch_run_case.py` arguments, timeout, and
`DENSE_*` solver env flags used by `./lca_smoke.sh`, while keeping outputs
under `../artifacts/lca_tree_stress_v5/smoke_target/...`. It is a reproducible
single-target helper, not a formal acceptance gate.

`./lca_smoke_repeatability.sh [repeat-count]` is a separate smoke
reproducibility helper. It reruns `./lca_smoke.sh` on the same working tree,
stores each invocation under
`../artifacts/lca_tree_stress_v5/smoke_repeatability/`, records each run's exit
outcome, and compares either the resulting smoke artifact tree or the published
failure signature after normalizing the known volatile timing and path-report
fields. `status=PASS` means repeated green runs matched. `status=CONSISTENT_FAIL`
means the current solver still fails, but the smoke path is at least producing
the same failure signal on each rerun. Use it to confirm smoke is stable enough
before spending more iteration budget on solver changes.

`./lca_required_repeatability.sh [repeat-count]` reruns the required gate
sequence, `./lca_strong_gate.sh` then `./lca_boj3s_gate.sh`, on the same
working tree without manual cleanup. It records each cycle under
`../artifacts/lca_tree_stress_v5/required_repeatability/`, extracts a stable
PASS signature from each gate's `certify.json`, and only returns PASS if every
gate verdict remains `PASS` and every extracted PASS signature matches the
baseline cycle.

Diagnostic-only helpers remain outside that required gate path:

- `./lca_rebuttal_gate.sh`
- `./lca_hunt.sh`

`./lca_hunt.sh` is only for hardest-case search/reporting and must not be
treated as a formal acceptance gate.
