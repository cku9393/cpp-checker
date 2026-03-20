#include "exhaustive_generator.hpp"

#include <algorithm>
#include <deque>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>

#include "failure_signature.hpp"
#include "reference_model.hpp"
#include "split_choice_oracle.hpp"

using namespace std;

namespace {

OccID new_occ(RawEngine& RE, Vertex orig) {
    const OccID id = RE.occ.alloc(make_occ_record(orig));
    RE.occOfOrig[orig].push_back(id);
    return id;
}

RawSkelID new_skeleton(RawEngine& RE) {
    return RE.skel.alloc(RawSkeleton{});
}

struct BranchLayout {
    vector<u32> coreLocalEids;
    u32 attachLocalV = NIL_U32;
};

BranchLayout append_core_branch(RawSkeletonBuilder& B, u32 sepA, u32 sepB, Vertex origBase, int pathLen) {
    BranchLayout branch;
    u32 prev = sepA;
    for (int step = 0; step < pathLen; ++step) {
        const u32 localV = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, origBase + static_cast<Vertex>(step), 0));
        if (branch.attachLocalV == NIL_U32) {
            branch.attachLocalV = localV;
        }
        branch.coreLocalEids.push_back(static_cast<u32>(B.E.size()));
        B.E.push_back(make_builder_edge(prev, localV, RawEdgeKind::CORE_REAL));
        prev = localV;
    }
    branch.coreLocalEids.push_back(static_cast<u32>(B.E.size()));
    B.E.push_back(make_builder_edge(prev, sepB, RawEdgeKind::CORE_REAL));
    if (branch.attachLocalV == NIL_U32) {
        branch.attachLocalV = sepA;
    }
    return branch;
}

void add_occ_center_edge(
    RawSkeletonBuilder& B,
    u32 center,
    u32 attach,
    bool bridgePort,
    BridgeRef bridgeRef,
    u8 side
) {
    if (bridgePort) {
        B.E.push_back(make_builder_edge(center, attach, RawEdgeKind::BRIDGE_PORT, bridgeRef, side));
        return;
    }
    B.E.push_back(make_builder_edge(center, attach, RawEdgeKind::REAL_PORT));
}

vector<BoundaryMapEntry> identity_boundary_map(Vertex aOrig = 2, Vertex bOrig = 8) {
    return {
        BoundaryMapEntry{aOrig, aOrig},
        BoundaryMapEntry{bOrig, bOrig},
    };
}

UpdJob make_integrate_queue_job(RawSkelID parentSid, RawSkelID childSid, Vertex aOrig = 2, Vertex bOrig = 8) {
    UpdJob job;
    job.kind = UpdJobKind::INTEGRATE_CHILD;
    job.parentSid = parentSid;
    job.childSid = childSid;
    job.aOrig = aOrig;
    job.bOrig = bOrig;
    job.bm = identity_boundary_map(aOrig, bOrig);
    return job;
}

UpdJob make_join_queue_job(RawSkelID leftSid, RawSkelID rightSid, Vertex aOrig = 2, Vertex bOrig = 8) {
    UpdJob job;
    job.kind = UpdJobKind::JOIN_PAIR;
    job.leftSid = leftSid;
    job.rightSid = rightSid;
    job.aOrig = aOrig;
    job.bOrig = bOrig;
    return job;
}

Vertex relabel_vertex(const unordered_map<Vertex, Vertex>& relabelOrig, Vertex orig) {
    const auto it = relabelOrig.find(orig);
    return it == relabelOrig.end() ? orig : it->second;
}

size_t count_live_real_vertices(const RawEngine& RE) {
    unordered_set<Vertex> origs;
    for (const auto& slot : RE.V.a) {
        if (slot.alive && slot.val.kind == RawVertexKind::REAL) {
            origs.insert(slot.val.orig);
        }
    }
    return origs.size();
}

size_t count_live_occurrences(const RawEngine& RE) {
    size_t count = 0U;
    for (const auto& slot : RE.occ.a) {
        count += slot.alive ? 1U : 0U;
    }
    return count;
}

size_t count_live_edges(const RawEngine& RE) {
    unordered_set<string> semanticEdges;
    for (u32 eid = 0; eid < RE.E.a.size(); ++eid) {
        if (!RE.E.a[eid].alive) {
            continue;
        }
        const RawEdge& edge = RE.E.get(eid);
        const RawVertex& aVertex = RE.V.get(edge.a);
        const RawVertex& bVertex = RE.V.get(edge.b);
        string aToken;
        string bToken;
        if (aVertex.kind == RawVertexKind::REAL) {
            aToken = string("R:") + to_string(aVertex.orig);
        } else {
            aToken = string("C:") + to_string(RE.occ.get(aVertex.occ).orig);
        }
        if (bVertex.kind == RawVertexKind::REAL) {
            bToken = string("R:") + to_string(bVertex.orig);
        } else {
            bToken = string("C:") + to_string(RE.occ.get(bVertex.occ).orig);
        }
        if (aToken > bToken) {
            swap(aToken, bToken);
        }
        ostringstream oss;
        oss << static_cast<int>(edge.kind)
            << ':' << aToken
            << ':' << bToken
            << ':' << edge.br
            << ':' << static_cast<int>(edge.side);
        semanticEdges.insert(oss.str());
    }
    return semanticEdges.size();
}

size_t count_live_components(const RawEngine& RE) {
    size_t count = 0U;
    for (const auto& slot : RE.skel.a) {
        count += slot.alive ? 1U : 0U;
    }
    return count;
}

size_t max_hosted_occurrences(const RawEngine& RE) {
    size_t maxHosted = 0U;
    for (const auto& slot : RE.skel.a) {
        if (!slot.alive) {
            continue;
        }
        maxHosted = max(maxHosted, slot.val.hostedOcc.size());
    }
    return maxHosted;
}

bool scenario_within_bounds(const ExhaustiveScenario& scenario, const TestOptions& options) {
    return count_live_real_vertices(scenario.RE) <= options.maxReal &&
           count_live_occurrences(scenario.RE) <= options.maxOcc &&
           count_live_edges(scenario.RE) <= options.maxEdges &&
           (options.maxComponents == 0U || count_live_components(scenario.RE) <= options.maxComponents) &&
           (options.maxHostedOcc == 0U || max_hosted_occurrences(scenario.RE) <= options.maxHostedOcc);
}

void maybe_push_scenario(vector<ExhaustiveScenario>& out, ExhaustiveScenario&& scenario, const TestOptions& options) {
    if (!scenario_within_bounds(scenario, options)) {
        return;
    }
    if (out.size() >= options.maxStates) {
        return;
    }
    out.push_back(std::move(scenario));
}

bool planner_stop_condition_satisfied(const RawEngine& RE, OccID targetOcc) {
    const RawOccRecord& O = RE.occ.get(targetOcc);
    const RawSkeleton& S = RE.skel.get(O.hostSkel);
    if (S.hostedOcc.size() != 1U || S.hostedOcc[0] != targetOcc) {
        return false;
    }
    if (discover_split_pair_from_support(RE, targetOcc).has_value()) {
        return false;
    }
    assert_occ_patch_consistent(RE, targetOcc);
    return true;
}

