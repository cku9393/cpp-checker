#include "harness/project_static_adapter.hpp"
#include "harness/types.hpp"
#include <algorithm>
#include <sstream>
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

bool failGraft(std::string *why, const char *msg) {
    if (why) *why = msg;
    return false;
}

bool validCoreNode(const harness::ReducedSPQRCore &core, harness::NodeId nodeId) {
    return nodeId >= 0 && nodeId < static_cast<harness::NodeId>(core.nodes.size());
}

bool validCoreArc(const harness::ReducedSPQRCore &core, harness::ArcId arcId) {
    return arcId >= 0 && arcId < static_cast<harness::ArcId>(core.arcs.size());
}

bool validCoreSlot(const harness::SPQRNodeCore &node, int slotId) {
    return slotId >= 0 && slotId < static_cast<int>(node.slots.size());
}

bool nodeHasAliveRealSlot(const harness::SPQRNodeCore &node) {
    for (const auto &slot : node.slots) {
        if (!slot.alive || slot.isVirtual) continue;
        return true;
    }
    return false;
}

void addAdjArc(harness::SPQRNodeCore &node, harness::ArcId arcId) {
    if (std::find(node.adjArcs.begin(), node.adjArcs.end(), arcId) == node.adjArcs.end()) {
        node.adjArcs.push_back(arcId);
    }
}

void removeAdjArc(harness::SPQRNodeCore &node, harness::ArcId arcId) {
    node.adjArcs.erase(std::remove(node.adjArcs.begin(), node.adjArcs.end(), arcId),
                       node.adjArcs.end());
}

std::vector<harness::ArcId> sortedAdjArcs(const std::vector<harness::ArcId> &adjArcs) {
    std::vector<harness::ArcId> sorted = adjArcs;
    std::sort(sorted.begin(), sorted.end());
    return sorted;
}

std::string formatAdjArcs(const std::vector<harness::ArcId> &adjArcs) {
    std::ostringstream oss;
    oss << '[';
    for (size_t i = 0; i < adjArcs.size(); ++i) {
        if (i) oss << ',';
        oss << adjArcs[i];
    }
    oss << ']';
    return oss.str();
}

void eraseRealOwnershipForNode(harness::ReducedSPQRCore &core, harness::NodeId nodeId) {
    for (auto it = core.ownerNodeOfRealEdge.begin(); it != core.ownerNodeOfRealEdge.end(); ) {
        if (it->second != nodeId) {
            ++it;
            continue;
        }
        core.ownerSlotOfRealEdge.erase(it->first);
        it = core.ownerNodeOfRealEdge.erase(it);
    }
}

void cleanupSequenceWeakRepairOldNode(harness::ReducedSPQRCore &core,
                                      harness::NodeId oldNode) {
    if (!validCoreNode(core, oldNode)) return;
    auto &node = core.nodes[oldNode];
    if (!node.alive) return;

    for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
        auto &slot = node.slots[slotId];
        if (!slot.alive || !slot.isVirtual) continue;
        if (!validCoreArc(core, slot.arcId) || !core.arcs[slot.arcId].alive) {
            slot.alive = false;
            slot.arcId = -1;
            continue;
        }
        const auto &arc = core.arcs[slot.arcId];
        const bool incident =
            (arc.a == oldNode && arc.slotInA == slotId) ||
            (arc.b == oldNode && arc.slotInB == slotId);
        if (!incident) {
            slot.alive = false;
            slot.arcId = -1;
        }
    }

    node.adjArcs.erase(std::remove_if(node.adjArcs.begin(),
                                      node.adjArcs.end(),
                                      [&](harness::ArcId arcId) {
                                          if (!validCoreArc(core, arcId)) return true;
                                          const auto &arc = core.arcs[arcId];
                                          return !arc.alive ||
                                                 (arc.a != oldNode && arc.b != oldNode);
                                      }),
                       node.adjArcs.end());
    node.realEdgesHere.clear();
    for (const auto &slot : node.slots) {
        if (!slot.alive || slot.isVirtual || slot.realEdge < 0) continue;
        node.realEdgesHere.push_back(slot.realEdge);
    }
    node.localAgg = {};
    node.subAgg = {};

    bool anyAliveSlot = false;
    for (const auto &slot : node.slots) {
        if (slot.alive) {
            anyAliveSlot = true;
            break;
        }
    }
    if (!anyAliveSlot && node.adjArcs.empty() && node.realEdgesHere.empty()) {
        eraseRealOwnershipForNode(core, oldNode);
        node.alive = false;
        node.localAgg = {};
        node.subAgg = {};
    }
}

void killArc(harness::ReducedSPQRCore &core, harness::ArcId arcId) {
    if (!validCoreArc(core, arcId)) return;
    auto &arc = core.arcs[arcId];
    if (!arc.alive) return;

    auto clearEndpoint = [&](harness::NodeId nodeId, int slotId) {
        if (!validCoreNode(core, nodeId)) return;
        auto &node = core.nodes[nodeId];
        removeAdjArc(node, arcId);
        if (!validCoreSlot(node, slotId)) return;
        auto &slot = node.slots[slotId];
        slot.alive = false;
        if (slot.isVirtual) slot.arcId = -1;
    };

    clearEndpoint(arc.a, arc.slotInA);
    clearEndpoint(arc.b, arc.slotInB);
    arc.alive = false;
}

bool clearNodeKeepIdForGraft(harness::ReducedSPQRCore &core,
                             harness::NodeId nodeId,
                             harness::SPQRType type,
                             std::string *why) {
    if (!validCoreNode(core, nodeId) || !core.nodes[nodeId].alive) {
        return failGraft(why, "graft: invalid old node");
    }

    auto &node = core.nodes[nodeId];
    std::vector<harness::ArcId> arcsToKill;
    for (const auto &slot : node.slots) {
        if (!slot.alive || !slot.isVirtual || !validCoreArc(core, slot.arcId)) continue;
        const auto &arc = core.arcs[slot.arcId];
        const harness::NodeId other =
            arc.a == nodeId ? arc.b :
            arc.b == nodeId ? arc.a :
            -1;
        if (!validCoreNode(core, other)) continue;
        if (!core.nodes[other].realEdgesHere.empty()) {
            arcsToKill.push_back(slot.arcId);
        }
    }

    for (harness::ArcId arcId : arcsToKill) {
        killArc(core, arcId);
    }

    eraseRealOwnershipForNode(core, nodeId);
    node.type = type;
    node.slots.clear();
    node.adjArcs.clear();
    node.realEdgesHere.clear();
    node.localAgg = {};
    node.subAgg = {};

    for (const auto &kv : core.ownerNodeOfRealEdge) {
        if (kv.second == nodeId) {
            return failGraft(why, "graft: failed to clear keep node ownership");
        }
    }
    return true;
}

void removeTouchedOccurrencesForNodes(harness::ReducedSPQRCore &core,
                                      const std::unordered_set<harness::VertexId> &touched,
                                      const std::unordered_set<harness::NodeId> &nodes) {
    for (harness::VertexId v : touched) {
        auto it = core.occ.find(v);
        if (it == core.occ.end()) continue;
        auto &refs = it->second;
        refs.erase(std::remove_if(refs.begin(), refs.end(),
                                  [&](const harness::OccRef &ref) {
                                      return nodes.count(ref.node) != 0;
                                  }),
                   refs.end());
        if (refs.empty()) {
            core.occ.erase(it);
        }
    }
}

void rebuildRealEdgesHereNode(harness::SPQRNodeCore &node) {
    node.realEdgesHere.clear();
    for (const auto &slot : node.slots) {
        if (!slot.alive || slot.isVirtual || slot.realEdge < 0) continue;
        node.realEdgesHere.push_back(slot.realEdge);
    }
}

