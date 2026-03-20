#pragma once
#include <algorithm>
#include <cstdint>
#include <optional>
#include <ostream>
#include <queue>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace harness {

using VertexId = int;
using EdgeId = int;
using NodeId = int;
using ArcId = int;
using BlockId = int;

enum class SPQRType : uint8_t { S_NODE, P_NODE, R_NODE };
enum class CoreKind : uint8_t { TINY, REDUCED_SPQR };
enum class CompactEdgeKind : uint8_t { REAL, PROXY };
enum class RawSlotKind : uint8_t { INPUT_EDGE, TREE_EDGE };
enum class MiniSlotKind : uint8_t { REAL_INPUT, PROXY_INPUT, INTERNAL_VIRTUAL };
enum class RawDecompError : uint8_t { NONE, NOT_BICONNECTED, HAS_SELF_LOOP, INTERNAL_BROKEN };

enum class HarnessStage : uint8_t {
    RAW_BACKEND_FAIL,
    RAW_VALIDATE_FAIL,
    MINI_MATERIALIZE_FAIL,
    MINI_PRECHECK_FAIL,
    MINI_NORMALIZE_THROW,
    MINI_POSTCHECK_FAIL,
    DUMMY_ENVELOPE_FAIL,
    KEEP_SELECT_FAIL,
    GRAFT_FAIL,
    ACTUAL_METADATA_FAIL,
    ACTUAL_INVARIANT_FAIL,
    DUMMY_REAL_SET_FAIL,
    DUMMY_PROXY_REWIRE_FAIL,
    LOCAL_BUILD_CORE_FAIL,
    LOCAL_CHOOSE_RX_FAIL,
    LOCAL_REWRITE_R_FAIL,
    LOCAL_NORMALIZE_FAIL,
    LOCAL_ACTUAL_INVARIANT_FAIL,
    LOCAL_ORACLE_FAIL
};

inline const char *stageName(HarnessStage s) {
    switch (s) {
        case HarnessStage::RAW_BACKEND_FAIL: return "RAW_BACKEND_FAIL";
        case HarnessStage::RAW_VALIDATE_FAIL: return "RAW_VALIDATE_FAIL";
        case HarnessStage::MINI_MATERIALIZE_FAIL: return "MINI_MATERIALIZE_FAIL";
        case HarnessStage::MINI_PRECHECK_FAIL: return "MINI_PRECHECK_FAIL";
        case HarnessStage::MINI_NORMALIZE_THROW: return "MINI_NORMALIZE_THROW";
        case HarnessStage::MINI_POSTCHECK_FAIL: return "MINI_POSTCHECK_FAIL";
        case HarnessStage::DUMMY_ENVELOPE_FAIL: return "DUMMY_ENVELOPE_FAIL";
        case HarnessStage::KEEP_SELECT_FAIL: return "KEEP_SELECT_FAIL";
        case HarnessStage::GRAFT_FAIL: return "GRAFT_FAIL";
        case HarnessStage::ACTUAL_METADATA_FAIL: return "ACTUAL_METADATA_FAIL";
        case HarnessStage::ACTUAL_INVARIANT_FAIL: return "ACTUAL_INVARIANT_FAIL";
        case HarnessStage::DUMMY_REAL_SET_FAIL: return "DUMMY_REAL_SET_FAIL";
        case HarnessStage::DUMMY_PROXY_REWIRE_FAIL: return "DUMMY_PROXY_REWIRE_FAIL";
        case HarnessStage::LOCAL_BUILD_CORE_FAIL: return "LOCAL_BUILD_CORE_FAIL";
        case HarnessStage::LOCAL_CHOOSE_RX_FAIL: return "LOCAL_CHOOSE_RX_FAIL";
        case HarnessStage::LOCAL_REWRITE_R_FAIL: return "LOCAL_REWRITE_R_FAIL";
        case HarnessStage::LOCAL_NORMALIZE_FAIL: return "LOCAL_NORMALIZE_FAIL";
        case HarnessStage::LOCAL_ACTUAL_INVARIANT_FAIL: return "LOCAL_ACTUAL_INVARIANT_FAIL";
        case HarnessStage::LOCAL_ORACLE_FAIL: return "LOCAL_ORACLE_FAIL";
    }
    return "UNKNOWN";
}

struct Agg {
    int edgeCnt = 0;
    int vertexCnt = 0;
    int watchedCnt = 0;
    int incCnt = 0;
    EdgeId repEdge = -1;
    VertexId repVertex = -1;
};

struct CompactEdge {
    int id = -1;
    CompactEdgeKind kind = CompactEdgeKind::REAL;
    int a = -1, b = -1;
    EdgeId realEdge = -1;
    ArcId oldArc = -1;
    NodeId outsideNode = -1;
    Agg sideAgg;
    int oldSlotInU = -1;
};

struct CompactGraph {
    BlockId block = -1;
    NodeId ownerR = -1;
    VertexId deletedX = -1;
    std::vector<VertexId> origOfCv;
    std::unordered_map<VertexId,int> cvOfOrig;
    std::vector<CompactEdge> edges;
    std::vector<VertexId> touchedVertices;
};

struct RawSlot {
    bool alive = true;
    RawSlotKind kind = RawSlotKind::INPUT_EDGE;
    int inputEdgeId = -1;
    int treeEdgeId = -1;
    VertexId poleA = -1, poleB = -1;
};

struct RawSShape { std::vector<int> cycleSlots; };
struct RawPShape { VertexId poleA = -1, poleB = -1; };
struct RawRShape {
    std::vector<VertexId> skelVertices;
    std::vector<std::pair<int,int>> endsOfSlot;
    std::vector<std::vector<int>> incSlots;
};

