# AC8 locality probe

## Shared guard change
- Added runtime artifact-root assertions to `solver_release_env.sh` for `BRANCH_ARTIFACT_TMP_ROOT`, `TMPDIR`, `TMP`, `TEMP`, `HOME`, `XDG_CONFIG_HOME`, `XDG_CACHE_HOME`, `XDG_STATE_HOME`, and `PYTHONPYCACHEPREFIX`.

## Validation
- `source ./solver_release_env.sh` resolved every routed path under `artifacts/lca_tree_stress_v5/.tmp/...`.
- `python3 branch_gen_case.py --mode chain_unary --n 8 --seed 1 ...` wrote fresh outputs under `artifacts/lca_tree_stress_v5/ac8_locality_probe/gen_case/`.
- A post-command `find . -path './artifacts' -prune -o -type f -newer <marker> -print` returned no files, so no newly generated files escaped `artifacts/`.

## Notes
- `./build.sh` still fails in this environment because the available compiler cannot find `bits/stdc++.h`; the resulting stderr was kept under `artifacts/lca_tree_stress_v5/ac8_locality_probe/build_postpatch.stderr.txt`.
