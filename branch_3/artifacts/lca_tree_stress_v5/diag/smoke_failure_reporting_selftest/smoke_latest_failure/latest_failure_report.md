# lca_smoke Failure Report

- Exit code: `124`
- Case tag: `smoke_comb_core_64_s7`
- Stage: `smoke`
- Mode: `comb_core`
- n: `64`
- Seed: `7`
- Shuffle labels: `1`
- Shuffle queries: `1`
- Timeout (s): `2`
- Manifest row: `smoke	comb_core	64	7	1	1	2`
- Failure root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure`
- Failure case dir: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/smoke_comb_core_64_s7`
- Smoke output root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke`
- Smoke manifest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/boj28350_resume/smoke_cases.tsv`
- Helper stdout: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/smoke_comb_core_64_s7/run_case.stdout.txt`
- Helper stderr: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/smoke_comb_core_64_s7/run_case.stderr.txt`
- Frozen solver snapshot: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/solver_snapshot`
- Failed row snapshot: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/failed_case_row.tsv`
- Smoke manifest snapshot: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/smoke_cases_manifest.tsv`
- Runtime env snapshot: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/runtime_env.txt`
- Runtime env exports: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/runtime_env_exports.sh`
- Artifact manifest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/artifact_manifest.tsv`
- Seed repro script: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/repro_from_seed.sh`
- Preserved-input replay script: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/replay_preserved_input.sh`

## Commands

Executed command:

```bash
python3 branch_run_case.py comb_core 64 7 1 1 ./boj28350_resume/solve /tmp/fake --timeout 2
```

Preferred seed repro invocation:

```bash
bash /Users/free_1/Library/Mobile\ Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/repro_from_seed.sh
```

Raw seed repro command body recorded for low-level debugging:

```bash
python3 /Users/free_1/Library/Mobile\ Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_run_case.py comb_core 64 7 1 1 /Users/free_1/Library/Mobile\ Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/solver_snapshot /Users/free_1/Library/Mobile\ Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/repro_from_seed/smoke_comb_core_64_s7 --timeout 2 --env DENSE_SHADOW_CASE_MODE=comb_core --env DENSE_SHADOW_CASE_N=64 --env DENSE_SHADOW_CASE_SEED=7 --env DENSE_PROFILE_OUTDIR=/Users/free_1/Library/Mobile\ Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/repro_from_seed/smoke_comb_core_64_s7 --env DENSE_DECOMPOSESERIES_ROUND45_SHADOWCHECK=1
```

Preferred preserved-input replay invocation:

```bash
bash /Users/free_1/Library/Mobile\ Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/replay_preserved_input.sh
```

Raw preserved-input replay command body recorded for low-level debugging:

```bash
mkdir -p /Users/free_1/Library/Mobile\ Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/replay_from_input/smoke_comb_core_64_s7 && env DENSE_SHADOW_CASE_MODE=comb_core DENSE_SHADOW_CASE_N=64 DENSE_SHADOW_CASE_SEED=7 DENSE_PROFILE_OUTDIR=/Users/free_1/Library/Mobile\ Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/replay_from_input/smoke_comb_core_64_s7 DENSE_DECOMPOSESERIES_ROUND45_SHADOWCHECK=1 /Users/free_1/Library/Mobile\ Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/solver_snapshot < /Users/free_1/Library/Mobile\ Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/smoke_comb_core_64_s7/in.txt > /Users/free_1/Library/Mobile\ Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/replay_from_input/smoke_comb_core_64_s7/out.txt 2> /Users/free_1/Library/Mobile\ Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/replay_from_input/smoke_comb_core_64_s7/solver_stderr.txt
```

## Artifact Paths

- input: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/smoke_comb_core_64_s7/in.txt`
- meta: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/smoke_comb_core_64_s7/meta.json`
- hidden parent: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/smoke_comb_core_64_s7/hidden_parent.txt`
- solver output: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/smoke_comb_core_64_s7/out.txt`
- timing: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/smoke_comb_core_64_s7/time.txt`
- solver stderr: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/smoke_comb_core_64_s7/solver_stderr.txt`
- helper stdout: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/smoke_comb_core_64_s7/run_case.stdout.txt`
- helper stderr: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/smoke_comb_core_64_s7/run_case.stderr.txt`
- artifact manifest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/artifact_manifest.tsv`

## Preserved Debug Bundle

- `solver_snapshot` freezes the exact failing binary.
- `runtime_env_exports.sh` restores the branch-local release env that the failure used.
- `repro_from_seed.sh` regenerates the same seed into `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/smoke_failure_reporting_selftest/smoke_latest_failure/repro_from_seed/smoke_comb_core_64_s7` without overwriting the preserved failure tree.
- `replay_preserved_input.sh` reruns the frozen solver directly on the preserved `in.txt` without regenerating the case.
- `artifact_manifest.tsv` records existence, size, and SHA-256 for every preserved debug artifact.

## Timing Artifact

```text
0.010000 1024
```

## Solver stderr tail

```text
[release_diag] synthetic stderr
```

## Helper stderr tail

```text
[run_case] solver timed out after 2s
```

## Helper stdout tail

```text
[run_case] synthetic stdout
```
