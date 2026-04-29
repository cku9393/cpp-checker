# Branch Workspaces

Use the branch-root wrappers when working on branch-specific solvers.
Builds, solver runs, smoke gates, and artifacts should stay inside the
corresponding branch subtree.

## Active Branch Roots

- `branch_1`
  - active workspace: `ogdf_local_harness_bundle_v2/`
  - build: `branch_1/build.sh`
  - direct solver entry: `branch_1/run.sh`
  - branch-local smoke helper: `branch_1/smoke.sh`
  - outputs stay under `branch_1/ogdf_local_harness_bundle_v2/{build,dumps,artifacts}`
  - legacy root dump archive: `branch_1/root_legacy_dumps/`
- `branch_2-1`
  - active workspace: `raw_engine_v1_package/`
  - build: `branch_2-1/build.sh`
  - direct solver entry: `branch_2-1/run.sh`
  - branch-local smoke helper: `branch_2-1/smoke.sh`
  - outputs stay under `branch_2-1/raw_engine_v1_package/{build-*,artifacts}`
  - legacy root artifact archive: `branch_2-1/root_legacy_artifacts/`
- `branch_2_2`
  - active workspace: `round45_resume/`
  - Round 45-specific branch workspace reconstructed from the Round 45 bundle
  - active source: `round45_resume/round45_branch_2_2_solver.cpp`
  - build: `branch_2_2/build.sh`
  - direct solver entry: `branch_2_2/run.sh`
  - branch-local smoke helper: `branch_2_2/smoke.sh`
  - outer suite wrapper dir: `branch_2_2/outer_suite_wrappers/`
  - outer suite helpers: `branch_2_2/outer_suite_wrappers/lca_smoke.sh`, `branch_2_2/outer_suite_wrappers/lca_strong_gate.sh`, `branch_2_2/outer_suite_wrappers/lca_rebuttal_gate.sh`, `branch_2_2/outer_suite_wrappers/lca_boj3s_gate.sh`, `branch_2_2/outer_suite_wrappers/lca_hunt.sh`
  - outputs stay under `branch_2_2/artifacts/round45_resume/` and `branch_2_2/artifacts/lca_tree_stress_v5/`
- `branch_3`
  - active workspace: `boj28350_resume/`
  - active source: `boj28350_resume/boj28350_branch_3_solver.cpp`
  - build: `branch_3/build.sh`
  - direct solver entry: `branch_3/run.sh`
  - branch-local smoke helper: `branch_3/smoke.sh`
  - outer suite wrapper dir: `branch_3/outer_suite_wrappers/`
  - outer suite helpers: `branch_3/outer_suite_wrappers/lca_smoke.sh`, `branch_3/outer_suite_wrappers/lca_strong_gate.sh`, `branch_3/outer_suite_wrappers/lca_rebuttal_gate.sh`, `branch_3/outer_suite_wrappers/lca_boj3s_gate.sh`, `branch_3/outer_suite_wrappers/lca_hunt.sh`
  - outputs stay under `branch_3/artifacts/boj28350_resume/` and `branch_3/artifacts/lca_tree_stress_v5/`
  - smoke generation/validation reuses outer `lca_tree_stress_v5/`

## Separate Non-Branch Workspace

- `lca_tree_stress_v5/`
  - separate outer workspace added during the 2026-03-24 workspace reorganization
  - the outer stress suite line itself predates `branch_2_2`; this subtree is the reorganized standalone home for that line
  - active source: `lca_tree_stress_v5/lca_tree_stress_v5_solver.cpp`
  - shared harness lives under `lca_tree_stress_v5/tooling/`
  - not part of the branch-folder wrapper model above
