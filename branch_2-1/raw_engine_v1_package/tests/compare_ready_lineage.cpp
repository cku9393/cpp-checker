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
        << " mixed_compare_eligible=" << bool_text(info.mixedCompareEligible)
        << " reason=" << compare_eligibility_reason_name(info.reason)
        << " evaluated_pairs=" << info.evaluatedPairCount;
    return oss.str();
}

string hash_target_prepare_signature(const RawEngine& RE, OccID targetOcc, const string& label) {
    const RawSkelID sid = RE.occ.get(targetOcc).hostSkel;
    const IsolatePrepared prep = prepare_isolate_checked(RE, sid, targetOcc, label);
    return hash_isolate_signature(capture_isolate_signature(prep));
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
    vector<string> followup;
    followup.reserve(trace.size());
    for (const PlannerTraceEntry& entry : trace) {
        if (entry.primitive == PlannerPrimitiveKind::JOIN ||
            entry.primitive == PlannerPrimitiveKind::INTEGRATE) {
            followup.push_back(planner_primitive_name(entry.primitive));
        }
    }
    if (followup.empty()) {
        return "none";
    }
    ostringstream oss;
    for (size_t i = 0; i < followup.size(); ++i) {
        if (i != 0U) {
            oss << '>';
        }
        oss << followup[i];
    }
    return oss.str();
}

const SplitChoiceCandidateOutcome* find_outcome_by_pair(
    const vector<SplitChoiceCandidateOutcome>& outcomes,
    pair<Vertex, Vertex> pairValue
) {
    const auto it = find_if(
        outcomes.begin(),
        outcomes.end(),
        [&](const SplitChoiceCandidateOutcome& outcome) {
            return outcome.pair == pairValue;
        }
    );
    return it == outcomes.end() ? nullptr : &*it;
}

PlannerExecutionResult run_lineage_planner_scenario(
    const TestOptions& options,
    ExhaustiveScenario scenario,
    const string& label
) {
    TestOptions plannerOptions = options;
    plannerOptions.oracleMode = OracleMode::PLANNER;
    plannerOptions.splitChoicePolicyMode = SplitChoicePolicyMode::EXACT_SHADOW;
    set_active_test_options(&plannerOptions);
    const PlannerExecutionResult result = run_planner_checked_capture(
        scenario.RE,
        scenario.ctx,
        scenario.U,
        planner_run_options(plannerOptions),
        scenario.initialQueue.empty() ? nullptr : &scenario.initialQueue,
        label
    );
    set_active_test_options(&options);
    return result;
}

string operation_count_delta_key(int delta) {
    return (delta >= 0 ? "+" : "") + to_string(delta);
}

string injected_structure_delta_key(const CompareReadyLineageEntry& entry) {
    vector<string> tokens;
    if (entry.forcedSplitReadySupport) {
        tokens.emplace_back("forced_split_ready_support");
    }
    if (entry.addedBoundaryArtifact) {
        tokens.emplace_back("added_boundary_artifact");
    }
    if (entry.injectedKeepOccSibling) {
        tokens.emplace_back("injected_keepOcc_sibling");
    }
    tokens.emplace_back("op_delta=" + operation_count_delta_key(entry.operationCountDelta));
    ostringstream oss;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i != 0U) {
            oss << ',';
        }
        oss << tokens[i];
    }
    return oss.str();
}

