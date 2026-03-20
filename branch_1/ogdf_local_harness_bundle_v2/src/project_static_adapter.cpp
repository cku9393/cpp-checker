#include "harness/project_static_adapter.hpp"
#include "harness/dump.hpp"
#include "harness/ogdf_wrapper.hpp"
#include "harness/project_hooks.hpp"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <queue>
#include <sstream>
#include <tuple>

bool validateRawSpqrDecomp(const harness::CompactGraph &,
                           const harness::RawSpqrDecomp &,
                           std::string &);
bool materializeMiniCore(const harness::CompactGraph &,
                         const harness::RawSpqrDecomp &,
                         harness::StaticMiniCore &,
                         std::string &);
void normalizeWholeMiniCore(harness::StaticMiniCore &);
bool buildDummyActualCoreEnvelope(const harness::CompactGraph &,
                                  harness::DummyActualEnvelope &,
                                  std::string &);
bool chooseKeepMiniNode(const harness::StaticMiniCore &,
                        int &,
                        std::string &);
bool graftMiniCoreIntoPlace(harness::ReducedSPQRCore &,
                            harness::NodeId,
                            const harness::CompactGraph &,
                            const harness::StaticMiniCore &,
                            int,
                            std::queue<harness::NodeId> &,
                            harness::GraftTrace *,
                            std::string &);

namespace harness {

namespace {

void dumpCompactBCResult(const CompactBCResult &bc, std::ostream &os);

bool fail(std::string &why, const std::string &msg) {
    why = msg;
    return false;
}

bool failSlot(std::string &why,
              int nodeId,
              int slotId,
              const std::string &msg) {
    std::ostringstream oss;
    oss << "ProjectRawSnapshot node " << nodeId
        << " slot " << slotId << ": " << msg;
    return fail(why, oss.str());
}

bool failNode(std::string &why,
              int nodeId,
              const std::string &msg) {
    std::ostringstream oss;
    oss << "ProjectRawSnapshot node " << nodeId << ": " << msg;
    return fail(why, oss.str());
}

bool failTree(std::string &why,
              int treeId,
              const std::string &msg) {
    std::ostringstream oss;
    oss << "ProjectRawSnapshot tree edge " << treeId << ": " << msg;
    return fail(why, oss.str());
}

bool failInput(std::string &why,
               int inputId,
               const std::string &msg) {
    std::ostringstream oss;
    oss << "ProjectRawSnapshot input edge " << inputId << ": " << msg;
    return fail(why, oss.str());
}

bool buildWholeCoreForSequenceTesting(const ExplicitBlockGraph &G,
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
            why = "buildWholeCoreForSequenceTesting: explicit edge id must be non-negative";
            return false;
        }
        if (edge.u < 0 || edge.v < 0) {
            why = "buildWholeCoreForSequenceTesting: explicit edge endpoint must be non-negative";
            return false;
        }
        if (!seenEdgeIds.insert(edge.id).second) {
            why = "buildWholeCoreForSequenceTesting: duplicate explicit edge id";
            return false;
        }
        if (!vertexSet.empty() &&
            (vertexSet.count(edge.u) == 0 || vertexSet.count(edge.v) == 0)) {
            why = "buildWholeCoreForSequenceTesting: explicit edge endpoint missing from vertex set";
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
    if (!usedVertices.empty()) {
        root.localAgg.repVertex = *std::min_element(usedVertices.begin(), usedVertices.end());
    }
    root.subAgg = root.localAgg;
    core.totalAgg = root.localAgg;
    why.clear();
    return true;
}

enum class SequenceFixpointChooseStatus : uint8_t {
    FOUND,
    NONE,
    ERROR
};

SequenceFixpointChooseStatus chooseDeterministicSequenceRewriteTargetForFixpoint(
    const ReducedSPQRCore &core,
    NodeId &chosenR,
    VertexId &chosenX,
    std::string &why) {
    chosenR = -1;
    chosenX = -1;
    why.clear();

    for (NodeId nodeId = 0; nodeId < static_cast<NodeId>(core.nodes.size()); ++nodeId) {
        const auto &node = core.nodes[nodeId];
        if (!node.alive || node.type != SPQRType::R_NODE) continue;

        std::unordered_map<VertexId, int> realCountByVertex;
        for (const auto &slot : node.slots) {
            if (!slot.alive || slot.isVirtual) continue;
            if (slot.poleA < 0 || slot.poleB < 0) {
                why = "chooseDeterministicSequenceRewriteTarget: invalid REAL slot pole";
                return SequenceFixpointChooseStatus::ERROR;
            }
            ++realCountByVertex[slot.poleA];
            if (slot.poleB != slot.poleA) ++realCountByVertex[slot.poleB];
        }

        VertexId bestX = -1;
        int bestCount = -1;
        for (const auto &[vertex, count] : realCountByVertex) {
            if (count < 2) continue;
            if (count > bestCount || (count == bestCount && (bestX < 0 || vertex < bestX))) {
                bestX = vertex;
                bestCount = count;
            }
        }

        if (bestX >= 0) {
            chosenR = nodeId;
            chosenX = bestX;
            return SequenceFixpointChooseStatus::FOUND;
        }
    }

    return SequenceFixpointChooseStatus::NONE;
}

bool validNodeId(const ProjectRawSnapshot &snap, int nodeId) {
    return nodeId >= 0 && nodeId < static_cast<int>(snap.nodes.size());
}

bool validTreeId(const ProjectRawSnapshot &snap, int treeId) {
    return treeId >= 0 && treeId < static_cast<int>(snap.treeEdges.size());
}

bool validSlotId(const ProjectRawNode &node, int slotId) {
    return slotId >= 0 && slotId < static_cast<int>(node.slots.size());
}

bool samePoles(VertexId a0, VertexId b0, VertexId a1, VertexId b1) {
    return canonPole(a0, b0) == canonPole(a1, b1);
}

bool validInputId(const CompactGraph &H, int inputId) {
    return inputId >= 0 && inputId < static_cast<int>(H.edges.size());
}

bool validMiniNodeId(const ProjectMiniCore &mini, int nodeId) {
    return nodeId >= 0 && nodeId < static_cast<int>(mini.nodes.size());
}

bool validMiniArcId(const ProjectMiniCore &mini, int arcId) {
    return arcId >= 0 && arcId < static_cast<int>(mini.arcs.size());
}

bool validMiniSlotId(const ProjectMiniNode &node, int slotId) {
    return slotId >= 0 && slotId < static_cast<int>(node.slots.size());
}

bool failMiniSlot(std::string &why,
                  int nodeId,
                  int slotId,
                  const std::string &msg) {
    std::ostringstream oss;
    oss << "ProjectMiniCore node " << nodeId
        << " slot " << slotId << ": " << msg;
    return fail(why, oss.str());
}

bool failMiniArc(std::string &why,
                 int arcId,
                 const std::string &msg) {
    std::ostringstream oss;
    oss << "ProjectMiniCore arc " << arcId << ": " << msg;
    return fail(why, oss.str());
}

bool failMiniInput(std::string &why,
                   int inputId,
                   const std::string &msg) {
    std::ostringstream oss;
    oss << "ProjectMiniCore input edge " << inputId << ": " << msg;
    return fail(why, oss.str());
}

bool failMiniNode(std::string &why,
                  int nodeId,
                  const std::string &msg) {
    std::ostringstream oss;
    oss << "ProjectMiniCore node " << nodeId << ": " << msg;
    return fail(why, oss.str());
}

bool validActualNodeId(const ReducedSPQRCore &core, int nodeId) {
    return nodeId >= 0 && nodeId < static_cast<int>(core.nodes.size());
}

bool validActualSlotId(const SPQRNodeCore &node, int slotId) {
    return slotId >= 0 && slotId < static_cast<int>(node.slots.size());
}

bool validActualArcId(const ReducedSPQRCore &core, int arcId) {
    return arcId >= 0 && arcId < static_cast<int>(core.arcs.size());
}

void addActualAdjArc(SPQRNodeCore &node, ArcId arcId) {
    if (std::find(node.adjArcs.begin(), node.adjArcs.end(), arcId) == node.adjArcs.end()) {
        node.adjArcs.push_back(arcId);
    }
}

void removeActualAdjArc(SPQRNodeCore &node, ArcId arcId) {
    node.adjArcs.erase(std::remove(node.adjArcs.begin(), node.adjArcs.end(), arcId),
                       node.adjArcs.end());
}

void eraseActualRealOwnershipForNode(ReducedSPQRCore &core, NodeId nodeId) {
    for (auto it = core.ownerNodeOfRealEdge.begin(); it != core.ownerNodeOfRealEdge.end();) {
        if (it->second != nodeId) {
            ++it;
            continue;
        }
        core.ownerSlotOfRealEdge.erase(it->first);
        it = core.ownerNodeOfRealEdge.erase(it);
    }
}

void killActualArcForPreserveClear(ReducedSPQRCore &core, ArcId arcId) {
    if (!validActualArcId(core, arcId)) return;
    auto &arc = core.arcs[arcId];
    if (!arc.alive) return;

    auto clearEndpoint = [&](NodeId nodeId, int slotId) {
        if (!validActualNodeId(core, nodeId)) return;
        auto &node = core.nodes[nodeId];
        removeActualAdjArc(node, arcId);
        if (!validActualSlotId(node, slotId)) return;
        auto &slot = node.slots[slotId];
        slot.alive = false;
        if (slot.isVirtual) slot.arcId = -1;
    };

    clearEndpoint(arc.a, arc.slotInA);
    clearEndpoint(arc.b, arc.slotInB);
    arc.alive = false;
}

void addAgg(Agg &dst, const Agg &src);

NodeId otherEndpointOfArc(const SPQRArcCore &arc, NodeId nodeId) {
    if (arc.a == nodeId) return arc.b;
    if (arc.b == nodeId) return arc.a;
    return -1;
}

Agg computeAggToward(const ReducedSPQRCore &core,
                     NodeId blocked,
                     NodeId start) {
    Agg total;
    if (!validActualNodeId(core, blocked) || !validActualNodeId(core, start)) return total;
    if (!core.nodes[start].alive) return total;

    std::vector<char> seen(core.nodes.size(), 0);
    std::queue<NodeId> q;
    seen[blocked] = 1;
    seen[start] = 1;
    q.push(start);

    while (!q.empty()) {
        const NodeId nodeId = q.front();
        q.pop();

        const auto &node = core.nodes[nodeId];
        if (!node.alive) continue;
        addAgg(total, node.localAgg);

        for (ArcId arcId : node.adjArcs) {
            if (!validActualArcId(core, arcId)) continue;
            const auto &arc = core.arcs[arcId];
            if (!arc.alive) continue;
            const NodeId other = otherEndpointOfArc(arc, nodeId);
            if (!validActualNodeId(core, other) || seen[other]) continue;
            if (!core.nodes[other].alive) continue;
            seen[other] = 1;
            q.push(other);
        }
    }

    return total;
}

void rebuildActualRealEdgesHereNode(SPQRNodeCore &node) {
    node.realEdgesHere.clear();
    for (const auto &slot : node.slots) {
        if (!slot.alive || slot.isVirtual || slot.realEdge < 0) continue;
        node.realEdgesHere.push_back(slot.realEdge);
    }
    std::sort(node.realEdgesHere.begin(), node.realEdgesHere.end());
}

Agg recomputeActualLocalAgg(const SPQRNodeCore &node,
                            const std::unordered_set<VertexId> &touched) {
    Agg agg;
    std::unordered_set<VertexId> liveVertices;
    std::unordered_set<VertexId> watchedVertices;

    for (const auto &slot : node.slots) {
        if (!slot.alive || slot.isVirtual) continue;
        ++agg.edgeCnt;
        agg.incCnt += 2;
        if (agg.repEdge < 0) agg.repEdge = slot.realEdge;
        if (agg.repVertex < 0) agg.repVertex = slot.poleA;
        liveVertices.insert(slot.poleA);
        liveVertices.insert(slot.poleB);
        if (touched.count(slot.poleA)) watchedVertices.insert(slot.poleA);
        if (touched.count(slot.poleB)) watchedVertices.insert(slot.poleB);
    }

    agg.vertexCnt = static_cast<int>(liveVertices.size());
    agg.watchedCnt = static_cast<int>(watchedVertices.size());
    return agg;
}

void recomputeWholeActualTotalAgg(ReducedSPQRCore &core,
                                  const std::unordered_set<VertexId> &touched) {
    Agg total;
    std::unordered_set<VertexId> liveVertices;
    std::unordered_set<VertexId> watchedVertices;

    for (const auto &node : core.nodes) {
        if (!node.alive) continue;
        for (const auto &slot : node.slots) {
            if (!slot.alive || slot.isVirtual || slot.realEdge < 0) continue;
            ++total.edgeCnt;
            total.incCnt += 2;
            if (total.repEdge < 0) total.repEdge = slot.realEdge;
            if (total.repVertex < 0) total.repVertex = slot.poleA;
            liveVertices.insert(slot.poleA);
            liveVertices.insert(slot.poleB);
            if (touched.count(slot.poleA)) watchedVertices.insert(slot.poleA);
            if (touched.count(slot.poleB)) watchedVertices.insert(slot.poleB);
        }
    }

    total.vertexCnt = static_cast<int>(liveVertices.size());
    total.watchedCnt = static_cast<int>(watchedVertices.size());
    core.totalAgg = total;
}

ProjectExplicitEdge makeProjectExplicitEdge(EdgeId id,
                                            VertexId a,
                                            VertexId b) {
    const auto poles = canonPole(a, b);
    return {id, poles.first, poles.second};
}

bool explicitEdgeLess(const ProjectExplicitEdge &lhs,
                      const ProjectExplicitEdge &rhs) {
    return std::tie(lhs.id, lhs.u, lhs.v) < std::tie(rhs.id, rhs.u, rhs.v);
}

void canonicalizeProjectExplicitGraph(ProjectExplicitBlockGraph &graph) {
    std::sort(graph.edges.begin(), graph.edges.end(), explicitEdgeLess);
    std::sort(graph.vertices.begin(), graph.vertices.end());
    graph.vertices.erase(std::unique(graph.vertices.begin(), graph.vertices.end()),
                         graph.vertices.end());
}

std::string formatExplicitEdge(const ProjectExplicitEdge &edge) {
    std::ostringstream oss;
    oss << '(' << edge.id << ',' << edge.u << ',' << edge.v << ')';
    return oss.str();
}

void addAgg(Agg &dst, const Agg &src) {
    dst.edgeCnt += src.edgeCnt;
    dst.vertexCnt += src.vertexCnt;
    dst.watchedCnt += src.watchedCnt;
    dst.incCnt += src.incCnt;
    if (dst.repEdge < 0 && src.repEdge >= 0) dst.repEdge = src.repEdge;
    if (dst.repVertex < 0 && src.repVertex >= 0) dst.repVertex = src.repVertex;
}

Agg makeRealInputAgg(const CompactGraph &H,
                     const CompactEdge &edge) {
    Agg out;
    out.edgeCnt = 1;
    out.incCnt = 2;
    out.repEdge = edge.realEdge;
    if (edge.a >= 0 && edge.a < static_cast<int>(H.origOfCv.size())) {
        out.repVertex = H.origOfCv[edge.a];
    }
    return out;
}

void recomputePayloadAgg(const CompactGraph &H, ProjectMiniCore &mini) {
    std::unordered_set<VertexId> touched(H.touchedVertices.begin(), H.touchedVertices.end());

    for (auto &node : mini.nodes) {
        node.localAgg = {};
        node.payloadAgg = {};
        if (!node.alive) continue;

        std::unordered_set<VertexId> localVertices;
        std::unordered_set<VertexId> localWatched;

        for (const auto &slot : node.slots) {
            if (!slot.alive || !validInputId(H, slot.inputEdgeId)) continue;

            const auto &edge = H.edges[slot.inputEdgeId];
            if (slot.kind == MiniSlotKind::REAL_INPUT) {
                Agg local = makeRealInputAgg(H, edge);
                addAgg(node.localAgg, local);
                localVertices.insert(slot.poleA);
                localVertices.insert(slot.poleB);
                if (touched.count(slot.poleA)) localWatched.insert(slot.poleA);
                if (touched.count(slot.poleB)) localWatched.insert(slot.poleB);
                continue;
            }
            if (slot.kind == MiniSlotKind::PROXY_INPUT) {
                addAgg(node.payloadAgg, edge.sideAgg);
            }
        }

        node.localAgg.vertexCnt = static_cast<int>(localVertices.size());
        node.localAgg.watchedCnt = static_cast<int>(localWatched.size());
        addAgg(node.payloadAgg, node.localAgg);
    }
}

CoreKind computeMiniKind(const ProjectMiniCore &mini) {
    int aliveNodeCount = 0;
    int aliveArcCount = 0;
    for (const auto &node : mini.nodes) {
        if (node.alive) ++aliveNodeCount;
    }
    for (const auto &arc : mini.arcs) {
        if (arc.alive) ++aliveArcCount;
    }
    if (aliveNodeCount <= 1 && aliveArcCount == 0) return CoreKind::TINY;
    return CoreKind::REDUCED_SPQR;
}

void canonicalizeExplicitGraph(ExplicitBlockGraph &graph) {
    for (auto &edge : graph.edges) {
        if (edge.u > edge.v) std::swap(edge.u, edge.v);
    }
    std::sort(graph.edges.begin(), graph.edges.end(), [](const ExplicitEdge &lhs,
                                                         const ExplicitEdge &rhs) {
        return std::tie(lhs.id, lhs.u, lhs.v) < std::tie(rhs.id, rhs.u, rhs.v);
    });
    std::sort(graph.vertices.begin(), graph.vertices.end());
    graph.vertices.erase(std::unique(graph.vertices.begin(), graph.vertices.end()),
                         graph.vertices.end());
}

bool shouldFallbackToWholeCoreRebuild(const std::string &why) {
    return why.find("S skeleton must have at least 3 edges") != std::string::npos ||
           why.find("not biconnected") != std::string::npos;
}

struct RewriteRCaseContext {
    uint64_t seed = 0;
    int tc = -1;
    bool sequenceMode = false;
    int stepIndex = -1;
    int sequenceLengthSoFar = 0;
    NodeId currentRNode = -1;
    VertexId currentX = -1;
    std::array<bool, kRewriteFallbackTriggerCount> seenFallbackTriggers{};
    std::array<bool, kTooSmallOtherSubtypeCount> seenTooSmallOtherSubtypes{};
    std::array<bool, kSequenceOneEdgeSubtypeCount> seenTooSmallOneEdgeSubtypes{};
    std::array<bool, kCompactBuildFailSubtypeCount> seenCompactBuildFailSubtypes{};
    std::array<bool, kSelfLoopBuildFailSubtypeCount> seenSelfLoopSubtypes{};
    std::array<bool, kSelfLoopRemainderOtherNBSubtypeCount> seenSelfLoopOtherNBSubtypes{};
    std::array<bool, kXIncidentVirtualSubtypeCount> seenXIncidentVirtualSubtypes{};
    std::array<bool, kXSharedResidualSubtypeCount> seenXSharedResidualSubtypes{};
    std::array<bool, kXSharedLoopSharedInputSubtypeCount> seenXSharedLoopSharedInputSubtypes{};
    std::array<bool, kXSharedBridgeBailoutCount> seenXSharedBridgeBailouts{};
    std::array<bool, kGraftRewireBailoutSubtypeCount> seenGraftRewireSubtypes{};
    std::array<bool, kGraftOtherSubtypeCount> seenGraftOtherSubtypes{};
    std::array<bool, kGraftPostcheckSubtypeCount> seenPostcheckSubtypes{};
    bool sawDeferredSameTypeSP = false;
    bool sawSameTypeSPCleanup = false;
    std::array<bool, kProxyArcNoCandidateSubtypeCount> seenProxyRepairNoCandidateSubtypes{};
    std::array<bool, kProxyArcLifecyclePhaseCount> seenProxyArcLifecycleBadPhases{};
};

struct WeakRepairPipelineContext {
    bool pending = false;
    RepairedProxyArcInfo info;
    CompactGraph compact;
    ReducedSPQRCore beforeCore;
    std::optional<GraftTrace> trace;
    std::string why;
};

struct SequenceReplayCaptureContext {
    bool enabled = false;
    bool hasTrace = false;
    GraftTrace trace;
};

struct SequenceDeferredSameTypeSPContext {
    bool pending = false;
    ReducedSPQRCore actualBeforeRewrite;
    ReducedSPQRCore actualAfterRewrite;
    CompactGraph compact;
    GraftTrace trace;
    std::string why;
    uint64_t seed = 0;
    int tc = -1;
    int stepIndex = -1;
    int sequenceLengthSoFar = 0;
    NodeId chosenR = -1;
    VertexId chosenX = -1;
};

RewriteRStats gRewriteRStats;
RewriteRCaseContext gRewriteRCaseContext;
WeakRepairPipelineContext gWeakRepairPipelineContext;
SequenceReplayCaptureContext gSequenceReplayCaptureContext;
SequenceDeferredSameTypeSPContext gSequenceDeferredSameTypeSPContext;

void dumpCompactBCResult(const CompactBCResult &bc, std::ostream &os);

size_t compactRejectReasonIndex(CompactRejectReason reason) {
    return static_cast<size_t>(reason);
}

size_t notBiconnectedSubtypeIndex(NotBiconnectedSubtype subtype) {
    return static_cast<size_t>(subtype);
}

size_t tooSmallSubtypeIndex(TooSmallSubtype subtype) {
    return static_cast<size_t>(subtype);
}

size_t tooSmallOtherSubtypeIndex(TooSmallOtherSubtype subtype) {
    return static_cast<size_t>(subtype);
}

size_t sequenceOneEdgeSubtypeIndex(SequenceOneEdgeSubtype subtype) {
    return static_cast<size_t>(subtype);
}

size_t seqFallbackReasonIndex(SeqFallbackReason reason) {
    return static_cast<size_t>(reason);
}

size_t rewriteFallbackTriggerIndex(RewriteFallbackTrigger trigger) {
    return static_cast<size_t>(trigger);
}

size_t compactBuildFailSubtypeIndex(CompactBuildFailSubtype subtype) {
    return static_cast<size_t>(subtype);
}

size_t selfLoopBuildFailSubtypeIndex(SelfLoopBuildFailSubtype subtype) {
    return static_cast<size_t>(subtype);
}

size_t selfLoopRemainderOtherNBSubtypeIndex(SelfLoopRemainderOtherNBSubtype subtype) {
    return static_cast<size_t>(subtype);
}

size_t xIncidentVirtualSubtypeIndex(XIncidentVirtualSubtype subtype) {
    return static_cast<size_t>(subtype);
}

size_t xSharedResidualSubtypeIndex(XSharedResidualSubtype subtype) {
    return static_cast<size_t>(subtype);
}

size_t xSharedLoopSharedInputSubtypeIndex(XSharedLoopSharedInputSubtype subtype) {
    return static_cast<size_t>(subtype);
}

size_t xSharedLoopSharedBailoutIndex(XSharedLoopSharedBailout bailout) {
    return static_cast<size_t>(bailout);
}

size_t xSharedBridgeBailoutIndex(XSharedBridgeBailout bailout) {
    return static_cast<size_t>(bailout);
}

size_t graftRewireBailoutSubtypeIndex(GraftRewireBailoutSubtype subtype) {
    return static_cast<size_t>(subtype);
}

size_t graftOtherSubtypeIndex(GraftOtherSubtype subtype) {
    return static_cast<size_t>(subtype);
}

size_t graftPostcheckSubtypeIndex(GraftPostcheckSubtype subtype) {
    return static_cast<size_t>(subtype);
}

size_t proxyArcRepairOutcomeIndex(ProxyArcRepairOutcome outcome) {
    return static_cast<size_t>(outcome);
}

size_t proxyArcNoCandidateSubtypeIndex(ProxyArcNoCandidateSubtype subtype) {
    return static_cast<size_t>(subtype);
}

size_t proxyArcLifecyclePhaseIndex(ProxyArcLifecyclePhase phase) {
    return static_cast<size_t>(phase);
}

size_t weakRepairGateSubtypeIndex(WeakRepairGateSubtype subtype) {
    return static_cast<size_t>(subtype);
}

size_t weakRepairCandidateSubtypeIndex(WeakRepairCandidateSubtype subtype) {
    return static_cast<size_t>(subtype);
}

size_t weakRepairCommitOutcomeIndex(WeakRepairCommitOutcome outcome) {
    return static_cast<size_t>(outcome);
}

size_t rewritePathTakenIndex(RewritePathTaken path) {
    return static_cast<size_t>(path);
}

void setRejectReason(CompactRejectReason *dst, CompactRejectReason reason) {
    if (dst) *dst = reason;
}

void setCompactBuildFailSubtype(CompactBuildFailSubtype *dst,
                                CompactBuildFailSubtype subtype) {
    if (dst) *dst = subtype;
}

bool isRecoverableCompactReject(CompactRejectReason reason) {
    switch (reason) {
        case CompactRejectReason::EMPTY_AFTER_DELETE:
        case CompactRejectReason::TOO_SMALL_FOR_SPQR:
        case CompactRejectReason::NOT_BICONNECTED:
        case CompactRejectReason::X_INCIDENT_VIRTUAL_UNSUPPORTED:
        case CompactRejectReason::SELF_LOOP:
            return true;
        case CompactRejectReason::OWNER_NOT_R:
        case CompactRejectReason::X_NOT_PRESENT_IN_R:
        case CompactRejectReason::OTHER:
        case CompactRejectReason::COUNT:
            return false;
    }
    return false;
}

bool shouldFallbackSequenceProxyGraftFailure(const std::string &why) {
    return why.find("rewireArcEndpoint returned false") != std::string::npos ||
           why.find("PROXY input has no owner mini slot") != std::string::npos ||
           why.find("PROXY actual slot missing") != std::string::npos ||
           why.find("invalid oldArc/outsideNode metadata") != std::string::npos ||
           why.find("dead relay") != std::string::npos ||
           why.find("adjacency mismatch on node") != std::string::npos ||
           why.find("oldNode adjacency") != std::string::npos ||
           why.find("still incident to oldNode") != std::string::npos ||
           why.find("missing from adjacency") != std::string::npos ||
           why.find("sequence rewrite produced adjacent same-type S/P nodes") !=
               std::string::npos;
}

RewriteFallbackTrigger classifyCompactBuildFallbackTrigger(CompactRejectReason rejectReason,
                                                           const std::string &why) {
    if (rejectReason == CompactRejectReason::X_INCIDENT_VIRTUAL_UNSUPPORTED ||
        why.find("virtual slot incident to x unsupported") != std::string::npos) {
        return RewriteFallbackTrigger::RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED;
    }
    if (rejectReason == CompactRejectReason::EMPTY_AFTER_DELETE) {
        return RewriteFallbackTrigger::RFT_COMPACT_EMPTY_AFTER_DELETE;
    }
    return RewriteFallbackTrigger::RFT_COMPACT_BUILD_FAIL;
}

RewriteFallbackTrigger classifyUnhandledPrecheckTrigger(CompactRejectReason rejectReason) {
    switch (rejectReason) {
        case CompactRejectReason::EMPTY_AFTER_DELETE:
            return RewriteFallbackTrigger::RFT_COMPACT_EMPTY_AFTER_DELETE;
        case CompactRejectReason::NOT_BICONNECTED:
            return RewriteFallbackTrigger::RFT_COMPACT_NOT_BICONNECTED_UNHANDLED;
        case CompactRejectReason::TOO_SMALL_FOR_SPQR:
            return RewriteFallbackTrigger::RFT_COMPACT_TOO_SMALL_UNHANDLED;
        case CompactRejectReason::X_INCIDENT_VIRTUAL_UNSUPPORTED:
            return RewriteFallbackTrigger::RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED;
        case CompactRejectReason::OTHER:
            return RewriteFallbackTrigger::RFT_OTHER;
        case CompactRejectReason::SELF_LOOP:
        case CompactRejectReason::OWNER_NOT_R:
        case CompactRejectReason::X_NOT_PRESENT_IN_R:
        case CompactRejectReason::COUNT:
            return RewriteFallbackTrigger::RFT_COMPACT_BUILD_FAIL;
    }
    return RewriteFallbackTrigger::RFT_OTHER;
}

RewriteFallbackTrigger classifyBackendBuildRawTrigger(const std::string &why) {
    if (why.find("S skeleton must have at least 3 edges") != std::string::npos) {
        return RewriteFallbackTrigger::RFT_BACKEND_BUILDRAW_S_LT3;
    }
    if (why.find("not biconnected") != std::string::npos) {
        return RewriteFallbackTrigger::RFT_BACKEND_BUILDRAW_NOT_BICONNECTED;
    }
    return RewriteFallbackTrigger::RFT_BACKEND_BUILDRAW_OTHER;
}

void noteRewritePathTaken(RewritePathTaken path) {
    ++gRewriteRStats.rewritePathTakenCounts[rewritePathTakenIndex(path)];
}

std::filesystem::path rejectDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_rejects";
}

std::filesystem::path tinyRejectDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_tiny_rejects";
}

std::filesystem::path tooSmallOtherDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_tso";
}

std::filesystem::path oneEdgeDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_oneedge";
}

std::filesystem::path seqFallbackDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_fallbacks";
}

std::filesystem::path findExistingRejectDump(CompactRejectReason reason) {
    const auto dir = rejectDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix =
        std::string("reject_") + compactRejectReasonName(reason) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingSeqFallbackDump(SeqFallbackReason reason) {
    const auto dir = seqFallbackDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix =
        std::string("seq_fallback_") + seqFallbackReasonName(reason) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingFallbackTriggerDump(RewriteFallbackTrigger trigger) {
    const auto dir = seqFallbackDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix =
        std::string("seq_fallback_") + rewriteFallbackTriggerName(trigger) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingTooSmallSubtypeDump(TooSmallSubtype subtype) {
    const auto dir = tinyRejectDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix =
        std::string("reject_") + tooSmallSubtypeName(subtype) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingTooSmallOtherDump(TooSmallOtherSubtype subtype) {
    const auto dir = tooSmallOtherDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix =
        std::string("reject_") + tooSmallOtherSubtypeName(subtype) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingTooSmallOneEdgeDump(SequenceOneEdgeSubtype subtype) {
    const auto dir = oneEdgeDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix =
        std::string("seq_tiny_oneedge_") + sequenceOneEdgeSubtypeName(subtype) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingNotBiconnectedSubtypeDump(NotBiconnectedSubtype subtype) {
    const auto dir = rejectDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix =
        std::string("reject_") + notBiconnectedSubtypeName(subtype) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path compactBuildFailDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_buildfail";
}

std::filesystem::path selfLoopDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_selfloop";
}

std::filesystem::path selfLoopSpqrReadyDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_selfloop_spqrready";
}

std::filesystem::path selfLoopOneEdgeDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_selfloop_oneedge";
}

std::filesystem::path selfLoopOtherNBDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_selfloop_othernb";
}

std::filesystem::path xIncidentVirtualDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_xincident";
}

std::filesystem::path xSharedResidualDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_xshared_residual";
}

std::filesystem::path xSharedSpqrReadyDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_xshared_spqrready";
}

std::filesystem::path xSharedResidual2DumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_xshared_residual2";
}

std::filesystem::path graftRewireDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_graftbreakdown";
}

std::filesystem::path graftOtherDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_graftother";
}

std::filesystem::path postcheckSubtypeDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_postcheck";
}

std::filesystem::path deferredSameTypeSPDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_defer_sp";
}

std::filesystem::path sameTypeSPCleanupDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_spcleanup" / "cleanup_samples";
}

std::filesystem::path proxyRepairNoCandidateDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_proxy_nocand";
}

std::filesystem::path oldArcWeakRepairDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_oldarcweak";
}

std::filesystem::path weakRepairPipelineDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_weak_pipeline";
}

std::filesystem::path proxyArcLifecycleDumpDir() {
    return std::filesystem::path("dumps") / "rewrite_r_seq_arcphase";
}

std::filesystem::path findExistingCompactBuildFailDump(CompactBuildFailSubtype subtype) {
    const auto dir = compactBuildFailDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix =
        std::string("seq_fallback_") + compactBuildFailSubtypeName(subtype) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingSelfLoopDump(SelfLoopBuildFailSubtype subtype) {
    const auto dir = selfLoopDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix =
        std::string("seq_fallback_") + selfLoopBuildFailSubtypeName(subtype) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingSelfLoopSpqrReadyDump(const std::string &kind) {
    const auto dir = selfLoopSpqrReadyDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix = std::string("seq_selfloop_spqrready_") + kind + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingSelfLoopOneEdgeDump(const std::string &kind) {
    const auto dir = selfLoopOneEdgeDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix = std::string("seq_selfloop_oneedge_") + kind + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingSelfLoopOtherNBDump(SelfLoopRemainderOtherNBSubtype subtype) {
    const auto dir = selfLoopOtherNBDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix =
        std::string("seq_selfloop_othernb_") +
        selfLoopRemainderOtherNBSubtypeName(subtype) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingXIncidentVirtualDump(XIncidentVirtualSubtype subtype) {
    const auto dir = xIncidentVirtualDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix =
        std::string("seq_fallback_") + xIncidentVirtualSubtypeName(subtype) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingXSharedResidualDump(XSharedResidualSubtype subtype) {
    const auto dir = xSharedResidualDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix =
        std::string("seq_xshared_residual_") + xSharedResidualSubtypeName(subtype) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingXSharedSpqrReadyDump(const std::string &kind) {
    const auto dir = xSharedSpqrReadyDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix = std::string("seq_xshared_spqrready_") + kind + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingXSharedLoopSharedInputDump(
    XSharedLoopSharedInputSubtype subtype) {
    const auto dir = xSharedResidual2DumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix =
        std::string("seq_xshared_loopshared_input_") +
        xSharedLoopSharedInputSubtypeName(subtype) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingXSharedLoopSharedBailoutDump(
    XSharedLoopSharedBailout bailout) {
    const auto dir = xSharedResidual2DumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix =
        std::string("seq_xshared_loopshared_bailout_") +
        xSharedLoopSharedBailoutName(bailout) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingXSharedBridgeBailoutDump(XSharedBridgeBailout bailout) {
    const auto dir = xSharedResidualDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix =
        std::string("seq_xshared_bailout_") + xSharedBridgeBailoutName(bailout) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingGraftRewireDump(GraftRewireBailoutSubtype subtype) {
    const auto dir = graftRewireDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix =
        std::string("seq_fallback_") + graftRewireBailoutSubtypeName(subtype) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingGraftOtherDump(GraftOtherSubtype subtype) {
    const auto dir = graftOtherDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix =
        std::string("seq_graft_other_") + graftOtherSubtypeName(subtype) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingPostcheckSubtypeDump(GraftPostcheckSubtype subtype) {
    const auto dir = postcheckSubtypeDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix =
        std::string("seq_postcheck_") + graftPostcheckSubtypeName(subtype) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingDeferredSameTypeSPDump() {
    const auto dir = deferredSameTypeSPDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix = "seq_defer_same_type_sp_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingSameTypeSPCleanupDump() {
    const auto dir = sameTypeSPCleanupDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix = "seq_sp_cleanup_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingProxyRepairNoCandidateDump(
    ProxyArcNoCandidateSubtype subtype) {
    const auto dir = proxyRepairNoCandidateDumpDir();
    if (!std::filesystem::exists(dir)) return {};

    const std::string prefix =
        std::string("seq_proxy_nocand_") + proxyArcNoCandidateSubtypeName(subtype) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingOldArcWeakRepairSuccessDump() {
    const auto dir = oldArcWeakRepairDumpDir();
    if (!std::filesystem::exists(dir)) return {};
    const std::string prefix = "seq_proxy_weakrepair_success_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingOldArcWeakRepairFailDump(ProxyArcRepairOutcome outcome) {
    const auto dir = oldArcWeakRepairDumpDir();
    if (!std::filesystem::exists(dir)) return {};
    const std::string prefix =
        std::string("seq_proxy_weakrepair_fail_") +
        proxyArcRepairOutcomeName(outcome) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingWeakRepairGateDump(WeakRepairGateSubtype subtype) {
    const auto dir = weakRepairPipelineDumpDir();
    if (!std::filesystem::exists(dir)) return {};
    const std::string prefix =
        std::string("seq_weak_gate_") + weakRepairGateSubtypeName(subtype) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingWeakRepairCandidateDump(
    WeakRepairCandidateSubtype subtype) {
    const auto dir = weakRepairPipelineDumpDir();
    if (!std::filesystem::exists(dir)) return {};
    const std::string prefix =
        std::string("seq_weak_candidate_") + weakRepairCandidateSubtypeName(subtype) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingWeakRepairCommitDump(WeakRepairCommitOutcome outcome) {
    const auto dir = weakRepairPipelineDumpDir();
    if (!std::filesystem::exists(dir)) return {};
    const std::string prefix =
        std::string("seq_weak_commit_") + weakRepairCommitOutcomeName(outcome) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

std::filesystem::path findExistingProxyArcLifecycleDump(ProxyArcLifecyclePhase phase) {
    const auto dir = proxyArcLifecycleDumpDir();
    if (!std::filesystem::exists(dir)) return {};
    const std::string prefix =
        std::string("seq_proxy_phase_") + proxyArcLifecyclePhaseName(phase) + "_";
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0) return entry.path();
    }
    return {};
}

void dumpActualNodeLocalSlotSummary(const ReducedSPQRCore &core,
                                    NodeId nodeId,
                                    std::ostream &ofs) {
    ofs << "=== RNodeLocalSlots ===\n";
    ofs << "nodeId=" << nodeId << "\n";
    if (!validActualNodeId(core, nodeId)) {
        ofs << "invalid-node\n";
        return;
    }
    const auto &node = core.nodes[nodeId];
    ofs << "alive=" << (node.alive ? 1 : 0)
        << " type=" << static_cast<int>(node.type) << "\n";
    for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
        const auto &slot = node.slots[slotId];
        ofs << "slot " << slotId
            << " alive=" << (slot.alive ? 1 : 0)
            << " kind=" << (slot.isVirtual ? "VIRTUAL" : "REAL")
            << " poles=(" << slot.poleA << "," << slot.poleB << ")";
        if (slot.isVirtual) {
            ofs << " arcId=" << slot.arcId;
            if (validActualArcId(core, slot.arcId)) {
                const auto &arc = core.arcs[slot.arcId];
                const NodeId outsideNode = otherEndpointOfArc(arc, nodeId);
                ofs << " oldArc=" << slot.arcId
                    << " outsideNode=" << outsideNode;
            }
        } else {
            ofs << " realEdge=" << slot.realEdge;
        }
        ofs << "\n";
    }
}

void noteSequenceCompactBuildFailSubtype(CompactBuildFailSubtype subtype,
                                         const ReducedSPQRCore &core,
                                         NodeId rNode,
                                         VertexId x,
                                         const CompactGraph *compact,
                                         const std::string &why) {
    ++gRewriteRStats
          .seqCompactBuildFailSubtypeCounts[compactBuildFailSubtypeIndex(subtype)];

    auto &seenSubtype =
        gRewriteRCaseContext
            .seenCompactBuildFailSubtypes[compactBuildFailSubtypeIndex(subtype)];
    if (!seenSubtype) {
        seenSubtype = true;
        ++gRewriteRStats
              .seqCompactBuildFailCaseCountsBySubtype[compactBuildFailSubtypeIndex(subtype)];
    }

    if (gRewriteRCaseContext.stepIndex >= 0 &&
        gRewriteRCaseContext.stepIndex < static_cast<int>(kRewriteSeqTrackedSteps)) {
        ++gRewriteRStats
              .seqCompactBuildFailAtStepCounts[gRewriteRCaseContext.stepIndex]
                                              [compactBuildFailSubtypeIndex(subtype)];
    }

    auto &storedPath =
        gRewriteRStats
            .firstCompactBuildFailDumpPaths[compactBuildFailSubtypeIndex(subtype)];
    if (storedPath.empty()) {
        const auto existing = findExistingCompactBuildFailDump(subtype);
        if (!existing.empty()) {
            storedPath = existing.string();
            return;
        }
    } else {
        return;
    }

    const auto dir = compactBuildFailDumpDir();
    std::filesystem::create_directories(dir);

    std::ostringstream filename;
    filename << "seq_fallback_" << compactBuildFailSubtypeName(subtype)
             << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc;
    if (gRewriteRCaseContext.stepIndex >= 0) {
        filename << "_step" << gRewriteRCaseContext.stepIndex;
    }
    filename << ".txt";
    const auto path = dir / filename.str();

    std::ofstream ofs(path);
    ofs << "trigger=" << rewriteFallbackTriggerName(RewriteFallbackTrigger::RFT_COMPACT_BUILD_FAIL)
        << "\n";
    ofs << "buildFailSubtype=" << compactBuildFailSubtypeName(subtype) << "\n";
    ofs << "why=" << why << "\n";
    ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
    ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
    ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
    ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar << "\n";
    ofs << "chosenR=" << rNode << "\n";
    ofs << "chosenX=" << x << "\n\n";
    if (compact && (!compact->origOfCv.empty() || !compact->edges.empty())) {
        dumpCompactGraph(*compact, ofs);
        ofs << "\n";
    }
    dumpActualNodeLocalSlotSummary(core, rNode, ofs);
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);

    storedPath = path.string();
}

void noteSequenceSelfLoopSubtype(SelfLoopBuildFailSubtype subtype,
                                 const ReducedSPQRCore &core,
                                 NodeId rNode,
                                 VertexId x,
                                 const CompactGraph &compact,
                                 const std::string &why) {
    ++gRewriteRStats.seqSelfLoopSubtypeCounts[selfLoopBuildFailSubtypeIndex(subtype)];

    auto &seenSubtype =
        gRewriteRCaseContext.seenSelfLoopSubtypes[selfLoopBuildFailSubtypeIndex(subtype)];
    if (!seenSubtype) {
        seenSubtype = true;
        ++gRewriteRStats
              .seqSelfLoopCaseCountsBySubtype[selfLoopBuildFailSubtypeIndex(subtype)];
    }

    if (gRewriteRCaseContext.stepIndex >= 0 &&
        gRewriteRCaseContext.stepIndex < static_cast<int>(kRewriteSeqTrackedSteps)) {
        ++gRewriteRStats
              .seqSelfLoopAtStepCounts[gRewriteRCaseContext.stepIndex]
                                      [selfLoopBuildFailSubtypeIndex(subtype)];
    }

    auto &storedPath =
        gRewriteRStats.firstSelfLoopDumpPaths[selfLoopBuildFailSubtypeIndex(subtype)];
    if (storedPath.empty()) {
        const auto existing = findExistingSelfLoopDump(subtype);
        if (!existing.empty()) {
            storedPath = existing.string();
            return;
        }
    } else {
        return;
    }

    CompactGraph remainder;
    std::vector<int> removedLoopEdgeIds;
    std::string stripWhy;
    std::string remainderSummary;
    if (stripSelfLoopsForAnalysis(compact, remainder, removedLoopEdgeIds, stripWhy)) {
        remainderSummary = "ok";
        if (remainder.edges.empty()) {
            remainderSummary += " empty";
        } else {
            CompactRejectReason rejectReason = CompactRejectReason::OTHER;
            std::string readyWhy;
            if (isCompactGraphSpqrReady(remainder, &rejectReason, readyWhy)) {
                remainderSummary += " spqr-ready";
            } else if (classifyTooSmallCompact(remainder) == TooSmallSubtype::TS_TWO_PATH) {
                remainderSummary += " two-path";
            } else {
                CompactBCResult bc;
                std::string bcWhy;
                if (decomposeCompactIntoBC(remainder, bc, bcWhy)) {
                    const auto nbSubtype = classifyNotBiconnected(remainder, bc);
                    remainderSummary += " ";
                    remainderSummary += notBiconnectedSubtypeName(nbSubtype);
                } else {
                    remainderSummary += " ";
                    remainderSummary += readyWhy.empty() ? bcWhy : readyWhy;
                }
            }
        }
    } else {
        remainderSummary = stripWhy;
    }

    const auto dir = selfLoopDumpDir();
    std::filesystem::create_directories(dir);

    std::ostringstream filename;
    filename << "seq_fallback_" << selfLoopBuildFailSubtypeName(subtype)
             << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc;
    if (gRewriteRCaseContext.stepIndex >= 0) {
        filename << "_step" << gRewriteRCaseContext.stepIndex;
    }
    filename << ".txt";
    const auto path = dir / filename.str();

    std::ofstream ofs(path);
    ofs << "trigger="
        << rewriteFallbackTriggerName(RewriteFallbackTrigger::RFT_COMPACT_BUILD_FAIL)
        << "\n";
    ofs << "buildFailSubtype="
        << compactBuildFailSubtypeName(CompactBuildFailSubtype::CBF_SELF_LOOP_PRECHECK)
        << "\n";
    ofs << "selfLoopSubtype=" << selfLoopBuildFailSubtypeName(subtype) << "\n";
    ofs << "why=" << why << "\n";
    ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
    ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
    ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
    ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar << "\n";
    ofs << "chosenR=" << rNode << "\n";
    ofs << "chosenX=" << x << "\n\n";
    dumpCompactGraph(compact, ofs);
    ofs << "\n=== SelfLoops ===\n";
    for (const auto &edge : compact.edges) {
        if (edge.a != edge.b) continue;
        ofs << "inputEdgeId=" << edge.id
            << " kind=" << (edge.kind == CompactEdgeKind::REAL ? "REAL" : "PROXY");
        if (edge.a >= 0 && edge.a < static_cast<int>(compact.origOfCv.size())) {
            const VertexId pole = compact.origOfCv[edge.a];
            ofs << " poles=(" << pole << "," << pole << ")";
        } else {
            ofs << " poles=(invalid,invalid)";
        }
        ofs << "\n";
    }
    ofs << "\n=== RemainderSummary ===\n";
    ofs << "removedLoopEdgeIds:";
    for (int edgeId : removedLoopEdgeIds) ofs << ' ' << edgeId;
    ofs << "\n";
    ofs << "remainderEdgeCount=" << remainder.edges.size() << "\n";
    ofs << "remainderVertexCount=" << remainder.origOfCv.size() << "\n";
    ofs << "remainderClassify=" << remainderSummary << "\n";
    if (!remainder.origOfCv.empty() || !remainder.edges.empty()) {
        ofs << "\n";
        dumpCompactGraph(remainder, ofs);
    }
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);

    storedPath = path.string();
}

void noteSequenceSelfLoopOtherNBSubtype(SelfLoopRemainderOtherNBSubtype subtype,
                                        const ReducedSPQRCore &core,
                                        NodeId rNode,
                                        VertexId x,
                                        const CompactGraph &compact,
                                        const std::string &why) {
    ++gRewriteRStats.seqSelfLoopOtherNBSubtypeCounts[selfLoopRemainderOtherNBSubtypeIndex(subtype)];

    auto &seenSubtype = gRewriteRCaseContext
                            .seenSelfLoopOtherNBSubtypes[selfLoopRemainderOtherNBSubtypeIndex(
                                subtype)];
    if (!seenSubtype) {
        seenSubtype = true;
        ++gRewriteRStats
              .seqSelfLoopOtherNBCaseCountsBySubtype[selfLoopRemainderOtherNBSubtypeIndex(
                  subtype)];
    }

    if (gRewriteRCaseContext.stepIndex >= 0 &&
        gRewriteRCaseContext.stepIndex < static_cast<int>(kRewriteSeqTrackedSteps)) {
        ++gRewriteRStats
              .seqSelfLoopOtherNBAtStepCounts[gRewriteRCaseContext.stepIndex]
                                             [selfLoopRemainderOtherNBSubtypeIndex(subtype)];
    }

    if (gRewriteRStats.firstSelfLoopOtherNBDumped[selfLoopRemainderOtherNBSubtypeIndex(subtype)]) {
        return;
    }
    auto &storedPath =
        gRewriteRStats.firstSelfLoopOtherNBDumpPaths[selfLoopRemainderOtherNBSubtypeIndex(
            subtype)];
    if (storedPath.empty()) {
        const auto existing = findExistingSelfLoopOtherNBDump(subtype);
        if (!existing.empty()) {
            gRewriteRStats
                .firstSelfLoopOtherNBDumped[selfLoopRemainderOtherNBSubtypeIndex(subtype)] = true;
            storedPath = existing.string();
            return;
        }
    } else {
        gRewriteRStats
            .firstSelfLoopOtherNBDumped[selfLoopRemainderOtherNBSubtypeIndex(subtype)] = true;
        return;
    }

    CompactBCResult bc;
    std::string bcWhy;
    std::string bcValidateWhy;
    std::string detailWhy;
    const auto detailedSubtype =
        classifySelfLoopOtherNotBiconnectedDetailed(compact, detailWhy);
    CompactGraph remainder;
    std::vector<int> removedLoopEdgeIds;
    std::string strippedWhy;
    const bool haveStripped =
        stripSelfLoopsForAnalysis(compact, remainder, removedLoopEdgeIds, strippedWhy);
    const bool haveBC =
        haveStripped && !remainder.edges.empty() && decomposeCompactIntoBC(remainder, bc, bcWhy) &&
        validateCompactBC(remainder, bc, bcValidateWhy);

    const auto dir = selfLoopOtherNBDumpDir();
    std::filesystem::create_directories(dir);

    std::ostringstream filename;
    filename << "seq_selfloop_othernb_" << selfLoopRemainderOtherNBSubtypeName(subtype)
             << "_seed" << gRewriteRCaseContext.seed << "_tc" << gRewriteRCaseContext.tc;
    if (gRewriteRCaseContext.stepIndex >= 0) {
        filename << "_step" << gRewriteRCaseContext.stepIndex;
    }
    filename << ".txt";
    const auto path = dir / filename.str();

    std::ofstream ofs(path);
    ofs << "trigger="
        << rewriteFallbackTriggerName(RewriteFallbackTrigger::RFT_COMPACT_BUILD_FAIL) << "\n";
    ofs << "buildFailSubtype="
        << compactBuildFailSubtypeName(CompactBuildFailSubtype::CBF_SELF_LOOP_PRECHECK) << "\n";
    ofs << "selfLoopSubtype="
        << selfLoopBuildFailSubtypeName(
               SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_OTHER_NOT_BICONNECTED)
        << "\n";
    ofs << "otherNBSubtype=" << selfLoopRemainderOtherNBSubtypeName(subtype) << "\n";
    ofs << "detailedSubtype="
        << selfLoopRemainderOtherNBSubtypeName(detailedSubtype) << "\n";
    ofs << "why=" << why << "\n";
    ofs << "whyDetail=" << detailWhy << "\n";
    ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
    ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
    ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
    ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar << "\n";
    ofs << "chosenR=" << rNode << "\n";
    ofs << "chosenX=" << x << "\n";
    ofs << "haveStripped=" << (haveStripped ? 1 : 0) << "\n";
    ofs << "strippedWhy=" << strippedWhy << "\n";
    ofs << "haveBC=" << (haveBC ? 1 : 0) << "\n";
    ofs << "bcWhy=" << bcWhy << "\n";
    ofs << "bcValidateWhy=" << bcValidateWhy << "\n";
    if (haveStripped) {
        ofs << "strippedLoopFullInputIds:";
        for (int inputId : removedLoopEdgeIds) ofs << ' ' << inputId;
        ofs << "\n";
    }
    ofs << "\n=== CompactFull ===\n";
    dumpCompactGraph(compact, ofs);
    if (haveStripped) {
        ofs << "\n=== CompactRemainder ===\n";
        dumpCompactGraph(remainder, ofs);
    }
    if (haveBC) {
        ofs << "\n";
        dumpCompactBCResult(bc, ofs);
    }
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);

    gRewriteRStats.firstSelfLoopOtherNBDumped[selfLoopRemainderOtherNBSubtypeIndex(subtype)] =
        true;
    storedPath = path.string();
}

void noteSequenceSelfLoopSpqrReadySample(const std::string &kind,
                                         const ReducedSPQRCore &core,
                                         NodeId rNode,
                                         VertexId x,
                                         const CompactGraph &compact,
                                         int keep,
                                         const GraftTrace *trace,
                                         const std::string &why) {
    const auto existing = findExistingSelfLoopSpqrReadyDump(kind);
    if (!existing.empty()) {
        return;
    }

    const auto dir = selfLoopSpqrReadyDumpDir();
    std::filesystem::create_directories(dir);

    std::ostringstream filename;
    filename << "seq_selfloop_spqrready_" << kind
             << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc;
    if (gRewriteRCaseContext.stepIndex >= 0) {
        filename << "_step" << gRewriteRCaseContext.stepIndex;
    }
    filename << ".txt";
    const auto path = dir / filename.str();

    StrippedSelfLoopRemainder stripped;
    std::string strippedWhy;
    const bool haveStripped =
        buildStrippedSelfLoopRemainder(compact, stripped, strippedWhy);

    std::ofstream ofs(path);
    ofs << "trigger="
        << rewriteFallbackTriggerName(RewriteFallbackTrigger::RFT_COMPACT_BUILD_FAIL)
        << "\n";
    ofs << "buildFailSubtype="
        << compactBuildFailSubtypeName(CompactBuildFailSubtype::CBF_SELF_LOOP_PRECHECK)
        << "\n";
    ofs << "selfLoopSubtype="
        << selfLoopBuildFailSubtypeName(SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_SPQR_READY)
        << "\n";
    ofs << "sampleKind=" << kind << "\n";
    ofs << "why=" << why << "\n";
    ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
    ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
    ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
    ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar << "\n";
    ofs << "chosenR=" << rNode << "\n";
    ofs << "chosenX=" << x << "\n";
    ofs << "keep=" << keep << "\n";
    ofs << "\n=== CompactFull ===\n";
    dumpCompactGraph(compact, ofs);
    if (haveStripped) {
        ofs << "\n=== StrippedLoopInfo ===\n";
        ofs << "loopVertex=" << stripped.loopVertex << "\n";
        ofs << "strippedLoopFullInputIds:";
        for (int inputId : stripped.strippedLoopFullInputIds) ofs << ' ' << inputId;
        ofs << "\n";
        ofs << "remInputIdToFullInputId:";
        for (size_t i = 0; i < stripped.remInputIdToFullInputId.size(); ++i) {
            ofs << ' ' << i << "->" << stripped.remInputIdToFullInputId[i];
        }
        ofs << "\n";
        ofs << "\n=== CompactRemainder ===\n";
        dumpCompactGraph(stripped.Hrem, ofs);
    } else {
        ofs << "\n=== StrippedLoopInfo ===\n";
        ofs << "buildStrippedSelfLoopRemainder failed: " << strippedWhy << "\n";
    }
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);
    if (trace) {
        ofs << "\n=== GraftTrace ===\n";
        dumpGraftTrace(*trace, ofs);
    }

    const std::string stored = path.string();
    if (kind == "attempt") {
        if (gRewriteRStats.firstSelfLoopRemainderSpqrReadyAttemptDumpPath.empty()) {
            gRewriteRStats.firstSelfLoopRemainderSpqrReadyAttemptDumpPath = stored;
        }
    } else if (kind == "success") {
        if (gRewriteRStats.firstSelfLoopRemainderSpqrReadySuccessDumpPath.empty()) {
            gRewriteRStats.firstSelfLoopRemainderSpqrReadySuccessDumpPath = stored;
        }
    } else if (kind == "fallback") {
        if (gRewriteRStats.firstSelfLoopRemainderSpqrReadyFallbackDumpPath.empty()) {
            gRewriteRStats.firstSelfLoopRemainderSpqrReadyFallbackDumpPath = stored;
        }
    }
}

void noteSequenceSelfLoopOneEdgeSample(const std::string &kind,
                                       const ReducedSPQRCore &core,
                                       NodeId rNode,
                                       VertexId x,
                                       const CompactGraph &compact,
                                       int keep,
                                       const GraftTrace *trace,
                                       const std::string &why) {
    const auto existing = findExistingSelfLoopOneEdgeDump(kind);
    if (!existing.empty()) {
        return;
    }

    const auto dir = selfLoopOneEdgeDumpDir();
    std::filesystem::create_directories(dir);

    std::ostringstream filename;
    filename << "seq_selfloop_oneedge_" << kind
             << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc;
    if (gRewriteRCaseContext.stepIndex >= 0) {
        filename << "_step" << gRewriteRCaseContext.stepIndex;
    }
    filename << ".txt";
    const auto path = dir / filename.str();

    StrippedSelfLoopRemainder stripped;
    std::string strippedWhy;
    const bool haveStripped =
        buildStrippedSelfLoopRemainder(compact, stripped, strippedWhy);
    const auto oneEdgeSubtype = haveStripped ? classifySequenceOneEdgeSubtype(stripped.Hrem)
                                             : SequenceOneEdgeSubtype::SOE_OTHER;

    std::ofstream ofs(path);
    ofs << "trigger="
        << rewriteFallbackTriggerName(RewriteFallbackTrigger::RFT_COMPACT_BUILD_FAIL)
        << "\n";
    ofs << "buildFailSubtype="
        << compactBuildFailSubtypeName(CompactBuildFailSubtype::CBF_SELF_LOOP_PRECHECK)
        << "\n";
    ofs << "selfLoopSubtype="
        << selfLoopBuildFailSubtypeName(
               SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_OTHER_NOT_BICONNECTED)
        << "\n";
    ofs << "sampleKind=" << kind << "\n";
    ofs << "oneEdgeSubtype=" << sequenceOneEdgeSubtypeName(oneEdgeSubtype) << "\n";
    ofs << "why=" << why << "\n";
    ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
    ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
    ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
    ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar << "\n";
    ofs << "chosenR=" << rNode << "\n";
    ofs << "chosenX=" << x << "\n";
    ofs << "keep=" << keep << "\n";
    if (haveStripped) {
        ofs << "loopVertex=" << stripped.loopVertex << "\n";
        ofs << "strippedLoopFullInputIds:";
        for (int inputId : stripped.strippedLoopFullInputIds) ofs << ' ' << inputId;
        ofs << "\n";
    } else {
        ofs << "strippedWhy=" << strippedWhy << "\n";
    }
    ofs << "\n=== CompactFull ===\n";
    dumpCompactGraph(compact, ofs);
    if (haveStripped) {
        ofs << "\n=== CompactRemainder ===\n";
        dumpCompactGraph(stripped.Hrem, ofs);
    }
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);
    if (trace) {
        ofs << "\n=== GraftTrace ===\n";
        dumpGraftTrace(*trace, ofs);
    }

    const std::string stored = path.string();
    if (kind == "attempt") {
        if (gRewriteRStats.firstSelfLoopRemainderOneEdgeAttemptDumpPath.empty()) {
            gRewriteRStats.firstSelfLoopRemainderOneEdgeAttemptDumpPath = stored;
        }
        gRewriteRStats.firstSelfLoopRemainderOneEdgeAttemptDumped = true;
    } else if (kind == "success") {
        if (gRewriteRStats.firstSelfLoopRemainderOneEdgeSuccessDumpPath.empty()) {
            gRewriteRStats.firstSelfLoopRemainderOneEdgeSuccessDumpPath = stored;
        }
        gRewriteRStats.firstSelfLoopRemainderOneEdgeSuccessDumped = true;
    } else if (kind == "fallback") {
        if (gRewriteRStats.firstSelfLoopRemainderOneEdgeFallbackDumpPath.empty()) {
            gRewriteRStats.firstSelfLoopRemainderOneEdgeFallbackDumpPath = stored;
        }
        gRewriteRStats.firstSelfLoopRemainderOneEdgeFallbackDumped = true;
    }
}

void noteSequenceXIncidentVirtualSubtype(XIncidentVirtualSubtype subtype,
                                         const ReducedSPQRCore &core,
                                         NodeId rNode,
                                         VertexId x,
                                         const std::string &why) {
    ++gRewriteRStats
          .seqXIncidentVirtualSubtypeCounts[xIncidentVirtualSubtypeIndex(subtype)];

    auto &seenSubtype =
        gRewriteRCaseContext
            .seenXIncidentVirtualSubtypes[xIncidentVirtualSubtypeIndex(subtype)];
    if (!seenSubtype) {
        seenSubtype = true;
        ++gRewriteRStats
              .seqXIncidentVirtualCaseCountsBySubtype[xIncidentVirtualSubtypeIndex(subtype)];
    }

    if (gRewriteRCaseContext.stepIndex >= 0 &&
        gRewriteRCaseContext.stepIndex < static_cast<int>(kRewriteSeqTrackedSteps)) {
        ++gRewriteRStats
              .seqXIncidentVirtualAtStepCounts[gRewriteRCaseContext.stepIndex]
                                              [xIncidentVirtualSubtypeIndex(subtype)];
    }

    auto &storedPath =
        gRewriteRStats.firstXIncidentVirtualDumpPaths[xIncidentVirtualSubtypeIndex(subtype)];
    if (storedPath.empty()) {
        const auto existing = findExistingXIncidentVirtualDump(subtype);
        if (!existing.empty()) {
            storedPath = existing.string();
            return;
        }
    } else {
        return;
    }

    const auto dir = xIncidentVirtualDumpDir();
    std::filesystem::create_directories(dir);

    std::ostringstream filename;
    filename << "seq_fallback_" << xIncidentVirtualSubtypeName(subtype)
             << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc;
    if (gRewriteRCaseContext.stepIndex >= 0) {
        filename << "_step" << gRewriteRCaseContext.stepIndex;
    }
    filename << ".txt";
    const auto path = dir / filename.str();

    std::ofstream ofs(path);
    ofs << "trigger="
        << rewriteFallbackTriggerName(
               RewriteFallbackTrigger::RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED)
        << "\n";
    ofs << "xIncidentVirtualSubtype=" << xIncidentVirtualSubtypeName(subtype) << "\n";
    ofs << "pathTaken=" << rewritePathTakenName(RewritePathTaken::WHOLE_CORE_REBUILD)
        << "\n";
    ofs << "why=" << why << "\n";
    ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
    ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
    ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
    ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar << "\n";
    ofs << "chosenR=" << rNode << "\n";
    ofs << "chosenX=" << x << "\n\n";
    ofs << "=== XIncidentVirtualSlots ===\n";
    if (!validActualNodeId(core, rNode)) {
        ofs << "invalid-node\n";
    } else {
        const auto &node = core.nodes[rNode];
        for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
            const auto &slot = node.slots[slotId];
            if (!slot.alive || !slot.isVirtual) continue;
            if (slot.poleA != x && slot.poleB != x) continue;
            ofs << "slot " << slotId
                << " poles=(" << slot.poleA << "," << slot.poleB << ")"
                << " loop=" << (slot.poleA == slot.poleB ? 1 : 0)
                << " arcId=" << slot.arcId;
            if (!validActualArcId(core, slot.arcId) || !core.arcs[slot.arcId].alive) {
                ofs << " metadata=invalid-arc\n";
                continue;
            }
            const auto &arc = core.arcs[slot.arcId];
            const NodeId outsideNode = otherEndpointOfArc(arc, rNode);
            ofs << " outsideNode=" << outsideNode;
            if (!validActualNodeId(core, outsideNode) || !core.nodes[outsideNode].alive) {
                ofs << " metadata=invalid-outside\n";
                continue;
            }
            const Agg sideAgg = computeAggToward(core, rNode, outsideNode);
            ofs << " sideAgg.edgeCnt=" << sideAgg.edgeCnt
                << " sideAgg.vertexCnt=" << sideAgg.vertexCnt << "\n";
        }
    }
    ofs << "\n";
    dumpActualNodeLocalSlotSummary(core, rNode, ofs);
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);

    storedPath = path.string();
}

void noteSequenceXSharedResidualSubtype(XSharedResidualSubtype subtype,
                                        const ReducedSPQRCore &core,
                                        NodeId rNode,
                                        VertexId x,
                                        const CompactGraph *compact,
                                        const std::string &why) {
    ++gRewriteRStats
          .seqXIncidentResidualSubtypeCounts[xSharedResidualSubtypeIndex(subtype)];

    auto &seenSubtype =
        gRewriteRCaseContext
            .seenXSharedResidualSubtypes[xSharedResidualSubtypeIndex(subtype)];
    if (!seenSubtype) {
        seenSubtype = true;
        ++gRewriteRStats
              .seqXIncidentResidualCaseCountsBySubtype[xSharedResidualSubtypeIndex(subtype)];
    }

    auto &storedPath =
        gRewriteRStats
            .firstXIncidentResidualDumpPaths[xSharedResidualSubtypeIndex(subtype)];
    if (storedPath.empty()) {
        const auto existing = findExistingXSharedResidualDump(subtype);
        if (!existing.empty()) {
            storedPath = existing.string();
            return;
        }
    } else {
        return;
    }

    const auto dir = xSharedResidualDumpDir();
    std::filesystem::create_directories(dir);

    std::ostringstream filename;
    filename << "seq_xshared_residual_" << xSharedResidualSubtypeName(subtype)
             << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc;
    if (gRewriteRCaseContext.stepIndex >= 0) {
        filename << "_step" << gRewriteRCaseContext.stepIndex;
    }
    filename << ".txt";
    const auto path = dir / filename.str();

    std::ofstream ofs(path);
    ofs << "trigger="
        << rewriteFallbackTriggerName(
               RewriteFallbackTrigger::RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED)
        << "\n";
    ofs << "xIncidentSubtype="
        << xIncidentVirtualSubtypeName(XIncidentVirtualSubtype::XIV_SHARED_WITH_LOOP)
        << "\n";
    ofs << "residualSubtype=" << xSharedResidualSubtypeName(subtype) << "\n";
    ofs << "why=" << why << "\n";
    ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
    ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
    ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
    ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar << "\n";
    ofs << "chosenR=" << rNode << "\n";
    ofs << "chosenX=" << x << "\n\n";
    if (compact && (!compact->origOfCv.empty() || !compact->edges.empty())) {
        dumpCompactGraph(*compact, ofs);
        ofs << "\n";
    }
    dumpActualNodeLocalSlotSummary(core, rNode, ofs);
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);

    storedPath = path.string();
}

void noteSequenceXSharedSpqrReadySample(const std::string &kind,
                                        const ReducedSPQRCore &core,
                                        NodeId rNode,
                                        VertexId x,
                                        const CompactGraph &compact,
                                        int keep,
                                        const GraftTrace *trace,
                                        const std::string &why) {
    const auto existing = findExistingXSharedSpqrReadyDump(kind);
    if (!existing.empty()) {
        return;
    }

    const auto dir = xSharedSpqrReadyDumpDir();
    std::filesystem::create_directories(dir);

    std::ostringstream filename;
    filename << "seq_xshared_spqrready_" << kind
             << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc;
    if (gRewriteRCaseContext.stepIndex >= 0) {
        filename << "_step" << gRewriteRCaseContext.stepIndex;
    }
    filename << ".txt";
    const auto path = dir / filename.str();

    std::ofstream ofs(path);
    ofs << "trigger="
        << rewriteFallbackTriggerName(
               RewriteFallbackTrigger::RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED)
        << "\n";
    ofs << "xIncidentSubtype="
        << xIncidentVirtualSubtypeName(XIncidentVirtualSubtype::XIV_SHARED_WITH_LOOP)
        << "\n";
    ofs << "residualSubtype="
        << xSharedResidualSubtypeName(XSharedResidualSubtype::XSR_HAFTER_SPQR_READY)
        << "\n";
    ofs << "sampleKind=" << kind << "\n";
    ofs << "keep=" << keep << "\n";
    ofs << "why=" << why << "\n";
    ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
    ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
    ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
    ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar << "\n";
    ofs << "chosenR=" << rNode << "\n";
    ofs << "chosenX=" << x << "\n\n";
    dumpCompactGraph(compact, ofs);
    ofs << "\n";
    dumpActualNodeLocalSlotSummary(core, rNode, ofs);
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);
    ofs << "\n";
    if (trace) {
        dumpGraftTrace(*trace, ofs);
    }
}

std::pair<VertexId, VertexId> compactEdgePoles(const CompactGraph &compact,
                                               const CompactEdge &edge) {
    if (edge.a < 0 || edge.a >= static_cast<int>(compact.origOfCv.size()) ||
        edge.b < 0 || edge.b >= static_cast<int>(compact.origOfCv.size())) {
        return {-1, -1};
    }
    return {compact.origOfCv[edge.a], compact.origOfCv[edge.b]};
}

VertexId findXSharedLoopSharedCutVertex(const CompactGraph &compact) {
    const CompactEdge *loopEdge = nullptr;
    const CompactEdge *nonLoopEdge = nullptr;
    for (const auto &edge : compact.edges) {
        const auto poles = compactEdgePoles(compact, edge);
        if (poles.first < 0 || poles.second < 0) continue;
        if (poles.first == poles.second) {
            if (loopEdge != nullptr) return -1;
            loopEdge = &edge;
        } else {
            if (nonLoopEdge != nullptr) return -1;
            nonLoopEdge = &edge;
        }
    }
    if (loopEdge == nullptr || nonLoopEdge == nullptr) return -1;

    const auto loopPoles = compactEdgePoles(compact, *loopEdge);
    const auto edgePoles = compactEdgePoles(compact, *nonLoopEdge);
    if (loopPoles.first == edgePoles.first || loopPoles.first == edgePoles.second) {
        return loopPoles.first;
    }
    return -1;
}

void dumpXSharedLoopSharedEdges(const CompactGraph &compact, std::ostream &os) {
    os << "=== HafterEdges ===\n";
    for (const auto &edge : compact.edges) {
        const auto poles = compactEdgePoles(compact, edge);
        os << "inputEdgeId=" << edge.id
           << " kind=" << (edge.kind == CompactEdgeKind::REAL ? "REAL" : "PROXY")
           << " poles=(" << poles.first << "," << poles.second << ")"
           << " loop=" << (poles.first >= 0 && poles.first == poles.second ? 1 : 0)
           << "\n";
    }
}

void noteSequenceXSharedLoopSharedInputSubtype(XSharedLoopSharedInputSubtype subtype,
                                               const ReducedSPQRCore &core,
                                               NodeId rNode,
                                               VertexId x,
                                               const CompactGraph &compact,
                                               const GraftTrace *trace,
                                               const std::string &why) {
    ++gRewriteRStats
          .seqXSharedLoopSharedInputSubtypeCounts[xSharedLoopSharedInputSubtypeIndex(subtype)];

    auto &seenSubtype =
        gRewriteRCaseContext
            .seenXSharedLoopSharedInputSubtypes[xSharedLoopSharedInputSubtypeIndex(subtype)];
    if (!seenSubtype) {
        seenSubtype = true;
        ++gRewriteRStats
              .seqXSharedLoopSharedCaseCountsByInputSubtype
                  [xSharedLoopSharedInputSubtypeIndex(subtype)];
    }

    auto &dumped =
        gRewriteRStats
            .firstXSharedLoopSharedInputDumped[xSharedLoopSharedInputSubtypeIndex(subtype)];
    auto &storedPath =
        gRewriteRStats
            .firstXSharedLoopSharedInputDumpPaths[xSharedLoopSharedInputSubtypeIndex(subtype)];
    if (dumped) return;
    if (storedPath.empty()) {
        const auto existing = findExistingXSharedLoopSharedInputDump(subtype);
        if (!existing.empty()) {
            storedPath = existing.string();
            dumped = true;
            return;
        }
    } else {
        dumped = true;
        return;
    }

    const auto dir = xSharedResidual2DumpDir();
    std::filesystem::create_directories(dir);

    std::ostringstream filename;
    filename << "seq_xshared_loopshared_input_"
             << xSharedLoopSharedInputSubtypeName(subtype)
             << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc;
    if (gRewriteRCaseContext.stepIndex >= 0) {
        filename << "_step" << gRewriteRCaseContext.stepIndex;
    }
    filename << ".txt";
    const auto path = dir / filename.str();

    std::ofstream ofs(path);
    ofs << "trigger="
        << rewriteFallbackTriggerName(
               RewriteFallbackTrigger::RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED)
        << "\n";
    ofs << "residualSubtype="
        << xSharedResidualSubtypeName(XSharedResidualSubtype::XSR_HAFTER_LOOP_SHARED)
        << "\n";
    ofs << "inputSubtype=" << xSharedLoopSharedInputSubtypeName(subtype) << "\n";
    ofs << "cutVertex=" << findXSharedLoopSharedCutVertex(compact) << "\n";
    ofs << "why=" << why << "\n";
    ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
    ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
    ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
    ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar << "\n";
    ofs << "chosenR=" << rNode << "\n";
    ofs << "chosenX=" << x << "\n\n";
    dumpXSharedLoopSharedEdges(compact, ofs);
    ofs << "\n";
    dumpCompactGraph(compact, ofs);
    ofs << "\n";
    dumpActualNodeLocalSlotSummary(core, rNode, ofs);
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);
    ofs << "\n";
    if (trace) {
        dumpGraftTrace(*trace, ofs);
    }

    storedPath = path.string();
    dumped = true;
}

void noteSequenceXSharedLoopSharedBailout(XSharedLoopSharedBailout bailout,
                                          XSharedLoopSharedInputSubtype inputSubtype,
                                          const ReducedSPQRCore &core,
                                          NodeId rNode,
                                          VertexId x,
                                          const CompactGraph &compact,
                                          const GraftTrace *trace,
                                          GraftPostcheckSubtype postcheckSubtype,
                                          const std::string &why) {
    ++gRewriteRStats
          .seqXSharedLoopSharedBailoutCounts[xSharedLoopSharedBailoutIndex(bailout)];

    if (gRewriteRCaseContext.stepIndex >= 0 &&
        gRewriteRCaseContext.stepIndex < static_cast<int>(kRewriteSeqTrackedSteps)) {
        ++gRewriteRStats
              .seqXSharedLoopSharedBailoutAtStepCounts[gRewriteRCaseContext.stepIndex]
                                                     [xSharedLoopSharedBailoutIndex(bailout)];
    }

    auto &dumped =
        gRewriteRStats
            .firstXSharedLoopSharedBailoutDumped[xSharedLoopSharedBailoutIndex(bailout)];
    auto &storedPath =
        gRewriteRStats
            .firstXSharedLoopSharedBailoutDumpPaths[xSharedLoopSharedBailoutIndex(bailout)];
    if (dumped) return;
    if (storedPath.empty()) {
        const auto existing = findExistingXSharedLoopSharedBailoutDump(bailout);
        if (!existing.empty()) {
            storedPath = existing.string();
            dumped = true;
            return;
        }
    } else {
        dumped = true;
        return;
    }

    const auto dir = xSharedResidual2DumpDir();
    std::filesystem::create_directories(dir);

    std::ostringstream filename;
    filename << "seq_xshared_loopshared_bailout_"
             << xSharedLoopSharedBailoutName(bailout)
             << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc;
    if (gRewriteRCaseContext.stepIndex >= 0) {
        filename << "_step" << gRewriteRCaseContext.stepIndex;
    }
    filename << ".txt";
    const auto path = dir / filename.str();

    std::ofstream ofs(path);
    ofs << "trigger="
        << rewriteFallbackTriggerName(
               RewriteFallbackTrigger::RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED)
        << "\n";
    ofs << "residualSubtype="
        << xSharedResidualSubtypeName(XSharedResidualSubtype::XSR_HAFTER_LOOP_SHARED)
        << "\n";
    ofs << "inputSubtype=" << xSharedLoopSharedInputSubtypeName(inputSubtype) << "\n";
    ofs << "bailout=" << xSharedLoopSharedBailoutName(bailout) << "\n";
    ofs << "postcheckSubtype=" << graftPostcheckSubtypeName(postcheckSubtype) << "\n";
    ofs << "cutVertex=" << findXSharedLoopSharedCutVertex(compact) << "\n";
    ofs << "why=" << why << "\n";
    ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
    ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
    ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
    ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar << "\n";
    ofs << "chosenR=" << rNode << "\n";
    ofs << "chosenX=" << x << "\n\n";
    dumpXSharedLoopSharedEdges(compact, ofs);
    ofs << "\n";
    dumpCompactGraph(compact, ofs);
    ofs << "\n";
    dumpActualNodeLocalSlotSummary(core, rNode, ofs);
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);
    ofs << "\n";
    if (trace) {
        dumpGraftTrace(*trace, ofs);
    }

    storedPath = path.string();
    dumped = true;
}

void noteSequenceXSharedBridgeBailout(XSharedBridgeBailout bailout,
                                      const ReducedSPQRCore &core,
                                      NodeId rNode,
                                      VertexId x,
                                      const CompactGraph *compact,
                                      const GraftTrace *trace,
                                      const std::string &why) {
    ++gRewriteRStats
          .seqXIncidentBridgeBailoutCounts[xSharedBridgeBailoutIndex(bailout)];

    auto &seenBailout =
        gRewriteRCaseContext
            .seenXSharedBridgeBailouts[xSharedBridgeBailoutIndex(bailout)];
    if (!seenBailout &&
        gRewriteRCaseContext.stepIndex >= 0 &&
        gRewriteRCaseContext.stepIndex < static_cast<int>(kRewriteSeqTrackedSteps)) {
        seenBailout = true;
    }

    if (gRewriteRCaseContext.stepIndex >= 0 &&
        gRewriteRCaseContext.stepIndex < static_cast<int>(kRewriteSeqTrackedSteps)) {
        ++gRewriteRStats
              .seqXIncidentBridgeBailoutAtStepCounts[gRewriteRCaseContext.stepIndex]
                                                    [xSharedBridgeBailoutIndex(bailout)];
    }

    auto &storedPath =
        gRewriteRStats
            .firstXIncidentBridgeBailoutDumpPaths[xSharedBridgeBailoutIndex(bailout)];
    if (storedPath.empty()) {
        const auto existing = findExistingXSharedBridgeBailoutDump(bailout);
        if (!existing.empty()) {
            storedPath = existing.string();
            return;
        }
    } else {
        return;
    }

    const auto dir = xSharedResidualDumpDir();
    std::filesystem::create_directories(dir);

    std::ostringstream filename;
    filename << "seq_xshared_bailout_" << xSharedBridgeBailoutName(bailout)
             << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc;
    if (gRewriteRCaseContext.stepIndex >= 0) {
        filename << "_step" << gRewriteRCaseContext.stepIndex;
    }
    filename << ".txt";
    const auto path = dir / filename.str();

    std::ofstream ofs(path);
    ofs << "trigger="
        << rewriteFallbackTriggerName(
               RewriteFallbackTrigger::RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED)
        << "\n";
    ofs << "xIncidentSubtype="
        << xIncidentVirtualSubtypeName(XIncidentVirtualSubtype::XIV_SHARED_WITH_LOOP)
        << "\n";
    ofs << "bailoutReason=" << xSharedBridgeBailoutName(bailout) << "\n";
    ofs << "why=" << why << "\n";
    ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
    ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
    ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
    ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar << "\n";
    ofs << "chosenR=" << rNode << "\n";
    ofs << "chosenX=" << x << "\n\n";
    if (compact && (!compact->origOfCv.empty() || !compact->edges.empty())) {
        dumpCompactGraph(*compact, ofs);
        ofs << "\n";
    }
    dumpActualNodeLocalSlotSummary(core, rNode, ofs);
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);
    ofs << "\n";
    if (trace) {
        dumpGraftTrace(*trace, ofs);
    }

    storedPath = path.string();
}

void noteSequencePostcheckSubtype(GraftPostcheckSubtype subtype,
                                  const ReducedSPQRCore &core,
                                  NodeId rNode,
                                  VertexId x,
                                  const CompactGraph *compact,
                                  const GraftTrace *trace,
                                  const std::string &why) {
    ++gRewriteRStats.seqPostcheckSubtypeCounts[graftPostcheckSubtypeIndex(subtype)];

    auto &seenSubtype =
        gRewriteRCaseContext.seenPostcheckSubtypes[graftPostcheckSubtypeIndex(subtype)];
    if (!seenSubtype) {
        seenSubtype = true;
        ++gRewriteRStats
              .seqPostcheckCaseCountsBySubtype[graftPostcheckSubtypeIndex(subtype)];
    }

    auto &dumped =
        gRewriteRStats.firstPostcheckSubtypeDumped[graftPostcheckSubtypeIndex(subtype)];
    auto &storedPath =
        gRewriteRStats.firstPostcheckSubtypeDumpPaths[graftPostcheckSubtypeIndex(subtype)];
    if (dumped) return;

    if (storedPath.empty()) {
        const auto existing = findExistingPostcheckSubtypeDump(subtype);
        if (!existing.empty()) {
            storedPath = existing.string();
            dumped = true;
            return;
        }
    } else {
        dumped = true;
        return;
    }

    const auto dir = postcheckSubtypeDumpDir();
    std::filesystem::create_directories(dir);

    std::ostringstream filename;
    filename << "seq_postcheck_" << graftPostcheckSubtypeName(subtype)
             << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc;
    if (gRewriteRCaseContext.stepIndex >= 0) {
        filename << "_step" << gRewriteRCaseContext.stepIndex;
    }
    filename << ".txt";
    const auto path = dir / filename.str();

    std::ofstream ofs(path);
    ofs << "trigger="
        << rewriteFallbackTriggerName(RewriteFallbackTrigger::RFT_GRAFT_REWIRE_FAIL)
        << "\n";
    ofs << "graftRewireSubtype="
        << graftRewireBailoutSubtypeName(GraftRewireBailoutSubtype::GRB_OTHER) << "\n";
    ofs << "graftOtherSubtype="
        << graftOtherSubtypeName(GraftOtherSubtype::GOS_POSTCHECK_ADJ_MISMATCH) << "\n";
    ofs << "postcheckSubtype=" << graftPostcheckSubtypeName(subtype) << "\n";
    ofs << "why=" << why << "\n";
    ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
    ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
    ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
    ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar << "\n";
    ofs << "chosenR=" << rNode << "\n";
    ofs << "chosenX=" << x << "\n";
    if (trace) {
        ofs << "postcheckWhyDetailed=" << trace->postcheckWhyDetailed << "\n";
        ofs << "firstBadAdjNode=" << trace->firstBadAdjNode << "\n";
    }
    ofs << "\n";
    if (compact) {
        dumpCompactGraph(*compact, ofs);
        ofs << "\n";
    }
    dumpActualNodeLocalSlotSummary(core, rNode, ofs);
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);
    ofs << "\n";
    if (trace) {
        dumpGraftTrace(*trace, ofs);
    }

    storedPath = path.string();
    dumped = true;
}

void noteSequenceGraftRewireSubtype(GraftRewireBailoutSubtype subtype,
                                    const ReducedSPQRCore &core,
                                    NodeId rNode,
                                    VertexId x,
                                    const CompactGraph *compact,
                                    const GraftTrace *trace,
                                    const std::string &why) {
    if (subtype == GraftRewireBailoutSubtype::GRB_OTHER) {
        const GraftOtherSubtype otherSubtype =
            trace ? trace->graftOtherSubtype : GraftOtherSubtype::GOS_OTHER;

        ++gRewriteRStats.seqGraftOtherSubtypeCounts[graftOtherSubtypeIndex(otherSubtype)];

        if (otherSubtype == GraftOtherSubtype::GOS_POSTCHECK_ADJ_MISMATCH) {
            const GraftPostcheckSubtype postcheckSubtype =
                trace ? trace->postcheckSubtype : GraftPostcheckSubtype::GPS_OTHER;
            noteSequencePostcheckSubtype(
                postcheckSubtype, core, rNode, x, compact, trace, why);
        }

        auto &seenOther =
            gRewriteRCaseContext.seenGraftOtherSubtypes[graftOtherSubtypeIndex(otherSubtype)];
        if (!seenOther) {
            seenOther = true;
            ++gRewriteRStats
                  .seqGraftOtherCaseCountsBySubtype[graftOtherSubtypeIndex(otherSubtype)];
        }

        if (gRewriteRCaseContext.stepIndex >= 0 &&
            gRewriteRCaseContext.stepIndex < static_cast<int>(kRewriteSeqTrackedSteps)) {
            ++gRewriteRStats.seqGraftOtherAtStepCounts[gRewriteRCaseContext.stepIndex]
                                                     [graftOtherSubtypeIndex(otherSubtype)];
        }

        auto &storedOtherPath =
            gRewriteRStats.firstGraftOtherDumpPaths[graftOtherSubtypeIndex(otherSubtype)];
        if (storedOtherPath.empty()) {
            const auto existing = findExistingGraftOtherDump(otherSubtype);
            if (!existing.empty()) {
                storedOtherPath = existing.string();
            } else {
                const auto dir = graftOtherDumpDir();
                std::filesystem::create_directories(dir);

                std::ostringstream filename;
                filename << "seq_graft_other_" << graftOtherSubtypeName(otherSubtype)
                         << "_seed" << gRewriteRCaseContext.seed
                         << "_tc" << gRewriteRCaseContext.tc;
                if (gRewriteRCaseContext.stepIndex >= 0) {
                    filename << "_step" << gRewriteRCaseContext.stepIndex;
                }
                filename << ".txt";
                const auto otherPath = dir / filename.str();

                std::ofstream ofs(otherPath);
                ofs << "trigger="
                    << rewriteFallbackTriggerName(
                           RewriteFallbackTrigger::RFT_GRAFT_REWIRE_FAIL)
                    << "\n";
                ofs << "graftRewireSubtype="
                    << graftRewireBailoutSubtypeName(
                           GraftRewireBailoutSubtype::GRB_OTHER)
                    << "\n";
                ofs << "graftOtherSubtype=" << graftOtherSubtypeName(otherSubtype) << "\n";
                ofs << "why=" << why << "\n";
                ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
                ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
                ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
                ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar
                    << "\n";
                ofs << "chosenR=" << rNode << "\n";
                ofs << "chosenX=" << x << "\n";
                if (trace) {
                    ofs << "preservedProxyArcsCount=" << trace->preservedProxyArcsCount
                        << "\n";
                    ofs << "sameNodeRehomeAttempted="
                        << (trace->sameNodeRehomeAttempted ? 1 : 0) << "\n";
                    ofs << "sameNodeRehomeSucceeded="
                        << (trace->sameNodeRehomeSucceeded ? 1 : 0) << "\n";
                    ofs << "failingPreservedInputEdge="
                        << trace->failingPreservedInputEdge << "\n";
                    ofs << "failingPreservedOldArc=" << trace->failingPreservedOldArc
                        << "\n";
                    ofs << "failingPreservedOldSlot=" << trace->failingPreservedOldSlot
                        << "\n";
                    ofs << "failingNewSlot=" << trace->failingNewSlot << "\n";
                    ofs << "graftOtherWhy=" << trace->graftOtherWhy << "\n";
                }
                ofs << "\n";
                if (compact) {
                    dumpCompactGraph(*compact, ofs);
                    ofs << "\n";
                }
                dumpActualNodeLocalSlotSummary(core, rNode, ofs);
                ofs << "\n=== ActualBeforeRewrite ===\n";
                dumpActualCore(core, ofs);
                ofs << "\n";
                if (trace) {
                    dumpGraftTrace(*trace, ofs);
                }

                storedOtherPath = otherPath.string();
            }
        }
    }

    ++gRewriteRStats
          .seqGraftRewireSubtypeCounts[graftRewireBailoutSubtypeIndex(subtype)];

    auto &seenSubtype =
        gRewriteRCaseContext
            .seenGraftRewireSubtypes[graftRewireBailoutSubtypeIndex(subtype)];
    if (!seenSubtype) {
        seenSubtype = true;
        ++gRewriteRStats
              .seqGraftRewireCaseCountsBySubtype[graftRewireBailoutSubtypeIndex(subtype)];
    }

    if (gRewriteRCaseContext.stepIndex >= 0 &&
        gRewriteRCaseContext.stepIndex < static_cast<int>(kRewriteSeqTrackedSteps)) {
        ++gRewriteRStats
              .seqGraftRewireAtStepCounts[gRewriteRCaseContext.stepIndex]
                                         [graftRewireBailoutSubtypeIndex(subtype)];
    }

    auto &storedPath =
        gRewriteRStats
            .firstGraftRewireDumpPaths[graftRewireBailoutSubtypeIndex(subtype)];
    if (storedPath.empty()) {
        const auto existing = findExistingGraftRewireDump(subtype);
        if (!existing.empty()) {
            storedPath = existing.string();
            return;
        }
    } else {
        return;
    }

    const auto dir = graftRewireDumpDir();
    std::filesystem::create_directories(dir);

    std::ostringstream filename;
    filename << "seq_fallback_" << graftRewireBailoutSubtypeName(subtype)
             << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc;
    if (gRewriteRCaseContext.stepIndex >= 0) {
        filename << "_step" << gRewriteRCaseContext.stepIndex;
    }
    filename << ".txt";
    const auto path = dir / filename.str();

    std::ofstream ofs(path);
    ofs << "trigger="
        << rewriteFallbackTriggerName(RewriteFallbackTrigger::RFT_GRAFT_REWIRE_FAIL)
        << "\n";
    ofs << "graftRewireSubtype=" << graftRewireBailoutSubtypeName(subtype) << "\n";
    ofs << "why=" << why << "\n";
    ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
    ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
    ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
    ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar << "\n";
    ofs << "chosenR=" << rNode << "\n";
    ofs << "chosenX=" << x << "\n";
    if (trace) {
        ofs << "failingInputEdge=" << trace->failingInputEdge << "\n";
        ofs << "failingOldArc=" << trace->failingOldArc << "\n";
        ofs << "failingOwnerMini=" << trace->failingOwnerMini << "\n";
        ofs << "failingOwnerMiniSlot=" << trace->failingOwnerMiniSlot << "\n";
    }
    ofs << "\n";
    if (compact) {
        dumpCompactGraph(*compact, ofs);
        ofs << "\n";
    }
    dumpActualNodeLocalSlotSummary(core, rNode, ofs);
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);
    ofs << "\n";
    if (trace) {
        dumpGraftTrace(*trace, ofs);
    }

    storedPath = path.string();
}

void noteSequenceProxyRepairNoCandidateSubtype(ProxyArcNoCandidateSubtype subtype,
                                               const ReducedSPQRCore &core,
                                               NodeId oldNode,
                                               const CompactGraph &compact,
                                               const CompactEdge &proxyEdge,
                                               const RepairedProxyArcInfo &repairInfo,
                                               const std::string &why) {
    ++gRewriteRStats
          .seqProxyRepairNoCandidateSubtypeCounts[proxyArcNoCandidateSubtypeIndex(subtype)];

    auto &seenSubtype =
        gRewriteRCaseContext
            .seenProxyRepairNoCandidateSubtypes[proxyArcNoCandidateSubtypeIndex(subtype)];
    if (!seenSubtype) {
        seenSubtype = true;
        ++gRewriteRStats
              .seqProxyRepairNoCandidateCaseCountsBySubtype
                  [proxyArcNoCandidateSubtypeIndex(subtype)];
    }

    if (gRewriteRCaseContext.stepIndex >= 0 &&
        gRewriteRCaseContext.stepIndex < static_cast<int>(kRewriteSeqTrackedSteps)) {
        ++gRewriteRStats
              .seqProxyRepairNoCandidateAtStepCounts[gRewriteRCaseContext.stepIndex]
                                                    [proxyArcNoCandidateSubtypeIndex(subtype)];
    }

    auto &storedPath =
        gRewriteRStats
            .firstProxyRepairNoCandidateDumpPaths[proxyArcNoCandidateSubtypeIndex(subtype)];
    if (storedPath.empty()) {
        const auto existing = findExistingProxyRepairNoCandidateDump(subtype);
        if (!existing.empty()) {
            storedPath = existing.string();
            return;
        }
    } else {
        return;
    }

    const auto dir = proxyRepairNoCandidateDumpDir();
    std::filesystem::create_directories(dir);

    std::ostringstream filename;
    filename << "seq_proxy_nocand_" << proxyArcNoCandidateSubtypeName(subtype)
             << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc;
    if (gRewriteRCaseContext.stepIndex >= 0) {
        filename << "_step" << gRewriteRCaseContext.stepIndex;
    }
    filename << ".txt";
    const auto path = dir / filename.str();

    const auto expectedPoles = (proxyEdge.a >= 0 &&
                                proxyEdge.a < static_cast<int>(compact.origOfCv.size()) &&
                                proxyEdge.b >= 0 &&
                                proxyEdge.b < static_cast<int>(compact.origOfCv.size()))
            ? canonPole(compact.origOfCv[proxyEdge.a], compact.origOfCv[proxyEdge.b])
            : std::pair<VertexId, VertexId>{-1, -1};

    std::ofstream ofs(path);
    ofs << "trigger="
        << rewriteFallbackTriggerName(RewriteFallbackTrigger::RFT_GRAFT_REWIRE_FAIL)
        << "\n";
    ofs << "repairOutcome="
        << proxyArcRepairOutcomeName(ProxyArcRepairOutcome::PAR_FAIL_NO_CANDIDATE) << "\n";
    ofs << "proxyRepairNoCandidateSubtype=" << proxyArcNoCandidateSubtypeName(subtype) << "\n";
    ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
    ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar << "\n";
    ofs << "chosenR=" << gRewriteRCaseContext.currentRNode << "\n";
    ofs << "chosenX=" << gRewriteRCaseContext.currentX << "\n";
    ofs << "inputEdgeId=" << proxyEdge.id << "\n";
    ofs << "oldArc(original)=" << proxyEdge.oldArc << "\n";
    ofs << "oldNode=" << oldNode << "\n";
    ofs << "outsideNode=" << proxyEdge.outsideNode << "\n";
    ofs << "proxyEdgePoles=(" << expectedPoles.first << "," << expectedPoles.second << ")\n";
    ofs << "resolvedArc=" << repairInfo.resolvedArc << "\n";
    ofs << "resolvedOldSlot=" << repairInfo.resolvedOldSlot << "\n";
    ofs << "why=" << why << "\n\n";
    ofs << "=== CompactLocalView ===\n";
    dumpCompactGraph(compact, ofs);
    ofs << "\n=== OldNodeLiveIncidentArcs ===\n";
    if (!validActualNodeId(core, oldNode) || !core.nodes[oldNode].alive) {
        ofs << "oldNode invalid-or-dead\n";
    } else {
        const auto &node = core.nodes[oldNode];
        for (ArcId arcId = 0; arcId < static_cast<int>(core.arcs.size()); ++arcId) {
            const auto &arc = core.arcs[arcId];
            if (!arc.alive) continue;
            const bool oldOnA = arc.a == oldNode;
            const bool oldOnB = arc.b == oldNode;
            if (oldOnA == oldOnB) continue;
            const NodeId other = oldOnA ? arc.b : arc.a;
            const int slotId = oldOnA ? arc.slotInA : arc.slotInB;
            ofs << "arc " << arcId
                << " other=" << other
                << " poles=(" << arc.poleA << "," << arc.poleB << ")"
                << " oldNodeSlot=" << slotId;
            if (!validActualSlotId(node, slotId)) {
                ofs << " slot=invalid\n";
                continue;
            }
            const auto &slot = node.slots[slotId];
            ofs << " slotAlive=" << (slot.alive ? 1 : 0)
                << " slotVirtual=" << (slot.isVirtual ? 1 : 0)
                << " slotArcId=" << slot.arcId << "\n";
        }
    }
    ofs << "\n=== OldNodeLocalSlots ===\n";
    dumpActualNodeLocalSlotSummary(core, oldNode, ofs);
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);

    storedPath = path.string();
}

void noteSequenceOldArcWeakRepairSample(const ReducedSPQRCore &core,
                                        NodeId oldNode,
                                        const CompactGraph &compact,
                                        const CompactEdge &proxyEdge,
                                        const RepairedProxyArcInfo &repairInfo,
                                        const std::string &why,
                                        bool success) {
    std::string *storedPath = nullptr;
    if (success) {
        storedPath = &gRewriteRStats.firstOldArcWeakRepairSuccessDumpPath;
        if (storedPath->empty()) {
            const auto existing = findExistingOldArcWeakRepairSuccessDump();
            if (!existing.empty()) {
                *storedPath = existing.string();
                return;
            }
        } else {
            return;
        }
    } else {
        auto &slot =
            gRewriteRStats.firstOldArcWeakRepairFailDumpPaths
                [proxyArcRepairOutcomeIndex(repairInfo.repairOutcome)];
        storedPath = &slot;
        if (storedPath->empty()) {
            const auto existing =
                findExistingOldArcWeakRepairFailDump(repairInfo.repairOutcome);
            if (!existing.empty()) {
                *storedPath = existing.string();
                return;
            }
        } else {
            return;
        }
    }

    const auto dir = oldArcWeakRepairDumpDir();
    std::filesystem::create_directories(dir);

    std::ostringstream filename;
    if (success) {
        filename << "seq_proxy_weakrepair_success";
    } else {
        filename << "seq_proxy_weakrepair_fail_"
                 << proxyArcRepairOutcomeName(repairInfo.repairOutcome);
    }
    filename << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc;
    if (gRewriteRCaseContext.stepIndex >= 0) {
        filename << "_step" << gRewriteRCaseContext.stepIndex;
    }
    filename << ".txt";
    const auto path = dir / filename.str();

    const auto expectedPoles = (proxyEdge.a >= 0 &&
                                proxyEdge.a < static_cast<int>(compact.origOfCv.size()) &&
                                proxyEdge.b >= 0 &&
                                proxyEdge.b < static_cast<int>(compact.origOfCv.size()))
            ? canonPole(compact.origOfCv[proxyEdge.a], compact.origOfCv[proxyEdge.b])
            : std::pair<VertexId, VertexId>{-1, -1};

    std::ofstream ofs(path);
    ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
    ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar << "\n";
    ofs << "chosenR=" << gRewriteRCaseContext.currentRNode << "\n";
    ofs << "chosenX=" << gRewriteRCaseContext.currentX << "\n";
    ofs << "inputEdgeId=" << proxyEdge.id << "\n";
    ofs << "originalOldArc=" << repairInfo.originalOldArc << "\n";
    ofs << "originalOutsideNode=" << repairInfo.originalOutsideNode << "\n";
    ofs << "resolvedArc=" << repairInfo.resolvedArc << "\n";
    ofs << "resolvedOutsideNode=" << repairInfo.resolvedOutsideNode << "\n";
    ofs << "oldNode=" << oldNode << "\n";
    ofs << "resolvedOldSlot=" << repairInfo.resolvedOldSlot << "\n";
    ofs << "proxyEdgePoles=(" << expectedPoles.first << "," << expectedPoles.second << ")\n";
    ofs << "repairOutcome=" << proxyArcRepairOutcomeName(repairInfo.repairOutcome) << "\n";
    ofs << "repairUsedWeakPolesOnly=" << (repairInfo.repairUsedWeakPolesOnly ? 1 : 0)
        << "\n";
    ofs << "why=" << why << "\n\n";
    ofs << "=== CompactLocalView ===\n";
    dumpCompactGraph(compact, ofs);
    ofs << "\n=== OldNodeLiveIncidentArcs ===\n";
    if (!validActualNodeId(core, oldNode) || !core.nodes[oldNode].alive) {
        ofs << "oldNode invalid-or-dead\n";
    } else {
        const auto &node = core.nodes[oldNode];
        for (ArcId arcId = 0; arcId < static_cast<int>(core.arcs.size()); ++arcId) {
            const auto &arc = core.arcs[arcId];
            if (!arc.alive) continue;
            const bool oldOnA = arc.a == oldNode;
            const bool oldOnB = arc.b == oldNode;
            if (oldOnA == oldOnB) continue;
            const NodeId other = oldOnA ? arc.b : arc.a;
            const int slotId = oldOnA ? arc.slotInA : arc.slotInB;
            ofs << "arc " << arcId
                << " other=" << other
                << " poles=(" << arc.poleA << "," << arc.poleB << ")"
                << " oldNodeSlot=" << slotId;
            if (!validActualSlotId(node, slotId)) {
                ofs << " slot=invalid\n";
                continue;
            }
            const auto &slot = node.slots[slotId];
            ofs << " slotAlive=" << (slot.alive ? 1 : 0)
                << " slotVirtual=" << (slot.isVirtual ? 1 : 0)
                << " slotArcId=" << slot.arcId << "\n";
        }
    }
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);

    *storedPath = path.string();
}

void dumpCompactBCResult(const CompactBCResult &bc, std::ostream &os) {
    os << "=== CompactBC ===\n";
    os << "valid=" << (bc.valid ? 1 : 0)
       << " blocks=" << bc.blocks.size()
       << " articulations=" << bc.articulationVertices.size()
       << " bcAdjNodes=" << bc.bcAdj.size() << "\n";
    os << "articulationVertices:";
    for (VertexId v : bc.articulationVertices) os << ' ' << v;
    os << "\n";
    if (!bc.articulationVertices.empty()) {
        os << "bcAdjArticulationNodeOffset=" << bc.blocks.size() << "\n";
    }
    for (int blockId = 0; blockId < static_cast<int>(bc.blocks.size()); ++blockId) {
        const auto &block = bc.blocks[blockId];
        os << "block " << blockId
           << " realEdgeCnt=" << block.realEdgeCnt
           << " proxyEdgeCnt=" << block.proxyEdgeCnt
           << " payloadEdgeCnt=" << block.payloadEdgeCnt
           << " payloadVertexCnt=" << block.payloadVertexCnt << "\n";
        os << "  edgeIds:";
        for (int edgeId : block.edgeIds) os << ' ' << edgeId;
        os << "\n  vertices:";
        for (VertexId v : block.vertices) os << ' ' << v;
        os << "\n";
    }
    os << "blocksOfVertex:\n";
    std::vector<VertexId> vertices;
    vertices.reserve(bc.blocksOfVertex.size());
    for (const auto &entry : bc.blocksOfVertex) {
        vertices.push_back(entry.first);
    }
    std::sort(vertices.begin(), vertices.end());
    for (VertexId v : vertices) {
        os << "  " << v << ':';
        auto memberships = bc.blocksOfVertex.at(v);
        std::sort(memberships.begin(), memberships.end());
        for (int blockId : memberships) os << ' ' << blockId;
        os << "\n";
    }
    os << "bcAdj:\n";
    for (int nodeId = 0; nodeId < static_cast<int>(bc.bcAdj.size()); ++nodeId) {
        os << "  " << nodeId << ':';
        auto neighbors = bc.bcAdj[nodeId];
        std::sort(neighbors.begin(), neighbors.end());
        neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
        for (int next : neighbors) os << ' ' << next;
        os << "\n";
    }
}

void noteCompactReject(CompactRejectReason reason,
                       const ReducedSPQRCore &core,
                       NodeId rNode,
                       VertexId x,
                       const CompactGraph *compact,
                       const std::string &why) {
    ++gRewriteRStats.compactRejectedFallbackCount;
    ++gRewriteRStats.compactRejectReasonCounts[compactRejectReasonIndex(reason)];

    auto &storedPath = gRewriteRStats.firstRejectDumpPaths[compactRejectReasonIndex(reason)];
    if (storedPath.empty()) {
        const auto existing = findExistingRejectDump(reason);
        if (!existing.empty()) {
            storedPath = existing.string();
            return;
        }
    } else {
        return;
    }

    const auto dir = rejectDumpDir();
    std::filesystem::create_directories(dir);

    std::ostringstream filename;
    filename << "reject_" << compactRejectReasonName(reason)
             << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc
             << ".txt";
    const auto path = dir / filename.str();

    std::ofstream ofs(path);
    ofs << "reason=" << compactRejectReasonName(reason) << "\n";
    ofs << "why=" << why << "\n";
    ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
    ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
    ofs << "chosenR=" << rNode << "\n";
    ofs << "chosenX=" << x << "\n\n";
    if (compact) {
        dumpCompactGraph(*compact, ofs);
        ofs << "\n";
    }
    ProjectExplicitBlockGraph projectBefore;
    materializeProjectWholeCoreExplicit(core, projectBefore);
    ExplicitBlockGraph explicitBefore;
    exportProjectExplicitBlockGraph(projectBefore, explicitBefore);
    ofs << "=== ExplicitBefore ===\n";
    dumpExplicitBlockGraph(explicitBefore, ofs);
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);

    storedPath = path.string();
}

std::vector<ArcId> collectWeakRepairCandidateArcs(const ReducedSPQRCore &core,
                                                  NodeId oldNode,
                                                  VertexId poleA,
                                                  VertexId poleB) {
    std::vector<ArcId> out;
    if (!validActualNodeId(core, oldNode) || !core.nodes[oldNode].alive) {
        return out;
    }
    const auto expectedPoles = canonPole(poleA, poleB);
    for (ArcId arcId = 0; arcId < static_cast<ArcId>(core.arcs.size()); ++arcId) {
        const auto &arc = core.arcs[arcId];
        if (!arc.alive) continue;
        const bool oldOnA = arc.a == oldNode;
        const bool oldOnB = arc.b == oldNode;
        if (oldOnA == oldOnB) continue;
        if (canonPole(arc.poleA, arc.poleB) != expectedPoles) continue;
        out.push_back(arcId);
    }
    return out;
}

void writeWeakRepairPipelineDump(std::ostream &ofs,
                                 const ReducedSPQRCore &core,
                                 NodeId oldNode,
                                 const CompactGraph &compact,
                                 const CompactEdge &proxyEdge,
                                 const RepairedProxyArcInfo &repairInfo,
                                 const std::string &why,
                                 const GraftTrace *trace,
                                 const ReducedSPQRCore *afterCore) {
    const auto expectedPoles = (proxyEdge.a >= 0 &&
                                proxyEdge.a < static_cast<int>(compact.origOfCv.size()) &&
                                proxyEdge.b >= 0 &&
                                proxyEdge.b < static_cast<int>(compact.origOfCv.size()))
        ? canonPole(compact.origOfCv[proxyEdge.a], compact.origOfCv[proxyEdge.b])
        : std::pair<VertexId, VertexId>{-1, -1};
    ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
    ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
    ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
    ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar << "\n";
    ofs << "chosenR=" << gRewriteRCaseContext.currentRNode << "\n";
    ofs << "chosenX=" << gRewriteRCaseContext.currentX << "\n";
    ofs << "inputEdgeId=" << repairInfo.inputEdgeId << "\n";
    ofs << "weakRepairGateSubtype="
        << weakRepairGateSubtypeName(repairInfo.weakRepairGateSubtype) << "\n";
    ofs << "weakRepairCandidateSubtype="
        << weakRepairCandidateSubtypeName(repairInfo.weakRepairCandidateSubtype) << "\n";
    ofs << "weakRepairCommitOutcome="
        << weakRepairCommitOutcomeName(repairInfo.weakRepairCommitOutcome) << "\n";
    ofs << "repairOutcome=" << proxyArcRepairOutcomeName(repairInfo.repairOutcome) << "\n";
    ofs << "originalOldArc=" << repairInfo.originalOldArc << "\n";
    ofs << "resolvedArc=" << repairInfo.resolvedArc << "\n";
    ofs << "oldNode=" << oldNode << "\n";
    ofs << "originalOutsideNode=" << repairInfo.originalOutsideNode << "\n";
    ofs << "resolvedOutsideNode=" << repairInfo.resolvedOutsideNode << "\n";
    ofs << "resolvedOldSlot=" << repairInfo.resolvedOldSlot << "\n";
    ofs << "proxyEdgePoles=(" << expectedPoles.first << "," << expectedPoles.second << ")\n";
    ofs << "why=" << why << "\n\n";
    ofs << "=== CompactLocalView ===\n";
    dumpCompactGraph(compact, ofs);
    ofs << "\n=== OldNodeLiveIncidentArcs ===\n";
    if (!validActualNodeId(core, oldNode) || !core.nodes[oldNode].alive) {
        ofs << "oldNode invalid-or-dead\n";
    } else {
        const auto &node = core.nodes[oldNode];
        const auto candidateArcs =
            collectWeakRepairCandidateArcs(core, oldNode, expectedPoles.first, expectedPoles.second);
        const std::unordered_set<ArcId> candidateSet(candidateArcs.begin(), candidateArcs.end());
        for (ArcId arcId = 0; arcId < static_cast<int>(core.arcs.size()); ++arcId) {
            const auto &arc = core.arcs[arcId];
            if (!arc.alive) continue;
            const bool oldOnA = arc.a == oldNode;
            const bool oldOnB = arc.b == oldNode;
            if (oldOnA == oldOnB) continue;
            const NodeId other = oldOnA ? arc.b : arc.a;
            const int slotId = oldOnA ? arc.slotInA : arc.slotInB;
            ofs << "arc " << arcId
                << " other=" << other
                << " poles=(" << arc.poleA << "," << arc.poleB << ")"
                << " oldNodeSlot=" << slotId
                << " samePoleCandidate=" << (candidateSet.count(arcId) ? 1 : 0);
            if (!validActualSlotId(node, slotId)) {
                ofs << " slot=invalid\n";
                continue;
            }
            const auto &slot = node.slots[slotId];
            ofs << " slotAlive=" << (slot.alive ? 1 : 0)
                << " slotVirtual=" << (slot.isVirtual ? 1 : 0)
                << " slotArcId=" << slot.arcId << "\n";
        }
    }
    ofs << "\n=== OldNodeLocalSlots ===\n";
    dumpActualNodeLocalSlotSummary(core, oldNode, ofs);
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);
    ofs << "\n";
    if (afterCore) {
        ofs << "=== ActualAfterRewrite ===\n";
        dumpActualCore(*afterCore, ofs);
        ofs << "\n";
    }
    if (trace) {
        dumpGraftTrace(*trace, ofs);
        ofs << "\n";
    }
}

void noteWeakRepairGateSample(const ReducedSPQRCore &core,
                              NodeId oldNode,
                              const CompactGraph &compact,
                              const CompactEdge &proxyEdge,
                              const RepairedProxyArcInfo &repairInfo,
                              const std::string &why) {
    ++gRewriteRStats.seqWeakRepairGateCounts
          [weakRepairGateSubtypeIndex(repairInfo.weakRepairGateSubtype)];
    auto &storedPath =
        gRewriteRStats
            .firstWeakRepairGateDumpPaths[weakRepairGateSubtypeIndex(
                repairInfo.weakRepairGateSubtype)];
    if (storedPath.empty()) {
        const auto existing =
            findExistingWeakRepairGateDump(repairInfo.weakRepairGateSubtype);
        if (!existing.empty()) {
            storedPath = existing.string();
            return;
        }
    } else {
        return;
    }
    const auto dir = weakRepairPipelineDumpDir();
    std::filesystem::create_directories(dir);
    std::ostringstream filename;
    filename << "seq_weak_gate_"
             << weakRepairGateSubtypeName(repairInfo.weakRepairGateSubtype)
             << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc;
    if (gRewriteRCaseContext.stepIndex >= 0) {
        filename << "_step" << gRewriteRCaseContext.stepIndex;
    }
    filename << ".txt";
    const auto path = dir / filename.str();
    std::ofstream ofs(path);
    writeWeakRepairPipelineDump(ofs, core, oldNode, compact, proxyEdge, repairInfo, why,
                                nullptr, nullptr);
    storedPath = path.string();
}

void noteWeakRepairCandidateSample(const ReducedSPQRCore &core,
                                   NodeId oldNode,
                                   const CompactGraph &compact,
                                   const CompactEdge &proxyEdge,
                                   const RepairedProxyArcInfo &repairInfo,
                                   const std::string &why) {
    ++gRewriteRStats.seqWeakRepairCandidateCounts
          [weakRepairCandidateSubtypeIndex(repairInfo.weakRepairCandidateSubtype)];
    auto &storedPath =
        gRewriteRStats
            .firstWeakRepairCandidateDumpPaths[weakRepairCandidateSubtypeIndex(
                repairInfo.weakRepairCandidateSubtype)];
    if (storedPath.empty()) {
        const auto existing =
            findExistingWeakRepairCandidateDump(repairInfo.weakRepairCandidateSubtype);
        if (!existing.empty()) {
            storedPath = existing.string();
            return;
        }
    } else {
        return;
    }
    const auto dir = weakRepairPipelineDumpDir();
    std::filesystem::create_directories(dir);
    std::ostringstream filename;
    filename << "seq_weak_candidate_"
             << weakRepairCandidateSubtypeName(repairInfo.weakRepairCandidateSubtype)
             << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc;
    if (gRewriteRCaseContext.stepIndex >= 0) {
        filename << "_step" << gRewriteRCaseContext.stepIndex;
    }
    filename << ".txt";
    const auto path = dir / filename.str();
    std::ofstream ofs(path);
    writeWeakRepairPipelineDump(ofs, core, oldNode, compact, proxyEdge, repairInfo, why,
                                nullptr, nullptr);
    storedPath = path.string();
}

void noteWeakRepairCommitSample(WeakRepairCommitOutcome outcome,
                                const ReducedSPQRCore *afterCore,
                                const std::string &why) {
    if (!gWeakRepairPipelineContext.pending) return;
    auto info = gWeakRepairPipelineContext.info;
    info.weakRepairCommitOutcome = outcome;
    ++gRewriteRStats.seqWeakRepairCommitCounts[weakRepairCommitOutcomeIndex(outcome)];
    if (outcome == WeakRepairCommitOutcome::WCO_COMMITTED) {
        ++gRewriteRStats.seqWeakRepairCommittedCount;
    } else if (outcome != WeakRepairCommitOutcome::WCO_NOT_ATTEMPTED) {
        ++gRewriteRStats.seqWeakRepairRollbackCount;
    }
    auto &storedPath =
        gRewriteRStats.firstWeakRepairCommitDumpPaths[weakRepairCommitOutcomeIndex(outcome)];
    if (storedPath.empty()) {
        const auto existing = findExistingWeakRepairCommitDump(outcome);
        if (!existing.empty()) {
            storedPath = existing.string();
        } else {
            const auto dir = weakRepairPipelineDumpDir();
            std::filesystem::create_directories(dir);
            std::ostringstream filename;
            filename << "seq_weak_commit_" << weakRepairCommitOutcomeName(outcome)
                     << "_seed" << gRewriteRCaseContext.seed
                     << "_tc" << gRewriteRCaseContext.tc;
            if (gRewriteRCaseContext.stepIndex >= 0) {
                filename << "_step" << gRewriteRCaseContext.stepIndex;
            }
            filename << ".txt";
            const auto path = dir / filename.str();
            std::ofstream ofs(path);
            const CompactEdge *proxyEdge = nullptr;
            for (const auto &edge : gWeakRepairPipelineContext.compact.edges) {
                if (edge.id != info.inputEdgeId) continue;
                proxyEdge = &edge;
                break;
            }
            CompactEdge fallbackEdge;
            if (!proxyEdge) {
                fallbackEdge.id = info.inputEdgeId;
                fallbackEdge.kind = CompactEdgeKind::PROXY;
                proxyEdge = &fallbackEdge;
            }
            writeWeakRepairPipelineDump(ofs,
                                        gWeakRepairPipelineContext.beforeCore,
                                        info.oldNode,
                                        gWeakRepairPipelineContext.compact,
                                        *proxyEdge,
                                        info,
                                        why.empty() ? gWeakRepairPipelineContext.why : why,
                                        gWeakRepairPipelineContext.trace
                                            ? &*gWeakRepairPipelineContext.trace
                                            : nullptr,
                                        afterCore);
            storedPath = path.string();
        }
    }
    gWeakRepairPipelineContext.pending = false;
}

bool isBadProxyArcLifecyclePhase(ProxyArcLifecyclePhase phase) {
    switch (phase) {
        case ProxyArcLifecyclePhase::PAL_AFTER_CLEAR_KEEP_DEAD:
        case ProxyArcLifecyclePhase::PAL_AFTER_CLEAR_KEEP_NOT_INCIDENT:
        case ProxyArcLifecyclePhase::PAL_AFTER_CLEAR_KEEP_SLOT_INVALID:
        case ProxyArcLifecyclePhase::PAL_AFTER_MATERIALIZE_DEAD:
        case ProxyArcLifecyclePhase::PAL_AFTER_MATERIALIZE_NOT_INCIDENT:
        case ProxyArcLifecyclePhase::PAL_AFTER_MATERIALIZE_SLOT_INVALID:
        case ProxyArcLifecyclePhase::PAL_AFTER_INTERNAL_ARCS_DEAD:
        case ProxyArcLifecyclePhase::PAL_AFTER_INTERNAL_ARCS_NOT_INCIDENT:
        case ProxyArcLifecyclePhase::PAL_AFTER_INTERNAL_ARCS_SLOT_INVALID:
        case ProxyArcLifecyclePhase::PAL_PRE_REWIRE_DEAD:
        case ProxyArcLifecyclePhase::PAL_PRE_REWIRE_NOT_INCIDENT:
        case ProxyArcLifecyclePhase::PAL_PRE_REWIRE_SLOT_INVALID:
        case ProxyArcLifecyclePhase::PAL_REWIRE_RET_FALSE:
        case ProxyArcLifecyclePhase::PAL_OTHER:
            return true;
        case ProxyArcLifecyclePhase::PAL_SNAPSHOT_OK:
        case ProxyArcLifecyclePhase::PAL_AFTER_CLEAR_KEEP_ALIVE:
        case ProxyArcLifecyclePhase::PAL_AFTER_MATERIALIZE_ALIVE:
        case ProxyArcLifecyclePhase::PAL_AFTER_INTERNAL_ARCS_ALIVE:
        case ProxyArcLifecyclePhase::PAL_PRE_REWIRE_ALIVE:
        case ProxyArcLifecyclePhase::COUNT:
            return false;
    }
    return true;
}

bool inspectResolvedProxyArcPhaseImpl(const ReducedSPQRCore &core,
                                      const RepairedProxyArcInfo &info,
                                      ProxyArcLifecyclePhase alivePhase,
                                      ProxyArcLifecyclePhase deadPhase,
                                      ProxyArcLifecyclePhase notIncidentPhase,
                                      ProxyArcLifecyclePhase slotInvalidPhase,
                                      ProxyArcLifecyclePhase &outPhase,
                                      std::string &why) {
    why.clear();
    if (!validActualArcId(core, info.resolvedArc)) {
        outPhase = deadPhase;
        why = "proxy lifecycle: resolved arc out of range";
        return false;
    }
    const auto &arc = core.arcs[info.resolvedArc];
    if (!arc.alive) {
        outPhase = deadPhase;
        why = "proxy lifecycle: resolved arc dead";
        return false;
    }
    const bool onA = arc.a == info.oldNode;
    const bool onB = arc.b == info.oldNode;
    if (onA == onB) {
        outPhase = notIncidentPhase;
        why = "proxy lifecycle: resolved arc no longer incident to oldNode";
        return false;
    }
    const int slotId = onA ? arc.slotInA : arc.slotInB;
    if (!validActualNodeId(core, info.oldNode) || !core.nodes[info.oldNode].alive ||
        !validActualSlotId(core.nodes[info.oldNode], slotId)) {
        outPhase = slotInvalidPhase;
        why = "proxy lifecycle: oldNode slot invalid/not virtual/arcId mismatch";
        return false;
    }
    const auto &slot = core.nodes[info.oldNode].slots[slotId];
    if (!slot.alive || !slot.isVirtual || slot.arcId != info.resolvedArc) {
        outPhase = slotInvalidPhase;
        why = "proxy lifecycle: oldNode slot invalid/not virtual/arcId mismatch";
        return false;
    }
    outPhase = alivePhase;
    return true;
}

void noteSequenceProxyArcLifecyclePhaseImpl(const ReducedSPQRCore &core,
                                            NodeId oldNode,
                                            const CompactGraph *compact,
                                            RepairedProxyArcInfo &info,
                                            ProxyArcLifecyclePhase phase,
                                            const std::string &why,
                                            const GraftTrace *trace) {
    info.phaseHistory.push_back(phase);
    ++gRewriteRStats.seqProxyArcLifecycleCounts[proxyArcLifecyclePhaseIndex(phase)];
    if (isBadProxyArcLifecyclePhase(phase) &&
        info.firstBadPhase == ProxyArcLifecyclePhase::PAL_OTHER) {
        info.firstBadPhase = phase;
        info.firstBadWhy = why;
        ++gRewriteRStats.seqProxyArcFirstBadPhaseCounts[proxyArcLifecyclePhaseIndex(phase)];
        auto &seen =
            gRewriteRCaseContext
                .seenProxyArcLifecycleBadPhases[proxyArcLifecyclePhaseIndex(phase)];
        if (!seen) {
            seen = true;
            ++gRewriteRStats
                  .seqProxyArcCaseCountsByFirstBadPhase[proxyArcLifecyclePhaseIndex(phase)];
        }
        auto &storedPath =
            gRewriteRStats.firstProxyArcPhaseDumpPaths[proxyArcLifecyclePhaseIndex(phase)];
        if (storedPath.empty()) {
            const auto existing = findExistingProxyArcLifecycleDump(phase);
            if (!existing.empty()) {
                storedPath = existing.string();
            } else {
                const auto dir = proxyArcLifecycleDumpDir();
                std::filesystem::create_directories(dir);
                std::ostringstream filename;
                filename << "seq_proxy_phase_" << proxyArcLifecyclePhaseName(phase)
                         << "_seed" << gRewriteRCaseContext.seed
                         << "_tc" << gRewriteRCaseContext.tc;
                if (gRewriteRCaseContext.stepIndex >= 0) {
                    filename << "_step" << gRewriteRCaseContext.stepIndex;
                }
                filename << ".txt";
                const auto path = dir / filename.str();
                std::ofstream ofs(path);
                ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
                ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar << "\n";
                ofs << "chosenR=" << gRewriteRCaseContext.currentRNode << "\n";
                ofs << "chosenX=" << gRewriteRCaseContext.currentX << "\n";
                ofs << "inputEdgeId=" << info.inputEdgeId << "\n";
                ofs << "originalOldArc=" << info.originalOldArc << "\n";
                ofs << "resolvedArc=" << info.resolvedArc << "\n";
                ofs << "originalOutsideNode=" << info.originalOutsideNode << "\n";
                ofs << "resolvedOutsideNode=" << info.resolvedOutsideNode << "\n";
                ofs << "oldNode=" << oldNode << "\n";
                ofs << "resolvedOldSlot=" << info.resolvedOldSlot << "\n";
                ofs << "firstBadPhase=" << proxyArcLifecyclePhaseName(info.firstBadPhase) << "\n";
                ofs << "firstBadWhy=" << info.firstBadWhy << "\n";
                ofs << "phaseHistory=";
                for (size_t i = 0; i < info.phaseHistory.size(); ++i) {
                    if (i) ofs << ',';
                    ofs << proxyArcLifecyclePhaseName(info.phaseHistory[i]);
                }
                ofs << "\n";
                ofs << "why=" << why << "\n\n";
                if (compact) {
                    dumpCompactGraph(*compact, ofs);
                    ofs << "\n";
                }
                ofs << "=== ActualBeforeRewrite ===\n";
                if (!gWeakRepairPipelineContext.beforeCore.nodes.empty()) {
                    dumpActualCore(gWeakRepairPipelineContext.beforeCore, ofs);
                } else {
                    dumpActualCore(core, ofs);
                }
                ofs << "\n=== ActualPhaseCore ===\n";
                dumpActualCore(core, ofs);
                ofs << "\n";
                if (trace) {
                    dumpGraftTrace(*trace, ofs);
                }
                storedPath = path.string();
            }
        }
    }
}

void noteTooSmallSubtypeReject(TooSmallSubtype subtype,
                               const ReducedSPQRCore &core,
                               NodeId rNode,
                               VertexId x,
                               const CompactGraph &compact,
                               const std::string &why) {
    ++gRewriteRStats.compactTooSmallSubtypeCounts[tooSmallSubtypeIndex(subtype)];

    auto &storedPath =
        gRewriteRStats.firstTooSmallSubtypeDumpPaths[tooSmallSubtypeIndex(subtype)];
    if (storedPath.empty()) {
        const auto existing = findExistingTooSmallSubtypeDump(subtype);
        if (!existing.empty()) {
            storedPath = existing.string();
            return;
        }
    } else {
        return;
    }

    const auto dir = tinyRejectDumpDir();
    std::filesystem::create_directories(dir);

    std::ostringstream filename;
    filename << "reject_" << tooSmallSubtypeName(subtype)
             << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc
             << ".txt";
    const auto path = dir / filename.str();

    std::ofstream ofs(path);
    ofs << "reason=TOO_SMALL_FOR_SPQR\n";
    ofs << "subtype=" << tooSmallSubtypeName(subtype) << "\n";
    ofs << "why=" << why << "\n";
    ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
    ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
    ofs << "chosenR=" << rNode << "\n";
    ofs << "chosenX=" << x << "\n\n";
    dumpCompactGraph(compact, ofs);
    ofs << "\n=== TinyEdges ===\n";
    for (const auto &edge : compact.edges) {
        ofs << "edge#" << edge.id;
        if (edge.a >= 0 && edge.a < static_cast<int>(compact.origOfCv.size()) &&
            edge.b >= 0 && edge.b < static_cast<int>(compact.origOfCv.size())) {
            const auto poles = canonPole(compact.origOfCv[edge.a], compact.origOfCv[edge.b]);
            ofs << " poles=(" << poles.first << "," << poles.second << ")";
        } else {
            ofs << " poles=(invalid,invalid)";
        }
        ofs << " kind=" << (edge.kind == CompactEdgeKind::REAL ? "REAL" : "PROXY") << "\n";
    }
    ProjectExplicitBlockGraph projectBefore;
    materializeProjectWholeCoreExplicit(core, projectBefore);
    ExplicitBlockGraph explicitBefore;
    exportProjectExplicitBlockGraph(projectBefore, explicitBefore);
    ofs << "\n=== ExplicitBefore ===\n";
    dumpExplicitBlockGraph(explicitBefore, ofs);
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);

    storedPath = path.string();
}

void noteSequenceTooSmallDetailedSample(TooSmallSubtype tinySubtype,
                                        TooSmallOtherSubtype otherSubtype,
                                        const ReducedSPQRCore &core,
                                        NodeId rNode,
                                        VertexId x,
                                        const CompactGraph &compact,
                                        RewriteFallbackTrigger trigger,
                                        const std::string &why) {
    ++gRewriteRStats.seqTooSmallSubtypeCounts[tooSmallSubtypeIndex(tinySubtype)];

    if (tinySubtype != TooSmallSubtype::TS_TWO_OTHER) {
        return;
    }

    ++gRewriteRStats.seqTooSmallOtherSubtypeCounts[tooSmallOtherSubtypeIndex(otherSubtype)];
    auto &seenSubtype =
        gRewriteRCaseContext.seenTooSmallOtherSubtypes[tooSmallOtherSubtypeIndex(otherSubtype)];
    if (!seenSubtype) {
        seenSubtype = true;
        ++gRewriteRStats
              .seqTooSmallCaseCountsBySubtype[tooSmallOtherSubtypeIndex(otherSubtype)];
    }

    auto &storedPath =
        gRewriteRStats.firstTooSmallOtherDumpPaths[tooSmallOtherSubtypeIndex(otherSubtype)];
    if (storedPath.empty()) {
        const auto existing = findExistingTooSmallOtherDump(otherSubtype);
        if (!existing.empty()) {
            storedPath = existing.string();
            return;
        }
    } else {
        return;
    }

    const auto dir = tooSmallOtherDumpDir();
    std::filesystem::create_directories(dir);

    std::ostringstream filename;
    filename << "reject_" << tooSmallOtherSubtypeName(otherSubtype)
             << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc;
    if (gRewriteRCaseContext.stepIndex >= 0) {
        filename << "_step" << gRewriteRCaseContext.stepIndex;
    }
    filename << ".txt";
    const auto path = dir / filename.str();

    std::ofstream ofs(path);
    ofs << "trigger=" << rewriteFallbackTriggerName(trigger) << "\n";
    ofs << "pathTaken=" << rewritePathTakenName(RewritePathTaken::WHOLE_CORE_REBUILD) << "\n";
    ofs << "tinySubtype=" << tooSmallSubtypeName(tinySubtype) << "\n";
    ofs << "otherSubtype=" << tooSmallOtherSubtypeName(otherSubtype) << "\n";
    ofs << "why=" << why << "\n";
    ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
    ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
    ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
    ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar << "\n";
    ofs << "chosenR=" << rNode << "\n";
    ofs << "chosenX=" << x << "\n\n";
    dumpCompactGraph(compact, ofs);
    ofs << "\n=== TinyEdges ===\n";
    for (const auto &edge : compact.edges) {
        ofs << "edge#" << edge.id
            << " kind=" << (edge.kind == CompactEdgeKind::REAL ? "REAL" : "PROXY");
        if (edge.a >= 0 && edge.a < static_cast<int>(compact.origOfCv.size()) &&
            edge.b >= 0 && edge.b < static_cast<int>(compact.origOfCv.size())) {
            const VertexId poleA = compact.origOfCv[edge.a];
            const VertexId poleB = compact.origOfCv[edge.b];
            ofs << " poles=(" << poleA << "," << poleB << ")"
                << " loop=" << (poleA == poleB ? 1 : 0);
        } else {
            ofs << " poles=(invalid,invalid) loop=-1";
        }
        ofs << "\n";
    }
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);

    storedPath = path.string();
}

void noteSequenceTooSmallOneEdgeSample(SequenceOneEdgeSubtype subtype,
                                       const ReducedSPQRCore &core,
                                       NodeId rNode,
                                       VertexId x,
                                       const CompactGraph &compact,
                                       const GraftTrace *trace,
                                       const std::string &why) {
    ++gRewriteRStats.seqTooSmallOneEdgeSubtypeCounts[sequenceOneEdgeSubtypeIndex(subtype)];

    auto &seenSubtype =
        gRewriteRCaseContext
            .seenTooSmallOneEdgeSubtypes[sequenceOneEdgeSubtypeIndex(subtype)];
    if (!seenSubtype) {
        seenSubtype = true;
        ++gRewriteRStats
              .seqTooSmallOneEdgeCaseCountsBySubtype[sequenceOneEdgeSubtypeIndex(subtype)];
    }

    auto &dumped =
        gRewriteRStats.firstTooSmallOneEdgeDumped[sequenceOneEdgeSubtypeIndex(subtype)];
    auto &storedPath =
        gRewriteRStats.firstTooSmallOneEdgeDumpPaths[sequenceOneEdgeSubtypeIndex(subtype)];
    if (dumped) {
        return;
    }
    dumped = true;
    if (storedPath.empty()) {
        const auto existing = findExistingTooSmallOneEdgeDump(subtype);
        if (!existing.empty()) {
            storedPath = existing.string();
            return;
        }
    } else {
        return;
    }

    const auto dir = oneEdgeDumpDir();
    std::filesystem::create_directories(dir);

    std::ostringstream filename;
    filename << "seq_tiny_oneedge_" << sequenceOneEdgeSubtypeName(subtype)
             << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc;
    if (gRewriteRCaseContext.stepIndex >= 0) {
        filename << "_step" << gRewriteRCaseContext.stepIndex;
    }
    filename << ".txt";
    const auto path = dir / filename.str();

    std::ofstream ofs(path);
    ofs << "trigger="
        << rewriteFallbackTriggerName(RewriteFallbackTrigger::RFT_COMPACT_TOO_SMALL_UNHANDLED)
        << "\n";
    ofs << "tinySubtype=" << tooSmallSubtypeName(TooSmallSubtype::TS_ONE_EDGE) << "\n";
    ofs << "oneEdgeSubtype=" << sequenceOneEdgeSubtypeName(subtype) << "\n";
    ofs << "why=" << why << "\n";
    ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
    ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
    ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
    ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar << "\n";
    ofs << "chosenR=" << rNode << "\n";
    ofs << "chosenX=" << x << "\n";
    if (!compact.edges.empty()) {
        const auto &edge = compact.edges.front();
        ofs << "inputEdgeId=" << edge.id << "\n";
        ofs << "kind=" << (edge.kind == CompactEdgeKind::REAL ? "REAL" : "PROXY") << "\n";
        if (edge.a >= 0 && edge.a < static_cast<int>(compact.origOfCv.size()) &&
            edge.b >= 0 && edge.b < static_cast<int>(compact.origOfCv.size())) {
            const VertexId poleA = compact.origOfCv[edge.a];
            const VertexId poleB = compact.origOfCv[edge.b];
            ofs << "poles=(" << poleA << "," << poleB << ")\n";
            ofs << "loop=" << (poleA == poleB ? 1 : 0) << "\n";
        } else {
            ofs << "poles=(invalid,invalid)\n";
            ofs << "loop=-1\n";
        }
        ofs << "realEdge=" << edge.realEdge << "\n";
    }
    ofs << "\n=== CompactLocalView ===\n";
    dumpCompactGraph(compact, ofs);
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);
    if (trace) {
        ofs << "\n=== GraftTrace ===\n";
        dumpGraftTrace(*trace, ofs);
    }

    storedPath = path.string();
}

void noteNotBiconnectedSubtypeReject(NotBiconnectedSubtype subtype,
                                     const ReducedSPQRCore &core,
                                     NodeId rNode,
                                     VertexId x,
                                     const CompactGraph &compact,
                                     const CompactBCResult &bc,
                                     const std::string &why) {
    ++gRewriteRStats
          .compactNotBiconnectedSubtypeCounts[notBiconnectedSubtypeIndex(subtype)];

    auto &storedPath =
        gRewriteRStats
            .firstNotBiconnectedSubtypeDumpPaths[notBiconnectedSubtypeIndex(subtype)];
    if (storedPath.empty()) {
        const auto existing = findExistingNotBiconnectedSubtypeDump(subtype);
        if (!existing.empty()) {
            storedPath = existing.string();
            return;
        }
    } else {
        return;
    }

    const auto dir = rejectDumpDir();
    std::filesystem::create_directories(dir);

    std::ostringstream filename;
    filename << "reject_" << notBiconnectedSubtypeName(subtype)
             << "_seed" << gRewriteRCaseContext.seed
             << "_tc" << gRewriteRCaseContext.tc
             << ".txt";
    const auto path = dir / filename.str();

    std::ofstream ofs(path);
    ofs << "reason=NOT_BICONNECTED\n";
    ofs << "subtype=" << notBiconnectedSubtypeName(subtype) << "\n";
    ofs << "why=" << why << "\n";
    ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
    ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
    ofs << "chosenR=" << rNode << "\n";
    ofs << "chosenX=" << x << "\n\n";
    dumpCompactGraph(compact, ofs);
    ofs << "\n";
    dumpCompactBCResult(bc, ofs);
    ofs << "\n";
    ProjectExplicitBlockGraph projectBefore;
    materializeProjectWholeCoreExplicit(core, projectBefore);
    ExplicitBlockGraph explicitBefore;
    exportProjectExplicitBlockGraph(projectBefore, explicitBefore);
    ofs << "=== ExplicitBefore ===\n";
    dumpExplicitBlockGraph(explicitBefore, ofs);
    ofs << "\n=== ActualBeforeRewrite ===\n";
    dumpActualCore(core, ofs);

    storedPath = path.string();
}

void writeSequenceFallbackDump(std::ostream &ofs,
                               SeqFallbackReason reason,
                               RewriteFallbackTrigger trigger,
                               RewritePathTaken pathTaken,
                               const ReducedSPQRCore &beforeCore,
                               const ReducedSPQRCore *afterCore,
                               NodeId rNode,
                               VertexId x,
                               const CompactGraph *compact,
                               const GraftTrace *trace,
                               const std::string &why,
                               const std::string &subtypeLabel) {
    ofs << "reason=" << seqFallbackReasonName(reason) << "\n";
    ofs << "trigger=" << rewriteFallbackTriggerName(trigger) << "\n";
    ofs << "pathTaken=" << rewritePathTakenName(pathTaken) << "\n";
    if (!subtypeLabel.empty()) {
        ofs << "subtype=" << subtypeLabel << "\n";
    }
    ofs << "why=" << why << "\n";
    ofs << "seed=" << gRewriteRCaseContext.seed << "\n";
    ofs << "tc=" << gRewriteRCaseContext.tc << "\n";
    ofs << "stepIndex=" << gRewriteRCaseContext.stepIndex << "\n";
    ofs << "sequenceLengthSoFar=" << gRewriteRCaseContext.sequenceLengthSoFar << "\n";
    ofs << "chosenR=" << rNode << "\n";
    ofs << "chosenX=" << x << "\n\n";
    if (compact) {
        dumpCompactGraph(*compact, ofs);
        ofs << "\n";
    }
    ofs << "=== ActualBeforeRewrite ===\n";
    dumpActualCore(beforeCore, ofs);
    ofs << "\n";
    if (afterCore) {
        ofs << "=== ActualAfterRewrite ===\n";
        dumpActualCore(*afterCore, ofs);
        ofs << "\n";
    }
    if (trace) {
        dumpGraftTrace(*trace, ofs);
        ofs << "\n";
    }
    ProjectExplicitBlockGraph explicitBeforeProject;
    materializeProjectWholeCoreExplicit(beforeCore, explicitBeforeProject);
    ExplicitBlockGraph explicitBefore;
    exportProjectExplicitBlockGraph(explicitBeforeProject, explicitBefore);
    ofs << "=== ExplicitBefore ===\n";
    dumpExplicitBlockGraph(explicitBefore, ofs);
    ofs << "\n";
    if (afterCore) {
        ProjectExplicitBlockGraph explicitAfterProject;
        materializeProjectWholeCoreExplicit(*afterCore, explicitAfterProject);
        ExplicitBlockGraph explicitAfter;
        exportProjectExplicitBlockGraph(explicitAfterProject, explicitAfter);
        ofs << "=== ExplicitAfter ===\n";
        dumpExplicitBlockGraph(explicitAfter, ofs);
        ofs << "\n";
    }
}

void noteSequenceFallbackSample(SeqFallbackReason reason,
                                RewriteFallbackTrigger trigger,
                                RewritePathTaken pathTaken,
                                const ReducedSPQRCore &beforeCore,
                                const ReducedSPQRCore *afterCore,
                                NodeId rNode,
                                VertexId x,
                                const CompactGraph *compact,
                                const GraftTrace *trace,
                                const std::string &why,
                                const std::string &subtypeLabel = {}) {
    ++gRewriteRStats.seqFallbackReasonCounts[seqFallbackReasonIndex(reason)];
    ++gRewriteRStats.rewriteFallbackTriggerCounts[rewriteFallbackTriggerIndex(trigger)];

    if (gRewriteRCaseContext.stepIndex >= 0 &&
        gRewriteRCaseContext.stepIndex < static_cast<int>(kRewriteSeqTrackedSteps)) {
        ++gRewriteRStats.seqFallbackAtStepCounts[gRewriteRCaseContext.stepIndex];
        ++gRewriteRStats
              .rewriteFallbackTriggerAtStepCounts[gRewriteRCaseContext.stepIndex]
                                                [rewriteFallbackTriggerIndex(trigger)];
    }

    auto &seenTrigger =
        gRewriteRCaseContext.seenFallbackTriggers[rewriteFallbackTriggerIndex(trigger)];
    if (!seenTrigger) {
        seenTrigger = true;
        ++gRewriteRStats
              .rewriteFallbackCaseCountsByTrigger[rewriteFallbackTriggerIndex(trigger)];
    }

    auto &reasonStoredPath =
        gRewriteRStats.firstSeqFallbackDumpPaths[seqFallbackReasonIndex(reason)];
    if (reasonStoredPath.empty()) {
        const auto existing = findExistingSeqFallbackDump(reason);
        if (!existing.empty()) {
            reasonStoredPath = existing.string();
        }
    }

    auto &triggerStoredPath =
        gRewriteRStats
            .firstFallbackTriggerDumpPaths[rewriteFallbackTriggerIndex(trigger)];
    if (triggerStoredPath.empty()) {
        const auto existing = findExistingFallbackTriggerDump(trigger);
        if (!existing.empty()) {
            triggerStoredPath = existing.string();
        }
    }

    const bool needReasonDump = reasonStoredPath.empty();
    const bool needTriggerDump = triggerStoredPath.empty();
    if (!needReasonDump && !needTriggerDump) {
        return;
    }

    const auto dir = seqFallbackDumpDir();
    std::filesystem::create_directories(dir);

    auto buildFilename = [&](const std::string &prefix) {
        std::ostringstream filename;
        filename << prefix
                 << "_seed" << gRewriteRCaseContext.seed
                 << "_tc" << gRewriteRCaseContext.tc;
        if (gRewriteRCaseContext.stepIndex >= 0) {
            filename << "_step" << gRewriteRCaseContext.stepIndex;
        }
        filename << ".txt";
        return dir / filename.str();
    };

    if (needReasonDump) {
        const auto path =
            buildFilename(std::string("seq_fallback_") + seqFallbackReasonName(reason));
        std::ofstream ofs(path);
        writeSequenceFallbackDump(ofs,
                                  reason,
                                  trigger,
                                  pathTaken,
                                  beforeCore,
                                  afterCore,
                                  rNode,
                                  x,
                                  compact,
                                  trace,
                                  why,
                                  subtypeLabel);
        reasonStoredPath = path.string();
    }

    if (needTriggerDump) {
        const auto path =
            buildFilename(std::string("seq_fallback_") + rewriteFallbackTriggerName(trigger));
        std::ofstream ofs(path);
        writeSequenceFallbackDump(ofs,
                                  reason,
                                  trigger,
                                  pathTaken,
                                  beforeCore,
                                  afterCore,
                                  rNode,
                                  x,
                                  compact,
                                  trace,
                                  why,
                                  subtypeLabel);
        triggerStoredPath = path.string();
    }
}

int countCompactConnectedComponents(const CompactGraph &H) {
    const int n = static_cast<int>(H.origOfCv.size());
    if (n == 0) return 0;

    std::vector<std::vector<int>> adj(n);
    for (const auto &edge : H.edges) {
        if (edge.a < 0 || edge.a >= n || edge.b < 0 || edge.b >= n) return -1;
        adj[edge.a].push_back(edge.b);
        adj[edge.b].push_back(edge.a);
    }

    std::vector<char> seen(n, 0);
    int components = 0;
    for (int start = 0; start < n; ++start) {
        if (seen[start]) continue;
        ++components;
        std::queue<int> q;
        q.push(start);
        seen[start] = 1;
        while (!q.empty()) {
            const int u = q.front();
            q.pop();
            for (int v : adj[u]) {
                if (seen[v]) continue;
                seen[v] = 1;
                q.push(v);
            }
        }
    }
    return components;
}

bool allCompactBlocksTiny(const CompactBCResult &bc) {
    if (bc.blocks.empty()) return false;
    for (const auto &block : bc.blocks) {
        const bool tinyByEdgeCount = block.edgeIds.size() < 3;
        const bool tinyByPayload =
            block.payloadEdgeCnt <= 2 && block.payloadVertexCnt <= 3;
        if (!(tinyByEdgeCount || tinyByPayload)) return false;
    }
    return true;
}

bool isBlockCutPathShape(const CompactBCResult &bc) {
    if (bc.bcAdj.empty()) return false;
    int nonIsolated = 0;
    int leafCount = 0;
    for (const auto &neighborsRaw : bc.bcAdj) {
        std::vector<int> neighbors = neighborsRaw;
        std::sort(neighbors.begin(), neighbors.end());
        neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
        const int degree = static_cast<int>(neighbors.size());
        if (degree > 2) return false;
        if (degree > 0) ++nonIsolated;
        if (degree == 1) ++leafCount;
    }
    return nonIsolated >= 3 && leafCount == 2;
}

} // namespace

bool inspectResolvedProxyArcPhase(const ReducedSPQRCore &core,
                                  const RepairedProxyArcInfo &info,
                                  ProxyArcLifecyclePhase alivePhase,
                                  ProxyArcLifecyclePhase deadPhase,
                                  ProxyArcLifecyclePhase notIncidentPhase,
                                  ProxyArcLifecyclePhase slotInvalidPhase,
                                  ProxyArcLifecyclePhase &outPhase,
                                  std::string &why) {
    return inspectResolvedProxyArcPhaseImpl(core,
                                            info,
                                            alivePhase,
                                            deadPhase,
                                            notIncidentPhase,
                                            slotInvalidPhase,
                                            outPhase,
                                            why);
}

std::vector<ArcId> collectAuthoritativeLiveAdjArcs(const ReducedSPQRCore &core,
                                                   NodeId u) {
    std::vector<ArcId> authoritative;
    if (!validActualNodeId(core, u)) return authoritative;

    for (ArcId arcId = 0; arcId < static_cast<ArcId>(core.arcs.size()); ++arcId) {
        const auto &arc = core.arcs[arcId];
        if (!arc.alive) continue;
        if (arc.a != u && arc.b != u) continue;
        authoritative.push_back(arcId);
    }

    std::sort(authoritative.begin(), authoritative.end());
    authoritative.erase(std::unique(authoritative.begin(), authoritative.end()),
                        authoritative.end());
    return authoritative;
}

void rebuildAdjacencyForNodeFromArcs(ReducedSPQRCore &core,
                                     NodeId u) {
    if (!validActualNodeId(core, u)) return;
    auto &node = core.nodes[u];
    node.adjArcs = collectAuthoritativeLiveAdjArcs(core, u);
}

void rebuildAdjacencyForAffectedNodesAfterGraft(ReducedSPQRCore &core,
                                                const std::vector<NodeId> &nodes) {
    std::unordered_set<NodeId> seenNodes;
    uint64_t rebuiltCount = 0;

    for (NodeId nodeId : nodes) {
        if (!validActualNodeId(core, nodeId) || !core.nodes[nodeId].alive) continue;
        if (!seenNodes.insert(nodeId).second) continue;
        rebuildAdjacencyForNodeFromArcs(core, nodeId);
        ++rebuiltCount;
    }

    if (rebuiltCount == 0) return;
    ++gRewriteRStats.seqAdjRepairUsedCount;
    gRewriteRStats.seqAdjRepairAffectedNodeCount += rebuiltCount;
}

std::vector<NodeId> collectAffectedNodesForAdjRepair(
    const GraftTrace &trace,
    NodeId oldNode,
    const std::vector<PreservedProxyArc> &preserved) {
    std::vector<NodeId> nodes;
    std::unordered_set<NodeId> seenNodes;
    std::unordered_set<NodeId> outsideNodes;

    auto addNode = [&](NodeId nodeId) {
        if (nodeId < 0) return;
        if (seenNodes.insert(nodeId).second) {
            nodes.push_back(nodeId);
        }
    };
    auto addOutsideNode = [&](NodeId nodeId) {
        if (nodeId < 0) return;
        outsideNodes.insert(nodeId);
        addNode(nodeId);
    };

    if (oldNode >= 0) {
        ++gRewriteRStats.seqAdjRepairOldNodeCount;
        addNode(oldNode);
    }

    for (NodeId nodeId : trace.actualNodes) {
        addNode(nodeId);
    }

    for (const auto &entry : preserved) {
        addOutsideNode(entry.outsideNode);
    }

    for (const auto &resolved : trace.resolvedProxyEndpoints) {
        addOutsideNode(resolved.outsideNode);
        addOutsideNode(resolved.originalOutsideNode);
        addOutsideNode(resolved.resolvedOutsideNode);
    }

    gRewriteRStats.seqAdjRepairOutsideNodeCount += outsideNodes.size();
    return nodes;
}

std::vector<NodeId> collectSequenceSPCleanupSeeds(
    const ReducedSPQRCore &core,
    NodeId oldNode,
    const GraftTrace *trace,
    const std::vector<PreservedProxyArc> *preserved) {
    std::vector<NodeId> nodes;
    std::unordered_set<NodeId> seenNodes;

    auto addNode = [&](NodeId nodeId) {
        if (!validActualNodeId(core, nodeId)) return;
        if (!core.nodes[nodeId].alive) return;
        if (!seenNodes.insert(nodeId).second) return;
        nodes.push_back(nodeId);
    };
    auto addNeighbors = [&](NodeId nodeId) {
        if (!validActualNodeId(core, nodeId) || !core.nodes[nodeId].alive) return;
        for (ArcId arcId : collectAuthoritativeLiveAdjArcs(core, nodeId)) {
            if (!validActualArcId(core, arcId) || !core.arcs[arcId].alive) continue;
            addNode(otherEndpointOfArc(core.arcs[arcId], nodeId));
        }
    };

    addNode(oldNode);
    addNeighbors(oldNode);

    if (trace != nullptr) {
        for (NodeId nodeId : trace->actualNodes) {
            addNode(nodeId);
            addNeighbors(nodeId);
        }
    }

    if (preserved != nullptr) {
        for (const auto &entry : *preserved) {
            addNode(entry.outsideNode);
        }
    }

    std::sort(nodes.begin(), nodes.end());
    return nodes;
}

bool findForbiddenSameTypeSPAdjacencyActual(const ReducedSPQRCore &core,
                                            ArcId &offendingArc,
                                            NodeId &nodeAOut,
                                            NodeId &nodeBOut);

namespace {

struct PostcheckFailureDetailedInfo {
    GraftPostcheckSubtype subtype = GraftPostcheckSubtype::GPS_OTHER;
    bool adjMismatch = false;
    bool sameTypeSp = false;
    NodeId firstAdjNode = -1;
    std::vector<ArcId> expectedAdj;
    std::vector<ArcId> actualAdj;
};

GraftTrace::ReplayNodeSnapshot captureReplayNodeSnapshot(const ReducedSPQRCore &core,
                                                         NodeId nodeId) {
    GraftTrace::ReplayNodeSnapshot snapshot;
    snapshot.nodeId = nodeId;
    if (!validActualNodeId(core, nodeId)) return snapshot;

    const auto &node = core.nodes[nodeId];
    snapshot.alive = node.alive;
    snapshot.type = node.type;
    snapshot.adjArcs = node.adjArcs;
    snapshot.realEdgesHere = node.realEdgesHere;

    for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
        const auto &slot = node.slots[slotId];
        snapshot.slots.push_back({slotId,
                                  slot.alive,
                                  slot.isVirtual,
                                  slot.poleA,
                                  slot.poleB,
                                  slot.realEdge,
                                  slot.arcId});
    }

    for (ArcId arcId = 0; arcId < static_cast<ArcId>(core.arcs.size()); ++arcId) {
        const auto &arc = core.arcs[arcId];
        if (!arc.alive) continue;
        if (arc.a != nodeId && arc.b != nodeId) continue;
        snapshot.neighboringLiveArcs.push_back(
            {arcId,
             otherEndpointOfArc(arc, nodeId),
             arc.a == nodeId ? arc.slotInA : arc.slotInB,
             arc.a == nodeId ? arc.slotInB : arc.slotInA,
             arc.poleA,
             arc.poleB});
    }

    std::sort(snapshot.adjArcs.begin(), snapshot.adjArcs.end());
    std::sort(snapshot.realEdgesHere.begin(), snapshot.realEdgesHere.end());
    std::sort(snapshot.neighboringLiveArcs.begin(),
              snapshot.neighboringLiveArcs.end(),
              [](const auto &lhs, const auto &rhs) {
                  return lhs.arcId < rhs.arcId;
              });
    return snapshot;
}

void appendReplayNodeSnapshots(std::vector<GraftTrace::ReplayNodeSnapshotPhase> &dst,
                               ReplaySnapshotPhase phase,
                               std::vector<GraftTrace::ReplayNodeSnapshot> nodes) {
    GraftTrace::ReplayNodeSnapshotPhase phaseSnapshot;
    phaseSnapshot.phase = phase;
    phaseSnapshot.nodes = std::move(nodes);
    dst.push_back(std::move(phaseSnapshot));
}

std::string formatAdjArcsForPostcheck(const std::vector<ArcId> &adjArcs) {
    std::ostringstream oss;
    oss << '[';
    for (size_t i = 0; i < adjArcs.size(); ++i) {
        if (i) oss << ',';
        oss << adjArcs[i];
    }
    oss << ']';
    return oss.str();
}

PostcheckFailureDetailedInfo classifyPostcheckFailureDetailedImpl(
    const ReducedSPQRCore &core,
    std::string &whyDetailed) {
    PostcheckFailureDetailedInfo info;
    whyDetailed.clear();

    for (NodeId nodeId = 0; nodeId < static_cast<NodeId>(core.nodes.size()); ++nodeId) {
        if (!validActualNodeId(core, nodeId) || !core.nodes[nodeId].alive) continue;
        const auto expectedAdj = collectAuthoritativeLiveAdjArcs(core, nodeId);

        auto actualAdj = core.nodes[nodeId].adjArcs;
        std::sort(actualAdj.begin(), actualAdj.end());
        actualAdj.erase(std::unique(actualAdj.begin(), actualAdj.end()), actualAdj.end());

        if (expectedAdj == actualAdj) continue;
        info.adjMismatch = true;
        if (info.firstAdjNode < 0) {
            info.firstAdjNode = nodeId;
            info.expectedAdj = expectedAdj;
            info.actualAdj = actualAdj;
        }
    }

    ArcId offendingArc = -1;
    NodeId offendingA = -1;
    NodeId offendingB = -1;
    info.sameTypeSp =
        findForbiddenSameTypeSPAdjacencyActual(core, offendingArc, offendingA, offendingB);

    if (info.adjMismatch && info.sameTypeSp) {
        info.subtype = GraftPostcheckSubtype::GPS_ADJ_AND_SAME_TYPE_SP;
        whyDetailed = "postcheck detailed: adjacency + same-type S/P";
    } else if (info.adjMismatch) {
        info.subtype = GraftPostcheckSubtype::GPS_ADJ_METADATA_ONLY;
        whyDetailed = "postcheck detailed: adjacency metadata only";
    } else if (info.sameTypeSp) {
        info.subtype = GraftPostcheckSubtype::GPS_SAME_TYPE_SP_ONLY;
        whyDetailed = "postcheck detailed: same-type S/P structural adjacency";
    } else {
        info.subtype = GraftPostcheckSubtype::GPS_OTHER;
        whyDetailed = "postcheck detailed: other";
    }

    return info;
}

} // namespace

GraftPostcheckSubtype classifyPostcheckFailureDetailed(const ReducedSPQRCore &core,
                                                       std::string &whyDetailed) {
    return classifyPostcheckFailureDetailedImpl(core, whyDetailed).subtype;
}

bool checkSequencePreNormalizeMetadataOnly(const ReducedSPQRCore &core,
                                           std::string &why) {
    why.clear();
    std::string whyDetailed;
    const auto detailed = classifyPostcheckFailureDetailedImpl(core, whyDetailed);
    if (!detailed.adjMismatch) return true;

    if (detailed.firstAdjNode >= 0) {
        std::ostringstream oss;
        oss << "graft other: adjacency mismatch on node " << detailed.firstAdjNode
            << ", expected=" << formatAdjArcsForPostcheck(detailed.expectedAdj)
            << ", actual=" << formatAdjArcsForPostcheck(detailed.actualAdj);
        why = oss.str();
    } else {
        why = whyDetailed.empty()
                ? std::string("graft other: adjacency metadata mismatch")
                : whyDetailed;
    }
    return false;
}

bool checkSequencePostNormalizeInvariant(const ReducedSPQRCore &core,
                                         std::string &why) {
    why.clear();

    ArcId offendingArc = -1;
    NodeId offendingA = -1;
    NodeId offendingB = -1;
    if (findForbiddenSameTypeSPAdjacencyActual(core, offendingArc, offendingA, offendingB)) {
        std::ostringstream oss;
        oss << "post-normalize invariant: adjacent same-type S/P nodes"
            << " on arc " << offendingArc
            << " (" << offendingA << "," << offendingB << ")";
        why = oss.str();
        return false;
    }
    return true;
}

void clearSequenceDeferredSameTypeSP() {
    gSequenceDeferredSameTypeSPContext = {};
}

bool hasPendingSequenceDeferredSameTypeSP() {
    return gSequenceDeferredSameTypeSPContext.pending;
}

bool peekSequenceDeferredSameTypeSPTrace(GraftTrace &trace) {
    if (!gSequenceDeferredSameTypeSPContext.pending) return false;
    trace = gSequenceDeferredSameTypeSPContext.trace;
    return true;
}

void setSequenceDeferredSameTypeSPTrace(const GraftTrace &trace) {
    if (!gSequenceDeferredSameTypeSPContext.pending) return;
    gSequenceDeferredSameTypeSPContext.trace = trace;
}

void noteSequenceDeferredSameTypeSP(const ReducedSPQRCore &actualBeforeRewrite,
                                    const ReducedSPQRCore &actualAfterRewrite,
                                    const CompactGraph &compact,
                                    const GraftTrace &trace,
                                    const std::string &why) {
    ++gRewriteRStats.seqDeferredSameTypeSPCount;
    if (!gRewriteRCaseContext.sawDeferredSameTypeSP) {
        gRewriteRCaseContext.sawDeferredSameTypeSP = true;
        ++gRewriteRStats.seqDeferredSameTypeSPCaseCount;
    }
    if (gRewriteRCaseContext.stepIndex >= 0 &&
        gRewriteRCaseContext.stepIndex < static_cast<int>(kRewriteSeqTrackedSteps)) {
        ++gRewriteRStats
              .seqDeferredSameTypeSPAtStepCounts[gRewriteRCaseContext.stepIndex];
    }

    if (!gRewriteRStats.firstDeferredSameTypeSPDumped &&
        gRewriteRStats.firstDeferredSameTypeSPDumpPath.empty()) {
        const auto existing = findExistingDeferredSameTypeSPDump();
        if (!existing.empty()) {
            gRewriteRStats.firstDeferredSameTypeSPDumpPath = existing.string();
            gRewriteRStats.firstDeferredSameTypeSPDumped = true;
        }
    } else if (!gRewriteRStats.firstDeferredSameTypeSPDumpPath.empty()) {
        gRewriteRStats.firstDeferredSameTypeSPDumped = true;
    }

    gSequenceDeferredSameTypeSPContext.pending = true;
    gSequenceDeferredSameTypeSPContext.actualBeforeRewrite = actualBeforeRewrite;
    gSequenceDeferredSameTypeSPContext.actualAfterRewrite = actualAfterRewrite;
    gSequenceDeferredSameTypeSPContext.compact = compact;
    gSequenceDeferredSameTypeSPContext.trace = trace;
    gSequenceDeferredSameTypeSPContext.why = why;
    gSequenceDeferredSameTypeSPContext.seed = gRewriteRCaseContext.seed;
    gSequenceDeferredSameTypeSPContext.tc = gRewriteRCaseContext.tc;
    gSequenceDeferredSameTypeSPContext.stepIndex = gRewriteRCaseContext.stepIndex;
    gSequenceDeferredSameTypeSPContext.sequenceLengthSoFar =
        gRewriteRCaseContext.sequenceLengthSoFar;
    gSequenceDeferredSameTypeSPContext.chosenR = gRewriteRCaseContext.currentRNode;
    gSequenceDeferredSameTypeSPContext.chosenX = gRewriteRCaseContext.currentX;
}

void flushSequenceDeferredSameTypeSPDump(const ReducedSPQRCore &actualAfterNormalize) {
    if (!gSequenceDeferredSameTypeSPContext.pending) return;

    auto clearPending = [&]() {
        gSequenceDeferredSameTypeSPContext = {};
    };

    if (!gRewriteRStats.firstDeferredSameTypeSPDumped) {
        if (gRewriteRStats.firstDeferredSameTypeSPDumpPath.empty()) {
            const auto existing = findExistingDeferredSameTypeSPDump();
            if (!existing.empty()) {
                gRewriteRStats.firstDeferredSameTypeSPDumpPath = existing.string();
                gRewriteRStats.firstDeferredSameTypeSPDumped = true;
            }
        } else {
            gRewriteRStats.firstDeferredSameTypeSPDumped = true;
        }
    }

    if (!gRewriteRStats.firstDeferredSameTypeSPDumped) {
        const auto dir = deferredSameTypeSPDumpDir();
        std::filesystem::create_directories(dir);

        std::ostringstream filename;
        filename << "seq_defer_same_type_sp_seed"
                 << gSequenceDeferredSameTypeSPContext.seed
                 << "_tc" << gSequenceDeferredSameTypeSPContext.tc;
        if (gSequenceDeferredSameTypeSPContext.stepIndex >= 0) {
            filename << "_step" << gSequenceDeferredSameTypeSPContext.stepIndex;
        }
        filename << ".txt";
        const auto path = dir / filename.str();

        std::ofstream ofs(path);
        ofs << "seed=" << gSequenceDeferredSameTypeSPContext.seed << "\n";
        ofs << "tc=" << gSequenceDeferredSameTypeSPContext.tc << "\n";
        ofs << "stepIndex=" << gSequenceDeferredSameTypeSPContext.stepIndex << "\n";
        ofs << "sequenceLengthSoFar="
            << gSequenceDeferredSameTypeSPContext.sequenceLengthSoFar << "\n";
        ofs << "chosenR=" << gSequenceDeferredSameTypeSPContext.chosenR << "\n";
        ofs << "chosenX=" << gSequenceDeferredSameTypeSPContext.chosenX << "\n";
        ofs << "postcheckSubtype="
            << graftPostcheckSubtypeName(GraftPostcheckSubtype::GPS_SAME_TYPE_SP_ONLY) << "\n";
        ofs << "reason=" << gSequenceDeferredSameTypeSPContext.why << "\n\n";
        dumpCompactGraph(gSequenceDeferredSameTypeSPContext.compact, ofs);
        ofs << "\n=== ActualBeforeRewrite ===\n";
        dumpActualCore(gSequenceDeferredSameTypeSPContext.actualBeforeRewrite, ofs);
        ofs << "\n=== ActualAfterRewrite ===\n";
        dumpActualCore(gSequenceDeferredSameTypeSPContext.actualAfterRewrite, ofs);
        ofs << "\n=== ActualAfterNormalize ===\n";
        dumpActualCore(actualAfterNormalize, ofs);
        ofs << "\n";
        dumpGraftTrace(gSequenceDeferredSameTypeSPContext.trace, ofs);

        gRewriteRStats.firstDeferredSameTypeSPDumpPath = path.string();
        gRewriteRStats.firstDeferredSameTypeSPDumped = true;
    }

    if (!gSequenceDeferredSameTypeSPContext.trace.sameTypeSPCleanupSeedNodes.empty() &&
        !gRewriteRStats.firstSameTypeSPCleanupDumped) {
        if (gRewriteRStats.firstSameTypeSPCleanupDumpPath.empty()) {
            const auto existing = findExistingSameTypeSPCleanupDump();
            if (!existing.empty()) {
                gRewriteRStats.firstSameTypeSPCleanupDumpPath = existing.string();
                gRewriteRStats.firstSameTypeSPCleanupDumped = true;
            }
        } else {
            gRewriteRStats.firstSameTypeSPCleanupDumped = true;
        }

        if (!gRewriteRStats.firstSameTypeSPCleanupDumped) {
            const auto cleanupDir = sameTypeSPCleanupDumpDir();
            std::filesystem::create_directories(cleanupDir);

            std::ostringstream cleanupFilename;
            cleanupFilename << "seq_sp_cleanup_seed"
                            << gSequenceDeferredSameTypeSPContext.seed
                            << "_tc" << gSequenceDeferredSameTypeSPContext.tc;
            if (gSequenceDeferredSameTypeSPContext.stepIndex >= 0) {
                cleanupFilename << "_step" << gSequenceDeferredSameTypeSPContext.stepIndex;
            }
            cleanupFilename << ".txt";
            const auto cleanupPath = cleanupDir / cleanupFilename.str();

            std::ofstream cleanupOfs(cleanupPath);
            cleanupOfs << "seed=" << gSequenceDeferredSameTypeSPContext.seed << "\n";
            cleanupOfs << "tc=" << gSequenceDeferredSameTypeSPContext.tc << "\n";
            cleanupOfs << "stepIndex=" << gSequenceDeferredSameTypeSPContext.stepIndex << "\n";
            cleanupOfs << "sequenceLengthSoFar="
                       << gSequenceDeferredSameTypeSPContext.sequenceLengthSoFar << "\n";
            cleanupOfs << "chosenR=" << gSequenceDeferredSameTypeSPContext.chosenR << "\n";
            cleanupOfs << "chosenX=" << gSequenceDeferredSameTypeSPContext.chosenX << "\n";
            cleanupOfs << "preCleanupPostcheckSubtype="
                       << graftPostcheckSubtypeName(
                              gSequenceDeferredSameTypeSPContext.trace.preCleanupPostcheckSubtype)
                       << "\n";
            cleanupOfs << "postCleanupPostcheckSubtype="
                       << graftPostcheckSubtypeName(
                              gSequenceDeferredSameTypeSPContext.trace.postCleanupPostcheckSubtype)
                       << "\n";
            cleanupOfs << "mergeCount="
                       << gSequenceDeferredSameTypeSPContext.trace.sameTypeSPCleanupMergeCount
                       << "\n";
            cleanupOfs << "reason=" << gSequenceDeferredSameTypeSPContext.why << "\n\n";
            dumpCompactGraph(gSequenceDeferredSameTypeSPContext.compact, cleanupOfs);
            cleanupOfs << "\n=== ActualBeforeRewrite ===\n";
            dumpActualCore(gSequenceDeferredSameTypeSPContext.actualBeforeRewrite, cleanupOfs);
            cleanupOfs << "\n=== ActualAfterRewrite ===\n";
            dumpActualCore(gSequenceDeferredSameTypeSPContext.actualAfterRewrite, cleanupOfs);
            cleanupOfs << "\n=== ActualAfterNormalize ===\n";
            dumpActualCore(actualAfterNormalize, cleanupOfs);
            cleanupOfs << "\n";
            dumpGraftTrace(gSequenceDeferredSameTypeSPContext.trace, cleanupOfs);

            gRewriteRStats.firstSameTypeSPCleanupDumpPath = cleanupPath.string();
            gRewriteRStats.firstSameTypeSPCleanupDumped = true;
        }
    }

    clearPending();
}

bool cleanupSequenceSameTypeSPAdjacency(ReducedSPQRCore &core,
                                        const std::vector<NodeId> &seedNodes,
                                        std::string &why) {
    why.clear();

    ++gRewriteRStats.seqSameTypeSPCleanupAttemptCount;
    if (!gRewriteRCaseContext.sawSameTypeSPCleanup) {
        gRewriteRCaseContext.sawSameTypeSPCleanup = true;
        ++gRewriteRStats.seqSameTypeSPCleanupCaseCount;
    }

    if (gRewriteRStats.firstSameTypeSPCleanupDumpPath.empty()) {
        const auto existing = findExistingSameTypeSPCleanupDump();
        if (!existing.empty()) {
            gRewriteRStats.firstSameTypeSPCleanupDumpPath = existing.string();
            gRewriteRStats.firstSameTypeSPCleanupDumped = true;
        }
    } else {
        gRewriteRStats.firstSameTypeSPCleanupDumped = true;
    }

    std::vector<NodeId> seeds;
    std::unordered_set<NodeId> seenSeeds;
    for (NodeId nodeId : seedNodes) {
        if (!validActualNodeId(core, nodeId) || !core.nodes[nodeId].alive) continue;
        if (!seenSeeds.insert(nodeId).second) continue;
        seeds.push_back(nodeId);
    }
    std::sort(seeds.begin(), seeds.end());

    if (gSequenceDeferredSameTypeSPContext.pending) {
        gSequenceDeferredSameTypeSPContext.trace.sameTypeSPCleanupSeedNodes = seeds;
        gSequenceDeferredSameTypeSPContext.trace.sameTypeSPCleanupMergeCount = 0;
        gSequenceDeferredSameTypeSPContext.trace.sameTypeSPCleanupMergedPairs.clear();
    }

    auto failCleanup = [&](const std::string &msg) {
        why = msg;
        ++gRewriteRStats.seqSameTypeSPCleanupFailCount;
        return false;
    };

    auto isSameTypeSPPair = [&](NodeId a, NodeId b) {
        if (!validActualNodeId(core, a) || !validActualNodeId(core, b)) return false;
        const auto &nodeA = core.nodes[a];
        const auto &nodeB = core.nodes[b];
        if (!nodeA.alive || !nodeB.alive) return false;
        if (nodeA.type != nodeB.type) return false;
        return nodeA.type == SPQRType::S_NODE || nodeA.type == SPQRType::P_NODE;
    };
    auto aliveSlotCount = [&](NodeId nodeId) {
        int count = 0;
        for (const auto &slot : core.nodes[nodeId].slots) {
            if (slot.alive) ++count;
        }
        return count;
    };
    auto aliveRealSlotCount = [&](NodeId nodeId) {
        int count = 0;
        for (const auto &slot : core.nodes[nodeId].slots) {
            if (slot.alive && !slot.isVirtual) ++count;
        }
        return count;
    };
    auto collectLiveNeighbors = [&](NodeId nodeId) {
        std::vector<NodeId> neighbors;
        std::unordered_set<NodeId> seenNodes;
        if (!validActualNodeId(core, nodeId) || !core.nodes[nodeId].alive) return neighbors;
        for (ArcId arcId : collectAuthoritativeLiveAdjArcs(core, nodeId)) {
            if (!validActualArcId(core, arcId) || !core.arcs[arcId].alive) continue;
            const NodeId other = otherEndpointOfArc(core.arcs[arcId], nodeId);
            if (!validActualNodeId(core, other) || !core.nodes[other].alive) continue;
            if (!seenNodes.insert(other).second) continue;
            neighbors.push_back(other);
        }
        return neighbors;
    };
    auto queueNode = [&](NodeId nodeId,
                         std::vector<NodeId> &worklist,
                         std::unordered_set<NodeId> &queued,
                         std::unordered_set<NodeId> &refreshNodes) {
        if (!validActualNodeId(core, nodeId) || !core.nodes[nodeId].alive) return;
        refreshNodes.insert(nodeId);
        if (!queued.insert(nodeId).second) return;
        worklist.push_back(nodeId);
    };

    std::vector<NodeId> worklist;
    std::unordered_set<NodeId> queued;
    std::unordered_set<NodeId> refreshNodes;
    for (NodeId nodeId : seeds) {
        queueNode(nodeId, worklist, queued, refreshNodes);
    }

    int mergeCount = 0;
    std::vector<GraftTrace::SameTypeSPCleanupMerge> merges;

    while (!worklist.empty()) {
        const NodeId u = worklist.back();
        worklist.pop_back();
        queued.erase(u);
        if (!validActualNodeId(core, u) || !core.nodes[u].alive) continue;

        bool mergedPair = false;
        for (ArcId arcId : collectAuthoritativeLiveAdjArcs(core, u)) {
            if (!validActualArcId(core, arcId) || !core.arcs[arcId].alive) continue;
            const NodeId v = otherEndpointOfArc(core.arcs[arcId], u);
            if (!isSameTypeSPPair(u, v)) continue;

            const auto score = [&](NodeId nodeId) {
                return std::make_tuple(aliveRealSlotCount(nodeId),
                                       aliveSlotCount(nodeId),
                                       -nodeId);
            };

            NodeId keep = u;
            NodeId loser = v;
            if (score(v) > score(u)) {
                keep = v;
                loser = u;
            }

            std::unordered_set<NodeId> impactedNeighbors;
            for (NodeId nodeId : collectLiveNeighbors(keep)) {
                if (nodeId != loser) impactedNeighbors.insert(nodeId);
            }
            for (NodeId nodeId : collectLiveNeighbors(loser)) {
                if (nodeId != keep) impactedNeighbors.insert(nodeId);
            }

            std::vector<ArcId> middleArcs;
            for (ArcId candidate = 0; candidate < static_cast<ArcId>(core.arcs.size()); ++candidate) {
                const auto &arc = core.arcs[candidate];
                if (!arc.alive) continue;
                if ((arc.a == keep && arc.b == loser) || (arc.a == loser && arc.b == keep)) {
                    middleArcs.push_back(candidate);
                }
            }
            if (middleArcs.empty()) {
                return failCleanup("seq sp-cleanup: same-type pair missing middle arc");
            }

            eraseActualRealOwnershipForNode(core, loser);
            for (ArcId middleArc : middleArcs) {
                killActualArcForPreserveClear(core, middleArc);
            }

            auto &keepNode = core.nodes[keep];
            auto &loserNode = core.nodes[loser];
            for (int slotId = 0; slotId < static_cast<int>(loserNode.slots.size()); ++slotId) {
                auto &slot = loserNode.slots[slotId];
                if (!slot.alive) continue;

                const SkeletonSlot moved = slot;
                const int newSlot = static_cast<int>(keepNode.slots.size());
                keepNode.slots.push_back(moved);
                auto &newKeepSlot = keepNode.slots.back();

                if (newKeepSlot.isVirtual) {
                    if (!validActualArcId(core, newKeepSlot.arcId) ||
                        !core.arcs[newKeepSlot.arcId].alive) {
                        return failCleanup("seq sp-cleanup: moved virtual slot arc invalid");
                    }

                    auto &arc = core.arcs[newKeepSlot.arcId];
                    if (arc.a == loser) {
                        arc.a = keep;
                        arc.slotInA = newSlot;
                    } else if (arc.b == loser) {
                        arc.b = keep;
                        arc.slotInB = newSlot;
                    } else {
                        return failCleanup("seq sp-cleanup: moved virtual slot arc not incident to loser");
                    }
                } else {
                    if (newKeepSlot.realEdge < 0) {
                        return failCleanup("seq sp-cleanup: moved real slot missing edge");
                    }
                    core.ownerNodeOfRealEdge[newKeepSlot.realEdge] = keep;
                    core.ownerSlotOfRealEdge[newKeepSlot.realEdge] = newSlot;
                }

                slot.alive = false;
                if (slot.isVirtual) slot.arcId = -1;
            }

            loserNode.alive = false;
            loserNode.adjArcs.clear();
            loserNode.realEdgesHere.clear();
            loserNode.localAgg = {};
            loserNode.subAgg = {};
            if (core.root == loser) {
                core.root = keep;
            }

            refreshNodes.insert(keep);
            for (NodeId nodeId : impactedNeighbors) {
                queueNode(nodeId, worklist, queued, refreshNodes);
            }
            queueNode(keep, worklist, queued, refreshNodes);

            merges.push_back({u, v, keep});
            ++mergeCount;
            mergedPair = true;
            break;
        }

        if (mergedPair) continue;
    }

    std::unordered_set<NodeId> expandedRefresh = refreshNodes;
    for (NodeId nodeId : std::vector<NodeId>(refreshNodes.begin(), refreshNodes.end())) {
        if (!validActualNodeId(core, nodeId) || !core.nodes[nodeId].alive) continue;
        for (NodeId neighbor : collectLiveNeighbors(nodeId)) {
            expandedRefresh.insert(neighbor);
        }
    }

    for (NodeId nodeId : expandedRefresh) {
        if (!validActualNodeId(core, nodeId) || !core.nodes[nodeId].alive) continue;
        rebuildAdjacencyForNodeFromArcs(core, nodeId);
    }

    std::string refreshWhy;
    if (!normalizeProjectTouchedRegion(core, refreshWhy)) {
        return failCleanup("seq sp-cleanup: normalize refresh failed: " + refreshWhy);
    }

    for (NodeId nodeId : expandedRefresh) {
        if (!validActualNodeId(core, nodeId) || !core.nodes[nodeId].alive) continue;
        for (ArcId arcId : collectAuthoritativeLiveAdjArcs(core, nodeId)) {
            if (!validActualArcId(core, arcId) || !core.arcs[arcId].alive) continue;
            const NodeId other = otherEndpointOfArc(core.arcs[arcId], nodeId);
            if (!isSameTypeSPPair(nodeId, other)) continue;
            return failCleanup(
                "seq sp-cleanup: same-type S/P still remains after cleanup");
        }
    }

    gRewriteRStats.seqSameTypeSPCleanupMergeCount += mergeCount;
    ++gRewriteRStats.seqSameTypeSPCleanupSuccessCount;

    if (gSequenceDeferredSameTypeSPContext.pending) {
        gSequenceDeferredSameTypeSPContext.trace.sameTypeSPCleanupSeedNodes = seeds;
        gSequenceDeferredSameTypeSPContext.trace.sameTypeSPCleanupMergeCount = mergeCount;
        gSequenceDeferredSameTypeSPContext.trace.sameTypeSPCleanupMergedPairs = merges;
    }

    return true;
}

void setRewriteRSequenceReplayCaptureEnabled(bool enabled) {
    gSequenceReplayCaptureContext.enabled = enabled;
    if (!enabled) {
        gSequenceReplayCaptureContext.hasTrace = false;
        gSequenceReplayCaptureContext.trace = {};
    }
}

void clearRewriteRSequenceReplayCapture() {
    gSequenceReplayCaptureContext.hasTrace = false;
    gSequenceReplayCaptureContext.trace = {};
}

bool takeRewriteRSequenceReplayTrace(GraftTrace &trace) {
    if (!gSequenceReplayCaptureContext.hasTrace) return false;
    trace = gSequenceReplayCaptureContext.trace;
    gSequenceReplayCaptureContext.hasTrace = false;
    gSequenceReplayCaptureContext.trace = {};
    return true;
}

void publishRewriteRSequenceReplayTrace(const GraftTrace &trace) {
    if (!gSequenceReplayCaptureContext.enabled) return;
    gSequenceReplayCaptureContext.trace = trace;
    gSequenceReplayCaptureContext.hasTrace = true;
}

void captureReplaySnapshotsForPhase(const ReducedSPQRCore &core,
                                    NodeId oldNode,
                                    NodeId chosenR,
                                    const std::vector<NodeId> &actualNodes,
                                    const std::vector<ResolvedProxyEndpoint> &resolved,
                                    const std::vector<PreservedProxyArc> &preserved,
                                    ReplaySnapshotPhase phase,
                                    GraftTrace &trace) {
    if (!gSequenceReplayCaptureContext.enabled) return;

    std::vector<GraftTrace::ReplayNodeSnapshot> oldNodeSnapshots;
    if (validActualNodeId(core, oldNode)) {
        oldNodeSnapshots.push_back(captureReplayNodeSnapshot(core, oldNode));
    }
    appendReplayNodeSnapshots(trace.oldNodeSnapshotsByPhase,
                              phase,
                              std::move(oldNodeSnapshots));

    std::unordered_set<NodeId> seenNodes;
    std::vector<GraftTrace::ReplayNodeSnapshot> affectedSnapshots;
    auto addNode = [&](NodeId nodeId) {
        if (nodeId < 0 || nodeId == oldNode) return;
        if (!validActualNodeId(core, nodeId)) return;
        if (!seenNodes.insert(nodeId).second) return;
        affectedSnapshots.push_back(captureReplayNodeSnapshot(core, nodeId));
    };

    for (NodeId nodeId : actualNodes) addNode(nodeId);
    for (const auto &entry : resolved) {
        addNode(entry.outsideNode);
        addNode(entry.originalOutsideNode);
        addNode(entry.resolvedOutsideNode);
    }
    for (const auto &entry : preserved) addNode(entry.outsideNode);
    for (ArcId arcId : collectAuthoritativeLiveAdjArcs(core, chosenR)) {
        if (!validActualArcId(core, arcId)) continue;
        addNode(otherEndpointOfArc(core.arcs[arcId], chosenR));
    }

    std::sort(affectedSnapshots.begin(),
              affectedSnapshots.end(),
              [](const auto &lhs, const auto &rhs) {
                  return lhs.nodeId < rhs.nodeId;
              });
    appendReplayNodeSnapshots(trace.affectedNodeSnapshotsByPhase,
                              phase,
                              std::move(affectedSnapshots));
}

void noteSequenceProxyArcLifecyclePhase(const ReducedSPQRCore &core,
                                        NodeId oldNode,
                                        const CompactGraph *compact,
                                        RepairedProxyArcInfo &info,
                                        ProxyArcLifecyclePhase phase,
                                        const std::string &why,
                                        const GraftTrace *trace) {
    noteSequenceProxyArcLifecyclePhaseImpl(core, oldNode, compact, info, phase, why, trace);
}

bool collectPreservedProxyArcsBeforeClear(const ReducedSPQRCore &core,
                                          NodeId oldNode,
                                          const std::vector<ResolvedProxyEndpoint> &resolved,
                                          std::vector<PreservedProxyArc> &out,
                                          std::string &why) {
    out.clear();
    why.clear();

    if (resolved.empty()) {
        return fail(why, "graft other: preserved snapshot empty");
    }

    if (!validActualNodeId(core, oldNode) || !core.nodes[oldNode].alive) {
        return fail(why, "preserve proxy: old node invalid before clear");
    }

    const auto &node = core.nodes[oldNode];
    std::unordered_set<int> seenSlots;
    std::unordered_set<ArcId> seenArcs;
    std::unordered_set<int> seenInputs;

    for (const auto &entry : resolved) {
        if (entry.inputEdgeId < 0) {
            return fail(why, "preserve proxy: input edge id invalid before clear");
        }
        if (!seenInputs.insert(entry.inputEdgeId).second) {
            return fail(why, "preserve proxy: duplicate input edge before clear");
        }
        if (!validActualArcId(core, entry.resolvedArc) ||
            !core.arcs[entry.resolvedArc].alive) {
            return fail(why, "preserve proxy: oldArc dead before clear");
        }

        const auto &arc = core.arcs[entry.resolvedArc];
        const bool oldOnA = arc.a == oldNode;
        const bool oldOnB = arc.b == oldNode;
        if (oldOnA == oldOnB) {
            return fail(why, "preserve proxy: oldArc not incident to oldNode before clear");
        }

        const NodeId outsideNode = oldOnA ? arc.b : arc.a;
        if (outsideNode != entry.outsideNode) {
            return fail(why, "preserve proxy: outsideNode mismatch before clear");
        }

        if (!validActualSlotId(node, entry.resolvedOldSlot)) {
            return fail(why, "preserve proxy: resolvedOldSlot invalid before clear");
        }
        const auto &slot = node.slots[entry.resolvedOldSlot];
        if (!slot.alive) {
            return fail(why, "graft other: preserved slot dead");
        }
        if (!slot.isVirtual) {
            return fail(why, "graft other: preserved slot not virtual");
        }
        if (slot.arcId != entry.resolvedArc) {
            return fail(why, "graft other: preserved slot arcId mismatch");
        }
        if (!seenSlots.insert(entry.resolvedOldSlot).second) {
            return fail(why, "graft other: duplicate preserved old slot");
        }
        if (!seenArcs.insert(entry.resolvedArc).second) {
            return fail(why, "preserve proxy: duplicate oldArc before clear");
        }

        PreservedProxyArc preserved;
        preserved.inputEdgeId = entry.inputEdgeId;
        preserved.oldArc = entry.resolvedArc;
        preserved.oldNode = oldNode;
        preserved.outsideNode = outsideNode;
        preserved.resolvedOldSlot = entry.resolvedOldSlot;
        preserved.poleA = arc.poleA;
        preserved.poleB = arc.poleB;
        out.push_back(preserved);
    }

    return true;
}

bool clearNodeKeepIdForGraftPreserveResolvedProxyArcs(
    ReducedSPQRCore &core,
    NodeId oldNode,
    const std::vector<PreservedProxyArc> &preserved,
    SPQRType newType,
    std::string &why) {
    why.clear();
    if (!validActualNodeId(core, oldNode) || !core.nodes[oldNode].alive) {
        return fail(why, "clear preserve: invalid old node");
    }

    auto &node = core.nodes[oldNode];
    std::unordered_set<int> preservedSlots;
    std::unordered_set<ArcId> preservedArcs;

    for (const auto &p : preserved) {
        if (p.oldNode != oldNode) {
            return fail(why, "clear preserve: preserved oldNode mismatch");
        }
        if (!validActualArcId(core, p.oldArc) || !core.arcs[p.oldArc].alive) {
            return fail(why, "clear preserve: preserved oldArc missing from adjacency");
        }
        const auto &arc = core.arcs[p.oldArc];
        const bool oldOnA = arc.a == oldNode;
        const bool oldOnB = arc.b == oldNode;
        if (oldOnA == oldOnB) {
            return fail(why, "clear preserve: preserved oldArc not incident to oldNode");
        }
        if (!validActualSlotId(node, p.resolvedOldSlot)) {
            return fail(why, "clear preserve: resolved old slot invalid");
        }
        const auto &slot = node.slots[p.resolvedOldSlot];
        if (!slot.alive) {
            return fail(why, "graft other: preserved slot dead");
        }
        if (!slot.isVirtual) {
            return fail(why, "clear preserve: preserved slot not virtual");
        }
        if (slot.arcId != p.oldArc) {
            return fail(why, "clear preserve: preserved slot arcId mismatch");
        }
        if (std::find(node.adjArcs.begin(), node.adjArcs.end(), p.oldArc) ==
            node.adjArcs.end()) {
            return fail(why, "clear preserve: preserved oldArc missing from adjacency");
        }
        if (!preservedSlots.insert(p.resolvedOldSlot).second) {
            return fail(why, "clear preserve: duplicate preserved old slot");
        }
        if (!preservedArcs.insert(p.oldArc).second) {
            return fail(why, "clear preserve: duplicate preserved oldArc");
        }
    }

    std::vector<ArcId> arcsToKill;
    arcsToKill.reserve(node.adjArcs.size());
    for (ArcId arcId : node.adjArcs) {
        if (preservedArcs.count(arcId) != 0) continue;
        if (!validActualArcId(core, arcId) || !core.arcs[arcId].alive) continue;
        arcsToKill.push_back(arcId);
    }
    for (ArcId arcId : arcsToKill) {
        killActualArcForPreserveClear(core, arcId);
    }

    for (auto &slot : node.slots) {
        if (!slot.alive) continue;
        const bool keepAlive =
            slot.isVirtual &&
            preservedSlots.count(static_cast<int>(&slot - node.slots.data())) != 0;
        if (keepAlive) continue;
        slot.alive = false;
        if (slot.isVirtual) slot.arcId = -1;
    }

    eraseActualRealOwnershipForNode(core, oldNode);
    node.type = newType;
    node.adjArcs.clear();
    for (ArcId arcId : preservedArcs) {
        if (!validActualArcId(core, arcId) || !core.arcs[arcId].alive) {
            return fail(why, "clear preserve: preserved oldArc missing from adjacency");
        }
        addActualAdjArc(node, arcId);
    }
    node.realEdgesHere.clear();
    node.localAgg = {};
    node.subAgg = {};

    for (const auto &kv : core.ownerNodeOfRealEdge) {
        if (kv.second == oldNode) {
            return fail(why, "clear preserve: failed to clear keep node ownership");
        }
    }
    return true;
}

bool rehomePreservedProxyArcOnSameNode(ReducedSPQRCore &core,
                                       const PreservedProxyArc &p,
                                       int newSlot,
                                       std::string &why) {
    why.clear();
    if (!validActualArcId(core, p.oldArc) || !core.arcs[p.oldArc].alive) {
        return fail(why, "rehome same-node: oldArc dead");
    }
    if (!validActualNodeId(core, p.oldNode) || !core.nodes[p.oldNode].alive) {
        return fail(why, "rehome same-node: oldNode invalid");
    }

    auto &node = core.nodes[p.oldNode];
    if (!validActualSlotId(node, newSlot)) {
        return fail(why, "rehome same-node: newSlot invalid");
    }
    if (!validActualSlotId(node, p.resolvedOldSlot)) {
        return fail(why, "rehome same-node: preserved old slot invalid");
    }

    auto &arc = core.arcs[p.oldArc];
    const bool oldOnA = arc.a == p.oldNode;
    const bool oldOnB = arc.b == p.oldNode;
    if (oldOnA == oldOnB) {
        return fail(why, "rehome same-node: oldNode not incident to oldArc");
    }

    auto &newActualSlot = node.slots[newSlot];
    if (!newActualSlot.alive) {
        return fail(why, "rehome same-node: newSlot invalid");
    }
    if (!newActualSlot.isVirtual) {
        return fail(why, "rehome same-node: newSlot not virtual");
    }

    newActualSlot.poleA = arc.poleA;
    newActualSlot.poleB = arc.poleB;
    newActualSlot.arcId = p.oldArc;
    if (oldOnA) {
        arc.slotInA = newSlot;
    } else {
        arc.slotInB = newSlot;
    }

    auto &oldSlot = node.slots[p.resolvedOldSlot];
    oldSlot.alive = false;
    if (oldSlot.isVirtual) oldSlot.arcId = -1;
    return true;
}

void noteSequenceClearPreserveRequested(size_t arcCount) {
    ++gRewriteRStats.seqClearPreserveRequestedCount;
    gRewriteRStats.seqClearPreserveArcCount += arcCount;
}

void noteSequenceClearPreserveCrossNodeRewire() {
    ++gRewriteRStats.seqClearPreserveCrossNodeRewireCount;
}

void noteSequenceClearPreserveSameNodeRehome() {
    ++gRewriteRStats.seqClearPreserveSameNodeRehomeCount;
}

void noteSequenceClearPreserveFallback() {
    ++gRewriteRStats.seqClearPreserveFallbackCount;
}

const char *compactRejectReasonName(CompactRejectReason reason) {
    switch (reason) {
        case CompactRejectReason::EMPTY_AFTER_DELETE: return "EMPTY_AFTER_DELETE";
        case CompactRejectReason::TOO_SMALL_FOR_SPQR: return "TOO_SMALL_FOR_SPQR";
        case CompactRejectReason::NOT_BICONNECTED: return "NOT_BICONNECTED";
        case CompactRejectReason::X_INCIDENT_VIRTUAL_UNSUPPORTED: return "X_INCIDENT_VIRTUAL_UNSUPPORTED";
        case CompactRejectReason::SELF_LOOP: return "SELF_LOOP";
        case CompactRejectReason::OWNER_NOT_R: return "OWNER_NOT_R";
        case CompactRejectReason::X_NOT_PRESENT_IN_R: return "X_NOT_PRESENT_IN_R";
        case CompactRejectReason::OTHER: return "OTHER";
        case CompactRejectReason::COUNT: return "COUNT";
    }
    return "OTHER";
}

const char *notBiconnectedSubtypeName(NotBiconnectedSubtype subtype) {
    switch (subtype) {
        case NotBiconnectedSubtype::NB_DISCONNECTED: return "NB_DISCONNECTED";
        case NotBiconnectedSubtype::NB_SINGLE_CUT_TWO_BLOCKS: return "NB_SINGLE_CUT_TWO_BLOCKS";
        case NotBiconnectedSubtype::NB_PATH_OF_BLOCKS: return "NB_PATH_OF_BLOCKS";
        case NotBiconnectedSubtype::NB_STAR_AROUND_ONE_CUT: return "NB_STAR_AROUND_ONE_CUT";
        case NotBiconnectedSubtype::NB_COMPLEX_MULTI_CUT: return "NB_COMPLEX_MULTI_CUT";
        case NotBiconnectedSubtype::NB_BLOCKS_ALL_TINY: return "NB_BLOCKS_ALL_TINY";
        case NotBiconnectedSubtype::NB_OTHER: return "NB_OTHER";
        case NotBiconnectedSubtype::COUNT: return "COUNT";
    }
    return "NB_OTHER";
}

const char *tooSmallSubtypeName(TooSmallSubtype subtype) {
    switch (subtype) {
        case TooSmallSubtype::TS_EMPTY: return "TS_EMPTY";
        case TooSmallSubtype::TS_ONE_EDGE: return "TS_ONE_EDGE";
        case TooSmallSubtype::TS_TWO_PARALLEL: return "TS_TWO_PARALLEL";
        case TooSmallSubtype::TS_TWO_PATH: return "TS_TWO_PATH";
        case TooSmallSubtype::TS_TWO_DISCONNECTED: return "TS_TWO_DISCONNECTED";
        case TooSmallSubtype::TS_TWO_OTHER: return "TS_TWO_OTHER";
        case TooSmallSubtype::TS_OTHER: return "TS_OTHER";
        case TooSmallSubtype::COUNT: return "COUNT";
    }
    return "TS_OTHER";
}

const char *tooSmallOtherSubtypeName(TooSmallOtherSubtype subtype) {
    switch (subtype) {
        case TooSmallOtherSubtype::TSO_LOOP_PLUS_EDGE_SHARED:
            return "TSO_LOOP_PLUS_EDGE_SHARED";
        case TooSmallOtherSubtype::TSO_LOOP_PLUS_EDGE_DISJOINT:
            return "TSO_LOOP_PLUS_EDGE_DISJOINT";
        case TooSmallOtherSubtype::TSO_TWO_LOOPS_SAME_VERTEX:
            return "TSO_TWO_LOOPS_SAME_VERTEX";
        case TooSmallOtherSubtype::TSO_TWO_LOOPS_DIFF_VERTEX:
            return "TSO_TWO_LOOPS_DIFF_VERTEX";
        case TooSmallOtherSubtype::TSO_TWO_NONLOOP_UNCLASSIFIED:
            return "TSO_TWO_NONLOOP_UNCLASSIFIED";
        case TooSmallOtherSubtype::TSO_KIND_MIXED_ANOMALY:
            return "TSO_KIND_MIXED_ANOMALY";
        case TooSmallOtherSubtype::TSO_OTHER: return "TSO_OTHER";
        case TooSmallOtherSubtype::COUNT: return "COUNT";
    }
    return "TSO_OTHER";
}

const char *sequenceOneEdgeSubtypeName(SequenceOneEdgeSubtype subtype) {
    switch (subtype) {
        case SequenceOneEdgeSubtype::SOE_REAL_NONLOOP: return "SOE_REAL_NONLOOP";
        case SequenceOneEdgeSubtype::SOE_REAL_LOOP: return "SOE_REAL_LOOP";
        case SequenceOneEdgeSubtype::SOE_PROXY_NONLOOP: return "SOE_PROXY_NONLOOP";
        case SequenceOneEdgeSubtype::SOE_PROXY_LOOP: return "SOE_PROXY_LOOP";
        case SequenceOneEdgeSubtype::SOE_OTHER: return "SOE_OTHER";
        case SequenceOneEdgeSubtype::COUNT: return "COUNT";
    }
    return "SOE_OTHER";
}

const char *seqFallbackReasonName(SeqFallbackReason reason) {
    switch (reason) {
        case SeqFallbackReason::PROXY_METADATA: return "PROXY_METADATA";
        case SeqFallbackReason::GRAFT_REWIRE: return "GRAFT_REWIRE";
        case SeqFallbackReason::OTHER: return "OTHER";
        case SeqFallbackReason::COUNT: return "COUNT";
    }
    return "OTHER";
}

const char *rewriteFallbackTriggerName(RewriteFallbackTrigger trigger) {
    switch (trigger) {
        case RewriteFallbackTrigger::RFT_NONE: return "RFT_NONE";
        case RewriteFallbackTrigger::RFT_COMPACT_BUILD_FAIL: return "RFT_COMPACT_BUILD_FAIL";
        case RewriteFallbackTrigger::RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED:
            return "RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED";
        case RewriteFallbackTrigger::RFT_COMPACT_EMPTY_AFTER_DELETE:
            return "RFT_COMPACT_EMPTY_AFTER_DELETE";
        case RewriteFallbackTrigger::RFT_COMPACT_NOT_BICONNECTED_UNHANDLED:
            return "RFT_COMPACT_NOT_BICONNECTED_UNHANDLED";
        case RewriteFallbackTrigger::RFT_COMPACT_TOO_SMALL_UNHANDLED:
            return "RFT_COMPACT_TOO_SMALL_UNHANDLED";
        case RewriteFallbackTrigger::RFT_SINGLE_CUT_BUILDER_FAIL:
            return "RFT_SINGLE_CUT_BUILDER_FAIL";
        case RewriteFallbackTrigger::RFT_PATH_OF_BLOCKS_BUILDER_FAIL:
            return "RFT_PATH_OF_BLOCKS_BUILDER_FAIL";
        case RewriteFallbackTrigger::RFT_TWO_PATH_BUILDER_FAIL:
            return "RFT_TWO_PATH_BUILDER_FAIL";
        case RewriteFallbackTrigger::RFT_BACKEND_BUILDRAW_S_LT3:
            return "RFT_BACKEND_BUILDRAW_S_LT3";
        case RewriteFallbackTrigger::RFT_BACKEND_BUILDRAW_NOT_BICONNECTED:
            return "RFT_BACKEND_BUILDRAW_NOT_BICONNECTED";
        case RewriteFallbackTrigger::RFT_BACKEND_BUILDRAW_OTHER:
            return "RFT_BACKEND_BUILDRAW_OTHER";
        case RewriteFallbackTrigger::RFT_RAW_VALIDATE_FAIL:
            return "RFT_RAW_VALIDATE_FAIL";
        case RewriteFallbackTrigger::RFT_MATERIALIZE_MINI_FAIL:
            return "RFT_MATERIALIZE_MINI_FAIL";
        case RewriteFallbackTrigger::RFT_CHOOSE_KEEP_FAIL:
            return "RFT_CHOOSE_KEEP_FAIL";
        case RewriteFallbackTrigger::RFT_PROXY_METADATA_INVALID:
            return "RFT_PROXY_METADATA_INVALID";
        case RewriteFallbackTrigger::RFT_GRAFT_REWIRE_FAIL:
            return "RFT_GRAFT_REWIRE_FAIL";
        case RewriteFallbackTrigger::RFT_GRAFT_OTHER: return "RFT_GRAFT_OTHER";
        case RewriteFallbackTrigger::RFT_OTHER: return "RFT_OTHER";
        case RewriteFallbackTrigger::COUNT: return "COUNT";
    }
    return "RFT_OTHER";
}

const char *compactBuildFailSubtypeName(CompactBuildFailSubtype subtype) {
    switch (subtype) {
        case CompactBuildFailSubtype::CBF_NODE_DEAD: return "CBF_NODE_DEAD";
        case CompactBuildFailSubtype::CBF_NODE_NOT_R: return "CBF_NODE_NOT_R";
        case CompactBuildFailSubtype::CBF_X_NOT_PRESENT_IN_R: return "CBF_X_NOT_PRESENT_IN_R";
        case CompactBuildFailSubtype::CBF_REAL_ENDPOINT_NOT_MAPPED:
            return "CBF_REAL_ENDPOINT_NOT_MAPPED";
        case CompactBuildFailSubtype::CBF_PROXY_OLDARC_INVALID:
            return "CBF_PROXY_OLDARC_INVALID";
        case CompactBuildFailSubtype::CBF_PROXY_OUTSIDENODE_INVALID:
            return "CBF_PROXY_OUTSIDENODE_INVALID";
        case CompactBuildFailSubtype::CBF_PROXY_OLDSLOT_INVALID:
            return "CBF_PROXY_OLDSLOT_INVALID";
        case CompactBuildFailSubtype::CBF_PROXY_OLDSLOT_ARC_MISMATCH:
            return "CBF_PROXY_OLDSLOT_ARC_MISMATCH";
        case CompactBuildFailSubtype::CBF_DUPLICATE_COMPACT_EDGE:
            return "CBF_DUPLICATE_COMPACT_EDGE";
        case CompactBuildFailSubtype::CBF_EMPTY_VERTEX_SET:
            return "CBF_EMPTY_VERTEX_SET";
        case CompactBuildFailSubtype::CBF_SELF_LOOP_PRECHECK:
            return "CBF_SELF_LOOP_PRECHECK";
        case CompactBuildFailSubtype::CBF_OTHER:
        case CompactBuildFailSubtype::COUNT:
            return "CBF_OTHER";
    }
    return "CBF_OTHER";
}

const char *selfLoopBuildFailSubtypeName(SelfLoopBuildFailSubtype subtype) {
    switch (subtype) {
        case SelfLoopBuildFailSubtype::SL_REAL_LOOP_PRESENT:
            return "SL_REAL_LOOP_PRESENT";
        case SelfLoopBuildFailSubtype::SL_MIXED_REAL_PROXY_LOOP:
            return "SL_MIXED_REAL_PROXY_LOOP";
        case SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_EMPTY:
            return "SL_PROXY_ONLY_REMAINDER_EMPTY";
        case SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_SPQR_READY:
            return "SL_PROXY_ONLY_REMAINDER_SPQR_READY";
        case SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_TWO_PATH:
            return "SL_PROXY_ONLY_REMAINDER_TWO_PATH";
        case SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_SINGLE_CUT:
            return "SL_PROXY_ONLY_REMAINDER_SINGLE_CUT";
        case SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_PATH_OF_BLOCKS:
            return "SL_PROXY_ONLY_REMAINDER_PATH_OF_BLOCKS";
        case SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_OTHER_NOT_BICONNECTED:
            return "SL_PROXY_ONLY_REMAINDER_OTHER_NOT_BICONNECTED";
        case SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_OTHER:
            return "SL_PROXY_ONLY_REMAINDER_OTHER";
        case SelfLoopBuildFailSubtype::SL_OTHER:
        case SelfLoopBuildFailSubtype::COUNT:
            return "SL_OTHER";
    }
    return "SL_OTHER";
}

const char *selfLoopRemainderOtherNBSubtypeName(SelfLoopRemainderOtherNBSubtype subtype) {
    switch (subtype) {
        case SelfLoopRemainderOtherNBSubtype::SLNB_DISCONNECTED:
            return "SLNB_DISCONNECTED";
        case SelfLoopRemainderOtherNBSubtype::SLNB_STAR_AROUND_ONE_CUT:
            return "SLNB_STAR_AROUND_ONE_CUT";
        case SelfLoopRemainderOtherNBSubtype::SLNB_COMPLEX_MULTI_CUT:
            return "SLNB_COMPLEX_MULTI_CUT";
        case SelfLoopRemainderOtherNBSubtype::SLNB_BLOCKS_ALL_TINY:
            return "SLNB_BLOCKS_ALL_TINY";
        case SelfLoopRemainderOtherNBSubtype::SLNB_OTHER:
        case SelfLoopRemainderOtherNBSubtype::COUNT:
            return "SLNB_OTHER";
    }
    return "SLNB_OTHER";
}

const char *xIncidentVirtualSubtypeName(XIncidentVirtualSubtype subtype) {
    switch (subtype) {
        case XIncidentVirtualSubtype::XIV_ZERO_PAYLOAD: return "XIV_ZERO_PAYLOAD";
        case XIncidentVirtualSubtype::XIV_ONE_POS_PROXY: return "XIV_ONE_POS_PROXY";
        case XIncidentVirtualSubtype::XIV_MULTI_POS_PROXY: return "XIV_MULTI_POS_PROXY";
        case XIncidentVirtualSubtype::XIV_SHARED_WITH_LOOP: return "XIV_SHARED_WITH_LOOP";
        case XIncidentVirtualSubtype::XIV_MIXED_OTHER:
        case XIncidentVirtualSubtype::COUNT:
            return "XIV_MIXED_OTHER";
    }
    return "XIV_MIXED_OTHER";
}

const char *xSharedResidualSubtypeName(XSharedResidualSubtype subtype) {
    switch (subtype) {
        case XSharedResidualSubtype::XSR_HAFTER_BUILD_FAIL:
            return "XSR_HAFTER_BUILD_FAIL";
        case XSharedResidualSubtype::XSR_HAFTER_EMPTY:
            return "XSR_HAFTER_EMPTY";
        case XSharedResidualSubtype::XSR_HAFTER_ONE_EDGE:
            return "XSR_HAFTER_ONE_EDGE";
        case XSharedResidualSubtype::XSR_HAFTER_TWO_PARALLEL:
            return "XSR_HAFTER_TWO_PARALLEL";
        case XSharedResidualSubtype::XSR_HAFTER_TWO_PATH:
            return "XSR_HAFTER_TWO_PATH";
        case XSharedResidualSubtype::XSR_HAFTER_LOOP_SHARED:
            return "XSR_HAFTER_LOOP_SHARED";
        case XSharedResidualSubtype::XSR_HAFTER_SPQR_READY:
            return "XSR_HAFTER_SPQR_READY";
        case XSharedResidualSubtype::XSR_HAFTER_SINGLE_CUT:
            return "XSR_HAFTER_SINGLE_CUT";
        case XSharedResidualSubtype::XSR_HAFTER_PATH_OF_BLOCKS:
            return "XSR_HAFTER_PATH_OF_BLOCKS";
        case XSharedResidualSubtype::XSR_HAFTER_OTHER_NOT_BICONNECTED:
            return "XSR_HAFTER_OTHER_NOT_BICONNECTED";
        case XSharedResidualSubtype::XSR_HAFTER_OTHER:
        case XSharedResidualSubtype::COUNT:
            return "XSR_HAFTER_OTHER";
    }
    return "XSR_HAFTER_OTHER";
}

const char *xSharedLoopSharedInputSubtypeName(XSharedLoopSharedInputSubtype subtype) {
    switch (subtype) {
        case XSharedLoopSharedInputSubtype::XLSI_REAL_LOOP_REAL_EDGE:
            return "XLSI_REAL_LOOP_REAL_EDGE";
        case XSharedLoopSharedInputSubtype::XLSI_REAL_LOOP_PROXY_EDGE:
            return "XLSI_REAL_LOOP_PROXY_EDGE";
        case XSharedLoopSharedInputSubtype::XLSI_PROXY_LOOP_REAL_EDGE:
            return "XLSI_PROXY_LOOP_REAL_EDGE";
        case XSharedLoopSharedInputSubtype::XLSI_PROXY_LOOP_PROXY_EDGE:
            return "XLSI_PROXY_LOOP_PROXY_EDGE";
        case XSharedLoopSharedInputSubtype::XLSI_OTHER:
        case XSharedLoopSharedInputSubtype::COUNT:
            return "XLSI_OTHER";
    }
    return "XLSI_OTHER";
}

const char *xSharedLoopSharedBailoutName(XSharedLoopSharedBailout bailout) {
    switch (bailout) {
        case XSharedLoopSharedBailout::XLSB_BUILDER_FAIL:
            return "XLSB_BUILDER_FAIL";
        case XSharedLoopSharedBailout::XLSB_CHOOSE_KEEP_FAIL:
            return "XLSB_CHOOSE_KEEP_FAIL";
        case XSharedLoopSharedBailout::XLSB_GRAFT_FAIL:
            return "XLSB_GRAFT_FAIL";
        case XSharedLoopSharedBailout::XLSB_METADATA_REFRESH_FAIL:
            return "XLSB_METADATA_REFRESH_FAIL";
        case XSharedLoopSharedBailout::XLSB_POSTCHECK_ADJ_ONLY:
            return "XLSB_POSTCHECK_ADJ_ONLY";
        case XSharedLoopSharedBailout::XLSB_POSTCHECK_SAME_TYPE_SP_ONLY:
            return "XLSB_POSTCHECK_SAME_TYPE_SP_ONLY";
        case XSharedLoopSharedBailout::XLSB_POSTCHECK_MIXED:
            return "XLSB_POSTCHECK_MIXED";
        case XSharedLoopSharedBailout::XLSB_ORACLE_FAIL:
            return "XLSB_ORACLE_FAIL";
        case XSharedLoopSharedBailout::XLSB_OTHER:
        case XSharedLoopSharedBailout::COUNT:
            return "XLSB_OTHER";
    }
    return "XLSB_OTHER";
}

const char *xSharedBridgeBailoutName(XSharedBridgeBailout bailout) {
    switch (bailout) {
        case XSharedBridgeBailout::XSB_NONE: return "XSB_NONE";
        case XSharedBridgeBailout::XSB_HAFTER_BUILD_FAIL:
            return "XSB_HAFTER_BUILD_FAIL";
        case XSharedBridgeBailout::XSB_UNSUPPORTED_HAFTER_SUBTYPE:
            return "XSB_UNSUPPORTED_HAFTER_SUBTYPE";
        case XSharedBridgeBailout::XSB_LOOP_SHARED_BUILDER_FAIL:
            return "XSB_LOOP_SHARED_BUILDER_FAIL";
        case XSharedBridgeBailout::XSB_TWO_PATH_BUILDER_FAIL:
            return "XSB_TWO_PATH_BUILDER_FAIL";
        case XSharedBridgeBailout::XSB_CHOOSE_KEEP_FAIL:
            return "XSB_CHOOSE_KEEP_FAIL";
        case XSharedBridgeBailout::XSB_GRAFT_FAIL: return "XSB_GRAFT_FAIL";
        case XSharedBridgeBailout::XSB_METADATA_REFRESH_FAIL:
            return "XSB_METADATA_REFRESH_FAIL";
        case XSharedBridgeBailout::XSB_OTHER:
        case XSharedBridgeBailout::COUNT:
            return "XSB_OTHER";
    }
    return "XSB_OTHER";
}

const char *graftRewireBailoutSubtypeName(GraftRewireBailoutSubtype subtype) {
    switch (subtype) {
        case GraftRewireBailoutSubtype::GRB_OWNER_MINI_MISSING:
            return "GRB_OWNER_MINI_MISSING";
        case GraftRewireBailoutSubtype::GRB_OWNER_MINI_SLOT_INVALID:
            return "GRB_OWNER_MINI_SLOT_INVALID";
        case GraftRewireBailoutSubtype::GRB_OWNER_SLOT_NOT_PROXY:
            return "GRB_OWNER_SLOT_NOT_PROXY";
        case GraftRewireBailoutSubtype::GRB_ACTUAL_OF_MINI_MISSING:
            return "GRB_ACTUAL_OF_MINI_MISSING";
        case GraftRewireBailoutSubtype::GRB_ACTUAL_SLOT_MISSING:
            return "GRB_ACTUAL_SLOT_MISSING";
        case GraftRewireBailoutSubtype::GRB_OLDARC_OUT_OF_RANGE:
            return "GRB_OLDARC_OUT_OF_RANGE";
        case GraftRewireBailoutSubtype::GRB_OLDARC_DEAD:
            return "GRB_OLDARC_DEAD";
        case GraftRewireBailoutSubtype::GRB_OLDARC_NOT_INCIDENT_TO_OLDNODE:
            return "GRB_OLDARC_NOT_INCIDENT_TO_OLDNODE";
        case GraftRewireBailoutSubtype::GRB_OUTSIDENODE_MISMATCH:
            return "GRB_OUTSIDENODE_MISMATCH";
        case GraftRewireBailoutSubtype::GRB_OLDSLOT_INVALID:
            return "GRB_OLDSLOT_INVALID";
        case GraftRewireBailoutSubtype::GRB_OLDSLOT_NOT_VIRTUAL:
            return "GRB_OLDSLOT_NOT_VIRTUAL";
        case GraftRewireBailoutSubtype::GRB_OLDSLOT_ARCID_MISMATCH:
            return "GRB_OLDSLOT_ARCID_MISMATCH";
        case GraftRewireBailoutSubtype::GRB_DUPLICATE_OLDARC:
            return "GRB_DUPLICATE_OLDARC";
        case GraftRewireBailoutSubtype::GRB_REWIRE_RET_FALSE:
            return "GRB_REWIRE_RET_FALSE";
        case GraftRewireBailoutSubtype::GRB_OTHER:
        case GraftRewireBailoutSubtype::COUNT:
            return "GRB_OTHER";
    }
    return "GRB_OTHER";
}

const char *graftOtherSubtypeName(GraftOtherSubtype subtype) {
    switch (subtype) {
        case GraftOtherSubtype::GOS_PRESERVED_SNAPSHOT_EMPTY:
            return "GOS_PRESERVED_SNAPSHOT_EMPTY";
        case GraftOtherSubtype::GOS_PRESERVED_DUPLICATE_SLOT:
            return "GOS_PRESERVED_DUPLICATE_SLOT";
        case GraftOtherSubtype::GOS_PRESERVED_SLOT_OUT_OF_RANGE:
            return "GOS_PRESERVED_SLOT_OUT_OF_RANGE";
        case GraftOtherSubtype::GOS_PRESERVED_SLOT_DEAD:
            return "GOS_PRESERVED_SLOT_DEAD";
        case GraftOtherSubtype::GOS_PRESERVED_SLOT_NOT_VIRTUAL:
            return "GOS_PRESERVED_SLOT_NOT_VIRTUAL";
        case GraftOtherSubtype::GOS_PRESERVED_SLOT_ARCID_MISMATCH:
            return "GOS_PRESERVED_SLOT_ARCID_MISMATCH";
        case GraftOtherSubtype::GOS_REHOME_NEWSLOT_INVALID:
            return "GOS_REHOME_NEWSLOT_INVALID";
        case GraftOtherSubtype::GOS_REHOME_OLDARC_DEAD:
            return "GOS_REHOME_OLDARC_DEAD";
        case GraftOtherSubtype::GOS_REHOME_OLDNODE_NOT_INCIDENT:
            return "GOS_REHOME_OLDNODE_NOT_INCIDENT";
        case GraftOtherSubtype::GOS_REHOME_NEWSLOT_NOT_VIRTUAL:
            return "GOS_REHOME_NEWSLOT_NOT_VIRTUAL";
        case GraftOtherSubtype::GOS_REHOME_ARC_SLOT_UPDATE_FAIL:
            return "GOS_REHOME_ARC_SLOT_UPDATE_FAIL";
        case GraftOtherSubtype::GOS_POSTCHECK_STALE_PRESERVED_SLOT:
            return "GOS_POSTCHECK_STALE_PRESERVED_SLOT";
        case GraftOtherSubtype::GOS_POSTCHECK_PRESERVED_ARC_DEAD:
            return "GOS_POSTCHECK_PRESERVED_ARC_DEAD";
        case GraftOtherSubtype::GOS_POSTCHECK_ADJ_MISMATCH:
            return "GOS_POSTCHECK_ADJ_MISMATCH";
        case GraftOtherSubtype::GOS_POSTCHECK_OUTSIDE_MISMATCH:
            return "GOS_POSTCHECK_OUTSIDE_MISMATCH";
        case GraftOtherSubtype::GOS_OTHER:
        case GraftOtherSubtype::COUNT:
            return "GOS_OTHER";
    }
    return "GOS_OTHER";
}

const char *graftPostcheckSubtypeName(GraftPostcheckSubtype subtype) {
    switch (subtype) {
        case GraftPostcheckSubtype::GPS_ADJ_METADATA_ONLY:
            return "GPS_ADJ_METADATA_ONLY";
        case GraftPostcheckSubtype::GPS_SAME_TYPE_SP_ONLY:
            return "GPS_SAME_TYPE_SP_ONLY";
        case GraftPostcheckSubtype::GPS_ADJ_AND_SAME_TYPE_SP:
            return "GPS_ADJ_AND_SAME_TYPE_SP";
        case GraftPostcheckSubtype::GPS_OTHER:
        case GraftPostcheckSubtype::COUNT:
            return "GPS_OTHER";
    }
    return "GPS_OTHER";
}

const char *replaySnapshotPhaseName(ReplaySnapshotPhase phase) {
    switch (phase) {
        case ReplaySnapshotPhase::BEFORE_CLEAR:
            return "BEFORE_CLEAR";
        case ReplaySnapshotPhase::AFTER_CLEAR_PRESERVE:
            return "AFTER_CLEAR_PRESERVE";
        case ReplaySnapshotPhase::AFTER_MATERIALIZE:
            return "AFTER_MATERIALIZE";
        case ReplaySnapshotPhase::AFTER_INTERNAL_ARC_CONNECT:
            return "AFTER_INTERNAL_ARC_CONNECT";
        case ReplaySnapshotPhase::AFTER_PROXY_REWIRE:
            return "AFTER_PROXY_REWIRE";
        case ReplaySnapshotPhase::AFTER_ADJ_REPAIR:
            return "AFTER_ADJ_REPAIR";
        case ReplaySnapshotPhase::BEFORE_SP_CLEANUP:
            return "BEFORE_SP_CLEANUP";
        case ReplaySnapshotPhase::AFTER_SP_CLEANUP:
            return "AFTER_SP_CLEANUP";
        case ReplaySnapshotPhase::AFTER_NORMALIZE:
            return "AFTER_NORMALIZE";
        case ReplaySnapshotPhase::COUNT:
            return "AFTER_NORMALIZE";
    }
    return "AFTER_NORMALIZE";
}

const char *proxyArcRepairOutcomeName(ProxyArcRepairOutcome outcome) {
    switch (outcome) {
        case ProxyArcRepairOutcome::PAR_OLDARC_ALREADY_LIVE:
            return "PAR_OLDARC_ALREADY_LIVE";
        case ProxyArcRepairOutcome::PAR_MATCH_BY_OUTSIDENODE_AND_POLES:
            return "PAR_MATCH_BY_OUTSIDENODE_AND_POLES";
        case ProxyArcRepairOutcome::PAR_MATCH_BY_POLES_ONLY_UNIQUE:
            return "PAR_MATCH_BY_POLES_ONLY_UNIQUE";
        case ProxyArcRepairOutcome::PAR_FAIL_OUTSIDENODE_DEAD:
            return "PAR_FAIL_OUTSIDENODE_DEAD";
        case ProxyArcRepairOutcome::PAR_FAIL_NO_CANDIDATE:
            return "PAR_FAIL_NO_CANDIDATE";
        case ProxyArcRepairOutcome::PAR_FAIL_MULTI_CANDIDATE:
            return "PAR_FAIL_MULTI_CANDIDATE";
        case ProxyArcRepairOutcome::PAR_FAIL_SLOT_NOT_VIRTUAL:
            return "PAR_FAIL_SLOT_NOT_VIRTUAL";
        case ProxyArcRepairOutcome::PAR_FAIL_SLOT_ARCID_MISMATCH:
            return "PAR_FAIL_SLOT_ARCID_MISMATCH";
        case ProxyArcRepairOutcome::PAR_FAIL_POLES_ONLY_MULTI_CANDIDATE:
            return "PAR_FAIL_POLES_ONLY_MULTI_CANDIDATE";
        case ProxyArcRepairOutcome::PAR_FAIL_POLES_ONLY_SLOT_INVALID:
            return "PAR_FAIL_POLES_ONLY_SLOT_INVALID";
        case ProxyArcRepairOutcome::PAR_FAIL_POLES_ONLY_OTHER:
            return "PAR_FAIL_POLES_ONLY_OTHER";
        case ProxyArcRepairOutcome::PAR_OTHER:
        case ProxyArcRepairOutcome::COUNT:
            return "PAR_OTHER";
    }
    return "PAR_OTHER";
}

const char *proxyArcNoCandidateSubtypeName(ProxyArcNoCandidateSubtype subtype) {
    switch (subtype) {
        case ProxyArcNoCandidateSubtype::PNC_OLDNODE_DEAD:
            return "PNC_OLDNODE_DEAD";
        case ProxyArcNoCandidateSubtype::PNC_OUTSIDENODE_DEAD:
            return "PNC_OUTSIDENODE_DEAD";
        case ProxyArcNoCandidateSubtype::PNC_OLDNODE_NO_LIVE_ARCS:
            return "PNC_OLDNODE_NO_LIVE_ARCS";
        case ProxyArcNoCandidateSubtype::PNC_NO_ARC_TO_OUTSIDENODE:
            return "PNC_NO_ARC_TO_OUTSIDENODE";
        case ProxyArcNoCandidateSubtype::PNC_TO_OUTSIDENODE_BUT_WRONG_POLES:
            return "PNC_TO_OUTSIDENODE_BUT_WRONG_POLES";
        case ProxyArcNoCandidateSubtype::PNC_SAME_POLES_BUT_OTHER_OUTSIDE:
            return "PNC_SAME_POLES_BUT_OTHER_OUTSIDE";
        case ProxyArcNoCandidateSubtype::PNC_CANDIDATE_SLOT_NOT_VIRTUAL:
            return "PNC_CANDIDATE_SLOT_NOT_VIRTUAL";
        case ProxyArcNoCandidateSubtype::PNC_CANDIDATE_SLOT_ARCID_MISMATCH:
            return "PNC_CANDIDATE_SLOT_ARCID_MISMATCH";
        case ProxyArcNoCandidateSubtype::PNC_MULTI_WEAK_CANDIDATES:
            return "PNC_MULTI_WEAK_CANDIDATES";
        case ProxyArcNoCandidateSubtype::PNC_OTHER:
        case ProxyArcNoCandidateSubtype::COUNT:
            return "PNC_OTHER";
    }
    return "PNC_OTHER";
}

const char *proxyArcLifecyclePhaseName(ProxyArcLifecyclePhase phase) {
    switch (phase) {
        case ProxyArcLifecyclePhase::PAL_SNAPSHOT_OK:
            return "PAL_SNAPSHOT_OK";
        case ProxyArcLifecyclePhase::PAL_AFTER_CLEAR_KEEP_ALIVE:
            return "PAL_AFTER_CLEAR_KEEP_ALIVE";
        case ProxyArcLifecyclePhase::PAL_AFTER_CLEAR_KEEP_DEAD:
            return "PAL_AFTER_CLEAR_KEEP_DEAD";
        case ProxyArcLifecyclePhase::PAL_AFTER_CLEAR_KEEP_NOT_INCIDENT:
            return "PAL_AFTER_CLEAR_KEEP_NOT_INCIDENT";
        case ProxyArcLifecyclePhase::PAL_AFTER_CLEAR_KEEP_SLOT_INVALID:
            return "PAL_AFTER_CLEAR_KEEP_SLOT_INVALID";
        case ProxyArcLifecyclePhase::PAL_AFTER_MATERIALIZE_ALIVE:
            return "PAL_AFTER_MATERIALIZE_ALIVE";
        case ProxyArcLifecyclePhase::PAL_AFTER_MATERIALIZE_DEAD:
            return "PAL_AFTER_MATERIALIZE_DEAD";
        case ProxyArcLifecyclePhase::PAL_AFTER_MATERIALIZE_NOT_INCIDENT:
            return "PAL_AFTER_MATERIALIZE_NOT_INCIDENT";
        case ProxyArcLifecyclePhase::PAL_AFTER_MATERIALIZE_SLOT_INVALID:
            return "PAL_AFTER_MATERIALIZE_SLOT_INVALID";
        case ProxyArcLifecyclePhase::PAL_AFTER_INTERNAL_ARCS_ALIVE:
            return "PAL_AFTER_INTERNAL_ARCS_ALIVE";
        case ProxyArcLifecyclePhase::PAL_AFTER_INTERNAL_ARCS_DEAD:
            return "PAL_AFTER_INTERNAL_ARCS_DEAD";
        case ProxyArcLifecyclePhase::PAL_AFTER_INTERNAL_ARCS_NOT_INCIDENT:
            return "PAL_AFTER_INTERNAL_ARCS_NOT_INCIDENT";
        case ProxyArcLifecyclePhase::PAL_AFTER_INTERNAL_ARCS_SLOT_INVALID:
            return "PAL_AFTER_INTERNAL_ARCS_SLOT_INVALID";
        case ProxyArcLifecyclePhase::PAL_PRE_REWIRE_ALIVE:
            return "PAL_PRE_REWIRE_ALIVE";
        case ProxyArcLifecyclePhase::PAL_PRE_REWIRE_DEAD:
            return "PAL_PRE_REWIRE_DEAD";
        case ProxyArcLifecyclePhase::PAL_PRE_REWIRE_NOT_INCIDENT:
            return "PAL_PRE_REWIRE_NOT_INCIDENT";
        case ProxyArcLifecyclePhase::PAL_PRE_REWIRE_SLOT_INVALID:
            return "PAL_PRE_REWIRE_SLOT_INVALID";
        case ProxyArcLifecyclePhase::PAL_REWIRE_RET_FALSE:
            return "PAL_REWIRE_RET_FALSE";
        case ProxyArcLifecyclePhase::PAL_OTHER:
        case ProxyArcLifecyclePhase::COUNT:
            return "PAL_OTHER";
    }
    return "PAL_OTHER";
}

const char *weakRepairGateSubtypeName(WeakRepairGateSubtype subtype) {
    switch (subtype) {
        case WeakRepairGateSubtype::WRG_NOT_NEEDED_STRONG_LIVE:
            return "WRG_NOT_NEEDED_STRONG_LIVE";
        case WeakRepairGateSubtype::WRG_ENTER_PNC_SAME_POLES_BUT_OTHER_OUTSIDE:
            return "WRG_ENTER_PNC_SAME_POLES_BUT_OTHER_OUTSIDE";
        case WeakRepairGateSubtype::WRG_SKIP_PNC_OLDNODE_NO_LIVE_ARCS:
            return "WRG_SKIP_PNC_OLDNODE_NO_LIVE_ARCS";
        case WeakRepairGateSubtype::WRG_SKIP_OTHER_PNC:
            return "WRG_SKIP_OTHER_PNC";
        case WeakRepairGateSubtype::WRG_OTHER:
        case WeakRepairGateSubtype::COUNT:
            return "WRG_OTHER";
    }
    return "WRG_OTHER";
}

const char *weakRepairCandidateSubtypeName(WeakRepairCandidateSubtype subtype) {
    switch (subtype) {
        case WeakRepairCandidateSubtype::WRC_ZERO_SAME_POLE_CANDIDATES:
            return "WRC_ZERO_SAME_POLE_CANDIDATES";
        case WeakRepairCandidateSubtype::WRC_ONE_SAME_POLE_CANDIDATE:
            return "WRC_ONE_SAME_POLE_CANDIDATE";
        case WeakRepairCandidateSubtype::WRC_MULTI_SAME_POLE_CANDIDATES:
            return "WRC_MULTI_SAME_POLE_CANDIDATES";
        case WeakRepairCandidateSubtype::WRC_SLOT_INVALID:
            return "WRC_SLOT_INVALID";
        case WeakRepairCandidateSubtype::WRC_OTHER:
        case WeakRepairCandidateSubtype::COUNT:
            return "WRC_OTHER";
    }
    return "WRC_OTHER";
}

const char *weakRepairCommitOutcomeName(WeakRepairCommitOutcome outcome) {
    switch (outcome) {
        case WeakRepairCommitOutcome::WCO_NOT_ATTEMPTED:
            return "WCO_NOT_ATTEMPTED";
        case WeakRepairCommitOutcome::WCO_FAILED_BEFORE_GRAFT:
            return "WCO_FAILED_BEFORE_GRAFT";
        case WeakRepairCommitOutcome::WCO_GRAFT_FAIL:
            return "WCO_GRAFT_FAIL";
        case WeakRepairCommitOutcome::WCO_NORMALIZE_FAIL:
            return "WCO_NORMALIZE_FAIL";
        case WeakRepairCommitOutcome::WCO_ACTUAL_INVARIANT_FAIL:
            return "WCO_ACTUAL_INVARIANT_FAIL";
        case WeakRepairCommitOutcome::WCO_ORACLE_FAIL:
            return "WCO_ORACLE_FAIL";
        case WeakRepairCommitOutcome::WCO_COMMITTED:
            return "WCO_COMMITTED";
        case WeakRepairCommitOutcome::COUNT:
            return "COUNT";
    }
    return "WCO_NOT_ATTEMPTED";
}

const char *rewritePathTakenName(RewritePathTaken path) {
    switch (path) {
        case RewritePathTaken::DIRECT_SPQR: return "DIRECT_SPQR";
        case RewritePathTaken::SPECIAL_SINGLE_CUT: return "SPECIAL_SINGLE_CUT";
        case RewritePathTaken::SPECIAL_ONE_EDGE: return "SPECIAL_ONE_EDGE";
        case RewritePathTaken::SPECIAL_TWO_PATH: return "SPECIAL_TWO_PATH";
        case RewritePathTaken::SPECIAL_PATH: return "SPECIAL_PATH";
        case RewritePathTaken::SPECIAL_LOOP_SHARED: return "SPECIAL_LOOP_SHARED";
        case RewritePathTaken::SPECIAL_SELF_LOOP_TWO_PATH:
            return "SPECIAL_SELF_LOOP_TWO_PATH";
        case RewritePathTaken::SPECIAL_SELF_LOOP_SPQR_READY:
            return "SPECIAL_SELF_LOOP_SPQR_READY";
        case RewritePathTaken::SPECIAL_SELF_LOOP_ONE_EDGE:
            return "SPECIAL_SELF_LOOP_ONE_EDGE";
        case RewritePathTaken::WHOLE_CORE_REBUILD: return "WHOLE_CORE_REBUILD";
        case RewritePathTaken::COUNT: return "COUNT";
    }
    return "WHOLE_CORE_REBUILD";
}

bool buildProjectRawSnapshot(const CompactGraph &H,
                             const RawSpqrDecomp &raw,
                             ProjectRawSnapshot &out,
                             std::string &why) {
    (void)why;

    out = {};
    out.block = H.block;
    out.ownerR = H.ownerR;
    out.deletedX = H.deletedX;
    out.valid = raw.valid;
    out.error = raw.error;
    out.inputEdgeCount = static_cast<int>(H.edges.size());
    out.ownerOfInputEdge = raw.ownerOfInputEdge;

    out.nodes.reserve(raw.nodes.size());
    for (const auto &node : raw.nodes) {
        ProjectRawNode dst;
        dst.alive = node.alive;
        dst.type = node.type;
        dst.cycleSlots = node.cycleSlots;
        dst.pShape = node.pShape;
        dst.rShape = node.rShape;
        dst.slots.reserve(node.slots.size());
        for (const auto &slot : node.slots) {
            ProjectRawSlot dstSlot;
            dstSlot.alive = slot.alive;
            dstSlot.kind = slot.kind;
            dstSlot.inputEdgeId = slot.inputEdgeId;
            dstSlot.treeEdgeId = slot.treeEdgeId;
            dstSlot.poleA = slot.poleA;
            dstSlot.poleB = slot.poleB;
            dst.slots.push_back(dstSlot);
        }
        out.nodes.push_back(std::move(dst));
    }

    out.treeEdges.reserve(raw.treeEdges.size());
    for (const auto &edge : raw.treeEdges) {
        ProjectRawTreeEdge dst;
        dst.alive = edge.alive;
        dst.a = edge.a;
        dst.b = edge.b;
        dst.slotInA = edge.slotInA;
        dst.slotInB = edge.slotInB;
        dst.poleA = edge.poleA;
        dst.poleB = edge.poleB;
        out.treeEdges.push_back(dst);
    }

    return true;
}

bool checkProjectRawSnapshot(const ProjectRawSnapshot &snap,
                             std::string &why) {
    if (!snap.valid) {
        std::ostringstream oss;
        oss << "raw.valid is false (error=" << static_cast<int>(snap.error) << ")";
        return fail(why, oss.str());
    }
    if (snap.error != RawDecompError::NONE) {
        std::ostringstream oss;
        oss << "raw.error must be NONE when valid=1, got " << static_cast<int>(snap.error);
        return fail(why, oss.str());
    }
    if (snap.inputEdgeCount < 0) {
        return fail(why, "inputEdgeCount is negative");
    }
    if (static_cast<int>(snap.ownerOfInputEdge.size()) != snap.inputEdgeCount) {
        std::ostringstream oss;
        oss << "ownerOfInputEdge size mismatch: got "
            << snap.ownerOfInputEdge.size()
            << ", expected " << snap.inputEdgeCount;
        return fail(why, oss.str());
    }

    std::vector<int> inputSeen(snap.inputEdgeCount, 0);
    std::vector<int> treeSeen(snap.treeEdges.size(), 0);

    for (int nodeId = 0; nodeId < static_cast<int>(snap.nodes.size()); ++nodeId) {
        const auto &node = snap.nodes[nodeId];
        if (!node.alive) continue;

        int aliveSlotCount = 0;
        for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
            const auto &slot = node.slots[slotId];
            if (!slot.alive) continue;
            ++aliveSlotCount;

            if (slot.poleA < 0 || slot.poleB < 0) {
                return failSlot(why, nodeId, slotId, "negative pole endpoint");
            }

            if (slot.kind == RawSlotKind::INPUT_EDGE) {
                if (slot.inputEdgeId < 0 || slot.inputEdgeId >= snap.inputEdgeCount) {
                    return failSlot(why, nodeId, slotId, "inputEdgeId out of range");
                }
                const auto owner = snap.ownerOfInputEdge[slot.inputEdgeId];
                if (owner.first != nodeId || owner.second != slotId) {
                    std::ostringstream oss;
                    oss << "ownerOfInputEdge[" << slot.inputEdgeId << "] = ("
                        << owner.first << "," << owner.second
                        << "), expected (" << nodeId << "," << slotId << ")";
                    return failSlot(why, nodeId, slotId, oss.str());
                }
                ++inputSeen[slot.inputEdgeId];
            } else {
                if (!validTreeId(snap, slot.treeEdgeId)) {
                    return failSlot(why, nodeId, slotId, "treeEdgeId out of range");
                }
                if (!snap.treeEdges[slot.treeEdgeId].alive) {
                    return failSlot(why, nodeId, slotId, "references dead tree edge");
                }
                ++treeSeen[slot.treeEdgeId];
            }
        }

        if (aliveSlotCount == 0) {
            return failNode(why, nodeId, "alive node has no alive slots");
        }

        if (node.type == SPQRType::S_NODE) {
            if (node.pShape || node.rShape) {
                return failNode(why, nodeId, "S-node must not carry P/R shape");
            }
            if (aliveSlotCount < 3) {
                return failNode(why, nodeId, "S-node must have at least 3 alive slots");
            }
            if (static_cast<int>(node.cycleSlots.size()) != aliveSlotCount) {
                return failNode(why, nodeId, "cycleSlots size must match alive slot count");
            }

            std::vector<int> aliveSlots;
            std::vector<int> cycle = node.cycleSlots;
            aliveSlots.reserve(aliveSlotCount);
            for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
                if (node.slots[slotId].alive) aliveSlots.push_back(slotId);
            }
            for (int slotId : cycle) {
                if (!validSlotId(node, slotId)) {
                    return failNode(why, nodeId, "cycleSlots contains out-of-range slot index");
                }
                if (!node.slots[slotId].alive) {
                    return failNode(why, nodeId, "cycleSlots references dead slot");
                }
            }
            std::sort(aliveSlots.begin(), aliveSlots.end());
            std::sort(cycle.begin(), cycle.end());
            if (std::adjacent_find(cycle.begin(), cycle.end()) != cycle.end()) {
                return failNode(why, nodeId, "cycleSlots contains duplicates");
            }
            if (aliveSlots != cycle) {
                return failNode(why, nodeId, "cycleSlots must cover exactly the alive slots");
            }
        } else if (node.type == SPQRType::P_NODE) {
            if (!node.pShape || node.rShape) {
                return failNode(why, nodeId, "P-node must carry only pShape");
            }
            if (!node.cycleSlots.empty()) {
                return failNode(why, nodeId, "P-node must not carry cycleSlots");
            }
            const auto poles = canonPole(node.pShape->poleA, node.pShape->poleB);
            for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
                const auto &slot = node.slots[slotId];
                if (!slot.alive) continue;
                if (canonPole(slot.poleA, slot.poleB) != poles) {
                    return failSlot(why, nodeId, slotId, "slot poles do not match pShape poles");
                }
            }
        } else {
            if (!node.rShape || node.pShape) {
                return failNode(why, nodeId, "R-node must carry only rShape");
            }
            if (!node.cycleSlots.empty()) {
                return failNode(why, nodeId, "R-node must not carry cycleSlots");
            }

            const auto &shape = *node.rShape;
            if (shape.skelVertices.empty()) {
                return failNode(why, nodeId, "R-node must have skeleton vertices");
            }
            if (shape.incSlots.size() != shape.skelVertices.size()) {
                return failNode(why, nodeId, "R-node incSlots size mismatch");
            }
            if (shape.endsOfSlot.size() != node.slots.size()) {
                return failNode(why, nodeId, "R-node endsOfSlot size mismatch");
            }

            std::vector<int> incidences(node.slots.size(), 0);
            for (int v = 0; v < static_cast<int>(shape.incSlots.size()); ++v) {
                auto inc = shape.incSlots[v];
                std::sort(inc.begin(), inc.end());
                if (std::adjacent_find(inc.begin(), inc.end()) != inc.end()) {
                    return failNode(why, nodeId, "R-node incSlots contains duplicates");
                }
                for (int slotId : inc) {
                    if (!validSlotId(node, slotId)) {
                        return failNode(why, nodeId, "R-node incSlots contains out-of-range slot");
                    }
                    if (!node.slots[slotId].alive) {
                        return failNode(why, nodeId, "R-node incSlots references dead slot");
                    }
                    ++incidences[slotId];
                }
            }

            for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
                const auto &slot = node.slots[slotId];
                if (!slot.alive) continue;

                const auto ends = shape.endsOfSlot[slotId];
                if (ends.first < 0 || ends.first >= static_cast<int>(shape.skelVertices.size()) ||
                    ends.second < 0 || ends.second >= static_cast<int>(shape.skelVertices.size())) {
                    return failSlot(why, nodeId, slotId, "R-node endsOfSlot endpoint out of range");
                }
                if (ends.first == ends.second) {
                    return failSlot(why, nodeId, slotId, "R-node endsOfSlot must connect distinct vertices");
                }
                const VertexId a = shape.skelVertices[ends.first];
                const VertexId b = shape.skelVertices[ends.second];
                if (!samePoles(slot.poleA, slot.poleB, a, b)) {
                    return failSlot(why, nodeId, slotId, "slot poles do not match R-node endsOfSlot");
                }
                if (incidences[slotId] != 2) {
                    return failSlot(why, nodeId, slotId, "R-node slot must appear in exactly two incidence lists");
                }
            }
        }
    }

    for (int inputId = 0; inputId < snap.inputEdgeCount; ++inputId) {
        const auto owner = snap.ownerOfInputEdge[inputId];
        if (!validNodeId(snap, owner.first)) {
            return failInput(why, inputId, "owner node out of range");
        }
        const auto &node = snap.nodes[owner.first];
        if (!node.alive) {
            return failInput(why, inputId, "owner node is dead");
        }
        if (!validSlotId(node, owner.second)) {
            return failInput(why, inputId, "owner slot out of range");
        }
        const auto &slot = node.slots[owner.second];
        if (!slot.alive || slot.kind != RawSlotKind::INPUT_EDGE || slot.inputEdgeId != inputId) {
            return failInput(why, inputId, "ownerOfInputEdge points to a non-matching slot");
        }
        if (inputSeen[inputId] != 1) {
            std::ostringstream oss;
            oss << "input edge must be owned exactly once, saw " << inputSeen[inputId] << " owners";
            return failInput(why, inputId, oss.str());
        }
    }

    for (int treeId = 0; treeId < static_cast<int>(snap.treeEdges.size()); ++treeId) {
        const auto &edge = snap.treeEdges[treeId];
        if (!edge.alive) {
            if (treeSeen[treeId] != 0) {
                return failTree(why, treeId, "dead tree edge is still referenced by slots");
            }
            continue;
        }
        if (!validNodeId(snap, edge.a) || !validNodeId(snap, edge.b)) {
            return failTree(why, treeId, "endpoint node out of range");
        }
        const auto &nodeA = snap.nodes[edge.a];
        const auto &nodeB = snap.nodes[edge.b];
        if (!nodeA.alive || !nodeB.alive) {
            return failTree(why, treeId, "endpoint node is dead");
        }
        if (!validSlotId(nodeA, edge.slotInA) || !validSlotId(nodeB, edge.slotInB)) {
            return failTree(why, treeId, "slot endpoint out of range");
        }
        const auto &slotA = nodeA.slots[edge.slotInA];
        const auto &slotB = nodeB.slots[edge.slotInB];
        if (!slotA.alive || !slotB.alive) {
            return failTree(why, treeId, "tree edge points to a dead slot");
        }
        if (slotA.kind != RawSlotKind::TREE_EDGE || slotB.kind != RawSlotKind::TREE_EDGE) {
            return failTree(why, treeId, "tree edge points to non-tree slot");
        }
        if (slotA.treeEdgeId != treeId || slotB.treeEdgeId != treeId) {
            return failTree(why, treeId, "tree edge slot backlink mismatch");
        }
        if (!samePoles(edge.poleA, edge.poleB, slotA.poleA, slotA.poleB) ||
            !samePoles(edge.poleA, edge.poleB, slotB.poleA, slotB.poleB)) {
            return failTree(why, treeId, "tree edge poles do not match endpoint slots");
        }
        if (treeSeen[treeId] != 2) {
            std::ostringstream oss;
            oss << "tree edge must be referenced by exactly two slots, saw " << treeSeen[treeId];
            return failTree(why, treeId, oss.str());
        }
    }

    return true;
}

bool buildProjectMiniCore(const CompactGraph &H,
                          const ProjectRawSnapshot &snap,
                          ProjectMiniCore &out,
                          std::string &why) {
    out = {};
    out.valid = snap.valid;
    out.ownerOfInputEdge = snap.ownerOfInputEdge;

    out.nodes.reserve(snap.nodes.size());
    for (int nodeId = 0; nodeId < static_cast<int>(snap.nodes.size()); ++nodeId) {
        const auto &node = snap.nodes[nodeId];

        ProjectMiniNode dst;
        dst.alive = node.alive;
        dst.type = node.type;
        dst.slots.reserve(node.slots.size());

        for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
            const auto &slot = node.slots[slotId];

            ProjectMiniSlot dstSlot;
            dstSlot.alive = slot.alive;
            dstSlot.poleA = slot.poleA;
            dstSlot.poleB = slot.poleB;

            if (slot.kind == RawSlotKind::INPUT_EDGE) {
                if (!validInputId(H, slot.inputEdgeId)) {
                    return failMiniSlot(why, nodeId, slotId, "inputEdgeId out of range");
                }
                const auto &edge = H.edges[slot.inputEdgeId];
                dstSlot.inputEdgeId = slot.inputEdgeId;
                if (edge.kind == CompactEdgeKind::REAL) {
                    dstSlot.kind = MiniSlotKind::REAL_INPUT;
                    dstSlot.realEdge = edge.realEdge;
                } else {
                    dstSlot.kind = MiniSlotKind::PROXY_INPUT;
                }
            } else {
                if (!validTreeId(snap, slot.treeEdgeId)) {
                    return failMiniSlot(why, nodeId, slotId, "treeEdgeId out of range");
                }
                dstSlot.kind = MiniSlotKind::INTERNAL_VIRTUAL;
                dstSlot.miniArcId = slot.treeEdgeId;
            }

            dst.slots.push_back(dstSlot);
        }

        out.nodes.push_back(std::move(dst));
    }

    out.arcs.reserve(snap.treeEdges.size());
    for (const auto &treeEdge : snap.treeEdges) {
        ProjectMiniArc dst;
        dst.alive = treeEdge.alive;
        dst.a = treeEdge.a;
        dst.b = treeEdge.b;
        dst.slotInA = treeEdge.slotInA;
        dst.slotInB = treeEdge.slotInB;
        dst.poleA = treeEdge.poleA;
        dst.poleB = treeEdge.poleB;
        out.arcs.push_back(dst);
    }

    for (int arcId = 0; arcId < static_cast<int>(out.arcs.size()); ++arcId) {
        const auto &arc = out.arcs[arcId];
        if (!arc.alive) continue;
        if (arc.a < 0 || arc.a >= static_cast<int>(out.nodes.size()) ||
            arc.b < 0 || arc.b >= static_cast<int>(out.nodes.size())) {
            return failMiniArc(why, arcId, "endpoint node out of range");
        }
        auto &nodeA = out.nodes[arc.a];
        auto &nodeB = out.nodes[arc.b];
        if (!nodeA.alive || !nodeB.alive) {
            return failMiniArc(why, arcId, "endpoint node is dead");
        }
        if (!validSlotId(snap.nodes[arc.a], arc.slotInA) ||
            !validSlotId(snap.nodes[arc.b], arc.slotInB)) {
            return failMiniArc(why, arcId, "endpoint slot out of range");
        }
        if (nodeA.slots[arc.slotInA].kind != MiniSlotKind::INTERNAL_VIRTUAL ||
            nodeA.slots[arc.slotInA].miniArcId != arcId) {
            return failMiniArc(why, arcId, "slotInA backlink mismatch");
        }
        if (nodeB.slots[arc.slotInB].kind != MiniSlotKind::INTERNAL_VIRTUAL ||
            nodeB.slots[arc.slotInB].miniArcId != arcId) {
            return failMiniArc(why, arcId, "slotInB backlink mismatch");
        }
        nodeA.adjArcs.push_back(arcId);
        nodeB.adjArcs.push_back(arcId);
    }

    recomputePayloadAgg(H, out);
    out.kind = computeMiniKind(out);
    return true;
}

bool exportProjectMiniCore(const ProjectMiniCore &in,
                           StaticMiniCore &out,
                           std::string &why) {
    (void)why;

    out = {};
    out.valid = in.valid;
    out.kind = in.kind;
    out.ownerOfInputEdge = in.ownerOfInputEdge;

    out.nodes.reserve(in.nodes.size());
    for (const auto &node : in.nodes) {
        MiniNode dst;
        dst.alive = node.alive;
        dst.type = node.type;
        dst.adjArcs = node.adjArcs;
        dst.localAgg = node.localAgg;
        dst.payloadAgg = node.payloadAgg;
        dst.slots.reserve(node.slots.size());
        for (const auto &slot : node.slots) {
            MiniSlot dstSlot;
            dstSlot.kind = slot.kind;
            dstSlot.alive = slot.alive;
            dstSlot.poleA = slot.poleA;
            dstSlot.poleB = slot.poleB;
            dstSlot.inputEdgeId = slot.inputEdgeId;
            dstSlot.realEdge = slot.realEdge;
            dstSlot.miniArcId = slot.miniArcId;
            dst.slots.push_back(dstSlot);
        }
        out.nodes.push_back(std::move(dst));
    }

    out.arcs.reserve(in.arcs.size());
    for (const auto &arc : in.arcs) {
        MiniArc dst;
        dst.alive = arc.alive;
        dst.a = arc.a;
        dst.b = arc.b;
        dst.slotInA = arc.slotInA;
        dst.slotInB = arc.slotInB;
        dst.poleA = arc.poleA;
        dst.poleB = arc.poleB;
        out.arcs.push_back(dst);
    }

    return true;
}

bool importStaticMiniCore(const StaticMiniCore &in,
                          ProjectMiniCore &out,
                          std::string &why) {
    (void)why;

    out = {};
    out.valid = in.valid;
    out.kind = in.kind;
    out.ownerOfInputEdge = in.ownerOfInputEdge;

    out.nodes.reserve(in.nodes.size());
    for (const auto &node : in.nodes) {
        ProjectMiniNode dst;
        dst.alive = node.alive;
        dst.type = node.type;
        dst.adjArcs = node.adjArcs;
        dst.localAgg = node.localAgg;
        dst.payloadAgg = node.payloadAgg;
        dst.slots.reserve(node.slots.size());
        for (const auto &slot : node.slots) {
            ProjectMiniSlot dstSlot;
            dstSlot.kind = slot.kind;
            dstSlot.alive = slot.alive;
            dstSlot.poleA = slot.poleA;
            dstSlot.poleB = slot.poleB;
            dstSlot.inputEdgeId = slot.inputEdgeId;
            dstSlot.realEdge = slot.realEdge;
            dstSlot.miniArcId = slot.miniArcId;
            dst.slots.push_back(dstSlot);
        }
        out.nodes.push_back(std::move(dst));
    }

    out.arcs.reserve(in.arcs.size());
    for (const auto &arc : in.arcs) {
        ProjectMiniArc dst;
        dst.alive = arc.alive;
        dst.a = arc.a;
        dst.b = arc.b;
        dst.slotInA = arc.slotInA;
        dst.slotInB = arc.slotInB;
        dst.poleA = arc.poleA;
        dst.poleB = arc.poleB;
        out.arcs.push_back(dst);
    }

    return true;
}

bool chooseProjectKeepMiniNode(const ProjectMiniCore &mini,
                               int &keep,
                               std::string &why) {
    struct KeepScore {
        int payloadSize = 0;
        int proxyInputs = 0;
        int realInputs = 0;
        int watched = 0;
        int typeRank = 0;
        int nodeId = -1;
    };

    auto typeRank = [](SPQRType type) {
        switch (type) {
            case SPQRType::R_NODE: return 3;
            case SPQRType::P_NODE: return 2;
            case SPQRType::S_NODE: return 1;
        }
        return 0;
    };

    auto scoreNode = [&](int nodeId, const ProjectMiniNode &node) {
        KeepScore score;
        score.payloadSize = node.payloadAgg.edgeCnt + node.payloadAgg.vertexCnt;
        score.watched = node.payloadAgg.watchedCnt;
        score.typeRank = typeRank(node.type);
        score.nodeId = nodeId;
        for (const auto &slot : node.slots) {
            if (!slot.alive) continue;
            if (slot.kind == MiniSlotKind::PROXY_INPUT) {
                ++score.proxyInputs;
            } else if (slot.kind == MiniSlotKind::REAL_INPUT) {
                ++score.realInputs;
            }
        }
        return score;
    };

    auto better = [](const KeepScore &lhs, const KeepScore &rhs) {
        if (lhs.payloadSize != rhs.payloadSize) return lhs.payloadSize > rhs.payloadSize;
        if (lhs.proxyInputs != rhs.proxyInputs) return lhs.proxyInputs > rhs.proxyInputs;
        if (lhs.realInputs != rhs.realInputs) return lhs.realInputs > rhs.realInputs;
        if (lhs.watched != rhs.watched) return lhs.watched > rhs.watched;
        if (lhs.typeRank != rhs.typeRank) return lhs.typeRank > rhs.typeRank;
        return lhs.nodeId < rhs.nodeId;
    };

    keep = -1;
    why.clear();

    bool found = false;
    KeepScore best;
    for (int nodeId = 0; nodeId < static_cast<int>(mini.nodes.size()); ++nodeId) {
        const auto &node = mini.nodes[nodeId];
        if (!node.alive) continue;

        const KeepScore score = scoreNode(nodeId, node);
        if (!found || better(score, best)) {
            best = score;
            keep = nodeId;
            found = true;
        }
    }

    if (!found) {
        why = "no alive mini node";
        return false;
    }

    return true;
}

bool checkProjectMiniOwnershipConsistency(const CompactGraph &H,
                                          const ProjectMiniCore &mini,
                                          std::string &why) {
    const int inputCount = static_cast<int>(H.edges.size());
    if (static_cast<int>(mini.ownerOfInputEdge.size()) != inputCount) {
        std::ostringstream oss;
        oss << "ownerOfInputEdge size mismatch: got "
            << mini.ownerOfInputEdge.size()
            << ", expected " << inputCount;
        return fail(why, oss.str());
    }

    std::vector<int> inputSeen(inputCount, 0);

    for (int nodeId = 0; nodeId < static_cast<int>(mini.nodes.size()); ++nodeId) {
        const auto &node = mini.nodes[nodeId];
        for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
            const auto &slot = node.slots[slotId];
            if (!slot.alive) continue;

            if (slot.kind == MiniSlotKind::INTERNAL_VIRTUAL) {
                if (!validMiniArcId(mini, slot.miniArcId)) {
                    return failMiniSlot(why, nodeId, slotId, "miniArcId out of range");
                }
                continue;
            }

            if (!validInputId(H, slot.inputEdgeId)) {
                return failMiniSlot(why, nodeId, slotId, "inputEdgeId out of range");
            }
            ++inputSeen[slot.inputEdgeId];

            const auto &edge = H.edges[slot.inputEdgeId];
            if (edge.kind == CompactEdgeKind::REAL) {
                if (slot.kind != MiniSlotKind::REAL_INPUT) {
                    return failMiniSlot(why, nodeId, slotId, "REAL compact edge must map to REAL_INPUT slot");
                }
                if (slot.realEdge != edge.realEdge) {
                    std::ostringstream oss;
                    oss << "realEdge mismatch: got " << slot.realEdge
                        << ", expected " << edge.realEdge;
                    return failMiniSlot(why, nodeId, slotId, oss.str());
                }
            } else {
                if (slot.kind != MiniSlotKind::PROXY_INPUT) {
                    return failMiniSlot(why, nodeId, slotId, "PROXY compact edge must map to PROXY_INPUT slot");
                }
            }
        }
    }

    for (int inputId = 0; inputId < inputCount; ++inputId) {
        const auto owner = mini.ownerOfInputEdge[inputId];
        if (!validMiniNodeId(mini, owner.first)) {
            return failMiniInput(why, inputId, "owner node out of range");
        }
        const auto &node = mini.nodes[owner.first];
        if (!node.alive) {
            return failMiniInput(why, inputId, "owner node is dead");
        }
        if (!validMiniSlotId(node, owner.second)) {
            return failMiniInput(why, inputId, "owner slot out of range");
        }
        const auto &slot = node.slots[owner.second];
        if (!slot.alive) {
            return failMiniInput(why, inputId, "owner slot is dead");
        }
        if (slot.inputEdgeId != inputId) {
            std::ostringstream oss;
            oss << "owner slot inputEdgeId mismatch: got " << slot.inputEdgeId
                << ", expected " << inputId;
            return failMiniInput(why, inputId, oss.str());
        }

        const auto &edge = H.edges[inputId];
        if (edge.kind == CompactEdgeKind::REAL) {
            if (slot.kind != MiniSlotKind::REAL_INPUT) {
                return failMiniInput(why, inputId, "REAL compact edge owner must be REAL_INPUT slot");
            }
            if (slot.realEdge != edge.realEdge) {
                std::ostringstream oss;
                oss << "owner slot realEdge mismatch: got " << slot.realEdge
                    << ", expected " << edge.realEdge;
                return failMiniInput(why, inputId, oss.str());
            }
        } else {
            if (slot.kind != MiniSlotKind::PROXY_INPUT) {
                return failMiniInput(why, inputId, "PROXY compact edge owner must be PROXY_INPUT slot");
            }
        }

        if (inputSeen[inputId] != 1) {
            std::ostringstream oss;
            oss << "input edge must appear exactly once, saw " << inputSeen[inputId] << " slots";
            return failMiniInput(why, inputId, oss.str());
        }
    }

    for (int arcId = 0; arcId < static_cast<int>(mini.arcs.size()); ++arcId) {
        const auto &arc = mini.arcs[arcId];
        if (!arc.alive) continue;
        if (!validMiniNodeId(mini, arc.a) || !validMiniNodeId(mini, arc.b)) {
            return failMiniArc(why, arcId, "endpoint node out of range");
        }
        const auto &nodeA = mini.nodes[arc.a];
        const auto &nodeB = mini.nodes[arc.b];
        if (!nodeA.alive || !nodeB.alive) {
            return failMiniArc(why, arcId, "endpoint node is dead");
        }
        if (!validMiniSlotId(nodeA, arc.slotInA) || !validMiniSlotId(nodeB, arc.slotInB)) {
            return failMiniArc(why, arcId, "endpoint slot out of range");
        }
        const auto &slotA = nodeA.slots[arc.slotInA];
        const auto &slotB = nodeB.slots[arc.slotInB];
        if (!slotA.alive || !slotB.alive) {
            return failMiniArc(why, arcId, "endpoint slot is dead");
        }
        if (slotA.kind != MiniSlotKind::INTERNAL_VIRTUAL ||
            slotA.miniArcId != arcId) {
            return failMiniArc(why, arcId, "slotInA backlink mismatch");
        }
        if (slotB.kind != MiniSlotKind::INTERNAL_VIRTUAL ||
            slotB.miniArcId != arcId) {
            return failMiniArc(why, arcId, "slotInB backlink mismatch");
        }
    }

    return true;
}

bool checkProjectMiniReducedInvariant(const CompactGraph &H,
                                      const ProjectMiniCore &mini,
                                      std::string &why) {
    (void)H;

    int aliveNodeCount = 0;
    int aliveArcCount = 0;

    for (int nodeId = 0; nodeId < static_cast<int>(mini.nodes.size()); ++nodeId) {
        const auto &node = mini.nodes[nodeId];
        if (node.alive) ++aliveNodeCount;

        for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
            const auto &slot = node.slots[slotId];
            if (!slot.alive) continue;
            if (slot.kind != MiniSlotKind::INTERNAL_VIRTUAL) continue;
            if (!validMiniArcId(mini, slot.miniArcId)) {
                return failMiniSlot(why, nodeId, slotId, "miniArcId out of range");
            }
        }
    }

    std::vector<int> liveInputSlots(mini.nodes.size(), 0);
    std::vector<int> liveArcDegree(mini.nodes.size(), 0);

    for (int nodeId = 0; nodeId < static_cast<int>(mini.nodes.size()); ++nodeId) {
        const auto &node = mini.nodes[nodeId];
        int liveSlotCount = 0;
        int inputSlotCount = 0;
        for (const auto &slot : node.slots) {
            if (!slot.alive) continue;
            ++liveSlotCount;
            if (slot.kind != MiniSlotKind::INTERNAL_VIRTUAL) {
                ++inputSlotCount;
            }
        }
        liveInputSlots[nodeId] = inputSlotCount;

        if (!node.alive) continue;
        if ((node.type == SPQRType::S_NODE || node.type == SPQRType::P_NODE) &&
            liveSlotCount < 3) {
            return failMiniNode(why, nodeId, "live S/P node must have at least 3 alive slots");
        }
    }

    for (int arcId = 0; arcId < static_cast<int>(mini.arcs.size()); ++arcId) {
        const auto &arc = mini.arcs[arcId];
        if (!arc.alive) continue;
        ++aliveArcCount;

        if (!validMiniNodeId(mini, arc.a) || !validMiniNodeId(mini, arc.b)) {
            return failMiniArc(why, arcId, "endpoint node out of range");
        }
        const auto &nodeA = mini.nodes[arc.a];
        const auto &nodeB = mini.nodes[arc.b];
        if (!nodeA.alive || !nodeB.alive) {
            return failMiniArc(why, arcId, "endpoint node is dead");
        }
        if (!validMiniSlotId(nodeA, arc.slotInA) || !validMiniSlotId(nodeB, arc.slotInB)) {
            return failMiniArc(why, arcId, "endpoint slot out of range");
        }

        const auto &slotA = nodeA.slots[arc.slotInA];
        const auto &slotB = nodeB.slots[arc.slotInB];
        if (!slotA.alive || !slotB.alive) {
            return failMiniArc(why, arcId, "endpoint slot is dead");
        }
        if (slotA.kind != MiniSlotKind::INTERNAL_VIRTUAL ||
            slotA.miniArcId != arcId) {
            return failMiniArc(why, arcId, "slotInA backlink mismatch");
        }
        if (slotB.kind != MiniSlotKind::INTERNAL_VIRTUAL ||
            slotB.miniArcId != arcId) {
            return failMiniArc(why, arcId, "slotInB backlink mismatch");
        }
        if (!samePoles(arc.poleA, arc.poleB, slotA.poleA, slotA.poleB) ||
            !samePoles(arc.poleA, arc.poleB, slotB.poleA, slotB.poleB)) {
            return failMiniArc(why, arcId, "arc poles do not match endpoint slot poles");
        }

        ++liveArcDegree[arc.a];
        ++liveArcDegree[arc.b];

        if (mini.kind == CoreKind::REDUCED_SPQR) {
            if ((nodeA.type == SPQRType::S_NODE && nodeB.type == SPQRType::S_NODE) ||
                (nodeA.type == SPQRType::P_NODE && nodeB.type == SPQRType::P_NODE)) {
                return failMiniArc(why, arcId, "adjacent S-S or P-P nodes are forbidden");
            }
        }
    }

    if (mini.kind == CoreKind::TINY) {
        if (aliveNodeCount <= 1 && aliveArcCount == 0) return true;
        std::ostringstream oss;
        oss << "TINY core must satisfy aliveNodeCount <= 1 and aliveArcCount == 0"
            << " (got nodes=" << aliveNodeCount
            << ", arcs=" << aliveArcCount << ")";
        return fail(why, oss.str());
    }

    for (int nodeId = 0; nodeId < static_cast<int>(mini.nodes.size()); ++nodeId) {
        const auto &node = mini.nodes[nodeId];
        if (!node.alive) continue;
        if (liveInputSlots[nodeId] == 0 && liveArcDegree[nodeId] <= 2) {
            std::ostringstream oss;
            oss << "dead relay forbidden: inputSlots=" << liveInputSlots[nodeId]
                << ", liveArcDegree=" << liveArcDegree[nodeId];
            return failMiniNode(why, nodeId, oss.str());
        }
    }

    return true;
}

bool buildProjectCompactLocalViewFromR(const ReducedSPQRCore &core,
                                       NodeId rNode,
                                       VertexId x,
                                       CompactGraph &out,
                                       CompactRejectReason *reason,
                                       CompactBuildFailSubtype *subtype,
                                       std::string &why) {
    out = {};
    why.clear();
    setRejectReason(reason, CompactRejectReason::OTHER);
    setCompactBuildFailSubtype(subtype, CompactBuildFailSubtype::CBF_OTHER);

    if (!validActualNodeId(core, rNode) || !core.nodes[rNode].alive) {
        setRejectReason(reason, CompactRejectReason::OWNER_NOT_R);
        setCompactBuildFailSubtype(subtype, CompactBuildFailSubtype::CBF_NODE_DEAD);
        return fail(why, "compact build: rNode dead");
    }
    const auto &node = core.nodes[rNode];
    if (node.type != SPQRType::R_NODE) {
        setRejectReason(reason, CompactRejectReason::OWNER_NOT_R);
        setCompactBuildFailSubtype(subtype, CompactBuildFailSubtype::CBF_NODE_NOT_R);
        return fail(why, "compact build: rNode not R");
    }

    bool xSeen = false;
    std::vector<VertexId> compactVertices;
    compactVertices.reserve(node.slots.size() * 2);
    for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
        const auto &slot = node.slots[slotId];
        if (!slot.alive) continue;
        if (slot.poleA == x || slot.poleB == x) xSeen = true;
        if (slot.poleA != x) compactVertices.push_back(slot.poleA);
        if (slot.poleB != x) compactVertices.push_back(slot.poleB);
    }
    if (!xSeen) {
        setRejectReason(reason, CompactRejectReason::X_NOT_PRESENT_IN_R);
        setCompactBuildFailSubtype(subtype, CompactBuildFailSubtype::CBF_X_NOT_PRESENT_IN_R);
        return fail(why, "compact build: x not present in R node");
    }

    std::sort(compactVertices.begin(), compactVertices.end());
    compactVertices.erase(std::unique(compactVertices.begin(), compactVertices.end()),
                          compactVertices.end());

    out.block = core.blockId;
    out.ownerR = rNode;
    out.deletedX = x;
    out.origOfCv = compactVertices;
    out.touchedVertices = compactVertices;
    for (int cv = 0; cv < static_cast<int>(compactVertices.size()); ++cv) {
        out.cvOfOrig[compactVertices[cv]] = cv;
    }

    int nextInputId = 0;
    for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
        const auto &slot = node.slots[slotId];
        if (!slot.alive) continue;

        const bool incidentToX = slot.poleA == x || slot.poleB == x;
        if (!slot.isVirtual) {
            if (incidentToX) continue;
            const auto itA = out.cvOfOrig.find(slot.poleA);
            const auto itB = out.cvOfOrig.find(slot.poleB);
            if (itA == out.cvOfOrig.end() || itB == out.cvOfOrig.end()) {
                setCompactBuildFailSubtype(subtype,
                                           CompactBuildFailSubtype::CBF_REAL_ENDPOINT_NOT_MAPPED);
                return fail(why, "compact build: REAL endpoint not mapped");
            }
            CompactEdge edge;
            edge.id = nextInputId++;
            edge.kind = CompactEdgeKind::REAL;
            edge.a = itA->second;
            edge.b = itB->second;
            edge.realEdge = slot.realEdge;
            out.edges.push_back(edge);
            continue;
        }

        if (incidentToX) {
            setRejectReason(reason, CompactRejectReason::X_INCIDENT_VIRTUAL_UNSUPPORTED);
            return fail(why, "compact build: virtual slot incident to x unsupported in first pass");
        }
        const int oldSlotInU = slotId;
        if (!validActualSlotId(node, oldSlotInU)) {
            setCompactBuildFailSubtype(subtype,
                                       CompactBuildFailSubtype::CBF_PROXY_OLDSLOT_INVALID);
            return fail(why, "compact build: invalid PROXY oldSlotInU");
        }
        const auto &oldSlot = node.slots[oldSlotInU];
        if (!validActualArcId(core, oldSlot.arcId) || !core.arcs[oldSlot.arcId].alive) {
            setCompactBuildFailSubtype(subtype,
                                       CompactBuildFailSubtype::CBF_PROXY_OLDARC_INVALID);
            return fail(why, "compact build: invalid PROXY oldArc");
        }
        if (oldSlot.arcId != slot.arcId) {
            setCompactBuildFailSubtype(subtype,
                                       CompactBuildFailSubtype::CBF_PROXY_OLDSLOT_ARC_MISMATCH);
            return fail(why, "compact build: old slot does not point to oldArc");
        }

        const auto &arc = core.arcs[oldSlot.arcId];
        const NodeId outsideNode = otherEndpointOfArc(arc, rNode);
        if (!validActualNodeId(core, outsideNode) || !core.nodes[outsideNode].alive) {
            setCompactBuildFailSubtype(subtype,
                                       CompactBuildFailSubtype::CBF_PROXY_OUTSIDENODE_INVALID);
            return fail(why, "compact build: invalid PROXY outsideNode");
        }

        Agg sideAgg = computeAggToward(core, rNode, outsideNode);
        if (sideAgg.edgeCnt == 0) continue;

        const auto itA = out.cvOfOrig.find(slot.poleA);
        const auto itB = out.cvOfOrig.find(slot.poleB);
        if (itA == out.cvOfOrig.end() || itB == out.cvOfOrig.end()) {
            setCompactBuildFailSubtype(subtype,
                                       CompactBuildFailSubtype::CBF_REAL_ENDPOINT_NOT_MAPPED);
            return fail(why, "compact build: endpoint not mapped");
        }

        CompactEdge edge;
        edge.id = nextInputId++;
        edge.kind = CompactEdgeKind::PROXY;
        edge.a = itA->second;
        edge.b = itB->second;
        edge.oldArc = oldSlot.arcId;
        edge.outsideNode = outsideNode;
        edge.oldSlotInU = oldSlotInU;
        edge.sideAgg = sideAgg;
        out.edges.push_back(edge);
    }

    if (out.edges.empty()) {
        setRejectReason(reason, CompactRejectReason::EMPTY_AFTER_DELETE);
        return fail(why, "rewriteR: compact local graph empty after deleting x");
    }

    return true;
}

bool validateProxyMetadataForRewrite(const ReducedSPQRCore &core,
                                     NodeId oldNode,
                                     const CompactGraph &H,
                                     std::string &why) {
    why.clear();

    if (!validActualNodeId(core, oldNode) || !core.nodes[oldNode].alive) {
        return fail(why, "rewrite proxy precheck: old node invalid");
    }

    std::unordered_set<ArcId> seenProxyArcs;
    const auto &oldActualNode = core.nodes[oldNode];
    for (const auto &edge : H.edges) {
        if (edge.kind != CompactEdgeKind::PROXY) continue;

        if (!validActualArcId(core, edge.oldArc)) {
            std::ostringstream oss;
            oss << "rewrite proxy precheck: oldArc out of range for input " << edge.id;
            return fail(why, oss.str());
        }
        const auto &oldArc = core.arcs[edge.oldArc];
        if (!oldArc.alive) {
            std::ostringstream oss;
            oss << "rewrite proxy precheck: oldArc dead for input " << edge.id;
            return fail(why, oss.str());
        }

        const bool oldOnA = oldArc.a == oldNode;
        const bool oldOnB = oldArc.b == oldNode;
        if (oldOnA == oldOnB) {
            std::ostringstream oss;
            oss << "rewrite proxy precheck: oldArc not incident to old node for input "
                << edge.id;
            return fail(why, oss.str());
        }

        const NodeId actualOutside = oldOnA ? oldArc.b : oldArc.a;
        if (actualOutside != edge.outsideNode) {
            std::ostringstream oss;
            oss << "rewrite proxy precheck: outsideNode mismatch for input " << edge.id;
            return fail(why, oss.str());
        }

        if (!validActualSlotId(oldActualNode, edge.oldSlotInU)) {
            std::ostringstream oss;
            oss << "rewrite proxy precheck: oldSlotInU invalid for input " << edge.id;
            return fail(why, oss.str());
        }
        const auto &oldSlot = oldActualNode.slots[edge.oldSlotInU];
        if (!oldSlot.alive || !oldSlot.isVirtual || oldSlot.arcId != edge.oldArc) {
            std::ostringstream oss;
            oss << "rewrite proxy precheck: old slot does not point to oldArc for input "
                << edge.id;
            return fail(why, oss.str());
        }

        if (!seenProxyArcs.insert(edge.oldArc).second) {
            std::ostringstream oss;
            oss << "rewrite proxy precheck: duplicate PROXY oldArc in compact graph for input "
                << edge.id;
            return fail(why, oss.str());
        }
    }

    return true;
}

struct ResolvedProxyEndpointFailureInfo {
    GraftRewireBailoutSubtype subtype = GraftRewireBailoutSubtype::GRB_OTHER;
    int inputEdgeId = -1;
    ArcId oldArc = -1;
    NodeId outsideNode = -1;
    int resolvedOldSlot = -1;
};

ProxyArcNoCandidateSubtype classifyProxyRepairNoCandidate(const ReducedSPQRCore &core,
                                                          NodeId oldNode,
                                                          const CompactGraph &H,
                                                          const CompactEdge &proxyEdge,
                                                          std::string &why) {
    why.clear();

    if (!validActualNodeId(core, oldNode) || !core.nodes[oldNode].alive) {
        why = "proxy repair no-candidate: oldNode dead";
        return ProxyArcNoCandidateSubtype::PNC_OLDNODE_DEAD;
    }
    if (!validActualNodeId(core, proxyEdge.outsideNode) ||
        !core.nodes[proxyEdge.outsideNode].alive) {
        why = "proxy repair no-candidate: outsideNode dead";
        return ProxyArcNoCandidateSubtype::PNC_OUTSIDENODE_DEAD;
    }
    if (proxyEdge.a < 0 || proxyEdge.a >= static_cast<int>(H.origOfCv.size()) ||
        proxyEdge.b < 0 || proxyEdge.b >= static_cast<int>(H.origOfCv.size())) {
        why = "proxy repair no-candidate: compact endpoints invalid";
        return ProxyArcNoCandidateSubtype::PNC_OTHER;
    }

    const auto expectedPoles =
        canonPole(H.origOfCv[proxyEdge.a], H.origOfCv[proxyEdge.b]);
    const auto &oldActualNode = core.nodes[oldNode];

    struct IncidentArcView {
        ArcId arcId = -1;
        NodeId other = -1;
        int oldSlot = -1;
        VertexId poleA = -1;
        VertexId poleB = -1;
        bool slotValid = false;
        bool slotAlive = false;
        bool slotVirtual = false;
        bool slotArcMatches = false;
    };

    std::vector<IncidentArcView> liveIncident;
    std::vector<IncidentArcView> toOutside;
    std::vector<IncidentArcView> samePolesOtherOutside;
    std::vector<IncidentArcView> weakCandidates;

    for (ArcId arcId = 0; arcId < static_cast<int>(core.arcs.size()); ++arcId) {
        const auto &arc = core.arcs[arcId];
        if (!arc.alive) continue;

        const bool oldOnA = arc.a == oldNode;
        const bool oldOnB = arc.b == oldNode;
        if (oldOnA == oldOnB) continue;

        IncidentArcView view;
        view.arcId = arcId;
        view.other = oldOnA ? arc.b : arc.a;
        view.oldSlot = oldOnA ? arc.slotInA : arc.slotInB;
        view.poleA = arc.poleA;
        view.poleB = arc.poleB;
        view.slotValid = validActualSlotId(oldActualNode, view.oldSlot);
        if (view.slotValid) {
            const auto &slot = oldActualNode.slots[view.oldSlot];
            view.slotAlive = slot.alive;
            view.slotVirtual = slot.isVirtual;
            view.slotArcMatches = slot.arcId == arcId;
        }
        liveIncident.push_back(view);

        const bool polesMatch = canonPole(arc.poleA, arc.poleB) == expectedPoles;
        if (view.other == proxyEdge.outsideNode) {
            toOutside.push_back(view);
            if (polesMatch) weakCandidates.push_back(view);
        } else if (polesMatch) {
            samePolesOtherOutside.push_back(view);
        }
    }

    if (liveIncident.empty()) {
        why = "proxy repair no-candidate: no live arcs on oldNode";
        return ProxyArcNoCandidateSubtype::PNC_OLDNODE_NO_LIVE_ARCS;
    }
    if (toOutside.empty()) {
        if (!samePolesOtherOutside.empty()) {
            why = "proxy repair no-candidate: same poles exist but outsideNode drifted";
            return ProxyArcNoCandidateSubtype::PNC_SAME_POLES_BUT_OTHER_OUTSIDE;
        }
        why = "proxy repair no-candidate: no arc to outsideNode";
        return ProxyArcNoCandidateSubtype::PNC_NO_ARC_TO_OUTSIDENODE;
    }
    if (weakCandidates.empty()) {
        why = "proxy repair no-candidate: arc to outsideNode exists but poles mismatch";
        return ProxyArcNoCandidateSubtype::PNC_TO_OUTSIDENODE_BUT_WRONG_POLES;
    }
    if (weakCandidates.size() > 1) {
        why = "proxy repair no-candidate: multiple weak candidates";
        return ProxyArcNoCandidateSubtype::PNC_MULTI_WEAK_CANDIDATES;
    }

    const auto &candidate = weakCandidates.front();
    if (!candidate.slotValid || !candidate.slotAlive || !candidate.slotVirtual) {
        why = "proxy repair no-candidate: candidate slot not virtual";
        return ProxyArcNoCandidateSubtype::PNC_CANDIDATE_SLOT_NOT_VIRTUAL;
    }
    if (!candidate.slotArcMatches) {
        why = "proxy repair no-candidate: candidate slot arcId mismatch";
        return ProxyArcNoCandidateSubtype::PNC_CANDIDATE_SLOT_ARCID_MISMATCH;
    }

    why = "proxy repair no-candidate: other";
    return ProxyArcNoCandidateSubtype::PNC_OTHER;
}

bool findForbiddenDeadRelayActual(const ReducedSPQRCore &core, NodeId &offendingNode) {
    offendingNode = -1;
    std::vector<int> liveRealSlotCount(core.nodes.size(), 0);
    std::vector<int> liveArcDegree(core.nodes.size(), 0);

    for (NodeId nodeId = 0; nodeId < static_cast<NodeId>(core.nodes.size()); ++nodeId) {
        const auto &node = core.nodes[nodeId];
        if (!node.alive) continue;
        for (const auto &slot : node.slots) {
            if (!slot.alive) continue;
            if (!slot.isVirtual) ++liveRealSlotCount[nodeId];
        }
    }

    for (ArcId arcId = 0; arcId < static_cast<ArcId>(core.arcs.size()); ++arcId) {
        const auto &arc = core.arcs[arcId];
        if (!arc.alive) continue;
        if (validActualNodeId(core, arc.a)) ++liveArcDegree[arc.a];
        if (validActualNodeId(core, arc.b)) ++liveArcDegree[arc.b];
    }

    for (NodeId nodeId = 0; nodeId < static_cast<NodeId>(core.nodes.size()); ++nodeId) {
        const auto &node = core.nodes[nodeId];
        if (!node.alive) continue;
        if (liveRealSlotCount[nodeId] == 0 && liveArcDegree[nodeId] <= 2) {
            offendingNode = nodeId;
            return true;
        }
    }

    return false;
}

bool findForbiddenSameTypeSPAdjacencyActual(const ReducedSPQRCore &core,
                                            ArcId &offendingArc,
                                            NodeId &nodeAOut,
                                            NodeId &nodeBOut) {
    offendingArc = -1;
    nodeAOut = -1;
    nodeBOut = -1;

    for (ArcId arcId = 0; arcId < static_cast<ArcId>(core.arcs.size()); ++arcId) {
        const auto &arc = core.arcs[arcId];
        if (!arc.alive) continue;
        if (!validActualNodeId(core, arc.a) || !validActualNodeId(core, arc.b)) continue;

        const auto &nodeA = core.nodes[arc.a];
        const auto &nodeB = core.nodes[arc.b];
        if (!nodeA.alive || !nodeB.alive) continue;
        if ((nodeA.type == SPQRType::S_NODE && nodeB.type == SPQRType::S_NODE) ||
            (nodeA.type == SPQRType::P_NODE && nodeB.type == SPQRType::P_NODE)) {
            offendingArc = arcId;
            nodeAOut = arc.a;
            nodeBOut = arc.b;
            return true;
        }
    }

    return false;
}

bool resolveLiveProxyArcForSequenceRepair(const ReducedSPQRCore &core,
                                          NodeId oldNode,
                                          const CompactGraph &H,
                                          const CompactEdge &proxyEdge,
                                          RepairedProxyArcInfo &out,
                                          std::string &why) {
    why.clear();
    out = {};
    out.inputEdgeId = proxyEdge.id;
    out.originalOldArc = proxyEdge.oldArc;
    out.oldNode = oldNode;
    out.originalOutsideNode = proxyEdge.outsideNode;
    out.resolvedOutsideNode = proxyEdge.outsideNode;
    out.outsideNode = proxyEdge.outsideNode;

    auto recordOutcome = [&](ProxyArcRepairOutcome outcome, bool success) {
        ++gRewriteRStats
              .seqResolvedOldArcRepairOutcomeCounts[proxyArcRepairOutcomeIndex(outcome)];
        if (outcome == ProxyArcRepairOutcome::PAR_OLDARC_ALREADY_LIVE) return;
        ++gRewriteRStats.seqResolvedOldArcRepairAttemptCount;
        if (success) {
            ++gRewriteRStats.seqResolvedOldArcRepairSuccessCount;
            ++gRewriteRStats.seqResolvedOldArcRepairUsedCount;
        } else {
            ++gRewriteRStats.seqResolvedOldArcRepairFailCount;
        }
    };

    auto recordWeakOutcome = [&](ProxyArcRepairOutcome outcome, bool success) {
        ++gRewriteRStats
              .seqResolvedOldArcWeakRepairOutcomeCounts[proxyArcRepairOutcomeIndex(outcome)];
        ++gRewriteRStats.seqResolvedOldArcWeakRepairAttemptCount;
        if (success) {
            ++gRewriteRStats.seqResolvedOldArcWeakRepairSuccessCount;
            ++gRewriteRStats.seqResolvedOldArcWeakRepairUsedCount;
        } else {
            ++gRewriteRStats.seqResolvedOldArcWeakRepairFailCount;
        }
    };

    auto failRepair = [&](ProxyArcRepairOutcome outcome, const std::string &msg) {
        out.repairOutcome = outcome;
        recordOutcome(outcome, false);
        if (gRewriteRCaseContext.sequenceMode &&
            outcome == ProxyArcRepairOutcome::PAR_FAIL_NO_CANDIDATE) {
            std::string subWhy;
            const auto subtype =
                classifyProxyRepairNoCandidate(core, oldNode, H, proxyEdge, subWhy);
            noteSequenceProxyRepairNoCandidateSubtype(subtype,
                                                      core,
                                                      oldNode,
                                                      H,
                                                      proxyEdge,
                                                      out,
                                                      subWhy.empty() ? msg : subWhy);
        }
        return fail(why, msg);
    };

    if (proxyEdge.kind != CompactEdgeKind::PROXY) {
        return failRepair(ProxyArcRepairOutcome::PAR_OTHER,
                          "proxy arc repair: input edge is not PROXY");
    }
    if (!validActualNodeId(core, oldNode) || !core.nodes[oldNode].alive) {
        return failRepair(ProxyArcRepairOutcome::PAR_OTHER,
                          "proxy arc repair: old node invalid");
    }
    if (proxyEdge.a < 0 || proxyEdge.a >= static_cast<int>(H.origOfCv.size()) ||
        proxyEdge.b < 0 || proxyEdge.b >= static_cast<int>(H.origOfCv.size())) {
        return failRepair(ProxyArcRepairOutcome::PAR_OTHER,
                          "proxy arc repair: compact endpoint out of range");
    }

    const auto expectedPoles =
        canonPole(H.origOfCv[proxyEdge.a], H.origOfCv[proxyEdge.b]);
    out.poleA = expectedPoles.first;
    out.poleB = expectedPoles.second;

    const auto &oldActualNode = core.nodes[oldNode];
    const bool sequenceMode = gRewriteRCaseContext.sequenceMode;

    if (validActualArcId(core, proxyEdge.oldArc)) {
        const auto &oldArc = core.arcs[proxyEdge.oldArc];
        if (oldArc.alive) {
            const bool oldOnA = oldArc.a == oldNode;
            const bool oldOnB = oldArc.b == oldNode;
            if (oldOnA != oldOnB) {
                const NodeId outsideNode = oldOnA ? oldArc.b : oldArc.a;
                const auto arcPoles = canonPole(oldArc.poleA, oldArc.poleB);
                if (outsideNode == proxyEdge.outsideNode && arcPoles == expectedPoles) {
                    const int resolvedOldSlot = oldOnA ? oldArc.slotInA : oldArc.slotInB;
                    if (!validActualSlotId(oldActualNode, resolvedOldSlot)) {
                        return failRepair(ProxyArcRepairOutcome::PAR_OTHER,
                                          "proxy arc repair: old slot invalid");
                    }
                    const auto &oldSlot = oldActualNode.slots[resolvedOldSlot];
                    if (!oldSlot.alive || !oldSlot.isVirtual) {
                        return failRepair(
                            ProxyArcRepairOutcome::PAR_FAIL_SLOT_NOT_VIRTUAL,
                            "proxy arc repair: old slot not virtual");
                    }
                    if (oldSlot.arcId != proxyEdge.oldArc) {
                        return failRepair(
                            ProxyArcRepairOutcome::PAR_FAIL_SLOT_ARCID_MISMATCH,
                            "proxy arc repair: old slot arcId mismatch");
                    }
                    out.resolvedArc = proxyEdge.oldArc;
                    out.outsideNode = outsideNode;
                    out.resolvedOutsideNode = outsideNode;
                    out.resolvedOldSlot = resolvedOldSlot;
                    out.poleA = oldArc.poleA;
                    out.poleB = oldArc.poleB;
                    out.weakRepairEntered = false;
                    out.weakRepairGateSubtype =
                        WeakRepairGateSubtype::WRG_NOT_NEEDED_STRONG_LIVE;
                    if (sequenceMode) {
                        noteWeakRepairGateSample(core,
                                                 oldNode,
                                                 H,
                                                 proxyEdge,
                                                 out,
                                                 "proxy weak repair gate: not needed, strong live match");
                    }
                    out.repairOutcome = ProxyArcRepairOutcome::PAR_OLDARC_ALREADY_LIVE;
                    recordOutcome(out.repairOutcome, true);
                    return true;
                }
            }
        }
    }

    struct Candidate {
        ArcId arcId = -1;
        NodeId outsideNode = -1;
        int slotId = -1;
        VertexId poleA = -1;
        VertexId poleB = -1;
        bool slotValid = false;
        bool slotAlive = false;
        bool slotVirtual = false;
        bool slotArcMatches = false;
    };
    const bool outsideNodeAlive = validActualNodeId(core, proxyEdge.outsideNode) &&
                                  core.nodes[proxyEdge.outsideNode].alive;
    std::vector<Candidate> candidates;
    bool sawSlotNotVirtual = false;
    bool sawSlotArcMismatch = false;

    for (ArcId arcId = 0; arcId < static_cast<int>(core.arcs.size()); ++arcId) {
        const auto &arc = core.arcs[arcId];
        if (!arc.alive) continue;

        const bool oldOnA = arc.a == oldNode;
        const bool oldOnB = arc.b == oldNode;
        if (oldOnA == oldOnB) continue;

        const NodeId outsideNode = oldOnA ? arc.b : arc.a;
        const auto arcPoles = canonPole(arc.poleA, arc.poleB);
        if (!outsideNodeAlive || outsideNode != proxyEdge.outsideNode) continue;
        if (arcPoles != expectedPoles) continue;

        const int resolvedOldSlot = oldOnA ? arc.slotInA : arc.slotInB;
        Candidate candidate;
        candidate.arcId = arcId;
        candidate.outsideNode = outsideNode;
        candidate.slotId = resolvedOldSlot;
        candidate.poleA = arc.poleA;
        candidate.poleB = arc.poleB;
        candidate.slotValid = validActualSlotId(oldActualNode, resolvedOldSlot);
        if (!candidate.slotValid) {
            sawSlotNotVirtual = true;
            continue;
        }

        const auto &oldSlot = oldActualNode.slots[resolvedOldSlot];
        candidate.slotAlive = oldSlot.alive;
        candidate.slotVirtual = oldSlot.isVirtual;
        candidate.slotArcMatches = oldSlot.arcId == arcId;
        if (!candidate.slotAlive || !candidate.slotVirtual) {
            sawSlotNotVirtual = true;
            continue;
        }
        if (!candidate.slotArcMatches) {
            sawSlotArcMismatch = true;
            continue;
        }

        candidates.push_back(candidate);
    }

    if (candidates.size() > 1) {
        return failRepair(
            ProxyArcRepairOutcome::PAR_FAIL_MULTI_CANDIDATE,
            "proxy arc repair: multiple live candidates with same outsideNode and poles");
    }
    if (!candidates.empty()) {
        const auto &candidate = candidates.front();
        out.resolvedArc = candidate.arcId;
        out.outsideNode = proxyEdge.outsideNode;
        out.resolvedOutsideNode = proxyEdge.outsideNode;
        out.resolvedOldSlot = candidate.slotId;
        out.poleA = candidate.poleA;
        out.poleB = candidate.poleB;
        out.repairOutcome = ProxyArcRepairOutcome::PAR_MATCH_BY_OUTSIDENODE_AND_POLES;
        recordOutcome(out.repairOutcome, true);
        return true;
    }

    if (sawSlotArcMismatch) {
        return failRepair(
            ProxyArcRepairOutcome::PAR_FAIL_SLOT_ARCID_MISMATCH,
            "proxy arc repair: old slot arcId mismatch");
    }
    if (sawSlotNotVirtual) {
        return failRepair(
            ProxyArcRepairOutcome::PAR_FAIL_SLOT_NOT_VIRTUAL,
            "proxy arc repair: old slot not virtual");
    }

    if (!sequenceMode) {
        if (!outsideNodeAlive) {
            return failRepair(ProxyArcRepairOutcome::PAR_FAIL_OUTSIDENODE_DEAD,
                              "proxy arc repair: outsideNode dead");
        }
        return failRepair(
            ProxyArcRepairOutcome::PAR_FAIL_NO_CANDIDATE,
            "proxy arc repair: no live candidate with same outsideNode and poles");
    }

    std::string gateWhy;
    const auto noCandidateSubtype =
        classifyProxyRepairNoCandidate(core, oldNode, H, proxyEdge, gateWhy);
    if (noCandidateSubtype ==
        ProxyArcNoCandidateSubtype::PNC_SAME_POLES_BUT_OTHER_OUTSIDE) {
        out.weakRepairEntered = true;
        out.weakRepairGateSubtype =
            WeakRepairGateSubtype::WRG_ENTER_PNC_SAME_POLES_BUT_OTHER_OUTSIDE;
        ++gRewriteRStats.seqWeakRepairEnteredCount;
    } else if (noCandidateSubtype ==
               ProxyArcNoCandidateSubtype::PNC_OLDNODE_NO_LIVE_ARCS) {
        out.weakRepairEntered = false;
        out.weakRepairGateSubtype =
            WeakRepairGateSubtype::WRG_SKIP_PNC_OLDNODE_NO_LIVE_ARCS;
    } else {
        out.weakRepairEntered = false;
        out.weakRepairGateSubtype = WeakRepairGateSubtype::WRG_SKIP_OTHER_PNC;
    }
    noteWeakRepairGateSample(core,
                             oldNode,
                             H,
                             proxyEdge,
                             out,
                             gateWhy.empty() ? "proxy weak repair gate: other" : gateWhy);

    std::vector<Candidate> weakCandidates;
    for (ArcId arcId = 0; arcId < static_cast<int>(core.arcs.size()); ++arcId) {
        const auto &arc = core.arcs[arcId];
        if (!arc.alive) continue;

        const bool oldOnA = arc.a == oldNode;
        const bool oldOnB = arc.b == oldNode;
        if (oldOnA == oldOnB) continue;
        if (canonPole(arc.poleA, arc.poleB) != expectedPoles) continue;

        Candidate candidate;
        candidate.arcId = arcId;
        candidate.outsideNode = oldOnA ? arc.b : arc.a;
        candidate.slotId = oldOnA ? arc.slotInA : arc.slotInB;
        candidate.poleA = arc.poleA;
        candidate.poleB = arc.poleB;
        candidate.slotValid = validActualSlotId(oldActualNode, candidate.slotId);
        if (candidate.slotValid) {
            const auto &oldSlot = oldActualNode.slots[candidate.slotId];
            candidate.slotAlive = oldSlot.alive;
            candidate.slotVirtual = oldSlot.isVirtual;
            candidate.slotArcMatches = oldSlot.arcId == arcId;
        }
        weakCandidates.push_back(candidate);
    }

    if (weakCandidates.empty()) {
        if (out.weakRepairEntered) {
            out.weakRepairCandidateSubtype =
                WeakRepairCandidateSubtype::WRC_ZERO_SAME_POLE_CANDIDATES;
            noteWeakRepairCandidateSample(
                core,
                oldNode,
                H,
                proxyEdge,
                out,
                "proxy weak repair candidate: zero same-pole candidates");
        }
        out.repairOutcome = ProxyArcRepairOutcome::PAR_FAIL_NO_CANDIDATE;
        recordWeakOutcome(out.repairOutcome, false);
        noteSequenceOldArcWeakRepairSample(core,
                                           oldNode,
                                           H,
                                           proxyEdge,
                                           out,
                                           "proxy arc repair: no same-poles candidate for weak repair",
                                           false);
        return failRepair(
            ProxyArcRepairOutcome::PAR_FAIL_NO_CANDIDATE,
            "proxy arc repair: no live candidate after weak poles-only repair");
    }

    if (weakCandidates.size() > 1) {
        if (out.weakRepairEntered) {
            out.weakRepairCandidateSubtype =
                WeakRepairCandidateSubtype::WRC_MULTI_SAME_POLE_CANDIDATES;
            noteWeakRepairCandidateSample(
                core,
                oldNode,
                H,
                proxyEdge,
                out,
                "proxy weak repair candidate: multiple same-pole candidates");
        }
        out.repairOutcome = ProxyArcRepairOutcome::PAR_FAIL_POLES_ONLY_MULTI_CANDIDATE;
        recordWeakOutcome(out.repairOutcome, false);
        noteSequenceOldArcWeakRepairSample(core,
                                           oldNode,
                                           H,
                                           proxyEdge,
                                           out,
                                           "proxy arc repair: multiple same-poles candidates for weak repair",
                                           false);
        return failRepair(
            ProxyArcRepairOutcome::PAR_FAIL_POLES_ONLY_MULTI_CANDIDATE,
            "proxy arc repair: multiple live candidates with same poles");
    }

    const auto &weakCandidate = weakCandidates.front();
    if (out.weakRepairEntered) {
        out.weakRepairCandidateSubtype =
            WeakRepairCandidateSubtype::WRC_ONE_SAME_POLE_CANDIDATE;
        ++gRewriteRStats.seqWeakRepairTentativeSuccessCount;
        noteWeakRepairCandidateSample(core,
                                      oldNode,
                                      H,
                                      proxyEdge,
                                      out,
                                      "proxy weak repair candidate: unique same-pole candidate");
    }
    if (!weakCandidate.slotValid || !weakCandidate.slotAlive || !weakCandidate.slotVirtual ||
        !weakCandidate.slotArcMatches) {
        if (out.weakRepairEntered) {
            out.weakRepairCandidateSubtype = WeakRepairCandidateSubtype::WRC_SLOT_INVALID;
            noteWeakRepairCandidateSample(core,
                                          oldNode,
                                          H,
                                          proxyEdge,
                                          out,
                                          "proxy weak repair candidate: slot invalid");
        }
        out.resolvedArc = weakCandidate.arcId;
        out.resolvedOutsideNode = weakCandidate.outsideNode;
        out.outsideNode = weakCandidate.outsideNode;
        out.resolvedOldSlot = weakCandidate.slotId;
        out.poleA = weakCandidate.poleA;
        out.poleB = weakCandidate.poleB;
        out.repairOutcome = ProxyArcRepairOutcome::PAR_FAIL_POLES_ONLY_SLOT_INVALID;
        recordWeakOutcome(out.repairOutcome, false);
        noteSequenceOldArcWeakRepairSample(core,
                                           oldNode,
                                           H,
                                           proxyEdge,
                                           out,
                                           "proxy arc repair: poles-only candidate slot invalid",
                                           false);
        return failRepair(
            ProxyArcRepairOutcome::PAR_FAIL_POLES_ONLY_SLOT_INVALID,
            "proxy arc repair: poles-only candidate slot invalid");
    }

    out.resolvedArc = weakCandidate.arcId;
    out.outsideNode = weakCandidate.outsideNode;
    out.resolvedOutsideNode = weakCandidate.outsideNode;
    out.resolvedOldSlot = weakCandidate.slotId;
    out.poleA = weakCandidate.poleA;
    out.poleB = weakCandidate.poleB;
    out.repairUsedWeakPolesOnly = true;
    out.repairOutcome = ProxyArcRepairOutcome::PAR_MATCH_BY_POLES_ONLY_UNIQUE;
    recordWeakOutcome(out.repairOutcome, true);
    recordOutcome(out.repairOutcome, true);
    noteSequenceOldArcWeakRepairSample(core,
                                       oldNode,
                                       H,
                                       proxyEdge,
                                       out,
                                       "proxy arc repair: unique same-poles weak repair",
                                       true);
    return true;
}

bool resolveLiveProxyEndpointsForGraftDetailed(
    const ReducedSPQRCore &core,
    NodeId oldNode,
    const CompactGraph &H,
    std::vector<ResolvedProxyEndpoint> &out,
    ResolvedProxyEndpointFailureInfo *failure,
    std::string &why) {
    why.clear();
    out.clear();
    if (failure) *failure = {};

    if (!validActualNodeId(core, oldNode) || !core.nodes[oldNode].alive) {
        if (failure) {
            failure->subtype = GraftRewireBailoutSubtype::GRB_OTHER;
        }
        return fail(why, "resolve proxy snapshot: old node invalid");
    }

    std::unordered_set<ArcId> seenProxyArcs;
    for (const auto &edge : H.edges) {
        if (edge.kind != CompactEdgeKind::PROXY) continue;

        auto failResolve = [&](GraftRewireBailoutSubtype subtype,
                               const std::string &msg,
                               int resolvedOldSlot = -1,
                               NodeId outsideNode = -1) {
            if (failure) {
                failure->subtype = subtype;
                failure->inputEdgeId = edge.id;
                failure->oldArc = edge.oldArc;
                failure->outsideNode = outsideNode;
                failure->resolvedOldSlot = resolvedOldSlot;
            }
            return fail(why, msg);
        };

        GraftRewireBailoutSubtype fallbackSubtype =
            GraftRewireBailoutSubtype::GRB_OTHER;
        if (!validActualArcId(core, edge.oldArc)) {
            fallbackSubtype = GraftRewireBailoutSubtype::GRB_OLDARC_OUT_OF_RANGE;
        } else {
            const auto &oldArc = core.arcs[edge.oldArc];
            if (!oldArc.alive) {
                fallbackSubtype = GraftRewireBailoutSubtype::GRB_OLDARC_DEAD;
            } else {
                const bool oldOnA = oldArc.a == oldNode;
                const bool oldOnB = oldArc.b == oldNode;
                if (oldOnA == oldOnB) {
                    fallbackSubtype =
                        GraftRewireBailoutSubtype::GRB_OLDARC_NOT_INCIDENT_TO_OLDNODE;
                } else {
                    const NodeId outsideNode = oldOnA ? oldArc.b : oldArc.a;
                    if (outsideNode != edge.outsideNode) {
                        fallbackSubtype =
                            GraftRewireBailoutSubtype::GRB_OUTSIDENODE_MISMATCH;
                    }
                }
            }
        }

        RepairedProxyArcInfo resolved;
        if (!resolveLiveProxyArcForSequenceRepair(core, oldNode, H, edge, resolved, why)) {
            GraftRewireBailoutSubtype subtype = fallbackSubtype;
            if (resolved.repairOutcome == ProxyArcRepairOutcome::PAR_FAIL_SLOT_NOT_VIRTUAL) {
                subtype = GraftRewireBailoutSubtype::GRB_OLDSLOT_NOT_VIRTUAL;
            } else if (resolved.repairOutcome ==
                       ProxyArcRepairOutcome::PAR_FAIL_SLOT_ARCID_MISMATCH) {
                subtype = GraftRewireBailoutSubtype::GRB_OLDSLOT_ARCID_MISMATCH;
            } else if (subtype == GraftRewireBailoutSubtype::GRB_OTHER) {
                subtype = GraftRewireBailoutSubtype::GRB_OTHER;
            }
            return failResolve(subtype,
                               why,
                               resolved.resolvedOldSlot,
                               resolved.outsideNode);
        }

        if (!seenProxyArcs.insert(resolved.resolvedArc).second) {
            std::ostringstream oss;
            oss << "resolve proxy snapshot: duplicate oldArc for input " << edge.id;
            return failResolve(GraftRewireBailoutSubtype::GRB_DUPLICATE_OLDARC,
                               oss.str(),
                               resolved.resolvedOldSlot,
                               resolved.outsideNode);
        }

        out.push_back(resolved);
    }

    return true;
}

bool resolveLiveProxyEndpointsForGraft(const ReducedSPQRCore &core,
                                       NodeId oldNode,
                                       const CompactGraph &H,
                                       std::vector<ResolvedProxyEndpoint> &out,
                                       GraftRewireBailoutSubtype *subtype,
                                       std::string &why) {
    ResolvedProxyEndpointFailureInfo failure;
    if (!resolveLiveProxyEndpointsForGraftDetailed(core, oldNode, H, out, &failure, why)) {
        if (subtype) *subtype = failure.subtype;
        return false;
    }
    if (subtype) *subtype = GraftRewireBailoutSubtype::GRB_OTHER;
    return true;
}

XIncidentVirtualSubtype classifyXIncidentVirtualDetailed(const ReducedSPQRCore &core,
                                                         NodeId rNode,
                                                         VertexId x,
                                                         std::string &why) {
    why.clear();

    if (!validActualNodeId(core, rNode) || !core.nodes[rNode].alive) {
        why = "x-incident virtual classify: node invalid";
        return XIncidentVirtualSubtype::XIV_MIXED_OTHER;
    }

    const auto &node = core.nodes[rNode];
    int zeroPayloadCount = 0;
    int positivePayloadCount = 0;
    int invalidMetadataCount = 0;
    int xIncidentVirtualCount = 0;
    bool sharedWithLoop = false;

    for (const auto &slot : node.slots) {
        if (!slot.alive || !slot.isVirtual) continue;
        if (slot.poleA != x && slot.poleB != x) continue;

        ++xIncidentVirtualCount;
        if (slot.poleA == slot.poleB) sharedWithLoop = true;

        if (!validActualArcId(core, slot.arcId) || !core.arcs[slot.arcId].alive) {
            ++invalidMetadataCount;
            continue;
        }
        const auto &arc = core.arcs[slot.arcId];
        const NodeId outsideNode = otherEndpointOfArc(arc, rNode);
        if (!validActualNodeId(core, outsideNode) || !core.nodes[outsideNode].alive) {
            ++invalidMetadataCount;
            continue;
        }

        const Agg sideAgg = computeAggToward(core, rNode, outsideNode);
        if (sideAgg.edgeCnt > 0) {
            ++positivePayloadCount;
        } else {
            ++zeroPayloadCount;
        }
    }

    if (xIncidentVirtualCount == 0) {
        why = "x-incident virtual classify: no x-incident virtual slots";
        return XIncidentVirtualSubtype::XIV_MIXED_OTHER;
    }
    if (sharedWithLoop) {
        why = "x-incident virtual classify: shared with loop";
        return XIncidentVirtualSubtype::XIV_SHARED_WITH_LOOP;
    }
    if (invalidMetadataCount > 0) {
        why = "x-incident virtual classify: mixed other (invalid proxy metadata)";
        return XIncidentVirtualSubtype::XIV_MIXED_OTHER;
    }
    if (positivePayloadCount == 0 && zeroPayloadCount > 0) {
        why = "x-incident virtual classify: zero-payload";
        return XIncidentVirtualSubtype::XIV_ZERO_PAYLOAD;
    }
    if (positivePayloadCount == 1 && zeroPayloadCount == 0) {
        why = "x-incident virtual classify: one positive proxy";
        return XIncidentVirtualSubtype::XIV_ONE_POS_PROXY;
    }
    if (positivePayloadCount >= 2 && zeroPayloadCount == 0) {
        why = "x-incident virtual classify: multiple positive proxies";
        return XIncidentVirtualSubtype::XIV_MULTI_POS_PROXY;
    }

    why = "x-incident virtual classify: mixed other";
    return XIncidentVirtualSubtype::XIV_MIXED_OTHER;
}

bool buildCompactAfterDeletingXForSharedLoop(const ReducedSPQRCore &core,
                                             NodeId rNode,
                                             VertexId x,
                                             CompactGraph &Hafter,
                                             std::string &why) {
    Hafter = {};
    why.clear();

    if (!validActualNodeId(core, rNode) || !core.nodes[rNode].alive) {
        return fail(why, "x-shared-loop: old node invalid");
    }
    const auto &node = core.nodes[rNode];
    if (node.type != SPQRType::R_NODE) {
        return fail(why, "x-shared-loop: old node not R");
    }

    std::string classifyWhy;
    if (classifyXIncidentVirtualDetailed(core, rNode, x, classifyWhy) !=
        XIncidentVirtualSubtype::XIV_SHARED_WITH_LOOP) {
        return fail(why, "x-shared-loop: invalid incident virtual pattern");
    }

    bool xSeen = false;
    std::vector<VertexId> compactVertices;
    compactVertices.reserve(node.slots.size() * 2);
    for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
        const auto &slot = node.slots[slotId];
        if (!slot.alive) continue;
        const bool incidentToX = slot.poleA == x || slot.poleB == x;
        if (incidentToX) {
            xSeen = true;
            continue;
        }
        compactVertices.push_back(slot.poleA);
        compactVertices.push_back(slot.poleB);
    }
    if (!xSeen) {
        return fail(why, "x-shared-loop: invalid incident virtual pattern");
    }

    std::sort(compactVertices.begin(), compactVertices.end());
    compactVertices.erase(std::unique(compactVertices.begin(), compactVertices.end()),
                          compactVertices.end());

    Hafter.block = core.blockId;
    Hafter.ownerR = rNode;
    Hafter.deletedX = x;
    Hafter.origOfCv = compactVertices;
    Hafter.touchedVertices = compactVertices;
    for (int cv = 0; cv < static_cast<int>(compactVertices.size()); ++cv) {
        Hafter.cvOfOrig[compactVertices[cv]] = cv;
    }

    int nextInputId = 0;
    for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
        const auto &slot = node.slots[slotId];
        if (!slot.alive) continue;
        if (slot.poleA == x || slot.poleB == x) continue;

        if (!slot.isVirtual) {
            const auto itA = Hafter.cvOfOrig.find(slot.poleA);
            const auto itB = Hafter.cvOfOrig.find(slot.poleB);
            if (itA == Hafter.cvOfOrig.end() || itB == Hafter.cvOfOrig.end()) {
                return fail(why, "x-shared-loop: REAL endpoint not mapped");
            }
            CompactEdge edge;
            edge.id = nextInputId++;
            edge.kind = CompactEdgeKind::REAL;
            edge.a = itA->second;
            edge.b = itB->second;
            edge.realEdge = slot.realEdge;
            Hafter.edges.push_back(edge);
            continue;
        }

        if (!validActualArcId(core, slot.arcId) || !core.arcs[slot.arcId].alive) {
            return fail(why, "x-shared-loop: invalid PROXY oldArc");
        }
        const auto &arc = core.arcs[slot.arcId];
        const NodeId outsideNode = otherEndpointOfArc(arc, rNode);
        if (!validActualNodeId(core, outsideNode) || !core.nodes[outsideNode].alive) {
            return fail(why, "x-shared-loop: invalid PROXY outsideNode");
        }

        const Agg sideAgg = computeAggToward(core, rNode, outsideNode);
        if (sideAgg.edgeCnt == 0) continue;

        const auto itA = Hafter.cvOfOrig.find(slot.poleA);
        const auto itB = Hafter.cvOfOrig.find(slot.poleB);
        if (itA == Hafter.cvOfOrig.end() || itB == Hafter.cvOfOrig.end()) {
            return fail(why, "x-shared-loop: PROXY endpoint not mapped");
        }

        CompactEdge edge;
        edge.id = nextInputId++;
        edge.kind = CompactEdgeKind::PROXY;
        edge.a = itA->second;
        edge.b = itB->second;
        edge.oldArc = slot.arcId;
        edge.outsideNode = outsideNode;
        edge.oldSlotInU = slotId;
        edge.sideAgg = sideAgg;
        Hafter.edges.push_back(edge);
    }

    return true;
}

XSharedResidualSubtype classifyXSharedResidualAfterDelete(const ReducedSPQRCore &core,
                                                          NodeId rNode,
                                                          VertexId x,
                                                          CompactGraph &Hafter,
                                                          std::string &why) {
    why.clear();
    if (!buildCompactAfterDeletingXForSharedLoop(core, rNode, x, Hafter, why)) {
        return XSharedResidualSubtype::XSR_HAFTER_BUILD_FAIL;
    }

    const size_t edgeCount = Hafter.edges.size();
    if (edgeCount == 0) {
        why = "x-shared residual: after-delete compact graph empty";
        return XSharedResidualSubtype::XSR_HAFTER_EMPTY;
    }

    const TooSmallSubtype tinySubtype = classifyTooSmallCompact(Hafter);
    if (tinySubtype == TooSmallSubtype::TS_ONE_EDGE) {
        why = "x-shared residual: after-delete compact graph one-edge";
        return XSharedResidualSubtype::XSR_HAFTER_ONE_EDGE;
    }
    if (tinySubtype == TooSmallSubtype::TS_TWO_PARALLEL) {
        why = "x-shared residual: after-delete compact graph two-parallel";
        return XSharedResidualSubtype::XSR_HAFTER_TWO_PARALLEL;
    }
    if (tinySubtype == TooSmallSubtype::TS_TWO_PATH) {
        why = "x-shared residual: after-delete compact graph two-path";
        return XSharedResidualSubtype::XSR_HAFTER_TWO_PATH;
    }
    if (tinySubtype == TooSmallSubtype::TS_TWO_OTHER &&
        classifyTooSmallOtherDetailed(Hafter) ==
            TooSmallOtherSubtype::TSO_LOOP_PLUS_EDGE_SHARED) {
        why = "x-shared residual: after-delete compact graph loop+edge-shared";
        return XSharedResidualSubtype::XSR_HAFTER_LOOP_SHARED;
    }

    CompactRejectReason rejectReason = CompactRejectReason::OTHER;
    std::string readyWhy;
    if (isCompactGraphSpqrReady(Hafter, &rejectReason, readyWhy)) {
        why = "x-shared residual: after-delete compact graph spqr-ready";
        return XSharedResidualSubtype::XSR_HAFTER_SPQR_READY;
    }

    CompactBCResult bc;
    std::string bcWhy;
    if (decomposeCompactIntoBC(Hafter, bc, bcWhy)) {
        const NotBiconnectedSubtype nbSubtype = classifyNotBiconnected(Hafter, bc);
        switch (nbSubtype) {
            case NotBiconnectedSubtype::NB_SINGLE_CUT_TWO_BLOCKS:
                why = "x-shared residual: after-delete compact graph single-cut";
                return XSharedResidualSubtype::XSR_HAFTER_SINGLE_CUT;
            case NotBiconnectedSubtype::NB_PATH_OF_BLOCKS:
                why = "x-shared residual: after-delete compact graph path-of-blocks";
                return XSharedResidualSubtype::XSR_HAFTER_PATH_OF_BLOCKS;
            case NotBiconnectedSubtype::NB_DISCONNECTED:
            case NotBiconnectedSubtype::NB_STAR_AROUND_ONE_CUT:
            case NotBiconnectedSubtype::NB_COMPLEX_MULTI_CUT:
            case NotBiconnectedSubtype::NB_BLOCKS_ALL_TINY:
            case NotBiconnectedSubtype::NB_OTHER:
                why = "x-shared residual: after-delete compact graph other not-biconnected";
                return XSharedResidualSubtype::XSR_HAFTER_OTHER_NOT_BICONNECTED;
            case NotBiconnectedSubtype::COUNT:
                break;
        }
    }

    if (!readyWhy.empty()) {
        why = "x-shared residual: " + readyWhy;
    } else if (!bcWhy.empty()) {
        why = "x-shared residual: " + bcWhy;
    } else {
        why = "x-shared residual: after-delete compact graph other";
    }
    return XSharedResidualSubtype::XSR_HAFTER_OTHER;
}

XSharedLoopSharedInputSubtype classifyXSharedLoopSharedInputSubtype(
    const CompactGraph &Hafter) {
    const CompactEdge *loopEdge = nullptr;
    const CompactEdge *nonLoopEdge = nullptr;

    for (const auto &edge : Hafter.edges) {
        if (edge.a >= 0 && edge.b >= 0 && edge.a == edge.b) {
            if (loopEdge != nullptr) return XSharedLoopSharedInputSubtype::XLSI_OTHER;
            loopEdge = &edge;
        } else {
            if (nonLoopEdge != nullptr) return XSharedLoopSharedInputSubtype::XLSI_OTHER;
            nonLoopEdge = &edge;
        }
    }

    if (loopEdge == nullptr || nonLoopEdge == nullptr) {
        return XSharedLoopSharedInputSubtype::XLSI_OTHER;
    }

    const bool loopIsProxy = loopEdge->kind == CompactEdgeKind::PROXY;
    const bool nonLoopIsProxy = nonLoopEdge->kind == CompactEdgeKind::PROXY;
    if (!loopIsProxy && !nonLoopIsProxy) {
        return XSharedLoopSharedInputSubtype::XLSI_REAL_LOOP_REAL_EDGE;
    }
    if (!loopIsProxy && nonLoopIsProxy) {
        return XSharedLoopSharedInputSubtype::XLSI_REAL_LOOP_PROXY_EDGE;
    }
    if (loopIsProxy && !nonLoopIsProxy) {
        return XSharedLoopSharedInputSubtype::XLSI_PROXY_LOOP_REAL_EDGE;
    }
    if (loopIsProxy && nonLoopIsProxy) {
        return XSharedLoopSharedInputSubtype::XLSI_PROXY_LOOP_PROXY_EDGE;
    }
    return XSharedLoopSharedInputSubtype::XLSI_OTHER;
}

XSharedLoopSharedBailout classifyXSharedLoopSharedBailoutFromContext(
    bool builderOk,
    bool chooseKeepOk,
    bool graftOk,
    bool metadataRefreshOk,
    GraftPostcheckSubtype postcheckSubtype,
    bool oracleOk) {
    if (!builderOk) return XSharedLoopSharedBailout::XLSB_BUILDER_FAIL;
    if (!chooseKeepOk) return XSharedLoopSharedBailout::XLSB_CHOOSE_KEEP_FAIL;
    if (!graftOk) return XSharedLoopSharedBailout::XLSB_GRAFT_FAIL;
    if (!metadataRefreshOk) return XSharedLoopSharedBailout::XLSB_METADATA_REFRESH_FAIL;
    if (postcheckSubtype == GraftPostcheckSubtype::GPS_ADJ_METADATA_ONLY) {
        return XSharedLoopSharedBailout::XLSB_POSTCHECK_ADJ_ONLY;
    }
    if (postcheckSubtype == GraftPostcheckSubtype::GPS_SAME_TYPE_SP_ONLY) {
        return XSharedLoopSharedBailout::XLSB_POSTCHECK_SAME_TYPE_SP_ONLY;
    }
    if (postcheckSubtype == GraftPostcheckSubtype::GPS_ADJ_AND_SAME_TYPE_SP) {
        return XSharedLoopSharedBailout::XLSB_POSTCHECK_MIXED;
    }
    if (!oracleOk) return XSharedLoopSharedBailout::XLSB_ORACLE_FAIL;
    return XSharedLoopSharedBailout::XLSB_OTHER;
}

bool stripSelfLoopsForAnalysis(const CompactGraph &H,
                               CompactGraph &remainder,
                               std::vector<int> &removedLoopEdgeIds,
                               std::string &why) {
    remainder = H;
    removedLoopEdgeIds.clear();
    why.clear();

    remainder.edges.clear();
    remainder.edges.reserve(H.edges.size());
    for (const auto &edge : H.edges) {
        if (edge.a < 0 || edge.a >= static_cast<int>(H.origOfCv.size()) ||
            edge.b < 0 || edge.b >= static_cast<int>(H.origOfCv.size())) {
            return fail(why, "self-loop strip: compact edge endpoint out of range");
        }
        if (edge.a == edge.b) {
            removedLoopEdgeIds.push_back(edge.id);
            continue;
        }
        remainder.edges.push_back(edge);
    }
    return true;
}

bool buildStrippedSelfLoopRemainder(const CompactGraph &Hfull,
                                    StrippedSelfLoopRemainder &out,
                                    std::string &why) {
    why.clear();
    out = {};

    std::string subtypeWhy;
    const auto fullSubtype = classifySelfLoopBuildFailDetailed(Hfull, subtypeWhy);
    if (fullSubtype != SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_SPQR_READY &&
        fullSubtype !=
            SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_OTHER_NOT_BICONNECTED) {
        return fail(why,
                    "self-loop remainder spqr-ready: compact shape is not supported");
    }

    CompactGraph stripped;
    std::vector<int> removedLoopEdgeIds;
    std::string stripWhy;
    if (!stripSelfLoopsForAnalysis(Hfull, stripped, removedLoopEdgeIds, stripWhy)) {
        return fail(why,
                    stripWhy.empty()
                        ? "self-loop remainder spqr-ready: strip self-loops failed"
                        : stripWhy);
    }
    if (removedLoopEdgeIds.empty()) {
        return fail(why,
                    "self-loop remainder spqr-ready: expected at least one stripped loop");
    }

    std::vector<const CompactEdge *> strippedLoopEdges;
    strippedLoopEdges.reserve(removedLoopEdgeIds.size());
    for (int removedId : removedLoopEdgeIds) {
        const CompactEdge *loopEdge = nullptr;
        for (const auto &edge : Hfull.edges) {
            if (edge.id == removedId) {
                loopEdge = &edge;
                break;
            }
        }
        if (loopEdge == nullptr) {
            return fail(why,
                        "self-loop remainder spqr-ready: expected stripped loop edge");
        }
        if (loopEdge->kind != CompactEdgeKind::PROXY) {
            return fail(why, "self-loop remainder spqr-ready: stripped loop is not PROXY");
        }
        if (loopEdge->a < 0 || loopEdge->a >= static_cast<int>(Hfull.origOfCv.size()) ||
            loopEdge->b < 0 || loopEdge->b >= static_cast<int>(Hfull.origOfCv.size()) ||
            loopEdge->a != loopEdge->b) {
            return fail(why, "self-loop remainder spqr-ready: invalid stripped loop");
        }
        strippedLoopEdges.push_back(loopEdge);
    }

    out.Hrem = Hfull;
    out.Hrem.edges.clear();
    out.remInputIdToFullInputId.clear();
    out.strippedLoopFullInputIds.clear();
    out.strippedLoopFullInputIds.reserve(strippedLoopEdges.size());
    for (const CompactEdge *loopEdge : strippedLoopEdges) {
        out.strippedLoopFullInputIds.push_back(loopEdge->id);
    }
    out.loopVertex = Hfull.origOfCv[strippedLoopEdges.front()->a];

    for (const auto &edge : Hfull.edges) {
        if (std::find(removedLoopEdgeIds.begin(), removedLoopEdgeIds.end(), edge.id) !=
            removedLoopEdgeIds.end()) {
            continue;
        }
        CompactEdge remEdge = edge;
        remEdge.id = static_cast<int>(out.Hrem.edges.size());
        out.Hrem.edges.push_back(remEdge);
        out.remInputIdToFullInputId.push_back(edge.id);
    }

    if (out.Hrem.edges.empty()) {
        return fail(why, "self-loop remainder spqr-ready: remainder unexpectedly empty");
    }

    if (fullSubtype == SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_SPQR_READY) {
        CompactRejectReason rejectReason = CompactRejectReason::OTHER;
        std::string readyWhy;
        if (!isCompactGraphSpqrReady(out.Hrem, &rejectReason, readyWhy)) {
            return fail(why,
                        readyWhy.empty()
                            ? "self-loop remainder spqr-ready: remainder unexpectedly not spqr-ready"
                            : "self-loop remainder spqr-ready: " + readyWhy);
        }
    }

    return true;
}

bool chooseKeepMiniNodeContainingVertex(const StaticMiniCore &mini,
                                        VertexId v,
                                        int &keep,
                                        std::string &why) {
    struct KeepScore {
        int payloadSize = 0;
        int proxyInputs = 0;
        int realInputs = 0;
        int watched = 0;
        int typeRank = 0;
        int nodeId = -1;
    };

    auto typeRank = [](SPQRType type) {
        switch (type) {
            case SPQRType::R_NODE: return 3;
            case SPQRType::P_NODE: return 2;
            case SPQRType::S_NODE: return 1;
        }
        return 0;
    };

    auto better = [](const KeepScore &lhs, const KeepScore &rhs) {
        if (lhs.payloadSize != rhs.payloadSize) return lhs.payloadSize > rhs.payloadSize;
        if (lhs.proxyInputs != rhs.proxyInputs) return lhs.proxyInputs > rhs.proxyInputs;
        if (lhs.realInputs != rhs.realInputs) return lhs.realInputs > rhs.realInputs;
        if (lhs.watched != rhs.watched) return lhs.watched > rhs.watched;
        if (lhs.typeRank != rhs.typeRank) return lhs.typeRank > rhs.typeRank;
        return lhs.nodeId < rhs.nodeId;
    };

    keep = -1;
    why.clear();

    bool found = false;
    KeepScore best;
    for (int nodeId = 0; nodeId < static_cast<int>(mini.nodes.size()); ++nodeId) {
        const auto &node = mini.nodes[nodeId];
        if (!node.alive) continue;

        bool containsVertex = false;
        KeepScore score;
        score.payloadSize = node.payloadAgg.edgeCnt + node.payloadAgg.vertexCnt;
        score.watched = node.payloadAgg.watchedCnt;
        score.typeRank = typeRank(node.type);
        score.nodeId = nodeId;
        for (const auto &slot : node.slots) {
            if (!slot.alive) continue;
            if (slot.poleA == v || slot.poleB == v) containsVertex = true;
            if (slot.kind == MiniSlotKind::PROXY_INPUT) {
                ++score.proxyInputs;
            } else if (slot.kind == MiniSlotKind::REAL_INPUT) {
                ++score.realInputs;
            }
        }
        if (!containsVertex) continue;

        if (!found || better(score, best)) {
            best = score;
            keep = nodeId;
            found = true;
        }
    }

    if (!found) {
        return fail(why,
                    "self-loop remainder spqr-ready: no mini node contains loop vertex");
    }
    return true;
}

bool liftMiniFromStrippedRemainderToFullInputs(const CompactGraph &Hfull,
                                               const StrippedSelfLoopRemainder &R,
                                               const StaticMiniCore &miniRem,
                                               StaticMiniCore &miniFull,
                                               std::string &why) {
    why.clear();
    miniFull = {};

    if (static_cast<int>(R.remInputIdToFullInputId.size()) !=
        static_cast<int>(R.Hrem.edges.size())) {
        return fail(why,
                    "self-loop remainder spqr-ready: full input remap missing");
    }

    ProjectMiniCore projectMini;
    std::string stageWhy;
    if (!importStaticMiniCore(miniRem, projectMini, stageWhy)) {
        return fail(why,
                    stageWhy.empty()
                        ? "self-loop remainder spqr-ready: importStaticMiniCore failed"
                        : "self-loop remainder spqr-ready: " + stageWhy);
    }

    projectMini.ownerOfInputEdge.assign(Hfull.edges.size(), {-1, -1});
    for (int nodeId = 0; nodeId < static_cast<int>(projectMini.nodes.size()); ++nodeId) {
        auto &node = projectMini.nodes[nodeId];
        for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
            auto &slot = node.slots[slotId];
            if (!slot.alive || slot.kind == MiniSlotKind::INTERNAL_VIRTUAL) continue;
            if (slot.inputEdgeId < 0 ||
                slot.inputEdgeId >= static_cast<int>(R.remInputIdToFullInputId.size())) {
                return fail(why,
                            "self-loop remainder spqr-ready: full input remap missing");
            }
            const int fullInputId = R.remInputIdToFullInputId[slot.inputEdgeId];
            if (fullInputId < 0 || fullInputId >= static_cast<int>(Hfull.edges.size())) {
                return fail(why,
                            "self-loop remainder spqr-ready: full input remap missing");
            }
            slot.inputEdgeId = fullInputId;
            projectMini.ownerOfInputEdge[fullInputId] = {nodeId, slotId};
        }
    }

    for (int remInputId = 0; remInputId < static_cast<int>(R.remInputIdToFullInputId.size());
         ++remInputId) {
        const int fullInputId = R.remInputIdToFullInputId[remInputId];
        if (fullInputId < 0 || fullInputId >= static_cast<int>(projectMini.ownerOfInputEdge.size())) {
            return fail(why,
                        "self-loop remainder spqr-ready: full input remap missing");
        }
        if (projectMini.ownerOfInputEdge[fullInputId].first < 0 ||
            projectMini.ownerOfInputEdge[fullInputId].second < 0) {
            return fail(why,
                        "self-loop remainder spqr-ready: lifted ownerOfInputEdge incomplete");
        }
    }

    return exportProjectMiniCore(projectMini, miniFull, why);
}

bool buildSequenceMiniForSelfLoopRemainderSpqrReady(const CompactGraph &Hfull,
                                                    StaticMiniCore &miniFull,
                                                    int &keep,
                                                    std::string &why) {
    why.clear();
    miniFull = {};
    keep = -1;

    StrippedSelfLoopRemainder stripped;
    if (!buildStrippedSelfLoopRemainder(Hfull, stripped, why)) {
        return false;
    }

    OgdfRawSpqrBackend backend;
    RawSpqrDecomp raw;
    std::string err;
    if (!backend.buildRaw(stripped.Hrem, raw, err)) {
        why = "self-loop remainder spqr-ready: backend.buildRaw failed: " +
              (err.empty() ? std::string("raw backend failed") : err);
        return false;
    }

    std::string stageWhy;
    if (!::validateRawSpqrDecomp(stripped.Hrem, raw, stageWhy)) {
        why = stageWhy.empty()
                ? "self-loop remainder spqr-ready: raw validation failed"
                : "self-loop remainder spqr-ready: raw validation failed: " + stageWhy;
        return false;
    }

    StaticMiniCore miniRem;
    if (!::materializeMiniCore(stripped.Hrem, raw, miniRem, stageWhy)) {
        why = stageWhy.empty()
                ? "self-loop remainder spqr-ready: materializeMiniCore failed"
                : "self-loop remainder spqr-ready: materializeMiniCore failed: " + stageWhy;
        return false;
    }

    try {
        ::normalizeWholeMiniCore(miniRem);
    } catch (const std::exception &e) {
        why = std::string("self-loop remainder spqr-ready: normalizeWholeMiniCore threw: ") +
              e.what();
        return false;
    } catch (...) {
        why = "self-loop remainder spqr-ready: normalizeWholeMiniCore threw";
        return false;
    }

    if (!chooseKeepMiniNodeContainingVertex(miniRem, stripped.loopVertex, keep, why)) {
        return false;
    }

    if (!liftMiniFromStrippedRemainderToFullInputs(Hfull, stripped, miniRem, miniFull, why)) {
        return false;
    }

    ProjectMiniCore projectMini;
    if (!importStaticMiniCore(miniFull, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "self-loop remainder spqr-ready: importStaticMiniCore failed"
                : "self-loop remainder spqr-ready: " + stageWhy;
        return false;
    }
    if (keep < 0 || keep >= static_cast<int>(projectMini.nodes.size()) ||
        !projectMini.nodes[keep].alive) {
        return fail(why, "self-loop remainder spqr-ready: keep node invalid");
    }
    if (stripped.strippedLoopFullInputIds.size() != 1) {
        return fail(why,
                    "self-loop remainder spqr-ready: expected exactly one stripped loop");
    }

    const int loopFullInputId = stripped.strippedLoopFullInputIds.front();
    if (loopFullInputId < 0 || loopFullInputId >= static_cast<int>(Hfull.edges.size())) {
        return fail(why, "self-loop remainder spqr-ready: stripped loop owner missing");
    }
    const auto &fullLoopEdge = Hfull.edges[loopFullInputId];
    if (fullLoopEdge.kind != CompactEdgeKind::PROXY || fullLoopEdge.a != fullLoopEdge.b) {
        return fail(why, "self-loop remainder spqr-ready: stripped loop is not PROXY");
    }

    ProjectMiniSlot loopSlot;
    loopSlot.alive = true;
    loopSlot.kind = MiniSlotKind::PROXY_INPUT;
    loopSlot.inputEdgeId = loopFullInputId;
    loopSlot.poleA = stripped.loopVertex;
    loopSlot.poleB = stripped.loopVertex;
    const int loopSlotId = static_cast<int>(projectMini.nodes[keep].slots.size());
    projectMini.nodes[keep].slots.push_back(loopSlot);
    projectMini.ownerOfInputEdge[loopFullInputId] = {keep, loopSlotId};

    recomputePayloadAgg(Hfull, projectMini);
    projectMini.kind = computeMiniKind(projectMini);
    projectMini.valid = true;

    if (projectMini.ownerOfInputEdge[loopFullInputId].first != keep ||
        projectMini.ownerOfInputEdge[loopFullInputId].second != loopSlotId) {
        return fail(why, "self-loop remainder spqr-ready: stripped loop owner missing");
    }

    const auto &ownedLoopSlot = projectMini.nodes[keep].slots[loopSlotId];
    if (!ownedLoopSlot.alive || ownedLoopSlot.kind != MiniSlotKind::PROXY_INPUT) {
        return fail(why,
                    "self-loop remainder spqr-ready: keep node does not own loop input");
    }

    for (int inputId = 0; inputId < static_cast<int>(projectMini.ownerOfInputEdge.size());
         ++inputId) {
        const auto owner = projectMini.ownerOfInputEdge[inputId];
        if (owner.first < 0 || owner.second < 0) {
            return fail(why,
                        "self-loop remainder spqr-ready: lifted ownerOfInputEdge incomplete");
        }
    }

    if (!checkProjectMiniOwnershipConsistency(Hfull, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "self-loop remainder spqr-ready: lifted ownerOfInputEdge incomplete"
                : "self-loop remainder spqr-ready: " + stageWhy;
        return false;
    }
    if (!checkProjectMiniReducedInvariant(Hfull, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "self-loop remainder spqr-ready: lifted mini reduced invariant failed"
                : "self-loop remainder spqr-ready: " + stageWhy;
        return false;
    }

    return exportProjectMiniCore(projectMini, miniFull, why);
}

bool buildSequenceMiniForSelfLoopRemainderOneEdge(const CompactGraph &Hfull,
                                                  StaticMiniCore &miniFull,
                                                  int &keep,
                                                  std::string &why) {
    why.clear();
    miniFull = {};
    keep = -1;

    std::string subtypeWhy;
    if (classifySelfLoopBuildFailDetailed(Hfull, subtypeWhy) !=
        SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_OTHER_NOT_BICONNECTED) {
        return fail(why,
                    "self-loop tiny remainder: compact shape is not OTHER_NOT_BICONNECTED");
    }

    StrippedSelfLoopRemainder stripped;
    if (!buildStrippedSelfLoopRemainder(Hfull, stripped, why)) {
        return false;
    }

    if (stripped.strippedLoopFullInputIds.empty()) {
        return fail(why, "self-loop tiny remainder: expected at least one stripped loop");
    }
    if (stripped.loopVertex < 0) {
        return fail(why, "self-loop tiny remainder: invalid loop vertex");
    }
    if (classifyTooSmallCompact(stripped.Hrem) != TooSmallSubtype::TS_ONE_EDGE) {
        return fail(why, "self-loop tiny remainder: remainder is not TS_ONE_EDGE");
    }
    if (classifySequenceOneEdgeSubtype(stripped.Hrem) !=
        SequenceOneEdgeSubtype::SOE_REAL_NONLOOP) {
        return fail(why, "self-loop tiny remainder: remainder is not SOE_REAL_NONLOOP");
    }
    if (stripped.Hrem.edges.size() != 1) {
        return fail(why, "self-loop tiny remainder: expected exactly one remainder edge");
    }
    const auto &remainderEdge = stripped.Hrem.edges.front();
    if (remainderEdge.a < 0 ||
        remainderEdge.a >= static_cast<int>(stripped.Hrem.origOfCv.size()) ||
        remainderEdge.b < 0 ||
        remainderEdge.b >= static_cast<int>(stripped.Hrem.origOfCv.size())) {
        return fail(why, "self-loop tiny remainder: invalid remainder edge poles");
    }
    const VertexId remPoleA = stripped.Hrem.origOfCv[remainderEdge.a];
    const VertexId remPoleB = stripped.Hrem.origOfCv[remainderEdge.b];

    for (int loopFullInputId : stripped.strippedLoopFullInputIds) {
        if (loopFullInputId < 0 || loopFullInputId >= static_cast<int>(Hfull.edges.size())) {
            return fail(why, "self-loop tiny remainder: stripped loop owner missing");
        }
        const auto &loopEdge = Hfull.edges[loopFullInputId];
        if (loopEdge.kind != CompactEdgeKind::PROXY) {
            return fail(why, "self-loop tiny remainder: stripped loop is not PROXY");
        }
        if (loopEdge.a < 0 || loopEdge.a >= static_cast<int>(Hfull.origOfCv.size()) ||
            loopEdge.b < 0 || loopEdge.b >= static_cast<int>(Hfull.origOfCv.size()) ||
            loopEdge.a != loopEdge.b) {
            return fail(why, "self-loop tiny remainder: stripped loop is not a loop");
        }
        const VertexId loopVertex = Hfull.origOfCv[loopEdge.a];
        if (loopVertex != remPoleA && loopVertex != remPoleB) {
            return fail(why,
                        "self-loop tiny remainder: stripped loop does not attach to remainder edge");
        }
    }

    StaticMiniCore miniRem;
    std::string stageWhy;
    if (!buildSyntheticMiniForOneEdgeRemainder(stripped.Hrem, miniRem, stageWhy)) {
        why = stageWhy.empty()
                ? "self-loop tiny remainder: one-edge synthetic mini build failed"
                : "self-loop tiny remainder: " + stageWhy;
        return false;
    }

    int aliveNodeCount = 0;
    int aliveNodeId = -1;
    for (int nodeId = 0; nodeId < static_cast<int>(miniRem.nodes.size()); ++nodeId) {
        if (!miniRem.nodes[nodeId].alive) continue;
        ++aliveNodeCount;
        aliveNodeId = nodeId;
    }
    if (aliveNodeCount != 1 || aliveNodeId != 0) {
        return fail(why, "self-loop tiny remainder: unexpected miniRem node count");
    }
    keep = 0;

    if (!liftMiniFromStrippedRemainderToFullInputs(Hfull, stripped, miniRem, miniFull, why)) {
        return false;
    }

    ProjectMiniCore projectMini;
    if (!importStaticMiniCore(miniFull, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "self-loop tiny remainder: importStaticMiniCore failed"
                : "self-loop tiny remainder: " + stageWhy;
        return false;
    }
    if (keep < 0 || keep >= static_cast<int>(projectMini.nodes.size()) ||
        !projectMini.nodes[keep].alive) {
        return fail(why, "self-loop tiny remainder: keep node invalid");
    }

    for (int loopFullInputId : stripped.strippedLoopFullInputIds) {
        const auto &loopEdge = Hfull.edges[loopFullInputId];
        ProjectMiniSlot loopSlot;
        loopSlot.alive = true;
        loopSlot.kind = MiniSlotKind::PROXY_INPUT;
        loopSlot.inputEdgeId = loopFullInputId;
        loopSlot.poleA = Hfull.origOfCv[loopEdge.a];
        loopSlot.poleB = Hfull.origOfCv[loopEdge.b];
        const int loopSlotId = static_cast<int>(projectMini.nodes[keep].slots.size());
        projectMini.nodes[keep].slots.push_back(loopSlot);
        projectMini.ownerOfInputEdge[loopFullInputId] = {keep, loopSlotId};
    }

    recomputePayloadAgg(Hfull, projectMini);
    projectMini.kind = computeMiniKind(projectMini);
    projectMini.valid = true;

    for (int fullInputId = 0; fullInputId < static_cast<int>(Hfull.edges.size()); ++fullInputId) {
        const auto owner = projectMini.ownerOfInputEdge[fullInputId];
        if (owner.first < 0 || owner.second < 0) {
            return fail(why, "self-loop tiny remainder: ownerOfInputEdge incomplete");
        }
        if (owner.first >= static_cast<int>(projectMini.nodes.size()) ||
            owner.second >= static_cast<int>(projectMini.nodes[owner.first].slots.size())) {
            return fail(why, "self-loop tiny remainder: ownerOfInputEdge incomplete");
        }
        const auto &slot = projectMini.nodes[owner.first].slots[owner.second];
        if (!slot.alive || slot.inputEdgeId != fullInputId) {
            return fail(why, "self-loop tiny remainder: ownerOfInputEdge incomplete");
        }
    }

    for (int loopFullInputId : stripped.strippedLoopFullInputIds) {
        const auto owner = projectMini.ownerOfInputEdge[loopFullInputId];
        if (owner.first != keep || owner.second < 0) {
            return fail(why,
                        "self-loop tiny remainder: keep does not own stripped loop input");
        }
        const auto &slot = projectMini.nodes[owner.first].slots[owner.second];
        if (slot.kind != MiniSlotKind::PROXY_INPUT) {
            return fail(why,
                        "self-loop tiny remainder: stripped loop slot is not PROXY_INPUT");
        }
    }

    const int remFullInputId = stripped.remInputIdToFullInputId.front();
    const auto remOwner = projectMini.ownerOfInputEdge[remFullInputId];
    if (remOwner.first < 0 || remOwner.second < 0) {
        return fail(why, "self-loop tiny remainder: ownerOfInputEdge incomplete");
    }
    const auto &remSlot = projectMini.nodes[remOwner.first].slots[remOwner.second];
    if (remSlot.kind != MiniSlotKind::REAL_INPUT) {
        return fail(why, "self-loop tiny remainder: remainder edge is not REAL_INPUT");
    }

    if (!checkProjectMiniOwnershipConsistency(Hfull, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "self-loop tiny remainder: ownerOfInputEdge incomplete"
                : "self-loop tiny remainder: " + stageWhy;
        return false;
    }
    if (!checkProjectMiniReducedInvariant(Hfull, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "self-loop tiny remainder: lifted mini reduced invariant failed"
                : "self-loop tiny remainder: " + stageWhy;
        return false;
    }

    return exportProjectMiniCore(projectMini, miniFull, why);
}

bool buildSequenceMiniForXSharedSpqrReady(const CompactGraph &Hafter,
                                          StaticMiniCore &mini,
                                          int &keep,
                                          std::string &why) {
    why.clear();
    mini = {};
    keep = -1;

    CompactRejectReason rejectReason = CompactRejectReason::OTHER;
    std::string readyWhy;
    if (!isCompactGraphSpqrReady(Hafter, &rejectReason, readyWhy)) {
        return fail(why,
                    readyWhy.empty()
                        ? "x-shared spqr-ready: Hafter is not spqr-ready"
                        : "x-shared spqr-ready: " + readyWhy);
    }

    OgdfRawSpqrBackend backend;
    RawSpqrDecomp raw;
    std::string err;
    if (!backend.buildRaw(Hafter, raw, err)) {
        why = "x-shared spqr-ready: backend.buildRaw failed: " +
              (err.empty() ? std::string("raw backend failed") : err);
        return false;
    }

    std::string stageWhy;
    if (!::validateRawSpqrDecomp(Hafter, raw, stageWhy)) {
        why = stageWhy.empty()
                ? "x-shared spqr-ready: raw validation failed"
                : "x-shared spqr-ready: raw validation failed: " + stageWhy;
        return false;
    }

    if (!::materializeMiniCore(Hafter, raw, mini, stageWhy)) {
        why = stageWhy.empty()
                ? "x-shared spqr-ready: materializeMiniCore failed"
                : "x-shared spqr-ready: materializeMiniCore failed: " + stageWhy;
        return false;
    }

    try {
        ::normalizeWholeMiniCore(mini);
    } catch (const std::exception &e) {
        why = std::string("x-shared spqr-ready: normalizeWholeMiniCore failed: ") + e.what();
        return false;
    } catch (...) {
        why = "x-shared spqr-ready: normalizeWholeMiniCore failed";
        return false;
    }

    if (!::chooseKeepMiniNode(mini, keep, stageWhy) || keep < 0) {
        why = keep < 0
                ? "x-shared spqr-ready: no alive mini node after normalize"
                : stageWhy.empty()
                    ? "x-shared spqr-ready: chooseKeepMiniNode failed"
                    : "x-shared spqr-ready: chooseKeepMiniNode failed: " + stageWhy;
        return false;
    }

    ProjectMiniCore projectMini;
    if (!importStaticMiniCore(mini, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "x-shared spqr-ready: importStaticMiniCore failed"
                : "x-shared spqr-ready: " + stageWhy;
        return false;
    }
    if (!mini.valid || !projectMini.valid) {
        return fail(why, "x-shared spqr-ready: mini not valid after normalize");
    }
    if (!checkProjectMiniOwnershipConsistency(Hafter, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "x-shared spqr-ready: ownerOfInputEdge incomplete"
                : "x-shared spqr-ready: " + stageWhy;
        return false;
    }
    if (!checkProjectMiniReducedInvariant(Hafter, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "x-shared spqr-ready: reduced mini invariant failed"
                : "x-shared spqr-ready: " + stageWhy;
        return false;
    }
    if (keep < 0 || keep >= static_cast<int>(projectMini.nodes.size()) ||
        !projectMini.nodes[keep].alive) {
        return fail(why, "x-shared spqr-ready: keep node invalid");
    }

    return true;
}

SelfLoopBuildFailSubtype classifySelfLoopBuildFailDetailed(const CompactGraph &H,
                                                           std::string &why) {
    why.clear();

    bool hasSelfLoop = false;
    bool hasRealLoop = false;
    bool hasProxyLoop = false;
    for (const auto &edge : H.edges) {
        if (edge.a != edge.b) continue;
        hasSelfLoop = true;
        if (edge.kind == CompactEdgeKind::REAL) {
            hasRealLoop = true;
        } else {
            hasProxyLoop = true;
        }
    }

    if (!hasSelfLoop) {
        why = "self-loop classify: no self-loop";
        return SelfLoopBuildFailSubtype::SL_OTHER;
    }
    if (hasRealLoop && hasProxyLoop) {
        why = "self-loop classify: mixed real/proxy loops";
        return SelfLoopBuildFailSubtype::SL_MIXED_REAL_PROXY_LOOP;
    }
    if (hasRealLoop) {
        why = "self-loop classify: real loop present";
        return SelfLoopBuildFailSubtype::SL_REAL_LOOP_PRESENT;
    }

    CompactGraph remainder;
    std::vector<int> removedLoopEdgeIds;
    std::string stripWhy;
    if (!stripSelfLoopsForAnalysis(H, remainder, removedLoopEdgeIds, stripWhy)) {
        why = stripWhy.empty() ? "self-loop classify: strip failed" : stripWhy;
        return SelfLoopBuildFailSubtype::SL_OTHER;
    }
    (void)removedLoopEdgeIds;

    if (remainder.edges.empty()) {
        why = "self-loop classify: proxy-only, remainder empty";
        return SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_EMPTY;
    }

    CompactRejectReason rejectReason = CompactRejectReason::OTHER;
    std::string readyWhy;
    if (isCompactGraphSpqrReady(remainder, &rejectReason, readyWhy)) {
        why = "self-loop classify: proxy-only, remainder spqr-ready";
        return SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_SPQR_READY;
    }

    if (classifyTooSmallCompact(remainder) == TooSmallSubtype::TS_TWO_PATH) {
        why = "self-loop classify: proxy-only, remainder two-path";
        return SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_TWO_PATH;
    }

    CompactBCResult bc;
    std::string bcWhy;
    if (decomposeCompactIntoBC(remainder, bc, bcWhy)) {
        const auto nbSubtype = classifyNotBiconnected(remainder, bc);
        if (nbSubtype == NotBiconnectedSubtype::NB_SINGLE_CUT_TWO_BLOCKS) {
            why = "self-loop classify: proxy-only, remainder single-cut-two-blocks";
            return SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_SINGLE_CUT;
        }
        if (nbSubtype == NotBiconnectedSubtype::NB_PATH_OF_BLOCKS) {
            why = "self-loop classify: proxy-only, remainder path-of-blocks";
            return SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_PATH_OF_BLOCKS;
        }
        if (nbSubtype == NotBiconnectedSubtype::NB_DISCONNECTED ||
            nbSubtype == NotBiconnectedSubtype::NB_STAR_AROUND_ONE_CUT ||
            nbSubtype == NotBiconnectedSubtype::NB_COMPLEX_MULTI_CUT ||
            nbSubtype == NotBiconnectedSubtype::NB_BLOCKS_ALL_TINY ||
            nbSubtype == NotBiconnectedSubtype::NB_OTHER) {
            why = "self-loop classify: proxy-only, remainder other not-biconnected";
            return SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_OTHER_NOT_BICONNECTED;
        }
    }

    why = readyWhy.empty()
            ? "self-loop classify: proxy-only, remainder other"
            : "self-loop classify: proxy-only, remainder other (" + readyWhy + ")";
    return SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_OTHER;
}

SelfLoopRemainderOtherNBSubtype classifySelfLoopOtherNotBiconnectedDetailed(
    const CompactGraph &Hfull,
    std::string &why) {
    why.clear();

    CompactGraph stripped;
    std::vector<int> removedLoopEdgeIds;
    std::string strippedWhy;
    if (!stripSelfLoopsForAnalysis(Hfull, stripped, removedLoopEdgeIds, strippedWhy)) {
        why = strippedWhy.empty() ? "self-loop residual other-nb: strip remainder failed"
                                  : "self-loop residual other-nb: " + strippedWhy;
        return SelfLoopRemainderOtherNBSubtype::SLNB_OTHER;
    }
    if (removedLoopEdgeIds.empty()) {
        why = "self-loop residual other-nb: no stripped self-loop";
        return SelfLoopRemainderOtherNBSubtype::SLNB_OTHER;
    }
    if (stripped.edges.empty()) {
        why = "self-loop residual other-nb: stripped remainder unexpectedly empty";
        return SelfLoopRemainderOtherNBSubtype::SLNB_OTHER;
    }

    CompactBCResult bc;
    std::string bcWhy;
    if (!decomposeCompactIntoBC(stripped, bc, bcWhy)) {
        why = bcWhy.empty() ? "self-loop residual other-nb: BC decomposition failed"
                            : "self-loop residual other-nb: " + bcWhy;
        return SelfLoopRemainderOtherNBSubtype::SLNB_OTHER;
    }

    std::string validateWhy;
    if (!validateCompactBC(stripped, bc, validateWhy)) {
        why = validateWhy.empty() ? "self-loop residual other-nb: BC validation failed"
                                  : "self-loop residual other-nb: " + validateWhy;
        return SelfLoopRemainderOtherNBSubtype::SLNB_OTHER;
    }

    switch (classifyNotBiconnected(stripped, bc)) {
        case NotBiconnectedSubtype::NB_DISCONNECTED:
            why = "self-loop residual other-nb: disconnected remainder";
            return SelfLoopRemainderOtherNBSubtype::SLNB_DISCONNECTED;
        case NotBiconnectedSubtype::NB_STAR_AROUND_ONE_CUT:
            why = "self-loop residual other-nb: star around one cut";
            return SelfLoopRemainderOtherNBSubtype::SLNB_STAR_AROUND_ONE_CUT;
        case NotBiconnectedSubtype::NB_COMPLEX_MULTI_CUT:
            why = "self-loop residual other-nb: complex multi-cut";
            return SelfLoopRemainderOtherNBSubtype::SLNB_COMPLEX_MULTI_CUT;
        case NotBiconnectedSubtype::NB_BLOCKS_ALL_TINY:
            why = "self-loop residual other-nb: blocks-all-tiny";
            return SelfLoopRemainderOtherNBSubtype::SLNB_BLOCKS_ALL_TINY;
        default:
            break;
    }

    why = "self-loop residual other-nb: other";
    return SelfLoopRemainderOtherNBSubtype::SLNB_OTHER;
}

bool isCompactGraphSpqrReady(const CompactGraph &H,
                             CompactRejectReason *reason,
                             std::string &why) {
    why.clear();
    setRejectReason(reason, CompactRejectReason::OTHER);

    if (H.edges.size() < 3) {
        setRejectReason(reason, CompactRejectReason::TOO_SMALL_FOR_SPQR);
        return fail(why, "rewriteR: compact local graph has fewer than 3 edges");
    }

    const int n = static_cast<int>(H.origOfCv.size());
    if (n < 2) {
        setRejectReason(reason, CompactRejectReason::NOT_BICONNECTED);
        return fail(why, "rewriteR: compact local graph not biconnected");
    }

    std::vector<std::vector<std::pair<int, int>>> adj(n);
    std::vector<int> degree(n, 0);

    for (int edgeIndex = 0; edgeIndex < static_cast<int>(H.edges.size()); ++edgeIndex) {
        const auto &edge = H.edges[edgeIndex];
        if (edge.a < 0 || edge.a >= n || edge.b < 0 || edge.b >= n) {
            return fail(why, "rewriteR: compact local graph endpoint out of range");
        }
        if (edge.a == edge.b) {
            setRejectReason(reason, CompactRejectReason::SELF_LOOP);
            return fail(why, "rewriteR: compact local graph has self-loop");
        }

        adj[edge.a].push_back({edge.b, edgeIndex});
        adj[edge.b].push_back({edge.a, edgeIndex});
        ++degree[edge.a];
        ++degree[edge.b];
    }

    if (n == 2) return true;

    int start = -1;
    for (int v = 0; v < n; ++v) {
        if (degree[v] == 0) {
            setRejectReason(reason, CompactRejectReason::NOT_BICONNECTED);
            return fail(why, "rewriteR: compact local graph not biconnected");
        }
        if (start < 0) start = v;
    }
    if (start < 0) {
        setRejectReason(reason, CompactRejectReason::NOT_BICONNECTED);
        return fail(why, "rewriteR: compact local graph not biconnected");
    }

    std::vector<int> disc(n, -1);
    std::vector<int> low(n, -1);
    std::vector<int> parent(n, -1);
    std::vector<int> parentEdge(n, -1);
    int timer = 0;
    bool articulationFound = false;

    auto dfs = [&](auto &&self, int u) -> void {
        disc[u] = low[u] = timer++;
        int childCount = 0;

        for (const auto &[v, edgeId] : adj[u]) {
            if (disc[v] == -1) {
                parent[v] = u;
                parentEdge[v] = edgeId;
                ++childCount;
                self(self, v);
                low[u] = std::min(low[u], low[v]);

                if (parent[u] == -1) {
                    if (childCount > 1) articulationFound = true;
                } else if (low[v] >= disc[u]) {
                    articulationFound = true;
                }
            } else if (edgeId != parentEdge[u]) {
                low[u] = std::min(low[u], disc[v]);
            }
        }
    };
    dfs(dfs, start);

    for (int v = 0; v < n; ++v) {
        if (disc[v] == -1) {
            setRejectReason(reason, CompactRejectReason::NOT_BICONNECTED);
            return fail(why, "rewriteR: compact local graph not biconnected");
        }
    }
    if (articulationFound) {
        setRejectReason(reason, CompactRejectReason::NOT_BICONNECTED);
        return fail(why, "rewriteR: compact local graph not biconnected");
    }

    return true;
}

bool decomposeCompactIntoBC(const CompactGraph &H,
                            CompactBCResult &out,
                            std::string &why) {
    why.clear();
    out = {};

    const int n = static_cast<int>(H.origOfCv.size());
    std::vector<std::vector<int>> adj(n);
    std::unordered_set<int> seenEdgeIds;
    for (int edgeIndex = 0; edgeIndex < static_cast<int>(H.edges.size()); ++edgeIndex) {
        const auto &edge = H.edges[edgeIndex];
        if (edge.a < 0 || edge.a >= n || edge.b < 0 || edge.b >= n) {
            return fail(why, "compact BC: edge endpoint out of range");
        }
        if (edge.a == edge.b) {
            return fail(why, "compact BC: self-loop unsupported");
        }
        if (!seenEdgeIds.insert(edge.id).second) {
            return fail(why, "compact BC: duplicate compact edge id");
        }
        adj[edge.a].push_back(edgeIndex);
        adj[edge.b].push_back(edgeIndex);
    }

    auto buildBlock = [&](const std::vector<int> &edgeIndices,
                          int blockId) -> bool {
        if (edgeIndices.empty()) {
            return fail(why, "compact BC: empty block emitted");
        }

        CompactBCBlock block;
        std::unordered_set<VertexId> vertexSet;
        for (int edgeIndex : edgeIndices) {
            if (edgeIndex < 0 || edgeIndex >= static_cast<int>(H.edges.size())) {
                return fail(why, "compact BC: block edge index out of range");
            }
            const auto &edge = H.edges[edgeIndex];
            if (!out.blockOfEdge.emplace(edge.id, blockId).second) {
                return fail(why, "compact BC: compact edge assigned to multiple blocks");
            }
            block.edgeIds.push_back(edge.id);
            vertexSet.insert(H.origOfCv[edge.a]);
            vertexSet.insert(H.origOfCv[edge.b]);
            if (edge.kind == CompactEdgeKind::REAL) {
                ++block.realEdgeCnt;
                ++block.payloadEdgeCnt;
            } else {
                ++block.proxyEdgeCnt;
                block.payloadEdgeCnt += std::max(0, edge.sideAgg.edgeCnt);
                block.payloadVertexCnt += std::max(0, edge.sideAgg.vertexCnt);
            }
        }
        block.vertices.assign(vertexSet.begin(), vertexSet.end());
        std::sort(block.edgeIds.begin(), block.edgeIds.end());
        std::sort(block.vertices.begin(), block.vertices.end());
        block.payloadVertexCnt += static_cast<int>(block.vertices.size());
        out.blocks.push_back(std::move(block));
        return true;
    };

    std::vector<int> disc(n, -1);
    std::vector<int> low(n, -1);
    std::vector<int> parent(n, -1);
    std::vector<int> parentEdge(n, -1);
    std::vector<int> edgeStack;
    edgeStack.reserve(H.edges.size());
    int timer = 0;

    auto emitBlockUpTo = [&](int stopEdgeIndex) -> bool {
        std::vector<int> edgesInBlock;
        while (!edgeStack.empty()) {
            const int edgeIndex = edgeStack.back();
            edgeStack.pop_back();
            edgesInBlock.push_back(edgeIndex);
            if (edgeIndex == stopEdgeIndex) break;
        }
        return buildBlock(edgesInBlock, static_cast<int>(out.blocks.size()));
    };

    auto emitRemainingBlock = [&]() -> bool {
        if (edgeStack.empty()) return true;
        std::vector<int> edgesInBlock;
        while (!edgeStack.empty()) {
            edgesInBlock.push_back(edgeStack.back());
            edgeStack.pop_back();
        }
        return buildBlock(edgesInBlock, static_cast<int>(out.blocks.size()));
    };

    auto dfs = [&](auto &&self, int u) -> bool {
        disc[u] = low[u] = timer++;
        for (int edgeIndex : adj[u]) {
            const auto &edge = H.edges[edgeIndex];
            const int v = (edge.a == u) ? edge.b : edge.a;
            if (disc[v] == -1) {
                parent[v] = u;
                parentEdge[v] = edgeIndex;
                edgeStack.push_back(edgeIndex);
                if (!self(self, v)) return false;
                low[u] = std::min(low[u], low[v]);
                if (low[v] >= disc[u]) {
                    if (!emitBlockUpTo(edgeIndex)) return false;
                }
            } else if (edgeIndex != parentEdge[u] && disc[v] < disc[u]) {
                edgeStack.push_back(edgeIndex);
                low[u] = std::min(low[u], disc[v]);
            }
        }
        return true;
    };

    for (int start = 0; start < n; ++start) {
        if (disc[start] != -1 || adj[start].empty()) {
            if (disc[start] == -1 && adj[start].empty()) disc[start] = low[start] = timer++;
            continue;
        }
        if (!dfs(dfs, start)) return false;
        if (!emitRemainingBlock()) return false;
    }

    for (int blockId = 0; blockId < static_cast<int>(out.blocks.size()); ++blockId) {
        for (VertexId vertex : out.blocks[blockId].vertices) {
            out.blocksOfVertex[vertex].push_back(blockId);
        }
    }
    for (auto &[vertex, memberships] : out.blocksOfVertex) {
        std::sort(memberships.begin(), memberships.end());
        memberships.erase(std::unique(memberships.begin(), memberships.end()),
                          memberships.end());
        if (memberships.size() >= 2) out.articulationVertices.push_back(vertex);
    }
    std::sort(out.articulationVertices.begin(), out.articulationVertices.end());

    out.bcAdj.assign(out.blocks.size() + out.articulationVertices.size(), {});
    for (int artIndex = 0; artIndex < static_cast<int>(out.articulationVertices.size()); ++artIndex) {
        const VertexId vertex = out.articulationVertices[artIndex];
        const int artNodeId = static_cast<int>(out.blocks.size()) + artIndex;
        const auto &memberships = out.blocksOfVertex[vertex];
        for (int blockId : memberships) {
            out.bcAdj[blockId].push_back(artNodeId);
            out.bcAdj[artNodeId].push_back(blockId);
        }
    }
    for (auto &neighbors : out.bcAdj) {
        std::sort(neighbors.begin(), neighbors.end());
        neighbors.erase(std::unique(neighbors.begin(), neighbors.end()),
                        neighbors.end());
    }

    out.valid = true;
    return true;
}

bool validateCompactBC(const CompactGraph &H,
                       const CompactBCResult &bc,
                       std::string &why) {
    why.clear();
    if (!bc.valid) return fail(why, "compact BC validate: result marked invalid");

    std::unordered_map<int, const CompactEdge *> edgeById;
    edgeById.reserve(H.edges.size());
    for (const auto &edge : H.edges) {
        if (!edgeById.emplace(edge.id, &edge).second) {
            return fail(why, "compact BC validate: duplicate compact edge id in input");
        }
    }
    if (bc.blockOfEdge.size() != edgeById.size()) {
        return fail(why, "compact BC validate: blockOfEdge size mismatch");
    }

    std::unordered_map<int, int> computedBlockOfEdge;
    std::unordered_map<VertexId, std::vector<int>> computedBlocksOfVertex;
    for (int blockId = 0; blockId < static_cast<int>(bc.blocks.size()); ++blockId) {
        const auto &block = bc.blocks[blockId];
        if (block.edgeIds.empty()) {
            return fail(why, "compact BC validate: block has no edges");
        }

        std::unordered_set<VertexId> vertexSet;
        int realEdgeCnt = 0;
        int proxyEdgeCnt = 0;
        int payloadEdgeCnt = 0;
        int payloadVertexCnt = 0;
        for (int edgeId : block.edgeIds) {
            const auto edgeIt = edgeById.find(edgeId);
            if (edgeIt == edgeById.end()) {
                return fail(why, "compact BC validate: block references unknown edge");
            }
            if (!computedBlockOfEdge.emplace(edgeId, blockId).second) {
                return fail(why, "compact BC validate: compact edge appears in multiple blocks");
            }
            const CompactEdge &edge = *edgeIt->second;
            if (edge.a < 0 || edge.a >= static_cast<int>(H.origOfCv.size()) ||
                edge.b < 0 || edge.b >= static_cast<int>(H.origOfCv.size())) {
                return fail(why, "compact BC validate: edge endpoint out of range");
            }
            vertexSet.insert(H.origOfCv[edge.a]);
            vertexSet.insert(H.origOfCv[edge.b]);
            if (edge.kind == CompactEdgeKind::REAL) {
                ++realEdgeCnt;
                ++payloadEdgeCnt;
            } else {
                ++proxyEdgeCnt;
                payloadEdgeCnt += std::max(0, edge.sideAgg.edgeCnt);
                payloadVertexCnt += std::max(0, edge.sideAgg.vertexCnt);
            }
        }

        std::vector<VertexId> expectedVertices(vertexSet.begin(), vertexSet.end());
        std::sort(expectedVertices.begin(), expectedVertices.end());
        if (expectedVertices != block.vertices) {
            return fail(why, "compact BC validate: block vertex set mismatch");
        }
        payloadVertexCnt += static_cast<int>(expectedVertices.size());
        if (realEdgeCnt != block.realEdgeCnt ||
            proxyEdgeCnt != block.proxyEdgeCnt ||
            payloadEdgeCnt != block.payloadEdgeCnt ||
            payloadVertexCnt != block.payloadVertexCnt) {
            return fail(why, "compact BC validate: block payload summary mismatch");
        }
        for (VertexId vertex : expectedVertices) {
            computedBlocksOfVertex[vertex].push_back(blockId);
        }
        const auto blockOfEdgeIt = bc.blockOfEdge.find(block.edgeIds.front());
        if (blockOfEdgeIt == bc.blockOfEdge.end()) {
            return fail(why, "compact BC validate: blockOfEdge missing representative edge");
        }
    }

    if (computedBlockOfEdge != bc.blockOfEdge) {
        return fail(why, "compact BC validate: blockOfEdge mapping mismatch");
    }

    std::vector<VertexId> computedArticulations;
    for (auto &[vertex, memberships] : computedBlocksOfVertex) {
        std::sort(memberships.begin(), memberships.end());
        memberships.erase(std::unique(memberships.begin(), memberships.end()),
                          memberships.end());
        auto it = bc.blocksOfVertex.find(vertex);
        if (it == bc.blocksOfVertex.end()) {
            return fail(why, "compact BC validate: blocksOfVertex missing vertex");
        }
        auto expectedMemberships = it->second;
        std::sort(expectedMemberships.begin(), expectedMemberships.end());
        expectedMemberships.erase(std::unique(expectedMemberships.begin(), expectedMemberships.end()),
                                  expectedMemberships.end());
        if (memberships != expectedMemberships) {
            return fail(why, "compact BC validate: articulation membership mismatch");
        }
        if (memberships.size() >= 2) computedArticulations.push_back(vertex);
    }
    if (computedBlocksOfVertex.size() != bc.blocksOfVertex.size()) {
        return fail(why, "compact BC validate: blocksOfVertex size mismatch");
    }
    std::sort(computedArticulations.begin(), computedArticulations.end());
    if (computedArticulations != bc.articulationVertices) {
        return fail(why, "compact BC validate: articulation vertex set mismatch");
    }

    if (bc.bcAdj.size() != bc.blocks.size() + bc.articulationVertices.size()) {
        return fail(why, "compact BC validate: bcAdj size mismatch");
    }
    for (int artIndex = 0; artIndex < static_cast<int>(bc.articulationVertices.size()); ++artIndex) {
        const VertexId vertex = bc.articulationVertices[artIndex];
        const int artNodeId = static_cast<int>(bc.blocks.size()) + artIndex;
        auto expectedNeighbors = bc.blocksOfVertex.at(vertex);
        std::sort(expectedNeighbors.begin(), expectedNeighbors.end());
        auto actualNeighbors = bc.bcAdj[artNodeId];
        std::sort(actualNeighbors.begin(), actualNeighbors.end());
        actualNeighbors.erase(std::unique(actualNeighbors.begin(), actualNeighbors.end()),
                              actualNeighbors.end());
        if (actualNeighbors != expectedNeighbors) {
            return fail(why, "compact BC validate: articulation adjacency mismatch");
        }
        for (int blockId : actualNeighbors) {
            if (blockId < 0 || blockId >= static_cast<int>(bc.blocks.size())) {
                return fail(why, "compact BC validate: articulation adjacency block id invalid");
            }
        }
    }
    for (int blockId = 0; blockId < static_cast<int>(bc.blocks.size()); ++blockId) {
        std::vector<int> expectedNeighbors;
        for (VertexId vertex : bc.blocks[blockId].vertices) {
            const auto it = std::find(bc.articulationVertices.begin(),
                                      bc.articulationVertices.end(),
                                      vertex);
            if (it == bc.articulationVertices.end()) continue;
            expectedNeighbors.push_back(static_cast<int>(bc.blocks.size()) +
                                        static_cast<int>(std::distance(bc.articulationVertices.begin(), it)));
        }
        std::sort(expectedNeighbors.begin(), expectedNeighbors.end());
        expectedNeighbors.erase(std::unique(expectedNeighbors.begin(), expectedNeighbors.end()),
                                expectedNeighbors.end());
        auto actualNeighbors = bc.bcAdj[blockId];
        std::sort(actualNeighbors.begin(), actualNeighbors.end());
        actualNeighbors.erase(std::unique(actualNeighbors.begin(), actualNeighbors.end()),
                              actualNeighbors.end());
        if (actualNeighbors != expectedNeighbors) {
            return fail(why, "compact BC validate: block adjacency mismatch");
        }
    }

    return true;
}

NotBiconnectedSubtype classifyNotBiconnected(const CompactGraph &H,
                                             const CompactBCResult &bc) {
    const int components = countCompactConnectedComponents(H);
    if (components > 1) return NotBiconnectedSubtype::NB_DISCONNECTED;
    if (bc.blocks.size() == 2 && bc.articulationVertices.size() == 1) {
        return NotBiconnectedSubtype::NB_SINGLE_CUT_TWO_BLOCKS;
    }
    if (bc.articulationVertices.size() == 1) {
        const auto it = bc.blocksOfVertex.find(bc.articulationVertices.front());
        if (it != bc.blocksOfVertex.end() && it->second.size() > 2) {
            return NotBiconnectedSubtype::NB_STAR_AROUND_ONE_CUT;
        }
    }
    if (isBlockCutPathShape(bc)) return NotBiconnectedSubtype::NB_PATH_OF_BLOCKS;
    if (allCompactBlocksTiny(bc)) return NotBiconnectedSubtype::NB_BLOCKS_ALL_TINY;
    if (bc.articulationVertices.size() >= 2) {
        return NotBiconnectedSubtype::NB_COMPLEX_MULTI_CUT;
    }
    return NotBiconnectedSubtype::NB_OTHER;
}

TooSmallSubtype classifyTooSmallCompact(const CompactGraph &H) {
    const int m = static_cast<int>(H.edges.size());
    if (m == 0) return TooSmallSubtype::TS_EMPTY;
    if (m == 1) return TooSmallSubtype::TS_ONE_EDGE;
    if (m != 2) return TooSmallSubtype::TS_OTHER;

    const auto edgePoles = [&](const CompactEdge &edge) {
        if (edge.a < 0 || edge.a >= static_cast<int>(H.origOfCv.size()) ||
            edge.b < 0 || edge.b >= static_cast<int>(H.origOfCv.size())) {
            return std::pair<VertexId, VertexId>{-1, -1};
        }
        return canonPole(H.origOfCv[edge.a], H.origOfCv[edge.b]);
    };

    const auto p0 = edgePoles(H.edges[0]);
    const auto p1 = edgePoles(H.edges[1]);
    if (p0.first < 0 || p1.first < 0) return TooSmallSubtype::TS_OTHER;
    if (p0 == p1) return TooSmallSubtype::TS_TWO_PARALLEL;

    std::unordered_set<VertexId> vertices = {p0.first, p0.second, p1.first, p1.second};
    if (vertices.size() == 3) return TooSmallSubtype::TS_TWO_PATH;
    if (vertices.size() == 4) return TooSmallSubtype::TS_TWO_DISCONNECTED;
    return TooSmallSubtype::TS_TWO_OTHER;
}

TooSmallOtherSubtype classifyTooSmallOtherDetailed(const CompactGraph &H) {
    if (classifyTooSmallCompact(H) != TooSmallSubtype::TS_TWO_OTHER ||
        H.edges.size() != 2) {
        return TooSmallOtherSubtype::TSO_OTHER;
    }

    struct DetailedEdgeInfo {
        bool valid = false;
        bool loop = false;
        CompactEdgeKind kind = CompactEdgeKind::REAL;
        VertexId poleA = -1;
        VertexId poleB = -1;
    };

    auto infoOf = [&](const CompactEdge &edge) {
        DetailedEdgeInfo info;
        info.kind = edge.kind;
        if (edge.a < 0 || edge.a >= static_cast<int>(H.origOfCv.size()) ||
            edge.b < 0 || edge.b >= static_cast<int>(H.origOfCv.size())) {
            return info;
        }
        info.valid = true;
        info.poleA = H.origOfCv[edge.a];
        info.poleB = H.origOfCv[edge.b];
        info.loop = (info.poleA == info.poleB);
        return info;
    };

    const DetailedEdgeInfo e0 = infoOf(H.edges[0]);
    const DetailedEdgeInfo e1 = infoOf(H.edges[1]);
    if (!e0.valid || !e1.valid) {
        return TooSmallOtherSubtype::TSO_KIND_MIXED_ANOMALY;
    }
    if ((e0.kind != CompactEdgeKind::REAL && e0.kind != CompactEdgeKind::PROXY) ||
        (e1.kind != CompactEdgeKind::REAL && e1.kind != CompactEdgeKind::PROXY)) {
        return TooSmallOtherSubtype::TSO_KIND_MIXED_ANOMALY;
    }

    const int loopCount = static_cast<int>(e0.loop) + static_cast<int>(e1.loop);
    if (loopCount == 2) {
        return (e0.poleA == e1.poleA)
                ? TooSmallOtherSubtype::TSO_TWO_LOOPS_SAME_VERTEX
                : TooSmallOtherSubtype::TSO_TWO_LOOPS_DIFF_VERTEX;
    }
    if (loopCount == 1) {
        const DetailedEdgeInfo &loopEdge = e0.loop ? e0 : e1;
        const DetailedEdgeInfo &nonLoopEdge = e0.loop ? e1 : e0;
        if (nonLoopEdge.loop) {
            return TooSmallOtherSubtype::TSO_KIND_MIXED_ANOMALY;
        }
        return (nonLoopEdge.poleA == loopEdge.poleA ||
                nonLoopEdge.poleB == loopEdge.poleA)
                ? TooSmallOtherSubtype::TSO_LOOP_PLUS_EDGE_SHARED
                : TooSmallOtherSubtype::TSO_LOOP_PLUS_EDGE_DISJOINT;
    }
    if (loopCount == 0) {
        return TooSmallOtherSubtype::TSO_TWO_NONLOOP_UNCLASSIFIED;
    }
    return TooSmallOtherSubtype::TSO_OTHER;
}

SequenceOneEdgeSubtype classifySequenceOneEdgeSubtype(const CompactGraph &H) {
    if (H.edges.size() != 1) {
        return SequenceOneEdgeSubtype::SOE_OTHER;
    }

    const auto &edge = H.edges.front();
    if (edge.a < 0 || edge.a >= static_cast<int>(H.origOfCv.size()) ||
        edge.b < 0 || edge.b >= static_cast<int>(H.origOfCv.size())) {
        return SequenceOneEdgeSubtype::SOE_OTHER;
    }

    const VertexId poleA = H.origOfCv[edge.a];
    const VertexId poleB = H.origOfCv[edge.b];
    const bool loop = (poleA == poleB);

    if (edge.kind == CompactEdgeKind::REAL) {
        return loop ? SequenceOneEdgeSubtype::SOE_REAL_LOOP
                    : SequenceOneEdgeSubtype::SOE_REAL_NONLOOP;
    }
    if (edge.kind == CompactEdgeKind::PROXY) {
        return loop ? SequenceOneEdgeSubtype::SOE_PROXY_LOOP
                    : SequenceOneEdgeSubtype::SOE_PROXY_NONLOOP;
    }
    return SequenceOneEdgeSubtype::SOE_OTHER;
}

bool canHandleSequenceOneEdgeDirect(const CompactGraph &H, std::string &why) {
    why.clear();
    if (H.edges.size() != 1 || classifyTooSmallCompact(H) != TooSmallSubtype::TS_ONE_EDGE) {
        return fail(why, "sequence one-edge direct: expected exactly one tiny edge");
    }

    const auto subtype = classifySequenceOneEdgeSubtype(H);
    if (subtype != SequenceOneEdgeSubtype::SOE_REAL_NONLOOP) {
        std::ostringstream oss;
        oss << "sequence one-edge direct: unsupported subtype "
            << sequenceOneEdgeSubtypeName(subtype);
        return fail(why, oss.str());
    }

    return true;
}

bool buildSyntheticMiniForOneEdgeRemainder(const CompactGraph &H,
                                           StaticMiniCore &mini,
                                           std::string &why) {
    why.clear();
    mini = {};

    if (H.edges.size() != 1 || classifyTooSmallCompact(H) != TooSmallSubtype::TS_ONE_EDGE) {
        return fail(why, "one-edge mini: expected exactly one edge");
    }

    const auto &edge = H.edges[0];
    if (edge.kind != CompactEdgeKind::REAL) {
        return fail(why, "one-edge mini: remaining edge is not REAL in first pass");
    }
    if (edge.id < 0 || edge.id >= static_cast<int>(H.edges.size())) {
        return fail(why, "one-edge mini: ownerOfInputEdge incomplete");
    }
    if (edge.a < 0 || edge.a >= static_cast<int>(H.origOfCv.size()) ||
        edge.b < 0 || edge.b >= static_cast<int>(H.origOfCv.size())) {
        return fail(why, "one-edge mini: expected exactly one edge");
    }

    ProjectMiniCore projectMini;
    projectMini.valid = true;
    projectMini.kind = CoreKind::TINY;
    projectMini.nodes.resize(1);
    projectMini.ownerOfInputEdge.assign(H.edges.size(), {-1, -1});

    auto &node = projectMini.nodes[0];
    node.alive = true;
    node.type = SPQRType::R_NODE;
    node.localAgg = {};
    node.payloadAgg = {};

    ProjectMiniSlot slot;
    slot.alive = true;
    slot.kind = MiniSlotKind::REAL_INPUT;
    slot.poleA = H.origOfCv[edge.a];
    slot.poleB = H.origOfCv[edge.b];
    slot.inputEdgeId = edge.id;
    slot.realEdge = edge.realEdge;
    node.slots.push_back(slot);
    projectMini.ownerOfInputEdge[edge.id] = {0, 0};

    if (projectMini.ownerOfInputEdge.size() != H.edges.size() ||
        projectMini.ownerOfInputEdge[edge.id].first != 0 ||
        projectMini.ownerOfInputEdge[edge.id].second != 0) {
        return fail(why, "one-edge mini: ownerOfInputEdge incomplete");
    }
    const int aliveNodeCount = projectMini.nodes[0].alive ? 1 : 0;
    int aliveSlotCount = 0;
    for (const auto &liveSlot : projectMini.nodes[0].slots) {
        if (liveSlot.alive) ++aliveSlotCount;
    }
    if (aliveNodeCount != 1) {
        return fail(why, "one-edge mini: unexpected alive node count");
    }
    if (aliveSlotCount != 1) {
        return fail(why, "one-edge mini: unexpected alive slot count");
    }
    if (projectMini.nodes[0].slots[0].kind != MiniSlotKind::REAL_INPUT) {
        return fail(why, "one-edge mini: slot kind mismatch");
    }

    recomputePayloadAgg(H, projectMini);
    projectMini.kind = computeMiniKind(projectMini);
    projectMini.valid = true;

    std::string stageWhy;
    if (!checkProjectMiniOwnershipConsistency(H, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "one-edge mini: ownerOfInputEdge incomplete"
                : "one-edge mini: " + stageWhy;
        return false;
    }
    if (!checkProjectMiniReducedInvariant(H, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "one-edge mini: synthetic mini reduced invariant failed"
                : "one-edge mini: " + stageWhy;
        return false;
    }

    return exportProjectMiniCore(projectMini, mini, why);
}

bool buildSyntheticMiniForLoopPlusEdgeShared(const CompactGraph &H,
                                             StaticMiniCore &mini,
                                             std::string &why) {
    why.clear();
    mini = {};

    if (classifyTooSmallCompact(H) != TooSmallSubtype::TS_TWO_OTHER ||
        classifyTooSmallOtherDetailed(H) != TooSmallOtherSubtype::TSO_LOOP_PLUS_EDGE_SHARED ||
        H.edges.size() != 2) {
        return fail(why, "loop+edge-shared mini: shared cut not found");
    }

    auto edgePoles = [&](const CompactEdge &edge) {
        if (edge.a < 0 || edge.a >= static_cast<int>(H.origOfCv.size()) ||
            edge.b < 0 || edge.b >= static_cast<int>(H.origOfCv.size())) {
            return std::pair<VertexId, VertexId>{-1, -1};
        }
        return std::pair<VertexId, VertexId>{H.origOfCv[edge.a], H.origOfCv[edge.b]};
    };

    int loopEdgeIndex = -1;
    int nonLoopEdgeIndex = -1;
    VertexId cut = -1;
    VertexId other = -1;
    for (int edgeIndex = 0; edgeIndex < 2; ++edgeIndex) {
        const auto poles = edgePoles(H.edges[edgeIndex]);
        if (poles.first < 0 || poles.second < 0) {
            return fail(why, "loop+edge-shared mini: shared cut not found");
        }
        if (poles.first == poles.second) {
            if (loopEdgeIndex >= 0) {
                return fail(why, "loop+edge-shared mini: missing non-loop edge");
            }
            loopEdgeIndex = edgeIndex;
            cut = poles.first;
        } else {
            if (nonLoopEdgeIndex >= 0) {
                return fail(why, "loop+edge-shared mini: missing loop edge");
            }
            nonLoopEdgeIndex = edgeIndex;
        }
    }
    if (loopEdgeIndex < 0) {
        return fail(why, "loop+edge-shared mini: missing loop edge");
    }
    if (nonLoopEdgeIndex < 0) {
        return fail(why, "loop+edge-shared mini: missing non-loop edge");
    }

    const auto nonLoopPoles = edgePoles(H.edges[nonLoopEdgeIndex]);
    if (nonLoopPoles.first == nonLoopPoles.second) {
        return fail(why, "loop+edge-shared mini: missing non-loop edge");
    }
    if (nonLoopPoles.first == cut) {
        other = nonLoopPoles.second;
    } else if (nonLoopPoles.second == cut) {
        other = nonLoopPoles.first;
    } else {
        return fail(why, "loop+edge-shared mini: shared cut not found");
    }
    if (other < 0) {
        return fail(why, "loop+edge-shared mini: shared cut not found");
    }

    ProjectMiniCore projectMini;
    projectMini.valid = true;
    projectMini.kind = CoreKind::REDUCED_SPQR;
    projectMini.nodes.resize(2);
    projectMini.arcs.resize(1);
    projectMini.ownerOfInputEdge.assign(H.edges.size(), {-1, -1});
    for (auto &node : projectMini.nodes) {
        node.alive = true;
        node.type = SPQRType::R_NODE;
        node.localAgg = {};
        node.payloadAgg = {};
    }

    auto addInputSlot = [&](int nodeId, const CompactEdge &edge) -> bool {
        if (edge.id < 0 || edge.id >= static_cast<int>(H.edges.size())) {
            return fail(why, "loop+edge-shared mini: ownerOfInputEdge incomplete");
        }
        const auto poles = edgePoles(edge);
        if (poles.first < 0 || poles.second < 0) {
            return fail(why, "loop+edge-shared mini: shared cut not found");
        }
        ProjectMiniSlot slot;
        slot.alive = true;
        slot.poleA = poles.first;
        slot.poleB = poles.second;
        slot.inputEdgeId = edge.id;
        if (edge.kind == CompactEdgeKind::REAL) {
            slot.kind = MiniSlotKind::REAL_INPUT;
            slot.realEdge = edge.realEdge;
        } else {
            slot.kind = MiniSlotKind::PROXY_INPUT;
        }
        auto &node = projectMini.nodes[nodeId];
        const int slotId = static_cast<int>(node.slots.size());
        node.slots.push_back(slot);
        projectMini.ownerOfInputEdge[edge.id] = {nodeId, slotId};
        return true;
    };

    if (!addInputSlot(0, H.edges[nonLoopEdgeIndex])) return false;
    if (!addInputSlot(1, H.edges[loopEdgeIndex])) return false;

    std::vector<int> articulationSlotOfNode(2, -1);
    for (int nodeId = 0; nodeId < 2; ++nodeId) {
        ProjectMiniSlot slot;
        slot.alive = true;
        slot.kind = MiniSlotKind::INTERNAL_VIRTUAL;
        slot.poleA = cut;
        slot.poleB = cut;
        slot.miniArcId = 0;
        articulationSlotOfNode[nodeId] =
            static_cast<int>(projectMini.nodes[nodeId].slots.size());
        projectMini.nodes[nodeId].slots.push_back(slot);
        projectMini.nodes[nodeId].adjArcs.push_back(0);
    }

    ProjectMiniArc arc;
    arc.alive = true;
    arc.a = 0;
    arc.b = 1;
    arc.slotInA = articulationSlotOfNode[0];
    arc.slotInB = articulationSlotOfNode[1];
    arc.poleA = cut;
    arc.poleB = cut;
    projectMini.arcs[0] = arc;

    for (int inputId = 0; inputId < static_cast<int>(projectMini.ownerOfInputEdge.size()); ++inputId) {
        const auto owner = projectMini.ownerOfInputEdge[inputId];
        if (owner.first < 0 || owner.second < 0) {
            return fail(why, "loop+edge-shared mini: ownerOfInputEdge incomplete");
        }
    }

    for (int nodeId = 0; nodeId < 2; ++nodeId) {
        const auto &node = projectMini.nodes[nodeId];
        int aliveSlotCount = 0;
        int inputSlotCount = 0;
        int virtualSlotCount = 0;
        for (const auto &slot : node.slots) {
            if (!slot.alive) continue;
            ++aliveSlotCount;
            if (slot.kind == MiniSlotKind::INTERNAL_VIRTUAL) {
                ++virtualSlotCount;
            } else if (slot.kind == MiniSlotKind::REAL_INPUT ||
                       slot.kind == MiniSlotKind::PROXY_INPUT) {
                ++inputSlotCount;
            }
        }
        if (aliveSlotCount != 2 || inputSlotCount != 1 || virtualSlotCount != 1) {
            return fail(why, "loop+edge-shared mini: articulation virtual slot missing");
        }
        if (node.slots[articulationSlotOfNode[nodeId]].kind != MiniSlotKind::INTERNAL_VIRTUAL) {
            return fail(why, "loop+edge-shared mini: articulation virtual slot missing");
        }
    }

    recomputePayloadAgg(H, projectMini);
    projectMini.kind = computeMiniKind(projectMini);
    projectMini.valid = true;

    std::string stageWhy;
    if (!checkProjectMiniOwnershipConsistency(H, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "loop+edge-shared mini: ownerOfInputEdge incomplete"
                : "loop+edge-shared mini: " + stageWhy;
        return false;
    }
    if (!checkProjectMiniReducedInvariant(H, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "loop+edge-shared mini: synthetic mini reduced invariant failed"
                : "loop+edge-shared mini: " + stageWhy;
        return false;
    }

    return exportProjectMiniCore(projectMini, mini, why);
}

bool buildSyntheticMiniForSelfLoopRemainderTwoPath(const CompactGraph &H,
                                                   StaticMiniCore &mini,
                                                   std::string &why) {
    why.clear();
    mini = {};

    std::string ignoredWhy;
    if (classifySelfLoopBuildFailDetailed(H, ignoredWhy) !=
        SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_TWO_PATH) {
        return fail(why, "self-loop remainder two-path: remainder is not TS_TWO_PATH");
    }

    CompactGraph remainder;
    std::vector<int> removedLoopEdgeIds;
    std::string stripWhy;
    if (!stripSelfLoopsForAnalysis(H, remainder, removedLoopEdgeIds, stripWhy)) {
        return fail(why, stripWhy.empty()
                             ? "self-loop remainder two-path: strip self-loops failed"
                             : stripWhy);
    }
    if (removedLoopEdgeIds.size() != 1) {
        return fail(why, "self-loop remainder two-path: expected exactly one stripped loop");
    }
    if (classifyTooSmallCompact(remainder) != TooSmallSubtype::TS_TWO_PATH ||
        remainder.edges.size() != 2) {
        return fail(why, "self-loop remainder two-path: remainder is not TS_TWO_PATH");
    }

    const CompactEdge *loopEdge = nullptr;
    for (const auto &edge : H.edges) {
        if (edge.id == removedLoopEdgeIds.front()) {
            loopEdge = &edge;
            break;
        }
    }
    if (loopEdge == nullptr) {
        return fail(why, "self-loop remainder two-path: expected exactly one stripped loop");
    }
    if (loopEdge->id < 0 || loopEdge->id >= static_cast<int>(H.edges.size())) {
        return fail(why, "self-loop remainder two-path: ownerOfInputEdge incomplete");
    }
    if (loopEdge->kind != CompactEdgeKind::PROXY) {
        return fail(why, "self-loop remainder two-path: stripped loop is not PROXY");
    }
    if (loopEdge->a < 0 || loopEdge->a >= static_cast<int>(H.origOfCv.size()) ||
        loopEdge->b < 0 || loopEdge->b >= static_cast<int>(H.origOfCv.size()) ||
        loopEdge->a != loopEdge->b) {
        return fail(why, "self-loop remainder two-path: shared cut not found");
    }

    auto edgePoles = [](const CompactGraph &graph, const CompactEdge &edge) {
        if (edge.a < 0 || edge.a >= static_cast<int>(graph.origOfCv.size()) ||
            edge.b < 0 || edge.b >= static_cast<int>(graph.origOfCv.size())) {
            return std::pair<VertexId, VertexId>{-1, -1};
        }
        return std::pair<VertexId, VertexId>{graph.origOfCv[edge.a], graph.origOfCv[edge.b]};
    };

    const auto poles0 = edgePoles(remainder, remainder.edges[0]);
    const auto poles1 = edgePoles(remainder, remainder.edges[1]);
    std::unordered_set<VertexId> shared;
    for (VertexId v0 : {poles0.first, poles0.second}) {
        for (VertexId v1 : {poles1.first, poles1.second}) {
            if (v0 == v1 && v0 >= 0) shared.insert(v0);
        }
    }
    if (shared.size() != 1) {
        return fail(why, "self-loop remainder two-path: shared cut not found");
    }
    const VertexId cut = *shared.begin();
    const VertexId loopVertex = H.origOfCv[loopEdge->a];
    const VertexId attachVertex = loopVertex;
    std::vector<int> incidentPathEdgeIds;
    for (const auto &edge : remainder.edges) {
        const auto poles = edgePoles(remainder, edge);
        if (poles.first == loopVertex || poles.second == loopVertex) {
            incidentPathEdgeIds.push_back(edge.id);
        }
    }
    if (incidentPathEdgeIds.empty()) {
        return fail(why, "self-loop remainder two-path: loop vertex not incident to remainder path");
    }

    CompactGraph normalizedRemainder = remainder;
    std::vector<int> originalInputIds;
    originalInputIds.reserve(normalizedRemainder.edges.size());
    for (int edgeIndex = 0; edgeIndex < static_cast<int>(normalizedRemainder.edges.size()); ++edgeIndex) {
        originalInputIds.push_back(normalizedRemainder.edges[edgeIndex].id);
        normalizedRemainder.edges[edgeIndex].id = edgeIndex;
    }

    StaticMiniCore baseMiniStatic;
    std::string stageWhy;
    if (!buildSyntheticMiniForTooSmallTwoPath(normalizedRemainder, baseMiniStatic, stageWhy)) {
        why = stageWhy.empty()
                ? "self-loop remainder two-path: base two-path builder failed"
                : "self-loop remainder two-path: " + stageWhy;
        return false;
    }

    ProjectMiniCore projectMini;
    if (!importStaticMiniCore(baseMiniStatic, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "self-loop remainder two-path: importStaticMiniCore failed"
                : "self-loop remainder two-path: " + stageWhy;
        return false;
    }
    if (projectMini.nodes.size() != 2 || projectMini.arcs.size() != 1 ||
        originalInputIds.size() != 2) {
        return fail(why, "self-loop remainder two-path: ownerOfInputEdge incomplete");
    }

    projectMini.ownerOfInputEdge.assign(H.edges.size(), {-1, -1});
    for (int nodeId = 0; nodeId < static_cast<int>(projectMini.nodes.size()); ++nodeId) {
        auto &node = projectMini.nodes[nodeId];
        for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
            auto &slot = node.slots[slotId];
            if (!slot.alive || slot.kind == MiniSlotKind::INTERNAL_VIRTUAL) continue;
            if (slot.inputEdgeId < 0 ||
                slot.inputEdgeId >= static_cast<int>(originalInputIds.size())) {
                return fail(why, "self-loop remainder two-path: ownerOfInputEdge incomplete");
            }
            const int originalInputId = originalInputIds[slot.inputEdgeId];
            if (originalInputId < 0 || originalInputId >= static_cast<int>(H.edges.size())) {
                return fail(why, "self-loop remainder two-path: ownerOfInputEdge incomplete");
            }
            slot.inputEdgeId = originalInputId;
            projectMini.ownerOfInputEdge[originalInputId] = {nodeId, slotId};
        }
    }

    int hubNodeId = -1;
    int hubInputId = -1;
    if (incidentPathEdgeIds.size() == 1) {
        hubInputId = incidentPathEdgeIds.front();
    } else if (incidentPathEdgeIds.size() == 2 && loopVertex == cut) {
        hubInputId = std::min(incidentPathEdgeIds[0], incidentPathEdgeIds[1]);
    } else {
        return fail(why, "self-loop remainder two-path: loop vertex not incident to remainder path");
    }
    if (hubInputId >= 0 &&
        hubInputId < static_cast<int>(projectMini.ownerOfInputEdge.size())) {
        hubNodeId = projectMini.ownerOfInputEdge[hubInputId].first;
    }
    if (hubNodeId < 0 || hubNodeId >= static_cast<int>(projectMini.nodes.size())) {
        hubNodeId = 0;
    }

    const int loopNodeId = static_cast<int>(projectMini.nodes.size());
    projectMini.nodes.emplace_back();
    auto &loopNode = projectMini.nodes.back();
    loopNode.alive = true;
    loopNode.type = SPQRType::R_NODE;
    loopNode.localAgg = {};
    loopNode.payloadAgg = {};

    ProjectMiniSlot loopInputSlot;
    loopInputSlot.alive = true;
    loopInputSlot.kind = MiniSlotKind::PROXY_INPUT;
    loopInputSlot.poleA = attachVertex;
    loopInputSlot.poleB = attachVertex;
    loopInputSlot.inputEdgeId = loopEdge->id;
    const int loopInputSlotId = static_cast<int>(loopNode.slots.size());
    loopNode.slots.push_back(loopInputSlot);
    projectMini.ownerOfInputEdge[loopEdge->id] = {loopNodeId, loopInputSlotId};

    const int newArcId = static_cast<int>(projectMini.arcs.size());

    ProjectMiniSlot hubVirtualSlot;
    hubVirtualSlot.alive = true;
    hubVirtualSlot.kind = MiniSlotKind::INTERNAL_VIRTUAL;
    hubVirtualSlot.poleA = attachVertex;
    hubVirtualSlot.poleB = attachVertex;
    hubVirtualSlot.miniArcId = newArcId;
    const int hubVirtualSlotId =
        static_cast<int>(projectMini.nodes[hubNodeId].slots.size());
    projectMini.nodes[hubNodeId].slots.push_back(hubVirtualSlot);
    projectMini.nodes[hubNodeId].adjArcs.push_back(newArcId);

    ProjectMiniSlot loopVirtualSlot;
    loopVirtualSlot.alive = true;
    loopVirtualSlot.kind = MiniSlotKind::INTERNAL_VIRTUAL;
    loopVirtualSlot.poleA = attachVertex;
    loopVirtualSlot.poleB = attachVertex;
    loopVirtualSlot.miniArcId = newArcId;
    const int loopVirtualSlotId = static_cast<int>(loopNode.slots.size());
    loopNode.slots.push_back(loopVirtualSlot);
    loopNode.adjArcs.push_back(newArcId);

    ProjectMiniArc arc;
    arc.alive = true;
    arc.a = hubNodeId;
    arc.b = loopNodeId;
    arc.slotInA = hubVirtualSlotId;
    arc.slotInB = loopVirtualSlotId;
    arc.poleA = attachVertex;
    arc.poleB = attachVertex;
    projectMini.arcs.push_back(arc);

    for (int inputId = 0; inputId < static_cast<int>(projectMini.ownerOfInputEdge.size()); ++inputId) {
        const auto owner = projectMini.ownerOfInputEdge[inputId];
        if (owner.first < 0 || owner.second < 0) {
            return fail(why, "self-loop remainder two-path: ownerOfInputEdge incomplete");
        }
    }

    const auto &finalLoopNode = projectMini.nodes[loopNodeId];
    int loopInputCount = 0;
    int loopVirtualCount = 0;
    for (const auto &slot : finalLoopNode.slots) {
        if (!slot.alive) continue;
        if (slot.kind == MiniSlotKind::INTERNAL_VIRTUAL) {
            ++loopVirtualCount;
        } else {
            ++loopInputCount;
        }
    }
    if (static_cast<int>(finalLoopNode.slots.size()) != 2 ||
        loopInputCount != 1 ||
        loopVirtualCount != 1) {
        return fail(why, "self-loop remainder two-path: loop node virtual slot missing");
    }
    if (projectMini.nodes[hubNodeId].slots[hubVirtualSlotId].kind !=
            MiniSlotKind::INTERNAL_VIRTUAL ||
        finalLoopNode.slots[loopVirtualSlotId].kind !=
            MiniSlotKind::INTERNAL_VIRTUAL) {
        return fail(why, "self-loop remainder two-path: loop node virtual slot missing");
    }

    recomputePayloadAgg(H, projectMini);
    projectMini.kind = computeMiniKind(projectMini);
    projectMini.valid = true;

    if (!checkProjectMiniOwnershipConsistency(H, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "self-loop remainder two-path: ownerOfInputEdge incomplete"
                : "self-loop remainder two-path: " + stageWhy;
        return false;
    }
    if (!checkProjectMiniReducedInvariant(H, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "self-loop remainder two-path: synthetic mini reduced invariant failed"
                : "self-loop remainder two-path: " + stageWhy;
        return false;
    }

    return exportProjectMiniCore(projectMini, mini, why);
}

bool applySequenceLoopSharedProxyLoopRealInPlaceDetailed(ReducedSPQRCore &core,
                                                         NodeId oldNode,
                                                         const CompactGraph &Hafter,
                                                         GraftTrace &trace,
                                                         std::string &why) {
    why.clear();
    trace = {};

    auto publishTrace = [&]() {
        publishRewriteRSequenceReplayTrace(trace);
    };
    auto failWithTrace = [&](const std::string &msg) {
        if (!msg.empty()) why = msg;
        trace.graftOtherWhy = why;
        publishTrace();
        return false;
    };
    auto edgePoles = [&](const CompactEdge &edge) {
        if (edge.a < 0 || edge.a >= static_cast<int>(Hafter.origOfCv.size()) ||
            edge.b < 0 || edge.b >= static_cast<int>(Hafter.origOfCv.size())) {
            return std::pair<VertexId, VertexId>{-1, -1};
        }
        return std::pair<VertexId, VertexId>{Hafter.origOfCv[edge.a], Hafter.origOfCv[edge.b]};
    };

    if (!gRewriteRCaseContext.sequenceMode) {
        return failWithTrace("seq loopshared in-place: sequence mode only");
    }
    if (!validActualNodeId(core, oldNode) || !core.nodes[oldNode].alive) {
        return failWithTrace("seq loopshared in-place: old node invalid");
    }
    if (Hafter.edges.size() != 2 ||
        classifyXSharedLoopSharedInputSubtype(Hafter) !=
            XSharedLoopSharedInputSubtype::XLSI_PROXY_LOOP_REAL_EDGE) {
        return failWithTrace("seq loopshared in-place: unsupported compact shape");
    }

    int loopEdgeIndex = -1;
    int realEdgeIndex = -1;
    VertexId cut = -1;
    VertexId other = -1;
    for (int edgeIndex = 0; edgeIndex < static_cast<int>(Hafter.edges.size()); ++edgeIndex) {
        const auto &edge = Hafter.edges[edgeIndex];
        const auto poles = edgePoles(edge);
        if (poles.first < 0 || poles.second < 0) {
            return failWithTrace("seq loopshared in-place: invalid compact poles");
        }
        if (poles.first == poles.second) {
            if (edge.kind != CompactEdgeKind::PROXY || loopEdgeIndex >= 0) {
                return failWithTrace("seq loopshared in-place: missing proxy loop edge");
            }
            loopEdgeIndex = edgeIndex;
            cut = poles.first;
            continue;
        }
        if (edge.kind != CompactEdgeKind::REAL || realEdgeIndex >= 0) {
            return failWithTrace("seq loopshared in-place: missing real edge");
        }
        realEdgeIndex = edgeIndex;
        if (poles.first == cut) {
            other = poles.second;
        } else if (poles.second == cut) {
            other = poles.first;
        }
    }

    if (loopEdgeIndex < 0) {
        return failWithTrace("seq loopshared in-place: missing proxy loop edge");
    }
    if (realEdgeIndex < 0) {
        return failWithTrace("seq loopshared in-place: missing real edge");
    }

    const auto &loopEdge = Hafter.edges[loopEdgeIndex];
    const auto &realEdge = Hafter.edges[realEdgeIndex];
    const auto realPoles = edgePoles(realEdge);
    if (realPoles.first == realPoles.second || cut < 0 ||
        (realPoles.first != cut && realPoles.second != cut)) {
        return failWithTrace("seq loopshared in-place: invalid shared cut");
    }
    if (other < 0) {
        other = realPoles.first == cut ? realPoles.second : realPoles.first;
    }

    trace.loopInputEdgeId = loopEdge.id;
    trace.realInputEdgeId = realEdge.id;
    trace.loopSharedCutVertex = cut;

    ResolvedProxyEndpointFailureInfo resolveFailure;
    std::vector<ResolvedProxyEndpoint> resolvedProxyEndpoints;
    if (!resolveLiveProxyEndpointsForGraftDetailed(core,
                                                   oldNode,
                                                   Hafter,
                                                   resolvedProxyEndpoints,
                                                   &resolveFailure,
                                                   why)) {
        ++gRewriteRStats.seqResolvedProxySnapshotFailCount;
        trace.graftRewireSubtype = resolveFailure.subtype;
        trace.failingInputEdge = resolveFailure.inputEdgeId;
        trace.failingOldArc = resolveFailure.oldArc;
        trace.resolvedProxyEndpoints = std::move(resolvedProxyEndpoints);
        return failWithTrace(why);
    }
    ++gRewriteRStats.seqResolvedProxySnapshotCount;
    trace.resolvedProxyEndpoints = resolvedProxyEndpoints;
    if (!trace.resolvedProxyEndpoints.empty()) {
        ++gRewriteRStats.seqResolvedProxyRepairUsedCount;
    }
    if (resolvedProxyEndpoints.size() != 1) {
        return failWithTrace(
            "seq loopshared in-place: expected exactly one resolved proxy endpoint");
    }
    if (resolvedProxyEndpoints.front().inputEdgeId != loopEdge.id) {
        return failWithTrace("seq loopshared in-place: resolved proxy endpoint does not match loop edge");
    }

    std::vector<PreservedProxyArc> preserved;
    if (!collectPreservedProxyArcsBeforeClear(core,
                                              oldNode,
                                              resolvedProxyEndpoints,
                                              preserved,
                                              why)) {
        noteSequenceClearPreserveFallback();
        return failWithTrace(why);
    }
    if (preserved.size() != 1) {
        noteSequenceClearPreserveFallback();
        return failWithTrace(
            "seq loopshared in-place: expected exactly one preserved proxy arc");
    }
    trace.preservedProxyArcs = preserved;
    trace.preservedProxyArcsCount = static_cast<int>(trace.preservedProxyArcs.size());
    trace.actualNodes = {oldNode};

    std::vector<NodeId> clearedNeighbors;
    {
        std::unordered_set<NodeId> seenNodes;
        const auto &oldNodeRef = core.nodes[oldNode];
        for (ArcId arcId : oldNodeRef.adjArcs) {
            if (!validActualArcId(core, arcId) || !core.arcs[arcId].alive) continue;
            const NodeId otherNode = otherEndpointOfArc(core.arcs[arcId], oldNode);
            if (otherNode < 0 || !validActualNodeId(core, otherNode) ||
                !core.nodes[otherNode].alive) {
                continue;
            }
            if (seenNodes.insert(otherNode).second) {
                clearedNeighbors.push_back(otherNode);
            }
        }
    }
    for (NodeId nodeId : clearedNeighbors) {
        if (nodeId == oldNode) continue;
        if (std::find(trace.actualNodes.begin(), trace.actualNodes.end(), nodeId) ==
            trace.actualNodes.end()) {
            trace.actualNodes.push_back(nodeId);
        }
    }

    captureReplaySnapshotsForPhase(core,
                                   oldNode,
                                   oldNode,
                                   trace.actualNodes,
                                   trace.resolvedProxyEndpoints,
                                   trace.preservedProxyArcs,
                                   ReplaySnapshotPhase::BEFORE_CLEAR,
                                   trace);

    noteSequenceClearPreserveRequested(preserved.size());
    if (!clearNodeKeepIdForGraftPreserveResolvedProxyArcs(core,
                                                          oldNode,
                                                          trace.preservedProxyArcs,
                                                          SPQRType::R_NODE,
                                                          why)) {
        noteSequenceClearPreserveFallback();
        return failWithTrace(why);
    }

    captureReplaySnapshotsForPhase(core,
                                   oldNode,
                                   oldNode,
                                   trace.actualNodes,
                                   trace.resolvedProxyEndpoints,
                                   trace.preservedProxyArcs,
                                   ReplaySnapshotPhase::AFTER_CLEAR_PRESERVE,
                                   trace);

    std::vector<std::pair<int, int>> ownerOfInputEdge(Hafter.edges.size(), {-1, -1});
    auto &oldActualNode = core.nodes[oldNode];
    const int realSlot = static_cast<int>(oldActualNode.slots.size());
    oldActualNode.slots.push_back(
        {true, realPoles.first, realPoles.second, false, realEdge.realEdge, -1});
    ownerOfInputEdge[realEdge.id] = {oldNode, realSlot};
    core.ownerNodeOfRealEdge[realEdge.realEdge] = oldNode;
    core.ownerSlotOfRealEdge[realEdge.realEdge] = realSlot;

    const int proxyLoopSlot = static_cast<int>(oldActualNode.slots.size());
    oldActualNode.slots.push_back({true, cut, cut, true, -1, -1});
    ownerOfInputEdge[loopEdge.id] = {oldNode, proxyLoopSlot};

    captureReplaySnapshotsForPhase(core,
                                   oldNode,
                                   oldNode,
                                   trace.actualNodes,
                                   trace.resolvedProxyEndpoints,
                                   trace.preservedProxyArcs,
                                   ReplaySnapshotPhase::AFTER_MATERIALIZE,
                                   trace);
    captureReplaySnapshotsForPhase(core,
                                   oldNode,
                                   oldNode,
                                   trace.actualNodes,
                                   trace.resolvedProxyEndpoints,
                                   trace.preservedProxyArcs,
                                   ReplaySnapshotPhase::AFTER_INTERNAL_ARC_CONNECT,
                                   trace);

    trace.sameNodeRehomeAttempted = true;
    trace.failingPreservedInputEdge = trace.preservedProxyArcs.front().inputEdgeId;
    trace.failingPreservedOldArc = trace.preservedProxyArcs.front().oldArc;
    trace.failingPreservedOldSlot = trace.preservedProxyArcs.front().resolvedOldSlot;
    trace.failingNewSlot = proxyLoopSlot;
    noteSequenceClearPreserveSameNodeRehome();
    if (!rehomePreservedProxyArcOnSameNode(core,
                                           trace.preservedProxyArcs.front(),
                                           proxyLoopSlot,
                                           why)) {
        noteSequenceClearPreserveFallback();
        return failWithTrace("seq loopshared in-place: same-node rehome failed");
    }
    trace.sameNodeRehomeSucceeded = true;
    trace.preservedProxyArcs.front().newSlot = proxyLoopSlot;
    trace.preservedProxyArcs.front().finalNode = oldNode;
    trace.preservedProxyArcs.front().sameNodeRehome = true;
    addActualAdjArc(oldActualNode, trace.preservedProxyArcs.front().oldArc);

    captureReplaySnapshotsForPhase(core,
                                   oldNode,
                                   oldNode,
                                   trace.actualNodes,
                                   trace.resolvedProxyEndpoints,
                                   trace.preservedProxyArcs,
                                   ReplaySnapshotPhase::AFTER_PROXY_REWIRE,
                                   trace);

    auto affectedNodes = collectAffectedNodesForAdjRepair(trace, oldNode, trace.preservedProxyArcs);
    for (NodeId nodeId : clearedNeighbors) {
        if (std::find(affectedNodes.begin(), affectedNodes.end(), nodeId) ==
            affectedNodes.end()) {
            affectedNodes.push_back(nodeId);
        }
    }
    std::sort(affectedNodes.begin(), affectedNodes.end());
    affectedNodes.erase(std::unique(affectedNodes.begin(), affectedNodes.end()),
                        affectedNodes.end());
    trace.affectedNodesAfterInPlaceApply = affectedNodes;
    trace.affectedAdjRepairNodes = affectedNodes;
    if (validActualNodeId(core, oldNode)) {
        trace.oldNodeAdjArcsBeforeRepair = core.nodes[oldNode].adjArcs;
    }
    rebuildAdjacencyForAffectedNodesAfterGraft(core, affectedNodes);
    if (validActualNodeId(core, oldNode)) {
        trace.oldNodeAdjArcsAfterRepair = core.nodes[oldNode].adjArcs;
    }

    captureReplaySnapshotsForPhase(core,
                                   oldNode,
                                   oldNode,
                                   trace.actualNodes,
                                   trace.resolvedProxyEndpoints,
                                   trace.preservedProxyArcs,
                                   ReplaySnapshotPhase::AFTER_ADJ_REPAIR,
                                   trace);

    std::unordered_set<VertexId> touched(Hafter.touchedVertices.begin(),
                                         Hafter.touchedVertices.end());
    touched.insert(cut);
    touched.insert(other);
    for (NodeId nodeId : affectedNodes) {
        if (!validActualNodeId(core, nodeId) || !core.nodes[nodeId].alive) continue;
        auto &node = core.nodes[nodeId];
        for (const auto &slot : node.slots) {
            if (!slot.alive) continue;
            touched.insert(slot.poleA);
            touched.insert(slot.poleB);
        }
        rebuildActualRealEdgesHereNode(node);
        node.localAgg = recomputeActualLocalAgg(node, touched);
        node.subAgg = node.localAgg;
    }
    recomputeWholeActualTotalAgg(core, touched);
    rebuildAllOccurrencesActual(core);

    if (loopEdge.id < 0 || loopEdge.id >= static_cast<int>(ownerOfInputEdge.size()) ||
        realEdge.id < 0 || realEdge.id >= static_cast<int>(ownerOfInputEdge.size()) ||
        ownerOfInputEdge[loopEdge.id].first < 0 ||
        ownerOfInputEdge[loopEdge.id].second < 0 ||
        ownerOfInputEdge[realEdge.id].first < 0 ||
        ownerOfInputEdge[realEdge.id].second < 0) {
        return failWithTrace("seq loopshared in-place: ownerOfInputEdge incomplete");
    }
    if (!validActualArcId(core, trace.preservedProxyArcs.front().oldArc) ||
        !core.arcs[trace.preservedProxyArcs.front().oldArc].alive) {
        return failWithTrace("seq loopshared in-place: preserved proxy arc missing");
    }
    const auto &loopArc = core.arcs[trace.preservedProxyArcs.front().oldArc];
    const NodeId outsideNode = trace.preservedProxyArcs.front().outsideNode;
    if (!((loopArc.a == oldNode && loopArc.b == outsideNode) ||
          (loopArc.b == oldNode && loopArc.a == outsideNode))) {
        return failWithTrace("seq loopshared in-place: preserved proxy arc endpoint mismatch");
    }
    if (!validActualSlotId(core.nodes[oldNode], proxyLoopSlot) ||
        !core.nodes[oldNode].slots[proxyLoopSlot].alive ||
        !core.nodes[oldNode].slots[proxyLoopSlot].isVirtual ||
        core.nodes[oldNode].slots[proxyLoopSlot].arcId !=
            trace.preservedProxyArcs.front().oldArc) {
        return failWithTrace("seq loopshared in-place: same-node rehome failed");
    }
    if (!validActualSlotId(core.nodes[oldNode], realSlot) ||
        !core.nodes[oldNode].slots[realSlot].alive ||
        core.nodes[oldNode].slots[realSlot].isVirtual ||
        core.nodes[oldNode].slots[realSlot].realEdge != realEdge.realEdge) {
        return failWithTrace("seq loopshared in-place: missing real edge");
    }
    const auto ownerNodeIt = core.ownerNodeOfRealEdge.find(realEdge.realEdge);
    const auto ownerSlotIt = core.ownerSlotOfRealEdge.find(realEdge.realEdge);
    if (ownerNodeIt == core.ownerNodeOfRealEdge.end() ||
        ownerSlotIt == core.ownerSlotOfRealEdge.end() ||
        ownerNodeIt->second != oldNode ||
        ownerSlotIt->second != realSlot) {
        return failWithTrace("seq loopshared in-place: ownerOfInputEdge incomplete");
    }

    trace.inPlaceLoopSharedApplied = true;
    trace.graftOtherWhy.clear();
    why.clear();
    publishTrace();
    return true;
}

bool applySequenceLoopSharedProxyLoopRealInPlace(ReducedSPQRCore &core,
                                                 NodeId oldNode,
                                                 const CompactGraph &Hafter,
                                                 std::string &why) {
    GraftTrace trace;
    return applySequenceLoopSharedProxyLoopRealInPlaceDetailed(
        core, oldNode, Hafter, trace, why);
}

bool applyLoopPlusEdgeSharedSequenceRewrite(ReducedSPQRCore &core,
                                            NodeId oldNode,
                                            const CompactGraph &H,
                                            std::string &why) {
    why.clear();

    if (!validActualNodeId(core, oldNode) || !core.nodes[oldNode].alive) {
        return fail(why, "loop+edge-shared local apply: old node invalid");
    }
    if (classifyTooSmallCompact(H) != TooSmallSubtype::TS_TWO_OTHER ||
        classifyTooSmallOtherDetailed(H) != TooSmallOtherSubtype::TSO_LOOP_PLUS_EDGE_SHARED ||
        H.edges.size() != 2) {
        return fail(why, "loop+edge-shared local apply: unsupported compact shape");
    }

    auto edgePoles = [&](const CompactEdge &edge) {
        if (edge.a < 0 || edge.a >= static_cast<int>(H.origOfCv.size()) ||
            edge.b < 0 || edge.b >= static_cast<int>(H.origOfCv.size())) {
            return std::pair<VertexId, VertexId>{-1, -1};
        }
        return std::pair<VertexId, VertexId>{H.origOfCv[edge.a], H.origOfCv[edge.b]};
    };

    int loopEdgeIndex = -1;
    int nonLoopEdgeIndex = -1;
    int realInputCount = 0;
    int proxyInputCount = 0;
    std::unordered_set<ArcId> keptProxyArcs;
    for (int edgeIndex = 0; edgeIndex < static_cast<int>(H.edges.size()); ++edgeIndex) {
        const auto &edge = H.edges[edgeIndex];
        const auto poles = edgePoles(edge);
        if (poles.first < 0 || poles.second < 0) {
            return fail(why, "loop+edge-shared local apply: compact edge poles invalid");
        }
        if (edge.kind == CompactEdgeKind::REAL) {
            ++realInputCount;
        } else {
            ++proxyInputCount;
            keptProxyArcs.insert(edge.oldArc);
        }
        if (poles.first == poles.second) {
            loopEdgeIndex = edgeIndex;
        } else {
            nonLoopEdgeIndex = edgeIndex;
        }
    }

    if (loopEdgeIndex < 0 || nonLoopEdgeIndex < 0) {
        return fail(why, "loop+edge-shared local apply: missing loop/non-loop pair");
    }
    if (realInputCount != 1 || proxyInputCount != 1 ||
        H.edges[loopEdgeIndex].kind != CompactEdgeKind::PROXY) {
        return fail(why, "loop+edge-shared local apply: requires one REAL non-loop and one PROXY loop");
    }

    const auto loopPoles = edgePoles(H.edges[loopEdgeIndex]);
    const auto nonLoopPoles = edgePoles(H.edges[nonLoopEdgeIndex]);
    const VertexId cut = loopPoles.first;
    if (nonLoopPoles.first != cut && nonLoopPoles.second != cut) {
        return fail(why, "loop+edge-shared local apply: shared cut not found");
    }

    const auto &oldActualNode = core.nodes[oldNode];
    std::unordered_set<ArcId> currentVirtualArcs;
    for (const auto &slot : oldActualNode.slots) {
        if (!slot.alive || !slot.isVirtual || slot.arcId < 0) continue;
        currentVirtualArcs.insert(slot.arcId);
    }
    if (currentVirtualArcs != keptProxyArcs) {
        return fail(why, "loop+edge-shared local apply: old node virtual arcs do not match compact proxy set");
    }

    std::vector<std::pair<int, const CompactEdge *>> orderedEdges;
    orderedEdges.push_back({nonLoopEdgeIndex, &H.edges[nonLoopEdgeIndex]});
    orderedEdges.push_back({loopEdgeIndex, &H.edges[loopEdgeIndex]});

    for (auto it = core.ownerNodeOfRealEdge.begin(); it != core.ownerNodeOfRealEdge.end();) {
        if (it->second != oldNode) {
            ++it;
            continue;
        }
        core.ownerSlotOfRealEdge.erase(it->first);
        it = core.ownerNodeOfRealEdge.erase(it);
    }

    auto &mutNode = core.nodes[oldNode];
    mutNode.type = SPQRType::R_NODE;
    mutNode.slots.clear();
    mutNode.adjArcs.clear();
    mutNode.realEdgesHere.clear();
    mutNode.localAgg = {};
    mutNode.subAgg = {};

    for (const auto &[_, edgePtr] : orderedEdges) {
        const auto &edge = *edgePtr;
        const auto poles = edgePoles(edge);
        const int slotId = static_cast<int>(mutNode.slots.size());
        const bool isVirtual = edge.kind == CompactEdgeKind::PROXY;
        mutNode.slots.push_back({true,
                                 poles.first,
                                 poles.second,
                                 isVirtual,
                                 isVirtual ? -1 : edge.realEdge,
                                 isVirtual ? edge.oldArc : -1});
        if (!isVirtual) {
            core.ownerNodeOfRealEdge[edge.realEdge] = oldNode;
            core.ownerSlotOfRealEdge[edge.realEdge] = slotId;
            continue;
        }

        if (!validActualArcId(core, edge.oldArc) ||
            !validActualNodeId(core, edge.outsideNode) ||
            !core.nodes[edge.outsideNode].alive) {
            return fail(why, "loop+edge-shared local apply: proxy metadata invalid");
        }

        auto &arc = core.arcs[edge.oldArc];
        if (!arc.alive) {
            return fail(why, "loop+edge-shared local apply: proxy arc dead");
        }
        if (arc.a == oldNode) {
            arc.slotInA = slotId;
        } else if (arc.b == oldNode) {
            arc.slotInB = slotId;
        } else {
            return fail(why, "loop+edge-shared local apply: proxy arc not incident to old node");
        }
        arc.poleA = poles.first;
        arc.poleB = poles.second;
        mutNode.adjArcs.push_back(edge.oldArc);
    }

    for (const auto &[_, edgePtr] : orderedEdges) {
        const auto &edge = *edgePtr;
        if (edge.kind != CompactEdgeKind::PROXY) continue;
        const auto &arc = core.arcs[edge.oldArc];
        const NodeId otherNode =
            arc.a == oldNode ? arc.b :
            arc.b == oldNode ? arc.a :
            -1;
        if (otherNode != edge.outsideNode) {
            return fail(why, "loop+edge-shared local apply: outsideNode mismatch after rewire");
        }

        const int oldNodeSlotId = arc.a == oldNode ? arc.slotInA : arc.slotInB;
        const int outsideSlotId = arc.a == oldNode ? arc.slotInB : arc.slotInA;
        if (!validActualSlotId(mutNode, oldNodeSlotId) ||
            !validActualNodeId(core, otherNode) ||
            !validActualSlotId(core.nodes[otherNode], outsideSlotId)) {
            return fail(why, "loop+edge-shared local apply: proxy endpoint slot invalid");
        }

        const auto &oldNodeSlot = mutNode.slots[oldNodeSlotId];
        const auto &outsideSlot = core.nodes[otherNode].slots[outsideSlotId];
        if (!oldNodeSlot.alive || !oldNodeSlot.isVirtual || oldNodeSlot.arcId != edge.oldArc ||
            !outsideSlot.alive || !outsideSlot.isVirtual || outsideSlot.arcId != edge.oldArc) {
            return fail(why, "loop+edge-shared local apply: proxy endpoint backlink mismatch");
        }

        const auto arcPoles = canonPole(arc.poleA, arc.poleB);
        if (arcPoles != canonPole(oldNodeSlot.poleA, oldNodeSlot.poleB) ||
            arcPoles != canonPole(outsideSlot.poleA, outsideSlot.poleB)) {
            return fail(why, "loop+edge-shared local apply: proxy endpoint pole mismatch");
        }
    }

    std::unordered_set<VertexId> touched(H.touchedVertices.begin(), H.touchedVertices.end());
    for (const auto &slot : mutNode.slots) {
        if (!slot.alive) continue;
        touched.insert(slot.poleA);
        touched.insert(slot.poleB);
    }
    rebuildActualRealEdgesHereNode(mutNode);
    mutNode.localAgg = recomputeActualLocalAgg(mutNode, touched);
    mutNode.subAgg = mutNode.localAgg;
    recomputeWholeActualTotalAgg(core, touched);

    return true;
}

bool applySelfLoopRemainderTwoPathSequenceRewrite(ReducedSPQRCore &core,
                                                  NodeId oldNode,
                                                  const CompactGraph &H,
                                                  std::string &why) {
    why.clear();

    if (!validActualNodeId(core, oldNode) || !core.nodes[oldNode].alive) {
        return fail(why, "self-loop remainder two-path local apply: old node invalid");
    }
    std::string subtypeWhy;
    if (classifySelfLoopBuildFailDetailed(H, subtypeWhy) !=
            SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_TWO_PATH ||
        H.edges.size() != 3) {
        return fail(why, "self-loop remainder two-path local apply: unsupported compact shape");
    }

    auto edgePoles = [&](const CompactEdge &edge) {
        if (edge.a < 0 || edge.a >= static_cast<int>(H.origOfCv.size()) ||
            edge.b < 0 || edge.b >= static_cast<int>(H.origOfCv.size())) {
            return std::pair<VertexId, VertexId>{-1, -1};
        }
        return std::pair<VertexId, VertexId>{H.origOfCv[edge.a], H.origOfCv[edge.b]};
    };

    int loopEdgeIndex = -1;
    std::vector<int> pathEdgeIndices;
    for (int edgeIndex = 0; edgeIndex < static_cast<int>(H.edges.size()); ++edgeIndex) {
        const auto &edge = H.edges[edgeIndex];
        const auto poles = edgePoles(edge);
        if (poles.first < 0 || poles.second < 0) {
            return fail(why, "self-loop remainder two-path local apply: compact edge poles invalid");
        }
        if (poles.first == poles.second) {
            loopEdgeIndex = edgeIndex;
        } else {
            pathEdgeIndices.push_back(edgeIndex);
        }
    }
    if (loopEdgeIndex < 0 || pathEdgeIndices.size() != 2) {
        return fail(why, "self-loop remainder two-path local apply: unsupported compact shape");
    }

    const auto &loopEdge = H.edges[loopEdgeIndex];
    if (loopEdge.kind != CompactEdgeKind::PROXY) {
        return fail(why, "self-loop remainder two-path local apply: stripped loop is not PROXY");
    }
    if (!validActualArcId(core, loopEdge.oldArc) || !core.arcs[loopEdge.oldArc].alive) {
        return fail(why, "self-loop remainder two-path local apply: proxy metadata invalid");
    }

    const auto loopPoles = edgePoles(loopEdge);
    const VertexId loopVertex = loopPoles.first;
    std::vector<int> incidentToLoop;
    for (int edgeIndex : pathEdgeIndices) {
        const auto poles = edgePoles(H.edges[edgeIndex]);
        if (poles.first == loopVertex || poles.second == loopVertex) {
            incidentToLoop.push_back(edgeIndex);
        }
    }
    if (incidentToLoop.empty()) {
        return fail(why, "self-loop remainder two-path local apply: loop vertex not incident to remainder path");
    }

    int hubEdgeIndex = incidentToLoop.front();
    for (int edgeIndex : incidentToLoop) {
        if (H.edges[edgeIndex].id < H.edges[hubEdgeIndex].id) hubEdgeIndex = edgeIndex;
    }
    int otherEdgeIndex = -1;
    for (int edgeIndex : pathEdgeIndices) {
        if (edgeIndex != hubEdgeIndex) {
            otherEdgeIndex = edgeIndex;
            break;
        }
    }
    if (otherEdgeIndex < 0) {
        return fail(why, "self-loop remainder two-path local apply: unsupported compact shape");
    }

    const auto &hubEdge = H.edges[hubEdgeIndex];
    const auto &otherEdge = H.edges[otherEdgeIndex];
    if (hubEdge.kind != CompactEdgeKind::REAL || otherEdge.kind != CompactEdgeKind::REAL) {
        return fail(why, "self-loop remainder two-path local apply: requires REAL remainder path");
    }

    const auto hubPoles = edgePoles(hubEdge);
    const auto otherPoles = edgePoles(otherEdge);
    std::unordered_set<VertexId> sharedCutCandidates;
    for (VertexId v0 : {hubPoles.first, hubPoles.second}) {
        for (VertexId v1 : {otherPoles.first, otherPoles.second}) {
            if (v0 == v1 && v0 >= 0) sharedCutCandidates.insert(v0);
        }
    }
    if (sharedCutCandidates.size() != 1) {
        return fail(why, "self-loop remainder two-path local apply: shared cut not found");
    }
    const VertexId sharedCut = *sharedCutCandidates.begin();

    std::unordered_set<ArcId> currentVirtualArcs;
    const auto &oldActualNode = core.nodes[oldNode];
    for (const auto &slot : oldActualNode.slots) {
        if (!slot.alive || !slot.isVirtual || slot.arcId < 0) continue;
        currentVirtualArcs.insert(slot.arcId);
    }
    if (currentVirtualArcs != std::unordered_set<ArcId>{loopEdge.oldArc}) {
        return fail(why, "self-loop remainder two-path local apply: old node virtual arcs do not match compact proxy set");
    }

    for (auto it = core.ownerNodeOfRealEdge.begin(); it != core.ownerNodeOfRealEdge.end();) {
        if (it->second != oldNode) {
            ++it;
            continue;
        }
        core.ownerSlotOfRealEdge.erase(it->first);
        it = core.ownerNodeOfRealEdge.erase(it);
    }

    NodeId newNodeId = -1;
    for (int nodeId = 0; nodeId < static_cast<int>(core.nodes.size()); ++nodeId) {
        if (!core.nodes[nodeId].alive) {
            newNodeId = nodeId;
            break;
        }
    }
    if (newNodeId < 0) {
        newNodeId = static_cast<int>(core.nodes.size());
        core.nodes.emplace_back();
    }
    auto &newNode = core.nodes[newNodeId];
    newNode = {};
    newNode.alive = true;
    newNode.type = SPQRType::R_NODE;
    newNode.localAgg = {};
    newNode.subAgg = {};

    auto &mutNode = core.nodes[oldNode];
    mutNode.type = SPQRType::R_NODE;
    mutNode.slots.clear();
    mutNode.adjArcs.clear();
    mutNode.realEdgesHere.clear();
    mutNode.localAgg = {};
    mutNode.subAgg = {};

    const int hubSlotId = static_cast<int>(mutNode.slots.size());
    mutNode.slots.push_back({true, hubPoles.first, hubPoles.second, false, hubEdge.realEdge, -1});
    core.ownerNodeOfRealEdge[hubEdge.realEdge] = oldNode;
    core.ownerSlotOfRealEdge[hubEdge.realEdge] = hubSlotId;

    const int loopSlotId = static_cast<int>(mutNode.slots.size());
    mutNode.slots.push_back({true, loopVertex, loopVertex, true, -1, loopEdge.oldArc});
    mutNode.adjArcs.push_back(loopEdge.oldArc);

    const int newRealSlotId = static_cast<int>(newNode.slots.size());
    newNode.slots.push_back({true, otherPoles.first, otherPoles.second, false, otherEdge.realEdge, -1});
    core.ownerNodeOfRealEdge[otherEdge.realEdge] = newNodeId;
    core.ownerSlotOfRealEdge[otherEdge.realEdge] = newRealSlotId;

    ArcId newArcId = -1;
    for (int arcId = 0; arcId < static_cast<int>(core.arcs.size()); ++arcId) {
        if (!core.arcs[arcId].alive) {
            newArcId = arcId;
            break;
        }
    }
    if (newArcId < 0) {
        newArcId = static_cast<int>(core.arcs.size());
        core.arcs.emplace_back();
    }

    const int oldVirtualSlotId = static_cast<int>(mutNode.slots.size());
    mutNode.slots.push_back({true, sharedCut, sharedCut, true, -1, newArcId});
    mutNode.adjArcs.push_back(newArcId);

    const int newVirtualSlotId = static_cast<int>(newNode.slots.size());
    newNode.slots.push_back({true, sharedCut, sharedCut, true, -1, newArcId});
    newNode.adjArcs.push_back(newArcId);

    auto &newArc = core.arcs[newArcId];
    newArc = {};
    newArc.alive = true;
    newArc.a = oldNode;
    newArc.b = newNodeId;
    newArc.slotInA = oldVirtualSlotId;
    newArc.slotInB = newVirtualSlotId;
    newArc.poleA = sharedCut;
    newArc.poleB = sharedCut;

    auto &loopArc = core.arcs[loopEdge.oldArc];
    if (loopArc.a == oldNode) {
        loopArc.slotInA = loopSlotId;
    } else if (loopArc.b == oldNode) {
        loopArc.slotInB = loopSlotId;
    } else {
        return fail(why, "self-loop remainder two-path local apply: proxy arc not incident to old node");
    }
    loopArc.poleA = loopVertex;
    loopArc.poleB = loopVertex;

    const NodeId outsideNode =
        loopArc.a == oldNode ? loopArc.b :
        loopArc.b == oldNode ? loopArc.a :
        -1;
    if (!validActualNodeId(core, outsideNode) || !core.nodes[outsideNode].alive) {
        return fail(why, "self-loop remainder two-path local apply: proxy metadata invalid");
    }
    const int outsideSlotId = loopArc.a == oldNode ? loopArc.slotInB : loopArc.slotInA;
    if (!validActualSlotId(core.nodes[outsideNode], outsideSlotId)) {
        return fail(why, "self-loop remainder two-path local apply: proxy metadata invalid");
    }
    auto &outsideSlot = core.nodes[outsideNode].slots[outsideSlotId];
    outsideSlot.poleA = loopVertex;
    outsideSlot.poleB = loopVertex;

    std::unordered_set<VertexId> touched(H.touchedVertices.begin(), H.touchedVertices.end());
    touched.insert(loopVertex);
    touched.insert(sharedCut);
    touched.insert(hubPoles.first);
    touched.insert(hubPoles.second);
    touched.insert(otherPoles.first);
    touched.insert(otherPoles.second);

    rebuildActualRealEdgesHereNode(mutNode);
    mutNode.localAgg = recomputeActualLocalAgg(mutNode, touched);
    mutNode.subAgg = mutNode.localAgg;
    rebuildActualRealEdgesHereNode(newNode);
    newNode.localAgg = recomputeActualLocalAgg(newNode, touched);
    newNode.subAgg = newNode.localAgg;
    recomputeWholeActualTotalAgg(core, touched);

    return true;
}

bool reconstructCompactBlockPathOrder(const CompactBCResult &bc,
                                      std::vector<int> &orderedBlocks,
                                      std::vector<VertexId> &orderedCuts,
                                      std::string &why) {
    orderedBlocks.clear();
    orderedCuts.clear();

    const int blockCount = static_cast<int>(bc.blocks.size());
    if (!bc.valid || blockCount < 2) {
        return fail(why, "path-of-blocks mini: failed to reconstruct block order");
    }
    if (static_cast<int>(bc.bcAdj.size()) !=
        blockCount + static_cast<int>(bc.articulationVertices.size())) {
        return fail(why, "path-of-blocks mini: failed to reconstruct block order");
    }

    auto normalizedNeighbors = [&](int nodeId) {
        std::vector<int> neighbors = bc.bcAdj[nodeId];
        std::sort(neighbors.begin(), neighbors.end());
        neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
        return neighbors;
    };

    std::vector<int> leafBlocks;
    for (int blockId = 0; blockId < blockCount; ++blockId) {
        const auto neighbors = normalizedNeighbors(blockId);
        if (neighbors.empty() || neighbors.size() > 2) {
            return fail(why, "path-of-blocks mini: failed to reconstruct block order");
        }
        for (int artNodeId : neighbors) {
            if (artNodeId < blockCount ||
                artNodeId >= static_cast<int>(bc.bcAdj.size())) {
                return fail(why, "path-of-blocks mini: failed to reconstruct block order");
            }
        }
        if (neighbors.size() == 1) leafBlocks.push_back(blockId);
    }
    if (leafBlocks.size() != 2) {
        return fail(why, "path-of-blocks mini: failed to reconstruct block order");
    }

    std::sort(leafBlocks.begin(), leafBlocks.end());
    int currentBlock = leafBlocks.front();
    int prevArtNode = -1;
    std::unordered_set<int> seenBlocks;

    while (true) {
        if (!seenBlocks.insert(currentBlock).second) {
            return fail(why, "path-of-blocks mini: failed to reconstruct block order");
        }
        orderedBlocks.push_back(currentBlock);

        std::vector<int> nextArtCandidates;
        for (int artNodeId : normalizedNeighbors(currentBlock)) {
            if (artNodeId == prevArtNode) continue;
            nextArtCandidates.push_back(artNodeId);
        }
        if (nextArtCandidates.empty()) break;
        if (nextArtCandidates.size() != 1) {
            return fail(why, "path-of-blocks mini: failed to reconstruct block order");
        }

        const int artNodeId = nextArtCandidates.front();
        const int artIndex = artNodeId - blockCount;
        if (artIndex < 0 || artIndex >= static_cast<int>(bc.articulationVertices.size())) {
            return fail(why, "path-of-blocks mini: failed to reconstruct block order");
        }

        const auto artNeighbors = normalizedNeighbors(artNodeId);
        if (artNeighbors.size() != 2) {
            return fail(why, "path-of-blocks mini: failed to reconstruct block order");
        }

        int nextBlock = -1;
        for (int blockId : artNeighbors) {
            if (blockId == currentBlock) continue;
            if (blockId < 0 || blockId >= blockCount) {
                return fail(why, "path-of-blocks mini: failed to reconstruct block order");
            }
            nextBlock = blockId;
        }
        if (nextBlock < 0) {
            return fail(why, "path-of-blocks mini: failed to reconstruct block order");
        }

        orderedCuts.push_back(bc.articulationVertices[artIndex]);
        prevArtNode = artNodeId;
        currentBlock = nextBlock;
    }

    if (orderedBlocks.size() != bc.blocks.size() ||
        orderedCuts.size() + 1 != orderedBlocks.size()) {
        return fail(why, "path-of-blocks mini: failed to reconstruct block order");
    }

    return true;
}

bool buildSyntheticMiniForTooSmallTwoPath(const CompactGraph &H,
                                          StaticMiniCore &mini,
                                          std::string &why) {
    why.clear();
    mini = {};

    if (classifyTooSmallCompact(H) != TooSmallSubtype::TS_TWO_PATH ||
        H.edges.size() != 2) {
        return fail(why, "two-path mini: missing shared articulation");
    }

    auto edgePoles = [&](const CompactEdge &edge) {
        if (edge.a < 0 || edge.a >= static_cast<int>(H.origOfCv.size()) ||
            edge.b < 0 || edge.b >= static_cast<int>(H.origOfCv.size())) {
            return std::pair<VertexId, VertexId>{-1, -1};
        }
        return std::pair<VertexId, VertexId>{H.origOfCv[edge.a], H.origOfCv[edge.b]};
    };

    const auto poles0 = edgePoles(H.edges[0]);
    const auto poles1 = edgePoles(H.edges[1]);
    std::unordered_set<VertexId> shared;
    for (VertexId v0 : {poles0.first, poles0.second}) {
        for (VertexId v1 : {poles1.first, poles1.second}) {
            if (v0 == v1 && v0 >= 0) shared.insert(v0);
        }
    }
    if (shared.size() != 1) {
        return fail(why, "two-path mini: missing shared articulation");
    }
    const VertexId cut = *shared.begin();

    ProjectMiniCore projectMini;
    projectMini.valid = true;
    projectMini.kind = CoreKind::REDUCED_SPQR;
    projectMini.nodes.resize(2);
    projectMini.arcs.resize(1);
    projectMini.ownerOfInputEdge.assign(H.edges.size(), {-1, -1});
    for (auto &node : projectMini.nodes) {
        node.alive = true;
        node.type = SPQRType::R_NODE;
        node.localAgg = {};
        node.payloadAgg = {};
    }

    std::vector<int> articulationSlotOfNode(2, -1);
    std::vector<int> seenInput(H.edges.size(), 0);
    for (int edgeIndex = 0; edgeIndex < 2; ++edgeIndex) {
        const auto &edge = H.edges[edgeIndex];
        if (edge.id < 0 || edge.id >= static_cast<int>(H.edges.size())) {
            return fail(why, "two-path mini: ownerOfInputEdge incomplete");
        }
        const auto poles = edgePoles(edge);
        if (poles.first < 0 || poles.second < 0) {
            return fail(why, "two-path mini: missing shared articulation");
        }
        if (poles.first != cut && poles.second != cut) {
            return fail(why, "two-path mini: missing shared articulation");
        }

        ProjectMiniSlot slot;
        slot.alive = true;
        slot.poleA = poles.first;
        slot.poleB = poles.second;
        slot.inputEdgeId = edge.id;
        if (edge.kind == CompactEdgeKind::REAL) {
            slot.kind = MiniSlotKind::REAL_INPUT;
            slot.realEdge = edge.realEdge;
        } else {
            slot.kind = MiniSlotKind::PROXY_INPUT;
        }

        auto &node = projectMini.nodes[edgeIndex];
        const int slotId = static_cast<int>(node.slots.size());
        node.slots.push_back(slot);
        projectMini.ownerOfInputEdge[edge.id] = {edgeIndex, slotId};
        ++seenInput[edge.id];
    }

    for (int inputId = 0; inputId < static_cast<int>(seenInput.size()); ++inputId) {
        if (seenInput[inputId] != 1 ||
            projectMini.ownerOfInputEdge[inputId].first < 0 ||
            projectMini.ownerOfInputEdge[inputId].second < 0) {
            return fail(why, "two-path mini: ownerOfInputEdge incomplete");
        }
    }

    for (int nodeId = 0; nodeId < 2; ++nodeId) {
        ProjectMiniSlot slot;
        slot.alive = true;
        slot.kind = MiniSlotKind::INTERNAL_VIRTUAL;
        slot.poleA = cut;
        slot.poleB = cut;
        slot.miniArcId = 0;
        articulationSlotOfNode[nodeId] =
            static_cast<int>(projectMini.nodes[nodeId].slots.size());
        projectMini.nodes[nodeId].slots.push_back(slot);
        projectMini.nodes[nodeId].adjArcs.push_back(0);
    }

    ProjectMiniArc arc;
    arc.alive = true;
    arc.a = 0;
    arc.b = 1;
    arc.slotInA = articulationSlotOfNode[0];
    arc.slotInB = articulationSlotOfNode[1];
    arc.poleA = cut;
    arc.poleB = cut;
    projectMini.arcs[0] = arc;

    for (int nodeId = 0; nodeId < 2; ++nodeId) {
        int articulationCount = 0;
        for (const auto &slot : projectMini.nodes[nodeId].slots) {
            if (!slot.alive || slot.kind != MiniSlotKind::INTERNAL_VIRTUAL) continue;
            ++articulationCount;
        }
        if (articulationCount != 1) {
            return fail(why, "two-path mini: internal virtual articulation slot missing");
        }
    }
    if (projectMini.nodes[0].slots[articulationSlotOfNode[0]].kind !=
            MiniSlotKind::INTERNAL_VIRTUAL ||
        projectMini.nodes[1].slots[articulationSlotOfNode[1]].kind !=
            MiniSlotKind::INTERNAL_VIRTUAL) {
        return fail(why, "two-path mini: internal virtual articulation slot missing");
    }

    recomputePayloadAgg(H, projectMini);
    projectMini.kind = computeMiniKind(projectMini);
    projectMini.valid = true;

    std::string stageWhy;
    if (!checkProjectMiniOwnershipConsistency(H, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "two-path mini: ownerOfInputEdge incomplete"
                : "two-path mini: " + stageWhy;
        return false;
    }
    if (!checkProjectMiniReducedInvariant(H, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "two-path mini: synthetic mini reduced invariant failed"
                : "two-path mini: " + stageWhy;
        return false;
    }

    return exportProjectMiniCore(projectMini, mini, why);
}

bool buildSyntheticMiniForPathOfBlocks(const CompactGraph &H,
                                       const CompactBCResult &bc,
                                       StaticMiniCore &mini,
                                       std::string &why) {
    why.clear();
    mini = {};

    if (!bc.valid || bc.blocks.size() < 2 ||
        classifyNotBiconnected(H, bc) != NotBiconnectedSubtype::NB_PATH_OF_BLOCKS) {
        return fail(why, "path-of-blocks mini: failed to reconstruct block order");
    }

    std::vector<int> orderedBlocks;
    std::vector<VertexId> orderedCuts;
    if (!reconstructCompactBlockPathOrder(bc, orderedBlocks, orderedCuts, why)) {
        return false;
    }

    const int nodeCount = static_cast<int>(orderedBlocks.size());
    ProjectMiniCore projectMini;
    projectMini.valid = true;
    projectMini.kind = CoreKind::REDUCED_SPQR;
    projectMini.nodes.resize(nodeCount);
    projectMini.arcs.resize(nodeCount - 1);
    projectMini.ownerOfInputEdge.assign(H.edges.size(), {-1, -1});
    for (auto &node : projectMini.nodes) {
        node.alive = true;
        node.type = SPQRType::R_NODE;
        node.localAgg = {};
        node.payloadAgg = {};
    }

    std::unordered_map<int, int> nodeOfBlock;
    for (int nodeId = 0; nodeId < nodeCount; ++nodeId) {
        nodeOfBlock.emplace(orderedBlocks[nodeId], nodeId);
    }

    auto blockContainsVertex = [&](int blockId, VertexId vertex) {
        const auto &vertices = bc.blocks[blockId].vertices;
        return std::find(vertices.begin(), vertices.end(), vertex) != vertices.end();
    };

    std::vector<int> seenInput(H.edges.size(), 0);
    std::vector<int> realInputsPerNode(nodeCount, 0);
    std::vector<int> articulationSlotsPerNode(nodeCount, 0);

    for (const auto &edge : H.edges) {
        if (edge.id < 0 || edge.id >= static_cast<int>(H.edges.size())) {
            return fail(why, "path-of-blocks mini: ownerOfInputEdge incomplete");
        }
        if (edge.a < 0 || edge.a >= static_cast<int>(H.origOfCv.size()) ||
            edge.b < 0 || edge.b >= static_cast<int>(H.origOfCv.size())) {
            return fail(why, "path-of-blocks mini: edge assigned to wrong block");
        }

        const auto blockIt = bc.blockOfEdge.find(edge.id);
        if (blockIt == bc.blockOfEdge.end()) {
            return fail(why, "path-of-blocks mini: edge assigned to wrong block");
        }
        const auto nodeIt = nodeOfBlock.find(blockIt->second);
        if (nodeIt == nodeOfBlock.end()) {
            return fail(why, "path-of-blocks mini: edge assigned to wrong block");
        }

        const VertexId poleA = H.origOfCv[edge.a];
        const VertexId poleB = H.origOfCv[edge.b];
        if (!blockContainsVertex(blockIt->second, poleA) ||
            !blockContainsVertex(blockIt->second, poleB)) {
            return fail(why, "path-of-blocks mini: edge assigned to wrong block");
        }

        ProjectMiniSlot slot;
        slot.alive = true;
        slot.poleA = poleA;
        slot.poleB = poleB;
        slot.inputEdgeId = edge.id;
        if (edge.kind == CompactEdgeKind::REAL) {
            slot.kind = MiniSlotKind::REAL_INPUT;
            slot.realEdge = edge.realEdge;
            ++realInputsPerNode[nodeIt->second];
        } else {
            slot.kind = MiniSlotKind::PROXY_INPUT;
        }

        auto &node = projectMini.nodes[nodeIt->second];
        const int slotId = static_cast<int>(node.slots.size());
        node.slots.push_back(slot);
        projectMini.ownerOfInputEdge[edge.id] = {nodeIt->second, slotId};
        ++seenInput[edge.id];
    }

    for (int inputId = 0; inputId < static_cast<int>(seenInput.size()); ++inputId) {
        if (seenInput[inputId] != 1 ||
            projectMini.ownerOfInputEdge[inputId].first < 0 ||
            projectMini.ownerOfInputEdge[inputId].second < 0) {
            return fail(why, "path-of-blocks mini: ownerOfInputEdge incomplete");
        }
    }

    for (int step = 0; step < nodeCount - 1; ++step) {
        const int leftBlockId = orderedBlocks[step];
        const int rightBlockId = orderedBlocks[step + 1];
        const int leftNodeId = step;
        const int rightNodeId = step + 1;
        const VertexId cut = orderedCuts[step];

        if (!blockContainsVertex(leftBlockId, cut) ||
            !blockContainsVertex(rightBlockId, cut)) {
            return fail(why, "path-of-blocks mini: articulation cut not present in adjacent blocks");
        }

        ProjectMiniSlot leftSlot;
        leftSlot.alive = true;
        leftSlot.kind = MiniSlotKind::INTERNAL_VIRTUAL;
        leftSlot.poleA = cut;
        leftSlot.poleB = cut;
        leftSlot.miniArcId = step;
        const int leftSlotId = static_cast<int>(projectMini.nodes[leftNodeId].slots.size());
        projectMini.nodes[leftNodeId].slots.push_back(leftSlot);
        projectMini.nodes[leftNodeId].adjArcs.push_back(step);
        ++articulationSlotsPerNode[leftNodeId];

        ProjectMiniSlot rightSlot;
        rightSlot.alive = true;
        rightSlot.kind = MiniSlotKind::INTERNAL_VIRTUAL;
        rightSlot.poleA = cut;
        rightSlot.poleB = cut;
        rightSlot.miniArcId = step;
        const int rightSlotId = static_cast<int>(projectMini.nodes[rightNodeId].slots.size());
        projectMini.nodes[rightNodeId].slots.push_back(rightSlot);
        projectMini.nodes[rightNodeId].adjArcs.push_back(step);
        ++articulationSlotsPerNode[rightNodeId];

        ProjectMiniArc arc;
        arc.alive = true;
        arc.a = leftNodeId;
        arc.b = rightNodeId;
        arc.slotInA = leftSlotId;
        arc.slotInB = rightSlotId;
        arc.poleA = cut;
        arc.poleB = cut;
        projectMini.arcs[step] = arc;
    }

    for (int nodeId = 0; nodeId < nodeCount; ++nodeId) {
        const int expectedArticulationSlots =
            (nodeId == 0 || nodeId == nodeCount - 1) ? 1 : 2;
        if (articulationSlotsPerNode[nodeId] != expectedArticulationSlots) {
            return fail(why, "path-of-blocks mini: internal virtual slot missing");
        }
        if (realInputsPerNode[nodeId] == 0) {
            return fail(why, "path-of-blocks mini: block has no REAL input");
        }
    }

    recomputePayloadAgg(H, projectMini);
    projectMini.kind = computeMiniKind(projectMini);
    projectMini.valid = true;

    std::string stageWhy;
    if (!checkProjectMiniOwnershipConsistency(H, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "path-of-blocks mini: ownerOfInputEdge incomplete"
                : "path-of-blocks mini: " + stageWhy;
        return false;
    }
    if (!checkProjectMiniReducedInvariant(H, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "path-of-blocks mini: synthetic mini reduced invariant failed"
                : "path-of-blocks mini: " + stageWhy;
        return false;
    }

    return exportProjectMiniCore(projectMini, mini, why);
}

bool buildSyntheticMiniForSingleCutTwoBlocks(const CompactGraph &H,
                                             const CompactBCResult &bc,
                                             StaticMiniCore &mini,
                                             std::string &why) {
    why.clear();
    mini = {};

    if (!bc.valid || bc.blocks.size() != 2 || bc.articulationVertices.size() != 1) {
        return fail(why, "single-cut-two-blocks: missing articulation vertex");
    }

    const VertexId cut = bc.articulationVertices.front();
    const auto cutMembershipsIt = bc.blocksOfVertex.find(cut);
    if (cutMembershipsIt == bc.blocksOfVertex.end() || cutMembershipsIt->second.size() != 2) {
        return fail(why, "single-cut-two-blocks: missing articulation vertex");
    }
    std::vector<int> blockIds = cutMembershipsIt->second;
    std::sort(blockIds.begin(), blockIds.end());
    blockIds.erase(std::unique(blockIds.begin(), blockIds.end()), blockIds.end());
    if (blockIds.size() != 2) {
        return fail(why, "single-cut-two-blocks: missing articulation vertex");
    }
    for (int blockId : blockIds) {
        if (blockId < 0 || blockId >= static_cast<int>(bc.blocks.size())) {
            return fail(why, "single-cut-two-blocks: missing articulation vertex");
        }
        if (std::find(bc.blocks[blockId].vertices.begin(),
                      bc.blocks[blockId].vertices.end(),
                      cut) == bc.blocks[blockId].vertices.end()) {
            return fail(why, "single-cut-two-blocks: missing articulation vertex");
        }
    }

    std::unordered_map<int, int> localNodeOfBlock = {
        {blockIds[0], 0},
        {blockIds[1], 1},
    };

    ProjectMiniCore projectMini;
    projectMini.valid = true;
    projectMini.kind = CoreKind::REDUCED_SPQR;
    projectMini.nodes.resize(2);
    projectMini.arcs.resize(1);
    projectMini.ownerOfInputEdge.assign(H.edges.size(), {-1, -1});
    for (auto &node : projectMini.nodes) {
        node.alive = true;
        node.type = SPQRType::R_NODE;
        node.localAgg = {};
        node.payloadAgg = {};
    }

    std::vector<int> articulationSlotOfNode(2, -1);
    std::vector<int> seenInput(H.edges.size(), 0);
    for (const auto &edge : H.edges) {
        if (edge.id < 0 || edge.id >= static_cast<int>(H.edges.size())) {
            return fail(why, "single-cut-two-blocks: edge not assigned to either block");
        }
        if (edge.a < 0 || edge.a >= static_cast<int>(H.origOfCv.size()) ||
            edge.b < 0 || edge.b >= static_cast<int>(H.origOfCv.size())) {
            return fail(why, "single-cut-two-blocks: edge endpoint out of range");
        }

        const auto blockIt = bc.blockOfEdge.find(edge.id);
        if (blockIt == bc.blockOfEdge.end()) {
            return fail(why, "single-cut-two-blocks: edge not assigned to either block");
        }
        const auto localNodeIt = localNodeOfBlock.find(blockIt->second);
        if (localNodeIt == localNodeOfBlock.end()) {
            return fail(why, "single-cut-two-blocks: edge not assigned to either block");
        }

        ProjectMiniSlot slot;
        slot.alive = true;
        slot.poleA = H.origOfCv[edge.a];
        slot.poleB = H.origOfCv[edge.b];
        slot.inputEdgeId = edge.id;
        if (edge.kind == CompactEdgeKind::REAL) {
            slot.kind = MiniSlotKind::REAL_INPUT;
            slot.realEdge = edge.realEdge;
        } else {
            slot.kind = MiniSlotKind::PROXY_INPUT;
        }

        auto &node = projectMini.nodes[localNodeIt->second];
        const int slotId = static_cast<int>(node.slots.size());
        node.slots.push_back(slot);
        projectMini.ownerOfInputEdge[edge.id] = {localNodeIt->second, slotId};
        ++seenInput[edge.id];
    }

    for (int inputId = 0; inputId < static_cast<int>(seenInput.size()); ++inputId) {
        if (seenInput[inputId] != 1) {
            return fail(why, "single-cut-two-blocks: ownerOfInputEdge incomplete");
        }
        const auto owner = projectMini.ownerOfInputEdge[inputId];
        if (owner.first < 0 || owner.second < 0) {
            return fail(why, "single-cut-two-blocks: ownerOfInputEdge incomplete");
        }
    }

    for (int nodeId = 0; nodeId < 2; ++nodeId) {
        ProjectMiniSlot slot;
        slot.alive = true;
        slot.kind = MiniSlotKind::INTERNAL_VIRTUAL;
        slot.poleA = cut;
        slot.poleB = cut;
        slot.miniArcId = 0;
        articulationSlotOfNode[nodeId] =
            static_cast<int>(projectMini.nodes[nodeId].slots.size());
        projectMini.nodes[nodeId].slots.push_back(slot);
        projectMini.nodes[nodeId].adjArcs.push_back(0);
    }

    if (articulationSlotOfNode[0] < 0 || articulationSlotOfNode[1] < 0) {
        return fail(why, "single-cut-two-blocks: missing articulation vertex");
    }

    ProjectMiniArc arc;
    arc.alive = true;
    arc.a = 0;
    arc.b = 1;
    arc.slotInA = articulationSlotOfNode[0];
    arc.slotInB = articulationSlotOfNode[1];
    arc.poleA = cut;
    arc.poleB = cut;
    projectMini.arcs[0] = arc;

    for (int nodeId = 0; nodeId < 2; ++nodeId) {
        int articulationCount = 0;
        for (int slotId = 0; slotId < static_cast<int>(projectMini.nodes[nodeId].slots.size()); ++slotId) {
            const auto &slot = projectMini.nodes[nodeId].slots[slotId];
            if (!slot.alive || slot.kind != MiniSlotKind::INTERNAL_VIRTUAL) continue;
            ++articulationCount;
        }
        if (articulationCount != 1) {
            return fail(why, "single-cut-two-blocks: missing articulation vertex");
        }
    }
    const auto &slotA = projectMini.nodes[0].slots[articulationSlotOfNode[0]];
    const auto &slotB = projectMini.nodes[1].slots[articulationSlotOfNode[1]];
    if (slotA.kind != MiniSlotKind::INTERNAL_VIRTUAL ||
        slotB.kind != MiniSlotKind::INTERNAL_VIRTUAL) {
        return fail(why, "single-cut-two-blocks: articulation slot kind mismatch");
    }

    recomputePayloadAgg(H, projectMini);
    projectMini.kind = computeMiniKind(projectMini);
    projectMini.valid = true;

    std::string stageWhy;
    if (!checkProjectMiniOwnershipConsistency(H, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "single-cut-two-blocks: ownerOfInputEdge incomplete"
                : "single-cut-two-blocks: " + stageWhy;
        return false;
    }
    if (!checkProjectMiniReducedInvariant(H, projectMini, stageWhy)) {
        why = stageWhy.empty()
                ? "single-cut-two-blocks: synthetic mini reduced invariant failed"
                : "single-cut-two-blocks: " + stageWhy;
        return false;
    }

    return exportProjectMiniCore(projectMini, mini, why);
}

bool buildExplicitAfterDeletingVertex(const ReducedSPQRCore &core,
                                      VertexId x,
                                      ExplicitBlockGraph &out,
                                      std::string &why) {
    why.clear();
    out = {};

    ProjectExplicitBlockGraph current;
    materializeProjectWholeCoreExplicit(core, current);

    std::unordered_set<VertexId> vertices;
    for (VertexId vertex : current.vertices) {
        if (vertex == x) continue;
        if (vertex < 0) {
            return fail(why, "rewriteR fallback: explicit vertex id negative after materialize");
        }
        vertices.insert(vertex);
    }

    for (const auto &edge : current.edges) {
        if (edge.id < 0) {
            return fail(why, "rewriteR fallback: explicit edge id negative after materialize");
        }
        if (edge.u < 0 || edge.v < 0) {
            return fail(why, "rewriteR fallback: explicit edge endpoint negative after materialize");
        }
        if (edge.u == x || edge.v == x) continue;

        const auto poles = canonPole(edge.u, edge.v);
        out.edges.push_back({edge.id, poles.first, poles.second});
        vertices.insert(poles.first);
        vertices.insert(poles.second);
    }

    out.vertices.assign(vertices.begin(), vertices.end());
    canonicalizeExplicitGraph(out);
    return true;
}

bool rebuildWholeCoreFromExplicit(const ExplicitBlockGraph &G,
                                  ReducedSPQRCore &out,
                                  std::string &why) {
    why.clear();
    out = {};

    ExplicitBlockGraph canonical = G;
    canonicalizeExplicitGraph(canonical);

    std::unordered_set<VertexId> vertexSet(canonical.vertices.begin(),
                                           canonical.vertices.end());
    std::unordered_set<EdgeId> seenEdgeIds;

    for (const auto &edge : canonical.edges) {
        if (edge.id < 0) {
            return fail(why, "rewriteR fallback: explicit edge id must be non-negative");
        }
        if (edge.u < 0 || edge.v < 0) {
            return fail(why, "rewriteR fallback: explicit edge endpoint must be non-negative");
        }
        if (!seenEdgeIds.insert(edge.id).second) {
            return fail(why, "rewriteR fallback: duplicate explicit edge id");
        }
        vertexSet.insert(edge.u);
        vertexSet.insert(edge.v);
    }

    std::vector<VertexId> vertices(vertexSet.begin(), vertexSet.end());
    std::sort(vertices.begin(), vertices.end());

    if (canonical.edges.empty()) {
        out.blockId = 0;
        out.root = -1;
        return true;
    }

    CompactGraph H;
    H.block = 0;
    H.ownerR = 0;
    H.deletedX = -1;
    H.origOfCv = vertices;
    H.touchedVertices = vertices;
    for (int cv = 0; cv < static_cast<int>(H.origOfCv.size()); ++cv) {
        H.cvOfOrig[H.origOfCv[cv]] = cv;
    }

    for (int inputId = 0; inputId < static_cast<int>(canonical.edges.size()); ++inputId) {
        const auto &edge = canonical.edges[inputId];
        const auto itU = H.cvOfOrig.find(edge.u);
        const auto itV = H.cvOfOrig.find(edge.v);
        if (itU == H.cvOfOrig.end() || itV == H.cvOfOrig.end()) {
            return fail(why, "rewriteR fallback: explicit edge endpoint missing from vertex set");
        }
        CompactEdge compactEdge;
        compactEdge.id = inputId;
        compactEdge.kind = CompactEdgeKind::REAL;
        compactEdge.a = itU->second;
        compactEdge.b = itV->second;
        compactEdge.realEdge = edge.id;
        H.edges.push_back(compactEdge);
    }

    DummyActualEnvelope env;
    if (!::buildDummyActualCoreEnvelope(H, env, why)) {
        if (!why.empty()) {
            why = "rewriteR fallback: whole-core rebuild failed: " + why;
        } else {
            why = "rewriteR fallback: whole-core rebuild failed";
        }
        return false;
    }

    out = std::move(env.core);
    return true;
}

bool rebuildWholeCoreAfterDeletingX(ReducedSPQRCore &core,
                                    VertexId x,
                                    std::string &why) {
    const BlockId originalBlockId = core.blockId;

    ExplicitBlockGraph explicitAfterDelete;
    if (!buildExplicitAfterDeletingVertex(core, x, explicitAfterDelete, why)) return false;

    ReducedSPQRCore rebuilt;
    if (!rebuildWholeCoreFromExplicit(explicitAfterDelete, rebuilt, why)) return false;

    rebuilt.blockId = originalBlockId;
    core = std::move(rebuilt);
    why.clear();
    return true;
}

bool buildWholeCoreForTesting(const ExplicitBlockGraph &G,
                              ReducedSPQRCore &core,
                              std::string &why) {
    return buildWholeCoreForSequenceTesting(G, core, why);
}

SequenceChooseStatus chooseDeterministicSequenceRewriteTarget(
    const ReducedSPQRCore &core,
    NodeId &chosenR,
    VertexId &chosenX,
    std::string &why) {
    const auto status =
        chooseDeterministicSequenceRewriteTargetForFixpoint(core, chosenR, chosenX, why);
    return static_cast<SequenceChooseStatus>(static_cast<uint8_t>(status));
}

bool runRewriteSequenceToFixpoint(ReducedSPQRCore &core,
                                  RewriteSeqStats &stats,
                                  std::string &why) {
    constexpr int kMaxSequenceSteps = 64;

    stats = {};
    why.clear();

    ProjectHarnessOps ops;

    auto recordFailure = [&](HarnessStage stage,
                             std::string where,
                             std::string msg,
                             int stepIndex,
                             int sequenceLengthSoFar,
                             std::optional<NodeId> chosenR,
                             std::optional<VertexId> chosenX,
                             const ReducedSPQRCore *beforeCore,
                             const ReducedSPQRCore *afterCore,
                             const ExplicitBlockGraph *explicitBefore,
                             const ExplicitBlockGraph *explicitAfter,
                             const ExplicitBlockGraph *explicitExpected,
                             const ExplicitBlockGraph *explicitGot) -> bool {
        stats.failureStage = stage;
        stats.failureWhere = std::move(where);
        stats.failureWhy = std::move(msg);
        stats.failureStepIndex = stepIndex;
        stats.sequenceLengthSoFar = sequenceLengthSoFar;
        stats.chosenR = chosenR;
        stats.chosenX = chosenX;
        if (beforeCore) stats.actualBeforeFailure = *beforeCore;
        if (afterCore) stats.actualAfterFailure = *afterCore;
        if (explicitBefore) stats.explicitBeforeFailure = *explicitBefore;
        if (explicitAfter) stats.explicitAfterFailure = *explicitAfter;
        if (explicitExpected) stats.explicitExpected = *explicitExpected;
        if (explicitGot) stats.explicitGot = *explicitGot;
        why = stats.failureWhy;
        return false;
    };

    for (int step = 0; step < kMaxSequenceSteps; ++step) {
        const ExplicitBlockGraph explicitBefore = ops.materializeWholeCoreExplicit(core);

        NodeId chosenR = -1;
        VertexId chosenX = -1;
        const SequenceFixpointChooseStatus chooseStatus =
            chooseDeterministicSequenceRewriteTargetForFixpoint(core, chosenR, chosenX, why);
        if (chooseStatus == SequenceFixpointChooseStatus::ERROR) {
            return recordFailure(HarnessStage::SEQ_CHOOSE_RX_FAIL,
                                 "chooseDeterministicSequenceRewriteTarget",
                                 why,
                                 step,
                                 step,
                                 std::nullopt,
                                 std::nullopt,
                                 &core,
                                 nullptr,
                                 &explicitBefore,
                                 nullptr,
                                 nullptr,
                                 nullptr);
        }
        if (chooseStatus == SequenceFixpointChooseStatus::NONE) {
            stats.reachedFixpoint = true;
            stats.success = true;
            why.clear();
            return true;
        }

        ReducedSPQRCore after = core;

        setRewriteRSequenceStepContext(step, step + 1);
        const uint64_t seqFallbacksBefore =
            getRewriteRStats().seqRewriteWholeCoreFallbackCount;
        setRewriteRSequenceMode(true);
        const bool rewriteOk = ops.rewriteRFallback(after, chosenR, chosenX, why);
        setRewriteRSequenceMode(false);
        const uint64_t seqFallbacksAfter =
            getRewriteRStats().seqRewriteWholeCoreFallbackCount;
        stats.hadSequenceFallback =
            stats.hadSequenceFallback || (seqFallbacksAfter > seqFallbacksBefore);
        if (!rewriteOk) {
            const ExplicitBlockGraph explicitAfter = ops.materializeWholeCoreExplicit(after);
            return recordFailure(HarnessStage::SEQ_REWRITE_R_FAIL,
                                 "rewriteR_fallback",
                                 why,
                                 step,
                                 step + 1,
                                 chosenR,
                                 chosenX,
                                 &core,
                                 &after,
                                 &explicitBefore,
                                 &explicitAfter,
                                 nullptr,
                                 nullptr);
        }

        if (!ops.normalizeTouchedRegion(after, why)) {
            noteRewriteRWeakRepairCommitOutcome(WeakRepairCommitOutcome::WCO_NORMALIZE_FAIL,
                                                &after,
                                                why);
            const ExplicitBlockGraph explicitAfter = ops.materializeWholeCoreExplicit(after);
            return recordFailure(HarnessStage::SEQ_NORMALIZE_FAIL,
                                 "normalizeTouchedRegion",
                                 why,
                                 step,
                                 step + 1,
                                 chosenR,
                                 chosenX,
                                 &core,
                                 &after,
                                 &explicitBefore,
                                 &explicitAfter,
                                 nullptr,
                                 nullptr);
        }

        if (hasPendingSequenceDeferredSameTypeSP()) {
            GraftTrace cleanupTrace;
            if (peekSequenceDeferredSameTypeSPTrace(cleanupTrace)) {
                std::string preCleanupWhyDetailed;
                cleanupTrace.preCleanupPostcheckSubtype =
                    classifyPostcheckFailureDetailed(after, preCleanupWhyDetailed);
                cleanupTrace.postCleanupPostcheckSubtype =
                    cleanupTrace.preCleanupPostcheckSubtype;
                if (!preCleanupWhyDetailed.empty()) {
                    cleanupTrace.postcheckWhyDetailed = preCleanupWhyDetailed;
                }
                if (cleanupTrace.preCleanupPostcheckSubtype ==
                    GraftPostcheckSubtype::GPS_SAME_TYPE_SP_ONLY) {
                    const auto seeds = collectSequenceSPCleanupSeeds(
                        after, chosenR, &cleanupTrace, &cleanupTrace.preservedProxyArcs);
                    cleanupTrace.sameTypeSPCleanupSeedNodes = seeds;
                    setSequenceDeferredSameTypeSPTrace(cleanupTrace);

                    std::string cleanupWhy;
                    if (!cleanupSequenceSameTypeSPAdjacency(after, seeds, cleanupWhy)) {
                        peekSequenceDeferredSameTypeSPTrace(cleanupTrace);
                        cleanupTrace.graftOtherWhy = cleanupWhy;
                        setSequenceDeferredSameTypeSPTrace(cleanupTrace);
                        flushSequenceDeferredSameTypeSPDump(after);
                        const ExplicitBlockGraph explicitAfter =
                            ops.materializeWholeCoreExplicit(after);
                        return recordFailure(HarnessStage::SEQ_ACTUAL_INVARIANT_FAIL,
                                             "cleanupSequenceSameTypeSPAdjacency",
                                             cleanupWhy,
                                             step,
                                             step + 1,
                                             chosenR,
                                             chosenX,
                                             &core,
                                             &after,
                                             &explicitBefore,
                                             &explicitAfter,
                                             nullptr,
                                             nullptr);
                    }

                    peekSequenceDeferredSameTypeSPTrace(cleanupTrace);
                    std::string postCleanupWhyDetailed;
                    cleanupTrace.postCleanupPostcheckSubtype =
                        classifyPostcheckFailureDetailed(after, postCleanupWhyDetailed);
                    if (!postCleanupWhyDetailed.empty()) {
                        cleanupTrace.postcheckWhyDetailed = postCleanupWhyDetailed;
                    }
                    setSequenceDeferredSameTypeSPTrace(cleanupTrace);
                } else {
                    setSequenceDeferredSameTypeSPTrace(cleanupTrace);
                }
            }
        }

        flushSequenceDeferredSameTypeSPDump(after);
        const ExplicitBlockGraph explicitAfter = ops.materializeWholeCoreExplicit(after);

        if (!ops.checkActualReducedInvariant(after, nullptr, why)) {
            noteRewriteRWeakRepairCommitOutcome(
                WeakRepairCommitOutcome::WCO_ACTUAL_INVARIANT_FAIL,
                &after,
                why);
            return recordFailure(HarnessStage::SEQ_ACTUAL_INVARIANT_FAIL,
                                 "checkActualReducedInvariant",
                                 why,
                                 step,
                                 step + 1,
                                 chosenR,
                                 chosenX,
                                 &core,
                                 &after,
                                 &explicitBefore,
                                 &explicitAfter,
                                 nullptr,
                                 nullptr);
        }

        NodeId nextR = -1;
        VertexId nextX = -1;
        const SequenceFixpointChooseStatus nextChooseStatus =
            chooseDeterministicSequenceRewriteTargetForFixpoint(after, nextR, nextX, why);
        if (nextChooseStatus == SequenceFixpointChooseStatus::ERROR) {
            return recordFailure(HarnessStage::SEQ_CHOOSE_RX_FAIL,
                                 "chooseDeterministicSequenceRewriteTarget(after)",
                                 why,
                                 step,
                                 step + 1,
                                 chosenR,
                                 chosenX,
                                 &core,
                                 &after,
                                 &explicitBefore,
                                 &explicitAfter,
                                 nullptr,
                                 nullptr);
        }
        if (explicitAfter.edges.size() >= explicitBefore.edges.size() &&
            nextChooseStatus == SequenceFixpointChooseStatus::FOUND) {
            return recordFailure(
                HarnessStage::SEQ_PROGRESS_STUCK,
                "sequence progress check",
                "rewrite sequence made no edge-count progress while another rewrite target remains",
                step,
                step + 1,
                chosenR,
                chosenX,
                &core,
                &after,
                &explicitBefore,
                &explicitAfter,
                nullptr,
                nullptr);
        }

        ReducedSPQRCore oracle;
        if (!buildWholeCoreForSequenceTesting(explicitAfter, oracle, why)) {
            noteRewriteRWeakRepairCommitOutcome(WeakRepairCommitOutcome::WCO_ORACLE_FAIL,
                                                &after,
                                                why);
            return recordFailure(HarnessStage::SEQ_ORACLE_FAIL,
                                 "buildWholeCoreForSequenceTesting",
                                 why,
                                 step,
                                 step + 1,
                                 chosenR,
                                 chosenX,
                                 &core,
                                 &after,
                                 &explicitBefore,
                                 &explicitAfter,
                                 nullptr,
                                 &explicitAfter);
        }

        const ExplicitBlockGraph explicitExpected =
            ops.materializeWholeCoreExplicit(oracle);
        if (!ops.checkEquivalentExplicitGraphs(explicitAfter, explicitExpected, why)) {
            noteRewriteRWeakRepairCommitOutcome(WeakRepairCommitOutcome::WCO_ORACLE_FAIL,
                                                &after,
                                                why);
            return recordFailure(HarnessStage::SEQ_ORACLE_FAIL,
                                 "checkEquivalentExplicitGraphs",
                                 why,
                                 step,
                                 step + 1,
                                 chosenR,
                                 chosenX,
                                 &core,
                                 &after,
                                 &explicitBefore,
                                 &explicitAfter,
                                 &explicitExpected,
                                 &explicitAfter);
        }

        noteRewriteRWeakRepairCommitOutcome(WeakRepairCommitOutcome::WCO_COMMITTED,
                                            &after,
                                            {});
        core = std::move(after);
        stats.completedSteps = step + 1;
        if (nextChooseStatus == SequenceFixpointChooseStatus::NONE) {
            stats.reachedFixpoint = true;
            why.clear();
            return true;
        }
    }

    stats.maxStepReached = true;
    return recordFailure(
        HarnessStage::SEQ_MAX_STEPS_REACHED,
        "rewrite sequence loop",
        "rewrite sequence reached maxSteps without exhausting rewrite targets",
        kMaxSequenceSteps,
        kMaxSequenceSteps,
        std::nullopt,
        std::nullopt,
        &core,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr);
}

void pruneCompactGraphForSpqr(CompactGraph &H) {
    std::vector<CompactEdge> keptEdges;
    keptEdges.reserve(H.edges.size());
    for (const auto &edge : H.edges) {
        if (edge.kind == CompactEdgeKind::PROXY && edge.sideAgg.edgeCnt == 0) continue;
        keptEdges.push_back(edge);
    }
    H.edges = std::move(keptEdges);

    std::vector<int> degree(H.origOfCv.size(), 0);
    for (const auto &edge : H.edges) {
        if (edge.a < 0 || edge.a >= static_cast<int>(degree.size()) ||
            edge.b < 0 || edge.b >= static_cast<int>(degree.size())) {
            continue;
        }
        ++degree[edge.a];
        ++degree[edge.b];
    }

    std::vector<int> newIndex(H.origOfCv.size(), -1);
    std::vector<VertexId> newOrigOfCv;
    newOrigOfCv.reserve(H.origOfCv.size());
    for (int cv = 0; cv < static_cast<int>(H.origOfCv.size()); ++cv) {
        if (degree[cv] == 0) continue;
        newIndex[cv] = static_cast<int>(newOrigOfCv.size());
        newOrigOfCv.push_back(H.origOfCv[cv]);
    }

    for (auto &edge : H.edges) {
        if (edge.a >= 0 && edge.a < static_cast<int>(newIndex.size())) edge.a = newIndex[edge.a];
        if (edge.b >= 0 && edge.b < static_cast<int>(newIndex.size())) edge.b = newIndex[edge.b];
    }

    H.origOfCv = std::move(newOrigOfCv);
    H.cvOfOrig.clear();
    for (int cv = 0; cv < static_cast<int>(H.origOfCv.size()); ++cv) {
        H.cvOfOrig[H.origOfCv[cv]] = cv;
    }
    H.touchedVertices = H.origOfCv;
}

bool rewriteProjectRFallback(ReducedSPQRCore &core,
                             NodeId rNode,
                             VertexId x,
                             std::string &why) {
    ++gRewriteRStats.rewriteCalls;
    gRewriteRCaseContext.currentRNode = rNode;
    gRewriteRCaseContext.currentX = x;
    const bool sequenceMode = gRewriteRCaseContext.sequenceMode;
    const ReducedSPQRCore originalCore = sequenceMode ? core : ReducedSPQRCore{};
    if (sequenceMode) {
        gWeakRepairPipelineContext.beforeCore = originalCore;
    }

    auto attemptWholeCoreFallback = [&](std::string &fallbackWhy) -> bool {
        ++gRewriteRStats.rewriteFallbackWholeCoreCount;
        if (!rebuildWholeCoreAfterDeletingX(core, x, fallbackWhy)) return false;
        noteRewritePathTaken(RewritePathTaken::WHOLE_CORE_REBUILD);
        return true;
    };

    auto attemptSequenceWholeCoreFallback = [&](const std::string &triggerWhy,
                                                uint64_t *categoryCounter,
                                                std::string &outWhy) -> bool {
        if (!sequenceMode) return false;
        if (categoryCounter) ++(*categoryCounter);
        ++gRewriteRStats.seqRewriteWholeCoreFallbackCount;
        core = originalCore;

        std::string fallbackWhy;
        if (attemptWholeCoreFallback(fallbackWhy)) {
            outWhy.clear();
            return true;
        }
        if (!fallbackWhy.empty()) {
            outWhy = triggerWhy.empty()
                    ? fallbackWhy
                    : triggerWhy + ": " + fallbackWhy;
        } else {
            outWhy = triggerWhy;
        }
        return false;
    };

    auto refreshActualAfterGraft = [&](const CompactGraph &compact,
                                       const GraftTrace &trace) {
        const std::unordered_set<VertexId> touched(compact.touchedVertices.begin(),
                                                   compact.touchedVertices.end());
        for (NodeId actualNodeId : trace.actualNodes) {
            if (!validActualNodeId(core, actualNodeId) || !core.nodes[actualNodeId].alive) continue;
            auto &actualNode = core.nodes[actualNodeId];
            rebuildActualRealEdgesHereNode(actualNode);
            actualNode.localAgg = recomputeActualLocalAgg(actualNode, touched);
            actualNode.subAgg = actualNode.localAgg;
        }
        recomputeWholeActualTotalAgg(core, touched);
    };

    auto effectiveGraftRewireSubtype = [&](const GraftTrace *trace) {
        if (!trace) return GraftRewireBailoutSubtype::GRB_OTHER;
        if (trace->graftRewireSubtype == GraftRewireBailoutSubtype::COUNT) {
            return GraftRewireBailoutSubtype::GRB_OTHER;
        }
        return trace->graftRewireSubtype;
    };

    auto shouldFallbackSequenceGraftFailure = [&](const GraftTrace *trace,
                                                  const std::string &graftWhy) {
        return effectiveGraftRewireSubtype(trace) != GraftRewireBailoutSubtype::GRB_OTHER ||
               shouldFallbackSequenceProxyGraftFailure(graftWhy);
    };

    auto attemptSequenceAwareGraft = [&](const CompactGraph &compact,
                                         const StaticMiniCore &miniCore,
                                         int keepNode,
                                         GraftTrace &trace,
                                         std::string &graftWhy) {
        trace = {};
        auto publishTrace = [&]() {
            if (!sequenceMode) return;
            publishRewriteRSequenceReplayTrace(trace);
        };
        if (sequenceMode) {
            ResolvedProxyEndpointFailureInfo resolveFailure;
            std::vector<ResolvedProxyEndpoint> resolvedProxyEndpoints;
            if (!resolveLiveProxyEndpointsForGraftDetailed(core,
                                                          rNode,
                                                          compact,
                                                          resolvedProxyEndpoints,
                                                          &resolveFailure,
                                                          graftWhy)) {
                ++gRewriteRStats.seqResolvedProxySnapshotFailCount;
                trace.graftRewireSubtype = resolveFailure.subtype;
                trace.failingInputEdge = resolveFailure.inputEdgeId;
                trace.failingOldArc = resolveFailure.oldArc;
                trace.resolvedProxyEndpoints = std::move(resolvedProxyEndpoints);
                publishTrace();
                return false;
            }
            ++gRewriteRStats.seqResolvedProxySnapshotCount;
            trace.resolvedProxyEndpoints = std::move(resolvedProxyEndpoints);
            if (!trace.resolvedProxyEndpoints.empty()) {
                ++gRewriteRStats.seqResolvedProxyRepairUsedCount;
            }
        }

        std::queue<NodeId> q;
        if (!::graftMiniCoreIntoPlace(core,
                                      rNode,
                                      compact,
                                      miniCore,
                                      keepNode,
                                      q,
                                      &trace,
                                      graftWhy)) {
            if (trace.graftOtherSubtype == GraftOtherSubtype::GOS_POSTCHECK_ADJ_MISMATCH) {
                std::string whyDetailed;
                const auto detailed =
                    classifyPostcheckFailureDetailedImpl(core, whyDetailed);
                trace.postcheckSubtype = detailed.subtype;
                trace.postcheckWhyDetailed = whyDetailed;
                if (detailed.firstAdjNode >= 0 && trace.firstBadAdjNode < 0) {
                    trace.firstBadAdjNode = detailed.firstAdjNode;
                    trace.expectedAdj = detailed.expectedAdj;
                    trace.actualAdj = detailed.actualAdj;
                }
            }
            for (const auto &resolved : trace.resolvedProxyEndpoints) {
                if (!resolved.repairUsedWeakPolesOnly) continue;
                gWeakRepairPipelineContext = {};
                gWeakRepairPipelineContext.pending = true;
                gWeakRepairPipelineContext.info = resolved;
                gWeakRepairPipelineContext.compact = compact;
                gWeakRepairPipelineContext.beforeCore = originalCore;
                gWeakRepairPipelineContext.trace = trace;
                gWeakRepairPipelineContext.why = graftWhy;
                noteWeakRepairCommitSample(WeakRepairCommitOutcome::WCO_GRAFT_FAIL,
                                           &core,
                                           graftWhy);
                break;
            }
            publishTrace();
            return false;
        }

        if (!sequenceMode) return true;

        bool usedWeakProxyRepair = false;
        for (const auto &resolved : trace.resolvedProxyEndpoints) {
            if (!resolved.repairUsedWeakPolesOnly) continue;
            usedWeakProxyRepair = true;
            break;
        }
        const bool usedClearPreserveRepair = !trace.preservedProxyArcs.empty();
        const bool usedSequenceProxyRepair =
            usedWeakProxyRepair || usedClearPreserveRepair;
        if (!usedSequenceProxyRepair) {
            publishTrace();
            return true;
        }

        NodeId offendingNode = -1;
        if (findForbiddenDeadRelayActual(core, offendingNode)) {
            std::ostringstream oss;
            oss << "graft: sequence proxy repair produced dead relay on node "
                << offendingNode;
            graftWhy = oss.str();
            trace.graftRewireSubtype = GraftRewireBailoutSubtype::GRB_OTHER;
            trace.graftOtherSubtype = GraftOtherSubtype::GOS_OTHER;
            trace.graftOtherWhy = graftWhy;
            trace.preservedProxyArcsCount = static_cast<int>(trace.preservedProxyArcs.size());
            if (usedWeakProxyRepair) {
                gWeakRepairPipelineContext = {};
                for (const auto &resolved : trace.resolvedProxyEndpoints) {
                    if (!resolved.repairUsedWeakPolesOnly) continue;
                    gWeakRepairPipelineContext.pending = true;
                    gWeakRepairPipelineContext.info = resolved;
                    gWeakRepairPipelineContext.compact = compact;
                    gWeakRepairPipelineContext.beforeCore = originalCore;
                    gWeakRepairPipelineContext.trace = trace;
                    gWeakRepairPipelineContext.why = graftWhy;
                    noteWeakRepairCommitSample(WeakRepairCommitOutcome::WCO_GRAFT_FAIL,
                                               &core,
                                               graftWhy);
                    break;
                }
            }
            publishTrace();
            return false;
        }

        auto stashWeakProxyRepairForCommit = [&]() {
            if (!usedWeakProxyRepair) return;
            gWeakRepairPipelineContext = {};
            for (const auto &resolved : trace.resolvedProxyEndpoints) {
                if (!resolved.repairUsedWeakPolesOnly) continue;
                gWeakRepairPipelineContext.pending = true;
                gWeakRepairPipelineContext.info = resolved;
                gWeakRepairPipelineContext.compact = compact;
                gWeakRepairPipelineContext.beforeCore = originalCore;
                gWeakRepairPipelineContext.trace = trace;
                gWeakRepairPipelineContext.why = graftWhy;
                break;
            }
        };

        std::string metadataWhy;
        if (!checkSequencePreNormalizeMetadataOnly(core, metadataWhy)) {
            graftWhy = metadataWhy;
            trace.graftRewireSubtype = GraftRewireBailoutSubtype::GRB_OTHER;
            trace.graftOtherSubtype = GraftOtherSubtype::GOS_POSTCHECK_ADJ_MISMATCH;
            trace.graftOtherWhy = graftWhy;
            trace.preservedProxyArcsCount = static_cast<int>(trace.preservedProxyArcs.size());
            std::string whyDetailed;
            const auto detailed =
                classifyPostcheckFailureDetailedImpl(core, whyDetailed);
            trace.postcheckSubtype = detailed.subtype;
            trace.postcheckWhyDetailed = whyDetailed;
            if (detailed.firstAdjNode >= 0) {
                trace.firstBadAdjNode = detailed.firstAdjNode;
                trace.expectedAdj = detailed.expectedAdj;
                trace.actualAdj = detailed.actualAdj;
            }
            if (usedWeakProxyRepair) {
                gWeakRepairPipelineContext = {};
                for (const auto &resolved : trace.resolvedProxyEndpoints) {
                    if (!resolved.repairUsedWeakPolesOnly) continue;
                    gWeakRepairPipelineContext.pending = true;
                    gWeakRepairPipelineContext.info = resolved;
                    gWeakRepairPipelineContext.compact = compact;
                    gWeakRepairPipelineContext.beforeCore = originalCore;
                    gWeakRepairPipelineContext.trace = trace;
                    gWeakRepairPipelineContext.why = graftWhy;
                    noteWeakRepairCommitSample(WeakRepairCommitOutcome::WCO_GRAFT_FAIL,
                                               &core,
                                               graftWhy);
                    break;
                }
            }
            publishTrace();
            return false;
        }

        ArcId offendingArc = -1;
        NodeId offendingA = -1;
        NodeId offendingB = -1;
        if (findForbiddenSameTypeSPAdjacencyActual(core, offendingArc, offendingA, offendingB)) {
            std::ostringstream oss;
            oss << "graft: sequence rewrite produced adjacent same-type S/P nodes"
                << " on arc " << offendingArc
                << " (" << offendingA << "," << offendingB << ")";
            graftWhy = oss.str();
            trace.graftRewireSubtype = GraftRewireBailoutSubtype::GRB_OTHER;
            trace.graftOtherSubtype = GraftOtherSubtype::GOS_POSTCHECK_ADJ_MISMATCH;
            trace.graftOtherWhy = graftWhy;
            trace.preservedProxyArcsCount = static_cast<int>(trace.preservedProxyArcs.size());
            std::string whyDetailed;
            const auto detailed =
                classifyPostcheckFailureDetailedImpl(core, whyDetailed);
            trace.postcheckSubtype = detailed.subtype;
            trace.postcheckWhyDetailed = whyDetailed;
            if (detailed.firstAdjNode >= 0) {
                trace.firstBadAdjNode = detailed.firstAdjNode;
                trace.expectedAdj = detailed.expectedAdj;
                trace.actualAdj = detailed.actualAdj;
            }
            if (sequenceMode &&
                detailed.subtype == GraftPostcheckSubtype::GPS_SAME_TYPE_SP_ONLY) {
                trace.deferredSameTypeSP = true;
                noteSequenceDeferredSameTypeSP(originalCore, core, compact, trace, graftWhy);
                stashWeakProxyRepairForCommit();
                publishTrace();
                return true;
            }
            if (usedWeakProxyRepair) {
                gWeakRepairPipelineContext = {};
                for (const auto &resolved : trace.resolvedProxyEndpoints) {
                    if (!resolved.repairUsedWeakPolesOnly) continue;
                    gWeakRepairPipelineContext.pending = true;
                    gWeakRepairPipelineContext.info = resolved;
                    gWeakRepairPipelineContext.compact = compact;
                    gWeakRepairPipelineContext.beforeCore = originalCore;
                    gWeakRepairPipelineContext.trace = trace;
                    gWeakRepairPipelineContext.why = graftWhy;
                    noteWeakRepairCommitSample(WeakRepairCommitOutcome::WCO_GRAFT_FAIL,
                                               &core,
                                               graftWhy);
                    break;
                }
            }
            publishTrace();
            return false;
        }

        stashWeakProxyRepairForCommit();
        publishTrace();
        return true;
    };

    CompactGraph H;
    CompactRejectReason rejectReason = CompactRejectReason::OTHER;
    CompactBuildFailSubtype buildFailSubtype = CompactBuildFailSubtype::CBF_OTHER;
    if (!buildProjectCompactLocalViewFromR(core, rNode, x, H, &rejectReason, &buildFailSubtype, why)) {
        if (isRecoverableCompactReject(rejectReason)) {
            const RewriteFallbackTrigger trigger =
                classifyCompactBuildFallbackTrigger(rejectReason, why);
            std::string triggerDetailWhy = why;
            XIncidentVirtualSubtype xIncidentSubtype = XIncidentVirtualSubtype::XIV_MIXED_OTHER;
            XSharedResidualSubtype xSharedResidualSubtype =
                XSharedResidualSubtype::XSR_HAFTER_OTHER;
            XSharedLoopSharedInputSubtype xSharedLoopSharedInputSubtype =
                XSharedLoopSharedInputSubtype::XLSI_OTHER;
            XSharedLoopSharedBailout xSharedLoopSharedBailout =
                XSharedLoopSharedBailout::XLSB_OTHER;
            XSharedBridgeBailout xSharedBridgeBailout = XSharedBridgeBailout::XSB_NONE;
            CompactGraph xSharedHafter;
            GraftTrace xSharedTrace;
            const GraftTrace *xSharedTracePtr = nullptr;
            if (sequenceMode &&
                trigger == RewriteFallbackTrigger::RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED) {
                std::string xIncidentWhy;
                xIncidentSubtype =
                    classifyXIncidentVirtualDetailed(originalCore, rNode, x, xIncidentWhy);
                if (!xIncidentWhy.empty()) {
                    if (!triggerDetailWhy.empty()) triggerDetailWhy += " | ";
                    triggerDetailWhy += xIncidentWhy;
                }
                if (xIncidentSubtype == XIncidentVirtualSubtype::XIV_SHARED_WITH_LOOP) {
                    std::string residualWhy;
                    xSharedResidualSubtype = classifyXSharedResidualAfterDelete(originalCore,
                                                                                rNode,
                                                                                x,
                                                                                xSharedHafter,
                                                                                residualWhy);
                    if (!residualWhy.empty()) {
                        if (!triggerDetailWhy.empty()) triggerDetailWhy += " | ";
                        triggerDetailWhy += residualWhy;
                    }
                    if (xSharedResidualSubtype ==
                        XSharedResidualSubtype::XSR_HAFTER_LOOP_SHARED) {
                        xSharedLoopSharedInputSubtype =
                            classifyXSharedLoopSharedInputSubtype(xSharedHafter);
                        noteSequenceXSharedLoopSharedInputSubtype(
                            xSharedLoopSharedInputSubtype,
                            originalCore,
                            rNode,
                            x,
                            xSharedHafter,
                            nullptr,
                            triggerDetailWhy);
                    }

                    StaticMiniCore syntheticMini;
                    RewritePathTaken handledPath = RewritePathTaken::WHOLE_CORE_REBUILD;
                    std::string stageContext;
                    bool miniBuilt = false;
                    bool keepPreselected = false;
                    int preselectedKeep = -1;
                    if (xSharedResidualSubtype == XSharedResidualSubtype::XSR_HAFTER_BUILD_FAIL) {
                        xSharedBridgeBailout = XSharedBridgeBailout::XSB_HAFTER_BUILD_FAIL;
                    } else if (xSharedResidualSubtype ==
                               XSharedResidualSubtype::XSR_HAFTER_SPQR_READY) {
                        ++gRewriteRStats.seqXIncidentSpqrReadyAttemptCount;
                        stageContext = "x-shared-loop spqr-ready";
                        miniBuilt = buildSequenceMiniForXSharedSpqrReady(xSharedHafter,
                                                                         syntheticMini,
                                                                         preselectedKeep,
                                                                         residualWhy);
                        keepPreselected = miniBuilt;
                        handledPath = RewritePathTaken::DIRECT_SPQR;
                        if (!miniBuilt) {
                            ++gRewriteRStats.seqXIncidentSpqrReadyFallbackCount;
                            if (residualWhy.find("no alive mini node after normalize") !=
                                std::string::npos ||
                                residualWhy.find("chooseKeepMiniNode failed") !=
                                    std::string::npos) {
                                xSharedBridgeBailout =
                                    XSharedBridgeBailout::XSB_CHOOSE_KEEP_FAIL;
                            } else {
                                xSharedBridgeBailout = XSharedBridgeBailout::XSB_OTHER;
                            }
                            noteSequenceXSharedSpqrReadySample("fallback",
                                                               originalCore,
                                                               rNode,
                                                               x,
                                                               xSharedHafter,
                                                               preselectedKeep,
                                                               nullptr,
                                                               residualWhy);
                        } else {
                            noteSequenceXSharedSpqrReadySample("attempt",
                                                               originalCore,
                                                               rNode,
                                                               x,
                                                               xSharedHafter,
                                                               preselectedKeep,
                                                               nullptr,
                                                               "rewriteR: x-shared spqr-ready local path attempt");
                        }
                    } else if (xSharedResidualSubtype ==
                               XSharedResidualSubtype::XSR_HAFTER_ONE_EDGE) {
                        stageContext = "x-shared-loop one-edge";
                        miniBuilt = buildSyntheticMiniForOneEdgeRemainder(xSharedHafter,
                                                                          syntheticMini,
                                                                          residualWhy);
                        handledPath = RewritePathTaken::SPECIAL_ONE_EDGE;
                        if (!miniBuilt) {
                            if (xSharedHafter.edges.size() == 1 &&
                                xSharedHafter.edges[0].kind == CompactEdgeKind::PROXY) {
                                ++gRewriteRStats.seqXIncidentOneEdgeUnsupportedProxyCount;
                                xSharedBridgeBailout =
                                    XSharedBridgeBailout::XSB_UNSUPPORTED_HAFTER_SUBTYPE;
                            } else {
                                xSharedBridgeBailout = XSharedBridgeBailout::XSB_OTHER;
                            }
                        }
                    } else if (xSharedResidualSubtype ==
                               XSharedResidualSubtype::XSR_HAFTER_TWO_PATH) {
                        stageContext = "x-shared-loop two-path";
                        miniBuilt = buildSyntheticMiniForTooSmallTwoPath(xSharedHafter,
                                                                         syntheticMini,
                                                                         residualWhy);
                        handledPath = RewritePathTaken::SPECIAL_TWO_PATH;
                        if (!miniBuilt) {
                            xSharedBridgeBailout =
                                XSharedBridgeBailout::XSB_TWO_PATH_BUILDER_FAIL;
                        }
                    } else if (xSharedResidualSubtype ==
                               XSharedResidualSubtype::XSR_HAFTER_LOOP_SHARED) {
                        stageContext = "x-shared-loop loop+edge-shared";
                        handledPath = RewritePathTaken::SPECIAL_LOOP_SHARED;
                        if (sequenceMode &&
                            xSharedLoopSharedInputSubtype ==
                                XSharedLoopSharedInputSubtype::XLSI_PROXY_LOOP_REAL_EDGE) {
                            ++gRewriteRStats.seqXSharedLoopSharedProxyLoopRealAttemptCount;
                            if (!applySequenceLoopSharedProxyLoopRealInPlaceDetailed(core,
                                                                                     rNode,
                                                                                     xSharedHafter,
                                                                                     xSharedTrace,
                                                                                     residualWhy)) {
                                ++gRewriteRStats
                                      .seqXSharedLoopSharedProxyLoopRealFallbackCount;
                                xSharedBridgeBailout = XSharedBridgeBailout::XSB_GRAFT_FAIL;
                                xSharedLoopSharedBailout =
                                    classifyXSharedLoopSharedBailoutFromContext(
                                        true,
                                        true,
                                        false,
                                        true,
                                        GraftPostcheckSubtype::GPS_OTHER,
                                        true);
                                xSharedTracePtr = &xSharedTrace;
                            } else {
                                ++gRewriteRStats.seqXIncidentSharedWithLoopHandledCount;
                                ++gRewriteRStats
                                      .seqXSharedLoopSharedProxyLoopRealHandledCount;
                                ++gRewriteRStats.rewriteFallbackSpecialCaseCount;
                                noteRewritePathTaken(handledPath);
                                why.clear();
                                return true;
                            }
                        } else {
                            CompactGraph repairedHafter = xSharedHafter;
                            ResolvedProxyEndpointFailureInfo resolveFailure;
                            std::vector<ResolvedProxyEndpoint> resolvedProxyEndpoints;
                            if (!resolveLiveProxyEndpointsForGraftDetailed(core,
                                                                           rNode,
                                                                           repairedHafter,
                                                                           resolvedProxyEndpoints,
                                                                           &resolveFailure,
                                                                           residualWhy)) {
                                xSharedBridgeBailout = XSharedBridgeBailout::XSB_GRAFT_FAIL;
                                xSharedLoopSharedBailout =
                                    classifyXSharedLoopSharedBailoutFromContext(
                                        true,
                                        true,
                                        false,
                                        true,
                                        GraftPostcheckSubtype::GPS_OTHER,
                                        true);
                                xSharedTrace.graftRewireSubtype = resolveFailure.subtype;
                                xSharedTrace.failingInputEdge = resolveFailure.inputEdgeId;
                                xSharedTrace.failingOldArc = resolveFailure.oldArc;
                                xSharedTrace.resolvedProxyEndpoints =
                                    std::move(resolvedProxyEndpoints);
                                xSharedTracePtr = &xSharedTrace;
                            } else if (resolvedProxyEndpoints.size() != 1) {
                                xSharedBridgeBailout = XSharedBridgeBailout::XSB_GRAFT_FAIL;
                                xSharedLoopSharedBailout =
                                    classifyXSharedLoopSharedBailoutFromContext(
                                        true,
                                        true,
                                        false,
                                        true,
                                        GraftPostcheckSubtype::GPS_OTHER,
                                        true);
                                xSharedTrace.resolvedProxyEndpoints =
                                    std::move(resolvedProxyEndpoints);
                                xSharedTracePtr = &xSharedTrace;
                                residualWhy =
                                    "x-shared-loop loop+edge-shared expected exactly one resolved PROXY endpoint";
                            } else {
                                const auto &resolved = resolvedProxyEndpoints.front();
                                bool patchedProxyEdge = false;
                                for (auto &edge : repairedHafter.edges) {
                                    if (edge.kind != CompactEdgeKind::PROXY ||
                                        edge.id != resolved.inputEdgeId) {
                                        continue;
                                    }
                                    edge.oldArc = resolved.resolvedArc;
                                    edge.outsideNode = resolved.outsideNode;
                                    edge.oldSlotInU = resolved.resolvedOldSlot;
                                    patchedProxyEdge = true;
                                    break;
                                }
                                if (!patchedProxyEdge) {
                                    xSharedBridgeBailout = XSharedBridgeBailout::XSB_GRAFT_FAIL;
                                    xSharedLoopSharedBailout =
                                        classifyXSharedLoopSharedBailoutFromContext(
                                            true,
                                            true,
                                            false,
                                            true,
                                            GraftPostcheckSubtype::GPS_OTHER,
                                            true);
                                    xSharedTrace.resolvedProxyEndpoints =
                                        std::move(resolvedProxyEndpoints);
                                    xSharedTracePtr = &xSharedTrace;
                                    residualWhy =
                                        "x-shared-loop loop+edge-shared missing PROXY edge for resolved snapshot";
                                } else if (!applyLoopPlusEdgeSharedSequenceRewrite(core,
                                                                                   rNode,
                                                                                   repairedHafter,
                                                                                   residualWhy)) {
                                    xSharedBridgeBailout = XSharedBridgeBailout::XSB_GRAFT_FAIL;
                                    xSharedLoopSharedBailout =
                                        classifyXSharedLoopSharedBailoutFromContext(
                                            true,
                                            true,
                                            false,
                                            true,
                                            GraftPostcheckSubtype::GPS_OTHER,
                                            true);
                                    xSharedTrace.resolvedProxyEndpoints =
                                        std::move(resolvedProxyEndpoints);
                                    xSharedTracePtr = &xSharedTrace;
                                } else {
                                    ++gRewriteRStats.seqXIncidentSharedWithLoopHandledCount;
                                    ++gRewriteRStats.rewriteFallbackSpecialCaseCount;
                                    noteRewritePathTaken(handledPath);
                                    why.clear();
                                    return true;
                                }
                            }
                        }
                    } else {
                        xSharedBridgeBailout =
                            XSharedBridgeBailout::XSB_UNSUPPORTED_HAFTER_SUBTYPE;
                        if (!triggerDetailWhy.empty()) triggerDetailWhy += " | ";
                        triggerDetailWhy +=
                            "x-shared-loop: after-delete subtype unsupported";
                    }

                    if (!miniBuilt) {
                        if (!stageContext.empty()) {
                            if (!triggerDetailWhy.empty()) triggerDetailWhy += " | ";
                            triggerDetailWhy += residualWhy.empty()
                                    ? "rewriteR: " + stageContext +
                                          " synthetic mini build failed"
                                    : "rewriteR: " + residualWhy;
                        }
                    } else {
                        auto chooseSequenceKeepForXShared = [&](const StaticMiniCore &miniCore,
                                                               XSharedResidualSubtype subtype,
                                                               int &keepOut,
                                                               std::string &keepWhy) {
                            keepOut = -1;
                            if (subtype == XSharedResidualSubtype::XSR_HAFTER_LOOP_SHARED) {
                                for (int nodeId = 0;
                                     nodeId < static_cast<int>(miniCore.nodes.size());
                                     ++nodeId) {
                                    const auto &node = miniCore.nodes[nodeId];
                                    if (!node.alive) continue;
                                    bool hasRealInput = false;
                                    for (const auto &slot : node.slots) {
                                        if (!slot.alive) continue;
                                        if (slot.kind == MiniSlotKind::REAL_INPUT) {
                                            hasRealInput = true;
                                            break;
                                        }
                                    }
                                    if (hasRealInput) {
                                        keepOut = nodeId;
                                        keepWhy.clear();
                                        return true;
                                    }
                                }
                            }
                            return ::chooseKeepMiniNode(miniCore, keepOut, keepWhy);
                        };

                        int keep = keepPreselected ? preselectedKeep : -1;
                        if ((!keepPreselected &&
                             (!chooseSequenceKeepForXShared(syntheticMini,
                                                            xSharedResidualSubtype,
                                                            keep,
                                                            residualWhy) ||
                              keep < 0)) ||
                            (keepPreselected && keep < 0)) {
                            xSharedBridgeBailout =
                                XSharedBridgeBailout::XSB_CHOOSE_KEEP_FAIL;
                            if (!triggerDetailWhy.empty()) triggerDetailWhy += " | ";
                            triggerDetailWhy += residualWhy.empty()
                                    ? "rewriteR: " + stageContext +
                                          " chooseKeepMiniNode failed"
                                    : "rewriteR: " + stageContext +
                                          " chooseKeepMiniNode failed: " + residualWhy;
                        } else {
                            if (!attemptSequenceAwareGraft(xSharedHafter,
                                                           syntheticMini,
                                                           keep,
                                                           xSharedTrace,
                                                           residualWhy)) {
                                xSharedBridgeBailout =
                                    XSharedBridgeBailout::XSB_GRAFT_FAIL;
                                xSharedTracePtr = &xSharedTrace;
                                if (xSharedResidualSubtype ==
                                    XSharedResidualSubtype::XSR_HAFTER_SPQR_READY) {
                                    ++gRewriteRStats.seqXIncidentSpqrReadyFallbackCount;
                                    noteSequenceXSharedSpqrReadySample("fallback",
                                                                       originalCore,
                                                                       rNode,
                                                                       x,
                                                                       xSharedHafter,
                                                                       keep,
                                                                       &xSharedTrace,
                                                                       residualWhy);
                                }
                                if (!triggerDetailWhy.empty()) triggerDetailWhy += " | ";
                                triggerDetailWhy += residualWhy.empty()
                                        ? "rewriteR: " + stageContext + " graft failed"
                                        : residualWhy;
                            } else {
                                refreshActualAfterGraft(xSharedHafter, xSharedTrace);
                                ++gRewriteRStats.seqXIncidentSharedWithLoopHandledCount;
                                if (xSharedResidualSubtype ==
                                    XSharedResidualSubtype::XSR_HAFTER_ONE_EDGE) {
                                    ++gRewriteRStats.seqXIncidentOneEdgeHandledCount;
                                    ++gRewriteRStats.seqXIncidentOneEdgeRealHandledCount;
                                } else if (xSharedResidualSubtype ==
                                           XSharedResidualSubtype::XSR_HAFTER_SPQR_READY) {
                                    rebuildAllOccurrencesActual(core);
                                    ++gRewriteRStats.seqXIncidentSpqrReadyHandledCount;
                                    noteSequenceXSharedSpqrReadySample("success",
                                                                       originalCore,
                                                                       rNode,
                                                                       x,
                                                                       xSharedHafter,
                                                                       keep,
                                                                       &xSharedTrace,
                                                                       "rewriteR: x-shared spqr-ready handled");
                                }
                                ++gRewriteRStats.rewriteFallbackSpecialCaseCount;
                                noteRewritePathTaken(handledPath);
                                why.clear();
                                return true;
                            }
                        }
                    }
                }
            }
            core = originalCore;
            noteCompactReject(rejectReason, core, rNode, x, &H, triggerDetailWhy);
            const std::string triggerWhy = triggerDetailWhy.empty()
                    ? "rewriteR fallback after local compact reject (" +
                          std::string(compactRejectReasonName(rejectReason)) + ")"
                    : "rewriteR fallback after local compact reject (" +
                          std::string(compactRejectReasonName(rejectReason)) + " | " +
                          triggerDetailWhy + ")";
            if (sequenceMode) {
                if (trigger == RewriteFallbackTrigger::RFT_COMPACT_BUILD_FAIL) {
                    noteSequenceCompactBuildFailSubtype(buildFailSubtype,
                                                        originalCore,
                                                        rNode,
                                                        x,
                                                        &H,
                                                        triggerDetailWhy);
                } else if (trigger ==
                           RewriteFallbackTrigger::RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED) {
                    noteSequenceXIncidentVirtualSubtype(xIncidentSubtype,
                                                        originalCore,
                                                        rNode,
                                                        x,
                                                        triggerDetailWhy);
                    if (xIncidentSubtype == XIncidentVirtualSubtype::XIV_SHARED_WITH_LOOP) {
                        noteSequenceXSharedResidualSubtype(
                            xSharedResidualSubtype,
                            originalCore,
                            rNode,
                            x,
                            xSharedResidualSubtype ==
                                    XSharedResidualSubtype::XSR_HAFTER_BUILD_FAIL
                                ? nullptr
                                : &xSharedHafter,
                            triggerDetailWhy);
                        if (xSharedResidualSubtype ==
                                XSharedResidualSubtype::XSR_HAFTER_LOOP_SHARED &&
                            xSharedBridgeBailout != XSharedBridgeBailout::XSB_NONE) {
                            noteSequenceXSharedLoopSharedBailout(
                                xSharedLoopSharedBailout,
                                xSharedLoopSharedInputSubtype,
                                originalCore,
                                rNode,
                                x,
                                xSharedHafter,
                                xSharedTracePtr,
                                GraftPostcheckSubtype::GPS_OTHER,
                                triggerDetailWhy);
                        }
                        if (xSharedBridgeBailout != XSharedBridgeBailout::XSB_NONE) {
                            noteSequenceXSharedBridgeBailout(
                                xSharedBridgeBailout,
                                originalCore,
                                rNode,
                                x,
                                xSharedResidualSubtype ==
                                        XSharedResidualSubtype::XSR_HAFTER_BUILD_FAIL
                                    ? nullptr
                                    : &xSharedHafter,
                                xSharedTracePtr,
                                triggerDetailWhy);
                        }
                    }
                }
                noteSequenceFallbackSample(SeqFallbackReason::OTHER,
                                           trigger,
                                           RewritePathTaken::WHOLE_CORE_REBUILD,
                                           originalCore,
                                           nullptr,
                                           rNode,
                                           x,
                                           &H,
                                           nullptr,
                                           triggerDetailWhy);
                if (attemptSequenceWholeCoreFallback(triggerWhy, nullptr, why)) {
                    return true;
                }
            } else {
                std::string fallbackWhy;
                if (attemptWholeCoreFallback(fallbackWhy)) {
                    why.clear();
                    return true;
                }
                if (!fallbackWhy.empty()) {
                    why = triggerWhy + ": " + fallbackWhy;
                } else {
                    why = triggerWhy;
                }
            }
        }
        return false;
    }

    pruneCompactGraphForSpqr(H);

    if (sequenceMode) {
        std::string proxyWhy;
        if (!validateProxyMetadataForRewrite(core, rNode, H, proxyWhy)) {
            noteSequenceFallbackSample(SeqFallbackReason::PROXY_METADATA,
                                       RewriteFallbackTrigger::RFT_PROXY_METADATA_INVALID,
                                       RewritePathTaken::WHOLE_CORE_REBUILD,
                                       originalCore,
                                       nullptr,
                                       rNode,
                                       x,
                                       &H,
                                       nullptr,
                                       proxyWhy);
            if (attemptSequenceWholeCoreFallback(proxyWhy,
                                                 &gRewriteRStats.seqProxyMetadataFallbackCount,
                                                 why)) {
                return true;
            }
            return false;
        }
    }

    std::string precheckWhy;
    if (!isCompactGraphSpqrReady(H, &rejectReason, precheckWhy)) {
        GraftTrace precheckGraftFailureTrace;
        const GraftTrace *precheckGraftFailureTracePtr = nullptr;
        RewriteFallbackTrigger precheckTrigger =
            classifyUnhandledPrecheckTrigger(rejectReason);
        std::string subtypeLabel;
        TooSmallSubtype tinySubtype = TooSmallSubtype::TS_OTHER;
        TooSmallOtherSubtype tinyOtherSubtype = TooSmallOtherSubtype::TSO_OTHER;
        SequenceOneEdgeSubtype oneEdgeSubtype = SequenceOneEdgeSubtype::SOE_OTHER;
        SelfLoopBuildFailSubtype selfLoopSubtype = SelfLoopBuildFailSubtype::SL_OTHER;
        if (rejectReason == CompactRejectReason::TOO_SMALL_FOR_SPQR) {
            tinySubtype = classifyTooSmallCompact(H);
            subtypeLabel = tooSmallSubtypeName(tinySubtype);
            noteTooSmallSubtypeReject(tinySubtype, core, rNode, x, H, precheckWhy);
            if (sequenceMode && tinySubtype == TooSmallSubtype::TS_ONE_EDGE) {
                oneEdgeSubtype = classifySequenceOneEdgeSubtype(H);
                subtypeLabel += "/";
                subtypeLabel += sequenceOneEdgeSubtypeName(oneEdgeSubtype);
                std::string oneEdgeWhy;
                if (!canHandleSequenceOneEdgeDirect(H, oneEdgeWhy)) {
                    if (!oneEdgeWhy.empty()) {
                        if (!precheckWhy.empty()) precheckWhy += " | ";
                        precheckWhy += oneEdgeWhy;
                    }
                }
                noteSequenceTooSmallOneEdgeSample(oneEdgeSubtype,
                                                  originalCore,
                                                  rNode,
                                                  x,
                                                  H,
                                                  nullptr,
                                                  oneEdgeWhy.empty() ? precheckWhy : oneEdgeWhy);
            }
            if (tinySubtype == TooSmallSubtype::TS_TWO_OTHER) {
                tinyOtherSubtype = classifyTooSmallOtherDetailed(H);
                subtypeLabel += "/";
                subtypeLabel += tooSmallOtherSubtypeName(tinyOtherSubtype);
            }
            if (sequenceMode && tinySubtype == TooSmallSubtype::TS_ONE_EDGE) {
                std::string directWhy;
                if (!canHandleSequenceOneEdgeDirect(H, directWhy)) {
                    ++gRewriteRStats.seqTooSmallOneEdgeFallbackCount;
                } else {
                    StaticMiniCore syntheticMini;
                    std::string stageWhy;
                    if (!buildSyntheticMiniForOneEdgeRemainder(H, syntheticMini, stageWhy)) {
                        ++gRewriteRStats.seqTooSmallOneEdgeFallbackCount;
                        precheckWhy = stageWhy.empty()
                                ? "rewriteR: one-edge synthetic mini build failed"
                                : "rewriteR: " + stageWhy;
                    } else {
                        int keep = -1;
                        if (!::chooseKeepMiniNode(syntheticMini, keep, stageWhy) || keep < 0) {
                            ++gRewriteRStats.seqTooSmallOneEdgeFallbackCount;
                            precheckWhy = stageWhy.empty()
                                    ? "rewriteR: one-edge chooseKeepMiniNode failed"
                                    : "rewriteR: one-edge chooseKeepMiniNode failed: " + stageWhy;
                        } else {
                            GraftTrace trace;
                            if (!attemptSequenceAwareGraft(H,
                                                           syntheticMini,
                                                           keep,
                                                           trace,
                                                           stageWhy)) {
                                core = originalCore;
                                ++gRewriteRStats.seqTooSmallOneEdgeFallbackCount;
                                precheckWhy = stageWhy.empty()
                                        ? "rewriteR: one-edge graft failed"
                                        : stageWhy;
                            } else {
                                refreshActualAfterGraft(H, trace);
                                ++gRewriteRStats.compactTooSmallHandledCount;
                                ++gRewriteRStats.seqTooSmallOneEdgeHandledCount;
                                ++gRewriteRStats.seqTooSmallOneEdgeRealNonLoopHandledCount;
                                ++gRewriteRStats.rewriteFallbackSpecialCaseCount;
                                noteRewritePathTaken(RewritePathTaken::SPECIAL_ONE_EDGE);
                                why.clear();
                                return true;
                            }
                        }
                    }
                }
            }
            if (sequenceMode &&
                tinySubtype == TooSmallSubtype::TS_TWO_OTHER &&
                tinyOtherSubtype == TooSmallOtherSubtype::TSO_LOOP_PLUS_EDGE_SHARED) {
                StaticMiniCore syntheticMini;
                std::string stageWhy;
                if (!buildSyntheticMiniForLoopPlusEdgeShared(H, syntheticMini, stageWhy)) {
                    precheckWhy = stageWhy.empty()
                            ? "rewriteR: loop+edge-shared synthetic mini build failed"
                            : "rewriteR: " + stageWhy;
                } else {
                    int keep = -1;
                    if (!::chooseKeepMiniNode(syntheticMini, keep, stageWhy) || keep < 0) {
                        precheckWhy = stageWhy.empty()
                                ? "rewriteR: loop+edge-shared chooseKeepMiniNode failed"
                                : "rewriteR: loop+edge-shared chooseKeepMiniNode failed: " + stageWhy;
                    } else {
                        if (!applyLoopPlusEdgeSharedSequenceRewrite(core, rNode, H, stageWhy)) {
                            precheckWhy = stageWhy.empty()
                                    ? "rewriteR: loop+edge-shared local apply failed"
                                    : "rewriteR: " + stageWhy;
                        } else {
                            ++gRewriteRStats.seqTooSmallOtherHandledCount;
                            ++gRewriteRStats.seqLoopPlusEdgeSharedHandledCount;
                            ++gRewriteRStats.rewriteFallbackSpecialCaseCount;
                            noteRewritePathTaken(RewritePathTaken::SPECIAL_LOOP_SHARED);
                            why.clear();
                            return true;
                        }
                    }
                }
            }
            if (tinySubtype == TooSmallSubtype::TS_TWO_PATH) {
                StaticMiniCore syntheticMini;
                std::string stageWhy;
                if (!buildSyntheticMiniForTooSmallTwoPath(H, syntheticMini, stageWhy)) {
                    why = stageWhy.empty()
                            ? "rewriteR: two-path synthetic mini build failed"
                            : "rewriteR: " + stageWhy;
                    return false;
                }

                int keep = -1;
                if (!::chooseKeepMiniNode(syntheticMini, keep, stageWhy) || keep < 0) {
                    why = stageWhy.empty()
                            ? "rewriteR: two-path chooseKeepMiniNode failed"
                            : "rewriteR: two-path chooseKeepMiniNode failed: " + stageWhy;
                    return false;
                }

                GraftTrace trace;
                if (!attemptSequenceAwareGraft(H, syntheticMini, keep, trace, stageWhy)) {
                    if (shouldFallbackSequenceGraftFailure(&trace, stageWhy)) {
                        noteSequenceGraftRewireSubtype(effectiveGraftRewireSubtype(&trace),
                                                       originalCore,
                                                       rNode,
                                                       x,
                                                       &H,
                                                       &trace,
                                                       stageWhy);
                        noteSequenceFallbackSample(SeqFallbackReason::GRAFT_REWIRE,
                                                   RewriteFallbackTrigger::RFT_GRAFT_REWIRE_FAIL,
                                                   RewritePathTaken::WHOLE_CORE_REBUILD,
                                                   originalCore,
                                                   &core,
                                                   rNode,
                                                   x,
                                                   &H,
                                                   &trace,
                                                   stageWhy,
                                                   subtypeLabel);
                    }
                    if (shouldFallbackSequenceGraftFailure(&trace, stageWhy) &&
                        attemptSequenceWholeCoreFallback(stageWhy,
                                                         &gRewriteRStats.seqGraftRewireFallbackCount,
                                                         why)) {
                        return true;
                    }
                    why = stageWhy.empty()
                            ? "rewriteR: two-path graft failed"
                            : stageWhy;
                    return false;
                }

                refreshActualAfterGraft(H, trace);
                ++gRewriteRStats.compactTooSmallHandledCount;
                ++gRewriteRStats.compactTooSmallTwoPathHandledCount;
                ++gRewriteRStats.rewriteFallbackSpecialCaseCount;
                noteRewritePathTaken(RewritePathTaken::SPECIAL_TWO_PATH);
                why.clear();
                return true;
            }
        } else if (rejectReason == CompactRejectReason::SELF_LOOP) {
            std::string selfLoopWhy;
            selfLoopSubtype = classifySelfLoopBuildFailDetailed(H, selfLoopWhy);
            subtypeLabel = selfLoopBuildFailSubtypeName(selfLoopSubtype);
            if (sequenceMode &&
                selfLoopSubtype ==
                    SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_OTHER_NOT_BICONNECTED) {
                ++gRewriteRStats.seqSelfLoopRemainderOneEdgeAttemptCount;

                StaticMiniCore syntheticMini;
                int keep = -1;
                std::string stageWhy;
                if (!buildSequenceMiniForSelfLoopRemainderOneEdge(H,
                                                                  syntheticMini,
                                                                  keep,
                                                                  stageWhy)) {
                    ++gRewriteRStats.seqSelfLoopRemainderOneEdgeFallbackCount;
                    noteSequenceSelfLoopOneEdgeSample("fallback",
                                                      originalCore,
                                                      rNode,
                                                      x,
                                                      H,
                                                      keep,
                                                      nullptr,
                                                      stageWhy);
                    precheckWhy = stageWhy.empty()
                            ? "rewriteR: self-loop tiny remainder synthetic mini build failed"
                            : "rewriteR: " + stageWhy;
                } else {
                    noteSequenceSelfLoopOneEdgeSample("attempt",
                                                      originalCore,
                                                      rNode,
                                                      x,
                                                      H,
                                                      keep,
                                                      nullptr,
                                                      "rewriteR: self-loop tiny remainder local path attempt");
                    GraftTrace trace;
                    if (!attemptSequenceAwareGraft(H,
                                                   syntheticMini,
                                                   keep,
                                                   trace,
                                                   stageWhy)) {
                        core = originalCore;
                        ++gRewriteRStats.seqSelfLoopRemainderOneEdgeFallbackCount;
                        noteSequenceSelfLoopOneEdgeSample("fallback",
                                                          originalCore,
                                                          rNode,
                                                          x,
                                                          H,
                                                          keep,
                                                          &trace,
                                                          stageWhy);
                        precheckWhy = stageWhy.empty()
                                ? "rewriteR: self-loop tiny remainder graft failed"
                                : stageWhy;
                    } else {
                        refreshActualAfterGraft(H, trace);
                        rebuildAllOccurrencesActual(core);
                        ++gRewriteRStats.seqSelfLoopRemainderOneEdgeHandledCount;
                        ++gRewriteRStats.rewriteFallbackSpecialCaseCount;
                        noteSequenceSelfLoopOneEdgeSample("success",
                                                          originalCore,
                                                          rNode,
                                                          x,
                                                          H,
                                                          keep,
                                                          &trace,
                                                          "rewriteR: self-loop tiny remainder handled");
                        noteRewritePathTaken(RewritePathTaken::SPECIAL_SELF_LOOP_ONE_EDGE);
                        why.clear();
                        return true;
                    }
                }
            } else if (sequenceMode &&
                selfLoopSubtype == SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_SPQR_READY) {
                ++gRewriteRStats.seqSelfLoopRemainderSpqrReadyAttemptCount;

                StaticMiniCore syntheticMini;
                int keep = -1;
                std::string stageWhy;
                if (!buildSequenceMiniForSelfLoopRemainderSpqrReady(H,
                                                                   syntheticMini,
                                                                   keep,
                                                                   stageWhy)) {
                    ++gRewriteRStats.seqSelfLoopRemainderSpqrReadyFallbackCount;
                    noteSequenceSelfLoopSpqrReadySample("fallback",
                                                        originalCore,
                                                        rNode,
                                                        x,
                                                        H,
                                                        keep,
                                                        nullptr,
                                                        stageWhy);
                    if (stageWhy.find("no mini node contains loop vertex") !=
                        std::string::npos) {
                        precheckTrigger = RewriteFallbackTrigger::RFT_CHOOSE_KEEP_FAIL;
                        precheckWhy = stageWhy.empty()
                                ? "rewriteR: self-loop remainder spqr-ready chooseKeepMiniNode failed"
                                : "rewriteR: self-loop remainder spqr-ready chooseKeepMiniNode failed: " + stageWhy;
                    } else {
                        precheckWhy = stageWhy.empty()
                                ? "rewriteR: self-loop remainder spqr-ready synthetic mini build failed"
                                : "rewriteR: " + stageWhy;
                    }
                } else {
                    noteSequenceSelfLoopSpqrReadySample("attempt",
                                                        originalCore,
                                                        rNode,
                                                        x,
                                                        H,
                                                        keep,
                                                        nullptr,
                                                        "rewriteR: self-loop remainder spqr-ready local path attempt");
                    GraftTrace trace;
                    if (!attemptSequenceAwareGraft(H,
                                                   syntheticMini,
                                                   keep,
                                                   trace,
                                                   stageWhy)) {
                        ++gRewriteRStats.seqSelfLoopRemainderSpqrReadyFallbackCount;
                        noteSequenceSelfLoopSpqrReadySample("fallback",
                                                            originalCore,
                                                            rNode,
                                                            x,
                                                            H,
                                                            keep,
                                                            &trace,
                                                            stageWhy);
                        precheckTrigger =
                            shouldFallbackSequenceGraftFailure(&trace, stageWhy)
                                ? RewriteFallbackTrigger::RFT_GRAFT_REWIRE_FAIL
                                : RewriteFallbackTrigger::RFT_GRAFT_OTHER;
                        precheckGraftFailureTrace = trace;
                        precheckGraftFailureTracePtr = &precheckGraftFailureTrace;
                        precheckWhy = stageWhy.empty()
                                ? "rewriteR: self-loop remainder spqr-ready graft failed"
                                : stageWhy;
                    } else {
                        refreshActualAfterGraft(H, trace);
                        rebuildAllOccurrencesActual(core);
                        ++gRewriteRStats.seqSelfLoopRemainderSpqrReadyHandledCount;
                        ++gRewriteRStats.rewriteFallbackSpecialCaseCount;
                        noteSequenceSelfLoopSpqrReadySample("success",
                                                            originalCore,
                                                            rNode,
                                                            x,
                                                            H,
                                                            keep,
                                                            &trace,
                                                            "rewriteR: self-loop remainder spqr-ready handled");
                        noteRewritePathTaken(RewritePathTaken::SPECIAL_SELF_LOOP_SPQR_READY);
                        why.clear();
                        return true;
                    }
                }
            } else if (sequenceMode &&
                selfLoopSubtype == SelfLoopBuildFailSubtype::SL_PROXY_ONLY_REMAINDER_TWO_PATH) {
                StaticMiniCore syntheticMini;
                std::string stageWhy;
                if (!buildSyntheticMiniForSelfLoopRemainderTwoPath(H, syntheticMini, stageWhy)) {
                    precheckWhy = stageWhy.empty()
                            ? "rewriteR: self-loop remainder two-path synthetic mini build failed"
                            : "rewriteR: " + stageWhy;
                } else {
                    int keep = -1;
                    if (!::chooseKeepMiniNode(syntheticMini, keep, stageWhy) || keep < 0) {
                        precheckTrigger = RewriteFallbackTrigger::RFT_CHOOSE_KEEP_FAIL;
                        precheckWhy = stageWhy.empty()
                                ? "rewriteR: self-loop remainder two-path chooseKeepMiniNode failed"
                                : "rewriteR: self-loop remainder two-path chooseKeepMiniNode failed: " + stageWhy;
                    } else {
                        const ReducedSPQRCore preGraftCore = core;
                        GraftTrace trace;
                        if (!attemptSequenceAwareGraft(H, syntheticMini, keep, trace, stageWhy)) {
                            ReducedSPQRCore fallbackCore = preGraftCore;
                            std::string localApplyWhy;
                            if (applySelfLoopRemainderTwoPathSequenceRewrite(fallbackCore,
                                                                            rNode,
                                                                            H,
                                                                            localApplyWhy)) {
                                core = std::move(fallbackCore);
                                ++gRewriteRStats.seqSelfLoopRemainderTwoPathHandledCount;
                                ++gRewriteRStats.rewriteFallbackSpecialCaseCount;
                                noteRewritePathTaken(RewritePathTaken::SPECIAL_SELF_LOOP_TWO_PATH);
                                why.clear();
                                return true;
                            }
                            precheckTrigger =
                                shouldFallbackSequenceGraftFailure(&trace, stageWhy)
                                    ? RewriteFallbackTrigger::RFT_GRAFT_REWIRE_FAIL
                                    : RewriteFallbackTrigger::RFT_GRAFT_OTHER;
                            precheckGraftFailureTrace = trace;
                            precheckGraftFailureTracePtr = &precheckGraftFailureTrace;
                            precheckWhy = stageWhy.empty()
                                    ? "rewriteR: self-loop remainder two-path graft failed"
                                    : stageWhy;
                        } else {
                            refreshActualAfterGraft(H, trace);
                            ++gRewriteRStats.seqSelfLoopRemainderTwoPathHandledCount;
                            ++gRewriteRStats.rewriteFallbackSpecialCaseCount;
                            noteRewritePathTaken(RewritePathTaken::SPECIAL_SELF_LOOP_TWO_PATH);
                            why.clear();
                            return true;
                        }
                    }
                }
            }
        } else if (rejectReason == CompactRejectReason::NOT_BICONNECTED) {
            CompactBCResult bc;
            std::string bcWhy;
            std::string bcValidateWhy;
            std::string subtypeWhy = precheckWhy;
            NotBiconnectedSubtype subtype = NotBiconnectedSubtype::NB_OTHER;
            if (!decomposeCompactIntoBC(H, bc, bcWhy)) {
                if (!subtypeWhy.empty() && !bcWhy.empty()) subtypeWhy += " | ";
                subtypeWhy += bcWhy;
            } else if (!validateCompactBC(H, bc, bcValidateWhy)) {
                if (!subtypeWhy.empty() && !bcValidateWhy.empty()) subtypeWhy += " | ";
                subtypeWhy += bcValidateWhy;
            } else {
                subtype = classifyNotBiconnected(H, bc);
            }
            subtypeLabel = notBiconnectedSubtypeName(subtype);
            noteNotBiconnectedSubtypeReject(subtype, core, rNode, x, H, bc, subtypeWhy);

            if (subtype == NotBiconnectedSubtype::NB_SINGLE_CUT_TWO_BLOCKS) {
                StaticMiniCore syntheticMini;
                std::string stageWhy;
                if (!buildSyntheticMiniForSingleCutTwoBlocks(H, bc, syntheticMini, stageWhy)) {
                    why = stageWhy.empty()
                            ? "rewriteR: single-cut-two-blocks synthetic mini build failed"
                            : "rewriteR: " + stageWhy;
                    return false;
                }

                int keep = -1;
                if (!::chooseKeepMiniNode(syntheticMini, keep, stageWhy) || keep < 0) {
                    why = stageWhy.empty()
                            ? "rewriteR: single-cut-two-blocks chooseKeepMiniNode failed"
                            : "rewriteR: single-cut-two-blocks chooseKeepMiniNode failed: " + stageWhy;
                    return false;
                }

                GraftTrace trace;
                if (!attemptSequenceAwareGraft(H, syntheticMini, keep, trace, stageWhy)) {
                    if (shouldFallbackSequenceGraftFailure(&trace, stageWhy)) {
                        noteSequenceGraftRewireSubtype(effectiveGraftRewireSubtype(&trace),
                                                       originalCore,
                                                       rNode,
                                                       x,
                                                       &H,
                                                       &trace,
                                                       stageWhy);
                        noteSequenceFallbackSample(SeqFallbackReason::GRAFT_REWIRE,
                                                   RewriteFallbackTrigger::RFT_GRAFT_REWIRE_FAIL,
                                                   RewritePathTaken::WHOLE_CORE_REBUILD,
                                                   originalCore,
                                                   &core,
                                                   rNode,
                                                   x,
                                                   &H,
                                                   &trace,
                                                   stageWhy,
                                                   subtypeLabel);
                    }
                    if (shouldFallbackSequenceGraftFailure(&trace, stageWhy) &&
                        attemptSequenceWholeCoreFallback(stageWhy,
                                                         &gRewriteRStats.seqGraftRewireFallbackCount,
                                                         why)) {
                        return true;
                    }
                    why = stageWhy.empty()
                            ? "rewriteR: single-cut-two-blocks graft failed"
                            : stageWhy;
                    return false;
                }

                refreshActualAfterGraft(H, trace);
                ++gRewriteRStats.compactSingleCutTwoBlocksHandled;
                ++gRewriteRStats.rewriteFallbackSpecialCaseCount;
                noteRewritePathTaken(RewritePathTaken::SPECIAL_SINGLE_CUT);
                why.clear();
                return true;
            }
            if (subtype == NotBiconnectedSubtype::NB_PATH_OF_BLOCKS) {
                StaticMiniCore syntheticMini;
                std::string stageWhy;
                if (!buildSyntheticMiniForPathOfBlocks(H, bc, syntheticMini, stageWhy)) {
                    precheckTrigger = RewriteFallbackTrigger::RFT_PATH_OF_BLOCKS_BUILDER_FAIL;
                    precheckWhy = stageWhy.empty()
                            ? "rewriteR: path-of-blocks synthetic mini build failed"
                            : "rewriteR: " + stageWhy;
                } else {
                    int keep = -1;
                    if (!::chooseKeepMiniNode(syntheticMini, keep, stageWhy) || keep < 0) {
                        precheckTrigger = RewriteFallbackTrigger::RFT_CHOOSE_KEEP_FAIL;
                        precheckWhy = stageWhy.empty()
                                ? "rewriteR: path-of-blocks chooseKeepMiniNode failed"
                                : "rewriteR: path-of-blocks chooseKeepMiniNode failed: " + stageWhy;
                    } else {
                        GraftTrace trace;
                        if (!attemptSequenceAwareGraft(H, syntheticMini, keep, trace, stageWhy)) {
                            if (shouldFallbackSequenceGraftFailure(&trace, stageWhy)) {
                                noteSequenceGraftRewireSubtype(
                                    effectiveGraftRewireSubtype(&trace),
                                    originalCore,
                                    rNode,
                                    x,
                                    &H,
                                    &trace,
                                    stageWhy);
                                noteSequenceFallbackSample(SeqFallbackReason::GRAFT_REWIRE,
                                                           RewriteFallbackTrigger::RFT_GRAFT_REWIRE_FAIL,
                                                           RewritePathTaken::WHOLE_CORE_REBUILD,
                                                           originalCore,
                                                           &core,
                                                           rNode,
                                                           x,
                                                           &H,
                                                           &trace,
                                                           stageWhy,
                                                           subtypeLabel);
                            }
                            if (shouldFallbackSequenceGraftFailure(&trace, stageWhy) &&
                                attemptSequenceWholeCoreFallback(stageWhy,
                                                                 &gRewriteRStats.seqGraftRewireFallbackCount,
                                                                 why)) {
                                return true;
                            }
                            why = stageWhy.empty()
                                    ? "rewriteR: path-of-blocks graft failed"
                                    : stageWhy;
                            return false;
                        }

                        refreshActualAfterGraft(H, trace);
                        ++gRewriteRStats.compactPathOfBlocksHandled;
                        ++gRewriteRStats.rewriteFallbackSpecialCaseCount;
                        noteRewritePathTaken(RewritePathTaken::SPECIAL_PATH);
                        why.clear();
                        return true;
                    }
                }
            }
        }

        noteCompactReject(rejectReason, core, rNode, x, &H, precheckWhy);
        const std::string triggerWhy = precheckWhy.empty()
                ? "rewriteR fallback after precheck failure"
                : "rewriteR fallback after precheck failure (" + precheckWhy + ")";
        if (sequenceMode) {
            if (precheckTrigger == RewriteFallbackTrigger::RFT_COMPACT_BUILD_FAIL) {
                CompactBuildFailSubtype precheckSubtype = CompactBuildFailSubtype::CBF_OTHER;
                if (rejectReason == CompactRejectReason::SELF_LOOP) {
                    precheckSubtype = CompactBuildFailSubtype::CBF_SELF_LOOP_PRECHECK;
                }
                noteSequenceCompactBuildFailSubtype(precheckSubtype,
                                                    originalCore,
                                                    rNode,
                                                    x,
                                                    &H,
                                                    precheckWhy);
                if (precheckSubtype == CompactBuildFailSubtype::CBF_SELF_LOOP_PRECHECK) {
                    std::string selfLoopWhy;
                    selfLoopSubtype =
                        classifySelfLoopBuildFailDetailed(H, selfLoopWhy);
                    if (!selfLoopWhy.empty()) {
                        precheckWhy += precheckWhy.empty() ? "" : " | ";
                        precheckWhy += selfLoopWhy;
                    }
                    noteSequenceSelfLoopSubtype(selfLoopSubtype,
                                                originalCore,
                                                rNode,
                                                x,
                                                H,
                                                precheckWhy);
                    if (selfLoopSubtype == SelfLoopBuildFailSubtype::
                                               SL_PROXY_ONLY_REMAINDER_OTHER_NOT_BICONNECTED) {
                        std::string otherNBWhy;
                        const auto otherNBSubtype =
                            classifySelfLoopOtherNotBiconnectedDetailed(H, otherNBWhy);
                        std::string detailedWhy = precheckWhy;
                        if (!otherNBWhy.empty()) {
                            if (!detailedWhy.empty()) detailedWhy += " | ";
                            detailedWhy += otherNBWhy;
                        }
                        noteSequenceSelfLoopOtherNBSubtype(otherNBSubtype,
                                                           originalCore,
                                                           rNode,
                                                           x,
                                                           H,
                                                           detailedWhy);
                    }
                }
            }
            if (precheckTrigger == RewriteFallbackTrigger::RFT_COMPACT_TOO_SMALL_UNHANDLED) {
                noteSequenceTooSmallDetailedSample(tinySubtype,
                                                   tinyOtherSubtype,
                                                   originalCore,
                                                   rNode,
                                                   x,
                                                   H,
                                                   precheckTrigger,
                                                   precheckWhy);
            }
            if (precheckTrigger == RewriteFallbackTrigger::RFT_GRAFT_REWIRE_FAIL) {
                noteSequenceGraftRewireSubtype(
                    effectiveGraftRewireSubtype(precheckGraftFailureTracePtr),
                    originalCore,
                    rNode,
                    x,
                    &H,
                    precheckGraftFailureTracePtr,
                    precheckWhy);
            }
            noteSequenceFallbackSample(SeqFallbackReason::OTHER,
                                       precheckTrigger,
                                       RewritePathTaken::WHOLE_CORE_REBUILD,
                                       originalCore,
                                       nullptr,
                                       rNode,
                                       x,
                                       &H,
                                       nullptr,
                                       precheckWhy,
                                       subtypeLabel);
            if (attemptSequenceWholeCoreFallback(triggerWhy, nullptr, why)) {
                return true;
            }
        } else {
            std::string fallbackWhy;
            if (attemptWholeCoreFallback(fallbackWhy)) {
                why.clear();
                return true;
            }
            if (!fallbackWhy.empty()) {
                why = triggerWhy + ": " + fallbackWhy;
            } else {
                why = triggerWhy;
            }
        }
        return false;
    }
    ++gRewriteRStats.compactReadyCount;

    OgdfRawSpqrBackend backend;
    RawSpqrDecomp raw;
    std::string err;
    if (!backend.buildRaw(H, raw, err)) {
        if (shouldFallbackToWholeCoreRebuild(err)) {
            ++gRewriteRStats.backendBuildRawFallbackCount;
            const RewriteFallbackTrigger trigger =
                classifyBackendBuildRawTrigger(err);
            const std::string triggerWhy = err.empty()
                    ? "rewriteR fallback after backend failure"
                    : "rewriteR fallback after backend failure (" + err + ")";
            if (sequenceMode) {
                noteSequenceFallbackSample(SeqFallbackReason::OTHER,
                                           trigger,
                                           RewritePathTaken::WHOLE_CORE_REBUILD,
                                           originalCore,
                                           nullptr,
                                           rNode,
                                           x,
                                           &H,
                                           nullptr,
                                           err);
                if (attemptSequenceWholeCoreFallback(triggerWhy, nullptr, why)) {
                    return true;
                }
            } else {
                std::string fallbackWhy;
                if (attemptWholeCoreFallback(fallbackWhy)) {
                    why.clear();
                    return true;
                }
                if (!fallbackWhy.empty()) {
                    why = triggerWhy + ": " + fallbackWhy;
                } else {
                    why = std::string("rewriteR: backend.buildRaw failed: ") +
                          (err.empty() ? "raw backend failed" : err);
                }
            }
            return false;
        }
        why = std::string("rewriteR: backend.buildRaw failed: ") +
              (err.empty() ? "raw backend failed" : err);
        return false;
    }
    ++gRewriteRStats.backendBuildRawDirectCount;

    std::string stageWhy;
    if (!::validateRawSpqrDecomp(H, raw, stageWhy)) {
        why = stageWhy.empty()
                ? "rewriteR: raw validation failed"
                : "rewriteR: raw validation failed: " + stageWhy;
        return false;
    }

    StaticMiniCore mini;
    if (!::materializeMiniCore(H, raw, mini, stageWhy)) {
        why = stageWhy.empty()
                ? "rewriteR: materializeMiniCore failed"
                : "rewriteR: materializeMiniCore failed: " + stageWhy;
        return false;
    }

    try {
        ::normalizeWholeMiniCore(mini);
    } catch (const std::exception &e) {
        why = std::string("rewriteR: normalizeWholeMiniCore threw: ") + e.what();
        return false;
    } catch (...) {
        why = "rewriteR: normalizeWholeMiniCore threw";
        return false;
    }

    int keep = -1;
    if (!::chooseKeepMiniNode(mini, keep, stageWhy)) {
        if (keep < 0) {
            why = "rewriteR: no alive mini node after normalize";
        } else {
            why = stageWhy.empty()
                    ? "rewriteR: chooseKeepMiniNode failed"
                    : "rewriteR: chooseKeepMiniNode failed: " + stageWhy;
        }
        return false;
    }
    if (keep < 0) {
        why = "rewriteR: no alive mini node after normalize";
        return false;
    }

    GraftTrace trace;
    if (!attemptSequenceAwareGraft(H, mini, keep, trace, stageWhy)) {
        if (shouldFallbackSequenceGraftFailure(&trace, stageWhy)) {
            noteSequenceGraftRewireSubtype(effectiveGraftRewireSubtype(&trace),
                                           originalCore,
                                           rNode,
                                           x,
                                           &H,
                                           &trace,
                                           stageWhy);
            noteSequenceFallbackSample(SeqFallbackReason::GRAFT_REWIRE,
                                       RewriteFallbackTrigger::RFT_GRAFT_REWIRE_FAIL,
                                       RewritePathTaken::WHOLE_CORE_REBUILD,
                                       originalCore,
                                       &core,
                                       rNode,
                                       x,
                                       &H,
                                       &trace,
                                       stageWhy);
        }
        if (shouldFallbackSequenceGraftFailure(&trace, stageWhy) &&
            attemptSequenceWholeCoreFallback(stageWhy,
                                             &gRewriteRStats.seqGraftRewireFallbackCount,
                                             why)) {
            return true;
        }
        why = stageWhy;
        return false;
    }

    refreshActualAfterGraft(H, trace);
    noteRewritePathTaken(RewritePathTaken::DIRECT_SPQR);

    return true;
}

bool normalizeProjectTouchedRegion(ReducedSPQRCore &core,
                                   std::string &why) {
    why.clear();

    std::vector<NodeId> aliveNodes;
    aliveNodes.reserve(core.nodes.size());
    std::unordered_set<VertexId> touched;

    for (NodeId nodeId = 0; nodeId < static_cast<NodeId>(core.nodes.size()); ++nodeId) {
        const auto &node = core.nodes[nodeId];
        if (!node.alive) continue;
        aliveNodes.push_back(nodeId);
        for (const auto &slot : node.slots) {
            if (!slot.alive) continue;
            touched.insert(slot.poleA);
            touched.insert(slot.poleB);
        }
    }

    if (aliveNodes.empty()) {
        core.root = -1;
        core.totalAgg = {};
        rebuildAllOccurrencesActual(core);
        return true;
    }

    for (NodeId nodeId : aliveNodes) {
        auto &node = core.nodes[nodeId];
        rebuildActualRealEdgesHereNode(node);
        node.localAgg = recomputeActualLocalAgg(node, touched);
        node.subAgg = node.localAgg;
    }

    if (!validActualNodeId(core, core.root) || !core.nodes[core.root].alive) {
        core.root = aliveNodes.front();
    }

    recomputeWholeActualTotalAgg(core, touched);
    rebuildAllOccurrencesActual(core);
    return true;
}

void rebuildAllOccurrencesActual(ReducedSPQRCore &core) {
    core.occ.clear();

    for (NodeId nodeId = 0; nodeId < static_cast<NodeId>(core.nodes.size()); ++nodeId) {
        const auto &node = core.nodes[nodeId];
        if (!node.alive) continue;

        for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
            const auto &slot = node.slots[slotId];
            if (!slot.alive) continue;

            if (slot.poleA != -1) {
                core.occ[slot.poleA].push_back({nodeId, slotId});
            }
            if (slot.poleB != -1 && slot.poleB != slot.poleA) {
                core.occ[slot.poleB].push_back({nodeId, slotId});
            }
        }
    }
}

void resetRewriteRStats() {
    gRewriteRStats = {};
    gRewriteRCaseContext = {};
}

RewriteRStats getRewriteRStats() {
    return gRewriteRStats;
}

void recordRewriteRPass(bool manualCase) {
    if (manualCase) {
        ++gRewriteRStats.rewriteManualPassCount;
    } else {
        ++gRewriteRStats.rewriteRandomPassCount;
    }
}

void setRewriteRCaseContext(uint64_t seed, int tc) {
    gRewriteRCaseContext.seed = seed;
    gRewriteRCaseContext.tc = tc;
}

void setRewriteRSequenceMode(bool enabled) {
    gRewriteRCaseContext.sequenceMode = enabled;
}

void setRewriteRSequenceStepContext(int stepIndex, int sequenceLengthSoFar) {
    gRewriteRCaseContext.stepIndex = stepIndex;
    gRewriteRCaseContext.sequenceLengthSoFar = sequenceLengthSoFar;
    gWeakRepairPipelineContext = {};
}

void noteRewriteSeqCaseStart() {
    ++gRewriteRStats.rewriteSeqCalls;
    gWeakRepairPipelineContext = {};
    gRewriteRCaseContext.currentRNode = -1;
    gRewriteRCaseContext.currentX = -1;
    gRewriteRCaseContext.seenFallbackTriggers.fill(false);
    gRewriteRCaseContext.seenTooSmallOtherSubtypes.fill(false);
    gRewriteRCaseContext.seenCompactBuildFailSubtypes.fill(false);
    gRewriteRCaseContext.seenSelfLoopSubtypes.fill(false);
    gRewriteRCaseContext.seenSelfLoopOtherNBSubtypes.fill(false);
    gRewriteRCaseContext.seenXIncidentVirtualSubtypes.fill(false);
    gRewriteRCaseContext.seenXSharedResidualSubtypes.fill(false);
    gRewriteRCaseContext.seenXSharedLoopSharedInputSubtypes.fill(false);
    gRewriteRCaseContext.seenXSharedBridgeBailouts.fill(false);
    gRewriteRCaseContext.seenGraftRewireSubtypes.fill(false);
    gRewriteRCaseContext.seenGraftOtherSubtypes.fill(false);
    gRewriteRCaseContext.seenPostcheckSubtypes.fill(false);
    gRewriteRCaseContext.sawDeferredSameTypeSP = false;
    gRewriteRCaseContext.sawSameTypeSPCleanup = false;
    gRewriteRCaseContext.seenProxyRepairNoCandidateSubtypes.fill(false);
    gRewriteRCaseContext.seenProxyArcLifecycleBadPhases.fill(false);
    setRewriteRSequenceReplayCaptureEnabled(false);
    clearSequenceDeferredSameTypeSP();
}

void noteRewriteSeqCaseFinish(bool success,
                              int stepsTaken,
                              bool hadFallback,
                              bool maxStepReached) {
    if (success) {
        ++gRewriteRStats.rewriteSeqSucceededCases;
        if (stepsTaken >= 0 &&
            stepsTaken < static_cast<int>(kRewriteSeqLengthHistogramSize)) {
            ++gRewriteRStats.sequenceLengthHistogram[stepsTaken];
        }
    } else {
        ++gRewriteRStats.rewriteSeqFailedCases;
    }
    if (hadFallback) {
        ++gRewriteRStats.seqFallbackCaseCount;
    }
    if (maxStepReached) {
        ++gRewriteRStats.rewriteSeqMaxStepReachedCount;
    }
    gWeakRepairPipelineContext = {};
}

void noteRewriteRWeakRepairCommitOutcome(WeakRepairCommitOutcome outcome,
                                         const ReducedSPQRCore *afterCore,
                                         std::string why) {
    noteWeakRepairCommitSample(outcome, afterCore, why);
}

void materializeProjectWholeCoreExplicit(const ReducedSPQRCore &core,
                                         ProjectExplicitBlockGraph &out) {
    out = {};

    std::unordered_map<EdgeId, int> seenRealEdge;
    std::unordered_set<VertexId> seenVertex;

    for (int nodeId = 0; nodeId < static_cast<int>(core.nodes.size()); ++nodeId) {
        if (!validActualNodeId(core, nodeId)) continue;
        const auto &node = core.nodes[nodeId];
        if (!node.alive) continue;

        for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
            if (!validActualSlotId(node, slotId)) continue;
            const auto &slot = node.slots[slotId];
            if (!slot.alive || slot.isVirtual) continue;

            ++seenRealEdge[slot.realEdge];
            out.edges.push_back(makeProjectExplicitEdge(slot.realEdge, slot.poleA, slot.poleB));
            seenVertex.insert(slot.poleA);
            seenVertex.insert(slot.poleB);
        }
    }

    out.vertices.assign(seenVertex.begin(), seenVertex.end());
    canonicalizeProjectExplicitGraph(out);
}

void materializeProjectCompactRealProjection(const CompactGraph &H,
                                             ProjectExplicitBlockGraph &out) {
    out = {};

    std::unordered_map<EdgeId, int> seenRealEdge;
    std::unordered_set<VertexId> seenVertex;

    for (const auto &edge : H.edges) {
        if (edge.kind != CompactEdgeKind::REAL) continue;
        if (edge.a < 0 || edge.a >= static_cast<int>(H.origOfCv.size()) ||
            edge.b < 0 || edge.b >= static_cast<int>(H.origOfCv.size())) {
            continue;
        }

        const VertexId u = H.origOfCv[edge.a];
        const VertexId v = H.origOfCv[edge.b];
        ++seenRealEdge[edge.realEdge];
        out.edges.push_back(makeProjectExplicitEdge(edge.realEdge, u, v));
        seenVertex.insert(u);
        seenVertex.insert(v);
    }

    out.vertices.assign(seenVertex.begin(), seenVertex.end());
    canonicalizeProjectExplicitGraph(out);
}

void exportProjectExplicitBlockGraph(const ProjectExplicitBlockGraph &in,
                                     ExplicitBlockGraph &out) {
    out = {};
    out.edges.reserve(in.edges.size());
    for (const auto &edge : in.edges) {
        out.edges.push_back({edge.id, edge.u, edge.v});
    }
    out.vertices = in.vertices;
}

void importExplicitBlockGraph(const ExplicitBlockGraph &in,
                              ProjectExplicitBlockGraph &out) {
    out = {};
    out.edges.reserve(in.edges.size());
    for (const auto &edge : in.edges) {
        out.edges.push_back({edge.id, edge.u, edge.v});
    }
    out.vertices = in.vertices;
}

bool checkProjectEquivalentExplicitGraphs(const ProjectExplicitBlockGraph &got,
                                          const ProjectExplicitBlockGraph &exp,
                                          std::string &why) {
    why.clear();

    ProjectExplicitBlockGraph expected = exp;
    ProjectExplicitBlockGraph actual = got;
    canonicalizeProjectExplicitGraph(expected);
    canonicalizeProjectExplicitGraph(actual);

    if (expected.edges.size() != actual.edges.size()) {
        std::ostringstream oss;
        oss << "explicit graph edge count mismatch: expected "
            << expected.edges.size() << " got " << actual.edges.size();
        why = oss.str();
        return false;
    }

    for (size_t i = 0; i < expected.edges.size(); ++i) {
        const auto &expectedEdge = expected.edges[i];
        const auto &actualEdge = actual.edges[i];
        if (expectedEdge.id != actualEdge.id ||
            expectedEdge.u != actualEdge.u ||
            expectedEdge.v != actualEdge.v) {
            std::ostringstream oss;
            oss << "explicit graph mismatch at index " << i
                << ": expected " << formatExplicitEdge(expectedEdge)
                << " got " << formatExplicitEdge(actualEdge);
            why = oss.str();
            return false;
        }
    }

    if (expected.vertices.size() != actual.vertices.size()) {
        std::ostringstream oss;
        oss << "explicit graph vertex count mismatch: expected "
            << expected.vertices.size() << " got " << actual.vertices.size();
        why = oss.str();
        return false;
    }

    for (size_t i = 0; i < expected.vertices.size(); ++i) {
        if (expected.vertices[i] != actual.vertices[i]) {
            std::ostringstream oss;
            oss << "explicit graph vertex mismatch at index " << i
                << ": expected " << expected.vertices[i]
                << " got " << actual.vertices[i];
            why = oss.str();
            return false;
        }
    }

    return true;
}

bool checkProjectDummyProxyRewire(const DummyActualEnvelope &env,
                                  const StaticMiniCore &mini,
                                  const GraftTrace &trace,
                                  std::string &why) {
    why.clear();

    int proxyCount = 0;
    for (const auto &edge : env.H.edges) {
        if (edge.kind == CompactEdgeKind::PROXY) ++proxyCount;
    }
    if (proxyCount == 0) return true;

    for (int inputId = 0; inputId < static_cast<int>(env.H.edges.size()); ++inputId) {
        const auto &edge = env.H.edges[inputId];
        if (edge.kind != CompactEdgeKind::PROXY) continue;

        const auto inputLabel = std::to_string(edge.id);
        if (!validActualNodeId(env.core, edge.outsideNode) ||
            !env.core.nodes[edge.outsideNode].alive) {
            why = "proxy rewire: outsideNode invalid for input " + inputLabel;
            return false;
        }
        if (edge.oldArc < 0 ||
            edge.oldArc >= static_cast<int>(env.core.arcs.size()) ||
            !env.core.arcs[edge.oldArc].alive) {
            why = "proxy rewire: oldArc invalid for input " + inputLabel;
            return false;
        }

        if (inputId >= static_cast<int>(mini.ownerOfInputEdge.size())) {
            why = "proxy rewire: mini owner missing for input " + inputLabel;
            return false;
        }
        const auto owner = mini.ownerOfInputEdge[inputId];
        if (owner.first < 0 ||
            owner.first >= static_cast<int>(mini.nodes.size()) ||
            owner.second < 0 ||
            owner.second >= static_cast<int>(mini.nodes[owner.first].slots.size())) {
            why = "proxy rewire: mini owner missing for input " + inputLabel;
            return false;
        }

        const auto &ownerMiniNode = mini.nodes[owner.first];
        const auto &ownerMiniSlot = ownerMiniNode.slots[owner.second];
        if (!ownerMiniNode.alive ||
            !ownerMiniSlot.alive ||
            ownerMiniSlot.kind != MiniSlotKind::PROXY_INPUT) {
            why = "proxy rewire: owner slot is not PROXY_INPUT for input " + inputLabel;
            return false;
        }

        if (owner.first < 0 ||
            owner.first >= static_cast<int>(trace.actualOfMini.size()) ||
            trace.actualOfMini[owner.first] < 0 ||
            !validActualNodeId(env.core, trace.actualOfMini[owner.first]) ||
            !env.core.nodes[trace.actualOfMini[owner.first]].alive) {
            why = "proxy rewire: trace.actualOfMini missing for ownerMini";
            return false;
        }

        if (owner.first < 0 ||
            owner.first >= static_cast<int>(trace.actualSlotOfMiniSlot.size()) ||
            owner.second < 0 ||
            owner.second >= static_cast<int>(trace.actualSlotOfMiniSlot[owner.first].size())) {
            why = "proxy rewire: actual endpoint slot inconsistent";
            return false;
        }

        const NodeId actualOwner = trace.actualOfMini[owner.first];
        const int expectedActualSlot = trace.actualSlotOfMiniSlot[owner.first][owner.second];
        const auto &oldArc = env.core.arcs[edge.oldArc];

        const bool matchesAB = oldArc.a == edge.outsideNode && oldArc.b == actualOwner;
        const bool matchesBA = oldArc.b == edge.outsideNode && oldArc.a == actualOwner;
        if (!matchesAB && !matchesBA) {
            why = "proxy rewire: oldArc does not connect stub to actual owner";
            return false;
        }

        const NodeId stubNode = matchesAB ? oldArc.a : oldArc.b;
        const int stubSlotId = matchesAB ? oldArc.slotInA : oldArc.slotInB;
        const NodeId actualNode = matchesAB ? oldArc.b : oldArc.a;
        const int actualSlotId = matchesAB ? oldArc.slotInB : oldArc.slotInA;

        if (actualNode != actualOwner ||
            expectedActualSlot < 0 ||
            actualSlotId != expectedActualSlot ||
            !validActualSlotId(env.core.nodes[actualNode], actualSlotId)) {
            why = "proxy rewire: actual endpoint slot inconsistent";
            return false;
        }
        if (!validActualSlotId(env.core.nodes[stubNode], stubSlotId)) {
            why = "proxy rewire: stub endpoint slot inconsistent";
            return false;
        }

        const auto &actualSlot = env.core.nodes[actualNode].slots[actualSlotId];
        if (!actualSlot.alive || !actualSlot.isVirtual || actualSlot.arcId != edge.oldArc) {
            why = "proxy rewire: actual endpoint slot inconsistent";
            return false;
        }

        const auto &stubSlot = env.core.nodes[stubNode].slots[stubSlotId];
        if (!stubSlot.alive || !stubSlot.isVirtual || stubSlot.arcId != edge.oldArc) {
            why = "proxy rewire: stub endpoint slot inconsistent";
            return false;
        }

        if (edge.a < 0 || edge.a >= static_cast<int>(env.H.origOfCv.size()) ||
            edge.b < 0 || edge.b >= static_cast<int>(env.H.origOfCv.size())) {
            why = "proxy rewire: pole pair mismatch";
            return false;
        }
        const auto expectedPoles = canonPole(env.H.origOfCv[edge.a], env.H.origOfCv[edge.b]);
        if (canonPole(actualSlot.poleA, actualSlot.poleB) != expectedPoles ||
            canonPole(stubSlot.poleA, stubSlot.poleB) != expectedPoles) {
            why = "proxy rewire: pole pair mismatch";
            return false;
        }
    }

    return true;
}

} // namespace harness
