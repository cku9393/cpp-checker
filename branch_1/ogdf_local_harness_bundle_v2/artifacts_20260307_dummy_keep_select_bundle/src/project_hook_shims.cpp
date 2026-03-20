#include "harness/project_static_adapter.hpp"
#include "harness/types.hpp"
#include <string>

namespace {

bool failShim(std::string &why, const char *hook, const char *target) {
    why = std::string("project_hook_shims.cpp: TODO wire ") + hook +
          " -> " + target;
    return false;
}

bool validCompactVertex(const harness::CompactGraph &H, int cv) {
    return cv >= 0 && cv < static_cast<int>(H.origOfCv.size());
}

void addOccurrence(harness::ReducedSPQRCore &core,
                   harness::NodeId nodeId,
                   int slotId,
                   harness::VertexId v) {
    core.occ[v].push_back({nodeId, slotId});
}

int watchedCountForPoles(const std::unordered_set<harness::VertexId> &touched,
                         harness::VertexId a,
                         harness::VertexId b) {
    int watched = 0;
    if (touched.count(a)) ++watched;
    if (b != a && touched.count(b)) ++watched;
    return watched;
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

bool buildDummyActualCoreEnvelope(const harness::CompactGraph &H,
                                  harness::DummyActualEnvelope &env,
                                  std::string &why) {
    env = {};
    env.H = H;
    env.root = 0;
    env.oldR = 0;
    env.core.blockId = H.block;
    env.core.root = env.root;
    env.core.nodes.resize(1 + H.edges.size());
    env.stubOfInputEdge.assign(H.edges.size(), -1);
    env.arcOfInputEdge.assign(H.edges.size(), -1);

    auto &oldR = env.core.nodes[env.oldR];
    oldR.alive = true;
    oldR.type = harness::SPQRType::R_NODE;

    std::unordered_set<harness::VertexId> touched(H.touchedVertices.begin(),
                                                  H.touchedVertices.end());
    std::unordered_set<harness::VertexId> liveVertices;
    bool haveRepEdge = false;

    for (int inputId = 0; inputId < static_cast<int>(H.edges.size()); ++inputId) {
        const auto &edge = H.edges[inputId];
        if (!validCompactVertex(H, edge.a) || !validCompactVertex(H, edge.b)) {
            why = "buildDummyActualCoreEnvelope: compact edge endpoint out of range";
            return false;
        }

        const harness::VertexId poleA = H.origOfCv[edge.a];
        const harness::VertexId poleB = H.origOfCv[edge.b];
        const harness::NodeId stubId = 1 + inputId;
        auto &stub = env.core.nodes[stubId];
        stub.alive = true;
        stub.type = harness::SPQRType::R_NODE;

        env.stubNodes.insert(stubId);
        env.stubOfInputEdge[inputId] = stubId;

        const int oldSlotId = static_cast<int>(oldR.slots.size());
        oldR.slots.push_back({true, poleA, poleB, true, -1, -1});

        int stubVirtualSlotId = -1;
        if (edge.kind == harness::CompactEdgeKind::REAL) {
            const int realSlotId = static_cast<int>(stub.slots.size());
            const int watched = watchedCountForPoles(touched, poleA, poleB);
            stub.slots.push_back({true, poleA, poleB, false, edge.realEdge, -1});
            stub.realEdgesHere.push_back(edge.realEdge);
            stub.localAgg = {1, 2, watched, 0, edge.realEdge, poleA};
            stub.subAgg = stub.localAgg;
            env.core.ownerNodeOfRealEdge[edge.realEdge] = stubId;
            env.core.ownerSlotOfRealEdge[edge.realEdge] = realSlotId;
            ++env.core.totalAgg.edgeCnt;
            if (!haveRepEdge) {
                env.core.totalAgg.repEdge = edge.realEdge;
                env.core.totalAgg.repVertex = poleA;
                haveRepEdge = true;
            }
            liveVertices.insert(poleA);
            liveVertices.insert(poleB);
        } else {
            stub.localAgg = edge.sideAgg;
            stub.localAgg.repVertex = poleA;
            stub.subAgg = stub.localAgg;
        }

        stubVirtualSlotId = static_cast<int>(stub.slots.size());
        const harness::ArcId arcId = static_cast<harness::ArcId>(env.core.arcs.size());
        oldR.slots[oldSlotId].arcId = arcId;
        stub.slots.push_back({true, poleA, poleB, true, -1, arcId});
        oldR.adjArcs.push_back(arcId);
        stub.adjArcs.push_back(arcId);
        env.core.arcs.push_back({true, env.oldR, stubId, oldSlotId, stubVirtualSlotId, poleA, poleB});
        env.arcOfInputEdge[inputId] = arcId;
    }

    env.core.totalAgg.vertexCnt = static_cast<int>(liveVertices.size());
    for (harness::VertexId v : liveVertices) {
        if (touched.count(v)) ++env.core.totalAgg.watchedCnt;
    }

    for (harness::NodeId nodeId = 0; nodeId < static_cast<harness::NodeId>(env.core.nodes.size()); ++nodeId) {
        const auto &node = env.core.nodes[nodeId];
        if (!node.alive) continue;
        for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
            const auto &slot = node.slots[slotId];
            if (!slot.alive) continue;
            addOccurrence(env.core, nodeId, slotId, slot.poleA);
            if (slot.poleB != slot.poleA) {
                addOccurrence(env.core, nodeId, slotId, slot.poleB);
            }
        }
    }

    return true;
}
