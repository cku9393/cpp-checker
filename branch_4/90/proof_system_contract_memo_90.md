# Proof System Contract Memo 90

## baseline

- code baseline: `branch_4/90/full_dynamic_top_tree_engine_90.cpp`
- bundle role: support8 / shell15 / tail / completion-lock proof-system engine
- explicit non-goal: BOJ complete solver

## current bundle metadata

| item | current code fact | assessment |
| --- | --- | --- |
| solver identity | header says `This is NOT the complete BOJ solver` | current truth |
| non-LOCAL_TEST main | `int main() { return 0; }` | current truth |
| current bundle version | `90` | current truth |
| current bundle role | `support8 / shell15 / tail / completion-lock proof-system bundle` | current truth |
| current bundle source basename | `full_dynamic_top_tree_engine_90.cpp` | current truth |
| current bundle summary basename | `project_status_summary_90.md` | current bundle reference |

## imported closed-output provenance

| item | source in code | assessment |
| --- | --- | --- |
| provenance catalog | `imported_closed_output_provenance_catalog_()` | current truth |
| family-chain lower data | `family_chain_output_57` | imported theorem-data output retained as provenance |
| lower frontier ladder catalog | `support_plus_one_frontier_output_67` through `support8_shell14_frontier_output_79` | retained as imported provenance catalog; direct shell15 dependency subset is now current-generated |
| archival shell15 source | `support8_shell15_frontier_output_84` | retained as compatibility fallback / equality oracle, no longer authoritative current path |
| explicit source path retained | `/mnt/data/full_dynamic_top_tree_engine_84.cpp` for the `84` frontier source | provenance retained, not current bundle identity |

## required lists

| contract item | source function | count |
| --- | --- | --- |
| required docs | `required_support8_tail_doc_paths_83_()` | `39` |
| required artifacts | `required_support8_tail_artifact_paths_83_()` | `8` |

## audit gates

| gate | source function | pass condition |
| --- | --- | --- |
| artifact audit | `validate_artifact_completion_audit_stats_()` | all required artifacts exist, are nonempty, shell15 artifact set complete, tail artifact set complete |
| document audit | `validate_document_completion_audit_stats_()` | all required docs are nonempty and core summary / shell theorem / tail pattern / artifact notes / bridge note are ready |
| rerun audit | `validate_rerun_completion_audit_stats_()` | local-test binary exists, current run or current stamp matches current binary, release binary exists and release stamp matches current binary |
| audit freshness | `validate_support8_audit_freshness_stats_()` | current artifact/doc/rerun audit fingerprints all match current filesystem / stamps |
| tail obstruction chain | `validate_support8_tail_obstruction_chain_theorem_data_()` | shell15 theorem + tail pattern + artifact audit + document audit + rerun audit + freshness all pass |
| completion lock | `validate_support8_authoritative_completion_lock_data_()` | shell15 frontier + shell15 scope + tail pattern + tail chain + artifact audit + document audit + rerun audit + freshness + stale audit eliminated + local test verified + release compile verified |

## provenance outputs

