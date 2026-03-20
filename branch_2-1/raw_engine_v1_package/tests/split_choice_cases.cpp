#include "exhaustive_cases.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "exhaustive_generator.hpp"
#include "multiclass_catalog.hpp"
#include "split_choice_oracle.hpp"
#include "state_dump.hpp"

using namespace std;

namespace {

filesystem::path regression_fixture_path(const string& filename) {
    const filesystem::path local = filesystem::current_path() / "regressions" / filename;
    if (filesystem::exists(local)) {
        return local;
    }
    return filesystem::path(__FILE__).parent_path() / "regressions" / filename;
}

PlannerStateDump planner_dump_from_scenario(
    const ExhaustiveScenario& scenario,
    const TestOptions& options,
    const string& caseName
) {
    PlannerStateDump dump;
    dump.engine = scenario.RE;
    dump.caseName = caseName;
    dump.targetOcc = scenario.ctx.targetOcc;
    dump.keepOcc.assign(scenario.ctx.keepOcc.begin(), scenario.ctx.keepOcc.end());
    sort(dump.keepOcc.begin(), dump.keepOcc.end());
    dump.stepBudget = options.stepBudget;
    dump.tracePrefixLength = 0U;
    dump.initialQueue = scenario.initialQueue;
    dump.traceLevel = TraceLevel::FULL;
    return dump;
}

void append_regression_candidate(
    const TestOptions& options,
    const string& caseName,
    const filesystem::path& dumpPath,
    const optional<filesystem::path>& reducedPath
) {
    const filesystem::path outPath = artifact_subdir(options, "logs") / "regression_candidates.txt";
    ofstream ofs(outPath, ios::app);
    if (!ofs) {
        throw runtime_error("failed to append split-choice regression candidate: " + outPath.string());
    }
    ofs << "case=" << caseName << " kind=split_choice dump=" << dumpPath.string();
    if (reducedPath.has_value()) {
        ofs << " reduced=" << reducedPath->string();
    }
    ofs << '\n';
}

void write_trace_log(
    const TestOptions& options,
    const string& stem,
    const vector<PlannerTraceEntry>& trace
) {
    const filesystem::path path = artifact_subdir(options, "traces") / (stem + ".log");
    ofstream ofs(path);
    if (!ofs) {
        throw runtime_error("failed to write split-choice trace: " + path.string());
    }
    ofs << describe_planner_trace(trace);
}

void write_split_choice_summary(
    const TestOptions& options,
    const string& caseName,
    const SplitChoiceOracleRunResult& result
) {
    const filesystem::path outPath = artifact_subdir(options, "logs") / (caseName + ".summary.txt");
    ofstream ofs(outPath);
    if (!ofs) {
        throw runtime_error("failed to write split-choice summary: " + outPath.string());
    }
    ofs << describe_split_choice_oracle_result(result);
}

void persist_split_choice_failure(
    const TestOptions& options,
    const string& caseName,
    const ExhaustiveScenario& scenario,
    const SplitChoiceOracleRunResult& result
) {
    const filesystem::path dumpPath = artifact_subdir(options, "counterexamples") / (caseName + "_input.txt");
    const PlannerStateDump dump = planner_dump_from_scenario(scenario, options, caseName);
    save_planner_state_dump(dumpPath, dump);
    set_pending_dump_path(dumpPath);

    const auto [selected, mismatch] = [&]() {
        const SplitChoiceCandidateOutcome* selectedOutcome = nullptr;
        const SplitChoiceCandidateOutcome* mismatchOutcome = nullptr;
        for (const SplitChoiceCandidateOutcome& outcome : result.outcomes) {
            if (outcome.selectedByPlanner) {
                selectedOutcome = &outcome;
            } else if (outcome.grade != SplitChoiceGrade::FULLY_EQUIVALENT && mismatchOutcome == nullptr) {
                mismatchOutcome = &outcome;
            }
        }
        return pair<const SplitChoiceCandidateOutcome*, const SplitChoiceCandidateOutcome*>{
            selectedOutcome,
            mismatchOutcome,
        };
    }();

    optional<filesystem::path> reducedPath;
    const bool shouldReduce =
        options.reduce ||
        caseName.find("split_choice_audit") != string::npos ||
        caseName.find("semantic_shift_regression") != string::npos;
    if (shouldReduce) {
        try {
            reducedPath = reduce_planner_state_dump_file(options, dumpPath, false);
        } catch (const exception&) {
            reducedPath = nullopt;
        }
    }
    append_regression_candidate(options, caseName, dumpPath, reducedPath);

    const filesystem::path failurePath = artifact_subdir(options, "logs") / (caseName + ".failure.txt");
    ofstream ofs(failurePath);
    if (!ofs) {
        throw runtime_error("failed to write split-choice failure summary: " + failurePath.string());
    }
    ofs << describe_split_choice_oracle_result(result);

    if (selected != nullptr) {
        write_trace_log(options, caseName + "_selected", selected->trace);
    }
    if (mismatch != nullptr) {
        write_trace_log(options, caseName + "_mismatch", mismatch->trace);
    }
}

void record_split_choice_result(
    const TestOptions& options,
    const string& caseName,
    const ExhaustiveScenario& scenario,
    const SplitChoiceOracleRunResult& result
) {
    write_split_choice_summary(options, caseName, result);
    if (const optional<MulticlassCatalogEntry> entry =
            build_multiclass_catalog_entry(exhaustive_family_name(scenario.family), scenario, result);
        entry.has_value()) {
        write_multiclass_catalog_entry(options, caseName, scenario, *entry, result);
    }
    const bool shouldPersist =
        split_choice_has_exact_audit_disagreement(result) ||
        result.semanticDisagreementCount != 0U ||
        (options.splitChoicePolicyMode == SplitChoicePolicyMode::FAST && split_choice_has_mismatch(result));
    if (shouldPersist) {
        persist_split_choice_failure(options, caseName, scenario, result);
    }
}

string semantic_projection(const SplitChoiceOracleRunResult& result) {
    ostringstream oss;
    oss << "selected=" << result.selectedPair.first << ',' << result.selectedPair.second << '\n';
    oss << "fast_selected=" << result.fastSelectedPair.first << ',' << result.fastSelectedPair.second << '\n';
    oss << "exact_shadow_selected=" << result.exactShadowSelectedPair.first << ',' << result.exactShadowSelectedPair.second << '\n';
    oss << "exact_full_selected=" << result.exactFullSelectedPair.first << ',' << result.exactFullSelectedPair.second << '\n';
    oss << "pairs=" << result.admissiblePairCount << '/' << result.comparedPairCount << '\n';
    for (const SplitChoiceCandidateOutcome& outcome : result.outcomes) {
        oss << outcome.pair.first << ',' << outcome.pair.second
            << " sel=" << (outcome.selectedByPlanner ? 1 : 0)
            << " fast=" << (outcome.selectedByFast ? 1 : 0)
            << " stop=" << (outcome.stopConditionSatisfied ? 1 : 0)
            << " final=" << outcome.finalStateHash
            << " target=" << outcome.targetPrepareHash
            << '\n';
    }
    return oss.str();
}

string selected_policy_projection(const SplitChoiceOracleRunResult& result) {
    for (const SplitChoiceCandidateOutcome& outcome : result.outcomes) {
        if (!outcome.selectedByPolicy) {
            continue;
        }
        ostringstream oss;
        oss << "stop=" << (outcome.stopConditionSatisfied ? 1 : 0)
            << "\nfinal=" << outcome.finalStateHash
            << "\ntarget=" << outcome.targetPrepareHash
            << "\ntrace=" << outcome.traceHash
            << "\ndetail=" << stable_hash_text(outcome.detail);
        return oss.str();
    }
    throw runtime_error("split-choice policy projection missing selected outcome");
}

string exact_class_projection(const SplitChoiceOracleRunResult& result) {
    ostringstream oss;
    oss << "fast_class_count=" << result.fastClassCount
        << "\nexact_class_count=" << result.exactClassCount
        << "\nexact_disagreement=" << result.exactVsFastClassDisagreementCount
        << "\nfast_selected=" << result.fastSelectedPair.first << ',' << result.fastSelectedPair.second
        << "\npolicy_selected=" << result.policySelectedPair.first << ',' << result.policySelectedPair.second
        << "\nexact_shadow_selected=" << result.exactShadowSelectedPair.first << ',' << result.exactShadowSelectedPair.second
        << "\nexact_full_selected=" << result.exactFullSelectedPair.first << ',' << result.exactFullSelectedPair.second
        << "\nexact_representative=" << result.exactRepresentativePair.first << ',' << result.exactRepresentativePair.second
        << "\nrepresentative_shift_count=" << result.representativeShiftCount
        << "\nrepresentative_shift_same_class_count=" << result.representativeShiftSameClassCount
        << "\nrepresentative_shift_semantic_divergence_count=" << result.representativeShiftSemanticDivergenceCount
        << "\nrepresentative_shift_followup_divergence_count=" << result.representativeShiftFollowupDivergenceCount
        << "\nrepresentative_shift_trace_divergence_count=" << result.representativeShiftTraceDivergenceCount
        << "\nharmless_shift_count=" << result.harmlessShiftCount
        << "\ntrace_only_shift_count=" << result.traceOnlyShiftCount
        << "\nsemantic_shift_count=" << result.semanticShiftCount
        << "\nrepresentative_shift_kind=" << split_choice_representative_shift_name(result.representativeShiftKind)
        << "\nrepresentative_shifted=" << (result.representativeShifted ? 1 : 0)
        << "\nexact_representative_selected=" << (result.exactRepresentativeSelected ? 1 : 0)
        << "\nsplit_choice_compare_state_count=" << result.compareStateCount
        << "\nsplit_choice_exact_shadow_eval_count=" << result.exactShadowEvalCount
        << "\nsplit_choice_exact_full_eval_count=" << result.exactFullEvalCount
        << "\nsplit_choice_fallback_count=" << result.fallbackCount
        << "\nsplit_choice_same_representative_count=" << result.sameRepresentativeCount
        << "\nsplit_choice_same_semantic_class_count=" << result.sameSemanticClassCount
        << "\nsplit_choice_same_final_state_count=" << result.sameFinalStateCount
        << "\nsplit_choice_semantic_disagreement_count=" << result.semanticDisagreementCount
        << "\nsplit_choice_cap_hit_count=" << result.capHitCount
        << '\n';
    for (const SplitChoiceCandidateOutcome& outcome : result.outcomes) {
        oss << outcome.pair.first << ',' << outcome.pair.second
            << " sel=" << (outcome.selectedByPlanner ? 1 : 0)
            << " fast=" << (outcome.selectedByFast ? 1 : 0)
            << " policy=" << (outcome.selectedByPolicy ? 1 : 0)
            << " exact_shadow=" << (outcome.selectedByExactShadow ? 1 : 0)
            << " exact=" << stable_hash_text(outcome.exactSplitChoiceKey)
            << " exact_full=" << (outcome.selectedByExactFull ? 1 : 0)
            << '\n';
    }
    return oss.str();
}

void require_same_result(
    const string& caseName,
    const SplitChoiceOracleRunResult& expected,
    const SplitChoiceOracleRunResult& actual
) {
    const string expectedText = semantic_projection(expected);
    const string actualText = semantic_projection(actual);
    if (expectedText != actualText) {
        throw runtime_error(
            caseName + " split-choice invariance mismatch\nexpected=\n" + expectedText +
            "\nactual=\n" + actualText
        );
    }
}

void require_same_policy_selection(
    const string& caseName,
    const SplitChoiceOracleRunResult& expected,
    const SplitChoiceOracleRunResult& actual
) {
    const string expectedText = selected_policy_projection(expected);
    const string actualText = selected_policy_projection(actual);
    if (expectedText != actualText) {
        throw runtime_error(
            caseName + " split-choice policy selection mismatch\nexpected=\n" + expectedText +
            "\nactual=\n" + actualText
        );
    }
}

void require_policy_stats(
    const string& caseName,
    const SplitChoicePolicyStats& stats,
    bool expectMulticlass
) {
    if (stats.candidateCount < 2U || stats.tieCount == 0U) {
        throw runtime_error(caseName + " expected split-choice tie stats");
    }
    if (stats.evalCount == 0U) {
        throw runtime_error(caseName + " expected split-choice evaluation stats");
    }
    if (stats.fallbackCount != 0U) {
        throw runtime_error(caseName + " expected semantic policy without fallback");
    }
    if (expectMulticlass && stats.multiclassCount == 0U) {
        throw runtime_error(caseName + " expected split-choice multiclass stats");
    }
}

void require_exact_audit(
    const string& caseName,
    const SplitChoiceOracleRunResult& result,
    bool expectMulticlass
) {
    if (!result.exactAuditAvailable) {
        throw runtime_error(caseName + " expected exact split-choice audit");
    }
    if (expectMulticlass && result.exactClassCount < 2U) {
        throw runtime_error(caseName + " expected multiple exact split-choice classes");
    }
    if (result.fastClassCount != result.exactClassCount) {
        throw runtime_error(caseName + " fast/exact class count mismatch");
    }
    if (result.exactVsFastClassDisagreementCount != 0U) {
        throw runtime_error(caseName + " fast/exact class partition mismatch");
    }
}

ExhaustiveScenario scenario_from_dump(const PlannerStateDump& dump) {
    ExhaustiveScenario scenario;
    scenario.RE = dump.engine;
    scenario.ctx.targetOcc = dump.targetOcc;
    scenario.ctx.keepOcc.insert(dump.keepOcc.begin(), dump.keepOcc.end());
    scenario.initialQueue = dump.initialQueue;
    scenario.label = dump.caseName;
    return scenario;
}

SplitChoicePolicyStats capture_policy_stats_for_scenario(
    const TestOptions& options,
    const ExhaustiveScenario& scenario
) {
    SplitChoicePolicyStats stats;
    const optional<pair<Vertex, Vertex>> selectedPair = discover_split_pair_from_support(
        scenario.RE,
        scenario.ctx.targetOcc,
        &scenario.ctx,
        planner_run_options(options),
        &stats
    );
    if (!selectedPair.has_value()) {
        throw runtime_error("split-choice policy stats expected a selected pair");
    }
    return stats;
}

void run_transform_case(
    const TestOptions& options,
    const string& caseName,
    const ExhaustiveScenario& baseScenario,
    const RebuildTransform& transform
) {
    TestOptions oracleOptions = options;
    oracleOptions.caseName = caseName;
    oracleOptions.oracleMode = OracleMode::ALL;
    oracleOptions.dumpOnFail = true;
    set_active_test_options(&oracleOptions);
    set_pending_dump_path(nullopt);

    const SplitChoiceOracleRunResult baseline = run_split_choice_oracle(oracleOptions, baseScenario, nullptr);
    record_split_choice_result(oracleOptions, caseName + "_baseline", baseScenario, baseline);

    const unordered_map<Vertex, Vertex> inverseRelabel = invert_relabel_map(transform.relabelOrig);
    const ExhaustiveScenario rebuilt = rebuild_exhaustive_scenario(baseScenario, transform);
    const SplitChoiceOracleRunResult transformed =
        run_split_choice_oracle(oracleOptions, rebuilt, inverseRelabel.empty() ? nullptr : &inverseRelabel);
    record_split_choice_result(oracleOptions, caseName, rebuilt, transformed);
    require_same_result(caseName, baseline, transformed);
    set_active_test_options(&options);
}

void run_policy_transform_case(
    const TestOptions& options,
    const string& caseName,
    const ExhaustiveScenario& baseScenario,
    const RebuildTransform& transform
) {
    TestOptions oracleOptions = options;
    oracleOptions.caseName = caseName;
    oracleOptions.oracleMode = OracleMode::ALL;
    oracleOptions.dumpOnFail = true;
    set_active_test_options(&oracleOptions);
    set_pending_dump_path(nullopt);

    const SplitChoiceOracleRunResult baseline = run_split_choice_oracle(oracleOptions, baseScenario, nullptr);
    const SplitChoicePolicyStats baselineStats = capture_policy_stats_for_scenario(oracleOptions, baseScenario);

    const unordered_map<Vertex, Vertex> inverseRelabel = invert_relabel_map(transform.relabelOrig);
    const ExhaustiveScenario rebuilt = rebuild_exhaustive_scenario(baseScenario, transform);
    const SplitChoiceOracleRunResult transformed =
        run_split_choice_oracle(oracleOptions, rebuilt, inverseRelabel.empty() ? nullptr : &inverseRelabel);
    const SplitChoicePolicyStats transformedStats = capture_policy_stats_for_scenario(oracleOptions, rebuilt);

    require_same_policy_selection(caseName, baseline, transformed);
    require_policy_stats(caseName + "_baseline", baselineStats, false);
    require_policy_stats(caseName + "_transformed", transformedStats, false);
    set_active_test_options(&options);
}

void run_exact_transform_case(
    const TestOptions& options,
    const string& caseName,
    const ExhaustiveScenario& baseScenario,
    const RebuildTransform& transform
) {
    TestOptions oracleOptions = options;
    oracleOptions.caseName = caseName;
    oracleOptions.oracleMode = OracleMode::ALL;
    oracleOptions.dumpOnFail = true;
    oracleOptions.exactCanonicalCap = max<size_t>(oracleOptions.exactCanonicalCap, 8U);
    set_active_test_options(&oracleOptions);
    set_pending_dump_path(nullopt);

    const SplitChoiceOracleRunResult baseline = run_split_choice_oracle(oracleOptions, baseScenario, nullptr);
    const unordered_map<Vertex, Vertex> inverseRelabel = invert_relabel_map(transform.relabelOrig);
    const ExhaustiveScenario rebuilt = rebuild_exhaustive_scenario(baseScenario, transform);
    const SplitChoiceOracleRunResult transformed =
        run_split_choice_oracle(oracleOptions, rebuilt, inverseRelabel.empty() ? nullptr : &inverseRelabel);

    require_exact_audit(caseName + "_baseline", baseline, false);
    require_exact_audit(caseName + "_transformed", transformed, false);
    if (exact_class_projection(baseline) != exact_class_projection(transformed)) {
        throw runtime_error(
            caseName + " exact split-choice invariance mismatch\nexpected=\n" +
            exact_class_projection(baseline) + "\nactual=\n" + exact_class_projection(transformed)
        );
    }
    set_active_test_options(&options);
}

} // namespace

