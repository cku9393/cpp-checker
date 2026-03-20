#include "harness/project_static_adapter.hpp"

#include <sstream>

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

} // namespace

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

} // namespace harness
