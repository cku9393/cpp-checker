# Ouroboros Prep

This document records the effective validation contract before starting
Ouroboros work.

## Effective Suite Structure

The practical `lca_tree_stress_v5` standard is split across two places.

- Solver workspace:
  - [lca_tree_stress_v5](/Users/free_1/Library/Mobile%20Documents/iCloud~md~obsidian/Documents/cpp-checker/lca_tree_stress_v5)
  - active source: `lca_tree_stress_v5/lca_tree_stress_v5_solver.cpp`
  - local smoke wrapper: `lca_tree_stress_v5/smoke.sh`
- Certification harness:
  - repo root [README.md](/Users/free_1/Library/Mobile%20Documents/iCloud~md~obsidian/Documents/cpp-checker/README.md)
  - [certify_suite.py](/Users/free_1/Library/Mobile%20Documents/iCloud~md~obsidian/Documents/cpp-checker/certify_suite.py)
  - [gate.sh](/Users/free_1/Library/Mobile%20Documents/iCloud~md~obsidian/Documents/cpp-checker/gate.sh)
  - [gate_boj3s.sh](/Users/free_1/Library/Mobile%20Documents/iCloud~md~obsidian/Documents/cpp-checker/gate_boj3s.sh)
  - [hunt.sh](/Users/free_1/Library/Mobile%20Documents/iCloud~md~obsidian/Documents/cpp-checker/hunt.sh)
  - [suite_presets](/Users/free_1/Library/Mobile%20Documents/iCloud~md~obsidian/Documents/cpp-checker/suite_presets)

Any branch target must therefore:

- build its own branch-local solver
- run the repo-root certification harness against that solver
- keep all outputs inside the branch subtree

## Target Branches

- `branch_2_2`
  - active source: `branch_2_2/round45_resume/round45_branch_2_2_solver.cpp`
  - internal Round 45 profiling flow still exists via `build.sh`, `run.sh`, `smoke.sh`
  - outer suite standard should use the plain solver binary `round45_resume/solve`
- `branch_3`
  - active source: `branch_3/boj28350_resume/boj28350_branch_3_solver.cpp`
  - outer suite standard uses `boj28350_resume/solve`

## Standard Ladder

These are the levels Ouroboros should target, in order.

1. `smoke`
   - fast sanity correctness
2. `strong_gate`
   - correctness fuzz + hard scaling + max-N mix
3. `rebuttal_gate`
   - focused hard-mode rebuttal standard
4. `boj_3s_hard_gate`
   - BOJ-oriented strict performance standard
5. `hunt`
   - identify the currently slowest validated cases

## Thresholds That Matter

From the current presets:

- `strong_gate`
  - hard scaling: `alpha <= 1.45`
  - hard scaling: `ratio <= 2.90`
- `rebuttal_gate`
  - hard scaling: `alpha <= 1.45`
  - hard scaling: `ratio <= 2.90`
- `boj_3s_hard_gate`
  - strict scaling: `alpha <= 1.35`
  - strict scaling: `ratio <= 2.60`
  - large-case `case_sec_max <= 2.70`
  - large adversarial `sec_max <= 2.55`

Passing means the preset verdict is `PASS`, not merely “finished once”.

## Prepared Branch Commands

`branch_2_2`

- `./lca_smoke.sh`
- `./lca_strong_gate.sh`
- `./lca_rebuttal_gate.sh`
- `./lca_boj3s_gate.sh`
- `./lca_hunt.sh`

`branch_3`

- `./lca_smoke.sh`
- `./lca_strong_gate.sh`
- `./lca_rebuttal_gate.sh`
- `./lca_boj3s_gate.sh`
- `./lca_hunt.sh`

All of these wrappers are branch-local and write under:

- `branch_2_2/artifacts/lca_tree_stress_v5/...`
- `branch_3/artifacts/lca_tree_stress_v5/...`

The root harness now runs each case from its own branch-local `runs/...`
directory and sets `DENSE_PROFILE_OUTDIR` to that case directory. This keeps
solver-side profiling and auxiliary outputs inside the target branch tree.

## Done Criteria

A branch is “ready” for Ouroboros optimization only when:

- branch-local build command is stable
- outer suite wrappers exist and are path-stable
- artifacts stay inside the branch folder
- target standard is defined as preset-based `PASS`
- success means repeatable attainment of the same preset standard
