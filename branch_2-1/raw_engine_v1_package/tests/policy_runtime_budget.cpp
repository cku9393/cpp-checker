#include "policy_runtime_budget.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

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

bool manifest_has_unsatisfied_family(const PolicyGateManifest& manifest) {
    return any_of(manifest.families.begin(), manifest.families.end(), [](const PolicyGateFamilyResult& family) {
        return !policy_gate_status_is_satisfied(family.status);
    });
}

PolicyRuntimeBudgetStatus parse_runtime_budget_status(const string& value) {
    if (value == "OK") {
        return PolicyRuntimeBudgetStatus::OK;
    }
    if (value == "WARN") {
        return PolicyRuntimeBudgetStatus::WARN;
    }
    if (value == "FAIL") {
        return PolicyRuntimeBudgetStatus::FAIL;
    }
    throw runtime_error("unknown policy runtime budget status: " + value);
}

void recompute_policy_runtime_budget_rollup_impl(PolicyRuntimeBudgetManifest& manifest) {
    manifest.warnCount = 0U;
    manifest.failCount = 0U;
    for (const PolicyRuntimeBudgetEntry& entry : manifest.entries) {
        if (entry.status == PolicyRuntimeBudgetStatus::WARN) {
            ++manifest.warnCount;
        } else if (entry.status == PolicyRuntimeBudgetStatus::FAIL) {
            ++manifest.failCount;
        }
    }
    if (manifest.failCount != 0U) {
        manifest.budgetVerdict = "BUDGET_FAIL";
    } else if (manifest.warnCount != 0U) {
        manifest.budgetVerdict = "BUDGET_WARN";
    } else {
        manifest.budgetVerdict = "PASS";
    }
    manifest.generatedAtUtc = timestamp_utc_now();
    manifest.currentRuntimeManifestHash = stable_hash_text(policy_runtime_budget_manifest_text(manifest));
}

} // namespace

const char* policy_pipeline_severity_name(PolicyPipelineSeverity severity) {
    switch (severity) {
        case PolicyPipelineSeverity::OK: return "OK";
        case PolicyPipelineSeverity::WARN: return "WARN";
        case PolicyPipelineSeverity::ACTION_REQUIRED: return "ACTION_REQUIRED";
        case PolicyPipelineSeverity::FAIL: return "FAIL";
    }
    return "FAIL";
}

int policy_pipeline_exit_code(PolicyPipelineSeverity severity, bool strictWarnExit) {
    switch (severity) {
        case PolicyPipelineSeverity::OK:
            return 0;
        case PolicyPipelineSeverity::WARN:
            return strictWarnExit ? 10 : 0;
        case PolicyPipelineSeverity::ACTION_REQUIRED:
            return 20;
        case PolicyPipelineSeverity::FAIL:
            return 30;
    }
    return 30;
}

PolicyPipelineSeverity evaluate_policy_pipeline_severity(
    const PolicyGateManifest& currentManifest,
    const PolicyGateManifest& refreshManifest,
    const PolicyRerunPlan& plan
) {
    if (manifest_has_unsatisfied_family(currentManifest) || plan.summaryVerdict == "FAIL") {
        return PolicyPipelineSeverity::FAIL;
    }
    if (refreshManifest.requiresRerunFamilyCount != 0U || refreshManifest.reclassifyRequiredCount != 0U) {
        return PolicyPipelineSeverity::ACTION_REQUIRED;
    }
    if (refreshManifest.staleFamilyCount != 0U) {
        return plan.entries.empty() ? PolicyPipelineSeverity::ACTION_REQUIRED : PolicyPipelineSeverity::WARN;
    }
    return PolicyPipelineSeverity::OK;
}

string policy_pipeline_action_recommendation(
    PolicyPipelineSeverity severity,
    const PolicyGateManifest& currentManifest,
    const PolicyGateManifest& refreshManifest,
    const PolicyRerunPlan& plan
) {
    switch (severity) {
        case PolicyPipelineSeverity::OK:
            return "no action required; exact_shadow lifecycle remains healthy";
        case PolicyPipelineSeverity::WARN:
            return "run policy_nightly_refresh or execute the rerun plan to refresh stale evidence";
        case PolicyPipelineSeverity::ACTION_REQUIRED:
            if (refreshManifest.reclassifyRequiredCount != 0U) {
                return "reclassification required; rerun affected families and review exact_shadow policy retention";
            }
            if (refreshManifest.requiresRerunFamilyCount != 0U) {
                return "rerun required families before treating the policy state as fresh";
            }
            if (plan.entries.empty()) {
                return "stale evidence detected without a rerun plan; regenerate manifests and inspect lifecycle inputs";
            }
            return "execute the rerun plan and refresh the policy manifests";
        case PolicyPipelineSeverity::FAIL:
            if (manifest_has_unsatisfied_family(currentManifest)) {
                return "policy gate failed; inspect failing families before approving exact_shadow";
            }
            return "lifecycle execution failed; inspect manifests, rerun execution, or artifact corruption";
    }
    return "policy lifecycle requires investigation";
}

