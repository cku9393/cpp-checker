# Family-Chain Lift Map Refinement Phase2 Scope Memo 90

## purpose

This memo fixes the phase2 proof-attempt target after `family_chain_lift_obstruction_preservation_proof`.

The round is not a full general theorem proof. It only concerns recognized family-chain source forms already accepted by the source-form recognizer and processed by the refined lift map.

## selected phase2 target

Selected statement:

`payload_refinement_or_higher_support_escape`

## exact selected statement

For a recognized family-chain source-form witness, the source obstruction payload and the lifted target family-chain payload are compared by an explicit payload refinement relation. If the layerwise correspondence and canonicalization obligations prove that refinement, the refinement is usable for obstruction preservation. If counterexample status is not preserved directly, a smaller-witness construction must be supplied under the support measure. If refinement or smaller-witness construction cannot be proved, the case remains a named blocker or higher-support escape.

## non-claims

- This does not prove arbitrary support-growth lift correctness.
- This does not prove full family-chain lift correctness.
- This does not prove support8 sufficiency outside the selected support-bound skeleton.
- This does not prove the full general theorem.

Runtime inventory: `branch_4/90/runtime/family_chain_lift_map_refinement_phase2_scope_inventory_90.tsv`.
