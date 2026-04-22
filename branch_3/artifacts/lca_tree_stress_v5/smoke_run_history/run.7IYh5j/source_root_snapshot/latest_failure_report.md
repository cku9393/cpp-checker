# lca_smoke Launcher Failure Report

- Stage: `dispatch_monitor`
- Exit code: `70`
- Failure kind: `launcher_preflight_failure`
- Failure origin: `launcher`
- Message: `inner wrapper dispatch monitor failed with exit code 1`
- Working directory: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3`
- Original launch working directory: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3`
- Branch root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3`
- Artifacts root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5`
- Failure root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure`
- Run archive root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.7IYh5j`
- Run archive manifest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.7IYh5j/artifact_manifest.tsv`
- Launcher console transcript: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.7IYh5j/console.stderr.txt`
- Inner wrapper: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/outer_suite_wrappers/lca_smoke.sh`
- Build wrapper: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/build.sh`
- Smoke target wrapper: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/lca_smoke_target.sh`
- Artifact resolver: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifact_paths.py`

## Failed Stage

- Failed stage scope: `launcher_pre_dispatch`
- Failed stage: `dispatch_monitor`
- Primary summary: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/failure_summary.txt`
- Primary report: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/latest_failure_report.md`
- Primary manifest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/preflight_manifest.tsv`
- Inspect first: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/failure_summary.txt | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/latest_failure_report.md | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/preflight_manifest.tsv | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/failure_reason.txt`

## Recorded Artifacts

- Failure reason: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/failure_reason.txt`
- Invocation command: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/invocation_command.txt`
- Dispatch command: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/dispatch_command.txt`
- Rerun command snapshot: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/rerun_command.txt`
- Launcher env snapshot: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/launcher_env.txt`
- Preflight manifest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/preflight_manifest.tsv`
- Artifact manifest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/artifact_manifest.tsv`
- Failure summary: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/failure_summary.txt`

## Last Recorded Check

- Kind: `dispatch_monitor`
- Label: `inner wrapper dispatch monitor`
- Status: `broken`
- Detail: `1`
- Artifact: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/.tmp/lca_smoke.launcher.tmp/preflight/dispatch_result.txt`

## Commands

Invocation command:

```bash
./lca_smoke.sh
```

Intended inner-wrapper dispatch command:

```bash
/bin/bash /Users/free_1/Library/Mobile\ Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/outer_suite_wrappers/lca_smoke.sh
```

## Retry Next

- Retry command: `./lca_smoke.sh`
- Guidance: `fix the launcher/preflight failure at stage dispatch_monitor, inspect /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/failure_summary.txt | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/latest_failure_report.md | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/preflight_manifest.tsv | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/failure_reason.txt, then rerun ./lca_smoke.sh`
