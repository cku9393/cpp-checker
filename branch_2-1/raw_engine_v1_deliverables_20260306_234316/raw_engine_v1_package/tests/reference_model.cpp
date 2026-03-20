#include "reference_model.hpp"

#include <algorithm>
#include <deque>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

using namespace std;

namespace {

using ReferenceModel = RawEngine;

string join_strings(const vector<string>& parts, const string& sep) {
    ostringstream oss;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i != 0U) {
            oss << sep;
        }
        oss << parts[i];
    }
    return oss.str();
}

template <class T>
string join_values(const vector<T>& parts, const string& sep) {
    ostringstream oss;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i != 0U) {
            oss << sep;
        }
        oss << parts[i];
    }
    return oss.str();
}

string vertex_label(const RawVertex& v) {
    if (v.kind == RawVertexKind::REAL) {
        return string("R:") + to_string(v.orig);
    }
    return string("C:") + to_string(v.orig) + ":" + to_string(v.occ);
}

string edge_kind_label(const RawEdge& e) {
    switch (e.kind) {
        case RawEdgeKind::CORE_REAL:
            return "CORE";
        case RawEdgeKind::REAL_PORT:
            return "REAL_PORT";
        case RawEdgeKind::BRIDGE_PORT:
            return string("BRIDGE_PORT:") + to_string(e.br) + ":" + to_string(static_cast<int>(e.side));
    }
    return "UNKNOWN";
}

string edge_label(const RawEngine& RE, RawEID eid) {
    const RawEdge& e = RE.E.get(eid);
    string a = vertex_label(RE.V.get(e.a));
    string b = vertex_label(RE.V.get(e.b));
    if (a > b) {
        swap(a, b);
    }
    return edge_kind_label(e) + "|" + a + "|" + b;
}

string core_patch_edge_label(const RawEngine& RE, RawEID eid) {
    const RawEdge& e = RE.E.get(eid);
    Vertex a = RE.V.get(e.a).orig;
    Vertex b = RE.V.get(e.b).orig;
    if (a > b) {
        swap(a, b);
    }
    return string("CORE|R:") + to_string(a) + "|R:" + to_string(b);
}

string describe_skeleton_signature(const CanonicalSkeletonSignature& skeleton) {
    return string("{verts=[") + join_strings(skeleton.vertices, ",") +
           "],edges=[" + join_strings(skeleton.edges, ",") +
           "],hosted=[" + join_values(skeleton.hostedOcc, ",") + "]}";
}

CanonicalSkeletonSignature capture_skeleton_signature(const RawEngine& RE, RawSkelID sid) {
    CanonicalSkeletonSignature sig;
    const RawSkeleton& S = RE.skel.get(sid);
    for (RawVID v : S.verts) {
        sig.vertices.push_back(vertex_label(RE.V.get(v)));
    }
    sort(sig.vertices.begin(), sig.vertices.end());

    for (RawEID eid : S.edges) {
        sig.edges.push_back(edge_label(RE, eid));
    }
    sort(sig.edges.begin(), sig.edges.end());

    sig.hostedOcc = S.hostedOcc;
    sort(sig.hostedOcc.begin(), sig.hostedOcc.end());
    return sig;
}

CanonicalOccurrenceSignature capture_occurrence_signature(
    const RawEngine& RE,
    OccID occ,
    const unordered_map<RawSkelID, string>& hostSig
) {
    const RawOccRecord& O = RE.occ.get(occ);
    CanonicalOccurrenceSignature sig;
    sig.occ = occ;
    sig.orig = O.orig;
    sig.hostSkel = hostSig.at(O.hostSkel);
    sig.centerV = vertex_label(RE.V.get(O.centerV));
    sig.allocNbr = O.allocNbr;
    sort(sig.allocNbr.begin(), sig.allocNbr.end());
    for (RawEID eid : O.corePatchEdges) {
        sig.corePatchEdges.push_back(core_patch_edge_label(RE, eid));
    }
    sort(sig.corePatchEdges.begin(), sig.corePatchEdges.end());
    return sig;
}

