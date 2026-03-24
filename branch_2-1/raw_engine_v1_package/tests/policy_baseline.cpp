#include "policy_baseline.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <initializer_list>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "failure_signature.hpp"
#include "policy_manifest_freshness.hpp"

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

string sanitize_token(string value) {
    for (char& ch : value) {
        const bool ok =
            (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') ||
            ch == '-' || ch == '_';
        if (!ok) {
            ch = '_';
        }
    }
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

string csv_from_strings(const vector<string>& values) {
    ostringstream oss;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i != 0U) {
            oss << ',';
        }
        oss << values[i];
    }
    return oss.str();
}

vector<string> split_csv(const string& value) {
    vector<string> out;
    size_t begin = 0U;
    while (begin <= value.size()) {
        const size_t end = value.find(',', begin);
        string token = end == string::npos ? value.substr(begin) : value.substr(begin, end - begin);
        token.erase(token.begin(), find_if(token.begin(), token.end(), [](unsigned char ch) {
            return !isspace(ch);
        }));
        token.erase(find_if(token.rbegin(), token.rend(), [](unsigned char ch) {
            return !isspace(ch);
        }).base(), token.end());
        if (!token.empty()) {
            out.push_back(token);
        }
        if (end == string::npos) {
            break;
        }
        begin = end + 1U;
    }
    return out;
}

bool contains_string(const vector<string>& values, const string& value) {
    return find(values.begin(), values.end(), value) != values.end();
}

void append_unique(vector<string>& values, const string& value) {
    if (!value.empty() && !contains_string(values, value)) {
        values.push_back(value);
    }
}

