#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <limits>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using u8 = std::uint8_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using Vertex = u32;
using RawSkelID = u32;
using RawVID = u32;
using RawEID = u32;
using OccID = u32;
using BridgeRef = u32;
using LVertex = std::uint16_t;

inline constexpr u32 NIL_U32 = std::numeric_limits<u32>::max();

template <class T>
struct SlotPool {
    struct Slot {
        T val{};
        bool alive = false;
        u32 nextFree = NIL_U32;
    };

    std::vector<Slot> a;
    u32 freeHead = NIL_U32;

    u32 alloc(const T& x = T{}) {
        if (freeHead != NIL_U32) {
            const u32 id = freeHead;
            freeHead = a[id].nextFree;
            a[id].val = x;
            a[id].alive = true;
            a[id].nextFree = NIL_U32;
            return id;
        }

        a.push_back({});
        const u32 id = static_cast<u32>(a.size() - 1U);
        a[id].val = x;
        a[id].alive = true;
        return id;
    }

    void retire(u32 id) {
        if (id == NIL_U32) {
            return;
        }
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
    OccID occ = 0;
    std::vector<RawEID> adj;
};

struct RawEdge {
    RawVID a = 0;
    RawVID b = 0;
    RawEdgeKind kind = RawEdgeKind::CORE_REAL;
    BridgeRef br = 0;
    u8 side = 0;
};

struct RawOccRecord {
    Vertex orig = NIL_U32;
    RawSkelID hostSkel = 0;
    RawVID centerV = 0;
    std::vector<OccID> allocNbr;
    std::vector<RawEID> corePatchEdges;
};

struct RawSkeleton {
    std::vector<RawVID> verts;
    std::vector<RawEID> edges;
    std::vector<OccID> hostedOcc;
};

struct RawEngine {
    SlotPool<RawVertex> V;
    SlotPool<RawEdge> E;
    SlotPool<RawSkeleton> skel;
    SlotPool<RawOccRecord> occ;
    std::unordered_map<Vertex, std::vector<OccID>> occOfOrig;
};

struct RawSkeletonBuilder {
    struct BV {
        RawVertexKind kind = RawVertexKind::REAL;
        Vertex orig = NIL_U32;
        OccID occ = 0;
    };

    struct BE {
        u32 a = 0;
        u32 b = 0;
        RawEdgeKind kind = RawEdgeKind::CORE_REAL;
        BridgeRef br = 0;
        u8 side = 0;
    };

    std::vector<BV> V;
    std::vector<BE> E;
    std::unordered_map<OccID, std::vector<OccID>> allocNbr;
    std::unordered_map<OccID, std::vector<u32>> corePatchLocalEids;
};

struct BuildCtx {
    RawSkeletonBuilder B;
    std::unordered_map<RawVID, u32> mapV;
    std::unordered_map<RawEID, u32> mapE;
    std::unordered_set<Vertex> seenRealOrig;
};

struct RawUpdateCtx {
    std::unordered_set<RawSkelID> dirtySkeletons;
    std::unordered_set<OccID> dirtyOccurrences;
};

struct LabeledComponents {
    std::unordered_map<RawVID, int> compId;
    std::vector<std::vector<RawVID>> compVerts;
};

struct TinyEdge {
    LVertex a = 0;
    LVertex b = 0;
};

struct TinyGraph {
    std::vector<Vertex> orig;
    std::vector<TinyEdge> edges;
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
    std::vector<OccID> allocNbr;
    std::vector<IsoPort> ports;
    TinyGraph core;
};

struct IsolateVertexResult {
    RawSkelID occSkel = NIL_U32;
    RawSkelID residualSkel = NIL_U32;
};

struct SepSidePrepared {
    std::vector<RawVID> innerVerts;
    std::vector<RawEID> innerEdges;
    std::vector<RawEID> boundaryEdges;
    std::vector<OccID> hostedOcc;
    bool useA = false;
    bool useB = false;
};

struct SplitPrepared {
    Vertex aOrig = NIL_U32;
    Vertex bOrig = NIL_U32;
    RawVID aV = 0;
    RawVID bV = 0;
    std::vector<SepSidePrepared> side;
};

struct SplitChildInfo {
    RawSkelID sid = NIL_U32;
    std::vector<OccID> hostedOcc;
    bool boundaryOnly = false;
};

struct SplitSeparationPairResult {
    Vertex aOrig = NIL_U32;
    Vertex bOrig = NIL_U32;
    RawSkelID reusedSid = NIL_U32;
    std::vector<SplitChildInfo> child;
};

struct JoinSeparationPairResult {
    RawSkelID mergedSid = NIL_U32;
    RawSkelID retiredSid = NIL_U32;
};

struct BoundaryMapEntry {
    Vertex childOrig = NIL_U32;
    Vertex parentOrig = NIL_U32;
};

struct IntegrateResult {
    RawSkelID mergedSid = NIL_U32;
    RawSkelID retiredSid = NIL_U32;
};

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
    std::vector<BoundaryMapEntry> bm;
};

struct RawPlannerCtx {
    OccID targetOcc = 0;
    std::unordered_set<OccID> keepOcc;
};