string paired_lineage_fingerprint(const CompareReadyLineageEntry& entry) {
    return stable_hash_text(
        entry.baseStateHash + "|" +
        entry.baseTargetPrepareHash + "|" +
        entry.compareReadyStateHash + "|" +
        entry.compareReadyTargetPrepareHash + "|" +
        entry.reasonCode
    );
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
        entry.addedBoundaryArtifact =
            !baseInfo.hasBoundaryOnlyChildAfterSplit && compareReadyInfo.hasBoundaryOnlyChildAfterSplit;
        entry.injectedKeepOccSibling =
            !baseInfo.hasKeepOccSiblingAfterSplit && compareReadyInfo.hasKeepOccSiblingAfterSplit;
        entry.forcedSplitReadySupport = !baseInfo.hasSplitReady && compareReadyInfo.hasSplitReady;

        ++summary.generatedStateCount;
        if (baseInfo.compareEligible) {
            ++summary.compareEligibleStateCount;
        } else {
            ++summary.compareIneligibleStateCount;
            ++summary.compareIneligibleReasonHistogram[entry.reasonCode];
            count_base_reason(summary, baseInfo.reason);
        }

        ++summary.compareReadyStateCount;
        entry.baseTargetPrepareHash = hash_target_prepare_signature(
            baseScenario.RE,
            baseScenario.ctx.targetOcc,
            "compare_ready_lineage_base_prepare"
        );
        const PlannerExecutionResult basePlannerResult = run_lineage_planner_scenario(
            auditOptions,
            baseScenario,
            "compare_ready_lineage_base"
        );
        entry.baseTraceClass = planner_trace_class_key(basePlannerResult.trace);
        entry.baseFollowupPattern = planner_followup_pattern_key(basePlannerResult.trace);
        if (entry.addedBoundaryArtifact) {
            ++summary.addedBoundaryArtifactCount;
        }
        if (entry.injectedKeepOccSibling) {
            ++summary.injectedKeepOccSiblingCount;
        }
        if (entry.forcedSplitReadySupport) {
            ++summary.forcedSplitReadySupportCount;
        }
        if (compareReadyInfo.compareEligible) {
            const SplitChoiceOracleRunResult auditResult =
                run_split_choice_oracle(auditOptions, compareReadyScenario, nullptr);
            if (auditResult.exactAuditAvailable) {
                const SplitChoiceCandidateOutcome* exactShadow =
                    find_outcome_by_pair(auditResult.outcomes, auditResult.exactShadowSelectedPair);
                const SplitChoiceCandidateOutcome* exactFull =
                    find_outcome_by_pair(auditResult.outcomes, auditResult.exactFullSelectedPair);
                if (exactShadow != nullptr) {
                    ++summary.pairReplayCount;
                    entry.compareReadyTargetPrepareHash = exactShadow->targetPrepareHash;
                    entry.pairedLineageFingerprint = paired_lineage_fingerprint(entry);
                    entry.derivedFromBaseState =
                        !entry.baseStateHash.empty() &&
                        !entry.baseTargetPrepareHash.empty() &&
                        !entry.compareReadyStateHash.empty() &&
                        !entry.compareReadyTargetPrepareHash.empty() &&
                        !entry.reasonCode.empty() &&
                        !entry.pairedLineageFingerprint.empty();
                    if (entry.derivedFromBaseState) {
                        ++summary.derivedFromBaseStateCount;
                    }
                }
                entry.compareCompleted = true;
                entry.sameSemanticClassCount = auditResult.sameSemanticClassCount;
                entry.sameFinalStateCount = auditResult.sameFinalStateCount;
                entry.compareReadyTraceClass = exactShadow == nullptr
                    ? "none"
                    : planner_trace_class_key(exactShadow->trace);
                entry.compareReadyFollowupPattern = exactShadow == nullptr
                    ? "none"
                    : planner_followup_pattern_key(exactShadow->trace);
                entry.followupPatternPreserved =
                    entry.baseFollowupPattern == entry.compareReadyFollowupPattern;
                entry.operationCountDelta = exactShadow == nullptr
                    ? 0
                    : static_cast<int>(exactShadow->trace.size()) - static_cast<int>(basePlannerResult.trace.size());
                entry.exactShadowTraceClass = exactShadow == nullptr
                    ? "none"
                    : planner_trace_class_key(exactShadow->trace);
                entry.exactFullTraceClass = exactFull == nullptr
                    ? "none"
                    : planner_trace_class_key(exactFull->trace);
                entry.sameTraceClass = entry.exactShadowTraceClass == entry.exactFullTraceClass;
                entry.injectedStructureDeltaKey = injected_structure_delta_key(entry);
                entry.structureOnlyEnrichment =
                    entry.derivedFromBaseState &&
                    !baseInfo.compareEligible &&
                    compareReadyInfo.compareEligible &&
                    auditResult.sameSemanticClassCount == 1U &&
                    auditResult.sameFinalStateCount == 1U &&
                    entry.sameTraceClass &&
                    auditResult.semanticDisagreementCount == 0U &&
                    auditResult.fallbackCount == 0U &&
                    entry.followupPatternPreserved;
                ++summary.compareCompletedStateCount;
                if (entry.derivedFromBaseState) {
                    ++summary.lineageSampleCount;
                }
                if (entry.followupPatternPreserved) {
                    ++summary.followupPatternPreservedCount;
                }
                ++summary.operationCountDeltaHistogram[operation_count_delta_key(entry.operationCountDelta)];
                ++summary.injectedStructureDeltaHistogram[entry.injectedStructureDeltaKey];
                summary.sameSemanticClassCount += auditResult.sameSemanticClassCount;
                summary.sameFinalStateCount += auditResult.sameFinalStateCount;
                if (entry.sameTraceClass) {
                    ++summary.sameTraceClassCount;
                }
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
    oss << "lineage_sample_count=" << summary.lineageSampleCount << '\n';
    oss << "same_semantic_class_count=" << summary.sameSemanticClassCount << '\n';
    oss << "same_final_state_count=" << summary.sameFinalStateCount << '\n';
    oss << "same_trace_class_count=" << summary.sameTraceClassCount << '\n';
    oss << "pair_replay_count=" << summary.pairReplayCount << '\n';
    oss << "structure_only_enrichment_count=" << summary.structureOnlyEnrichmentCount << '\n';
    oss << "added_boundary_artifact_count=" << summary.addedBoundaryArtifactCount << '\n';
    oss << "injected_keepOcc_sibling_count=" << summary.injectedKeepOccSiblingCount << '\n';
    oss << "forced_split_ready_support_count=" << summary.forcedSplitReadySupportCount << '\n';
    oss << "followup_pattern_preserved_count=" << summary.followupPatternPreservedCount << '\n';
    oss << "injected_structure_delta_histogram={";
    {
        bool first = true;
        for (const auto& [key, value] : summary.injectedStructureDeltaHistogram) {
            if (!first) {
                oss << ',';
            }
            first = false;
            oss << '"' << key << "\":" << value;
        }
    }
    oss << "}\n";
    oss << "operation_count_delta_histogram={";
    {
        bool first = true;
        for (const auto& [key, value] : summary.operationCountDeltaHistogram) {
            if (!first) {
                oss << ',';
            }
            first = false;
            oss << '"' << key << "\":" << value;
        }
    }
    oss << "}\n";
    return oss.str();
}

string compare_ready_lineage_log_text(const CompareReadyLineageSummary& summary) {
    ostringstream oss;
    for (const CompareReadyLineageEntry& entry : summary.entries) {
        oss << "scenario_seed=" << entry.scenarioSeed
            << " base_family=" << entry.baseFamily
            << " base_state_hash=" << entry.baseStateHash
            << " compare_ready_state_hash=" << entry.compareReadyStateHash
            << " base_target_prepare_hash=" << entry.baseTargetPrepareHash
            << " compare_ready_target_prepare_hash=" << entry.compareReadyTargetPrepareHash
            << " paired_lineage_fingerprint=" << entry.pairedLineageFingerprint
            << " reason_code=" << entry.reasonCode
            << " derived_from_base_state=" << bool_text(entry.derivedFromBaseState)
            << " compare_completed=" << bool_text(entry.compareCompleted)
            << " same_semantic_class_count=" << entry.sameSemanticClassCount
            << " same_final_state_count=" << entry.sameFinalStateCount
            << " same_trace_class=" << bool_text(entry.sameTraceClass)
            << " structure_only_enrichment=" << bool_text(entry.structureOnlyEnrichment)
            << " added_boundary_artifact=" << bool_text(entry.addedBoundaryArtifact)
            << " injected_keepOcc_sibling=" << bool_text(entry.injectedKeepOccSibling)
            << " forced_split_ready_support=" << bool_text(entry.forcedSplitReadySupport)
            << " operation_count_delta=" << entry.operationCountDelta
            << " base_trace_class=" << entry.baseTraceClass
            << " compare_ready_trace_class=" << entry.compareReadyTraceClass
            << " base_followup_pattern=" << entry.baseFollowupPattern
            << " compare_ready_followup_pattern=" << entry.compareReadyFollowupPattern
            << " followup_pattern_preserved=" << bool_text(entry.followupPatternPreserved)
            << " exact_shadow_trace_class=" << entry.exactShadowTraceClass
            << " exact_full_trace_class=" << entry.exactFullTraceClass
            << " injected_structure_delta={" << entry.injectedStructureDeltaKey << '}'
            << " base_precheck={" << describe_precheck(entry.basePrecheck) << '}'
            << " compare_ready_precheck={" << describe_precheck(entry.compareReadyPrecheck) << '}'
            << '\n';
    }
    return oss.str();
}