harness::Agg recomputeLocalAgg(const harness::SPQRNodeCore &node,
                               const std::unordered_set<harness::VertexId> &touched) {
    harness::Agg agg;
    std::unordered_set<harness::VertexId> liveVertices;
    std::unordered_set<harness::VertexId> watchedVertices;

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

bool connectVirtualArc(harness::ReducedSPQRCore &core,
                       harness::NodeId a,
                       int slotA,
                       harness::NodeId b,
                       int slotB,
                       harness::VertexId poleA,
                       harness::VertexId poleB,
                       std::string *why) {
    if (!validCoreNode(core, a) || !validCoreNode(core, b)) {
        return failGraft(why, "graft: mini internal arc endpoint not materialized");
    }
    auto &nodeA = core.nodes[a];
    auto &nodeB = core.nodes[b];
    if (!nodeA.alive || !nodeB.alive ||
        !validCoreSlot(nodeA, slotA) || !validCoreSlot(nodeB, slotB)) {
        return failGraft(why, "graft: mini internal arc endpoint not materialized");
    }

    auto &actualSlotA = nodeA.slots[slotA];
    auto &actualSlotB = nodeB.slots[slotB];
    if (!actualSlotA.alive || !actualSlotB.alive ||
        !actualSlotA.isVirtual || !actualSlotB.isVirtual ||
        actualSlotA.arcId >= 0 || actualSlotB.arcId >= 0) {
        return failGraft(why, "graft: connectVirtualArc produced inconsistent arc");
    }

    const harness::ArcId arcId = static_cast<harness::ArcId>(core.arcs.size());
    core.arcs.push_back({true, a, b, slotA, slotB, poleA, poleB});
    actualSlotA.arcId = arcId;
    actualSlotB.arcId = arcId;
    addAdjArc(nodeA, arcId);
    addAdjArc(nodeB, arcId);

    const auto &arc = core.arcs[arcId];
    if (!arc.alive ||
        !validCoreSlot(nodeA, arc.slotInA) ||
        !validCoreSlot(nodeB, arc.slotInB) ||
        !nodeA.slots[arc.slotInA].isVirtual ||
        !nodeB.slots[arc.slotInB].isVirtual ||
        nodeA.slots[arc.slotInA].arcId != arcId ||
        nodeB.slots[arc.slotInB].arcId != arcId) {
        return failGraft(why, "graft: connectVirtualArc produced inconsistent arc");
    }
    return true;
}

bool rewireArcEndpoint(harness::ReducedSPQRCore &core,
                       harness::ArcId arcId,
                       harness::NodeId oldNode,
                       harness::NodeId newNode,
                       int newSlot,
                       std::string *why) {
    if (!validCoreArc(core, arcId) || !validCoreNode(core, newNode)) {
        return failGraft(why, "graft: rewireArcEndpoint returned false");
    }

    auto &arc = core.arcs[arcId];
    if (!arc.alive) {
        return failGraft(why, "graft: rewireArcEndpoint returned false");
    }

    auto &node = core.nodes[newNode];
    if (!node.alive || !validCoreSlot(node, newSlot)) {
        return failGraft(why, "graft: PROXY actual slot missing");
    }

    auto &slot = node.slots[newSlot];
    if (!slot.alive || !slot.isVirtual) {
        return failGraft(why, "graft: PROXY actual slot missing");
    }

    const bool rewiredA = arc.a == oldNode;
    const bool rewiredB = arc.b == oldNode;
    if (rewiredA == rewiredB) {
        return failGraft(why, "graft: rewireArcEndpoint returned false");
    }

    if (rewiredA) {
        arc.a = newNode;
        arc.slotInA = newSlot;
    } else {
        arc.b = newNode;
        arc.slotInB = newSlot;
    }

    slot.arcId = arcId;
    addAdjArc(node, arcId);

    auto endpointOkay = [&](harness::NodeId nodeId, int slotId) {
        if (!validCoreNode(core, nodeId)) return false;
        const auto &endpointNode = core.nodes[nodeId];
        if (!endpointNode.alive || !validCoreSlot(endpointNode, slotId)) return false;
        const auto &endpointSlot = endpointNode.slots[slotId];
        return endpointSlot.alive && endpointSlot.isVirtual && endpointSlot.arcId == arcId;
    };

    if (!endpointOkay(arc.a, arc.slotInA) || !endpointOkay(arc.b, arc.slotInB)) {
        return failGraft(why, "graft: rewireArcEndpoint returned false");
    }
    return true;
}

bool rebuildTouchedOccurrences(harness::ReducedSPQRCore &core,
                               const std::unordered_set<harness::VertexId> &touched,
                               const std::vector<harness::NodeId> &actualNodes,
                               std::string *why) {
    std::unordered_set<harness::NodeId> actualNodeSet(actualNodes.begin(), actualNodes.end());
    removeTouchedOccurrencesForNodes(core, touched, actualNodeSet);

    std::unordered_set<harness::VertexId> touchedWithActualOccurrence;
    for (harness::NodeId nodeId : actualNodes) {
        if (!validCoreNode(core, nodeId) || !core.nodes[nodeId].alive) continue;
        const auto &node = core.nodes[nodeId];
        for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
            const auto &slot = node.slots[slotId];
            if (!slot.alive) continue;
            if (touched.count(slot.poleA)) {
                addOccurrence(core, nodeId, slotId, slot.poleA);
                touchedWithActualOccurrence.insert(slot.poleA);
            }
            if (slot.poleB != slot.poleA && touched.count(slot.poleB)) {
                addOccurrence(core, nodeId, slotId, slot.poleB);
                touchedWithActualOccurrence.insert(slot.poleB);
            }
        }
    }

    for (harness::VertexId v : touchedWithActualOccurrence) {
        auto it = core.occ.find(v);
        if (it == core.occ.end() || it->second.empty()) {
            return failGraft(why, "graft: touched occurrence rebuild failed");
        }
    }
    return true;
}

bool occRefLess(const harness::OccRef &lhs, const harness::OccRef &rhs) {
    if (lhs.node != rhs.node) return lhs.node < rhs.node;
    return lhs.slot < rhs.slot;
}

std::vector<harness::OccRef> sortedOccRefs(std::vector<harness::OccRef> refs) {
    std::sort(refs.begin(), refs.end(), occRefLess);
    return refs;
}

bool sameOccRefs(const std::vector<harness::OccRef> &lhs,
                 const std::vector<harness::OccRef> &rhs) {
    if (lhs.size() != rhs.size()) return false;
    for (size_t i = 0; i < lhs.size(); ++i) {
        if (lhs[i].node != rhs[i].node || lhs[i].slot != rhs[i].slot) return false;
    }
    return true;
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

bool rewriteRFallback(harness::ReducedSPQRCore &core,
                      harness::NodeId rNode,
                      harness::VertexId x,
                      std::string &why) {
    return harness::rewriteProjectRFallback(core, rNode, x, why);
}

bool normalizeTouchedRegion(harness::ReducedSPQRCore &core,
                            std::string &why) {
    return harness::normalizeProjectTouchedRegion(core, why);
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
    for (auto &node : env.core.nodes) {
        node.alive = false;
    }
    env.stubOfInputEdge.assign(H.edges.size(), -1);
    env.arcOfInputEdge.assign(H.edges.size(), -1);

    auto &oldR = env.core.nodes[env.oldR];
    oldR.alive = true;
    oldR.type = harness::SPQRType::R_NODE;

    std::unordered_set<harness::VertexId> touched(H.touchedVertices.begin(),
                                                  H.touchedVertices.end());

    for (int inputId = 0; inputId < static_cast<int>(H.edges.size()); ++inputId) {
        const auto &edge = H.edges[inputId];
        auto &storedEdge = env.H.edges[inputId];
        if (!validCompactVertex(H, edge.a) || !validCompactVertex(H, edge.b)) {
            why = "buildDummyActualCoreEnvelope: compact edge endpoint out of range";
            return false;
        }

        const harness::VertexId poleA = H.origOfCv[edge.a];
        const harness::VertexId poleB = H.origOfCv[edge.b];
        if (edge.kind == harness::CompactEdgeKind::REAL) {
            const int oldRealSlotId = static_cast<int>(oldR.slots.size());
            oldR.slots.push_back({true, poleA, poleB, false, edge.realEdge, -1});
            env.core.ownerNodeOfRealEdge[edge.realEdge] = env.oldR;
            env.core.ownerSlotOfRealEdge[edge.realEdge] = oldRealSlotId;
            storedEdge.oldArc = -1;
            storedEdge.outsideNode = -1;
            storedEdge.oldSlotInU = -1;
            continue;
        }

        const harness::NodeId stubId = 1 + inputId;
        auto &stub = env.core.nodes[stubId];
        stub.alive = true;
        stub.type = harness::SPQRType::R_NODE;
        env.stubNodes.insert(stubId);
        env.stubOfInputEdge[inputId] = stubId;

        const int oldSlotId = static_cast<int>(oldR.slots.size());
        oldR.slots.push_back({true, poleA, poleB, true, -1, -1});

        const int stubVirtualSlotId = static_cast<int>(stub.slots.size());
        stub.slots.push_back({true, poleA, poleB, true, -1, -1});
        stub.localAgg = edge.sideAgg;
        if (stub.localAgg.repVertex < 0) stub.localAgg.repVertex = poleA;
        stub.subAgg = stub.localAgg;

        const harness::ArcId arcId = static_cast<harness::ArcId>(env.core.arcs.size());
        oldR.slots[oldSlotId].arcId = arcId;
        stub.slots[stubVirtualSlotId].arcId = arcId;
        addAdjArc(oldR, arcId);
        addAdjArc(stub, arcId);
        env.core.arcs.push_back({true, env.oldR, stubId, oldSlotId, stubVirtualSlotId, poleA, poleB});
        env.arcOfInputEdge[inputId] = arcId;
        storedEdge.oldArc = arcId;
        storedEdge.outsideNode = stubId;
        storedEdge.oldSlotInU = oldSlotId;
    }

    rebuildRealEdgesHereNode(oldR);
    oldR.localAgg = recomputeLocalAgg(oldR, touched);
    oldR.subAgg = oldR.localAgg;
    env.core.totalAgg = oldR.localAgg;

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

    std::unordered_map<harness::EdgeId, harness::OccRef> seenRealSlot;
    for (harness::NodeId nodeId = 0; nodeId < static_cast<harness::NodeId>(env.core.nodes.size()); ++nodeId) {
        const auto &node = env.core.nodes[nodeId];
        if (!node.alive) continue;
        for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
            const auto &slot = node.slots[slotId];
            if (!slot.alive || slot.isVirtual) continue;
            const auto [_, inserted] = seenRealSlot.emplace(slot.realEdge,
                                                            harness::OccRef{nodeId, slotId});
            if (!inserted) {
                why = "dummy envelope: duplicate REAL slot for edge " +
                      std::to_string(slot.realEdge);
                return false;
            }
        }
    }

    for (harness::NodeId stubId : env.stubNodes) {
        if (!validCoreNode(env.core, stubId) || !env.core.nodes[stubId].alive) continue;
        const auto &stub = env.core.nodes[stubId];
        for (const auto &slot : stub.slots) {
            if (!slot.alive || slot.isVirtual) continue;
            why = "dummy envelope: stub node " + std::to_string(stubId) +
                  " owns REAL slot edge " + std::to_string(slot.realEdge);
            return false;
        }
    }

    for (int inputId = 0; inputId < static_cast<int>(env.H.edges.size()); ++inputId) {
        const auto &edge = env.H.edges[inputId];
        if (edge.kind == harness::CompactEdgeKind::REAL) {
            const auto ownerNodeIt = env.core.ownerNodeOfRealEdge.find(edge.realEdge);
            const auto ownerSlotIt = env.core.ownerSlotOfRealEdge.find(edge.realEdge);
            if (ownerNodeIt == env.core.ownerNodeOfRealEdge.end() ||
                ownerSlotIt == env.core.ownerSlotOfRealEdge.end() ||
                ownerNodeIt->second != env.oldR ||
                !validCoreSlot(env.core.nodes[env.oldR], ownerSlotIt->second)) {
                why = "dummy envelope: REAL input edge " +
                      std::to_string(edge.realEdge) + " has no owner on oldR";
                return false;
            }
            const auto &ownerSlot = env.core.nodes[env.oldR].slots[ownerSlotIt->second];
            if (!ownerSlot.alive || ownerSlot.isVirtual || ownerSlot.realEdge != edge.realEdge) {
                why = "dummy envelope: REAL input edge " +
                      std::to_string(edge.realEdge) + " has no owner on oldR";
                return false;
            }
            continue;
        }

        if (!validCoreArc(env.core, edge.oldArc) ||
            !validCoreNode(env.core, edge.outsideNode) ||
            edge.oldSlotInU < 0 ||
            !validCoreSlot(env.core.nodes[env.oldR], edge.oldSlotInU)) {
            why = "dummy envelope: PROXY input edge " +
                  std::to_string(edge.id) + " missing oldArc/outside metadata";
            return false;
        }

        const auto &oldSlot = env.core.nodes[env.oldR].slots[edge.oldSlotInU];
        const auto &oldArc = env.core.arcs[edge.oldArc];
        const harness::NodeId otherNode =
            oldArc.a == env.oldR ? oldArc.b :
            oldArc.b == env.oldR ? oldArc.a :
            -1;
        if (!oldSlot.alive || !oldSlot.isVirtual || oldSlot.arcId != edge.oldArc ||
            !oldArc.alive || otherNode != edge.outsideNode) {
            why = "dummy envelope: PROXY input edge " +
                  std::to_string(edge.id) + " missing oldArc/outside metadata";
            return false;
        }
    }

    return true;
}

