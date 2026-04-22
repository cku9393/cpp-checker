# Shell16 Dependency Map 90

## summary

Shell16 should reuse the shell15 frontier pattern, but it needs its own first-class paths, cache names, audit rows, and promotion guard. The current code has a shell16 optional ledger in the support8 tail obstruction chain, but it is explicitly not attempted.

## shell15 reference pattern

- Spec builder: `build_antecedent_shell15_frontier_specs_`
- Candidate enumerator: `enumerate_antecedent_shell15_frontier_candidates_`
- Pass1 builder: `scan_antecedent_shell15_frontier_`
- Pass2/pass3 cache loader: `load_current_shell15_frontier_runtime_artifact_`
- Runtime audit writer: `write_shell15_frontier_generation_audit_files_`
- Imported source: `support8_shell15_frontier_output_84`, retained only as fallback/equality oracle for shell15

## shell16 needed pattern

Shell16 should have an independent spec builder, candidate-universe artifact, local-exact survivor audit, plus-one survivor audit, theorem-preserving survivor audit, rowset equality audit, constructor fingerprint, and cache-load path. Reusing shell15 artifacts as shell16 results is forbidden.

## tail escape integration

The `shell16_escape` row in the minimal-counterexample and tail bridge audits maps to the first unchecked extension boundary after checked range `9..15`. This preflight map turns that escape into a concrete future `shell16_attempt`.

