#include "harness/project_hooks.hpp"

#if defined(HARNESS_PROJECT_USE_FREE_FUNCTIONS)
bool validateRawSpqrDecomp(const harness::CompactGraph &,
                           const harness::RawSpqrDecomp &,
                           std::string &);
bool materializeMiniCore(const harness::CompactGraph &,
                         const harness::RawSpqrDecomp &,
                         harness::StaticMiniCore &,
                         std::string &);
void normalizeWholeMiniCore(harness::StaticMiniCore &);
bool checkOwnershipConsistency(const harness::StaticMiniCore &,
                               const harness::CompactGraph &,
                               std::string &);
bool checkReducedInvariant(const harness::StaticMiniCore &,
                           const harness::CompactGraph &,
                           std::string &);
bool buildDummyActualCoreEnvelope(const harness::CompactGraph &,
                                  harness::DummyActualEnvelope &,
                                  std::string &);
bool chooseKeepMiniNode(const harness::StaticMiniCore &,
                        int &,
                        std::string &);
#endif

namespace harness {

namespace {

bool failUnwired(std::string &why, const char *hook, const char *target) {
    why = std::string("ProjectHarnessOps: unwired hook: ") + hook +
          " (expected symbol: " + target + ")";
    return false;
}

} // namespace

bool ProjectHarnessOps::validateRawSpqrDecomp(const CompactGraph &H,
                                              const RawSpqrDecomp &raw,
                                              std::string &why) {
#if defined(HARNESS_PROJECT_USE_FREE_FUNCTIONS)
    if (::validateRawSpqrDecomp(H, raw, why)) return true;
    if (why.empty()) why = "validateRawSpqrDecomp returned false";
    return false;
#else
    (void)H;
    (void)raw;
    return failUnwired(why, "validateRawSpqrDecomp",
                       "validateRawSpqrDecomp(H, raw, why)");
#endif
}

bool ProjectHarnessOps::materializeMiniCore(const CompactGraph &H,
                                            const RawSpqrDecomp &raw,
                                            StaticMiniCore &mini,
                                            std::string &why) {
#if defined(HARNESS_PROJECT_USE_FREE_FUNCTIONS)
    if (::materializeMiniCore(H, raw, mini, why)) return true;
    if (why.empty()) why = "materializeMiniCore returned false";
    return false;
#else
    (void)H;
    (void)raw;
    (void)mini;
    return failUnwired(why, "materializeMiniCore",
                       "materializeMiniCore(H, raw, mini, why)");
#endif
}

void ProjectHarnessOps::normalizeWholeMiniCore(StaticMiniCore &mini) {
#if defined(HARNESS_PROJECT_USE_FREE_FUNCTIONS)
    ::normalizeWholeMiniCore(mini);
#else
    (void)mini;
#endif
}

bool ProjectHarnessOps::checkMiniOwnershipConsistency(const StaticMiniCore &mini,
                                                      const CompactGraph &H,
                                                      std::string &why) {
#if defined(HARNESS_PROJECT_USE_FREE_FUNCTIONS)
    if (::checkOwnershipConsistency(mini, H, why)) return true;
    if (why.empty()) why = "checkOwnershipConsistency returned false";
    return false;
#else
    (void)mini;
    (void)H;
    return failUnwired(why, "checkMiniOwnershipConsistency",
                       "checkOwnershipConsistency(mini, H, why)");
#endif
}

bool ProjectHarnessOps::checkMiniReducedInvariant(const StaticMiniCore &mini,
                                                  const CompactGraph &H,
                                                  std::string &why) {
#if defined(HARNESS_PROJECT_USE_FREE_FUNCTIONS)
    if (::checkReducedInvariant(mini, H, why)) return true;
    if (why.empty()) why = "checkReducedInvariant returned false";
    return false;
#else
    (void)mini;
    (void)H;
    return failUnwired(why, "checkMiniReducedInvariant",
                       "checkReducedInvariant(mini, H, why)");
#endif
}

bool ProjectHarnessOps::buildDummyActualCoreEnvelope(const CompactGraph &H,
                                                     DummyActualEnvelope &env,
                                                     std::string &why) {
#if defined(HARNESS_PROJECT_USE_FREE_FUNCTIONS)
    if (::buildDummyActualCoreEnvelope(H, env, why)) return true;
    if (why.empty()) why = "buildDummyActualCoreEnvelope returned false";
    return false;
#else
    (void)H;
    (void)env;
    return failUnwired(why, "buildDummyActualCoreEnvelope",
                       "buildDummyActualCoreEnvelope(H, env, why)");
#endif
}

bool ProjectHarnessOps::chooseKeepMiniNode(const StaticMiniCore &mini,
                                           int &keep,
                                           std::string &why) {
#if defined(HARNESS_PROJECT_USE_FREE_FUNCTIONS)
    if (::chooseKeepMiniNode(mini, keep, why)) return true;
    if (why.empty()) why = "chooseKeepMiniNode returned false";
    if (keep < 0) keep = -1;
    return false;
#else
    (void)mini;
    keep = -1;
    return failUnwired(why, "chooseKeepMiniNode",
                       "chooseKeepMiniNode(mini, keep, why)");
#endif
}

bool ProjectHarnessOps::graftMiniCoreIntoPlace(ReducedSPQRCore &,
                                               NodeId,
                                               const CompactGraph &,
                                               const StaticMiniCore &,
                                               int,
                                               std::queue<NodeId> &,
                                               GraftTrace *,
                                               std::string &why) {
    return failUnwired(why, "graftMiniCoreIntoPlace",
                       "graftMiniCoreIntoPlace(core, oldR, H, mini, keep, q, trace, why)");
}

bool ProjectHarnessOps::rebuildActualMetadata(ReducedSPQRCore &,
                                              std::string &why) {
    return failUnwired(why, "rebuildActualMetadata",
                       "rebuildActualMetadata(core, why)");
}

bool ProjectHarnessOps::checkActualReducedInvariant(const ReducedSPQRCore &,
                                                    const std::unordered_set<NodeId> *,
                                                    std::string &why) {
    return failUnwired(why, "checkActualReducedInvariant",
                       "checkReducedInvariantActual(core, allowedStubNodes, why)");
}

ExplicitBlockGraph ProjectHarnessOps::materializeWholeCoreExplicit(const ReducedSPQRCore &) {
    return {};
}

ExplicitBlockGraph ProjectHarnessOps::materializeCompactRealProjection(const CompactGraph &) {
    return {};
}

bool ProjectHarnessOps::checkEquivalentExplicitGraphs(const ExplicitBlockGraph &,
                                                      const ExplicitBlockGraph &,
                                                      std::string &why) {
    return failUnwired(why, "checkEquivalentExplicitGraphs",
                       "checkEquivalentExplicitGraphs(got, exp, why)");
}

bool ProjectHarnessOps::checkDummyProxyRewire(const DummyActualEnvelope &,
                                              const StaticMiniCore &,
                                              const GraftTrace &,
                                              std::string &why) {
    return failUnwired(why, "checkDummyProxyRewire",
                       "checkDummyProxyRewire(env, mini, trace, why)");
}

} // namespace harness