void run_split_choice_oracle_smoke_case(const TestOptions& options) {
    TestOptions oracleOptions = options;
    oracleOptions.caseName = "split_choice_oracle";
    oracleOptions.oracleMode = OracleMode::ALL;
    oracleOptions.dumpOnFail = true;
    set_active_test_options(&oracleOptions);
    set_pending_dump_path(nullopt);

    const vector<pair<string, ExhaustiveScenario>> scenarios = {
        {"split_tie_ready", make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_READY, 880101U)},
        {"split_tie_structural", make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_STRUCTURAL, 880301U)},
    };

    for (const auto& [familyName, scenario] : scenarios) {
        const SplitChoiceOracleRunResult result = run_split_choice_oracle(oracleOptions, scenario, nullptr);
        record_split_choice_result(oracleOptions, "split_choice_oracle_" + familyName, scenario, result);
    }
    set_active_test_options(&options);
}

void run_split_choice_relabel_invariance_case(const TestOptions& options) {
    const ExhaustiveScenario scenario =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_STRUCTURAL, 880301U);
    run_transform_case(
        options,
        "split_choice_relabel_invariance",
        scenario,
        make_relabel_transform(scenario)
    );
}

void run_split_choice_edge_order_invariance_case(const TestOptions& options) {
    const ExhaustiveScenario scenario =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_STRUCTURAL, 880303U);
    run_transform_case(
        options,
        "split_choice_edge_order_invariance",
        scenario,
        make_edge_order_transform()
    );
}

