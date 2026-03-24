# RELEASE GATE RESULT

- release gate status: green
- hard compare status: green
- random sanity status: green
- direct solver smoke status: green
- oracle-vs-rewrite mismatch count: 0
- all summaries validated: true
- legacy path status: diagnostic-only

## Validation

- hard compare: 6 passed / 0 failed
- random sanity s1_r100: 100 passed / 0 failed
- direct solver smoke: `--mode rewrite-seq --seed 1 --rounds 10`

## Artifact Paths

- hard compare summary: `dumps/release_gate_test_manual/hard_compare/summary.json`
- random sanity summary: `dumps/release_gate_test_manual/random_sanity_s1_r100/summary.json`
- direct solver smoke log: `dumps/release_gate_test_manual/direct_solver_smoke/run.log`
