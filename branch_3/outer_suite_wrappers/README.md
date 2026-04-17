# branch_3 Outer Suite Wrappers

These wrappers run the shared `lca_tree_stress_v5` harness against the
branch-local build output. The required gates build
`../artifacts/boj28350_resume/build/solve` and hand the harness a frozen
artifact-rooted solver snapshot for each run, while ad hoc direct runs can
still use `../boj28350_resume/solve`.

Outputs stay under `../artifacts/lca_tree_stress_v5/`.

The branch also keeps a materialized strong-gate preset mirror at
`../artifacts/lca_tree_stress_v5/.preset_cache/lca_strong_gate.json`. The
wrapper can safely prefer that full-gate JSON when the live iCloud-backed
`suite_presets/strong_gate.json` sources surface as `compressed,dataless`,
which keeps AC3 reproducible without changing the actual gate surface.

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
the same solver-side failure signal on each rerun. Repeated harness failures
stay `status=FAIL` even when their normalized exit code matches, because they
do not provide a safe basis for continued solver iteration. Use it to confirm
smoke is stable enough before spending more iteration budget on solver changes.
Its published `summary.txt` also records `supports_solver_iteration=1|0` and
`solver_iteration_basis=...` so the back-to-back verdict is explicit instead of
being inferred indirectly from the top-level status alone. The snapshot-manifest
comparison normalizes the volatile `solver_env_snapshot.json` fields that only
track per-run build metadata or temp output roots, so the check stays focused on
stable smoke behavior.

The public branch-local entrypoint `../lca_smoke.sh` normalizes smoke into
three outcome families for downstream tooling: exit `0` for PASS, exit `1` for
a preserved reproducible solver failure, and exit `70` for launcher or
harness/infrastructure failure. It also refreshes
`../artifacts/lca_tree_stress_v5/smoke_latest_status/summary.txt` and
`latest_status_report.md` on every run so callers can inspect a stable status
bundle regardless of which family occurred.

`./lca_required_repeatability.sh [repeat-count]` reruns the required gate
sequence, `./lca_strong_gate.sh` then `./lca_boj3s_gate.sh`, on the same
working tree without manual cleanup. It records each cycle under
`../artifacts/lca_tree_stress_v5/required_repeatability/`, extracts a stable
PASS signature from each gate's `certify.json`, preserves the gate-local
`runtime_env.txt` and `preflight_manifest.tsv` evidence, and only returns PASS
if every gate verdict remains `PASS`, every extracted PASS signature matches
the baseline cycle, those runtime/preflight artifacts were freshly regenerated,
and the shared `../artifacts/lca_tree_stress_v5/` root itself survives the
consecutive reruns.

`./lca_acceptance_repeatability.sh [repeat-count]` is the full-flow closure
helper for AC7. It reruns `./lca_smoke.sh -> ./lca_strong_gate.sh ->
./lca_boj3s_gate.sh` on the same working tree without manual cleanup, records
each full cycle under `../artifacts/lca_tree_stress_v5/acceptance_repeatability/`,
requires the smoke status bundle to refresh to a PASS signature on every cycle,
compares the normalized smoke snapshot manifests against the baseline cycle,
and then applies the same fresh PASS-signature checks to the strong and BOJ 3s
gate outputs before declaring a repeated full-flow PASS.

Diagnostic-only helpers remain outside that required gate path:

- `./lca_rebuttal_gate.sh`
- `./lca_hunt.sh`

`./lca_hunt.sh` is only for hardest-case search/reporting and must not be
treated as a formal acceptance gate.
