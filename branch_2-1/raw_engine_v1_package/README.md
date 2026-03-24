# raw_engine_v1 package

First-working-version raw primitive engine, split into a small CMake project.

Structure:
- `include/raw_engine/raw_engine.hpp`
- `src/raw_core.cpp`
- `src/raw_validators.cpp`
- `src/raw_primitives.cpp`
- `src/raw_planner.cpp`
- `tests/raw_engine_cases.cpp`
- `tests/exhaustive_generator.cpp`
- `tests/exhaustive_cases.cpp`
- `tests/metamorphic_cases.cpp`
- `tests/split_choice_oracle.cpp`
- `tests/split_choice_cases.cpp`
- `tests/raw_engine_main.cpp`

Primary target:
- `raw_engine_tests`

CLI examples:
```bash
./raw_engine_tests --case micro
./raw_engine_tests --case fuzz --seed 44001 --iters 1
./raw_engine_tests --case planner --seed 123 --iters 1000
./raw_engine_tests --case fuzz_matrix --iters 20
./raw_engine_tests --case isolate_fuzz --seed 710001 --iters 100
./raw_engine_tests --case split_join_fuzz --seed 715001 --iters 100
./raw_engine_tests --case planner_mixed_fuzz --seed 717001 --iters 100 --step-budget 200000
./raw_engine_tests --case regression_44001
./raw_engine_tests --case regression_isolate_split_no_sep
./raw_engine_tests --case repro --repro-file counterexamples/reduced_isolate_split_seed3736675150.txt
./raw_engine_tests --case planner_targeted_mixed_smoke
./raw_engine_tests --case planner_coverage_smoke
./raw_engine_tests --case planner_random_coverage_smoke
./raw_engine_tests --case planner_weighted_coverage_smoke
./raw_engine_tests --case planner_join_ready_smoke
./raw_engine_tests --case planner_integrate_ready_smoke
./raw_engine_tests --case planner_structural_mixed_smoke
./raw_engine_tests --case primitive_fault_detection_smoke
./raw_engine_tests --case planner_fault_detection_smoke
./raw_engine_tests --case mutation_matrix_smoke
./raw_engine_tests --case planner_fixpoint_idempotence
./raw_engine_tests --case planner_replay_determinism
./raw_engine_tests --case reducer_determinism_smoke
./raw_engine_tests --case corpus_roundtrip_smoke
./raw_engine_tests --case corpus_replay_smoke
./raw_engine_tests --case exhaustive --family split_ready --max-real 5 --max-occ 2 --max-edges 7 --max-states 5000 --dedupe-canonical
./raw_engine_tests --case exhaustive --family mixed --max-real 5 --max-occ 3 --max-edges 8 --max-states 3000 --dedupe-canonical
./raw_engine_tests --case exhaustive --family join_ready --max-real 6 --max-occ 3 --max-edges 9 --max-components 3 --max-hosted-occ 2 --max-states 6000 --dedupe-canonical --collision-spot-checks 8
./raw_engine_tests --case exhaustive_split_ready_smoke
./raw_engine_tests --case exhaustive_join_ready_smoke
./raw_engine_tests --case exhaustive_integrate_ready_smoke
./raw_engine_tests --case exhaustive_mixed_smoke
./raw_engine_tests --case exhaustive_canonical_dedupe_smoke
./raw_engine_tests --case exhaustive_natural_dedupe_smoke
./raw_engine_tests --case exhaustive_family_sweep_smoke
./raw_engine_tests --case exhaustive_collision_guard_smoke
./raw_engine_tests --case exhaustive_natural_dedupe_large_smoke
./raw_engine_tests --case exhaustive_organic_duplicate_examples_smoke
./raw_engine_tests --case metamorphic_relabel_invariance
./raw_engine_tests --case metamorphic_occid_invariance
./raw_engine_tests --case metamorphic_edge_order_invariance
./raw_engine_tests --case metamorphic_vertex_order_invariance
./raw_engine_tests --case replay_serialization_invariance
./raw_engine_tests --case metamorphic_family_matrix_smoke
./raw_engine_tests --case metamorphic_planner_multistep_smoke
./raw_engine_tests --case metamorphic_replay_matrix_smoke
./raw_engine_tests --case split_choice_oracle_smoke
./raw_engine_tests --case split_choice_relabel_invariance
./raw_engine_tests --case split_choice_edge_order_invariance
./raw_engine_tests --case split_choice_vertex_order_invariance
./raw_engine_tests --case split_choice_oracle_regression
./raw_engine_tests --case split_choice_policy_smoke
./raw_engine_tests --case split_choice_policy_relabel_invariance
./raw_engine_tests --case split_choice_policy_edge_order_invariance
./raw_engine_tests --case split_choice_policy_vertex_order_invariance
./raw_engine_tests --case split_choice_policy_occid_invariance
./raw_engine_tests --case split_choice_policy_multiclass_smoke
./raw_engine_tests --case exact_canonicalizer_smoke
./raw_engine_tests --case fast_vs_exact_canonical_dedupe_smoke
./raw_engine_tests --case split_choice_exact_class_smoke
./raw_engine_tests --case split_choice_exact_relabel_invariance
./raw_engine_tests --case split_choice_exact_vertex_order_invariance
./raw_engine_tests --case split_choice_exact_edge_order_invariance
./raw_engine_tests --case planner_tie_mixed_smoke
./raw_engine_tests --case planner_tie_symmetric_smoke
./raw_engine_tests --case planner_tie_mixed_exhaustive_smoke
./raw_engine_tests --case planner_relabel_structural_regression
./raw_engine_tests --case exhaustive --family split_tie_ready --max-real 6 --max-occ 3 --max-edges 9 --max-states 8000 --dedupe-canonical --collision-spot-checks 8
./raw_engine_tests --case exhaustive --family split_tie_structural --max-real 6 --max-occ 3 --max-edges 9 --max-states 8000 --dedupe-canonical --collision-spot-checks 8
./raw_engine_tests --case exhaustive --family planner_tie_mixed --max-real 8 --max-occ 3 --max-edges 14 --max-states 6000 --dedupe-canonical --collision-spot-checks 8
./raw_engine_tests --case exhaustive --family split_tie_symmetric_large --max-real 8 --max-occ 3 --max-edges 12 --max-states 6000 --dedupe-canonical --collision-spot-checks 8 --exact-canonical-cap 8
./raw_engine_tests --case exhaustive --family canonical_collision_probe --max-real 8 --max-occ 3 --max-edges 12 --max-states 4000 --dedupe-canonical --collision-spot-checks 12 --exact-canonical-cap 8
./raw_engine_tests --case planner_oracle_fuzz --seed 880001 --iters 1500 --oracle planner --fuzz-mode split_tie_ready
./raw_engine_tests --case planner_oracle_fuzz --seed 880002 --iters 1500 --oracle planner --fuzz-mode planner_tie_mixed
./raw_engine_tests --case planner_oracle_fuzz --seed 880003 --iters 1500 --oracle planner --fuzz-mode planner_mixed_structural
./raw_engine_tests --case planner_oracle_fuzz --seed 900001 --iters 1500 --oracle planner --fuzz-mode split_tie_symmetric_large --save-corpus artifacts/corpus_symmetry
./raw_engine_tests --case planner_oracle_fuzz --seed 900002 --iters 1500 --oracle planner --fuzz-mode planner_tie_mixed_symmetric --save-corpus artifacts/corpus_symmetry
./raw_engine_tests --case planner_oracle_fuzz --seed 900003 --iters 1500 --oracle planner --fuzz-mode canonical_collision_probe --save-corpus artifacts/corpus_symmetry
./raw_engine_tests --case campaign --campaign-config tests/campaigns/planner_phase4.txt
./raw_engine_tests --case planner_oracle_fuzz --seed 840001 --iters 2000 --step-budget 200000 --oracle planner --dump-on-fail --artifact-dir build-release/tests/artifacts/planner_fuzz_phase_20260307_phase3 --stats-file build-release/tests/artifacts/planner_fuzz_phase_20260307_phase3/logs/seed840001_random.json --fuzz-mode random --precondition-bias-profile balanced
./raw_engine_tests --case planner_oracle_fuzz --seed 840003 --iters 2000 --step-budget 200000 --oracle planner --dump-on-fail --artifact-dir build-release/tests/artifacts/planner_fuzz_phase_20260307_phase3 --stats-file build-release/tests/artifacts/planner_fuzz_phase_20260307_phase3/logs/seed840003_weighted_join.json --fuzz-mode weighted_join_heavy
./raw_engine_tests --case planner_oracle_fuzz --seed 840005 --iters 2000 --step-budget 200000 --oracle planner --dump-on-fail --artifact-dir build-release/tests/artifacts/planner_fuzz_phase_20260307_phase3 --stats-file build-release/tests/artifacts/planner_fuzz_phase_20260307_phase3/logs/seed840005_join_ready.json --fuzz-mode join_ready
./raw_engine_tests --case planner_oracle_fuzz --seed 840007 --iters 2000 --step-budget 200000 --oracle planner --dump-on-fail --artifact-dir build-release/tests/artifacts/planner_fuzz_phase_20260307_phase3 --stats-file build-release/tests/artifacts/planner_fuzz_phase_20260307_phase3/logs/seed840007_structural_mixed.json --fuzz-mode planner_mixed_structural
```

