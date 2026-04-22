#include "policy_manifest_freshness.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <initializer_list>
#include <sstream>

#include "failure_signature.hpp"

using namespace std;

namespace {

void append_unique_path(vector<filesystem::path>& paths, const filesystem::path& path) {
    if (find(paths.begin(), paths.end(), path) == paths.end()) {
        paths.push_back(path);
    }
}

void append_unique_string(vector<string>& values, const string& value) {
    if (!value.empty() && find(values.begin(), values.end(), value) == values.end()) {
        values.push_back(value);
    }
}

bool contains_source(const vector<PolicyEvidenceSource>& sources, PolicyEvidenceSource source) {
    return find(sources.begin(), sources.end(), source) != sources.end();
}

vector<PolicyEvidenceSource> default_evidence_sources_for_family(const string& family) {
    if (family == "split_tie_organic_symmetric" || family == "automorphism_probe_large") {
        return {PolicyEvidenceSource::DIRECT_COMPARE, PolicyEvidenceSource::CAMPAIGN_COMPARE};
    }
    if (family == "planner_tie_mixed_organic") {
        return {PolicyEvidenceSource::APPLICABILITY_ONLY};
    }
    if (family == "planner_tie_mixed_organic_compare_ready") {
        return {PolicyEvidenceSource::DIAGNOSTIC_LINEAGE};
    }
    return {};
}

string hash_file_set(const filesystem::path& sourceRoot, const vector<filesystem::path>& relPaths) {
    ostringstream oss;
    bool anyExisting = false;
    for (const filesystem::path& relPath : relPaths) {
        const filesystem::path fullPath = sourceRoot / relPath;
        oss << relPath.generic_string() << '\n';
        std::error_code ec;
        const bool exists = filesystem::exists(fullPath, ec);
        if (ec || !exists) {
            oss << "<missing>\n";
            continue;
        }
        anyExisting = true;
        const auto size = filesystem::file_size(fullPath, ec);
        oss << "size=" << (ec ? static_cast<uintmax_t>(0) : size) << '\n';
    }
    if (!anyExisting) {
        oss << "<no-existing-files>\n";
    }
    return stable_hash_text(oss.str());
}

filesystem::path first_existing_under_root(
    const filesystem::path& sourceRoot,
    initializer_list<filesystem::path> relPaths
) {
    for (const filesystem::path& relPath : relPaths) {
        if (filesystem::exists(sourceRoot / relPath)) {
            return relPath;
        }
    }
    return *relPaths.begin();
}

filesystem::path resolve_campaign_relpath(const filesystem::path& sourceRoot, const string& family) {
    if (family == "split_tie_organic_symmetric") {
        return first_existing_under_root(
            sourceRoot,
            {
                filesystem::path("tests/campaigns/phase22_split_tie_organic_compare.txt"),
                filesystem::path("tests/campaigns/phase21_split_tie_organic_compare.txt"),
                filesystem::path("tests/campaigns/phase19_split_tie_organic_compare.txt"),
                filesystem::path("tests/campaigns/phase18_split_tie_organic_compare.txt"),
                filesystem::path("tests/campaigns/phase17_split_tie_organic_compare.txt"),
                filesystem::path("tests/campaigns/phase16_split_tie_organic_compare.txt"),
            }
        );
    }
    if (family == "automorphism_probe_large") {
        return first_existing_under_root(
            sourceRoot,
            {
                filesystem::path("tests/campaigns/phase22_automorphism_compare.txt"),
                filesystem::path("tests/campaigns/phase21_automorphism_compare.txt"),
                filesystem::path("tests/campaigns/phase19_automorphism_compare.txt"),
                filesystem::path("tests/campaigns/phase18_automorphism_compare.txt"),
                filesystem::path("tests/campaigns/phase17_automorphism_compare.txt"),
                filesystem::path("tests/campaigns/phase16_automorphism_compare.txt"),
            }
        );
    }
    if (family == "planner_tie_mixed_organic") {
        return first_existing_under_root(
            sourceRoot,
            {
                filesystem::path("tests/campaigns/phase22_planner_tie_gap_audit.txt"),
                filesystem::path("tests/campaigns/phase19_planner_tie_gap_audit.txt"),
                filesystem::path("tests/campaigns/phase18_planner_tie_gap_audit.txt"),
                filesystem::path("tests/campaigns/phase17_planner_tie_gap_audit.txt"),
                filesystem::path("tests/campaigns/phase16_planner_tie_gap_probe.txt"),
            }
        );
    }
    if (family == "planner_tie_mixed_organic_compare_ready") {
        return first_existing_under_root(
            sourceRoot,
            {
                filesystem::path("tests/campaigns/phase22_planner_tie_compare_ready.txt"),
                filesystem::path("tests/campaigns/phase19_planner_tie_compare_ready.txt"),
                filesystem::path("tests/campaigns/phase18_planner_tie_compare_ready.txt"),
                filesystem::path("tests/campaigns/phase17_planner_tie_compare_ready.txt"),
                filesystem::path("tests/campaigns/phase16_planner_tie_compare_ready.txt"),
            }
        );
    }
    return {};
}

const PolicyGateFamilyResult* find_family(const PolicyGateManifest& manifest, const string& family) {
    for (const PolicyGateFamilyResult& entry : manifest.families) {
        if (entry.family == family) {
            return &entry;
        }
    }
    return nullptr;
}

bool selected_for_revalidation(const PolicyManifestRefreshOptions& options, const string& family) {
    return options.revalidateFamilies.empty() ||
        find(options.revalidateFamilies.begin(), options.revalidateFamilies.end(), family) != options.revalidateFamilies.end();
}

bool relevant_hashes_changed(
    const vector<PolicyEvidenceSource>& evidenceSources,
    const PolicyRelevantInputHashes& baselineHashes,
    const PolicyRelevantInputHashes& currentHashes
) {
    const bool usesCampaign = contains_source(evidenceSources, PolicyEvidenceSource::CAMPAIGN_COMPARE) ||
        contains_source(evidenceSources, PolicyEvidenceSource::APPLICABILITY_ONLY);
    const bool usesCore = contains_source(evidenceSources, PolicyEvidenceSource::DIRECT_COMPARE) ||
        contains_source(evidenceSources, PolicyEvidenceSource::APPLICABILITY_ONLY) ||
        contains_source(evidenceSources, PolicyEvidenceSource::DIAGNOSTIC_LINEAGE) ||
        usesCampaign;
    const bool usesCompareEngine = contains_source(evidenceSources, PolicyEvidenceSource::DIRECT_COMPARE) ||
        contains_source(evidenceSources, PolicyEvidenceSource::DIAGNOSTIC_LINEAGE) ||
        contains_source(evidenceSources, PolicyEvidenceSource::CAMPAIGN_COMPARE);
    return (usesCore && baselineHashes.plannerSemanticsHash != currentHashes.plannerSemanticsHash) ||
        (usesCore && baselineHashes.generatorFamilyHash != currentHashes.generatorFamilyHash) ||
        (usesCompareEngine && baselineHashes.compareEngineHash != currentHashes.compareEngineHash) ||
        (usesCampaign && baselineHashes.campaignConfigHash != currentHashes.campaignConfigHash);
}

bool missing_relevant_hashes(
    const vector<PolicyEvidenceSource>& evidenceSources,
    const PolicyRelevantInputHashes& hashes
) {
    const bool usesCampaign = contains_source(evidenceSources, PolicyEvidenceSource::CAMPAIGN_COMPARE) ||
        contains_source(evidenceSources, PolicyEvidenceSource::APPLICABILITY_ONLY);
    const bool usesCore = contains_source(evidenceSources, PolicyEvidenceSource::DIRECT_COMPARE) ||
        contains_source(evidenceSources, PolicyEvidenceSource::APPLICABILITY_ONLY) ||
        contains_source(evidenceSources, PolicyEvidenceSource::DIAGNOSTIC_LINEAGE) ||
        usesCampaign;
    const bool usesCompareEngine = contains_source(evidenceSources, PolicyEvidenceSource::DIRECT_COMPARE) ||
        contains_source(evidenceSources, PolicyEvidenceSource::DIAGNOSTIC_LINEAGE) ||
        contains_source(evidenceSources, PolicyEvidenceSource::CAMPAIGN_COMPARE);
    if (usesCore && (hashes.plannerSemanticsHash.empty() || hashes.generatorFamilyHash.empty())) {
        return true;
    }
    if (usesCompareEngine && hashes.compareEngineHash.empty()) {
        return true;
    }
    return usesCampaign && hashes.campaignConfigHash.empty();
}

void infer_refresh_policy_drift(PolicyGateFamilyResult& currentFamily) {
    if (currentFamily.status != PolicyGateStatus::NON_APPLICABLE) {
        return;
    }
    const bool eligibleAppeared = currentFamily.measured.compareEligibleStateCount > 0U;
    const bool compareRelevanceExceeded =
        currentFamily.threshold.applicabilityMaxCompareRelevance > 0.0 &&
        currentFamily.measured.compareRelevance > currentFamily.threshold.applicabilityMaxCompareRelevance;
    const bool splitReadyRelevanceExceeded =
        currentFamily.threshold.applicabilityMaxSplitReadyRelevance > 0.0 &&
        currentFamily.measured.splitReadyRelevance > currentFamily.threshold.applicabilityMaxSplitReadyRelevance;
    const bool dominantReasonShifted =
        !currentFamily.measured.dominantIneligibleReason.empty() &&
        currentFamily.measured.dominantIneligibleReason != "no_split_ready";
    const bool dominantReasonConfidenceDropped =
        currentFamily.threshold.applicabilityMinDominantReasonConfidence > 0.0 &&
        currentFamily.measured.dominantIneligibleReasonConfidence <
            currentFamily.threshold.applicabilityMinDominantReasonConfidence;

    if (eligibleAppeared || compareRelevanceExceeded || splitReadyRelevanceExceeded ||
        dominantReasonShifted || dominantReasonConfidenceDropped) {
        currentFamily.driftFlag = true;
        currentFamily.reclassifyRequired = true;
        currentFamily.freshnessRationale = "non-applicable family drifted beyond approved applicability envelope";
    }
}

PolicyFreshnessStatus evaluate_family_freshness(
    PolicyGateFamilyResult& currentFamily,
    const PolicyGateFamilyResult* baselineFamily,
    const PolicyManifestRefreshOptions& options
) {
    if (currentFamily.evidenceSources.empty()) {
        currentFamily.evidenceSources = default_evidence_sources_for_family(currentFamily.family);
    }

    infer_refresh_policy_drift(currentFamily);

    if (currentFamily.status == PolicyGateStatus::FAIL || currentFamily.status == PolicyGateStatus::INSUFFICIENT_EVIDENCE) {
        currentFamily.freshnessRationale = "current evidence status requires rerun";
        return PolicyFreshnessStatus::REQUIRES_RERUN;
    }
    if (currentFamily.driftFlag || currentFamily.reclassifyRequired) {
        currentFamily.freshnessRationale = "drift or reclassification flag requires rerun";
        return PolicyFreshnessStatus::REQUIRES_RERUN;
    }
    if (currentFamily.family == "planner_tie_mixed_organic_compare_ready" && currentFamily.countsAsProductionEvidence) {
        currentFamily.reclassifyRequired = true;
        currentFamily.freshnessRationale = "diagnostic family must not count as production evidence";
        return PolicyFreshnessStatus::REQUIRES_RERUN;
    }
    if (baselineFamily == nullptr) {
        currentFamily.freshnessRationale = "baseline manifest missing family entry";
        return PolicyFreshnessStatus::REQUIRES_RERUN;
    }
    if (baselineFamily->status != currentFamily.status) {
        currentFamily.reclassifyRequired = true;
        currentFamily.freshnessRationale = "family status changed relative to baseline";
        return PolicyFreshnessStatus::REQUIRES_RERUN;
    }
    if (baselineFamily->countsAsProductionEvidence != currentFamily.countsAsProductionEvidence) {
        currentFamily.reclassifyRequired = true;
        currentFamily.freshnessRationale = "production evidence classification changed relative to baseline";
        return PolicyFreshnessStatus::REQUIRES_RERUN;
    }
    if (missing_relevant_hashes(currentFamily.evidenceSources, baselineFamily->relevantInputHashes) ||
        missing_relevant_hashes(currentFamily.evidenceSources, currentFamily.relevantInputHashes)) {
        currentFamily.freshnessRationale = "relevant input hashes missing";
        return PolicyFreshnessStatus::REQUIRES_RERUN;
    }
    if (options.markStaleOnHashChange &&
        relevant_hashes_changed(currentFamily.evidenceSources, baselineFamily->relevantInputHashes, currentFamily.relevantInputHashes)) {
        currentFamily.freshnessRationale = "relevant input hash changed since baseline";
        return PolicyFreshnessStatus::STALE;
    }
    currentFamily.freshnessRationale = "baseline-aligned evidence remains fresh";
    return PolicyFreshnessStatus::FRESH;
}

} // namespace

