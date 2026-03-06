#include "raw_engine/raw_engine.hpp"

using namespace std;

RawSkeletonBuilder::BV make_builder_vertex(RawVertexKind kind, Vertex orig, OccID occ) {
    return RawSkeletonBuilder::BV{kind, orig, occ};
}

RawSkeletonBuilder::BE make_builder_edge(u32 a, u32 b, RawEdgeKind kind, BridgeRef br, u8 side) {
    return RawSkeletonBuilder::BE{a, b, kind, br, side};
}

RawOccRecord make_occ_record(Vertex orig) {
    return RawOccRecord{orig, 0, 0, {}, {}};
}

RawVID other_end(const RawEdge& e, RawVID u) {
    assert(e.a == u || e.b == u);
    return (e.a == u ? e.b : e.a);
}

u32 copy_old_vertex(BuildCtx& C, const RawEngine& RE, RawVID oldv, bool checkRealUnique) {
    const auto it = C.mapV.find(oldv);
    if (it != C.mapV.end()) {
        return it->second;
    }

    const RawVertex& RV = RE.V.get(oldv);
    if (checkRealUnique && RV.kind == RawVertexKind::REAL) {
        const bool inserted = C.seenRealOrig.insert(RV.orig).second;
        if (!inserted) {
            assert(false);
        }
    }

    const u32 idx = static_cast<u32>(C.B.V.size());
    C.B.V.push_back(make_builder_vertex(RV.kind, RV.orig, RV.occ));
    C.mapV.emplace(oldv, idx);
    return idx;
}

u32 copy_old_edge(BuildCtx& C, const RawEngine& RE, RawEID olde) {
    const auto it = C.mapE.find(olde);
    if (it != C.mapE.end()) {
        return it->second;
    }

    const RawEdge& e = RE.E.get(olde);
    assert(C.mapV.count(e.a) != 0U);
    assert(C.mapV.count(e.b) != 0U);

    const u32 a = C.mapV.at(e.a);
    const u32 b = C.mapV.at(e.b);
    assert(a != b);

    const u32 idx = static_cast<u32>(C.B.E.size());
    C.B.E.push_back(make_builder_edge(a, b, e.kind, e.br, e.side));
    C.mapE.emplace(olde, idx);
    return idx;
}

void remap_occ_meta(BuildCtx& C, const RawEngine& RE, OccID occ, const vector<RawEID>& oldCoreEdges) {
    const RawOccRecord& O = RE.occ.get(occ);
    C.B.allocNbr[occ] = O.allocNbr;

    vector<u32> ce;
    ce.reserve(oldCoreEdges.size());
    for (RawEID olde : oldCoreEdges) {
        const auto it = C.mapE.find(olde);
        if (it != C.mapE.end()) {
            ce.push_back(it->second);
        }
    }
    C.B.corePatchLocalEids[occ] = move(ce);
}

LabeledComponents label_components_without_blocked(
    const RawEngine& RE,
    const RawSkeleton& S,
    const unordered_set<RawVID>& blocked
) {
    LabeledComponents out;
    for (RawVID start : S.verts) {
        if (blocked.count(start) != 0U) {
            continue;
        }
        if (out.compId.count(start) != 0U) {
            continue;
        }

        const int cid = static_cast<int>(out.compVerts.size());
        out.compVerts.push_back({});

        deque<RawVID> dq;
        dq.push_back(start);
        out.compId[start] = cid;

        while (!dq.empty()) {
            const RawVID u = dq.front();
            dq.pop_front();
            out.compVerts[cid].push_back(u);

            for (RawEID eid : RE.V.get(u).adj) {
                const RawVID v = other_end(RE.E.get(eid), u);
                if (blocked.count(v) != 0U) {
                    continue;
                }
                if (out.compId.count(v) == 0U) {
                    out.compId[v] = cid;
                    dq.push_back(v);
                }
            }
        }
    }
    return out;
}

vector<RawVID> collect_support_vertices(const RawEngine& RE, OccID occ) {
    const RawOccRecord& O = RE.occ.get(occ);
    const RawVID c = O.centerV;

    unordered_set<RawVID> seen;
    vector<RawVID> out;

    const auto add = [&](RawVID v) {
        if (seen.insert(v).second) {
            out.push_back(v);
        }
    };

    for (RawEID eid : RE.V.get(c).adj) {
        add(other_end(RE.E.get(eid), c));
    }

    for (RawEID eid : O.corePatchEdges) {
        const RawEdge& e = RE.E.get(eid);
        add(e.a);
        add(e.b);
    }

    return out;
}

RawVID find_real_orig_in_skeleton(const RawEngine& RE, const RawSkeleton& S, Vertex ov) {
    RawVID ans = NIL_U32;
    for (RawVID v : S.verts) {
        const RawVertex& RV = RE.V.get(v);
        if (RV.kind == RawVertexKind::REAL && RV.orig == ov) {
            assert(ans == NIL_U32);
            ans = v;
        }
    }
    assert(ans != NIL_U32);
    return ans;
}

void retire_skeleton_contents(RawEngine& RE, RawSkelID sid) {
    RawSkeleton& S = RE.skel.get(sid);
    for (RawVID v : S.verts) {
        RE.V.retire(v);
    }
    for (RawEID e : S.edges) {
        RE.E.retire(e);
    }
    S.verts.clear();
    S.edges.clear();
    S.hostedOcc.clear();
}

void commit_skeleton(RawEngine& RE, RawSkelID sid, RawSkeletonBuilder&& B, RawUpdateCtx& U) {
    RawSkeleton& existing = RE.skel.get(sid);
    if (!existing.verts.empty() || !existing.edges.empty()) {
        retire_skeleton_contents(RE, sid);
    }

    RawSkeleton& S = RE.skel.get(sid);
    vector<RawVID> mapV(B.V.size(), 0);
    for (u32 i = 0; i < static_cast<u32>(B.V.size()); ++i) {
        mapV[i] = RE.V.alloc(RawVertex{B.V[i].kind, B.V[i].orig, B.V[i].occ, {}});
        S.verts.push_back(mapV[i]);
    }

    vector<RawEID> mapE(B.E.size(), 0);
    for (u32 i = 0; i < static_cast<u32>(B.E.size()); ++i) {
        const RawVID a = mapV[B.E[i].a];
        const RawVID b = mapV[B.E[i].b];
        mapE[i] = RE.E.alloc(RawEdge{a, b, B.E[i].kind, B.E[i].br, B.E[i].side});
        S.edges.push_back(mapE[i]);
        RE.V.get(a).adj.push_back(mapE[i]);
        RE.V.get(b).adj.push_back(mapE[i]);
    }

    S.hostedOcc.clear();
    for (RawVID v : S.verts) {
        const RawVertex& RV = RE.V.get(v);
        if (RV.kind != RawVertexKind::OCC_CENTER) {
            continue;
        }

        const OccID id = RV.occ;
        S.hostedOcc.push_back(id);

        RawOccRecord& O = RE.occ.get(id);
        O.orig = RV.orig;
        O.hostSkel = sid;
        O.centerV = v;

        const auto itNbr = B.allocNbr.find(id);
        O.allocNbr = (itNbr == B.allocNbr.end() ? vector<OccID>{} : itNbr->second);

        O.corePatchEdges.clear();
        const auto itEdge = B.corePatchLocalEids.find(id);
        if (itEdge != B.corePatchLocalEids.end()) {
            for (u32 le : itEdge->second) {
                O.corePatchEdges.push_back(mapE[le]);
            }
        }

        U.dirtyOccurrences.insert(id);
    }

    U.dirtySkeletons.insert(sid);
}
