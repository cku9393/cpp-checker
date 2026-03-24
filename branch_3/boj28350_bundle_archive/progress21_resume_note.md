# Progress21 release rerun resume note

The environment reset again while authoritative RELEASE reruns were in progress.

Still missing at authoritative quality:
- `both_on, comb_rect_dense 1024 RELEASE` clean rerun
- dense 1024 repeat stability
- clean 4096 representative for dense
- clean 4096 representative for multi

Recommended restart point:
1. Use `boj28350_literature_progress21_transition_state_branch_state_load_core.cpp` as the source of truth.
2. Run `progress21_resume_release_runs.sh` to rebuild the stress suite, rebuild the release binary, restore the heartbeat runner, and execute the missing release matrix.
3. Merge those fresh result.json files into the existing `boj28350_progress21_results_merged.json` and update the report.

Current clean LOCAL conclusion remains:
`next pivot after branch-state round: state load materialization collapse`