PolicyRelevantInputHashes compute_policy_relevant_input_hashes(
    const filesystem::path& sourceRoot,
    const string& family,
    const vector<PolicyEvidenceSource>& evidenceSources
) {
    vector<filesystem::path> plannerFiles{
        filesystem::path("src/raw_planner.cpp"),
        filesystem::path("include/raw_engine/raw_engine.hpp"),
    };
    vector<filesystem::path> generatorFiles{
        filesystem::path("tests/exhaustive_generator.cpp"),
        filesystem::path("tests/exhaustive_generator.hpp"),
    };
    vector<filesystem::path> compareFiles{
        filesystem::path("tests/split_choice_oracle.cpp"),
        filesystem::path("tests/split_choice_oracle.hpp"),
        filesystem::path("tests/exact_canonicalizer.cpp"),
        filesystem::path("tests/exact_canonicalizer.hpp"),
        filesystem::path("tests/policy_gate.cpp"),
        filesystem::path("tests/policy_gate.hpp"),
        filesystem::path("tests/policy_manifest_freshness.cpp"),
        filesystem::path("tests/policy_manifest_freshness.hpp"),
    };

    if (family == "planner_tie_mixed_organic") {
        append_unique_path(generatorFiles, filesystem::path("tests/family_applicability_audit.cpp"));
        append_unique_path(generatorFiles, filesystem::path("tests/family_applicability_audit.hpp"));
    } else if (family == "planner_tie_mixed_organic_compare_ready") {
        append_unique_path(generatorFiles, filesystem::path("tests/compare_ready_lineage.cpp"));
        append_unique_path(generatorFiles, filesystem::path("tests/compare_ready_lineage.hpp"));
    }

    PolicyRelevantInputHashes hashes;
    hashes.plannerSemanticsHash = hash_file_set(sourceRoot, plannerFiles);
    hashes.generatorFamilyHash = hash_file_set(sourceRoot, generatorFiles);
    hashes.compareEngineHash = hash_file_set(sourceRoot, compareFiles);

    if (contains_source(evidenceSources, PolicyEvidenceSource::CAMPAIGN_COMPARE) ||
        contains_source(evidenceSources, PolicyEvidenceSource::APPLICABILITY_ONLY)) {
        const filesystem::path campaignRelPath = resolve_campaign_relpath(sourceRoot, family);
        if (!campaignRelPath.empty()) {
            hashes.campaignConfigHash = hash_file_set(sourceRoot, {campaignRelPath});
        }
    }

    hashes.combinedHash = stable_hash_text(
        hashes.plannerSemanticsHash + "|" +
        hashes.generatorFamilyHash + "|" +
        hashes.compareEngineHash + "|" +
        hashes.campaignConfigHash);
    return hashes;
}

