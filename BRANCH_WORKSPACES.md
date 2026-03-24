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
- `branch_2-1`
  - active workspace: `raw_engine_v1_package/`
  - build: `branch_2-1/build.sh`
  - direct solver entry: `branch_2-1/run.sh`
  - branch-local smoke helper: `branch_2-1/smoke.sh`
  - outputs stay under `branch_2-1/raw_engine_v1_package/{build-*,artifacts}`
- `branch_2_2`
  - active workspace: `round45_resume/`
  - active source: `round45_resume/round45_branch_2_2_solver.cpp`
  - build: `branch_2_2/build.sh`
  - direct solver entry: `branch_2_2/run.sh`
  - branch-local smoke helper: `branch_2_2/smoke.sh`
  - outer suite helpers: `branch_2_2/lca_smoke.sh`, `branch_2_2/lca_strong_gate.sh`, `branch_2_2/lca_rebuttal_gate.sh`, `branch_2_2/lca_boj3s_gate.sh`, `branch_2_2/lca_hunt.sh`
  - outputs stay under `branch_2_2/artifacts/round45_resume/` and `branch_2_2/artifacts/lca_tree_stress_v5/`
- `branch_3`
  - active workspace: `boj28350_resume/`
  - active source: `boj28350_resume/boj28350_branch_3_solver.cpp`
  - build: `branch_3/build.sh`
  - direct solver entry: `branch_3/run.sh`
  - branch-local smoke helper: `branch_3/smoke.sh`
  - outer suite helpers: `branch_3/lca_smoke.sh`, `branch_3/lca_strong_gate.sh`, `branch_3/lca_rebuttal_gate.sh`, `branch_3/lca_boj3s_gate.sh`, `branch_3/lca_hunt.sh`
  - outputs stay under `branch_3/artifacts/boj28350_resume/` and `branch_3/artifacts/lca_tree_stress_v5/`
  - smoke generation/validation reuses outer `lca_tree_stress_v5/`

## Separate Non-Branch Workspace

- `lca_tree_stress_v5/`
  - separate outer workspace
  - active source: `lca_tree_stress_v5/lca_tree_stress_v5_solver.cpp`
  - not part of the branch-folder wrapper model above
