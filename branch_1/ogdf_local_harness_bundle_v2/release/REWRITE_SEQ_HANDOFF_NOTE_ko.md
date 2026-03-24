# REWRITE SEQ HANDOFF NOTE

- 현재 release status = ship-ready evidence green, tag pending
- default solver path = rewrite-seq
- legacy path = diagnostic-only
- hard compare / random sanity / direct smoke 결과 = green / green / green
- release manifest path = `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_1/ogdf_local_harness_bundle_v2/release/rewrite_seq_release_manifest.json`
- release delivery bundle path = `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_1/ogdf_local_harness_bundle_v2/artifacts/rewrite_seq_release_delivery_20260324_103121.zip`
- checksum path = `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_1/ogdf_local_harness_bundle_v2/release/rewrite_seq_release_delivery.sha256`
- git tag = blocked (dirty worktree; HEAD commit `ad0399141f8e2948e304cae645f94d2fb72550ef` does not capture current handoff artifacts)
- operator runbook path = `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_1/ogdf_local_harness_bundle_v2/release/rewrite_seq_operator_runbook_ko.md`
- regression policy path = `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_1/ogdf_local_harness_bundle_v2/docs/rewrite_seq_regression_policy_ko.md`

## Rollback
- direct legacy diagnostic: `./build/rewrite_r_harness --backend ogdf --mode rewrite-r --seed 1 --rounds 10 --dump-dir dumps/legacy_diagnostic_smoke`
- compare diagnostic: `./build/rewrite_r_harness --backend ogdf --mode solver-compare --manifest regressions/rewrite_seq_cases.json --baseline legacy --dump-dir dumps/legacy_compare_diagnostic`

## Gate Summary
- release gate status = green
- hard compare status = green
- random sanity status = green
- direct solver smoke status = green
- all summaries validated = true

## Blocking Issue
- earliest issue: dirty worktree prevents a safe annotated tag on `main`.
