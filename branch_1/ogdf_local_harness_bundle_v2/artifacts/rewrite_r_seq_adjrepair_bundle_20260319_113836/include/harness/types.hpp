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
    LOCAL_ORACLE_FAIL,
    SEQ_BUILD_CORE_FAIL,
    SEQ_CHOOSE_RX_FAIL,
    SEQ_REWRITE_R_FAIL,
    SEQ_NORMALIZE_FAIL,
    SEQ_ACTUAL_INVARIANT_FAIL,
    SEQ_ORACLE_FAIL,
    SEQ_PROGRESS_STUCK,
    SEQ_MAX_STEPS_REACHED
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
        case HarnessStage::SEQ_BUILD_CORE_FAIL: return "SEQ_BUILD_CORE_FAIL";
        case HarnessStage::SEQ_CHOOSE_RX_FAIL: return "SEQ_CHOOSE_RX_FAIL";
        case HarnessStage::SEQ_REWRITE_R_FAIL: return "SEQ_REWRITE_R_FAIL";
        case HarnessStage::SEQ_NORMALIZE_FAIL: return "SEQ_NORMALIZE_FAIL";
        case HarnessStage::SEQ_ACTUAL_INVARIANT_FAIL: return "SEQ_ACTUAL_INVARIANT_FAIL";
        case HarnessStage::SEQ_ORACLE_FAIL: return "SEQ_ORACLE_FAIL";
        case HarnessStage::SEQ_PROGRESS_STUCK: return "SEQ_PROGRESS_STUCK";
        case HarnessStage::SEQ_MAX_STEPS_REACHED: return "SEQ_MAX_STEPS_REACHED";
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

enum class ProxyArcRepairOutcome : uint8_t {
    PAR_OLDARC_ALREADY_LIVE,
    PAR_MATCH_BY_OUTSIDENODE_AND_POLES,
    PAR_MATCH_BY_POLES_ONLY_UNIQUE,
    PAR_FAIL_OUTSIDENODE_DEAD,
    PAR_FAIL_NO_CANDIDATE,
    PAR_FAIL_MULTI_CANDIDATE,
    PAR_FAIL_SLOT_NOT_VIRTUAL,
    PAR_FAIL_SLOT_ARCID_MISMATCH,
    PAR_FAIL_POLES_ONLY_MULTI_CANDIDATE,
    PAR_FAIL_POLES_ONLY_SLOT_INVALID,
    PAR_FAIL_POLES_ONLY_OTHER,
    PAR_OTHER,
    COUNT
};

enum class WeakRepairGateSubtype : uint8_t {
    WRG_NOT_NEEDED_STRONG_LIVE,
    WRG_ENTER_PNC_SAME_POLES_BUT_OTHER_OUTSIDE,
    WRG_SKIP_PNC_OLDNODE_NO_LIVE_ARCS,
    WRG_SKIP_OTHER_PNC,
    WRG_OTHER,
    COUNT
};

inline constexpr size_t kWeakRepairGateSubtypeCount =
    static_cast<size_t>(WeakRepairGateSubtype::COUNT);

enum class WeakRepairCandidateSubtype : uint8_t {
    WRC_ZERO_SAME_POLE_CANDIDATES,
    WRC_ONE_SAME_POLE_CANDIDATE,
    WRC_MULTI_SAME_POLE_CANDIDATES,
    WRC_SLOT_INVALID,
    WRC_OTHER,
    COUNT
};

inline constexpr size_t kWeakRepairCandidateSubtypeCount =
    static_cast<size_t>(WeakRepairCandidateSubtype::COUNT);

enum class WeakRepairCommitOutcome : uint8_t {
    WCO_NOT_ATTEMPTED,
    WCO_FAILED_BEFORE_GRAFT,
    WCO_GRAFT_FAIL,
    WCO_NORMALIZE_FAIL,
    WCO_ACTUAL_INVARIANT_FAIL,
    WCO_ORACLE_FAIL,
    WCO_COMMITTED,
    COUNT
};