void ref_retire_skeleton_contents(ReferenceModel& RE, RawSkelID sid) {
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

void ref_commit_skeleton(ReferenceModel& RE, RawSkelID sid, RawSkeletonBuilder&& B) {
    RawSkeleton& existing = RE.skel.get(sid);
    if (!existing.verts.empty() || !existing.edges.empty()) {
        ref_retire_skeleton_contents(RE, sid);
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
    }
}

u32 ref_copy_old_vertex(BuildCtx& C, const ReferenceModel& RE, RawVID oldv, bool checkRealUnique) {
    const auto it = C.mapV.find(oldv);
    if (it != C.mapV.end()) {
        return it->second;
    }

    const RawVertex& RV = RE.V.get(oldv);
    if (checkRealUnique && RV.kind == RawVertexKind::REAL) {
        if (!C.seenRealOrig.insert(RV.orig).second) {
            throw runtime_error("reference model duplicate real vertex");
        }
    }

    const u32 idx = static_cast<u32>(C.B.V.size());
    C.B.V.push_back(make_builder_vertex(RV.kind, RV.orig, RV.occ));
    C.mapV.emplace(oldv, idx);
    return idx;
}

u32 ref_copy_old_edge(BuildCtx& C, const ReferenceModel& RE, RawEID olde) {
    const auto it = C.mapE.find(olde);
    if (it != C.mapE.end()) {
        return it->second;
    }

    const RawEdge& e = RE.E.get(olde);
    if (C.mapV.count(e.a) == 0U || C.mapV.count(e.b) == 0U) {
        throw runtime_error("reference model missing edge endpoint");
    }

    const u32 a = C.mapV.at(e.a);
    const u32 b = C.mapV.at(e.b);
    if (a == b) {
        throw runtime_error("reference model loop edge");
    }

    const u32 idx = static_cast<u32>(C.B.E.size());
    C.B.E.push_back(make_builder_edge(a, b, e.kind, e.br, e.side));
    C.mapE.emplace(olde, idx);
    return idx;
}

void ref_remap_occ_meta(BuildCtx& C, const ReferenceModel& RE, OccID occ, const vector<RawEID>& oldCoreEdges) {
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

IsolatePrepared ref_prepare_isolate(const ReferenceModel& RE, RawSkelID sid, OccID occ) {
    const RawOccRecord& O = RE.occ.get(occ);
    if (O.hostSkel != sid) {
        throw runtime_error("reference model isolate host mismatch");
    }

    IsolatePrepared out;
    out.occ = occ;
    out.orig = O.orig;
    out.allocNbr = O.allocNbr;
    sort(out.allocNbr.begin(), out.allocNbr.end());
    out.allocNbr.erase(unique(out.allocNbr.begin(), out.allocNbr.end()), out.allocNbr.end());

    unordered_set<u64> seenPort;
    const auto port_key = [](Vertex attach, BridgeRef br, u8 side, u8 kind) -> u64 {
        u64 key = attach;
        key = (key << 32U) ^ br;
        key = (key << 8U) ^ side;
        key = (key << 8U) ^ kind;
        return key;
    };

    for (RawEID eid : RE.V.get(O.centerV).adj) {
        const RawEdge& e = RE.E.get(eid);
        const RawVID other = other_end(e, O.centerV);
        const Vertex attachOrig = RE.V.get(other).orig;
        if (e.kind == RawEdgeKind::REAL_PORT) {
            const u64 key = port_key(attachOrig, 0, 0, 0);
            if (seenPort.insert(key).second) {
                out.ports.push_back(IsoPort{IsoPort::REAL, attachOrig, 0, 0});
            }
        } else if (e.kind == RawEdgeKind::BRIDGE_PORT) {
            const u64 key = port_key(attachOrig, e.br, e.side, 1);
            if (seenPort.insert(key).second) {
                out.ports.push_back(IsoPort{IsoPort::BRIDGE, attachOrig, e.br, e.side});
            }
        } else {
            throw runtime_error("reference model invalid isolate port edge");
        }
    }

    unordered_map<Vertex, LVertex> localOfOrig;
    const auto ensure_local = [&](Vertex orig) -> LVertex {
        const auto it = localOfOrig.find(orig);
        if (it != localOfOrig.end()) {
            return it->second;
        }
        const LVertex next = static_cast<LVertex>(out.core.orig.size());
        out.core.orig.push_back(orig);
        localOfOrig.emplace(orig, next);
        return next;
    };

    for (RawEID eid : O.corePatchEdges) {
        const RawEdge& e = RE.E.get(eid);
        if (e.kind != RawEdgeKind::CORE_REAL) {
            throw runtime_error("reference model invalid isolate core edge");
        }
        const Vertex a = RE.V.get(e.a).orig;
        const Vertex b = RE.V.get(e.b).orig;
        if (a == b) {
            continue;
        }
        out.core.edges.push_back(TinyEdge{ensure_local(a), ensure_local(b)});
    }

    return out;
}

SplitPrepared ref_prepare_split(const ReferenceModel& RE, RawSkelID sid, Vertex saOrig, Vertex sbOrig) {
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
            compVerts[static_cast<size_t>(cid)].push_back(u);
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
        for (RawVID v : side.innerVerts) {
            const RawVertex& RV = RE.V.get(v);
            if (RV.kind == RawVertexKind::OCC_CENTER) {
                side.hostedOcc.push_back(RV.occ);
            }
        }
        sort(side.hostedOcc.begin(), side.hostedOcc.end());
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
                throw runtime_error("reference model split cross-component edge");
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

        const bool touchesA = (aSep ? e.a == aV : e.b == aV);
        if (touchesA) {
            out.side[static_cast<size_t>(cid)].useA = true;
        } else {
            out.side[static_cast<size_t>(cid)].useB = true;
        }
    }

    for (int i = 0; i < static_cast<int>(out.side.size()); ++i) {
        if (i == directABSide) {
            continue;
        }
        const SepSidePrepared& side = out.side[static_cast<size_t>(i)];
        if ((!side.innerVerts.empty() || !side.hostedOcc.empty()) && (!side.useA || !side.useB)) {
            throw runtime_error("reference model invalid split side");
        }
    }

    return out;
}

RawSkeletonBuilder ref_build_child_after_split(const ReferenceModel& RE, const SplitPrepared& prep, u32 sideIdx) {
    const SepSidePrepared& side = prep.side[sideIdx];
    BuildCtx C;

    if (side.useA) {
        ref_copy_old_vertex(C, RE, prep.aV, true);
    }
    if (side.useB) {
        ref_copy_old_vertex(C, RE, prep.bV, true);
    }
    for (RawVID oldv : side.innerVerts) {
        ref_copy_old_vertex(C, RE, oldv, true);
    }
    for (RawEID olde : side.innerEdges) {
        ref_copy_old_edge(C, RE, olde);
    }
    for (RawEID olde : side.boundaryEdges) {
        ref_copy_old_edge(C, RE, olde);
    }
    for (OccID occ : side.hostedOcc) {
        ref_remap_occ_meta(C, RE, occ, RE.occ.get(occ).corePatchEdges);
    }
    return move(C.B);
}

SplitSeparationPairResult ref_split(ReferenceModel& RE, RawSkelID sid, Vertex saOrig, Vertex sbOrig) {
    SplitPrepared prep = ref_prepare_split(RE, sid, saOrig, sbOrig);

    vector<RawSkeletonBuilder> builders;
    builders.reserve(prep.side.size());
    for (u32 i = 0; i < static_cast<u32>(prep.side.size()); ++i) {
        builders.push_back(ref_build_child_after_split(RE, prep, i));
    }

    SplitSeparationPairResult res;
    res.aOrig = saOrig;
    res.bOrig = sbOrig;
    res.reusedSid = sid;
    res.child.resize(builders.size());

    if (builders.empty()) {
        ref_retire_skeleton_contents(RE, sid);
        return res;
    }

    const auto boundary_only = [&](const SepSidePrepared& side) {
        return side.innerVerts.empty() && side.hostedOcc.empty();
    };

    res.child[0].sid = sid;
    res.child[0].hostedOcc = prep.side[0].hostedOcc;
    res.child[0].boundaryOnly = boundary_only(prep.side[0]);
    ref_commit_skeleton(RE, sid, move(builders[0]));

    for (u32 i = 1; i < static_cast<u32>(builders.size()); ++i) {
        const RawSkelID nsid = RE.skel.alloc(RawSkeleton{});
        res.child[i].sid = nsid;
        res.child[i].hostedOcc = prep.side[i].hostedOcc;
        res.child[i].boundaryOnly = boundary_only(prep.side[i]);
        ref_commit_skeleton(RE, nsid, move(builders[i]));
    }

    return res;
}

RawSkeletonBuilder ref_build_join(const ReferenceModel& RE, RawSkelID leftSid, RawSkelID rightSid, Vertex saOrig, Vertex sbOrig) {
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
            ref_copy_old_vertex(C, RE, v, true);
        }
    }
    for (RawVID v : R.verts) {
        if (v != rA && v != rB) {
            ref_copy_old_vertex(C, RE, v, true);
        }
    }

    for (RawEID eid : L.edges) {
        ref_copy_old_edge(C, RE, eid);
    }
    for (RawEID eid : R.edges) {
        ref_copy_old_edge(C, RE, eid);
    }

    unordered_set<OccID> seenOcc;
    const auto copy_occ = [&](const RawSkeleton& S) {
        for (OccID occ : S.hostedOcc) {
            if (!seenOcc.insert(occ).second) {
                throw runtime_error("reference model duplicate join occurrence");
            }
            ref_remap_occ_meta(C, RE, occ, RE.occ.get(occ).corePatchEdges);
        }
    };
    copy_occ(L);
    copy_occ(R);
    return move(C.B);
}

