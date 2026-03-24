# INTEGRATION FINALIZE RESULT

- default flip status: green
- hard compare status: green
- direct solver smoke status: green
- random sanity status: green
- oracle-vs-rewrite mismatch count: 0
- all summaries validated: true
- legacy path status: diagnostic-only

## Validation

- hard compare: 6 passed / 0 failed
- random sanity s1_r100: 100 passed / 0 failed
- direct solver smoke: `--mode rewrite-seq --seed 1 --rounds 10` exit code 0

## Packaging

- release manifest: `release/rewrite_seq_release_manifest.json`
- finalize doc: `docs/rewrite_seq_integration_finalize_ko.md`
- policy doc: `docs/rewrite_seq_default_flip_policy_ko.md`
- regression policy: `docs/rewrite_seq_regression_policy_ko.md`
