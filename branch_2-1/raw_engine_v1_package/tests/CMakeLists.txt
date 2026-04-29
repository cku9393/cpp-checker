add_executable(raw_engine_tests
    raw_engine_cases.cpp
    raw_engine_main.cpp
    exhaustive_cases.cpp
    exhaustive_generator.cpp
    exact_canonicalizer.cpp
    metamorphic_cases.cpp
    multiclass_catalog.cpp
    split_choice_cases.cpp
    split_choice_oracle.cpp
    compare_ready_lineage.cpp
    family_applicability_audit.cpp
    policy_gate.cpp
    policy_baseline.cpp
    policy_manifest_freshness.cpp
    policy_runtime_budget.cpp
    runtime_manifest.cpp
    failure_signature.cpp
    reference_model.cpp
    reference_planner.cpp
    reference_oracle_cases.cpp
    planner_oracle_cases.cpp
    state_dump.cpp
    stabilization_support.cpp
)

target_include_directories(raw_engine_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/tests
)

target_link_libraries(raw_engine_tests PRIVATE raw_engine_lib)

raw_engine_apply_common_flags(raw_engine_tests)

add_custom_command(TARGET raw_engine_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/regressions
        $<TARGET_FILE_DIR:raw_engine_tests>/regressions
)

if (RAW_ENGINE_TEST_STRICT_WARNINGS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(raw_engine_tests PRIVATE
        -Wconversion
        -Wsign-conversion
        -Wformat=2
    )
endif()

add_test(NAME raw_engine_micro COMMAND raw_engine_tests --case micro)
add_test(NAME raw_engine_roundtrip COMMAND raw_engine_tests --case roundtrip)
add_test(NAME raw_engine_split_integrate_ensure COMMAND raw_engine_tests --case split_integrate_ensure)
add_test(NAME raw_engine_regression_44001 COMMAND raw_engine_tests --case regression_44001)
add_test(NAME raw_engine_regression_isolate_split_no_sep COMMAND raw_engine_tests --case regression_isolate_split_no_sep)
add_test(NAME raw_engine_planner COMMAND raw_engine_tests --case planner)
add_test(NAME raw_engine_fuzz_matrix COMMAND raw_engine_tests --case fuzz_matrix)
add_test(NAME raw_engine_fuzz COMMAND raw_engine_tests --case fuzz)
add_test(NAME raw_engine_isolate_oracle COMMAND raw_engine_tests --case isolate_oracle)
add_test(NAME raw_engine_split_oracle COMMAND raw_engine_tests --case split_oracle)
add_test(NAME raw_engine_join_oracle COMMAND raw_engine_tests --case join_oracle)
add_test(NAME raw_engine_integrate_oracle COMMAND raw_engine_tests --case integrate_oracle)
add_test(NAME raw_engine_planner_oracle COMMAND raw_engine_tests --case planner_oracle)
add_test(NAME raw_engine_planner_oracle_fuzz COMMAND raw_engine_tests --case planner_oracle_fuzz)
add_test(NAME raw_engine_replay_state COMMAND raw_engine_tests --case replay_state)
add_test(NAME raw_engine_reducer_smoke COMMAND raw_engine_tests --case reducer_smoke)
add_test(NAME raw_engine_replay_planner_state COMMAND raw_engine_tests --case replay_planner_state)
add_test(
    NAME raw_engine_reduce_planner_state_smoke
    COMMAND raw_engine_tests --case reduce_planner_state_smoke --artifact-dir artifacts/reduce_planner_state_smoke
)
add_test(NAME raw_engine_failure_signature_smoke COMMAND raw_engine_tests --case failure_signature_smoke)
add_test(NAME raw_engine_planner_failure_signature_smoke COMMAND raw_engine_tests --case planner_failure_signature_smoke)
add_test(
    NAME raw_engine_reduce_planner_state_step_budget
    COMMAND raw_engine_tests --case reduce_planner_state_step_budget --artifact-dir artifacts/reduce_planner_state_step_budget
)
add_test(
    NAME raw_engine_reduce_planner_state_trace_prefix
    COMMAND raw_engine_tests --case reduce_planner_state_trace_prefix --artifact-dir artifacts/reduce_planner_state_trace_prefix
)
add_test(NAME raw_engine_planner_oracle_fuzz_stats COMMAND raw_engine_tests --case planner_oracle_fuzz_stats)
add_test(NAME raw_engine_primitive_fault_detection_smoke COMMAND raw_engine_tests --case primitive_fault_detection_smoke)
add_test(NAME raw_engine_planner_fault_detection_smoke COMMAND raw_engine_tests --case planner_fault_detection_smoke)
add_test(NAME raw_engine_mutation_matrix_smoke COMMAND raw_engine_tests --case mutation_matrix_smoke)
add_test(NAME raw_engine_planner_fixpoint_idempotence COMMAND raw_engine_tests --case planner_fixpoint_idempotence)
add_test(NAME raw_engine_planner_replay_determinism COMMAND raw_engine_tests --case planner_replay_determinism)
add_test(
    NAME raw_engine_reducer_determinism_smoke
    COMMAND raw_engine_tests --case reducer_determinism_smoke --artifact-dir artifacts/reducer_determinism_smoke
)
add_test(NAME raw_engine_corpus_roundtrip_smoke COMMAND raw_engine_tests --case corpus_roundtrip_smoke)
add_test(NAME raw_engine_corpus_replay_smoke COMMAND raw_engine_tests --case corpus_replay_smoke)
add_test(NAME raw_engine_compare_ready_lineage_smoke COMMAND raw_engine_tests --case compare_ready_lineage_smoke)
add_test(
    NAME raw_engine_planner_tie_organic_applicability_smoke
    COMMAND raw_engine_tests --case planner_tie_organic_applicability_smoke
)
add_test(
    NAME raw_engine_planner_tie_organic_proxy_pass_smoke
    COMMAND raw_engine_tests --case planner_tie_organic_proxy_pass_smoke
)
add_test(
    NAME raw_engine_campaign_phase16_resume_gate_smoke
    COMMAND raw_engine_tests --case campaign_phase16_resume_gate_smoke --artifact-dir artifacts/campaign_phase16_resume_gate_smoke
)
add_test(
    NAME raw_engine_campaign_phase16_manifest_compatibility_smoke
    COMMAND raw_engine_tests --case campaign_phase16_manifest_compatibility_smoke --artifact-dir artifacts/campaign_phase16_manifest_compatibility_smoke
)
set_tests_properties(
    raw_engine_campaign_phase16_resume_gate_smoke
    raw_engine_campaign_phase16_manifest_compatibility_smoke
    PROPERTIES
    RUN_SERIAL TRUE
)
add_test(NAME raw_engine_split_choice_policy_graduation_smoke COMMAND raw_engine_tests --case split_choice_policy_graduation_smoke)
add_test(NAME raw_engine_policy_graduation_gate_smoke COMMAND raw_engine_tests --case policy_graduation_gate_smoke)
add_test(NAME raw_engine_policy_gate_manifest_smoke COMMAND raw_engine_tests --case policy_gate_manifest_smoke)
add_test(NAME raw_engine_policy_baseline_promote_smoke COMMAND raw_engine_tests --case policy_baseline_promote_smoke)
add_test(NAME raw_engine_policy_baseline_roundtrip_smoke COMMAND raw_engine_tests --case policy_baseline_roundtrip_smoke)
add_test(NAME raw_engine_policy_promote_reject_fail_smoke COMMAND raw_engine_tests --case policy_promote_reject_fail_smoke)
add_test(NAME raw_engine_policy_refresh_smoke COMMAND raw_engine_tests --case policy_refresh_smoke)
add_test(
    NAME raw_engine_policy_refresh_against_promoted_baseline_smoke
    COMMAND raw_engine_tests --case policy_refresh_against_promoted_baseline_smoke
)
add_test(
    NAME raw_engine_policy_refresh_family_filter_smoke
    COMMAND raw_engine_tests --case policy_refresh_family_filter_smoke
)
add_test(
    NAME raw_engine_policy_refresh_freshness_only_smoke
    COMMAND raw_engine_tests --case policy_refresh_freshness_only_smoke
)
add_test(NAME raw_engine_policy_refresh_stale_hash_smoke COMMAND raw_engine_tests --case policy_refresh_stale_hash_smoke)
add_test(
    NAME raw_engine_policy_refresh_compare_engine_hash_smoke
    COMMAND raw_engine_tests --case policy_refresh_compare_engine_hash_smoke
)
add_test(
    NAME raw_engine_policy_refresh_generator_hash_smoke
    COMMAND raw_engine_tests --case policy_refresh_generator_hash_smoke
)
add_test(
    NAME raw_engine_policy_refresh_campaign_config_hash_smoke
    COMMAND raw_engine_tests --case policy_refresh_campaign_config_hash_smoke
)
add_test(NAME raw_engine_policy_gate_family_filter_smoke COMMAND raw_engine_tests --case policy_gate_family_filter_smoke)
add_test(NAME raw_engine_policy_gate_non_applicable_smoke COMMAND raw_engine_tests --case policy_gate_non_applicable_smoke)
add_test(
    NAME raw_engine_policy_gate_non_applicable_drift_smoke
    COMMAND raw_engine_tests --case policy_gate_non_applicable_drift_smoke
)
add_test(
    NAME raw_engine_planner_tie_organic_applicability_drift_smoke
    COMMAND raw_engine_tests --case planner_tie_organic_applicability_drift_smoke
)
add_test(
    NAME raw_engine_planner_tie_organic_reclassify_trigger_smoke
    COMMAND raw_engine_tests --case planner_tie_organic_reclassify_trigger_smoke
)
add_test(
    NAME raw_engine_policy_reclassify_non_applicable_smoke
    COMMAND raw_engine_tests --case policy_reclassify_non_applicable_smoke
)
add_test(NAME raw_engine_compare_ready_lineage_gate_smoke COMMAND raw_engine_tests --case compare_ready_lineage_gate_smoke)
add_test(
    NAME raw_engine_compare_ready_not_counted_as_pass_smoke
    COMMAND raw_engine_tests --case compare_ready_not_counted_as_pass_smoke
)
add_test(
    NAME raw_engine_policy_gate_diagnostic_only_smoke
    COMMAND raw_engine_tests --case policy_gate_diagnostic_only_smoke
)
add_test(
    NAME raw_engine_compare_ready_not_promoted_smoke
    COMMAND raw_engine_tests --case compare_ready_not_promoted_smoke
)
add_test(
    NAME raw_engine_policy_reject_diagnostic_promotion_smoke
    COMMAND raw_engine_tests --case policy_reject_diagnostic_promotion_smoke
)
add_test(NAME raw_engine_policy_gate_nightly_smoke COMMAND raw_engine_tests --case policy_gate_nightly_smoke)
add_test(NAME raw_engine_policy_refresh_nightly_smoke COMMAND raw_engine_tests --case policy_refresh_nightly_smoke)
add_test(NAME raw_engine_manifest_roundtrip_smoke COMMAND raw_engine_tests --case manifest_roundtrip_smoke)
add_test(NAME raw_engine_evidence_bundle_smoke COMMAND raw_engine_tests --case evidence_bundle_smoke)
add_test(
    NAME raw_engine_evidence_bundle_manifest_freshness_smoke
    COMMAND raw_engine_tests --case evidence_bundle_manifest_freshness_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_policy_refresh_smoke
    COMMAND raw_engine_tests --case evidence_bundle_policy_refresh_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_rerun_plan_smoke
    COMMAND raw_engine_tests --case evidence_bundle_rerun_plan_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_idempotent_smoke
    COMMAND raw_engine_tests --case evidence_bundle_idempotent_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_policy_summary_smoke
    COMMAND raw_engine_tests --case evidence_bundle_policy_summary_smoke
)
add_test(
    NAME raw_engine_stale_family_selective_rerun_smoke
    COMMAND raw_engine_tests --case stale_family_selective_rerun_smoke
)
add_test(NAME raw_engine_policy_rerun_plan_smoke COMMAND raw_engine_tests --case policy_rerun_plan_smoke)
add_test(NAME raw_engine_policy_rerun_plan_empty_smoke COMMAND raw_engine_tests --case policy_rerun_plan_empty_smoke)
add_test(
    NAME raw_engine_policy_rerun_plan_family_filter_smoke
    COMMAND raw_engine_tests --case policy_rerun_plan_family_filter_smoke
)
add_test(
    NAME raw_engine_policy_rerun_plan_mode_selection_smoke
    COMMAND raw_engine_tests --case policy_rerun_plan_mode_selection_smoke
)
add_test(
    NAME raw_engine_policy_selective_rerun_execute_smoke
    COMMAND raw_engine_tests --case policy_selective_rerun_execute_smoke
)
add_test(
    NAME raw_engine_policy_selective_rerun_noop_smoke
    COMMAND raw_engine_tests --case policy_selective_rerun_noop_smoke
)
add_test(
    NAME raw_engine_policy_baseline_history_smoke
    COMMAND raw_engine_tests --case policy_baseline_history_smoke
)
add_test(
    NAME raw_engine_policy_baseline_archive_roundtrip_smoke
    COMMAND raw_engine_tests --case policy_baseline_archive_roundtrip_smoke
)
add_test(NAME raw_engine_policy_ci_check_smoke COMMAND raw_engine_tests --case policy_ci_check_smoke)
add_test(NAME raw_engine_policy_nightly_refresh_smoke COMMAND raw_engine_tests --case policy_nightly_refresh_smoke)
add_test(NAME raw_engine_policy_severity_smoke COMMAND raw_engine_tests --case policy_severity_smoke)
add_test(NAME raw_engine_pipeline_exit_code_smoke COMMAND raw_engine_tests --case pipeline_exit_code_smoke)
add_test(NAME raw_engine_stale_warn_mode_smoke COMMAND raw_engine_tests --case stale_warn_mode_smoke)
add_test(NAME raw_engine_action_required_mode_smoke COMMAND raw_engine_tests --case action_required_mode_smoke)
add_test(NAME raw_engine_runtime_budget_smoke COMMAND raw_engine_tests --case runtime_budget_smoke)
add_test(
    NAME raw_engine_runtime_budget_manifest_roundtrip_smoke
    COMMAND raw_engine_tests --case runtime_budget_manifest_roundtrip_smoke
)
add_test(NAME raw_engine_runtime_fingerprint_roundtrip_smoke COMMAND raw_engine_tests --case runtime_fingerprint_roundtrip_smoke)
add_test(NAME raw_engine_runtime_not_comparable_smoke COMMAND raw_engine_tests --case runtime_not_comparable_smoke)
add_test(NAME raw_engine_runtime_refresh_same_fingerprint_smoke COMMAND raw_engine_tests --case runtime_refresh_same_fingerprint_smoke)
add_test(NAME raw_engine_runtime_baseline_promote_smoke COMMAND raw_engine_tests --case runtime_baseline_promote_smoke)
add_test(NAME raw_engine_runtime_registry_promote_smoke COMMAND raw_engine_tests --case runtime_registry_promote_smoke)
add_test(
    NAME raw_engine_runtime_registry_activate_retire_smoke
    COMMAND raw_engine_tests --case runtime_registry_activate_retire_smoke
)
add_test(NAME raw_engine_runtime_registry_roundtrip_smoke COMMAND raw_engine_tests --case runtime_registry_roundtrip_smoke)
add_test(
    NAME raw_engine_runtime_registry_select_exact_match_smoke
    COMMAND raw_engine_tests --case runtime_registry_select_exact_match_smoke
)
add_test(
    NAME raw_engine_runtime_registry_select_no_match_smoke
    COMMAND raw_engine_tests --case runtime_registry_select_no_match_smoke
)
add_test(
    NAME raw_engine_runtime_registry_rebaseline_required_smoke
    COMMAND raw_engine_tests --case runtime_registry_rebaseline_required_smoke
)
add_test(
    NAME raw_engine_runtime_refresh_against_promoted_baseline_smoke
    COMMAND raw_engine_tests --case runtime_refresh_against_promoted_baseline_smoke
)
add_test(NAME raw_engine_runtime_rerun_plan_smoke COMMAND raw_engine_tests --case runtime_rerun_plan_smoke)
add_test(NAME raw_engine_runtime_rerun_plan_empty_smoke COMMAND raw_engine_tests --case runtime_rerun_plan_empty_smoke)
add_test(NAME raw_engine_runtime_proposal_gate_smoke COMMAND raw_engine_tests --case runtime_proposal_gate_smoke)
add_test(
    NAME raw_engine_runtime_proposal_gate_need_more_samples_smoke
    COMMAND raw_engine_tests --case runtime_proposal_gate_need_more_samples_smoke
)
add_test(
    NAME raw_engine_runtime_proposal_gate_approvable_smoke
    COMMAND raw_engine_tests --case runtime_proposal_gate_approvable_smoke
)
add_test(
    NAME raw_engine_runtime_proposal_gate_reject_hard_breach_smoke
    COMMAND raw_engine_tests --case runtime_proposal_gate_reject_hard_breach_smoke
)
add_test(NAME raw_engine_runtime_approve_rebaseline_smoke COMMAND raw_engine_tests --case runtime_approve_rebaseline_smoke)
add_test(
    NAME raw_engine_runtime_approve_rebaseline_from_gate_smoke
    COMMAND raw_engine_tests --case runtime_approve_rebaseline_from_gate_smoke
)
add_test(
    NAME raw_engine_runtime_approve_rebaseline_registry_switch_smoke
    COMMAND raw_engine_tests --case runtime_approve_rebaseline_registry_switch_smoke
)
add_test(
    NAME raw_engine_runtime_registry_active_switch_phase30_smoke
    COMMAND raw_engine_tests --case runtime_registry_active_switch_phase30_smoke
)
add_test(
    NAME raw_engine_runtime_approve_rebaseline_archive_proposal_smoke
    COMMAND raw_engine_tests --case runtime_approve_rebaseline_archive_proposal_smoke
)
add_test(
    NAME raw_engine_runtime_approve_rebaseline_reject_without_gate_smoke
    COMMAND raw_engine_tests --case runtime_approve_rebaseline_reject_without_gate_smoke
)
add_test(
    NAME raw_engine_runtime_refresh_after_rebaseline_smoke
    COMMAND raw_engine_tests --case runtime_refresh_after_rebaseline_smoke
)
add_test(
    NAME raw_engine_runtime_refresh_after_phase30_approval_smoke
    COMMAND raw_engine_tests --case runtime_refresh_after_phase30_approval_smoke
)
add_test(
    NAME raw_engine_runtime_rerun_plan_after_rebaseline_smoke
    COMMAND raw_engine_tests --case runtime_rerun_plan_after_rebaseline_smoke
)
add_test(
    NAME raw_engine_runtime_rerun_plan_after_phase30_approval_smoke
    COMMAND raw_engine_tests --case runtime_rerun_plan_after_phase30_approval_smoke
)
add_test(
    NAME raw_engine_runtime_selection_after_registry_switch_smoke
    COMMAND raw_engine_tests --case runtime_selection_after_registry_switch_smoke
)
add_test(
    NAME raw_engine_combined_pipeline_after_rebaseline_quick_smoke
    COMMAND raw_engine_tests --case combined_pipeline_after_rebaseline_quick_smoke
)
add_test(
    NAME raw_engine_combined_pipeline_after_phase30_rebaseline_quick_smoke
    COMMAND raw_engine_tests --case combined_pipeline_after_phase30_rebaseline_quick_smoke
)
add_test(
    NAME raw_engine_combined_pipeline_after_rebaseline_nightly_smoke
    COMMAND raw_engine_tests --case combined_pipeline_after_rebaseline_nightly_smoke
)
add_test(
    NAME raw_engine_combined_pipeline_after_phase30_rebaseline_nightly_smoke
    COMMAND raw_engine_tests --case combined_pipeline_after_phase30_rebaseline_nightly_smoke
)
add_test(
    NAME raw_engine_runtime_history_append_after_rebaseline_smoke
    COMMAND raw_engine_tests --case runtime_history_append_after_rebaseline_smoke
)
add_test(
    NAME raw_engine_runtime_history_append_after_phase30_rebaseline_smoke
    COMMAND raw_engine_tests --case runtime_history_append_after_phase30_rebaseline_smoke
)
add_test(
    NAME raw_engine_runtime_registry_history_transition_smoke
    COMMAND raw_engine_tests --case runtime_registry_history_transition_smoke
)
add_test(
    NAME raw_engine_runtime_registry_history_transition_phase30_smoke
    COMMAND raw_engine_tests --case runtime_registry_history_transition_phase30_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_contains_rebaseline_transition_smoke
    COMMAND raw_engine_tests --case evidence_bundle_contains_rebaseline_transition_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_phase30_transition_smoke
    COMMAND raw_engine_tests --case evidence_bundle_phase30_transition_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_runtime_transition_metadata_smoke
    COMMAND raw_engine_tests --case evidence_bundle_runtime_transition_metadata_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_phase30_metadata_smoke
    COMMAND raw_engine_tests --case evidence_bundle_phase30_metadata_smoke
)
add_test(
    NAME raw_engine_runtime_budget_proposal_gate_smoke
    COMMAND raw_engine_tests --case runtime_budget_proposal_gate_smoke
)
add_test(
    NAME raw_engine_runtime_budget_reproposal_gate_smoke
    COMMAND raw_engine_tests --case runtime_budget_reproposal_gate_smoke
)
add_test(
    NAME raw_engine_runtime_budget_approve_reprofile_smoke
    COMMAND raw_engine_tests --case runtime_budget_approve_reprofile_smoke
)
add_test(
    NAME raw_engine_runtime_budget_refresh_after_approval_smoke
    COMMAND raw_engine_tests --case runtime_budget_refresh_after_approval_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_watch_state_machine_smoke
    COMMAND raw_engine_tests --case runtime_current_env_watch_state_machine_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_age_tick_smoke
    COMMAND raw_engine_tests --case runtime_current_env_age_tick_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_plan_watch_smoke
    COMMAND raw_engine_tests --case runtime_current_env_plan_watch_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_reproposal_trigger_gate_smoke
    COMMAND raw_engine_tests --case runtime_current_env_reproposal_trigger_gate_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_due_scheduler_smoke
    COMMAND raw_engine_tests --case runtime_current_env_due_scheduler_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_time_travel_due_now_smoke
    COMMAND raw_engine_tests --case runtime_current_env_time_travel_due_now_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_plan_reproposal_smoke
    COMMAND raw_engine_tests --case runtime_current_env_plan_reproposal_smoke
)
add_test(
    NAME raw_engine_policy_ops_agenda_smoke
    COMMAND raw_engine_tests --case policy_ops_agenda_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_execute_watch_smoke
    COMMAND raw_engine_tests --case runtime_current_env_execute_watch_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_apply_watch_smoke
    COMMAND raw_engine_tests --case runtime_current_env_apply_watch_smoke
)
add_test(
    NAME raw_engine_current_env_monitoring_due_execute_apply_smoke
    COMMAND raw_engine_tests --case current_env_monitoring_due_execute_apply_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_execute_reproposal_gate_smoke
    COMMAND raw_engine_tests --case runtime_current_env_execute_reproposal_gate_smoke
)
add_test(
    NAME raw_engine_policy_ops_agenda_action_status_smoke
    COMMAND raw_engine_tests --case policy_ops_agenda_action_status_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_action_ledger_smoke
    COMMAND raw_engine_tests --case runtime_current_env_action_ledger_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_action_retry_plan_smoke
    COMMAND raw_engine_tests --case runtime_current_env_action_retry_plan_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_reproposal_handoff_smoke
    COMMAND raw_engine_tests --case runtime_current_env_reproposal_handoff_smoke
)
add_test(
    NAME raw_engine_current_env_next_cycle_monitoring_due_smoke
    COMMAND raw_engine_tests --case current_env_next_cycle_monitoring_due_smoke
)
add_test(
    NAME raw_engine_policy_ops_agenda_lifecycle_v2_smoke
    COMMAND raw_engine_tests --case policy_ops_agenda_lifecycle_v2_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_operator_decision_smoke
    COMMAND raw_engine_tests --case runtime_current_env_operator_decision_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_apply_operator_decision_smoke
    COMMAND raw_engine_tests --case runtime_current_env_apply_operator_decision_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_action_ledger_compact_smoke
    COMMAND raw_engine_tests --case runtime_current_env_action_ledger_compact_smoke
)
add_test(
    NAME raw_engine_reproposal_handoff_closure_only_smoke
    COMMAND raw_engine_tests --case reproposal_handoff_closure_only_smoke
)
add_test(
    NAME raw_engine_policy_ops_agenda_lifecycle_v3_smoke
    COMMAND raw_engine_tests --case policy_ops_agenda_lifecycle_v3_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_approval_runbook_smoke
    COMMAND raw_engine_tests --case runtime_current_env_approval_runbook_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_execute_budget_approval_dry_run_smoke
    COMMAND raw_engine_tests --case runtime_current_env_execute_budget_approval_dry_run_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_link_approval_execution_smoke
    COMMAND raw_engine_tests --case runtime_current_env_link_approval_execution_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_decision_skip_flow_smoke
    COMMAND raw_engine_tests --case runtime_current_env_decision_skip_flow_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_decision_defer_flow_smoke
    COMMAND raw_engine_tests --case runtime_current_env_decision_defer_flow_smoke
)
add_test(
    NAME raw_engine_runtime_current_env_decision_retry_now_flow_smoke
    COMMAND raw_engine_tests --case runtime_current_env_decision_retry_now_flow_smoke
)
add_test(
    NAME raw_engine_policy_ops_agenda_lifecycle_v4_smoke
    COMMAND raw_engine_tests --case policy_ops_agenda_lifecycle_v4_smoke
)
add_test(
    NAME raw_engine_runtime_budget_history_append_smoke
    COMMAND raw_engine_tests --case runtime_budget_history_append_smoke
)
add_test(
    NAME raw_engine_combined_pipeline_after_budget_reprofile_quick_smoke
    COMMAND raw_engine_tests --case combined_pipeline_after_budget_reprofile_quick_smoke
)
add_test(
    NAME raw_engine_combined_pipeline_after_budget_reprofile_nightly_smoke
    COMMAND raw_engine_tests --case combined_pipeline_after_budget_reprofile_nightly_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_budget_vs_rebaseline_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_budget_vs_rebaseline_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_current_env_clear_new_env_action_required_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_current_env_clear_new_env_action_required_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_budget_lifecycle_smoke
    COMMAND raw_engine_tests --case evidence_bundle_budget_lifecycle_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_budget_transition_metadata_smoke
    COMMAND raw_engine_tests --case evidence_bundle_budget_transition_metadata_smoke
)
add_test(NAME raw_engine_runtime_synthetic_warn_smoke COMMAND raw_engine_tests --case runtime_synthetic_warn_smoke)
add_test(
    NAME raw_engine_runtime_synthetic_action_required_smoke
    COMMAND raw_engine_tests --case runtime_synthetic_action_required_smoke
)
add_test(NAME raw_engine_runtime_synthetic_fail_smoke COMMAND raw_engine_tests --case runtime_synthetic_fail_smoke)
add_test(NAME raw_engine_runtime_missing_baseline_smoke COMMAND raw_engine_tests --case runtime_missing_baseline_smoke)
add_test(
    NAME raw_engine_runtime_fingerprint_mismatch_smoke
    COMMAND raw_engine_tests --case runtime_fingerprint_mismatch_smoke
)
add_test(NAME raw_engine_runtime_history_append_smoke COMMAND raw_engine_tests --case runtime_history_append_smoke)
add_test(
    NAME raw_engine_runtime_watch_history_append_phase30_smoke
    COMMAND raw_engine_tests --case runtime_watch_history_append_phase30_smoke
)
add_test(NAME raw_engine_runtime_history_summary_smoke COMMAND raw_engine_tests --case runtime_history_summary_smoke)
add_test(NAME raw_engine_runtime_history_jitter_smoke COMMAND raw_engine_tests --case runtime_history_jitter_smoke)
add_test(
    NAME raw_engine_runtime_rebaseline_proposal_smoke
    COMMAND raw_engine_tests --case runtime_rebaseline_proposal_smoke
)
add_test(NAME raw_engine_runtime_rebaseline_noop_smoke COMMAND raw_engine_tests --case runtime_rebaseline_noop_smoke)
add_test(NAME raw_engine_combined_policy_runtime_ok_smoke COMMAND raw_engine_tests --case combined_policy_runtime_ok_smoke)
add_test(NAME raw_engine_combined_policy_runtime_warn_smoke COMMAND raw_engine_tests --case combined_policy_runtime_warn_smoke)
add_test(
    NAME raw_engine_combined_policy_runtime_action_required_smoke
    COMMAND raw_engine_tests --case combined_policy_runtime_action_required_smoke
)
add_test(NAME raw_engine_combined_policy_runtime_fail_smoke COMMAND raw_engine_tests --case combined_policy_runtime_fail_smoke)
add_test(NAME raw_engine_artifact_index_smoke COMMAND raw_engine_tests --case artifact_index_smoke)
add_test(NAME raw_engine_artifact_prune_smoke COMMAND raw_engine_tests --case artifact_prune_smoke)
add_test(NAME raw_engine_bundle_index_roundtrip_smoke COMMAND raw_engine_tests --case bundle_index_roundtrip_smoke)
add_test(
    NAME raw_engine_policy_pipeline_quick_registry_smoke
    COMMAND raw_engine_tests --case policy_pipeline_quick_registry_smoke
)
add_test(
    NAME raw_engine_policy_pipeline_nightly_history_smoke
    COMMAND raw_engine_tests --case policy_pipeline_nightly_history_smoke
)
add_test(
    NAME raw_engine_policy_pipeline_rebaseline_needed_smoke
    COMMAND raw_engine_tests --case policy_pipeline_rebaseline_needed_smoke
)
add_test(
    NAME raw_engine_runtime_registry_unknown_fingerprint_smoke
    COMMAND raw_engine_tests --case runtime_registry_unknown_fingerprint_smoke
)
add_test(
    NAME raw_engine_runtime_registry_multiple_candidate_smoke
    COMMAND raw_engine_tests --case runtime_registry_multiple_candidate_smoke
)
add_test(
    NAME raw_engine_runtime_registry_retired_only_smoke
    COMMAND raw_engine_tests --case runtime_registry_retired_only_smoke
)
add_test(
    NAME raw_engine_runtime_registry_multi_fingerprint_smoke
    COMMAND raw_engine_tests --case runtime_registry_multi_fingerprint_smoke
)
add_test(
    NAME raw_engine_runtime_registry_same_host_compiler_bump_smoke
    COMMAND raw_engine_tests --case runtime_registry_same_host_compiler_bump_smoke
)
add_test(
    NAME raw_engine_runtime_registry_sanitizer_change_smoke
    COMMAND raw_engine_tests --case runtime_registry_sanitizer_change_smoke
)
add_test(
    NAME raw_engine_runtime_registry_runner_tag_change_smoke
    COMMAND raw_engine_tests --case runtime_registry_runner_tag_change_smoke
)
add_test(
    NAME raw_engine_runtime_registry_cross_host_smoke
    COMMAND raw_engine_tests --case runtime_registry_cross_host_smoke
)
add_test(
    NAME raw_engine_runtime_history_regression_spike_smoke
    COMMAND raw_engine_tests --case runtime_history_regression_spike_smoke
)
add_test(NAME raw_engine_runtime_history_stable_smoke COMMAND raw_engine_tests --case runtime_history_stable_smoke)
add_test(NAME raw_engine_runtime_history_noisy_smoke COMMAND raw_engine_tests --case runtime_history_noisy_smoke)
add_test(NAME raw_engine_runtime_history_regressing_smoke COMMAND raw_engine_tests --case runtime_history_regressing_smoke)
add_test(
    NAME raw_engine_runtime_history_recovery_after_rebaseline_smoke
    COMMAND raw_engine_tests --case runtime_history_recovery_after_rebaseline_smoke
)
add_test(NAME raw_engine_runtime_watch_campaign_smoke COMMAND raw_engine_tests --case runtime_watch_campaign_smoke)
add_test(
    NAME raw_engine_runtime_watch_campaign_phase30_repeated_smoke
    COMMAND raw_engine_tests --case runtime_watch_campaign_phase30_repeated_smoke
)
add_test(
    NAME raw_engine_runtime_watch_campaign_longer_smoke
    COMMAND raw_engine_tests --case runtime_watch_campaign_longer_smoke
)
add_test(
    NAME raw_engine_runtime_watch_campaign_phase30_repeat10_smoke
    COMMAND raw_engine_tests --case runtime_watch_campaign_phase30_repeat10_smoke
)
if(RAW_ENGINE_REGISTER_NIGHTLY_TESTS)
add_test(
    NAME raw_engine_runtime_watch_multi_fingerprint_smoke
    COMMAND raw_engine_tests --case runtime_watch_multi_fingerprint_smoke
)
add_test(
    NAME raw_engine_runtime_watch_same_host_compiler_bump_smoke
    COMMAND raw_engine_tests --case runtime_watch_same_host_compiler_bump_smoke
)
add_test(
    NAME raw_engine_runtime_watch_sanitizer_change_smoke
    COMMAND raw_engine_tests --case runtime_watch_sanitizer_change_smoke
)
add_test(
    NAME raw_engine_runtime_watch_runner_tag_change_smoke
    COMMAND raw_engine_tests --case runtime_watch_runner_tag_change_smoke
)
add_test(NAME raw_engine_runtime_watch_cross_host_smoke COMMAND raw_engine_tests --case runtime_watch_cross_host_smoke)
add_test(NAME raw_engine_runtime_watch_retired_only_smoke COMMAND raw_engine_tests --case runtime_watch_retired_only_smoke)
endif()
add_test(
    NAME raw_engine_runtime_watch_stable_overrun_smoke
    COMMAND raw_engine_tests --case runtime_watch_stable_overrun_smoke
)
add_test(
    NAME raw_engine_runtime_watch_clear_after_recovery_smoke
    COMMAND raw_engine_tests --case runtime_watch_clear_after_recovery_smoke
)
add_test(NAME raw_engine_runtime_watch_escalate_smoke COMMAND raw_engine_tests --case runtime_watch_escalate_smoke)
add_test(
    NAME raw_engine_runtime_watch_transition_clear_smoke
    COMMAND raw_engine_tests --case runtime_watch_transition_clear_smoke
)
add_test(
    NAME raw_engine_runtime_watch_transition_stable_smoke
    COMMAND raw_engine_tests --case runtime_watch_transition_stable_smoke
)
add_test(
    NAME raw_engine_runtime_watch_transition_escalate_smoke
    COMMAND raw_engine_tests --case runtime_watch_transition_escalate_smoke
)
add_test(
    NAME raw_engine_runtime_watch_to_rebaseline_candidate_smoke
    COMMAND raw_engine_tests --case runtime_watch_to_rebaseline_candidate_smoke
)
add_test(
    NAME raw_engine_runtime_watch_production_hard_fail_smoke
    COMMAND raw_engine_tests --case runtime_watch_production_hard_fail_smoke
)
add_test(
    NAME raw_engine_runtime_watch_diagnostic_soft_warn_smoke
    COMMAND raw_engine_tests --case runtime_watch_diagnostic_soft_warn_smoke
)
add_test(
    NAME raw_engine_runtime_registry_import_current_smoke
    COMMAND raw_engine_tests --case runtime_registry_import_current_smoke
)
add_test(
    NAME raw_engine_runtime_registry_import_new_fingerprint_smoke
    COMMAND raw_engine_tests --case runtime_registry_import_new_fingerprint_smoke
)
add_test(
    NAME raw_engine_runtime_watch_import_same_fingerprint_smoke
    COMMAND raw_engine_tests --case runtime_watch_import_same_fingerprint_smoke
)
add_test(
    NAME raw_engine_runtime_watch_import_fixture_vs_real_smoke
    COMMAND raw_engine_tests --case runtime_watch_import_fixture_vs_real_smoke
)
add_test(
    NAME raw_engine_runtime_watch_confidence_smoke
    COMMAND raw_engine_tests --case runtime_watch_confidence_smoke
)
add_test(
    NAME raw_engine_runtime_watch_campaign_release_full_smoke
    COMMAND raw_engine_tests --case runtime_watch_campaign_release_full_smoke
)
add_test(
    NAME raw_engine_runtime_watch_release_full_repeat10_smoke
    COMMAND raw_engine_tests --case runtime_watch_release_full_repeat10_smoke
)
add_test(
    NAME raw_engine_runtime_watch_campaign_release_repeat10_smoke
    COMMAND raw_engine_tests --case runtime_watch_campaign_release_repeat10_smoke
)
add_test(
    NAME raw_engine_runtime_watch_release_full_confidence_smoke
    COMMAND raw_engine_tests --case runtime_watch_release_full_confidence_smoke
)
add_test(
    NAME raw_engine_runtime_watch_confidence_real_vs_fixture_smoke
    COMMAND raw_engine_tests --case runtime_watch_confidence_real_vs_fixture_smoke
)
add_test(
    NAME raw_engine_runtime_history_compact_smoke
    COMMAND raw_engine_tests --case runtime_history_compact_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_unified_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_unified_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_current_vs_new_env_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_current_vs_new_env_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_release_watch_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_release_watch_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_matrix_rebaseline_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_matrix_rebaseline_smoke
)
add_test(
    NAME raw_engine_runtime_budget_role_sensitive_smoke
    COMMAND raw_engine_tests --case runtime_budget_role_sensitive_smoke
)
add_test(
    NAME raw_engine_publication_snapshot_smoke
    COMMAND raw_engine_tests --case publication_snapshot_smoke
)
add_test(
    NAME raw_engine_publication_snapshot_bundle_smoke
    COMMAND raw_engine_tests --case publication_snapshot_bundle_smoke
)
add_test(
    NAME raw_engine_bundle_published_snapshot_smoke
    COMMAND raw_engine_tests --case bundle_published_snapshot_smoke
)
add_test(
    NAME raw_engine_bundle_delivery_contents_smoke
    COMMAND raw_engine_tests --case bundle_delivery_contents_smoke
)
add_test(
    NAME raw_engine_bundle_idempotent_from_published_snapshot_smoke
    COMMAND raw_engine_tests --case bundle_idempotent_from_published_snapshot_smoke
)
add_test(
    NAME raw_engine_staged_ctest_smoke
    COMMAND raw_engine_tests --case staged_ctest_smoke
)
add_test(
    NAME raw_engine_campaign_wrapper_tail_smoke
    COMMAND raw_engine_tests --case campaign_wrapper_tail_smoke
)
add_test(
    NAME raw_engine_ctest_executable_lookup_smoke
    COMMAND raw_engine_tests --case ctest_executable_lookup_smoke
)
add_test(
    NAME raw_engine_wrapper_infra_vs_semantic_failure_smoke
    COMMAND raw_engine_tests --case wrapper_infra_vs_semantic_failure_smoke
)
add_test(
    NAME raw_engine_runtime_recommendation_watch_smoke
    COMMAND raw_engine_tests --case runtime_recommendation_watch_smoke
)
add_test(
    NAME raw_engine_runtime_recommendation_rebaseline_required_smoke
    COMMAND raw_engine_tests --case runtime_recommendation_rebaseline_required_smoke
)
add_test(
    NAME raw_engine_runtime_recommendation_not_comparable_smoke
    COMMAND raw_engine_tests --case runtime_recommendation_not_comparable_smoke
)
add_test(
    NAME raw_engine_combined_pipeline_recommendation_smoke
    COMMAND raw_engine_tests --case combined_pipeline_recommendation_smoke
)
add_test(
    NAME raw_engine_combined_pipeline_continue_monitoring_smoke
    COMMAND raw_engine_tests --case combined_pipeline_continue_monitoring_smoke
)
add_test(
    NAME raw_engine_combined_pipeline_watch_rationale_smoke
    COMMAND raw_engine_tests --case combined_pipeline_watch_rationale_smoke
)
add_test(
    NAME raw_engine_combined_pipeline_propose_runtime_rebaseline_smoke
    COMMAND raw_engine_tests --case combined_pipeline_propose_runtime_rebaseline_smoke
)
add_test(
    NAME raw_engine_combined_pipeline_fail_on_production_budget_smoke
    COMMAND raw_engine_tests --case combined_pipeline_fail_on_production_budget_smoke
)
add_test(
    NAME raw_engine_runtime_rebaseline_proposal_multi_candidate_smoke
    COMMAND raw_engine_tests --case runtime_rebaseline_proposal_multi_candidate_smoke
)
add_test(
    NAME raw_engine_runtime_rebaseline_proposal_retired_only_smoke
    COMMAND raw_engine_tests --case runtime_rebaseline_proposal_retired_only_smoke
)
add_test(
    NAME raw_engine_runtime_rebaseline_proposal_cross_host_smoke
    COMMAND raw_engine_tests --case runtime_rebaseline_proposal_cross_host_smoke
)
add_test(
    NAME raw_engine_runtime_import_external_bundle_smoke
    COMMAND raw_engine_tests --case runtime_import_external_bundle_smoke
)
add_test(
    NAME raw_engine_runtime_import_external_new_fingerprint_smoke
    COMMAND raw_engine_tests --case runtime_import_external_new_fingerprint_smoke
)
add_test(
    NAME raw_engine_runtime_import_external_same_fingerprint_append_smoke
    COMMAND raw_engine_tests --case runtime_import_external_same_fingerprint_append_smoke
)
add_test(
    NAME raw_engine_runtime_import_external_malformed_bundle_smoke
    COMMAND raw_engine_tests --case runtime_import_external_malformed_bundle_smoke
)
add_test(
    NAME raw_engine_runtime_new_env_proposal_gate_smoke
    COMMAND raw_engine_tests --case runtime_new_env_proposal_gate_smoke
)
add_test(
    NAME raw_engine_runtime_new_env_need_more_samples_smoke
    COMMAND raw_engine_tests --case runtime_new_env_need_more_samples_smoke
)
add_test(
    NAME raw_engine_runtime_new_env_approvable_smoke
    COMMAND raw_engine_tests --case runtime_new_env_approvable_smoke
)
add_test(
    NAME raw_engine_runtime_new_env_reject_hard_breach_smoke
    COMMAND raw_engine_tests --case runtime_new_env_reject_hard_breach_smoke
)
add_test(
    NAME raw_engine_runtime_approve_new_env_baseline_smoke
    COMMAND raw_engine_tests --case runtime_approve_new_env_baseline_smoke
)
add_test(
    NAME raw_engine_runtime_registry_keep_current_env_active_smoke
    COMMAND raw_engine_tests --case runtime_registry_keep_current_env_active_smoke
)
add_test(
    NAME raw_engine_runtime_registry_multi_fingerprint_active_smoke
    COMMAND raw_engine_tests --case runtime_registry_multi_fingerprint_active_smoke
)
add_test(
    NAME raw_engine_runtime_approve_new_env_reject_without_gate_smoke
    COMMAND raw_engine_tests --case runtime_approve_new_env_reject_without_gate_smoke
)
add_test(
    NAME raw_engine_runtime_refresh_after_new_env_approval_smoke
    COMMAND raw_engine_tests --case runtime_refresh_after_new_env_approval_smoke
)
add_test(
    NAME raw_engine_runtime_rerun_after_new_env_approval_smoke
    COMMAND raw_engine_tests --case runtime_rerun_after_new_env_approval_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_three_way_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_three_way_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_current_vs_known_vs_foreign_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_current_vs_known_vs_foreign_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_new_env_onboarding_smoke
    COMMAND raw_engine_tests --case evidence_bundle_new_env_onboarding_smoke
)
add_test(
    NAME raw_engine_light_ops_bundle_registry_summary_smoke
    COMMAND raw_engine_tests --case light_ops_bundle_registry_summary_smoke
)
add_test(
    NAME raw_engine_bundle_metadata_multi_active_registry_smoke
    COMMAND raw_engine_tests --case bundle_metadata_multi_active_registry_smoke
)
add_test(
    NAME raw_engine_runtime_registry_health_smoke
    COMMAND raw_engine_tests --case runtime_registry_health_smoke
)
add_test(
    NAME raw_engine_runtime_known_env_stale_smoke
    COMMAND raw_engine_tests --case runtime_known_env_stale_smoke
)
add_test(
    NAME raw_engine_runtime_known_env_reverify_required_smoke
    COMMAND raw_engine_tests --case runtime_known_env_reverify_required_smoke
)
add_test(
    NAME raw_engine_runtime_known_env_retire_candidate_smoke
    COMMAND raw_engine_tests --case runtime_known_env_retire_candidate_smoke
)
add_test(
    NAME raw_engine_runtime_known_env_state_machine_smoke
    COMMAND raw_engine_tests --case runtime_known_env_state_machine_smoke
)
add_test(
    NAME raw_engine_runtime_import_known_env_evidence_smoke
    COMMAND raw_engine_tests --case runtime_import_known_env_evidence_smoke
)
add_test(
    NAME raw_engine_runtime_known_env_age_tick_smoke
    COMMAND raw_engine_tests --case runtime_known_env_age_tick_smoke
)
add_test(
    NAME raw_engine_runtime_known_env_plan_reverify_smoke
    COMMAND raw_engine_tests --case runtime_known_env_plan_reverify_smoke
)
add_test(
    NAME raw_engine_runtime_known_env_reverify_gate_smoke
    COMMAND raw_engine_tests --case runtime_known_env_reverify_gate_smoke
)
add_test(
    NAME raw_engine_runtime_known_env_apply_reverify_smoke
    COMMAND raw_engine_tests --case runtime_known_env_apply_reverify_smoke
)
add_test(
    NAME raw_engine_runtime_known_env_plan_retire_smoke
    COMMAND raw_engine_tests --case runtime_known_env_plan_retire_smoke
)
add_test(
    NAME raw_engine_runtime_known_env_apply_retire_smoke
    COMMAND raw_engine_tests --case runtime_known_env_apply_retire_smoke
)
add_test(
    NAME raw_engine_runtime_known_env_retired_smoke
    COMMAND raw_engine_tests --case runtime_known_env_retired_smoke
)
add_test(
    NAME raw_engine_runtime_registry_health_v2_smoke
    COMMAND raw_engine_tests --case runtime_registry_health_v2_smoke
)
add_test(
    NAME raw_engine_runtime_registry_health_v3_smoke
    COMMAND raw_engine_tests --case runtime_registry_health_v3_smoke
)
add_test(
    NAME raw_engine_runtime_registry_same_fingerprint_supersession_smoke
    COMMAND raw_engine_tests --case runtime_registry_same_fingerprint_supersession_smoke
)
add_test(
    NAME raw_engine_runtime_registry_retired_not_selected_smoke
    COMMAND raw_engine_tests --case runtime_registry_retired_not_selected_smoke
)
add_test(
    NAME raw_engine_runtime_foreign_env_state_machine_smoke
    COMMAND raw_engine_tests --case runtime_foreign_env_state_machine_smoke
)
add_test(
    NAME raw_engine_publication_health_smoke
    COMMAND raw_engine_tests --case publication_health_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_v4_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_v4_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_v5_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_v5_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_v6_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_v6_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_v7_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_v7_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_v8_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_v8_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_v9_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_v9_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_v10_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_v10_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_v11_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_v11_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_v12_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_v12_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_v13_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_v13_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_known_vs_foreign_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_known_vs_foreign_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_publication_health_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_publication_health_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_registry_health_smoke
    COMMAND raw_engine_tests --case evidence_bundle_registry_health_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_known_env_governance_smoke
    COMMAND raw_engine_tests --case evidence_bundle_known_env_governance_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_known_env_timeline_smoke
    COMMAND raw_engine_tests --case evidence_bundle_known_env_timeline_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_current_env_budget_lifecycle_smoke
    COMMAND raw_engine_tests --case evidence_bundle_current_env_budget_lifecycle_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_current_env_guardrail_timeline_smoke
    COMMAND raw_engine_tests --case evidence_bundle_current_env_guardrail_timeline_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_current_env_due_timeline_smoke
    COMMAND raw_engine_tests --case evidence_bundle_current_env_due_timeline_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_current_env_execute_apply_smoke
    COMMAND raw_engine_tests --case evidence_bundle_current_env_execute_apply_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_action_ledger_smoke
    COMMAND raw_engine_tests --case evidence_bundle_action_ledger_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_operator_decision_smoke
    COMMAND raw_engine_tests --case evidence_bundle_operator_decision_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_approval_runbook_smoke
    COMMAND raw_engine_tests --case evidence_bundle_approval_runbook_smoke
)
add_test(
    NAME raw_engine_operator_runbook_index_smoke
    COMMAND raw_engine_tests --case operator_runbook_index_smoke
)
add_test(
    NAME raw_engine_approval_execution_dry_run_smoke
    COMMAND raw_engine_tests --case approval_execution_dry_run_smoke
)
add_test(
    NAME raw_engine_approval_execution_handoff_only_smoke
    COMMAND raw_engine_tests --case approval_execution_handoff_only_smoke
)
add_test(
    NAME raw_engine_approval_execution_integrated_requires_optin_smoke
    COMMAND raw_engine_tests --case approval_execution_integrated_requires_optin_smoke
)
add_test(
    NAME raw_engine_approval_execution_integrated_optin_fixture_smoke
    COMMAND raw_engine_tests --case approval_execution_integrated_optin_fixture_smoke
)
add_test(
    NAME raw_engine_current_env_decision_skip_e2e_smoke
    COMMAND raw_engine_tests --case current_env_decision_skip_e2e_smoke
)
add_test(
    NAME raw_engine_current_env_decision_defer_e2e_smoke
    COMMAND raw_engine_tests --case current_env_decision_defer_e2e_smoke
)
add_test(
    NAME raw_engine_current_env_decision_reject_e2e_smoke
    COMMAND raw_engine_tests --case current_env_decision_reject_e2e_smoke
)
add_test(
    NAME raw_engine_current_env_decision_retry_now_e2e_smoke
    COMMAND raw_engine_tests --case current_env_decision_retry_now_e2e_smoke
)
add_test(
    NAME raw_engine_action_ledger_closure_invariants_smoke
    COMMAND raw_engine_tests --case action_ledger_closure_invariants_smoke
)
add_test(
    NAME raw_engine_source_health_preflight_smoke
    COMMAND raw_engine_tests --case source_health_preflight_smoke
)
add_test(
    NAME raw_engine_staged_sparse_clone_overlay_smoke
    COMMAND raw_engine_tests --case staged_sparse_clone_overlay_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_v14_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_v14_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_runbook_index_smoke
    COMMAND raw_engine_tests --case evidence_bundle_runbook_index_smoke
)
add_test(
    NAME raw_engine_operator_runbook_catalog_smoke
    COMMAND raw_engine_tests --case operator_runbook_catalog_smoke
)
add_test(
    NAME raw_engine_operator_decision_metadata_audit_smoke
    COMMAND raw_engine_tests --case operator_decision_metadata_audit_smoke
)
add_test(
    NAME raw_engine_integrated_approval_requires_confirmation_smoke
    COMMAND raw_engine_tests --case integrated_approval_requires_confirmation_smoke
)
add_test(
    NAME raw_engine_integrated_approval_rejects_without_preflight_smoke
    COMMAND raw_engine_tests --case integrated_approval_rejects_without_preflight_smoke
)
add_test(
    NAME raw_engine_handoff_only_never_mutates_registry_smoke
    COMMAND raw_engine_tests --case handoff_only_never_mutates_registry_smoke
)
add_test(
    NAME raw_engine_dry_run_never_mutates_registry_smoke
    COMMAND raw_engine_tests --case dry_run_never_mutates_registry_smoke
)
add_test(
    NAME raw_engine_integrated_approval_transaction_metadata_smoke
    COMMAND raw_engine_tests --case integrated_approval_transaction_metadata_smoke
)
add_test(
    NAME raw_engine_operator_runbook_replay_smoke
    COMMAND raw_engine_tests --case operator_runbook_replay_smoke
)
add_test(
    NAME raw_engine_pipeline_source_health_stage_smoke
    COMMAND raw_engine_tests --case pipeline_source_health_stage_smoke
)
add_test(
    NAME raw_engine_staged_materialization_transaction_smoke
    COMMAND raw_engine_tests --case staged_materialization_transaction_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_v15_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_v15_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_runbook_catalog_smoke
    COMMAND raw_engine_tests --case evidence_bundle_runbook_catalog_smoke
)
add_test(
    NAME raw_engine_operator_runbook_catalog_prune_smoke
    COMMAND raw_engine_tests --case operator_runbook_catalog_prune_smoke
)
add_test(
    NAME raw_engine_operator_runbook_validate_lifecycle_smoke
    COMMAND raw_engine_tests --case operator_runbook_validate_lifecycle_smoke
)
add_test(
    NAME raw_engine_integrated_approval_mutation_audit_dry_run_smoke
    COMMAND raw_engine_tests --case integrated_approval_mutation_audit_dry_run_smoke
)
add_test(
    NAME raw_engine_integrated_approval_mutation_audit_handoff_only_smoke
    COMMAND raw_engine_tests --case integrated_approval_mutation_audit_handoff_only_smoke
)
add_test(
    NAME raw_engine_source_health_plan_smoke
    COMMAND raw_engine_tests --case source_health_plan_smoke
)
add_test(
    NAME raw_engine_pipeline_source_health_required_stage_smoke
    COMMAND raw_engine_tests --case pipeline_source_health_required_stage_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_v16_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_v16_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_runbook_retention_smoke
    COMMAND raw_engine_tests --case evidence_bundle_runbook_retention_smoke
)
add_test(
    NAME raw_engine_operator_runbook_pointer_audit_smoke
    COMMAND raw_engine_tests --case operator_runbook_pointer_audit_smoke
)
add_test(
    NAME raw_engine_operator_runbook_provenance_migrate_smoke
    COMMAND raw_engine_tests --case operator_runbook_provenance_migrate_smoke
)
add_test(
    NAME raw_engine_operator_runbook_lifecycle_after_migration_smoke
    COMMAND raw_engine_tests --case operator_runbook_lifecycle_after_migration_smoke
)
add_test(
    NAME raw_engine_operator_artifact_path_policy_lint_smoke
    COMMAND raw_engine_tests --case operator_artifact_path_policy_lint_smoke
)
add_test(
    NAME raw_engine_operator_runbook_catalog_prune_after_migration_smoke
    COMMAND raw_engine_tests --case operator_runbook_catalog_prune_after_migration_smoke
)
add_test(
    NAME raw_engine_operator_runbook_replay_after_migration_smoke
    COMMAND raw_engine_tests --case operator_runbook_replay_after_migration_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_v17_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_v17_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_provenance_migration_smoke
    COMMAND raw_engine_tests --case evidence_bundle_provenance_migration_smoke
)
add_test(
    NAME raw_engine_operator_archive_reference_graph_smoke
    COMMAND raw_engine_tests --case operator_archive_reference_graph_smoke
)
add_test(
    NAME raw_engine_operator_waiver_review_smoke
    COMMAND raw_engine_tests --case operator_waiver_review_smoke
)
add_test(
    NAME raw_engine_verification_preflight_gate_smoke
    COMMAND raw_engine_tests --case verification_preflight_gate_smoke
)
add_test(
    NAME raw_engine_staged_verification_retention_smoke
    COMMAND raw_engine_tests --case staged_verification_retention_smoke
)
add_test(
    NAME raw_engine_operator_artifact_path_policy_lint_v2_smoke
    COMMAND raw_engine_tests --case operator_artifact_path_policy_lint_v2_smoke
)
add_test(
    NAME raw_engine_policy_ops_summary_v18_smoke
    COMMAND raw_engine_tests --case policy_ops_summary_v18_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_archive_governance_smoke
    COMMAND raw_engine_tests --case evidence_bundle_archive_governance_smoke
)
add_test(NAME raw_engine_operator_archive_age_tick_smoke COMMAND raw_engine_tests --case operator_archive_age_tick_smoke)
add_test(NAME raw_engine_operator_archive_waiver_due_soon_smoke COMMAND raw_engine_tests --case operator_archive_waiver_due_soon_smoke)
add_test(NAME raw_engine_operator_archive_waiver_expired_smoke COMMAND raw_engine_tests --case operator_archive_waiver_expired_smoke)
add_test(NAME raw_engine_operator_archive_snapshot_prune_candidate_smoke COMMAND raw_engine_tests --case operator_archive_snapshot_prune_candidate_smoke)
add_test(NAME raw_engine_operator_archive_snapshot_prune_blocked_smoke COMMAND raw_engine_tests --case operator_archive_snapshot_prune_blocked_smoke)
add_test(NAME raw_engine_operator_waiver_apply_review_smoke COMMAND raw_engine_tests --case operator_waiver_apply_review_smoke)
add_test(NAME raw_engine_operator_waiver_apply_expire_smoke COMMAND raw_engine_tests --case operator_waiver_apply_expire_smoke)
add_test(NAME raw_engine_operator_waiver_apply_retire_smoke COMMAND raw_engine_tests --case operator_waiver_apply_retire_smoke)
add_test(NAME raw_engine_operator_waiver_apply_blocked_active_smoke COMMAND raw_engine_tests --case operator_waiver_apply_blocked_active_smoke)
add_test(NAME raw_engine_staged_overlay_completeness_smoke COMMAND raw_engine_tests --case staged_overlay_completeness_smoke)
add_test(NAME raw_engine_staged_overlay_completeness_missing_budget_json_smoke COMMAND raw_engine_tests --case staged_overlay_completeness_missing_budget_json_smoke)
add_test(NAME raw_engine_staged_overlay_completeness_missing_policy_summary_smoke COMMAND raw_engine_tests --case staged_overlay_completeness_missing_policy_summary_smoke)
add_test(NAME raw_engine_staged_overlay_completeness_current_tools_smoke COMMAND raw_engine_tests --case staged_overlay_completeness_current_tools_smoke)
add_test(NAME raw_engine_verification_preflight_gate_v2_smoke COMMAND raw_engine_tests --case verification_preflight_gate_v2_smoke)
add_test(NAME raw_engine_verification_preflight_gate_v2_missing_overlay_smoke COMMAND raw_engine_tests --case verification_preflight_gate_v2_missing_overlay_smoke)
add_test(NAME raw_engine_verification_preflight_gate_v2_sparse_allowed_smoke COMMAND raw_engine_tests --case verification_preflight_gate_v2_sparse_allowed_smoke)
add_test(NAME raw_engine_verification_preflight_gate_v2_fail_smoke COMMAND raw_engine_tests --case verification_preflight_gate_v2_fail_smoke)
add_test(NAME raw_engine_staged_materialization_transaction_v2_smoke COMMAND raw_engine_tests --case staged_materialization_transaction_v2_smoke)
add_test(NAME raw_engine_staged_materialization_transaction_v2_overlay_count_smoke COMMAND raw_engine_tests --case staged_materialization_transaction_v2_overlay_count_smoke)
add_test(NAME raw_engine_published_snapshot_retention_apply_v2_smoke COMMAND raw_engine_tests --case published_snapshot_retention_apply_v2_smoke)
add_test(NAME raw_engine_staged_verification_retention_apply_v2_smoke COMMAND raw_engine_tests --case staged_verification_retention_apply_v2_smoke)
add_test(NAME raw_engine_published_snapshot_retention_blocked_by_waiver_smoke COMMAND raw_engine_tests --case published_snapshot_retention_blocked_by_waiver_smoke)
add_test(NAME raw_engine_policy_ops_summary_v19_smoke COMMAND raw_engine_tests --case policy_ops_summary_v19_smoke)
add_test(NAME raw_engine_policy_ops_summary_archive_queue_smoke COMMAND raw_engine_tests --case policy_ops_summary_archive_queue_smoke)
add_test(NAME raw_engine_policy_ops_summary_overlay_completeness_smoke COMMAND raw_engine_tests --case policy_ops_summary_overlay_completeness_smoke)
add_test(NAME raw_engine_policy_ops_summary_preflight_v2_smoke COMMAND raw_engine_tests --case policy_ops_summary_preflight_v2_smoke)
add_test(NAME raw_engine_evidence_bundle_archive_timeline_smoke COMMAND raw_engine_tests --case evidence_bundle_archive_timeline_smoke)
add_test(NAME raw_engine_light_ops_bundle_archive_timeline_smoke COMMAND raw_engine_tests --case light_ops_bundle_archive_timeline_smoke)
add_test(NAME raw_engine_bundle_metadata_preflight_v2_smoke COMMAND raw_engine_tests --case bundle_metadata_preflight_v2_smoke)
add_test(NAME raw_engine_runtime_comparability_triage_smoke COMMAND raw_engine_tests --case runtime_comparability_triage_smoke)
add_test(NAME raw_engine_runtime_rebaseline_action_plan_smoke COMMAND raw_engine_tests --case runtime_rebaseline_action_plan_smoke)
add_test(NAME raw_engine_runtime_rebaseline_runbook_smoke COMMAND raw_engine_tests --case runtime_rebaseline_runbook_smoke)
add_test(NAME raw_engine_runtime_registry_selection_sanity_smoke COMMAND raw_engine_tests --case runtime_registry_selection_sanity_smoke)
add_test(NAME raw_engine_pipeline_runtime_not_comparable_separated_from_correctness_smoke COMMAND raw_engine_tests --case pipeline_runtime_not_comparable_separated_from_correctness_smoke)
add_test(NAME raw_engine_policy_ops_summary_v20_smoke COMMAND raw_engine_tests --case policy_ops_summary_v20_smoke)
add_test(NAME raw_engine_evidence_bundle_runtime_triage_smoke COMMAND raw_engine_tests --case evidence_bundle_runtime_triage_smoke)
add_test(
    NAME raw_engine_evidence_bundle_publication_health_smoke
    COMMAND raw_engine_tests --case evidence_bundle_publication_health_smoke
)
add_test(
    NAME raw_engine_light_ops_bundle_operator_summary_v4_smoke
    COMMAND raw_engine_tests --case light_ops_bundle_operator_summary_v4_smoke
)
set_tests_properties(
    raw_engine_runtime_import_external_bundle_smoke
    raw_engine_runtime_import_external_new_fingerprint_smoke
    raw_engine_runtime_import_external_same_fingerprint_append_smoke
    raw_engine_runtime_import_external_malformed_bundle_smoke
    raw_engine_runtime_new_env_proposal_gate_smoke
    raw_engine_runtime_new_env_need_more_samples_smoke
    raw_engine_runtime_new_env_approvable_smoke
    raw_engine_runtime_new_env_reject_hard_breach_smoke
    raw_engine_runtime_approve_new_env_baseline_smoke
    raw_engine_runtime_registry_keep_current_env_active_smoke
    raw_engine_runtime_registry_multi_fingerprint_active_smoke
    raw_engine_runtime_approve_new_env_reject_without_gate_smoke
    raw_engine_runtime_refresh_after_new_env_approval_smoke
    raw_engine_runtime_rerun_after_new_env_approval_smoke
    raw_engine_policy_ops_summary_three_way_smoke
    raw_engine_policy_ops_summary_current_vs_known_vs_foreign_smoke
    raw_engine_evidence_bundle_new_env_onboarding_smoke
    raw_engine_light_ops_bundle_registry_summary_smoke
    raw_engine_bundle_metadata_multi_active_registry_smoke
    raw_engine_runtime_registry_health_smoke
    raw_engine_runtime_known_env_stale_smoke
    raw_engine_runtime_known_env_reverify_required_smoke
    raw_engine_runtime_known_env_retire_candidate_smoke
    raw_engine_runtime_known_env_state_machine_smoke
    raw_engine_runtime_import_known_env_evidence_smoke
    raw_engine_runtime_known_env_reverify_gate_smoke
    raw_engine_runtime_known_env_plan_retire_smoke
    raw_engine_runtime_known_env_retired_smoke
    raw_engine_runtime_registry_health_v2_smoke
    raw_engine_runtime_registry_same_fingerprint_supersession_smoke
    raw_engine_runtime_registry_retired_not_selected_smoke
    raw_engine_runtime_foreign_env_state_machine_smoke
    raw_engine_publication_health_smoke
    raw_engine_policy_ops_summary_v4_smoke
    raw_engine_policy_ops_summary_v5_smoke
    raw_engine_policy_ops_summary_known_vs_foreign_smoke
    raw_engine_policy_ops_summary_publication_health_smoke
    raw_engine_evidence_bundle_registry_health_smoke
    raw_engine_evidence_bundle_known_env_governance_smoke
    raw_engine_evidence_bundle_publication_health_smoke
    raw_engine_light_ops_bundle_operator_summary_v4_smoke
    PROPERTIES RUN_SERIAL TRUE
)
if(RAW_ENGINE_REGISTER_NIGHTLY_TESTS)
add_test(NAME raw_engine_pipeline_matrix_smoke COMMAND raw_engine_tests --case pipeline_matrix_smoke)
add_test(
    NAME raw_engine_pipeline_matrix_mixed_fingerprints_smoke
    COMMAND raw_engine_tests --case pipeline_matrix_mixed_fingerprints_smoke
)
add_test(
    NAME raw_engine_pipeline_nightly_rebaseline_candidate_smoke
    COMMAND raw_engine_tests --case pipeline_nightly_rebaseline_candidate_smoke
)
endif()
add_test(NAME raw_engine_policy_roundtrip_smoke COMMAND raw_engine_tests --case policy_roundtrip_smoke)
add_test(NAME raw_engine_policy_empty_plan_noop_smoke COMMAND raw_engine_tests --case policy_empty_plan_noop_smoke)
add_test(NAME raw_engine_policy_manifest_missing_input_smoke COMMAND raw_engine_tests --case policy_manifest_missing_input_smoke)
add_test(
    NAME raw_engine_evidence_bundle_runtime_manifest_smoke
    COMMAND raw_engine_tests --case evidence_bundle_runtime_manifest_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_runtime_registry_smoke
    COMMAND raw_engine_tests --case evidence_bundle_runtime_registry_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_runtime_history_smoke
    COMMAND raw_engine_tests --case evidence_bundle_runtime_history_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_runtime_registry_snapshot_smoke
    COMMAND raw_engine_tests --case evidence_bundle_runtime_registry_snapshot_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_runtime_history_snapshot_smoke
    COMMAND raw_engine_tests --case evidence_bundle_runtime_history_snapshot_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_runtime_proposal_smoke
    COMMAND raw_engine_tests --case evidence_bundle_runtime_proposal_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_runtime_watch_metadata_smoke
    COMMAND raw_engine_tests --case evidence_bundle_runtime_watch_metadata_smoke
)
if(RAW_ENGINE_REGISTER_NIGHTLY_TESTS)
add_test(
    NAME raw_engine_evidence_bundle_watch_history_smoke
    COMMAND raw_engine_tests --case evidence_bundle_watch_history_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_watch_transition_smoke
    COMMAND raw_engine_tests --case evidence_bundle_watch_transition_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_multi_fingerprint_watch_smoke
    COMMAND raw_engine_tests --case evidence_bundle_multi_fingerprint_watch_smoke
)
endif()
add_test(
    NAME raw_engine_evidence_bundle_combined_summary_smoke
    COMMAND raw_engine_tests --case evidence_bundle_combined_summary_smoke
)
add_test(
    NAME raw_engine_evidence_bundle_light_ops_smoke
    COMMAND raw_engine_tests --case evidence_bundle_light_ops_smoke
)
add_test(NAME raw_engine_ops_guide_paths_smoke COMMAND raw_engine_tests --case ops_guide_paths_smoke)
if(RAW_ENGINE_REGISTER_NIGHTLY_TESTS)
add_test(NAME raw_engine_campaign_manifest_parallel_smoke COMMAND raw_engine_tests --case campaign_manifest_parallel_smoke)
add_test(NAME raw_engine_artifact_dir_uniqueness_smoke COMMAND raw_engine_tests --case artifact_dir_uniqueness_smoke)
add_test(NAME raw_engine_policy_pipeline_parallel_smoke COMMAND raw_engine_tests --case policy_pipeline_parallel_smoke)
add_test(NAME raw_engine_transient_repro_smoke COMMAND raw_engine_tests --case transient_repro_smoke)
endif()
add_test(
    NAME raw_engine_policy_selective_rerun_n_family_smoke
    COMMAND raw_engine_tests --case policy_selective_rerun_n_family_smoke
)
add_test(NAME raw_engine_compare_evidence_threshold_smoke COMMAND raw_engine_tests --case compare_evidence_threshold_smoke)
add_test(NAME raw_engine_exhaustive_split_ready_smoke COMMAND raw_engine_tests --case exhaustive_split_ready_smoke)
add_test(NAME raw_engine_exhaustive_join_ready_smoke COMMAND raw_engine_tests --case exhaustive_join_ready_smoke)
add_test(NAME raw_engine_exhaustive_integrate_ready_smoke COMMAND raw_engine_tests --case exhaustive_integrate_ready_smoke)
add_test(NAME raw_engine_exhaustive_mixed_smoke COMMAND raw_engine_tests --case exhaustive_mixed_smoke)
add_test(NAME raw_engine_planner_tie_mixed_exhaustive_smoke COMMAND raw_engine_tests --case planner_tie_mixed_exhaustive_smoke)
add_test(NAME raw_engine_exhaustive_canonical_dedupe_smoke COMMAND raw_engine_tests --case exhaustive_canonical_dedupe_smoke)
add_test(NAME raw_engine_exhaustive_natural_dedupe_smoke COMMAND raw_engine_tests --case exhaustive_natural_dedupe_smoke)
add_test(NAME raw_engine_exhaustive_family_sweep_smoke COMMAND raw_engine_tests --case exhaustive_family_sweep_smoke)
add_test(NAME raw_engine_exhaustive_collision_guard_smoke COMMAND raw_engine_tests --case exhaustive_collision_guard_smoke)
add_test(NAME raw_engine_exact_canonicalizer_smoke COMMAND raw_engine_tests --case exact_canonicalizer_smoke)
add_test(NAME raw_engine_metamorphic_relabel_invariance COMMAND raw_engine_tests --case metamorphic_relabel_invariance)
add_test(NAME raw_engine_metamorphic_occid_invariance COMMAND raw_engine_tests --case metamorphic_occid_invariance)
add_test(NAME raw_engine_metamorphic_edge_order_invariance COMMAND raw_engine_tests --case metamorphic_edge_order_invariance)
add_test(NAME raw_engine_metamorphic_vertex_order_invariance COMMAND raw_engine_tests --case metamorphic_vertex_order_invariance)
add_test(NAME raw_engine_replay_serialization_invariance COMMAND raw_engine_tests --case replay_serialization_invariance)
add_test(NAME raw_engine_metamorphic_family_matrix_smoke COMMAND raw_engine_tests --case metamorphic_family_matrix_smoke)
add_test(NAME raw_engine_metamorphic_planner_multistep_smoke COMMAND raw_engine_tests --case metamorphic_planner_multistep_smoke)
add_test(NAME raw_engine_metamorphic_replay_matrix_smoke COMMAND raw_engine_tests --case metamorphic_replay_matrix_smoke)
add_test(NAME raw_engine_split_choice_oracle_smoke COMMAND raw_engine_tests --case split_choice_oracle_smoke)
add_test(NAME raw_engine_split_choice_relabel_invariance COMMAND raw_engine_tests --case split_choice_relabel_invariance)
add_test(NAME raw_engine_split_choice_edge_order_invariance COMMAND raw_engine_tests --case split_choice_edge_order_invariance)
add_test(NAME raw_engine_split_choice_vertex_order_invariance COMMAND raw_engine_tests --case split_choice_vertex_order_invariance)
add_test(NAME raw_engine_split_choice_oracle_regression COMMAND raw_engine_tests --case split_choice_oracle_regression)
add_test(NAME raw_engine_split_choice_policy_smoke COMMAND raw_engine_tests --case split_choice_policy_smoke)
add_test(NAME raw_engine_split_choice_policy_relabel_invariance COMMAND raw_engine_tests --case split_choice_policy_relabel_invariance)
add_test(NAME raw_engine_split_choice_policy_edge_order_invariance COMMAND raw_engine_tests --case split_choice_policy_edge_order_invariance)
add_test(NAME raw_engine_split_choice_policy_vertex_order_invariance COMMAND raw_engine_tests --case split_choice_policy_vertex_order_invariance)
add_test(NAME raw_engine_split_choice_policy_occid_invariance COMMAND raw_engine_tests --case split_choice_policy_occid_invariance)
add_test(NAME raw_engine_split_choice_policy_multiclass_smoke COMMAND raw_engine_tests --case split_choice_policy_multiclass_smoke)
add_test(NAME raw_engine_split_choice_exact_class_smoke COMMAND raw_engine_tests --case split_choice_exact_class_smoke)
add_test(NAME raw_engine_split_choice_exact_relabel_invariance COMMAND raw_engine_tests --case split_choice_exact_relabel_invariance)
add_test(NAME raw_engine_split_choice_exact_vertex_order_invariance COMMAND raw_engine_tests --case split_choice_exact_vertex_order_invariance)
add_test(NAME raw_engine_split_choice_exact_edge_order_invariance COMMAND raw_engine_tests --case split_choice_exact_edge_order_invariance)
add_test(NAME raw_engine_split_choice_representative_shift_smoke COMMAND raw_engine_tests --case split_choice_representative_shift_smoke)
add_test(NAME raw_engine_split_choice_semantic_shift_smoke COMMAND raw_engine_tests --case split_choice_semantic_shift_smoke)
add_test(NAME raw_engine_split_choice_semantic_shift_regression COMMAND raw_engine_tests --case split_choice_semantic_shift_regression)
add_test(NAME raw_engine_split_choice_policy_exact_shadow_smoke COMMAND raw_engine_tests --case split_choice_policy_exact_shadow_smoke)
add_test(NAME raw_engine_split_choice_policy_fast_vs_exact_shadow_compare COMMAND raw_engine_tests --case split_choice_policy_fast_vs_exact_shadow_compare)
add_test(NAME raw_engine_split_choice_exact_shadow_relabel_invariance COMMAND raw_engine_tests --case split_choice_exact_shadow_relabel_invariance)
add_test(NAME raw_engine_split_choice_exact_shadow_vertex_order_invariance COMMAND raw_engine_tests --case split_choice_exact_shadow_vertex_order_invariance)
add_test(NAME raw_engine_split_choice_exact_shadow_edge_order_invariance COMMAND raw_engine_tests --case split_choice_exact_shadow_edge_order_invariance)
add_test(NAME raw_engine_split_choice_fallback_zero_smoke COMMAND raw_engine_tests --case split_choice_fallback_zero_smoke)
add_test(NAME raw_engine_planner_relabel_structural_regression COMMAND raw_engine_tests --case planner_relabel_structural_regression)
add_test(NAME raw_engine_planner_targeted_split_smoke COMMAND raw_engine_tests --case planner_targeted_split_smoke)
add_test(NAME raw_engine_planner_targeted_join_smoke COMMAND raw_engine_tests --case planner_targeted_join_smoke)
add_test(NAME raw_engine_planner_targeted_integrate_smoke COMMAND raw_engine_tests --case planner_targeted_integrate_smoke)
add_test(NAME raw_engine_planner_targeted_mixed_smoke COMMAND raw_engine_tests --case planner_targeted_mixed_smoke)
add_test(NAME raw_engine_split_ready_unique_orig_regression COMMAND raw_engine_tests --case split_ready_unique_orig_regression)
add_test(NAME raw_engine_planner_coverage_smoke COMMAND raw_engine_tests --case planner_coverage_smoke)
add_test(NAME raw_engine_planner_random_coverage_smoke COMMAND raw_engine_tests --case planner_random_coverage_smoke)
add_test(NAME raw_engine_planner_weighted_coverage_smoke COMMAND raw_engine_tests --case planner_weighted_coverage_smoke)
add_test(NAME raw_engine_planner_join_ready_smoke COMMAND raw_engine_tests --case planner_join_ready_smoke)
add_test(NAME raw_engine_planner_integrate_ready_smoke COMMAND raw_engine_tests --case planner_integrate_ready_smoke)
add_test(NAME raw_engine_planner_structural_mixed_smoke COMMAND raw_engine_tests --case planner_structural_mixed_smoke)
add_test(NAME raw_engine_planner_tie_mixed_smoke COMMAND raw_engine_tests --case planner_tie_mixed_smoke)
add_test(NAME raw_engine_planner_tie_mixed_exact_shadow_smoke COMMAND raw_engine_tests --case planner_tie_mixed_exact_shadow_smoke)
add_test(NAME raw_engine_planner_tie_symmetric_smoke COMMAND raw_engine_tests --case planner_tie_symmetric_smoke)
add_test(
    NAME raw_engine_artifact_retention_smoke
    COMMAND raw_engine_tests --case artifact_retention_smoke --artifact-dir artifacts/artifact_retention_smoke
)

set(raw_engine_registered_nightly_tests)
if (RAW_ENGINE_REGISTER_NIGHTLY_TESTS)
    add_test(NAME raw_engine_exhaustive_natural_dedupe_large_smoke COMMAND raw_engine_tests --case exhaustive_natural_dedupe_large_smoke)
    add_test(NAME raw_engine_exhaustive_organic_duplicate_examples_smoke COMMAND raw_engine_tests --case exhaustive_organic_duplicate_examples_smoke)
    add_test(NAME raw_engine_fast_vs_exact_canonical_dedupe_smoke COMMAND raw_engine_tests --case fast_vs_exact_canonical_dedupe_smoke)
    add_test(NAME raw_engine_split_choice_harmless_shift_smoke COMMAND raw_engine_tests --case split_choice_harmless_shift_smoke)
    add_test(NAME raw_engine_canonical_collision_probe_smoke COMMAND raw_engine_tests --case canonical_collision_probe_smoke)
    add_test(NAME raw_engine_split_tie_organic_symmetric_smoke COMMAND raw_engine_tests --case split_tie_organic_symmetric_smoke)
    add_test(NAME raw_engine_planner_tie_mixed_organic_smoke COMMAND raw_engine_tests --case planner_tie_mixed_organic_smoke)
    add_test(NAME raw_engine_planner_tie_mixed_organic_compare_ready_smoke COMMAND raw_engine_tests --case planner_tie_mixed_organic_compare_ready_smoke)
    add_test(NAME raw_engine_automorphism_probe_large_smoke COMMAND raw_engine_tests --case automorphism_probe_large_smoke)
    add_test(NAME raw_engine_sampled_exact_audit_smoke COMMAND raw_engine_tests --case sampled_exact_audit_smoke)
    add_test(NAME raw_engine_duplicate_attribution_smoke COMMAND raw_engine_tests --case duplicate_attribution_smoke)
    add_test(
        NAME raw_engine_compare_campaign_checkpoint_smoke
        COMMAND /usr/bin/python3
            ${PROJECT_SOURCE_DIR}/tests/tools/run_ctest_staged.py
            --binary $<TARGET_FILE:raw_engine_tests>
            --stage-name compare_campaign_checkpoint_smoke
            --result-json ${CMAKE_CURRENT_BINARY_DIR}/artifacts/phase15_campaign_checkpoint_smoke/result.json
            --
            --case campaign
            --campaign-config ${PROJECT_SOURCE_DIR}/tests/campaigns/phase15_checkpoint_smoke.txt
            --checkpoint-dir ${CMAKE_CURRENT_BINARY_DIR}/artifacts/phase15_campaign_checkpoint_smoke/checkpoints
            --checkpoint-every 2
            --max-wall-seconds 1
            --stop-after-checkpoint
    )
    add_test(
        NAME raw_engine_campaign_resume_smoke
        COMMAND /usr/bin/python3
            ${PROJECT_SOURCE_DIR}/tests/tools/run_ctest_staged.py
            --binary $<TARGET_FILE:raw_engine_tests>
            --stage-name campaign_resume_smoke
            --result-json ${CMAKE_CURRENT_BINARY_DIR}/artifacts/phase15_campaign_resume_smoke/result.json
            --
            --case campaign
            --resume-from ${CMAKE_CURRENT_BINARY_DIR}/artifacts/phase15_campaign_checkpoint_smoke/checkpoints/latest.chk
    )
    add_test(
        NAME raw_engine_campaign_partial_merge_smoke
        COMMAND /usr/bin/python3
            ${PROJECT_SOURCE_DIR}/tests/tools/run_ctest_staged.py
            --binary $<TARGET_FILE:raw_engine_tests>
            --stage-name campaign_partial_merge_smoke
            --result-json ${CMAKE_CURRENT_BINARY_DIR}/artifacts/phase15_campaign_partial_merge_smoke/result.json
            --
            --case campaign
            --campaign-config ${PROJECT_SOURCE_DIR}/tests/campaigns/phase15_resume_smoke.txt
            --checkpoint-dir ${CMAKE_CURRENT_BINARY_DIR}/artifacts/phase15_campaign_partial_merge_smoke/checkpoints
            --checkpoint-every 2
            --max-wall-seconds 1
            --stop-after-checkpoint
    )
    list(APPEND raw_engine_registered_nightly_tests
        raw_engine_exhaustive_natural_dedupe_large_smoke
        raw_engine_exhaustive_organic_duplicate_examples_smoke
        raw_engine_fast_vs_exact_canonical_dedupe_smoke
        raw_engine_split_choice_harmless_shift_smoke
        raw_engine_canonical_collision_probe_smoke
        raw_engine_compare_ready_lineage_smoke
        raw_engine_split_tie_organic_symmetric_smoke
        raw_engine_planner_tie_mixed_organic_smoke
        raw_engine_planner_tie_mixed_organic_compare_ready_smoke
        raw_engine_automorphism_probe_large_smoke
        raw_engine_sampled_exact_audit_smoke
        raw_engine_duplicate_attribution_smoke
        raw_engine_compare_campaign_checkpoint_smoke
        raw_engine_campaign_resume_smoke
        raw_engine_campaign_partial_merge_smoke
    )
endif()

function(raw_engine_label_tests label)
    foreach(test_name IN ITEMS ${ARGN})
        if(TEST ${test_name})
            set_property(TEST ${test_name} APPEND PROPERTY LABELS ${label})
        endif()
    endforeach()
endfunction()

# Labels are additive. Plain `ctest` runs the full *registered* suite, while
# `ctest -L <label>` provides filtered entry points for day-to-day tiers.
# Nightly-only cases are only registered when RAW_ENGINE_REGISTER_NIGHTLY_TESTS=ON.
raw_engine_label_tests(core
    raw_engine_micro
    raw_engine_roundtrip
    raw_engine_split_integrate_ensure
    raw_engine_regression_44001
    raw_engine_regression_isolate_split_no_sep
    raw_engine_planner
    raw_engine_isolate_oracle
    raw_engine_split_oracle
    raw_engine_join_oracle
    raw_engine_integrate_oracle
    raw_engine_planner_oracle
    raw_engine_replay_state
    raw_engine_reducer_smoke
    raw_engine_replay_planner_state
    raw_engine_reduce_planner_state_smoke
    raw_engine_failure_signature_smoke
    raw_engine_planner_failure_signature_smoke
    raw_engine_reduce_planner_state_step_budget
    raw_engine_reduce_planner_state_trace_prefix
    raw_engine_primitive_fault_detection_smoke
    raw_engine_planner_fault_detection_smoke
    raw_engine_mutation_matrix_smoke
    raw_engine_planner_fixpoint_idempotence
    raw_engine_planner_replay_determinism
    raw_engine_reducer_determinism_smoke
    raw_engine_corpus_roundtrip_smoke
    raw_engine_corpus_replay_smoke
    raw_engine_campaign_phase16_manifest_compatibility_smoke
    raw_engine_split_choice_policy_graduation_smoke
    raw_engine_policy_graduation_gate_smoke
    raw_engine_policy_gate_manifest_smoke
    raw_engine_policy_refresh_smoke
    raw_engine_policy_refresh_family_filter_smoke
    raw_engine_policy_gate_family_filter_smoke
    raw_engine_policy_gate_non_applicable_smoke
    raw_engine_policy_gate_non_applicable_drift_smoke
    raw_engine_planner_tie_organic_applicability_drift_smoke
    raw_engine_planner_tie_organic_reclassify_trigger_smoke
    raw_engine_compare_ready_lineage_gate_smoke
    raw_engine_compare_ready_not_counted_as_pass_smoke
    raw_engine_policy_gate_diagnostic_only_smoke
    raw_engine_compare_ready_not_promoted_smoke
    raw_engine_policy_gate_nightly_smoke
    raw_engine_policy_refresh_nightly_smoke
    raw_engine_manifest_roundtrip_smoke
    raw_engine_evidence_bundle_smoke
    raw_engine_evidence_bundle_manifest_freshness_smoke
    raw_engine_stale_family_selective_rerun_smoke
    raw_engine_compare_evidence_threshold_smoke
    raw_engine_exact_canonicalizer_smoke
    raw_engine_metamorphic_relabel_invariance
    raw_engine_metamorphic_occid_invariance
    raw_engine_metamorphic_edge_order_invariance
    raw_engine_metamorphic_vertex_order_invariance
    raw_engine_replay_serialization_invariance
    raw_engine_split_choice_relabel_invariance
    raw_engine_split_choice_edge_order_invariance
    raw_engine_split_choice_vertex_order_invariance
    raw_engine_split_choice_oracle_regression
    raw_engine_split_choice_policy_smoke
    raw_engine_split_choice_policy_relabel_invariance
    raw_engine_split_choice_policy_edge_order_invariance
    raw_engine_split_choice_policy_vertex_order_invariance
    raw_engine_split_choice_policy_occid_invariance
    raw_engine_split_choice_policy_multiclass_smoke
    raw_engine_split_choice_exact_relabel_invariance
    raw_engine_split_choice_exact_vertex_order_invariance
    raw_engine_split_choice_exact_edge_order_invariance
    raw_engine_split_choice_semantic_shift_regression
    raw_engine_split_choice_policy_exact_shadow_smoke
    raw_engine_split_choice_exact_shadow_relabel_invariance
    raw_engine_split_choice_exact_shadow_vertex_order_invariance
    raw_engine_split_choice_exact_shadow_edge_order_invariance
    raw_engine_split_choice_fallback_zero_smoke
    raw_engine_planner_relabel_structural_regression
    raw_engine_planner_targeted_split_smoke
    raw_engine_planner_targeted_join_smoke
    raw_engine_planner_targeted_integrate_smoke
    raw_engine_planner_targeted_mixed_smoke
    raw_engine_split_ready_unique_orig_regression
)

raw_engine_label_tests(slow
    raw_engine_fuzz
    raw_engine_planner_oracle_fuzz_stats
    raw_engine_exhaustive_split_ready_smoke
    raw_engine_exhaustive_join_ready_smoke
    raw_engine_exhaustive_integrate_ready_smoke
    raw_engine_exhaustive_mixed_smoke
    raw_engine_exhaustive_canonical_dedupe_smoke
    raw_engine_exhaustive_natural_dedupe_smoke
    raw_engine_exhaustive_collision_guard_smoke
    raw_engine_planner_tie_mixed_exhaustive_smoke
    raw_engine_exhaustive_family_sweep_smoke
    raw_engine_metamorphic_family_matrix_smoke
    raw_engine_metamorphic_planner_multistep_smoke
    raw_engine_metamorphic_replay_matrix_smoke
    raw_engine_split_choice_oracle_smoke
    raw_engine_compare_ready_lineage_smoke
    raw_engine_planner_tie_organic_applicability_smoke
    raw_engine_planner_tie_organic_proxy_pass_smoke
    raw_engine_split_choice_exact_class_smoke
    raw_engine_split_choice_representative_shift_smoke
    raw_engine_split_choice_semantic_shift_smoke
    raw_engine_split_choice_policy_fast_vs_exact_shadow_compare
    raw_engine_planner_tie_mixed_exact_shadow_smoke
    raw_engine_planner_coverage_smoke
    raw_engine_planner_join_ready_smoke
    raw_engine_planner_integrate_ready_smoke
    raw_engine_planner_structural_mixed_smoke
    raw_engine_planner_tie_mixed_smoke
    raw_engine_planner_tie_symmetric_smoke
    raw_engine_artifact_retention_smoke
)
if (raw_engine_registered_nightly_tests)
    raw_engine_label_tests(slow
        raw_engine_split_choice_harmless_shift_smoke
        raw_engine_split_tie_organic_symmetric_smoke
        raw_engine_planner_tie_mixed_organic_smoke
        raw_engine_planner_tie_mixed_organic_compare_ready_smoke
        raw_engine_sampled_exact_audit_smoke
        raw_engine_duplicate_attribution_smoke
    )
endif()

raw_engine_label_tests(exhaustive
    raw_engine_exhaustive_split_ready_smoke
    raw_engine_exhaustive_join_ready_smoke
    raw_engine_exhaustive_integrate_ready_smoke
    raw_engine_exhaustive_mixed_smoke
    raw_engine_planner_tie_mixed_exhaustive_smoke
    raw_engine_exhaustive_canonical_dedupe_smoke
    raw_engine_exhaustive_natural_dedupe_smoke
    raw_engine_exhaustive_family_sweep_smoke
    raw_engine_exhaustive_collision_guard_smoke
)
if (raw_engine_registered_nightly_tests)
    raw_engine_label_tests(exhaustive
        raw_engine_exhaustive_natural_dedupe_large_smoke
        raw_engine_exhaustive_organic_duplicate_examples_smoke
        raw_engine_canonical_collision_probe_smoke
        raw_engine_automorphism_probe_large_smoke
    )
endif()

raw_engine_label_tests(fuzz
    raw_engine_fuzz_matrix
    raw_engine_fuzz
    raw_engine_planner_oracle_fuzz
    raw_engine_planner_oracle_fuzz_stats
    raw_engine_corpus_roundtrip_smoke
    raw_engine_corpus_replay_smoke
    raw_engine_planner_random_coverage_smoke
    raw_engine_planner_weighted_coverage_smoke
)

raw_engine_label_tests(compare
    raw_engine_campaign_phase16_resume_gate_smoke
    raw_engine_campaign_phase16_manifest_compatibility_smoke
    raw_engine_split_choice_policy_graduation_smoke
    raw_engine_policy_graduation_gate_smoke
    raw_engine_policy_gate_manifest_smoke
    raw_engine_policy_refresh_smoke
    raw_engine_policy_refresh_family_filter_smoke
    raw_engine_policy_refresh_stale_hash_smoke
    raw_engine_policy_refresh_compare_engine_hash_smoke
    raw_engine_policy_refresh_generator_hash_smoke
    raw_engine_policy_refresh_campaign_config_hash_smoke
    raw_engine_policy_gate_family_filter_smoke
    raw_engine_policy_gate_non_applicable_smoke
    raw_engine_policy_gate_non_applicable_drift_smoke
    raw_engine_planner_tie_organic_applicability_drift_smoke
    raw_engine_planner_tie_organic_reclassify_trigger_smoke
    raw_engine_policy_reclassify_non_applicable_smoke
    raw_engine_compare_ready_lineage_gate_smoke
    raw_engine_compare_ready_not_counted_as_pass_smoke
    raw_engine_policy_gate_diagnostic_only_smoke
    raw_engine_compare_ready_not_promoted_smoke
    raw_engine_policy_reject_diagnostic_promotion_smoke
    raw_engine_policy_gate_nightly_smoke
    raw_engine_policy_refresh_nightly_smoke
    raw_engine_manifest_roundtrip_smoke
    raw_engine_evidence_bundle_smoke
    raw_engine_evidence_bundle_manifest_freshness_smoke
    raw_engine_evidence_bundle_idempotent_smoke
    raw_engine_evidence_bundle_policy_summary_smoke
    raw_engine_stale_family_selective_rerun_smoke
    raw_engine_policy_rerun_plan_mode_selection_smoke
    raw_engine_policy_selective_rerun_execute_smoke
    raw_engine_policy_selective_rerun_noop_smoke
    raw_engine_policy_baseline_history_smoke
    raw_engine_policy_baseline_archive_roundtrip_smoke
    raw_engine_policy_ci_check_smoke
    raw_engine_policy_nightly_refresh_smoke
    raw_engine_policy_severity_smoke
    raw_engine_pipeline_exit_code_smoke
    raw_engine_stale_warn_mode_smoke
    raw_engine_action_required_mode_smoke
    raw_engine_runtime_budget_smoke
    raw_engine_runtime_budget_manifest_roundtrip_smoke
    raw_engine_runtime_fingerprint_roundtrip_smoke
    raw_engine_runtime_not_comparable_smoke
    raw_engine_runtime_refresh_same_fingerprint_smoke
    raw_engine_runtime_baseline_promote_smoke
    raw_engine_runtime_refresh_against_promoted_baseline_smoke
    raw_engine_runtime_rerun_plan_smoke
    raw_engine_runtime_rerun_plan_empty_smoke
    raw_engine_runtime_proposal_gate_smoke
    raw_engine_runtime_proposal_gate_need_more_samples_smoke
    raw_engine_runtime_proposal_gate_approvable_smoke
    raw_engine_runtime_proposal_gate_reject_hard_breach_smoke
    raw_engine_runtime_approve_rebaseline_smoke
    raw_engine_runtime_approve_rebaseline_from_gate_smoke
    raw_engine_runtime_approve_rebaseline_registry_switch_smoke
    raw_engine_runtime_registry_active_switch_phase30_smoke
    raw_engine_runtime_approve_rebaseline_archive_proposal_smoke
    raw_engine_runtime_approve_rebaseline_reject_without_gate_smoke
    raw_engine_runtime_refresh_after_rebaseline_smoke
    raw_engine_runtime_refresh_after_phase30_approval_smoke
    raw_engine_runtime_budget_proposal_gate_smoke
    raw_engine_runtime_budget_approve_reprofile_smoke
    raw_engine_runtime_budget_refresh_after_approval_smoke
    raw_engine_runtime_current_env_age_tick_smoke
    raw_engine_runtime_current_env_plan_watch_smoke
    raw_engine_runtime_current_env_reproposal_trigger_gate_smoke
    raw_engine_runtime_current_env_due_scheduler_smoke
    raw_engine_runtime_current_env_time_travel_due_now_smoke
    raw_engine_runtime_current_env_plan_reproposal_smoke
    raw_engine_policy_ops_agenda_smoke
    raw_engine_runtime_current_env_execute_watch_smoke
    raw_engine_runtime_current_env_apply_watch_smoke
    raw_engine_current_env_monitoring_due_execute_apply_smoke
    raw_engine_runtime_current_env_execute_reproposal_gate_smoke
    raw_engine_policy_ops_agenda_action_status_smoke
    raw_engine_runtime_current_env_action_ledger_smoke
    raw_engine_runtime_current_env_action_retry_plan_smoke
    raw_engine_runtime_current_env_reproposal_handoff_smoke
    raw_engine_current_env_next_cycle_monitoring_due_smoke
    raw_engine_policy_ops_agenda_lifecycle_v2_smoke
    raw_engine_runtime_current_env_operator_decision_smoke
    raw_engine_runtime_current_env_apply_operator_decision_smoke
    raw_engine_runtime_current_env_action_ledger_compact_smoke
    raw_engine_reproposal_handoff_closure_only_smoke
    raw_engine_policy_ops_agenda_lifecycle_v3_smoke
    raw_engine_runtime_current_env_approval_runbook_smoke
    raw_engine_runtime_current_env_execute_budget_approval_dry_run_smoke
    raw_engine_runtime_current_env_link_approval_execution_smoke
    raw_engine_runtime_current_env_decision_skip_flow_smoke
    raw_engine_runtime_current_env_decision_defer_flow_smoke
    raw_engine_runtime_current_env_decision_retry_now_flow_smoke
    raw_engine_policy_ops_agenda_lifecycle_v4_smoke
    raw_engine_runtime_budget_history_append_smoke
    raw_engine_combined_pipeline_after_budget_reprofile_quick_smoke
    raw_engine_combined_pipeline_after_budget_reprofile_nightly_smoke
    raw_engine_policy_ops_summary_budget_vs_rebaseline_smoke
    raw_engine_policy_ops_summary_v8_smoke
    raw_engine_policy_ops_summary_v9_smoke
    raw_engine_policy_ops_summary_v10_smoke
    raw_engine_policy_ops_summary_v11_smoke
    raw_engine_policy_ops_summary_v12_smoke
    raw_engine_policy_ops_summary_v13_smoke
    raw_engine_operator_runbook_index_smoke
    raw_engine_approval_execution_dry_run_smoke
    raw_engine_approval_execution_handoff_only_smoke
    raw_engine_approval_execution_integrated_requires_optin_smoke
    raw_engine_approval_execution_integrated_optin_fixture_smoke
    raw_engine_current_env_decision_skip_e2e_smoke
    raw_engine_current_env_decision_defer_e2e_smoke
    raw_engine_current_env_decision_reject_e2e_smoke
    raw_engine_current_env_decision_retry_now_e2e_smoke
    raw_engine_action_ledger_closure_invariants_smoke
    raw_engine_source_health_preflight_smoke
    raw_engine_staged_sparse_clone_overlay_smoke
    raw_engine_policy_ops_summary_v14_smoke
    raw_engine_evidence_bundle_runbook_index_smoke
    raw_engine_operator_runbook_catalog_smoke
    raw_engine_operator_decision_metadata_audit_smoke
    raw_engine_integrated_approval_requires_confirmation_smoke
    raw_engine_integrated_approval_rejects_without_preflight_smoke
    raw_engine_handoff_only_never_mutates_registry_smoke
    raw_engine_dry_run_never_mutates_registry_smoke
    raw_engine_integrated_approval_transaction_metadata_smoke
    raw_engine_operator_runbook_replay_smoke
    raw_engine_pipeline_source_health_stage_smoke
    raw_engine_staged_materialization_transaction_smoke
    raw_engine_policy_ops_summary_v15_smoke
    raw_engine_evidence_bundle_runbook_catalog_smoke
    raw_engine_policy_ops_summary_current_env_clear_new_env_action_required_smoke
    raw_engine_evidence_bundle_budget_lifecycle_smoke
    raw_engine_evidence_bundle_budget_transition_metadata_smoke
    raw_engine_evidence_bundle_current_env_guardrail_timeline_smoke
    raw_engine_evidence_bundle_current_env_due_timeline_smoke
    raw_engine_evidence_bundle_current_env_execute_apply_smoke
    raw_engine_evidence_bundle_action_ledger_smoke
    raw_engine_evidence_bundle_operator_decision_smoke
    raw_engine_evidence_bundle_approval_runbook_smoke
    raw_engine_runtime_rerun_plan_after_rebaseline_smoke
    raw_engine_runtime_rerun_plan_after_phase30_approval_smoke
    raw_engine_runtime_selection_after_registry_switch_smoke
    raw_engine_runtime_synthetic_warn_smoke
    raw_engine_runtime_synthetic_action_required_smoke
    raw_engine_runtime_synthetic_fail_smoke
    raw_engine_runtime_missing_baseline_smoke
    raw_engine_runtime_fingerprint_mismatch_smoke
    raw_engine_runtime_registry_multi_fingerprint_smoke
    raw_engine_runtime_registry_same_host_compiler_bump_smoke
    raw_engine_runtime_registry_sanitizer_change_smoke
    raw_engine_runtime_registry_runner_tag_change_smoke
    raw_engine_runtime_registry_cross_host_smoke
    raw_engine_runtime_history_stable_smoke
    raw_engine_runtime_history_noisy_smoke
    raw_engine_runtime_history_regressing_smoke
    raw_engine_runtime_history_recovery_after_rebaseline_smoke
    raw_engine_runtime_recommendation_watch_smoke
    raw_engine_runtime_recommendation_rebaseline_required_smoke
    raw_engine_runtime_recommendation_not_comparable_smoke
    raw_engine_combined_pipeline_recommendation_smoke
    raw_engine_runtime_rebaseline_proposal_multi_candidate_smoke
    raw_engine_runtime_rebaseline_proposal_retired_only_smoke
    raw_engine_runtime_rebaseline_proposal_cross_host_smoke
    raw_engine_pipeline_matrix_smoke
    raw_engine_pipeline_matrix_mixed_fingerprints_smoke
    raw_engine_pipeline_nightly_rebaseline_candidate_smoke
    raw_engine_combined_pipeline_after_rebaseline_quick_smoke
    raw_engine_combined_pipeline_after_phase30_rebaseline_quick_smoke
    raw_engine_combined_pipeline_after_rebaseline_nightly_smoke
    raw_engine_combined_pipeline_after_phase30_rebaseline_nightly_smoke
    raw_engine_runtime_history_append_after_rebaseline_smoke
    raw_engine_runtime_history_append_after_phase30_rebaseline_smoke
    raw_engine_runtime_registry_history_transition_smoke
    raw_engine_runtime_registry_history_transition_phase30_smoke
    raw_engine_combined_policy_runtime_ok_smoke
    raw_engine_combined_policy_runtime_warn_smoke
    raw_engine_combined_policy_runtime_action_required_smoke
    raw_engine_combined_policy_runtime_fail_smoke
    raw_engine_artifact_index_smoke
    raw_engine_artifact_prune_smoke
    raw_engine_bundle_index_roundtrip_smoke
    raw_engine_policy_roundtrip_smoke
    raw_engine_policy_empty_plan_noop_smoke
    raw_engine_policy_manifest_missing_input_smoke
    raw_engine_policy_selective_rerun_n_family_smoke
    raw_engine_compare_evidence_threshold_smoke
    raw_engine_split_choice_oracle_smoke
    raw_engine_split_choice_exact_class_smoke
    raw_engine_split_choice_representative_shift_smoke
    raw_engine_split_choice_semantic_shift_smoke
    raw_engine_split_choice_policy_fast_vs_exact_shadow_compare
    raw_engine_compare_ready_lineage_smoke
    raw_engine_planner_tie_organic_applicability_smoke
    raw_engine_planner_tie_organic_proxy_pass_smoke
)
if (raw_engine_registered_nightly_tests)
    # Expensive exact_full / collision / sampled-audit compare cases are kept
    # behind nightly registration, but still carry the compare label once
    # enabled so `ctest -L compare` works on nightly builds as expected.
    raw_engine_label_tests(compare
        raw_engine_split_choice_harmless_shift_smoke
        raw_engine_fast_vs_exact_canonical_dedupe_smoke
        raw_engine_sampled_exact_audit_smoke
        raw_engine_canonical_collision_probe_smoke
        raw_engine_planner_tie_mixed_organic_compare_ready_smoke
        raw_engine_automorphism_probe_large_smoke
        raw_engine_compare_campaign_checkpoint_smoke
        raw_engine_campaign_resume_smoke
        raw_engine_campaign_partial_merge_smoke
    )
endif()

raw_engine_label_tests(asan_slow
    raw_engine_planner_oracle_fuzz
    raw_engine_planner_oracle_fuzz_stats
)
if (raw_engine_registered_nightly_tests)
    # ASan-tail cases are a subset worth running separately from the quick
    # sanitizer pass because they are disproportionately expensive.
    raw_engine_label_tests(asan_slow
        raw_engine_exhaustive_natural_dedupe_large_smoke
        raw_engine_exhaustive_organic_duplicate_examples_smoke
        raw_engine_fast_vs_exact_canonical_dedupe_smoke
        raw_engine_split_choice_harmless_shift_smoke
        raw_engine_canonical_collision_probe_smoke
        raw_engine_planner_tie_mixed_organic_compare_ready_smoke
        raw_engine_automorphism_probe_large_smoke
        raw_engine_compare_campaign_checkpoint_smoke
        raw_engine_campaign_resume_smoke
        raw_engine_campaign_partial_merge_smoke
    )
endif()

raw_engine_label_tests(nightly
    raw_engine_policy_gate_nightly_smoke
    raw_engine_policy_refresh_nightly_smoke
)

raw_engine_label_tests(policy_core
    raw_engine_policy_gate_manifest_smoke
    raw_engine_policy_baseline_promote_smoke
    raw_engine_policy_baseline_roundtrip_smoke
    raw_engine_policy_promote_reject_fail_smoke
    raw_engine_policy_refresh_smoke
    raw_engine_policy_refresh_against_promoted_baseline_smoke
    raw_engine_policy_refresh_family_filter_smoke
    raw_engine_policy_refresh_freshness_only_smoke
    raw_engine_policy_gate_diagnostic_only_smoke
    raw_engine_policy_ci_check_smoke
    raw_engine_policy_severity_smoke
    raw_engine_pipeline_exit_code_smoke
    raw_engine_runtime_budget_smoke
    raw_engine_runtime_budget_manifest_roundtrip_smoke
    raw_engine_runtime_fingerprint_roundtrip_smoke
    raw_engine_runtime_refresh_same_fingerprint_smoke
    raw_engine_runtime_baseline_promote_smoke
    raw_engine_runtime_refresh_against_promoted_baseline_smoke
    raw_engine_runtime_rerun_plan_empty_smoke
    raw_engine_combined_policy_runtime_ok_smoke
    raw_engine_policy_roundtrip_smoke
    raw_engine_policy_empty_plan_noop_smoke
    raw_engine_policy_manifest_missing_input_smoke
)

raw_engine_label_tests(policy_refresh
    raw_engine_policy_baseline_promote_smoke
    raw_engine_policy_baseline_roundtrip_smoke
    raw_engine_policy_baseline_history_smoke
    raw_engine_policy_baseline_archive_roundtrip_smoke
    raw_engine_policy_promote_reject_fail_smoke
    raw_engine_policy_refresh_smoke
    raw_engine_policy_refresh_against_promoted_baseline_smoke
    raw_engine_policy_refresh_family_filter_smoke
    raw_engine_policy_refresh_freshness_only_smoke
    raw_engine_policy_refresh_stale_hash_smoke
    raw_engine_policy_refresh_compare_engine_hash_smoke
    raw_engine_policy_refresh_generator_hash_smoke
    raw_engine_policy_refresh_campaign_config_hash_smoke
    raw_engine_policy_gate_non_applicable_drift_smoke
    raw_engine_planner_tie_organic_reclassify_trigger_smoke
    raw_engine_policy_reclassify_non_applicable_smoke
    raw_engine_policy_gate_diagnostic_only_smoke
    raw_engine_compare_ready_not_promoted_smoke
    raw_engine_policy_reject_diagnostic_promotion_smoke
    raw_engine_stale_family_selective_rerun_smoke
    raw_engine_policy_rerun_plan_smoke
    raw_engine_policy_rerun_plan_empty_smoke
    raw_engine_policy_rerun_plan_family_filter_smoke
    raw_engine_policy_rerun_plan_mode_selection_smoke
    raw_engine_policy_selective_rerun_execute_smoke
    raw_engine_policy_selective_rerun_noop_smoke
    raw_engine_policy_ci_check_smoke
    raw_engine_policy_nightly_refresh_smoke
    raw_engine_stale_warn_mode_smoke
    raw_engine_action_required_mode_smoke
    raw_engine_policy_severity_smoke
    raw_engine_pipeline_exit_code_smoke
    raw_engine_runtime_budget_proposal_gate_smoke
    raw_engine_runtime_budget_approve_reprofile_smoke
    raw_engine_runtime_budget_refresh_after_approval_smoke
    raw_engine_combined_pipeline_after_budget_reprofile_quick_smoke
    raw_engine_combined_pipeline_after_budget_reprofile_nightly_smoke
    raw_engine_policy_ops_summary_budget_vs_rebaseline_smoke
    raw_engine_policy_ops_summary_current_env_clear_new_env_action_required_smoke
    raw_engine_evidence_bundle_budget_lifecycle_smoke
    raw_engine_evidence_bundle_budget_transition_metadata_smoke
    raw_engine_policy_roundtrip_smoke
    raw_engine_policy_empty_plan_noop_smoke
    raw_engine_policy_manifest_missing_input_smoke
    raw_engine_policy_selective_rerun_n_family_smoke
    raw_engine_runtime_budget_smoke
    raw_engine_runtime_budget_manifest_roundtrip_smoke
    raw_engine_runtime_fingerprint_roundtrip_smoke
    raw_engine_runtime_not_comparable_smoke
    raw_engine_runtime_refresh_same_fingerprint_smoke
    raw_engine_runtime_baseline_promote_smoke
    raw_engine_runtime_refresh_against_promoted_baseline_smoke
    raw_engine_runtime_rerun_plan_smoke
    raw_engine_runtime_rerun_plan_empty_smoke
    raw_engine_runtime_synthetic_warn_smoke
    raw_engine_runtime_synthetic_action_required_smoke
    raw_engine_runtime_synthetic_fail_smoke
    raw_engine_runtime_missing_baseline_smoke
    raw_engine_runtime_fingerprint_mismatch_smoke
    raw_engine_combined_policy_runtime_ok_smoke
    raw_engine_combined_policy_runtime_warn_smoke
    raw_engine_combined_policy_runtime_action_required_smoke
    raw_engine_combined_policy_runtime_fail_smoke
    raw_engine_artifact_index_smoke
    raw_engine_artifact_prune_smoke
    raw_engine_bundle_index_roundtrip_smoke
    raw_engine_evidence_bundle_manifest_freshness_smoke
    raw_engine_evidence_bundle_policy_refresh_smoke
    raw_engine_evidence_bundle_rerun_plan_smoke
    raw_engine_evidence_bundle_idempotent_smoke
    raw_engine_evidence_bundle_policy_summary_smoke
    raw_engine_evidence_bundle_runtime_manifest_smoke
    raw_engine_evidence_bundle_runtime_registry_snapshot_smoke
    raw_engine_evidence_bundle_runtime_history_snapshot_smoke
    raw_engine_evidence_bundle_runtime_proposal_smoke
    raw_engine_evidence_bundle_contains_rebaseline_transition_smoke
    raw_engine_evidence_bundle_phase30_transition_smoke
    raw_engine_evidence_bundle_runtime_transition_metadata_smoke
    raw_engine_evidence_bundle_phase30_metadata_smoke
    raw_engine_evidence_bundle_combined_summary_smoke
    raw_engine_policy_refresh_nightly_smoke
)

raw_engine_label_tests(policy_sentinel
    raw_engine_policy_gate_non_applicable_smoke
    raw_engine_policy_gate_non_applicable_drift_smoke
    raw_engine_planner_tie_organic_applicability_drift_smoke
    raw_engine_planner_tie_organic_reclassify_trigger_smoke
    raw_engine_policy_reclassify_non_applicable_smoke
    raw_engine_compare_ready_lineage_gate_smoke
    raw_engine_compare_ready_not_counted_as_pass_smoke
    raw_engine_policy_gate_diagnostic_only_smoke
    raw_engine_compare_ready_not_promoted_smoke
    raw_engine_policy_reject_diagnostic_promotion_smoke
    raw_engine_policy_refresh_stale_hash_smoke
    raw_engine_policy_refresh_compare_engine_hash_smoke
    raw_engine_policy_refresh_generator_hash_smoke
    raw_engine_policy_refresh_campaign_config_hash_smoke
    raw_engine_stale_family_selective_rerun_smoke
    raw_engine_policy_rerun_plan_smoke
    raw_engine_policy_rerun_plan_empty_smoke
    raw_engine_policy_rerun_plan_family_filter_smoke
    raw_engine_policy_rerun_plan_mode_selection_smoke
    raw_engine_policy_selective_rerun_noop_smoke
    raw_engine_stale_warn_mode_smoke
    raw_engine_action_required_mode_smoke
    raw_engine_runtime_budget_smoke
    raw_engine_runtime_fingerprint_roundtrip_smoke
    raw_engine_runtime_not_comparable_smoke
    raw_engine_runtime_refresh_same_fingerprint_smoke
    raw_engine_runtime_rerun_plan_smoke
    raw_engine_runtime_rerun_plan_empty_smoke
    raw_engine_runtime_synthetic_warn_smoke
    raw_engine_runtime_synthetic_action_required_smoke
    raw_engine_runtime_synthetic_fail_smoke
    raw_engine_runtime_missing_baseline_smoke
    raw_engine_runtime_fingerprint_mismatch_smoke
    raw_engine_runtime_registry_multi_fingerprint_smoke
    raw_engine_runtime_registry_same_host_compiler_bump_smoke
    raw_engine_runtime_registry_sanitizer_change_smoke
    raw_engine_runtime_registry_runner_tag_change_smoke
    raw_engine_runtime_registry_cross_host_smoke
    raw_engine_runtime_history_stable_smoke
    raw_engine_runtime_history_noisy_smoke
    raw_engine_runtime_history_regressing_smoke
    raw_engine_runtime_history_recovery_after_rebaseline_smoke
    raw_engine_runtime_recommendation_watch_smoke
    raw_engine_runtime_recommendation_rebaseline_required_smoke
    raw_engine_runtime_recommendation_not_comparable_smoke
    raw_engine_combined_pipeline_recommendation_smoke
    raw_engine_runtime_rebaseline_proposal_multi_candidate_smoke
    raw_engine_runtime_rebaseline_proposal_retired_only_smoke
    raw_engine_runtime_rebaseline_proposal_cross_host_smoke
    raw_engine_pipeline_matrix_smoke
    raw_engine_pipeline_matrix_mixed_fingerprints_smoke
    raw_engine_pipeline_nightly_rebaseline_candidate_smoke
    raw_engine_combined_policy_runtime_warn_smoke
    raw_engine_combined_policy_runtime_action_required_smoke
    raw_engine_combined_policy_runtime_fail_smoke
    raw_engine_artifact_index_smoke
    raw_engine_artifact_prune_smoke
    raw_engine_bundle_index_roundtrip_smoke
    raw_engine_evidence_bundle_smoke
    raw_engine_evidence_bundle_manifest_freshness_smoke
    raw_engine_evidence_bundle_policy_refresh_smoke
    raw_engine_evidence_bundle_rerun_plan_smoke
    raw_engine_evidence_bundle_idempotent_smoke
    raw_engine_evidence_bundle_policy_summary_smoke
    raw_engine_evidence_bundle_runtime_manifest_smoke
    raw_engine_evidence_bundle_runtime_registry_snapshot_smoke
    raw_engine_evidence_bundle_runtime_history_snapshot_smoke
    raw_engine_evidence_bundle_runtime_proposal_smoke
    raw_engine_evidence_bundle_combined_summary_smoke
)

raw_engine_label_tests(policy_nightly
    raw_engine_policy_gate_nightly_smoke
    raw_engine_policy_refresh_nightly_smoke
    raw_engine_policy_nightly_refresh_smoke
    raw_engine_policy_selective_rerun_execute_smoke
    raw_engine_policy_roundtrip_smoke
    raw_engine_combined_policy_runtime_ok_smoke
    raw_engine_pipeline_nightly_rebaseline_candidate_smoke
)

if (raw_engine_registered_nightly_tests)
    # Nightly is the opt-in registration bucket for the heaviest compare,
    # exhaustive, and long-tail coverage cases.
    raw_engine_label_tests(nightly
        raw_engine_fuzz_matrix
        raw_engine_fuzz
        raw_engine_planner_oracle_fuzz
        raw_engine_planner_oracle_fuzz_stats
        raw_engine_planner_tie_mixed_exhaustive_smoke
        raw_engine_exhaustive_family_sweep_smoke
        raw_engine_exhaustive_natural_dedupe_large_smoke
        raw_engine_exhaustive_organic_duplicate_examples_smoke
        raw_engine_fast_vs_exact_canonical_dedupe_smoke
        raw_engine_split_choice_harmless_shift_smoke
        raw_engine_canonical_collision_probe_smoke
        raw_engine_split_tie_organic_symmetric_smoke
        raw_engine_planner_tie_mixed_organic_smoke
        raw_engine_planner_tie_mixed_organic_compare_ready_smoke
        raw_engine_automorphism_probe_large_smoke
        raw_engine_duplicate_attribution_smoke
        raw_engine_compare_campaign_checkpoint_smoke
        raw_engine_campaign_resume_smoke
        raw_engine_campaign_partial_merge_smoke
    )
endif()

set_tests_properties(
    raw_engine_reduce_planner_state_smoke
    raw_engine_reduce_planner_state_step_budget
    raw_engine_reduce_planner_state_trace_prefix
    raw_engine_reducer_determinism_smoke
    raw_engine_artifact_retention_smoke
    raw_engine_artifact_index_smoke
    raw_engine_artifact_prune_smoke
    raw_engine_bundle_index_roundtrip_smoke
    raw_engine_policy_roundtrip_smoke
    PROPERTIES
        RUN_SERIAL TRUE
)

foreach(test_name
        raw_engine_runtime_registry_promote_smoke
        raw_engine_runtime_registry_activate_retire_smoke
        raw_engine_runtime_registry_roundtrip_smoke
        raw_engine_runtime_watch_campaign_smoke
        raw_engine_runtime_watch_campaign_phase30_repeated_smoke
        raw_engine_runtime_watch_campaign_longer_smoke
        raw_engine_runtime_watch_campaign_phase30_repeat10_smoke
        raw_engine_runtime_watch_multi_fingerprint_smoke
        raw_engine_runtime_watch_same_host_compiler_bump_smoke
        raw_engine_runtime_watch_sanitizer_change_smoke
        raw_engine_runtime_watch_runner_tag_change_smoke
        raw_engine_runtime_watch_cross_host_smoke
        raw_engine_runtime_watch_retired_only_smoke
        raw_engine_runtime_watch_stable_overrun_smoke
        raw_engine_runtime_watch_clear_after_recovery_smoke
        raw_engine_runtime_watch_escalate_smoke
        raw_engine_runtime_watch_transition_clear_smoke
        raw_engine_runtime_watch_transition_stable_smoke
        raw_engine_runtime_watch_transition_escalate_smoke
        raw_engine_runtime_watch_to_rebaseline_candidate_smoke
        raw_engine_runtime_watch_production_hard_fail_smoke
        raw_engine_runtime_watch_diagnostic_soft_warn_smoke
        raw_engine_runtime_registry_import_current_smoke
        raw_engine_runtime_registry_import_new_fingerprint_smoke
        raw_engine_runtime_watch_import_same_fingerprint_smoke
        raw_engine_runtime_watch_import_fixture_vs_real_smoke
        raw_engine_runtime_watch_confidence_smoke
        raw_engine_runtime_watch_campaign_release_full_smoke
        raw_engine_runtime_watch_release_full_repeat10_smoke
        raw_engine_runtime_watch_release_full_confidence_smoke
        raw_engine_runtime_refresh_after_rebaseline_smoke
        raw_engine_runtime_rerun_plan_after_rebaseline_smoke
        raw_engine_combined_pipeline_after_rebaseline_quick_smoke
        raw_engine_combined_pipeline_after_phase30_rebaseline_quick_smoke
        raw_engine_combined_pipeline_after_phase30_rebaseline_nightly_smoke
        raw_engine_runtime_history_append_after_phase30_rebaseline_smoke
        raw_engine_runtime_registry_history_transition_phase30_smoke
        raw_engine_evidence_bundle_phase30_transition_smoke
        raw_engine_evidence_bundle_phase30_metadata_smoke
        raw_engine_runtime_budget_proposal_gate_smoke
        raw_engine_runtime_budget_approve_reprofile_smoke
        raw_engine_runtime_budget_refresh_after_approval_smoke
        raw_engine_runtime_current_env_age_tick_smoke
        raw_engine_runtime_current_env_plan_watch_smoke
        raw_engine_runtime_current_env_reproposal_trigger_gate_smoke
        raw_engine_runtime_current_env_due_scheduler_smoke
        raw_engine_runtime_current_env_time_travel_due_now_smoke
        raw_engine_runtime_current_env_plan_reproposal_smoke
        raw_engine_policy_ops_agenda_smoke
        raw_engine_runtime_current_env_execute_watch_smoke
        raw_engine_runtime_current_env_apply_watch_smoke
        raw_engine_current_env_monitoring_due_execute_apply_smoke
        raw_engine_runtime_current_env_execute_reproposal_gate_smoke
        raw_engine_policy_ops_agenda_action_status_smoke
        raw_engine_runtime_current_env_action_ledger_smoke
        raw_engine_runtime_current_env_action_retry_plan_smoke
        raw_engine_runtime_current_env_reproposal_handoff_smoke
        raw_engine_current_env_next_cycle_monitoring_due_smoke
        raw_engine_policy_ops_agenda_lifecycle_v2_smoke
        raw_engine_runtime_budget_history_append_smoke
        raw_engine_combined_pipeline_after_budget_reprofile_quick_smoke
        raw_engine_combined_pipeline_after_budget_reprofile_nightly_smoke
        raw_engine_policy_ops_summary_budget_vs_rebaseline_smoke
        raw_engine_policy_ops_summary_v8_smoke
        raw_engine_policy_ops_summary_v9_smoke
        raw_engine_policy_ops_summary_v10_smoke
        raw_engine_policy_ops_summary_v11_smoke
        raw_engine_policy_ops_summary_current_env_clear_new_env_action_required_smoke
        raw_engine_evidence_bundle_budget_lifecycle_smoke
        raw_engine_evidence_bundle_budget_transition_metadata_smoke
        raw_engine_evidence_bundle_current_env_guardrail_timeline_smoke
        raw_engine_evidence_bundle_current_env_due_timeline_smoke
        raw_engine_evidence_bundle_current_env_execute_apply_smoke
        raw_engine_evidence_bundle_action_ledger_smoke
        raw_engine_runtime_watch_history_append_phase30_smoke
        raw_engine_runtime_watch_confidence_real_vs_fixture_smoke
        raw_engine_policy_baseline_promote_smoke
        raw_engine_policy_baseline_roundtrip_smoke
        raw_engine_policy_baseline_history_smoke
        raw_engine_policy_baseline_archive_roundtrip_smoke
        raw_engine_policy_promote_reject_fail_smoke
        raw_engine_policy_refresh_smoke
        raw_engine_policy_refresh_against_promoted_baseline_smoke
        raw_engine_policy_refresh_family_filter_smoke
        raw_engine_policy_refresh_freshness_only_smoke
        raw_engine_policy_refresh_stale_hash_smoke
        raw_engine_policy_refresh_compare_engine_hash_smoke
        raw_engine_policy_refresh_generator_hash_smoke
        raw_engine_policy_refresh_campaign_config_hash_smoke
        raw_engine_policy_gate_non_applicable_drift_smoke
        raw_engine_planner_tie_organic_reclassify_trigger_smoke
        raw_engine_policy_reclassify_non_applicable_smoke
        raw_engine_runtime_history_append_smoke
        raw_engine_runtime_history_summary_smoke
        raw_engine_runtime_history_jitter_smoke
        raw_engine_runtime_history_regression_spike_smoke
        raw_engine_runtime_history_stable_smoke
        raw_engine_runtime_history_noisy_smoke
        raw_engine_runtime_history_regressing_smoke
        raw_engine_runtime_history_recovery_after_rebaseline_smoke
        raw_engine_runtime_history_compact_smoke
        raw_engine_policy_ops_summary_unified_smoke
        raw_engine_policy_ops_summary_current_vs_new_env_smoke
        raw_engine_policy_ops_summary_release_watch_smoke
        raw_engine_policy_ops_summary_matrix_rebaseline_smoke
        raw_engine_evidence_bundle_light_ops_smoke
        raw_engine_publication_snapshot_smoke
        raw_engine_publication_snapshot_bundle_smoke
        raw_engine_bundle_published_snapshot_smoke
        raw_engine_bundle_delivery_contents_smoke
        raw_engine_bundle_idempotent_from_published_snapshot_smoke
        raw_engine_staged_ctest_smoke
        raw_engine_campaign_wrapper_tail_smoke
        raw_engine_ctest_executable_lookup_smoke
        raw_engine_wrapper_infra_vs_semantic_failure_smoke
        raw_engine_policy_rerun_plan_smoke
        raw_engine_policy_rerun_plan_empty_smoke
        raw_engine_policy_rerun_plan_family_filter_smoke
        raw_engine_policy_rerun_plan_mode_selection_smoke
)
    if (TEST ${test_name})
        set_tests_properties(${test_name} PROPERTIES RUN_SERIAL TRUE)
    endif()
endforeach()

if (raw_engine_registered_nightly_tests)
    set_tests_properties(
        raw_engine_compare_campaign_checkpoint_smoke
        raw_engine_campaign_resume_smoke
        raw_engine_campaign_partial_merge_smoke
        raw_engine_runtime_watch_multi_fingerprint_smoke
        raw_engine_evidence_bundle_multi_fingerprint_watch_smoke
        raw_engine_campaign_manifest_parallel_smoke
        raw_engine_policy_pipeline_parallel_smoke
        raw_engine_transient_repro_smoke
        PROPERTIES
            RUN_SERIAL TRUE
    )
    set_tests_properties(raw_engine_campaign_resume_smoke PROPERTIES DEPENDS raw_engine_compare_campaign_checkpoint_smoke)
endif()

if (RAW_ENGINE_ENABLE_ASAN OR RAW_ENGINE_ENABLE_UBSAN)
    if (APPLE)
        set(raw_engine_asan_env "ASAN_OPTIONS=detect_leaks=0:strict_string_checks=1;UBSAN_OPTIONS=print_stacktrace=1")
    else()
        set(raw_engine_asan_env "ASAN_OPTIONS=detect_leaks=1:strict_string_checks=1;UBSAN_OPTIONS=print_stacktrace=1")
    endif()

    foreach(test_name
            raw_engine_micro
            raw_engine_roundtrip
            raw_engine_split_integrate_ensure
            raw_engine_regression_44001
            raw_engine_regression_isolate_split_no_sep
            raw_engine_planner
            raw_engine_fuzz_matrix
            raw_engine_fuzz
            raw_engine_isolate_oracle
            raw_engine_split_oracle
            raw_engine_join_oracle
            raw_engine_integrate_oracle
            raw_engine_planner_oracle
            raw_engine_planner_oracle_fuzz
            raw_engine_replay_state
            raw_engine_reducer_smoke
            raw_engine_replay_planner_state
            raw_engine_reduce_planner_state_smoke
            raw_engine_failure_signature_smoke
            raw_engine_planner_failure_signature_smoke
            raw_engine_reduce_planner_state_step_budget
            raw_engine_reduce_planner_state_trace_prefix
            raw_engine_planner_oracle_fuzz_stats
            raw_engine_primitive_fault_detection_smoke
            raw_engine_planner_fault_detection_smoke
            raw_engine_mutation_matrix_smoke
            raw_engine_planner_fixpoint_idempotence
            raw_engine_planner_replay_determinism
            raw_engine_reducer_determinism_smoke
            raw_engine_corpus_roundtrip_smoke
            raw_engine_corpus_replay_smoke
            raw_engine_exhaustive_split_ready_smoke
            raw_engine_exhaustive_join_ready_smoke
            raw_engine_exhaustive_integrate_ready_smoke
            raw_engine_exhaustive_mixed_smoke
            raw_engine_planner_tie_mixed_exhaustive_smoke
            raw_engine_exhaustive_canonical_dedupe_smoke
            raw_engine_exhaustive_natural_dedupe_smoke
            raw_engine_exhaustive_family_sweep_smoke
            raw_engine_exhaustive_collision_guard_smoke
            raw_engine_exact_canonicalizer_smoke
            raw_engine_metamorphic_relabel_invariance
            raw_engine_metamorphic_occid_invariance
            raw_engine_metamorphic_edge_order_invariance
            raw_engine_metamorphic_vertex_order_invariance
            raw_engine_replay_serialization_invariance
            raw_engine_metamorphic_family_matrix_smoke
            raw_engine_metamorphic_planner_multistep_smoke
            raw_engine_metamorphic_replay_matrix_smoke
            raw_engine_split_choice_oracle_smoke
            raw_engine_split_choice_relabel_invariance
            raw_engine_split_choice_edge_order_invariance
            raw_engine_split_choice_vertex_order_invariance
            raw_engine_split_choice_oracle_regression
            raw_engine_split_choice_policy_smoke
            raw_engine_split_choice_policy_relabel_invariance
            raw_engine_split_choice_policy_edge_order_invariance
            raw_engine_split_choice_policy_vertex_order_invariance
            raw_engine_split_choice_policy_occid_invariance
            raw_engine_split_choice_policy_multiclass_smoke
            raw_engine_split_choice_exact_class_smoke
            raw_engine_split_choice_exact_relabel_invariance
            raw_engine_split_choice_exact_vertex_order_invariance
            raw_engine_split_choice_exact_edge_order_invariance
            raw_engine_split_choice_representative_shift_smoke
            raw_engine_split_choice_semantic_shift_smoke
            raw_engine_split_choice_semantic_shift_regression
            raw_engine_split_choice_policy_exact_shadow_smoke
            raw_engine_split_choice_policy_fast_vs_exact_shadow_compare
            raw_engine_split_choice_exact_shadow_relabel_invariance
            raw_engine_split_choice_exact_shadow_vertex_order_invariance
            raw_engine_split_choice_exact_shadow_edge_order_invariance
            raw_engine_split_choice_fallback_zero_smoke
            raw_engine_planner_relabel_structural_regression
            raw_engine_planner_targeted_split_smoke
            raw_engine_planner_targeted_join_smoke
            raw_engine_planner_targeted_integrate_smoke
            raw_engine_planner_targeted_mixed_smoke
            raw_engine_split_ready_unique_orig_regression
            raw_engine_planner_coverage_smoke
            raw_engine_planner_random_coverage_smoke
            raw_engine_planner_weighted_coverage_smoke
            raw_engine_planner_join_ready_smoke
            raw_engine_planner_integrate_ready_smoke
            raw_engine_planner_structural_mixed_smoke
            raw_engine_planner_tie_mixed_smoke
            raw_engine_planner_tie_mixed_exact_shadow_smoke
            raw_engine_planner_tie_symmetric_smoke
            raw_engine_artifact_retention_smoke
    )
        set_tests_properties(${test_name} PROPERTIES
            ENVIRONMENT "${raw_engine_asan_env}"
        )
    endforeach()
    if (raw_engine_registered_nightly_tests)
        set(raw_engine_asan_registered_nightly_tests
            raw_engine_exhaustive_natural_dedupe_large_smoke
            raw_engine_exhaustive_organic_duplicate_examples_smoke
            raw_engine_fast_vs_exact_canonical_dedupe_smoke
            raw_engine_split_choice_harmless_shift_smoke
            raw_engine_canonical_collision_probe_smoke
            raw_engine_split_tie_organic_symmetric_smoke
            raw_engine_planner_tie_mixed_organic_smoke
            raw_engine_planner_tie_mixed_organic_compare_ready_smoke
            raw_engine_automorphism_probe_large_smoke
            raw_engine_sampled_exact_audit_smoke
            raw_engine_duplicate_attribution_smoke
            raw_engine_compare_campaign_checkpoint_smoke
            raw_engine_campaign_resume_smoke
            raw_engine_campaign_partial_merge_smoke
        )
    else()
        set(raw_engine_asan_registered_nightly_tests)
    endif()
    foreach(test_name IN LISTS raw_engine_asan_registered_nightly_tests)
        set_tests_properties(${test_name} PROPERTIES
            ENVIRONMENT "${raw_engine_asan_env}"
        )
    endforeach()
endif()