JoinSeparationPairResult ref_join(
    ReferenceModel& RE,
    RawSkelID leftSid,
    RawSkelID rightSid,
    Vertex saOrig,
    Vertex sbOrig
) {
    RawSkeletonBuilder B = ref_build_join(RE, leftSid, rightSid, saOrig, sbOrig);
    ref_commit_skeleton(RE, leftSid, move(B));
    ref_retire_skeleton_contents(RE, rightSid);
    RE.skel.retire(rightSid);
    return JoinSeparationPairResult{leftSid, rightSid};
}

RawSkeletonBuilder ref_build_integrate(
    const ReferenceModel& RE,
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

    for (const BoundaryMapEntry& entry : bm) {
        if (!seenParentBoundary.insert(entry.parentOrig).second ||
            !seenChildBoundary.insert(entry.childOrig).second) {
            throw runtime_error("reference model duplicate integrate boundary");
        }
        parentBoundaryRaw.emplace(entry.parentOrig, find_real_orig_in_skeleton(RE, P, entry.parentOrig));
        childBoundaryRaw.emplace(entry.childOrig, find_real_orig_in_skeleton(RE, Cc, entry.childOrig));
    }

    for (RawVID v : P.verts) {
        ref_copy_old_vertex(C, RE, v, true);
    }
    for (const BoundaryMapEntry& entry : bm) {
        const RawVID childV = childBoundaryRaw.at(entry.childOrig);
        const RawVID parentV = parentBoundaryRaw.at(entry.parentOrig);
        C.mapV.emplace(childV, C.mapV.at(parentV));
    }

    const auto is_child_boundary_real = [&](RawVID oldv) {
        const RawVertex& RV = RE.V.get(oldv);
        if (RV.kind != RawVertexKind::REAL) {
            return false;
        }
        const auto it = childBoundaryRaw.find(RV.orig);
        return it != childBoundaryRaw.end() && it->second == oldv;
    };

    for (RawVID v : Cc.verts) {
        if (!is_child_boundary_real(v)) {
            ref_copy_old_vertex(C, RE, v, true);
        }
    }
    for (RawEID eid : P.edges) {
        ref_copy_old_edge(C, RE, eid);
    }
    for (RawEID eid : Cc.edges) {
        ref_copy_old_edge(C, RE, eid);
    }

    unordered_set<OccID> seenOcc;
    const auto copy_occ = [&](const RawSkeleton& S) {
        for (OccID occ : S.hostedOcc) {
            if (!seenOcc.insert(occ).second) {
                throw runtime_error("reference model duplicate integrate occurrence");
            }
            ref_remap_occ_meta(C, RE, occ, RE.occ.get(occ).corePatchEdges);
        }
    };
    copy_occ(P);
    copy_occ(Cc);
    return move(C.B);
}

