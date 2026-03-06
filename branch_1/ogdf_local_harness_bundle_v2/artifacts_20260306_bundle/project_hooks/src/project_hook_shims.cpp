#include "harness/types.hpp"
#include <string>

namespace {

bool failShim(std::string &why, const char *hook, const char *target) {
    why = std::string("project_hook_shims.cpp: TODO wire ") + hook +
          " -> " + target;
    return false;
}

} // namespace

bool validateRawSpqrDecomp(const harness::CompactGraph &,
                           const harness::RawSpqrDecomp &,
                           std::string &why) {
    return failShim(why, "validateRawSpqrDecomp",
                    "project::<validateRawSpqrDecomp>");
}

bool materializeMiniCore(const harness::CompactGraph &,
                         const harness::RawSpqrDecomp &,
                         harness::StaticMiniCore &,
                         std::string &why) {
    return failShim(why, "materializeMiniCore",
                    "project::<materializeMiniCore>");
}

void normalizeWholeMiniCore(harness::StaticMiniCore &) {
    // Fill this in once the project normalizer symbol is known.
}

bool checkOwnershipConsistency(const harness::StaticMiniCore &,
                               const harness::CompactGraph &,
                               std::string &why) {
    return failShim(why, "checkOwnershipConsistency",
                    "project::<checkOwnershipConsistency>");
}

bool checkReducedInvariant(const harness::StaticMiniCore &,
                           const harness::CompactGraph &,
                           std::string &why) {
    return failShim(why, "checkReducedInvariant",
                    "project::<checkReducedInvariant>");
}