bool chooseKeepMiniNode(const harness::StaticMiniCore &mini,
                        int &keep,
                        std::string &why) {
    harness::ProjectMiniCore pm;
    if (!harness::importStaticMiniCore(mini, pm, why)) return false;
    return harness::chooseProjectKeepMiniNode(pm, keep, why);
}

bool graftMiniCoreIntoPlace(harness::ReducedSPQRCore &core,
                            harness::NodeId oldNode,
                            const harness::CompactGraph &compact,
                            const harness::StaticMiniCore &mini,
                            int keep,
                            std::queue<harness::NodeId> &q,
                            harness::GraftTrace *trace,
                            std::string &why) {
    std::vector<harness::ResolvedProxyEndpoint> resolvedProxySnapshot;
    if (trace) {
        resolvedProxySnapshot = trace->resolvedProxyEndpoints;
        *trace = {};
        trace->resolvedProxyEndpoints = resolvedProxySnapshot;
    }
    if (!validCoreNode(core, oldNode) || !core.nodes[oldNode].alive) {
        why = "graft: invalid old node";
        return false;
    }
    if (keep < 0 || keep >= static_cast<int>(mini.nodes.size()) ||
        !mini.nodes[keep].alive) {
        why = "graft: invalid keep mini node";
        return false;
    }

    std::vector<harness::NodeId> actualOfMini(mini.nodes.size(), -1);
    std::vector<harness::NodeId> actualNodes;
    actualNodes.reserve(mini.nodes.size());

    for (int miniNodeId = 0; miniNodeId < static_cast<int>(mini.nodes.size()); ++miniNodeId) {
        const auto &miniNode = mini.nodes[miniNodeId];
        if (!miniNode.alive) continue;

        harness::NodeId actualNode = oldNode;
        if (miniNodeId != keep) {
            actualNode = static_cast<harness::NodeId>(core.nodes.size());
            harness::SPQRNodeCore newNode;
            newNode.alive = true;
            newNode.type = miniNode.type;
            core.nodes.push_back(std::move(newNode));
        }
        actualOfMini[miniNodeId] = actualNode;
        actualNodes.push_back(actualNode);
    }

    for (int miniNodeId = 0; miniNodeId < static_cast<int>(mini.nodes.size()); ++miniNodeId) {
        if (!mini.nodes[miniNodeId].alive) continue;
        if (actualOfMini[miniNodeId] < 0) {
            why = "graft: missing actual node for alive mini node";
            return false;
        }
    }

    std::unordered_set<harness::VertexId> touched(compact.touchedVertices.begin(),
                                                  compact.touchedVertices.end());
    for (const auto &miniNode : mini.nodes) {
        if (!miniNode.alive) continue;
        for (const auto &miniSlot : miniNode.slots) {
            if (!miniSlot.alive) continue;
            touched.insert(miniSlot.poleA);
            touched.insert(miniSlot.poleB);
        }
    }
    const std::unordered_set<harness::NodeId> oldNodeOnly = {oldNode};
    removeTouchedOccurrencesForNodes(core, touched, oldNodeOnly);

    std::vector<harness::PreservedProxyArc> preservedProxyArcs;
    std::unordered_map<int, size_t> preservedProxyByInput;
    auto syncResolvedSnapshotToTrace = [&]() {
        if (trace) trace->resolvedProxyEndpoints = resolvedProxySnapshot;
    };
    auto syncPreservedProxyToTrace = [&]() {
        if (!trace) return;
        trace->preservedProxyArcs = preservedProxyArcs;
        trace->preservedProxyArcsCount = static_cast<int>(preservedProxyArcs.size());
    };
    auto noteResolvedLifecyclePhase = [&](harness::ResolvedProxyEndpoint &resolved,
                                          harness::ProxyArcLifecyclePhase phase,
                                          const std::string &phaseWhy) {
        harness::noteSequenceProxyArcLifecyclePhase(core,
                                                    oldNode,
                                                    &compact,
                                                    resolved,
                                                    phase,
                                                    phaseWhy,
                                                    trace);
    };
    auto noteSnapshotPhase = [&]() {
        if (resolvedProxySnapshot.empty()) return;
        for (auto &resolved : resolvedProxySnapshot) {
            noteResolvedLifecyclePhase(resolved,
                                       harness::ProxyArcLifecyclePhase::PAL_SNAPSHOT_OK,
                                       "proxy lifecycle: snapshot ok");
        }
        syncResolvedSnapshotToTrace();
    };
    auto probeResolvedPhaseForAll = [&](harness::ProxyArcLifecyclePhase alivePhase,
                                        harness::ProxyArcLifecyclePhase deadPhase,
                                        harness::ProxyArcLifecyclePhase notIncidentPhase,
                                        harness::ProxyArcLifecyclePhase slotInvalidPhase) {
        if (resolvedProxySnapshot.empty()) return;
        for (auto &resolved : resolvedProxySnapshot) {
            harness::ProxyArcLifecyclePhase phase = harness::ProxyArcLifecyclePhase::PAL_OTHER;
            std::string phaseWhy;
            harness::inspectResolvedProxyArcPhase(core,
                                                  resolved,
                                                  alivePhase,
                                                  deadPhase,
                                                  notIncidentPhase,
                                                  slotInvalidPhase,
                                                  phase,
                                                  phaseWhy);
            noteResolvedLifecyclePhase(resolved, phase, phaseWhy);
        }
        syncResolvedSnapshotToTrace();
    };
    auto noteSpecificResolvedPhase = [&](int inputEdgeId,
                                         harness::ProxyArcLifecyclePhase phase,
                                         const std::string &phaseWhy) {
        for (auto &resolved : resolvedProxySnapshot) {
            if (resolved.inputEdgeId != inputEdgeId) continue;
            noteResolvedLifecyclePhase(resolved, phase, phaseWhy);
            break;
        }
        syncResolvedSnapshotToTrace();
    };
    auto inferPreserveSubtype = [&](const std::string &msg) {
        if (msg.find("oldArc dead") != std::string::npos) {
            return harness::GraftRewireBailoutSubtype::GRB_OLDARC_DEAD;
        }
        if (msg.find("not incident") != std::string::npos) {
            return harness::GraftRewireBailoutSubtype::GRB_OLDARC_NOT_INCIDENT_TO_OLDNODE;
        }
        if (msg.find("outsideNode mismatch") != std::string::npos) {
            return harness::GraftRewireBailoutSubtype::GRB_OUTSIDENODE_MISMATCH;
        }
        if (msg.find("resolvedOldSlot invalid") != std::string::npos ||
            msg.find("resolved old slot invalid") != std::string::npos ||
            msg.find("duplicate preserved old slot") != std::string::npos) {
            return harness::GraftRewireBailoutSubtype::GRB_OLDSLOT_INVALID;
        }
        if (msg.find("not virtual") != std::string::npos) {
            return harness::GraftRewireBailoutSubtype::GRB_OLDSLOT_NOT_VIRTUAL;
        }
        if (msg.find("arcId mismatch") != std::string::npos ||
            msg.find("does not point to oldArc") != std::string::npos) {
            return harness::GraftRewireBailoutSubtype::GRB_OLDSLOT_ARCID_MISMATCH;
        }
        if (msg.find("duplicate oldArc") != std::string::npos) {
            return harness::GraftRewireBailoutSubtype::GRB_DUPLICATE_OLDARC;
        }
        return harness::GraftRewireBailoutSubtype::GRB_OTHER;
    };
    auto inferGraftOtherSubtype = [&](const std::string &msg) {
        if (msg.find("preserved snapshot empty") != std::string::npos) {
            return harness::GraftOtherSubtype::GOS_PRESERVED_SNAPSHOT_EMPTY;
        }
        if (msg.find("duplicate preserved old slot") != std::string::npos) {
            return harness::GraftOtherSubtype::GOS_PRESERVED_DUPLICATE_SLOT;
        }
        if (msg.find("resolvedOldSlot invalid") != std::string::npos ||
            msg.find("resolved old slot invalid") != std::string::npos ||
            msg.find("preserved old slot invalid") != std::string::npos) {
            return harness::GraftOtherSubtype::GOS_PRESERVED_SLOT_OUT_OF_RANGE;
        }
        if (msg.find("preserved slot dead") != std::string::npos) {
            return harness::GraftOtherSubtype::GOS_PRESERVED_SLOT_DEAD;
        }
        if (msg.find("preserved slot not virtual") != std::string::npos) {
            return harness::GraftOtherSubtype::GOS_PRESERVED_SLOT_NOT_VIRTUAL;
        }
        if (msg.find("preserved slot arcId mismatch") != std::string::npos ||
            msg.find("old slot does not point to oldArc") != std::string::npos) {
            return harness::GraftOtherSubtype::GOS_PRESERVED_SLOT_ARCID_MISMATCH;
        }
        if (msg.find("rehome same-node: newSlot invalid") != std::string::npos ||
            msg.find("same-node rehome new slot invalid") != std::string::npos) {
            return harness::GraftOtherSubtype::GOS_REHOME_NEWSLOT_INVALID;
        }
        if (msg.find("rehome same-node: oldArc dead") != std::string::npos) {
            return harness::GraftOtherSubtype::GOS_REHOME_OLDARC_DEAD;
        }
        if (msg.find("rehome same-node: oldNode not incident") != std::string::npos) {
            return harness::GraftOtherSubtype::GOS_REHOME_OLDNODE_NOT_INCIDENT;
        }
        if (msg.find("rehome same-node: newSlot not virtual") != std::string::npos) {
            return harness::GraftOtherSubtype::GOS_REHOME_NEWSLOT_NOT_VIRTUAL;
        }
        if (msg.find("same-node rehome lost oldArc incidence") != std::string::npos ||
            msg.find("same-node rehome slot inconsistent") != std::string::npos) {
            return harness::GraftOtherSubtype::GOS_REHOME_ARC_SLOT_UPDATE_FAIL;
        }
        if (msg.find("stale preserved old slot remained alive") != std::string::npos) {
            return harness::GraftOtherSubtype::GOS_POSTCHECK_STALE_PRESERVED_SLOT;
        }
        if (msg.find("preserved oldArc died during rewrite") != std::string::npos) {
            return harness::GraftOtherSubtype::GOS_POSTCHECK_PRESERVED_ARC_DEAD;
        }
        if (msg.find("outside mismatch") != std::string::npos) {
            return harness::GraftOtherSubtype::GOS_POSTCHECK_OUTSIDE_MISMATCH;
        }
        if (msg.find("adjacency") != std::string::npos ||
            msg.find("still incident to oldNode") != std::string::npos ||
            msg.find("missing from adjacency") != std::string::npos) {
            return harness::GraftOtherSubtype::GOS_POSTCHECK_ADJ_MISMATCH;
        }
        return harness::GraftOtherSubtype::GOS_OTHER;
    };
    auto stampGraftOtherTrace = [&](harness::GraftOtherSubtype subtype,
                                    int inputEdgeId,
                                    harness::ArcId oldArc,
                                    int oldSlot,
                                    int newSlot,
                                    const std::string &msg) {
        if (!trace) return;
        trace->graftOtherSubtype = subtype;
        trace->preservedProxyArcsCount = static_cast<int>(preservedProxyArcs.size());
        trace->failingPreservedInputEdge = inputEdgeId;
        trace->failingPreservedOldArc = oldArc;
        trace->failingPreservedOldSlot = oldSlot;
        trace->failingNewSlot = newSlot;
        trace->graftOtherWhy = msg;
        trace->preservedProxyArcs = preservedProxyArcs;
    };

    noteSnapshotPhase();
    if (!resolvedProxySnapshot.empty()) {
        std::string preserveWhy;
        if (!harness::collectPreservedProxyArcsBeforeClear(core,
                                                           oldNode,
                                                           resolvedProxySnapshot,
                                                           preservedProxyArcs,
                                                           preserveWhy)) {
            harness::noteSequenceClearPreserveFallback();
            if (trace) {
                trace->graftRewireSubtype = inferPreserveSubtype(preserveWhy);
                stampGraftOtherTrace(inferGraftOtherSubtype(preserveWhy),
                                     -1,
                                     -1,
                                     -1,
                                     -1,
                                     preserveWhy);
            }
            why = preserveWhy;
            return false;
        }
        harness::noteSequenceClearPreserveRequested(preservedProxyArcs.size());
        for (size_t i = 0; i < preservedProxyArcs.size(); ++i) {
            preservedProxyByInput[preservedProxyArcs[i].inputEdgeId] = i;
        }
        syncPreservedProxyToTrace();
        if (!harness::clearNodeKeepIdForGraftPreserveResolvedProxyArcs(core,
                                                                       oldNode,
                                                                       preservedProxyArcs,
                                                                       mini.nodes[keep].type,
                                                                       preserveWhy)) {
            harness::noteSequenceClearPreserveFallback();
            if (trace) {
                trace->graftRewireSubtype = inferPreserveSubtype(preserveWhy);
                stampGraftOtherTrace(inferGraftOtherSubtype(preserveWhy),
                                     -1,
                                     -1,
                                     -1,
                                     -1,
                                     preserveWhy);
            }
            why = preserveWhy;
            return false;
        }
        syncPreservedProxyToTrace();
    } else if (!clearNodeKeepIdForGraft(core, oldNode, mini.nodes[keep].type, &why)) {
        return false;
    }
    probeResolvedPhaseForAll(harness::ProxyArcLifecyclePhase::PAL_AFTER_CLEAR_KEEP_ALIVE,
                             harness::ProxyArcLifecyclePhase::PAL_AFTER_CLEAR_KEEP_DEAD,
                             harness::ProxyArcLifecyclePhase::PAL_AFTER_CLEAR_KEEP_NOT_INCIDENT,
                             harness::ProxyArcLifecyclePhase::PAL_AFTER_CLEAR_KEEP_SLOT_INVALID);

    std::vector<std::vector<int>> actualSlotOfMiniSlot(mini.nodes.size());
    for (int miniNodeId = 0; miniNodeId < static_cast<int>(mini.nodes.size()); ++miniNodeId) {
        actualSlotOfMiniSlot[miniNodeId].assign(mini.nodes[miniNodeId].slots.size(), -1);
    }

    for (int miniNodeId = 0; miniNodeId < static_cast<int>(mini.nodes.size()); ++miniNodeId) {
        const auto &miniNode = mini.nodes[miniNodeId];
        if (!miniNode.alive) continue;

        const harness::NodeId actualNodeId = actualOfMini[miniNodeId];
        auto &actualNode = core.nodes[actualNodeId];
        actualNode.alive = true;
        actualNode.type = miniNode.type;
        actualNode.realEdgesHere.clear();
        actualNode.localAgg = {};
        actualNode.subAgg = {};
        if (miniNodeId != keep) {
            actualNode.slots.clear();
            actualNode.adjArcs.clear();
        }

        for (int miniSlotId = 0; miniSlotId < static_cast<int>(miniNode.slots.size()); ++miniSlotId) {
            const auto &miniSlot = miniNode.slots[miniSlotId];
            if (!miniSlot.alive) continue;

            const int actualSlotId = static_cast<int>(actualNode.slots.size());
            const bool isVirtual = miniSlot.kind != harness::MiniSlotKind::REAL_INPUT;
            actualNode.slots.push_back({true,
                                        miniSlot.poleA,
                                        miniSlot.poleB,
                                        isVirtual,
                                        isVirtual ? -1 : miniSlot.realEdge,
                                        -1});
            actualSlotOfMiniSlot[miniNodeId][miniSlotId] = actualSlotId;

            if (miniSlot.kind == harness::MiniSlotKind::REAL_INPUT) {
                core.ownerNodeOfRealEdge[miniSlot.realEdge] = actualNodeId;
                core.ownerSlotOfRealEdge[miniSlot.realEdge] = actualSlotId;
            }
        }
    }

    for (int miniNodeId = 0; miniNodeId < static_cast<int>(mini.nodes.size()); ++miniNodeId) {
        const auto &miniNode = mini.nodes[miniNodeId];
        if (!miniNode.alive) continue;
        const harness::NodeId actualNodeId = actualOfMini[miniNodeId];
        for (int miniSlotId = 0; miniSlotId < static_cast<int>(miniNode.slots.size()); ++miniSlotId) {
            const auto &miniSlot = miniNode.slots[miniSlotId];
            if (!miniSlot.alive) continue;
            const int actualSlotId = actualSlotOfMiniSlot[miniNodeId][miniSlotId];
            if (actualSlotId < 0) {
                why = "graft: mini slot not materialized";
                return false;
            }
            if (miniSlot.kind == harness::MiniSlotKind::REAL_INPUT) {
                const auto nodeIt = core.ownerNodeOfRealEdge.find(miniSlot.realEdge);
                const auto slotIt = core.ownerSlotOfRealEdge.find(miniSlot.realEdge);
                if (nodeIt == core.ownerNodeOfRealEdge.end() ||
                    slotIt == core.ownerSlotOfRealEdge.end() ||
                    nodeIt->second != actualNodeId ||
                    slotIt->second != actualSlotId) {
                    why = "graft: REAL_INPUT ownership install mismatch";
                    return false;
                }
            }
        }
    }
    probeResolvedPhaseForAll(harness::ProxyArcLifecyclePhase::PAL_AFTER_MATERIALIZE_ALIVE,
                             harness::ProxyArcLifecyclePhase::PAL_AFTER_MATERIALIZE_DEAD,
                             harness::ProxyArcLifecyclePhase::PAL_AFTER_MATERIALIZE_NOT_INCIDENT,
                             harness::ProxyArcLifecyclePhase::PAL_AFTER_MATERIALIZE_SLOT_INVALID);

    for (const auto &miniArc : mini.arcs) {
        if (!miniArc.alive) continue;
        if (miniArc.a < 0 || miniArc.b < 0 ||
            miniArc.a >= static_cast<int>(actualOfMini.size()) ||
            miniArc.b >= static_cast<int>(actualOfMini.size())) {
            why = "graft: mini internal arc endpoint not materialized";
            return false;
        }
        if (miniArc.slotInA < 0 || miniArc.slotInB < 0 ||
            miniArc.slotInA >= static_cast<int>(actualSlotOfMiniSlot[miniArc.a].size()) ||
            miniArc.slotInB >= static_cast<int>(actualSlotOfMiniSlot[miniArc.b].size())) {
            why = "graft: mini internal arc endpoint not materialized";
            return false;
        }
        const harness::NodeId actualA = actualOfMini[miniArc.a];
        const harness::NodeId actualB = actualOfMini[miniArc.b];
        const int actualSlotA = actualSlotOfMiniSlot[miniArc.a][miniArc.slotInA];
        const int actualSlotB = actualSlotOfMiniSlot[miniArc.b][miniArc.slotInB];
        if (actualA < 0 || actualB < 0 || actualSlotA < 0 || actualSlotB < 0) {
            why = "graft: mini internal arc endpoint not materialized";
            return false;
        }
        if (!connectVirtualArc(core, actualA, actualSlotA, actualB, actualSlotB,
                               miniArc.poleA, miniArc.poleB, &why)) {
            return false;
        }
    }
    probeResolvedPhaseForAll(harness::ProxyArcLifecyclePhase::PAL_AFTER_INTERNAL_ARCS_ALIVE,
                             harness::ProxyArcLifecyclePhase::PAL_AFTER_INTERNAL_ARCS_DEAD,
                             harness::ProxyArcLifecyclePhase::PAL_AFTER_INTERNAL_ARCS_NOT_INCIDENT,
                             harness::ProxyArcLifecyclePhase::PAL_AFTER_INTERNAL_ARCS_SLOT_INVALID);

    const bool useResolvedProxySnapshot = !resolvedProxySnapshot.empty();
    std::unordered_map<int, harness::ResolvedProxyEndpoint> resolvedProxyByInput;
    for (const auto &resolved : resolvedProxySnapshot) {
        resolvedProxyByInput.emplace(resolved.inputEdgeId, resolved);
    }
    const std::unordered_set<harness::NodeId> actualNodeSet(actualNodes.begin(),
                                                            actualNodes.end());

    std::unordered_map<int, int> seenProxyInput;
    std::vector<harness::RewiredProxyEdge> rewiredProxyEdges;
    auto failGraftRewire = [&](harness::GraftRewireBailoutSubtype subtype,
                               int inputEdgeId,
                               harness::ArcId oldArc,
                               int ownerMini,
                               int ownerMiniSlot,
                               const std::string &msg) {
        if (trace) {
            trace->actualOfMini = actualOfMini;
            trace->actualNodes = actualNodes;
            trace->actualSlotOfMiniSlot = actualSlotOfMiniSlot;
            trace->rewiredProxyEdges = rewiredProxyEdges;
            trace->preservedProxyArcs = preservedProxyArcs;
            for (const auto &resolved : trace->resolvedProxyEndpoints) {
                if (!resolved.weakRepairEntered) continue;
                trace->weakRepairEntered = resolved.weakRepairEntered;
                trace->weakRepairGateSubtype = resolved.weakRepairGateSubtype;
                trace->weakRepairCandidateSubtype = resolved.weakRepairCandidateSubtype;
                trace->weakRepairCommitOutcome = resolved.weakRepairCommitOutcome;
                trace->weakRepairOriginalOldArc = resolved.originalOldArc;
                trace->weakRepairResolvedArc = resolved.resolvedArc;
                trace->weakRepairOriginalOutsideNode = resolved.originalOutsideNode;
                trace->weakRepairResolvedOutsideNode = resolved.resolvedOutsideNode;
                trace->weakRepairInputEdgeId = resolved.inputEdgeId;
                break;
            }
            trace->graftRewireSubtype = subtype;
            trace->failingInputEdge = inputEdgeId;
            trace->failingOldArc = oldArc;
            trace->failingOwnerMini = ownerMini;
            trace->failingOwnerMiniSlot = ownerMiniSlot;
        }
        why = msg;
        return false;
    };
    auto failPreserveDuringGraft = [&](int inputEdgeId,
                                       harness::ArcId oldArc,
                                       int ownerMini,
                                       int ownerMiniSlot,
                                       const std::string &msg) {
        harness::noteSequenceClearPreserveFallback();
        int failingOldSlot = -1;
        int failingNewSlot = -1;
        const auto preservedIndexIt = preservedProxyByInput.find(inputEdgeId);
        if (preservedIndexIt != preservedProxyByInput.end() &&
            preservedIndexIt->second < preservedProxyArcs.size()) {
            const auto &preserved = preservedProxyArcs[preservedIndexIt->second];
            failingOldSlot = preserved.resolvedOldSlot;
            failingNewSlot = preserved.newSlot;
        }
        if (ownerMiniSlot >= 0 &&
            ownerMini >= 0 &&
            ownerMini < static_cast<int>(actualSlotOfMiniSlot.size()) &&
            ownerMiniSlot < static_cast<int>(actualSlotOfMiniSlot[ownerMini].size())) {
            failingNewSlot = actualSlotOfMiniSlot[ownerMini][ownerMiniSlot];
        }
        stampGraftOtherTrace(inferGraftOtherSubtype(msg),
                             inputEdgeId,
                             oldArc,
                             failingOldSlot,
                             failingNewSlot,
                             msg);
        return failGraftRewire(inferPreserveSubtype(msg),
                               inputEdgeId,
                               oldArc,
                               ownerMini,
                               ownerMiniSlot,
                               msg);
    };
    auto buildAdjMismatchWhy = [&](harness::NodeId nodeId,
                                   const std::vector<harness::ArcId> &expectedAdj,
                                   const std::vector<harness::ArcId> &actualAdj) {
        if (trace) {
            trace->firstBadAdjNode = nodeId;
            trace->expectedAdj = expectedAdj;
            trace->actualAdj = actualAdj;
        }

        std::ostringstream oss;
        oss << "graft other: adjacency mismatch on node " << nodeId
            << ", expected=" << formatAdjArcs(expectedAdj)
            << ", actual=" << formatAdjArcs(actualAdj);
        return oss.str();
    };
    auto findFirstAdjMismatch = [&](const std::vector<harness::NodeId> &nodes,
                                    std::string &msg) {
        std::unordered_set<harness::NodeId> seenNodes;
        for (harness::NodeId nodeId : nodes) {
            if (!validCoreNode(core, nodeId) || !core.nodes[nodeId].alive) continue;
            if (!seenNodes.insert(nodeId).second) continue;

            const auto expectedAdj = harness::collectAuthoritativeLiveAdjArcs(core, nodeId);
            const auto actualAdj = sortedAdjArcs(core.nodes[nodeId].adjArcs);
            if (expectedAdj == actualAdj) continue;

            msg = buildAdjMismatchWhy(nodeId, expectedAdj, actualAdj);
            return true;
        }
        return false;
    };
    std::unordered_set<harness::ArcId> seenOldArcs;
    probeResolvedPhaseForAll(harness::ProxyArcLifecyclePhase::PAL_PRE_REWIRE_ALIVE,
                             harness::ProxyArcLifecyclePhase::PAL_PRE_REWIRE_DEAD,
                             harness::ProxyArcLifecyclePhase::PAL_PRE_REWIRE_NOT_INCIDENT,
                             harness::ProxyArcLifecyclePhase::PAL_PRE_REWIRE_SLOT_INVALID);
    for (const auto &edge : compact.edges) {
        if (edge.kind != harness::CompactEdgeKind::PROXY) continue;
        const int inputEdgeId = edge.id;

        if (inputEdgeId < 0 ||
            inputEdgeId >= static_cast<int>(mini.ownerOfInputEdge.size())) {
            return failGraftRewire(harness::GraftRewireBailoutSubtype::GRB_OWNER_MINI_MISSING,
                                   inputEdgeId,
                                   edge.oldArc,
                                   -1,
                                   -1,
                                   "graft: PROXY input has no owner mini slot");
        }
        const auto owner = mini.ownerOfInputEdge[inputEdgeId];
        if (owner.first < 0 || owner.first >= static_cast<int>(mini.nodes.size()) ||
            owner.second < 0 ||
            owner.second >= static_cast<int>(mini.nodes[owner.first].slots.size())) {
            return failGraftRewire(
                harness::GraftRewireBailoutSubtype::GRB_OWNER_MINI_SLOT_INVALID,
                inputEdgeId,
                edge.oldArc,
                owner.first,
                owner.second,
                "graft: PROXY input has no owner mini slot");
        }
        const auto &ownerMiniNode = mini.nodes[owner.first];
        const auto &ownerMiniSlot = ownerMiniNode.slots[owner.second];
        if (!ownerMiniNode.alive || !ownerMiniSlot.alive ||
            ownerMiniSlot.kind != harness::MiniSlotKind::PROXY_INPUT) {
            return failGraftRewire(harness::GraftRewireBailoutSubtype::GRB_OWNER_SLOT_NOT_PROXY,
                                   inputEdgeId,
                                   edge.oldArc,
                                   owner.first,
                                   owner.second,
                                   "graft: PROXY input has no owner mini slot");
        }

        const harness::NodeId actualNodeId = actualOfMini[owner.first];
        if (actualNodeId < 0 || !validCoreNode(core, actualNodeId) ||
            !core.nodes[actualNodeId].alive) {
            return failGraftRewire(
                harness::GraftRewireBailoutSubtype::GRB_ACTUAL_OF_MINI_MISSING,
                inputEdgeId,
                edge.oldArc,
                owner.first,
                owner.second,
                "graft: PROXY actual slot missing");
        }
        if (owner.first >= static_cast<int>(actualSlotOfMiniSlot.size()) ||
            owner.second >= static_cast<int>(actualSlotOfMiniSlot[owner.first].size())) {
            return failGraftRewire(harness::GraftRewireBailoutSubtype::GRB_ACTUAL_SLOT_MISSING,
                                   inputEdgeId,
                                   edge.oldArc,
                                   owner.first,
                                   owner.second,
                                   "graft: PROXY actual slot missing");
        }
        const int actualSlotId = actualSlotOfMiniSlot[owner.first][owner.second];
        if (actualSlotId < 0 || !validCoreSlot(core.nodes[actualNodeId], actualSlotId) ||
            !core.nodes[actualNodeId].slots[actualSlotId].alive ||
            !core.nodes[actualNodeId].slots[actualSlotId].isVirtual) {
            return failGraftRewire(harness::GraftRewireBailoutSubtype::GRB_ACTUAL_SLOT_MISSING,
                                   inputEdgeId,
                                   edge.oldArc,
                                   owner.first,
                                   owner.second,
                                   "graft: PROXY actual slot missing");
        }
        harness::ArcId oldArcId = edge.oldArc;
        harness::NodeId outsideNode = edge.outsideNode;
        int resolvedOldSlot = edge.oldSlotInU;
        bool usedWeakPolesOnlyRepair = false;
        harness::PreservedProxyArc *preservedProxy = nullptr;
        if (useResolvedProxySnapshot) {
            const auto resolvedIt = resolvedProxyByInput.find(inputEdgeId);
            if (resolvedIt == resolvedProxyByInput.end()) {
                return failGraftRewire(harness::GraftRewireBailoutSubtype::GRB_OTHER,
                                       inputEdgeId,
                                       edge.oldArc,
                                       owner.first,
                                       owner.second,
                                       "graft: missing resolved PROXY snapshot");
            }
            oldArcId = resolvedIt->second.resolvedArc;
            outsideNode = resolvedIt->second.outsideNode;
            resolvedOldSlot = resolvedIt->second.resolvedOldSlot;
            usedWeakPolesOnlyRepair = resolvedIt->second.repairUsedWeakPolesOnly;
            const auto preservedIt = preservedProxyByInput.find(inputEdgeId);
            if (preservedIt == preservedProxyByInput.end() ||
                preservedIt->second >= preservedProxyArcs.size()) {
                return failPreserveDuringGraft(inputEdgeId,
                                              oldArcId,
                                              owner.first,
                                              owner.second,
                                              "graft preserve: missing preserved proxy arc snapshot");
            }
            preservedProxy = &preservedProxyArcs[preservedIt->second];
        }
        const bool oldNodeRelayLikeBeforeRewire =
            validCoreNode(core, oldNode) &&
            core.nodes[oldNode].alive &&
            !nodeHasAliveRealSlot(core.nodes[oldNode]);

        auto applyResolvedRepair = [&](const harness::RepairedProxyArcInfo &repaired) {
            oldArcId = repaired.resolvedArc;
            outsideNode = repaired.outsideNode;
            resolvedOldSlot = repaired.resolvedOldSlot;
            usedWeakPolesOnlyRepair = repaired.repairUsedWeakPolesOnly;
            resolvedProxyByInput[inputEdgeId] = repaired;
            for (auto &stored : resolvedProxySnapshot) {
                if (stored.inputEdgeId != inputEdgeId) continue;
                stored = repaired;
                break;
            }
            if (trace) {
                bool updated = false;
                for (auto &stored : trace->resolvedProxyEndpoints) {
                    if (stored.inputEdgeId != inputEdgeId) continue;
                    stored = repaired;
                    updated = true;
                    break;
                }
                if (!updated) {
                    trace->resolvedProxyEndpoints.push_back(repaired);
                }
                if (repaired.weakRepairEntered) {
                    trace->weakRepairEntered = repaired.weakRepairEntered;
                    trace->weakRepairGateSubtype = repaired.weakRepairGateSubtype;
                    trace->weakRepairCandidateSubtype = repaired.weakRepairCandidateSubtype;
                    trace->weakRepairCommitOutcome = repaired.weakRepairCommitOutcome;
                    trace->weakRepairOriginalOldArc = repaired.originalOldArc;
                    trace->weakRepairResolvedArc = repaired.resolvedArc;
                    trace->weakRepairOriginalOutsideNode = repaired.originalOutsideNode;
                    trace->weakRepairResolvedOutsideNode = repaired.resolvedOutsideNode;
                    trace->weakRepairInputEdgeId = repaired.inputEdgeId;
                }
            }
        };

        auto tryRepairResolvedOldArc = [&]() -> bool {
            if (!useResolvedProxySnapshot) return false;
            if (!validCompactVertex(compact, edge.a) || !validCompactVertex(compact, edge.b)) {
                why = "graft: resolved PROXY snapshot has invalid compact poles";
                return false;
            }

            const auto expectedPoles =
                std::minmax(compact.origOfCv[edge.a], compact.origOfCv[edge.b]);
            harness::RepairedProxyArcInfo repaired = resolvedProxyByInput[inputEdgeId];
            int candidateCount = 0;
            for (harness::ArcId candidateArc = 0;
                 candidateArc < static_cast<harness::ArcId>(core.arcs.size());
                 ++candidateArc) {
                const auto &arc = core.arcs[candidateArc];
                if (!arc.alive) continue;
                const bool oldOnA = arc.a == oldNode;
                const bool oldOnB = arc.b == oldNode;
                if (oldOnA == oldOnB) continue;

                const harness::NodeId otherNode = oldOnA ? arc.b : arc.a;
                if (actualNodeSet.count(otherNode) != 0) continue;
                if (!validCoreNode(core, otherNode) || !core.nodes[otherNode].alive) continue;
                if (core.nodes[otherNode].type != harness::SPQRType::R_NODE) continue;
                if (std::minmax(arc.poleA, arc.poleB) != expectedPoles) continue;

                const int candidateSlot = oldOnA ? arc.slotInA : arc.slotInB;
                if (!validCoreSlot(core.nodes[oldNode], candidateSlot)) continue;
                const auto &slot = core.nodes[oldNode].slots[candidateSlot];
                if (!slot.alive || !slot.isVirtual || slot.arcId != candidateArc) continue;

                repaired.resolvedArc = candidateArc;
                repaired.resolvedOldSlot = candidateSlot;
                repaired.outsideNode = otherNode;
                repaired.resolvedOutsideNode = otherNode;
                repaired.repairOutcome =
                    harness::ProxyArcRepairOutcome::PAR_MATCH_BY_POLES_ONLY_UNIQUE;
                repaired.repairUsedWeakPolesOnly = true;
                ++candidateCount;
            }

            if (candidateCount != 1) {
                why = "graft: resolved PROXY snapshot became stale after clear/materialize";
                return false;
            }
            applyResolvedRepair(repaired);
            if (preservedProxy) {
                preservedProxy->oldArc = repaired.resolvedArc;
                preservedProxy->outsideNode = repaired.outsideNode;
                preservedProxy->resolvedOldSlot = repaired.resolvedOldSlot;
                preservedProxy->poleA = repaired.poleA;
                preservedProxy->poleB = repaired.poleB;
                syncPreservedProxyToTrace();
            }
            return true;
        };

        if (oldArcId < 0 || oldArcId >= static_cast<int>(core.arcs.size())) {
            if (!tryRepairResolvedOldArc()) {
                return failGraftRewire(
                    harness::GraftRewireBailoutSubtype::GRB_OLDARC_OUT_OF_RANGE,
                    inputEdgeId,
                    oldArcId,
                    owner.first,
                    owner.second,
                    why.empty() ? std::string("graft: invalid oldArc/outsideNode metadata")
                                : why);
            }
        }
        if (!validCoreNode(core, outsideNode) || !core.nodes[outsideNode].alive) {
            if (!tryRepairResolvedOldArc()) {
                return failGraftRewire(
                    harness::GraftRewireBailoutSubtype::GRB_OUTSIDENODE_MISMATCH,
                    inputEdgeId,
                    oldArcId,
                    owner.first,
                    owner.second,
                    why.empty() ? std::string("graft: invalid oldArc/outsideNode metadata")
                                : why);
            }
        }
        if (!useResolvedProxySnapshot &&
            (resolvedOldSlot < 0 ||
             !validCoreSlot(core.nodes[oldNode], resolvedOldSlot))) {
            return failGraftRewire(harness::GraftRewireBailoutSubtype::GRB_OLDSLOT_INVALID,
                                   inputEdgeId,
                                   oldArcId,
                                   owner.first,
                                   owner.second,
                                   "graft: invalid oldArc/outsideNode metadata");
        }

        if (!core.arcs[oldArcId].alive) {
            if (!tryRepairResolvedOldArc()) {
                return failGraftRewire(
                    harness::GraftRewireBailoutSubtype::GRB_OLDARC_DEAD,
                    inputEdgeId,
                    oldArcId,
                    owner.first,
                    owner.second,
                    why.empty() ? std::string("graft: invalid oldArc/outsideNode metadata")
                                : why);
            }
        }
        auto computeOtherNode = [&]() -> harness::NodeId {
            const auto &arc = core.arcs[oldArcId];
            return arc.a == oldNode ? arc.b :
                   arc.b == oldNode ? arc.a :
                   -1;
        };

        harness::NodeId otherNode = computeOtherNode();
        if (otherNode < 0) {
            if (!tryRepairResolvedOldArc()) {
                return failGraftRewire(
                    harness::GraftRewireBailoutSubtype::GRB_OLDARC_NOT_INCIDENT_TO_OLDNODE,
                    inputEdgeId,
                    oldArcId,
                    owner.first,
                    owner.second,
                    why.empty() ? std::string("graft: invalid oldArc/outsideNode metadata")
                                : why);
            }
            otherNode = computeOtherNode();
        }
        if (otherNode != outsideNode) {
            if (!tryRepairResolvedOldArc()) {
                return failGraftRewire(
                    harness::GraftRewireBailoutSubtype::GRB_OUTSIDENODE_MISMATCH,
                    inputEdgeId,
                    oldArcId,
                    owner.first,
                    owner.second,
                    why.empty() ? std::string("graft: invalid oldArc/outsideNode metadata")
                                : why);
            }
            otherNode = computeOtherNode();
        }
        if (otherNode < 0 || otherNode != outsideNode) {
            return failGraftRewire(
                otherNode < 0
                    ? harness::GraftRewireBailoutSubtype::GRB_OLDARC_NOT_INCIDENT_TO_OLDNODE
                    : harness::GraftRewireBailoutSubtype::GRB_OUTSIDENODE_MISMATCH,
                inputEdgeId,
                oldArcId,
                owner.first,
                owner.second,
                "graft: invalid oldArc/outsideNode metadata");
        }

        if (!seenOldArcs.insert(oldArcId).second) {
            return failGraftRewire(harness::GraftRewireBailoutSubtype::GRB_DUPLICATE_OLDARC,
                                   inputEdgeId,
                                   oldArcId,
                                   owner.first,
                                   owner.second,
                                   "graft: invalid oldArc/outsideNode metadata");
        }

        if (!useResolvedProxySnapshot) {
            const auto &oldSlot = core.nodes[oldNode].slots[resolvedOldSlot];
            if (!oldSlot.alive || !oldSlot.isVirtual) {
                return failGraftRewire(
                    harness::GraftRewireBailoutSubtype::GRB_OLDSLOT_NOT_VIRTUAL,
                    inputEdgeId,
                    oldArcId,
                    owner.first,
                    owner.second,
                    "graft: invalid oldArc/outsideNode metadata");
            }
            if (oldSlot.arcId != oldArcId) {
                return failGraftRewire(
                    harness::GraftRewireBailoutSubtype::GRB_OLDSLOT_ARCID_MISMATCH,
                    inputEdgeId,
                    oldArcId,
                    owner.first,
                    owner.second,
                    "graft: invalid oldArc/outsideNode metadata");
            }
        }

        if (preservedProxy && actualNodeId == oldNode) {
            if (trace) trace->sameNodeRehomeAttempted = true;
            if (!harness::rehomePreservedProxyArcOnSameNode(core,
                                                            *preservedProxy,
                                                            actualSlotId,
                                                            why)) {
                return failPreserveDuringGraft(inputEdgeId,
                                              oldArcId,
                                              owner.first,
                                              owner.second,
                                              why.empty() ? std::string("graft preserve: same-node rehome failed")
                                                          : why);
            }
            preservedProxy->sameNodeRehome = true;
            preservedProxy->crossNodeRewire = false;
            preservedProxy->newSlot = actualSlotId;
            preservedProxy->finalNode = actualNodeId;
            harness::noteSequenceClearPreserveSameNodeRehome();
            if (trace) trace->sameNodeRehomeSucceeded = true;
            syncPreservedProxyToTrace();
        } else {
            if (!rewireArcEndpoint(core, oldArcId, oldNode, actualNodeId, actualSlotId, &why)) {
                noteSpecificResolvedPhase(inputEdgeId,
                                          harness::ProxyArcLifecyclePhase::PAL_REWIRE_RET_FALSE,
                                          why.empty()
                                              ? std::string("proxy lifecycle: rewireArcEndpoint returned false")
                                              : why);
                return failGraftRewire(
                    harness::GraftRewireBailoutSubtype::GRB_REWIRE_RET_FALSE,
                    inputEdgeId,
                    oldArcId,
                    owner.first,
                    owner.second,
                    why.empty() ? std::string("graft: rewireArcEndpoint returned false")
                                : why);
            }
            if (preservedProxy) {
                if (!validCoreSlot(core.nodes[oldNode], preservedProxy->resolvedOldSlot)) {
                    return failPreserveDuringGraft(
                        inputEdgeId,
                        oldArcId,
                        owner.first,
                        owner.second,
                        "graft preserve: preserved old slot invalid after cross-node rewire");
                }
                auto &oldSlot = core.nodes[oldNode].slots[preservedProxy->resolvedOldSlot];
                oldSlot.alive = false;
                if (oldSlot.isVirtual) oldSlot.arcId = -1;
                removeAdjArc(core.nodes[oldNode], oldArcId);
                preservedProxy->crossNodeRewire = true;
                preservedProxy->sameNodeRehome = false;
                preservedProxy->newSlot = actualSlotId;
                preservedProxy->finalNode = actualNodeId;
                harness::noteSequenceClearPreserveCrossNodeRewire();
                syncPreservedProxyToTrace();
            }
        }
        if (useResolvedProxySnapshot &&
            usedWeakPolesOnlyRepair &&
            oldNode != actualNodeId &&
            oldNodeRelayLikeBeforeRewire) {
            cleanupSequenceWeakRepairOldNode(core, oldNode);
        }

        ++seenProxyInput[inputEdgeId];
        rewiredProxyEdges.push_back({inputEdgeId, oldArcId, actualNodeId, actualSlotId});
    }

    for (const auto &edge : compact.edges) {
        if (edge.kind != harness::CompactEdgeKind::PROXY) continue;
        if (seenProxyInput[edge.id] != 1) {
            why = "graft: some PROXY input was not rewired exactly once";
            return false;
        }
    }

    if (useResolvedProxySnapshot) {
        harness::GraftTrace repairTrace;
        const harness::GraftTrace *repairTraceSource = trace;
        if (trace) {
            trace->actualOfMini = actualOfMini;
            trace->actualNodes = actualNodes;
            trace->actualSlotOfMiniSlot = actualSlotOfMiniSlot;
            trace->rewiredProxyEdges = rewiredProxyEdges;
            trace->preservedProxyArcs = preservedProxyArcs;
        } else {
            repairTrace.actualNodes = actualNodes;
            repairTrace.resolvedProxyEndpoints = resolvedProxySnapshot;
            repairTrace.preservedProxyArcs = preservedProxyArcs;
            repairTraceSource = &repairTrace;
        }

        const auto affectedAdjNodes =
            harness::collectAffectedNodesForAdjRepair(*repairTraceSource,
                                                      oldNode,
                                                      preservedProxyArcs);
        if (trace) {
            trace->affectedAdjRepairNodes = affectedAdjNodes;
            if (validCoreNode(core, oldNode) && core.nodes[oldNode].alive) {
                trace->oldNodeAdjArcsBeforeRepair = core.nodes[oldNode].adjArcs;
            } else {
                trace->oldNodeAdjArcsBeforeRepair.clear();
            }
        }

        harness::rebuildAdjacencyForAffectedNodesAfterGraft(core, affectedAdjNodes);

        if (trace) {
            if (validCoreNode(core, oldNode) && core.nodes[oldNode].alive) {
                trace->oldNodeAdjArcsAfterRepair = core.nodes[oldNode].adjArcs;
            } else {
                trace->oldNodeAdjArcsAfterRepair.clear();
            }
        }

        std::string adjRepairWhy;
        if (findFirstAdjMismatch(affectedAdjNodes, adjRepairWhy)) {
            return failPreserveDuringGraft(-1, -1, -1, -1, adjRepairWhy);
        }
    }

    if (!preservedProxyArcs.empty()) {
        const auto &oldActualNode = core.nodes[oldNode];
        for (const auto &preserved : preservedProxyArcs) {
            if (!validCoreArc(core, preserved.oldArc) || !core.arcs[preserved.oldArc].alive) {
                return failPreserveDuringGraft(preserved.inputEdgeId,
                                              preserved.oldArc,
                                              -1,
                                              -1,
                                              "graft preserve: preserved oldArc died during rewrite");
            }
            if (!validCoreSlot(oldActualNode, preserved.resolvedOldSlot) ||
                oldActualNode.slots[preserved.resolvedOldSlot].alive) {
                return failPreserveDuringGraft(
                    preserved.inputEdgeId,
                    preserved.oldArc,
                    -1,
                    -1,
                    "graft preserve: stale preserved old slot remained alive");
            }

            const auto &arc = core.arcs[preserved.oldArc];
            if (preserved.crossNodeRewire) {
                if (arc.a == oldNode || arc.b == oldNode) {
                    return failPreserveDuringGraft(
                        preserved.inputEdgeId,
                        preserved.oldArc,
                        -1,
                        -1,
                        "graft preserve: preserved oldArc still incident to oldNode after cross-node rewire");
                }
                if (std::find(oldActualNode.adjArcs.begin(),
                              oldActualNode.adjArcs.end(),
                              preserved.oldArc) != oldActualNode.adjArcs.end()) {
                    return failPreserveDuringGraft(
                        preserved.inputEdgeId,
                        preserved.oldArc,
                        -1,
                        -1,
                        "graft preserve: stale preserved oldArc remained on oldNode adjacency");
                }
            }

            if (preserved.sameNodeRehome) {
                const bool incident =
                    (arc.a == oldNode && arc.slotInA == preserved.newSlot) ||
                    (arc.b == oldNode && arc.slotInB == preserved.newSlot);
                if (!incident) {
                    return failPreserveDuringGraft(
                        preserved.inputEdgeId,
                        preserved.oldArc,
                        -1,
                        -1,
                        "graft preserve: same-node rehome lost oldArc incidence");
                }
                if (!validCoreSlot(oldActualNode, preserved.newSlot)) {
                    return failPreserveDuringGraft(
                        preserved.inputEdgeId,
                        preserved.oldArc,
                        -1,
                        -1,
                        "graft preserve: same-node rehome new slot invalid");
                }
                const auto &newSlot = oldActualNode.slots[preserved.newSlot];
                if (!newSlot.alive || !newSlot.isVirtual || newSlot.arcId != preserved.oldArc) {
                    return failPreserveDuringGraft(
                        preserved.inputEdgeId,
                        preserved.oldArc,
                        -1,
                        -1,
                        "graft preserve: same-node rehome slot inconsistent");
                }
            }
        }
        syncPreservedProxyToTrace();
    }

    if (!rebuildTouchedOccurrences(core, touched, actualNodes, &why)) {
        return false;
    }

    for (harness::NodeId actualNodeId : actualNodes) {
        if (!validCoreNode(core, actualNodeId) || !core.nodes[actualNodeId].alive) {
            why = "graft: actual node dead after materialization";
            return false;
        }
        auto &actualNode = core.nodes[actualNodeId];
        rebuildRealEdgesHereNode(actualNode);
        actualNode.localAgg = recomputeLocalAgg(actualNode, touched);
        actualNode.subAgg = actualNode.localAgg;
        q.push(actualNodeId);
    }

    if (trace) {
        trace->actualOfMini = std::move(actualOfMini);
        trace->actualNodes = std::move(actualNodes);
        trace->actualSlotOfMiniSlot = std::move(actualSlotOfMiniSlot);
        trace->rewiredProxyEdges = std::move(rewiredProxyEdges);
        trace->preservedProxyArcs = std::move(preservedProxyArcs);
        for (const auto &resolved : trace->resolvedProxyEndpoints) {
            if (!resolved.weakRepairEntered) continue;
            trace->weakRepairEntered = resolved.weakRepairEntered;
            trace->weakRepairGateSubtype = resolved.weakRepairGateSubtype;
            trace->weakRepairCandidateSubtype = resolved.weakRepairCandidateSubtype;
            trace->weakRepairCommitOutcome = resolved.weakRepairCommitOutcome;
            trace->weakRepairOriginalOldArc = resolved.originalOldArc;
            trace->weakRepairResolvedArc = resolved.resolvedArc;
            trace->weakRepairOriginalOutsideNode = resolved.originalOutsideNode;
            trace->weakRepairResolvedOutsideNode = resolved.resolvedOutsideNode;
            trace->weakRepairInputEdgeId = resolved.inputEdgeId;
            break;
        }
        trace->graftRewireSubtype = harness::GraftRewireBailoutSubtype::GRB_OTHER;
        trace->failingInputEdge = -1;
        trace->failingOldArc = -1;
        trace->failingOwnerMini = -1;
        trace->failingOwnerMiniSlot = -1;
    }

    return true;
}