unordered_map<string, string> read_key_value_file(const filesystem::path& path) {
    ifstream ifs(path);
    if (!ifs) {
        throw runtime_error("failed to read key-value file: " + path.string());
    }
    unordered_map<string, string> values;
    string line;
    while (getline(ifs, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const size_t eq = line.find('=');
        if (eq == string::npos) {
            continue;
        }
        values.emplace(line.substr(0U, eq), line.substr(eq + 1U));
    }
    return values;
}

string lookup_value(const unordered_map<string, string>& values, const string& key) {
    const auto it = values.find(key);
    return it == values.end() ? string() : it->second;
}

string resolve_promoted_report_path(const filesystem::path& sourceRoot, const string& reportVersion) {
    const string prefix = string(reportVersion.begin(), reportVersion.end());
    filesystem::path latestPath;
    filesystem::file_time_type latestTime{};
    bool found = false;
    for (const auto& entry : filesystem::directory_iterator(sourceRoot)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const string name = entry.path().filename().string();
        if (name.find("_STABILIZATION_REPORT") == string::npos || name.find(prefix.substr(0U, 5U)) == string::npos) {
            continue;
        }
        const auto writeTime = entry.last_write_time();
        if (!found || writeTime > latestTime) {
            latestPath = entry.path();
            latestTime = writeTime;
            found = true;
        }
    }
    return found ? filesystem::absolute(latestPath).string() : string();
}

string resolve_promoted_bundle_path(const string& promotedReportPath) {
    if (promotedReportPath.empty()) {
        return {};
    }
    const filesystem::path reportPath = filesystem::absolute(promotedReportPath);
    if (!filesystem::exists(reportPath)) {
        return {};
    }
    const string reportName = reportPath.filename().string();
    const size_t reportPos = reportName.find("_STABILIZATION_REPORT");
    if (reportPos == string::npos) {
        return {};
    }

    string phaseToken = reportName.substr(0U, reportPos);
    transform(phaseToken.begin(), phaseToken.end(), phaseToken.begin(), [](unsigned char ch) {
        return static_cast<char>(tolower(ch));
    });

    filesystem::path latestPath;
    filesystem::file_time_type latestTime{};
    bool found = false;
    for (const auto& entry : filesystem::directory_iterator(reportPath.parent_path())) {
        if (!entry.is_regular_file() || entry.path().extension() != ".zip") {
            continue;
        }
        const string name = entry.path().filename().string();
        if (name.find(phaseToken) == string::npos || name.find("stabilization") == string::npos ||
            name.find("curated") != string::npos) {
            continue;
        }
        const auto writeTime = entry.last_write_time();
        if (!found || writeTime > latestTime) {
            latestPath = entry.path();
            latestTime = writeTime;
            found = true;
        }
    }
    return found ? filesystem::absolute(latestPath).string() : string();
}

filesystem::path first_existing_campaign_config(initializer_list<filesystem::path> relPaths) {
    const filesystem::path testsRoot = filesystem::path(__FILE__).parent_path();
    for (const filesystem::path& relPath : relPaths) {
        if (filesystem::exists(testsRoot / relPath)) {
            return filesystem::path("tests") / relPath;
        }
    }
    return filesystem::path("tests") / *relPaths.begin();
}

string recommended_campaign_config(const string& family) {
    if (family == "split_tie_organic_symmetric") {
        return first_existing_campaign_config(
            {
                filesystem::path("campaigns/phase22_split_tie_organic_compare.txt"),
                filesystem::path("campaigns/phase21_split_tie_organic_compare.txt"),
                filesystem::path("campaigns/phase17_split_tie_organic_compare.txt"),
                filesystem::path("campaigns/phase16_split_tie_organic_compare.txt"),
            }
        ).generic_string();
    }
    if (family == "automorphism_probe_large") {
        return first_existing_campaign_config(
            {
                filesystem::path("campaigns/phase22_automorphism_compare.txt"),
                filesystem::path("campaigns/phase21_automorphism_compare.txt"),
                filesystem::path("campaigns/phase17_automorphism_compare.txt"),
                filesystem::path("campaigns/phase16_automorphism_compare.txt"),
            }
        ).generic_string();
    }
    if (family == "planner_tie_mixed_organic") {
        return first_existing_campaign_config(
            {
                filesystem::path("campaigns/phase22_planner_tie_gap_audit.txt"),
                filesystem::path("campaigns/phase17_planner_tie_gap_audit.txt"),
                filesystem::path("campaigns/phase16_planner_tie_gap_probe.txt"),
            }
        ).generic_string();
    }
    if (family == "planner_tie_mixed_organic_compare_ready") {
        return first_existing_campaign_config(
            {
                filesystem::path("campaigns/phase22_planner_tie_compare_ready.txt"),
                filesystem::path("campaigns/phase17_planner_tie_compare_ready.txt"),
                filesystem::path("campaigns/phase16_planner_tie_compare_ready.txt"),
            }
        ).generic_string();
    }
    return {};
}

bool family_selected_for_plan(const PolicyGateFamilyResult& family, const PolicyRerunPlanOptions& options) {
    if (!options.familyFilter.empty() && !contains_string(options.familyFilter, family.family)) {
        return false;
    }
    if (family.status == PolicyGateStatus::DIAGNOSTIC_ONLY && !options.includeDiagnostic) {
        return false;
    }
    if (family.status == PolicyGateStatus::NON_APPLICABLE && !options.includeNonApplicable) {
        return false;
    }
    return true;
}

PolicyRerunPlanEntry make_plan_entry(const PolicyGateFamilyResult& family) {
    PolicyRerunPlanEntry entry;
    entry.family = family.family;
    entry.currentStatus = policy_gate_status_name(family.status);
    entry.freshnessStatus = policy_freshness_status_name(family.freshnessStatus);
    entry.countsAsProductionEvidence = family.countsAsProductionEvidence;

    if (family.status == PolicyGateStatus::PASS) {
        entry.rerunKind = "direct_compare_rerun";
        entry.evidenceMode = "campaign_compare";
        entry.campaignConfigPath = recommended_campaign_config(family.family);
        entry.recommendedCommand =
            "./raw_engine_tests --case campaign --campaign-config " + entry.campaignConfigPath +
            " --target-compared-states 32 --target-eligible-states 32 --stop-when-gate-passes";
        entry.expectedStopCriteria =
            "target-compared-states=32,target-eligible-states=32,semantic_disagreement=0,fallback=0";
        entry.statusImpact = "PASS family stale";
        return entry;
    }
    if (family.status == PolicyGateStatus::NON_APPLICABLE) {
        entry.rerunKind = "applicability_audit_rerun";
        entry.evidenceMode = "applicability_only";
        entry.recommendedCommand =
            "./raw_engine_tests --case planner_tie_organic_applicability_audit --artifact-dir artifacts/phase22_applicability";
        entry.expectedStopCriteria =
            "target-applicability-confidence=0.95,compare_eligible_state_count=0,dominant_reason=no_split_ready";
        entry.statusImpact = "NON_APPLICABLE family stale";
        return entry;
    }
    if (family.status == PolicyGateStatus::DIAGNOSTIC_ONLY) {
        entry.rerunKind = "lineage_audit_rerun";
        entry.evidenceMode = "diagnostic_lineage";
        entry.recommendedCommand =
            "./raw_engine_tests --case compare_ready_lineage_audit --artifact-dir artifacts/phase22_lineage";
        entry.expectedStopCriteria =
            "target-lineage-samples=16,same_semantic_class=same_final_state=same_trace_class,counts_as_production_evidence=false";
        entry.statusImpact = "DIAGNOSTIC_ONLY family stale";
        return entry;
    }

    entry.rerunKind = "manual_reclassification";
    entry.evidenceMode = "policy_gate";
    entry.recommendedCommand = "./raw_engine_tests --case policy_gate --gate-strict";
    entry.expectedStopCriteria = "family status must return to PASS, NON_APPLICABLE, or DIAGNOSTIC_ONLY";
    entry.statusImpact = "family requires manual policy reclassification";
    return entry;
}

vector<filesystem::path> sidecar_paths(const filesystem::path& jsonPath) {
    return {
        jsonPath,
        jsonPath.parent_path() / (jsonPath.stem().string() + ".txt"),
        jsonPath.parent_path() / (jsonPath.stem().string() + ".summary.txt"),
    };
}

string sidecar_suffix(const filesystem::path& path) {
    if (path.extension() == ".txt" && path.stem().extension() == ".summary") {
        return ".summary.txt";
    }
    return path.extension().string();
}

string family_snapshot(const PolicyGateManifest& manifest) {
    vector<string> values;
    for (const PolicyGateFamilyResult& family : manifest.families) {
        values.push_back(
            family.family + ":" + policy_gate_status_name(family.status) + "/" +
            policy_freshness_status_name(family.freshnessStatus)
        );
    }
    return csv_from_strings(values);
}

struct HistoryRecord {
    string baselineTag;
    string approvalTimestampUtc;
    string reportFile;
    string bundleFile;
    string baselineManifestPath;
    string promotedFromManifest;
    string familyStatusSnapshot;
    string recordPath;
};

void rebuild_baseline_history_index(const filesystem::path& historyDir) {
    vector<HistoryRecord> records;
    if (!filesystem::exists(historyDir)) {
        return;
    }
    for (const auto& entry : filesystem::directory_iterator(historyDir)) {
        if (!entry.is_regular_file() || entry.path().filename().string().find(".meta.txt") == string::npos) {
            continue;
        }
        const auto values = read_key_value_file(entry.path());
        HistoryRecord record;
        record.baselineTag = lookup_value(values, "baseline_tag");
        record.approvalTimestampUtc = lookup_value(values, "approval_timestamp_utc");
        record.reportFile = lookup_value(values, "report_file");
        record.bundleFile = lookup_value(values, "bundle_file");
        record.baselineManifestPath = lookup_value(values, "baseline_manifest_path");
        record.promotedFromManifest = lookup_value(values, "promoted_from_manifest");
        record.familyStatusSnapshot = lookup_value(values, "family_status_snapshot");
        record.recordPath = filesystem::absolute(entry.path()).string();
        records.push_back(std::move(record));
    }

    sort(records.begin(), records.end(), [](const HistoryRecord& lhs, const HistoryRecord& rhs) {
        return lhs.approvalTimestampUtc < rhs.approvalTimestampUtc;
    });

    {
        ofstream txt(historyDir / "baseline_history_index.txt");
        if (!txt) {
            throw runtime_error("failed to write baseline history index");
        }
        txt << "entry_count=" << records.size() << '\n';
        for (const HistoryRecord& record : records) {
            txt << '\n';
            txt << "baseline_tag=" << record.baselineTag << '\n';
            txt << "approval_timestamp_utc=" << record.approvalTimestampUtc << '\n';
            txt << "report_file=" << record.reportFile << '\n';
            txt << "bundle_file=" << record.bundleFile << '\n';
            txt << "baseline_manifest_path=" << record.baselineManifestPath << '\n';
            txt << "promoted_from_manifest=" << record.promotedFromManifest << '\n';
            txt << "family_status_snapshot=" << record.familyStatusSnapshot << '\n';
            txt << "record_path=" << record.recordPath << '\n';
        }
    }
    filesystem::copy_file(
        historyDir / "baseline_history_index.txt",
        historyDir / "history_index.txt",
        filesystem::copy_options::overwrite_existing
    );

    {
        ofstream json(historyDir / "baseline_history_index.json");
        if (!json) {
            throw runtime_error("failed to write baseline history json");
        }
        json << "{\n  \"entry_count\":" << records.size() << ",\n  \"entries\":[\n";
        for (size_t i = 0; i < records.size(); ++i) {
            const HistoryRecord& record = records[i];
            json << "    {\n";
            json << "      \"baseline_tag\":\"" << json_escape(record.baselineTag) << "\",\n";
            json << "      \"approval_timestamp_utc\":\"" << json_escape(record.approvalTimestampUtc) << "\",\n";
            json << "      \"report_file\":\"" << json_escape(record.reportFile) << "\",\n";
            json << "      \"bundle_file\":\"" << json_escape(record.bundleFile) << "\",\n";
            json << "      \"baseline_manifest_path\":\"" << json_escape(record.baselineManifestPath) << "\",\n";
            json << "      \"promoted_from_manifest\":\"" << json_escape(record.promotedFromManifest) << "\",\n";
            json << "      \"family_status_snapshot\":\"" << json_escape(record.familyStatusSnapshot) << "\",\n";
            json << "      \"record_path\":\"" << json_escape(record.recordPath) << "\"\n";
            json << "    }";
            if (i + 1U != records.size()) {
                json << ',';
            }
            json << '\n';
        }
        json << "  ]\n}\n";
    }
    filesystem::copy_file(
        historyDir / "baseline_history_index.json",
        historyDir / "history_index.json",
        filesystem::copy_options::overwrite_existing
    );
}

} // namespace