string port_token(const RawEngine& RE, OccID occ, RawEID eid) {
    const RawOccRecord& O = RE.occ.get(occ);
    const RawEdge& edge = RE.E.get(eid);
    const RawVID attach = other_end(edge, O.centerV);
    const Vertex attachOrig = RE.V.get(attach).orig;
    ostringstream oss;
    if (edge.kind == RawEdgeKind::REAL_PORT) {
        oss << "REAL:" << attachOrig;
    } else if (edge.kind == RawEdgeKind::BRIDGE_PORT) {
        oss << "BRIDGE:" << attachOrig << ':' << edge.br << ':' << static_cast<int>(edge.side);
    } else {
        oss << "CORE:" << attachOrig;
    }
    return oss.str();
}

vector<string> capture_port_tokens(const RawEngine& RE, OccID occ) {
    vector<string> tokens;
    const RawOccRecord& O = RE.occ.get(occ);
    for (RawEID eid : RE.V.get(O.centerV).adj) {
        tokens.push_back(port_token(RE, occ, eid));
    }
    sort(tokens.begin(), tokens.end());
    return tokens;
}

vector<string> capture_core_patch_tokens(const RawEngine& RE, OccID occ) {
    vector<string> tokens;
    const RawOccRecord& O = RE.occ.get(occ);
    for (RawEID eid : O.corePatchEdges) {
        const RawEdge& edge = RE.E.get(eid);
        Vertex a = RE.V.get(edge.a).orig;
        Vertex b = RE.V.get(edge.b).orig;
        if (a > b) {
            swap(a, b);
        }
        ostringstream oss;
        oss << a << '-' << b;
        tokens.push_back(oss.str());
    }
    sort(tokens.begin(), tokens.end());
    return tokens;
}

string occ_shape_token(const RawEngine& RE, OccID occ) {
    const RawOccRecord& O = RE.occ.get(occ);
    vector<Vertex> allocNbrOrig;
    allocNbrOrig.reserve(O.allocNbr.size());
    for (OccID nbr : O.allocNbr) {
        allocNbrOrig.push_back(RE.occ.get(nbr).orig);
    }
    sort(allocNbrOrig.begin(), allocNbrOrig.end());

    ostringstream oss;
    oss << "orig=" << O.orig << ";alloc=[";
    for (size_t i = 0; i < allocNbrOrig.size(); ++i) {
        if (i != 0U) {
            oss << ',';
        }
        oss << allocNbrOrig[i];
    }
    oss << "];ports=[";
    const vector<string> ports = capture_port_tokens(RE, occ);
    for (size_t i = 0; i < ports.size(); ++i) {
        if (i != 0U) {
            oss << ',';
        }
        oss << ports[i];
    }
    oss << "];core=[";
    const vector<string> core = capture_core_patch_tokens(RE, occ);
    for (size_t i = 0; i < core.size(); ++i) {
        if (i != 0U) {
            oss << ',';
        }
        oss << core[i];
    }
    oss << ']';
    return oss.str();
}

string vertex_token(const RawEngine& RE, const unordered_map<OccID, string>& occTokenByOcc, RawVID vid) {
    const RawVertex& vertex = RE.V.get(vid);
    if (vertex.kind == RawVertexKind::REAL) {
        return string("R:") + to_string(vertex.orig);
    }
    return string("C:") + occTokenByOcc.at(vertex.occ);
}

string edge_token(
    const RawEngine& RE,
    const unordered_map<OccID, string>& occTokenByOcc,
    RawEID eid
) {
    const RawEdge& edge = RE.E.get(eid);
    string a = vertex_token(RE, occTokenByOcc, edge.a);
    string b = vertex_token(RE, occTokenByOcc, edge.b);
    if (a > b) {
        swap(a, b);
    }
    ostringstream oss;
    oss << static_cast<int>(edge.kind) << ':' << a << ':' << b << ':' << edge.br << ':' << static_cast<int>(edge.side);
    return oss.str();
}

vector<ExhaustiveScenario> generate_split_ready_scenarios(const TestOptions& options) {
    vector<ExhaustiveScenario> out;
    for (int targetLen = 1; targetLen <= 3 && out.size() < options.maxStates; ++targetLen) {
        for (int siblingLen = 1; siblingLen <= 3 && out.size() < options.maxStates; ++siblingLen) {
            for (int directAB = 0; directAB <= 1 && out.size() < options.maxStates; ++directAB) {
                for (int bridgePort = 0; bridgePort <= 1 && out.size() < options.maxStates; ++bridgePort) {
                    for (int targetFirst = 0; targetFirst <= 1 && out.size() < options.maxStates; ++targetFirst) {
                        ExhaustiveScenario scenario;
                        scenario.family = ExhaustiveFamily::SPLIT_READY;
                        scenario.label = string("split_ready_t") + to_string(targetLen) +
                            "_s" + to_string(siblingLen) +
                            "_ab" + to_string(directAB) +
                            "_bp" + to_string(bridgePort) +
                            "_tf" + to_string(targetFirst);

                        const Vertex targetOrig =
                            5000U + static_cast<Vertex>(targetLen * 100 + siblingLen * 10 + directAB * 2 + bridgePort);
                        const Vertex targetBase =
                            6000U + static_cast<Vertex>(targetLen * 64 + bridgePort * 8 + directAB * 4);
                        const Vertex siblingBase =
                            7000U + static_cast<Vertex>(siblingLen * 64 + directAB * 8 + bridgePort * 4);
                        const BridgeRef bridgeRef =
                            8000U + static_cast<BridgeRef>(targetLen * 32 + siblingLen * 8 + directAB * 2 + bridgePort);

                        const OccID targetOcc = new_occ(scenario.RE, targetOrig);
                        const RawSkelID sid = new_skeleton(scenario.RE);

                        RawSkeletonBuilder B;
                        const u32 sepA = static_cast<u32>(B.V.size());
                        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 2, 0));
                        const u32 sepB = static_cast<u32>(B.V.size());
                        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 8, 0));

                        BranchLayout targetBranch;
                        if (targetFirst != 0) {
                            targetBranch = append_core_branch(B, sepA, sepB, targetBase, targetLen);
                            (void)append_core_branch(B, sepA, sepB, siblingBase, siblingLen);
                        } else {
                            (void)append_core_branch(B, sepA, sepB, siblingBase, siblingLen);
                            targetBranch = append_core_branch(B, sepA, sepB, targetBase, targetLen);
                        }

                        const u32 center = static_cast<u32>(B.V.size());
                        B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, scenario.RE.occ.get(targetOcc).orig, targetOcc));
                        add_occ_center_edge(
                            B,
                            center,
                            targetBranch.attachLocalV,
                            bridgePort != 0,
                            bridgeRef,
                            static_cast<u8>((targetLen + siblingLen + directAB) & 1)
                        );

                        if (directAB != 0) {
                            B.E.push_back(make_builder_edge(sepA, sepB, RawEdgeKind::CORE_REAL));
                        }

                        B.allocNbr[targetOcc] = {};
                        B.corePatchLocalEids[targetOcc] = targetBranch.coreLocalEids;
                        commit_skeleton(scenario.RE, sid, std::move(B), scenario.U);

                        scenario.ctx.targetOcc = targetOcc;
                        scenario.ctx.keepOcc = {targetOcc};
                        scenario.primitivePlan.primitive = PrimitiveKind::SPLIT;
                        scenario.primitivePlan.sid = sid;
                        scenario.primitivePlan.aOrig = 2;
                        scenario.primitivePlan.bOrig = 8;
                        maybe_push_scenario(out, std::move(scenario), options);
                    }
                }
            }
        }
    }
    return out;
}

