#include "raw_engine/raw_engine.hpp"

using namespace std;

namespace {

bool side_is_boundary_only(const SepSidePrepared& side) {
    return side.innerVerts.empty() && side.hostedOcc.empty();
}

} // namespace

IsolatePrepared prepare_isolate_neighborhood(const RawEngine& RE, RawSkelID sid, OccID occ) {
    const RawOccRecord& O = RE.occ.get(occ);
    if (O.hostSkel != sid) {
        assert(false);
    }

    const RawVID c = O.centerV;
    assert(RE.V.get(c).kind == RawVertexKind::OCC_CENTER);
    assert(RE.V.get(c).orig == O.orig);

    IsolatePrepared out;
    out.occ = occ;
    out.orig = O.orig;
    out.allocNbr = O.allocNbr;
    sort(out.allocNbr.begin(), out.allocNbr.end());
    out.allocNbr.erase(unique(out.allocNbr.begin(), out.allocNbr.end()), out.allocNbr.end());

    unordered_set<u64> seenPort;
    const auto port_key = [](Vertex attach, BridgeRef br, u8 side, u8 kind) -> u64 {
        u64 k = attach;
        k = (k << 32U) ^ br;
        k = (k << 8U) ^ side;
        k = (k << 8U) ^ kind;
        return k;
    };

    for (RawEID eid : RE.V.get(c).adj) {
        const RawEdge& e = RE.E.get(eid);
        const RawVID x = other_end(e, c);

        if (e.kind == RawEdgeKind::REAL_PORT) {
            const Vertex ov = RE.V.get(x).orig;
            const u64 key = port_key(ov, 0, 0, 0);
            if (seenPort.insert(key).second) {
                out.ports.push_back(IsoPort{IsoPort::REAL, ov, 0, 0});
            }
        } else if (e.kind == RawEdgeKind::BRIDGE_PORT) {
            const Vertex ov = RE.V.get(x).orig;
            const u64 key = port_key(ov, e.br, e.side, 1);
            if (seenPort.insert(key).second) {
                out.ports.push_back(IsoPort{IsoPort::BRIDGE, ov, e.br, e.side});
            }
        } else {
            assert(false);
        }
    }

    unordered_map<Vertex, LVertex> idOf;
    const auto get_tid = [&](Vertex ov) -> LVertex {
        const auto it = idOf.find(ov);
        if (it != idOf.end()) {
            return it->second;
        }

        const LVertex id = static_cast<LVertex>(out.core.orig.size());
        out.core.orig.push_back(ov);
        idOf.emplace(ov, id);
        return id;
    };

    for (RawEID eid : O.corePatchEdges) {
        const RawEdge& e = RE.E.get(eid);
        assert(e.kind == RawEdgeKind::CORE_REAL);
        const Vertex oa = RE.V.get(e.a).orig;
        const Vertex ob = RE.V.get(e.b).orig;
        if (oa == ob) {
            continue;
        }

        const LVertex a = get_tid(oa);
        const LVertex b = get_tid(ob);
        out.core.edges.push_back(TinyEdge{a, b});
    }

    return out;
}

RawSkeletonBuilder build_isolated_occ_builder(const IsolatePrepared& in) {
    RawSkeletonBuilder B;
    unordered_map<Vertex, u32> vid;

    const auto get_vid = [&](Vertex ov, RawVertexKind kind, OccID occ) -> u32 {
        const auto it = vid.find(ov);
        if (it != vid.end()) {
            return it->second;
        }

        const u32 idx = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(kind, ov, occ));
        vid.emplace(ov, idx);
        return idx;
    };

    const u32 c = get_vid(in.orig, RawVertexKind::OCC_CENTER, in.occ);
    B.allocNbr[in.occ] = in.allocNbr;

    for (const IsoPort& p : in.ports) {
        const u32 x = get_vid(p.attachOrig, RawVertexKind::REAL, 0);
        const RawEdgeKind kind = (p.kind == IsoPort::REAL ? RawEdgeKind::REAL_PORT : RawEdgeKind::BRIDGE_PORT);
        B.E.push_back(make_builder_edge(c, x, kind, p.br, p.side));
    }

    vector<u32> coreLocal;
    for (const TinyEdge& e : in.core.edges) {
        const Vertex oa = in.core.orig[e.a];
        const Vertex ob = in.core.orig[e.b];
        if (oa == ob) {
            continue;
        }

        const u32 a = get_vid(oa, RawVertexKind::REAL, 0);
        const u32 b = get_vid(ob, RawVertexKind::REAL, 0);
        const u32 eid = static_cast<u32>(B.E.size());
        B.E.push_back(make_builder_edge(a, b, RawEdgeKind::CORE_REAL));
        coreLocal.push_back(eid);
    }

    B.corePatchLocalEids[in.occ] = move(coreLocal);
    return B;
}

