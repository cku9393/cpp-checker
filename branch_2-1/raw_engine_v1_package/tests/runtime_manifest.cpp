#include "runtime_manifest.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "failure_signature.hpp"

using namespace std;

namespace {

string timestamp_utc_now() {
    const auto now = chrono::system_clock::now();
    const time_t nowTime = chrono::system_clock::to_time_t(now);
    tm tmUtc{};
#if defined(_WIN32)
    gmtime_s(&tmUtc, &nowTime);
#else
    gmtime_r(&nowTime, &tmUtc);
#endif
    ostringstream oss;
    oss << put_time(&tmUtc, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

string trim_copy(string value) {
    const auto isNotSpace = [](unsigned char ch) { return !isspace(ch); };
    value.erase(value.begin(), find_if(value.begin(), value.end(), isNotSpace));
    value.erase(find_if(value.rbegin(), value.rend(), isNotSpace).base(), value.end());
    return value;
}

string json_escape(const string& value) {
    ostringstream oss;
    for (const char ch : value) {
        switch (ch) {
            case '\\': oss << "\\\\"; break;
            case '"': oss << "\\\""; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default: oss << ch; break;
        }
    }
    return oss.str();
}

string json_number(double value) {
    ostringstream oss;
    oss << fixed << setprecision(3) << value;
    return oss.str();
}

void append_unique(vector<string>& values, const string& value) {
    if (!value.empty() && find(values.begin(), values.end(), value) == values.end()) {
        values.push_back(value);
    }
}

RuntimeCurrentStatus parse_runtime_current_status(const string& value) {
    if (value == "PASS") {
        return RuntimeCurrentStatus::PASS;
    }
    if (value == "BUDGET_WARN") {
        return RuntimeCurrentStatus::BUDGET_WARN;
    }
    if (value == "BUDGET_FAIL") {
        return RuntimeCurrentStatus::BUDGET_FAIL;
    }
    if (value == "INFO_ONLY") {
        return RuntimeCurrentStatus::INFO_ONLY;
    }
    throw runtime_error("unknown runtime current status: " + value);
}

RuntimeFreshnessStatus parse_runtime_freshness_status(const string& value) {
    if (value == "FRESH") {
        return RuntimeFreshnessStatus::FRESH;
    }
    if (value == "STALE") {
        return RuntimeFreshnessStatus::STALE;
    }
    if (value == "REQUIRES_RERUN") {
        return RuntimeFreshnessStatus::REQUIRES_RERUN;
    }
    throw runtime_error("unknown runtime freshness status: " + value);
}

RuntimeComparabilityStatus parse_runtime_comparability_status(const string& value) {
    if (value == "COMPARABLE") {
        return RuntimeComparabilityStatus::COMPARABLE;
    }
    if (value == "NOT_COMPARABLE") {
        return RuntimeComparabilityStatus::NOT_COMPARABLE;
    }
    if (value == "REBASELINE_REQUIRED") {
        return RuntimeComparabilityStatus::REBASELINE_REQUIRED;
    }
    if (value == "INFO_ONLY") {
        return RuntimeComparabilityStatus::INFO_ONLY;
    }
    throw runtime_error("unknown runtime comparability status: " + value);
}

RuntimeCurrentStatus evaluate_budget_status(
    double currentSeconds,
    double baselineSeconds,
    const PolicyRuntimeBudgetThreshold& threshold
) {
    const double deltaPercent =
        baselineSeconds > 0.0 ? ((currentSeconds - baselineSeconds) / baselineSeconds) * 100.0 : 0.0;
    const bool hardSecondsExceeded = threshold.hardSeconds > 0.0 && currentSeconds > threshold.hardSeconds;
    const bool softSecondsExceeded = threshold.softSeconds > 0.0 && currentSeconds > threshold.softSeconds;
    const bool hardDeltaExceeded = baselineSeconds > 0.0 && threshold.hardDeltaPercent > 0.0 &&
                                   deltaPercent > threshold.hardDeltaPercent;
    const bool softDeltaExceeded = baselineSeconds > 0.0 && threshold.softDeltaPercent > 0.0 &&
                                   deltaPercent > threshold.softDeltaPercent;
    if (hardSecondsExceeded || hardDeltaExceeded) {
        return RuntimeCurrentStatus::BUDGET_FAIL;
    }
    if (softSecondsExceeded || softDeltaExceeded) {
        return RuntimeCurrentStatus::BUDGET_WARN;
    }
    return RuntimeCurrentStatus::PASS;
}

string recommended_runtime_rerun_command(const string& executionClass) {
    if (executionClass == "release_full") {
        return "ctest --test-dir build-release --output-on-failure";
    }
    if (executionClass == "debug_full") {
        return "ctest --test-dir build-debug --output-on-failure";
    }
    if (executionClass == "asan_full") {
        return "ctest --test-dir build-asan-isolated --output-on-failure";
    }
    if (executionClass == "policy_core" || executionClass == "policy_refresh") {
        return "python tests/tools/run_policy_pipeline.py --mode quick --strict";
    }
    if (executionClass == "policy_nightly") {
        return "python tests/tools/run_policy_pipeline.py --mode nightly --strict";
    }
    if (executionClass.find("compare") != string::npos) {
        return "./raw_engine_tests --case campaign --stop-when-gate-passes";
    }
    return "re-run the corresponding execution class on the current host/toolchain";
}

string runtime_rerun_kind(const string& executionClass) {
    if (executionClass == "release_full") {
        return "release_full_rerun";
    }
    if (executionClass == "debug_full") {
        return "debug_full_rerun";
    }
    if (executionClass == "asan_full") {
        return "asan_full_rerun";
    }
    if (executionClass == "policy_core") {
        return "policy_core_rerun";
    }
    if (executionClass == "policy_refresh") {
        return "policy_refresh_rerun";
    }
    if (executionClass == "policy_nightly") {
        return "policy_nightly_rerun";
    }
    if (executionClass.find("compare") != string::npos) {
        return "compare_campaign_rerun";
    }
    return "manual_runtime_rerun";
}

void recompute_runtime_rerun_plan_rollup(RuntimeRerunPlan& plan) {
    plan.staleEntryCount = 0U;
    plan.requiresRerunEntryCount = 0U;
    plan.rebaselineRequiredCount = 0U;
    plan.selectedEntryCount = plan.entries.size();
    plan.staleExecutionClasses.clear();
    plan.requiresRerunExecutionClasses.clear();
    plan.rebaselineRequiredExecutionClasses.clear();
    for (const RuntimeRerunPlanEntry& entry : plan.entries) {
        if (entry.freshnessStatus == "STALE") {
            ++plan.staleEntryCount;
            append_unique(plan.staleExecutionClasses, entry.executionClass);
        }
        if (entry.freshnessStatus == "REQUIRES_RERUN") {
            ++plan.requiresRerunEntryCount;
            append_unique(plan.requiresRerunExecutionClasses, entry.executionClass);
        }
        if (entry.comparabilityStatus == "REBASELINE_REQUIRED") {
            ++plan.rebaselineRequiredCount;
            append_unique(plan.rebaselineRequiredExecutionClasses, entry.executionClass);
        }
    }
    if (plan.entries.empty()) {
        plan.summaryVerdict = "PASS";
        plan.rationale = "no stale or over-budget runtime execution classes detected";
    } else if (plan.rebaselineRequiredCount != 0U) {
        plan.summaryVerdict = "ACTION_REQUIRED";
        plan.rationale = "runtime baseline refresh requires a comparable host/toolchain or explicit rebaseline";
    } else {
        plan.summaryVerdict = "ACTION_REQUIRED";
        plan.rationale = "rerun required for stale or over-budget runtime execution classes";
    }
}

} // namespace

const char* runtime_current_status_name(RuntimeCurrentStatus status) {
    switch (status) {
        case RuntimeCurrentStatus::PASS: return "PASS";
        case RuntimeCurrentStatus::BUDGET_WARN: return "BUDGET_WARN";
        case RuntimeCurrentStatus::BUDGET_FAIL: return "BUDGET_FAIL";
        case RuntimeCurrentStatus::INFO_ONLY: return "INFO_ONLY";
    }
    return "INFO_ONLY";
}

const char* runtime_freshness_status_name(RuntimeFreshnessStatus status) {
    switch (status) {
        case RuntimeFreshnessStatus::FRESH: return "FRESH";
        case RuntimeFreshnessStatus::STALE: return "STALE";
        case RuntimeFreshnessStatus::REQUIRES_RERUN: return "REQUIRES_RERUN";
    }
    return "REQUIRES_RERUN";
}

const char* runtime_comparability_status_name(RuntimeComparabilityStatus status) {
    switch (status) {
        case RuntimeComparabilityStatus::COMPARABLE: return "COMPARABLE";
        case RuntimeComparabilityStatus::NOT_COMPARABLE: return "NOT_COMPARABLE";
        case RuntimeComparabilityStatus::REBASELINE_REQUIRED: return "REBASELINE_REQUIRED";
        case RuntimeComparabilityStatus::INFO_ONLY: return "INFO_ONLY";
    }
    return "INFO_ONLY";
}

bool runtime_manifest_acceptable_for_baseline(const RuntimeManifest& manifest) {
    return none_of(manifest.entries.begin(), manifest.entries.end(), [](const RuntimeManifestEntry& entry) {
        return entry.currentStatus == RuntimeCurrentStatus::BUDGET_FAIL;
    });
}

RuntimeHostFingerprint make_runtime_host_fingerprint(const string& runnerTag) {
    RuntimeHostFingerprint fingerprint;
#if defined(__APPLE__)
    fingerprint.osName = "Darwin";
#elif defined(_WIN32)
    fingerprint.osName = "Windows";
#elif defined(__linux__)
    fingerprint.osName = "Linux";
#else
    fingerprint.osName = "Unknown";
#endif

#if defined(__aarch64__) || defined(__arm64__)
    fingerprint.cpuArch = "arm64";
#elif defined(__x86_64__) || defined(_M_X64)
    fingerprint.cpuArch = "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
    fingerprint.cpuArch = "x86";
#else
    fingerprint.cpuArch = "unknown";
#endif
    fingerprint.runnerTag = runnerTag;
    fingerprint.combinedHash =
        stable_hash_text(fingerprint.osName + "|" + fingerprint.cpuArch + "|" + fingerprint.runnerTag);
    return fingerprint;
}

RuntimeToolchainFingerprint make_runtime_toolchain_fingerprint(
    const string& buildType,
    const string& sanitizerMode,
    const string& sanitizerFlags,
    const string& compilerId,
    const string& compilerVersion
) {
    RuntimeToolchainFingerprint fingerprint;
    fingerprint.compilerId = compilerId;
    fingerprint.compilerVersion = compilerVersion;
    fingerprint.buildType = buildType;
    fingerprint.sanitizerMode = sanitizerMode;
    fingerprint.sanitizerFlags = sanitizerFlags;
    fingerprint.combinedHash = stable_hash_text(
        fingerprint.compilerId + "|" + fingerprint.compilerVersion + "|" + fingerprint.buildType + "|" +
        fingerprint.sanitizerMode + "|" + fingerprint.sanitizerFlags
    );
    return fingerprint;
}

RuntimeManifestEntry make_runtime_manifest_entry(
    const string& executionClass,
    double wallTimeSec,
    size_t testCount,
    const string& buildType,
    const string& sanitizerMode,
    const string& sanitizerFlags,
    const PolicyRuntimeBudgetThreshold& threshold,
    const string& runnerTag,
    const string& compilerId,
    const string& compilerVersion
) {
    RuntimeManifestEntry entry;
    entry.executionClass = executionClass;
    entry.wallTimeSec = wallTimeSec;
    entry.testCount = testCount;
    entry.threshold = threshold;
    entry.hostFingerprint = make_runtime_host_fingerprint(runnerTag);
    entry.toolchainFingerprint =
        make_runtime_toolchain_fingerprint(buildType, sanitizerMode, sanitizerFlags, compilerId, compilerVersion);
    entry.currentStatus = evaluate_budget_status(wallTimeSec, 0.0, threshold);
    entry.freshnessStatus = RuntimeFreshnessStatus::FRESH;
    entry.comparabilityStatus = RuntimeComparabilityStatus::COMPARABLE;
    entry.rationale =
        entry.currentStatus == RuntimeCurrentStatus::PASS
            ? "runtime current entry is within the configured absolute budget"
            : (entry.currentStatus == RuntimeCurrentStatus::BUDGET_WARN
                   ? "runtime current entry exceeded the soft absolute budget"
                   : "runtime current entry exceeded the hard absolute budget");
    return entry;
}

RuntimeManifest build_runtime_current_manifest(
    const string& artifactRoot,
    const vector<RuntimeManifestEntry>& entries,
    const string& reportVersion
) {
    RuntimeManifest manifest;
    manifest.reportVersion = reportVersion;
    manifest.manifestRole = "current";
    manifest.artifactRoot = artifactRoot;
    manifest.entries = entries;
    recompute_runtime_manifest_rollup(manifest);
    return manifest;
}

RuntimeManifest promote_runtime_baseline(
    const RuntimeManifest& currentManifest,
    const filesystem::path& sourceManifestPath,
    const filesystem::path& baselineOutputPath,
    const RuntimeBaselinePromotionOptions& options
) {
    if (options.requireAcceptableStatus && !runtime_manifest_acceptable_for_baseline(currentManifest)) {
        throw runtime_error("runtime baseline promotion rejected: current runtime manifest contains BUDGET_FAIL");
    }
    RuntimeManifest baseline = currentManifest;
    baseline.manifestRole = "baseline";
    baseline.sourceManifestPath = filesystem::absolute(sourceManifestPath).string();
    baseline.promotedFromManifest = filesystem::absolute(sourceManifestPath).string();
    baseline.baselineManifestPath = filesystem::absolute(baselineOutputPath).string();
    baseline.baselineTag = options.baselineTag;
    baseline.approvalTimestampUtc = timestamp_utc_now();
    recompute_runtime_manifest_rollup(baseline);
    return baseline;
}

RuntimeManifest refresh_runtime_manifest(
    const RuntimeManifest& baselineManifest,
    const RuntimeManifest& currentManifest,
    const filesystem::path& baselineManifestPath,
    const filesystem::path& currentManifestPath
) {
    unordered_map<string, RuntimeManifestEntry> baselineEntries;
    for (const RuntimeManifestEntry& entry : baselineManifest.entries) {
        baselineEntries[entry.executionClass] = entry;
    }

    RuntimeManifest refresh = currentManifest;
    refresh.manifestRole = "refresh";
    refresh.baselineManifestPath = filesystem::absolute(baselineManifestPath).string();
    refresh.baselineManifestHash = stable_hash_text(runtime_manifest_text(baselineManifest));
    refresh.sourceManifestPath = filesystem::absolute(currentManifestPath).string();
    refresh.promotedFromManifest = baselineManifest.promotedFromManifest;
    refresh.baselineTag = baselineManifest.baselineTag;
    refresh.approvalTimestampUtc = baselineManifest.approvalTimestampUtc;

    for (RuntimeManifestEntry& entry : refresh.entries) {
        const auto baselineIt = baselineEntries.find(entry.executionClass);
        if (baselineIt == baselineEntries.end()) {
            entry.currentStatus = RuntimeCurrentStatus::INFO_ONLY;
            entry.freshnessStatus = RuntimeFreshnessStatus::REQUIRES_RERUN;
            entry.comparabilityStatus = RuntimeComparabilityStatus::REBASELINE_REQUIRED;
            entry.rationale = "runtime baseline entry missing for execution class";
            continue;
        }

        const RuntimeManifestEntry& baselineEntry = baselineIt->second;
        entry.baselineWallTimeSec = baselineEntry.wallTimeSec;
        entry.deltaPercent = baselineEntry.wallTimeSec > 0.0
                                 ? ((entry.wallTimeSec - baselineEntry.wallTimeSec) / baselineEntry.wallTimeSec) * 100.0
                                 : 0.0;

        if (entry.hostFingerprint.combinedHash != baselineEntry.hostFingerprint.combinedHash) {
            entry.currentStatus = RuntimeCurrentStatus::INFO_ONLY;
            entry.freshnessStatus = RuntimeFreshnessStatus::REQUIRES_RERUN;
            entry.comparabilityStatus = RuntimeComparabilityStatus::REBASELINE_REQUIRED;
            entry.rationale = "host fingerprint mismatch prevents strict wall-time comparison";
            continue;
        }
        if (entry.toolchainFingerprint.combinedHash != baselineEntry.toolchainFingerprint.combinedHash) {
            entry.currentStatus = RuntimeCurrentStatus::INFO_ONLY;
            entry.freshnessStatus = entry.executionClass.find("compare") != string::npos
                                        ? RuntimeFreshnessStatus::FRESH
                                        : RuntimeFreshnessStatus::REQUIRES_RERUN;
            entry.comparabilityStatus = entry.executionClass.find("compare") != string::npos
                                            ? RuntimeComparabilityStatus::INFO_ONLY
                                            : RuntimeComparabilityStatus::NOT_COMPARABLE;
            entry.rationale = "toolchain fingerprint mismatch prevents strict wall-time comparison";
            continue;
        }

        entry.comparabilityStatus = RuntimeComparabilityStatus::COMPARABLE;
        entry.currentStatus =
            evaluate_budget_status(entry.wallTimeSec, baselineEntry.wallTimeSec, entry.threshold);
        if (entry.currentStatus == RuntimeCurrentStatus::BUDGET_FAIL) {
            entry.freshnessStatus = RuntimeFreshnessStatus::REQUIRES_RERUN;
            entry.rationale = "runtime exceeded hard budget relative to comparable baseline";
        } else if (entry.currentStatus == RuntimeCurrentStatus::BUDGET_WARN) {
            entry.freshnessStatus = RuntimeFreshnessStatus::STALE;
            entry.rationale = "runtime exceeded soft budget relative to comparable baseline";
        } else {
            entry.freshnessStatus = RuntimeFreshnessStatus::FRESH;
            entry.rationale = "runtime remained within the comparable baseline budget";
        }
    }

    recompute_runtime_manifest_rollup(refresh);
    return refresh;
}

RuntimeRerunPlan build_runtime_rerun_plan(
    const RuntimeManifest& refreshManifest,
    const filesystem::path& baselineManifestPath,
    const filesystem::path& currentManifestPath,
    const filesystem::path& refreshManifestPath
) {
    RuntimeRerunPlan plan;
    plan.generatedAtUtc = timestamp_utc_now();
    plan.artifactRoot = refreshManifest.artifactRoot;
    plan.baselineManifestPath = filesystem::absolute(baselineManifestPath).string();
    plan.currentManifestPath = filesystem::absolute(currentManifestPath).string();
    plan.refreshManifestPath = refreshManifestPath.empty() ? string() : filesystem::absolute(refreshManifestPath).string();
    plan.baselineManifestHash = stable_hash_text(runtime_manifest_text(load_runtime_manifest_text(baselineManifestPath)));
    plan.currentManifestHash = stable_hash_text(runtime_manifest_text(load_runtime_manifest_text(currentManifestPath)));
    plan.refreshManifestHash = stable_hash_text(runtime_manifest_text(refreshManifest));

    for (const RuntimeManifestEntry& entry : refreshManifest.entries) {
        const bool stale = entry.freshnessStatus == RuntimeFreshnessStatus::STALE;
        const bool requiresRerun = entry.freshnessStatus == RuntimeFreshnessStatus::REQUIRES_RERUN;
        const bool rebaseline = entry.comparabilityStatus == RuntimeComparabilityStatus::REBASELINE_REQUIRED;
        if (!stale && !requiresRerun && !rebaseline) {
            continue;
        }
        RuntimeRerunPlanEntry planEntry;
        planEntry.executionClass = entry.executionClass;
        planEntry.currentStatus = runtime_current_status_name(entry.currentStatus);
        planEntry.freshnessStatus = runtime_freshness_status_name(entry.freshnessStatus);
        planEntry.comparabilityStatus = runtime_comparability_status_name(entry.comparabilityStatus);
        planEntry.rerunKind = runtime_rerun_kind(entry.executionClass);
        planEntry.recommendedCommand = recommended_runtime_rerun_command(entry.executionClass);
        if (rebaseline) {
            planEntry.expectedStopCriteria = "same host/toolchain fingerprint or explicit runtime rebaseline approval";
            planEntry.statusImpact = "runtime entry requires rebaseline before strict comparison";
        } else if (requiresRerun) {
            planEntry.expectedStopCriteria = "runtime entry must return within the hard budget";
            planEntry.statusImpact = "runtime entry requires rerun";
        } else {
            planEntry.expectedStopCriteria = "runtime entry should return within the soft budget";
            planEntry.statusImpact = "runtime entry is stale";
        }
        plan.entries.push_back(planEntry);
    }

    recompute_runtime_rerun_plan_rollup(plan);
    return plan;
}

void recompute_runtime_manifest_rollup(RuntimeManifest& manifest) {
    manifest.generatedAtUtc = timestamp_utc_now();
    manifest.freshEntryCount = 0U;
    manifest.staleEntryCount = 0U;
    manifest.requiresRerunEntryCount = 0U;
    manifest.rebaselineRequiredCount = 0U;
    manifest.notComparableCount = 0U;
    manifest.infoOnlyCount = 0U;
    manifest.warnCount = 0U;
    manifest.failCount = 0U;
    manifest.staleExecutionClasses.clear();
    manifest.requiresRerunExecutionClasses.clear();
    manifest.rebaselineRequiredExecutionClasses.clear();

    for (const RuntimeManifestEntry& entry : manifest.entries) {
        if (entry.currentStatus == RuntimeCurrentStatus::BUDGET_WARN) {
            ++manifest.warnCount;
        } else if (entry.currentStatus == RuntimeCurrentStatus::BUDGET_FAIL) {
            ++manifest.failCount;
        }

        if (entry.freshnessStatus == RuntimeFreshnessStatus::FRESH) {
            ++manifest.freshEntryCount;
        } else if (entry.freshnessStatus == RuntimeFreshnessStatus::STALE) {
            ++manifest.staleEntryCount;
            append_unique(manifest.staleExecutionClasses, entry.executionClass);
        } else if (entry.freshnessStatus == RuntimeFreshnessStatus::REQUIRES_RERUN) {
            ++manifest.requiresRerunEntryCount;
            append_unique(manifest.requiresRerunExecutionClasses, entry.executionClass);
        }

        if (entry.comparabilityStatus == RuntimeComparabilityStatus::REBASELINE_REQUIRED) {
            ++manifest.rebaselineRequiredCount;
            append_unique(manifest.rebaselineRequiredExecutionClasses, entry.executionClass);
        } else if (entry.comparabilityStatus == RuntimeComparabilityStatus::NOT_COMPARABLE) {
            ++manifest.notComparableCount;
        } else if (entry.comparabilityStatus == RuntimeComparabilityStatus::INFO_ONLY) {
            ++manifest.infoOnlyCount;
        }
    }

    manifest.budgetVerdict =
        manifest.failCount != 0U ? "BUDGET_FAIL" : (manifest.warnCount != 0U ? "BUDGET_WARN" : "PASS");
    manifest.freshnessVerdict =
        manifest.requiresRerunEntryCount != 0U
            ? "REQUIRES_RERUN"
            : (manifest.staleEntryCount != 0U ? "STALE" : "FRESH");
    manifest.comparabilityVerdict =
        manifest.rebaselineRequiredCount != 0U
            ? "REBASELINE_REQUIRED"
            : (manifest.notComparableCount != 0U
                   ? "NOT_COMPARABLE"
                   : (manifest.infoOnlyCount != 0U ? "INFO_ONLY" : "COMPARABLE"));
    manifest.currentManifestHash = stable_hash_text(runtime_manifest_text(manifest));
}

string runtime_manifest_text(const RuntimeManifest& manifest) {
    ostringstream oss;
    oss << "manifest_version=" << manifest.manifestVersion << '\n';
    oss << "report_version=" << manifest.reportVersion << '\n';
    oss << "manifest_role=" << manifest.manifestRole << '\n';
    oss << "generated_at_utc=" << manifest.generatedAtUtc << '\n';
    oss << "artifact_root=" << manifest.artifactRoot << '\n';
    oss << "source_manifest_path=" << manifest.sourceManifestPath << '\n';
    oss << "promoted_from_manifest=" << manifest.promotedFromManifest << '\n';
    oss << "baseline_manifest_path=" << manifest.baselineManifestPath << '\n';
    oss << "baseline_manifest_hash=" << manifest.baselineManifestHash << '\n';
    oss << "current_manifest_hash=" << manifest.currentManifestHash << '\n';
    oss << "baseline_tag=" << manifest.baselineTag << '\n';
    oss << "approval_timestamp_utc=" << manifest.approvalTimestampUtc << '\n';
    oss << "budget_verdict=" << manifest.budgetVerdict << '\n';
    oss << "freshness_verdict=" << manifest.freshnessVerdict << '\n';
    oss << "comparability_verdict=" << manifest.comparabilityVerdict << '\n';
    oss << "fresh_entry_count=" << manifest.freshEntryCount << '\n';
    oss << "stale_entry_count=" << manifest.staleEntryCount << '\n';
    oss << "requires_rerun_entry_count=" << manifest.requiresRerunEntryCount << '\n';
    oss << "rebaseline_required_count=" << manifest.rebaselineRequiredCount << '\n';
    oss << "not_comparable_count=" << manifest.notComparableCount << '\n';
    oss << "info_only_count=" << manifest.infoOnlyCount << '\n';
    oss << "warn_count=" << manifest.warnCount << '\n';
    oss << "fail_count=" << manifest.failCount << '\n';
    oss << "entry_count=" << manifest.entries.size() << '\n';
    for (const RuntimeManifestEntry& entry : manifest.entries) {
        oss << '\n';
        oss << "entry=" << entry.executionClass << '\n';
        oss << "wall_time_sec=" << json_number(entry.wallTimeSec) << '\n';
        oss << "test_count=" << entry.testCount << '\n';
        oss << "threshold_soft_seconds=" << json_number(entry.threshold.softSeconds) << '\n';
        oss << "threshold_hard_seconds=" << json_number(entry.threshold.hardSeconds) << '\n';
        oss << "threshold_soft_delta_percent=" << json_number(entry.threshold.softDeltaPercent) << '\n';
        oss << "threshold_hard_delta_percent=" << json_number(entry.threshold.hardDeltaPercent) << '\n';
        oss << "host_os=" << entry.hostFingerprint.osName << '\n';
        oss << "host_arch=" << entry.hostFingerprint.cpuArch << '\n';
        oss << "host_runner_tag=" << entry.hostFingerprint.runnerTag << '\n';
        oss << "host_combined_hash=" << entry.hostFingerprint.combinedHash << '\n';
        oss << "compiler_id=" << entry.toolchainFingerprint.compilerId << '\n';
        oss << "compiler_version=" << entry.toolchainFingerprint.compilerVersion << '\n';
        oss << "build_type=" << entry.toolchainFingerprint.buildType << '\n';
        oss << "sanitizer_mode=" << entry.toolchainFingerprint.sanitizerMode << '\n';
        oss << "sanitizer_flags=" << entry.toolchainFingerprint.sanitizerFlags << '\n';
        oss << "toolchain_combined_hash=" << entry.toolchainFingerprint.combinedHash << '\n';
        oss << "baseline_wall_time_sec=" << json_number(entry.baselineWallTimeSec) << '\n';
        oss << "delta_percent=" << json_number(entry.deltaPercent) << '\n';
        oss << "current_status=" << runtime_current_status_name(entry.currentStatus) << '\n';
        oss << "freshness_status=" << runtime_freshness_status_name(entry.freshnessStatus) << '\n';
        oss << "comparability_status=" << runtime_comparability_status_name(entry.comparabilityStatus) << '\n';
        oss << "rationale=" << entry.rationale << '\n';
    }
    return oss.str();
}

string runtime_manifest_json(const RuntimeManifest& manifest) {
    ostringstream oss;
    oss << "{\n";
    oss << "  \"manifest_version\":\"" << json_escape(manifest.manifestVersion) << "\",\n";
    oss << "  \"report_version\":\"" << json_escape(manifest.reportVersion) << "\",\n";
    oss << "  \"manifest_role\":\"" << json_escape(manifest.manifestRole) << "\",\n";
    oss << "  \"generated_at_utc\":\"" << json_escape(manifest.generatedAtUtc) << "\",\n";
    oss << "  \"artifact_root\":\"" << json_escape(manifest.artifactRoot) << "\",\n";
    oss << "  \"source_manifest_path\":\"" << json_escape(manifest.sourceManifestPath) << "\",\n";
    oss << "  \"promoted_from_manifest\":\"" << json_escape(manifest.promotedFromManifest) << "\",\n";
    oss << "  \"baseline_manifest_path\":\"" << json_escape(manifest.baselineManifestPath) << "\",\n";
    oss << "  \"baseline_manifest_hash\":\"" << json_escape(manifest.baselineManifestHash) << "\",\n";
    oss << "  \"current_manifest_hash\":\"" << json_escape(manifest.currentManifestHash) << "\",\n";
    oss << "  \"baseline_tag\":\"" << json_escape(manifest.baselineTag) << "\",\n";
    oss << "  \"approval_timestamp_utc\":\"" << json_escape(manifest.approvalTimestampUtc) << "\",\n";
    oss << "  \"budget_verdict\":\"" << json_escape(manifest.budgetVerdict) << "\",\n";
    oss << "  \"freshness_verdict\":\"" << json_escape(manifest.freshnessVerdict) << "\",\n";
    oss << "  \"comparability_verdict\":\"" << json_escape(manifest.comparabilityVerdict) << "\",\n";
    oss << "  \"fresh_entry_count\":" << manifest.freshEntryCount << ",\n";
    oss << "  \"stale_entry_count\":" << manifest.staleEntryCount << ",\n";
    oss << "  \"requires_rerun_entry_count\":" << manifest.requiresRerunEntryCount << ",\n";
    oss << "  \"rebaseline_required_count\":" << manifest.rebaselineRequiredCount << ",\n";
    oss << "  \"not_comparable_count\":" << manifest.notComparableCount << ",\n";
    oss << "  \"info_only_count\":" << manifest.infoOnlyCount << ",\n";
    oss << "  \"warn_count\":" << manifest.warnCount << ",\n";
    oss << "  \"fail_count\":" << manifest.failCount << ",\n";
    oss << "  \"entries\":[\n";
    for (size_t i = 0; i < manifest.entries.size(); ++i) {
        const RuntimeManifestEntry& entry = manifest.entries[i];
        oss << "    {\n";
        oss << "      \"execution_class\":\"" << json_escape(entry.executionClass) << "\",\n";
        oss << "      \"wall_time_sec\":" << json_number(entry.wallTimeSec) << ",\n";
        oss << "      \"test_count\":" << entry.testCount << ",\n";
        oss << "      \"threshold\":{\n";
        oss << "        \"soft_seconds\":" << json_number(entry.threshold.softSeconds) << ",\n";
        oss << "        \"hard_seconds\":" << json_number(entry.threshold.hardSeconds) << ",\n";
        oss << "        \"soft_delta_percent\":" << json_number(entry.threshold.softDeltaPercent) << ",\n";
        oss << "        \"hard_delta_percent\":" << json_number(entry.threshold.hardDeltaPercent) << "\n";
        oss << "      },\n";
        oss << "      \"host_fingerprint\":{\n";
        oss << "        \"os\":\"" << json_escape(entry.hostFingerprint.osName) << "\",\n";
        oss << "        \"arch\":\"" << json_escape(entry.hostFingerprint.cpuArch) << "\",\n";
        oss << "        \"runner_tag\":\"" << json_escape(entry.hostFingerprint.runnerTag) << "\",\n";
        oss << "        \"combined_hash\":\"" << json_escape(entry.hostFingerprint.combinedHash) << "\"\n";
        oss << "      },\n";
        oss << "      \"toolchain_fingerprint\":{\n";
        oss << "        \"compiler_id\":\"" << json_escape(entry.toolchainFingerprint.compilerId) << "\",\n";
        oss << "        \"compiler_version\":\"" << json_escape(entry.toolchainFingerprint.compilerVersion) << "\",\n";
        oss << "        \"build_type\":\"" << json_escape(entry.toolchainFingerprint.buildType) << "\",\n";
        oss << "        \"sanitizer_mode\":\"" << json_escape(entry.toolchainFingerprint.sanitizerMode) << "\",\n";
        oss << "        \"sanitizer_flags\":\"" << json_escape(entry.toolchainFingerprint.sanitizerFlags) << "\",\n";
        oss << "        \"combined_hash\":\"" << json_escape(entry.toolchainFingerprint.combinedHash) << "\"\n";
        oss << "      },\n";
        oss << "      \"baseline_wall_time_sec\":" << json_number(entry.baselineWallTimeSec) << ",\n";
        oss << "      \"delta_percent\":" << json_number(entry.deltaPercent) << ",\n";
        oss << "      \"current_status\":\"" << runtime_current_status_name(entry.currentStatus) << "\",\n";
        oss << "      \"freshness_status\":\"" << runtime_freshness_status_name(entry.freshnessStatus) << "\",\n";
        oss << "      \"comparability_status\":\"" << runtime_comparability_status_name(entry.comparabilityStatus) << "\",\n";
        oss << "      \"rationale\":\"" << json_escape(entry.rationale) << "\"\n";
        oss << "    }";
        if (i + 1U != manifest.entries.size()) {
            oss << ',';
        }
        oss << '\n';
    }
    oss << "  ]\n";
    oss << "}\n";
    return oss.str();
}

string runtime_manifest_summary(const RuntimeManifest& manifest) {
    ostringstream oss;
    oss << "runtime_manifest_summary"
        << " role=" << manifest.manifestRole
        << " budget_verdict=" << manifest.budgetVerdict
        << " freshness_verdict=" << manifest.freshnessVerdict
        << " comparability_verdict=" << manifest.comparabilityVerdict
        << " stale_entry_count=" << manifest.staleEntryCount
        << " requires_rerun_entry_count=" << manifest.requiresRerunEntryCount
        << " rebaseline_required_count=" << manifest.rebaselineRequiredCount
        << '\n';
    for (const RuntimeManifestEntry& entry : manifest.entries) {
        oss << "entry=" << entry.executionClass
            << " wall_time_sec=" << json_number(entry.wallTimeSec)
            << " current_status=" << runtime_current_status_name(entry.currentStatus)
            << " freshness_status=" << runtime_freshness_status_name(entry.freshnessStatus)
            << " comparability_status=" << runtime_comparability_status_name(entry.comparabilityStatus)
            << '\n';
    }
    return oss.str();
}

void write_runtime_manifest_outputs(const filesystem::path& jsonPath, const RuntimeManifest& manifest) {
    filesystem::create_directories(jsonPath.parent_path());
    ofstream jsonOfs(jsonPath);
    if (!jsonOfs) {
        throw runtime_error("failed to write runtime manifest json: " + jsonPath.string());
    }
    jsonOfs << runtime_manifest_json(manifest);

    const filesystem::path txtPath = jsonPath.parent_path() / (jsonPath.stem().string() + ".txt");
    ofstream txtOfs(txtPath);
    if (!txtOfs) {
        throw runtime_error("failed to write runtime manifest text: " + txtPath.string());
    }
    txtOfs << runtime_manifest_text(manifest);

    const filesystem::path summaryPath = jsonPath.parent_path() / (jsonPath.stem().string() + ".summary.txt");
    ofstream summaryOfs(summaryPath);
    if (!summaryOfs) {
        throw runtime_error("failed to write runtime manifest summary: " + summaryPath.string());
    }
    summaryOfs << runtime_manifest_summary(manifest);
}

RuntimeManifest load_runtime_manifest_text(const filesystem::path& manifestPath) {
    ifstream ifs(manifestPath);
    if (!ifs) {
        throw runtime_error("failed to read runtime manifest: " + manifestPath.string());
    }

    RuntimeManifest manifest;
    RuntimeManifestEntry* current = nullptr;
    string line;
    while (getline(ifs, line)) {
        line = trim_copy(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const size_t equals = line.find('=');
        if (equals == string::npos) {
            continue;
        }
        const string key = trim_copy(line.substr(0U, equals));
        const string value = trim_copy(line.substr(equals + 1U));
        if (key == "manifest_version") {
            manifest.manifestVersion = value;
            continue;
        }
        if (key == "report_version") {
            manifest.reportVersion = value;
            continue;
        }
        if (key == "manifest_role") {
            manifest.manifestRole = value;
            continue;
        }
        if (key == "generated_at_utc") {
            manifest.generatedAtUtc = value;
            continue;
        }
        if (key == "artifact_root") {
            manifest.artifactRoot = value;
            continue;
        }
        if (key == "source_manifest_path") {
            manifest.sourceManifestPath = value;
            continue;
        }
        if (key == "promoted_from_manifest") {
            manifest.promotedFromManifest = value;
            continue;
        }
        if (key == "baseline_manifest_path") {
            manifest.baselineManifestPath = value;
            continue;
        }
        if (key == "baseline_manifest_hash") {
            manifest.baselineManifestHash = value;
            continue;
        }
        if (key == "current_manifest_hash") {
            manifest.currentManifestHash = value;
            continue;
        }
        if (key == "baseline_tag") {
            manifest.baselineTag = value;
            continue;
        }
        if (key == "approval_timestamp_utc") {
            manifest.approvalTimestampUtc = value;
            continue;
        }
        if (key == "budget_verdict") {
            manifest.budgetVerdict = value;
            continue;
        }
        if (key == "freshness_verdict") {
            manifest.freshnessVerdict = value;
            continue;
        }
        if (key == "comparability_verdict") {
            manifest.comparabilityVerdict = value;
            continue;
        }
        if (key == "fresh_entry_count") {
            manifest.freshEntryCount = static_cast<size_t>(stoull(value));
            continue;
        }
        if (key == "stale_entry_count") {
            manifest.staleEntryCount = static_cast<size_t>(stoull(value));
            continue;
        }
        if (key == "requires_rerun_entry_count") {
            manifest.requiresRerunEntryCount = static_cast<size_t>(stoull(value));
            continue;
        }
        if (key == "rebaseline_required_count") {
            manifest.rebaselineRequiredCount = static_cast<size_t>(stoull(value));
            continue;
        }
        if (key == "not_comparable_count") {
            manifest.notComparableCount = static_cast<size_t>(stoull(value));
            continue;
        }
        if (key == "info_only_count") {
            manifest.infoOnlyCount = static_cast<size_t>(stoull(value));
            continue;
        }
        if (key == "warn_count") {
            manifest.warnCount = static_cast<size_t>(stoull(value));
            continue;
        }
        if (key == "fail_count") {
            manifest.failCount = static_cast<size_t>(stoull(value));
            continue;
        }
        if (key == "entry_count") {
            continue;
        }
        if (key == "entry") {
            manifest.entries.push_back(RuntimeManifestEntry{});
            current = &manifest.entries.back();
            current->executionClass = value;
            continue;
        }
        if (current == nullptr) {
            continue;
        }
        if (key == "wall_time_sec") {
            current->wallTimeSec = stod(value);
        } else if (key == "test_count") {
            current->testCount = static_cast<size_t>(stoull(value));
        } else if (key == "threshold_soft_seconds") {
            current->threshold.softSeconds = stod(value);
        } else if (key == "threshold_hard_seconds") {
            current->threshold.hardSeconds = stod(value);
        } else if (key == "threshold_soft_delta_percent") {
            current->threshold.softDeltaPercent = stod(value);
        } else if (key == "threshold_hard_delta_percent") {
            current->threshold.hardDeltaPercent = stod(value);
        } else if (key == "host_os") {
            current->hostFingerprint.osName = value;
        } else if (key == "host_arch") {
            current->hostFingerprint.cpuArch = value;
        } else if (key == "host_runner_tag") {
            current->hostFingerprint.runnerTag = value;
        } else if (key == "host_combined_hash") {
            current->hostFingerprint.combinedHash = value;
        } else if (key == "compiler_id") {
            current->toolchainFingerprint.compilerId = value;
        } else if (key == "compiler_version") {
            current->toolchainFingerprint.compilerVersion = value;
        } else if (key == "build_type") {
            current->toolchainFingerprint.buildType = value;
        } else if (key == "sanitizer_mode") {
            current->toolchainFingerprint.sanitizerMode = value;
        } else if (key == "sanitizer_flags") {
            current->toolchainFingerprint.sanitizerFlags = value;
        } else if (key == "toolchain_combined_hash") {
            current->toolchainFingerprint.combinedHash = value;
        } else if (key == "baseline_wall_time_sec") {
            current->baselineWallTimeSec = stod(value);
        } else if (key == "delta_percent") {
            current->deltaPercent = stod(value);
        } else if (key == "current_status") {
            current->currentStatus = parse_runtime_current_status(value);
        } else if (key == "freshness_status") {
            current->freshnessStatus = parse_runtime_freshness_status(value);
        } else if (key == "comparability_status") {
            current->comparabilityStatus = parse_runtime_comparability_status(value);
        } else if (key == "rationale") {
            current->rationale = value;
        }
    }
    return manifest;
}

string runtime_rerun_plan_text(const RuntimeRerunPlan& plan) {
    ostringstream oss;
    oss << "plan_version=" << plan.planVersion << '\n';
    oss << "generated_at_utc=" << plan.generatedAtUtc << '\n';
    oss << "artifact_root=" << plan.artifactRoot << '\n';
    oss << "baseline_manifest_path=" << plan.baselineManifestPath << '\n';
    oss << "current_manifest_path=" << plan.currentManifestPath << '\n';
    oss << "refresh_manifest_path=" << plan.refreshManifestPath << '\n';
    oss << "baseline_manifest_hash=" << plan.baselineManifestHash << '\n';
    oss << "current_manifest_hash=" << plan.currentManifestHash << '\n';
    oss << "refresh_manifest_hash=" << plan.refreshManifestHash << '\n';
    oss << "stale_entry_count=" << plan.staleEntryCount << '\n';
    oss << "requires_rerun_entry_count=" << plan.requiresRerunEntryCount << '\n';
    oss << "rebaseline_required_count=" << plan.rebaselineRequiredCount << '\n';
    oss << "selected_entry_count=" << plan.selectedEntryCount << '\n';
    oss << "summary_verdict=" << plan.summaryVerdict << '\n';
    oss << "rationale=" << plan.rationale << '\n';
    for (const RuntimeRerunPlanEntry& entry : plan.entries) {
        oss << '\n';
        oss << "entry=" << entry.executionClass << '\n';
        oss << "current_status=" << entry.currentStatus << '\n';
        oss << "freshness_status=" << entry.freshnessStatus << '\n';
        oss << "comparability_status=" << entry.comparabilityStatus << '\n';
        oss << "rerun_kind=" << entry.rerunKind << '\n';
        oss << "recommended_command=" << entry.recommendedCommand << '\n';
        oss << "expected_stop_criteria=" << entry.expectedStopCriteria << '\n';
        oss << "status_impact=" << entry.statusImpact << '\n';
    }
    return oss.str();
}

string runtime_rerun_plan_json(const RuntimeRerunPlan& plan) {
    ostringstream oss;
    oss << "{\n";
    oss << "  \"plan_version\":\"" << json_escape(plan.planVersion) << "\",\n";
    oss << "  \"generated_at_utc\":\"" << json_escape(plan.generatedAtUtc) << "\",\n";
    oss << "  \"artifact_root\":\"" << json_escape(plan.artifactRoot) << "\",\n";
    oss << "  \"baseline_manifest_path\":\"" << json_escape(plan.baselineManifestPath) << "\",\n";
    oss << "  \"current_manifest_path\":\"" << json_escape(plan.currentManifestPath) << "\",\n";
    oss << "  \"refresh_manifest_path\":\"" << json_escape(plan.refreshManifestPath) << "\",\n";
    oss << "  \"baseline_manifest_hash\":\"" << json_escape(plan.baselineManifestHash) << "\",\n";
    oss << "  \"current_manifest_hash\":\"" << json_escape(plan.currentManifestHash) << "\",\n";
    oss << "  \"refresh_manifest_hash\":\"" << json_escape(plan.refreshManifestHash) << "\",\n";
    oss << "  \"stale_entry_count\":" << plan.staleEntryCount << ",\n";
    oss << "  \"requires_rerun_entry_count\":" << plan.requiresRerunEntryCount << ",\n";
    oss << "  \"rebaseline_required_count\":" << plan.rebaselineRequiredCount << ",\n";
    oss << "  \"selected_entry_count\":" << plan.selectedEntryCount << ",\n";
    oss << "  \"summary_verdict\":\"" << json_escape(plan.summaryVerdict) << "\",\n";
    oss << "  \"rationale\":\"" << json_escape(plan.rationale) << "\",\n";
    oss << "  \"entries\":[\n";
    for (size_t i = 0; i < plan.entries.size(); ++i) {
        const RuntimeRerunPlanEntry& entry = plan.entries[i];
        oss << "    {\n";
        oss << "      \"execution_class\":\"" << json_escape(entry.executionClass) << "\",\n";
        oss << "      \"current_status\":\"" << json_escape(entry.currentStatus) << "\",\n";
        oss << "      \"freshness_status\":\"" << json_escape(entry.freshnessStatus) << "\",\n";
        oss << "      \"comparability_status\":\"" << json_escape(entry.comparabilityStatus) << "\",\n";
        oss << "      \"rerun_kind\":\"" << json_escape(entry.rerunKind) << "\",\n";
        oss << "      \"recommended_command\":\"" << json_escape(entry.recommendedCommand) << "\",\n";
        oss << "      \"expected_stop_criteria\":\"" << json_escape(entry.expectedStopCriteria) << "\",\n";
        oss << "      \"status_impact\":\"" << json_escape(entry.statusImpact) << "\"\n";
        oss << "    }";
        if (i + 1U != plan.entries.size()) {
            oss << ',';
        }
        oss << '\n';
    }
    oss << "  ]\n";
    oss << "}\n";
    return oss.str();
}

string runtime_rerun_plan_summary(const RuntimeRerunPlan& plan) {
    ostringstream oss;
    oss << "runtime_rerun_plan_summary"
        << " summary_verdict=" << plan.summaryVerdict
        << " selected_entry_count=" << plan.selectedEntryCount
        << " stale_entry_count=" << plan.staleEntryCount
        << " requires_rerun_entry_count=" << plan.requiresRerunEntryCount
        << " rebaseline_required_count=" << plan.rebaselineRequiredCount
        << '\n';
    return oss.str();
}

void write_runtime_rerun_plan_outputs(const filesystem::path& jsonPath, const RuntimeRerunPlan& plan) {
    filesystem::create_directories(jsonPath.parent_path());
    ofstream jsonOfs(jsonPath);
    if (!jsonOfs) {
        throw runtime_error("failed to write runtime rerun plan json: " + jsonPath.string());
    }
    jsonOfs << runtime_rerun_plan_json(plan);

    const filesystem::path txtPath = jsonPath.parent_path() / (jsonPath.stem().string() + ".txt");
    ofstream txtOfs(txtPath);
    if (!txtOfs) {
        throw runtime_error("failed to write runtime rerun plan text: " + txtPath.string());
    }
    txtOfs << runtime_rerun_plan_text(plan);

    const filesystem::path summaryPath = jsonPath.parent_path() / (jsonPath.stem().string() + ".summary.txt");
    ofstream summaryOfs(summaryPath);
    if (!summaryOfs) {
        throw runtime_error("failed to write runtime rerun plan summary: " + summaryPath.string());
    }
    summaryOfs << runtime_rerun_plan_summary(plan);
}

RuntimeRerunPlan load_runtime_rerun_plan_text(const filesystem::path& planPath) {
    ifstream ifs(planPath);
    if (!ifs) {
        throw runtime_error("failed to read runtime rerun plan: " + planPath.string());
    }
    RuntimeRerunPlan plan;
    RuntimeRerunPlanEntry* current = nullptr;
    string line;
    while (getline(ifs, line)) {
        line = trim_copy(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const size_t equals = line.find('=');
        if (equals == string::npos) {
            continue;
        }
        const string key = trim_copy(line.substr(0U, equals));
        const string value = trim_copy(line.substr(equals + 1U));
        if (key == "plan_version") {
            plan.planVersion = value;
            continue;
        }
        if (key == "generated_at_utc") {
            plan.generatedAtUtc = value;
            continue;
        }
        if (key == "artifact_root") {
            plan.artifactRoot = value;
            continue;
        }
        if (key == "baseline_manifest_path") {
            plan.baselineManifestPath = value;
            continue;
        }
        if (key == "current_manifest_path") {
            plan.currentManifestPath = value;
            continue;
        }
        if (key == "refresh_manifest_path") {
            plan.refreshManifestPath = value;
            continue;
        }
        if (key == "baseline_manifest_hash") {
            plan.baselineManifestHash = value;
            continue;
        }
        if (key == "current_manifest_hash") {
            plan.currentManifestHash = value;
            continue;
        }
        if (key == "refresh_manifest_hash") {
            plan.refreshManifestHash = value;
            continue;
        }
        if (key == "stale_entry_count") {
            plan.staleEntryCount = static_cast<size_t>(stoull(value));
            continue;
        }
        if (key == "requires_rerun_entry_count") {
            plan.requiresRerunEntryCount = static_cast<size_t>(stoull(value));
            continue;
        }
        if (key == "rebaseline_required_count") {
            plan.rebaselineRequiredCount = static_cast<size_t>(stoull(value));
            continue;
        }
        if (key == "selected_entry_count") {
            plan.selectedEntryCount = static_cast<size_t>(stoull(value));
            continue;
        }
        if (key == "summary_verdict") {
            plan.summaryVerdict = value;
            continue;
        }
        if (key == "rationale") {
            plan.rationale = value;
            continue;
        }
        if (key == "entry") {
            plan.entries.push_back(RuntimeRerunPlanEntry{});
            current = &plan.entries.back();
            current->executionClass = value;
            continue;
        }
        if (current == nullptr) {
            continue;
        }
        if (key == "current_status") {
            current->currentStatus = value;
        } else if (key == "freshness_status") {
            current->freshnessStatus = value;
        } else if (key == "comparability_status") {
            current->comparabilityStatus = value;
        } else if (key == "rerun_kind") {
            current->rerunKind = value;
        } else if (key == "recommended_command") {
            current->recommendedCommand = value;
        } else if (key == "expected_stop_criteria") {
            current->expectedStopCriteria = value;
        } else if (key == "status_impact") {
            current->statusImpact = value;
        }
    }
    recompute_runtime_rerun_plan_rollup(plan);
    return plan;
}