vector<ExhaustiveScenario> generate_join_ready_scenarios(const TestOptions& options) {
    vector<ExhaustiveScenario> out;
    for (int leftLen = 1; leftLen <= 3 && out.size() < options.maxStates; ++leftLen) {
        for (int rightLen = 1; rightLen <= 3 && out.size() < options.maxStates; ++rightLen) {
            for (int leftBridge = 0; leftBridge <= 1 && out.size() < options.maxStates; ++leftBridge) {
                for (int rightBridge = 0; rightBridge <= 1 && out.size() < options.maxStates; ++rightBridge) {
                    for (int commitRightFirst = 0; commitRightFirst <= 1 && out.size() < options.maxStates; ++commitRightFirst) {
                        ExhaustiveScenario scenario;
                        scenario.family = ExhaustiveFamily::JOIN_READY;
                        scenario.label = string("join_ready_l") + to_string(leftLen) +
                            "_r" + to_string(rightLen) +
                            "_lb" + to_string(leftBridge) +
                            "_rb" + to_string(rightBridge) +
                            "_crf" + to_string(commitRightFirst);

                        const Vertex targetOrig =
                            9000U + static_cast<Vertex>(leftLen * 100 + rightLen * 10 + leftBridge * 2 + rightBridge);
                        const Vertex keepOrig =
                            10000U + static_cast<Vertex>(rightLen * 100 + leftLen * 10 + rightBridge * 2 + leftBridge);
                        const Vertex leftBase =
                            11000U + static_cast<Vertex>(leftLen * 64 + leftBridge * 8 + rightLen * 4);
                        const Vertex rightBase =
                            13000U + static_cast<Vertex>(rightLen * 64 + rightBridge * 8 + leftLen * 4);

                        const OccID targetOcc = new_occ(scenario.RE, targetOrig);
                        const OccID keepOcc = new_occ(scenario.RE, keepOrig);
                        const RawSkelID leftSid = new_skeleton(scenario.RE);
                        const RawSkelID rightSid = new_skeleton(scenario.RE);

                        auto commit_left = [&]() {
                            RawSkeletonBuilder B;
                            const u32 sepA = static_cast<u32>(B.V.size());
                            B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 2, 0));
                            const u32 sepB = static_cast<u32>(B.V.size());
                            B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 8, 0));
                            const BranchLayout branch = append_core_branch(B, sepA, sepB, leftBase, leftLen);
                            const u32 center = static_cast<u32>(B.V.size());
                            B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, scenario.RE.occ.get(targetOcc).orig, targetOcc));
                            add_occ_center_edge(
                                B,
                                center,
                                branch.attachLocalV,
                                leftBridge != 0,
                                12000U + static_cast<BridgeRef>(leftLen * 32 + rightLen * 8 + leftBridge * 2 + rightBridge),
                                static_cast<u8>((leftLen + rightLen + leftBridge) & 1)
                            );
                            B.allocNbr[targetOcc] = {};
                            B.corePatchLocalEids[targetOcc] = branch.coreLocalEids;
                            commit_skeleton(scenario.RE, leftSid, std::move(B), scenario.U);
                        };

                        auto commit_right = [&]() {
                            RawSkeletonBuilder B;
                            const u32 sepA = static_cast<u32>(B.V.size());
                            B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 2, 0));
                            const u32 sepB = static_cast<u32>(B.V.size());
                            B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 8, 0));
                            const BranchLayout branch = append_core_branch(B, sepA, sepB, rightBase, rightLen);
                            const u32 center = static_cast<u32>(B.V.size());
                            B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, scenario.RE.occ.get(keepOcc).orig, keepOcc));
                            add_occ_center_edge(
                                B,
                                center,
                                branch.attachLocalV,
                                rightBridge != 0,
                                14000U + static_cast<BridgeRef>(rightLen * 32 + leftLen * 8 + rightBridge * 2 + leftBridge),
                                static_cast<u8>((leftLen + rightLen + rightBridge + 1) & 1)
                            );
                            B.allocNbr[keepOcc] = {};
                            B.corePatchLocalEids[keepOcc] = branch.coreLocalEids;
                            commit_skeleton(scenario.RE, rightSid, std::move(B), scenario.U);
                        };

                        if (commitRightFirst != 0) {
                            commit_right();
                            commit_left();
                        } else {
                            commit_left();
                            commit_right();
                        }

                        scenario.ctx.targetOcc = targetOcc;
                        scenario.ctx.keepOcc = {keepOcc};
                        scenario.initialQueue.push_back(make_join_queue_job(leftSid, rightSid, 2, 8));
                        scenario.primitivePlan.primitive = PrimitiveKind::JOIN;
                        scenario.primitivePlan.leftSid = leftSid;
                        scenario.primitivePlan.rightSid = rightSid;
                        scenario.primitivePlan.aOrig = 2;
                        scenario.primitivePlan.bOrig = 8;
                        maybe_push_scenario(out, std::move(scenario), options);
                    }
                }
            }
        }
    }
    return out;
}