RawSkeletonBuilder build_residual_after_isolate(const RawEngine& RE, RawSkelID sid, OccID removedOcc) {
    const RawSkeleton& S = RE.skel.get(sid);
    const RawOccRecord& dead = RE.occ.get(removedOcc);
    const RawVID deadCenter = dead.centerV;

    unordered_set<RawVID> keepV;
    unordered_set<RawEID> keepE;
    vector<OccID> survivors;

    for (OccID id : S.hostedOcc) {
        if (id != removedOcc) {
            survivors.push_back(id);
        }
    }

    for (OccID id : survivors) {
        const RawOccRecord& O = RE.occ.get(id);
        keepV.insert(O.centerV);

        for (RawEID eid : RE.V.get(O.centerV).adj) {
            const RawEdge& e = RE.E.get(eid);
            if (e.a == deadCenter || e.b == deadCenter) {
                continue;
            }
            keepE.insert(eid);
            keepV.insert(e.a);
            keepV.insert(e.b);
        }

        for (RawEID eid : O.corePatchEdges) {
            const RawEdge& e = RE.E.get(eid);
            if (e.a == deadCenter || e.b == deadCenter) {
                continue;
            }
            keepE.insert(eid);
            keepV.insert(e.a);
            keepV.insert(e.b);
        }
    }

    BuildCtx C;
    for (RawVID oldv : S.verts) {
        if (keepV.count(oldv) != 0U) {
            copy_old_vertex(C, RE, oldv, true);
        }
    }
    for (RawEID olde : S.edges) {
        if (keepE.count(olde) != 0U) {
            copy_old_edge(C, RE, olde);
        }
    }
    for (OccID id : survivors) {
        remap_occ_meta(C, RE, id, RE.occ.get(id).corePatchEdges);
    }
    return move(C.B);
}

IsolateVertexResult isolate_vertex(RawEngine& RE, RawSkelID sid, OccID occ, RawUpdateCtx& U) {
    IsolatePrepared prep = prepare_isolate_neighborhood(RE, sid, occ);
    RawSkeletonBuilder occB = build_isolated_occ_builder(prep);
    RawSkeletonBuilder residualB = build_residual_after_isolate(RE, sid, occ);

    assert_builder_basic(occB);
    assert_builder_basic(residualB);

    const RawSkelID sidOcc = RE.skel.alloc(RawSkeleton{});
    commit_skeleton(RE, sidOcc, move(occB), U);
    commit_skeleton(RE, sid, move(residualB), U);
    debug_validate_skeleton_and_hosted(RE, sidOcc);
    debug_validate_skeleton_and_hosted(RE, sid);

    return IsolateVertexResult{sidOcc, sid};
}

