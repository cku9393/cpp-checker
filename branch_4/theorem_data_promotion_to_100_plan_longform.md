# theorem-data 승격 완성도를 100점으로 만들기 위한 계획 문서

## 문서 목적

theorem-data promotion 관점에서 이번 기준점의 핵심 성과는 네 줄로 정리된다.

- support8 lock은 current runtime에서 유지된다.
- current bundle metadata와 imported provenance metadata는 코드 / TSV / markdown report에서 분리된다.
- top-level current verified theorem-data item은 모두 `fresh_current_runtime_generated`다.
- lower-frontier inventory-only row의 처리 방식은 `keep_inventory_only_nonblocking`으로 확정했다.
- family-chain lower layer `7`개는 모두 current constructor/cache-backed path로 승격했다.
- family-chain lower-layer imported caveat는 `7/7` fresh, imported `0`으로 닫혔다.

## 1. 현재 promotion substrate

현재 top-level machine-readable inventory는 theorem / claim / audit item을 다음 네 층으로 나눈다.

- fresh current runtime generated: `16`
- current runtime validated imported data: `0`
- mixed: `0`
- archival only: `3`

이건 현재 문장 설명이 아니라 runtime files에 남는다.

- `branch_4/90/runtime/theorem_data_provenance_inventory_90.tsv`
- `branch_4/90/runtime/provenance_audit_fingerprint_90.tsv`
- `branch_4/90/runtime/lower_frontier_ladder_inventory_90.tsv`
- `branch_4/90/runtime/support8_antecedent15_shell_theorem_generation_audit_90.tsv`

## 2. 지금 current verified인 theorem-data layer

현재 support8 slice에서 fresh current-runtime generated로 올라온 top-level item은 다음과 같다.

- exact minimal basis size `96`
- basis-only theorem trio
- bounded family-chain theorem
- family-chain self verification
- shell15 frontier pair
- support8 antecedent15 shell theorem
- support8 outside-bounded tail pattern theorem
- support8 tail obstruction chain theorem
- support8 authoritative completion lock
- artifact / document / rerun / freshness audit

즉 top-level theorem-data promotion 관점에서 support8 slice는 현재 닫혀 있다.

## 3. 왜 아직 theorem-data 100점이 아닌가

theorem-data 100점이 아직 아닌 이유는 다음 구조 때문이다.

1. lower-frontier first-class inventory에는 shell11/shell12 pair `4`개가 direct shell15 dependency subset 밖 `keep_inventory_only_nonblocking` row로 남아 있다.
2. family-chain lower layer `7`개는 freshized됐고, remaining imported lower-layer row는 없다.
3. archival only `3`개 item은 preserved evidence로 남고 current theorem validation 결과로 승격되지 않는다.

즉 지금 남은 문제는 “top-level mixed item이 남아 있다”가 아니라 “broader theorem-data perimeter를 어디까지 fresh path로 넓힐 것인가”다.

## 4. 앞으로의 theorem-data promotion 계획

### 단계 1. lower-frontier perimeter 정리

lower-frontier inventory-only shell11/shell12 pair `4`개는 계속 분리해 두는 것으로 결정했다. 이들은 visible nonblocking perimeter row이며 top-level support8 blocker가 아니다.

### 단계 2. family-chain lower layers 정리

`pair_expansion_aggregate_52`는 다음 경로로 승격 완료됐다.

- pass1 constructor: `build_current_pair_expansion_aggregate_52_from_bounded_region_scan_`
- pass2/pass3 cache loader: `load_current_pair_expansion_aggregate_52_runtime_artifact_`
- payload fingerprint: `6003:2005080337376028436`
- imported equality result: `counts_and_consumer_visible_fingerprint_equal`

`triple_family_expansion_theorem_data_53`도 다음 경로로 승격 완료됐다.

- pass1 constructor: `build_current_triple_family_expansion_theorem_data_53_from_pair52_ready_family_scan_`
- pass2/pass3 cache loader: `load_current_triple_family_expansion_theorem_data_53_runtime_artifact_`
- payload fingerprint: `11888:3562593626991170520`
- imported equality result: `counts_and_consumer_visible_fingerprint_equal`
- upstream pair52 fallback hit: `0`

`quadruple_family_expansion_theorem_data_55`도 다음 경로로 승격 완료됐다.

