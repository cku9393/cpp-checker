#include "policy_gate.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <initializer_list>
#include <sstream>
#include <stdexcept>

using namespace std;

namespace {

string trim(string text) {
    const auto notSpace = [](unsigned char ch) {
        return !isspace(ch);
    };
    const auto beginIt = find_if(text.begin(), text.end(), notSpace);
    if (beginIt == text.end()) {
        return {};
    }
    const auto endIt = find_if(text.rbegin(), text.rend(), notSpace).base();
    return string(beginIt, endIt);
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
        const size_t equals = line.find('=');
        if (equals == string::npos) {
            continue;
        }
        values.emplace(trim(line.substr(0, equals)), trim(line.substr(equals + 1U)));
    }
    return values;
}

optional<string> find_value(const unordered_map<string, string>& values, const string& key) {
    const auto it = values.find(key);
    if (it == values.end()) {
        return nullopt;
    }
    return it->second;
}

size_t read_size(const unordered_map<string, string>& values, const string& key) {
    if (const optional<string> value = find_value(values, key); value.has_value()) {
        return static_cast<size_t>(stoull(*value));
    }
    return 0U;
}

double read_double(const unordered_map<string, string>& values, const string& key) {
    if (const optional<string> value = find_value(values, key); value.has_value()) {
        return stod(*value);
    }
    return 0.0;
}

string read_string(const unordered_map<string, string>& values, const string& key) {
    if (const optional<string> value = find_value(values, key); value.has_value()) {
        return *value;
    }
    return {};
}

unordered_map<string, size_t> parse_numeric_map(string text) {
    unordered_map<string, size_t> out;
    text = trim(std::move(text));
    if (text.size() < 2U || text.front() != '{' || text.back() != '}') {
        return out;
    }
    text = text.substr(1U, text.size() - 2U);
    size_t offset = 0U;
    while (offset < text.size()) {
        while (offset < text.size() && isspace(static_cast<unsigned char>(text[offset]))) {
            ++offset;
        }
        if (offset >= text.size()) {
            break;
        }
        if (text[offset] != '"') {
            break;
        }
        const size_t keyEnd = text.find('"', offset + 1U);
        if (keyEnd == string::npos) {
            break;
        }
        const string key = text.substr(offset + 1U, keyEnd - offset - 1U);
        const size_t colon = text.find(':', keyEnd + 1U);
        if (colon == string::npos) {
            break;
        }
        size_t valueEnd = text.find(',', colon + 1U);
        if (valueEnd == string::npos) {
            valueEnd = text.size();
        }
        const string valueText = trim(text.substr(colon + 1U, valueEnd - colon - 1U));
        out[key] = valueText.empty() ? 0U : static_cast<size_t>(stoull(valueText));
        offset = valueEnd + 1U;
    }
    return out;
}

