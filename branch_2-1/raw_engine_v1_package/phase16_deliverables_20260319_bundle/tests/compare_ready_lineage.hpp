#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "split_choice_oracle.hpp"
#include "test_harness.hpp"

struct CompareReadyLineageEntry {
    u32 scenarioSeed = 0U;
    std::string baseFamily;
    std::string baseStateHash;
    std::string compareReadyStateHash;
    std::string reasonCode;
    CompareEligibilityInfo basePrecheck;
    CompareEligibilityInfo compareReadyPrecheck;
    bool compareCompleted = false;
    std::size_t sameSemanticClassCount = 0U;
    std::size_t sameFinalStateCount = 0U;
    bool structureOnlyEnrichment = false;
};

struct CompareReadyLineageSummary {
    std::size_t generatedStateCount = 0U;
    std::size_t compareEligibleStateCount = 0U;
    std::size_t compareIneligibleStateCount = 0U;
    std::size_t noSplitReadyCount = 0U;
    std::size_t singleAdmissiblePairCount = 0U;
    std::size_t tieButFollowupAbsentCount = 0U;
    std::size_t keepOccAbsentCount = 0U;
    std::size_t boundaryArtifactAbsentCount = 0U;
    std::unordered_map<std::string, std::size_t> compareIneligibleReasonHistogram;
    std::size_t derivedFromBaseStateCount = 0U;
    std::size_t compareReadyStateCount = 0U;
    std::size_t compareCompletedStateCount = 0U;
    std::size_t sameSemanticClassCount = 0U;
    std::size_t sameFinalStateCount = 0U;
    std::size_t pairReplayCount = 0U;
    std::size_t structureOnlyEnrichmentCount = 0U;
    std::vector<CompareReadyLineageEntry> entries;
};

CompareReadyLineageSummary audit_compare_ready_lineage(
    const TestOptions& options,
    const std::vector<u32>& scenarioSeeds
);

std::string compare_ready_lineage_summary_text(const CompareReadyLineageSummary& summary);
std::string compare_ready_lineage_log_text(const CompareReadyLineageSummary& summary);