void run_split_choice_vertex_order_invariance_case(const TestOptions& options) {
    const ExhaustiveScenario scenario =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_STRUCTURAL, 880305U);
    run_transform_case(
        options,
        "split_choice_vertex_order_invariance",
        scenario,
        make_vertex_order_transform()
    );
}

void run_split_choice_oracle_regression_case(const TestOptions& options) {
    TestOptions oracleOptions = options;
    oracleOptions.caseName = "split_choice_oracle_regression";
    set_active_test_options(&oracleOptions);
    set_pending_dump_path(nullopt);

    const filesystem::path regressionPath = regression_fixture_path("split_choice_oracle_reduced_state.txt");
    const PlannerStateDump dump = load_planner_state_dump(regressionPath);
    FailureSignature failure;
    if (replay_planner_state_dump(oracleOptions, dump, false, &failure)) {
        throw runtime_error("split_choice_oracle_regression expected replay mismatch");
    }
    if (failure.failureClass != FailureClass::PLANNER_ORACLE_MISMATCH ||
        failure.mismatchKind == FailureMismatchKind::NONE) {
        throw runtime_error("split_choice_oracle_regression produced unexpected failure classification");
    }
    set_active_test_options(&options);
}

void run_split_choice_policy_smoke_case(const TestOptions& options) {
    TestOptions oracleOptions = options;
    oracleOptions.caseName = "split_choice_policy_smoke";
    oracleOptions.oracleMode = OracleMode::ALL;
    set_active_test_options(&oracleOptions);

    const filesystem::path regressionPath = regression_fixture_path("split_choice_oracle_reduced_state.txt");
    const PlannerStateDump dump = load_planner_state_dump(regressionPath);
    const ExhaustiveScenario scenario = scenario_from_dump(dump);

    const SplitChoiceOracleRunResult first = run_split_choice_oracle(oracleOptions, scenario, nullptr);
    const SplitChoicePolicyStats firstStats = capture_policy_stats_for_scenario(oracleOptions, scenario);
    const SplitChoiceOracleRunResult second = run_split_choice_oracle(oracleOptions, scenario, nullptr);
    const SplitChoicePolicyStats secondStats = capture_policy_stats_for_scenario(oracleOptions, scenario);

    if (!split_choice_has_mismatch(first)) {
        throw runtime_error("split_choice_policy_smoke expected multiclass split-choice regression");
    }
    require_same_policy_selection("split_choice_policy_smoke", first, second);
    require_policy_stats("split_choice_policy_smoke_first", firstStats, true);
    require_policy_stats("split_choice_policy_smoke_second", secondStats, true);
    set_active_test_options(&options);
}