bool policy_gate_status_acceptable_for_baseline(PolicyGateStatus status) {
    return status == PolicyGateStatus::PASS ||
        status == PolicyGateStatus::NON_APPLICABLE ||
        status == PolicyGateStatus::DIAGNOSTIC_ONLY;
}

bool policy_manifest_has_expected_family_set(const PolicyGateManifest& manifest) {
    static constexpr array<const char*, 4> kExpectedFamilies = {
        "split_tie_organic_symmetric",
        "automorphism_probe_large",
        "planner_tie_mixed_organic",
        "planner_tie_mixed_organic_compare_ready",
    };
    if (manifest.families.size() != kExpectedFamilies.size()) {
        return false;
    }
    return all_of(kExpectedFamilies.begin(), kExpectedFamilies.end(), [&](const char* familyName) {
        return any_of(manifest.families.begin(), manifest.families.end(), [&](const PolicyGateFamilyResult& family) {
            return family.family == familyName;
        });
    });
}

bool policy_manifest_has_complete_provenance(const PolicyGateManifest& manifest) {
    return all_of(manifest.families.begin(), manifest.families.end(), [](const PolicyGateFamilyResult& family) {
        return !family.relevantInputHashes.combinedHash.empty();
    });
}

bool policy_manifest_acceptable_for_baseline(const PolicyGateManifest& manifest) {
    if (!policy_manifest_has_expected_family_set(manifest)) {
        return false;
    }
    return all_of(manifest.families.begin(), manifest.families.end(), [](const PolicyGateFamilyResult& family) {
        if (!policy_gate_status_acceptable_for_baseline(family.status)) {
            return false;
        }
        if (family.freshnessStatus != PolicyFreshnessStatus::FRESH) {
            return false;
        }
        if (family.driftFlag || family.reclassifyRequired) {
            return false;
        }
        if (family.family == "planner_tie_mixed_organic_compare_ready" && family.countsAsProductionEvidence) {
            return false;
        }
        return true;
    });
}

