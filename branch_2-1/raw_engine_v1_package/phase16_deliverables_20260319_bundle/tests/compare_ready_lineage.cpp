#include "compare_ready_lineage.hpp"

#include <algorithm>
#include <sstream>

#include "exhaustive_generator.hpp"
#include "failure_signature.hpp"

using namespace std;

namespace {

const char* bool_text(bool value) {
    return value ? "1" : "0";
}

void count_base_reason(CompareReadyLineageSummary& summary, CompareEligibilityReason reason) {
    switch (reason) {
        case CompareEligibilityReason::ELIGIBLE:
            break;
        case CompareEligibilityReason::NO_SPLIT_READY:
            ++summary.noSplitReadyCount;
            break;
        case CompareEligibilityReason::SINGLE_ADMISSIBLE_PAIR:
            ++summary.singleAdmissiblePairCount;
            break;
        case CompareEligibilityReason::TIE_BUT_NO_BOUNDARY_ARTIFACT:
            ++summary.boundaryArtifactAbsentCount;
            break;
        case CompareEligibilityReason::TIE_BUT_NO_KEEPOCC_SIBLING:
            ++summary.keepOccAbsentCount;
            break;
        case CompareEligibilityReason::PLANNER_MIXED_FOLLOWUP_ABSENT:
            ++summary.tieButFollowupAbsentCount;
            break;
    }
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
        << " reason=" << compare_eligibility_reason_name(info.reason)
        << " evaluated_pairs=" << info.evaluatedPairCount;
    return oss.str();
}

} // namespace

CompareReadyLineageSummary audit_compare_ready_lineage(
    const TestOptions& options,
    const vector<u32>& scenarioSeeds
) {
    CompareReadyLineageSummary summary;

    TestOptions auditOptions = options;
    auditOptions.oracleMode = OracleMode::ALL;
    auditOptions.splitChoicePolicyMode = SplitChoicePolicyMode::EXACT_SHADOW;
    auditOptions.splitChoiceCompareMode = SplitChoiceCompareMode::EXACT_FULL;
    auditOptions.exactCanonicalCap = max<size_t>(auditOptions.exactCanonicalCap, 8U);

    for (u32 scenarioSeed : scenarioSeeds) {
        const ExhaustiveScenario baseScenario =
            make_targeted_planner_exhaustive_scenario(ScenarioFamily::PLANNER_TIE_MIXED_ORGANIC, scenarioSeed);
        const ExhaustiveScenario compareReadyScenario =
            make_targeted_planner_exhaustive_scenario(
                ScenarioFamily::PLANNER_TIE_MIXED_ORGANIC_COMPARE_READY,
                scenarioSeed
            );

        const CompareEligibilityInfo baseInfo =
            analyze_compare_eligibility(baseScenario, options.maxSplitPairCandidates);
        const CompareEligibilityInfo compareReadyInfo =
            analyze_compare_eligibility(compareReadyScenario, options.maxSplitPairCandidates);

        CompareReadyLineageEntry entry;
        entry.scenarioSeed = scenarioSeed;
        entry.baseFamily = "planner_tie_mixed_organic";
        entry.baseStateHash = hash_explorer_state_signature(capture_explorer_state_signature(baseScenario));
        entry.compareReadyStateHash =
            hash_explorer_state_signature(capture_explorer_state_signature(compareReadyScenario));
        entry.reasonCode = compare_eligibility_reason_name(baseInfo.reason);
        entry.basePrecheck = baseInfo;
        entry.compareReadyPrecheck = compareReadyInfo;

        ++summary.generatedStateCount;
        if (baseInfo.compareEligible) {
            ++summary.compareEligibleStateCount;
        } else {
            ++summary.compareIneligibleStateCount;
            ++summary.compareIneligibleReasonHistogram[entry.reasonCode];
            count_base_reason(summary, baseInfo.reason);
        }

        ++summary.derivedFromBaseStateCount;
        ++summary.compareReadyStateCount;

        if (compareReadyInfo.compareEligible) {
            const SplitChoiceOracleRunResult auditResult =
                run_split_choice_oracle(auditOptions, compareReadyScenario, nullptr);
            if (auditResult.exactAuditAvailable) {
                entry.compareCompleted = true;
                entry.sameSemanticClassCount = auditResult.sameSemanticClassCount;
                entry.sameFinalStateCount = auditResult.sameFinalStateCount;
                entry.structureOnlyEnrichment =
                    !baseInfo.compareEligible &&
                    compareReadyInfo.compareEligible &&
                    auditResult.sameSemanticClassCount == 1U &&
                    auditResult.sameFinalStateCount == 1U &&
                    auditResult.semanticDisagreementCount == 0U &&
                    auditResult.fallbackCount == 0U;
                ++summary.compareCompletedStateCount;
                summary.sameSemanticClassCount += auditResult.sameSemanticClassCount;
                summary.sameFinalStateCount += auditResult.sameFinalStateCount;
                ++summary.pairReplayCount;
                if (entry.structureOnlyEnrichment) {
                    ++summary.structureOnlyEnrichmentCount;
                }
            }
        }

        summary.entries.push_back(std::move(entry));
    }

    return summary;
}