const char* policy_runtime_budget_status_name(PolicyRuntimeBudgetStatus status) {
    switch (status) {
        case PolicyRuntimeBudgetStatus::OK: return "OK";
        case PolicyRuntimeBudgetStatus::WARN: return "WARN";
        case PolicyRuntimeBudgetStatus::FAIL: return "FAIL";
    }
    return "FAIL";
}

PolicyRuntimeBudgetThreshold default_policy_runtime_budget_threshold(const string& entryName) {
    if (entryName == "release_ctest") {
        return {240.0, 480.0, 25.0, 60.0};
    }
    if (entryName == "debug_ctest") {
        return {900.0, 1800.0, 25.0, 60.0};
    }
    if (entryName == "asan_ctest") {
        return {2400.0, 4200.0, 25.0, 60.0};
    }
    if (entryName == "policy_quick" || entryName == "policy_core" || entryName == "policy_refresh") {
        return {30.0, 90.0, 25.0, 75.0};
    }
    if (entryName == "policy_nightly" || entryName == "policy_full_local") {
        return {120.0, 360.0, 25.0, 75.0};
    }
    if (entryName.find("compare") != string::npos) {
        return {300.0, 1200.0, 30.0, 80.0};
    }
    return {60.0, 180.0, 25.0, 75.0};
}

PolicyRuntimeBudgetEntry evaluate_policy_runtime_budget_entry(
    const string& entryName,
    double currentSeconds,
    double baselineSeconds,
    const PolicyRuntimeBudgetThreshold& threshold
) {
    PolicyRuntimeBudgetEntry entry;
    entry.name = entryName;
    entry.currentSeconds = currentSeconds;
    entry.baselineSeconds = baselineSeconds;
    entry.threshold = threshold;
    if (baselineSeconds > 0.0) {
        entry.deltaPercent = ((currentSeconds - baselineSeconds) / baselineSeconds) * 100.0;
    }

    const bool hardSecondsExceeded = threshold.hardSeconds > 0.0 && currentSeconds > threshold.hardSeconds;
    const bool softSecondsExceeded = threshold.softSeconds > 0.0 && currentSeconds > threshold.softSeconds;
    const bool hardDeltaExceeded = threshold.hardDeltaPercent > 0.0 && entry.deltaPercent > threshold.hardDeltaPercent;
    const bool softDeltaExceeded = threshold.softDeltaPercent > 0.0 && entry.deltaPercent > threshold.softDeltaPercent;

    if (hardSecondsExceeded || hardDeltaExceeded) {
        entry.status = PolicyRuntimeBudgetStatus::FAIL;
        entry.rationale = "runtime exceeded hard budget";
    } else if (softSecondsExceeded || softDeltaExceeded) {
        entry.status = PolicyRuntimeBudgetStatus::WARN;
        entry.rationale = "runtime exceeded soft budget";
    } else {
        entry.status = PolicyRuntimeBudgetStatus::OK;
        entry.rationale = "runtime remained within budget";
    }
    return entry;
}

PolicyRuntimeBudgetManifest evaluate_policy_runtime_budget_manifest(
    const string& artifactRoot,
    const vector<PolicyRuntimeBudgetEntry>& entries,
    const string& baselineRuntimeManifestPath,
    const string& baselineRuntimeManifestHash
) {
    PolicyRuntimeBudgetManifest manifest;
    manifest.artifactRoot = artifactRoot;
    manifest.baselineRuntimeManifestPath = baselineRuntimeManifestPath;
    manifest.baselineRuntimeManifestHash = baselineRuntimeManifestHash;
    manifest.entries = entries;
    recompute_policy_runtime_budget_rollup_impl(manifest);
    return manifest;
}

PolicyRuntimeBudgetManifest evaluate_policy_runtime_budget_manifest(
    const string& artifactRoot,
    const filesystem::path& baselineRuntimeManifestPath,
    const vector<PolicyRuntimeBudgetEntry>& entries
) {
    return evaluate_policy_runtime_budget_manifest(
        artifactRoot,
        entries,
        baselineRuntimeManifestPath.empty() ? string() : filesystem::absolute(baselineRuntimeManifestPath).string(),
        baselineRuntimeManifestPath.empty() ? string() : stable_hash_text(baselineRuntimeManifestPath.string())
    );
}