void run_split_choice_policy_relabel_invariance_case(const TestOptions& options) {
    const ExhaustiveScenario scenario =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_STRUCTURAL, 880301U);
    run_policy_transform_case(
        options,
        "split_choice_policy_relabel_invariance",
        scenario,
        make_relabel_transform(scenario)
    );
}

void run_split_choice_policy_edge_order_invariance_case(const TestOptions& options) {
    const ExhaustiveScenario scenario =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_STRUCTURAL, 880303U);
    run_policy_transform_case(
        options,
        "split_choice_policy_edge_order_invariance",
        scenario,
        make_edge_order_transform()
    );
}

void run_split_choice_policy_vertex_order_invariance_case(const TestOptions& options) {
    const ExhaustiveScenario scenario =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_STRUCTURAL, 880305U);
    run_policy_transform_case(
        options,
        "split_choice_policy_vertex_order_invariance",
        scenario,
        make_vertex_order_transform()
    );
}

void run_split_choice_policy_occid_invariance_case(const TestOptions& options) {
    const ExhaustiveScenario scenario =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_STRUCTURAL, 880307U);
    run_policy_transform_case(
        options,
        "split_choice_policy_occid_invariance",
        scenario,
        make_occid_renumber_transform()
    );
}

void run_split_choice_policy_multiclass_smoke_case(const TestOptions& options) {
    TestOptions oracleOptions = options;
    oracleOptions.caseName = "split_choice_policy_multiclass_smoke";
    oracleOptions.oracleMode = OracleMode::ALL;
    set_active_test_options(&oracleOptions);

    const ExhaustiveScenario scenario =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_READY, 880101U);
    const SplitChoiceOracleRunResult first = run_split_choice_oracle(oracleOptions, scenario, nullptr);
    const SplitChoicePolicyStats firstStats = capture_policy_stats_for_scenario(oracleOptions, scenario);
    const SplitChoiceOracleRunResult second = run_split_choice_oracle(oracleOptions, scenario, nullptr);
    const SplitChoicePolicyStats secondStats = capture_policy_stats_for_scenario(oracleOptions, scenario);

    if (!split_choice_has_mismatch(first)) {
        throw runtime_error("split_choice_policy_multiclass_smoke expected multi-class split-choice state");
    }
    require_same_policy_selection("split_choice_policy_multiclass_smoke", first, second);
    require_policy_stats("split_choice_policy_multiclass_smoke_first", firstStats, true);
    require_policy_stats("split_choice_policy_multiclass_smoke_second", secondStats, true);
    set_active_test_options(&options);
}

