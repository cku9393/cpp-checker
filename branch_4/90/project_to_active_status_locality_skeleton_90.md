# Project To Active Status Locality Skeleton 90

## lemma

`project_to_active_status_preserved_under_locality_or_reduced_or_escape`

## statement

Let `W` be a normal support `>8` witness with support `S`, active support
`A = active_support(W)`, and `W_active = normalize(restrict(W,A))`.

If `A` is a strict subset of `S`, active projection either:

- preserves counterexample status when payload locality, status dependency
  containment, status-domain invariance, and normal-form transfer hold;
- gives a smaller witness when the projected status changes but remains a valid
  counterexample or reduced obstruction; or
- routes the failure to a named project-to-active blocker or a deferred
  higher-support escape after operation proofs close.

## status

`proof_ready_skeleton_project_to_active_locality_status_domain_open`

## missing steps

- status-domain invariance under `normalize(restrict(W,A))`;
- complete status/certificate dependency extraction;
- normal-form preservation for the status predicate;
- valid reduced-status fallback when projection changes domain;
- family-chain source-form projection when source/lift fields are relevant.

This is not a completed proof of `project_to_active_support`, not a full support
reduction proof, not support8 sufficiency, and not a full general theorem.

Runtime skeleton:
`branch_4/90/runtime/project_to_active_status_locality_skeleton_90.tsv`.
## Project To Active Domain Refinement Round

| metric | value |
| --- | --- |
| selected_statement | projected_status_domain_refined_under_active_projection_or_reduced_or_escape |
| payload_locality | inactive_support_payload_locality_proved_under_current_scope |
| status_domain_semantics | project_to_active_status_domain_semantics_contract_ready |
| domain_transfer_lemma | project_to_active_domain_transfer_proof_ready_refinement_status_predicate_open |
| normal_form_interface | project_to_active_normal_form_transfer_interface_contract_ready |
| counterexample_status_locality | inactive_support_counterexample_status_locality_proof_ready_domain_refined_normal_form_open |
| final_status | proof_ready_skeleton_project_to_active_locality_domain_refined_normal_form_open |
| caveat | project_to_active_support_not_fully_proved |
