# lca_smoke Status Report

- Run id: `run.000007`
- Run started at UTC: `2026-04-11T05:26:46Z`
- Run finished at UTC: `2026-04-11T05:29:56Z`
- Run elapsed seconds: `190`
- Public status: `FAIL`
- Result family: `harness`
- Failure partition: `harness/setup`
- Normalized outcome: `harness_infrastructure_failure`
- Normalized exit code: `70`
- Raw exit code: `130`
- Outcome source: `launcher`
- Required standard: `lca_tree_stress_v5`
- Summary: `launcher exited non-zero before inner wrapper dispatch`
- Working directory: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3`
- Original launch working directory: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3`
- Branch root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3`
- Artifacts root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5`
- Smoke output root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke`
- Smoke failure root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_failure`
- Launcher failure root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure`
- Status root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status`
- Run history root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history`
- Run history index: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/history.tsv`
- Run archive root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.000007`
- Run-archive source snapshot: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.000007/source_root_snapshot`
- Run-archive source snapshot manifest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.000007/source_failure_snapshot_manifest.tsv`
- Run archive manifest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.000007/artifact_manifest.tsv`
- Launcher console transcript: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.000007/console.stderr.txt`
- Run record json: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/run_record.json`
- Run comparison json: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/run_comparison.json`
- Source root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure`
- Source summary: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/failure_summary.txt`
- Source report: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/latest_failure_report.md`

## Failed Stage

- Failed stage scope: `launcher_pre_dispatch`
- Failed stage: `shell_entrypoint_validation`
- Stage label: `launcher_pre_dispatch:shell_entrypoint_validation`
- Primary summary: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/failure_summary.txt`
- Primary report: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/latest_failure_report.md`
- Primary manifest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/preflight_manifest.tsv`
- Iteration evidence: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/iteration_evidence.txt`
- Inspect first: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/failure_summary.txt | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/latest_failure_report.md | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/preflight_manifest.tsv | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/failure_reason.txt`

## Standard Gap

- Status: `smoke_blocker_detected`
- Explanation: `launcher exited non-zero before inner wrapper dispatch`
- Triage focus: `fix the launcher/preflight failure at stage shell_entrypoint_validation, inspect /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/failure_summary.txt | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/latest_failure_report.md | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/preflight_manifest.tsv | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/failure_reason.txt, then rerun ./lca_smoke.sh`

## Gate Chain

- AC2 (`./lca_smoke.sh`): status=`failed`; depends_on=`none`; summary=`smoke is the active blocker: launcher exited non-zero before inner wrapper dispatch`
- AC3 (`./lca_strong_gate.sh`): status=`blocked_by_ac2`; depends_on=`AC2`; summary=`strong gate is intentionally blocked until smoke publishes a fresh same-worktree pass`
- AC4 (`./lca_strong_gate.sh && ./lca_strong_gate.sh`): status=`blocked_by_ac2`; depends_on=`AC3`; summary=`strong-gate repeatability is intentionally blocked until AC3 has fresh same-worktree pass evidence`
- AC5 (`./lca_boj3s_gate.sh`): status=`blocked_by_ac2`; depends_on=`AC3`; summary=`boj3s gate is intentionally blocked until smoke and AC3 produce fresh same-worktree pass evidence`
- AC6 (`./lca_boj3s_gate.sh && ./lca_boj3s_gate.sh`): status=`blocked_by_ac2`; depends_on=`AC5`; summary=`boj3s repeatability is intentionally blocked until AC5 has fresh same-worktree pass evidence`
- Smoke summary mirror: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/summary.txt`
- Smoke report mirror: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/status_report.md`
- Smoke failure-report mirror: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/failure_report.md`
- Smoke iteration-evidence mirror: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/iteration_evidence.txt`
- Smoke retry-loop control mirror: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/retry_loop_control.json`
- Smoke diagnostics-manifest mirror: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/diagnostics_manifest.tsv`
- Smoke standard-gap json: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/standard_gap.json`
- Smoke run-record mirror: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/run_record.json`
- Smoke run-comparison mirror: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/run_comparison.json`

## Iteration Comparison

- Summary: `changed public_status,result_family,normalized_outcome,stage_label relative to previous run run.000006`
- Changed fields: `public_status,result_family,normalized_outcome,stage_label`
- Comparison artifact: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/run_comparison.json`
- Previous run id: `run.000006`
- Previous run archive root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.000006`
- Previous normalized outcome: `pass`
- Previous stage label: `completed:completed`
- Previous failure case: ``
- Previous status summary: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.000006/summary.txt`
- Previous iteration evidence: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.000006/iteration_evidence.txt`

## Diagnostics

- Iteration evidence: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/iteration_evidence.txt`
- Retry-loop control: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/retry_loop_control.json`
- Diagnostics manifest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/diagnostics_manifest.tsv`
- Run history index: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/history.tsv`
- Run record json: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/run_record.json`
- Run comparison json: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/run_comparison.json`
- Launcher console transcript: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.000007/console.stderr.txt`
- Suite config: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/suite_config.txt`
- Suite plan: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/suite_plan.tsv`
- Environment validation report: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/environment_validation.txt`
- Environment preflight manifest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/environment_validation/preflight_manifest.tsv`
- Environment setup snapshot: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/environment_validation/setup_env.txt`
- Build command snapshot: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/environment_validation/build.command.txt`
- Manifest snapshot: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/environment_validation/smoke_cases.snapshot.tsv`
- Next iteration anchor: start with `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/diagnostics_manifest.tsv`, `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/suite_config.txt`, `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/suite_plan.tsv`, and `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/environment_validation.txt` before escalating to the next gate.

## Retry Next

- Retry command: `./lca_smoke.sh`
- Guidance: `fix the launcher/preflight failure at stage shell_entrypoint_validation, inspect /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/failure_summary.txt | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/latest_failure_report.md | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/preflight_manifest.tsv | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/failure_reason.txt, then rerun ./lca_smoke.sh`
- Retry-loop action: `resume_progress40_retry_loop`
- Preferred retry-loop command: `cd /Users/free_1/Library/Mobile\ Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3 && zsh .ouroboros/launch_retry_loop.sh smoke_latest_status_retry_loop.log .ouroboros/seed_branch3_progress40_research_loop.yaml .ouroboros/seed_branch3_failure_analysis.yaml`
- Launch-helper retry-loop command: `cd /Users/free_1/Library/Mobile\ Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3 && zsh .ouroboros/launch_retry_loop.sh smoke_latest_status_retry_loop.log .ouroboros/seed_branch3_progress40_research_loop.yaml .ouroboros/seed_branch3_failure_analysis.yaml`
- Direct retry-loop command: `cd /Users/free_1/Library/Mobile\ Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3 && zsh .ouroboros/run_until_pass_progress40.sh .ouroboros/seed_branch3_progress40_research_loop.yaml .ouroboros/seed_branch3_failure_analysis.yaml`
- Retry-loop log path: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/smoke_latest_status_retry_loop.log`
- Retry-loop control json: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/retry_loop_control.json`

## Commands

Invocation command:

```bash
./lca_smoke.sh
```

Dispatch command:

```bash
/bin/bash /Users/free_1/Library/Mobile\ Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/outer_suite_wrappers/lca_smoke.sh
```