bool rebuildActualMetadata(harness::ReducedSPQRCore &,
                           std::string &why) {
    why.clear();
    return true;
}

harness::ExplicitBlockGraph materializeWholeCoreExplicit(const harness::ReducedSPQRCore &core) {
    harness::ProjectExplicitBlockGraph project;
    harness::materializeProjectWholeCoreExplicit(core, project);

    harness::ExplicitBlockGraph out;
    harness::exportProjectExplicitBlockGraph(project, out);
    return out;
}

harness::ExplicitBlockGraph materializeCompactRealProjection(const harness::CompactGraph &H) {
    harness::ProjectExplicitBlockGraph project;
    harness::materializeProjectCompactRealProjection(H, project);

    harness::ExplicitBlockGraph out;
    harness::exportProjectExplicitBlockGraph(project, out);
    return out;
}

bool checkEquivalentExplicitGraphs(const harness::ExplicitBlockGraph &got,
                                   const harness::ExplicitBlockGraph &exp,
                                   std::string &why) {
    harness::ProjectExplicitBlockGraph projectGot;
    harness::ProjectExplicitBlockGraph projectExp;
    harness::importExplicitBlockGraph(got, projectGot);
    harness::importExplicitBlockGraph(exp, projectExp);
    return harness::checkProjectEquivalentExplicitGraphs(projectGot, projectExp, why);
}