Planner targeted coverage modes:
- `--fuzz-mode random|weighted_split_heavy|weighted_join_heavy|weighted_integrate_heavy|artifact_heavy|multiedge_heavy`
- `--fuzz-mode split_ready|split_with_boundary_artifact|split_with_keepOcc_sibling|split_with_join_and_integrate|planner_mixed_targeted|join_ready|integrate_ready|planner_mixed_structural`
- `--fuzz-mode split_tie_ready|split_tie_structural|planner_tie_mixed|split_tie_symmetric_large|planner_tie_mixed_symmetric|canonical_collision_probe|split_tie_organic_symmetric|planner_tie_mixed_organic|planner_tie_mixed_organic_compare_ready|automorphism_probe_large` adds tie-heavy and symmetry-heavy planner families
- `--scenario-family random|split_ready|split_with_boundary_artifact|split_with_keepOcc_sibling|split_with_join_and_integrate|planner_mixed_targeted|join_ready|integrate_ready|planner_mixed_structural|split_tie_ready|split_tie_structural|planner_tie_mixed|split_tie_symmetric_large|planner_tie_mixed_symmetric|canonical_collision_probe|split_tie_organic_symmetric|planner_tie_mixed_organic|planner_tie_mixed_organic_compare_ready|automorphism_probe_large`
- `--precondition-bias-profile default|balanced|split_heavy|join_heavy|integrate_heavy|artifact_heavy|structural`
- `--bias-split <0..8>` / `--bias-join <0..8>` / `--bias-integrate <0..8>` override the active bias profile per primitive family
- `--stats` / `--stats-file` emit JSON plus `<stats>.summary.txt`
- `--save-corpus <dir>` / `--load-corpus <dir>` / `--corpus-policy best|append|replace` persist and replay high-value planner seeds
- `--case campaign --campaign-config tests/campaigns/planner_phase4.txt` runs a long planner campaign and emits aggregate stats/summary files
- `--case exhaustive --family split_ready|join_ready|integrate_ready|mixed|split_tie_ready|split_tie_structural|planner_tie_mixed|split_tie_symmetric_large|planner_tie_mixed_symmetric|canonical_collision_probe|split_tie_organic_symmetric|planner_tie_mixed_organic|automorphism_probe_large|all --max-real <N> --max-occ <N> --max-edges <N> --max-components <N> --max-hosted-occ <N> --max-states <N> --dedupe-canonical --collision-spot-checks <N> --exact-canonical-cap <N> --exact-canonical-sample-rate <N>` runs the bounded tiny-state explorer with natural dedupe stats, optional sampled collision guards, and bounded exact-canonical audit
- `--max-split-pair-candidates <N>` bounds split-choice oracle comparison while always keeping the planner-selected pair in the compared subset
- `--max-split-choice-eval <N>` caps semantic split-choice lookahead evaluation; over-cap choices fall back to deterministic structural ranking and increment fallback stats
- `--exact-canonical-cap <N>` bounds the tests-only exact canonicalizer by live REAL-orig count
- `--exact-canonical-sample-rate <N>` samples exhaustive states for exact canonical audit; `1` audits every eligible state
- `--exact-audit-sample-rate <float>` samples split-choice exact audits during planner fuzz
- `--exact-audit-budget <N>` limits the number of split-choice exact audits per run
- `--exact-audit-family <scenario-family>` restricts sampled exact audits to one targeted family
- `--split-choice-policy fast|exact_shadow|exact_full` keeps production on `exact_shadow`; `fast` remains compare/replay-only and `exact_full` is tests-only for bounded representative comparison
- `--compare-against none|exact_full` enables exact-shadow vs exact-full compare mode inside split-choice audits
- `--compare-sample-rate <float>` samples compare states during exhaustive/fuzz exact-shadow adequacy checks
- `--compare-budget <N>` limits bounded exact-full compare states per run
- `--checkpoint-dir <dir>` writes resumable campaign checkpoints and merged partial summaries
- `--checkpoint-every <N>` chunks campaign runs every `N` iterations per seed
- `--resume-from <path>` resumes a prior campaign from `latest.chk` or the checkpoint directory itself
- `--max-wall-seconds <sec>` stops a campaign after the current checkpoint flush and keeps merged aggregate outputs
- `--target-compared-states <N>` raises the compared/completed evidence target used by the policy graduation gate
- `--target-eligible-states <N>` raises the eligible-state target used by the policy graduation gate
- `--target-lineage-samples <N>` raises the lineage sample target used by diagnostic compare-ready lineage monitoring
- `--target-applicability-confidence <float>` raises the minimum dominant ineligible-reason confidence required for `NON_APPLICABLE`
- `--stop-when-gate-passes` exits after the next checkpoint flush once every production family reaches `PASS` or `NON_APPLICABLE` and every diagnostic family is at least `DIAGNOSTIC_ONLY`
- `--max-partial-runs <N>` bounds how many committed checkpoint chunks a long compare campaign may consume before writing a partial aggregate with `stop_reason=max_partial_runs`
- `--stop-after-checkpoint` exits immediately after the next chunk checkpoint is committed
- `--policy-manifest <path>` loads an existing policy-gate text manifest (`.json` input resolves to the sibling `.txt`)
- `--baseline-manifest <path>` selects the approved baseline manifest for freshness/revalidation
- `--current-manifest <path>` selects the manifest being revalidated; defaults to `--policy-manifest`
- `--gate-family <name>` filters `--case policy_gate` to one named family
- `--gate-strict` makes `--case policy_gate` fail unless every selected family is `PASS`, `NON_APPLICABLE`, or `DIAGNOSTIC_ONLY`
- `--gate-output <path>` writes policy-gate json plus sibling text/summary outputs
- `--freshness-only` makes `--case policy_gate_refresh` treat the run as freshness/revalidation only
- `--revalidate-families <csv>` limits `--case policy_gate_refresh` to named stale families
- `--mark-stale-on-hash-change` turns hash deltas into `STALE` instead of silently inheriting the baseline
- `--allow-stale` lets refresh mode tolerate `STALE` families while still failing `REQUIRES_RERUN`
- split-choice compare examples:
  - `./raw_engine_tests --case split_choice_oracle_smoke --split-choice-policy exact_shadow --compare-against exact_full`
  - `./raw_engine_tests --case exhaustive --family split_tie_ready --max-real 6 --max-occ 3 --max-edges 9 --max-states 8000 --dedupe-canonical --split-choice-policy exact_shadow --compare-against exact_full --compare-sample-rate 0.2 --compare-budget 200`
- `planner_tie_mixed_organic` stays the real post-split mixed follow-up family. `planner_tie_mixed_organic_compare_ready` remains a tests-only diagnostic split-tie precursor and is never counted as production graduation evidence.
- `compare_ready_lineage_audit` / `compare_ready_lineage_smoke` quantify how the tests-only compare precursor relates to the real `planner_tie_mixed_organic` family and emit a dedicated lineage log with base-state hashes, reason codes, and precheck stats.
- phase18 policy-gate minima are `32/32/32` compared/eligible/completed states for `split_tie_organic_symmetric` and `automorphism_probe_large`, `48` generated applicability states plus dominant `no_split_ready` confidence for `planner_tie_mixed_organic`, and `32/32/32` compare plus `16` lineage samples for the diagnostic-only `planner_tie_mixed_organic_compare_ready`
- compare/nightly campaign examples:
  - `./raw_engine_tests --case campaign --campaign-config tests/campaigns/phase17_split_tie_organic_compare.txt --checkpoint-dir artifacts/checkpoints/split_tie_organic --checkpoint-every 4 --target-compared-states 32 --target-eligible-states 32 --stop-when-gate-passes --max-partial-runs 12 --max-wall-seconds 1800`
  - `./raw_engine_tests --case campaign --campaign-config tests/campaigns/phase17_planner_tie_gap_audit.txt --checkpoint-dir artifacts/checkpoints/planner_tie_gap --checkpoint-every 4 --target-applicability-confidence 0.90 --stop-when-gate-passes --max-partial-runs 12 --max-wall-seconds 1800`
  - `./raw_engine_tests --case campaign --campaign-config tests/campaigns/phase17_planner_tie_compare_ready.txt --checkpoint-dir artifacts/checkpoints/planner_tie_compare_ready --checkpoint-every 4 --target-compared-states 32 --target-eligible-states 32 --stop-when-gate-passes --max-partial-runs 12 --max-wall-seconds 1800`
  - `./raw_engine_tests --case campaign --campaign-config tests/campaigns/phase17_automorphism_compare.txt --checkpoint-dir artifacts/checkpoints/automorphism_probe --checkpoint-every 4 --target-compared-states 32 --target-eligible-states 32 --stop-when-gate-passes --max-partial-runs 12 --max-wall-seconds 1800`
  - `./raw_engine_tests --case campaign --resume-from artifacts/checkpoints/planner_tie_compare_ready/latest.chk`