void recompute_policy_runtime_budget_rollup(PolicyRuntimeBudgetManifest& manifest) {
    recompute_policy_runtime_budget_rollup_impl(manifest);
}

string policy_runtime_budget_manifest_text(const PolicyRuntimeBudgetManifest& manifest) {
    ostringstream oss;
    oss << "manifest_version=" << manifest.manifestVersion << '\n';
    oss << "report_version=" << manifest.reportVersion << '\n';
    oss << "generated_at_utc=" << manifest.generatedAtUtc << '\n';
    oss << "artifact_root=" << manifest.artifactRoot << '\n';
    oss << "baseline_runtime_manifest_path=" << manifest.baselineRuntimeManifestPath << '\n';
    oss << "baseline_runtime_manifest_hash=" << manifest.baselineRuntimeManifestHash << '\n';
    oss << "current_runtime_manifest_hash=" << manifest.currentRuntimeManifestHash << '\n';
    oss << "budget_verdict=" << manifest.budgetVerdict << '\n';
    oss << "warn_count=" << manifest.warnCount << '\n';
    oss << "fail_count=" << manifest.failCount << '\n';
    oss << "entry_count=" << manifest.entries.size() << '\n';
    for (const PolicyRuntimeBudgetEntry& entry : manifest.entries) {
        oss << '\n';
        oss << "entry=" << entry.name << '\n';
        oss << "current_seconds=" << json_number(entry.currentSeconds) << '\n';
        oss << "baseline_seconds=" << json_number(entry.baselineSeconds) << '\n';
        oss << "threshold_soft_seconds=" << json_number(entry.threshold.softSeconds) << '\n';
        oss << "threshold_hard_seconds=" << json_number(entry.threshold.hardSeconds) << '\n';
        oss << "threshold_soft_delta_percent=" << json_number(entry.threshold.softDeltaPercent) << '\n';
        oss << "threshold_hard_delta_percent=" << json_number(entry.threshold.hardDeltaPercent) << '\n';
        oss << "delta_percent=" << json_number(entry.deltaPercent) << '\n';
        oss << "status=" << policy_runtime_budget_status_name(entry.status) << '\n';
        oss << "rationale=" << entry.rationale << '\n';
    }
    return oss.str();
}

