#include <bits/stdc++.h>
using namespace std;

using u8 = uint8_t;
using u32 = uint32_t;
using u64 = uint64_t;
using Vertex = u32;
using RawSkelID = u32;
using RawVID = u32;
using RawEID = u32;
using OccID = u32;
using BridgeRef = u32;
using LVertex = uint16_t;

static constexpr u32 NIL_U32 = numeric_limits<u32>::max();

// ============================================================
// SlotPool
// ============================================================

template <class T>
struct SlotPool {
    struct Slot {
        T val{};
        bool alive = false;
        u32 nextFree = NIL_U32;
    };

    vector<Slot> a;
    u32 freeHead = NIL_U32;

    u32 alloc(const T& x = T{}) {
        if (freeHead != NIL_U32) {
            u32 id = freeHead;
            freeHead = a[id].nextFree;
            a[id].val = x;
            a[id].alive = true;
            a[id].nextFree = NIL_U32;
            return id;
        }
        a.push_back({});
        u32 id = (u32)a.size() - 1;
        a[id].val = x;
        a[id].alive = true;
        return id;
    }

    void retire(u32 id) {
        if (id == NIL_U32) return;
        assert(id < a.size() && a[id].alive);
        a[id].alive = false;
        a[id].nextFree = freeHead;
        freeHead = id;
    }

    T& get(u32 id) {
        assert(id < a.size() && a[id].alive);
        return a[id].val;
    }

    const T& get(u32 id) const {
        assert(id < a.size() && a[id].alive);
        return a[id].val;
    }
};

// ============================================================
// Raw core types
// ============================================================

enum class RawVertexKind : u8 {
    REAL = 0,
    OCC_CENTER = 1,
};

enum class RawEdgeKind : u8 {
    CORE_REAL = 0,
    REAL_PORT = 1,
    BRIDGE_PORT = 2,
};

struct RawVertex {
    RawVertexKind kind = RawVertexKind::REAL;
    Vertex orig = NIL_U32;
    OccID occ = 0; // valid for OCC_CENTER
    vector<RawEID> adj;
};

struct RawEdge {
    RawVID a = 0, b = 0;
    RawEdgeKind kind = RawEdgeKind::CORE_REAL;
    BridgeRef br = 0; // BRIDGE_PORT only
    u8 side = 0;      // BRIDGE_PORT only
};

struct RawOccRecord {
    Vertex orig = NIL_U32;
    RawSkelID hostSkel = 0;
    RawVID centerV = 0;
    vector<OccID> allocNbr;      // same-original occurrence graph
    vector<RawEID> corePatchEdges; // non-center CORE_REAL edges belonging to this occurrence stencil
};

struct RawSkeleton {
    vector<RawVID> verts;
    vector<RawEID> edges;
    vector<OccID> hostedOcc;
};

struct RawEngine {
    SlotPool<RawVertex> V;
    SlotPool<RawEdge> E;
    SlotPool<RawSkeleton> skel;
    SlotPool<RawOccRecord> occ;

    unordered_map<Vertex, vector<OccID>> occOfOrig;
};

struct RawSkeletonBuilder {
    struct BV {
        RawVertexKind kind = RawVertexKind::REAL;
        Vertex orig = NIL_U32;
        OccID occ = 0;
    };

    struct BE {
        u32 a = 0, b = 0;
        RawEdgeKind kind = RawEdgeKind::CORE_REAL;
        BridgeRef br = 0;
        u8 side = 0;
    };

    vector<BV> V;
    vector<BE> E;
    unordered_map<OccID, vector<OccID>> allocNbr;
    unordered_map<OccID, vector<u32>> corePatchLocalEids;
};

struct BuildCtx {
    RawSkeletonBuilder B;
    unordered_map<RawVID, u32> mapV;
    unordered_map<RawEID, u32> mapE;
    unordered_set<Vertex> seenRealOrig;
};

struct RawUpdateCtx {
    unordered_set<RawSkelID> dirtySkeletons;
    unordered_set<OccID> dirtyOccurrences;
};

// ============================================================
// Generic helpers
// ============================================================

inline RawVID other_end(const RawEdge& e, RawVID u) {
    assert(e.a == u || e.b == u);
    return (e.a == u ? e.b : e.a);
}

u32 copy_old_vertex(BuildCtx& C,
                    const RawEngine& RE,
                    RawVID oldv,
                    bool checkRealUnique = true)
{
    auto it = C.mapV.find(oldv);
    if (it != C.mapV.end()) return it->second;

    const RawVertex& RV = RE.V.get(oldv);
    if (checkRealUnique && RV.kind == RawVertexKind::REAL) {
        bool ok = C.seenRealOrig.insert(RV.orig).second;
        assert(ok);
    }

    u32 idx = (u32)C.B.V.size();
    C.B.V.push_back(RawSkeletonBuilder::BV{
        .kind = RV.kind,
        .orig = RV.orig,
        .occ = RV.occ,
    });
    C.mapV.emplace(oldv, idx);
    return idx;
}

u32 copy_old_edge(BuildCtx& C,
                  const RawEngine& RE,
                  RawEID olde)
{
    auto it = C.mapE.find(olde);
    if (it != C.mapE.end()) return it->second;

    const RawEdge& e = RE.E.get(olde);
    assert(C.mapV.count(e.a));
    assert(C.mapV.count(e.b));

    u32 a = C.mapV.at(e.a);
    u32 b = C.mapV.at(e.b);
    assert(a != b);

    u32 idx = (u32)C.B.E.size();
    C.B.E.push_back(RawSkeletonBuilder::BE{
        .a = a,
        .b = b,
        .kind = e.kind,
        .br = e.br,
        .side = e.side,
    });
    C.mapE.emplace(olde, idx);
    return idx;
}

void remap_occ_meta(BuildCtx& C,
                    const RawEngine& RE,
                    OccID occ,
                    const vector<RawEID>& oldCoreEdges)
{
    const RawOccRecord& O = RE.occ.get(occ);
    C.B.allocNbr[occ] = O.allocNbr;

    vector<u32> ce;
    ce.reserve(oldCoreEdges.size());
    for (RawEID olde : oldCoreEdges) {
        auto it = C.mapE.find(olde);
        if (it != C.mapE.end()) ce.push_back(it->second);
    }
    C.B.corePatchLocalEids[occ] = std::move(ce);
}

struct LabeledComponents {
    unordered_map<RawVID, int> compId;
    vector<vector<RawVID>> compVerts;
};