- `phase17_planner_tie_gap_audit.txt` is the direct `planner_tie_mixed_organic` applicability audit. Interpret it via `generated_state_count`, `split_ready_state_count`, `compare_eligible_state_count`, `compare_ineligible_reason_histogram`, `split_ready_relevance`, `compare_relevance`, `dominant_ineligible_reason`, `dominant_ineligible_reason_confidence`, and `classification={DIRECTLY_APPLICABLE|UNDER_GENERATED|NON_APPLICABLE}`; phase18 policy gate flags drift as soon as `compare_eligible_state_count` or `split_ready_state_count` rises meaningfully above zero.
- `split_choice_oracle_*` cases enumerate admissible split pairs, compare final canonical state / target isolate signature / stop condition, and route detected instability through dump/reduce/regression
- `split_choice_policy_*` cases verify that the planner chooses a relabel/order invariant semantic representative across admissible split pairs and multi-class tie states
- `split_choice_representative_shift_*` and sampled exact-audit fuzz stats classify representative shifts as `harmless`, `trace_only`, or `semantic`
- `split_choice_semantic_shift_regression` replays a saved sampled exact-audit counterexample and pins the current fast-policy semantic shift against the exact-shadow representative
- `exact_canonicalizer_smoke`, `fast_vs_exact_canonical_dedupe_smoke`, `split_choice_exact_*`, `planner_tie_symmetric_smoke`, and `canonical_collision_probe_smoke` extend the deterministic quality gates with tests-only exact canonicalization, fast-vs-exact dedupe audit, symmetry-heavy split-choice class audit, and canonical-collision probes
- `split_tie_organic_symmetric_smoke`, `planner_tie_mixed_organic_smoke`, `planner_tie_mixed_organic_compare_ready_smoke`, `automorphism_probe_large_smoke`, `sampled_exact_audit_smoke`, and `duplicate_attribution_smoke` cover the organic-symmetry families, the compare-ready mixed precursor, sampled larger-state exact audit, and duplicate-cause attribution
- `exhaustive_*_smoke`, `metamorphic_*`, `split_choice_oracle_*`, and `split_choice_policy_*` remain the deterministic quality gates for natural canonical dedupe, relabel/occid/order invariance, planner multi-step matrix coverage, replay serialization invariance, split-pair stability, and semantic split-choice selection
- stats include:
  - preconditions and actual hits: `split_ready_count`, `boundary_only_child_count`, `join_candidate_count`, `integrate_candidate_count`, `actual_split_hits`, `actual_join_hits`, `actual_integrate_hits`, `first_*_iter`
  - conversion ratios: `precondition_to_actual.split_conversion`, `join_conversion`, `integrate_conversion`
  - diversity: `trace_prefix_histogram`, `primitive_multiset_histogram`, `diversity.unique_trace_prefix_count`, `diversity.unique_primitive_multiset_count`
  - coverage summary: `coverage_summary.isolate_heavy_ratio`, `split_hit_density`, `join_hit_density`, `integrate_hit_density`
  - split-choice policy: `split_choice_candidate_count`, `split_choice_eval_count`, `split_choice_tie_count`, `split_choice_multiclass_count`, `split_choice_fallback_count`, `split_choice_equiv_class_count_histogram`, `first_split_choice_tie_iter`
  - exact-shadow vs exact-full compare: `split_choice_compare_state_count`, `split_choice_exact_shadow_eval_count`, `split_choice_exact_full_eval_count`, `split_choice_same_representative_count`, `split_choice_same_semantic_class_count`, `split_choice_same_final_state_count`, `split_choice_semantic_disagreement_count`, `split_choice_cap_hit_count`
  - compare eligibility: `compare_eligible_state_count`, `compare_ineligible_state_count`, `compare_completed_state_count`, `compare_partial_state_count`, `compare_ineligible_reason_histogram`
  - policy graduation gate and manifest inputs: `gate_status`, `run_gate`, `target_compared_states`, `target_eligible_states`, `target_lineage_samples`, `target_applicability_states`, `target_applicability_confidence`, `stop_reason`
  - representative-shift audit: `representative_shift_count`, `representative_shift_same_class_count`, `representative_shift_semantic_divergence_count`, `representative_shift_followup_divergence_count`, `representative_shift_trace_divergence_count`, `harmless_shift_count`, `trace_only_shift_count`, `semantic_shift_count`
  - multiclass catalog: `multiclass_catalog_cluster_count`, `multiclass_harmless_cluster_count`, `multiclass_trace_only_cluster_count`, `multiclass_semantic_shift_cluster_count`, `multiclass_catalog_histogram`
  - fast-vs-exact audit: `exact_audited_state_count`, `exact_audited_pair_count`, `fast_unique_count`, `exact_unique_count`, `fast_vs_exact_disagreement_count`, `false_merge_count`, `false_split_count`, `exact_audit_skipped_cap_count`, `exact_audit_skipped_budget_count`, `exact_audit_skipped_sample_count`, `exact_audit_skipped_family_count`, `exact_audit_skipped_non_tie_count`
  - sampled duplicate attribution: `build_order_duplicate_count`, `commit_order_duplicate_count`, `hosted_occ_order_duplicate_count`, `relabel_duplicate_count`, `occid_duplicate_count`, `symmetric_structure_duplicate_count`, `mixed_duplicate_count`, `unknown_duplicate_count`
  - compare profiling: `avg_compare_time_per_state_ns`, `avg_exact_full_eval_time_per_pair_ns`, `scenario_hash_cache_hit_ratio`, `exact_full_pair_evaluation_cache_hit_ratio`, `exact_canonical_key_cache_hit_ratio`

Build and run:
```bash
bash build_and_run.sh
```

If `cmake` / `ctest` are not on `PATH`, pass them explicitly:
```bash
CMAKE_BIN=/path/to/cmake CTEST_BIN=/path/to/ctest bash build_and_run.sh
```

Manual commands:
```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug -j
ctest --test-dir build-debug --output-on-failure
```

Nightly registration:
```bash
cmake -S . -B build-nightly -DCMAKE_BUILD_TYPE=Debug -DRAW_ENGINE_REGISTER_NIGHTLY_TESTS=ON
cmake --build build-nightly -j
ctest --test-dir build-nightly -L nightly --output-on-failure
```

CTest tiers:
```bash
ctest --test-dir build-debug -L core --output-on-failure
ctest --test-dir build-debug -L slow --output-on-failure
ctest --test-dir build-debug -L exhaustive --output-on-failure
ctest --test-dir build-debug -L fuzz --output-on-failure
ctest --test-dir build-debug -L compare --output-on-failure
ctest --test-dir build-debug -L nightly --output-on-failure
ctest --test-dir build-debug -L policy_core --output-on-failure
ctest --test-dir build-debug -L policy_refresh --output-on-failure
ctest --test-dir build-debug -L policy_sentinel --output-on-failure
ctest --test-dir build-debug -L policy_nightly --output-on-failure
ctest --test-dir build-asan -L asan_slow --output-on-failure
```

Plain `ctest --test-dir build-debug --output-on-failure` now runs the default registered suite with nightly-only tail smokes omitted. Enable `-DRAW_ENGINE_REGISTER_NIGHTLY_TESTS=ON` when you want the expensive compare/exhaustive/nightly cases registered into CTest, then use `ctest -L nightly` or `ctest -L compare`.

Tiering rules:
- `core` is the short deterministic gate for regular local iteration.
- `slow` is the heavier day-to-day tier that still fits a normal debug/release pass.
- `exhaustive` isolates explorer-heavy bounded-state tests.
- `fuzz` isolates planner fuzz, corpus replay, and stats-oriented coverage cases.
- `compare` isolates exact-shadow vs exact-full representative checks, sampled exact audits, lineage/gate smokes, and collision/tie compare regressions.
- `policy_core` is the cheapest freshness/regression layer for manifest round-trips and diagnostic-only enforcement.
- `policy_refresh` extends `policy_core` with baseline/current refresh checks, stale-family selection, reclassify triggers, and evidence-bundle freshness checks.
- `policy_sentinel` is the cheap family-specific drift tier for `NON_APPLICABLE` applicability drift, diagnostic-only pinning, and stale-family rerun-selection behavior.
- `policy_nightly` is the policy-oriented nightly tail that exercises CI summary generation, selective rerun execution, and manifest refresh against the real artifact tree.
- `asan_slow` is the sanitizer-tail subset that is intentionally safe to skip from a quick ASan pass.
- `nightly` is opt-in: those tests are not even registered unless `-DRAW_ENGINE_REGISTER_NIGHTLY_TESTS=ON` is set at configure time, and it is the tier where the policy gate is refreshed from compare/applicability/lineage evidence.

