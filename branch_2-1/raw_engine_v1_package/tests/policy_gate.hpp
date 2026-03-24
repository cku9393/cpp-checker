#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "compare_ready_lineage.hpp"
#include "family_applicability_audit.hpp"

enum class PolicyGateStatus : u8 {
    PASS = 0,
    NON_APPLICABLE = 1,
    DIAGNOSTIC_ONLY = 2,
    FAIL = 3,
    INSUFFICIENT_EVIDENCE = 4,
};

enum class PolicyEvidenceSource : u8 {
    DIRECT_COMPARE = 0,
    APPLICABILITY_ONLY = 1,
    DIAGNOSTIC_LINEAGE = 2,
    CAMPAIGN_COMPARE = 3,
};

enum class PolicyFreshnessStatus : u8 {
    FRESH = 0,
    STALE = 1,
    REQUIRES_RERUN = 2,
};

struct PolicyRelevantInputHashes {
    std::string plannerSemanticsHash;
    std::string generatorFamilyHash;
    std::string compareEngineHash;
    std::string campaignConfigHash;
    std::string combinedHash;
};

struct PolicyGateThreshold {
    std::size_t comparedStateCountMin = 0U;
    std::size_t compareEligibleStateCountMin = 0U;
    std::size_t compareCompletedStateCountMin = 0U;
    std::size_t semanticDisagreementCountMax = 0U;
    std::size_t fallbackCountMax = 0U;
    std::size_t semanticShiftCountMax = 0U;
    std::size_t representativeShiftCountMax = static_cast<std::size_t>(-1);
    std::size_t applicabilityGeneratedStateCountMin = 0U;
    double applicabilityMaxCompareRelevance = 0.0;
    double applicabilityMaxSplitReadyRelevance = 0.0;
    double applicabilityMinDominantReasonConfidence = 0.0;
    std::size_t lineageSampleCountMin = 0U;
};

struct PolicyGateCompareEvidence {
    std::string family;
    std::size_t comparedStateCount = 0U;
    std::size_t compareEligibleStateCount = 0U;
    std::size_t compareIneligibleStateCount = 0U;
    std::unordered_map<std::string, std::size_t> compareIneligibleReasonHistogram;
    std::size_t compareCompletedStateCount = 0U;
    std::size_t semanticDisagreementCount = 0U;
    std::size_t fallbackCount = 0U;
    std::size_t semanticShiftCount = 0U;
    std::size_t representativeShiftCount = 0U;
    std::string sourceSummaryPath;
    bool available = false;
};

struct PolicyGateMeasuredEvidence {
    std::size_t generatedStateCount = 0U;
    std::size_t splitReadyStateCount = 0U;
    std::size_t tieReadyStateCount = 0U;
    std::size_t actualSplitHits = 0U;
    std::size_t actualJoinHits = 0U;
    std::size_t actualIntegrateHits = 0U;
    std::size_t comparedStateCount = 0U;
    std::size_t compareEligibleStateCount = 0U;
    std::size_t compareIneligibleStateCount = 0U;
    std::unordered_map<std::string, std::size_t> compareIneligibleReasonHistogram;
    std::size_t compareCompletedStateCount = 0U;
    std::size_t semanticDisagreementCount = 0U;
    std::size_t fallbackCount = 0U;
    std::size_t semanticShiftCount = 0U;
    std::size_t representativeShiftCount = 0U;
    std::size_t noSplitReadyCount = 0U;
    double compareRelevance = 0.0;
    double splitReadyRelevance = 0.0;
    double tieReadyRelevance = 0.0;
    std::string dominantIneligibleReason;
    double dominantIneligibleReasonConfidence = 0.0;
    std::size_t derivedFromBaseStateCount = 0U;
    std::size_t sameSemanticClassCount = 0U;
    std::size_t sameFinalStateCount = 0U;
    std::size_t sameTraceClassCount = 0U;
    std::size_t followupPatternPreservedCount = 0U;
    std::size_t structureOnlyEnrichmentCount = 0U;
    std::size_t lineageSampleCount = 0U;
};