vector<ExhaustiveScenario> generate_integrate_ready_scenarios(const TestOptions& options) {
    vector<ExhaustiveScenario> out;
    for (int parentLen = 1; parentLen <= 3 && out.size() < options.maxStates; ++parentLen) {
        for (int childMode = 0; childMode < 3 && out.size() < options.maxStates; ++childMode) {
            for (int childLen = 0; childLen <= 3 && out.size() < options.maxStates; ++childLen) {
                for (int parentBridge = 0; parentBridge <= 1 && out.size() < options.maxStates; ++parentBridge) {
                    for (int commitChildFirst = 0; commitChildFirst <= 1 && out.size() < options.maxStates; ++commitChildFirst) {
                        ExhaustiveScenario scenario;
                        scenario.family = ExhaustiveFamily::INTEGRATE_READY;
                        scenario.label = string("integrate_ready_p") + to_string(parentLen) +
                            "_cm" + to_string(childMode) +
                            "_cl" + to_string(childLen) +
                            "_pb" + to_string(parentBridge) +
                            "_ccf" + to_string(commitChildFirst);

                        const Vertex targetOrig = 15000U + static_cast<Vertex>(
                            parentLen * 100 + childMode * 20 + childLen * 2 + parentBridge
                        );
                        const Vertex keepOrig = 16000U + static_cast<Vertex>(
                            childMode * 100 + childLen * 10 + parentLen * 2 + parentBridge
                        );
                        const Vertex parentBase = 17000U + static_cast<Vertex>(
                            parentLen * 64 + childMode * 16 + parentBridge * 8
                        );
                        const Vertex childBase = 19000U + static_cast<Vertex>(
                            childMode * 64 + childLen * 16 + parentLen * 4
                        );

                        const OccID targetOcc = new_occ(scenario.RE, targetOrig);
                        optional<OccID> keepOcc;
                        if (childMode == 2) {
                            keepOcc = new_occ(scenario.RE, keepOrig);
                        }
                        const RawSkelID parentSid = new_skeleton(scenario.RE);
                        const RawSkelID childSid = new_skeleton(scenario.RE);

                        auto commit_parent = [&]() {
                            RawSkeletonBuilder B;
                            const u32 sepA = static_cast<u32>(B.V.size());
                            B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 2, 0));
                            const u32 sepB = static_cast<u32>(B.V.size());
                            B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 8, 0));
                            const BranchLayout branch = append_core_branch(B, sepA, sepB, parentBase, parentLen);
                            const u32 center = static_cast<u32>(B.V.size());
                            B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, scenario.RE.occ.get(targetOcc).orig, targetOcc));
                            add_occ_center_edge(
                                B,
                                center,
                                branch.attachLocalV,
                                parentBridge != 0,
                                18000U + static_cast<BridgeRef>(parentLen * 32 + childMode * 8 + childLen * 2 + parentBridge),
                                static_cast<u8>((parentLen + childMode + parentBridge) & 1)
                            );
                            B.allocNbr[targetOcc] = {};
                            B.corePatchLocalEids[targetOcc] = branch.coreLocalEids;
                            commit_skeleton(scenario.RE, parentSid, std::move(B), scenario.U);
                        };

                        auto commit_child = [&]() {
                            RawSkeletonBuilder B;
                            const u32 sepA = static_cast<u32>(B.V.size());
                            B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 2, 0));
                            const u32 sepB = static_cast<u32>(B.V.size());
                            B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 8, 0));

                            if (childMode == 0) {
                                B.E.push_back(make_builder_edge(sepA, sepB, RawEdgeKind::CORE_REAL));
                            } else {
                                const BranchLayout branch =
                                    append_core_branch(B, sepA, sepB, childBase, max(1, childLen));
                                if (childMode == 2 && keepOcc.has_value()) {
                                    const u32 center = static_cast<u32>(B.V.size());
                                    B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, scenario.RE.occ.get(*keepOcc).orig, *keepOcc));
                                    add_occ_center_edge(B, center, branch.attachLocalV, false, 0, 0);
                                    B.allocNbr[*keepOcc] = {};
                                    B.corePatchLocalEids[*keepOcc] = branch.coreLocalEids;
                                }
                            }
                            commit_skeleton(scenario.RE, childSid, std::move(B), scenario.U);
                        };

                        if (commitChildFirst != 0) {
                            commit_child();
                            commit_parent();
                        } else {
                            commit_parent();
                            commit_child();
                        }

                        scenario.ctx.targetOcc = targetOcc;
                        scenario.ctx.keepOcc = keepOcc.has_value() ? unordered_set<OccID>{*keepOcc} : unordered_set<OccID>{targetOcc};
                        scenario.initialQueue.push_back(make_integrate_queue_job(parentSid, childSid, 2, 8));
                        scenario.primitivePlan.primitive = PrimitiveKind::INTEGRATE;
                        scenario.primitivePlan.parentSid = parentSid;
                        scenario.primitivePlan.childSid = childSid;
                        scenario.primitivePlan.boundaryMap = identity_boundary_map();
                        maybe_push_scenario(out, std::move(scenario), options);
                    }
                }
            }
        }
    }
    return out;
}

vector<ExhaustiveScenario> generate_mixed_scenarios(const TestOptions& options) {
    const TestOptions familyOptions = options;
    vector<ExhaustiveScenario> split = generate_split_ready_scenarios(familyOptions);
    vector<ExhaustiveScenario> join = generate_join_ready_scenarios(familyOptions);
    vector<ExhaustiveScenario> integrate = generate_integrate_ready_scenarios(familyOptions);
    vector<ExhaustiveScenario> out;
    const size_t limit = options.maxStates;
    for (size_t i = 0; out.size() < limit && (i < split.size() || i < join.size() || i < integrate.size()); ++i) {
        if (i < split.size() && out.size() < limit) {
            split[i].family = ExhaustiveFamily::MIXED;
            out.push_back(std::move(split[i]));
        }
        if (i < join.size() && out.size() < limit) {
            join[i].family = ExhaustiveFamily::MIXED;
            out.push_back(std::move(join[i]));
        }
        if (i < integrate.size() && out.size() < limit) {
            integrate[i].family = ExhaustiveFamily::MIXED;
            out.push_back(std::move(integrate[i]));
        }
    }
    return out;
}

vector<ExhaustiveScenario> generate_targeted_family_scenarios(
    const TestOptions& options,
    ScenarioFamily scenarioFamily,
    ExhaustiveFamily exhaustiveFamily,
    u32 seedBase,
    u32 seedStep
) {
    vector<ExhaustiveScenario> out;
    for (u32 index = 0; out.size() < options.maxStates && index < options.maxStates * 4U; ++index) {
        ExhaustiveScenario scenario = make_targeted_planner_exhaustive_scenario(
            scenarioFamily,
            seedBase + index * seedStep
        );
        scenario.family = exhaustiveFamily;
        if (enumerate_valid_split_pairs(scenario.RE, scenario.ctx.targetOcc).size() < 2U) {
            continue;
        }
        maybe_push_scenario(out, std::move(scenario), options);
    }
    return out;
}

vector<ExhaustiveScenario> generate_split_tie_ready_scenarios(const TestOptions& options) {
    return generate_targeted_family_scenarios(
        options,
        ScenarioFamily::SPLIT_TIE_READY,
        ExhaustiveFamily::SPLIT_TIE_READY,
        880101U,
        17U
    );
}

vector<ExhaustiveScenario> generate_split_tie_structural_scenarios(const TestOptions& options) {
    return generate_targeted_family_scenarios(
        options,
        ScenarioFamily::SPLIT_TIE_STRUCTURAL,
        ExhaustiveFamily::SPLIT_TIE_STRUCTURAL,
        880301U,
        3U
    );
}