bool checkDummyProxyRewire(const harness::DummyActualEnvelope &env,
                           const harness::StaticMiniCore &mini,
                           const harness::GraftTrace &trace,
                           std::string &why) {
    return harness::checkProjectDummyProxyRewire(env, mini, trace, why);
}

bool checkOwnershipConsistencyActual(const harness::ReducedSPQRCore &core,
                                     std::string &why) {
    struct RealOwner {
        harness::NodeId nodeId = -1;
        int slotId = -1;
    };

    std::unordered_map<harness::EdgeId, RealOwner> seenRealEdge;

    for (harness::NodeId nodeId = 0; nodeId < static_cast<harness::NodeId>(core.nodes.size()); ++nodeId) {
        const auto &node = core.nodes[nodeId];
        if (!node.alive) continue;

        std::vector<harness::EdgeId> scannedRealEdges;
        for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
            const auto &slot = node.slots[slotId];
            if (!slot.alive || slot.isVirtual) continue;
            if (slot.realEdge < 0) {
                why = "actual ownership: alive REAL slot missing real edge";
                return false;
            }

            scannedRealEdges.push_back(slot.realEdge);
            const auto [it, inserted] = seenRealEdge.emplace(slot.realEdge, RealOwner{nodeId, slotId});
            if (!inserted) {
                why = "actual ownership: duplicate alive REAL slot for same real edge";
                return false;
            }

            const auto ownerNodeIt = core.ownerNodeOfRealEdge.find(slot.realEdge);
            const auto ownerSlotIt = core.ownerSlotOfRealEdge.find(slot.realEdge);
            if (ownerNodeIt == core.ownerNodeOfRealEdge.end() ||
                ownerSlotIt == core.ownerSlotOfRealEdge.end() ||
                ownerNodeIt->second != nodeId ||
                ownerSlotIt->second != slotId) {
                why = "actual ownership: owner map mismatch for real edge";
                return false;
            }
        }

        auto expected = scannedRealEdges;
        auto actual = node.realEdgesHere;
        std::sort(expected.begin(), expected.end());
        std::sort(actual.begin(), actual.end());
        if (expected != actual) {
            why = "actual ownership: realEdgesHere mismatch";
            return false;
        }
    }

    if (core.ownerNodeOfRealEdge.size() != seenRealEdge.size() ||
        core.ownerSlotOfRealEdge.size() != seenRealEdge.size()) {
        why = "actual ownership: owner map size mismatch";
        return false;
    }

    for (const auto &[realEdge, ownerNode] : core.ownerNodeOfRealEdge) {
        const auto ownerSlotIt = core.ownerSlotOfRealEdge.find(realEdge);
        const auto seenIt = seenRealEdge.find(realEdge);
        if (ownerSlotIt == core.ownerSlotOfRealEdge.end() ||
            seenIt == seenRealEdge.end()) {
            why = "actual ownership: owner map references missing real edge";
            return false;
        }
        if (ownerNode != seenIt->second.nodeId ||
            ownerSlotIt->second != seenIt->second.slotId) {
            why = "actual ownership: owner map does not match scanned REAL slot";
            return false;
        }
        if (!validCoreNode(core, ownerNode)) {
            why = "actual ownership: owner node out of range";
            return false;
        }
        const auto &node = core.nodes[ownerNode];
        if (!node.alive || !validCoreSlot(node, ownerSlotIt->second)) {
            why = "actual ownership: owner slot out of range";
            return false;
        }
        const auto &slot = node.slots[ownerSlotIt->second];
        if (!slot.alive || slot.isVirtual || slot.realEdge != realEdge) {
            why = "actual ownership: owner slot content mismatch";
            return false;
        }
    }

    return true;
}