| output | path | role |
| --- | --- | --- |
| top-level inventory tsv | `branch_4/90/runtime/theorem_data_provenance_inventory_90.tsv` | machine-readable top-level theorem/audit validation split |
| lower-frontier inventory tsv | `branch_4/90/runtime/lower_frontier_ladder_inventory_90.tsv` | machine-readable first-class lower-frontier inventory |
| lower-frontier generation audit | `branch_4/90/runtime/lower_frontier_ladder_generation_audit_90.tsv` | direct shell-theorem dependency subset generation/cache audit |
| shell-theorem generation audit | `branch_4/90/runtime/support8_antecedent15_shell_theorem_generation_audit_90.tsv` | shell theorem freshization status over the lower ladder |
| inventory-only decision audit | `branch_4/90/runtime/lower_frontier_inventory_only_decision_90.tsv` | shell11/shell12 inventory-only perimeter decision |
| family-chain lower-layer inventory | `branch_4/90/runtime/family_chain_lower_layers_inventory_90.tsv` | next family-chain lower imported layer priority map |
| pair-expansion aggregate 52 audit | `branch_4/90/runtime/pair_expansion_aggregate_52_generation_audit_90.tsv` | current constructor/cache-backed output 52 audit |
| pair-expansion aggregate 52 payload | `branch_4/90/runtime/pair_expansion_aggregate_52_payload_90.tsv` | 28-region single/pair bounded expansion payload |
| triple-family theorem data 53 audit | `branch_4/90/runtime/triple_family_expansion_theorem_data_53_generation_audit_90.tsv` | current constructor/cache-backed output 53 audit |
| triple-family theorem data 53 payload | `branch_4/90/runtime/triple_family_expansion_theorem_data_53_payload_90.tsv` | 35-region triple-family bounded expansion payload |
| quadruple-family theorem data 55 audit | `branch_4/90/runtime/quadruple_family_expansion_theorem_data_55_generation_audit_90.tsv` | current constructor/cache-backed output 55 audit |
| quadruple-family theorem data 55 payload | `branch_4/90/runtime/quadruple_family_expansion_theorem_data_55_payload_90.tsv` | 35-region quadruple-family bounded expansion payload |
| quintuple-family theorem data 57 audit | `branch_4/90/runtime/quintuple_family_expansion_theorem_data_57_generation_audit_90.tsv` | current constructor/cache-backed quintuple output 57 audit |
| quintuple-family theorem data 57 payload | `branch_4/90/runtime/quintuple_family_expansion_theorem_data_57_payload_90.tsv` | 21-region quintuple-family bounded expansion payload |
| sextuple-family theorem data 57 audit | `branch_4/90/runtime/sextuple_family_expansion_theorem_data_57_generation_audit_90.tsv` | current constructor/cache-backed sextuple output 57 audit |
| sextuple-family theorem data 57 payload | `branch_4/90/runtime/sextuple_family_expansion_theorem_data_57_payload_90.tsv` | 7-region sextuple-family bounded expansion payload |
| provenance summary tsv | `branch_4/90/runtime/provenance_audit_fingerprint_90.tsv` | machine-readable counts and current bundle metadata |
| runtime audit report | `branch_4/90/runtime/provenance_audit_report_90.md` | short current-runtime provenance report |
| archival inventory note | `branch_4/90/theorem_data_provenance_inventory_90.md` | preserved markdown view of the current inventory |

## current verified recovery state

- release compile: verified
- LOCAL_TEST compile: verified
- pass1: `support8_authoritative_completion_locked`
- pass2: `support8_authoritative_completion_locked`
- pass3: `support8_authoritative_completion_locked`
- current runtime root: `branch_4/90/runtime`
- required docs: `39 / 39`
- required artifacts: `8 / 8`

## provenance snapshot

- top-level item count: `19`
- top-level fresh current runtime generated: `16`
- top-level current runtime validated imported data: `0`
- top-level mixed: `0`
- top-level archival only: `3`
- lower-frontier first-class inventory row count: `23`
- lower-frontier direct dependency subset count: `19`
- lower-frontier direct dependency freshized count: `19`
- lower-frontier inventory-only mixed rows: shell11/shell12 pair `4`
- inventory-only decision: `freshize_now=0`, `keep_inventory_only_nonblocking=4`, `defer_after_family_chain=0`
- pair-expansion aggregate 52: `fresh_current_runtime_generated`
- pair-expansion aggregate 52 constructor/cache: `build_current_pair_expansion_aggregate_52_from_bounded_region_scan_` -> `load_current_pair_expansion_aggregate_52_runtime_artifact_`
- triple-family theorem data 53: `fresh_current_runtime_generated`
- triple-family theorem data 53 constructor/cache: `build_current_triple_family_expansion_theorem_data_53_from_pair52_ready_family_scan_` -> `load_current_triple_family_expansion_theorem_data_53_runtime_artifact_`
- quadruple-family theorem data 55: `fresh_current_runtime_generated`
- quadruple-family theorem data 55 constructor/cache: `build_current_quadruple_family_expansion_theorem_data_55_from_triple53_ready_family_scan_` -> `load_current_quadruple_family_expansion_theorem_data_55_runtime_artifact_`
- quintuple-family theorem data 57: `fresh_current_runtime_generated`
- quintuple-family theorem data 57 constructor/cache: `build_current_quintuple_family_expansion_theorem_data_57_from_quad55_ready_family_scan_` -> `load_current_quintuple_family_expansion_theorem_data_57_runtime_artifact_`
- sextuple-family theorem data 57: `fresh_current_runtime_generated`
- sextuple-family theorem data 57 constructor/cache: `build_current_sextuple_family_expansion_theorem_data_57_from_quintuple57_ready_family_scan_` -> `load_current_sextuple_family_expansion_theorem_data_57_runtime_artifact_`
- septuple-family theorem data 57: `fresh_current_runtime_generated`
- septuple-family theorem data 57 constructor/cache: `build_current_septuple_family_expansion_theorem_data_57_from_sextuple57_ready_family_scan_` -> `load_current_septuple_family_expansion_theorem_data_57_runtime_artifact_`
- high-family theorem data 57: `fresh_current_runtime_generated`
- high-family theorem data 57 constructor/cache: `build_current_high_family_expansion_theorem_data_57_from_septuple57_ready_aggregate_scan_` -> `load_current_high_family_expansion_theorem_data_57_runtime_artifact_`
- family-chain lower-layer status: fresh `7`, imported lower-layer `0`
- next priority: `none_family_chain_lower_layers_complete`

