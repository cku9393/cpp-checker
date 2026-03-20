#include "raw_engine/raw_engine.hpp"

using namespace std;

namespace {

bool fail_validation(string* error, const string& detail) {
    if (error != nullptr) {
        *error = detail;
    }
    return false;
}

} // namespace

bool validate_builder_basic(const RawSkeletonBuilder& B, string* error) {
    for (const auto& e : B.E) {
        if (e.a >= B.V.size() || e.b >= B.V.size() || e.a == e.b) {
            return fail_validation(error, "validator builder edge endpoint mismatch");
        }
    }

    unordered_set<OccID> centerOcc;
    for (const auto& v : B.V) {
        if (v.kind == RawVertexKind::OCC_CENTER) {
            if (!centerOcc.insert(v.occ).second) {
                return fail_validation(error, "validator duplicate occurrence center in builder");
            }
        }
    }

    for (const auto& entry : B.allocNbr) {
        if (centerOcc.count(entry.first) == 0U) {
            return fail_validation(error, "validator builder allocNbr references missing occurrence center");
        }
    }

    for (const auto& entry : B.corePatchLocalEids) {
        if (centerOcc.count(entry.first) == 0U) {
            return fail_validation(error, "validator builder corePatch references missing occurrence center");
        }
        for (u32 eid : entry.second) {
            if (eid >= B.E.size()) {
                return fail_validation(error, "validator builder corePatch edge index out of range");
            }
            const RawSkeletonBuilder::BE& e = B.E[eid];
            if (e.kind != RawEdgeKind::CORE_REAL ||
                B.V[e.a].kind != RawVertexKind::REAL ||
                B.V[e.b].kind != RawVertexKind::REAL) {
                return fail_validation(error, "validator builder corePatch edge is not REAL-REAL CORE edge");
            }
        }
    }
    return true;
}

void assert_builder_basic(const RawSkeletonBuilder& B) {
    (void)B;
    assert(validate_builder_basic(B, nullptr));
}

bool validate_skeleton_wellformed(const RawEngine& RE, RawSkelID sid, string* error) {
    const RawSkeleton& S = RE.skel.get(sid);

    const unordered_set<RawVID> inV(S.verts.begin(), S.verts.end());
    const unordered_set<RawEID> inE(S.edges.begin(), S.edges.end());

    for (RawVID v : S.verts) {
        const RawVertex& RV = RE.V.get(v);
        for (RawEID eid : RV.adj) {
            const RawEdge& e = RE.E.get(eid);
            if (inE.count(eid) == 0U || (e.a != v && e.b != v)) {
                return fail_validation(error, "validator skeleton adjacency contains foreign or malformed edge");
            }
        }
    }

    for (RawEID eid : S.edges) {
        const RawEdge& e = RE.E.get(eid);
        if (inV.count(e.a) == 0U || inV.count(e.b) == 0U || e.a == e.b) {
            return fail_validation(error, "validator skeleton edge endpoint mismatch");
        }
    }

    unordered_set<OccID> seenOcc;
    for (OccID occ : S.hostedOcc) {
        if (!seenOcc.insert(occ).second) {
            return fail_validation(error, "validator skeleton duplicate hosted occurrence");
        }
        const RawOccRecord& O = RE.occ.get(occ);
        if (O.hostSkel != sid ||
            inV.count(O.centerV) == 0U ||
            RE.V.get(O.centerV).kind != RawVertexKind::OCC_CENTER ||
            RE.V.get(O.centerV).occ != occ) {
            return fail_validation(error, "validator skeleton host skeleton or center vertex mismatch");
        }

        for (RawEID eid : O.corePatchEdges) {
            if (inE.count(eid) == 0U || RE.E.get(eid).kind != RawEdgeKind::CORE_REAL) {
                return fail_validation(error, "validator skeleton corePatch edge not owned by host skeleton");
            }
        }
    }
    return true;
}

void assert_skeleton_wellformed(const RawEngine& RE, RawSkelID sid) {
    (void)RE;
    (void)sid;
    assert(validate_skeleton_wellformed(RE, sid, nullptr));
}

bool validate_occ_patch_consistent(const RawEngine& RE, OccID occ, string* error) {
    const RawOccRecord& O = RE.occ.get(occ);
    const RawSkeleton& S = RE.skel.get(O.hostSkel);
    const unordered_set<RawVID> inV(S.verts.begin(), S.verts.end());
    const unordered_set<RawEID> inE(S.edges.begin(), S.edges.end());

    const RawVID c = O.centerV;
    if (inV.count(c) == 0U ||
        RE.V.get(c).kind != RawVertexKind::OCC_CENTER ||
        RE.V.get(c).occ != occ) {
        return fail_validation(error, "validator occurrence center vertex mismatch");
    }

    for (RawEID eid : RE.V.get(c).adj) {
        const RawEdge& e = RE.E.get(eid);
        if (inE.count(eid) == 0U ||
            (e.kind != RawEdgeKind::REAL_PORT && e.kind != RawEdgeKind::BRIDGE_PORT)) {
            return fail_validation(error, "validator occurrence center incident edge mismatch");
        }
    }

    unordered_set<OccID> seenNbr;
    for (OccID nbr : O.allocNbr) {
        if (nbr == occ || !seenNbr.insert(nbr).second || RE.occ.get(nbr).orig != O.orig) {
            return fail_validation(error, "validator occurrence allocNbr mismatch");
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
            return fail_validation(error, "validator occurrence corePatch mismatch");
        }
    }
    return true;
}

void assert_occ_patch_consistent(const RawEngine& RE, OccID occ) {
    (void)RE;
    (void)occ;
    assert(validate_occ_patch_consistent(RE, occ, nullptr));
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
