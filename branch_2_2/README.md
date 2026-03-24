# branch_2_2 Workspace Guide

`branch_2_2` is the self-contained Round 45 branch workspace.

Layout:

- `round45_resume/`
  - active Round 45 workspace
  - branch-local solver source is `round45_resume/round45_branch_2_2_solver.cpp`
  - branch-local binaries are `round45_resume/solve` and `round45_resume/solve_prof`
- `round45_resume.py`
  - branch-local build/smoke runner
- `artifacts/round45_resume/`
  - branch-local smoke outputs, row files, and summaries
- `round45_bundle_archive/`
  - preserved historical bundle root
- `round45_artifacts/`
  - extracted reports, manifests, and reference files

Execution model:

- `./build.sh` compiles `branch_2_2/round45_resume/round45_branch_2_2_solver.cpp`
- `./run.sh` directly executes `branch_2_2/round45_resume/{solve_prof|solve}`
- `./smoke.sh` runs `branch_2_2/round45_resume/solve_prof`
- smoke outputs go under `branch_2_2/artifacts/round45_resume/...`
- this branch does not use `lca_tree_stress_v5/lca_tree_stress_v5_solver.cpp`

Recommended commands:

```bash
cd branch_2_2
./build.sh
./run.sh < in.txt > out.txt
./smoke.sh
```

Outer `lca_tree_stress_v5` standard wrappers:

```bash
./lca_smoke.sh
./lca_strong_gate.sh
./lca_rebuttal_gate.sh
./lca_boj3s_gate.sh
./lca_hunt.sh
```

These wrappers use the plain branch solver `round45_resume/solve` and keep
outer-suite outputs under `branch_2_2/artifacts/lca_tree_stress_v5/...`.
Each outer-suite case now runs with its own `runs/...` case directory as both
working directory and `DENSE_PROFILE_OUTDIR`, so Round 45 row files also stay
inside the branch-local artifact tree.

The outer `lca_tree_stress_v5/` workspace is separate. It has its own
`lca_tree_stress_v5_solver.cpp`, its own binary, and its own `lca_tree_stress_v5/artifacts/`
output tree.

Historical notes:

- old summaries in `artifacts/round45_resume/` were path-normalized after the
  branch-local move
- they were not regenerated during that cleanup