struct RawSpqrNode {
    bool alive = true;
    SPQRType type = SPQRType::R_NODE;
    std::vector<RawSlot> slots;
    std::vector<int> cycleSlots;
    std::optional<RawPShape> pShape;
    std::optional<RawRShape> rShape;
};

struct RawSpqrTreeEdge {
    bool alive = true;
    int a = -1, b = -1;
    int slotInA = -1, slotInB = -1;
    VertexId poleA = -1, poleB = -1;
};

struct RawSpqrDecomp {
    bool valid = false;
    RawDecompError error = RawDecompError::NONE;
    std::vector<RawSpqrNode> nodes;
    std::vector<RawSpqrTreeEdge> treeEdges;
    std::vector<std::pair<int,int>> ownerOfInputEdge;
};

struct MiniSlot {
    MiniSlotKind kind = MiniSlotKind::REAL_INPUT;
    bool alive = true;
    VertexId poleA = -1, poleB = -1;
    int inputEdgeId = -1;
    EdgeId realEdge = -1;
    int miniArcId = -1;
};

struct MiniNode {
    bool alive = true;
    SPQRType type = SPQRType::R_NODE;
    std::vector<MiniSlot> slots;
    std::vector<int> adjArcs;
    Agg localAgg;
    Agg payloadAgg;
};

struct MiniArc {
    bool alive = true;
    int a = -1, b = -1;
    int slotInA = -1, slotInB = -1;
    VertexId poleA = -1, poleB = -1;
};

struct StaticMiniCore {
    bool valid = false;
    CoreKind kind = CoreKind::REDUCED_SPQR;
    std::vector<MiniNode> nodes;
    std::vector<MiniArc> arcs;
    std::vector<std::pair<int,int>> ownerOfInputEdge;
};

struct SkeletonSlot {
    bool alive = true;
    VertexId poleA = -1, poleB = -1;
    bool isVirtual = false;
    EdgeId realEdge = -1;
    ArcId arcId = -1;
};

struct SPQRNodeCore {
    bool alive = true;
    SPQRType type = SPQRType::R_NODE;
    std::vector<SkeletonSlot> slots;
    std::vector<ArcId> adjArcs;
    std::vector<EdgeId> realEdgesHere;
    Agg localAgg;
    Agg subAgg;
};

struct SPQRArcCore {
    bool alive = true;
    NodeId a = -1, b = -1;
    int slotInA = -1, slotInB = -1;
    VertexId poleA = -1, poleB = -1;
};

struct OccRef { NodeId node = -1; int slot = -1; };

struct ReducedSPQRCore {
    BlockId blockId = -1;
    NodeId root = -1;
    std::vector<SPQRNodeCore> nodes;
    std::vector<SPQRArcCore> arcs;
    std::unordered_map<VertexId, std::vector<OccRef>> occ;
    std::unordered_map<EdgeId, NodeId> ownerNodeOfRealEdge;
    std::unordered_map<EdgeId, int> ownerSlotOfRealEdge;
    Agg totalAgg;
};

struct RewiredProxyEdge {
    int inputEdgeId = -1;
    ArcId oldArc = -1;
    NodeId actualNode = -1;
    int actualSlot = -1;
};

struct GraftTrace {
    std::vector<NodeId> actualOfMini;
    std::vector<NodeId> actualNodes;
    std::vector<std::vector<int>> actualSlotOfMiniSlot;
    std::vector<RewiredProxyEdge> rewiredProxyEdges;
};

struct DummyActualEnvelope {
    CompactGraph H;
    ReducedSPQRCore core;
    NodeId root = -1;
    NodeId oldR = -1;
    std::unordered_set<NodeId> stubNodes;
    std::vector<NodeId> stubOfInputEdge;
    std::vector<ArcId> arcOfInputEdge;
};

struct ExplicitEdge { EdgeId id = -1; VertexId u = -1, v = -1; };
struct ExplicitBlockGraph { std::vector<ExplicitEdge> edges; std::vector<VertexId> vertices; };

struct HarnessBundle {
    uint64_t seed = 0;
    int tc = -1;
    std::string backendName;
    HarnessStage stage = HarnessStage::RAW_BACKEND_FAIL;
    std::string where;
    std::string why;
    std::optional<ExplicitBlockGraph> explicitInput;
    std::optional<CompactGraph> compact;
    std::optional<RawSpqrDecomp> raw;
    std::optional<StaticMiniCore> miniBeforeNormalize;
    std::optional<StaticMiniCore> miniAfterNormalize;
    std::optional<ReducedSPQRCore> actualBeforeGraft;
    std::optional<ReducedSPQRCore> actualAfterGraft;
    std::optional<ReducedSPQRCore> actualBeforeRewrite;
    std::optional<ReducedSPQRCore> actualAfterRewrite;
    std::optional<GraftTrace> trace;
    std::optional<NodeId> chosenR;
    std::optional<VertexId> chosenX;
    std::optional<ExplicitBlockGraph> explicitBefore;
    std::optional<ExplicitBlockGraph> explicitAfter;
    std::optional<ExplicitBlockGraph> explicitExpected;
    std::optional<ExplicitBlockGraph> explicitGot;
};

struct HarnessResult {
    bool ok = true;
    std::string where;
    std::string why;
    std::string dumpPath;
};

struct RunConfig {
    uint64_t seed = 1;
    int rounds = 1;
    bool manualOnly = false;
    std::string mode = "static";
    std::string dumpDir = "dumps";
};

inline std::pair<VertexId, VertexId> canonPole(VertexId a, VertexId b) {
    if (a > b) std::swap(a, b);
    return {a, b};
}

} // namespace harness