- pass1 constructor: `build_current_quadruple_family_expansion_theorem_data_55_from_triple53_ready_family_scan_`
- pass2/pass3 cache loader: `load_current_quadruple_family_expansion_theorem_data_55_runtime_artifact_`
- payload fingerprint: `15637:3406948456738223960`
- imported equality result: `counts_and_consumer_visible_fingerprint_equal`
- upstream triple53 fallback hit: `0`
- upstream pair52 fallback hit: `0`

`quintuple_family_expansion_theorem_data_57`도 다음 경로로 승격 완료됐다.

- pass1 constructor: `build_current_quintuple_family_expansion_theorem_data_57_from_quad55_ready_family_scan_`
- pass2/pass3 cache loader: `load_current_quintuple_family_expansion_theorem_data_57_runtime_artifact_`
- payload fingerprint: `11519:14985224666762482157`
- imported equality result: `counts_and_consumer_visible_fingerprint_equal`
- upstream quad55 fallback hit: `0`
- upstream triple53 fallback hit: `0`
- upstream pair52 fallback hit: `0`

`sextuple_family_expansion_theorem_data_57`도 다음 경로로 승격 완료됐다.

- pass1 constructor: `build_current_sextuple_family_expansion_theorem_data_57_from_quintuple57_ready_family_scan_`
- pass2/pass3 cache loader: `load_current_sextuple_family_expansion_theorem_data_57_runtime_artifact_`
- payload fingerprint: `4567:5441664472856347648`
- imported equality result: `counts_and_consumer_visible_fingerprint_equal`
- upstream quintuple57 fallback hit: `0`
- upstream quad55 fallback hit: `0`
- upstream triple53 fallback hit: `0`
- upstream pair52 fallback hit: `0`

`septuple_family_expansion_theorem_data_57`도 다음 경로로 승격 완료됐다.

- pass1 constructor: `build_current_septuple_family_expansion_theorem_data_57_from_sextuple57_ready_family_scan_`
- pass2/pass3 cache loader: `load_current_septuple_family_expansion_theorem_data_57_runtime_artifact_`
- payload fingerprint: `804:10183455833117365445`
- imported equality result: `counts_and_consumer_visible_fingerprint_equal`
- upstream sextuple57 fallback hit: `0`
- upstream quintuple57 fallback hit: `0`
- upstream quad55 fallback hit: `0`
- upstream triple53 fallback hit: `0`
- upstream pair52 fallback hit: `0`

`high_family_expansion_theorem_data_57`도 다음 경로로 승격 완료됐다.

- pass1 constructor: `build_current_high_family_expansion_theorem_data_57_from_septuple57_ready_aggregate_scan_`
- pass2/pass3 cache loader: `load_current_high_family_expansion_theorem_data_57_runtime_artifact_`
- payload fingerprint: `317:16323892766005099572`
- imported equality result: `counts_and_consumer_visible_fingerprint_equal`
- upstream septuple57 fallback hit: `0`
- upstream sextuple57 fallback hit: `0`
- upstream quintuple57 fallback hit: `0`
- upstream quad55 fallback hit: `0`
- upstream triple53 fallback hit: `0`
- upstream pair52 fallback hit: `0`

다음 family-chain lower-layer target은 `none_family_chain_lower_layers_complete`이다.

### 단계 2-1. next theorem-data scope decision

Theorem-data promotion 자체는 current support8 top-level과 family-chain lower-layer 축에서 닫혔다. 다음 scope는 `general_gap_bridge_formalization`으로 둔다.

- shell16은 `preflight_contract_ready_no_scan`
- higher-support는 `higher_support_deferred_after_contract_equivalent_congruence_domain_normal_form_open`
- broader general gap theorem은 `ready_for_bridge_formalization`
- BOJ solver bridge는 `ready_for_problem_bridge_formalization`
- archive cleanup은 `worth_cleaning_later`

Bridge formalization 이후 proof-obligation 관점의 `prove_minimal_counterexample_reduction`은 proof-ready skeleton까지 진행됐고, limited bridge theorem은 current scope에서 증명됐다. support-bound/support-reduction/family-chain rounds now keep operation routes through `family_chain_absorption_reduction`. The current `status_preservation_congruence_bridge` formalizes the common status language and operation table, classifying preserved/reduced/refuted/absorbed/escaped outcomes while leaving operation-specific status proofs and residual absorption measure open. The higher-support recheck did not run support9+. The project-to-active locality refinement proved payload locality under the active support contract and moved counterexample-status locality to proof-ready/status-domain-open, while coordinate-contraction status is refined to payload/domain/normal-form/status-predicate open proof-ready skeleton, canonical-compression status remains proof-ready with canonical-motif status congruence open, and family-chain absorption status remains proof-ready with source-target alignment and residual measure open. Full theorem-data/general promotion은 아니다. 다음 target은 `canonical_compression_status_congruence_refinement`다.