IntegrateResult ref_integrate(
    ReferenceModel& RE,
    RawSkelID parentSid,
    RawSkelID childSid,
    const vector<BoundaryMapEntry>& bm
) {
    RawSkeletonBuilder B = ref_build_integrate(RE, parentSid, childSid, bm);
    ref_commit_skeleton(RE, parentSid, move(B));
    ref_retire_skeleton_contents(RE, childSid);
    RE.skel.retire(childSid);
    return IntegrateResult{parentSid, childSid};
}

SplitResultSignature normalize_split_signature(SplitResultSignature sig) {
    sort(sig.child.begin(), sig.child.end());
    return sig;
}

MergeResultSignature normalize_merge_signature(MergeResultSignature sig) {
    sort(sig.hostedOccurrence.begin(), sig.hostedOccurrence.end());
    return sig;
}

bool fail_signature_compare(const string& title, const string& expected, const string& actual, string* failure) {
    if (failure != nullptr) {
        *failure = title + "\nexpected:\n" + expected + "\nactual:\n" + actual;
    }
    return false;
}

} // namespace

bool PortSignature::operator==(const PortSignature& rhs) const {
    return tie(kind, attachOrig, br, side) == tie(rhs.kind, rhs.attachOrig, rhs.br, rhs.side);
}