inline constexpr size_t kWeakRepairCommitOutcomeCount =
    static_cast<size_t>(WeakRepairCommitOutcome::COUNT);

enum class ProxyArcLifecyclePhase : uint8_t {
    PAL_SNAPSHOT_OK,
    PAL_AFTER_CLEAR_KEEP_ALIVE,
    PAL_AFTER_CLEAR_KEEP_DEAD,
    PAL_AFTER_CLEAR_KEEP_NOT_INCIDENT,
    PAL_AFTER_CLEAR_KEEP_SLOT_INVALID,
    PAL_AFTER_MATERIALIZE_ALIVE,
    PAL_AFTER_MATERIALIZE_DEAD,
    PAL_AFTER_MATERIALIZE_NOT_INCIDENT,
    PAL_AFTER_MATERIALIZE_SLOT_INVALID,
    PAL_AFTER_INTERNAL_ARCS_ALIVE,
    PAL_AFTER_INTERNAL_ARCS_DEAD,
    PAL_AFTER_INTERNAL_ARCS_NOT_INCIDENT,
    PAL_AFTER_INTERNAL_ARCS_SLOT_INVALID,
    PAL_PRE_REWIRE_ALIVE,
    PAL_PRE_REWIRE_DEAD,
    PAL_PRE_REWIRE_NOT_INCIDENT,
    PAL_PRE_REWIRE_SLOT_INVALID,
    PAL_REWIRE_RET_FALSE,
    PAL_OTHER,
    COUNT
};

inline constexpr size_t kProxyArcLifecyclePhaseCount =
    static_cast<size_t>(ProxyArcLifecyclePhase::COUNT);

struct RepairedProxyArcInfo {
    int inputEdgeId = -1;
    ArcId originalOldArc = -1;
    ArcId resolvedArc = -1;
    NodeId oldNode = -1;
    NodeId originalOutsideNode = -1;
    NodeId resolvedOutsideNode = -1;
    NodeId outsideNode = -1;
    int resolvedOldSlot = -1;
    VertexId poleA = -1;
    VertexId poleB = -1;
    bool repairUsedWeakPolesOnly = false;
    bool weakRepairEntered = false;
    WeakRepairGateSubtype weakRepairGateSubtype = WeakRepairGateSubtype::WRG_OTHER;
    WeakRepairCandidateSubtype weakRepairCandidateSubtype =
        WeakRepairCandidateSubtype::WRC_OTHER;
    WeakRepairCommitOutcome weakRepairCommitOutcome =
        WeakRepairCommitOutcome::WCO_NOT_ATTEMPTED;
    std::vector<ProxyArcLifecyclePhase> phaseHistory;
    ProxyArcLifecyclePhase firstBadPhase = ProxyArcLifecyclePhase::PAL_OTHER;
    std::string firstBadWhy;
    ProxyArcRepairOutcome repairOutcome = ProxyArcRepairOutcome::PAR_OTHER;
};

using ResolvedProxyEndpoint = RepairedProxyArcInfo;

struct PreservedProxyArc {
    int inputEdgeId = -1;
    ArcId oldArc = -1;
    NodeId oldNode = -1;
    NodeId outsideNode = -1;
    int resolvedOldSlot = -1;
    VertexId poleA = -1;
    VertexId poleB = -1;
    int newSlot = -1;
    NodeId finalNode = -1;
    bool crossNodeRewire = false;
    bool sameNodeRehome = false;
};

inline constexpr size_t kProxyArcRepairOutcomeCount =
    static_cast<size_t>(ProxyArcRepairOutcome::COUNT);

enum class GraftRewireBailoutSubtype : uint8_t {
    GRB_OWNER_MINI_MISSING,
    GRB_OWNER_MINI_SLOT_INVALID,
    GRB_OWNER_SLOT_NOT_PROXY,
    GRB_ACTUAL_OF_MINI_MISSING,
    GRB_ACTUAL_SLOT_MISSING,
    GRB_OLDARC_OUT_OF_RANGE,
    GRB_OLDARC_DEAD,
    GRB_OLDARC_NOT_INCIDENT_TO_OLDNODE,
    GRB_OUTSIDENODE_MISMATCH,
    GRB_OLDSLOT_INVALID,
    GRB_OLDSLOT_NOT_VIRTUAL,
    GRB_OLDSLOT_ARCID_MISMATCH,
    GRB_DUPLICATE_OLDARC,
    GRB_REWIRE_RET_FALSE,
    GRB_OTHER,
    COUNT
};

