#include "harness/runners.hpp"
#include <random>

namespace harness {

static CompactGraph makeTriangle(int offset) {
    CompactGraph H;
    H.block = offset;
    H.origOfCv = {offset + 1, offset + 2, offset + 3};
    for (int i = 0; i < 3; ++i) H.cvOfOrig[H.origOfCv[i]] = i;
    H.touchedVertices = H.origOfCv;
    H.edges = {
        {0, CompactEdgeKind::REAL, 0, 1, offset * 10 + 1},
        {1, CompactEdgeKind::REAL, 1, 2, offset * 10 + 2},
        {2, CompactEdgeKind::REAL, 2, 0, offset * 10 + 3},
    };
    return H;
}

std::vector<CompactGraph> buildManualCases() {
    std::vector<CompactGraph> out;
    out.push_back(makeTriangle(0));
    CompactGraph H = makeTriangle(10);
    H.edges.push_back({3, CompactEdgeKind::PROXY, 0, 1, -1, -1, -1, Agg{1,2,0,0,-1,H.origOfCv[0]}, -1});
    out.push_back(H);
    return out;
}

CompactGraph makeRandomCompactGraph(uint64_t seed, int caseIndex) {
    std::mt19937_64 rng(seed + caseIndex * 239017ULL);
    int n = 4 + (int)(rng() % 3);
    CompactGraph H;
    H.block = caseIndex;
    for (int i = 0; i < n; ++i) {
        H.origOfCv.push_back(i + 1);
        H.cvOfOrig[i + 1] = i;
        H.touchedVertices.push_back(i + 1);
    }
    int id = 0;
    for (int i = 0; i < n; ++i) {
        int j = (i + 1) % n;
        H.edges.push_back({id++, CompactEdgeKind::REAL, i, j, id});
    }
    if (rng() & 1ULL) {
        H.edges.push_back({id++, CompactEdgeKind::PROXY, 0, 2, -1, -1, -1, Agg{1,2,0,0,-1,H.origOfCv[0]}, -1});
    }
    return H;
}

HarnessResult runStaticPipelineCaseDumpAware(const CompactGraph &H,
                                             IRawSpqrBackend &backend,
                                             IHarnessOps &ops,
                                             uint64_t seed,
                                             int tc,
                                             const std::string &dumpDir) {
    HarnessBundle B = makeBaseBundle(seed, tc, backend.name(), H);

    RawSpqrDecomp raw;
    std::string err;
    if (!backend.buildRaw(H, raw, err)) {
        setFailure(B, HarnessStage::RAW_BACKEND_FAIL, "backend.buildRaw", err.empty() ? "raw backend failed" : err);
        return failAndDump(B, dumpDir);
    }
    B.raw = raw;

    std::string why;
    if (!ops.validateRawSpqrDecomp(H, raw, why)) {
        setFailure(B, HarnessStage::RAW_VALIDATE_FAIL, "validateRawSpqrDecomp", why);
        return failAndDump(B, dumpDir);
    }

    StaticMiniCore mini;
    if (!ops.materializeMiniCore(H, raw, mini, why)) {
        B.miniBeforeNormalize = mini;
        setFailure(B, HarnessStage::MINI_MATERIALIZE_FAIL, "materializeMiniCore", why);
        return failAndDump(B, dumpDir);
    }
    B.miniBeforeNormalize = mini;

    if (!ops.checkMiniOwnershipConsistency(mini, H, why) ||
        !ops.checkMiniReducedInvariant(mini, H, why)) {
        setFailure(B, HarnessStage::MINI_PRECHECK_FAIL, "mini precheck", why);
        return failAndDump(B, dumpDir);
    }

    try {
        ops.normalizeWholeMiniCore(mini);
    } catch (const std::exception &e) {
        B.miniAfterNormalize = mini;
        setFailure(B, HarnessStage::MINI_NORMALIZE_THROW, "normalizeWholeMiniCore", e.what());
        return failAndDump(B, dumpDir);
    } catch (...) {
        B.miniAfterNormalize = mini;
        setFailure(B, HarnessStage::MINI_NORMALIZE_THROW, "normalizeWholeMiniCore", "unknown exception");
        return failAndDump(B, dumpDir);
    }
    B.miniAfterNormalize = mini;

    if (!ops.checkMiniOwnershipConsistency(mini, H, why) ||
        !ops.checkMiniReducedInvariant(mini, H, why)) {
        setFailure(B, HarnessStage::MINI_POSTCHECK_FAIL, "mini postcheck", why);
        return failAndDump(B, dumpDir);
    }

    return {};
}

HarnessResult runDummyGraftCaseDumpAware(const CompactGraph &H,
                                         IRawSpqrBackend &backend,
                                         IHarnessOps &ops,
                                         uint64_t seed,
                                         int tc,
                                         const std::string &dumpDir) {
    HarnessBundle B = makeBaseBundle(seed, tc, backend.name(), H);

    RawSpqrDecomp raw;
    std::string err;
    if (!backend.buildRaw(H, raw, err)) {
        setFailure(B, HarnessStage::RAW_BACKEND_FAIL, "backend.buildRaw", err.empty() ? "raw backend failed" : err);
        return failAndDump(B, dumpDir);
    }
    B.raw = raw;

    std::string why;
    if (!ops.validateRawSpqrDecomp(H, raw, why)) {
        setFailure(B, HarnessStage::RAW_VALIDATE_FAIL, "validateRawSpqrDecomp", why);
        return failAndDump(B, dumpDir);
    }

    StaticMiniCore mini;
    if (!ops.materializeMiniCore(H, raw, mini, why)) {
        B.miniBeforeNormalize = mini;
        setFailure(B, HarnessStage::MINI_MATERIALIZE_FAIL, "materializeMiniCore", why);
        return failAndDump(B, dumpDir);
    }
    B.miniBeforeNormalize = mini;

    try {
        ops.normalizeWholeMiniCore(mini);
    } catch (const std::exception &e) {
        B.miniAfterNormalize = mini;
        setFailure(B, HarnessStage::MINI_NORMALIZE_THROW, "normalizeWholeMiniCore", e.what());
        return failAndDump(B, dumpDir);
    } catch (...) {
        B.miniAfterNormalize = mini;
        setFailure(B, HarnessStage::MINI_NORMALIZE_THROW, "normalizeWholeMiniCore", "unknown exception");
        return failAndDump(B, dumpDir);
    }
    B.miniAfterNormalize = mini;

    if (!ops.checkMiniOwnershipConsistency(mini, H, why) ||
        !ops.checkMiniReducedInvariant(mini, H, why)) {
        setFailure(B, HarnessStage::MINI_POSTCHECK_FAIL, "mini postcheck", why);
        return failAndDump(B, dumpDir);
    }

    DummyActualEnvelope env;
    if (!ops.buildDummyActualCoreEnvelope(H, env, why)) {
        setFailure(B, HarnessStage::DUMMY_ENVELOPE_FAIL, "buildDummyActualCoreEnvelope", why);
        return failAndDump(B, dumpDir);
    }
    B.actualBeforeGraft = env.core;

    int keep = -1;
    if (!ops.chooseKeepMiniNode(mini, keep, why) || keep < 0) {
        setFailure(B, HarnessStage::KEEP_SELECT_FAIL, "chooseKeepMiniNode", why.empty() ? "invalid keep" : why);
        return failAndDump(B, dumpDir);
    }

    std::queue<NodeId> q;
    GraftTrace trace;
    if (!ops.graftMiniCoreIntoPlace(env.core, env.oldR, env.H, mini, keep, q, &trace, why)) {
        B.actualAfterGraft = env.core;
        setFailure(B, HarnessStage::GRAFT_FAIL, "graftMiniCoreIntoPlace", why.empty() ? "returned false" : why);
        return failAndDump(B, dumpDir);
    }
    B.actualAfterGraft = env.core;
    B.trace = trace;

    if (!ops.rebuildActualMetadata(env.core, why)) {
        B.actualAfterGraft = env.core;
        setFailure(B, HarnessStage::ACTUAL_METADATA_FAIL, "rebuildActualMetadata", why);
        return failAndDump(B, dumpDir);
    }
    B.actualAfterGraft = env.core;

    if (!ops.checkActualReducedInvariant(env.core, &env.stubNodes, why)) {
        setFailure(B, HarnessStage::ACTUAL_INVARIANT_FAIL, "checkActualReducedInvariant", why);
        return failAndDump(B, dumpDir);
    }

    B.explicitExpected = ops.materializeCompactRealProjection(env.H);
    B.explicitGot = ops.materializeWholeCoreExplicit(env.core);
    if (!ops.checkEquivalentExplicitGraphs(*B.explicitGot, *B.explicitExpected, why)) {
        setFailure(B, HarnessStage::DUMMY_REAL_SET_FAIL, "checkEquivalentExplicitGraphs", why);
        return failAndDump(B, dumpDir);
    }

    if (!ops.checkDummyProxyRewire(env, mini, trace, why)) {
        setFailure(B, HarnessStage::DUMMY_PROXY_REWIRE_FAIL, "checkDummyProxyRewire", why);
        return failAndDump(B, dumpDir);
    }

    return {};
}

} // namespace harness