Tier notes:
- `core` passes when the short deterministic regressions, replay/reducer/fault gates, and the lighter split-choice invariance checks all pass.
- `slow` passes when the heavier day-to-day smokes, catalog/audit checks, and the non-nightly tie families all pass.
- `exhaustive` passes when the bounded explorer-heavy cases complete without new exact-audit or collision failures.
- `fuzz` passes when planner fuzz, corpus replay, and stats-oriented coverage reruns complete without oracle mismatches.
- `compare` passes when the registered exact-shadow vs exact-full representative checks, lineage/gate smokes, and sampled compare audits all pass.
- `policy_core` passes when the machine-readable manifest, refresh baseline/current round-trip, and diagnostic-only pinning stay stable.
- `policy_refresh` passes when baseline/current refresh, stale-family selection, applicability drift escalation, and bundle freshness metadata all behave as expected.
- `policy_sentinel` passes when cheap family-specific checks still support `PASS`, `NON_APPLICABLE`, and `DIAGNOSTIC_ONLY` without forcing a nightly rerun.
- `policy_nightly` passes when a real artifact-tree manifest can refresh as `FRESH` against the approved baseline, or when selective stale-family reruns restore the manifest to `FRESH` without reclassification.
- `asan_slow` passes when the sanitizer-tail subset clears under the ASan/UBSan environment.
- `nightly` passes when the opt-in heavy exhaustive/fuzz/tail coverage tests pass and `--case policy_gate --gate-strict` reports only `PASS`, `NON_APPLICABLE`, or `DIAGNOSTIC_ONLY`.

Checkpointed compare campaigns:
- Each completed chunk writes a per-chunk JSON/summary pair plus `latest.chk` and a `chunks/*.chk` manifest entry.
- Resume reuses the saved campaign-config snapshot, skips completed seed/iteration chunks, and rewrites merged per-run plus aggregate summaries after every chunk.
- The checkpoint manifest also persists compare mode, compare budget/sample rate, exact-canonical cap, compared/eligible targets, `max_partial_runs`, and `stop_when_gate_passes`.
- The merged stats files now emit compare timing/caching counters and a sibling `*.compare_profile.summary.txt` for quick hotspot inspection.
- Aggregate summaries emit `gate_status`, `stop_reason`, and per-family `run_gate=` lines with the effective compared/eligible/completed targets used for each run.
- Every campaign aggregate rewrite also refreshes `artifacts/manifests/policy_gate.json` plus sibling `.txt` and `.summary.txt`, so nightly/CI can evaluate exact-shadow policy retention without reparsing raw campaign logs.

Checkpoint / resume workflow:
```bash
./raw_engine_tests --case campaign \
  --campaign-config tests/campaigns/phase17_split_tie_organic_compare.txt \
  --checkpoint-dir artifacts/checkpoints/split_tie_organic \
  --checkpoint-every 4 \
  --target-compared-states 32 \
  --target-eligible-states 32 \
  --stop-when-gate-passes \
  --max-partial-runs 12

./raw_engine_tests --case campaign \
  --resume-from artifacts/checkpoints/split_tie_organic/latest.chk
```

Campaign usage notes:
- `--checkpoint-every <N>` is the chunk size used for resumable compare campaigns.
- `--max-wall-seconds <sec>` stops after the next checkpoint flush, not in the middle of a chunk.
- `--target-compared-states <N>` raises both the compared-state and completed-state gate threshold; `--target-eligible-states <N>` raises the eligible-state threshold above the built-in minima when you need stronger evidence.
- `--target-lineage-samples <N>` raises the minimum proxy-lineage sample count when a family depends on compare-ready representativeness evidence.
- `--target-applicability-confidence <float>` raises the minimum dominant ineligible-reason confidence required before a direct family may graduate as `NON_APPLICABLE`.
- `--stop-when-gate-passes` only stops after a checkpoint flush, and only when every production family in the campaign is `PASS` or `NON_APPLICABLE`; diagnostic families remain `DIAGNOSTIC_ONLY`.
- `--max-partial-runs <N>` is the operational backstop for nightly time-slicing; hitting it writes the current merged aggregate with `stop_reason=max_partial_runs` and leaves the policy manifest at `INSUFFICIENT_EVIDENCE` unless the configured gate already passed.
- `--stop-after-checkpoint` is useful for smoke-testing resume logic or for manually time-slicing long nightly compare runs.
- `--resume-from <path>` accepts either `latest.chk` or the checkpoint directory itself and restores the saved targets, compare settings, and gate-stop policy from the checkpoint manifest.
- Aggregate outputs are rewritten after every completed chunk, so interrupted runs still leave usable partial summaries.
- For compare-heavy nightly runs, prefer `ctest -L compare` for focused local validation, refresh the policy manifest with `./raw_engine_tests --case policy_gate --gate-strict --gate-output artifacts/manifests/policy_gate.json`, and keep the direct mixed-family applicability audit separate because it is expected to remain `NON_APPLICABLE` unless drift is detected.

Policy graduation gate:
- Canonical phase22 manifest paths are `artifacts/manifests/policy_graduation_manifest_v1.{json,txt,summary.txt}` for the current manifest, `artifacts/manifests/policy_graduation_manifest_phase21_approved_v1.{json,txt,summary.txt}` for the approved baseline, `artifacts/manifests/policy_graduation_manifest_refresh_phase22_v1.{json,txt,summary.txt}` for the refresh manifest, and `artifacts/manifests/policy_rerun_plan_phase22_v1.{json,txt,summary.txt}` for the rerun plan.
- Canonical phase22 lifecycle summaries are `artifacts/manifests/policy_pipeline_quick_phase22.{json,txt}` for quick checks and `artifacts/manifests/policy_pipeline_nightly_phase22.{json,txt}` for nightly refresh. The tools accept `.json` paths, but the C++ loader still reads the sibling `.txt` sidecar under the hood, so operator inspection should include both.
- Built-in family minima remain `split_tie_organic_symmetric >= 32/32/32`, `automorphism_probe_large >= 32/32/32`, `planner_tie_mixed_organic >= 48` applicability states with dominant `no_split_ready`, and `planner_tie_mixed_organic_compare_ready >= 32/32/32` compare plus `16` lineage samples.
- `PASS` means the family met its direct compared/eligible/completed targets with `split_choice_semantic_disagreement_count=0`, `split_choice_fallback_count=0`, and `semantic_shift_count=0`.
- `NON_APPLICABLE` means split-choice compare is stably not relevant for that family: applicability evidence met its audited-state target, compare/split-ready relevance stayed below threshold, and the dominant ineligible reason remained `no_split_ready` at or above the configured confidence.
- `DIAGNOSTIC_ONLY` means the family is useful for bounded exact compare or lineage monitoring, but it is not counted as production evidence. `planner_tie_mixed_organic_compare_ready` is intentionally pinned here.
- `FAIL` means a semantic disagreement, fallback, or semantic shift was observed.
- `INSUFFICIENT_EVIDENCE` means the family has not yet met its direct compare/applicability/lineage threshold, or a previously `NON_APPLICABLE` family started drifting and needs reclassification.
- `planner_tie_mixed_organic` now graduates through applicability audit, not through direct compare `PASS`.
- `planner_tie_mixed_organic_compare_ready` can still reach bounded compare `PASS` internally, but the machine-readable policy manifest always surfaces it as `DIAGNOSTIC_ONLY`.
- The exact-shadow retention statement should always separate evidence classes: direct compare families (`split_tie_organic_symmetric`, `automorphism_probe_large`) support `PASS`, `planner_tie_mixed_organic` supports `NON_APPLICABLE` through applicability evidence, and `planner_tie_mixed_organic_compare_ready` remains diagnostic lineage support only.
- Run `./raw_engine_tests --case policy_gate --gate-output artifacts/manifests/policy_graduation_manifest_v1.json` for the current manifest, `--gate-family <name>` for one family, and `--gate-strict` when CI/nightly should fail on `FAIL` or `INSUFFICIENT_EVIDENCE`.
- Run `./raw_engine_tests --case policy_gate_promote_baseline --source-manifest artifacts/manifests/policy_graduation_manifest_v1.json --baseline-out artifacts/manifests/policy_graduation_manifest_phase22_approved_v1.json --baseline-tag phase22-approved --require-acceptable-status --freeze-provenance` to promote the current manifest into an approved baseline with refresh-ready provenance. Promotion archives the previous baseline under `artifacts/manifests/history/`, refreshes `baseline_history_index.{txt,json}` and `history_index.{txt,json}`, and emits one `.meta.txt` record per promotion.
- Inspect `artifacts/manifests/history/baseline_history_index.{txt,json}` first when tracing a prior approval. Each `.meta.txt` record carries `baseline_tag`, `approval_timestamp_utc`, `report_file`, `bundle_file`, `baseline_manifest_path`, `promoted_from_manifest`, and `family_status_snapshot`.
- Run `./raw_engine_tests --case policy_gate_refresh --baseline-manifest artifacts/manifests/policy_graduation_manifest_v1.json --current-manifest artifacts/manifests/policy_graduation_manifest_v1.json --freshness-only --gate-output artifacts/manifests/policy_graduation_manifest_refresh_v1.json` for a quick freshness check.
- Run `./raw_engine_tests --case policy_gate_refresh --baseline-manifest artifacts/manifests/policy_graduation_manifest_phase21_approved_v1.json --current-manifest artifacts/manifests/policy_graduation_manifest_v1.json --freshness-only --gate-output artifacts/manifests/policy_graduation_manifest_refresh_phase22_v1.json` to validate that an approved baseline and a same-provenance current manifest remain `FRESH`.
- Run `./raw_engine_tests --case policy_gate_plan_rerun --baseline-manifest artifacts/manifests/policy_graduation_manifest_phase21_approved_v1.json --current-manifest artifacts/manifests/policy_graduation_manifest_v1.json --plan-out artifacts/manifests/policy_rerun_plan_phase22_v1.json` to build a stale-family rerun plan. Add `--family-filter <csv>`, `--include-diagnostic`, or `--include-non-applicable` when you want the plan to cover only selected families or include diagnostic/applicability-only reruns explicitly.
- Run `./raw_engine_tests --case policy_ci_check --baseline-manifest artifacts/manifests/policy_graduation_manifest_phase21_approved_v1.json --current-manifest artifacts/manifests/policy_graduation_manifest_v1.json --refresh-manifest artifacts/manifests/policy_graduation_manifest_refresh_phase22_v1.json --rerun-plan artifacts/manifests/policy_rerun_plan_phase22_v1.json --emit-summary artifacts/manifests/policy_ci_summary_phase22.txt --allow-empty-plan` for the cheap lifecycle materialization path. This case writes current/refresh/plan together but leaves the final operator verdict to the pipeline runner.
- Run `./raw_engine_tests --case policy_nightly_refresh --baseline-manifest artifacts/manifests/policy_graduation_manifest_phase21_approved_v1.json --current-manifest artifacts/manifests/policy_graduation_manifest_v1.json --refresh-manifest artifacts/manifests/policy_graduation_manifest_refresh_phase22_v1.json --rerun-plan artifacts/manifests/policy_rerun_plan_phase22_v1.json --emit-summary artifacts/manifests/policy_nightly_refresh_phase22.txt --allow-empty-plan` for the nightly lifecycle runner. It materializes current/refresh/plan first, includes diagnostic and non-applicable families in the rerun plan, executes only the selected stale entries, then rebuilds current/refresh/plan and leaves one short operator summary.
- Run `./raw_engine_tests --case policy_gate_refresh --baseline-manifest artifacts/manifests/policy_graduation_manifest_phase21_approved_v1.json --current-manifest artifacts/manifests/policy_graduation_manifest_v1.json --freshness-only --gate-output artifacts/manifests/policy_refresh_synthetic_stale_phase22.json --synthetic-hash-drift compare_engine_hash` when CI needs to prove that stale direct evidence still downgrades to `STALE` on a relevant compare-engine hash change.
- `policy_nightly_refresh` rerun modes are evidence-class aware: direct `PASS` families rerun compare campaigns with phase22 campaign configs and checkpoint directories under `artifacts/nightly_reruns/<family>/checkpoints`, `NON_APPLICABLE` families rerun `planner_tie_organic_applicability_audit` into `artifacts/phase22_applicability`, and `DIAGNOSTIC_ONLY` families rerun `compare_ready_lineage_audit` into `artifacts/phase22_lineage`.
- Run `./raw_engine_tests --case policy_gate_refresh --baseline-manifest artifacts/manifests/policy_graduation_manifest_phase21_approved_v1.json --current-manifest artifacts/manifests/policy_graduation_manifest_v1.json --gate-output artifacts/manifests/policy_refresh_synthetic_reclassify_phase22.json --synthetic-applicability-drift planner_tie_mixed_organic` or `--gate-output artifacts/manifests/policy_refresh_synthetic_diagnostic_phase22.json --synthetic-diagnostic-promotion planner_tie_mixed_organic_compare_ready` when you need a cheap operator test that `NON_APPLICABLE` drift and diagnostic-promotion rejection still escalate to `REQUIRES_RERUN`.
- Selective rerun is evidence-class aware: direct `PASS` families rerun compare campaigns, `NON_APPLICABLE` families rerun applicability audits, and `DIAGNOSTIC_ONLY` families rerun lineage audits. `planner_tie_mixed_organic_compare_ready` remains diagnostic-only even after a successful rerun.
- A `policy_nightly_refresh` summary with `selected_entry_count=0` is the expected no-op steady state. When stale families appear, the rerun plan and the execution summary are regenerated together so operators can see which evidence class was rerun and whether the manifest returned to `FRESH`.
- Campaign aggregate summaries plus the policy manifest are the inputs nightly automation should use to decide whether `exact_shadow` stays graduated.

