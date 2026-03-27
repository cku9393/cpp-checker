# Next Probe Result

- Command: `LCA_STAGE_FILTER=correctness_fuzz ./lca_strong_gate.sh`
- Primary axis: `zero_span_fastpath`
- Secondary axis: `watch_diff`
- Why this axis: `Selected `zero_span_fastpath` as the primary progress40 axis because the latest `strong_gate_unspecified` failure stayed in the `correctness-proof` lane and the bundled summary still names `zero-span eligibility and fastpath commit` as the safest next pivot; `watch_diff` remains a secondary cross-check axis only because the newer evidence narrows work inside the same pivot instead of proving an unrelated axis shift. Do not broaden into other progress40 axes unless later solver/runtime/profile evidence contradicts this baseline.`
- Exit code: `124`
- Timed out: `yes`
- Elapsed seconds: `1.003`

- Stdout log: `branch_3/artifacts/lca_tree_stress_v5/retry_loop/attempt_001_20260325_183441/next_probe.stdout.log`
- Stderr log: `branch_3/artifacts/lca_tree_stress_v5/retry_loop/attempt_001_20260325_183441/next_probe.stderr.log`