PolicyGateManifest promote_policy_gate_baseline(
    const PolicyGateManifest& sourceManifest,
    const filesystem::path& sourceRoot,
    const filesystem::path& sourceManifestPath,
    const filesystem::path& baselineOutputPath,
    const PolicyBaselinePromotionOptions& options
) {
    (void)baselineOutputPath;
    if (options.requireAcceptableStatus && !policy_manifest_acceptable_for_baseline(sourceManifest)) {
        throw runtime_error("policy baseline promotion rejected: manifest contains unacceptable family state");
    }
    if (options.freezeProvenance && !policy_manifest_has_complete_provenance(sourceManifest)) {
        throw runtime_error("policy baseline promotion rejected: source manifest lacks complete provenance");
    }

    PolicyGateManifest promoted = sourceManifest;
    promoted.manifestRole = "baseline";
    promoted.baselineVersion = sourceManifest.manifestVersion;
    promoted.promotedFromManifest = filesystem::absolute(sourceManifestPath).string();
    promoted.promotedFromReport = resolve_promoted_report_path(sourceRoot, sourceManifest.reportVersion);
    promoted.baselineTag = options.baselineTag.empty()
        ? sourceManifest.reportVersion + "-approved"
        : options.baselineTag;
    promoted.approvalTimestampUtc = timestamp_utc_now();
    promoted.timestampUtc = promoted.approvalTimestampUtc;
    promoted.provenanceFrozen = options.freezeProvenance;
    promoted.baselineManifestPath.clear();
    promoted.baselineManifestHash.clear();
    promoted.staleFamilies.clear();
    promoted.reclassifyRequiredFamilies.clear();
    promoted.freshFamilyCount = promoted.families.size();
    promoted.staleFamilyCount = 0U;
    promoted.requiresRerunFamilyCount = 0U;
    promoted.reclassifyRequiredCount = 0U;
    promoted.revalidatedFamilyCount = 0U;
    for (PolicyGateFamilyResult& family : promoted.families) {
        family.freshnessStatus = PolicyFreshnessStatus::FRESH;
        family.freshnessRationale = "approved baseline provenance frozen at promotion";
        family.driftFlag = false;
        family.reclassifyRequired = false;
    }
    if (!options.freezeProvenance) {
        assign_policy_manifest_input_hashes(promoted, sourceRoot);
    }
    promoted.currentManifestHash = hash_policy_manifest_content(promoted);
    return promoted;
}

