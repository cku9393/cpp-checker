#include "harness/project_static_adapter.hpp"
#include "harness/types.hpp"
#include <string>

namespace {

bool failShim(std::string &why, const char *hook, const char *target) {
    why = std::string("project_hook_shims.cpp: TODO wire ") + hook +
          " -> " + target;
    return false;
}

} // namespace

bool validateRawSpqrDecomp(const harness::CompactGraph &H,
                           const harness::RawSpqrDecomp &raw,
                           std::string &why) {
    harness::ProjectRawSnapshot snap;
    if (!harness::buildProjectRawSnapshot(H, raw, snap, why)) return false;
    return harness::checkProjectRawSnapshot(snap, why);
}

bool materializeMiniCore(const harness::CompactGraph &H,
                         const harness::RawSpqrDecomp &raw,
                         harness::StaticMiniCore &mini,
                         std::string &why) {
    harness::ProjectRawSnapshot snap;
    if (!harness::buildProjectRawSnapshot(H, raw, snap, why)) return false;

    harness::ProjectMiniCore pm;
    if (!harness::buildProjectMiniCore(H, snap, pm, why)) return false;

    return harness::exportProjectMiniCore(pm, mini, why);
}

void normalizeWholeMiniCore(harness::StaticMiniCore &) {
    // Fill this in once the project normalizer symbol is known.
}

bool checkOwnershipConsistency(const harness::StaticMiniCore &mini,
                               const harness::CompactGraph &H,
                               std::string &why) {
    harness::ProjectMiniCore pm;
    if (!harness::importStaticMiniCore(mini, pm, why)) return false;
    return harness::checkProjectMiniOwnershipConsistency(H, pm, why);
}

bool checkReducedInvariant(const harness::StaticMiniCore &mini,
                           const harness::CompactGraph &H,
                           std::string &why) {
    harness::ProjectMiniCore pm;
    if (!harness::importStaticMiniCore(mini, pm, why)) return false;
    return harness::checkProjectMiniReducedInvariant(H, pm, why);
}