Evidence freshness:
- The approved baseline manifest is the last accepted policy snapshot with frozen provenance. The current manifest is the evidence measured or loaded for the present run. The refresh manifest is the baseline-vs-current comparison artifact that drives stale/rerun decisions.
- Lifecycle runner semantics are fixed: `policy_ci_check` keeps baseline/current/refresh/rerun-plan in sync for cheap CI, `policy_nightly_refresh` reruns only stale families, and both emit a short machine-readable summary that can be copied into reports or CI logs.
- Baseline promotion now records `baseline_version`, `promoted_from_report`, `promoted_from_manifest`, `baseline_tag`, `approval_timestamp_utc`, and `provenance_frozen`, while preserving the source manifest's family-level relevant-input hashes when provenance is frozen so refresh can compare like-for-like evidence.
- Baseline history is append-only under `artifacts/manifests/history/`: every promotion keeps the previous sidecars, emits a `.meta.txt` record, and rewrites `baseline_history_index.{txt,json}` so operators can trace which baseline tag, report, and bundle justified approval.
- Every family now carries `current_status` plus `freshness_status={FRESH|STALE|REQUIRES_RERUN}`.
- `PASS` families go stale when direct-compare hashes change, `NON_APPLICABLE` families go stale when applicability-audit inputs change, and `DIAGNOSTIC_ONLY` families go stale when lineage/diagnostic inputs change.
- `planner_tie_mixed_organic_compare_ready` always remains `DIAGNOSTIC_ONLY` and `counts_as_production_evidence=false`, even when semantic/final/trace consistency is perfect.
- `planner_tie_mixed_organic` flips from `NON_APPLICABLE` to `REQUIRES_RERUN` as soon as applicability drift appears through nonzero compare-eligible states, split-ready relevance drift, or loss of dominant `no_split_ready`.
- If the approved baseline and current manifest share the same relevant-input provenance, refresh should remain `FRESH`; stale families should then be sliced into a rerun plan instead of forcing a whole-nightly rerun.
- The rerun plan is intentionally evidence-class aware: `PASS` families schedule direct compare reruns, `NON_APPLICABLE` families schedule applicability audits, and `DIAGNOSTIC_ONLY` families schedule lineage audits without promoting compare-ready evidence into production evidence.
- Synthetic validation hooks are available for policy-only tests: `--synthetic-hash-drift <field>`, `--synthetic-applicability-drift <family>`, and `--synthetic-diagnostic-promotion <family>` let CI prove that stale, drift, and diagnostic-promotion rejection paths still fire before a real regression appears.
- `exact_shadow` should be considered broken and reclassified only when one of these happens: a production family falls to `FAIL`, a `NON_APPLICABLE` family begins producing meaningful compare-eligible states, or a diagnostic family starts counting as production evidence.

