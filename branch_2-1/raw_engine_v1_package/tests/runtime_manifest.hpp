#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "policy_runtime_budget.hpp"

enum class RuntimeCurrentStatus : unsigned char {
    PASS = 0,
    BUDGET_WARN = 1,
    BUDGET_FAIL = 2,
    INFO_ONLY = 3,
};

enum class RuntimeFreshnessStatus : unsigned char {
    FRESH = 0,
    STALE = 1,
    REQUIRES_RERUN = 2,
};

enum class RuntimeComparabilityStatus : unsigned char {
    COMPARABLE = 0,
    NOT_COMPARABLE = 1,
    REBASELINE_REQUIRED = 2,
    INFO_ONLY = 3,
};

struct RuntimeHostFingerprint {
    std::string osName;
    std::string cpuArch;
    std::string runnerTag;
    std::string combinedHash;
};

struct RuntimeToolchainFingerprint {
    std::string compilerId;
    std::string compilerVersion;
    std::string buildType;
    std::string sanitizerMode;
    std::string sanitizerFlags;
    std::string combinedHash;
};

struct RuntimeManifestEntry {
    std::string executionClass;
    double wallTimeSec = 0.0;
    std::size_t testCount = 0U;
    PolicyRuntimeBudgetThreshold threshold;
    RuntimeHostFingerprint hostFingerprint;
    RuntimeToolchainFingerprint toolchainFingerprint;
    double baselineWallTimeSec = 0.0;
    double deltaPercent = 0.0;
    RuntimeCurrentStatus currentStatus = RuntimeCurrentStatus::PASS;
    RuntimeFreshnessStatus freshnessStatus = RuntimeFreshnessStatus::FRESH;
    RuntimeComparabilityStatus comparabilityStatus = RuntimeComparabilityStatus::COMPARABLE;
    std::string rationale;
};

struct RuntimeManifest {
    std::string manifestVersion = "runtime_lifecycle_manifest_v1";
    std::string reportVersion = "phase23";
    std::string manifestRole = "current";
    std::string generatedAtUtc;
    std::string artifactRoot;
    std::string sourceManifestPath;
    std::string promotedFromManifest;
    std::string baselineManifestPath;
    std::string baselineManifestHash;
    std::string currentManifestHash;
    std::string baselineTag;
    std::string approvalTimestampUtc;
    std::string budgetVerdict = "PASS";
    std::string freshnessVerdict = "FRESH";
    std::string comparabilityVerdict = "COMPARABLE";
    std::size_t freshEntryCount = 0U;
    std::size_t staleEntryCount = 0U;
    std::size_t requiresRerunEntryCount = 0U;
    std::size_t rebaselineRequiredCount = 0U;
    std::size_t notComparableCount = 0U;
    std::size_t infoOnlyCount = 0U;
    std::size_t warnCount = 0U;
    std::size_t failCount = 0U;
    std::vector<std::string> staleExecutionClasses;
    std::vector<std::string> requiresRerunExecutionClasses;
    std::vector<std::string> rebaselineRequiredExecutionClasses;
    std::vector<RuntimeManifestEntry> entries;
};

struct RuntimeBaselinePromotionOptions {
    std::string baselineTag;
    bool requireAcceptableStatus = false;
};

struct RuntimeRerunPlanEntry {
    std::string executionClass;
    std::string currentStatus;
    std::string freshnessStatus;
    std::string comparabilityStatus;
    std::string rerunKind;
    std::string recommendedCommand;
    std::string expectedStopCriteria;
    std::string statusImpact;
};

struct RuntimeRerunPlan {
    std::string planVersion = "runtime_rerun_plan_v1";
    std::string generatedAtUtc;
    std::string artifactRoot;
    std::string baselineManifestPath;
    std::string currentManifestPath;
    std::string refreshManifestPath;
    std::string baselineManifestHash;
    std::string currentManifestHash;
    std::string refreshManifestHash;
    std::size_t staleEntryCount = 0U;
    std::size_t requiresRerunEntryCount = 0U;
    std::size_t rebaselineRequiredCount = 0U;
    std::size_t selectedEntryCount = 0U;
    std::vector<std::string> staleExecutionClasses;
    std::vector<std::string> requiresRerunExecutionClasses;
    std::vector<std::string> rebaselineRequiredExecutionClasses;
    std::vector<RuntimeRerunPlanEntry> entries;
    std::string summaryVerdict = "PASS";
    std::string rationale;
};

const char* runtime_current_status_name(RuntimeCurrentStatus status);
const char* runtime_freshness_status_name(RuntimeFreshnessStatus status);
const char* runtime_comparability_status_name(RuntimeComparabilityStatus status);

bool runtime_manifest_acceptable_for_baseline(const RuntimeManifest& manifest);

RuntimeHostFingerprint make_runtime_host_fingerprint(const std::string& runnerTag = {});
RuntimeToolchainFingerprint make_runtime_toolchain_fingerprint(
    const std::string& buildType,
    const std::string& sanitizerMode,
    const std::string& sanitizerFlags,
    const std::string& compilerId = "unknown",
    const std::string& compilerVersion = "unknown"
);

RuntimeManifestEntry make_runtime_manifest_entry(
    const std::string& executionClass,
    double wallTimeSec,
    std::size_t testCount,
    const std::string& buildType,
    const std::string& sanitizerMode,
    const std::string& sanitizerFlags,
    const PolicyRuntimeBudgetThreshold& threshold,
    const std::string& runnerTag = {},
    const std::string& compilerId = "unknown",
    const std::string& compilerVersion = "unknown"
);

RuntimeManifest build_runtime_current_manifest(
    const std::string& artifactRoot,
    const std::vector<RuntimeManifestEntry>& entries,
    const std::string& reportVersion = "phase23"
);

RuntimeManifest promote_runtime_baseline(
    const RuntimeManifest& currentManifest,
    const std::filesystem::path& sourceManifestPath,
    const std::filesystem::path& baselineOutputPath,
    const RuntimeBaselinePromotionOptions& options
);

RuntimeManifest refresh_runtime_manifest(
    const RuntimeManifest& baselineManifest,
    const RuntimeManifest& currentManifest,
    const std::filesystem::path& baselineManifestPath,
    const std::filesystem::path& currentManifestPath
);

RuntimeRerunPlan build_runtime_rerun_plan(
    const RuntimeManifest& refreshManifest,
    const std::filesystem::path& baselineManifestPath,
    const std::filesystem::path& currentManifestPath,
    const std::filesystem::path& refreshManifestPath
);

void recompute_runtime_manifest_rollup(RuntimeManifest& manifest);

std::string runtime_manifest_text(const RuntimeManifest& manifest);
std::string runtime_manifest_json(const RuntimeManifest& manifest);
std::string runtime_manifest_summary(const RuntimeManifest& manifest);
void write_runtime_manifest_outputs(const std::filesystem::path& jsonPath, const RuntimeManifest& manifest);
RuntimeManifest load_runtime_manifest_text(const std::filesystem::path& manifestPath);

std::string runtime_rerun_plan_text(const RuntimeRerunPlan& plan);
std::string runtime_rerun_plan_json(const RuntimeRerunPlan& plan);
std::string runtime_rerun_plan_summary(const RuntimeRerunPlan& plan);
void write_runtime_rerun_plan_outputs(const std::filesystem::path& jsonPath, const RuntimeRerunPlan& plan);
RuntimeRerunPlan load_runtime_rerun_plan_text(const std::filesystem::path& planPath);