bool PortSignature::operator<(const PortSignature& rhs) const {
    return tie(kind, attachOrig, br, side) < tie(rhs.kind, rhs.attachOrig, rhs.br, rhs.side);
}

bool TinyGraphSignature::operator==(const TinyGraphSignature& rhs) const {
    return verts == rhs.verts && edges == rhs.edges;
}

bool IsolatePreparedSignature::operator==(const IsolatePreparedSignature& rhs) const {
    return orig == rhs.orig && allocNbr == rhs.allocNbr && ports == rhs.ports && core == rhs.core;
}

bool CanonicalSkeletonSignature::operator==(const CanonicalSkeletonSignature& rhs) const {
    return vertices == rhs.vertices && edges == rhs.edges && hostedOcc == rhs.hostedOcc;
}

bool CanonicalSkeletonSignature::operator<(const CanonicalSkeletonSignature& rhs) const {
    return tie(vertices, edges, hostedOcc) < tie(rhs.vertices, rhs.edges, rhs.hostedOcc);
}

bool CanonicalOccurrenceSignature::operator==(const CanonicalOccurrenceSignature& rhs) const {
    return tie(occ, orig, hostSkel, centerV, allocNbr, corePatchEdges) ==
           tie(rhs.occ, rhs.orig, rhs.hostSkel, rhs.centerV, rhs.allocNbr, rhs.corePatchEdges);
}

bool CanonicalOccurrenceSignature::operator<(const CanonicalOccurrenceSignature& rhs) const {
    return tie(occ, orig, hostSkel, centerV, allocNbr, corePatchEdges) <
           tie(rhs.occ, rhs.orig, rhs.hostSkel, rhs.centerV, rhs.allocNbr, rhs.corePatchEdges);
}

bool EngineStateSignature::operator==(const EngineStateSignature& rhs) const {
    return skeletons == rhs.skeletons && occurrences == rhs.occurrences;
}

bool SplitChildSignature::operator==(const SplitChildSignature& rhs) const {
    return tie(boundaryOnly, hostedOcc, graph) == tie(rhs.boundaryOnly, rhs.hostedOcc, rhs.graph);
}