Policy pipeline:
- Correctness and runtime baselines are separate on purpose. The correctness baseline tracks exact-shadow evidence freshness and family classification; the runtime baseline tracks wall-clock budgets on a comparable host/toolchain fingerprint. Refresh correctness when evidence hashes change, refresh runtime when the same host/toolchain rerun drifts, and promote a new runtime baseline when the fingerprint changes.
- Runtime baseline management is registry-based now. `artifacts/manifests/runtime_baseline_registry_v1.json` is the active operator registry, `policy_runtime_current_*.json` is the measured current run, `policy_runtime_refresh_*.json` is the strict comparison against the selected registry baseline, `policy_runtime_rerun_*.json` is the runtime rerun plan, `runtime_history_index_v1.json` is the advisory trend archive, and `runtime_rebaseline_proposal_*.json` is the machine-readable operator proposal when the registry no longer gives a strict comparable baseline.
- `python tests/tools/run_policy_pipeline.py --mode quick --baseline-manifest artifacts/manifests/policy_graduation_manifest_phase22_approved_v1.json --current-manifest artifacts/manifests/policy_graduation_manifest_v1.json --refresh-manifest artifacts/manifests/policy_graduation_manifest_refresh_phase22_v1.json --rerun-plan artifacts/manifests/policy_rerun_plan_phase22_v1.json --runtime-baseline-manifest artifacts/manifests/policy_runtime_baseline_phase22_approved.json --runtime-current-manifest artifacts/manifests/policy_runtime_current_phase23.json --runtime-refresh-manifest artifacts/manifests/policy_runtime_refresh_phase23.json --runtime-rerun-plan artifacts/manifests/policy_runtime_rerun_phase23.json --artifact-root artifacts --summary-out artifacts/manifests/policy_pipeline_quick_phase23.json --strict` is the cheap operator path. It reuses or regenerates correctness and runtime lifecycle artifacts together and emits `policy_pipeline_<mode>_<phase>.{json,txt}` with combined severity plus exit code.
- `python tests/tools/run_policy_pipeline.py --mode nightly --baseline-manifest artifacts/manifests/policy_graduation_manifest_phase22_approved_v1.json --current-manifest artifacts/manifests/policy_graduation_manifest_v1.json --refresh-manifest artifacts/manifests/policy_graduation_manifest_refresh_phase22_v1.json --rerun-plan artifacts/manifests/policy_rerun_plan_phase22_v1.json --runtime-baseline-manifest artifacts/manifests/policy_runtime_baseline_phase22_approved.json --runtime-current-manifest artifacts/manifests/policy_runtime_current_phase23.json --runtime-refresh-manifest artifacts/manifests/policy_runtime_refresh_phase23.json --runtime-rerun-plan artifacts/manifests/policy_runtime_rerun_phase23.json --artifact-root artifacts --report-out PHASE23_STABILIZATION_REPORT.txt --summary-out artifacts/manifests/policy_pipeline_nightly_phase23.json --strict` is the operator refresh path. It runs the correctness lifecycle, refreshes stale runtime entries, emits a combined summary, and then calls the evidence-bundle builder.
- `python tests/tools/run_policy_pipeline.py --mode full_local ...` is the local superset for the same lifecycle flow when you want the nightly path plus bundle generation under one command.
- `python tests/tools/run_policy_pipeline.py --mode bundle_only ...` skips refresh/rerun and only packages the already-materialized correctness/runtime lifecycle state.
- `python tests/tools/run_policy_pipeline.py --mode rebaseline_candidate ...` is the operator pre-approval path for a new runner or toolchain. It keeps correctness green, refreshes runtime against the registry, writes `runtime_rebaseline_proposal_*.json`, and exits `ACTION_REQUIRED` when the current environment needs an explicit runtime-baseline promotion.
- `python tests/tools/run_policy_pipeline.py --mode matrix --baseline-manifest ... --runtime-baseline-registry artifacts/manifests/runtime_baseline_registry_v1.json --runtime-history-index artifacts/manifests/runtime_history_index_v1.json --artifact-root artifacts --summary-out artifacts/manifests/policy_pipeline_matrix_phase25.json --strict` is the synthetic multi-environment runbook path. It evaluates a list of fixture entries one by one and records `selected_baseline_id`, `selected_baseline_tag`, `comparability_verdict`, `severity`, `recommended_action`, and `verification_status` for each entry.
- Phase24 runtime-aware quick path is `python tests/tools/run_policy_pipeline.py --mode quick --baseline-manifest artifacts/manifests/policy_graduation_manifest_phase22_approved_v1.json --current-manifest artifacts/manifests/policy_graduation_manifest_v1.json --refresh-manifest artifacts/manifests/policy_graduation_manifest_refresh_phase23_v1.json --rerun-plan artifacts/manifests/policy_rerun_plan_phase23_v1.json --runtime-baseline-manifest artifacts/manifests/policy_runtime_baseline_phase23_approved.json --runtime-baseline-registry artifacts/manifests/runtime_baseline_registry_v1.json --runtime-current-manifest artifacts/manifests/policy_runtime_current_phase23.json --runtime-refresh-manifest artifacts/manifests/policy_runtime_refresh_phase23_v2.json --runtime-rerun-plan artifacts/manifests/policy_runtime_rerun_phase23_v2.json --runtime-history-index artifacts/manifests/runtime_history_index_v1.json --runtime-proposal artifacts/manifests/runtime_rebaseline_proposal_phase24.json --artifact-root artifacts --summary-out artifacts/manifests/policy_pipeline_quick_phase24.json --strict`. This selects the best active runtime baseline from the registry, emits a refresh verdict, and writes a rebaseline proposal only when the registry stops being strictly comparable.
- Runtime lifecycle artifacts are the quartet `runtime current manifest`, `runtime approved baseline`, `runtime refresh manifest`, and `runtime rerun plan`. They track `release_full`, `debug_full`, `asan_full`, `policy_core`, `policy_refresh`, `policy_nightly`, and optional compare-campaign classes with host/toolchain fingerprints, wall times, test counts, and budget thresholds.
- Runtime comparability is strict only on the same fingerprint. `COMPARABLE` means same host/toolchain/build/sanitizer class, `NOT_COMPARABLE` means the runtime evidence is informational only, and `REBASELINE_REQUIRED` means the runtime baseline must be promoted again on the new host/toolchain before strict budget comparisons are meaningful.
- Runtime registry selection uses three buckets: `exact match`, `compatible match`, and `no match`. Exact match keeps strict budget comparison. Compatible match is reported as `NOT_COMPARABLE` and never mutates correctness retention. No match is `REBASELINE_REQUIRED`, which means operators should generate a proposal and explicitly approve a new runtime baseline for the new host/toolchain. On a fresh host or toolchain change, the expected operator flow is: select baseline -> see `REBASELINE_REQUIRED` -> inspect `runtime_rebaseline_proposal_*.json` -> rerun full validation -> explicitly promote the new runtime baseline into the registry.
- Runtime multi-fingerprint fixtures intentionally cover `exact_current`, `same_host_compiler_bump`, `sanitizer_change`, `runner_tag_change`, `cross_host`, and `retired_only`. The expected selection outputs are `candidate_count`, `exact_match_count`, `compatible_match_count`, `retired_match_count`, and `comparability_reason`, so operators can tell whether a result is a strict exact match, a compatible informational match, or a real rebaseline boundary.
- Runtime history is advisory only. `runtime_history_index_v1.json` and its `*_summary.json` sidecar keep per-fingerprint wall-time samples, rolling median, p90/p95, MAD-based jitter estimate, and `trend_direction={stable|noisy|regressing|improved|insufficient_history}`. The trend archive never changes correctness gate results by itself; it is there so nightly operators can see long-term drift before it hits the hard budget.
- Runtime triage is intentionally more specific than severity. `runtime_recommendation` is one of `NO_ACTION`, `WATCH_RUNTIME`, `INVESTIGATE_RUNTIME_DRIFT`, `PROPOSE_REBASELINE`, `REBASELINE_REQUIRED`, `NOT_COMPARABLE`, or `FAIL`. Use it as the operator-facing next step while keeping severity/exit code stable for CI.
- Runtime watch is now a sibling lifecycle, not a replacement for runtime refresh. Read `runtime_watch_current_*.json`, `runtime_watch_refresh_*.json`, and `runtime_watch_history_index_v1.json` when a class is over a soft budget but still comparable/fresh. The watch state machine is `CLEAR -> WATCH -> WATCH_STABLE -> REBASELINE_CANDIDATE`, with `WATCH_ESCALATE`, `REBASELINE_REQUIRED`, and `FAIL` as escalation exits.
- Role matters. `release_full` and `debug_full` are `production_critical`, `asan_full` is `diagnostic`, and `policy_*` classes are `operator`. A diagnostic soft overrun should normally land in `WATCH_RUNTIME` or `CONTINUE_MONITORING`; a production-critical hard overrun should move straight to `ACTION_REQUIRED` or `FAIL`.
- Use repeated same-fingerprint sampling to close a watch: `./raw_engine_tests --case runtime_watch_campaign --execution-class asan_full --repeat 5 --runtime-baseline-manifest artifacts/manifests/policy_runtime_baseline_phase25_approved.json --runtime-watch-out artifacts/manifests/runtime_watch_current_phase27.json --artifact-dir artifacts/phase27_watch` followed by `./raw_engine_tests --case runtime_watch_refresh --runtime-baseline-manifest artifacts/manifests/policy_runtime_baseline_phase25_approved.json --runtime-current-manifest artifacts/manifests/policy_runtime_current_phase25.json --runtime-watch-current artifacts/manifests/runtime_watch_current_phase27.json --runtime-watch-refresh artifacts/manifests/runtime_watch_refresh_phase27.json`.
- Interpret combined pipeline `WARN` carefully. If correctness stays `PASS/FRESH` and the only runtime signal is a diagnostic-class watch, the expected operator action is `WATCH_RUNTIME` or `CONTINUE_MONITORING`, not a correctness reclassification. Escalate only when watch refresh shows hard breaches, sustained severe regression, or a promotion into `REBASELINE_CANDIDATE`.
- Bundle/operator reading order for watch-only states is: `policy_pipeline_<mode>_phase27.json` -> `runtime_watch_refresh_phase27.json` -> `runtime_watch_current_phase27.json` -> `runtime_watch_history_index_v1_summary.json` -> `runtime_budget_profile_phase27.json` -> `bundle_metadata.json`. `bundle_metadata.json` carries `runtime_watch_status`, `runtime_watch_reason`, `runtime_watch_sample_count`, `runtime_watch_recommendation`, `runtime_budget_profile_id`, and `diagnostic_watch_only`.
- Severity is fixed as `OK`, `WARN`, `ACTION_REQUIRED`, `FAIL`. `OK` means correctness is `FRESH` with an empty rerun plan and runtime is within budget. `WARN` means correctness is still healthy but runtime exceeded a soft budget without needing a rebaseline. `ACTION_REQUIRED` means stale runtime or correctness evidence, `requires_rerun`, `reclassify_required`, or `REBASELINE_REQUIRED`. `FAIL` means manifest corruption, a failing policy family, a semantic mismatch, or a hard runtime budget violation.
- Pipeline exit codes are fixed as `0=OK`, `10=WARN`, `20=ACTION_REQUIRED`, `30=FAIL`. CI should treat `10` as non-promotable but operator-actionable, and `20+` as a hard gate.
- `recommended_next_action` is part of the machine-readable summary. `OK` keeps exact_shadow as-is, `WARN` points operators to `policy_nightly_refresh` or the rerun plan, `ACTION_REQUIRED` points to rerun or reclassification work, and `FAIL` means operator triage before any promotion.
- When stale families appear, follow the rerun plan instead of re-running everything: direct `PASS` families rerun compare campaigns, `NON_APPLICABLE` families rerun applicability audit, and `DIAGNOSTIC_ONLY` families rerun lineage audit. `planner_tie_mixed_organic_compare_ready` stays diagnostic-only after rerun.
- If runtime lifecycle artifacts are supplied, the pipeline also emits `runtime_current_verdict`, `runtime_freshness_verdict`, `runtime_comparability_verdict`, and `runtime_budget_verdict`. Soft overrun maps to `BUDGET_WARN`; hard overrun maps to `BUDGET_FAIL`. This is separate from correctness, so the operator summary can say `policy OK but budget warn`.
- Runtime watch is a sibling lifecycle on top of budget verdicts. `runtime_watch_current_*.json` captures repeated same-fingerprint sampling, `runtime_watch_refresh_*.json` records the current watch state after comparing baseline/current/history, and `runtime_watch_history_index_v1.json` keeps the transition log. The watch state machine is `CLEAR`, `WATCH`, `WATCH_STABLE`, `WATCH_ESCALATE`, `REBASELINE_CANDIDATE`, `REBASELINE_REQUIRED`, `FAIL`.
- Execution classes are role-sensitive. `release_full` and `debug_full` are `production_critical`, `asan_full` is `diagnostic`, and `policy_*` classes are `operator`. A soft overrun on a diagnostic class should normally land in `WATCH` or `WATCH_STABLE`; a hard overrun on a production-critical class should escalate to `ACTION_REQUIRED` or `FAIL`.
- Use repeated same-fingerprint sampling before changing a runtime baseline. `./raw_engine_tests --case runtime_watch_campaign --execution-class asan_full --repeat 5 --runtime-baseline-manifest <baseline.json> --runtime-watch-out <watch_current.json> --artifact-dir <artifact-dir>` collects bounded repeated samples, and `./raw_engine_tests --case runtime_watch_refresh --runtime-baseline-manifest <baseline.json> --runtime-current-manifest <current.json> --runtime-watch-current <watch_current.json> --runtime-watch-refresh <watch_refresh.json>` converts them into a watch verdict.
- The default operator interpretation is fixed: `WATCH_RUNTIME` means a live soft overrun exists, `CONTINUE_MONITORING` means the overrun is stable and bounded across repeated samples, `PROPOSE_RUNTIME_REBASELINE` means a stable watch persisted long enough to justify a new proposal, and `INVESTIGATE_RUNTIME_DRIFT` means escalation because hard breach or severe sustained drift appeared.
- `quick` and `nightly` can legitimately return `WARN` even while correctness is fully green. If the rationale says the watch is diagnostic-only and bounded, keep monitoring and do not touch correctness baselines or `src/`. Only escalate to rebaseline or drift investigation when the watch lifecycle says so.
- `planner_tie_mixed_organic_compare_ready` remains diagnostic-only even when runtime registry/proposal artifacts are healthy. Runtime operations never promote compare-ready evidence into production correctness evidence.
- Runtime registry approval flow is explicit: `registry-promote-baseline --activate` creates or updates the active entry for the current fingerprint, `--retire-baseline <id>` keeps the old entry in history without deleting it, and `runtime_rebaseline_proposal_*.json` is review-only until an operator promotes a new approved runtime baseline.
- Runtime proposal approval is a separate operator step now. `./raw_engine_tests --case runtime_approve_rebaseline --runtime-current-manifest <current.json> --proposal-out <proposal.json> --runtime-baseline-registry artifacts/manifests/runtime_baseline_registry_v1.json --runtime-baseline-out <approved.json> --baseline-tag <tag> --activate --archive-proposal <archived_proposal.json> --require-acceptable-status` consumes the proposal, writes the approved runtime baseline, retires the previous active registry entry for the active rebaseline transition, archives the reviewed proposal snapshot, and writes `<approved>_approval_metadata.json`.
- Treat the archived proposal as immutable evidence. After approval, keep `runtime_rebaseline_proposal_*_archived.json` for bundle/reporting only and write any post-approval no-op proposal checks to a fresh path such as `runtime_rebaseline_proposal_phase26.json`.
- After approval, rerun `runtime_gate_refresh`, `runtime_gate_plan_rerun`, and then `run_policy_pipeline.py --mode quick|nightly` against the updated registry/baseline/current artifacts. The healthy post-approval target is `runtime comparability=COMPARABLE`, `freshness=FRESH`, `rerun plan=PASS/empty`, and combined pipeline `OK` or an explicit runtime-only `WARN` when a soft budget warning remains.
- `REBASELINE_REQUIRED` or `NOT_COMPARABLE` in runtime does not change correctness retention by itself. `exact_shadow` can stay fully green while runtime asks for operator approval because runtime compares wall-clock evidence against a host/toolchain registry instead of semantic evidence.
- Approval and transition breadcrumbs are intentionally easy to find: the approved baseline manifest carries `approval_metadata`, the approval sidecar sits next to it as `*_approval_metadata.json`, the archived proposal keeps the pre-approval recommendation, and `runtime_history_index_v1_summary.json` exposes `transition_count` plus `recent_transitions` after the first post-approval append.

