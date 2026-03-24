# branch_3 Workspace Guide

`branch_3` is the self-contained BOJ 28350 branch workspace reconstructed from
`boj28350_current_state_bundle.zip`.

Layout:

- `boj28350_resume/`
  - active BOJ 28350 workspace
  - branch-local solver source is `boj28350_resume/boj28350_branch_3_solver.cpp`
  - branch-local binary is `boj28350_resume/solve`
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
- `./run.sh` directly executes `branch_3/boj28350_resume/solve`
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

Outer `lca_tree_stress_v5` acceptance wrappers:

```bash
./lca_smoke.sh
./lca_strong_gate.sh
./lca_rebuttal_gate.sh
./lca_boj3s_gate.sh
```

Diagnostic wrapper:

```bash
./lca_hunt.sh
```

These wrappers keep outer-suite outputs under
`branch_3/artifacts/lca_tree_stress_v5/...`.
Each outer-suite case runs from its own `runs/...` case directory, so any
solver-side auxiliary outputs also remain inside the branch-local artifact tree.
`./lca_hunt.sh` is a hardest-case search/reporting helper; formal acceptance is
determined by the required gate wrappers, especially `./lca_strong_gate.sh` and
`./lca_boj3s_gate.sh`.

Notes:

- the active baseline is the latest extracted solver snapshot,
  `boj28350_literature_progress40_layout_signature_reuse_gate.cpp`, copied into
  `boj28350_resume/boj28350_branch_3_solver.cpp`
- smoke tooling reuses the outer `lca_tree_stress_v5/` workspace at repo root
- the original archive scripts inside `boj28350_bundle_archive/` still use old
  `/mnt/data/...` paths and are preserved as references only
