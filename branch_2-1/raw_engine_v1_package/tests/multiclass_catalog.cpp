#include "multiclass_catalog.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include "state_dump.hpp"

using namespace std;

namespace {

MulticlassCatalogStats g_multiclassCatalogStats;
unordered_set<string> g_seenClusters;

const SplitChoiceCandidateOutcome* find_outcome(
    const SplitChoiceOracleRunResult& result,
    pair<Vertex, Vertex> pairValue
) {
    for (const SplitChoiceCandidateOutcome& outcome : result.outcomes) {
        if (outcome.pair == pairValue) {
            return &outcome;
        }
    }
    return nullptr;
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

} // namespace

const char* multiclass_catalog_category_name(MulticlassCatalogCategory category) {
    switch (category) {
        case MulticlassCatalogCategory::NONE:
            return "none";
        case MulticlassCatalogCategory::HARMLESS:
            return "harmless_multiclass";
        case MulticlassCatalogCategory::TRACE_ONLY:
            return "trace_only_multiclass";
        case MulticlassCatalogCategory::SEMANTIC_SHIFT:
            return "semantic_shift_multiclass";
    }
    return "unknown";
}

void reset_multiclass_catalog() {
    g_multiclassCatalogStats = MulticlassCatalogStats{};
    g_seenClusters.clear();
}

const MulticlassCatalogStats& multiclass_catalog_stats() {
    return g_multiclassCatalogStats;
}

MulticlassCatalogObservation observe_multiclass_catalog(const SplitChoiceOracleRunResult& result) {
    MulticlassCatalogObservation observation;
    if (result.exactClassCount < 2U || result.multiclassCatalogKey.empty()) {
        return observation;
    }

    observation.clusterKey = stable_hash_text(result.multiclassCatalogKey);
    observation.admissiblePairCount = result.admissiblePairCount;
    observation.semanticClassCount = result.exactClassCount;
    observation.representativeShifted = result.exactShadowVsExactFullShifted;
    observation.fastSelectedPair = result.fastSelectedPair;
    observation.exactShadowSelectedPair = result.exactShadowSelectedPair;
    observation.exactFullSelectedPair = result.exactFullSelectedPair;

    const SplitChoiceCandidateOutcome* exactShadow = find_outcome(result, result.exactShadowSelectedPair);
    const SplitChoiceCandidateOutcome* exactFull = find_outcome(result, result.exactFullSelectedPair);
    if (exactShadow != nullptr && exactFull != nullptr) {
        observation.stopConditionDivergence =
            exactShadow->stopConditionSatisfied != exactFull->stopConditionSatisfied;
        observation.finalStateDivergence =
            !split_choice_same_exact_projection(*exactShadow, *exactFull);
    }

    if (result.semanticDisagreementCount != 0U) {
        observation.category = MulticlassCatalogCategory::SEMANTIC_SHIFT;
    } else if (result.traceOnlyCompareCount != 0U) {
        observation.category = MulticlassCatalogCategory::TRACE_ONLY;
    } else {
        observation.category = MulticlassCatalogCategory::HARMLESS;
    }
    return observation;
}

void note_multiclass_catalog_observation(
    const TestOptions& options,
    const string& caseName,
    ScenarioFamily family,
    const ExhaustiveScenario& scenario,
    const SplitChoiceOracleRunResult& result
) {
    const MulticlassCatalogObservation observation = observe_multiclass_catalog(result);
    if (observation.clusterKey.empty()) {
        return;
    }

    const string familyName = scenario_family_name_string(family);
    const string dedupeKey =
        familyName + ":" + multiclass_catalog_category_name(observation.category) + ":" + observation.clusterKey;
    const string fileKey = stable_hash_text(dedupeKey);
    if (!g_seenClusters.insert(dedupeKey).second) {
        return;
    }

    ++g_multiclassCatalogStats.clusterCount;
    switch (observation.category) {
        case MulticlassCatalogCategory::NONE:
            break;
        case MulticlassCatalogCategory::HARMLESS:
            ++g_multiclassCatalogStats.harmlessClusterCount;
            break;
        case MulticlassCatalogCategory::TRACE_ONLY:
            ++g_multiclassCatalogStats.traceOnlyClusterCount;
            break;
        case MulticlassCatalogCategory::SEMANTIC_SHIFT:
            ++g_multiclassCatalogStats.semanticShiftClusterCount;
            break;
    }
    const string familyCategoryKey =
        familyName + ":" + multiclass_catalog_category_name(observation.category);
    ++g_multiclassCatalogStats.familyCategoryHistogram[familyCategoryKey];

    const filesystem::path outDir = artifact_subdir(options, "logs") / "multiclass_catalog";
    filesystem::create_directories(outDir);
    const filesystem::path outPath = outDir / (caseName + "_" + fileKey + ".summary.txt");
    ofstream ofs(outPath);
    if (!ofs) {
        throw runtime_error("failed to write multiclass catalog summary: " + outPath.string());
    }

    const filesystem::path witnessPath = outDir / (caseName + "_" + fileKey + ".state.txt");
    save_planner_state_dump(witnessPath, planner_dump_from_scenario(scenario, options, caseName));

    ofs << "case=" << caseName << '\n';
    ofs << "family=" << familyName << '\n';
    ofs << "cluster_key=" << fileKey << '\n';
    ofs << "catalog_dedupe_key=" << dedupeKey << '\n';
    ofs << "base_cluster_key=" << observation.clusterKey << '\n';
    ofs << "category=" << multiclass_catalog_category_name(observation.category) << '\n';
    ofs << "admissible_pair_count=" << observation.admissiblePairCount << '\n';
    ofs << "semantic_class_count=" << observation.semanticClassCount << '\n';
    ofs << "representative_shifted=" << (observation.representativeShifted ? 1 : 0) << '\n';
    ofs << "fast_selected_pair=" << observation.fastSelectedPair.first << ',' << observation.fastSelectedPair.second
        << '\n';
    ofs << "exact_shadow_selected_pair=" << observation.exactShadowSelectedPair.first << ','
        << observation.exactShadowSelectedPair.second << '\n';
    ofs << "exact_full_selected_pair=" << observation.exactFullSelectedPair.first << ','
        << observation.exactFullSelectedPair.second << '\n';
    ofs << "stop_condition_divergence=" << (observation.stopConditionDivergence ? 1 : 0) << '\n';
    ofs << "final_state_divergence=" << (observation.finalStateDivergence ? 1 : 0) << '\n';
    ofs << "scenario_label=" << scenario.label << '\n';
    ofs << "witness_state=" << witnessPath.string() << '\n';
    ofs << describe_split_choice_oracle_result(result);
}

optional<MulticlassCatalogEntry> build_multiclass_catalog_entry(
    const string& familyName,
    const ExhaustiveScenario&,
    const SplitChoiceOracleRunResult& result
) {
    const MulticlassCatalogObservation observation = observe_multiclass_catalog(result);
    if (observation.clusterKey.empty()) {
        return nullopt;
    }

    MulticlassCatalogEntry entry;
    entry.clusterKey = stable_hash_text(
        familyName + ":" + multiclass_catalog_category_name(observation.category) + ":" + observation.clusterKey
    );
    entry.familyName = familyName;
    entry.admissiblePairCount = observation.admissiblePairCount;
    entry.semanticClassCount = observation.semanticClassCount;
    entry.representativeShifted = observation.representativeShifted;
    entry.fastSelectedPair = observation.fastSelectedPair;
    entry.exactShadowSelectedPair = observation.exactShadowSelectedPair;
    entry.exactFullSelectedPair = observation.exactFullSelectedPair;
    entry.stopConditionDivergence = observation.stopConditionDivergence;
    entry.finalStateDivergence = observation.finalStateDivergence;
    entry.category = observation.category;
    return entry;
}

void write_multiclass_catalog_entry(
    const TestOptions& options,
    const string& caseName,
    const ExhaustiveScenario& scenario,
    const MulticlassCatalogEntry& entry,
    const SplitChoiceOracleRunResult& result
) {
    const filesystem::path outDir = artifact_subdir(options, "logs") / "multiclass_catalog";
    filesystem::create_directories(outDir);
    const filesystem::path outPath = outDir / (caseName + "_" + entry.clusterKey + ".summary.txt");
    ofstream ofs(outPath);
    if (!ofs) {
        throw runtime_error("failed to write multiclass catalog summary: " + outPath.string());
    }
    const filesystem::path witnessPath = outDir / (caseName + "_" + entry.clusterKey + ".state.txt");
    save_planner_state_dump(witnessPath, planner_dump_from_scenario(scenario, options, caseName));

    ofs << "case=" << caseName << '\n';
    ofs << "family=" << entry.familyName << '\n';
    ofs << "cluster_key=" << entry.clusterKey << '\n';
    ofs << "category=" << multiclass_catalog_category_name(entry.category) << '\n';
    ofs << "admissible_pair_count=" << entry.admissiblePairCount << '\n';
    ofs << "semantic_class_count=" << entry.semanticClassCount << '\n';
    ofs << "representative_shifted=" << (entry.representativeShifted ? 1 : 0) << '\n';
    ofs << "fast_selected_pair=" << entry.fastSelectedPair.first << ',' << entry.fastSelectedPair.second << '\n';
    ofs << "exact_shadow_selected_pair=" << entry.exactShadowSelectedPair.first << ','
        << entry.exactShadowSelectedPair.second << '\n';
    ofs << "exact_full_selected_pair=" << entry.exactFullSelectedPair.first << ','
        << entry.exactFullSelectedPair.second << '\n';
    ofs << "stop_condition_divergence=" << (entry.stopConditionDivergence ? 1 : 0) << '\n';
    ofs << "final_state_divergence=" << (entry.finalStateDivergence ? 1 : 0) << '\n';
    ofs << "scenario_label=" << scenario.label << '\n';
    ofs << "witness_state=" << witnessPath.string() << '\n';
    ofs << describe_split_choice_oracle_result(result);
}
