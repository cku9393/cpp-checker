# Basis Family Freshization Scope Memo 90

## current note

This memo records the earlier basis/family freshization scope decision. It has since been superseded by later current-runtime work:

- exact minimal basis size `96` is now fresh current-runtime generated.
- family-chain theorem objects are now current constructor-backed.
- family-chain lower layers `52`, `53`, `55`, `quintuple_family_expansion_theorem_data_57`, and `sextuple_family_expansion_theorem_data_57` are now current constructor/cache-backed.
- the remaining family-chain lower-layer caveat is the final `high_family_expansion_theorem_data_57` aggregate row.

## target grouping

### A. basis_only_theorem_chain

- exact minimal basis size = `96`
- exact n=5 basis-only theorem
- bounded n=6, c<=5 basis-only theorem
- bounded n=7, c<=3 basis-only theorem

### B. family_chain_output_57

- bounded family-chain theorem
- family-chain self verification

### C. support8_shell15_frontier_output_84

- antecedent plus twelve frontier
- support8 antecedent15 frontier

## scope decision

- in-scope this round: `A`
- in-scope if cheap and grounded after A: `B`
- stretch goal only: `C`
- out of scope: shell16, higher-support expansion, BOJ solver

## grounded feasibility

### A. basis_only_theorem_chain

- `build_exact_theorem_audit_stats_()` had been reading imported closed-output audit counts only.
- `ensure_exact_minimal_basis_ready_()` already owns current-runtime exhaustive / bounded rerun paths for:
  - exact n=5
  - bounded n=6, c<=5
  - bounded n=7, c<=3
- therefore the three theorem-verification items are freshization candidates now.
- blocker for `exact minimal basis size = 96`:
  - `exact_minimal_proof_motif_basis_()` still installs `authoritative_exact_minimal_proof_motif_basis_data_48_()`
  - the current authoritative path does not yet replace that payload with a fresh minimal-basis derivation

### B. family_chain_output_57

- some slow rebuild paths exist for quintuple / sextuple / septuple / high-family expansions.
- `support_bounded_schema_universe_obstruction_theorem_()` and `unified_bounded_schema_universe_obstruction_theorem_()` are no longer the current authoritative theorem-object constructors.
- current authoritative family-chain theorem objects use `build_current_family_chain_output_57_theorem_objects_()`.
- lower-layer caveat remains below the fresh theorem object layer, currently starting at `septuple_family_expansion_theorem_data_57`.

### C. support8_shell15_frontier_output_84

- still closed-output frontier data from `84`
- not needed for this pilot
- do not touch unless A is stable and B is clearly blocked

## round objective

- preserve `support8_authoritative_completion_locked`
- move the three basis-only theorem verification items to a truthful fresh current-runtime path if reruns succeed
- leave `basis size = 96` and family-chain on validated-import status unless a real constructor appears
