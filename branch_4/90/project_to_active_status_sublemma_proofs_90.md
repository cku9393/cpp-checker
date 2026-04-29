# Project To Active Status Sublemma Proofs 90

## status after locality refinement

`project_to_active_status_sublemma_proofs_payload_proved_status_domain_open`

The project-to-active status sublemma proofs now point to the locality-specific
proof attempt. Payload locality and failure classification are current-scope
proved. Counterexample-status locality remains proof-ready but blocked by
status-domain invariance, normal-form transfer, and valid reduced-status
fallback.

Runtime table:
`branch_4/90/runtime/project_to_active_status_sublemma_proofs_90.tsv`.
## Project To Active Domain Refinement Round

| sublemma_key | proof_status | assumptions | conclusion | proof_summary | evidence_path | missing_hypothesis | next_action |
| --- | --- | --- | --- | --- | --- | --- | --- |
| project_to_active_domain_language_well_defined | proved_under_current_scope | active support notation;projection semantics;status language | preserved/refined/reduced/lost/escape domain cases are well-defined | The round names the projected domain as a restriction/refinement candidate and separates domain reduction from escape. | project_to_active_status_domain_semantics_90.md |  | use_domain_language |
| source_status_domain_well_defined | proved_under_current_scope | source witness satisfies status semantics | source status-domain exists before projection | The source side is the existing status-domain predicate input and is not changed by this round. | project_to_active_status_semantics_90.md;status_preservation_language_90.md |  | use_source_domain |
| projected_status_domain_well_defined | proved_under_current_scope | active support A;projected witness normalize(restrict(W,A)) | projected status-domain candidate exists | Projection semantics provides the active restriction candidate; equality with source domain is not claimed. | project_to_active_support_operation_semantics_90.md;project_to_active_status_domain_semantics_90.md |  | domain_transfer_lemma |
| inactive_support_removal_preserves_or_refines_domain | proof_sketch_only | inactive support is outside active dependency map;payload locality holds | domain is preserved or refined by active projection | Payload locality rules out inactive payload carriers, but status-domain dependencies still need an irrelevance/refinement argument. | inactive_support_payload_locality_90.md;project_to_active_domain_transfer_lemma_90.md | status dependency irrelevance | domain_dependency_sublemma |
| domain_refinement_preserves_status_predicate_meaning | blocked_by_status_predicate | projected domain is a source-domain refinement;normal-form interface holds | counterexample-status predicate remains meaningful | The refined domain comparison is proof-ready, but status-predicate determination over the refined projected domain is still open. | project_to_active_domain_transfer_lemma_90.md | status predicate determination over refined domain | project_to_active_normal_form_refinement |
| domain_reduction_implies_smaller_witness | blocked_by_smaller_witness | domain shrinks;active support strictly smaller;projected status valid | smaller witness branch | Support measure decreases under strict active projection, but a valid reduced counterexample/status theorem is still required. | project_to_active_support_smaller_witness_90.md;project_to_active_domain_transfer_lemma_90.md | valid reduced-status theorem | valid_reduced_status_sublemma |
| payload_locality_supports_domain_transfer | proof_sketch_only | inactive-support payload locality is proved | payload-local dependencies are active inputs for domain transfer | Payload locality can feed the domain map, but it does not eliminate non-payload status-domain dependencies. | inactive_support_payload_locality_90.md | payload-to-status dependency bridge | payload_to_status_dependency_sublemma |
| domain_transfer_failure_is_named_escape | proved_under_current_scope | domain transfer cannot be preserved/refined/reduced | named project-to-active blocker or higher-support escape | The failure case remains explicit and does not become a hidden proof of status locality. | higher_support_necessity_after_project_to_active_domain_90.md;higher_support_escape_interface_90.md |  | keep_escape_visible |