LabeledComponents label_components_without_blocked(const RawEngine& RE,
                                                   const RawSkeleton& S,
                                                   const unordered_set<RawVID>& blocked)
{
    LabeledComponents out;
    for (RawVID start : S.verts) {
        if (blocked.count(start)) continue;
        if (out.compId.count(start)) continue;

        int cid = (int)out.compVerts.size();
        out.compVerts.push_back({});

        deque<RawVID> dq;
        dq.push_back(start);
        out.compId[start] = cid;

        while (!dq.empty()) {
            RawVID u = dq.front();
            dq.pop_front();
            out.compVerts[cid].push_back(u);

            for (RawEID eid : RE.V.get(u).adj) {
                RawVID v = other_end(RE.E.get(eid), u);
                if (blocked.count(v)) continue;
                if (!out.compId.count(v)) {
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
    RawVID c = O.centerV;

    unordered_set<RawVID> seen;
    vector<RawVID> out;

    auto add = [&](RawVID v) {
        if (seen.insert(v).second) out.push_back(v);
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

RawVID find_real_orig_in_skeleton(const RawEngine& RE,
                                  const RawSkeleton& S,
                                  Vertex ov)
{
    RawVID ans = 0;
    bool found = false;
    for (RawVID v : S.verts) {
        const RawVertex& RV = RE.V.get(v);
        if (RV.kind == RawVertexKind::REAL && RV.orig == ov) {
            assert(!found);
            found = true;
            ans = v;
        }
    }
    assert(found);
    return ans;
}

void retire_skeleton_contents(RawEngine& RE, RawSkelID sid) {
    RawSkeleton& S = RE.skel.get(sid);
    for (RawVID v : S.verts) RE.V.retire(v);
    for (RawEID e : S.edges) RE.E.retire(e);
    S.verts.clear();
    S.edges.clear();
    S.hostedOcc.clear();
}

void commit_skeleton(RawEngine& RE,
                     RawSkelID sid,
                     RawSkeletonBuilder&& B,
                     RawUpdateCtx& U)
{
    if (!RE.skel.get(sid).verts.empty() || !RE.skel.get(sid).edges.empty()) {
        retire_skeleton_contents(RE, sid);
    }

    RawSkeleton& S = RE.skel.get(sid);
    vector<RawVID> mapV(B.V.size(), 0);
    for (u32 i = 0; i < B.V.size(); ++i) {
        mapV[i] = RE.V.alloc(RawVertex{
            .kind = B.V[i].kind,
            .orig = B.V[i].orig,
            .occ = B.V[i].occ,
            .adj = {},
        });
        S.verts.push_back(mapV[i]);
    }

    vector<RawEID> mapE(B.E.size(), 0);
    for (u32 i = 0; i < B.E.size(); ++i) {
        RawVID a = mapV[B.E[i].a];
        RawVID b = mapV[B.E[i].b];
        mapE[i] = RE.E.alloc(RawEdge{
            .a = a,
            .b = b,
            .kind = B.E[i].kind,
            .br = B.E[i].br,
            .side = B.E[i].side,
        });
        S.edges.push_back(mapE[i]);
        RE.V.get(a).adj.push_back(mapE[i]);
        RE.V.get(b).adj.push_back(mapE[i]);
    }

    S.hostedOcc.clear();
    for (RawVID v : S.verts) {
        const RawVertex& RV = RE.V.get(v);
        if (RV.kind != RawVertexKind::OCC_CENTER) continue;

        OccID id = RV.occ;
        S.hostedOcc.push_back(id);

        RawOccRecord& O = RE.occ.get(id);
        O.orig = RV.orig;
        O.hostSkel = sid;
        O.centerV = v;

        auto itNbr = B.allocNbr.find(id);
        O.allocNbr = (itNbr == B.allocNbr.end() ? vector<OccID>{} : itNbr->second);

        auto itEdge = B.corePatchLocalEids.find(id);
        O.corePatchEdges.clear();
        if (itEdge != B.corePatchLocalEids.end()) {
            for (u32 le : itEdge->second) {
                O.corePatchEdges.push_back(mapE[le]);
            }
        }

        U.dirtyOccurrences.insert(id);
    }

    U.dirtySkeletons.insert(sid);
}

// ============================================================
// Validators
// ============================================================

void assert_builder_basic(const RawSkeletonBuilder& B) {
    for (const auto& e : B.E) {
        assert(e.a < B.V.size());
        assert(e.b < B.V.size());
        assert(e.a != e.b);
    }

    unordered_set<OccID> centerOcc;
    for (const auto& v : B.V) {
        if (v.kind == RawVertexKind::OCC_CENTER) {
            bool ok = centerOcc.insert(v.occ).second;
            assert(ok);
        }
    }

    for (const auto& [occ, _] : B.allocNbr) {
        assert(centerOcc.count(occ));
    }

    for (const auto& [occ, ce] : B.corePatchLocalEids) {
        assert(centerOcc.count(occ));
        for (u32 eid : ce) {
            assert(eid < B.E.size());
            assert(B.E[eid].kind == RawEdgeKind::CORE_REAL);
            const auto& e = B.E[eid];
            assert(B.V[e.a].kind == RawVertexKind::REAL);
            assert(B.V[e.b].kind == RawVertexKind::REAL);
        }
    }
}

void assert_skeleton_wellformed(const RawEngine& RE, RawSkelID sid) {
    const RawSkeleton& S = RE.skel.get(sid);

    unordered_set<RawVID> inV(S.verts.begin(), S.verts.end());
    unordered_set<RawEID> inE(S.edges.begin(), S.edges.end());

    for (RawVID v : S.verts) {
        const RawVertex& RV = RE.V.get(v);
        for (RawEID eid : RV.adj) {
            assert(inE.count(eid));
            const RawEdge& e = RE.E.get(eid);
            assert(e.a == v || e.b == v);
        }
    }

    for (RawEID eid : S.edges) {
        const RawEdge& e = RE.E.get(eid);
        assert(inV.count(e.a));
        assert(inV.count(e.b));
        assert(e.a != e.b);
    }

    unordered_set<OccID> seenOcc;
    for (OccID occ : S.hostedOcc) {
        assert(seenOcc.insert(occ).second);
        const RawOccRecord& O = RE.occ.get(occ);
        assert(O.hostSkel == sid);
        assert(inV.count(O.centerV));
        assert(RE.V.get(O.centerV).kind == RawVertexKind::OCC_CENTER);
        assert(RE.V.get(O.centerV).occ == occ);

        for (RawEID eid : O.corePatchEdges) {
            assert(inE.count(eid));
            assert(RE.E.get(eid).kind == RawEdgeKind::CORE_REAL);
        }
    }
}

void assert_occ_patch_consistent(const RawEngine& RE, OccID occ) {
    const RawOccRecord& O = RE.occ.get(occ);
    const RawSkeleton& S = RE.skel.get(O.hostSkel);
    unordered_set<RawVID> inV(S.verts.begin(), S.verts.end());
    unordered_set<RawEID> inE(S.edges.begin(), S.edges.end());

    RawVID c = O.centerV;
    assert(inV.count(c));
    assert(RE.V.get(c).kind == RawVertexKind::OCC_CENTER);
    assert(RE.V.get(c).occ == occ);

    for (RawEID eid : RE.V.get(c).adj) {
        assert(inE.count(eid));
        const RawEdge& e = RE.E.get(eid);
        assert(e.kind == RawEdgeKind::REAL_PORT || e.kind == RawEdgeKind::BRIDGE_PORT);
    }

    unordered_set<OccID> seenNbr;
    for (OccID nbr : O.allocNbr) {
        assert(nbr != occ);
        bool ok = seenNbr.insert(nbr).second;
        assert(ok);
        assert(RE.occ.get(nbr).orig == O.orig);
    }

    for (RawEID eid : O.corePatchEdges) {
        assert(inE.count(eid));
        const RawEdge& e = RE.E.get(eid);
        assert(e.kind == RawEdgeKind::CORE_REAL);
        assert(inV.count(e.a));
        assert(inV.count(e.b));
        assert(RE.V.get(e.a).kind == RawVertexKind::REAL);
        assert(RE.V.get(e.b).kind == RawVertexKind::REAL);
        assert(e.a != c && e.b != c);
    }
}

#ifndef NDEBUG
void debug_validate_skeleton_and_hosted(const RawEngine& RE, RawSkelID sid) {
    if (sid == NIL_U32) return;
    const RawSkeleton& S = RE.skel.get(sid);
    assert_skeleton_wellformed(RE, sid);
    for (OccID occ : S.hostedOcc) {
        assert_occ_patch_consistent(RE, occ);
    }
}
#else
inline void debug_validate_skeleton_and_hosted(const RawEngine&, RawSkelID) {}
#endif

// ============================================================
// Isolate
// ============================================================

struct TinyEdge {
    LVertex a = 0, b = 0;
};

struct TinyGraph {
    vector<Vertex> orig; // local -> original
    vector<TinyEdge> edges;
};

struct IsoPort {
    enum Kind : u8 {
        REAL = 0,
        BRIDGE = 1,
    } kind = REAL;

    Vertex attachOrig = NIL_U32;
    BridgeRef br = 0;
    u8 side = 0;
};

struct IsolatePrepared {
    OccID occ = 0;
    Vertex orig = NIL_U32;
    vector<OccID> allocNbr;
    vector<IsoPort> ports;
    TinyGraph core;
};

struct IsolateVertexResult {
    RawSkelID occSkel = NIL_U32;
    RawSkelID residualSkel = NIL_U32;
};

IsolatePrepared prepare_isolate_neighborhood(const RawEngine& RE,
                                            RawSkelID sid,
                                            OccID occ)
{
    const RawOccRecord& O = RE.occ.get(occ);
    assert(O.hostSkel == sid);

    RawVID c = O.centerV;
    assert(RE.V.get(c).kind == RawVertexKind::OCC_CENTER);
    assert(RE.V.get(c).orig == O.orig);

    IsolatePrepared out;
    out.occ = occ;
    out.orig = O.orig;
    out.allocNbr = O.allocNbr;
    sort(out.allocNbr.begin(), out.allocNbr.end());
    out.allocNbr.erase(unique(out.allocNbr.begin(), out.allocNbr.end()), out.allocNbr.end());

    unordered_set<u64> seenPort;
    auto port_key = [](Vertex attach, BridgeRef br, u8 side, u8 kind) -> u64 {
        u64 k = attach;
        k = (k << 32) ^ br;
        k = (k << 8) ^ side;
        k = (k << 8) ^ kind;
        return k;
    };

    for (RawEID eid : RE.V.get(c).adj) {
        const RawEdge& e = RE.E.get(eid);
        RawVID x = other_end(e, c);

        if (e.kind == RawEdgeKind::REAL_PORT) {
            Vertex ov = RE.V.get(x).orig;
            u64 key = port_key(ov, 0, 0, 0);
            if (seenPort.insert(key).second) {
                out.ports.push_back(IsoPort{
                    .kind = IsoPort::REAL,
                    .attachOrig = ov,
                });
            }
        } else if (e.kind == RawEdgeKind::BRIDGE_PORT) {
            Vertex ov = RE.V.get(x).orig;
            u64 key = port_key(ov, e.br, e.side, 1);
            if (seenPort.insert(key).second) {
                out.ports.push_back(IsoPort{
                    .kind = IsoPort::BRIDGE,
                    .attachOrig = ov,
                    .br = e.br,
                    .side = e.side,
                });
            }
        } else {
            assert(false);
        }
    }

    // Keep multiedges: do NOT deduplicate by original pair here.
    unordered_map<Vertex, LVertex> idOf;
    auto get_tid = [&](Vertex ov) -> LVertex {
        auto it = idOf.find(ov);
        if (it != idOf.end()) return it->second;
        LVertex id = (LVertex)out.core.orig.size();
        out.core.orig.push_back(ov);
        idOf.emplace(ov, id);
        return id;
    };

    for (RawEID eid : O.corePatchEdges) {
        const RawEdge& e = RE.E.get(eid);
        assert(e.kind == RawEdgeKind::CORE_REAL);
        Vertex oa = RE.V.get(e.a).orig;
        Vertex ob = RE.V.get(e.b).orig;
        if (oa == ob) continue;

        LVertex a = get_tid(oa);
        LVertex b = get_tid(ob);
        out.core.edges.push_back(TinyEdge{a, b});
    }

    return out;
}

RawSkeletonBuilder build_isolated_occ_builder(const IsolatePrepared& in) {
    RawSkeletonBuilder B;
    unordered_map<Vertex, u32> vid;

    auto get_vid = [&](Vertex ov, RawVertexKind kind, OccID occ = 0) -> u32 {
        auto it = vid.find(ov);
        if (it != vid.end()) return it->second;
        u32 idx = (u32)B.V.size();
        B.V.push_back(RawSkeletonBuilder::BV{.kind = kind, .orig = ov, .occ = occ});
        vid.emplace(ov, idx);
        return idx;
    };

    u32 c = get_vid(in.orig, RawVertexKind::OCC_CENTER, in.occ);
    B.allocNbr[in.occ] = in.allocNbr;

    for (const IsoPort& p : in.ports) {
        u32 x = get_vid(p.attachOrig, RawVertexKind::REAL);
        RawEdgeKind k = (p.kind == IsoPort::REAL ? RawEdgeKind::REAL_PORT : RawEdgeKind::BRIDGE_PORT);
        B.E.push_back(RawSkeletonBuilder::BE{.a = c, .b = x, .kind = k, .br = p.br, .side = p.side});
    }

    vector<u32> coreLocal;
    for (const TinyEdge& e : in.core.edges) {
        Vertex oa = in.core.orig[e.a];
        Vertex ob = in.core.orig[e.b];
        if (oa == ob) continue;
        u32 a = get_vid(oa, RawVertexKind::REAL);
        u32 b = get_vid(ob, RawVertexKind::REAL);
        u32 eid = (u32)B.E.size();
        B.E.push_back(RawSkeletonBuilder::BE{.a = a, .b = b, .kind = RawEdgeKind::CORE_REAL});
        coreLocal.push_back(eid);
    }
    B.corePatchLocalEids[in.occ] = std::move(coreLocal);
    return B;
}

RawSkeletonBuilder build_residual_after_isolate(const RawEngine& RE,
                                                RawSkelID sid,
                                                OccID removedOcc)
{
    const RawSkeleton& S = RE.skel.get(sid);
    const RawOccRecord& dead = RE.occ.get(removedOcc);
    RawVID deadCenter = dead.centerV;

    RawSkeletonBuilder B;
    unordered_set<RawVID> keepV;
    unordered_set<RawEID> keepE;
    vector<OccID> survivors;

    for (OccID id : S.hostedOcc) {
        if (id == removedOcc) continue;
        survivors.push_back(id);
    }

    for (OccID id : survivors) {
        const RawOccRecord& O = RE.occ.get(id);
        keepV.insert(O.centerV);

        for (RawEID eid : RE.V.get(O.centerV).adj) {
            const RawEdge& e = RE.E.get(eid);
            if (e.a == deadCenter || e.b == deadCenter) continue;
            keepE.insert(eid);
            keepV.insert(e.a);
            keepV.insert(e.b);
        }
        for (RawEID eid : O.corePatchEdges) {
            const RawEdge& e = RE.E.get(eid);
            if (e.a == deadCenter || e.b == deadCenter) continue;
            keepE.insert(eid);
            keepV.insert(e.a);
            keepV.insert(e.b);
        }
    }

    BuildCtx C;
    for (RawVID oldv : S.verts) {
        if (!keepV.count(oldv)) continue;
        copy_old_vertex(C, RE, oldv, true);
    }
    for (RawEID olde : S.edges) {
        if (!keepE.count(olde)) continue;
        copy_old_edge(C, RE, olde);
    }
    for (OccID id : survivors) {
        remap_occ_meta(C, RE, id, RE.occ.get(id).corePatchEdges);
    }
    return std::move(C.B);
}

IsolateVertexResult isolate_vertex(RawEngine& RE,
                                   RawSkelID sid,
                                   OccID occ,
                                   RawUpdateCtx& U)
{
    IsolatePrepared prep = prepare_isolate_neighborhood(RE, sid, occ);
    RawSkeletonBuilder occB = build_isolated_occ_builder(prep);
    RawSkeletonBuilder residualB = build_residual_after_isolate(RE, sid, occ);

    assert_builder_basic(occB);
    assert_builder_basic(residualB);

    RawSkelID sidOcc = RE.skel.alloc(RawSkeleton{});
    commit_skeleton(RE, sidOcc, std::move(occB), U);
    commit_skeleton(RE, sid, std::move(residualB), U);
    debug_validate_skeleton_and_hosted(RE, sidOcc);
    debug_validate_skeleton_and_hosted(RE, sid);

    return IsolateVertexResult{.occSkel = sidOcc, .residualSkel = sid};
}

// ============================================================
// Split
// ============================================================

struct SepSidePrepared {
    vector<RawVID> innerVerts;
    vector<RawEID> innerEdges;
    vector<RawEID> boundaryEdges;
    vector<OccID> hostedOcc;
    bool useA = false;
    bool useB = false;
};

struct SplitPrepared {
    Vertex aOrig = NIL_U32;
    Vertex bOrig = NIL_U32;
    RawVID aV = 0;
    RawVID bV = 0;
    vector<SepSidePrepared> side;
};

struct SplitChildInfo {
    RawSkelID sid = NIL_U32;
    vector<OccID> hostedOcc;
    bool boundaryOnly = false;
};

struct SplitSeparationPairResult {
    Vertex aOrig = NIL_U32;
    Vertex bOrig = NIL_U32;
    RawSkelID reusedSid = NIL_U32;
    vector<SplitChildInfo> child;
};

bool side_is_boundary_only(const SepSidePrepared& side) {
    return side.innerVerts.empty() && side.hostedOcc.empty();
}

SplitPrepared prepare_split_separation_pair(const RawEngine& RE,
                                           RawSkelID sid,
                                           Vertex saOrig,
                                           Vertex sbOrig)
{
    const RawSkeleton& S = RE.skel.get(sid);
    SplitPrepared out;
    out.aOrig = saOrig;
    out.bOrig = sbOrig;
    out.aV = find_real_orig_in_skeleton(RE, S, saOrig);
    out.bV = find_real_orig_in_skeleton(RE, S, sbOrig);

    RawVID aV = out.aV;
    RawVID bV = out.bV;
    auto is_sep = [&](RawVID v) { return v == aV || v == bV; };

    unordered_map<RawVID, int> compId;
    vector<vector<RawVID>> compVerts;

    for (RawVID start : S.verts) {
        if (is_sep(start)) continue;
        if (compId.count(start)) continue;
        int cid = (int)compVerts.size();
        compVerts.push_back({});
        deque<RawVID> dq;
        dq.push_back(start);
        compId[start] = cid;
        while (!dq.empty()) {
            RawVID u = dq.front();
            dq.pop_front();
            compVerts[cid].push_back(u);
            for (RawEID eid : RE.V.get(u).adj) {
                RawVID v = other_end(RE.E.get(eid), u);
                if (is_sep(v)) continue;
                if (!compId.count(v)) {
                    compId[v] = cid;
                    dq.push_back(v);
                }
            }
        }
    }

    out.side.resize(compVerts.size());
    for (int cid = 0; cid < (int)compVerts.size(); ++cid) {
        auto& side = out.side[cid];
        side.innerVerts = compVerts[cid];
        for (RawVID v : compVerts[cid]) {
            const RawVertex& RV = RE.V.get(v);
            if (RV.kind == RawVertexKind::OCC_CENTER) side.hostedOcc.push_back(RV.occ);
        }
    }

    int directABSide = -1;
    auto ensure_direct_ab_side = [&]() -> int {
        if (directABSide != -1) return directABSide;
        directABSide = (int)out.side.size();
        out.side.push_back(SepSidePrepared{});
        out.side[directABSide].useA = true;
        out.side[directABSide].useB = true;
        return directABSide;
    };

    for (RawEID eid : S.edges) {
        const RawEdge& e = RE.E.get(eid);
        bool aSep = is_sep(e.a);
        bool bSep = is_sep(e.b);

        if (!aSep && !bSep) {
            int ca = compId.at(e.a);
            int cb = compId.at(e.b);
            assert(ca == cb);
            out.side[ca].innerEdges.push_back(eid);
            continue;
        }

        if (aSep && bSep) {
            int sidx = ensure_direct_ab_side();
            out.side[sidx].boundaryEdges.push_back(eid);
            continue;
        }

        RawVID non = (aSep ? e.b : e.a);
        int cid = compId.at(non);
        out.side[cid].boundaryEdges.push_back(eid);

        if (aSep) {
            if (e.a == aV) out.side[cid].useA = true;
            else out.side[cid].useB = true;
        } else {
            if (e.b == aV) out.side[cid].useA = true;
            else out.side[cid].useB = true;
        }
    }

    for (int i = 0; i < (int)out.side.size(); ++i) {
        if (i == directABSide) continue;
        auto& side = out.side[i];
        if (!side.innerVerts.empty() || !side.hostedOcc.empty()) {
            assert(side.useA && side.useB);
        }
    }

    return out;
}

RawSkeletonBuilder build_child_after_sep_split(const RawEngine& RE,
                                              const SplitPrepared& P,
                                              u32 sideIdx)
{
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
    return std::move(C.B);
}

SplitSeparationPairResult split_separation_pair(RawEngine& RE,
                                                RawSkelID sid,
                                                Vertex saOrig,
                                                Vertex sbOrig,
                                                RawUpdateCtx& U)
{
    SplitPrepared prep = prepare_split_separation_pair(RE, sid, saOrig, sbOrig);
    vector<RawSkeletonBuilder> builders;
    builders.reserve(prep.side.size());
    for (u32 i = 0; i < prep.side.size(); ++i) {
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
    commit_skeleton(RE, sid, std::move(builders[0]), U);
    debug_validate_skeleton_and_hosted(RE, sid);

    for (u32 i = 1; i < builders.size(); ++i) {
        RawSkelID nsid = RE.skel.alloc(RawSkeleton{});
        res.child[i].sid = nsid;
        res.child[i].hostedOcc = prep.side[i].hostedOcc;
        res.child[i].boundaryOnly = side_is_boundary_only(prep.side[i]);
        commit_skeleton(RE, nsid, std::move(builders[i]), U);
        debug_validate_skeleton_and_hosted(RE, nsid);
    }
    return res;
}

// ============================================================
// Join
// ============================================================

struct JoinSeparationPairResult {
    RawSkelID mergedSid = NIL_U32;
    RawSkelID retiredSid = NIL_U32;
};

RawSkeletonBuilder build_merged_after_sep_join(const RawEngine& RE,
                                              RawSkelID leftSid,
                                              RawSkelID rightSid,
                                              Vertex saOrig,
                                              Vertex sbOrig)
{
    const RawSkeleton& L = RE.skel.get(leftSid);
    const RawSkeleton& R = RE.skel.get(rightSid);

    RawVID lA = find_real_orig_in_skeleton(RE, L, saOrig);
    RawVID lB = find_real_orig_in_skeleton(RE, L, sbOrig);
    RawVID rA = find_real_orig_in_skeleton(RE, R, saOrig);
    RawVID rB = find_real_orig_in_skeleton(RE, R, sbOrig);

    BuildCtx C;

    // coalesce separator copies
    u32 a = (u32)C.B.V.size();
    C.B.V.push_back(RawSkeletonBuilder::BV{.kind = RawVertexKind::REAL, .orig = saOrig, .occ = 0});
    C.mapV.emplace(lA, a);
    C.mapV.emplace(rA, a);
    C.seenRealOrig.insert(saOrig);

    u32 b = (u32)C.B.V.size();
    C.B.V.push_back(RawSkeletonBuilder::BV{.kind = RawVertexKind::REAL, .orig = sbOrig, .occ = 0});
    C.mapV.emplace(lB, b);
    C.mapV.emplace(rB, b);
    C.seenRealOrig.insert(sbOrig);

    for (RawVID v : L.verts) {
        if (v == lA || v == lB) continue;
        copy_old_vertex(C, RE, v, true);
    }
    for (RawVID v : R.verts) {
        if (v == rA || v == rB) continue;
        copy_old_vertex(C, RE, v, true);
    }

    for (RawEID e : L.edges) copy_old_edge(C, RE, e);
    for (RawEID e : R.edges) copy_old_edge(C, RE, e);

    unordered_set<OccID> seenOcc;
    auto copy_occ_meta_from = [&](const RawSkeleton& S) {
        for (OccID occ : S.hostedOcc) {
            bool ok = seenOcc.insert(occ).second;
            assert(ok);
            remap_occ_meta(C, RE, occ, RE.occ.get(occ).corePatchEdges);
        }
    };
    copy_occ_meta_from(L);
    copy_occ_meta_from(R);

    return std::move(C.B);
}

JoinSeparationPairResult join_separation_pair(RawEngine& RE,
                                              RawSkelID leftSid,
                                              RawSkelID rightSid,
                                              Vertex saOrig,
                                              Vertex sbOrig,
                                              RawUpdateCtx& U)
{
    assert(leftSid != rightSid);
    RawSkeletonBuilder B = build_merged_after_sep_join(RE, leftSid, rightSid, saOrig, sbOrig);
    assert_builder_basic(B);
    commit_skeleton(RE, leftSid, std::move(B), U);
    retire_skeleton_contents(RE, rightSid);
    RE.skel.retire(rightSid);
    debug_validate_skeleton_and_hosted(RE, leftSid);
    return JoinSeparationPairResult{.mergedSid = leftSid, .retiredSid = rightSid};
}

// ============================================================
// Integrate
// ============================================================

struct BoundaryMapEntry {
    Vertex childOrig = NIL_U32;
    Vertex parentOrig = NIL_U32;
};

struct IntegrateResult {
    RawSkelID mergedSid = NIL_U32;
    RawSkelID retiredSid = NIL_U32;
};

RawSkeletonBuilder build_merged_after_integrate(const RawEngine& RE,
                                                RawSkelID parentSid,
                                                RawSkelID childSid,
                                                const vector<BoundaryMapEntry>& bm)
{
    const RawSkeleton& P = RE.skel.get(parentSid);
    const RawSkeleton& Cc = RE.skel.get(childSid);

    BuildCtx C;
    unordered_map<Vertex, RawVID> parentBoundaryRaw;
    unordered_map<Vertex, RawVID> childBoundaryRaw;
    unordered_set<Vertex> seenParentBoundary;
    unordered_set<Vertex> seenChildBoundary;

    for (const auto& x : bm) {
        bool ok1 = seenParentBoundary.insert(x.parentOrig).second;
        bool ok2 = seenChildBoundary.insert(x.childOrig).second;
        assert(ok1 && ok2);

        parentBoundaryRaw.emplace(x.parentOrig, find_real_orig_in_skeleton(RE, P, x.parentOrig));
        childBoundaryRaw.emplace(x.childOrig, find_real_orig_in_skeleton(RE, Cc, x.childOrig));
    }

    // parent all copied first
    for (RawVID v : P.verts) {
        copy_old_vertex(C, RE, v, true);
    }

    // child boundary coalesced onto parent boundary copies
    for (const auto& x : bm) {
        RawVID childV = childBoundaryRaw.at(x.childOrig);
        RawVID parentV = parentBoundaryRaw.at(x.parentOrig);
        assert(C.mapV.count(parentV));
        C.mapV.emplace(childV, C.mapV.at(parentV));
    }

    auto is_child_boundary_real = [&](RawVID oldv) -> bool {
        const RawVertex& RV = RE.V.get(oldv);
        if (RV.kind != RawVertexKind::REAL) return false;
        auto it = childBoundaryRaw.find(RV.orig);
        return it != childBoundaryRaw.end() && it->second == oldv;
    };

    for (RawVID v : Cc.verts) {
        if (is_child_boundary_real(v)) continue;
        copy_old_vertex(C, RE, v, true);
    }

    for (RawEID e : P.edges) copy_old_edge(C, RE, e);
    for (RawEID e : Cc.edges) copy_old_edge(C, RE, e);

    unordered_set<OccID> seenOcc;
    auto copy_occ_meta_from = [&](const RawSkeleton& S) {
        for (OccID occ : S.hostedOcc) {
            bool ok = seenOcc.insert(occ).second;
            assert(ok);
            remap_occ_meta(C, RE, occ, RE.occ.get(occ).corePatchEdges);
        }
    };
    copy_occ_meta_from(P);
    copy_occ_meta_from(Cc);

    return std::move(C.B);
}

IntegrateResult integrate_skeleton(RawEngine& RE,
                                   RawSkelID parentSid,
                                   RawSkelID childSid,
                                   const vector<BoundaryMapEntry>& bm,
                                   RawUpdateCtx& U)
{
    assert(parentSid != childSid);
    RawSkeletonBuilder B = build_merged_after_integrate(RE, parentSid, childSid, bm);
    assert_builder_basic(B);
    commit_skeleton(RE, parentSid, std::move(B), U);
    retire_skeleton_contents(RE, childSid);
    RE.skel.retire(childSid);
    debug_validate_skeleton_and_hosted(RE, parentSid);
    return IntegrateResult{.mergedSid = parentSid, .retiredSid = childSid};
}

// ============================================================
// Planner
// ============================================================

enum class UpdJobKind : u8 {
    ENSURE_SOLE = 0,
    JOIN_PAIR = 1,
    INTEGRATE_CHILD = 2,
};

struct UpdJob {
    UpdJobKind kind = UpdJobKind::ENSURE_SOLE;
    OccID occ = 0;
    RawSkelID leftSid = NIL_U32;
    RawSkelID rightSid = NIL_U32;
    Vertex aOrig = NIL_U32;
    Vertex bOrig = NIL_U32;
    RawSkelID parentSid = NIL_U32;
    RawSkelID childSid = NIL_U32;
    vector<BoundaryMapEntry> bm;
};

struct RawPlannerCtx {
    OccID targetOcc = 0;
    unordered_set<OccID> keepOcc;
};

struct RawUpdaterHooks {
    function<void(const SplitSeparationPairResult&, deque<UpdJob>&)> afterSplit;
    function<void(const JoinSeparationPairResult&, deque<UpdJob>&)> afterJoin;
    function<void(const IntegrateResult&, deque<UpdJob>&)> afterIntegrate;
};

bool queue_has_ensure_sole(const deque<UpdJob>& q, OccID occ) {
    for (const UpdJob& j : q) {
        if (j.kind == UpdJobKind::ENSURE_SOLE && j.occ == occ) return true;
    }
    return false;
}

void enqueue_ensure_sole_once_back(deque<UpdJob>& q, OccID occ) {
    if (!queue_has_ensure_sole(q, occ)) {
        q.push_back(UpdJob{.kind = UpdJobKind::ENSURE_SOLE, .occ = occ});
    }
}

void prepend_jobs(deque<UpdJob>& q, vector<UpdJob> jobs) {
    for (auto it = jobs.rbegin(); it != jobs.rend(); ++it) {
        q.push_front(std::move(*it));
    }
}

bool valid_split_pair_for_support(const RawEngine& RE,
                                  const RawSkeleton& S,
                                  RawVID aV,
                                  RawVID bV,
                                  const vector<RawVID>& support)
{
    auto is_sep = [&](RawVID v) { return v == aV || v == bV; };
    unordered_map<RawVID, int> comp;
    vector<char> touchA, touchB;
    int compCnt = 0;

    for (RawVID start : S.verts) {
        if (is_sep(start)) continue;
        if (comp.count(start)) continue;
        int cid = compCnt++;
        touchA.push_back(false);
        touchB.push_back(false);
        deque<RawVID> dq;
        dq.push_back(start);
        comp[start] = cid;
        while (!dq.empty()) {
            RawVID u = dq.front();
            dq.pop_front();
            for (RawEID eid : RE.V.get(u).adj) {
                const RawEdge& e = RE.E.get(eid);
                RawVID v = other_end(e, u);
                if (v == aV) { touchA[cid] = true; continue; }
                if (v == bV) { touchB[cid] = true; continue; }
                if (!comp.count(v)) {
                    comp[v] = cid;
                    dq.push_back(v);
                }
            }
        }
    }

    bool directAB = false;
    for (RawEID eid : S.edges) {
        const RawEdge& e = RE.E.get(eid);
        if ((e.a == aV && e.b == bV) || (e.a == bV && e.b == aV)) {
            directAB = true;
            break;
        }
    }

    int supportComp = -1;
    for (RawVID v : support) {
        if (is_sep(v)) continue;
        auto it = comp.find(v);
        if (it == comp.end()) return false;
        if (supportComp == -1) supportComp = it->second;
        else if (supportComp != it->second) return false;
    }
    if (supportComp == -1) return false;

    for (int cid = 0; cid < compCnt; ++cid) {
        if (!touchA[cid] || !touchB[cid]) return false;
    }

    (void)directAB;
    // Reject pure direct a-b artifact loops: require at least two actual
    // non-separator components before allowing split.
    return compCnt >= 2;
}

optional<pair<Vertex, Vertex>> discover_split_pair_from_support(const RawEngine& RE,
                                                                OccID occ)
{
    const RawOccRecord& O = RE.occ.get(occ);
    const RawSkeleton& S = RE.skel.get(O.hostSkel);
    if (S.hostedOcc.size() != 1 || S.hostedOcc[0] != occ) return nullopt;

    vector<RawVID> support = collect_support_vertices(RE, occ);
    vector<pair<Vertex, RawVID>> realVs;
    for (RawVID v : S.verts) {
        const RawVertex& RV = RE.V.get(v);
        if (RV.kind == RawVertexKind::REAL) realVs.push_back({RV.orig, v});
    }
    sort(realVs.begin(), realVs.end(), [](auto& x, auto& y){ return x.first < y.first; });

    for (u32 i = 0; i < realVs.size(); ++i) {
        for (u32 j = i + 1; j < realVs.size(); ++j) {
            if (valid_split_pair_for_support(RE, S, realVs[i].second, realVs[j].second, support)) {
                return pair<Vertex, Vertex>{realVs[i].first, realVs[j].first};
            }
        }
    }
    return nullopt;
}

bool child_contains_occ(const SplitChildInfo& c, OccID occ) {
    for (OccID x : c.hostedOcc) if (x == occ) return true;
    return false;
}

bool child_intersects_keep_occ(const SplitChildInfo& c, const RawPlannerCtx& ctx) {
    if (ctx.keepOcc.empty()) return child_contains_occ(c, ctx.targetOcc);
    for (OccID x : c.hostedOcc) if (ctx.keepOcc.count(x)) return true;
    return false;
}

int choose_anchor_child(const SplitSeparationPairResult& res,
                        const RawPlannerCtx& ctx)
{
    for (int i = 0; i < (int)res.child.size(); ++i)
        if (child_contains_occ(res.child[i], ctx.targetOcc)) return i;
    for (int i = 0; i < (int)res.child.size(); ++i)
        if (child_intersects_keep_occ(res.child[i], ctx)) return i;
    for (int i = 0; i < (int)res.child.size(); ++i)
        if (!res.child[i].boundaryOnly) return i;
    return res.child.empty() ? -1 : 0;
}

RawUpdaterHooks make_basic_hooks_for_target(const RawPlannerCtx& ctx) {
    RawUpdaterHooks hooks;

    hooks.afterSplit = [ctx](const SplitSeparationPairResult& res, deque<UpdJob>& q) {
        if (res.child.empty()) return;
        int anchorIdx = choose_anchor_child(res, ctx);
        assert(anchorIdx >= 0);
        RawSkelID anchorSid = res.child[anchorIdx].sid;
        vector<UpdJob> jobs;

        for (int i = 0; i < (int)res.child.size(); ++i) {
            if (i == anchorIdx) continue;
            const SplitChildInfo& c = res.child[i];
            if (!c.boundaryOnly) continue;
            jobs.push_back(UpdJob{
                .kind = UpdJobKind::INTEGRATE_CHILD,
                .parentSid = anchorSid,
                .childSid = c.sid,
                .bm = {
                    BoundaryMapEntry{.childOrig = res.aOrig, .parentOrig = res.aOrig},
                    BoundaryMapEntry{.childOrig = res.bOrig, .parentOrig = res.bOrig},
                }
            });
        }

        for (int i = 0; i < (int)res.child.size(); ++i) {
            if (i == anchorIdx) continue;
            const SplitChildInfo& c = res.child[i];
            if (c.boundaryOnly) continue;
            if (!child_intersects_keep_occ(c, ctx)) continue;
            jobs.push_back(UpdJob{
                .kind = UpdJobKind::JOIN_PAIR,
                .leftSid = anchorSid,
                .rightSid = c.sid,
                .aOrig = res.aOrig,
                .bOrig = res.bOrig,
            });
        }

        jobs.push_back(UpdJob{.kind = UpdJobKind::ENSURE_SOLE, .occ = ctx.targetOcc});
        prepend_jobs(q, std::move(jobs));
    };

    hooks.afterJoin = [ctx](const JoinSeparationPairResult&, deque<UpdJob>& q) {
        enqueue_ensure_sole_once_back(q, ctx.targetOcc);
    };

    hooks.afterIntegrate = [ctx](const IntegrateResult&, deque<UpdJob>& q) {
        enqueue_ensure_sole_once_back(q, ctx.targetOcc);
    };

    return hooks;
}

void run_raw_local_updater(RawEngine& RE,
                           OccID targetOcc,
                           RawUpdateCtx& U,
                           RawUpdaterHooks* hooks = nullptr)
{
    deque<UpdJob> q;
    q.push_back(UpdJob{.kind = UpdJobKind::ENSURE_SOLE, .occ = targetOcc});

#ifndef NDEBUG
    size_t debugStepBudget = 200000;
#endif

    while (!q.empty()) {
#ifndef NDEBUG
        assert(debugStepBudget-- > 0);
#endif
        UpdJob job = std::move(q.front());
        q.pop_front();

        switch (job.kind) {
            case UpdJobKind::ENSURE_SOLE: {
                const RawOccRecord& O = RE.occ.get(job.occ);
                const RawSkeleton& S = RE.skel.get(O.hostSkel);

                if (S.hostedOcc.size() != 1 || S.hostedOcc[0] != job.occ) {
                    isolate_vertex(RE, O.hostSkel, job.occ, U);
                    q.push_front(UpdJob{.kind = UpdJobKind::ENSURE_SOLE, .occ = job.occ});
                    break;
                }

                auto sep = discover_split_pair_from_support(RE, job.occ);
                if (!sep.has_value()) break;

                auto [a, b] = *sep;
                SplitSeparationPairResult res = split_separation_pair(RE, O.hostSkel, a, b, U);
                if (hooks && hooks->afterSplit) hooks->afterSplit(res, q);
                else q.push_front(UpdJob{.kind = UpdJobKind::ENSURE_SOLE, .occ = job.occ});
                break;
            }

            case UpdJobKind::JOIN_PAIR: {
                JoinSeparationPairResult res = join_separation_pair(RE, job.leftSid, job.rightSid, job.aOrig, job.bOrig, U);
                if (hooks && hooks->afterJoin) hooks->afterJoin(res, q);
                else enqueue_ensure_sole_once_back(q, targetOcc);
                break;
            }

            case UpdJobKind::INTEGRATE_CHILD: {
                IntegrateResult res = integrate_skeleton(RE, job.parentSid, job.childSid, job.bm, U);
                if (hooks && hooks->afterIntegrate) hooks->afterIntegrate(res, q);
                else enqueue_ensure_sole_once_back(q, targetOcc);
                break;
            }
        }
    }
}

// ============================================================
// Test helpers
// ============================================================

OccID new_occ(RawEngine& RE, Vertex orig) {
    OccID id = RE.occ.alloc(RawOccRecord{.orig = orig});
    RE.occOfOrig[orig].push_back(id);
    return id;
}

RawSkelID new_skeleton(RawEngine& RE) {
    return RE.skel.alloc(RawSkeleton{});
}

void assert_engine_ok(const RawEngine& RE, const vector<RawSkelID>& sids, const vector<OccID>& occs) {
    for (RawSkelID sid : sids) {
        if (sid != NIL_U32) assert_skeleton_wellformed(RE, sid);
    }
    for (OccID occ : occs) {
        assert_occ_patch_consistent(RE, occ);
    }
}

// ============================================================
// Microcases
// ============================================================

void test_isolate() {
    RawEngine RE;
    RawUpdateCtx U;

    OccID occ17 = new_occ(RE, 5);
    OccID occ31 = new_occ(RE, 9);
    OccID occ23 = new_occ(RE, 5); // neighbor occurrence only in allocNbr metadata
    (void)occ23;

    RawSkelID sid = new_skeleton(RE);

    RawSkeletonBuilder B;
    // 0: REAL 2, 1: REAL 8, 2: REAL 7, 3: OCC 5(17), 4: REAL 6, 5: OCC 9(31)
    B.V = {
        {RawVertexKind::REAL, 2, 0},
        {RawVertexKind::REAL, 8, 0},
        {RawVertexKind::REAL, 7, 0},
        {RawVertexKind::OCC_CENTER, 5, occ17},
        {RawVertexKind::REAL, 6, 0},
        {RawVertexKind::OCC_CENTER, 9, occ31},
    };
    // occ17: ports to 2,8 and core 2-7,7-8,2-8
    B.E.push_back({3,0,RawEdgeKind::REAL_PORT});          // e0
    B.E.push_back({3,1,RawEdgeKind::BRIDGE_PORT,41,0});   // e1
    B.E.push_back({0,2,RawEdgeKind::CORE_REAL});          // e2
    B.E.push_back({2,1,RawEdgeKind::CORE_REAL});          // e3
    B.E.push_back({0,1,RawEdgeKind::CORE_REAL});          // e4
    // occ31: port to 6 and core 6-8
    B.E.push_back({5,4,RawEdgeKind::REAL_PORT});          // e5
    B.E.push_back({4,1,RawEdgeKind::CORE_REAL});          // e6

    B.allocNbr[occ17] = {occ23};
    B.corePatchLocalEids[occ17] = {2,3,4};
    B.allocNbr[occ31] = {};
    B.corePatchLocalEids[occ31] = {6};

    assert_builder_basic(B);
    commit_skeleton(RE, sid, std::move(B), U);
    assert_engine_ok(RE, {sid}, {occ17, occ31});

    IsolatePrepared prep = prepare_isolate_neighborhood(RE, sid, occ17);
    assert(prep.occ == occ17 && prep.orig == 5);
    assert(prep.allocNbr.size() == 1 && prep.allocNbr[0] == occ23);
    assert(prep.ports.size() == 2);
    assert(prep.core.orig.size() == 3);
    assert(prep.core.edges.size() == 3);

    IsolateVertexResult res = isolate_vertex(RE, sid, occ17, U);
    assert(res.residualSkel == sid);
    assert(res.occSkel != sid);
    assert_engine_ok(RE, {sid, res.occSkel}, {occ17, occ31});

    // residual should only keep occ31-side structure
    const RawSkeleton& SR = RE.skel.get(sid);
    assert(SR.hostedOcc.size() == 1 && SR.hostedOcc[0] == occ31);

    // isolated sid should only host occ17
    const RawSkeleton& SI = RE.skel.get(res.occSkel);
    assert(SI.hostedOcc.size() == 1 && SI.hostedOcc[0] == occ17);
}

void test_split_join() {
    RawEngine RE;
    RawUpdateCtx U;

    OccID occ31 = new_occ(RE, 9);
    OccID occ44 = new_occ(RE, 12);

    RawSkelID sid = new_skeleton(RE);

    RawSkeletonBuilder B;
    // 0:a=2, 1:b=8, 2:c9, 3:v6, 4:c12, 5:v10
    B.V = {
        {RawVertexKind::REAL, 2, 0},
        {RawVertexKind::REAL, 8, 0},
        {RawVertexKind::OCC_CENTER, 9, occ31},
        {RawVertexKind::REAL, 6, 0},
        {RawVertexKind::OCC_CENTER, 12, occ44},
        {RawVertexKind::REAL, 10, 0},
    };
    B.E.push_back({2,3,RawEdgeKind::REAL_PORT}); // e0
    B.E.push_back({0,3,RawEdgeKind::CORE_REAL}); // e1
    B.E.push_back({3,1,RawEdgeKind::CORE_REAL}); // e2

    B.E.push_back({4,5,RawEdgeKind::REAL_PORT}); // e3
    B.E.push_back({0,5,RawEdgeKind::CORE_REAL}); // e4
    B.E.push_back({5,1,RawEdgeKind::CORE_REAL}); // e5

    B.E.push_back({0,1,RawEdgeKind::CORE_REAL}); // e6 direct a-b

    B.allocNbr[occ31] = {};
    B.corePatchLocalEids[occ31] = {1,2};
    B.allocNbr[occ44] = {};
    B.corePatchLocalEids[occ44] = {4,5};

    commit_skeleton(RE, sid, std::move(B), U);
    assert_engine_ok(RE, {sid}, {occ31, occ44});

    SplitSeparationPairResult sp = split_separation_pair(RE, sid, 2, 8, U);
    assert(sp.child.size() == 3);

    vector<RawSkelID> sids;
    for (auto& c : sp.child) sids.push_back(c.sid);
    assert_engine_ok(RE, sids, {occ31, occ44});

    // identify target children by hosted occ
    RawSkelID sid31 = NIL_U32, sid44 = NIL_U32, sidAB = NIL_U32;
    for (auto& c : sp.child) {
        if (child_contains_occ(c, occ31)) sid31 = c.sid;
        else if (child_contains_occ(c, occ44)) sid44 = c.sid;
        else if (c.boundaryOnly) sidAB = c.sid;
    }
    assert(sid31 != NIL_U32 && sid44 != NIL_U32 && sidAB != NIL_U32);

    // join the two non-boundary children
    JoinSeparationPairResult jr = join_separation_pair(RE, sid31, sid44, 2, 8, U);
    assert(jr.mergedSid == sid31);
    assert_engine_ok(RE, {sid31, sidAB}, {occ31, occ44});

    // joined skeleton should host both occurrences
    const RawSkeleton& SJ = RE.skel.get(sid31);
    assert(SJ.hostedOcc.size() == 2);
}

void test_integrate() {
    RawEngine RE;
    RawUpdateCtx U;

    OccID occ31 = new_occ(RE, 9);
    OccID occ44 = new_occ(RE, 14);

    RawSkelID parentSid = new_skeleton(RE);
    RawSkelID childSid = new_skeleton(RE);

    // parent skeleton: boundary 6,8 plus occ31 via 2
    {
        RawSkeletonBuilder B;
        // 0:6, 1:8, 2:2, 3:c9
        B.V = {
            {RawVertexKind::REAL, 6, 0},
            {RawVertexKind::REAL, 8, 0},
            {RawVertexKind::REAL, 2, 0},
            {RawVertexKind::OCC_CENTER, 9, occ31},
        };
        B.E.push_back({3,2,RawEdgeKind::REAL_PORT}); // e0
        B.E.push_back({2,0,RawEdgeKind::CORE_REAL}); // e1
        B.E.push_back({0,1,RawEdgeKind::CORE_REAL}); // e2
        B.allocNbr[occ31] = {};
        B.corePatchLocalEids[occ31] = {1,2};
        commit_skeleton(RE, parentSid, std::move(B), U);
    }

    // child skeleton: boundary 6,8 plus occ44 via 11
    {
        RawSkeletonBuilder B;
        // 0:6, 1:8, 2:11, 3:c14
        B.V = {
            {RawVertexKind::REAL, 6, 0},
            {RawVertexKind::REAL, 8, 0},
            {RawVertexKind::REAL, 11, 0},
            {RawVertexKind::OCC_CENTER, 14, occ44},
        };
        B.E.push_back({3,2,RawEdgeKind::REAL_PORT}); // f0
        B.E.push_back({2,0,RawEdgeKind::CORE_REAL}); // f1
        B.E.push_back({2,1,RawEdgeKind::CORE_REAL}); // f2
        B.allocNbr[occ44] = {};
        B.corePatchLocalEids[occ44] = {1,2};
        commit_skeleton(RE, childSid, std::move(B), U);
    }

    assert_engine_ok(RE, {parentSid, childSid}, {occ31, occ44});

    vector<BoundaryMapEntry> bm = {
        {.childOrig = 6, .parentOrig = 6},
        {.childOrig = 8, .parentOrig = 8},
    };
    IntegrateResult ir = integrate_skeleton(RE, parentSid, childSid, bm, U);
    assert(ir.mergedSid == parentSid);
    assert_engine_ok(RE, {parentSid}, {occ31, occ44});
    const RawSkeleton& SM = RE.skel.get(parentSid);
    assert(SM.hostedOcc.size() == 2);
}

void test_planner() {
    RawEngine RE;
    RawUpdateCtx U;

    OccID occ31 = new_occ(RE, 9);
    OccID occ44 = new_occ(RE, 12);

    RawSkelID sid = new_skeleton(RE);
    RawSkeletonBuilder B;
    // same as split microcase
    B.V = {
        {RawVertexKind::REAL, 2, 0},
        {RawVertexKind::REAL, 8, 0},
        {RawVertexKind::OCC_CENTER, 9, occ31},
        {RawVertexKind::REAL, 6, 0},
        {RawVertexKind::OCC_CENTER, 12, occ44},
        {RawVertexKind::REAL, 10, 0},
    };
    B.E.push_back({2,3,RawEdgeKind::REAL_PORT});
    B.E.push_back({0,3,RawEdgeKind::CORE_REAL});
    B.E.push_back({3,1,RawEdgeKind::CORE_REAL});
    B.E.push_back({4,5,RawEdgeKind::REAL_PORT});
    B.E.push_back({0,5,RawEdgeKind::CORE_REAL});
    B.E.push_back({5,1,RawEdgeKind::CORE_REAL});
    B.E.push_back({0,1,RawEdgeKind::CORE_REAL});
    B.allocNbr[occ31] = {};
    B.corePatchLocalEids[occ31] = {1,2};
    B.allocNbr[occ44] = {};
    B.corePatchLocalEids[occ44] = {4,5};
    commit_skeleton(RE, sid, std::move(B), U);

    RawPlannerCtx ctx;
    ctx.targetOcc = occ31;
    ctx.keepOcc = {occ31};
    RawUpdaterHooks hooks = make_basic_hooks_for_target(ctx);
    run_raw_local_updater(RE, occ31, U, &hooks);

    // target still valid and planner terminates
    assert_occ_patch_consistent(RE, occ31);
}



// ============================================================
// Extra stress / fuzz helpers
// ============================================================

struct NormalizedPrep {
    Vertex orig = NIL_U32;
    vector<OccID> allocNbr;
    vector<tuple<int, Vertex, BridgeRef, u8>> ports;   // kind, attach, br, side
    vector<pair<Vertex, Vertex>> core;                 // sorted endpoint pairs, duplicates kept

    bool operator==(const NormalizedPrep& rhs) const {
        return orig == rhs.orig && allocNbr == rhs.allocNbr && ports == rhs.ports && core == rhs.core;
    }
};

NormalizedPrep normalize_prep(const IsolatePrepared& p) {
    NormalizedPrep out;
    out.orig = p.orig;
    out.allocNbr = p.allocNbr;
    sort(out.allocNbr.begin(), out.allocNbr.end());

    for (const auto& port : p.ports) {
        out.ports.push_back({(int)port.kind, port.attachOrig, port.br, port.side});
    }
    sort(out.ports.begin(), out.ports.end());

    for (const auto& e : p.core.edges) {
        Vertex a = p.core.orig[e.a];
        Vertex b = p.core.orig[e.b];
        if (a > b) swap(a, b);
        out.core.push_back({a, b});
    }
    sort(out.core.begin(), out.core.end());
    return out;
}

struct GeneratedCase {
    RawSkelID sid = NIL_U32;
    vector<OccID> occs;
    bool hasDirectAB = false;
    unordered_map<OccID, NormalizedPrep> expected;
};

GeneratedCase make_random_split_case(RawEngine& RE, RawUpdateCtx& U, std::mt19937& rng) {
    std::uniform_int_distribution<int> sideCntDist(2, 4);
    std::uniform_int_distribution<int> lenDist(1, 2);
    std::uniform_int_distribution<int> portKindDist(0, 1);
    std::bernoulli_distribution directABDist(0.5);

    GeneratedCase gc;
    gc.sid = new_skeleton(RE);
    gc.hasDirectAB = directABDist(rng);

    RawSkeletonBuilder B;

    // fixed separators 2 and 8
    u32 sepA = (u32)B.V.size(); B.V.push_back({RawVertexKind::REAL, 2, 0});
    u32 sepB = (u32)B.V.size(); B.V.push_back({RawVertexKind::REAL, 8, 0});

    int sideCnt = sideCntDist(rng);
    u32 bridgeRefBase = 1000;

    for (int i = 0; i < sideCnt; ++i) {
        Vertex occOrig = 100 + i;
        OccID occ = new_occ(RE, occOrig);
        gc.occs.push_back(occ);

        // side shape: 2-x-b or 2-x-y-b
        Vertex xOrig = 1000 + i * 10 + 1;
        Vertex yOrig = 1000 + i * 10 + 2;

        u32 cx = (u32)B.V.size(); B.V.push_back({RawVertexKind::REAL, xOrig, 0});
        u32 cy = NIL_U32;
        bool len2 = (lenDist(rng) == 2);
        if (len2) {
            cy = (u32)B.V.size(); B.V.push_back({RawVertexKind::REAL, yOrig, 0});
        }
        u32 cc = (u32)B.V.size(); B.V.push_back({RawVertexKind::OCC_CENTER, occOrig, occ});

        // port from center to chosen attach vertex
        bool useBridgePort = (portKindDist(rng) == 1);
        u32 attach = len2 ? cy : cx;
        if (useBridgePort) {
            B.E.push_back({cc, attach, RawEdgeKind::BRIDGE_PORT, bridgeRefBase + (u32)i, (u8)(i & 1)});
        } else {
            B.E.push_back({cc, attach, RawEdgeKind::REAL_PORT, 0, 0});
        }

        vector<u32> coreE;
        u32 e = (u32)B.E.size(); B.E.push_back({sepA, cx, RawEdgeKind::CORE_REAL, 0, 0}); coreE.push_back(e);
        if (len2) {
            e = (u32)B.E.size(); B.E.push_back({cx, cy, RawEdgeKind::CORE_REAL, 0, 0}); coreE.push_back(e);
            e = (u32)B.E.size(); B.E.push_back({cy, sepB, RawEdgeKind::CORE_REAL, 0, 0}); coreE.push_back(e);
        } else {
            e = (u32)B.E.size(); B.E.push_back({cx, sepB, RawEdgeKind::CORE_REAL, 0, 0}); coreE.push_back(e);
        }

        B.allocNbr[occ] = {};
        B.corePatchLocalEids[occ] = coreE;
    }

    if (gc.hasDirectAB) {
        B.E.push_back({sepA, sepB, RawEdgeKind::CORE_REAL, 0, 0});
    }

    commit_skeleton(RE, gc.sid, std::move(B), U);
    assert_engine_ok(RE, {gc.sid}, gc.occs);

    for (OccID occ : gc.occs) {
        gc.expected.emplace(occ, normalize_prep(prepare_isolate_neighborhood(RE, gc.sid, occ)));
    }
    return gc;
}

void test_isolate_multiedge() {
    RawEngine RE;
    RawUpdateCtx U;

    OccID occ = new_occ(RE, 5);
    RawSkelID sid = new_skeleton(RE);

    RawSkeletonBuilder B;
    // 0:2, 1:8, 2:c5
    B.V = {
        {RawVertexKind::REAL, 2, 0},
        {RawVertexKind::REAL, 8, 0},
        {RawVertexKind::OCC_CENTER, 5, occ},
    };
    B.E.push_back({2, 0, RawEdgeKind::REAL_PORT}); // port
    B.E.push_back({0, 1, RawEdgeKind::CORE_REAL}); // parallel core 1
    B.E.push_back({0, 1, RawEdgeKind::CORE_REAL}); // parallel core 2
    B.allocNbr[occ] = {};
    B.corePatchLocalEids[occ] = {1, 2};
    commit_skeleton(RE, sid, std::move(B), U);

    IsolatePrepared p = prepare_isolate_neighborhood(RE, sid, occ);
    assert(p.core.orig.size() == 2);
    assert(p.core.edges.size() == 2); // multiedges preserved
}

void test_split_join_roundtrip_many() {
    const vector<u32> seeds = {123456u, 123457u, 700001u};
    for (u32 seed : seeds) {
        std::mt19937 rng(seed);
        for (int it = 0; it < 160; ++it) {
            RawEngine RE;
            RawUpdateCtx U;
            GeneratedCase gc = make_random_split_case(RE, U, rng);

            SplitSeparationPairResult sp = split_separation_pair(RE, gc.sid, 2, 8, U);
            assert(sp.child.size() >= 2);

            // choose anchor child containing first occurrence
            RawSkelID anchor = NIL_U32;
            vector<RawSkelID> boundaryOnly;
            vector<RawSkelID> otherOccChildren;
            for (auto& c : sp.child) {
                if (child_contains_occ(c, gc.occs[0])) anchor = c.sid;
                else if (c.boundaryOnly) boundaryOnly.push_back(c.sid);
                else otherOccChildren.push_back(c.sid);
            }
            assert(anchor != NIL_U32);

            // integrate artifacts first
            for (RawSkelID sidAB : boundaryOnly) {
                IntegrateResult ir = integrate_skeleton(RE, anchor, sidAB,
                                                        {{2,2},{8,8}}, U);
                assert(ir.mergedSid == anchor);
            }

            // join remaining occurrence children into anchor
            for (RawSkelID sidX : otherOccChildren) {
                JoinSeparationPairResult jr = join_separation_pair(RE, anchor, sidX, 2, 8, U);
                assert(jr.mergedSid == anchor);
            }

            // all occs should now live in anchor and preserve isolate certificate
            assert_skeleton_wellformed(RE, anchor);
            const RawSkeleton& S = RE.skel.get(anchor);
            assert(S.hostedOcc.size() == gc.occs.size());
            for (OccID occ : gc.occs) {
                assert(RE.occ.get(occ).hostSkel == anchor);
                assert_occ_patch_consistent(RE, occ);
                NormalizedPrep got = normalize_prep(prepare_isolate_neighborhood(RE, anchor, occ));
                auto itexp = gc.expected.find(occ);
                assert(itexp != gc.expected.end());
                assert(got == itexp->second);
            }
        }
    }
}

void test_split_integrate_ensure_flow() {
    const vector<u32> seeds = {44001u, 44002u, 44003u, 44004u};
    for (u32 seed : seeds) {
        std::mt19937 rng(seed);
        for (int it = 0; it < 120; ++it) {
            RawEngine RE;
            RawUpdateCtx U;
            GeneratedCase gc = make_random_split_case(RE, U, rng);
            OccID target = gc.occs[0];

            SplitSeparationPairResult sp = split_separation_pair(RE, gc.sid, 2, 8, U);
            assert(sp.child.size() >= 2);

            RawSkelID anchor = NIL_U32;
            vector<RawSkelID> boundaryOnly;
            for (const auto& c : sp.child) {
                if (child_contains_occ(c, target)) anchor = c.sid;
                else if (c.boundaryOnly) boundaryOnly.push_back(c.sid);
            }
            assert(anchor != NIL_U32);

            for (RawSkelID sidAB : boundaryOnly) {
                IntegrateResult ir = integrate_skeleton(RE, anchor, sidAB, {{2,2},{8,8}}, U);
                assert(ir.mergedSid == anchor);
            }

            RawPlannerCtx ctx;
            ctx.targetOcc = target;
            ctx.keepOcc = {target};
            RawUpdaterHooks hooks = make_basic_hooks_for_target(ctx);
            run_raw_local_updater(RE, target, U, &hooks);

            const RawOccRecord& O = RE.occ.get(target);
            const RawSkeleton& S = RE.skel.get(O.hostSkel);
            assert(S.hostedOcc.size() == 1 && S.hostedOcc[0] == target);
            assert_occ_patch_consistent(RE, target);
            auto sep = discover_split_pair_from_support(RE, target);
            assert(!sep.has_value());
        }
    }
}

void test_regression_direct_ab_artifact_no_resplit() {
    std::mt19937 rng(44001);

    // Reproduce the previously failing stream position (seed=44001, iter=1).
    {
        RawEngine RE;
        RawUpdateCtx U;
        (void)make_random_split_case(RE, U, rng);
    }

    RawEngine RE;
    RawUpdateCtx U;
    GeneratedCase gc = make_random_split_case(RE, U, rng);
    OccID target = gc.occs[0];

    SplitSeparationPairResult sp = split_separation_pair(RE, gc.sid, 2, 8, U);
    RawSkelID anchor = NIL_U32;
    vector<RawSkelID> boundaryOnly;
    for (const auto& c : sp.child) {
        if (child_contains_occ(c, target)) anchor = c.sid;
        else if (c.boundaryOnly) boundaryOnly.push_back(c.sid);
    }
    assert(anchor != NIL_U32);

    for (RawSkelID sidAB : boundaryOnly) {
        IntegrateResult ir = integrate_skeleton(RE, anchor, sidAB, {{2,2},{8,8}}, U);
        assert(ir.mergedSid == anchor);
    }

    RawPlannerCtx ctx;
    ctx.targetOcc = target;
    ctx.keepOcc = {target};
    RawUpdaterHooks hooks = make_basic_hooks_for_target(ctx);
    run_raw_local_updater(RE, target, U, &hooks);

    const RawOccRecord& O = RE.occ.get(target);
    const RawSkeleton& S = RE.skel.get(O.hostSkel);
    assert(S.hostedOcc.size() == 1 && S.hostedOcc[0] == target);
    auto sep = discover_split_pair_from_support(RE, target);
    assert(!sep.has_value());
}

void test_planner_stress() {
    const vector<u32> seeds = {987654u, 987655u, 987656u};
    for (u32 seed : seeds) {
        std::mt19937 rng(seed);
        for (int it = 0; it < 180; ++it) {
            RawEngine RE;
            RawUpdateCtx U;
            GeneratedCase gc = make_random_split_case(RE, U, rng);
            OccID target = gc.occs[0];

            RawPlannerCtx ctx;
            ctx.targetOcc = target;

            // 50%: only keep target, 50%: keep all
            if ((it & 1) == 0) {
                ctx.keepOcc = {target};
            } else {
                for (OccID occ : gc.occs) ctx.keepOcc.insert(occ);
            }

            RawUpdaterHooks hooks = make_basic_hooks_for_target(ctx);
            run_raw_local_updater(RE, target, U, &hooks);

            // planner should terminate with target sole and no further split pair
            const RawOccRecord& O = RE.occ.get(target);
            const RawSkeleton& S = RE.skel.get(O.hostSkel);
            assert(S.hostedOcc.size() == 1 && S.hostedOcc[0] == target);
            assert_occ_patch_consistent(RE, target);
            auto sep = discover_split_pair_from_support(RE, target);
            assert(!sep.has_value());
        }
    }
}

void test_small_fuzz() {
    const vector<u32> seeds = {424242u, 424243u, 424244u, 424245u};
    for (u32 seed : seeds) {
        std::mt19937 rng(seed);
        for (int it = 0; it < 120; ++it) {
            RawEngine RE;
            RawUpdateCtx U;
            GeneratedCase gc = make_random_split_case(RE, U, rng);

            // Randomly isolate a target directly and verify validator invariants hold.
            OccID target = gc.occs[it % gc.occs.size()];
            RawSkelID sid = RE.occ.get(target).hostSkel;
            IsolateVertexResult ir = isolate_vertex(RE, sid, target, U);
            assert_skeleton_wellformed(RE, ir.occSkel);
            assert_occ_patch_consistent(RE, target);
            if (ir.residualSkel != NIL_U32) assert_skeleton_wellformed(RE, ir.residualSkel);
        }
    }
}

int main() {
    test_isolate();
    test_isolate_multiedge();
    test_split_join();
    test_integrate();
    test_planner();
    test_split_join_roundtrip_many();
    test_split_integrate_ensure_flow();
    test_regression_direct_ab_artifact_no_resplit();
    test_planner_stress();
    test_small_fuzz();
    cout << "raw_engine_v1 microcases+stress passed\n";
    return 0;
}
