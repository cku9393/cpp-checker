# AC3 Progress40 Investigation (2026-03-25)

- Scope: `AC3` prerequisite gate work for `./lca_strong_gate.sh`
- Active solver target at close: `boj28350_resume/boj28350_branch_3_solver.cpp` restored to the bundled `progress40` source line, with only the portable include shim retained
- Runtime env delta retained at close: `solver_release_env.sh` now defaults `ENABLE_LAYOUT_REUSE_ZERO_ELISION_OPT=1` and `ENABLE_LAYOUT_SIGNATURE_GATE_OPT=1`

## Formal failure observed

- A full `./lca_strong_gate.sh` run was launched on the pre-fix working tree.
- The run eventually returned `verdict=FAIL` after a long sequence of late-case stalls.
- The temporary workdir named in the wrapper output was `artifacts/lca_tree_stress_v5/.tmp/lca_strong_gate.run.FrjUPi`.
- That failing run did not publish a new `strong_gate/` artifact tree over the previous passing artifact set, so the stable published `artifacts/lca_tree_stress_v5/strong_gate/` directory still reflects the older PASS baseline rather than the new failed rerun.

## Isolated repro signal

- The strongest reproducible failing probe on the restored `progress40` baseline is:
  - `python3 branch_run_case.py comb_rect_dense 1024 1 0 0 boj28350_resume/solve ... --timeout 2.0`
- The generated input is byte-identical to the previously published passing artifact input at:
  - `artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed1_L0_Q0/in.txt`
- Isolated timings on the restored `progress40` baseline:
  - `comb_rect_dense n64 seed1_L0_Q0`: `0.133667s`
  - `comb_rect_dense n128 seed1_L0_Q0`: `0.204405s`
  - `comb_rect_dense n256 seed1_L0_Q0`: `0.865914s`
  - `comb_rect_dense n512 seed1_L0_Q0`: timeout at `2.0s`
  - `comb_rect_dense n1024 seed1_L0_Q0`: timeout at `2.0s`

## Failed retry lines in this session

- `progress40` bundle plus runtime layout-gate defaults enabled:
  - Improved branch alignment, but still timed out on the isolated `comb_rect_dense` ladder above.
- One-time initial DFS/low-link artifact seeding for owner partitions:
  - Regressed even the `n64` probe and was discarded.
- `progress39` archived source probe:
  - Timed out even on `n64`, so it was also discarded.

## Current best interpretation

- The branch-local published PASS artifact for `strong_gate` does not currently reproduce from the available `progress40` archive snapshot under the current branch build/runtime path.
- The current blocker is not generator drift. The new repro input exactly matches the older passing artifact input.
- The remaining gap is therefore in the executable path itself: either a missing uncommitted progress40-derived optimization layer that was previously present in the working tree, or a branch-local runtime/build difference not yet recovered from the bundled materials.

## Next useful retry axis

- Primary axis: recover the missing progress40-derived optimizer layer that existed in the prior branch-local working tree but is not present in the raw `progress40` archive snapshot.
- Secondary axis: audit the branch-local runtime/build path against the older passing artifact workflow, focusing on executable path and feature-toggle parity rather than on the generator.