PolicyRerunPlan build_policy_rerun_plan(
    const PolicyGateManifest& refreshManifest,
    const filesystem::path& baselineManifestPath,
    const filesystem::path& currentManifestPath,
    const filesystem::path& refreshManifestPath,
    const PolicyRerunPlanOptions& options
) {
    PolicyRerunPlan plan;
    plan.generatedAtUtc = timestamp_utc_now();
    plan.artifactRoot = refreshManifest.artifactRoot;
    plan.baselineManifestPath = baselineManifestPath.empty() ? string() : filesystem::absolute(baselineManifestPath).string();
    plan.currentManifestPath = currentManifestPath.empty() ? string() : filesystem::absolute(currentManifestPath).string();
    plan.refreshManifestPath = refreshManifestPath.empty() ? string() : filesystem::absolute(refreshManifestPath).string();
    plan.baselineManifestHash = baselineManifestPath.empty() ? string() : hash_policy_manifest_file(baselineManifestPath);
    plan.currentManifestHash = currentManifestPath.empty() ? string() : hash_policy_manifest_file(currentManifestPath);
    plan.refreshManifestHash = refreshManifestPath.empty() ? hash_policy_manifest_content(refreshManifest) : hash_policy_manifest_file(refreshManifestPath);

    for (const PolicyGateFamilyResult& family : refreshManifest.families) {
        if (family.freshnessStatus == PolicyFreshnessStatus::STALE) {
            ++plan.staleFamilyCount;
            append_unique(plan.staleFamilies, family.family);
        } else if (family.freshnessStatus == PolicyFreshnessStatus::REQUIRES_RERUN) {
            ++plan.requiresRerunFamilyCount;
            append_unique(plan.requiresRerunFamilies, family.family);
        }
        if (family.reclassifyRequired) {
            ++plan.reclassifyRequiredCount;
        }
        if (family.freshnessStatus == PolicyFreshnessStatus::FRESH) {
            continue;
        }
        if (!family_selected_for_plan(family, options)) {
            append_unique(plan.omittedFamilies, family.family);
            continue;
        }
        plan.entries.push_back(make_plan_entry(family));
    }

    sort(plan.entries.begin(), plan.entries.end(), [](const PolicyRerunPlanEntry& lhs, const PolicyRerunPlanEntry& rhs) {
        return lhs.family < rhs.family;
    });
    plan.selectedEntryCount = plan.entries.size();

    if (plan.staleFamilyCount == 0U && plan.requiresRerunFamilyCount == 0U) {
        plan.summaryVerdict = "PASS";
        plan.rationale = "no stale or requires_rerun families detected";
    } else if (plan.entries.empty()) {
        plan.summaryVerdict = "RERUN_REQUIRED";
        plan.rationale = "stale families exist but filters excluded every rerun entry";
    } else {
        plan.summaryVerdict = "RERUN_REQUIRED";
        plan.rationale = "rerun entries generated for stale or requires_rerun families";
    }
    return plan;
}

void apply_policy_synthetic_mutations(
    PolicyGateManifest& manifest,
    const PolicySyntheticMutationOptions& options
) {
    if (!options.hashDriftField.empty()) {
        for (PolicyGateFamilyResult& family : manifest.families) {
            if (options.hashDriftField == "planner_semantics_hash") {
                family.relevantInputHashes.plannerSemanticsHash += "-synthetic";
            } else if (options.hashDriftField == "generator_family_hash") {
                family.relevantInputHashes.generatorFamilyHash += "-synthetic";
            } else if (options.hashDriftField == "compare_engine_hash") {
                family.relevantInputHashes.compareEngineHash += "-synthetic";
            } else if (options.hashDriftField == "campaign_config_hash") {
                family.relevantInputHashes.campaignConfigHash += "-synthetic";
            } else {
                throw runtime_error("unknown synthetic hash drift field: " + options.hashDriftField);
            }
            family.relevantInputHashes.combinedHash = stable_hash_text(
                family.relevantInputHashes.plannerSemanticsHash + "|" +
                family.relevantInputHashes.generatorFamilyHash + "|" +
                family.relevantInputHashes.compareEngineHash + "|" +
                family.relevantInputHashes.campaignConfigHash
            );
        }
    }

    if (!options.applicabilityDriftFamily.empty()) {
        auto it = find_if(manifest.families.begin(), manifest.families.end(), [&](const PolicyGateFamilyResult& family) {
            return family.family == options.applicabilityDriftFamily;
        });
        if (it == manifest.families.end()) {
            throw runtime_error("synthetic applicability drift family missing: " + options.applicabilityDriftFamily);
        }
        it->measured.generatedStateCount = max<size_t>(it->measured.generatedStateCount, 48U);
        it->measured.splitReadyStateCount = max<size_t>(it->measured.splitReadyStateCount, 3U);
        it->measured.tieReadyStateCount = max<size_t>(it->measured.tieReadyStateCount, 2U);
        it->measured.compareEligibleStateCount = max<size_t>(it->measured.compareEligibleStateCount, 2U);
        it->measured.compareRelevance = max(it->measured.compareRelevance, 2.0 / 48.0);
        it->measured.splitReadyRelevance = max(it->measured.splitReadyRelevance, 3.0 / 48.0);
        it->measured.dominantIneligibleReason = "single_admissible_pair";
        it->measured.dominantIneligibleReasonConfidence = 0.5;
        it->driftFlag = true;
        it->reclassifyRequired = true;
    }

    if (!options.diagnosticPromotionFamily.empty()) {
        auto it = find_if(manifest.families.begin(), manifest.families.end(), [&](const PolicyGateFamilyResult& family) {
            return family.family == options.diagnosticPromotionFamily;
        });
        if (it == manifest.families.end()) {
            throw runtime_error("synthetic diagnostic promotion family missing: " + options.diagnosticPromotionFamily);
        }
        it->countsAsProductionEvidence = true;
        it->reclassifyRequired = true;
    }

    manifest.currentManifestHash = hash_policy_manifest_content(manifest);
}