SplitPrepared prepare_split_separation_pair(const RawEngine& RE, RawSkelID sid, Vertex saOrig, Vertex sbOrig) {
    const RawSkeleton& S = RE.skel.get(sid);
    SplitPrepared out;
    out.aOrig = saOrig;
    out.bOrig = sbOrig;
    out.aV = find_real_orig_in_skeleton(RE, S, saOrig);
    out.bV = find_real_orig_in_skeleton(RE, S, sbOrig);

    const RawVID aV = out.aV;
    const RawVID bV = out.bV;
    const auto is_sep = [&](RawVID v) { return v == aV || v == bV; };

    unordered_map<RawVID, int> compId;
    vector<vector<RawVID>> compVerts;

    for (RawVID start : S.verts) {
        if (is_sep(start) || compId.count(start) != 0U) {
            continue;
        }

        const int cid = static_cast<int>(compVerts.size());
        compVerts.push_back({});
        deque<RawVID> dq;
        dq.push_back(start);
        compId[start] = cid;

        while (!dq.empty()) {
            const RawVID u = dq.front();
            dq.pop_front();
            compVerts[cid].push_back(u);
            for (RawEID eid : RE.V.get(u).adj) {
                const RawVID v = other_end(RE.E.get(eid), u);
                if (is_sep(v)) {
                    continue;
                }
                if (compId.count(v) == 0U) {
                    compId[v] = cid;
                    dq.push_back(v);
                }
            }
        }
    }

    out.side.resize(compVerts.size());
    for (int cid = 0; cid < static_cast<int>(compVerts.size()); ++cid) {
        SepSidePrepared& side = out.side[static_cast<size_t>(cid)];
        side.innerVerts = compVerts[static_cast<size_t>(cid)];
        for (RawVID v : compVerts[static_cast<size_t>(cid)]) {
            const RawVertex& RV = RE.V.get(v);
            if (RV.kind == RawVertexKind::OCC_CENTER) {
                side.hostedOcc.push_back(RV.occ);
            }
        }
    }

    int directABSide = -1;
    const auto ensure_direct_ab_side = [&]() -> int {
        if (directABSide != -1) {
            return directABSide;
        }
        directABSide = static_cast<int>(out.side.size());
        out.side.push_back(SepSidePrepared{});
        out.side[static_cast<size_t>(directABSide)].useA = true;
        out.side[static_cast<size_t>(directABSide)].useB = true;
        return directABSide;
    };

    for (RawEID eid : S.edges) {
        const RawEdge& e = RE.E.get(eid);
        const bool aSep = is_sep(e.a);
        const bool bSep = is_sep(e.b);

        if (!aSep && !bSep) {
            const int ca = compId.at(e.a);
            const int cb = compId.at(e.b);
            if (ca != cb) {
                assert(false);
            }
            out.side[static_cast<size_t>(ca)].innerEdges.push_back(eid);
            continue;
        }

        if (aSep && bSep) {
            const int sidx = ensure_direct_ab_side();
            out.side[static_cast<size_t>(sidx)].boundaryEdges.push_back(eid);
            continue;
        }

        const RawVID non = (aSep ? e.b : e.a);
        const int cid = compId.at(non);
        out.side[static_cast<size_t>(cid)].boundaryEdges.push_back(eid);

        if (aSep) {
            if (e.a == aV) {
                out.side[static_cast<size_t>(cid)].useA = true;
            } else {
                out.side[static_cast<size_t>(cid)].useB = true;
            }
        } else {
            if (e.b == aV) {
                out.side[static_cast<size_t>(cid)].useA = true;
            } else {
                out.side[static_cast<size_t>(cid)].useB = true;
            }
        }
    }

    for (int i = 0; i < static_cast<int>(out.side.size()); ++i) {
        if (i == directABSide) {
            continue;
        }
        const SepSidePrepared& side = out.side[static_cast<size_t>(i)];
        if (!side.innerVerts.empty() || !side.hostedOcc.empty()) {
            assert(side.useA && side.useB);
        }
    }

    return out;
}

RawSkeletonBuilder build_child_after_sep_split(const RawEngine& RE, const SplitPrepared& P, u32 sideIdx) {
    const SepSidePrepared& side = P.side[sideIdx];
    BuildCtx C;

    if (side.useA) {
        copy_old_vertex(C, RE, P.aV, true);
    }
    if (side.useB) {
        copy_old_vertex(C, RE, P.bV, true);
    }
    for (RawVID oldv : side.innerVerts) {
        copy_old_vertex(C, RE, oldv, true);
    }

    for (RawEID olde : side.innerEdges) {
        copy_old_edge(C, RE, olde);
    }
    for (RawEID olde : side.boundaryEdges) {
        copy_old_edge(C, RE, olde);
    }

    for (OccID occ : side.hostedOcc) {
        remap_occ_meta(C, RE, occ, RE.occ.get(occ).corePatchEdges);
    }
    return move(C.B);
}