struct RawUpdaterHooks {
    std::function<void(const SplitSeparationPairResult&, std::deque<UpdJob>&)> afterSplit;
    std::function<void(const JoinSeparationPairResult&, std::deque<UpdJob>&)> afterJoin;
    std::function<void(const IntegrateResult&, std::deque<UpdJob>&)> afterIntegrate;
};

struct RawUpdaterRunOptions {
    std::size_t stepBudget = 200000;
};

RawSkeletonBuilder::BV make_builder_vertex(RawVertexKind kind, Vertex orig, OccID occ = 0);
RawSkeletonBuilder::BE make_builder_edge(u32 a, u32 b, RawEdgeKind kind, BridgeRef br = 0, u8 side = 0);
RawOccRecord make_occ_record(Vertex orig);

RawVID other_end(const RawEdge& e, RawVID u);
u32 copy_old_vertex(BuildCtx& C, const RawEngine& RE, RawVID oldv, bool checkRealUnique = true);
u32 copy_old_edge(BuildCtx& C, const RawEngine& RE, RawEID olde);
void remap_occ_meta(BuildCtx& C, const RawEngine& RE, OccID occ, const std::vector<RawEID>& oldCoreEdges);
LabeledComponents label_components_without_blocked(
    const RawEngine& RE,
    const RawSkeleton& S,
    const std::unordered_set<RawVID>& blocked
);
std::vector<RawVID> collect_support_vertices(const RawEngine& RE, OccID occ);
RawVID find_real_orig_in_skeleton(const RawEngine& RE, const RawSkeleton& S, Vertex ov);
void retire_skeleton_contents(RawEngine& RE, RawSkelID sid);
void commit_skeleton(RawEngine& RE, RawSkelID sid, RawSkeletonBuilder&& B, RawUpdateCtx& U);

void assert_builder_basic(const RawSkeletonBuilder& B);
void assert_skeleton_wellformed(const RawEngine& RE, RawSkelID sid);
void assert_occ_patch_consistent(const RawEngine& RE, OccID occ);
void debug_validate_skeleton_and_hosted(const RawEngine& RE, RawSkelID sid);

IsolatePrepared prepare_isolate_neighborhood(const RawEngine& RE, RawSkelID sid, OccID occ);
RawSkeletonBuilder build_isolated_occ_builder(const IsolatePrepared& in);
RawSkeletonBuilder build_residual_after_isolate(const RawEngine& RE, RawSkelID sid, OccID removedOcc);
IsolateVertexResult isolate_vertex(RawEngine& RE, RawSkelID sid, OccID occ, RawUpdateCtx& U);

SplitPrepared prepare_split_separation_pair(const RawEngine& RE, RawSkelID sid, Vertex saOrig, Vertex sbOrig);
RawSkeletonBuilder build_child_after_sep_split(const RawEngine& RE, const SplitPrepared& P, u32 sideIdx);
SplitSeparationPairResult split_separation_pair(RawEngine& RE, RawSkelID sid, Vertex saOrig, Vertex sbOrig, RawUpdateCtx& U);

RawSkeletonBuilder build_merged_after_sep_join(
    const RawEngine& RE,
    RawSkelID leftSid,
    RawSkelID rightSid,
    Vertex saOrig,
    Vertex sbOrig
);
JoinSeparationPairResult join_separation_pair(
    RawEngine& RE,
    RawSkelID leftSid,
    RawSkelID rightSid,
    Vertex saOrig,
    Vertex sbOrig,
    RawUpdateCtx& U
);

RawSkeletonBuilder build_merged_after_integrate(
    const RawEngine& RE,
    RawSkelID parentSid,
    RawSkelID childSid,
    const std::vector<BoundaryMapEntry>& bm
);
IntegrateResult integrate_skeleton(
    RawEngine& RE,
    RawSkelID parentSid,
    RawSkelID childSid,
    const std::vector<BoundaryMapEntry>& bm,
    RawUpdateCtx& U
);

bool queue_has_ensure_sole(const std::deque<UpdJob>& q, OccID occ);
void enqueue_ensure_sole_once_back(std::deque<UpdJob>& q, OccID occ);
void prepend_jobs(std::deque<UpdJob>& q, std::vector<UpdJob> jobs);
bool valid_split_pair_for_support(
    const RawEngine& RE,
    const RawSkeleton& S,
    RawVID aV,
    RawVID bV,
    const std::vector<RawVID>& support
);
std::optional<std::pair<Vertex, Vertex>> discover_split_pair_from_support(const RawEngine& RE, OccID occ);
bool child_contains_occ(const SplitChildInfo& c, OccID occ);
bool child_intersects_keep_occ(const SplitChildInfo& c, const RawPlannerCtx& ctx);
int choose_anchor_child(const SplitSeparationPairResult& res, const RawPlannerCtx& ctx);
RawUpdaterHooks make_basic_hooks_for_target(const RawPlannerCtx& ctx);
void run_raw_local_updater(
    RawEngine& RE,
    OccID targetOcc,
    RawUpdateCtx& U,
    RawUpdaterHooks* hooks = nullptr,
    const RawUpdaterRunOptions& runOptions = {}
);