bool checkArcEndpointConsistencyActual(const harness::ReducedSPQRCore &core,
                                       std::string &why) {
    for (harness::ArcId arcId = 0; arcId < static_cast<harness::ArcId>(core.arcs.size()); ++arcId) {
        const auto &arc = core.arcs[arcId];
        if (!arc.alive) continue;

        if (!validCoreNode(core, arc.a) || !validCoreNode(core, arc.b) ||
            !core.nodes[arc.a].alive || !core.nodes[arc.b].alive) {
            why = "actual arc: live arc endpoint node is dead or invalid";
            return false;
        }

        const auto &nodeA = core.nodes[arc.a];
        const auto &nodeB = core.nodes[arc.b];
        if (!validCoreSlot(nodeA, arc.slotInA) || !validCoreSlot(nodeB, arc.slotInB)) {
            why = "actual arc: live arc endpoint slot index invalid";
            return false;
        }

        const auto &slotA = nodeA.slots[arc.slotInA];
        const auto &slotB = nodeB.slots[arc.slotInB];
        if (!slotA.alive || !slotB.alive || !slotA.isVirtual || !slotB.isVirtual) {
            why = "actual arc: endpoint slot must be alive virtual";
            return false;
        }
        if (slotA.arcId != arcId || slotB.arcId != arcId) {
            why = "actual arc: endpoint slot arc backlink mismatch";
            return false;
        }
        if (harness::canonPole(arc.poleA, arc.poleB) != harness::canonPole(slotA.poleA, slotA.poleB) ||
            harness::canonPole(arc.poleA, arc.poleB) != harness::canonPole(slotB.poleA, slotB.poleB)) {
            why = "actual arc: arc pole pair does not match endpoint slot poles";
            return false;
        }
    }

    return true;
}