void assign_policy_manifest_input_hashes(
    PolicyGateManifest& manifest,
    const filesystem::path& sourceRoot
) {
    for (PolicyGateFamilyResult& family : manifest.families) {
        if (family.evidenceSources.empty()) {
            family.evidenceSources = default_evidence_sources_for_family(family.family);
        }
        family.relevantInputHashes =
            compute_policy_relevant_input_hashes(sourceRoot, family.family, family.evidenceSources);
    }
}

string hash_policy_manifest_file(const filesystem::path& manifestPath) {
    ifstream ifs(manifestPath, ios::binary);
    if (!ifs) {
        return {};
    }
    ostringstream oss;
    oss << ifs.rdbuf();
    return stable_hash_text(oss.str());
}

string hash_policy_manifest_content(const PolicyGateManifest& manifest) {
    PolicyGateManifest copy = manifest;
    copy.currentManifestHash.clear();
    return stable_hash_text(policy_gate_manifest_text(copy));
}

PolicyGateManifest refresh_policy_gate_manifest(
    const PolicyGateManifest& baselineManifest,
    const PolicyGateManifest& currentManifest,
    const filesystem::path& sourceRoot,
    const filesystem::path& baselineManifestPath,
    const filesystem::path& currentManifestPath,
    const PolicyManifestRefreshOptions& options
) {
    PolicyGateManifest refreshed = currentManifest;
    refreshed.manifestRole = "refresh";
    refreshed.baselineManifestPath = baselineManifestPath.empty()
        ? string()
        : filesystem::absolute(baselineManifestPath).string();
    refreshed.baselineManifestHash = baselineManifestPath.empty()
        ? hash_policy_manifest_content(baselineManifest)
        : hash_policy_manifest_file(baselineManifestPath);

    assign_policy_manifest_input_hashes(refreshed, sourceRoot);
    if (!options.revalidateFamilies.empty()) {
        vector<PolicyGateFamilyResult> selectedFamilies;
        for (const PolicyGateFamilyResult& family : refreshed.families) {
            if (selected_for_revalidation(options, family.family)) {
                selectedFamilies.push_back(family);
            }
        }
        refreshed.families = std::move(selectedFamilies);
    }
    refreshed.currentManifestHash = currentManifestPath.empty()
        ? hash_policy_manifest_content(refreshed)
        : hash_policy_manifest_file(currentManifestPath);

    refreshed.freshFamilyCount = 0U;
    refreshed.staleFamilyCount = 0U;
    refreshed.requiresRerunFamilyCount = 0U;
    refreshed.reclassifyRequiredCount = 0U;
    refreshed.revalidatedFamilyCount = 0U;
    refreshed.staleFamilies.clear();
    refreshed.reclassifyRequiredFamilies.clear();

    for (PolicyGateFamilyResult& family : refreshed.families) {
        const PolicyGateFamilyResult* baselineFamily = find_family(baselineManifest, family.family);
        family.freshnessStatus = evaluate_family_freshness(family, baselineFamily, options);
        if (selected_for_revalidation(options, family.family)) {
            ++refreshed.revalidatedFamilyCount;
        }
        if (family.reclassifyRequired) {
            ++refreshed.reclassifyRequiredCount;
            append_unique_string(refreshed.reclassifyRequiredFamilies, family.family);
        }
        switch (family.freshnessStatus) {
            case PolicyFreshnessStatus::FRESH:
                ++refreshed.freshFamilyCount;
                break;
            case PolicyFreshnessStatus::STALE:
                ++refreshed.staleFamilyCount;
                append_unique_string(refreshed.staleFamilies, family.family);
                break;
            case PolicyFreshnessStatus::REQUIRES_RERUN:
                ++refreshed.requiresRerunFamilyCount;
                append_unique_string(refreshed.staleFamilies, family.family);
                break;
        }
    }
    return refreshed;
}

bool policy_manifest_freshness_satisfied(
    const PolicyGateManifest& manifest,
    const optional<string>& familyFilter
) {
    bool sawFamily = false;
    for (const PolicyGateFamilyResult& family : manifest.families) {
        if (familyFilter.has_value() && !familyFilter->empty() && family.family != *familyFilter) {
            continue;
        }
        sawFamily = true;
        if (!policy_freshness_status_is_satisfied(family.freshnessStatus)) {
            return false;
        }
    }
    return sawFamily;
}
