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
add_test(NAME raw_engine_campaign_phase16_resume_gate_smoke COMMAND raw_engine_tests --case campaign_phase16_resume_gate_smoke)
add_test(
    NAME raw_engine_campaign_phase16_manifest_compatibility_smoke
    COMMAND raw_engine_tests --case campaign_phase16_manifest_compatibility_smoke
)
add_test(NAME raw_engine_split_choice_policy_graduation_smoke COMMAND raw_engine_tests --case split_choice_policy_graduation_smoke)
add_test(NAME raw_engine_policy_graduation_gate_smoke COMMAND raw_engine_tests --case policy_graduation_gate_smoke)
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
        COMMAND raw_engine_tests
            --case campaign
            --campaign-config ${PROJECT_SOURCE_DIR}/tests/campaigns/phase15_checkpoint_smoke.txt
            --checkpoint-dir ${CMAKE_CURRENT_BINARY_DIR}/artifacts/phase15_campaign_checkpoint_smoke/checkpoints
            --checkpoint-every 2
            --max-wall-seconds 1
            --stop-after-checkpoint
    )
    add_test(
        NAME raw_engine_campaign_resume_smoke
        COMMAND raw_engine_tests
            --case campaign
            --resume-from ${CMAKE_CURRENT_BINARY_DIR}/artifacts/phase15_campaign_checkpoint_smoke/checkpoints/latest.chk
    )
    add_test(
        NAME raw_engine_campaign_partial_merge_smoke
        COMMAND raw_engine_tests
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
        set_property(TEST ${test_name} APPEND PROPERTY LABELS ${label})
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
    PROPERTIES
        RUN_SERIAL TRUE
)

if (raw_engine_registered_nightly_tests)
    set_tests_properties(
        raw_engine_compare_campaign_checkpoint_smoke
        raw_engine_campaign_resume_smoke
        raw_engine_campaign_partial_merge_smoke
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
