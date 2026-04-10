# Basis Family Freshization Scope Memo 90

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
- however `support_bounded_schema_universe_obstruction_theorem_()` and `unified_bounded_schema_universe_obstruction_theorem_()` are still imported theorem constructors.
- therefore family-chain self verification can remain current verified, but not fresh-current-generated, until those theorem objects themselves have a genuine current constructor.

### C. support8_shell15_frontier_output_84

- still closed-output frontier data from `84`
- not needed for this pilot
- do not touch unless A is stable and B is clearly blocked

## round objective

- preserve `support8_authoritative_completion_locked`
- move the three basis-only theorem verification items to a truthful fresh current-runtime path if reruns succeed
- leave `basis size = 96` and family-chain on validated-import status unless a real constructor appears