vector<ExhaustiveScenario> generate_planner_tie_mixed_scenarios(const TestOptions& options) {
    vector<ExhaustiveScenario> out;
    for (u32 index = 0; out.size() < options.maxStates && index < options.maxStates * 4U; ++index) {
        ExhaustiveScenario scenario = make_targeted_planner_exhaustive_scenario(
            ScenarioFamily::PLANNER_TIE_MIXED,
            880501U + index * 11U
        );
        scenario.family = ExhaustiveFamily::PLANNER_TIE_MIXED;
        maybe_push_scenario(out, std::move(scenario), options);
    }
    return out;
}

vector<ExhaustiveScenario> generate_split_tie_symmetric_large_scenarios(const TestOptions& options) {
    return generate_targeted_family_scenarios(
        options,
        ScenarioFamily::SPLIT_TIE_SYMMETRIC_LARGE,
        ExhaustiveFamily::SPLIT_TIE_SYMMETRIC_LARGE,
        890101U,
        5U
    );
}

vector<ExhaustiveScenario> generate_planner_tie_mixed_symmetric_scenarios(const TestOptions& options) {
    vector<ExhaustiveScenario> out;
    for (u32 index = 0; out.size() < options.maxStates && index < options.maxStates * 4U; ++index) {
        ExhaustiveScenario scenario = make_targeted_planner_exhaustive_scenario(
            ScenarioFamily::PLANNER_TIE_MIXED_SYMMETRIC,
            890301U + index * 7U
        );
        scenario.family = ExhaustiveFamily::PLANNER_TIE_MIXED_SYMMETRIC;
        maybe_push_scenario(out, std::move(scenario), options);
    }
    return out;
}

vector<ExhaustiveScenario> generate_canonical_collision_probe_scenarios(const TestOptions& options) {
    return generate_targeted_family_scenarios(
        options,
        ScenarioFamily::CANONICAL_COLLISION_PROBE,
        ExhaustiveFamily::CANONICAL_COLLISION_PROBE,
        890501U,
        1U
    );
}

vector<ExhaustiveScenario> generate_split_tie_organic_symmetric_scenarios(const TestOptions& options) {
    return generate_targeted_family_scenarios(
        options,
        ScenarioFamily::SPLIT_TIE_ORGANIC_SYMMETRIC,
        ExhaustiveFamily::SPLIT_TIE_ORGANIC_SYMMETRIC,
        910101U,
        3U
    );
}

vector<ExhaustiveScenario> generate_planner_tie_mixed_organic_scenarios(const TestOptions& options) {
    vector<ExhaustiveScenario> out;
    for (u32 index = 0; out.size() < options.maxStates && index < options.maxStates * 4U; ++index) {
        ExhaustiveScenario scenario = make_targeted_planner_exhaustive_scenario(
            ScenarioFamily::PLANNER_TIE_MIXED_ORGANIC,
            910201U + index * 5U
        );
        scenario.family = ExhaustiveFamily::PLANNER_TIE_MIXED_ORGANIC;
        maybe_push_scenario(out, std::move(scenario), options);
    }
    return out;
}

vector<ExhaustiveScenario> generate_automorphism_probe_large_scenarios(const TestOptions& options) {
    return generate_targeted_family_scenarios(
        options,
        ScenarioFamily::AUTOMORPHISM_PROBE_LARGE,
        ExhaustiveFamily::AUTOMORPHISM_PROBE_LARGE,
        910301U,
        1U
    );
}

template <class T>
vector<T> reversed_copy(const vector<T>& in) {
    vector<T> out = in;
    reverse(out.begin(), out.end());
    return out;
}

vector<RawVID> ordered_vertices(const RawEngine& RE, RawSkelID sid, const RebuildTransform& transform) {
    vector<RawVID> verts = RE.skel.get(sid).verts;
    if (transform.reverseVertexOrder) {
        reverse(verts.begin(), verts.end());
    }
    if (transform.reverseHostedOccOrder) {
        vector<RawVID> realVerts;
        vector<RawVID> centerVerts;
        for (RawVID vid : verts) {
            if (RE.V.get(vid).kind == RawVertexKind::REAL) {
                realVerts.push_back(vid);
            } else {
                centerVerts.push_back(vid);
            }
        }
        reverse(centerVerts.begin(), centerVerts.end());
        verts = std::move(realVerts);
        verts.insert(verts.end(), centerVerts.begin(), centerVerts.end());
    }
    return verts;
}

vector<RawEID> ordered_edges(const RawEngine& RE, RawSkelID sid, const RebuildTransform& transform) {
    vector<RawEID> edges = RE.skel.get(sid).edges;
    if (transform.reverseEdgeOrder) {
        reverse(edges.begin(), edges.end());
    }
    return edges;
}

} // namespace

bool SemanticOccurrenceSignature::operator==(const SemanticOccurrenceSignature& rhs) const {
    return orig == rhs.orig &&
           allocNbrOrig == rhs.allocNbrOrig &&
           ports == rhs.ports &&
           corePatchEdges == rhs.corePatchEdges;
}
bool SemanticOccurrenceSignature::operator<(const SemanticOccurrenceSignature& rhs) const {
    return tie(orig, allocNbrOrig, ports, corePatchEdges) <
           tie(rhs.orig, rhs.allocNbrOrig, rhs.ports, rhs.corePatchEdges);
}

bool SemanticSkeletonSignature::operator==(const SemanticSkeletonSignature& rhs) const {
    return vertices == rhs.vertices &&
           edges == rhs.edges &&
           hostedOccurrences == rhs.hostedOccurrences;
}
bool SemanticSkeletonSignature::operator<(const SemanticSkeletonSignature& rhs) const {
    return tie(vertices, edges, hostedOccurrences) < tie(rhs.vertices, rhs.edges, rhs.hostedOccurrences);
}

bool SemanticStateCanonicalSignature::operator==(const SemanticStateCanonicalSignature& rhs) const {
    return skeletons == rhs.skeletons && occurrences == rhs.occurrences;
}

bool PrimitiveCanonicalSignature::operator==(const PrimitiveCanonicalSignature& rhs) const {
    return primitive == rhs.primitive && finalState == rhs.finalState;
}

bool PlannerFinalStateCanonicalSignature::operator==(const PlannerFinalStateCanonicalSignature& rhs) const {
    return finalState == rhs.finalState && stopConditionSatisfied == rhs.stopConditionSatisfied;
}

bool ExplorerStateCanonicalSignature::operator==(const ExplorerStateCanonicalSignature& rhs) const {
    return family == rhs.family &&
           state == rhs.state &&
           targetOccurrence == rhs.targetOccurrence &&
           keepOccurrences == rhs.keepOccurrences &&
           initialQueue == rhs.initialQueue;
}