string policy_runtime_budget_manifest_json(const PolicyRuntimeBudgetManifest& manifest) {
    ostringstream oss;
    oss << "{\n";
    oss << "  \"manifest_version\":\"" << json_escape(manifest.manifestVersion) << "\",\n";
    oss << "  \"report_version\":\"" << json_escape(manifest.reportVersion) << "\",\n";
    oss << "  \"generated_at_utc\":\"" << json_escape(manifest.generatedAtUtc) << "\",\n";
    oss << "  \"artifact_root\":\"" << json_escape(manifest.artifactRoot) << "\",\n";
    oss << "  \"baseline_runtime_manifest_path\":\"" << json_escape(manifest.baselineRuntimeManifestPath) << "\",\n";
    oss << "  \"baseline_runtime_manifest_hash\":\"" << json_escape(manifest.baselineRuntimeManifestHash) << "\",\n";
    oss << "  \"current_runtime_manifest_hash\":\"" << json_escape(manifest.currentRuntimeManifestHash) << "\",\n";
    oss << "  \"budget_verdict\":\"" << json_escape(manifest.budgetVerdict) << "\",\n";
    oss << "  \"warn_count\":" << manifest.warnCount << ",\n";
    oss << "  \"fail_count\":" << manifest.failCount << ",\n";
    oss << "  \"entries\":[\n";
    for (size_t i = 0; i < manifest.entries.size(); ++i) {
        const PolicyRuntimeBudgetEntry& entry = manifest.entries[i];
        oss << "    {\n";
        oss << "      \"name\":\"" << json_escape(entry.name) << "\",\n";
        oss << "      \"current_seconds\":" << json_number(entry.currentSeconds) << ",\n";
        oss << "      \"baseline_seconds\":" << json_number(entry.baselineSeconds) << ",\n";
        oss << "      \"delta_percent\":" << json_number(entry.deltaPercent) << ",\n";
        oss << "      \"threshold\":{\n";
        oss << "        \"soft_seconds\":" << json_number(entry.threshold.softSeconds) << ",\n";
        oss << "        \"hard_seconds\":" << json_number(entry.threshold.hardSeconds) << ",\n";
        oss << "        \"soft_delta_percent\":" << json_number(entry.threshold.softDeltaPercent) << ",\n";
        oss << "        \"hard_delta_percent\":" << json_number(entry.threshold.hardDeltaPercent) << "\n";
        oss << "      },\n";
        oss << "      \"status\":\"" << policy_runtime_budget_status_name(entry.status) << "\",\n";
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

string policy_runtime_budget_summary(const PolicyRuntimeBudgetManifest& manifest) {
    ostringstream oss;
    oss << "policy_runtime_budget_summary"
        << " budget_verdict=" << manifest.budgetVerdict
        << " warn_count=" << manifest.warnCount
        << " fail_count=" << manifest.failCount
        << " entry_count=" << manifest.entries.size()
        << '\n';
    for (const PolicyRuntimeBudgetEntry& entry : manifest.entries) {
        oss << "entry=" << entry.name
            << " status=" << policy_runtime_budget_status_name(entry.status)
            << " current_seconds=" << json_number(entry.currentSeconds)
            << " baseline_seconds=" << json_number(entry.baselineSeconds)
            << " delta_percent=" << json_number(entry.deltaPercent)
            << " rationale=" << entry.rationale
            << '\n';
    }
    return oss.str();
}

void write_policy_runtime_budget_outputs(
    const filesystem::path& jsonPath,
    const PolicyRuntimeBudgetManifest& manifest
) {
    filesystem::create_directories(jsonPath.parent_path());

    ofstream jsonOfs(jsonPath);
    if (!jsonOfs) {
        throw runtime_error("failed to write policy runtime budget json: " + jsonPath.string());
    }
    jsonOfs << policy_runtime_budget_manifest_json(manifest);

    const filesystem::path txtPath = jsonPath.parent_path() / (jsonPath.stem().string() + ".txt");
    ofstream txtOfs(txtPath);
    if (!txtOfs) {
        throw runtime_error("failed to write policy runtime budget text: " + txtPath.string());
    }
    txtOfs << policy_runtime_budget_manifest_text(manifest);

    const filesystem::path summaryPath = jsonPath.parent_path() / (jsonPath.stem().string() + ".summary.txt");
    ofstream summaryOfs(summaryPath);
    if (!summaryOfs) {
        throw runtime_error("failed to write policy runtime budget summary: " + summaryPath.string());
    }
    summaryOfs << policy_runtime_budget_summary(manifest);
}

PolicyRuntimeBudgetManifest load_policy_runtime_budget_manifest_text(const filesystem::path& manifestPath) {
    ifstream ifs(manifestPath);
    if (!ifs) {
        throw runtime_error("failed to read policy runtime budget manifest: " + manifestPath.string());
    }

    PolicyRuntimeBudgetManifest manifest;
    PolicyRuntimeBudgetEntry* current = nullptr;
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
        if (key == "generated_at_utc") {
            manifest.generatedAtUtc = value;
            continue;
        }
        if (key == "artifact_root") {
            manifest.artifactRoot = value;
            continue;
        }
        if (key == "baseline_runtime_manifest_path") {
            manifest.baselineRuntimeManifestPath = value;
            continue;
        }
        if (key == "baseline_runtime_manifest_hash") {
            manifest.baselineRuntimeManifestHash = value;
            continue;
        }
        if (key == "current_runtime_manifest_hash") {
            manifest.currentRuntimeManifestHash = value;
            continue;
        }
        if (key == "budget_verdict") {
            manifest.budgetVerdict = value;
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
            manifest.entries.push_back(PolicyRuntimeBudgetEntry{});
            current = &manifest.entries.back();
            current->name = value;
            continue;
        }
        if (current == nullptr) {
            continue;
        }
        if (key == "current_seconds") {
            current->currentSeconds = stod(value);
        } else if (key == "baseline_seconds") {
            current->baselineSeconds = stod(value);
        } else if (key == "threshold_soft_seconds") {
            current->threshold.softSeconds = stod(value);
        } else if (key == "threshold_hard_seconds") {
            current->threshold.hardSeconds = stod(value);
        } else if (key == "threshold_soft_delta_percent") {
            current->threshold.softDeltaPercent = stod(value);
        } else if (key == "threshold_hard_delta_percent") {
            current->threshold.hardDeltaPercent = stod(value);
        } else if (key == "delta_percent") {
            current->deltaPercent = stod(value);
        } else if (key == "status") {
            current->status = parse_runtime_budget_status(value);
        } else if (key == "rationale") {
            current->rationale = value;
        }
    }
    return manifest;
}

PolicyRuntimeBudgetManifest load_policy_runtime_budget_text(const filesystem::path& manifestPath) {
    return load_policy_runtime_budget_manifest_text(manifestPath);
}
