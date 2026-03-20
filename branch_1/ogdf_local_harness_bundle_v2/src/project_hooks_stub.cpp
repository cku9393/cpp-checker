#include "harness/project_hooks.hpp"

namespace harness {

static bool failStub(std::string &why, const char *what) {
    why = std::string("StubHarnessOps: replace this hook with project integration: ") + what;
    return false;
}

bool StubHarnessOps::validateRawSpqrDecomp(const CompactGraph &, const RawSpqrDecomp &, std::string &why) { return failStub(why, "validateRawSpqrDecomp"); }
bool StubHarnessOps::materializeMiniCore(const CompactGraph &, const RawSpqrDecomp &, StaticMiniCore &, std::string &why) { return failStub(why, "materializeMiniCore"); }
void StubHarnessOps::normalizeWholeMiniCore(StaticMiniCore &) {}
bool StubHarnessOps::checkMiniOwnershipConsistency(const StaticMiniCore &, const CompactGraph &, std::string &why) { return failStub(why, "checkMiniOwnershipConsistency"); }
bool StubHarnessOps::checkMiniReducedInvariant(const StaticMiniCore &, const CompactGraph &, std::string &why) { return failStub(why, "checkMiniReducedInvariant"); }
bool StubHarnessOps::buildDummyActualCoreEnvelope(const CompactGraph &, DummyActualEnvelope &, std::string &why) { return failStub(why, "buildDummyActualCoreEnvelope"); }
bool StubHarnessOps::chooseKeepMiniNode(const StaticMiniCore &, int &keep, std::string &why) { keep = -1; return failStub(why, "chooseKeepMiniNode"); }
bool StubHarnessOps::graftMiniCoreIntoPlace(ReducedSPQRCore &, NodeId, const CompactGraph &, const StaticMiniCore &, int, std::queue<NodeId> &, GraftTrace *, std::string &why) { return failStub(why, "graftMiniCoreIntoPlace"); }
bool StubHarnessOps::rebuildActualMetadata(ReducedSPQRCore &, std::string &why) { return failStub(why, "rebuildActualMetadata"); }
bool StubHarnessOps::checkActualReducedInvariant(const ReducedSPQRCore &, const std::unordered_set<NodeId> *, std::string &why) { return failStub(why, "checkActualReducedInvariant"); }
ExplicitBlockGraph StubHarnessOps::materializeWholeCoreExplicit(const ReducedSPQRCore &) { return {}; }
ExplicitBlockGraph StubHarnessOps::materializeCompactRealProjection(const CompactGraph &) { return {}; }
bool StubHarnessOps::checkEquivalentExplicitGraphs(const ExplicitBlockGraph &, const ExplicitBlockGraph &, std::string &why) { return failStub(why, "checkEquivalentExplicitGraphs"); }
bool StubHarnessOps::checkDummyProxyRewire(const DummyActualEnvelope &, const StaticMiniCore &, const GraftTrace &, std::string &why) { return failStub(why, "checkDummyProxyRewire"); }
bool StubHarnessOps::rewriteRFallback(ReducedSPQRCore &, NodeId, VertexId, std::string &why) { return failStub(why, "rewriteRFallback"); }
bool StubHarnessOps::normalizeTouchedRegion(ReducedSPQRCore &, std::string &why) { return failStub(why, "normalizeTouchedRegion"); }

} // namespace harness
