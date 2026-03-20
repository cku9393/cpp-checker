#include "harness/types.hpp"
#include <string>

// Copy the five bodies below into project_hook_shims.cpp and replace
// project::<...> with the actual symbols from the project codebase.

bool validateRawSpqrDecomp(const harness::CompactGraph &H,
                           const harness::RawSpqrDecomp &raw,
                           std::string &why) {
    return project::validateRawSpqrDecomp(H, raw, why);
}

bool materializeMiniCore(const harness::CompactGraph &H,
                         const harness::RawSpqrDecomp &raw,
                         harness::StaticMiniCore &mini,
                         std::string &why) {
    return project::materializeMiniCore(H, raw, mini, why);
}

void normalizeWholeMiniCore(harness::StaticMiniCore &mini) {
    project::normalizeWholeMiniCore(mini);
}

bool checkOwnershipConsistency(const harness::StaticMiniCore &mini,
                               const harness::CompactGraph &H,
                               std::string &why) {
    return project::checkOwnershipConsistency(mini, H, why);
}

bool checkReducedInvariant(const harness::StaticMiniCore &mini,
                           const harness::CompactGraph &H,
                           std::string &why) {
    return project::checkReducedInvariant(mini, H, why);
}
