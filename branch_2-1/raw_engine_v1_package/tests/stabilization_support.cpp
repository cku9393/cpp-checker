#include "stabilization_support.hpp"

#include <algorithm>
#include <deque>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#include "failure_signature.hpp"
#include "reference_model.hpp"
#include "reference_planner.hpp"

using namespace std;

namespace {

RawSkelID find_alternate_skeleton(const RawEngine& RE, RawSkelID currentSid) {
    for (u32 sid = 0; sid < RE.skel.a.size(); ++sid) {
        if (RE.skel.a[sid].alive && sid != currentSid) {
            return sid;
        }
    }
    return NIL_U32;
}

RawVID find_alternate_vertex(const RawEngine& RE, RawSkelID sid, RawVID currentV) {
    const RawSkeleton& S = RE.skel.get(sid);
    for (RawVID v : S.verts) {
        if (v != currentV) {
            return v;
        }
    }
    return currentV;
}

RawEID find_center_incident_edge(const RawEngine& RE, OccID occ) {
    const RawVID centerV = RE.occ.get(occ).centerV;
    for (RawEID eid : RE.V.get(centerV).adj) {
        return eid;
    }
    return NIL_U32;
}

} // namespace

string primitive_fault_name_string(PrimitiveFaultKind fault) {
    switch (fault) {
        case PrimitiveFaultKind::NONE:
            return "none";
        case PrimitiveFaultKind::DROP_ALLOC_NBR:
            return "drop_alloc_nbr";
        case PrimitiveFaultKind::DUPLICATE_ALLOC_NBR:
            return "duplicate_alloc_nbr";
        case PrimitiveFaultKind::WRONG_HOST_SKEL:
            return "wrong_host_skel";
        case PrimitiveFaultKind::WRONG_CENTER_V:
            return "wrong_center_v";
        case PrimitiveFaultKind::DROP_CORE_PATCH_EDGE:
            return "drop_core_patch_edge";
        case PrimitiveFaultKind::ADD_CENTER_INCIDENT_CORE_EDGE:
            return "add_center_incident_core_edge";
        case PrimitiveFaultKind::DROP_SKELETON_EDGE:
            return "drop_skeleton_edge";
        case PrimitiveFaultKind::REWIRE_SKELETON_EDGE:
            return "rewire_skeleton_edge";
        case PrimitiveFaultKind::FLIP_BOUNDARY_ONLY:
            return "flip_boundary_only";
    }
    return "unknown";
}

string planner_fault_name_string(PlannerFaultKind fault) {
    switch (fault) {
        case PlannerFaultKind::NONE:
            return "none";
        case PlannerFaultKind::OMIT_AFTER_SPLIT_JOIN:
            return "omit_after_split_join";
        case PlannerFaultKind::OMIT_AFTER_SPLIT_INTEGRATE:
            return "omit_after_split_integrate";
        case PlannerFaultKind::OMIT_ENSURE_SOLE_AFTER_JOIN:
            return "omit_ensure_sole_after_join";
        case PlannerFaultKind::OMIT_ENSURE_SOLE_AFTER_INTEGRATE:
            return "omit_ensure_sole_after_integrate";
        case PlannerFaultKind::WRONG_BRANCH_ROUTING:
            return "wrong_branch_routing";
    }
    return "unknown";
}

bool validate_engine_state_soft(const RawEngine& RE, string* error) {
    for (u32 sid = 0; sid < RE.skel.a.size(); ++sid) {
        if (!RE.skel.a[sid].alive) {
            continue;
        }
        if (!validate_skeleton_wellformed(RE, sid, error)) {
            return false;
        }
    }
    for (u32 occ = 0; occ < RE.occ.a.size(); ++occ) {
        if (!RE.occ.a[occ].alive) {
            continue;
        }
        if (!validate_occ_patch_consistent(RE, occ, error)) {
            return false;
        }
    }
    return true;
}

void inject_primitive_fault(RawEngine& RE, PrimitiveFaultKind fault, OccID targetOcc) {
    RawOccRecord& O = RE.occ.get(targetOcc);
    RawSkeleton& S = RE.skel.get(O.hostSkel);

    switch (fault) {
        case PrimitiveFaultKind::NONE:
            return;
        case PrimitiveFaultKind::DROP_ALLOC_NBR:
            if (!O.allocNbr.empty()) {
                O.allocNbr.erase(O.allocNbr.begin());
            }
            return;
        case PrimitiveFaultKind::DUPLICATE_ALLOC_NBR:
            if (!O.allocNbr.empty()) {
                O.allocNbr.push_back(O.allocNbr.front());
            } else {
                const Vertex orig = O.orig;
                const auto it = RE.occOfOrig.find(orig);
                if (it != RE.occOfOrig.end()) {
                    for (OccID other : it->second) {
                        if (other != targetOcc) {
                            O.allocNbr.push_back(other);
                            O.allocNbr.push_back(other);
                            return;
                        }
                    }
                }
            }
            return;
        case PrimitiveFaultKind::WRONG_HOST_SKEL: {
            const RawSkelID otherSid = find_alternate_skeleton(RE, O.hostSkel);
            O.hostSkel = otherSid;
            return;
        }
        case PrimitiveFaultKind::WRONG_CENTER_V:
            O.centerV = find_alternate_vertex(RE, O.hostSkel, O.centerV);
            return;
        case PrimitiveFaultKind::DROP_CORE_PATCH_EDGE:
            if (!O.corePatchEdges.empty()) {
                O.corePatchEdges.erase(O.corePatchEdges.begin());
            }
            return;
        case PrimitiveFaultKind::ADD_CENTER_INCIDENT_CORE_EDGE: {
            const RawEID eid = find_center_incident_edge(RE, targetOcc);
            if (eid != NIL_U32) {
                O.corePatchEdges.push_back(eid);
            }
            return;
        }
        case PrimitiveFaultKind::DROP_SKELETON_EDGE:
            if (!S.edges.empty()) {
                S.edges.erase(S.edges.begin());
            }
            return;
        case PrimitiveFaultKind::REWIRE_SKELETON_EDGE:
            if (!S.edges.empty()) {
                RawEdge& edge = RE.E.get(S.edges.front());
                edge.b = edge.a;
            }
            return;
        case PrimitiveFaultKind::FLIP_BOUNDARY_ONLY:
            throw runtime_error("flip_boundary_only must be applied to split result, not engine state");
    }
}
