#include "harness/runners.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

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

static ExplicitBlockGraph makeExplicitGraph(std::vector<VertexId> vertices,
                                            std::vector<ExplicitEdge> edges) {
    std::sort(vertices.begin(), vertices.end());
    vertices.erase(std::unique(vertices.begin(), vertices.end()), vertices.end());

    for (auto &edge : edges) {
        if (edge.u > edge.v) std::swap(edge.u, edge.v);
    }
    std::sort(edges.begin(), edges.end(), [](const ExplicitEdge &lhs, const ExplicitEdge &rhs) {
        return std::tie(lhs.id, lhs.u, lhs.v) < std::tie(rhs.id, rhs.u, rhs.v);
    });

    ExplicitBlockGraph G;
    G.vertices = std::move(vertices);
    G.edges = std::move(edges);
    return G;
}

static bool buildWholeCoreForTesting(const ExplicitBlockGraph &G,
                                     IRawSpqrBackend &,
                                     ReducedSPQRCore &core,
                                     std::string &why) {
    core = {};
    core.blockId = 0;
    core.root = 0;
    core.nodes.resize(1);

    auto &root = core.nodes[0];
    root.alive = true;
    root.type = SPQRType::R_NODE;

    std::unordered_set<VertexId> vertexSet(G.vertices.begin(), G.vertices.end());
    std::unordered_set<VertexId> usedVertices;
    std::unordered_set<EdgeId> seenEdgeIds;

    for (const auto &edge : G.edges) {
        if (edge.id < 0) {
            why = "buildWholeCoreForTesting: explicit edge id must be non-negative";
            return false;
        }
        if (edge.u < 0 || edge.v < 0) {
            why = "buildWholeCoreForTesting: explicit edge endpoint must be non-negative";
            return false;
        }
        if (!seenEdgeIds.insert(edge.id).second) {
            why = "buildWholeCoreForTesting: duplicate explicit edge id";
            return false;
        }
        if (!vertexSet.empty() &&
            (vertexSet.count(edge.u) == 0 || vertexSet.count(edge.v) == 0)) {
            why = "buildWholeCoreForTesting: explicit edge endpoint missing from vertex set";
            return false;
        }

        const auto [u, v] = canonPole(edge.u, edge.v);
        usedVertices.insert(u);
        usedVertices.insert(v);
        const int slotId = static_cast<int>(root.slots.size());
        root.slots.push_back({true, u, v, false, edge.id, -1});
        root.realEdgesHere.push_back(edge.id);
        core.ownerNodeOfRealEdge[edge.id] = 0;
        core.ownerSlotOfRealEdge[edge.id] = slotId;
        core.occ[u].push_back({0, slotId});
        if (u != v) core.occ[v].push_back({0, slotId});
    }

    std::sort(root.realEdgesHere.begin(), root.realEdgesHere.end());
    root.localAgg.edgeCnt = static_cast<int>(G.edges.size());
    root.localAgg.vertexCnt = static_cast<int>(usedVertices.size());
    root.localAgg.incCnt = static_cast<int>(G.edges.size()) * 2;
    if (!G.edges.empty()) root.localAgg.repEdge = G.edges.front().id;
    if (!usedVertices.empty()) root.localAgg.repVertex = *std::min_element(usedVertices.begin(), usedVertices.end());
    root.subAgg = root.localAgg;
    core.totalAgg = root.localAgg;
    return true;
}

static bool chooseDeterministicRewriteTarget(const ReducedSPQRCore &core,
                                             NodeId &chosenR,
                                             VertexId &chosenX,
                                             std::string &why) {
    chosenR = -1;
    chosenX = -1;

    for (NodeId nodeId = 0; nodeId < static_cast<NodeId>(core.nodes.size()); ++nodeId) {
        const auto &node = core.nodes[nodeId];
        if (!node.alive || node.type != SPQRType::R_NODE) continue;
        chosenR = nodeId;
        break;
    }
    if (chosenR < 0) {
        why = "chooseDeterministicRewriteTarget: no alive R-node";
        return false;
    }

    std::unordered_map<VertexId, int> countByVertex;
    const auto &node = core.nodes[chosenR];
    for (const auto &slot : node.slots) {
        if (!slot.alive) continue;
        ++countByVertex[slot.poleA];
        if (slot.poleB != slot.poleA) ++countByVertex[slot.poleB];
    }

    for (const auto &[vertex, count] : countByVertex) {
        if (count < 2) continue;
        if (chosenX < 0 || vertex < chosenX) chosenX = vertex;
    }
    if (chosenX < 0) {
        why = "chooseDeterministicRewriteTarget: no vertex with occurrence count >= 2 on chosen R-node";
        return false;
    }
    return true;
}

static bool rewriteRFallbackLocalHarness(ReducedSPQRCore &,
                                         NodeId,
                                         VertexId,
                                         std::string &why) {
    why = "rewriteR_fallback: unwired local harness hook";
    return false;
}

