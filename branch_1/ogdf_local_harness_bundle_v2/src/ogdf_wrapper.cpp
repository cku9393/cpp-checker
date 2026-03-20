#include "harness/ogdf_wrapper.hpp"
#include <sstream>

namespace harness {

bool OgdfRawSpqrBackend::buildRaw(const CompactGraph &H,
                                  RawSpqrDecomp &raw,
                                  std::string &err) {
#if USE_OGDF
    if (!buildRawSpqrDecompWithOgdf(H, raw, err)) {
        if (err.empty()) err = "buildRawSpqrDecompWithOgdf returned false";
        return false;
    }
    return true;
#else
    (void)H; (void)raw;
    err = "USE_OGDF not enabled";
    return false;
#endif
}

#if USE_OGDF
namespace ogdf_wrap {

void buildOgdfGraph(const CompactGraph &H, BuildContext &B) {
    B.cvNode.resize(H.origOfCv.size());
    B.inputEdge.resize(H.edges.size());

    for (int cv = 0; cv < (int)H.origOfCv.size(); ++cv) {
        B.cvNode[cv] = B.G.newNode();
    }

    B.inputIdOfEdge = std::make_unique<ogdf::EdgeArray<int>>(B.G, -1);
    B.origVertexOfGraphNode = std::make_unique<ogdf::NodeArray<VertexId>>(B.G, -1);

    for (int cv = 0; cv < (int)H.origOfCv.size(); ++cv) {
        (*B.origVertexOfGraphNode)[B.cvNode[cv]] = H.origOfCv[cv];
    }

    for (const auto &e : H.edges) {
        ogdf::edge ge = B.G.newEdge(B.cvNode[e.a], B.cvNode[e.b]);
        B.inputEdge[e.id] = ge;
        (*B.inputIdOfEdge)[ge] = e.id;
    }

}

SPQRType mapType(ogdf::SPQRTree::NodeType t) {
    using NT = ogdf::SPQRTree::NodeType;
    switch (t) {
        case NT::SNode: return SPQRType::S_NODE;
        case NT::PNode: return SPQRType::P_NODE;
        case NT::RNode: return SPQRType::R_NODE;
        default: return SPQRType::R_NODE;
    }
}

VertexId originalOfSkeletonNode(const ogdf::Skeleton &S,
                                ogdf::node vSk,
                                const ogdf::NodeArray<VertexId> &origVertexOfGraphNode) {
    ogdf::node vOrig = S.original(vSk);
    return origVertexOfGraphNode[vOrig];
}

std::pair<VertexId,VertexId>
polesOfSkeletonEdge(const ogdf::Skeleton &S,
                    ogdf::edge eSk,
                    const ogdf::NodeArray<VertexId> &origVertexOfGraphNode) {
    VertexId a = originalOfSkeletonNode(S, eSk->source(), origVertexOfGraphNode);
    VertexId b = originalOfSkeletonNode(S, eSk->target(), origVertexOfGraphNode);
    return {a, b};
}

bool computeCycleOrderFromSkeleton(
    const ogdf::Skeleton &S,
    const ogdf::EdgeArray<int> &slotOfSkel,
    std::vector<int> &out,
    std::string &why
) {
    out.clear();
    why.clear();
    const ogdf::Graph &GS = S.getGraph();

    int m = 0;
#if OGDF_HAS_RANGE_GRAPH_ITER
    for (ogdf::edge e : GS.edges) { (void)e; ++m; }
#else
    for (ogdf::edge e = GS.firstEdge(); e; e = e->succ()) { ++m; }
#endif
    if (m < 3) {
        why = "S skeleton must have at least 3 edges";
        return false;
    }

#if OGDF_HAS_RANGE_GRAPH_ITER
    for (ogdf::node v : GS.nodes) {
#else
    for (ogdf::node v = GS.firstNode(); v; v = v->succ()) {
#endif
        int deg = 0;
#if OGDF_HAS_NODE_ADJENTRIES
        for (ogdf::adjEntry adj : v->adjEntries) { (void)adj; ++deg; }
#else
        for (ogdf::adjEntry adj = v->firstAdj(); adj; adj = adj->succ()) { ++deg; }
#endif
        if (deg != 2) {
            why = "S skeleton vertex is not degree-2";
            return false;
        }
    }

    ogdf::edge start = GS.firstEdge();
    if (start == nullptr) {
        why = "S skeleton has no edge";
        return false;
    }

    std::vector<int> order;
    order.reserve(m);

    std::unordered_set<void*> used;
    ogdf::edge curE = start;
    ogdf::node curV = start->target();

    while (true) {
        if (slotOfSkel[curE] < 0) {
            why = "slotOfSkel missing on S edge";
            return false;
        }

        if (!used.insert((void*)curE).second) {
            if (curE != start || (int)order.size() != m) {
                why = "S cycle walk revisited edge too early";
                return false;
            }
            break;
        }

        order.push_back(slotOfSkel[curE]);

        ogdf::edge nextE = nullptr;
#if OGDF_HAS_NODE_ADJENTRIES
        for (ogdf::adjEntry adj : curV->adjEntries) {
#else
        for (ogdf::adjEntry adj = curV->firstAdj(); adj; adj = adj->succ()) {
#endif
            ogdf::edge cand = adj->theEdge();
            if (cand == curE) continue;
            nextE = cand;
            break;
        }
        if (nextE == nullptr) {
            why = "S cycle walk could not find next edge";
            return false;
        }

        ogdf::node nextV =
            (nextE->source() == curV ? nextE->target() : nextE->source());

        curE = nextE;
        curV = nextV;

        if (curE == start) break;
    }

    std::vector<int> sorted = order;
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());

    if ((int)sorted.size() != m) {
        why = "S cycleSlots has duplicates";
        return false;
    }
    if ((int)order.size() != m) {
        why = "S cycleSlots size mismatch";
        return false;
    }
    out = std::move(order);
    return true;
}

bool buildRShapeFromSkeleton(
    const ogdf::Skeleton &S,
    const ogdf::NodeArray<VertexId> &origVertexOfGraphNode,
    const ogdf::EdgeArray<int> &slotOfSkel,
    int numSlots,
    RawRShape &out,
    std::string &why
) {
    why.clear();
    const ogdf::Graph &GS = S.getGraph();

    std::unordered_map<void*, int> localId;
    out.skelVertices.clear();
    out.incSlots.clear();
    out.endsOfSlot.assign(numSlots, {-1, -1});

#if OGDF_HAS_RANGE_GRAPH_ITER
    for (ogdf::node v : GS.nodes) {
#else
    for (ogdf::node v = GS.firstNode(); v; v = v->succ()) {
#endif
        int id = (int)out.skelVertices.size();
        localId[(void*)v] = id;
        out.skelVertices.push_back(originalOfSkeletonNode(S, v, origVertexOfGraphNode));
        out.incSlots.push_back({});
    }

    std::vector<int> seen(numSlots, 0);

#if OGDF_HAS_RANGE_GRAPH_ITER
    for (ogdf::edge eSk : GS.edges) {
#else
    for (ogdf::edge eSk = GS.firstEdge(); eSk; eSk = eSk->succ()) {
#endif
        int slot = slotOfSkel[eSk];
        if (slot < 0 || slot >= numSlots) {
            why = "R slotOfSkel invalid";
            return false;
        }

        int a = localId[(void*)eSk->source()];
        int b = localId[(void*)eSk->target()];

        out.endsOfSlot[slot] = {a, b};
        out.incSlots[a].push_back(slot);
        out.incSlots[b].push_back(slot);
        ++seen[slot];
    }

    for (int s = 0; s < numSlots; ++s) {
        if (seen[s] != 1) {
            why = "R shape slot missing or duplicated";
            return false;
        }
    }

    for (auto &vec : out.incSlots) {
        std::sort(vec.begin(), vec.end());
        vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
    }
    return true;
}

} // namespace ogdf_wrap
#endif

bool buildRawSpqrDecompWithOgdf(const CompactGraph &H,
                                RawSpqrDecomp &raw,
                                std::string &why) {
#if USE_OGDF
    using namespace ogdf_wrap;
    why.clear();
    try {
        raw = {};
        raw.ownerOfInputEdge.assign(H.edges.size(), {-1, -1});

        BuildContext B;
        buildOgdfGraph(H, B);
        if (B.inputEdge.empty()) {
            why = "OGDF raw materialize: compact graph has no input edges";
            return false;
        }

        ogdf::StaticSPQRTree T(B.G, B.inputEdge[0]);

        std::unordered_map<void*, int> rawIdOfTreeNode;
        forEachTreeNode(T, [&](ogdf::node tv) {
            int id = (int)raw.nodes.size();
            rawIdOfTreeNode[(void*)tv] = id;
            raw.nodes.push_back({});
            raw.nodes.back().alive = true;
            raw.nodes.back().type = mapType(T.typeOf(tv));
        });

        std::unordered_map<SkelEdgeKey, int, SkelEdgeKeyHash> treeEdgeIdOfSkelCopy;

        forEachTreeEdge(T, [&](ogdf::edge te) {
            int a = rawIdOfTreeNode[(void*)te->source()];
            int b = rawIdOfTreeNode[(void*)te->target()];

            auto &E = raw.treeEdges.emplace_back();
            int tid = (int)raw.treeEdges.size() - 1;
            E.alive = true;
            E.a = a;
            E.b = b;

            ogdf::edge sa = T.skeletonEdgeSrc(te);
            ogdf::edge sb = T.skeletonEdgeTgt(te);

            const auto &SkA = T.skeleton(te->source());
            const auto &SkB = T.skeleton(te->target());

            auto pa = polesOfSkeletonEdge(SkA, sa, *B.origVertexOfGraphNode);
            auto pb = polesOfSkeletonEdge(SkB, sb, *B.origVertexOfGraphNode);

            if (canonPole(pa.first, pa.second) != canonPole(pb.first, pb.second)) {
                throw HarnessError("tree edge endpoint pole mismatch");
            }

            E.poleA = pa.first;
            E.poleB = pa.second;

            treeEdgeIdOfSkelCopy[keyOf(SkA, sa)] = tid;
            treeEdgeIdOfSkelCopy[keyOf(SkB, sb)] = tid;
        });

        std::unordered_map<SkelEdgeKey, int, SkelEdgeKeyHash> inputIdOfSkelCopy;
        for (int inputId = 0; inputId < (int)B.inputEdge.size(); ++inputId) {
            ogdf::edge e = B.inputEdge[inputId];
            ogdf::edge eSk = T.copyOfReal(e);
            const auto &Sk = T.skeletonOfReal(e);
            inputIdOfSkelCopy[keyOf(Sk, eSk)] = inputId;
        }

        forEachTreeNode(T, [&](ogdf::node tv) {
            int u = rawIdOfTreeNode[(void*)tv];
            auto &RN = raw.nodes[u];
            const auto &Sk = T.skeleton(tv);
            ogdf::EdgeArray<int> slotOfSkel(Sk.getGraph(), -1);

            forEachSkeletonEdge(Sk, [&](ogdf::edge eSk) {
                RawSlot rs;
                rs.alive = true;
                auto poles = polesOfSkeletonEdge(Sk, eSk, *B.origVertexOfGraphNode);
                rs.poleA = poles.first;
                rs.poleB = poles.second;

                auto k = keyOf(Sk, eSk);
                auto itInput = inputIdOfSkelCopy.find(k);
                if (itInput != inputIdOfSkelCopy.end()) {
                    rs.kind = RawSlotKind::INPUT_EDGE;
                    rs.inputEdgeId = itInput->second;
                } else {
                    auto itTree = treeEdgeIdOfSkelCopy.find(k);
                    if (itTree == treeEdgeIdOfSkelCopy.end()) {
                        throw HarnessError("skeleton edge could not be classified as INPUT or TREE");
                    }
                    rs.kind = RawSlotKind::TREE_EDGE;
                    rs.treeEdgeId = itTree->second;
                }

                int pos = (int)RN.slots.size();
                RN.slots.push_back(rs);
                slotOfSkel[eSk] = pos;

                if (rs.kind == RawSlotKind::INPUT_EDGE) {
                    raw.ownerOfInputEdge[rs.inputEdgeId] = {u, pos};
                }
            });

            if (RN.type == SPQRType::S_NODE) {
                if (!computeCycleOrderFromSkeleton(Sk, slotOfSkel, RN.cycleSlots, why)) {
                    why = "OGDF raw materialize: " + why;
                    throw HarnessError(why);
                }
            } else if (RN.type == SPQRType::P_NODE) {
                RawPShape p;
                for (const auto &s : RN.slots) {
                    if (!s.alive) continue;
                    p.poleA = s.poleA;
                    p.poleB = s.poleB;
                    break;
                }
                RN.pShape = p;
            } else {
                RawRShape r;
                if (!buildRShapeFromSkeleton(Sk,
                                             *B.origVertexOfGraphNode,
                                             slotOfSkel,
                                             (int)RN.slots.size(),
                                             r,
                                             why)) {
                    why = "OGDF raw materialize: " + why;
                    throw HarnessError(why);
                }
                RN.rShape = std::move(r);
            }
        });

        for (int tid = 0; tid < (int)raw.treeEdges.size(); ++tid) {
            auto &E = raw.treeEdges[tid];
            bool okA = false, okB = false;
            for (int i = 0; i < (int)raw.nodes[E.a].slots.size(); ++i) {
                const auto &s = raw.nodes[E.a].slots[i];
                if (s.alive && s.kind == RawSlotKind::TREE_EDGE && s.treeEdgeId == tid) {
                    E.slotInA = i;
                    okA = true;
                    break;
                }
            }
            for (int i = 0; i < (int)raw.nodes[E.b].slots.size(); ++i) {
                const auto &s = raw.nodes[E.b].slots[i];
                if (s.alive && s.kind == RawSlotKind::TREE_EDGE && s.treeEdgeId == tid) {
                    E.slotInB = i;
                    okB = true;
                    break;
                }
            }
            if (!okA || !okB) {
                why = "OGDF raw materialize: tree edge slot lookup failed";
                return false;
            }
        }

        raw.valid = true;
        raw.error = RawDecompError::NONE;
        return true;
    } catch (const HarnessError &e) {
        if (why.empty()) why = std::string("OGDF raw materialize: ") + e.what();
        return false;
    } catch (const std::exception &e) {
        why = std::string("OGDF exception: ") + e.what();
        return false;
    } catch (...) {
        why = "OGDF exception: unknown";
        return false;
    }
#else
    (void)H; (void)raw; (void)why;
    return false;
#endif
}

} // namespace harness
