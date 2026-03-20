#include "harness/project_static_adapter.hpp"
#include "harness/dump.hpp"
#include "harness/ogdf_wrapper.hpp"

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
    std::array<bool, kRewriteFallbackTriggerCount> seenFallbackTriggers{};
    std::array<bool, kTooSmallOtherSubtypeCount> seenTooSmallOtherSubtypes{};
};

RewriteRStats gRewriteRStats;
RewriteRCaseContext gRewriteRCaseContext;

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

size_t seqFallbackReasonIndex(SeqFallbackReason reason) {
    return static_cast<size_t>(reason);
}

size_t rewriteFallbackTriggerIndex(RewriteFallbackTrigger trigger) {
    return static_cast<size_t>(trigger);
}

size_t rewritePathTakenIndex(RewritePathTaken path) {
    return static_cast<size_t>(path);
}

void setRejectReason(CompactRejectReason *dst, CompactRejectReason reason) {
    if (dst) *dst = reason;
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
           why.find("invalid oldArc/outsideNode metadata") != std::string::npos;
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

const char *rewritePathTakenName(RewritePathTaken path) {
    switch (path) {
        case RewritePathTaken::DIRECT_SPQR: return "DIRECT_SPQR";
        case RewritePathTaken::SPECIAL_SINGLE_CUT: return "SPECIAL_SINGLE_CUT";
        case RewritePathTaken::SPECIAL_TWO_PATH: return "SPECIAL_TWO_PATH";
        case RewritePathTaken::SPECIAL_PATH: return "SPECIAL_PATH";
        case RewritePathTaken::SPECIAL_LOOP_SHARED: return "SPECIAL_LOOP_SHARED";
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
                                       std::string &why) {
    out = {};
    why.clear();
    setRejectReason(reason, CompactRejectReason::OTHER);

    if (!validActualNodeId(core, rNode) || !core.nodes[rNode].alive) {
        setRejectReason(reason, CompactRejectReason::OWNER_NOT_R);
        return fail(why, "rewriteR: chosen rNode is dead or invalid");
    }
    const auto &node = core.nodes[rNode];
    if (node.type != SPQRType::R_NODE) {
        setRejectReason(reason, CompactRejectReason::OWNER_NOT_R);
        return fail(why, "rewriteR: chosen node is not R_NODE");
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
        return fail(why, "rewriteR: x does not occur on chosen R-node");
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
                return fail(why, "rewriteR: failed to map REAL slot poles into compact vertices");
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
            return fail(why, "rewriteR: virtual slot incident to x unsupported in first pass");
        }
        if (!validActualArcId(core, slot.arcId) || !core.arcs[slot.arcId].alive) {
            return fail(why, "rewriteR: virtual slot arc backlink invalid");
        }

        const auto &arc = core.arcs[slot.arcId];
        const NodeId outsideNode = otherEndpointOfArc(arc, rNode);
        if (!validActualNodeId(core, outsideNode) || !core.nodes[outsideNode].alive) {
            return fail(why, "rewriteR: virtual slot outsideNode invalid");
        }

        Agg sideAgg = computeAggToward(core, rNode, outsideNode);
        if (sideAgg.edgeCnt == 0) continue;

        const auto itA = out.cvOfOrig.find(slot.poleA);
        const auto itB = out.cvOfOrig.find(slot.poleB);
        if (itA == out.cvOfOrig.end() || itB == out.cvOfOrig.end()) {
            return fail(why, "rewriteR: failed to map PROXY slot poles into compact vertices");
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
    const bool sequenceMode = gRewriteRCaseContext.sequenceMode;
    const ReducedSPQRCore originalCore = sequenceMode ? core : ReducedSPQRCore{};

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

    CompactGraph H;
    CompactRejectReason rejectReason = CompactRejectReason::OTHER;
    if (!buildProjectCompactLocalViewFromR(core, rNode, x, H, &rejectReason, why)) {
        if (isRecoverableCompactReject(rejectReason)) {
            const RewriteFallbackTrigger trigger =
                classifyCompactBuildFallbackTrigger(rejectReason, why);
            noteCompactReject(rejectReason, core, rNode, x, &H, why);
            const std::string triggerWhy =
                "rewriteR fallback after local compact reject (" +
                std::string(compactRejectReasonName(rejectReason)) + ")";
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
                                           why);
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
        RewriteFallbackTrigger precheckTrigger =
            classifyUnhandledPrecheckTrigger(rejectReason);
        std::string subtypeLabel;
        TooSmallSubtype tinySubtype = TooSmallSubtype::TS_OTHER;
        TooSmallOtherSubtype tinyOtherSubtype = TooSmallOtherSubtype::TSO_OTHER;
        if (rejectReason == CompactRejectReason::TOO_SMALL_FOR_SPQR) {
            tinySubtype = classifyTooSmallCompact(H);
            subtypeLabel = tooSmallSubtypeName(tinySubtype);
            noteTooSmallSubtypeReject(tinySubtype, core, rNode, x, H, precheckWhy);
            if (tinySubtype == TooSmallSubtype::TS_TWO_OTHER) {
                tinyOtherSubtype = classifyTooSmallOtherDetailed(H);
                subtypeLabel += "/";
                subtypeLabel += tooSmallOtherSubtypeName(tinyOtherSubtype);
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

                std::queue<NodeId> q;
                GraftTrace trace;
                if (!::graftMiniCoreIntoPlace(core, rNode, H, syntheticMini, keep, q, &trace, stageWhy)) {
                    if (shouldFallbackSequenceProxyGraftFailure(stageWhy)) {
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
                    if (shouldFallbackSequenceProxyGraftFailure(stageWhy) &&
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

                std::queue<NodeId> q;
                GraftTrace trace;
                if (!::graftMiniCoreIntoPlace(core, rNode, H, syntheticMini, keep, q, &trace, stageWhy)) {
                    if (shouldFallbackSequenceProxyGraftFailure(stageWhy)) {
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
                    if (shouldFallbackSequenceProxyGraftFailure(stageWhy) &&
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
                        std::queue<NodeId> q;
                        GraftTrace trace;
                        if (!::graftMiniCoreIntoPlace(core, rNode, H, syntheticMini, keep, q, &trace, stageWhy)) {
                            if (shouldFallbackSequenceProxyGraftFailure(stageWhy)) {
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
                            if (shouldFallbackSequenceProxyGraftFailure(stageWhy) &&
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

    std::queue<NodeId> q;
    GraftTrace trace;
    if (!::graftMiniCoreIntoPlace(core, rNode, H, mini, keep, q, &trace, stageWhy)) {
        if (shouldFallbackSequenceProxyGraftFailure(stageWhy)) {
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
        if (shouldFallbackSequenceProxyGraftFailure(stageWhy) &&
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
}

void noteRewriteSeqCaseStart() {
    ++gRewriteRStats.rewriteSeqCalls;
    gRewriteRCaseContext.seenFallbackTriggers.fill(false);
    gRewriteRCaseContext.seenTooSmallOtherSubtypes.fill(false);
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