bool checkOccurrenceConsistencyActual(const harness::ReducedSPQRCore &core,
                                      std::string &why) {
    std::unordered_map<harness::VertexId, std::vector<harness::OccRef>> expected;
    for (harness::NodeId nodeId = 0; nodeId < static_cast<harness::NodeId>(core.nodes.size()); ++nodeId) {
        const auto &node = core.nodes[nodeId];
        if (!node.alive) continue;
        for (int slotId = 0; slotId < static_cast<int>(node.slots.size()); ++slotId) {
            const auto &slot = node.slots[slotId];
            if (!slot.alive) continue;
            expected[slot.poleA].push_back({nodeId, slotId});
            if (slot.poleB != slot.poleA) {
                expected[slot.poleB].push_back({nodeId, slotId});
            }
        }
    }

    for (const auto &[vertex, refs] : core.occ) {
        (void)refs;
        for (const auto &ref : refs) {
            if (!validCoreNode(core, ref.node) || !core.nodes[ref.node].alive) {
                why = "actual occ: occurrence references dead or invalid node";
                return false;
            }
            const auto &node = core.nodes[ref.node];
            if (!validCoreSlot(node, ref.slot) || !node.slots[ref.slot].alive) {
                why = "actual occ: occurrence references dead or invalid slot";
                return false;
            }
            const auto &slot = node.slots[ref.slot];
            if (slot.poleA != vertex && slot.poleB != vertex) {
                why = "actual occ: occurrence pole mismatch";
                return false;
            }
        }
    }

    std::set<harness::VertexId> allVertices;
    for (const auto &[vertex, _] : expected) allVertices.insert(vertex);
    for (const auto &[vertex, _] : core.occ) allVertices.insert(vertex);

    for (harness::VertexId vertex : allVertices) {
        std::vector<harness::OccRef> expectedRefs;
        std::vector<harness::OccRef> actualRefs;

        if (const auto it = expected.find(vertex); it != expected.end()) {
            expectedRefs = sortedOccRefs(it->second);
        }
        if (const auto it = core.occ.find(vertex); it != core.occ.end()) {
            actualRefs = sortedOccRefs(it->second);
        }
        if (!sameOccRefs(expectedRefs, actualRefs)) {
            why = "actual occ: occurrence table does not match alive slot scan";
            return false;
        }
    }

    return true;
}