SplitSeparationPairResult split_separation_pair(RawEngine& RE, RawSkelID sid, Vertex saOrig, Vertex sbOrig, RawUpdateCtx& U) {
    SplitPrepared prep = prepare_split_separation_pair(RE, sid, saOrig, sbOrig);
    vector<RawSkeletonBuilder> builders;
    builders.reserve(prep.side.size());
    for (u32 i = 0; i < static_cast<u32>(prep.side.size()); ++i) {
        builders.push_back(build_child_after_sep_split(RE, prep, i));
        assert_builder_basic(builders.back());
    }

    SplitSeparationPairResult res;
    res.aOrig = saOrig;
    res.bOrig = sbOrig;
    res.reusedSid = sid;
    res.child.resize(builders.size());

    if (builders.empty()) {
        retire_skeleton_contents(RE, sid);
        U.dirtySkeletons.insert(sid);
        debug_validate_skeleton_and_hosted(RE, sid);
        return res;
    }

    res.child[0].sid = sid;
    res.child[0].hostedOcc = prep.side[0].hostedOcc;
    res.child[0].boundaryOnly = side_is_boundary_only(prep.side[0]);
    commit_skeleton(RE, sid, move(builders[0]), U);
    debug_validate_skeleton_and_hosted(RE, sid);

    for (u32 i = 1; i < static_cast<u32>(builders.size()); ++i) {
        const RawSkelID nsid = RE.skel.alloc(RawSkeleton{});
        res.child[i].sid = nsid;
        res.child[i].hostedOcc = prep.side[i].hostedOcc;
        res.child[i].boundaryOnly = side_is_boundary_only(prep.side[i]);
        commit_skeleton(RE, nsid, move(builders[i]), U);
        debug_validate_skeleton_and_hosted(RE, nsid);
    }

    return res;
}

RawSkeletonBuilder build_merged_after_sep_join(
    const RawEngine& RE,
    RawSkelID leftSid,
    RawSkelID rightSid,
    Vertex saOrig,
    Vertex sbOrig
) {
    const RawSkeleton& L = RE.skel.get(leftSid);
    const RawSkeleton& R = RE.skel.get(rightSid);

    const RawVID lA = find_real_orig_in_skeleton(RE, L, saOrig);
    const RawVID lB = find_real_orig_in_skeleton(RE, L, sbOrig);
    const RawVID rA = find_real_orig_in_skeleton(RE, R, saOrig);
    const RawVID rB = find_real_orig_in_skeleton(RE, R, sbOrig);

    BuildCtx C;

    const u32 a = static_cast<u32>(C.B.V.size());
    C.B.V.push_back(make_builder_vertex(RawVertexKind::REAL, saOrig, 0));
    C.mapV.emplace(lA, a);
    C.mapV.emplace(rA, a);
    C.seenRealOrig.insert(saOrig);

    const u32 b = static_cast<u32>(C.B.V.size());
    C.B.V.push_back(make_builder_vertex(RawVertexKind::REAL, sbOrig, 0));
    C.mapV.emplace(lB, b);
    C.mapV.emplace(rB, b);
    C.seenRealOrig.insert(sbOrig);

    for (RawVID v : L.verts) {
        if (v != lA && v != lB) {
            copy_old_vertex(C, RE, v, true);
        }
    }
    for (RawVID v : R.verts) {
        if (v != rA && v != rB) {
            copy_old_vertex(C, RE, v, true);
        }
    }

    for (RawEID e : L.edges) {
        copy_old_edge(C, RE, e);
    }
    for (RawEID e : R.edges) {
        copy_old_edge(C, RE, e);
    }

    unordered_set<OccID> seenOcc;
    const auto copy_occ_meta_from = [&](const RawSkeleton& S) {
        for (OccID occ : S.hostedOcc) {
            const bool inserted = seenOcc.insert(occ).second;
            if (!inserted) {
                assert(false);
            }
            remap_occ_meta(C, RE, occ, RE.occ.get(occ).corePatchEdges);
        }
    };
    copy_occ_meta_from(L);
    copy_occ_meta_from(R);

    return move(C.B);
}