### 단계 3. archival only 보존

archival only 항목은 삭제 대상이 아니다.  
이들은 preserved evidence로 남되, current verified와 섞이지 않도록 유지해야 한다.

## contract-equivalent congruence refinement update

- selected statement: `equivalent_coordinate_status_preserved_under_refined_congruence_or_reduced_or_escape`
- status-domain transfer: `contract_equivalent_status_domain_transfer_proof_ready_quotient_domain_open`
- normal-form transfer: `contract_equivalent_normal_form_transfer_proof_ready_quotient_normal_form_open`
- equivalent-coordinate congruence refinement: `equivalent_coordinate_congruence_payload_ready_domain_normal_form_open`
- coordinate-congruence skeleton: `proof_ready_skeleton_contract_equivalent_congruence_domain_normal_form_open`
- contract-equivalent operation status: `partial_contract_equivalent_congruence_proof_ready_domain_normal_form_open`
- higher-support necessity: `higher_support_deferred_after_contract_equivalent_congruence_domain_normal_form_open`
- general theorem readiness: `ready_for_canonical_compression_status_congruence_refinement`
- next action order: `canonical_compression_status_congruence_refinement`, `project_to_active_status_domain_refinement`, `family_chain_absorption_source_alignment_refinement`

This update does not prove the full general theorem and does not prove
`contract_equivalent_support_coordinates` fully.
## canonical-compression status congruence refinement round

Latest round: `canonical_compression_status_congruence_refinement`.

- selected canonical-congruence statement: `canonical_motif_status_preserved_under_refined_congruence_or_reduced_or_escape`
- status-domain transfer: `canonical_compression_status_domain_transfer_proof_ready_motif_domain_open`
- normal-form transfer: `canonical_compression_normal_form_transfer_proof_ready_motif_normal_form_open`
- canonical motif congruence refinement: `canonical_motif_congruence_payload_ready_domain_normal_form_open`
- canonical-congruence skeleton: `proof_ready_skeleton_canonical_compression_congruence_domain_normal_form_open`
- canonical-compression operation status: `partial_canonical_compression_congruence_proof_ready_domain_normal_form_open`
- status-congruence skeleton: `partial_status_congruence_canonical_refined_domain_normal_form_open_remaining_project_contract_alignment_measure_open`
- support reduction skeleton: `partition_ready_canonical_refined_domain_normal_form_open_remaining_project_contract_alignment_measure_open`
- support-bound lemma skeleton: `proof_ready_skeleton_canonical_refined_domain_normal_form_open_remaining_project_contract_alignment_measure_open`
- higher-support necessity: `higher_support_deferred_after_canonical_congruence_domain_normal_form_open`
- general theorem readiness: `ready_for_family_chain_absorption_source_alignment_refinement`
- next action order: `family_chain_absorption_source_alignment_refinement`, `project_to_active_status_domain_refinement`, `contract_equivalent_domain_normal_form_refinement`

The support8 lock remains `support8_authoritative_completion_locked`; required
docs/artifacts remain `39/39` and `8/8`; top-level provenance remains fresh
`16`, imported `0`, mixed `0`, archival `3`; family-chain lower layers remain
total `7`, fresh `7`, imported `0`, caveat closed `1`; and the limited bridge
theorem remains `limited_bridge_theorem_proved_under_current_scope`.

This does not prove the full general theorem, does not prove
`canonical_motif_compression` fully, does not prove support8 sufficiency, and
does not run support9+.
## Family Chain Source Alignment Refinement Round

