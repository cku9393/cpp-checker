# Family-Chain Lift Map Refinement Skeleton 90

## lemma

`lift_map_refinement_or_higher_support_escape`, updated by `obstruction_preservation_or_higher_support_escape`.

## exact statement

For a support `>8` witness in the family-chain branch of `support_growth_partition`, the source-form recognizer either accepts the witness as a recognized source form or routes it to a named escape. For recognized source forms, the refined lift map is defined, the source/target obstruction payloads are well-defined, and phase2 defines a `layerwise_payload_refinement` relation plus a layerwise source-target correspondence table. If semantic layerwise refinement and counterexample-status preservation are later proved, the current bounded family-chain theorem absorbs the lifted target object.

## current status

`partial_refinement_phase2_relation_ready_status_open`.

What is now current-scope proved:

- recognizer/classification contract;
- refined lift totality for recognized source forms;
- source and target payload well-definedness for recognized lifts;
- phase2 payload refinement relation well-definedness;
- recognized source-target payload correspondence totality;
- conditional support-measure decrease after an actual smaller witness is constructed;
- conditional target theorem absorption after valid preserved lift;
- named escape routing for preservation failure.

Still open:

- semantic layerwise payload refinement;
- canonical lift soundness;
- counterexample-status preservation;
- operation-specific smaller-witness construction when status is not preserved.

Runtime skeleton: `branch_4/90/runtime/family_chain_lift_map_refinement_skeleton_90.tsv`.
