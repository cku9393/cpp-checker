# LCA Tree Stress Workspace

This directory is the standalone outer workspace for the current
`lca_tree_stress_v5` solver line. It no longer shares its runtime artifacts
with `branch_2_2`.

The source file here is this workspace's own active solver:

- `lca_tree_stress_v5/lca_tree_stress_v5_solver.cpp`

The outer stress suite line itself predates `branch_2_2`.

This `lca_tree_stress_v5/` subdirectory was added later during workspace
reorganization so outer-suite-focused work could live in its own subtree.

The shared outer-suite harness files that previously sat at repo root now live
under `lca_tree_stress_v5/tooling/`.

A Round 45 hold bundle also carried its own `lca_tree_stress_v5/` tree, but
this workspace should be treated as the repository's outer-suite line, not as a
`branch_2_2`-derived workspace.

`branch_2_2` keeps its own Round 45 resume workspace under
`branch_2_2/round45_resume/`. Work for the outer `lca_tree_stress_v5` line
should happen only in this directory.

Execution model:

- `./lca_tree_stress_v5/build.sh` compiles `lca_tree_stress_v5/lca_tree_stress_v5_solver.cpp`
- `./lca_tree_stress_v5/build.sh` delegates to `lca_tree_stress_v5/tooling/build.sh`
- `./lca_tree_stress_v5/smoke.sh` runs `lca_tree_stress_v5/solve`
- `./lca_tree_stress_v5/smoke.sh` delegates case execution to `lca_tree_stress_v5/tooling/run_case.sh`
- outputs go under `lca_tree_stress_v5/artifacts/smoke/...`
- this workspace does not read `branch_2_2/round45_resume/round45_branch_2_2_solver.cpp`

Shared harness entrypoints:

- `lca_tree_stress_v5/tooling/build.sh`
- `lca_tree_stress_v5/tooling/run_case.sh`
- `lca_tree_stress_v5/tooling/gate.sh`
- `lca_tree_stress_v5/tooling/gate_boj3s.sh`
- `lca_tree_stress_v5/tooling/hunt.sh`
- `lca_tree_stress_v5/tooling/suite_presets/*.json`

## Build

```bash
./lca_tree_stress_v5/build.sh
```

This builds:

- `lca_tree_stress_v5/solve`

## Smoke

```bash
./lca_tree_stress_v5/smoke.sh
```

This writes outputs under:

- `lca_tree_stress_v5/artifacts/smoke/smoke_comb_dense_256_s1`
- `lca_tree_stress_v5/artifacts/smoke/smoke_comb_dense_1024_s1`

## Immediate next task

Round 45 did not stop on optimization quality. It stopped because the profiler
hook contract was missing from the baseline source. The next source change
should restore the missing row-emission hooks before any separator-prefilter
optimization is attempted.