| metric | value |
| --- | --- |
| latest_round | family_chain_absorption_source_alignment_refinement |
| selected_statement | source_alignment_or_smaller_witness_or_escape |
| source_alignment_semantics_status | family_chain_absorption_source_target_alignment_semantics_contract_ready |
| payload_alignment_status | family_chain_absorption_payload_alignment_proof_ready_source_target_payload_open |
| status_domain_alignment_status | family_chain_absorption_status_domain_alignment_proof_ready_source_target_domain_open |
| normal_form_alignment_status | family_chain_absorption_normal_form_alignment_proof_ready_source_target_normal_form_open |
| lifted_refutation_to_source_status | lifted_refutation_to_source_refutation_payload_domain_normal_form_open |
| source_alignment_skeleton_status | proof_ready_skeleton_family_chain_source_alignment_payload_domain_normal_form_open |
| family_chain_absorption_status | partial_family_chain_absorption_source_alignment_proof_ready_residual_measure_open |
| status_congruence_skeleton | partial_status_congruence_family_alignment_refined_payload_domain_normal_open_remaining_residual_project_contract_canonical_open |
| support_reduction_skeleton | partition_ready_family_alignment_refined_payload_domain_normal_open_remaining_residual_project_contract_canonical_open |
| support_bound_skeleton | proof_ready_skeleton_family_alignment_refined_payload_domain_normal_open_remaining_residual_project_contract_canonical_open |
| higher_support_necessity | higher_support_deferred_after_family_chain_source_alignment_payload_domain_normal_open |
| general_theorem_readiness | ready_for_residual_absorption_measure_decrease |
| next_action_1 | residual_absorption_measure_decrease |
| next_action_2 | project_to_active_status_domain_refinement |
| next_action_3 | contract_equivalent_domain_normal_form_refinement |
| caveat | not_full_general_theorem_or_full_absorption_proof |
## Residual Measure Decrease Refinement Round

| metric | value |
| --- | --- |
| latest_round | residual_absorption_measure_decrease |
| selected_statement | residual_absorption_lexicographic_measure_decreases_or_escape |
| residual_branch_classification_status | residual_absorption_branch_classification_contract_ready |
| residual_measure_tuple_status | residual_absorption_measure_tuple_well_founded_proof_ready |
| residual_smaller_witness_construction_status | residual_absorption_smaller_witness_construction_proof_ready_alignment_defect_open |
| residual_measure_skeleton_status | proof_ready_skeleton_residual_absorption_measure_decrease_alignment_defect_open |
| family_chain_absorption_status | partial_family_chain_absorption_residual_measure_proof_ready_shared_domain_normal_form_open |
| source_alignment_skeleton_status | proof_ready_skeleton_family_chain_source_alignment_payload_domain_normal_form_open_measure_refined |
| status_congruence_skeleton | partial_status_congruence_residual_measure_refined_remaining_project_contract_canonical_domain_normal_open |
| support_reduction_skeleton | partition_ready_residual_measure_refined_remaining_project_contract_canonical_domain_normal_open |
| support_bound_skeleton | proof_ready_skeleton_residual_measure_refined_remaining_project_contract_canonical_domain_normal_open |
| higher_support_necessity | higher_support_deferred_after_residual_absorption_measure_proof_ready_domain_normal_open |
| general_theorem_readiness | ready_for_project_to_active_status_domain_refinement |
| next_action_1 | project_to_active_status_domain_refinement |
| next_action_2 | contract_equivalent_domain_normal_form_refinement |
| next_action_3 | canonical_compression_domain_normal_form_refinement |
| caveat | not_full_general_theorem_or_full_absorption_proof |
## Project To Active Domain Refinement Round

| metric | value |
| --- | --- |
| latest_round | project_to_active_status_domain_refinement |
| selected_statement | projected_status_domain_refined_under_active_projection_or_reduced_or_escape |
| project_to_active_status_domain_semantics_status | project_to_active_status_domain_semantics_contract_ready |
| project_to_active_domain_transfer_lemma_status | project_to_active_domain_transfer_proof_ready_refinement_status_predicate_open |
| project_to_active_normal_form_interface_status | project_to_active_normal_form_transfer_interface_contract_ready |
| project_to_active_domain_skeleton_status | proof_ready_skeleton_project_to_active_domain_refinement_status_predicate_normal_form_open |
| project_to_active_operation_status | partial_project_to_active_domain_refinement_proof_ready_normal_form_status_predicate_open |
| status_congruence_skeleton | partial_status_congruence_project_domain_refined_remaining_contract_canonical_normal_open |
| support_reduction_skeleton | partition_ready_project_domain_refined_remaining_contract_canonical_normal_open |
| support_bound_skeleton | proof_ready_skeleton_project_domain_refined_remaining_contract_canonical_normal_open |
| higher_support_necessity | higher_support_deferred_after_project_to_active_domain_refinement_normal_form_open |
| general_theorem_readiness | ready_for_project_to_active_normal_form_refinement |
| next_action_1 | project_to_active_normal_form_refinement |
| next_action_2 | contract_equivalent_domain_normal_form_refinement |
| next_action_3 | canonical_compression_domain_normal_form_refinement |
| caveat | not_full_general_theorem_or_project_to_active_full_proof |