bool SplitChildSignature::operator<(const SplitChildSignature& rhs) const {
    return tie(boundaryOnly, hostedOcc, graph) < tie(rhs.boundaryOnly, rhs.hostedOcc, rhs.graph);
}

bool SplitResultSignature::operator==(const SplitResultSignature& rhs) const {
    return aOrig == rhs.aOrig && bOrig == rhs.bOrig && child == rhs.child;
}

bool MergeResultSignature::operator==(const MergeResultSignature& rhs) const {
    return mergedSkeleton == rhs.mergedSkeleton && hostedOccurrence == rhs.hostedOccurrence;
}

IsolatePreparedSignature capture_isolate_signature(const IsolatePrepared& prep) {
    IsolatePreparedSignature sig;
    sig.orig = prep.orig;
    sig.allocNbr = prep.allocNbr;
    sort(sig.allocNbr.begin(), sig.allocNbr.end());

    for (const IsoPort& port : prep.ports) {
        sig.ports.push_back(PortSignature{
            static_cast<int>(port.kind),
            port.attachOrig,
            port.br,
            port.side,
        });
    }
    sort(sig.ports.begin(), sig.ports.end());

    sig.core.verts = prep.core.orig;
    sort(sig.core.verts.begin(), sig.core.verts.end());

    for (const TinyEdge& e : prep.core.edges) {
        Vertex a = prep.core.orig[e.a];
        Vertex b = prep.core.orig[e.b];
        if (a > b) {
            swap(a, b);
        }
        sig.core.edges.push_back({a, b});
    }
    sort(sig.core.edges.begin(), sig.core.edges.end());
    return sig;
}

SplitResultSignature capture_split_result_signature(const RawEngine& RE, const SplitSeparationPairResult& result) {
    SplitResultSignature sig;
    sig.aOrig = result.aOrig;
    sig.bOrig = result.bOrig;
    for (const SplitChildInfo& child : result.child) {
        SplitChildSignature childSig;
        childSig.boundaryOnly = child.boundaryOnly;
        childSig.hostedOcc = child.hostedOcc;
        sort(childSig.hostedOcc.begin(), childSig.hostedOcc.end());
        childSig.graph = capture_skeleton_signature(RE, child.sid);
        sig.child.push_back(move(childSig));
    }
    return normalize_split_signature(move(sig));
}

MergeResultSignature capture_join_result_signature(const RawEngine& RE, const JoinSeparationPairResult& result) {
    MergeResultSignature sig;
    sig.mergedSkeleton = capture_skeleton_signature(RE, result.mergedSid);

    unordered_map<RawSkelID, string> hostSig;
    hostSig.emplace(result.mergedSid, describe_skeleton_signature(sig.mergedSkeleton));
    for (OccID occ : RE.skel.get(result.mergedSid).hostedOcc) {
        sig.hostedOccurrence.push_back(capture_occurrence_signature(RE, occ, hostSig));
    }
    return normalize_merge_signature(move(sig));
}

MergeResultSignature capture_integrate_result_signature(const RawEngine& RE, const IntegrateResult& result) {
    MergeResultSignature sig;
    sig.mergedSkeleton = capture_skeleton_signature(RE, result.mergedSid);

    unordered_map<RawSkelID, string> hostSig;
    hostSig.emplace(result.mergedSid, describe_skeleton_signature(sig.mergedSkeleton));
    for (OccID occ : RE.skel.get(result.mergedSid).hostedOcc) {
        sig.hostedOccurrence.push_back(capture_occurrence_signature(RE, occ, hostSig));
    }
    return normalize_merge_signature(move(sig));
}

