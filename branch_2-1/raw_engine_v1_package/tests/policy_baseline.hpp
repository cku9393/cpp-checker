#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "policy_gate.hpp"

struct PolicyBaselinePromotionOptions {
    std::string baselineTag;
    bool requireAcceptableStatus = false;
    bool freezeProvenance = false;
};

struct PolicySyntheticMutationOptions {
    std::string hashDriftField;
    std::string applicabilityDriftFamily;
    std::string diagnosticPromotionFamily;
};

struct PolicyRerunPlanEntry {
    std::string family;
    std::string currentStatus;
    std::string freshnessStatus;
    std::string rerunKind;
    std::string evidenceMode;
    std::string campaignConfigPath;
    std::string recommendedCommand;
    std::string expectedStopCriteria;
    std::string statusImpact;
    bool countsAsProductionEvidence = false;
};

struct PolicyRerunPlan {
    std::string planVersion = "policy_rerun_plan_v1";
    std::string generatedAtUtc;
    std::string artifactRoot;
    std::string baselineManifestPath;
    std::string currentManifestPath;
    std::string refreshManifestPath;
    std::string baselineManifestHash;
    std::string currentManifestHash;
    std::string refreshManifestHash;
    std::size_t staleFamilyCount = 0U;
    std::size_t requiresRerunFamilyCount = 0U;
    std::size_t reclassifyRequiredCount = 0U;
    std::size_t selectedEntryCount = 0U;
    std::vector<std::string> staleFamilies;
    std::vector<std::string> requiresRerunFamilies;
    std::vector<std::string> omittedFamilies;
    std::vector<PolicyRerunPlanEntry> entries;
    std::string summaryVerdict = "PASS";
    std::string rationale;
};

struct PolicyRerunPlanOptions {
    std::vector<std::string> familyFilter;
    bool includeDiagnostic = false;
    bool includeNonApplicable = false;
};

struct PolicyRerunExecutionSummary {
    std::string executedAtUtc;
    std::string artifactRoot;
    std::size_t selectedFamilyCount = 0U;
    std::size_t executedFamilyCount = 0U;
    std::size_t noopFamilyCount = 0U;
    std::vector<std::string> executedFamilies;
    std::vector<std::string> noopFamilies;
    std::vector<std::string> failedFamilies;
    std::string summaryVerdict = "PASS";
    std::string rationale;
};

bool policy_gate_status_acceptable_for_baseline(PolicyGateStatus status);
bool policy_manifest_acceptable_for_baseline(const PolicyGateManifest& manifest);

PolicyGateManifest promote_policy_gate_baseline(
    const PolicyGateManifest& sourceManifest,
    const std::filesystem::path& sourceRoot,
    const std::filesystem::path& sourceManifestPath,
    const std::filesystem::path& baselineOutputPath,
    const PolicyBaselinePromotionOptions& options
);

PolicyRerunPlan build_policy_rerun_plan(
    const PolicyGateManifest& refreshManifest,
    const std::filesystem::path& baselineManifestPath,
    const std::filesystem::path& currentManifestPath,
    const std::filesystem::path& refreshManifestPath,
    const PolicyRerunPlanOptions& options
);

void apply_policy_synthetic_mutations(
    PolicyGateManifest& manifest,
    const PolicySyntheticMutationOptions& options
);

std::string policy_rerun_plan_text(const PolicyRerunPlan& plan);
std::string policy_rerun_plan_json(const PolicyRerunPlan& plan);
std::string policy_rerun_plan_summary(const PolicyRerunPlan& plan);
void write_policy_rerun_plan_outputs(const std::filesystem::path& jsonPath, const PolicyRerunPlan& plan);
PolicyRerunPlan load_policy_rerun_plan_text(const std::filesystem::path& planPath);

std::string policy_rerun_execution_summary_text(const PolicyRerunExecutionSummary& summary);
void write_policy_baseline_outputs_with_history(const std::filesystem::path& jsonPath, const PolicyGateManifest& baseline);