const char* exhaustive_family_name(ExhaustiveFamily family) {
    switch (family) {
        case ExhaustiveFamily::SPLIT_READY:
            return "split_ready";
        case ExhaustiveFamily::JOIN_READY:
            return "join_ready";
        case ExhaustiveFamily::INTEGRATE_READY:
            return "integrate_ready";
        case ExhaustiveFamily::MIXED:
            return "mixed";
        case ExhaustiveFamily::SPLIT_TIE_READY:
            return "split_tie_ready";
        case ExhaustiveFamily::SPLIT_TIE_STRUCTURAL:
            return "split_tie_structural";
        case ExhaustiveFamily::PLANNER_TIE_MIXED:
            return "planner_tie_mixed";
        case ExhaustiveFamily::SPLIT_TIE_SYMMETRIC_LARGE:
            return "split_tie_symmetric_large";
        case ExhaustiveFamily::PLANNER_TIE_MIXED_SYMMETRIC:
            return "planner_tie_mixed_symmetric";
        case ExhaustiveFamily::CANONICAL_COLLISION_PROBE:
            return "canonical_collision_probe";
        case ExhaustiveFamily::SPLIT_TIE_ORGANIC_SYMMETRIC:
            return "split_tie_organic_symmetric";
        case ExhaustiveFamily::PLANNER_TIE_MIXED_ORGANIC:
            return "planner_tie_mixed_organic";
        case ExhaustiveFamily::AUTOMORPHISM_PROBE_LARGE:
            return "automorphism_probe_large";
        case ExhaustiveFamily::ALL:
            return "all";
    }
    return "unknown";
}

SemanticStateCanonicalSignature capture_semantic_state_signature(const RawEngine& RE) {
    unordered_map<OccID, string> occTokenByOcc;
    for (u32 occ = 0; occ < RE.occ.a.size(); ++occ) {
        if (!RE.occ.a[occ].alive) {
            continue;
        }
        occTokenByOcc[occ] = occ_shape_token(RE, occ);
    }

    SemanticStateCanonicalSignature sig;

    for (u32 sid = 0; sid < RE.skel.a.size(); ++sid) {
        if (!RE.skel.a[sid].alive) {
            continue;
        }
        const RawSkeleton& S = RE.skel.get(sid);
        SemanticSkeletonSignature skelSig;
        for (RawVID vid : S.verts) {
            skelSig.vertices.push_back(vertex_token(RE, occTokenByOcc, vid));
        }
        sort(skelSig.vertices.begin(), skelSig.vertices.end());
        for (RawEID eid : S.edges) {
            skelSig.edges.push_back(edge_token(RE, occTokenByOcc, eid));
        }
        sort(skelSig.edges.begin(), skelSig.edges.end());
        for (OccID occ : S.hostedOcc) {
            skelSig.hostedOccurrences.push_back(occTokenByOcc.at(occ));
        }
        sort(skelSig.hostedOccurrences.begin(), skelSig.hostedOccurrences.end());
        sig.skeletons.push_back(std::move(skelSig));
    }
    sort(sig.skeletons.begin(), sig.skeletons.end());

    for (u32 occ = 0; occ < RE.occ.a.size(); ++occ) {
        if (!RE.occ.a[occ].alive) {
            continue;
        }
        const RawOccRecord& O = RE.occ.get(occ);
        SemanticOccurrenceSignature occSig;
        occSig.orig = O.orig;
        for (OccID nbr : O.allocNbr) {
            occSig.allocNbrOrig.push_back(RE.occ.get(nbr).orig);
        }
        sort(occSig.allocNbrOrig.begin(), occSig.allocNbrOrig.end());
        occSig.ports = capture_port_tokens(RE, occ);
        occSig.corePatchEdges = capture_core_patch_tokens(RE, occ);
        sig.occurrences.push_back(std::move(occSig));
    }
    sort(sig.occurrences.begin(), sig.occurrences.end());
    return sig;
}

PrimitiveCanonicalSignature capture_primitive_canonical_signature(PrimitiveKind primitive, const RawEngine& RE) {
    PrimitiveCanonicalSignature sig;
    sig.primitive = primitive;
    sig.finalState = capture_semantic_state_signature(RE);
    return sig;
}

PlannerFinalStateCanonicalSignature capture_planner_final_state_canonical_signature(const RawEngine& RE, OccID targetOcc) {
    PlannerFinalStateCanonicalSignature sig;
    sig.finalState = capture_semantic_state_signature(RE);
    sig.stopConditionSatisfied = planner_stop_condition_satisfied(RE, targetOcc);
    return sig;
}

ExplorerStateCanonicalSignature capture_explorer_state_signature(const ExhaustiveScenario& scenario) {
    ExplorerStateCanonicalSignature sig;
    sig.family = scenario.family;
    sig.state = capture_semantic_state_signature(scenario.RE);

    unordered_map<OccID, string> occTokenByOcc;
    unordered_map<RawSkelID, string> skelHashBySid;
    for (u32 occ = 0; occ < scenario.RE.occ.a.size(); ++occ) {
        if (!scenario.RE.occ.a[occ].alive) {
            continue;
        }
        occTokenByOcc[occ] = occ_shape_token(scenario.RE, occ);
    }
    for (u32 sid = 0; sid < scenario.RE.skel.a.size(); ++sid) {
        if (!scenario.RE.skel.a[sid].alive) {
            continue;
        }
        SemanticSkeletonSignature skelSig;
        const RawSkeleton& S = scenario.RE.skel.get(sid);
        for (RawVID vid : S.verts) {
            skelSig.vertices.push_back(vertex_token(scenario.RE, occTokenByOcc, vid));
        }
        sort(skelSig.vertices.begin(), skelSig.vertices.end());
        for (RawEID eid : S.edges) {
            skelSig.edges.push_back(edge_token(scenario.RE, occTokenByOcc, eid));
        }
        sort(skelSig.edges.begin(), skelSig.edges.end());
        for (OccID occ : S.hostedOcc) {
            skelSig.hostedOccurrences.push_back(occTokenByOcc.at(occ));
        }
        sort(skelSig.hostedOccurrences.begin(), skelSig.hostedOccurrences.end());
        ostringstream oss;
        oss << "V=[";
        for (const string& token : skelSig.vertices) {
            oss << token << ';';
        }
        oss << "]E=[";
        for (const string& token : skelSig.edges) {
            oss << token << ';';
        }
        oss << "]H=[";
        for (const string& token : skelSig.hostedOccurrences) {
            oss << token << ';';
        }
        oss << ']';
        skelHashBySid[sid] = stable_hash_text(oss.str());
    }

    sig.targetOccurrence = occTokenByOcc.at(scenario.ctx.targetOcc);
    for (OccID keep : scenario.ctx.keepOcc) {
        sig.keepOccurrences.push_back(occTokenByOcc.at(keep));
    }
    sort(sig.keepOccurrences.begin(), sig.keepOccurrences.end());

    for (const UpdJob& job : scenario.initialQueue) {
        ostringstream oss;
        oss << static_cast<int>(job.kind);
        switch (job.kind) {
            case UpdJobKind::ENSURE_SOLE:
                oss << ':' << occTokenByOcc.at(job.occ);
                break;
            case UpdJobKind::JOIN_PAIR:
                oss << ':' << skelHashBySid.at(job.leftSid)
                    << ':' << skelHashBySid.at(job.rightSid)
                    << ':' << job.aOrig
                    << ':' << job.bOrig;
                break;
            case UpdJobKind::INTEGRATE_CHILD:
                oss << ':' << skelHashBySid.at(job.parentSid)
                    << ':' << skelHashBySid.at(job.childSid)
                    << ':' << job.aOrig
                    << ':' << job.bOrig;
                for (const BoundaryMapEntry& entry : job.bm) {
                    oss << ':' << entry.childOrig << "->" << entry.parentOrig;
                }
                break;
        }
        sig.initialQueue.push_back(oss.str());
    }
    return sig;
}