string policy_rerun_plan_text(const PolicyRerunPlan& plan) {
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
    oss << "stale_family_count=" << plan.staleFamilyCount << '\n';
    oss << "requires_rerun_family_count=" << plan.requiresRerunFamilyCount << '\n';
    oss << "reclassify_required_count=" << plan.reclassifyRequiredCount << '\n';
    oss << "selected_entry_count=" << plan.selectedEntryCount << '\n';
    oss << "stale_families=" << csv_from_strings(plan.staleFamilies) << '\n';
    oss << "requires_rerun_families=" << csv_from_strings(plan.requiresRerunFamilies) << '\n';
    oss << "omitted_families=" << csv_from_strings(plan.omittedFamilies) << '\n';
    oss << "summary_verdict=" << plan.summaryVerdict << '\n';
    oss << "rationale=" << plan.rationale << '\n';
    for (const PolicyRerunPlanEntry& entry : plan.entries) {
        oss << '\n';
        oss << "family=" << entry.family << '\n';
        oss << "current_status=" << entry.currentStatus << '\n';
        oss << "freshness_status=" << entry.freshnessStatus << '\n';
        oss << "rerun_kind=" << entry.rerunKind << '\n';
        oss << "evidence_mode=" << entry.evidenceMode << '\n';
        oss << "campaign_config_path=" << entry.campaignConfigPath << '\n';
        oss << "recommended_command=" << entry.recommendedCommand << '\n';
        oss << "expected_stop_criteria=" << entry.expectedStopCriteria << '\n';
        oss << "status_impact=" << entry.statusImpact << '\n';
        oss << "counts_as_production_evidence=" << (entry.countsAsProductionEvidence ? 1 : 0) << '\n';
    }
    return oss.str();
}

string policy_rerun_plan_json(const PolicyRerunPlan& plan) {
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
    oss << "  \"stale_family_count\":" << plan.staleFamilyCount << ",\n";
    oss << "  \"requires_rerun_family_count\":" << plan.requiresRerunFamilyCount << ",\n";
    oss << "  \"reclassify_required_count\":" << plan.reclassifyRequiredCount << ",\n";
    oss << "  \"selected_entry_count\":" << plan.selectedEntryCount << ",\n";
    oss << "  \"stale_families\":[";
    for (size_t i = 0; i < plan.staleFamilies.size(); ++i) {
        if (i != 0U) oss << ',';
        oss << '"' << json_escape(plan.staleFamilies[i]) << '"';
    }
    oss << "],\n";
    oss << "  \"requires_rerun_families\":[";
    for (size_t i = 0; i < plan.requiresRerunFamilies.size(); ++i) {
        if (i != 0U) oss << ',';
        oss << '"' << json_escape(plan.requiresRerunFamilies[i]) << '"';
    }
    oss << "],\n";
    oss << "  \"summary_verdict\":\"" << json_escape(plan.summaryVerdict) << "\",\n";
    oss << "  \"rationale\":\"" << json_escape(plan.rationale) << "\",\n";
    oss << "  \"entries\":[\n";
    for (size_t i = 0; i < plan.entries.size(); ++i) {
        const auto& entry = plan.entries[i];
        oss << "    {\n";
        oss << "      \"family\":\"" << json_escape(entry.family) << "\",\n";
        oss << "      \"current_status\":\"" << json_escape(entry.currentStatus) << "\",\n";
        oss << "      \"freshness_status\":\"" << json_escape(entry.freshnessStatus) << "\",\n";
        oss << "      \"rerun_kind\":\"" << json_escape(entry.rerunKind) << "\",\n";
        oss << "      \"evidence_mode\":\"" << json_escape(entry.evidenceMode) << "\",\n";
        oss << "      \"campaign_config_path\":\"" << json_escape(entry.campaignConfigPath) << "\",\n";
        oss << "      \"recommended_command\":\"" << json_escape(entry.recommendedCommand) << "\",\n";
        oss << "      \"expected_stop_criteria\":\"" << json_escape(entry.expectedStopCriteria) << "\",\n";
        oss << "      \"status_impact\":\"" << json_escape(entry.statusImpact) << "\",\n";
        oss << "      \"counts_as_production_evidence\":" << (entry.countsAsProductionEvidence ? "true" : "false") << '\n';
        oss << "    }";
        if (i + 1U != plan.entries.size()) {
            oss << ',';
        }
        oss << '\n';
    }
    oss << "  ]\n}\n";
    return oss.str();
}

