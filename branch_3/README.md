# branch_3 Workspace Guide

`branch_3` is the self-contained BOJ 28350 branch workspace reconstructed from
`boj28350_current_state_bundle.zip`.

Layout:

- `boj28350_resume/`
  - active BOJ 28350 workspace
  - branch-local solver source is `boj28350_resume/boj28350_branch_3_solver.cpp`
  - branch-local solver launcher is `boj28350_resume/solve`
- `boj28350_resume.py`
  - branch-local build/smoke runner
- `artifacts/boj28350_resume/`
  - branch-local smoke outputs and summaries
- `boj28350_bundle_archive/`
  - extracted archive of the original current-state bundle
- `boj28350_current_state_bundle.zip`
  - preserved original zip

Execution model:

- `./build.sh` compiles `branch_3/boj28350_resume/boj28350_branch_3_solver.cpp`
  into `branch_3/artifacts/boj28350_resume/build/solve`
- `./run.sh` directly executes `branch_3/boj28350_resume/solve`
- direct `boj28350_resume/solve` / `./run.sh` invocations default solver-side
  auxiliary/profile output to `branch_3/artifacts/boj28350_resume/direct_solver_aux`
- `./smoke.sh` runs branch-local smoke cases using the outer `lca_tree_stress_v5/`
  generator and validator
- smoke outputs go under `branch_3/artifacts/boj28350_resume/...`
- this branch does not use the top-level `lca_tree_stress_v5/lca_tree_stress_v5_solver.cpp`

Recommended commands:

```bash
cd branch_3
./build.sh
./run.sh < in.txt > out.txt
./smoke.sh
```

Required acceptance procedure:

```bash
./lca_smoke.sh
./lca_smoke_repeatability.sh 3
./outer_suite_wrappers/lca_strong_gate.sh
./outer_suite_wrappers/lca_boj3s_gate.sh
```

AC7 repeatability helper after the required gates are individually green:

```bash
./lca_required_repeatability.sh 2
```

Single-case smoke replay helper:

```bash
./lca_smoke_target.sh --list
./lca_smoke_target.sh 1
```

Optional diagnostic wrappers:

```bash
./outer_suite_wrappers/lca_rebuttal_gate.sh
./lca_hunt.sh [label] [sizes_csv] [seeds_csv] [timeout_sec]
```

Optional hardest-case hunt usage:

```bash
./lca_hunt.sh [label] [sizes_csv] [seeds_csv] [timeout_sec]
```

`./lca_hunt.sh` forwards to `./outer_suite_wrappers/lca_hunt.sh`, searches for
the slowest validated cases, and writes its outputs
under `artifacts/lca_tree_stress_v5/hunt/...`. It is an optional
diagnostic/reporting helper only. Use it after or between gate runs when you
need bottleneck signal; do not include it in the required acceptance sequence
or formal pass/fail evaluation.

These wrappers keep outer-suite outputs under
`branch_3/artifacts/lca_tree_stress_v5/...`.
Each outer-suite case runs from its own `runs/...` case directory, so any
solver-side auxiliary outputs also remain inside the branch-local artifact tree.
`./lca_smoke_repeatability.sh` reruns `./lca_smoke.sh` on the same working tree,
stores each iteration under `artifacts/lca_tree_stress_v5/smoke_repeatability/`,
records each run's exit outcome, snapshots either the stable smoke output tree
or the published `smoke_latest_failure/` bundle, and fails with run-local logs
plus a manifest/signature diff if a rerun flakes or changes content outside of
`time.txt` and the volatile timing/staging-path lines in `run_case.stdout.txt`.
Treat smoke as stable enough for further solver iteration only after this
repeatability wrapper shows a matching repeated outcome: `status=PASS` means
the smoke path stayed green, while `status=CONSISTENT_FAIL` means the current
solver still fails but does so reproducibly enough to use as a stable debugging
signal.
`./lca_required_repeatability.sh` is the branch-local AC7 helper for the
required gate sequence. It reruns `./lca_strong_gate.sh` followed by
`./lca_boj3s_gate.sh` on the same working tree, stores each cycle under
`artifacts/lca_tree_stress_v5/required_repeatability/`, snapshots each gate's
`certify.json` and `certify_summary.md`, extracts a stable PASS signature from
each gate's `certify.json`, and only returns PASS if every cycle's gate verdict
is `PASS` and the extracted PASS signatures match the baseline cycle without
any manual artifact cleanup between cycles.
`./lca_smoke_target.sh` is a branch-local helper for rerunning one
manifest-defined `lca_smoke` case with the same `branch_run_case.py` arguments,
timeout, and `DENSE_*` solver env flags used by `./lca_smoke.sh`. Use
`./lca_smoke_target.sh --list` to inspect the deterministic case tags and
`./lca_smoke_target.sh <case-index-or-tag> [artifact_subpath]` to replay one
target under `artifacts/lca_tree_stress_v5/smoke_target/...` without editing
the wrapper body.
Formal pass/fail evaluation in this branch is determined by the wrapper path
`./outer_suite_wrappers/lca_smoke.sh` -> `./outer_suite_wrappers/lca_strong_gate.sh` -> `./outer_suite_wrappers/lca_boj3s_gate.sh`, with
repeatability checks on the required gates as needed. `./outer_suite_wrappers/lca_hunt.sh` remains a
separate hardest-case search/reporting helper and is not part of formal
acceptance.

Notes:

- the active baseline is the latest extracted solver snapshot,
  `boj28350_literature_progress40_layout_signature_reuse_gate.cpp`, copied into
  `boj28350_resume/boj28350_branch_3_solver.cpp`
- smoke tooling reuses the outer `lca_tree_stress_v5/` workspace and
  `lca_tree_stress_v5/tooling/` harness
- the original archive scripts inside `boj28350_bundle_archive/` still use old
  `/mnt/data/...` paths and are preserved as references only
