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
- the required gate wrappers build from the same artifact-rooted binary and run
  per-gate frozen solver snapshots so each gate attempt keeps a stable
  executable under its own artifact-owned runtime envelope
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
./lca_acceptance_repeatability.sh 2
```

Solver-change workflow notes:

- before any major rewrite or pivot of
  `boj28350_resume/boj28350_branch_3_solver.cpp`, both required research
  source sets must be reviewed and recorded first
- `source set A` is the branch-local research package:
  `boj28350_resume/README.md`,
  `boj28350_resume/current_state_summary.md`,
  `boj28350_resume/next_session_briefing.md`,
  `boj28350_complete_master_document_partA_raw.md`,
  `boj28350_integrated_technical_history.md`,
  `boj28350_literature_progress7_bcdecomp_report.md`,
  `literature_grade_proof_package.md`,
  `boj28350_resume/pre_rewrite_checkpoint.md`,
  `boj28350_resume/pre_rewrite_synthesis_note.md`, and
  `boj28350_resume/progress40_derived_reference.md`
- `source set B` is the bundled `progress40` authoritative package:
  `boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp`,
  `boj28350_bundle_archive/boj28350_progress40_layout_signature_reuse_gate_report.md`,
  and `boj28350_bundle_archive/boj28350_progress40_results_merged.json`
- do not open a planning note, retry note, or solver-side major-change task as
  a rewrite/pivot unless it explicitly cites
  `boj28350_resume/pre_rewrite_checkpoint.md` or
  `boj28350_resume/pre_rewrite_synthesis_note.md` and restates that both
  source-set reviews are complete in the current working tree

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
`./lca_smoke.sh` itself now normalizes its public outcome into exactly three
families for downstream iteration: exit `0` for PASS, exit `1` for a preserved
reproducible solver failure, and exit `70` for launcher/harness/infrastructure
failure. Each run also refreshes
`artifacts/lca_tree_stress_v5/smoke_latest_status/summary.txt` plus
`latest_status_report.md` so downstream wrappers can tell which family they saw
without reverse-engineering the raw inner-wrapper exit code.
Treat smoke as stable enough for further solver iteration only after this
repeatability wrapper shows a matching repeated outcome: `status=PASS` means
the smoke path stayed green, while `status=CONSISTENT_FAIL` means the current
solver still fails but does so reproducibly enough to use as a stable debugging
signal. Repeated launcher or harness failures remain `status=FAIL` even when
their normalized exit code is stable, because those runs are not valid solver
iteration signal. The published
`artifacts/lca_tree_stress_v5/smoke_repeatability/summary.txt` now records
that decision explicitly with `supports_solver_iteration=1|0` plus a
`solver_iteration_basis=` classification tied to the back-to-back smoke runs.
The manifest comparison also normalizes the known volatile
`solver_env_snapshot.json` fields
(`solver.mtime_ns`, `solver.sha256`, `solver.path`, and
`tracked_env.DENSE_PROFILE_OUTDIR`) so reproducibility is judged on stable
smoke behavior rather than per-run build IDs or temporary output directories.
`./lca_required_repeatability.sh` is the branch-local AC7 helper for the
required gate sequence. It reruns `./lca_strong_gate.sh` followed by
`./lca_boj3s_gate.sh` on the same working tree, stores each cycle under
`artifacts/lca_tree_stress_v5/required_repeatability/`, snapshots each gate's
`certify.json`, `certify_summary.md`, `runtime_env.txt`, and
`preflight_manifest.tsv`, extracts a stable PASS signature from each gate's
`certify.json`, and only returns PASS if every cycle's gate verdict is `PASS`,
the extracted PASS signatures match the baseline cycle, fresh runtime/preflight
artifacts were regenerated instead of reused, and the shared
`artifacts/lca_tree_stress_v5/` root survives each consecutive cycle without
any manual artifact cleanup between cycles.
`./lca_acceptance_repeatability.sh` is the branch-local full-flow
reproducibility helper for AC7 closure. It reruns the full
`./lca_smoke.sh -> ./lca_strong_gate.sh -> ./lca_boj3s_gate.sh` path on the
same unchanged working tree, stores each full cycle under
`artifacts/lca_tree_stress_v5/acceptance_repeatability/`, compares the
normalized smoke snapshot and smoke status signature against the baseline
cycle, reuses the gate PASS-signature checks from the required-gate helper,
and only returns PASS if every repeated cycle stays green without any manual
artifact cleanup between runs.
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
