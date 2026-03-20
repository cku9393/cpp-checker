#include "family_applicability_audit.hpp"

#include <algorithm>
#include <deque>
#include <sstream>

#include "exhaustive_generator.hpp"

using namespace std;

namespace {

string bool_text(bool value) {
    return value ? "1" : "0";
}

double safe_fraction(size_t numerator, size_t denominator) {
    if (denominator == 0U) {
        return 0.0;
    }
    return static_cast<double>(numerator) / static_cast<double>(denominator);
}

string planner_trace_class_key(const vector<PlannerTraceEntry>& trace) {
    if (trace.empty()) {
        return "empty";
    }
    ostringstream oss;
    for (size_t i = 0; i < trace.size(); ++i) {
        if (i != 0U) {
            oss << '>';
        }
        oss << planner_primitive_name(trace[i].primitive);
    }
    return oss.str();
}

string planner_followup_pattern_key(const vector<PlannerTraceEntry>& trace) {
    vector<string> tokens;
    tokens.reserve(trace.size());
    for (const PlannerTraceEntry& entry : trace) {
        if (entry.primitive == PlannerPrimitiveKind::JOIN ||
            entry.primitive == PlannerPrimitiveKind::INTEGRATE) {
            tokens.push_back(planner_primitive_name(entry.primitive));
        }
    }
    if (tokens.empty()) {
        return "none";
    }
    ostringstream oss;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i != 0U) {
            oss << '>';
        }
        oss << tokens[i];
    }
    return oss.str();
}

string describe_precheck(const CompareEligibilityInfo& info) {
    ostringstream oss;
    oss << "has_split_ready=" << bool_text(info.hasSplitReady)
        << " admissible_pairs=" << info.admissibleSplitPairCount
        << " has_tie=" << bool_text(info.hasTie)
        << " boundary_artifact=" << bool_text(info.hasBoundaryOnlyChildAfterSplit)
        << " keep_occ=" << bool_text(info.hasKeepOccSiblingAfterSplit)
        << " mixed_followup=" << bool_text(info.hasPlannerMixedFollowup)
        << " compare_eligible=" << bool_text(info.compareEligible)
        << " mixed_compare_eligible=" << bool_text(info.mixedCompareEligible)
        << " reason=" << compare_eligibility_reason_name(info.reason)
        << " evaluated_pairs=" << info.evaluatedPairCount;
    return oss.str();
}

bool policy_relevant_compare_eligible(ScenarioFamily family, const CompareEligibilityInfo& info) {
    switch (family) {
        case ScenarioFamily::PLANNER_TIE_MIXED:
        case ScenarioFamily::PLANNER_TIE_MIXED_SYMMETRIC:
        case ScenarioFamily::PLANNER_TIE_MIXED_ORGANIC:
            return info.mixedCompareEligible;
        default:
            return info.compareEligible;
    }
}

FamilyApplicabilityThresholds resolve_thresholds(const TestOptions& options) {
    FamilyApplicabilityThresholds thresholds;
    if (options.targetApplicabilityConfidence > 0.0) {
        thresholds.minDominantReasonConfidence = options.targetApplicabilityConfidence;
    }
    return thresholds;
}

FamilyApplicabilityClassification classify_summary(
    const FamilyApplicabilitySummary& summary
) {
    if (summary.generatedStateCount < summary.thresholds.minGeneratedStates) {
        return FamilyApplicabilityClassification::UNDER_GENERATED;
    }
    if (summary.compareEligibleStateCount != 0U) {
        return FamilyApplicabilityClassification::DIRECTLY_APPLICABLE;
    }
    if (summary.compareRelevance <= summary.thresholds.maxCompareRelevanceForNonApplicable &&
        summary.splitReadyRelevance <= summary.thresholds.maxSplitReadyRelevanceForNonApplicable &&
        summary.dominantIneligibleReason == "no_split_ready" &&
        summary.dominantIneligibleReasonConfidence >= summary.thresholds.minDominantReasonConfidence &&
        summary.actualJoinHits != 0U &&
        summary.actualIntegrateHits != 0U) {
        return FamilyApplicabilityClassification::NON_APPLICABLE;
    }
    return FamilyApplicabilityClassification::UNDER_GENERATED;
}

} // namespace

const char* family_applicability_classification_name(FamilyApplicabilityClassification classification) {
    switch (classification) {
        case FamilyApplicabilityClassification::DIRECTLY_APPLICABLE:
            return "DIRECTLY_APPLICABLE";
        case FamilyApplicabilityClassification::UNDER_GENERATED:
            return "UNDER_GENERATED";
        case FamilyApplicabilityClassification::NON_APPLICABLE:
            return "NON_APPLICABLE";
    }
    return "UNDER_GENERATED";
}