Evidence bundle:
- `python tests/tools/build_evidence_bundle.py --phase phase23 --artifact-root artifacts --report-out PHASE23_STABILIZATION_REPORT.txt --policy-manifest artifacts/manifests/policy_graduation_manifest_v1.json --baseline-manifest artifacts/manifests/policy_graduation_manifest_phase22_approved_v1.json --current-manifest artifacts/manifests/policy_graduation_manifest_v1.json --refresh-manifest artifacts/manifests/policy_graduation_manifest_refresh_phase22_v1.json --rerun-plan artifacts/manifests/policy_rerun_plan_phase22_v1.json --runtime-baseline-manifest artifacts/manifests/policy_runtime_baseline_phase22_approved.json --runtime-current-manifest artifacts/manifests/policy_runtime_current_phase23.json --runtime-refresh-manifest artifacts/manifests/policy_runtime_refresh_phase23.json --runtime-rerun-plan artifacts/manifests/policy_runtime_rerun_phase23.json --pipeline-summary artifacts/manifests/policy_pipeline_nightly_phase23.json --zip-out raw_engine_phase23_stabilization.zip --curated-zip raw_engine_phase23_stabilization_curated.zip` assembles the report, approved baseline, current manifest, refresh manifest, rerun plan, runtime lifecycle manifests, pipeline summary, selected logs, regressions, and curated artifacts into one bundle.
- The bundle builder now also emits one delivery zip by default. Unless `--delivery-zip <path>` overrides it, the output sits next to `--zip-out` as `..._delivery.zip` and contains the top-level deliverables only: report, correctness current/baseline/refresh/rerun-plan artifacts, runtime current/baseline/refresh/rerun-plan artifacts, nightly pipeline summary, bundle metadata, bundle zip, and curated zip.
- Inspect `<artifact-root>/phase23_evidence_bundle/bundle_metadata.json` first after bundle generation. The expected top-level layout is `reports/`, `manifests/`, `curated/`, `regressions/`, and `logs/`.
- When `--refresh-manifest` and `--rerun-plan` are present, the bundle metadata also records `refresh_manifest_hash`, `rerun_plan_hash`, `stale_family_count`, `requires_rerun_family_count`, and `reclassify_required_count`.
- Bundle layout is fixed as `reports/`, `manifests/`, `curated/`, `regressions/`, and `logs/`.
- `bundle_metadata.json` now carries baseline/current/refresh/rerun-plan paths and hashes for both correctness and runtime, current/freshness/comparability verdicts, a short `policy_summary`, refresh rollup counts, drift flags, reclassification-needed families, runtime budget rollup, and a compact family status table so CI/nightly can inspect exact-shadow retention and runtime drift without unpacking every manifest.
- The bundle now also carries runtime registry/history/proposal state: `runtime_baseline_registry_v1.json`, `runtime_history_index_v1.json`, `runtime_history_index_v1_summary.json`, and `runtime_rebaseline_proposal_*.json` when supplied. Start from `bundle_metadata.json` and read `runtime_registry_hash`, `runtime_history_index_hash`, `runtime_proposal_hash`, `runtime_selected_baseline_id`, and `runtime_rebaseline_proposal_needed` before opening the raw runtime manifests.
- When a runtime rebaseline transition happened, the bundle also carries the approval sidecar and transition metadata. Read `approved_runtime_baseline_hash`, `proposal_archive_hash`, `previous_active_runtime_baseline_id`, `new_active_runtime_baseline_id`, `runtime_transition_status`, and `combined_pipeline_status_after_rebaseline` in `bundle_metadata.json` before unpacking the raw manifests.
- When runtime watch is enabled, start from `bundle_metadata.json` and read `runtime_watch_status`, `runtime_watch_reason`, `runtime_watch_sample_count`, `runtime_watch_recommendation`, `runtime_budget_profile_id`, and `diagnostic_watch_only`. Then open `runtime_watch_refresh_*.json`, `runtime_watch_current_*.json`, and `runtime_watch_history_index_v1_summary.json` in that order.
- Phase28 keeps the naming aligned with the normal stabilization flow even though the new evidence is runtime-watch heavy. The canonical operator outputs are `PHASE28_STABILIZATION_REPORT_20260324.txt`, `artifacts/manifests/policy_pipeline_quick_phase28.json`, `artifacts/manifests/policy_pipeline_nightly_phase28.json`, `artifacts/manifests/policy_pipeline_matrix_phase28.json`, `artifacts/manifests/runtime_watch_current_phase28.json`, `artifacts/manifests/runtime_watch_refresh_phase28.json`, and the root-level `raw_engine_phase28_stabilization_20260324.zip` plus curated and delivery companions.
- `run_policy_pipeline.py` now defaults to dated root deliverables when `--report-out` / `--zip-out` / `--curated-zip` are omitted, so nightly/full_local publication does not leave a stale undated stub report or bundle behind the newer phase report.
- Interpret same-fingerprint and multi-fingerprint results separately. `quick` and `nightly` are the same-host/operator view, so phase28 intentionally lands on `WARN + CONTINUE_MONITORING` because `asan_full` remains a bounded diagnostic-only soft overrun across repeated same-fingerprint samples. `matrix` is the cross-environment readiness view, so it can return `ACTION_REQUIRED` when `same_host_compiler_bump`, `sanitizer_change`, `runner_tag_change`, `cross_host`, or `retired_only` lose strict comparability even though correctness stays green.
- Phase28 multi-fingerprint fixtures always record `selected_baseline_id`, `selected_baseline_tag`, `candidate_count`, `exact_match_count`, `compatible_match_count`, `retired_match_count`, and `comparability_reason`. Read `policy_pipeline_matrix_phase28.json` before opening per-entry manifests so you can distinguish `COMPARABLE` same-host evidence from `NOT_COMPARABLE` or `REBASELINE_REQUIRED` cross-environment evidence.
- Concurrency-safe artifact policy is now fixed: every pipeline or child `raw_engine_tests` invocation must pass an explicit `--artifact-dir`, JSON/text manifest writes are atomic, and filesystem-heavy matrix/bundle/parallel smokes are registered only in the nightly-only bucket. Direct phase16 manifest compatibility tests also stay `RUN_SERIAL` so shared checkpoint and manifest paths cannot race under parallel `ctest`.
- When `--pipeline-matrix-summary` is supplied, the bundle also preserves the synthetic matrix verdicts. Read `pipeline_matrix_summary`, `pipeline_matrix_summary_data`, `runtime_recommendation`, `runtime_trend_summary`, `selected_runtime_baseline_id`, and `selected_runtime_baseline_tag` in `bundle_metadata.json` before diving into the raw registry/history/proposal files.
- When both quick and nightly operator views matter, pass `--pipeline-quick-summary` and `--pipeline-summary` together. The delivery bundle will then carry `quick_pipeline_summary`, `nightly_pipeline_summary`, and `pipeline_matrix_summary` side by side so the same-fingerprint `WARN + CONTINUE_MONITORING` view is not confused with the cross-environment matrix `ACTION_REQUIRED` view.
- The curated slice also carries the latest approved baseline, current manifest, refresh manifest, rerun plan, `policy_summary.txt`, and the manifest-linked applicability/compare-ready lineage summaries when they are present.
- Curated/runtime operator flow is fixed: `policy_pipeline_<mode>_*.json` -> `policy_pipeline_matrix_*.json` when present -> `policy_runtime_refresh_*.json` -> `runtime_baseline_registry_v1.json` -> `runtime_history_index_v1_summary.json` -> `runtime_rebaseline_proposal_*.json`.
- After a runtime approval, the preferred curated reading order is `policy_pipeline_<mode>_*.json` -> approved `policy_runtime_baseline_*.json` + `*_approval_metadata.json` -> archived `runtime_rebaseline_proposal_*.json` -> `policy_runtime_refresh_*.json` -> `runtime_baseline_registry_v1.json` -> `runtime_history_index_v1_summary.json`.
- Re-running the bundle script with the same inputs is idempotent at the metadata level: the manifest sidecars and policy summary stay stable, and only the bundle timestamp tracks the report timestamp.
- `build_evidence_bundle.py` now also accepts `--pipeline-summary`, `--runtime-manifest`, `--bundle-index-out`, `--delivery-zip`, and `--prune-artifacts`. The bundle metadata records pipeline/runtime hashes plus the delivery zip payload, and the index files summarize reports, manifests, bundles, delivery bundles, curated bundles, regressions, logs, checkpoints, and nightly runs.
- Retention is intentionally conservative: keep the latest approved baselines, keep the canonical current/refresh manifests, prune old bundle zips or curated zips by count, prune old nightly run directories by count, and never prune regression artifacts automatically.
- Artifact pruning is tied to the bundle output root. Use `--prune-artifacts --max-bundles <N> --max-nightly-runs <N> --keep-approved` to retain the newest approved baseline, the latest current/refresh manifests, the newest bundle zips, and the newest nightly run directories while keeping regression artifacts forever.

