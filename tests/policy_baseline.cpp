#include "policy_baseline.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
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

string uppercase_copy(string value) {
    transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(toupper(ch));
    });
    return value;
}

string trim_copy(string value) {
    value.erase(value.begin(), find_if(value.begin(), value.end(), [](unsigned char ch) {
        return !isspace(ch);
    }));
    value.erase(find_if(value.rbegin(), value.rend(), [](unsigned char ch) {
        return !isspace(ch);
    }).base(), value.end());
    return value;
}

bool parse_bool_text_impl(const string& value) {
    const string upper = uppercase_copy(trim_copy(value));
    return upper == "1" || upper == "TRUE" || upper == "YES";
}

string json_escape(const string& value) {
    ostringstream oss;
    for (const char ch : value) {
        switch (ch) {
            case '\\':
                oss << "\\\\";
                break;
            case '"':
                oss << "\\\"";
                break;
            case '\n':
                oss << "\\n";
                break;
            case '\r':
                oss << "\\r";
                break;
            case '\t':
                oss << "\\t";
                break;
            default:
                oss << ch;
                break;
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

string json_array_from_strings(const vector<string>& values) {
    ostringstream oss;
    oss << '[';
    for (size_t i = 0; i < values.size(); ++i) {
        if (i != 0U) {
            oss << ',';
        }
        oss << '"' << json_escape(values[i]) << '"';
    }
    oss << ']';
    return oss.str();
}

vector<string> split_csv(const string& csv) {
    vector<string> values;
    size_t start = 0U;
    while (start <= csv.size()) {
        const size_t comma = csv.find(',', start);
        const string token = trim_copy(csv.substr(start, comma == string::npos ? string::npos : comma - start));
        if (!token.empty()) {
            values.push_back(token);
        }
        if (comma == string::npos) {
            break;
        }
        start = comma + 1U;
    }
    return values;
}

bool contains_string(const vector<string>& values, const string& value) {
    return find(values.begin(), values.end(), value) != values.end();
}

void append_unique(vector<string>& values, const string& value) {
    if (!value.empty() && !contains_string(values, value)) {
        values.push_back(value);
    }
}

unordered_map<string, string> read_key_value_file_if_exists(const filesystem::path& path) {
    unordered_map<string, string> values;
    ifstream ifs(path);
    if (!ifs) {
        return values;
    }
    string line;
    while (getline(ifs, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const size_t eq = line.find('=');
        if (eq == string::npos) {
            continue;
        }
        values.emplace(trim_copy(line.substr(0U, eq)), trim_copy(line.substr(eq + 1U)));
    }
    return values;
}

bool family_has_complete_provenance(const PolicyGateFamilyResult& family) {
    return !family.relevantInputHashes.plannerSemanticsHash.empty() &&
        !family.relevantInputHashes.generatorFamilyHash.empty() &&
        !family.relevantInputHashes.compareEngineHash.empty() &&
        !family.relevantInputHashes.combinedHash.empty();
}

string resolve_promoted_report_path(const filesystem::path& sourceRoot, const string& reportVersion) {
    const string prefix = uppercase_copy(reportVersion) + "_STABILIZATION_REPORT";
    filesystem::path latestMatch;
    filesystem::file_time_type latestTime{};
    bool found = false;
    for (const auto& entry : filesystem::directory_iterator(sourceRoot)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const string filename = entry.path().filename().string();
        if (filename.rfind(prefix, 0U) != 0U || entry.path().extension() != ".txt") {
            continue;
        }
        const auto writeTime = entry.last_write_time();
        if (!found || writeTime > latestTime) {
            latestMatch = entry.path();
            latestTime = writeTime;
            found = true;
        }
    }
    return found ? filesystem::absolute(latestMatch).string() : string();
}

string resolve_promoted_bundle_path(const filesystem::path& sourceRoot, const string& reportVersion) {
    const string prefix = "raw_engine_" + reportVersion + "_stabilization";
    filesystem::path latestMatch;
    filesystem::file_time_type latestTime{};
    bool found = false;
    for (const auto& entry : filesystem::directory_iterator(sourceRoot)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".zip") {
            continue;
        }
        const string filename = entry.path().filename().string();
        if (filename.rfind(prefix, 0U) != 0U) {
            continue;
        }
        const auto writeTime = entry.last_write_time();
        if (!found || writeTime > latestTime) {
            latestMatch = entry.path();
            latestTime = writeTime;
            found = true;
        }
    }
    return found ? filesystem::absolute(latestMatch).string() : string();
}

string recommended_campaign_config(const string& family) {
    if (family == "split_tie_organic_symmetric") {
        return "tests/campaigns/phase19_split_tie_organic_compare.txt";
    }
    if (family == "automorphism_probe_large") {
        return "tests/campaigns/phase19_automorphism_compare.txt";
    }
    if (family == "planner_tie_mixed_organic") {
        return "tests/campaigns/phase19_planner_tie_gap_audit.txt";
    }
    if (family == "planner_tie_mixed_organic_compare_ready") {
        return "tests/campaigns/phase19_planner_tie_compare_ready.txt";
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
        entry.campaignConfigPath = recommended_campaign_config(family.family);
        entry.recommendedCommand =
            "./raw_engine_tests --case planner_tie_organic_applicability_audit --artifact-dir artifacts/phase20_applicability";
        entry.expectedStopCriteria =
            "target-applicability-confidence=0.95,compare_eligible_state_count=0,dominant_reason=no_split_ready";
        entry.statusImpact = "NON_APPLICABLE family stale";
        return entry;
    }
    if (family.status == PolicyGateStatus::DIAGNOSTIC_ONLY) {
        entry.rerunKind = "lineage_audit_rerun";
        entry.evidenceMode = "diagnostic_lineage";
        entry.campaignConfigPath = recommended_campaign_config(family.family);
        entry.recommendedCommand =
            "./raw_engine_tests --case compare_ready_lineage_audit --artifact-dir artifacts/phase20_lineage";
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

void update_combined_hash(PolicyGateFamilyResult& family) {
    family.relevantInputHashes.combinedHash = stable_hash_text(
        family.relevantInputHashes.plannerSemanticsHash + "|" +
        family.relevantInputHashes.generatorFamilyHash + "|" +
        family.relevantInputHashes.compareEngineHash + "|" +
        family.relevantInputHashes.campaignConfigHash);
}

filesystem::path sibling_text_path(const filesystem::path& jsonPath) {
    return jsonPath.parent_path() / (jsonPath.stem().string() + ".txt");
}

filesystem::path sibling_summary_path(const filesystem::path& jsonPath) {
    return jsonPath.parent_path() / (jsonPath.stem().string() + ".summary.txt");
}

vector<filesystem::path> manifest_sidecars(const filesystem::path& jsonPath) {
    return {jsonPath, sibling_text_path(jsonPath), sibling_summary_path(jsonPath)};
}

void copy_if_exists(const filesystem::path& from, const filesystem::path& to) {
    if (!filesystem::exists(from)) {
        return;
    }
    filesystem::create_directories(to.parent_path());
    filesystem::copy_file(from, to, filesystem::copy_options::overwrite_existing);
}

string timestamp_token(const string& timestamp) {
    string token;
    for (const char ch : timestamp) {
        if (isalnum(static_cast<unsigned char>(ch))) {
            token.push_back(ch);
        }
    }
    return token.empty() ? string("baseline") : token;
}

} // namespace

bool policy_gate_status_acceptable_for_baseline(PolicyGateStatus status) {
    return status == PolicyGateStatus::PASS ||
        status == PolicyGateStatus::NON_APPLICABLE ||
        status == PolicyGateStatus::DIAGNOSTIC_ONLY;
}

bool policy_manifest_acceptable_for_baseline(const PolicyGateManifest& manifest) {
    if (manifest.families.empty()) {
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

    PolicyGateManifest promoted = sourceManifest;
    promoted.manifestRole = "baseline";
    promoted.baselineVersion = promoted.manifestVersion;
    promoted.promotedFromManifest = sourceManifestPath.empty()
        ? string()
        : filesystem::absolute(sourceManifestPath).string();
    promoted.promotedFromReport = resolve_promoted_report_path(sourceRoot, sourceManifest.reportVersion);
    promoted.baselineTag = options.baselineTag.empty()
        ? sourceManifest.reportVersion + "-approved"
        : options.baselineTag;
    promoted.approvalTimestampUtc = timestamp_utc_now();
    promoted.timestampUtc = promoted.approvalTimestampUtc;
    promoted.provenanceFrozen = options.freezeProvenance;
    promoted.baselineManifestPath.clear();
    promoted.baselineManifestHash.clear();
    promoted.freshFamilyCount = promoted.families.size();
    promoted.staleFamilyCount = 0U;
    promoted.requiresRerunFamilyCount = 0U;
    promoted.reclassifyRequiredCount = 0U;
    promoted.revalidatedFamilyCount = 0U;
    promoted.staleFamilies.clear();
    promoted.reclassifyRequiredFamilies.clear();

    bool missingProvenance = false;
    for (PolicyGateFamilyResult& family : promoted.families) {
        family.freshnessStatus = PolicyFreshnessStatus::FRESH;
        family.freshnessRationale = "approved baseline provenance frozen at promotion";
        family.reclassifyRequired = false;
        missingProvenance = missingProvenance || !family_has_complete_provenance(family);
    }
    if (!options.freezeProvenance || missingProvenance) {
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
    plan.baselineManifestPath = baselineManifestPath.empty()
        ? string()
        : filesystem::absolute(baselineManifestPath).string();
    plan.currentManifestPath = currentManifestPath.empty()
        ? string()
        : filesystem::absolute(currentManifestPath).string();
    plan.refreshManifestPath = refreshManifestPath.empty()
        ? string()
        : filesystem::absolute(refreshManifestPath).string();
    plan.baselineManifestHash = baselineManifestPath.empty() ? string() : hash_policy_manifest_file(baselineManifestPath);
    plan.currentManifestHash = currentManifestPath.empty() ? string() : hash_policy_manifest_file(currentManifestPath);
    plan.refreshManifestHash = refreshManifestPath.empty()
        ? hash_policy_manifest_content(refreshManifest)
        : hash_policy_manifest_file(refreshManifestPath);

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
        plan.rationale = "stale or requires_rerun families exist, but filters excluded them from the plan";
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
    for (PolicyGateFamilyResult& family : manifest.families) {
        if (!options.hashDriftField.empty()) {
            if (options.hashDriftField == "planner_semantics_hash") {
                family.relevantInputHashes.plannerSemanticsHash = "synthetic-planner-" + family.family;
            } else if (options.hashDriftField == "generator_family_hash") {
                family.relevantInputHashes.generatorFamilyHash = "synthetic-generator-" + family.family;
            } else if (options.hashDriftField == "compare_engine_hash") {
                family.relevantInputHashes.compareEngineHash = "synthetic-compare-" + family.family;
            } else if (options.hashDriftField == "campaign_config_hash") {
                family.relevantInputHashes.campaignConfigHash = "synthetic-campaign-" + family.family;
            } else {
                throw runtime_error("unknown synthetic hash drift field: " + options.hashDriftField);
            }
            update_combined_hash(family);
        }

        if (!options.applicabilityDriftFamily.empty() && family.family == options.applicabilityDriftFamily) {
            family.status = PolicyGateStatus::INSUFFICIENT_EVIDENCE;
            family.driftFlag = true;
            family.reclassifyRequired = true;
            family.measured.generatedStateCount = max<size_t>(family.measured.generatedStateCount, 48U);
            family.measured.splitReadyStateCount = max<size_t>(family.measured.splitReadyStateCount, 4U);
            family.measured.tieReadyStateCount = max<size_t>(family.measured.tieReadyStateCount, 3U);
            family.measured.compareEligibleStateCount = max<size_t>(family.measured.compareEligibleStateCount, 2U);
            family.measured.compareRelevance = 2.0 / 48.0;
            family.measured.splitReadyRelevance = 4.0 / 48.0;
            family.rationale = "synthetic applicability drift injection";
        }

        if (!options.diagnosticPromotionFamily.empty() && family.family == options.diagnosticPromotionFamily) {
            family.countsAsProductionEvidence = true;
            family.reclassifyRequired = true;
            family.rationale = "synthetic diagnostic promotion injection";
        }
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
    oss << "  \"stale_families\":" << json_array_from_strings(plan.staleFamilies) << ",\n";
    oss << "  \"requires_rerun_families\":" << json_array_from_strings(plan.requiresRerunFamilies) << ",\n";
    oss << "  \"omitted_families\":" << json_array_from_strings(plan.omittedFamilies) << ",\n";
    oss << "  \"summary_verdict\":\"" << json_escape(plan.summaryVerdict) << "\",\n";
    oss << "  \"rationale\":\"" << json_escape(plan.rationale) << "\",\n";
    oss << "  \"entries\":[\n";
    for (size_t i = 0; i < plan.entries.size(); ++i) {
        const PolicyRerunPlanEntry& entry = plan.entries[i];
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
    oss << "  ]\n";
    oss << "}\n";
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
    for (const PolicyRerunPlanEntry& entry : plan.entries) {
        oss << "family=" << entry.family
            << " current_status=" << entry.currentStatus
            << " freshness_status=" << entry.freshnessStatus
            << " rerun_kind=" << entry.rerunKind
            << " evidence_mode=" << entry.evidenceMode
            << " status_impact=" << entry.statusImpact
            << " expected_stop_criteria=" << entry.expectedStopCriteria
            << '\n';
    }
    return oss.str();
}

void write_policy_rerun_plan_outputs(const filesystem::path& jsonPath, const PolicyRerunPlan& plan) {
    filesystem::create_directories(jsonPath.parent_path());
    ofstream(jsonPath) << policy_rerun_plan_json(plan);
    ofstream(sibling_text_path(jsonPath)) << policy_rerun_plan_text(plan);
    ofstream(sibling_summary_path(jsonPath)) << policy_rerun_plan_summary(plan);
}

PolicyRerunPlan load_policy_rerun_plan_text(const filesystem::path& planPath) {
    ifstream ifs(planPath);
    if (!ifs) {
        throw runtime_error("failed to open policy rerun plan text: " + planPath.string());
    }

    PolicyRerunPlan plan;
    PolicyRerunPlanEntry currentEntry;
    bool inEntry = false;

    auto flush_entry = [&]() {
        if (inEntry && !currentEntry.family.empty()) {
            plan.entries.push_back(currentEntry);
        }
        currentEntry = PolicyRerunPlanEntry{};
        inEntry = false;
    };

    string line;
    while (getline(ifs, line)) {
        if (line.empty()) {
            flush_entry();
            continue;
        }
        const size_t eq = line.find('=');
        if (eq == string::npos) {
            continue;
        }
        const string key = trim_copy(line.substr(0, eq));
        const string value = trim_copy(line.substr(eq + 1U));
        if (key == "family") {
            flush_entry();
            inEntry = true;
            currentEntry.family = value;
            continue;
        }
        if (inEntry) {
            if (key == "current_status") {
                currentEntry.currentStatus = value;
            } else if (key == "freshness_status") {
                currentEntry.freshnessStatus = value;
            } else if (key == "rerun_kind") {
                currentEntry.rerunKind = value;
            } else if (key == "evidence_mode") {
                currentEntry.evidenceMode = value;
            } else if (key == "campaign_config_path") {
                currentEntry.campaignConfigPath = value;
            } else if (key == "recommended_command") {
                currentEntry.recommendedCommand = value;
            } else if (key == "expected_stop_criteria") {
                currentEntry.expectedStopCriteria = value;
            } else if (key == "status_impact") {
                currentEntry.statusImpact = value;
            } else if (key == "counts_as_production_evidence") {
                currentEntry.countsAsProductionEvidence = parse_bool_text_impl(value);
            }
            continue;
        }

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
        }
    }
    flush_entry();
    plan.selectedEntryCount = plan.entries.size();
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
        << '\n';
    oss << "executed_at_utc=" << summary.executedAtUtc << '\n';
    oss << "artifact_root=" << summary.artifactRoot << '\n';
    oss << "executed_families=" << csv_from_strings(summary.executedFamilies) << '\n';
    oss << "noop_families=" << csv_from_strings(summary.noopFamilies) << '\n';
    oss << "failed_families=" << csv_from_strings(summary.failedFamilies) << '\n';
    oss << "rationale=" << summary.rationale << '\n';
    return oss.str();
}

void write_policy_baseline_outputs_with_history(const filesystem::path& jsonPath, const PolicyGateManifest& baseline) {
    filesystem::create_directories(jsonPath.parent_path());
    const filesystem::path historyDir = jsonPath.parent_path() / "history";
    filesystem::create_directories(historyDir);

    if (filesystem::exists(jsonPath)) {
        const string archiveToken = timestamp_token(timestamp_utc_now());
        const filesystem::path archiveDir = historyDir / (jsonPath.stem().string() + "_" + archiveToken);
        for (const filesystem::path& sidecar : manifest_sidecars(jsonPath)) {
            copy_if_exists(sidecar, archiveDir / sidecar.filename());
        }
    }

    write_policy_gate_outputs(jsonPath, baseline);

    const string recordToken = timestamp_token(baseline.approvalTimestampUtc.empty() ? timestamp_utc_now() : baseline.approvalTimestampUtc);
    const string recordStem =
        (baseline.baselineTag.empty() ? string("baseline") : baseline.baselineTag) + "_" + recordToken;
    const filesystem::path metaPath = historyDir / (recordStem + ".meta.txt");
    const filesystem::path recordPath = historyDir / (recordStem + ".record.txt");
    ofstream meta(metaPath);
    if (!meta) {
        throw runtime_error("failed to write baseline history meta: " + metaPath.string());
    }
    meta << "baseline_tag=" << baseline.baselineTag << '\n';
    meta << "approval_timestamp_utc=" << baseline.approvalTimestampUtc << '\n';
    meta << "report_file=" << baseline.promotedFromReport << '\n';
    meta << "bundle_file=" << resolve_promoted_bundle_path(filesystem::absolute(jsonPath).parent_path().parent_path().parent_path(), baseline.reportVersion) << '\n';
    meta << "baseline_manifest=" << filesystem::absolute(jsonPath).string() << '\n';
    meta << "family_status_snapshot=";
    bool first = true;
    for (const PolicyGateFamilyResult& family : baseline.families) {
        if (!first) {
            meta << ',';
        }
        first = false;
        meta << family.family << ':' << policy_gate_status_name(family.status);
    }
    meta << '\n';
    meta.close();

    copy_if_exists(metaPath, recordPath);

    vector<filesystem::path> metaFiles;
    for (const auto& entry : filesystem::directory_iterator(historyDir)) {
        if (entry.is_regular_file() && entry.path().filename().string().find(".record.txt") != string::npos) {
            metaFiles.push_back(entry.path());
        }
    }
    sort(metaFiles.begin(), metaFiles.end());

    const filesystem::path historyIndexTxt = historyDir / "history_index.txt";
    const filesystem::path historyIndexJson = historyDir / "history_index.json";
    const filesystem::path baselineHistoryIndexTxt = historyDir / "baseline_history_index.txt";
    const filesystem::path baselineHistoryIndexJson = historyDir / "baseline_history_index.json";
    ofstream indexTxt(historyIndexTxt);
    ofstream indexJson(historyIndexJson);
    if (!indexTxt || !indexJson) {
        throw runtime_error("failed to write history index");
    }
    indexTxt << "entry_count=" << metaFiles.size() << '\n';
    indexJson << "{\n  \"entries\":[\n";
    for (size_t i = 0; i < metaFiles.size(); ++i) {
        const auto values = read_key_value_file_if_exists(metaFiles[i]);
        const string baselineTag = values.count("baseline_tag") != 0U ? values.at("baseline_tag") : string();
        const string approvalTimestamp = values.count("approval_timestamp_utc") != 0U ? values.at("approval_timestamp_utc") : string();
        const string reportFile = values.count("report_file") != 0U ? values.at("report_file") : string();
        const string bundleFile = values.count("bundle_file") != 0U ? values.at("bundle_file") : string();
        const string baselineManifest = values.count("baseline_manifest") != 0U ? values.at("baseline_manifest") : string();
        const string snapshot = values.count("family_status_snapshot") != 0U ? values.at("family_status_snapshot") : string();

        indexTxt << '\n';
        indexTxt << "baseline_tag=" << baselineTag << '\n';
        indexTxt << "approval_timestamp_utc=" << approvalTimestamp << '\n';
        indexTxt << "report_file=" << reportFile << '\n';
        indexTxt << "bundle_file=" << bundleFile << '\n';
        indexTxt << "baseline_manifest=" << baselineManifest << '\n';
        indexTxt << "family_status_snapshot=" << snapshot << '\n';

        indexJson << "    {\n";
        indexJson << "      \"baseline_tag\":\"" << json_escape(baselineTag) << "\",\n";
        indexJson << "      \"approval_timestamp_utc\":\"" << json_escape(approvalTimestamp) << "\",\n";
        indexJson << "      \"report_file\":\"" << json_escape(reportFile) << "\",\n";
        indexJson << "      \"bundle_file\":\"" << json_escape(bundleFile) << "\",\n";
        indexJson << "      \"baseline_manifest\":\"" << json_escape(baselineManifest) << "\",\n";
        indexJson << "      \"family_status_snapshot\":\"" << json_escape(snapshot) << "\"\n";
        indexJson << "    }";
        if (i + 1U != metaFiles.size()) {
            indexJson << ',';
        }
        indexJson << '\n';
    }
    indexJson << "  ]\n}\n";
    copy_if_exists(historyIndexTxt, baselineHistoryIndexTxt);
    copy_if_exists(historyIndexJson, baselineHistoryIndexJson);
}