void run_split_choice_exact_class_smoke_case(const TestOptions& options) {
    TestOptions oracleOptions = options;
    oracleOptions.caseName = "split_choice_exact_class_smoke";
    oracleOptions.oracleMode = OracleMode::ALL;
    oracleOptions.dumpOnFail = true;
    oracleOptions.exactCanonicalCap = max<size_t>(oracleOptions.exactCanonicalCap, 8U);
    oracleOptions.splitChoiceCompareMode = SplitChoiceCompareMode::EXACT_FULL;
    set_active_test_options(&oracleOptions);
    set_pending_dump_path(nullopt);

    const ExhaustiveScenario scenario =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_STRUCTURAL, 880301U);
    const SplitChoiceOracleRunResult result = run_split_choice_oracle(oracleOptions, scenario, nullptr);
    record_split_choice_result(oracleOptions, "split_choice_exact_class_smoke", scenario, result);
    require_exact_audit("split_choice_exact_class_smoke", result, true);
    if (result.compareStateCount != 1U ||
        result.exactFullEvalCount == 0U ||
        result.exactShadowEvalCount == 0U ||
        result.fallbackCount != 0U) {
        throw runtime_error("split_choice_exact_class_smoke expected exact_full compare coverage");
    }
    set_active_test_options(&options);
}