string compare_ready_lineage_summary_text(const CompareReadyLineageSummary& summary) {
    ostringstream oss;
    oss << "base_family=planner_tie_mixed_organic\n";
    oss << "generated_state_count=" << summary.generatedStateCount << '\n';
    oss << "compare_eligible_state_count=" << summary.compareEligibleStateCount << '\n';
    oss << "compare_ineligible_state_count=" << summary.compareIneligibleStateCount << '\n';
    oss << "compare_ineligible_reason_histogram=";
    {
        bool first = true;
        oss << '{';
        for (const auto& [key, value] : summary.compareIneligibleReasonHistogram) {
            if (!first) {
                oss << ',';
            }
            first = false;
            oss << '"' << key << "\":" << value;
        }
        oss << '}';
    }
    oss << '\n';
    oss << "no_split_ready_count=" << summary.noSplitReadyCount << '\n';
    oss << "single_admissible_pair_count=" << summary.singleAdmissiblePairCount << '\n';
    oss << "tie_but_followup_absent_count=" << summary.tieButFollowupAbsentCount << '\n';
    oss << "keepOcc_absent_count=" << summary.keepOccAbsentCount << '\n';
    oss << "boundary_artifact_absent_count=" << summary.boundaryArtifactAbsentCount << '\n';
    oss << "derived_from_base_state_count=" << summary.derivedFromBaseStateCount << '\n';
    oss << "compare_ready_state_count=" << summary.compareReadyStateCount << '\n';
    oss << "compare_completed_state_count=" << summary.compareCompletedStateCount << '\n';
    oss << "same_semantic_class_count=" << summary.sameSemanticClassCount << '\n';
    oss << "same_final_state_count=" << summary.sameFinalStateCount << '\n';
    oss << "pair_replay_count=" << summary.pairReplayCount << '\n';
    oss << "structure_only_enrichment_count=" << summary.structureOnlyEnrichmentCount << '\n';
    return oss.str();
}

string compare_ready_lineage_log_text(const CompareReadyLineageSummary& summary) {
    ostringstream oss;
    for (const CompareReadyLineageEntry& entry : summary.entries) {
        oss << "scenario_seed=" << entry.scenarioSeed
            << " base_family=" << entry.baseFamily
            << " base_state_hash=" << entry.baseStateHash
            << " compare_ready_state_hash=" << entry.compareReadyStateHash
            << " reason_code=" << entry.reasonCode
            << " compare_completed=" << bool_text(entry.compareCompleted)
            << " same_semantic_class_count=" << entry.sameSemanticClassCount
            << " same_final_state_count=" << entry.sameFinalStateCount
            << " structure_only_enrichment=" << bool_text(entry.structureOnlyEnrichment)
            << " base_precheck={" << describe_precheck(entry.basePrecheck) << '}'
            << " compare_ready_precheck={" << describe_precheck(entry.compareReadyPrecheck) << '}'
            << '\n';
    }
    return oss.str();
}
