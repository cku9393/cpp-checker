# Failure Analysis Iteration Ledger

- Timestamp: `2026-03-26 06:35:38 KST`
- Failed attempt: `attempt_001`
- Analysis round: `1`
- Analysis log: `artifacts/lca_tree_stress_v5/retry_loop/attempt_001_20260326_002918/analysis_workflow_round_01.log`
- Current for latest failure: `yes`
- Current failure session: `orch_1736140fd0df`
- Current failure execution: `exec_19e8677de11e`
- Current failure timestamp: `2026-03-26 05:46:51 KST`
- Current failure failed ACs: `4, 6`
- Current failure signature: `attempt_001|orch_1736140fd0df|2026-03-26 05:46:51 KST|4,6`
- Primary axis: `zero_span_fastpath`
- Secondary axis: `state_materialization`
- Pinned ACs: `4`
- Pinned paths: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/lca_smoke_target.sh, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifact_paths.py, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/solver_release_env.sh, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/build.sh`
- Pinned symbols: `function artifacts_root [34-35], function ensure_under_artifacts [38-39], function branch_tmp_root [42-43], function configure_branch_process_env [46-71], function __solver_release_env_keep_or_set [41-112], function _read_tokens [9-18]`
- Failure families: `strong_gate_unspecified`
- Next probe command: `LCA_STAGE_FILTER=correctness_fuzz ./lca_strong_gate.sh`
- Why this axis: `Selected `zero_span_fastpath` as the primary progress40 axis because the latest `strong_gate_unspecified` failure stayed in the `correctness-proof` lane and the bundled summary still names `zero-span eligibility and fastpath commit` as the safest next pivot; `state_materialization` remains a secondary cross-check axis only because the newer evidence narrows work inside the same pivot instead of proving an unrelated axis shift. Do not broaden into other progress40 axes unless later solver/runtime/profile evidence contradicts this baseline.`
- Next narrowing target: `function artifacts_root [34-35], function ensure_under_artifacts [38-39]`

## Repeat Signal Summary
Primary axis `zero_span_fastpath` recurred 0 times; current failure families recurred 0 times in prior captured failures.

## Refreshed Assets
- `.ouroboros/failure_analysis_state.json`
- `.ouroboros/failure_analysis_iteration.md`

## Retry Gate Requirement
- The next solver retry must stay blocked unless `.ouroboros/failure_analysis_state.json` still carries this exact current-failure signature.