enum class GraftOtherSubtype : uint8_t {
    GOS_PRESERVED_SNAPSHOT_EMPTY,
    GOS_PRESERVED_DUPLICATE_SLOT,
    GOS_PRESERVED_SLOT_OUT_OF_RANGE,
    GOS_PRESERVED_SLOT_DEAD,
    GOS_PRESERVED_SLOT_NOT_VIRTUAL,
    GOS_PRESERVED_SLOT_ARCID_MISMATCH,
    GOS_REHOME_NEWSLOT_INVALID,
    GOS_REHOME_OLDARC_DEAD,
    GOS_REHOME_OLDNODE_NOT_INCIDENT,
    GOS_REHOME_NEWSLOT_NOT_VIRTUAL,
    GOS_REHOME_ARC_SLOT_UPDATE_FAIL,
    GOS_POSTCHECK_STALE_PRESERVED_SLOT,
    GOS_POSTCHECK_PRESERVED_ARC_DEAD,
    GOS_POSTCHECK_ADJ_MISMATCH,
    GOS_POSTCHECK_OUTSIDE_MISMATCH,
    GOS_OTHER,
    COUNT
};

inline constexpr size_t kGraftOtherSubtypeCount =
    static_cast<size_t>(GraftOtherSubtype::COUNT);

struct GraftTrace {
    std::vector<NodeId> actualOfMini;
    std::vector<NodeId> actualNodes;
    std::vector<std::vector<int>> actualSlotOfMiniSlot;
    std::vector<RewiredProxyEdge> rewiredProxyEdges;
    std::vector<ResolvedProxyEndpoint> resolvedProxyEndpoints;
    std::vector<PreservedProxyArc> preservedProxyArcs;
    std::vector<NodeId> affectedAdjRepairNodes;
    std::vector<ArcId> oldNodeAdjArcsBeforeRepair;
    std::vector<ArcId> oldNodeAdjArcsAfterRepair;
    bool weakRepairEntered = false;
    WeakRepairGateSubtype weakRepairGateSubtype = WeakRepairGateSubtype::WRG_OTHER;
    WeakRepairCandidateSubtype weakRepairCandidateSubtype =
        WeakRepairCandidateSubtype::WRC_OTHER;
    WeakRepairCommitOutcome weakRepairCommitOutcome =
        WeakRepairCommitOutcome::WCO_NOT_ATTEMPTED;
    ArcId weakRepairOriginalOldArc = -1;
    ArcId weakRepairResolvedArc = -1;
    NodeId weakRepairOriginalOutsideNode = -1;
    NodeId weakRepairResolvedOutsideNode = -1;
    int weakRepairInputEdgeId = -1;
    GraftRewireBailoutSubtype graftRewireSubtype = GraftRewireBailoutSubtype::GRB_OTHER;
    GraftOtherSubtype graftOtherSubtype = GraftOtherSubtype::GOS_OTHER;
    int preservedProxyArcsCount = 0;
    bool sameNodeRehomeAttempted = false;
    bool sameNodeRehomeSucceeded = false;
    int failingPreservedInputEdge = -1;
    ArcId failingPreservedOldArc = -1;
    int failingPreservedOldSlot = -1;
    int failingNewSlot = -1;
    std::string graftOtherWhy;
    NodeId firstBadAdjNode = -1;
    std::vector<ArcId> expectedAdj;
    std::vector<ArcId> actualAdj;
    int failingInputEdge = -1;
    ArcId failingOldArc = -1;
    int failingOwnerMini = -1;
    int failingOwnerMiniSlot = -1;
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
    std::optional<int> stepIndex;
    std::optional<int> sequenceLengthSoFar;
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