Optional warning hygiene for tests only:
```bash
cmake -S . -B build-debug-strict -DCMAKE_BUILD_TYPE=Debug -DRAW_ENGINE_TEST_STRICT_WARNINGS=ON
cmake --build build-debug-strict -j
```

Known fixed repros:
```bash
./raw_engine_tests --case regression_44001
./raw_engine_tests --case regression_isolate_split_no_sep
```

Coverage quality gates:
- `exhaustive_split_ready_smoke`
- `exhaustive_join_ready_smoke`
- `exhaustive_integrate_ready_smoke`
- `exhaustive_mixed_smoke`
- `exhaustive_canonical_dedupe_smoke`
- `exhaustive_natural_dedupe_smoke`
- `exhaustive_family_sweep_smoke`
- `exhaustive_collision_guard_smoke`
- `exhaustive_natural_dedupe_large_smoke`
- `exhaustive_organic_duplicate_examples_smoke`
- `metamorphic_relabel_invariance`
- `metamorphic_occid_invariance`
- `metamorphic_edge_order_invariance`
- `metamorphic_vertex_order_invariance`
- `replay_serialization_invariance`
- `metamorphic_family_matrix_smoke`
- `metamorphic_planner_multistep_smoke`
- `metamorphic_replay_matrix_smoke`
- `split_choice_oracle_smoke`
- `split_choice_relabel_invariance`
- `split_choice_edge_order_invariance`
- `split_choice_vertex_order_invariance`
- `split_choice_oracle_regression`
- `split_choice_policy_smoke`
- `split_choice_policy_relabel_invariance`
- `split_choice_policy_edge_order_invariance`
- `split_choice_policy_vertex_order_invariance`
- `split_choice_policy_occid_invariance`
- `split_choice_policy_multiclass_smoke`
- `exact_canonicalizer_smoke`
- `fast_vs_exact_canonical_dedupe_smoke`
- `split_choice_exact_class_smoke`
- `split_choice_exact_relabel_invariance`
- `split_choice_exact_vertex_order_invariance`
- `split_choice_exact_edge_order_invariance`
- `planner_relabel_structural_regression`
- `planner_targeted_split_smoke`
- `planner_targeted_join_smoke`
- `planner_targeted_integrate_smoke`
- `planner_targeted_mixed_smoke`
- `planner_coverage_smoke`
- `planner_random_coverage_smoke`
- `planner_weighted_coverage_smoke`
- `planner_join_ready_smoke`
- `planner_integrate_ready_smoke`
- `planner_structural_mixed_smoke`
- `planner_tie_mixed_smoke`
- `planner_tie_mixed_exhaustive_smoke`
- `planner_tie_symmetric_smoke`
- `canonical_collision_probe_smoke`
- `split_choice_representative_shift_smoke`
- `split_choice_harmless_shift_smoke`
- `split_choice_semantic_shift_smoke`
- `split_choice_semantic_shift_regression`
- `split_tie_organic_symmetric_smoke`
- `planner_tie_mixed_organic_smoke`
- `automorphism_probe_large_smoke`
- `sampled_exact_audit_smoke`
- `duplicate_attribution_smoke`

Sync policy:
- `include/`, `src/`, `tests/`, `CMakeLists.txt` are the authoritative source tree.
- `raw_engine_v1.cpp` is a compatibility/export artifact, not the source of truth.
- New implementation/debug/test work must land in the split source tree first.
- If a standalone snapshot is needed again, regenerate/export from the split tree instead of patching `raw_engine_v1.cpp` directly.