string describe_semantic_state_signature(const SemanticStateCanonicalSignature& sig) {
    ostringstream oss;
    oss << "skeletons=[";
    for (size_t i = 0; i < sig.skeletons.size(); ++i) {
        if (i != 0U) {
            oss << ',';
        }
        const SemanticSkeletonSignature& skel = sig.skeletons[i];
        oss << "{V=";
        for (const string& token : skel.vertices) {
            oss << token << ';';
        }
        oss << ",E=";
        for (const string& token : skel.edges) {
            oss << token << ';';
        }
        oss << ",H=";
        for (const string& token : skel.hostedOccurrences) {
            oss << token << ';';
        }
        oss << '}';
    }
    oss << "];occ=[";
    for (size_t i = 0; i < sig.occurrences.size(); ++i) {
        if (i != 0U) {
            oss << ',';
        }
        const SemanticOccurrenceSignature& occ = sig.occurrences[i];
        oss << "{orig=" << occ.orig << ";alloc=";
        for (Vertex orig : occ.allocNbrOrig) {
            oss << orig << ';';
        }
        oss << ";ports=";
        for (const string& token : occ.ports) {
            oss << token << ';';
        }
        oss << ";core=";
        for (const string& token : occ.corePatchEdges) {
            oss << token << ';';
        }
        oss << '}';
    }
    oss << ']';
    return oss.str();
}

string describe_explorer_state_signature(const ExplorerStateCanonicalSignature& sig) {
    ostringstream oss;
    oss << "family=" << exhaustive_family_name(sig.family)
        << "\ntarget=" << sig.targetOccurrence
        << "\nkeep=[";
    for (const string& token : sig.keepOccurrences) {
        oss << token << ';';
    }
    oss << "]\nqueue=[";
    for (const string& token : sig.initialQueue) {
        oss << token << ';';
    }
    oss << "]\nstate=" << describe_semantic_state_signature(sig.state);
    return oss.str();
}

string hash_semantic_state_signature(const SemanticStateCanonicalSignature& sig) {
    return stable_hash_text(describe_semantic_state_signature(sig));
}

string hash_explorer_state_signature(const ExplorerStateCanonicalSignature& sig) {
    return stable_hash_text(describe_explorer_state_signature(sig));
}

vector<ExhaustiveScenario> generate_exhaustive_scenarios(const TestOptions& options, ExhaustiveFamily family) {
    switch (family) {
        case ExhaustiveFamily::SPLIT_READY:
            return generate_split_ready_scenarios(options);
        case ExhaustiveFamily::JOIN_READY:
            return generate_join_ready_scenarios(options);
        case ExhaustiveFamily::INTEGRATE_READY:
            return generate_integrate_ready_scenarios(options);
        case ExhaustiveFamily::MIXED:
            return generate_mixed_scenarios(options);
        case ExhaustiveFamily::SPLIT_TIE_READY:
            return generate_split_tie_ready_scenarios(options);
        case ExhaustiveFamily::SPLIT_TIE_STRUCTURAL:
            return generate_split_tie_structural_scenarios(options);
        case ExhaustiveFamily::PLANNER_TIE_MIXED:
            return generate_planner_tie_mixed_scenarios(options);
        case ExhaustiveFamily::SPLIT_TIE_SYMMETRIC_LARGE:
            return generate_split_tie_symmetric_large_scenarios(options);
        case ExhaustiveFamily::PLANNER_TIE_MIXED_SYMMETRIC:
            return generate_planner_tie_mixed_symmetric_scenarios(options);
        case ExhaustiveFamily::CANONICAL_COLLISION_PROBE:
            return generate_canonical_collision_probe_scenarios(options);
        case ExhaustiveFamily::SPLIT_TIE_ORGANIC_SYMMETRIC:
            return generate_split_tie_organic_symmetric_scenarios(options);
        case ExhaustiveFamily::PLANNER_TIE_MIXED_ORGANIC:
            return generate_planner_tie_mixed_organic_scenarios(options);
        case ExhaustiveFamily::AUTOMORPHISM_PROBE_LARGE:
            return generate_automorphism_probe_large_scenarios(options);
        case ExhaustiveFamily::ALL:
            break;
    }
    throw runtime_error("generate_exhaustive_scenarios requires a concrete family");
}

