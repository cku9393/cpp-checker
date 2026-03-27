# AC8 Artifact Locality Check

- Scope: direct `./run.sh` execution, including the `BRANCH3_SOLVER` override path that can bypass the branch-local launcher.
- Validation command: `BRANCH3_SOLVER=/usr/bin/env ./run.sh > artifacts/lca_tree_stress_v5/.tmp/ac8_env.txt`
- Result: the launcher-exported runtime paths stayed under `branch_3/artifacts/...`.
- Result: a post-run `find` scan reported no files outside `./artifacts` newer than the validation marker.

Observed routed paths from `env_dump.txt`:

- `DENSE_PROFILE_OUTDIR=/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/boj28350_resume/direct_solver_aux`
- `TMPDIR=/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/.tmp`
- `TMP=/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/.tmp`
- `TEMP=/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/.tmp`
- `HOME=/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/.tmp/home`
- `XDG_CONFIG_HOME=/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/.tmp/xdg_config`
- `XDG_CACHE_HOME=/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/.tmp/xdg_cache`
- `XDG_STATE_HOME=/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/.tmp/xdg_state`
- `PYTHONPYCACHEPREFIX=/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/.tmp/pycache`
