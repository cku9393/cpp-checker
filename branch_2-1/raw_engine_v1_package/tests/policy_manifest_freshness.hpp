#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "policy_gate.hpp"

struct PolicyManifestRefreshOptions {
    bool freshnessOnly = false;
    bool markStaleOnHashChange = true;
    std::vector<std::string> revalidateFamilies;
};

PolicyRelevantInputHashes compute_policy_relevant_input_hashes(
    const std::filesystem::path& sourceRoot,
    const std::string& family,
    const std::vector<PolicyEvidenceSource>& evidenceSources
);

void assign_policy_manifest_input_hashes(
    PolicyGateManifest& manifest,
    const std::filesystem::path& sourceRoot
);

std::string hash_policy_manifest_file(const std::filesystem::path& manifestPath);
std::string hash_policy_manifest_content(const PolicyGateManifest& manifest);

PolicyGateManifest refresh_policy_gate_manifest(
    const PolicyGateManifest& baselineManifest,
    const PolicyGateManifest& currentManifest,
    const std::filesystem::path& sourceRoot,
    const std::filesystem::path& baselineManifestPath,
    const std::filesystem::path& currentManifestPath,
    const PolicyManifestRefreshOptions& options
);

bool policy_manifest_freshness_satisfied(
    const PolicyGateManifest& manifest,
    const std::optional<std::string>& familyFilter = std::nullopt
);
