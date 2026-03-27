# Next Probe Result

- Command: `LCA_STAGE_FILTER=correctness_fuzz ./lca_strong_gate.sh`
- Primary axis: `zero_span_fastpath`
- Secondary axis: `state_materialization`
- Why this axis: `Selected `zero_span_fastpath` as the primary progress40 axis because the latest `strong_gate_unspecified` failure stayed in the `correctness-proof` lane and the bundled summary still names `zero-span eligibility and fastpath commit` as the safest next pivot; `state_materialization` remains a secondary cross-check axis only.`
- Exit code: `1`
- Timed out: `no`
- Elapsed seconds: `0.465`

- Stdout log: `branch_3/artifacts/lca_tree_stress_v5/retry_loop/attempt_001_20260325_123707/next_probe.stdout.log`
- Stderr log: `branch_3/artifacts/lca_tree_stress_v5/retry_loop/attempt_001_20260325_123707/next_probe.stderr.log`
