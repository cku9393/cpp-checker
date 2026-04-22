This preserved failure bundle was restored from the saved in-tree fixtures under
`artifacts/lca_tree_stress_v5/retry_loop/ac3_timeout_regression_runs_v2/`.

Purpose:
- Keep `ac3_timeout_regression_cases.tsv` pointing at real branch-local case directories.
- Preserve the strong-gate retry-loop path expected by the regression manifests and tests.
- Keep all reproducer data under `branch_3/artifacts/...` with no dependency on vanished live run trees.
