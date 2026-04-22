# lca_smoke Status Report

- Run id: `run.000019`
- Run started at UTC: `2026-04-12T01:36:39Z`
- Run finished at UTC: `2026-04-12T01:37:28Z`
- Run elapsed seconds: `49`
- Public status: `FAIL`
- Result family: `harness`
- Failure partition: `harness/setup`
- Normalized outcome: `harness_infrastructure_failure`
- Normalized exit code: `70`
- Raw exit code: `0`
- Outcome source: `inner_wrapper`
- Required standard: `lca_tree_stress_v5`
- Summary: `inner smoke wrapper returned success without publishing a fresh smoke bundle: stale smoke output root at /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke; stale suite config at /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/suite_config.txt; stale suite plan at /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/suite_plan.tsv`
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
- Run archive root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.000019`
- Run-archive source snapshot: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.000019/source_root_snapshot`
- Run-archive source snapshot manifest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.000019/source_failure_snapshot_manifest.tsv`
- Run archive manifest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.000019/artifact_manifest.tsv`
- Launcher console transcript: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.000019/console.stderr.txt`
- Dispatch result snapshot: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.000019/dispatch_result.txt`
- Run record json: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/run_record.json`
- Run comparison json: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/run_comparison.json`
- Source root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke`

## Acceptance Signal

- Acceptance status: `FAIL`
- Acceptance summary: `smoke did not satisfy AC2 because the launcher or smoke harness failed before acceptance evidence was trustworthy; keep later gates blocked and rerun smoke after repairing the wrapper path`

## Iteration Support

- Iteration support: `ACTIONABLE`
- Next step: `repair_then_retry`
- Iteration summary: `stable smoke failure status is published, but this run stopped before acceptance-grade evidence was trustworthy; repair the smoke launcher or harness path, then rerun ./lca_smoke.sh before later gates`
- Control action: `repair_and_rerun_smoke`
- Preferred next command: `./lca_smoke.sh`
- Command control mode: `smoke_repair_retry`
- Preferred command kind: `smoke_rerun`
- Failure terminal: `no`
- Gate escalation allowed: `no`
- Next gate command: `./lca_strong_gate.sh`
- Next gate status: `blocked_by_ac2`
- Next gate depends on: `AC2`
- Next gate summary: `strong gate is intentionally blocked until smoke publishes a fresh same-worktree pass`

## Failed Stage

- Failed stage scope: `inner_wrapper_bundle_validation`
- Failed stage: `bundle_validation`
- Stage label: `inner_wrapper_bundle_validation:bundle_validation`
- Primary summary: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/summary.txt`
- Primary report: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/latest_status_report.md`
- Primary manifest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/diagnostics_manifest.tsv`
- Iteration evidence: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/iteration_evidence.txt`
- Inspect first: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/summary.txt | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/latest_status_report.md | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/diagnostics_manifest.tsv`

## Standard Gap

- Status: `smoke_blocker_detected`
- Explanation: `inner smoke wrapper returned success without publishing a fresh smoke bundle: stale smoke output root at /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke; stale suite config at /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/suite_config.txt; stale suite plan at /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/suite_plan.tsv`
- Triage focus: `inspect /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/summary.txt | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/latest_status_report.md | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/diagnostics_manifest.tsv for the smoke failure at stage bundle_validation, then rerun ./lca_smoke.sh`

## Gate Chain

- AC2 (`./lca_smoke.sh`): status=`failed`; depends_on=`none`; summary=`smoke is the active blocker: inner smoke wrapper returned success without publishing a fresh smoke bundle: stale smoke output root at /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke; stale suite config at /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/suite_config.txt; stale suite plan at /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/suite_plan.tsv`
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

- Summary: `changed stage_label relative to previous run run.000020`
- Changed fields: `stage_label`
- Comparison artifact: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/run_comparison.json`
- Previous run id: `run.000020`
- Previous run archive root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.000020`
- Previous normalized outcome: `harness_infrastructure_failure`
- Previous stage label: `launcher_pre_dispatch:stale_inner_rerun_cleanup`
- Previous failure case: ``
- Previous status summary: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.000020/summary.txt`
- Previous iteration evidence: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.000020/iteration_evidence.txt`

## Diagnostics

- Iteration evidence: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/iteration_evidence.txt`
- Retry-loop control: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/retry_loop_control.json`
- Diagnostics manifest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/diagnostics_manifest.tsv`
- Run history index: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/history.tsv`
- Run record json: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/run_record.json`
- Run comparison json: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/run_comparison.json`
- Launcher console transcript: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_run_history/run.000019/console.stderr.txt`
- Suite config: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/suite_config.txt`
- Suite plan: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/suite_plan.tsv`
- Environment validation report: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/environment_validation.txt`
- Environment preflight manifest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/environment_validation/preflight_manifest.tsv`
- Environment setup snapshot: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/environment_validation/setup_env.txt`
- Build command snapshot: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/environment_validation/build.command.txt`
- Manifest snapshot: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/environment_validation/smoke_cases.snapshot.tsv`
- Dispatch timeout: `600`
- Next iteration anchor: start with `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/diagnostics_manifest.tsv`, `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/suite_config.txt`, `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/suite_plan.tsv`, and `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/environment_validation.txt` before escalating to the next gate.

## Retry Next

- Retry command: `./lca_smoke.sh`
- Guidance: `inspect /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/summary.txt | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/latest_status_report.md | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_latest_status/diagnostics_manifest.tsv for the smoke failure at stage bundle_validation, then rerun ./lca_smoke.sh`
- Retry-loop action: `repair_and_rerun_smoke`
- Preferred retry-loop command: `./lca_smoke.sh`
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
