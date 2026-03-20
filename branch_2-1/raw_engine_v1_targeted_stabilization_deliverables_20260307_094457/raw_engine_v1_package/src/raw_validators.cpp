#include "raw_engine/raw_engine.hpp"

using namespace std;

void assert_builder_basic(const RawSkeletonBuilder& B) {
    for (const auto& e : B.E) {
        if (e.a >= B.V.size() || e.b >= B.V.size() || e.a == e.b) {
            assert(false);
        }
    }

    unordered_set<OccID> centerOcc;
    for (const auto& v : B.V) {
        if (v.kind == RawVertexKind::OCC_CENTER) {
            if (!centerOcc.insert(v.occ).second) {
                assert(false);
            }
        }
    }

    for (const auto& entry : B.allocNbr) {
        if (centerOcc.count(entry.first) == 0U) {
            assert(false);
        }
    }

    for (const auto& entry : B.corePatchLocalEids) {
        if (centerOcc.count(entry.first) == 0U) {
            assert(false);
        }
        for (u32 eid : entry.second) {
            if (eid >= B.E.size()) {
                assert(false);
            }
            const RawSkeletonBuilder::BE& e = B.E[eid];
            if (e.kind != RawEdgeKind::CORE_REAL ||
                B.V[e.a].kind != RawVertexKind::REAL ||
                B.V[e.b].kind != RawVertexKind::REAL) {
                assert(false);
            }
        }
    }
}

void assert_skeleton_wellformed(const RawEngine& RE, RawSkelID sid) {
    const RawSkeleton& S = RE.skel.get(sid);

    const unordered_set<RawVID> inV(S.verts.begin(), S.verts.end());
    const unordered_set<RawEID> inE(S.edges.begin(), S.edges.end());

    for (RawVID v : S.verts) {
        const RawVertex& RV = RE.V.get(v);
        for (RawEID eid : RV.adj) {
            const RawEdge& e = RE.E.get(eid);
            if (inE.count(eid) == 0U || (e.a != v && e.b != v)) {
                assert(false);
            }
        }
    }

    for (RawEID eid : S.edges) {
        const RawEdge& e = RE.E.get(eid);
        if (inV.count(e.a) == 0U || inV.count(e.b) == 0U || e.a == e.b) {
            assert(false);
        }
    }

    unordered_set<OccID> seenOcc;
    for (OccID occ : S.hostedOcc) {
        if (!seenOcc.insert(occ).second) {
            assert(false);
        }
        const RawOccRecord& O = RE.occ.get(occ);
        if (O.hostSkel != sid ||
            inV.count(O.centerV) == 0U ||
            RE.V.get(O.centerV).kind != RawVertexKind::OCC_CENTER ||
            RE.V.get(O.centerV).occ != occ) {
            assert(false);
        }

        for (RawEID eid : O.corePatchEdges) {
            if (inE.count(eid) == 0U || RE.E.get(eid).kind != RawEdgeKind::CORE_REAL) {
                assert(false);
            }
        }
    }
}

void assert_occ_patch_consistent(const RawEngine& RE, OccID occ) {
    const RawOccRecord& O = RE.occ.get(occ);
    const RawSkeleton& S = RE.skel.get(O.hostSkel);
    const unordered_set<RawVID> inV(S.verts.begin(), S.verts.end());
    const unordered_set<RawEID> inE(S.edges.begin(), S.edges.end());

    const RawVID c = O.centerV;
    if (inV.count(c) == 0U ||
        RE.V.get(c).kind != RawVertexKind::OCC_CENTER ||
        RE.V.get(c).occ != occ) {
        assert(false);
    }

    for (RawEID eid : RE.V.get(c).adj) {
        const RawEdge& e = RE.E.get(eid);
        if (inE.count(eid) == 0U ||
            (e.kind != RawEdgeKind::REAL_PORT && e.kind != RawEdgeKind::BRIDGE_PORT)) {
            assert(false);
        }
    }

    unordered_set<OccID> seenNbr;
    for (OccID nbr : O.allocNbr) {
        if (nbr == occ || !seenNbr.insert(nbr).second || RE.occ.get(nbr).orig != O.orig) {
            assert(false);
        }
    }

    for (RawEID eid : O.corePatchEdges) {
        const RawEdge& e = RE.E.get(eid);
        if (inE.count(eid) == 0U ||
            e.kind != RawEdgeKind::CORE_REAL ||
            inV.count(e.a) == 0U ||
            inV.count(e.b) == 0U ||
            RE.V.get(e.a).kind != RawVertexKind::REAL ||
            RE.V.get(e.b).kind != RawVertexKind::REAL ||
            e.a == c ||
            e.b == c) {
            assert(false);
        }
    }
}

void debug_validate_skeleton_and_hosted(const RawEngine& RE, RawSkelID sid) {
#ifndef NDEBUG
    if (sid == NIL_U32) {
        return;
    }

    const RawSkeleton& S = RE.skel.get(sid);
    assert_skeleton_wellformed(RE, sid);
    for (OccID occ : S.hostedOcc) {
        assert_occ_patch_consistent(RE, occ);
    }
#else
    (void)RE;
    (void)sid;
#endif
}