string json_escape(const string& text) {
    ostringstream oss;
    for (char ch : text) {
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

string json_number(double value) {
    ostringstream oss;
    oss << fixed << setprecision(6) << value;
    return oss.str();
}

string json_object_from_map(const unordered_map<string, size_t>& values) {
    vector<pair<string, size_t>> ordered(values.begin(), values.end());
    sort(ordered.begin(), ordered.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
    });
    ostringstream oss;
    oss << '{';
    for (size_t i = 0; i < ordered.size(); ++i) {
        if (i != 0U) {
            oss << ',';
        }
        oss << '"' << json_escape(ordered[i].first) << "\":" << ordered[i].second;
    }
    oss << '}';
    return oss.str();
}

filesystem::path first_existing_path(const filesystem::path& artifactRoot, initializer_list<filesystem::path> rels) {
    for (const filesystem::path& rel : rels) {
        const filesystem::path candidate = artifactRoot / rel;
        if (filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return artifactRoot / *rels.begin();
}

string timestamp_utc_now() {
    const chrono::system_clock::time_point now = chrono::system_clock::now();
    const time_t tt = chrono::system_clock::to_time_t(now);
    tm utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &tt);
#else
    gmtime_r(&tt, &utc);
#endif
    ostringstream oss;
    oss << put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

FamilyApplicabilityClassification parse_applicability_classification(const string& text) {
    if (text == "DIRECTLY_APPLICABLE") {
        return FamilyApplicabilityClassification::DIRECTLY_APPLICABLE;
    }
    if (text == "NON_APPLICABLE") {
        return FamilyApplicabilityClassification::NON_APPLICABLE;
    }
    return FamilyApplicabilityClassification::UNDER_GENERATED;
}

FamilyApplicabilitySummary load_family_applicability_summary(const filesystem::path& summaryPath) {
    FamilyApplicabilitySummary summary;
    const unordered_map<string, string> values = read_key_value_file(summaryPath);
    summary.family = read_string(values, "family");
    summary.generatedStateCount = read_size(values, "generated_state_count");
    summary.splitReadyStateCount = read_size(values, "split_ready_state_count");
    summary.tieReadyStateCount = read_size(values, "tie_ready_state_count");
    summary.compareEligibleStateCount = read_size(values, "compare_eligible_state_count");
    summary.compareIneligibleStateCount = read_size(values, "compare_ineligible_state_count");
    summary.compareIneligibleReasonHistogram =
        parse_numeric_map(read_string(values, "compare_ineligible_reason_histogram"));
    summary.actualSplitHits = read_size(values, "actual_split_hits");
    summary.actualJoinHits = read_size(values, "actual_join_hits");
    summary.actualIntegrateHits = read_size(values, "actual_integrate_hits");
    summary.splitReadyRelevance = read_double(values, "split_ready_relevance");
    summary.tieReadyRelevance = read_double(values, "tie_ready_relevance");
    summary.compareRelevance = read_double(values, "compare_relevance");
    summary.dominantIneligibleReason = read_string(values, "dominant_ineligible_reason");
    summary.dominantIneligibleReasonConfidence = read_double(values, "dominant_ineligible_reason_confidence");
    summary.classification = parse_applicability_classification(read_string(values, "classification"));
    summary.thresholds.minGeneratedStates = read_size(values, "threshold_min_generated_states");
    summary.thresholds.maxCompareRelevanceForNonApplicable =
        read_double(values, "threshold_max_compare_relevance");
    summary.thresholds.maxSplitReadyRelevanceForNonApplicable =
        read_double(values, "threshold_max_split_ready_relevance");
    summary.thresholds.minDominantReasonConfidence =
        read_double(values, "threshold_min_dominant_reason_confidence");
    return summary;
}

CompareReadyLineageSummary load_compare_ready_lineage_summary(const filesystem::path& summaryPath) {
    CompareReadyLineageSummary summary;
    const unordered_map<string, string> values = read_key_value_file(summaryPath);
    summary.generatedStateCount = read_size(values, "generated_state_count");
    summary.compareEligibleStateCount = read_size(values, "compare_eligible_state_count");
    summary.compareIneligibleStateCount = read_size(values, "compare_ineligible_state_count");
    summary.compareIneligibleReasonHistogram =
        parse_numeric_map(read_string(values, "compare_ineligible_reason_histogram"));
    summary.noSplitReadyCount = read_size(values, "no_split_ready_count");
    summary.singleAdmissiblePairCount = read_size(values, "single_admissible_pair_count");
    summary.tieButFollowupAbsentCount = read_size(values, "tie_but_followup_absent_count");
    summary.keepOccAbsentCount = read_size(values, "keepOcc_absent_count");
    summary.boundaryArtifactAbsentCount = read_size(values, "boundary_artifact_absent_count");
    summary.derivedFromBaseStateCount = read_size(values, "derived_from_base_state_count");
    summary.compareReadyStateCount = read_size(values, "compare_ready_state_count");
    summary.compareCompletedStateCount = read_size(values, "compare_completed_state_count");
    summary.lineageSampleCount = read_size(values, "lineage_sample_count");
    summary.sameSemanticClassCount = read_size(values, "same_semantic_class_count");
    summary.sameFinalStateCount = read_size(values, "same_final_state_count");
    summary.sameTraceClassCount = read_size(values, "same_trace_class_count");
    summary.pairReplayCount = read_size(values, "pair_replay_count");
    summary.structureOnlyEnrichmentCount = read_size(values, "structure_only_enrichment_count");
    summary.addedBoundaryArtifactCount = read_size(values, "added_boundary_artifact_count");
    summary.injectedKeepOccSiblingCount = read_size(values, "injected_keepOcc_sibling_count");
    summary.forcedSplitReadySupportCount = read_size(values, "forced_split_ready_support_count");
    summary.followupPatternPreservedCount = read_size(values, "followup_pattern_preserved_count");
    summary.injectedStructureDeltaHistogram =
        parse_numeric_map(read_string(values, "injected_structure_delta_histogram"));
    summary.operationCountDeltaHistogram =
        parse_numeric_map(read_string(values, "operation_count_delta_histogram"));
    return summary;
}

void copy_compare_evidence(PolicyGateMeasuredEvidence& measured, const PolicyGateCompareEvidence& compareEvidence) {
    measured.comparedStateCount = compareEvidence.comparedStateCount;
    measured.compareEligibleStateCount = compareEvidence.compareEligibleStateCount;
    measured.compareIneligibleStateCount = compareEvidence.compareIneligibleStateCount;
    measured.compareIneligibleReasonHistogram = compareEvidence.compareIneligibleReasonHistogram;
    measured.compareCompletedStateCount = compareEvidence.compareCompletedStateCount;
    measured.semanticDisagreementCount = compareEvidence.semanticDisagreementCount;
    measured.fallbackCount = compareEvidence.fallbackCount;
    measured.semanticShiftCount = compareEvidence.semanticShiftCount;
    measured.representativeShiftCount = compareEvidence.representativeShiftCount;
}

unordered_map<string, string> parse_run_gate_fields(const string& runGateValue) {
    unordered_map<string, string> out;
    istringstream iss(runGateValue);
    string token;
    while (iss >> token) {
        const size_t equals = token.find('=');
        if (equals == string::npos) {
            continue;
        }
        out.emplace(token.substr(0, equals), token.substr(equals + 1U));
    }
    return out;
}

PolicyGateCompareEvidence missing_compare_evidence(const filesystem::path& path) {
    PolicyGateCompareEvidence evidence;
    evidence.sourceSummaryPath = filesystem::absolute(path).string();
    return evidence;
}

PolicyGateThreshold compare_threshold(size_t comparedMin) {
    PolicyGateThreshold threshold;
    threshold.comparedStateCountMin = comparedMin;
    threshold.compareEligibleStateCountMin = comparedMin;
    threshold.compareCompletedStateCountMin = comparedMin;
    threshold.semanticDisagreementCountMax = 0U;
    threshold.fallbackCountMax = 0U;
    threshold.semanticShiftCountMax = 0U;
    return threshold;
}

PolicyGateThreshold planner_non_applicable_threshold() {
    PolicyGateThreshold threshold;
    threshold.semanticDisagreementCountMax = 0U;
    threshold.fallbackCountMax = 0U;
    threshold.semanticShiftCountMax = 0U;
    threshold.applicabilityGeneratedStateCountMin = 48U;
    threshold.applicabilityMaxCompareRelevance = 0.02;
    threshold.applicabilityMaxSplitReadyRelevance = 0.05;
    threshold.applicabilityMinDominantReasonConfidence = 0.90;
    return threshold;
}

PolicyGateThreshold compare_ready_diagnostic_threshold() {
    PolicyGateThreshold threshold = compare_threshold(32U);
    threshold.lineageSampleCountMin = 16U;
    return threshold;
}

} // namespace

const char* policy_gate_status_name(PolicyGateStatus status) {
    switch (status) {
        case PolicyGateStatus::PASS:
            return "PASS";
        case PolicyGateStatus::NON_APPLICABLE:
            return "NON_APPLICABLE";
        case PolicyGateStatus::DIAGNOSTIC_ONLY:
            return "DIAGNOSTIC_ONLY";
        case PolicyGateStatus::FAIL:
            return "FAIL";
        case PolicyGateStatus::INSUFFICIENT_EVIDENCE:
            return "INSUFFICIENT_EVIDENCE";
    }
    return "FAIL";
}

bool policy_gate_status_is_satisfied(PolicyGateStatus status) {
    return status == PolicyGateStatus::PASS ||
        status == PolicyGateStatus::NON_APPLICABLE ||
        status == PolicyGateStatus::DIAGNOSTIC_ONLY;
}

PolicyGateFamilyResult evaluate_production_compare_gate(
    const string& family,
    const PolicyGateCompareEvidence& compareEvidence,
    size_t comparedStateCountMin
) {
    PolicyGateFamilyResult result;
    result.family = family;
    result.role = "production_family";
    result.threshold = compare_threshold(comparedStateCountMin);
    result.sourceSummaryPath = compareEvidence.sourceSummaryPath;
    result.countsAsProductionEvidence = false;
    copy_compare_evidence(result.measured, compareEvidence);

    if (!compareEvidence.available) {
        result.status = PolicyGateStatus::INSUFFICIENT_EVIDENCE;
        result.rationale = "compare summary missing";
        return result;
    }
    if (compareEvidence.semanticDisagreementCount > result.threshold.semanticDisagreementCountMax ||
        compareEvidence.fallbackCount > result.threshold.fallbackCountMax ||
        compareEvidence.semanticShiftCount > result.threshold.semanticShiftCountMax) {
        result.status = PolicyGateStatus::FAIL;
        result.rationale = "semantic disagreement, fallback, or semantic shift detected";
        return result;
    }
    if (compareEvidence.comparedStateCount < result.threshold.comparedStateCountMin ||
        compareEvidence.compareEligibleStateCount < result.threshold.compareEligibleStateCountMin ||
        compareEvidence.compareCompletedStateCount < result.threshold.compareCompletedStateCountMin) {
        result.status = PolicyGateStatus::INSUFFICIENT_EVIDENCE;
        result.rationale = "direct compare evidence below threshold";
        return result;
    }

    result.status = PolicyGateStatus::PASS;
    result.rationale = "direct compare evidence satisfied";
    result.countsAsProductionEvidence = true;
    return result;
}

PolicyGateFamilyResult evaluate_planner_tie_non_applicable_gate(
    const FamilyApplicabilitySummary& applicability,
    const PolicyGateCompareEvidence& compareEvidence
) {
    PolicyGateFamilyResult result;
    result.family = applicability.family.empty() ? "planner_tie_mixed_organic" : applicability.family;
    result.role = "production_family";
    result.threshold = planner_non_applicable_threshold();
    result.sourceSummaryPath = compareEvidence.sourceSummaryPath;
    result.countsAsProductionEvidence = false;

    result.measured.generatedStateCount = applicability.generatedStateCount;
    result.measured.splitReadyStateCount = applicability.splitReadyStateCount;
    result.measured.tieReadyStateCount = applicability.tieReadyStateCount;
    result.measured.compareEligibleStateCount = applicability.compareEligibleStateCount;
    result.measured.compareIneligibleStateCount = applicability.compareIneligibleStateCount;
    result.measured.compareIneligibleReasonHistogram = applicability.compareIneligibleReasonHistogram;
    result.measured.noSplitReadyCount = applicability.compareIneligibleReasonHistogram.count("no_split_ready") == 0U
        ? 0U
        : applicability.compareIneligibleReasonHistogram.at("no_split_ready");
    result.measured.compareRelevance = applicability.compareRelevance;
    result.measured.splitReadyRelevance = applicability.splitReadyRelevance;
    result.measured.dominantIneligibleReason = applicability.dominantIneligibleReason;
    result.measured.dominantIneligibleReasonConfidence = applicability.dominantIneligibleReasonConfidence;
    copy_compare_evidence(result.measured, compareEvidence);

    if (compareEvidence.available &&
        (compareEvidence.semanticDisagreementCount > result.threshold.semanticDisagreementCountMax ||
         compareEvidence.fallbackCount > result.threshold.fallbackCountMax ||
         compareEvidence.semanticShiftCount > result.threshold.semanticShiftCountMax)) {
        result.status = PolicyGateStatus::FAIL;
        result.rationale = "gap audit surfaced semantic disagreement, fallback, or semantic shift";
        return result;
    }
    if (applicability.generatedStateCount < result.threshold.applicabilityGeneratedStateCountMin) {
        result.status = PolicyGateStatus::INSUFFICIENT_EVIDENCE;
        result.rationale = "applicability audit below generated-state threshold";
        return result;
    }

    result.driftFlag =
        applicability.compareEligibleStateCount != 0U ||
        applicability.splitReadyStateCount != 0U ||
        applicability.tieReadyStateCount != 0U ||
        applicability.compareRelevance > result.threshold.applicabilityMaxCompareRelevance ||
        applicability.splitReadyRelevance > result.threshold.applicabilityMaxSplitReadyRelevance ||
        applicability.dominantIneligibleReason != "no_split_ready" ||
        applicability.dominantIneligibleReasonConfidence < result.threshold.applicabilityMinDominantReasonConfidence;
    if (result.driftFlag) {
        result.status = PolicyGateStatus::INSUFFICIENT_EVIDENCE;
        result.rationale = "applicability drift detected; NON_APPLICABLE reclassification required";
        return result;
    }

    result.status = PolicyGateStatus::NON_APPLICABLE;
    result.rationale = "no_split_ready remains dominant and compare relevance stayed below threshold";
    return result;
}

PolicyGateFamilyResult evaluate_compare_ready_diagnostic_gate(
    const PolicyGateCompareEvidence& compareEvidence,
    const CompareReadyLineageSummary& lineage
) {
    PolicyGateFamilyResult result;
    result.family = compareEvidence.family.empty()
        ? "planner_tie_mixed_organic_compare_ready"
        : compareEvidence.family;
    result.role = "diagnostic_support_family";
    result.threshold = compare_ready_diagnostic_threshold();
    result.sourceSummaryPath = compareEvidence.sourceSummaryPath;
    result.countsAsProductionEvidence = false;
    copy_compare_evidence(result.measured, compareEvidence);
    result.measured.derivedFromBaseStateCount = lineage.derivedFromBaseStateCount;
    result.measured.sameSemanticClassCount = lineage.sameSemanticClassCount;
    result.measured.sameFinalStateCount = lineage.sameFinalStateCount;
    result.measured.sameTraceClassCount = lineage.sameTraceClassCount;
    result.measured.followupPatternPreservedCount = lineage.followupPatternPreservedCount;
    result.measured.structureOnlyEnrichmentCount = lineage.structureOnlyEnrichmentCount;
    result.measured.lineageSampleCount = lineage.lineageSampleCount;

    if (!compareEvidence.available) {
        result.status = PolicyGateStatus::INSUFFICIENT_EVIDENCE;
        result.rationale = "diagnostic compare summary missing";
        return result;
    }
    if (compareEvidence.semanticDisagreementCount > result.threshold.semanticDisagreementCountMax ||
        compareEvidence.fallbackCount > result.threshold.fallbackCountMax ||
        compareEvidence.semanticShiftCount > result.threshold.semanticShiftCountMax) {
        result.status = PolicyGateStatus::FAIL;
        result.rationale = "diagnostic compare family surfaced semantic disagreement, fallback, or semantic shift";
        return result;
    }
    if (compareEvidence.comparedStateCount < result.threshold.comparedStateCountMin ||
        compareEvidence.compareEligibleStateCount < result.threshold.compareEligibleStateCountMin ||
        compareEvidence.compareCompletedStateCount < result.threshold.compareCompletedStateCountMin ||
        lineage.lineageSampleCount < result.threshold.lineageSampleCountMin ||
        lineage.derivedFromBaseStateCount < lineage.lineageSampleCount) {
        result.status = PolicyGateStatus::INSUFFICIENT_EVIDENCE;
        result.rationale = "diagnostic compare or lineage evidence below threshold";
        return result;
    }
    if (lineage.sameSemanticClassCount < lineage.lineageSampleCount ||
        lineage.sameFinalStateCount < lineage.lineageSampleCount ||
        lineage.sameTraceClassCount < lineage.lineageSampleCount) {
        result.status = PolicyGateStatus::FAIL;
        result.driftFlag = true;
        result.rationale = "diagnostic lineage consistency failed";
        return result;
    }

    result.status = PolicyGateStatus::DIAGNOSTIC_ONLY;
    result.rationale =
        "diagnostic compare family met bounded exact evidence and lineage consistency; not counted as production evidence";
    return result;
}

PolicyGateCompareEvidence load_policy_gate_compare_evidence(const filesystem::path& summaryPath) {
    if (!filesystem::exists(summaryPath)) {
        return missing_compare_evidence(summaryPath);
    }

    const unordered_map<string, string> values = read_key_value_file(summaryPath);
    PolicyGateCompareEvidence evidence;
    evidence.sourceSummaryPath = filesystem::absolute(summaryPath).string();
    evidence.comparedStateCount = read_size(values, "split_choice_compare_state_count");
    evidence.compareEligibleStateCount = read_size(values, "compare_eligible_state_count");
    evidence.compareIneligibleStateCount = read_size(values, "compare_ineligible_state_count");
    evidence.compareIneligibleReasonHistogram =
        parse_numeric_map(read_string(values, "compare_ineligible_reason_histogram"));
    evidence.compareCompletedStateCount = read_size(values, "compare_completed_state_count");
    evidence.semanticDisagreementCount = read_size(values, "split_choice_semantic_disagreement_count");
    evidence.fallbackCount = read_size(values, "split_choice_fallback_count");
    evidence.semanticShiftCount = read_size(values, "semantic_shift_count");
    evidence.representativeShiftCount = read_size(values, "representative_shift_count");
    if (const optional<string> runGate = find_value(values, "run_gate"); runGate.has_value()) {
        const unordered_map<string, string> runGateFields = parse_run_gate_fields(*runGate);
        evidence.family = read_string(runGateFields, "family");
    }
    evidence.available = true;
    return evidence;
}

PolicyGateManifest build_policy_gate_manifest(
    const filesystem::path& artifactRoot,
    const string& buildTag
) {
    PolicyGateManifest manifest;
    manifest.buildTag = buildTag;
    manifest.timestampUtc = timestamp_utc_now();
    manifest.artifactRoot = filesystem::absolute(artifactRoot).string();

    const filesystem::path splitTieSummary = first_existing_path(
        artifactRoot,
        {
            filesystem::path("phase18_campaigns_compare/split_tie_organic/logs/phase18_split_tie_organic_aggregate.summary.txt"),
            filesystem::path("phase17_campaigns_compare/split_tie_organic/logs/phase17_split_tie_organic_aggregate.summary.txt")
        }
    );
    const filesystem::path automorphismSummary = first_existing_path(
        artifactRoot,
        {
            filesystem::path("phase18_campaigns_compare/automorphism_probe/logs/phase18_automorphism_probe_aggregate.summary.txt"),
            filesystem::path("phase17_campaigns_compare/automorphism_probe/logs/phase17_automorphism_probe_aggregate.summary.txt")
        }
    );
    const filesystem::path plannerGapSummary = first_existing_path(
        artifactRoot,
        {
            filesystem::path("phase18_campaigns_compare/planner_tie_mixed_organic_gap_audit/logs/phase18_planner_tie_mixed_organic_gap_audit_aggregate.summary.txt"),
            filesystem::path("phase17_campaigns_compare/planner_tie_mixed_organic_gap_audit/logs/phase17_planner_tie_mixed_organic_gap_audit_aggregate.summary.txt")
        }
    );
    const filesystem::path compareReadySummary = first_existing_path(
        artifactRoot,
        {
            filesystem::path("phase18_campaigns_compare/planner_tie_mixed_organic_compare_ready/logs/phase18_planner_tie_mixed_organic_compare_ready_aggregate.summary.txt"),
            filesystem::path("phase17_campaigns_compare/planner_tie_mixed_organic_compare_ready/logs/phase17_planner_tie_mixed_organic_compare_ready_aggregate.summary.txt")
        }
    );
    const filesystem::path applicabilitySummary = first_existing_path(
        artifactRoot,
        {
            filesystem::path("phase18_final_applicability/logs/planner_tie_mixed_organic_applicability_audit.summary.txt"),
            filesystem::path("phase17_final_applicability/logs/planner_tie_mixed_organic_applicability_audit.summary.txt")
        }
    );
    const filesystem::path lineageSummary = first_existing_path(
        artifactRoot,
        {
            filesystem::path("phase18_final_lineage/logs/compare_ready_lineage_audit.summary.txt"),
            filesystem::path("phase17_final_lineage/logs/compare_ready_lineage_audit.summary.txt")
        }
    );

    manifest.families.push_back(
        evaluate_production_compare_gate(
            "split_tie_organic_symmetric",
            load_policy_gate_compare_evidence(splitTieSummary),
            32U
        )
    );
    manifest.families.push_back(
        evaluate_production_compare_gate(
            "automorphism_probe_large",
            load_policy_gate_compare_evidence(automorphismSummary),
            32U
        )
    );

    const FamilyApplicabilitySummary applicability = filesystem::exists(applicabilitySummary)
        ? load_family_applicability_summary(applicabilitySummary)
        : FamilyApplicabilitySummary{};
    PolicyGateFamilyResult plannerTie =
        evaluate_planner_tie_non_applicable_gate(applicability, load_policy_gate_compare_evidence(plannerGapSummary));
    if (!plannerTie.sourceSummaryPath.empty()) {
        plannerTie.sourceSummaryPath += ";applicability=" + filesystem::absolute(applicabilitySummary).string();
    } else {
        plannerTie.sourceSummaryPath = filesystem::absolute(applicabilitySummary).string();
    }
    manifest.families.push_back(std::move(plannerTie));

    const CompareReadyLineageSummary lineage = filesystem::exists(lineageSummary)
        ? load_compare_ready_lineage_summary(lineageSummary)
        : CompareReadyLineageSummary{};
    PolicyGateFamilyResult diagnostic =
        evaluate_compare_ready_diagnostic_gate(load_policy_gate_compare_evidence(compareReadySummary), lineage);
    if (!diagnostic.sourceSummaryPath.empty()) {
        diagnostic.sourceSummaryPath += ";lineage=" + filesystem::absolute(lineageSummary).string();
    } else {
        diagnostic.sourceSummaryPath = filesystem::absolute(lineageSummary).string();
    }
    manifest.families.push_back(std::move(diagnostic));

    return manifest;
}

PolicyGateManifest filter_policy_gate_manifest(
    const PolicyGateManifest& manifest,
    const optional<string>& familyFilter
) {
    if (!familyFilter.has_value() || familyFilter->empty()) {
        return manifest;
    }
    PolicyGateManifest filtered = manifest;
    filtered.families.clear();
    for (const PolicyGateFamilyResult& family : manifest.families) {
        if (family.family == *familyFilter) {
            filtered.families.push_back(family);
        }
    }
    return filtered;
}

bool policy_gate_manifest_satisfied(
    const PolicyGateManifest& manifest,
    const optional<string>& familyFilter
) {
    const PolicyGateManifest filtered = filter_policy_gate_manifest(manifest, familyFilter);
    if (filtered.families.empty()) {
        return false;
    }
    return all_of(filtered.families.begin(), filtered.families.end(), [](const PolicyGateFamilyResult& family) {
        return policy_gate_status_is_satisfied(family.status);
    });
}

string policy_gate_manifest_text(const PolicyGateManifest& manifest) {
    ostringstream oss;
    oss << "manifest_version=" << manifest.manifestVersion << '\n';
    oss << "report_version=" << manifest.reportVersion << '\n';
    oss << "build_tag=" << manifest.buildTag << '\n';
    oss << "timestamp_utc=" << manifest.timestampUtc << '\n';
    oss << "artifact_root=" << manifest.artifactRoot << '\n';
    oss << "family_count=" << manifest.families.size() << '\n';
    for (const PolicyGateFamilyResult& family : manifest.families) {
        oss << '\n';
        oss << "family=" << family.family << '\n';
        oss << "role=" << family.role << '\n';
        oss << "status=" << policy_gate_status_name(family.status) << '\n';
        oss << "counts_as_production_evidence=" << (family.countsAsProductionEvidence ? 1 : 0) << '\n';
        oss << "drift_flag=" << (family.driftFlag ? 1 : 0) << '\n';
        oss << "source_summary_path=" << family.sourceSummaryPath << '\n';
        oss << "threshold_compared_state_count_min=" << family.threshold.comparedStateCountMin << '\n';
        oss << "threshold_compare_eligible_state_count_min=" << family.threshold.compareEligibleStateCountMin << '\n';
        oss << "threshold_compare_completed_state_count_min=" << family.threshold.compareCompletedStateCountMin << '\n';
        oss << "threshold_semantic_disagreement_count_max=" << family.threshold.semanticDisagreementCountMax << '\n';
        oss << "threshold_fallback_count_max=" << family.threshold.fallbackCountMax << '\n';
        oss << "threshold_semantic_shift_count_max=" << family.threshold.semanticShiftCountMax << '\n';
        oss << "threshold_representative_shift_count_max=" << family.threshold.representativeShiftCountMax << '\n';
        oss << "threshold_applicability_generated_state_count_min="
            << family.threshold.applicabilityGeneratedStateCountMin << '\n';
        oss << "threshold_applicability_max_compare_relevance="
            << json_number(family.threshold.applicabilityMaxCompareRelevance) << '\n';
        oss << "threshold_applicability_max_split_ready_relevance="
            << json_number(family.threshold.applicabilityMaxSplitReadyRelevance) << '\n';
        oss << "threshold_applicability_min_dominant_reason_confidence="
            << json_number(family.threshold.applicabilityMinDominantReasonConfidence) << '\n';
        oss << "threshold_lineage_sample_count_min=" << family.threshold.lineageSampleCountMin << '\n';
        oss << "measured_generated_state_count=" << family.measured.generatedStateCount << '\n';
        oss << "measured_split_ready_state_count=" << family.measured.splitReadyStateCount << '\n';
        oss << "measured_tie_ready_state_count=" << family.measured.tieReadyStateCount << '\n';
        oss << "measured_compared_state_count=" << family.measured.comparedStateCount << '\n';
        oss << "measured_compare_eligible_state_count=" << family.measured.compareEligibleStateCount << '\n';
        oss << "measured_compare_ineligible_state_count=" << family.measured.compareIneligibleStateCount << '\n';
        oss << "measured_compare_ineligible_reason_histogram="
            << json_object_from_map(family.measured.compareIneligibleReasonHistogram) << '\n';
        oss << "measured_compare_completed_state_count=" << family.measured.compareCompletedStateCount << '\n';
        oss << "measured_semantic_disagreement_count=" << family.measured.semanticDisagreementCount << '\n';
        oss << "measured_fallback_count=" << family.measured.fallbackCount << '\n';
        oss << "measured_semantic_shift_count=" << family.measured.semanticShiftCount << '\n';
        oss << "measured_representative_shift_count=" << family.measured.representativeShiftCount << '\n';
        oss << "measured_no_split_ready_count=" << family.measured.noSplitReadyCount << '\n';
        oss << "measured_compare_relevance=" << json_number(family.measured.compareRelevance) << '\n';
        oss << "measured_split_ready_relevance=" << json_number(family.measured.splitReadyRelevance) << '\n';
        oss << "measured_dominant_ineligible_reason=" << family.measured.dominantIneligibleReason << '\n';
        oss << "measured_dominant_ineligible_reason_confidence="
            << json_number(family.measured.dominantIneligibleReasonConfidence) << '\n';
        oss << "measured_derived_from_base_state_count=" << family.measured.derivedFromBaseStateCount << '\n';
        oss << "measured_same_semantic_class_count=" << family.measured.sameSemanticClassCount << '\n';
        oss << "measured_same_final_state_count=" << family.measured.sameFinalStateCount << '\n';
        oss << "measured_same_trace_class_count=" << family.measured.sameTraceClassCount << '\n';
        oss << "measured_followup_pattern_preserved_count=" << family.measured.followupPatternPreservedCount << '\n';
        oss << "measured_structure_only_enrichment_count=" << family.measured.structureOnlyEnrichmentCount << '\n';
        oss << "measured_lineage_sample_count=" << family.measured.lineageSampleCount << '\n';
        oss << "rationale=" << family.rationale << '\n';
    }
    return oss.str();
}

string policy_gate_manifest_json(const PolicyGateManifest& manifest) {
    ostringstream oss;
    oss << "{\n";
    oss << "\"manifest_version\":\"" << json_escape(manifest.manifestVersion) << "\",\n";
    oss << "\"report_version\":\"" << json_escape(manifest.reportVersion) << "\",\n";
    oss << "\"build_tag\":\"" << json_escape(manifest.buildTag) << "\",\n";
    oss << "\"timestamp_utc\":\"" << json_escape(manifest.timestampUtc) << "\",\n";
    oss << "\"artifact_root\":\"" << json_escape(manifest.artifactRoot) << "\",\n";
    oss << "\"families\":[\n";
    for (size_t i = 0; i < manifest.families.size(); ++i) {
        const PolicyGateFamilyResult& family = manifest.families[i];
        oss << "  {\n";
        oss << "    \"family\":\"" << json_escape(family.family) << "\",\n";
        oss << "    \"role\":\"" << json_escape(family.role) << "\",\n";
        oss << "    \"status\":\"" << policy_gate_status_name(family.status) << "\",\n";
        oss << "    \"counts_as_production_evidence\":" << (family.countsAsProductionEvidence ? "true" : "false") << ",\n";
        oss << "    \"drift_flag\":" << (family.driftFlag ? "true" : "false") << ",\n";
        oss << "    \"source_summary_path\":\"" << json_escape(family.sourceSummaryPath) << "\",\n";
        oss << "    \"threshold\":{\n";
        oss << "      \"compared_state_count_min\":" << family.threshold.comparedStateCountMin << ",\n";
        oss << "      \"compare_eligible_state_count_min\":" << family.threshold.compareEligibleStateCountMin << ",\n";
        oss << "      \"compare_completed_state_count_min\":" << family.threshold.compareCompletedStateCountMin << ",\n";
        oss << "      \"semantic_disagreement_count_max\":" << family.threshold.semanticDisagreementCountMax << ",\n";
        oss << "      \"fallback_count_max\":" << family.threshold.fallbackCountMax << ",\n";
        oss << "      \"semantic_shift_count_max\":" << family.threshold.semanticShiftCountMax << ",\n";
        oss << "      \"representative_shift_count_max\":" << family.threshold.representativeShiftCountMax << ",\n";
        oss << "      \"applicability_generated_state_count_min\":"
            << family.threshold.applicabilityGeneratedStateCountMin << ",\n";
        oss << "      \"applicability_max_compare_relevance\":"
            << json_number(family.threshold.applicabilityMaxCompareRelevance) << ",\n";
        oss << "      \"applicability_max_split_ready_relevance\":"
            << json_number(family.threshold.applicabilityMaxSplitReadyRelevance) << ",\n";
        oss << "      \"applicability_min_dominant_reason_confidence\":"
            << json_number(family.threshold.applicabilityMinDominantReasonConfidence) << ",\n";
        oss << "      \"lineage_sample_count_min\":" << family.threshold.lineageSampleCountMin << "\n";
        oss << "    },\n";
        oss << "    \"measured\":{\n";
        oss << "      \"generated_state_count\":" << family.measured.generatedStateCount << ",\n";
        oss << "      \"split_ready_state_count\":" << family.measured.splitReadyStateCount << ",\n";
        oss << "      \"tie_ready_state_count\":" << family.measured.tieReadyStateCount << ",\n";
        oss << "      \"compared_state_count\":" << family.measured.comparedStateCount << ",\n";
        oss << "      \"compare_eligible_state_count\":" << family.measured.compareEligibleStateCount << ",\n";
        oss << "      \"compare_ineligible_state_count\":" << family.measured.compareIneligibleStateCount << ",\n";
        oss << "      \"compare_ineligible_reason_histogram\":"
            << json_object_from_map(family.measured.compareIneligibleReasonHistogram) << ",\n";
        oss << "      \"compare_completed_state_count\":" << family.measured.compareCompletedStateCount << ",\n";
        oss << "      \"semantic_disagreement_count\":" << family.measured.semanticDisagreementCount << ",\n";
        oss << "      \"fallback_count\":" << family.measured.fallbackCount << ",\n";
        oss << "      \"semantic_shift_count\":" << family.measured.semanticShiftCount << ",\n";
        oss << "      \"representative_shift_count\":" << family.measured.representativeShiftCount << ",\n";
        oss << "      \"no_split_ready_count\":" << family.measured.noSplitReadyCount << ",\n";
        oss << "      \"compare_relevance\":" << json_number(family.measured.compareRelevance) << ",\n";
        oss << "      \"split_ready_relevance\":" << json_number(family.measured.splitReadyRelevance) << ",\n";
        oss << "      \"dominant_ineligible_reason\":\"" << json_escape(family.measured.dominantIneligibleReason) << "\",\n";
        oss << "      \"dominant_ineligible_reason_confidence\":"
            << json_number(family.measured.dominantIneligibleReasonConfidence) << ",\n";
        oss << "      \"derived_from_base_state_count\":" << family.measured.derivedFromBaseStateCount << ",\n";
        oss << "      \"same_semantic_class_count\":" << family.measured.sameSemanticClassCount << ",\n";
        oss << "      \"same_final_state_count\":" << family.measured.sameFinalStateCount << ",\n";
        oss << "      \"same_trace_class_count\":" << family.measured.sameTraceClassCount << ",\n";
        oss << "      \"followup_pattern_preserved_count\":" << family.measured.followupPatternPreservedCount << ",\n";
        oss << "      \"structure_only_enrichment_count\":" << family.measured.structureOnlyEnrichmentCount << ",\n";
        oss << "      \"lineage_sample_count\":" << family.measured.lineageSampleCount << "\n";
        oss << "    },\n";
        oss << "    \"rationale\":\"" << json_escape(family.rationale) << "\"\n";
        oss << "  }";
        if (i + 1U != manifest.families.size()) {
            oss << ',';
        }
        oss << '\n';
    }
    oss << "]\n";
    oss << "}\n";
    return oss.str();
}

string policy_gate_short_summary(
    const PolicyGateManifest& manifest,
    const optional<string>& familyFilter
) {
    const PolicyGateManifest filtered = filter_policy_gate_manifest(manifest, familyFilter);
    ostringstream oss;
    oss << "policy_gate_summary";
    for (const PolicyGateFamilyResult& family : filtered.families) {
        oss << ' ' << family.family << '=' << policy_gate_status_name(family.status);
        if (family.driftFlag) {
            oss << "(drift)";
        }
    }
    oss << '\n';
    for (const PolicyGateFamilyResult& family : filtered.families) {
        oss << "family=" << family.family
            << " status=" << policy_gate_status_name(family.status)
            << " role=" << family.role
            << " compared=" << family.measured.comparedStateCount
            << " eligible=" << family.measured.compareEligibleStateCount
            << " completed=" << family.measured.compareCompletedStateCount
            << " rationale=" << family.rationale
            << '\n';
    }
    return oss.str();
}

void write_policy_gate_outputs(const filesystem::path& jsonPath, const PolicyGateManifest& manifest) {
    filesystem::create_directories(jsonPath.parent_path());
    ofstream jsonOfs(jsonPath);
    if (!jsonOfs) {
        throw runtime_error("failed to write policy gate json: " + jsonPath.string());
    }
    jsonOfs << policy_gate_manifest_json(manifest);

    const filesystem::path txtPath = jsonPath.parent_path() / (jsonPath.stem().string() + ".txt");
    ofstream txtOfs(txtPath);
    if (!txtOfs) {
        throw runtime_error("failed to write policy gate text manifest: " + txtPath.string());
    }
    txtOfs << policy_gate_manifest_text(manifest);

    const filesystem::path summaryPath = jsonPath.parent_path() / (jsonPath.stem().string() + ".summary.txt");
    ofstream summaryOfs(summaryPath);
    if (!summaryOfs) {
        throw runtime_error("failed to write policy gate summary: " + summaryPath.string());
    }
    summaryOfs << policy_gate_short_summary(manifest);
}

PolicyGateManifest load_policy_gate_manifest_text(const filesystem::path& manifestPath) {
    ifstream ifs(manifestPath);
    if (!ifs) {
        throw runtime_error("failed to read policy gate manifest: " + manifestPath.string());
    }

    PolicyGateManifest manifest;
    PolicyGateFamilyResult* current = nullptr;
    string line;
    while (getline(ifs, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const size_t equals = line.find('=');
        if (equals == string::npos) {
            continue;
        }
        const string key = trim(line.substr(0, equals));
        const string value = trim(line.substr(equals + 1U));
        if (key == "manifest_version") {
            manifest.manifestVersion = value;
            continue;
        }
        if (key == "report_version") {
            manifest.reportVersion = value;
            continue;
        }
        if (key == "build_tag") {
            manifest.buildTag = value;
            continue;
        }
        if (key == "timestamp_utc") {
            manifest.timestampUtc = value;
            continue;
        }
        if (key == "artifact_root") {
            manifest.artifactRoot = value;
            continue;
        }
        if (key == "family_count") {
            continue;
        }
        if (key == "family") {
            manifest.families.push_back(PolicyGateFamilyResult{});
            current = &manifest.families.back();
            current->family = value;
            continue;
        }
        if (current == nullptr) {
            continue;
        }
        if (key == "role") {
            current->role = value;
        } else if (key == "status") {
            if (value == "PASS") {
                current->status = PolicyGateStatus::PASS;
            } else if (value == "NON_APPLICABLE") {
                current->status = PolicyGateStatus::NON_APPLICABLE;
            } else if (value == "DIAGNOSTIC_ONLY") {
                current->status = PolicyGateStatus::DIAGNOSTIC_ONLY;
            } else if (value == "FAIL") {
                current->status = PolicyGateStatus::FAIL;
            } else {
                current->status = PolicyGateStatus::INSUFFICIENT_EVIDENCE;
            }
        } else if (key == "counts_as_production_evidence") {
            current->countsAsProductionEvidence = (value == "1" || value == "true");
        } else if (key == "drift_flag") {
            current->driftFlag = (value == "1" || value == "true");
        } else if (key == "source_summary_path") {
            current->sourceSummaryPath = value;
        } else if (key == "threshold_compared_state_count_min") {
            current->threshold.comparedStateCountMin = static_cast<size_t>(stoull(value));
        } else if (key == "threshold_compare_eligible_state_count_min") {
            current->threshold.compareEligibleStateCountMin = static_cast<size_t>(stoull(value));
        } else if (key == "threshold_compare_completed_state_count_min") {
            current->threshold.compareCompletedStateCountMin = static_cast<size_t>(stoull(value));
        } else if (key == "threshold_semantic_disagreement_count_max") {
            current->threshold.semanticDisagreementCountMax = static_cast<size_t>(stoull(value));
        } else if (key == "threshold_fallback_count_max") {
            current->threshold.fallbackCountMax = static_cast<size_t>(stoull(value));
        } else if (key == "threshold_semantic_shift_count_max") {
            current->threshold.semanticShiftCountMax = static_cast<size_t>(stoull(value));
        } else if (key == "threshold_representative_shift_count_max") {
            current->threshold.representativeShiftCountMax = static_cast<size_t>(stoull(value));
        } else if (key == "threshold_applicability_generated_state_count_min") {
            current->threshold.applicabilityGeneratedStateCountMin = static_cast<size_t>(stoull(value));
        } else if (key == "threshold_applicability_max_compare_relevance") {
            current->threshold.applicabilityMaxCompareRelevance = stod(value);
        } else if (key == "threshold_applicability_max_split_ready_relevance") {
            current->threshold.applicabilityMaxSplitReadyRelevance = stod(value);
        } else if (key == "threshold_applicability_min_dominant_reason_confidence") {
            current->threshold.applicabilityMinDominantReasonConfidence = stod(value);
        } else if (key == "threshold_lineage_sample_count_min") {
            current->threshold.lineageSampleCountMin = static_cast<size_t>(stoull(value));
        } else if (key == "measured_generated_state_count") {
            current->measured.generatedStateCount = static_cast<size_t>(stoull(value));
        } else if (key == "measured_split_ready_state_count") {
            current->measured.splitReadyStateCount = static_cast<size_t>(stoull(value));
        } else if (key == "measured_tie_ready_state_count") {
            current->measured.tieReadyStateCount = static_cast<size_t>(stoull(value));
        } else if (key == "measured_compared_state_count") {
            current->measured.comparedStateCount = static_cast<size_t>(stoull(value));
        } else if (key == "measured_compare_eligible_state_count") {
            current->measured.compareEligibleStateCount = static_cast<size_t>(stoull(value));
        } else if (key == "measured_compare_ineligible_state_count") {
            current->measured.compareIneligibleStateCount = static_cast<size_t>(stoull(value));
        } else if (key == "measured_compare_ineligible_reason_histogram") {
            current->measured.compareIneligibleReasonHistogram = parse_numeric_map(value);
        } else if (key == "measured_compare_completed_state_count") {
            current->measured.compareCompletedStateCount = static_cast<size_t>(stoull(value));
        } else if (key == "measured_semantic_disagreement_count") {
            current->measured.semanticDisagreementCount = static_cast<size_t>(stoull(value));
        } else if (key == "measured_fallback_count") {
            current->measured.fallbackCount = static_cast<size_t>(stoull(value));
        } else if (key == "measured_semantic_shift_count") {
            current->measured.semanticShiftCount = static_cast<size_t>(stoull(value));
        } else if (key == "measured_representative_shift_count") {
            current->measured.representativeShiftCount = static_cast<size_t>(stoull(value));
        } else if (key == "measured_no_split_ready_count") {
            current->measured.noSplitReadyCount = static_cast<size_t>(stoull(value));
        } else if (key == "measured_compare_relevance") {
            current->measured.compareRelevance = stod(value);
        } else if (key == "measured_split_ready_relevance") {
            current->measured.splitReadyRelevance = stod(value);
        } else if (key == "measured_dominant_ineligible_reason") {
            current->measured.dominantIneligibleReason = value;
        } else if (key == "measured_dominant_ineligible_reason_confidence") {
            current->measured.dominantIneligibleReasonConfidence = stod(value);
        } else if (key == "measured_derived_from_base_state_count") {
            current->measured.derivedFromBaseStateCount = static_cast<size_t>(stoull(value));
        } else if (key == "measured_same_semantic_class_count") {
            current->measured.sameSemanticClassCount = static_cast<size_t>(stoull(value));
        } else if (key == "measured_same_final_state_count") {
            current->measured.sameFinalStateCount = static_cast<size_t>(stoull(value));
        } else if (key == "measured_same_trace_class_count") {
            current->measured.sameTraceClassCount = static_cast<size_t>(stoull(value));
        } else if (key == "measured_followup_pattern_preserved_count") {
            current->measured.followupPatternPreservedCount = static_cast<size_t>(stoull(value));
        } else if (key == "measured_structure_only_enrichment_count") {
            current->measured.structureOnlyEnrichmentCount = static_cast<size_t>(stoull(value));
        } else if (key == "measured_lineage_sample_count") {
            current->measured.lineageSampleCount = static_cast<size_t>(stoull(value));
        } else if (key == "rationale") {
            current->rationale = value;
        }
    }
    return manifest;
}