JoinSeparationPairResult join_separation_pair(
    RawEngine& RE,
    RawSkelID leftSid,
    RawSkelID rightSid,
    Vertex saOrig,
    Vertex sbOrig,
    RawUpdateCtx& U
) {
    assert(leftSid != rightSid);
    RawSkeletonBuilder B = build_merged_after_sep_join(RE, leftSid, rightSid, saOrig, sbOrig);
    assert_builder_basic(B);
    commit_skeleton(RE, leftSid, move(B), U);
    retire_skeleton_contents(RE, rightSid);
    RE.skel.retire(rightSid);
    debug_validate_skeleton_and_hosted(RE, leftSid);
    return JoinSeparationPairResult{leftSid, rightSid};
}

RawSkeletonBuilder build_merged_after_integrate(
    const RawEngine& RE,
    RawSkelID parentSid,
    RawSkelID childSid,
    const vector<BoundaryMapEntry>& bm
) {
    const RawSkeleton& P = RE.skel.get(parentSid);
    const RawSkeleton& Cc = RE.skel.get(childSid);

    BuildCtx C;
    unordered_map<Vertex, RawVID> parentBoundaryRaw;
    unordered_map<Vertex, RawVID> childBoundaryRaw;
    unordered_set<Vertex> seenParentBoundary;
    unordered_set<Vertex> seenChildBoundary;

    for (const BoundaryMapEntry& x : bm) {
        const bool ok1 = seenParentBoundary.insert(x.parentOrig).second;
        const bool ok2 = seenChildBoundary.insert(x.childOrig).second;
        if (!ok1 || !ok2) {
            assert(false);
        }

        parentBoundaryRaw.emplace(x.parentOrig, find_real_orig_in_skeleton(RE, P, x.parentOrig));
        childBoundaryRaw.emplace(x.childOrig, find_real_orig_in_skeleton(RE, Cc, x.childOrig));
    }

    for (RawVID v : P.verts) {
        copy_old_vertex(C, RE, v, true);
    }

    for (const BoundaryMapEntry& x : bm) {
        const RawVID childV = childBoundaryRaw.at(x.childOrig);
        const RawVID parentV = parentBoundaryRaw.at(x.parentOrig);
        assert(C.mapV.count(parentV) != 0U);
        C.mapV.emplace(childV, C.mapV.at(parentV));
    }

    const auto is_child_boundary_real = [&](RawVID oldv) -> bool {
        const RawVertex& RV = RE.V.get(oldv);
        if (RV.kind != RawVertexKind::REAL) {
            return false;
        }
        const auto it = childBoundaryRaw.find(RV.orig);
        return it != childBoundaryRaw.end() && it->second == oldv;
    };

    for (RawVID v : Cc.verts) {
        if (!is_child_boundary_real(v)) {
            copy_old_vertex(C, RE, v, true);
        }
    }

    for (RawEID e : P.edges) {
        copy_old_edge(C, RE, e);
    }
    for (RawEID e : Cc.edges) {
        copy_old_edge(C, RE, e);
    }

    unordered_set<OccID> seenOcc;
    const auto copy_occ_meta_from = [&](const RawSkeleton& S) {
        for (OccID occ : S.hostedOcc) {
            const bool inserted = seenOcc.insert(occ).second;
            if (!inserted) {
                assert(false);
            }
            remap_occ_meta(C, RE, occ, RE.occ.get(occ).corePatchEdges);
        }
    };
    copy_occ_meta_from(P);
    copy_occ_meta_from(Cc);

    return move(C.B);
}

IntegrateResult integrate_skeleton(
    RawEngine& RE,
    RawSkelID parentSid,
    RawSkelID childSid,
    const vector<BoundaryMapEntry>& bm,
    RawUpdateCtx& U
) {
    assert(parentSid != childSid);
    RawSkeletonBuilder B = build_merged_after_integrate(RE, parentSid, childSid, bm);
    assert_builder_basic(B);
    commit_skeleton(RE, parentSid, move(B), U);
    retire_skeleton_contents(RE, childSid);
    RE.skel.retire(childSid);
    debug_validate_skeleton_and_hosted(RE, parentSid);
    return IntegrateResult{parentSid, childSid};
}
