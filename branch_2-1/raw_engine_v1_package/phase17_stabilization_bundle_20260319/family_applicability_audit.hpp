#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "split_choice_oracle.hpp"
#include "test_harness.hpp"

enum class FamilyApplicabilityClassification : u8 {
    DIRECTLY_APPLICABLE = 0,
    UNDER_GENERATED = 1,
    NON_APPLICABLE = 2,
};

struct FamilyApplicabilityThresholds {
    std::size_t minGeneratedStates = 48U;
    double maxCompareRelevanceForNonApplicable = 0.02;
    double maxSplitReadyRelevanceForNonApplicable = 0.05;
    double minDominantReasonConfidence = 0.90;
};

struct FamilyApplicabilityEntry {
    u32 scenarioSeed = 0U;
    std::string family;
    std::string stateHash;
    CompareEligibilityInfo precheck;
    std::size_t actualSplitHits = 0U;
    std::size_t actualJoinHits = 0U;
    std::size_t actualIntegrateHits = 0U;
    std::string traceClass;
    std::string followupPattern;
};

struct FamilyApplicabilitySummary {
    std::string family;
    std::size_t generatedStateCount = 0U;
    std::size_t splitReadyStateCount = 0U;
    std::size_t tieReadyStateCount = 0U;
    std::size_t compareEligibleStateCount = 0U;
    std::size_t compareIneligibleStateCount = 0U;
    std::unordered_map<std::string, std::size_t> compareIneligibleReasonHistogram;
    std::size_t actualSplitHits = 0U;
    std::size_t actualJoinHits = 0U;
    std::size_t actualIntegrateHits = 0U;
    double splitReadyRelevance = 0.0;
    double tieReadyRelevance = 0.0;
    double compareRelevance = 0.0;
    std::string dominantIneligibleReason;
    double dominantIneligibleReasonConfidence = 0.0;
    FamilyApplicabilityThresholds thresholds;
    FamilyApplicabilityClassification classification = FamilyApplicabilityClassification::UNDER_GENERATED;
    std::vector<FamilyApplicabilityEntry> entries;
};

const char* family_applicability_classification_name(FamilyApplicabilityClassification classification);

FamilyApplicabilitySummary audit_family_applicability(
    const TestOptions& options,
    ScenarioFamily family,
    const std::vector<u32>& scenarioSeeds
);

std::string family_applicability_summary_text(const FamilyApplicabilitySummary& summary);
std::string family_applicability_log_text(const FamilyApplicabilitySummary& summary);