string policy_rerun_plan_summary(const PolicyRerunPlan& plan) {
    ostringstream oss;
    oss << "policy_rerun_plan_summary"
        << " summary_verdict=" << plan.summaryVerdict
        << " stale_family_count=" << plan.staleFamilyCount
        << " requires_rerun_family_count=" << plan.requiresRerunFamilyCount
        << " reclassify_required_count=" << plan.reclassifyRequiredCount
        << " selected_entry_count=" << plan.selectedEntryCount
        << '\n';
    for (const auto& entry : plan.entries) {
        oss << "family=" << entry.family
            << " current_status=" << entry.currentStatus
            << " freshness_status=" << entry.freshnessStatus
            << " rerun_kind=" << entry.rerunKind
            << " evidence_mode=" << entry.evidenceMode
            << '\n';
    }
    return oss.str();
}

void write_policy_rerun_plan_outputs(const filesystem::path& jsonPath, const PolicyRerunPlan& plan) {
    filesystem::create_directories(jsonPath.parent_path());
    ofstream json(jsonPath);
    if (!json) {
        throw runtime_error("failed to write policy rerun plan json: " + jsonPath.string());
    }
    json << policy_rerun_plan_json(plan);

    const filesystem::path txtPath = jsonPath.parent_path() / (jsonPath.stem().string() + ".txt");
    ofstream txt(txtPath);
    if (!txt) {
        throw runtime_error("failed to write policy rerun plan text: " + txtPath.string());
    }
    txt << policy_rerun_plan_text(plan);

    const filesystem::path summaryPath = jsonPath.parent_path() / (jsonPath.stem().string() + ".summary.txt");
    ofstream summary(summaryPath);
    if (!summary) {
        throw runtime_error("failed to write policy rerun plan summary: " + summaryPath.string());
    }
    summary << policy_rerun_plan_summary(plan);
}