EngineStateSignature capture_engine_state_signature(const RawEngine& RE) {
    EngineStateSignature sig;
    unordered_map<RawSkelID, string> hostSig;

    for (u32 sid = 0; sid < RE.skel.a.size(); ++sid) {
        if (!RE.skel.a[sid].alive) {
            continue;
        }
        CanonicalSkeletonSignature skeletonSig = capture_skeleton_signature(RE, sid);
        hostSig.emplace(sid, describe_skeleton_signature(skeletonSig));
        sig.skeletons.push_back(move(skeletonSig));
    }
    sort(sig.skeletons.begin(), sig.skeletons.end());

    for (u32 occ = 0; occ < RE.occ.a.size(); ++occ) {
        if (!RE.occ.a[occ].alive) {
            continue;
        }
        sig.occurrences.push_back(capture_occurrence_signature(RE, occ, hostSig));
    }
    sort(sig.occurrences.begin(), sig.occurrences.end());
    return sig;
}

string describe_signature(const IsolatePreparedSignature& sig) {
    vector<string> portDesc;
    for (const PortSignature& port : sig.ports) {
        portDesc.push_back(
            to_string(port.kind) + ":" + to_string(port.attachOrig) + ":" +
            to_string(port.br) + ":" + to_string(static_cast<int>(port.side))
        );
    }
    vector<string> edgeDesc;
    for (const auto& edge : sig.core.edges) {
        edgeDesc.push_back(to_string(edge.first) + "-" + to_string(edge.second));
    }

    ostringstream oss;
    oss << "orig=" << sig.orig
        << "\nallocNbr=[" << join_values(sig.allocNbr, ",") << "]"
        << "\nports=[" << join_strings(portDesc, ",") << "]"
        << "\ncoreVerts=[" << join_values(sig.core.verts, ",") << "]"
        << "\ncoreEdges=[" << join_strings(edgeDesc, ",") << "]";
    return oss.str();
}

string describe_signature(const SplitResultSignature& sig) {
    vector<string> childDesc;
    for (const SplitChildSignature& child : sig.child) {
        ostringstream oss;
        oss << "{boundaryOnly=" << (child.boundaryOnly ? 1 : 0)
            << ",hosted=[" << join_values(child.hostedOcc, ",") << "]"
            << ",graph=" << describe_signature(EngineStateSignature{{child.graph}, {}}) << "}";
        childDesc.push_back(oss.str());
    }

    ostringstream oss;
    oss << "sep=" << sig.aOrig << "," << sig.bOrig
        << "\nchildCount=" << sig.child.size()
        << "\nchildren=[" << join_strings(childDesc, ",") << "]";
    return oss.str();
}

string describe_signature(const MergeResultSignature& sig) {
    vector<string> occDesc;
    for (const CanonicalOccurrenceSignature& occ : sig.hostedOccurrence) {
        occDesc.push_back(
            "{occ=" + to_string(occ.occ) +
            ",orig=" + to_string(occ.orig) +
            ",host=" + occ.hostSkel +
            ",center=" + occ.centerV +
            ",allocNbr=[" + join_values(occ.allocNbr, ",") + "]" +
            ",core=[" + join_strings(occ.corePatchEdges, ",") + "]}"
        );
    }

    ostringstream oss;
    oss << "merged=" << describe_signature(EngineStateSignature{{sig.mergedSkeleton}, {}})
        << "\noccurrence=[" << join_strings(occDesc, ",") << "]";
    return oss.str();
}

string describe_signature(const EngineStateSignature& sig) {
    vector<string> skeletonDesc;
    for (const CanonicalSkeletonSignature& skeleton : sig.skeletons) {
        skeletonDesc.push_back(describe_skeleton_signature(skeleton));
    }

    vector<string> occDesc;
    for (const CanonicalOccurrenceSignature& occ : sig.occurrences) {
        occDesc.push_back(
            "{occ=" + to_string(occ.occ) +
            ",orig=" + to_string(occ.orig) +
            ",host=" + occ.hostSkel +
            ",center=" + occ.centerV +
            ",allocNbr=[" + join_values(occ.allocNbr, ",") + "]" +
            ",core=[" + join_strings(occ.corePatchEdges, ",") + "]}"
        );
    }

    ostringstream oss;
    oss << "{skeletons=[" << join_strings(skeletonDesc, ",")
        << "],occurrences=[" << join_strings(occDesc, ",") << "]}";
    return oss.str();
}

