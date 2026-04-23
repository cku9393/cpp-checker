# Family-Chain Lift Phase3 Need Assessment 90

## purpose

This assessment decides whether `family_chain_lift_phase3_if_needed` is actually needed after phase2.

## result

Phase3 is needed, but its target is restricted.

Selected phase3 target:

`layer_projection_payload_preservation_and_canonical_lift_soundness`

Phase3 does not attempt to prove all remaining blockers at once. It targets the two blockers that directly sit between the phase2 correspondence contract and semantic payload preservation:

- layer projection payload preservation;
- canonical lift soundness.

Counterexample-status preservation and operation-specific smaller-witness construction remain separate obligations. The next round should move to `prove_support_reduction_operation_sublemma` unless phase3 discovers a narrower phase4 semantic-preservation target.

Runtime assessment: `branch_4/90/runtime/family_chain_lift_phase3_need_assessment_90.tsv`.
