# AC2 Smoke Stabilization Report

- Timestamp: `2026-03-26 11:48 KST`
- Criterion: `./lca_smoke.sh is stabilized enough to support further iteration`
- Targeted failure mode: successful `lca_smoke` runs left branch-local success-path residue under `artifacts/lca_tree_stress_v5/` (`smoke_setup`, `.tmp/lca_smoke.session`, `.locks/lca_smoke`), which polluted follow-up iteration state even after a nominal PASS.
- Changed file: `outer_suite_wrappers/lca_smoke.sh`

## Change

Added an eager `cleanup_success_state()` step on the successful publish path so `lca_smoke` removes its setup/session/tmp residue before returning, instead of depending only on the EXIT trap.

## Validation

- `./lca_smoke.sh` -> exit `0`
- post-run residue check for `smoke_setup`, `.tmp/lca_smoke.session`, `.locks/*` -> no paths
- `./lca_smoke_repeatability.sh 3` -> exit `0`
- `artifacts/lca_tree_stress_v5/smoke_repeatability/summary.txt` -> `status=PASS`, `requested_runs=3`, `completed_runs=3`
- `artifacts/lca_tree_stress_v5/smoke_repeatability/runs/run01/exit_code.txt` -> `0`
- `artifacts/lca_tree_stress_v5/smoke_repeatability/runs/run02/exit_code.txt` -> `0`
- `artifacts/lca_tree_stress_v5/smoke_repeatability/runs/run03/exit_code.txt` -> `0`
- post-repeatability residue check for `smoke_setup`, `.tmp/lca_smoke.session`, `.locks/*`, `.repeatability_stage` -> no paths

## Outcome

`lca_smoke` now leaves a clean branch-local state after successful runs and survives the documented three-run smoke repeatability workflow on the same working tree without manual cleanup.
