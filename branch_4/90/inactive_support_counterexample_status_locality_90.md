# Inactive Support Counterexample Status Locality 90

## status

`inactive_support_counterexample_status_locality_proof_ready_status_domain_open`

## proof attempt

The desired locality lemma is:

If every counterexample-status dependency of `W` is contained in
`active_support(W)`, and if `normalize(restrict(W,A))` remains in the same
status predicate domain, then deleting inactive coordinates does not change the
counterexample status.

The current artifacts support the shape of this proof, but they do not yet prove
arbitrary status-domain invariance or normal-form transfer. Therefore this
round does not promote inactive-support status locality to completed proof.

Failure is classified rather than hidden:

- if the projected object remains a valid smaller counterexample or reduced
  obstruction, use the existing strict active-subset measure decrease;
- if a status dependency was not actually active, name a project-to-active
  status-dependency blocker;
- if normal form or status-domain transfer fails, name a project-to-active
  domain/normal-form blocker;
- higher-support remains deferred until operation-specific proof blockers close.

Runtime table:
`branch_4/90/runtime/inactive_support_counterexample_status_locality_90.tsv`.
## Project To Active Domain Refinement Round

| metric | value |
| --- | --- |
| payload_locality_status | inactive_support_payload_locality_proved_under_current_scope |
| status_domain_transfer_status | project_to_active_domain_transfer_proof_ready_refinement_status_predicate_open |
| normal_form_transfer_status | project_to_active_normal_form_transfer_interface_contract_ready |
| status_predicate_meaning | blocked_by_status_predicate |
| valid_reduced_status | blocked_by_smaller_witness |
| final_status | inactive_support_counterexample_status_locality_proof_ready_domain_refined_normal_form_open |
| caveat | payload_locality_not_status_locality |