PolicyRerunPlan load_policy_rerun_plan_text(const filesystem::path& planPath) {
    ifstream ifs(planPath);
    if (!ifs) {
        throw runtime_error("failed to read policy rerun plan: " + planPath.string());
    }
    PolicyRerunPlan plan;
    PolicyRerunPlanEntry* current = nullptr;
    string line;
    while (getline(ifs, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const size_t eq = line.find('=');
        if (eq == string::npos) {
            continue;
        }
        const string key = line.substr(0U, eq);
        const string value = line.substr(eq + 1U);
        if (key == "plan_version") {
            plan.planVersion = value;
        } else if (key == "generated_at_utc") {
            plan.generatedAtUtc = value;
        } else if (key == "artifact_root") {
            plan.artifactRoot = value;
        } else if (key == "baseline_manifest_path") {
            plan.baselineManifestPath = value;
        } else if (key == "current_manifest_path") {
            plan.currentManifestPath = value;
        } else if (key == "refresh_manifest_path") {
            plan.refreshManifestPath = value;
        } else if (key == "baseline_manifest_hash") {
            plan.baselineManifestHash = value;
        } else if (key == "current_manifest_hash") {
            plan.currentManifestHash = value;
        } else if (key == "refresh_manifest_hash") {
            plan.refreshManifestHash = value;
        } else if (key == "stale_family_count") {
            plan.staleFamilyCount = static_cast<size_t>(stoull(value));
        } else if (key == "requires_rerun_family_count") {
            plan.requiresRerunFamilyCount = static_cast<size_t>(stoull(value));
        } else if (key == "reclassify_required_count") {
            plan.reclassifyRequiredCount = static_cast<size_t>(stoull(value));
        } else if (key == "selected_entry_count") {
            plan.selectedEntryCount = static_cast<size_t>(stoull(value));
        } else if (key == "stale_families") {
            plan.staleFamilies = split_csv(value);
        } else if (key == "requires_rerun_families") {
            plan.requiresRerunFamilies = split_csv(value);
        } else if (key == "omitted_families") {
            plan.omittedFamilies = split_csv(value);
        } else if (key == "summary_verdict") {
            plan.summaryVerdict = value;
        } else if (key == "rationale") {
            plan.rationale = value;
        } else if (key == "family") {
            plan.entries.push_back(PolicyRerunPlanEntry{});
            current = &plan.entries.back();
            current->family = value;
        } else if (current != nullptr) {
            if (key == "current_status") current->currentStatus = value;
            else if (key == "freshness_status") current->freshnessStatus = value;
            else if (key == "rerun_kind") current->rerunKind = value;
            else if (key == "evidence_mode") current->evidenceMode = value;
            else if (key == "campaign_config_path") current->campaignConfigPath = value;
            else if (key == "recommended_command") current->recommendedCommand = value;
            else if (key == "expected_stop_criteria") current->expectedStopCriteria = value;
            else if (key == "status_impact") current->statusImpact = value;
            else if (key == "counts_as_production_evidence") current->countsAsProductionEvidence = (value == "1" || value == "true");
        }
    }
    if (plan.selectedEntryCount == 0U) {
        plan.selectedEntryCount = plan.entries.size();
    }
    return plan;
}

string policy_rerun_execution_summary_text(const PolicyRerunExecutionSummary& summary) {
    ostringstream oss;
    oss << "policy_rerun_execution_summary"
        << " summary_verdict=" << summary.summaryVerdict
        << " selected_family_count=" << summary.selectedFamilyCount
        << " executed_family_count=" << summary.executedFamilyCount
        << " noop_family_count=" << summary.noopFamilyCount
        << " failed_family_count=" << summary.failedFamilies.size()
        << " artifact_root=" << summary.artifactRoot
        << " rationale=" << summary.rationale
        << '\n';
    if (!summary.executedFamilies.empty()) {
        oss << "executed_families=" << csv_from_strings(summary.executedFamilies) << '\n';
    }
    if (!summary.noopFamilies.empty()) {
        oss << "noop_families=" << csv_from_strings(summary.noopFamilies) << '\n';
    }
    if (!summary.failedFamilies.empty()) {
        oss << "failed_families=" << csv_from_strings(summary.failedFamilies) << '\n';
    }
    return oss.str();
}

void write_policy_baseline_outputs_with_history(const filesystem::path& jsonPath, const PolicyGateManifest& baseline) {
    filesystem::create_directories(jsonPath.parent_path());
    const filesystem::path historyDir = jsonPath.parent_path() / "history";
    filesystem::create_directories(historyDir);

    const string stamp = sanitize_token(baseline.approvalTimestampUtc.empty() ? timestamp_utc_now() : baseline.approvalTimestampUtc);
    const string tag = sanitize_token(baseline.baselineTag.empty() ? string("approved") : baseline.baselineTag);

    if (filesystem::exists(jsonPath)) {
        for (const filesystem::path& sidecar : sidecar_paths(jsonPath)) {
            if (!filesystem::exists(sidecar)) {
                continue;
            }
            filesystem::copy_file(
                sidecar,
                historyDir / (jsonPath.stem().string() + "_" + stamp + "_previous" + sidecar_suffix(sidecar)),
                filesystem::copy_options::overwrite_existing
            );
        }
    }

    write_policy_gate_outputs(jsonPath, baseline);
    for (const filesystem::path& sidecar : sidecar_paths(jsonPath)) {
        if (!filesystem::exists(sidecar)) {
            continue;
        }
        filesystem::copy_file(
            sidecar,
            historyDir / (jsonPath.stem().string() + "_" + stamp + "_" + tag + sidecar_suffix(sidecar)),
            filesystem::copy_options::overwrite_existing
        );
    }

    const filesystem::path recordPath = historyDir / (jsonPath.stem().string() + "_" + stamp + "_" + tag + ".meta.txt");
    ofstream record(recordPath);
    if (!record) {
        throw runtime_error("failed to write baseline history record: " + recordPath.string());
    }
    record << "baseline_tag=" << baseline.baselineTag << '\n';
    record << "approval_timestamp_utc=" << baseline.approvalTimestampUtc << '\n';
    record << "report_file=" << baseline.promotedFromReport << '\n';
    record << "bundle_file=" << resolve_promoted_bundle_path(baseline.promotedFromReport) << '\n';
    record << "baseline_manifest_path=" << filesystem::absolute(jsonPath).string() << '\n';
    record << "promoted_from_manifest=" << baseline.promotedFromManifest << '\n';
    record << "family_status_snapshot=" << family_snapshot(baseline) << '\n';

    rebuild_baseline_history_index(historyDir);
}