static bool normalizeTouchedRegionLocalHarness(ReducedSPQRCore &,
                                               std::string &why) {
    why = "normalizeTouchedRegion: unwired local harness hook";
    return false;
}

static void captureRewriteAfter(HarnessBundle &B,
                                IHarnessOps &ops,
                                const ReducedSPQRCore &core) {
    B.actualAfterRewrite = core;
    B.explicitAfter = ops.materializeWholeCoreExplicit(core);
}

std::vector<CompactGraph> buildManualCases() {
    std::vector<CompactGraph> out;
    out.push_back(makeTriangle(0));
    CompactGraph H = makeTriangle(10);
    H.edges.push_back({3, CompactEdgeKind::PROXY, 0, 1, -1, -1, -1, Agg{1,2,0,0,-1,H.origOfCv[0]}, -1});
    out.push_back(H);
    return out;
}

std::vector<ExplicitBlockGraph> buildManualRewriteCases() {
    std::vector<ExplicitBlockGraph> out;
    out.push_back(makeExplicitGraph({1, 2, 3, 4},
                                    {{1, 1, 2}, {2, 1, 3}, {3, 1, 4},
                                     {4, 2, 3}, {5, 2, 4}, {6, 3, 4}}));
    out.push_back(makeExplicitGraph({1, 2, 3, 4, 5},
                                    {{10, 1, 2}, {11, 1, 3}, {12, 1, 4}, {13, 1, 5},
                                     {14, 2, 3}, {15, 2, 4}, {16, 2, 5}, {17, 3, 4},
                                     {18, 3, 5}}));
    out.push_back(makeExplicitGraph({1, 2, 3, 4, 5},
                                    {{20, 1, 2}, {21, 2, 3}, {22, 3, 4}, {23, 4, 5},
                                     {24, 1, 5}, {25, 1, 3}, {26, 1, 4}, {27, 2, 4},
                                     {28, 2, 5}, {29, 3, 5}}));
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

HarnessResult runRewriteRFallbackCaseDumpAware(const ExplicitBlockGraph &G,
                                               IRawSpqrBackend &backend,
                                               IHarnessOps &ops,
                                               uint64_t seed,
                                               int tc,
                                               const std::string &dumpDir) {
    HarnessBundle B;
    B.seed = seed;
    B.tc = tc;
    B.backendName = backend.name();
    B.explicitInput = G;
    B.explicitBefore = G;

    ReducedSPQRCore before;
    std::string why;
    if (!buildWholeCoreForTesting(G, backend, before, why)) {
        setFailure(B, HarnessStage::LOCAL_BUILD_CORE_FAIL, "buildWholeCoreForTesting", why);
        return failAndDump(B, dumpDir);
    }

    B.actualBeforeRewrite = before;
    B.explicitBefore = ops.materializeWholeCoreExplicit(before);

    NodeId chosenR = -1;
    VertexId chosenX = -1;
    if (!chooseDeterministicRewriteTarget(before, chosenR, chosenX, why)) {
        B.actualAfterRewrite = before;
        setFailure(B, HarnessStage::LOCAL_CHOOSE_RX_FAIL, "chooseDeterministicRewriteTarget", why);
        return failAndDump(B, dumpDir);
    }
    B.chosenR = chosenR;
    B.chosenX = chosenX;

    ReducedSPQRCore after = before;
    if (!rewriteRFallbackLocalHarness(after, chosenR, chosenX, why)) {
        captureRewriteAfter(B, ops, after);
        setFailure(B, HarnessStage::LOCAL_REWRITE_R_FAIL, "rewriteR_fallback", why);
        return failAndDump(B, dumpDir);
    }

    if (!normalizeTouchedRegionLocalHarness(after, why)) {
        captureRewriteAfter(B, ops, after);
        setFailure(B, HarnessStage::LOCAL_NORMALIZE_FAIL, "normalizeTouchedRegion", why);
        return failAndDump(B, dumpDir);
    }
    captureRewriteAfter(B, ops, after);

    if (!ops.checkActualReducedInvariant(after, nullptr, why)) {
        setFailure(B, HarnessStage::LOCAL_ACTUAL_INVARIANT_FAIL, "checkActualReducedInvariant", why);
        return failAndDump(B, dumpDir);
    }

    ReducedSPQRCore oracle;
    if (!buildWholeCoreForTesting(*B.explicitAfter, backend, oracle, why)) {
        setFailure(B, HarnessStage::LOCAL_ORACLE_FAIL, "buildWholeCoreForTesting(oracle)", why);
        return failAndDump(B, dumpDir);
    }
    B.explicitExpected = ops.materializeWholeCoreExplicit(oracle);
    B.explicitGot = B.explicitAfter;
    if (!ops.checkEquivalentExplicitGraphs(*B.explicitGot, *B.explicitExpected, why)) {
        setFailure(B, HarnessStage::LOCAL_ORACLE_FAIL, "checkEquivalentExplicitGraphs", why);
        return failAndDump(B, dumpDir);
    }

    return {};
}

} // namespace harness
