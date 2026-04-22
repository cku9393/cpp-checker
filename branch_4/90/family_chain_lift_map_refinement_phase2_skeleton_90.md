# Family-Chain Lift Map Refinement Phase2 Skeleton 90

## lemma

`payload_refinement_or_higher_support_escape`.

## exact statement

For a recognized family-chain source-form witness, the source obstruction payload and lifted family-chain target payload are compared by the phase2 `layerwise_payload_refinement` relation. The relation and recognized-source correspondence are current-scope well-defined. If layerwise refinement and canonical compatibility are later proved, obstruction preservation follows conditionally. If counterexample status is not preserved, a smaller witness must be constructed under the lexicographic support measure. If refinement or smaller-witness construction is not proved, the case remains a named blocker or higher-support escape.

## proof outline

1. Start from a recognized family-chain source-form witness.
2. Use the refined lift map to construct source and target payload records.
3. Apply the phase2 payload refinement relation.
4. Use the source-target correspondence table to split by layer.
5. For exact equality, reduce to canonical commutation.
6. For layerwise refinement, require layer projection payload-preservation sublemmas.
7. For obstruction preservation, use refinement as a conditional semantic bridge.
8. For counterexample status preservation, require a status preservation theorem.
9. If status is not preserved, attempt smaller-witness construction.
10. If no construction is available, route to named escape/blocker.

## final status

`partial_phase2_proved_escape_open`.

This is proof progress on recognized-source payload refinement only. It is not full lift correctness and not a full general theorem.

Runtime skeleton: `branch_4/90/runtime/family_chain_lift_map_refinement_phase2_skeleton_90.tsv`.