void run_split_choice_exact_relabel_invariance_case(const TestOptions& options) {
    const ExhaustiveScenario scenario =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_STRUCTURAL, 880301U);
    run_exact_transform_case(
        options,
        "split_choice_exact_relabel_invariance",
        scenario,
        make_relabel_transform(scenario)
    );
}

void run_split_choice_exact_vertex_order_invariance_case(const TestOptions& options) {
    const ExhaustiveScenario scenario =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_STRUCTURAL, 880305U);
    run_exact_transform_case(
        options,
        "split_choice_exact_vertex_order_invariance",
        scenario,
        make_vertex_order_transform()
    );
}

void run_split_choice_exact_edge_order_invariance_case(const TestOptions& options) {
    const ExhaustiveScenario scenario =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_STRUCTURAL, 880303U);
    run_exact_transform_case(
        options,
        "split_choice_exact_edge_order_invariance",
        scenario,
        make_edge_order_transform()
    );
}

namespace {

SplitChoiceOracleRunResult run_shift_audit(
    const TestOptions& options,
    const string& caseName,
    const ExhaustiveScenario& scenario
) {
    TestOptions oracleOptions = options;
    oracleOptions.caseName = caseName;
    oracleOptions.oracleMode = OracleMode::ALL;
    oracleOptions.dumpOnFail = true;
    oracleOptions.exactCanonicalCap = max<size_t>(oracleOptions.exactCanonicalCap, 8U);
    set_active_test_options(&oracleOptions);
    set_pending_dump_path(nullopt);
    const SplitChoiceOracleRunResult result = run_split_choice_oracle(oracleOptions, scenario, nullptr);
    record_split_choice_result(oracleOptions, caseName, scenario, result);
    set_active_test_options(&options);
    return result;
}

TestOptions with_split_choice_policy(const TestOptions& options, SplitChoicePolicyMode mode) {
    TestOptions out = options;
    out.splitChoicePolicyMode = mode;
    return out;
}

} // namespace

void run_split_choice_representative_shift_smoke_case(const TestOptions& options) {
    const TestOptions fastOptions = with_split_choice_policy(options, SplitChoicePolicyMode::FAST);
    const ExhaustiveScenario scenario =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_SYMMETRIC_LARGE, 890101U);
    const SplitChoiceOracleRunResult result =
        run_shift_audit(fastOptions, "split_choice_representative_shift_smoke", scenario);
    require_exact_audit("split_choice_representative_shift_smoke", result, true);
    if (result.representativeShiftCount == 0U) {
        throw runtime_error("split_choice_representative_shift_smoke expected representative shift");
    }
}

void run_split_choice_harmless_shift_smoke_case(const TestOptions& options) {
    const TestOptions fastOptions = with_split_choice_policy(options, SplitChoicePolicyMode::FAST);
    const vector<pair<ScenarioFamily, u32>> candidates = {
        {ScenarioFamily::SPLIT_TIE_SYMMETRIC_LARGE, 890101U},
        {ScenarioFamily::SPLIT_TIE_SYMMETRIC_LARGE, 890103U},
        {ScenarioFamily::SPLIT_TIE_ORGANIC_SYMMETRIC, 910101U},
        {ScenarioFamily::SPLIT_TIE_ORGANIC_SYMMETRIC, 910103U},
    };

    size_t exactAuditCount = 0U;
    for (const auto& [family, seed] : candidates) {
        const ExhaustiveScenario scenario = make_targeted_planner_exhaustive_scenario(family, seed);
        const SplitChoiceOracleRunResult result = run_shift_audit(
            fastOptions,
            "split_choice_harmless_shift_smoke_" + scenario_family_name_string(family) + "_seed" + to_string(seed),
            scenario
        );
        exactAuditCount += result.exactAuditAvailable ? 1U : 0U;
        if (result.representativeShiftKind == SplitChoiceRepresentativeShiftKind::HARMLESS) {
            const TestOptions shadowOptions =
                with_split_choice_policy(options, SplitChoicePolicyMode::EXACT_SHADOW);
            const SplitChoiceOracleRunResult shadowResult = run_shift_audit(
                shadowOptions,
                "split_choice_harmless_shift_smoke_shadow_" + scenario_family_name_string(family) + "_seed" +
                    to_string(seed),
                scenario
            );
            if (semantic_projection(result) != semantic_projection(shadowResult)) {
                throw runtime_error("split_choice_harmless_shift_smoke exact-shadow semantic drift");
            }
            return;
        }
    }
    if (exactAuditCount == 0U) {
        throw runtime_error("split_choice_harmless_shift_smoke expected exact split-choice audit coverage");
    }
}

