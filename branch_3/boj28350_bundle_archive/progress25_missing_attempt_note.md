# Progress25 missing release attempt note

This attempt rebuilt the stress suite and `p25_release`, and restarted the missing release batch from `run_progress25_missing.py`.

Observed state before stopping the batch for consistency:

- `after_both_on_dense_1024_release` solver process was still running after about 15 minutes of wall clock.
- No completed `result.json` row was produced yet for that rerun.
- Therefore no new authoritative release or representative rows were committed into the progress25 package in this attempt.

Current authoritative package remains the last updated partial package:

- `boj28350_literature_progress25_carry_hit_apply_cursor_advance.cpp`
- `boj28350_progress25_carry_hit_apply_cursor_advance_report.md`
- `boj28350_progress25_results_merged.json`

Current clean LOCAL conclusion remains:

`next pivot after carry-hit round: prev-state writeback and scalar store`