struct PolicyGateFamilyResult {
    std::string family;
    std::string role;
    PolicyGateStatus status = PolicyGateStatus::INSUFFICIENT_EVIDENCE;
    std::vector<PolicyEvidenceSource> evidenceSources;
    PolicyFreshnessStatus freshnessStatus = PolicyFreshnessStatus::FRESH;
    PolicyGateThreshold threshold;
    PolicyGateMeasuredEvidence measured;
    std::string rationale;
    std::string freshnessRationale;
    bool driftFlag = false;
    bool reclassifyRequired = false;
    bool countsAsProductionEvidence = false;
    std::string sourceSummaryPath;
    PolicyRelevantInputHashes relevantInputHashes;
};

struct PolicyGateManifest {
    std::string manifestVersion = "policy_graduation_manifest_v1";
    std::string reportVersion = "phase22";
    std::string manifestRole = "current";
    std::string baselineVersion;
    std::string promotedFromReport;
    std::string promotedFromManifest;
    std::string baselineTag;
    std::string approvalTimestampUtc;
    bool provenanceFrozen = false;
    std::string buildTag;
    std::string timestampUtc;
    std::string artifactRoot;
    std::string baselineManifestPath;
    std::string baselineManifestHash;
    std::string currentManifestHash;
    std::size_t freshFamilyCount = 0U;
    std::size_t staleFamilyCount = 0U;
    std::size_t requiresRerunFamilyCount = 0U;
    std::size_t reclassifyRequiredCount = 0U;
    std::size_t revalidatedFamilyCount = 0U;
    std::vector<std::string> staleFamilies;
    std::vector<std::string> reclassifyRequiredFamilies;
    std::vector<PolicyGateFamilyResult> families;
};

const char* policy_gate_status_name(PolicyGateStatus status);
bool policy_gate_status_is_satisfied(PolicyGateStatus status);
const char* policy_evidence_source_name(PolicyEvidenceSource source);
const char* policy_freshness_status_name(PolicyFreshnessStatus status);
bool policy_freshness_status_is_satisfied(PolicyFreshnessStatus status);

PolicyGateFamilyResult evaluate_production_compare_gate(
    const std::string& family,
    const PolicyGateCompareEvidence& compareEvidence,
    std::size_t comparedStateCountMin
);

PolicyGateFamilyResult evaluate_planner_tie_non_applicable_gate(
    const FamilyApplicabilitySummary& applicability,
    const PolicyGateCompareEvidence& compareEvidence
);

PolicyGateFamilyResult evaluate_compare_ready_diagnostic_gate(
    const PolicyGateCompareEvidence& compareEvidence,
    const CompareReadyLineageSummary& lineage
);

PolicyGateCompareEvidence load_policy_gate_compare_evidence(const std::filesystem::path& summaryPath);
PolicyGateManifest build_policy_gate_manifest(
    const std::filesystem::path& artifactRoot,
    const std::string& buildTag
);
PolicyGateManifest filter_policy_gate_manifest(
    const PolicyGateManifest& manifest,
    const std::optional<std::string>& familyFilter
);
bool policy_gate_manifest_satisfied(
    const PolicyGateManifest& manifest,
    const std::optional<std::string>& familyFilter = std::nullopt
);

std::string policy_gate_manifest_text(const PolicyGateManifest& manifest);
std::string policy_gate_manifest_json(const PolicyGateManifest& manifest);
std::string policy_gate_short_summary(
    const PolicyGateManifest& manifest,
    const std::optional<std::string>& familyFilter = std::nullopt
);

void write_policy_gate_outputs(const std::filesystem::path& jsonPath, const PolicyGateManifest& manifest);
PolicyGateManifest load_policy_gate_manifest_text(const std::filesystem::path& manifestPath);
