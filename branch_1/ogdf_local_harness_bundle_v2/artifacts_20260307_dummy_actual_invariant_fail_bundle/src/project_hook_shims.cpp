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

void addAdjArc(harness::SPQRNodeCore &node, harness::ArcId arcId) {
    if (std::find(node.adjArcs.begin(), node.adjArcs.end(), arcId) == node.adjArcs.end()) {
        node.adjArcs.push_back(arcId);
    }
}

void removeAdjArc(harness::SPQRNodeCore &node, harness::ArcId arcId) {
    node.adjArcs.erase(std::remove(node.adjArcs.begin(), node.adjArcs.end(), arcId),
                       node.adjArcs.end());
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

    if (!clearNodeKeepIdForGraft(core, oldNode, mini.nodes[keep].type, &why)) {
        return false;
    }

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

    std::vector<int> seenProxyInput(compact.edges.size(), 0);
    std::vector<harness::RewiredProxyEdge> rewiredProxyEdges;
    for (int inputEdgeId = 0; inputEdgeId < static_cast<int>(compact.edges.size()); ++inputEdgeId) {
        const auto &edge = compact.edges[inputEdgeId];
        if (edge.kind != harness::CompactEdgeKind::PROXY) continue;

        if (inputEdgeId >= static_cast<int>(mini.ownerOfInputEdge.size())) {
            why = "graft: PROXY input has no owner mini slot";
            return false;
        }
        const auto owner = mini.ownerOfInputEdge[inputEdgeId];
        if (owner.first < 0 || owner.first >= static_cast<int>(mini.nodes.size()) ||
            owner.second < 0 ||
            owner.second >= static_cast<int>(mini.nodes[owner.first].slots.size())) {
            why = "graft: PROXY input has no owner mini slot";
            return false;
        }
        const auto &ownerMiniNode = mini.nodes[owner.first];
        const auto &ownerMiniSlot = ownerMiniNode.slots[owner.second];
        if (!ownerMiniNode.alive || !ownerMiniSlot.alive ||
            ownerMiniSlot.kind != harness::MiniSlotKind::PROXY_INPUT) {
            why = "graft: PROXY input has no owner mini slot";
            return false;
        }

        const harness::NodeId actualNodeId = actualOfMini[owner.first];
        const int actualSlotId = actualSlotOfMiniSlot[owner.first][owner.second];
        if (actualNodeId < 0 || actualSlotId < 0) {
            why = "graft: PROXY actual slot missing";
            return false;
        }
        if (!validCoreArc(core, edge.oldArc) ||
            !validCoreNode(core, edge.outsideNode) ||
            edge.oldSlotInU < 0) {
            why = "graft: invalid oldArc/outsideNode metadata";
            return false;
        }

        const auto &oldArc = core.arcs[edge.oldArc];
        const harness::NodeId otherNode =
            oldArc.a == oldNode ? oldArc.b :
            oldArc.b == oldNode ? oldArc.a :
            -1;
        if (otherNode != edge.outsideNode) {
            why = "graft: invalid oldArc/outsideNode metadata";
            return false;
        }

        if (!rewireArcEndpoint(core, edge.oldArc, oldNode, actualNodeId, actualSlotId, &why)) {
            return false;
        }

        ++seenProxyInput[inputEdgeId];
        rewiredProxyEdges.push_back({inputEdgeId, edge.oldArc, actualNodeId, actualSlotId});
    }

    for (int inputEdgeId = 0; inputEdgeId < static_cast<int>(compact.edges.size()); ++inputEdgeId) {
        if (compact.edges[inputEdgeId].kind != harness::CompactEdgeKind::PROXY) continue;
        if (seenProxyInput[inputEdgeId] != 1) {
            why = "graft: some PROXY input was not rewired exactly once";
            return false;
        }
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
    }

    return true;
}

bool rebuildActualMetadata(harness::ReducedSPQRCore &,
                           std::string &why) {
    why.clear();
    return true;
}