void run_split_choice_semantic_shift_smoke_case(const TestOptions& options) {
    const TestOptions exactShadowOptions =
        with_split_choice_policy(options, SplitChoicePolicyMode::EXACT_SHADOW);
    const vector<pair<ScenarioFamily, u32>> candidates = {
        {ScenarioFamily::SPLIT_TIE_SYMMETRIC_LARGE, 890101U},
        {ScenarioFamily::SPLIT_TIE_ORGANIC_SYMMETRIC, 910101U},
        {ScenarioFamily::AUTOMORPHISM_PROBE_LARGE, 910301U},
    };

    size_t auditedCount = 0U;
    for (const auto& [family, seed] : candidates) {
        const ExhaustiveScenario scenario = make_targeted_planner_exhaustive_scenario(family, seed);
        const SplitChoiceOracleRunResult result = run_shift_audit(
            exactShadowOptions,
            "split_choice_semantic_shift_smoke_" + scenario_family_name_string(family) + "_seed" + to_string(seed),
            scenario
        );
        require_exact_audit("split_choice_semantic_shift_smoke", result, true);
        ++auditedCount;
        if (result.representativeShiftCount != 0U ||
            result.semanticShiftCount != 0U ||
            result.representativeShiftSemanticDivergenceCount != 0U) {
            throw runtime_error(
                "split_choice_semantic_shift_smoke expected exact-shadow semantic_shift_count=0\n" +
                describe_split_choice_oracle_result(result)
            );
        }
    }
    if (auditedCount == 0U) {
        throw runtime_error("split_choice_semantic_shift_smoke expected audited tie scenarios");
    }
}

void run_split_choice_semantic_shift_regression_case(const TestOptions& options) {
    TestOptions oracleOptions = options;
    oracleOptions.caseName = "split_choice_semantic_shift_regression";
    oracleOptions.oracleMode = OracleMode::ALL;
    oracleOptions.exactCanonicalCap = max<size_t>(oracleOptions.exactCanonicalCap, 8U);
    set_active_test_options(&oracleOptions);
    set_pending_dump_path(nullopt);

    const filesystem::path regressionPath = regression_fixture_path("split_choice_semantic_shift_state.txt");
    const PlannerStateDump dump = load_planner_state_dump(regressionPath);
    const ExhaustiveScenario scenario = scenario_from_dump(dump);
    const SplitChoiceOracleRunResult result = run_split_choice_oracle(oracleOptions, scenario, nullptr);
    write_split_choice_summary(oracleOptions, "split_choice_semantic_shift_regression", result);

    require_exact_audit("split_choice_semantic_shift_regression", result, true);
    if (options.splitChoicePolicyMode == SplitChoicePolicyMode::FAST) {
        if (result.semanticShiftCount == 0U ||
            result.representativeShiftKind != SplitChoiceRepresentativeShiftKind::SEMANTIC) {
            throw runtime_error("split_choice_semantic_shift_regression expected fast-mode semantic shift");
        }
        if (result.selectedPair != pair<Vertex, Vertex>{2U, 6U} ||
            result.exactShadowSelectedPair != pair<Vertex, Vertex>{2U, 8U}) {
            throw runtime_error("split_choice_semantic_shift_regression expected fast-vs-exact representative mismatch");
        }
    } else {
        if (result.semanticShiftCount != 0U ||
            result.representativeShiftCount != 0U ||
            result.fallbackCount != 0U ||
            result.semanticDisagreementCount != 0U ||
            (options.splitChoicePolicyMode == SplitChoicePolicyMode::EXACT_FULL
                 ? result.selectedPair != result.exactFullSelectedPair
                 : result.selectedPair != result.exactShadowSelectedPair)) {
            throw runtime_error(
                "split_choice_semantic_shift_regression expected stabilized semantic_shift=0\n" +
                describe_split_choice_oracle_result(result)
            );
        }
    }
    set_active_test_options(&options);
}

void run_split_choice_policy_exact_shadow_smoke_case(const TestOptions& options) {
    const TestOptions exactShadowOptions =
        with_split_choice_policy(options, SplitChoicePolicyMode::EXACT_SHADOW);
    TestOptions compareOptions = exactShadowOptions;
    compareOptions.splitChoiceCompareMode = SplitChoiceCompareMode::EXACT_FULL;
    const filesystem::path regressionPath = regression_fixture_path("split_choice_semantic_shift_state.txt");
    const PlannerStateDump dump = load_planner_state_dump(regressionPath);
    const ExhaustiveScenario scenario = scenario_from_dump(dump);
    const SplitChoiceOracleRunResult result =
        run_shift_audit(compareOptions, "split_choice_policy_exact_shadow_smoke", scenario);
    const SplitChoicePolicyStats stats = capture_policy_stats_for_scenario(compareOptions, scenario);
    require_exact_audit("split_choice_policy_exact_shadow_smoke", result, true);
    require_policy_stats("split_choice_policy_exact_shadow_smoke", stats, true);
    if (result.representativeShiftCount != 0U ||
        result.semanticShiftCount != 0U ||
        result.fallbackCount != 0U ||
        result.policySelectedPair != result.exactShadowSelectedPair) {
        throw runtime_error(
            "split_choice_policy_exact_shadow_smoke expected exact-shadow stable representative\n" +
            describe_split_choice_oracle_result(result)
        );
    }
}

