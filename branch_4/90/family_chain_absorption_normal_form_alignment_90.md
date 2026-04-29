# Family Chain Absorption Normal Form Alignment 90

Final status: `family_chain_absorption_normal_form_alignment_proof_ready_source_target_normal_form_open`.

Normal-form alignment is kept separate from payload and status-domain alignment. A target normal form does not automatically certify a valid source witness.

| component_key | source_component | target_component | alignment_rule | relation_to_refutation | relation_to_counterexample_status | relation_to_smaller_witness | failure_effect | proof_status | missing_hypothesis | caveat |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| source_normal_form_well_formed | recognized source normal form | none | source normal form anchors status predicate meaning | target refutation must be interpretable without changing source form | normal form is a predicate precondition | normal-form failure may expose smaller witness | normal-form blocker | proof_sketch_only | recognized source normal-form completeness | Recognized-source only. |
| target_normal_form_well_formed | lift context | lifted target normal form | target refutation uses a normal-form target row | target proof is meaningful on target side | needs transfer to source normal form | none by itself | target-normal escape | proved_under_current_scope |  | Target normality is not source normality. |
| normal_form_map | source normal-form fields | target normal-form fields | target normal-form fields preserve status-relevant source fields | permits source reading of target contradiction | prevents predicate shift | field mismatch may route to residual measure | blocked_by_normal_form | proof_sketch_only | normal-form field preservation | Open shared blocker with canonical/contract normal-form transfer. |
| normal_refinement_case | source form with residual family-chain fields | target refined form | refinement is acceptable only when residual fields are refuted or reduced | supports source contradiction conditionally | requires status predicate invariance under refinement | residual fields must decrease if not invariant | normal-refinement blocker | blocked_by_normal_form | normal-form invariance or residual measure decrease | Not a completed valid-witness transfer. |
| normal_escape_case | source normal form outside recognized map | target normal form outside transfer contract | classify as named normal-form escape | no source refutation claimed | status comparison remains open | future residual measure may discharge | named_escape | proved_under_current_scope |  | Escape classification is not theorem completion. |