ExhaustiveScenario rebuild_exhaustive_scenario(const ExhaustiveScenario& scenario, const RebuildTransform& transform) {
    ExhaustiveScenario out;
    out.family = scenario.family;
    out.label = scenario.label;

    vector<OccID> oldOccs;
    for (u32 occ = 0; occ < scenario.RE.occ.a.size(); ++occ) {
        if (scenario.RE.occ.a[occ].alive) {
            oldOccs.push_back(occ);
        }
    }
    sort(oldOccs.begin(), oldOccs.end());
    if (transform.reverseOccAllocationOrder) {
        reverse(oldOccs.begin(), oldOccs.end());
    }

    unordered_map<OccID, OccID> occMap;
    for (OccID oldOcc : oldOccs) {
        const Vertex newOrig = relabel_vertex(transform.relabelOrig, scenario.RE.occ.get(oldOcc).orig);
        const OccID newOcc = out.RE.occ.alloc(make_occ_record(newOrig));
        out.RE.occOfOrig[newOrig].push_back(newOcc);
        occMap[oldOcc] = newOcc;
    }

    vector<RawSkelID> oldSids;
    for (u32 sid = 0; sid < scenario.RE.skel.a.size(); ++sid) {
        if (scenario.RE.skel.a[sid].alive) {
            oldSids.push_back(sid);
        }
    }
    sort(oldSids.begin(), oldSids.end());
    if (transform.reverseSkeletonOrder) {
        reverse(oldSids.begin(), oldSids.end());
    }

    unordered_map<RawSkelID, RawSkelID> sidMap;
    for (RawSkelID oldSid : oldSids) {
        sidMap[oldSid] = out.RE.skel.alloc(RawSkeleton{});
    }

    for (RawSkelID oldSid : oldSids) {
        const RawSkeleton& oldS = scenario.RE.skel.get(oldSid);
        RawSkeletonBuilder B;
        unordered_map<RawVID, u32> localVid;
        unordered_map<RawEID, u32> localEid;

        for (RawVID oldVid : ordered_vertices(scenario.RE, oldSid, transform)) {
            const RawVertex& oldV = scenario.RE.V.get(oldVid);
            const u32 local = static_cast<u32>(B.V.size());
            if (oldV.kind == RawVertexKind::REAL) {
                B.V.push_back(make_builder_vertex(
                    RawVertexKind::REAL,
                    relabel_vertex(transform.relabelOrig, oldV.orig),
                    0
                ));
            } else {
                B.V.push_back(make_builder_vertex(
                    RawVertexKind::OCC_CENTER,
                    relabel_vertex(transform.relabelOrig, oldV.orig),
                    occMap.at(oldV.occ)
                ));
            }
            localVid[oldVid] = local;
        }

        for (RawEID oldEid : ordered_edges(scenario.RE, oldSid, transform)) {
            const RawEdge& oldE = scenario.RE.E.get(oldEid);
            localEid[oldEid] = static_cast<u32>(B.E.size());
            B.E.push_back(make_builder_edge(
                localVid.at(oldE.a),
                localVid.at(oldE.b),
                oldE.kind,
                oldE.br,
                oldE.side
            ));
        }

        for (OccID oldOcc : oldS.hostedOcc) {
            const RawOccRecord& oldO = scenario.RE.occ.get(oldOcc);
            vector<OccID> allocNbr;
            allocNbr.reserve(oldO.allocNbr.size());
            for (OccID nbr : oldO.allocNbr) {
                allocNbr.push_back(occMap.at(nbr));
            }
            B.allocNbr[occMap.at(oldOcc)] = std::move(allocNbr);

            vector<u32> corePatchLocalEids;
            corePatchLocalEids.reserve(oldO.corePatchEdges.size());
            for (RawEID oldEid : oldO.corePatchEdges) {
                corePatchLocalEids.push_back(localEid.at(oldEid));
            }
            B.corePatchLocalEids[occMap.at(oldOcc)] = std::move(corePatchLocalEids);
        }

        commit_skeleton(out.RE, sidMap.at(oldSid), std::move(B), out.U);
    }

    out.ctx.targetOcc = occMap.at(scenario.ctx.targetOcc);
    for (OccID keep : scenario.ctx.keepOcc) {
        out.ctx.keepOcc.insert(occMap.at(keep));
    }

    out.initialQueue.reserve(scenario.initialQueue.size());
    for (const UpdJob& job : scenario.initialQueue) {
        UpdJob mapped = job;
        if (mapped.occ != NIL_U32 && occMap.count(mapped.occ) != 0U) {
            mapped.occ = occMap.at(mapped.occ);
        }
        if (mapped.leftSid != NIL_U32 && sidMap.count(mapped.leftSid) != 0U) {
            mapped.leftSid = sidMap.at(mapped.leftSid);
        }
        if (mapped.rightSid != NIL_U32 && sidMap.count(mapped.rightSid) != 0U) {
            mapped.rightSid = sidMap.at(mapped.rightSid);
        }
        if (mapped.parentSid != NIL_U32 && sidMap.count(mapped.parentSid) != 0U) {
            mapped.parentSid = sidMap.at(mapped.parentSid);
        }
        if (mapped.childSid != NIL_U32 && sidMap.count(mapped.childSid) != 0U) {
            mapped.childSid = sidMap.at(mapped.childSid);
        }
        mapped.aOrig = relabel_vertex(transform.relabelOrig, mapped.aOrig);
        mapped.bOrig = relabel_vertex(transform.relabelOrig, mapped.bOrig);
        for (BoundaryMapEntry& entry : mapped.bm) {
            entry.childOrig = relabel_vertex(transform.relabelOrig, entry.childOrig);
            entry.parentOrig = relabel_vertex(transform.relabelOrig, entry.parentOrig);
        }
        out.initialQueue.push_back(std::move(mapped));
    }

    out.primitivePlan = scenario.primitivePlan;
    out.primitivePlan.sid = scenario.primitivePlan.sid != NIL_U32 ? sidMap.at(scenario.primitivePlan.sid) : NIL_U32;
    out.primitivePlan.leftSid =
        scenario.primitivePlan.leftSid != NIL_U32 ? sidMap.at(scenario.primitivePlan.leftSid) : NIL_U32;
    out.primitivePlan.rightSid =
        scenario.primitivePlan.rightSid != NIL_U32 ? sidMap.at(scenario.primitivePlan.rightSid) : NIL_U32;
    out.primitivePlan.parentSid =
        scenario.primitivePlan.parentSid != NIL_U32 ? sidMap.at(scenario.primitivePlan.parentSid) : NIL_U32;
    out.primitivePlan.childSid =
        scenario.primitivePlan.childSid != NIL_U32 ? sidMap.at(scenario.primitivePlan.childSid) : NIL_U32;
    out.primitivePlan.aOrig = relabel_vertex(transform.relabelOrig, scenario.primitivePlan.aOrig);
    out.primitivePlan.bOrig = relabel_vertex(transform.relabelOrig, scenario.primitivePlan.bOrig);
    out.primitivePlan.boundaryMap.clear();
    for (const BoundaryMapEntry& entry : scenario.primitivePlan.boundaryMap) {
        out.primitivePlan.boundaryMap.push_back(BoundaryMapEntry{
            relabel_vertex(transform.relabelOrig, entry.childOrig),
            relabel_vertex(transform.relabelOrig, entry.parentOrig),
        });
    }
    return out;
}

unordered_map<Vertex, Vertex> invert_relabel_map(const unordered_map<Vertex, Vertex>& relabelOrig) {
    unordered_map<Vertex, Vertex> inverse;
    for (const auto& [from, to] : relabelOrig) {
        inverse[to] = from;
    }
    return inverse;
}

RebuildTransform make_relabel_transform(const ExhaustiveScenario& scenario) {
    RebuildTransform transform;
    vector<Vertex> origs;
    unordered_set<Vertex> seen;
    for (const auto& slot : scenario.RE.V.a) {
        if (!slot.alive) {
            continue;
        }
        if (seen.insert(slot.val.orig).second) {
            origs.push_back(slot.val.orig);
        }
    }
    sort(origs.begin(), origs.end());
    vector<Vertex> permuted = origs;
    reverse(permuted.begin(), permuted.end());
    for (size_t i = 0; i < origs.size(); ++i) {
        transform.relabelOrig[origs[i]] = permuted[i] + 100000U;
    }
    return transform;
}

RebuildTransform make_occid_renumber_transform() {
    RebuildTransform transform;
    transform.reverseOccAllocationOrder = true;
    return transform;
}

RebuildTransform make_edge_order_transform() {
    RebuildTransform transform;
    transform.reverseEdgeOrder = true;
    return transform;
}

RebuildTransform make_vertex_order_transform() {
    RebuildTransform transform;
    transform.reverseVertexOrder = true;
    return transform;
}

RebuildTransform make_hosted_occ_order_transform() {
    RebuildTransform transform;
    transform.reverseHostedOccOrder = true;
    return transform;
}