bool checkReducedInvariantActual(const harness::ReducedSPQRCore &core,
                                 const std::unordered_set<harness::NodeId> *allowedStubNodes,
                                 std::string &why) {
    if (!checkOwnershipConsistencyActual(core, why)) return false;
    if (!checkArcEndpointConsistencyActual(core, why)) return false;
    if (!checkOccurrenceConsistencyActual(core, why)) return false;

    std::vector<int> liveSlotCount(core.nodes.size(), 0);
    std::vector<int> liveRealSlotCount(core.nodes.size(), 0);
    std::vector<int> liveArcDegree(core.nodes.size(), 0);
    int explicitRealProjectionEdgeCount = 0;

    for (harness::NodeId nodeId = 0; nodeId < static_cast<harness::NodeId>(core.nodes.size()); ++nodeId) {
        const auto &node = core.nodes[nodeId];
        if (!node.alive) continue;

        for (const auto &slot : node.slots) {
            if (!slot.alive) continue;
            ++liveSlotCount[nodeId];
            if (!slot.isVirtual) {
                ++liveRealSlotCount[nodeId];
                ++explicitRealProjectionEdgeCount;
            }
        }

        if ((node.type == harness::SPQRType::S_NODE || node.type == harness::SPQRType::P_NODE) &&
            liveSlotCount[nodeId] < 3) {
            why = "actual reduced: live S/P node must have at least 3 alive slots";
            return false;
        }
    }

    for (harness::ArcId arcId = 0; arcId < static_cast<harness::ArcId>(core.arcs.size()); ++arcId) {
        const auto &arc = core.arcs[arcId];
        if (!arc.alive) continue;
        ++liveArcDegree[arc.a];
        ++liveArcDegree[arc.b];

        const auto &nodeA = core.nodes[arc.a];
        const auto &nodeB = core.nodes[arc.b];
        if ((nodeA.type == harness::SPQRType::S_NODE && nodeB.type == harness::SPQRType::S_NODE) ||
            (nodeA.type == harness::SPQRType::P_NODE && nodeB.type == harness::SPQRType::P_NODE)) {
            why = "actual reduced: adjacent same-type S/P nodes are forbidden";
            return false;
        }
    }

    for (harness::NodeId nodeId = 0; nodeId < static_cast<harness::NodeId>(core.nodes.size()); ++nodeId) {
        const auto &node = core.nodes[nodeId];
        if (!node.alive) continue;
        if (allowedStubNodes && allowedStubNodes->count(nodeId)) continue;
        if (liveRealSlotCount[nodeId] == 0 && liveArcDegree[nodeId] <= 2) {
            why = "actual reduced: dead relay forbidden";
            return false;
        }
    }

    if (core.totalAgg.edgeCnt != explicitRealProjectionEdgeCount) {
        std::ostringstream oss;
        oss << "actual reduced: totalAgg.edgeCnt mismatch (got "
            << core.totalAgg.edgeCnt
            << ", expected " << explicitRealProjectionEdgeCount << ")";
        why = oss.str();
        return false;
    }

    return true;
}
