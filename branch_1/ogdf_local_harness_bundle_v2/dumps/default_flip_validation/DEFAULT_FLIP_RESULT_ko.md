# Default Flip Validation Result

- default solver path: `rewrite-seq`
- hard compare: `6 / 6 green`
- random sanity:
  - `s1_r100`: green
  - `s1_r1000`: green
  - `seed 1..10 x 1000`: green
- total random compare passed: `11100`
- total random compare failed: `0`
- total oracle fail count: `0`
- total rewrite-seq fail count: `0`
- total oracle-vs-rewrite mismatch count: `0`
- total explicit mismatch count: `0`
- summary writer mode: `atomic`
- all summaries validated: `true`
- legacy path status: `diagnostic-only`
- direct solver smoke:
  - mode: `rewrite-seq`
  - seed: `1`
  - rounds: `10`
  - exit: `0`
  - summary artifact: `none`
  - log: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_1/ogdf_local_harness_bundle_v2/dumps/default_flip_validation/direct_solver_smoke/run.log`

## Notes

- `s1_r1000` appears twice in the aggregate because the validation plan intentionally ran a standalone `s1_r1000` and then included seed `1` again in the `seed 1..10 x 1000` loop.
- compare baseline standard remains `--baseline oracle --oracle-handoff normalize`.
- legacy solver path remains diagnostic-only via explicit `--mode rewrite-r`.