FamilyApplicabilitySummary audit_family_applicability(
    const TestOptions& options,
    ScenarioFamily family,
    const vector<u32>& scenarioSeeds
) {
    FamilyApplicabilitySummary summary;
    summary.family = scenario_family_name_string(family);
    summary.thresholds = resolve_thresholds(options);

    TestOptions plannerOptions = options;
    plannerOptions.oracleMode = OracleMode::PLANNER;

    for (u32 scenarioSeed : scenarioSeeds) {
        ExhaustiveScenario scenario = make_targeted_planner_exhaustive_scenario(family, scenarioSeed);
        const CompareEligibilityInfo precheck =
            analyze_compare_eligibility(scenario, options.maxSplitPairCandidates);

        set_active_test_options(&plannerOptions);
        const PlannerExecutionResult plannerResult = run_planner_checked_capture(
            scenario.RE,
            scenario.ctx,
            scenario.U,
            planner_run_options(plannerOptions),
            scenario.initialQueue.empty() ? nullptr : &scenario.initialQueue,
            summary.family + "_applicability"
        );
        set_active_test_options(&options);

        FamilyApplicabilityEntry entry;
        entry.scenarioSeed = scenarioSeed;
        entry.family = summary.family;
        entry.stateHash = stable_hash_text(summary.family + "_seed" + to_string(scenarioSeed));
        entry.precheck = precheck;
        entry.actualSplitHits = plannerResult.coverage.actualSplitHits;
        entry.actualJoinHits = plannerResult.coverage.actualJoinHits;
        entry.actualIntegrateHits = plannerResult.coverage.actualIntegrateHits;
        entry.traceClass = planner_trace_class_key(plannerResult.trace);
        entry.followupPattern = planner_followup_pattern_key(plannerResult.trace);
        summary.entries.push_back(entry);

        ++summary.generatedStateCount;
        if (precheck.hasSplitReady) {
            ++summary.splitReadyStateCount;
        }
        if (precheck.hasTie) {
            ++summary.tieReadyStateCount;
        }
        if (policy_relevant_compare_eligible(family, precheck)) {
            ++summary.compareEligibleStateCount;
        } else {
            ++summary.compareIneligibleStateCount;
            ++summary.compareIneligibleReasonHistogram[compare_eligibility_reason_name(precheck.reason)];
        }
        summary.actualSplitHits += plannerResult.coverage.actualSplitHits;
        summary.actualJoinHits += plannerResult.coverage.actualJoinHits;
        summary.actualIntegrateHits += plannerResult.coverage.actualIntegrateHits;
    }

    summary.splitReadyRelevance = safe_fraction(summary.splitReadyStateCount, summary.generatedStateCount);
    summary.tieReadyRelevance = safe_fraction(summary.tieReadyStateCount, summary.generatedStateCount);
    summary.compareRelevance = safe_fraction(summary.compareEligibleStateCount, summary.generatedStateCount);

    size_t dominantCount = 0U;
    for (const auto& [reason, count] : summary.compareIneligibleReasonHistogram) {
        if (count > dominantCount || (count == dominantCount && reason < summary.dominantIneligibleReason)) {
            dominantCount = count;
            summary.dominantIneligibleReason = reason;
        }
    }
    summary.dominantIneligibleReasonConfidence =
        safe_fraction(dominantCount, summary.compareIneligibleStateCount);
    summary.classification = classify_summary(summary);
    return summary;
}

string family_applicability_summary_text(const FamilyApplicabilitySummary& summary) {
    ostringstream oss;
    oss << "family=" << summary.family << '\n';
    oss << "generated_state_count=" << summary.generatedStateCount << '\n';
    oss << "split_ready_state_count=" << summary.splitReadyStateCount << '\n';
    oss << "tie_ready_state_count=" << summary.tieReadyStateCount << '\n';
    oss << "compare_eligible_state_count=" << summary.compareEligibleStateCount << '\n';
    oss << "compare_ineligible_state_count=" << summary.compareIneligibleStateCount << '\n';
    oss << "compare_ineligible_reason_histogram={";
    bool first = true;
    for (const auto& [key, value] : summary.compareIneligibleReasonHistogram) {
        if (!first) {
            oss << ',';
        }
        first = false;
        oss << '"' << key << "\":" << value;
    }
    oss << "}\n";
    oss << "actual_split_hits=" << summary.actualSplitHits << '\n';
    oss << "actual_join_hits=" << summary.actualJoinHits << '\n';
    oss << "actual_integrate_hits=" << summary.actualIntegrateHits << '\n';
    oss << "split_ready_relevance=" << summary.splitReadyRelevance << '\n';
    oss << "tie_ready_relevance=" << summary.tieReadyRelevance << '\n';
    oss << "compare_relevance=" << summary.compareRelevance << '\n';
    oss << "dominant_ineligible_reason=" << summary.dominantIneligibleReason << '\n';
    oss << "dominant_ineligible_reason_confidence=" << summary.dominantIneligibleReasonConfidence << '\n';
    oss << "classification=" << family_applicability_classification_name(summary.classification) << '\n';
    oss << "threshold_min_generated_states=" << summary.thresholds.minGeneratedStates << '\n';
    oss << "threshold_max_compare_relevance=" << summary.thresholds.maxCompareRelevanceForNonApplicable << '\n';
    oss << "threshold_max_split_ready_relevance=" << summary.thresholds.maxSplitReadyRelevanceForNonApplicable
        << '\n';
    oss << "threshold_min_dominant_reason_confidence=" << summary.thresholds.minDominantReasonConfidence << '\n';
    return oss.str();
}

string family_applicability_log_text(const FamilyApplicabilitySummary& summary) {
    ostringstream oss;
    for (const FamilyApplicabilityEntry& entry : summary.entries) {
        oss << "scenario_seed=" << entry.scenarioSeed
            << " family=" << entry.family
            << " state_hash=" << entry.stateHash
            << " actual_split_hits=" << entry.actualSplitHits
            << " actual_join_hits=" << entry.actualJoinHits
            << " actual_integrate_hits=" << entry.actualIntegrateHits
            << " trace_class=" << entry.traceClass
            << " followup_pattern=" << entry.followupPattern
            << " precheck={" << describe_precheck(entry.precheck) << '}'
            << '\n';
    }
    return oss.str();
}
