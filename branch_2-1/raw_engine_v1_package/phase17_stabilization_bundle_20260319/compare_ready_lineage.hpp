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
    std::string baseTargetPrepareHash;
    std::string compareReadyTargetPrepareHash;
    std::string pairedLineageFingerprint;
    std::string reasonCode;
    CompareEligibilityInfo basePrecheck;
    CompareEligibilityInfo compareReadyPrecheck;
    bool derivedFromBaseState = false;
    bool compareCompleted = false;
    std::size_t sameSemanticClassCount = 0U;
    std::size_t sameFinalStateCount = 0U;
    bool sameTraceClass = false;
    bool structureOnlyEnrichment = false;
    bool addedBoundaryArtifact = false;
    bool injectedKeepOccSibling = false;
    bool forcedSplitReadySupport = false;
    int operationCountDelta = 0;
    std::string baseTraceClass;
    std::string compareReadyTraceClass;
    std::string baseFollowupPattern;
    std::string compareReadyFollowupPattern;
    std::string exactShadowTraceClass;
    std::string exactFullTraceClass;
    bool followupPatternPreserved = false;
    std::string injectedStructureDeltaKey;
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
    std::size_t lineageSampleCount = 0U;
    std::size_t sameSemanticClassCount = 0U;
    std::size_t sameFinalStateCount = 0U;
    std::size_t sameTraceClassCount = 0U;
    std::size_t pairReplayCount = 0U;
    std::size_t structureOnlyEnrichmentCount = 0U;
    std::size_t addedBoundaryArtifactCount = 0U;
    std::size_t injectedKeepOccSiblingCount = 0U;
    std::size_t forcedSplitReadySupportCount = 0U;
    std::size_t followupPatternPreservedCount = 0U;
    std::unordered_map<std::string, std::size_t> injectedStructureDeltaHistogram;
    std::unordered_map<std::string, std::size_t> operationCountDeltaHistogram;
    std::vector<CompareReadyLineageEntry> entries;
};

CompareReadyLineageSummary audit_compare_ready_lineage(
    const TestOptions& options,
    const std::vector<u32>& scenarioSeeds
);

std::string compare_ready_lineage_summary_text(const CompareReadyLineageSummary& summary);
std::string compare_ready_lineage_log_text(const CompareReadyLineageSummary& summary);
