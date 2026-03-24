#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "policy_baseline.hpp"
#include "policy_gate.hpp"

enum class PolicyPipelineSeverity : unsigned char {
    OK = 0,
    WARN = 1,
    ACTION_REQUIRED = 2,
    FAIL = 3,
};

enum class PolicyRuntimeBudgetStatus : unsigned char {
    OK = 0,
    WARN = 1,
    FAIL = 2,
};

struct PolicyRuntimeBudgetThreshold {
    double softSeconds = 0.0;
    double hardSeconds = 0.0;
    double softDeltaPercent = 0.0;
    double hardDeltaPercent = 0.0;
};

struct PolicyRuntimeBudgetEntry {
    std::string name;
    double currentSeconds = 0.0;
    double baselineSeconds = 0.0;
    double deltaPercent = 0.0;
    PolicyRuntimeBudgetThreshold threshold;
    PolicyRuntimeBudgetStatus status = PolicyRuntimeBudgetStatus::OK;
    std::string rationale;
};

struct PolicyRuntimeBudgetManifest {
    std::string manifestVersion = "policy_runtime_budget_v1";
    std::string reportVersion = "phase22";
    std::string generatedAtUtc;
    std::string artifactRoot;
    std::string baselineRuntimeManifestPath;
    std::string baselineRuntimeManifestHash;
    std::string currentRuntimeManifestHash;
    std::vector<PolicyRuntimeBudgetEntry> entries;
    std::size_t warnCount = 0U;
    std::size_t failCount = 0U;
    std::string budgetVerdict = "PASS";
};

const char* policy_pipeline_severity_name(PolicyPipelineSeverity severity);
int policy_pipeline_exit_code(PolicyPipelineSeverity severity, bool strictWarnExit = true);

PolicyPipelineSeverity evaluate_policy_pipeline_severity(
    const PolicyGateManifest& currentManifest,
    const PolicyGateManifest& refreshManifest,
    const PolicyRerunPlan& plan
);

std::string policy_pipeline_action_recommendation(
    PolicyPipelineSeverity severity,
    const PolicyGateManifest& currentManifest,
    const PolicyGateManifest& refreshManifest,
    const PolicyRerunPlan& plan
);

const char* policy_runtime_budget_status_name(PolicyRuntimeBudgetStatus status);
PolicyRuntimeBudgetThreshold default_policy_runtime_budget_threshold(const std::string& entryName);

PolicyRuntimeBudgetEntry evaluate_policy_runtime_budget_entry(
    const std::string& entryName,
    double currentSeconds,
    double baselineSeconds,
    const PolicyRuntimeBudgetThreshold& threshold
);

PolicyRuntimeBudgetManifest evaluate_policy_runtime_budget_manifest(
    const std::string& artifactRoot,
    const std::vector<PolicyRuntimeBudgetEntry>& entries,
    const std::string& baselineRuntimeManifestPath = {},
    const std::string& baselineRuntimeManifestHash = {}
);
PolicyRuntimeBudgetManifest evaluate_policy_runtime_budget_manifest(
    const std::string& artifactRoot,
    const std::filesystem::path& baselineRuntimeManifestPath,
    const std::vector<PolicyRuntimeBudgetEntry>& entries
);

void recompute_policy_runtime_budget_rollup(PolicyRuntimeBudgetManifest& manifest);

std::string policy_runtime_budget_manifest_text(const PolicyRuntimeBudgetManifest& manifest);
std::string policy_runtime_budget_manifest_json(const PolicyRuntimeBudgetManifest& manifest);
std::string policy_runtime_budget_summary(const PolicyRuntimeBudgetManifest& manifest);

void write_policy_runtime_budget_outputs(
    const std::filesystem::path& jsonPath,
    const PolicyRuntimeBudgetManifest& manifest
);

PolicyRuntimeBudgetManifest load_policy_runtime_budget_manifest_text(const std::filesystem::path& manifestPath);
PolicyRuntimeBudgetManifest load_policy_runtime_budget_text(const std::filesystem::path& manifestPath);
