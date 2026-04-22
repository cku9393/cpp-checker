# Family-Chain Obstruction Preservation Skeleton 90

## lemma

`obstruction_preservation_or_higher_support_escape`.

## exact statement

For a recognized family-chain source-form witness processed by the refined lift map, the source and target obstruction payloads are well-defined and compared by the phase2 `layerwise_payload_refinement` relation. If layerwise refinement, canonical compatibility, and counterexample-status preservation are established, the lifted target object is absorbed by the current bounded family-chain theorem and self verification. If counterexample status is not preserved, a smaller witness must be constructed under the support measure. If neither preservation nor reduction is established, the case remains a named payload-preservation blocker or higher-support escape.

## proof outline

1. Start from a recognized family-chain source form.
2. Use the refined lift map to construct a target family-chain object.
3. Establish source and target payload well-definedness.
4. Apply the phase2 payload refinement relation and source-target correspondence table.
5. Separate relation well-definedness from semantic layerwise preservation.
6. If payload refinement is semantically preserved, apply bounded family-chain theorem and self verification.
7. If status is not preserved, require a smaller-witness construction under the support measure.
8. If neither preservation nor reduction is proved, route to named escape/blocker.

## final status

`partial_preservation_phase2_relation_ready_status_open`.

This is not full family-chain lift correctness and not a full general theorem.

Runtime skeleton: `branch_4/90/runtime/family_chain_obstruction_preservation_skeleton_90.tsv`.