bool check_prepare_isolate_oracle(
    const RawEngine& before,
    RawSkelID sid,
    OccID occ,
    const IsolatePrepared& actual,
    string* failure
) {
    const IsolatePreparedSignature actualSig = capture_isolate_signature(actual);
    const IsolatePreparedSignature expectedSig = capture_isolate_signature(ref_prepare_isolate(before, sid, occ));
    if (expectedSig == actualSig) {
        return true;
    }
    return fail_signature_compare(
        "prepare_isolate_neighborhood oracle mismatch",
        describe_signature(expectedSig),
        describe_signature(actualSig),
        failure
    );
}

bool check_split_oracle(
    const RawEngine& before,
    const RawEngine& after,
    const PrimitiveInvocation& invocation,
    const SplitSeparationPairResult& actual,
    string* failure
) {
    ReferenceModel expected = before;
    const SplitSeparationPairResult refResult =
        ref_split(expected, invocation.sid, invocation.aOrig, invocation.bOrig);

    const SplitResultSignature expectedResult = capture_split_result_signature(expected, refResult);
    const SplitResultSignature actualResult = capture_split_result_signature(after, actual);
    if (!(expectedResult == actualResult)) {
        return fail_signature_compare(
            "split_separation_pair result mismatch",
            describe_signature(expectedResult),
            describe_signature(actualResult),
            failure
        );
    }

    const EngineStateSignature expectedState = capture_engine_state_signature(expected);
    const EngineStateSignature actualState = capture_engine_state_signature(after);
    if (!(expectedState == actualState)) {
        return fail_signature_compare(
            "split_separation_pair state mismatch",
            describe_signature(expectedState),
            describe_signature(actualState),
            failure
        );
    }
    return true;
}

bool check_join_oracle(
    const RawEngine& before,
    const RawEngine& after,
    const PrimitiveInvocation& invocation,
    const JoinSeparationPairResult& actual,
    string* failure
) {
    ReferenceModel expected = before;
    const JoinSeparationPairResult refResult = ref_join(
        expected,
        invocation.leftSid,
        invocation.rightSid,
        invocation.aOrig,
        invocation.bOrig
    );

    const MergeResultSignature expectedResult = capture_join_result_signature(expected, refResult);
    const MergeResultSignature actualResult = capture_join_result_signature(after, actual);
    if (!(expectedResult == actualResult)) {
        return fail_signature_compare(
            "join_separation_pair result mismatch",
            describe_signature(expectedResult),
            describe_signature(actualResult),
            failure
        );
    }

    const EngineStateSignature expectedState = capture_engine_state_signature(expected);
    const EngineStateSignature actualState = capture_engine_state_signature(after);
    if (!(expectedState == actualState)) {
        return fail_signature_compare(
            "join_separation_pair state mismatch",
            describe_signature(expectedState),
            describe_signature(actualState),
            failure
        );
    }
    return true;
}

bool check_integrate_oracle(
    const RawEngine& before,
    const RawEngine& after,
    const PrimitiveInvocation& invocation,
    const IntegrateResult& actual,
    string* failure
) {
    ReferenceModel expected = before;
    const IntegrateResult refResult = ref_integrate(
        expected,
        invocation.parentSid,
        invocation.childSid,
        invocation.boundaryMap
    );

    const MergeResultSignature expectedResult = capture_integrate_result_signature(expected, refResult);
    const MergeResultSignature actualResult = capture_integrate_result_signature(after, actual);
    if (!(expectedResult == actualResult)) {
        return fail_signature_compare(
            "integrate_skeleton result mismatch",
            describe_signature(expectedResult),
            describe_signature(actualResult),
            failure
        );
    }

    const EngineStateSignature expectedState = capture_engine_state_signature(expected);
    const EngineStateSignature actualState = capture_engine_state_signature(after);
    if (!(expectedState == actualState)) {
        return fail_signature_compare(
            "integrate_skeleton state mismatch",
            describe_signature(expectedState),
            describe_signature(actualState),
            failure
        );
    }
    return true;
}