void run_split_choice_policy_fast_vs_exact_shadow_compare_case(const TestOptions& options) {
    const filesystem::path regressionPath = regression_fixture_path("split_choice_semantic_shift_state.txt");
    const PlannerStateDump dump = load_planner_state_dump(regressionPath);
    const ExhaustiveScenario scenario = scenario_from_dump(dump);

    const TestOptions fastOptions = with_split_choice_policy(options, SplitChoicePolicyMode::FAST);
    TestOptions fastCompareOptions = fastOptions;
    fastCompareOptions.splitChoiceCompareMode = SplitChoiceCompareMode::EXACT_FULL;
    const SplitChoiceOracleRunResult fastResult =
        run_shift_audit(fastCompareOptions, "split_choice_policy_fast_vs_exact_shadow_compare_fast", scenario);
    const TestOptions exactShadowOptions =
        with_split_choice_policy(options, SplitChoicePolicyMode::EXACT_SHADOW);
    TestOptions exactShadowCompareOptions = exactShadowOptions;
    exactShadowCompareOptions.splitChoiceCompareMode = SplitChoiceCompareMode::EXACT_FULL;
    const SplitChoiceOracleRunResult exactShadowResult =
        run_shift_audit(exactShadowCompareOptions, "split_choice_policy_fast_vs_exact_shadow_compare_exact", scenario);

    require_exact_audit("split_choice_policy_fast_vs_exact_shadow_compare_fast", fastResult, true);
    require_exact_audit("split_choice_policy_fast_vs_exact_shadow_compare_exact", exactShadowResult, true);
    if (fastResult.semanticShiftCount == 0U ||
        fastResult.policySelectedPair == exactShadowResult.policySelectedPair) {
        throw runtime_error("split_choice_policy_fast_vs_exact_shadow_compare expected fast mismatch reproduction");
    }
    if (exactShadowResult.semanticShiftCount != 0U ||
        exactShadowResult.representativeShiftCount != 0U ||
        exactShadowResult.fallbackCount != 0U ||
        exactShadowResult.semanticDisagreementCount != 0U ||
        exactShadowResult.policySelectedPair != exactShadowResult.exactShadowSelectedPair) {
        throw runtime_error(
            "split_choice_policy_fast_vs_exact_shadow_compare expected exact-shadow stabilization\n" +
            describe_split_choice_oracle_result(exactShadowResult)
        );
    }
}

void run_split_choice_exact_shadow_relabel_invariance_case(const TestOptions& options) {
    const ExhaustiveScenario scenario =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_STRUCTURAL, 880301U);
    run_policy_transform_case(
        with_split_choice_policy(options, SplitChoicePolicyMode::EXACT_SHADOW),
        "split_choice_exact_shadow_relabel_invariance",
        scenario,
        make_relabel_transform(scenario)
    );
}

void run_split_choice_exact_shadow_vertex_order_invariance_case(const TestOptions& options) {
    const ExhaustiveScenario scenario =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_STRUCTURAL, 880305U);
    run_policy_transform_case(
        with_split_choice_policy(options, SplitChoicePolicyMode::EXACT_SHADOW),
        "split_choice_exact_shadow_vertex_order_invariance",
        scenario,
        make_vertex_order_transform()
    );
}

void run_split_choice_exact_shadow_edge_order_invariance_case(const TestOptions& options) {
    const ExhaustiveScenario scenario =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_STRUCTURAL, 880303U);
    run_policy_transform_case(
        with_split_choice_policy(options, SplitChoicePolicyMode::EXACT_SHADOW),
        "split_choice_exact_shadow_edge_order_invariance",
        scenario,
        make_edge_order_transform()
    );
}

void run_split_choice_fallback_zero_smoke_case(const TestOptions& options) {
    const TestOptions exactShadowOptions =
        with_split_choice_policy(options, SplitChoicePolicyMode::EXACT_SHADOW);
    const vector<pair<ScenarioFamily, u32>> scenarios = {
        {ScenarioFamily::SPLIT_TIE_READY, 880101U},
        {ScenarioFamily::SPLIT_TIE_STRUCTURAL, 880301U},
        {ScenarioFamily::SPLIT_TIE_ORGANIC_SYMMETRIC, 910101U},
    };
    for (const auto& [family, seed] : scenarios) {
        const ExhaustiveScenario scenario = make_targeted_planner_exhaustive_scenario(family, seed);
        const SplitChoicePolicyStats stats = capture_policy_stats_for_scenario(exactShadowOptions, scenario);
        require_policy_stats(
            "split_choice_fallback_zero_smoke_" + scenario_family_name_string(family),
            stats,
            true
        );
    }
}
