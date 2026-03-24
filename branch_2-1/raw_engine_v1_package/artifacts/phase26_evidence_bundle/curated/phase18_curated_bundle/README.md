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
- `--gate-family <name>` filters `--case policy_gate` to one named family
- `--gate-strict` makes `--case policy_gate` fail unless every selected family is `PASS`, `NON_APPLICABLE`, or `DIAGNOSTIC_ONLY`
- `--gate-output <path>` writes policy-gate json plus sibling text/summary outputs
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
ctest --test-dir build-asan -L asan_slow --output-on-failure
```

Plain `ctest --test-dir build-debug --output-on-failure` now runs the default registered suite with nightly-only tail smokes omitted. Enable `-DRAW_ENGINE_REGISTER_NIGHTLY_TESTS=ON` when you want the expensive compare/exhaustive/nightly cases registered into CTest, then use `ctest -L nightly` or `ctest -L compare`.

Tiering rules:
- `core` is the short deterministic gate for regular local iteration.
- `slow` is the heavier day-to-day tier that still fits a normal debug/release pass.
- `exhaustive` isolates explorer-heavy bounded-state tests.
- `fuzz` isolates planner fuzz, corpus replay, and stats-oriented coverage cases.
- `compare` isolates exact-shadow vs exact-full representative checks, sampled exact audits, lineage/gate smokes, and collision/tie compare regressions.
- `asan_slow` is the sanitizer-tail subset that is intentionally safe to skip from a quick ASan pass.
- `nightly` is opt-in: those tests are not even registered unless `-DRAW_ENGINE_REGISTER_NIGHTLY_TESTS=ON` is set at configure time, and it is the tier where the policy gate is refreshed from compare/applicability/lineage evidence.

Tier notes:
- `core` passes when the short deterministic regressions, replay/reducer/fault gates, and the lighter split-choice invariance checks all pass.
- `slow` passes when the heavier day-to-day smokes, catalog/audit checks, and the non-nightly tie families all pass.
- `exhaustive` passes when the bounded explorer-heavy cases complete without new exact-audit or collision failures.
- `fuzz` passes when planner fuzz, corpus replay, and stats-oriented coverage reruns complete without oracle mismatches.
- `compare` passes when the registered exact-shadow vs exact-full representative checks, lineage/gate smokes, and sampled compare audits all pass.
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
- Built-in phase18 family minima are `split_tie_organic_symmetric >= 32/32/32`, `automorphism_probe_large >= 32/32/32`, `planner_tie_mixed_organic >= 48` audited states with dominant `no_split_ready` confidence and zero compare-eligible drift, and `planner_tie_mixed_organic_compare_ready >= 32/32/32` compare plus `16` lineage samples.
- `PASS` means the family met its direct compared/eligible/completed targets with `split_choice_semantic_disagreement_count=0`, `split_choice_fallback_count=0`, and `semantic_shift_count=0`.
- `NON_APPLICABLE` means split-choice compare is stably not relevant for that family: applicability evidence met its audited-state target, compare/split-ready relevance stayed below threshold, and the dominant ineligible reason remained `no_split_ready` at or above the configured confidence.
- `DIAGNOSTIC_ONLY` means the family is useful for bounded exact compare or lineage monitoring, but it is not counted as production evidence. `planner_tie_mixed_organic_compare_ready` is intentionally pinned here.
- `FAIL` means a semantic disagreement, fallback, or semantic shift was observed.
- `INSUFFICIENT_EVIDENCE` means the family has not yet met its direct compare/applicability/lineage threshold, or a previously `NON_APPLICABLE` family started drifting and needs reclassification.
- `planner_tie_mixed_organic` now graduates through applicability audit, not through direct compare `PASS`.
- `planner_tie_mixed_organic_compare_ready` can still reach bounded compare `PASS` internally, but the machine-readable policy manifest always surfaces it as `DIAGNOSTIC_ONLY`.
- Run `./raw_engine_tests --case policy_gate --gate-output artifacts/manifests/policy_gate.json` for the full manifest, `--gate-family <name>` for one family, and `--gate-strict` when CI/nightly should fail on `FAIL` or `INSUFFICIENT_EVIDENCE`.
- Campaign aggregate summaries plus the policy manifest are the inputs nightly automation should use to decide whether `exact_shadow` stays graduated.

Evidence bundle:
- `python tests/tools/build_evidence_bundle.py --phase phase18 --artifact-root artifacts --report-out PHASE18_STABILIZATION_REPORT_20260319.txt --policy-manifest artifacts/manifests/policy_gate.json --zip-out raw_engine_phase18_stabilization_20260319.zip` assembles the current report, policy manifest, selected logs, regressions, and curated artifacts into one bundle.
- Bundle layout is fixed as `reports/`, `manifests/`, `curated/`, `regressions/`, and `logs/`, plus a root `bundle_metadata.json` with phase metadata and gate summary.

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