## immediate implication

- support8 lock recovery, lower-frontier direct dependency freshization, shell theorem freshization, tail-chain freshization, and completion-lock freshization are now current runtime facts for the support8 slice.
- the audit order and required counts were not weakened.
- the lower-frontier inventory-only shell11/shell12 rows are now explicitly retained as nonblocking perimeter rows.
- the remaining limitation is no longer a family-chain lower-layer imported theorem-data item; that caveat is closed at total `7`, fresh `7`, imported `0`.

## next-scope readiness contract

- current support8 closure certificate: `current_support8_closure_certificate_90.md`
- next-scope candidate inventory: `next_scope_candidate_inventory_90.md`
- decision matrix: `next_scope_decision_matrix_90.md`
- top recommendation: `general_gap_bridge_formalization`
- shell16 readiness label: `preflight_contract_ready_no_scan`
- higher-support necessity label: `needs_theoretical_bound_first`
- general gap theorem readiness label: `ready_for_bridge_formalization`
- BOJ solver bridge readiness label: `ready_for_problem_bridge_formalization`
- archive/provenance cleanup label: `worth_cleaning_later`

This contract keeps shell16, higher-support, general theorem proof, and BOJ solver implementation out of the current verified closure until a selected readiness target installs its own proof/runtime contract.

## general gap bridge contract

- bridge input package: `general_gap_bridge_input_package_90.md`
- statement scope inventory: `general_gap_statement_scope_memo_90.md`
- obligation inventory: `general_gap_bridge_obligation_inventory_90.md`
- dependency graph: `general_gap_bridge_dependency_graph_90.md`
- lemma candidates: `general_gap_bridge_lemma_candidates_90.md`
- limited skeleton: `limited_general_gap_bridge_skeleton_90.md`
- minimal counterexample reduction proof plan: `minimal_counterexample_reduction_limited_skeleton_90.md`
- tail monotonicity bridge proof plan: `tail_monotonicity_limited_bridge_skeleton_90.md`
- next action matrix: `general_gap_bridge_next_action_matrix_90.md`
- target statement candidates: `3`
- bridge obligation count: `10`
- limited bridge theorem status: `limited_bridge_theorem_proved_under_current_scope`
- minimal counterexample reduction status: `limited_reduction_used_in_limited_bridge_theorem`
- tail bridge status: `tail_escape_closed_for_limited_bridge_theorem`
- shell16 result semantics label: `shell16_probe_completed_local_exact_survivors_present_no_theorem_preserving_survivors`
- support-bound lemma status: `proof_ready_skeleton_phase2_relation_ready_operation_open`
- support reduction step status: `partition_ready_phase2_relation_ready_smaller_witness_operation_open`
- family-chain lift status: `partial_lift_phase2_relation_ready_status_open`
- refined lift map status: `partial_refinement_phase2_relation_ready_status_open`
- obstruction preservation skeleton: `partial_preservation_phase2_relation_ready_status_open`
- phase2 skeleton: `partial_phase2_proved_escape_open`
- phase2 sublemmas proved/sketch/blocked: `4/5/1`
- next exact target: `family_chain_lift_phase3_if_needed`

This contract defines proof obligations only. It does not add a shell16 gate, support9+ gate, general theorem gate, or solver gate.

## shell16 preflight and attempt contract

- selected shell16 preflight scope: `shell16_candidate_universe_dry_run`
- dependency map: `shell16_dependency_map_90.md`
- artifact contract: `shell16_artifact_contract_90.md`
- document contract: `shell16_document_contract_90.md`
- audit contract: `shell16_audit_contract_90.md`
- feasibility audit: `shell16_feasibility_90.md`
- attempt protocol: `shell16_attempt_protocol_90.md`
- guardrail fingerprint: `branch_4/90/runtime/shell16_preflight_fingerprint_90.tsv`
- attempt report: `shell16_attempt_report_90.md`
- result classification: `shell16_result_classification_90.md`
- current shell16 status: `shell16_probe_completed_local_exact_survivors_present_no_theorem_preserving_survivors`
- shell16 candidate universe: `4`
- shell16 raw/canonical/outside-bounded: `8/4/4`
- shell16 local exact/plus-one/theorem-preserving survivors: `2/0/0`
- shell16 fingerprint: `981:4479772858934799504`
- theorem promotion guard: active
- next exact target: `family_chain_lift_phase3_if_needed`

This promotion review does not add a full shell16 theorem result. It promotes first-boundary runtime facts and limited bridge lemmas under the no-promotion guard.
