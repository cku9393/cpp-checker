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
    SEQ_MAX_STEPS_REACHED,
    SEQ_REPLAY_CAPTURE
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
        case HarnessStage::SEQ_REPLAY_CAPTURE: return "SEQ_REPLAY_CAPTURE";
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

enum class GraftPostcheckSubtype : uint8_t {
    GPS_ADJ_METADATA_ONLY,
    GPS_SAME_TYPE_SP_ONLY,
    GPS_ADJ_AND_SAME_TYPE_SP,
    GPS_OTHER,
    COUNT
};

inline constexpr size_t kGraftPostcheckSubtypeCount =
    static_cast<size_t>(GraftPostcheckSubtype::COUNT);

enum class ReplaySnapshotPhase : uint8_t {
    BEFORE_CLEAR,
    AFTER_CLEAR_PRESERVE,
    AFTER_MATERIALIZE,
    AFTER_INTERNAL_ARC_CONNECT,
    AFTER_PROXY_REWIRE,
    AFTER_ADJ_REPAIR,
    BEFORE_SP_CLEANUP,
    AFTER_SP_CLEANUP,
    AFTER_NORMALIZE,
    COUNT
};

struct GraftTrace {
    std::vector<NodeId> actualOfMini;
    std::vector<NodeId> actualNodes;
    std::vector<std::vector<int>> actualSlotOfMiniSlot;
    std::vector<RewiredProxyEdge> rewiredProxyEdges;
    std::vector<ResolvedProxyEndpoint> resolvedProxyEndpoints;
    std::vector<PreservedProxyArc> preservedProxyArcs;
    std::vector<NodeId> affectedAdjRepairNodes;
    std::vector<NodeId> affectedNodesAfterInPlaceApply;
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
    bool inPlaceLoopSharedApplied = false;
    int loopInputEdgeId = -1;
    int realInputEdgeId = -1;
    VertexId loopSharedCutVertex = -1;
    NodeId loopSharedChildNode = -1;
    bool sameNodeRehomeAttempted = false;
    bool sameNodeRehomeSucceeded = false;
    bool deferredSameTypeSP = false;
    struct SameTypeSPCleanupMerge {
        NodeId u = -1;
        NodeId v = -1;
        NodeId keep = -1;
    };
    GraftPostcheckSubtype postcheckSubtype = GraftPostcheckSubtype::GPS_OTHER;
    GraftPostcheckSubtype preCleanupPostcheckSubtype = GraftPostcheckSubtype::GPS_OTHER;
    GraftPostcheckSubtype postCleanupPostcheckSubtype = GraftPostcheckSubtype::GPS_OTHER;
    std::string postcheckWhyDetailed;
    std::vector<NodeId> sameTypeSPCleanupSeedNodes;
    int sameTypeSPCleanupMergeCount = 0;
    std::vector<SameTypeSPCleanupMerge> sameTypeSPCleanupMergedPairs;
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
    struct ReplayLiveArcSummary {
        ArcId arcId = -1;
        NodeId otherNode = -1;
        int slotInNode = -1;
        int slotInOther = -1;
        VertexId poleA = -1;
        VertexId poleB = -1;
    };
    struct ReplaySlotSnapshot {
        int slotId = -1;
        bool alive = false;
        bool isVirtual = false;
        VertexId poleA = -1;
        VertexId poleB = -1;
        EdgeId realEdge = -1;
        ArcId arcId = -1;
    };
    struct ReplayNodeSnapshot {
        NodeId nodeId = -1;
        bool alive = false;
        SPQRType type = SPQRType::R_NODE;
        std::vector<ArcId> adjArcs;
        std::vector<EdgeId> realEdgesHere;
        std::vector<ReplaySlotSnapshot> slots;
        std::vector<ReplayLiveArcSummary> neighboringLiveArcs;
    };
    struct ReplayNodeSnapshotPhase {
        ReplaySnapshotPhase phase = ReplaySnapshotPhase::BEFORE_CLEAR;
        std::vector<ReplayNodeSnapshot> nodes;
    };
    std::vector<ReplayNodeSnapshotPhase> oldNodeSnapshotsByPhase;
    std::vector<ReplayNodeSnapshotPhase> affectedNodeSnapshotsByPhase;
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
struct CanonicalExplicitGraph {
    std::vector<std::tuple<int, int, int>> edges;
    std::vector<int> vertices;
};

enum class CompareBaselineKind : uint8_t {
    LEGACY_BASELINE,
    ORACLE_FIXPOINT_BASELINE
};

inline const char *compareBaselineKindName(CompareBaselineKind kind) {
    switch (kind) {
        case CompareBaselineKind::LEGACY_BASELINE: return "LEGACY_BASELINE";
        case CompareBaselineKind::ORACLE_FIXPOINT_BASELINE:
            return "ORACLE_FIXPOINT_BASELINE";
    }
    return "UNKNOWN_BASELINE";
}

enum class OracleHandoffPolicy : uint8_t {
    OHP_DELETE_EXPLICIT,
    OHP_NORMALIZE_EXPLICIT
};

inline const char *oracleHandoffPolicyName(OracleHandoffPolicy policy) {
    switch (policy) {
        case OracleHandoffPolicy::OHP_DELETE_EXPLICIT:
            return "OHP_DELETE_EXPLICIT";
        case OracleHandoffPolicy::OHP_NORMALIZE_EXPLICIT:
            return "OHP_NORMALIZE_EXPLICIT";
    }
    return "OHP_UNKNOWN";
}

struct SolverOutput {
    std::optional<CompareBaselineKind> baselineKind;
    ExplicitBlockGraph explicitGraph;
    CanonicalExplicitGraph canonicalExplicitGraph;
    std::vector<int> parent;
    std::string debugTag;
    std::string why;
    bool valid = false;
    bool actualInvariantOk = false;
};

enum class OgdfRawCrashReplaySourceKind : uint8_t {
    ORACLE,
    REWRITE,
    BASELINE,
    AUTO
};

inline const char *ogdfRawCrashReplaySourceKindName(OgdfRawCrashReplaySourceKind kind) {
    switch (kind) {
        case OgdfRawCrashReplaySourceKind::ORACLE: return "ORACLE";
        case OgdfRawCrashReplaySourceKind::REWRITE: return "REWRITE";
        case OgdfRawCrashReplaySourceKind::BASELINE: return "BASELINE";
        case OgdfRawCrashReplaySourceKind::AUTO: return "AUTO";
    }
    return "AUTO";
}

struct CompactPrecheckSummary {
    int edgeCount = 0;
    int vertexCount = 0;
    int selfLoopCount = 0;
    int connectedComponentCount = 0;
    bool connected = false;
    bool biconnected = false;
    bool spqrReady = false;
    std::string spqrReadyWhy;
    std::string tooSmallSubtype = "TS_OTHER";
    std::string notBiconnectedSubtype = "NB_OTHER";
    std::string selfLoopSubtype = "SL_OTHER";
};

struct ChildRunResult {
    bool launched = false;
    bool exited = false;
    bool crashed = false;
    int childExitCode = -1;
    int childSignal = -1;
    std::string childWhy;
};

struct CrashReplayContext {
    std::string sourceSide;
    std::string callSiteTag;
    std::string phaseTag;
    std::string dispatchKind;
    bool directRawAllowed = false;
    std::string directRawBlockedReason;
    bool usedSharedDispatchPath = false;
    bool usedWholeCoreFallback = false;
    int stepIndex = -1;
    int chosenR = -1;
    int chosenX = -1;
    std::string why;
};

struct OgdfRawCrashReplayBundle {
    std::string caseName;
    std::string manifestPath;
    uint64_t seed = 0;
    int tcIndex = -1;
    std::optional<int> targetStep;
    bool topLevelOk = false;
    OgdfRawCrashReplaySourceKind requestedSource = OgdfRawCrashReplaySourceKind::AUTO;
    bool stopBeforeOgdf = false;
    bool runChild = true;
    std::string sourceSide;
    std::string callSiteTag;
    std::string phaseTag;
    std::string dispatchKind;
    bool directRawAllowed = false;
    std::string directRawBlockedReason;
    bool usedSharedDispatchPath = false;
    bool usedWholeCoreFallback = false;
    int stepIndex = -1;
    int chosenR = -1;
    int chosenX = -1;
    CompactGraph compactGraphRaw;
    std::string compactGraphCanonicalSummary;
    CompactPrecheckSummary precheckSummary;
    std::string compactGraphDumpPath;
    int childExitCode = -1;
    int childSignal = -1;
    bool crashed = false;
    std::string crashWhy;
    std::string lldbBacktracePath;
    std::string notes;
};

struct SolverCompareBundle {
    std::string inputCaseId;
    uint64_t seed = 0;
    int tc = -1;
    std::optional<int> targetStep;
    OracleHandoffPolicy oracleHandoffPolicy =
        OracleHandoffPolicy::OHP_DELETE_EXPLICIT;
    ExplicitBlockGraph inputExplicit;
    std::optional<SolverOutput> legacyOutput;
    std::optional<SolverOutput> oracleOutput;
    std::optional<SolverOutput> rewriteSeqOutput;
    std::optional<CanonicalExplicitGraph> legacyCanonicalExplicit;
    std::optional<CanonicalExplicitGraph> oracleCanonicalExplicit;
    std::optional<CanonicalExplicitGraph> rewriteSeqCanonicalExplicit;
    std::string legacyWhy;
    std::string oracleWhy;
    std::string rewriteSeqWhy;
    std::optional<bool> legacyVsRewriteEquivalent;
    std::optional<bool> oracleVsRewriteEquivalent;
    std::optional<bool> legacyVsOracleEquivalent;
    std::optional<bool> legacyVsRewriteRawExplicitEquivalent;
    std::optional<bool> oracleVsRewriteRawExplicitEquivalent;
    std::optional<bool> legacyVsOracleRawExplicitEquivalent;
    std::optional<bool> legacyVsRewriteCanonicalExplicitEquivalent;
    std::optional<bool> oracleVsRewriteCanonicalExplicitEquivalent;
    std::optional<bool> legacyVsOracleCanonicalExplicitEquivalent;
    std::optional<bool> legacyVsRewriteParentEquivalent;
    std::optional<bool> oracleVsRewriteParentEquivalent;
    std::optional<bool> legacyVsOracleParentEquivalent;
    std::optional<CanonicalExplicitGraph> oracleDeletePolicyFinalExplicit;
    std::optional<CanonicalExplicitGraph> oracleNormalizePolicyFinalExplicit;
    std::optional<CanonicalExplicitGraph> rewriteFinalExplicit;
    std::optional<bool> deleteVsRewriteCanonicalEqual;
    std::optional<bool> normalizeVsRewriteCanonicalEqual;
    std::optional<bool> deleteVsNormalizeCanonicalEqual;
    std::string firstMismatchDescription;
    double legacyElapsedMs = 0.0;
    double oracleElapsedMs = 0.0;
    double rewriteSeqElapsedMs = 0.0;
};

struct SolverCompareStats {
    uint64_t compareCases = 0;
    uint64_t comparePassed = 0;
    uint64_t compareFailed = 0;
    uint64_t legacyVsRewritePassed = 0;
    uint64_t oracleVsRewritePassed = 0;
    uint64_t legacyVsOraclePassed = 0;
    uint64_t legacyFailCount = 0;
    uint64_t oracleFailCount = 0;
    uint64_t rewriteSeqFailCount = 0;
    uint64_t legacyVsRewriteMismatchCount = 0;
    uint64_t oracleVsRewriteMismatchCount = 0;
    uint64_t legacyVsOracleMismatchCount = 0;
    uint64_t explicitMismatchCount = 0;
    uint64_t parentMismatchCount = 0;
    double totalLegacyMs = 0.0;
    double totalOracleMs = 0.0;
    double totalRewriteSeqMs = 0.0;
    double averageLegacyMs = 0.0;
    double averageOracleMs = 0.0;
    double averageRewriteSeqMs = 0.0;
    double maxLegacyMs = 0.0;
    double maxOracleMs = 0.0;
    double maxRewriteSeqMs = 0.0;
};

enum class SolverBaselineStage : uint8_t {
    BASELINE_BUILD_CORE_FAIL,
    BASELINE_CHOOSE_RX_FAIL,
    BASELINE_REWRITE_FAIL,
    BASELINE_NORMALIZE_FAIL,
    BASELINE_ACTUAL_INVARIANT_FAIL,
    BASELINE_ORACLE_FAIL,
    BASELINE_PROGRESS_STUCK,
    BASELINE_DONE
};

inline const char *solverBaselineStageName(SolverBaselineStage stage) {
    switch (stage) {
        case SolverBaselineStage::BASELINE_BUILD_CORE_FAIL:
            return "BASELINE_BUILD_CORE_FAIL";
        case SolverBaselineStage::BASELINE_CHOOSE_RX_FAIL:
            return "BASELINE_CHOOSE_RX_FAIL";
        case SolverBaselineStage::BASELINE_REWRITE_FAIL:
            return "BASELINE_REWRITE_FAIL";
        case SolverBaselineStage::BASELINE_NORMALIZE_FAIL:
            return "BASELINE_NORMALIZE_FAIL";
        case SolverBaselineStage::BASELINE_ACTUAL_INVARIANT_FAIL:
            return "BASELINE_ACTUAL_INVARIANT_FAIL";
        case SolverBaselineStage::BASELINE_ORACLE_FAIL:
            return "BASELINE_ORACLE_FAIL";
        case SolverBaselineStage::BASELINE_PROGRESS_STUCK:
            return "BASELINE_PROGRESS_STUCK";
        case SolverBaselineStage::BASELINE_DONE:
            return "BASELINE_DONE";
    }
    return "BASELINE_UNKNOWN";
}

enum class SolverBaselineReplayPhase : uint8_t {
    BEFORE_REWRITE,
    AFTER_REWRITE,
    AFTER_NORMALIZE,
    AFTER_INVARIANT,
    AFTER_ORACLE
};

inline const char *solverBaselineReplayPhaseName(SolverBaselineReplayPhase phase) {
    switch (phase) {
        case SolverBaselineReplayPhase::BEFORE_REWRITE: return "BEFORE_REWRITE";
        case SolverBaselineReplayPhase::AFTER_REWRITE: return "AFTER_REWRITE";
        case SolverBaselineReplayPhase::AFTER_NORMALIZE: return "AFTER_NORMALIZE";
        case SolverBaselineReplayPhase::AFTER_INVARIANT: return "AFTER_INVARIANT";
        case SolverBaselineReplayPhase::AFTER_ORACLE: return "AFTER_ORACLE";
    }
    return "BASELINE_REPLAY_PHASE_UNKNOWN";
}

enum class SolverBaselineInvariantKind : uint8_t {
    NONE,
    DEAD_RELAY_ONLY,
    SAME_TYPE_SP_ONLY,
    ADJ_ONLY,
    MIXED,
    OTHER
};

inline const char *solverBaselineInvariantKindName(
    SolverBaselineInvariantKind kind) {
    switch (kind) {
        case SolverBaselineInvariantKind::NONE: return "NONE";
        case SolverBaselineInvariantKind::DEAD_RELAY_ONLY: return "DEAD_RELAY_ONLY";
        case SolverBaselineInvariantKind::SAME_TYPE_SP_ONLY:
            return "SAME_TYPE_SP_ONLY";
        case SolverBaselineInvariantKind::ADJ_ONLY: return "ADJ_ONLY";
        case SolverBaselineInvariantKind::MIXED: return "MIXED";
        case SolverBaselineInvariantKind::OTHER: return "OTHER";
    }
    return "BASELINE_INVARIANT_UNKNOWN";
}

struct SolverBaselineReplayPhaseSnapshot {
    SolverBaselineReplayPhase phase = SolverBaselineReplayPhase::BEFORE_REWRITE;
    int aliveNodeCount = 0;
    NodeId currentRoot = -1;
    int currentExplicitEdgeCount = 0;
    std::vector<GraftTrace::ReplayNodeSnapshot> nodes;
};

struct SolverBaselineReplayStepSnapshot {
    int stepIndex = -1;
    int sequenceLengthSoFar = 0;
    int aliveNodeCount = 0;
    NodeId currentRoot = -1;
    int currentExplicitEdgeCount = 0;
    std::optional<NodeId> chosenR;
    std::optional<VertexId> chosenX;
    bool actualInvariantOk = false;
    std::string actualInvariantWhy;
    std::string actualInvariantDetailedSubtype;
    std::optional<NodeId> firstFailingNodeId;
    SolverBaselineInvariantKind firstFailingInvariantKind =
        SolverBaselineInvariantKind::NONE;
    std::vector<NodeId> deadRelayCandidateNodes;
    bool sameTypeSPPresent = false;
    bool adjacencyMismatchPresent = false;
    std::vector<SolverBaselineReplayPhaseSnapshot> phaseSnapshots;
};

struct SolverBaselineReplayBundle {
    std::string caseName;
    std::string manifestPath;
    std::string debugTag;
    uint64_t seed = 0;
    int tc = -1;
    std::optional<int> targetStep;
    ExplicitBlockGraph explicitInput;
    std::optional<int> stepIndex;
    std::optional<int> sequenceLengthSoFar;
    std::optional<NodeId> chosenR;
    std::optional<VertexId> chosenX;
    std::optional<ExplicitBlockGraph> explicitBefore;
    std::optional<ReducedSPQRCore> actualBeforeRewrite;
    std::optional<ReducedSPQRCore> actualAfterRewrite;
    std::optional<ReducedSPQRCore> actualAfterNormalize;
    std::optional<bool> actualInvariantOk;
    std::string actualInvariantWhy;
    std::string actualInvariantDetailedSubtype;
    std::optional<ExplicitBlockGraph> explicitAfter;
    std::optional<bool> oracleEquivalentOk;
    std::string oracleWhy;
    SolverBaselineStage baselineStage = SolverBaselineStage::BASELINE_DONE;
    std::optional<NodeId> firstFailingNodeId;
    SolverBaselineInvariantKind firstFailingInvariantKind =
        SolverBaselineInvariantKind::NONE;
    std::vector<NodeId> deadRelayCandidateNodes;
    bool sameTypeSPPresent = false;
    bool adjacencyMismatchPresent = false;
    std::vector<SolverBaselineReplayStepSnapshot> stepSnapshots;
};

enum class SemanticDivergenceKind : uint8_t {
    SDK_NONE,
    SDK_CHOICE_R_DIFFER,
    SDK_CHOICE_X_DIFFER,
    SDK_POST_DELETE_EXPLICIT_DIFFER,
    SDK_POST_NORMALIZE_EXPLICIT_DIFFER,
    SDK_TERMINATION_DIFFER,
    SDK_EDGESET_ONLY_MATCH_VERTEXSET_DIFFER,
    SDK_OTHER
};

inline const char *semanticDivergenceKindName(SemanticDivergenceKind kind) {
    switch (kind) {
        case SemanticDivergenceKind::SDK_NONE: return "SDK_NONE";
        case SemanticDivergenceKind::SDK_CHOICE_R_DIFFER:
            return "SDK_CHOICE_R_DIFFER";
        case SemanticDivergenceKind::SDK_CHOICE_X_DIFFER:
            return "SDK_CHOICE_X_DIFFER";
        case SemanticDivergenceKind::SDK_POST_DELETE_EXPLICIT_DIFFER:
            return "SDK_POST_DELETE_EXPLICIT_DIFFER";
        case SemanticDivergenceKind::SDK_POST_NORMALIZE_EXPLICIT_DIFFER:
            return "SDK_POST_NORMALIZE_EXPLICIT_DIFFER";
        case SemanticDivergenceKind::SDK_TERMINATION_DIFFER:
            return "SDK_TERMINATION_DIFFER";
        case SemanticDivergenceKind::SDK_EDGESET_ONLY_MATCH_VERTEXSET_DIFFER:
            return "SDK_EDGESET_ONLY_MATCH_VERTEXSET_DIFFER";
        case SemanticDivergenceKind::SDK_OTHER: return "SDK_OTHER";
    }
    return "SDK_UNKNOWN";
}

enum class CanonicalDivergenceKind : uint8_t {
    CDK_NONE,
    CDK_CHOICE_R_DIFFER,
    CDK_CHOICE_X_DIFFER,
    CDK_POST_DELETE_EXPLICIT_DIFFER,
    CDK_POST_NORMALIZE_EXPLICIT_DIFFER,
    CDK_TERMINATION_DIFFER,
    CDK_FINAL_EXPLICIT_DIFFER,
    CDK_OTHER
};

inline const char *canonicalDivergenceKindName(CanonicalDivergenceKind kind) {
    switch (kind) {
        case CanonicalDivergenceKind::CDK_NONE: return "CDK_NONE";
        case CanonicalDivergenceKind::CDK_CHOICE_R_DIFFER: return "CDK_CHOICE_R_DIFFER";
        case CanonicalDivergenceKind::CDK_CHOICE_X_DIFFER: return "CDK_CHOICE_X_DIFFER";
        case CanonicalDivergenceKind::CDK_POST_DELETE_EXPLICIT_DIFFER:
            return "CDK_POST_DELETE_EXPLICIT_DIFFER";
        case CanonicalDivergenceKind::CDK_POST_NORMALIZE_EXPLICIT_DIFFER:
            return "CDK_POST_NORMALIZE_EXPLICIT_DIFFER";
        case CanonicalDivergenceKind::CDK_TERMINATION_DIFFER:
            return "CDK_TERMINATION_DIFFER";
        case CanonicalDivergenceKind::CDK_FINAL_EXPLICIT_DIFFER:
            return "CDK_FINAL_EXPLICIT_DIFFER";
        case CanonicalDivergenceKind::CDK_OTHER: return "CDK_OTHER";
    }
    return "CDK_UNKNOWN";
}

enum class SemanticReplayStopPolicy : uint8_t {
    SRSP_RAW_FIRST_DIFF,
    SRSP_CANONICAL_FIRST_DIFF,
    SRSP_RUN_TO_END
};

inline const char *semanticReplayStopPolicyName(SemanticReplayStopPolicy policy) {
    switch (policy) {
        case SemanticReplayStopPolicy::SRSP_RAW_FIRST_DIFF:
            return "SRSP_RAW_FIRST_DIFF";
        case SemanticReplayStopPolicy::SRSP_CANONICAL_FIRST_DIFF:
            return "SRSP_CANONICAL_FIRST_DIFF";
        case SemanticReplayStopPolicy::SRSP_RUN_TO_END: return "SRSP_RUN_TO_END";
    }
    return "SRSP_UNKNOWN";
}

struct SemanticStepTrace {
    int stepIndex = -1;
    std::string side;
    ExplicitBlockGraph explicitBefore;
    CanonicalExplicitGraph canonicalExplicitBefore;
    int chosenR = -1;
    int chosenX = -1;
    ExplicitBlockGraph explicitAfterDelete;
    ExplicitBlockGraph explicitAfterNormalize;
    CanonicalExplicitGraph canonicalExplicitAfterDelete;
    CanonicalExplicitGraph canonicalExplicitAfterNormalize;
    bool actualInvariantOk = false;
    std::string actualInvariantWhy;
    std::string debugTag;
    bool stepOk = false;
    bool terminated = false;
    std::string terminateReason;
};

struct SolverSemanticReplayBundle {
    std::string caseName;
    std::string manifestPath;
    SemanticReplayStopPolicy stopPolicy = SemanticReplayStopPolicy::SRSP_RAW_FIRST_DIFF;
    SemanticDivergenceKind divergenceKind = SemanticDivergenceKind::SDK_NONE;
    int divergenceStepIndex = -1;
    std::string divergenceWhy;
    SemanticDivergenceKind rawFirstDivergenceKind = SemanticDivergenceKind::SDK_NONE;
    int rawFirstDivergenceStep = -1;
    std::string rawFirstDivergenceWhy;
    CanonicalDivergenceKind canonicalFirstDivergenceKind =
        CanonicalDivergenceKind::CDK_NONE;
    int canonicalFirstDivergenceStep = -1;
    std::string canonicalFirstDivergenceWhy;
    ExplicitBlockGraph explicitInput;
    std::optional<CanonicalExplicitGraph> oracleCanonicalExplicit;
    std::optional<CanonicalExplicitGraph> rewriteCanonicalExplicit;
    std::vector<SemanticStepTrace> oracleTrace;
    std::vector<SemanticStepTrace> rewriteTrace;
    std::string firstOracleWhy;
    std::string firstRewriteWhy;
    int oracleTerminatedStep = -1;
    int rewriteTerminatedStep = -1;
    bool canonicalEquivalent = false;
    std::string canonicalWhy;
    bool finalCanonicalEquivalent = false;
    bool finalRawEquivalent = false;
    bool topLevelOk = false;
};

struct SemanticReplayResult {
    bool ok = false;
    std::vector<SemanticStepTrace> steps;
    ExplicitBlockGraph finalExplicitRaw;
    CanonicalExplicitGraph finalExplicitCanonical;
    int terminatedStep = -1;
    std::string why;
};

enum class SemanticTargetSeamKind : uint8_t {
    STSK_NONE,
    STSK_ORACLE_HAS_TARGET_REWRITE_NO_TARGET_SHADOW_HAS_TARGET,
    STSK_ORACLE_HAS_TARGET_REWRITE_NO_TARGET_SHADOW_NO_TARGET,
    STSK_REWRITE_HAS_TARGET_ORACLE_NO_TARGET,
    STSK_BOTH_HAVE_TARGET_CHOICE_DIFFER,
    STSK_CANONICAL_EXPLICIT_DIFFER,
    STSK_OTHER
};

inline const char *semanticTargetSeamKindName(SemanticTargetSeamKind kind) {
    switch (kind) {
        case SemanticTargetSeamKind::STSK_NONE: return "STSK_NONE";
        case SemanticTargetSeamKind::
            STSK_ORACLE_HAS_TARGET_REWRITE_NO_TARGET_SHADOW_HAS_TARGET:
            return "STSK_ORACLE_HAS_TARGET_REWRITE_NO_TARGET_SHADOW_HAS_TARGET";
        case SemanticTargetSeamKind::
            STSK_ORACLE_HAS_TARGET_REWRITE_NO_TARGET_SHADOW_NO_TARGET:
            return "STSK_ORACLE_HAS_TARGET_REWRITE_NO_TARGET_SHADOW_NO_TARGET";
        case SemanticTargetSeamKind::STSK_REWRITE_HAS_TARGET_ORACLE_NO_TARGET:
            return "STSK_REWRITE_HAS_TARGET_ORACLE_NO_TARGET";
        case SemanticTargetSeamKind::STSK_BOTH_HAVE_TARGET_CHOICE_DIFFER:
            return "STSK_BOTH_HAVE_TARGET_CHOICE_DIFFER";
        case SemanticTargetSeamKind::STSK_CANONICAL_EXPLICIT_DIFFER:
            return "STSK_CANONICAL_EXPLICIT_DIFFER";
        case SemanticTargetSeamKind::STSK_OTHER: return "STSK_OTHER";
    }
    return "STSK_UNKNOWN";
}

enum class CompareAssemblySeamKind : uint8_t {
    CASK_NONE,
    CASK_BASELINE_OUTPUT_NOT_EQUAL_REPLAY_FINAL,
    CASK_REWRITE_OUTPUT_NOT_EQUAL_REPLAY_FINAL,
    CASK_COMPARE_CANONICALIZATION_MISMATCH,
    CASK_COMPARE_SELECTION_OR_ROUTING_MISMATCH,
    CASK_OTHER
};

inline const char *compareAssemblySeamKindName(CompareAssemblySeamKind kind) {
    switch (kind) {
        case CompareAssemblySeamKind::CASK_NONE: return "CASK_NONE";
        case CompareAssemblySeamKind::CASK_BASELINE_OUTPUT_NOT_EQUAL_REPLAY_FINAL:
            return "CASK_BASELINE_OUTPUT_NOT_EQUAL_REPLAY_FINAL";
        case CompareAssemblySeamKind::CASK_REWRITE_OUTPUT_NOT_EQUAL_REPLAY_FINAL:
            return "CASK_REWRITE_OUTPUT_NOT_EQUAL_REPLAY_FINAL";
        case CompareAssemblySeamKind::CASK_COMPARE_CANONICALIZATION_MISMATCH:
            return "CASK_COMPARE_CANONICALIZATION_MISMATCH";
        case CompareAssemblySeamKind::CASK_COMPARE_SELECTION_OR_ROUTING_MISMATCH:
            return "CASK_COMPARE_SELECTION_OR_ROUTING_MISMATCH";
        case CompareAssemblySeamKind::CASK_OTHER: return "CASK_OTHER";
    }
    return "CASK_UNKNOWN";
}

struct SolverCompareReplayBundle {
    std::string caseName;
    std::string manifestPath;
    uint64_t seed = 0;
    int tc = -1;
    std::optional<int> targetStep;
    bool topLevelOk = false;
    ExplicitBlockGraph inputExplicit;
    CompareAssemblySeamKind compareAssemblySeamKind =
        CompareAssemblySeamKind::CASK_NONE;
    std::string compareAssemblyWhy;
    std::string firstMismatchDescription;
    std::optional<SolverOutput> oracleSolverOutput;
    std::optional<SolverOutput> rewriteSolverOutput;
    std::optional<CanonicalExplicitGraph> oracleSolverOutputCanonical;
    std::optional<CanonicalExplicitGraph> rewriteSolverOutputCanonical;
    std::optional<SemanticReplayResult> oracleReplay;
    std::optional<SemanticReplayResult> rewriteReplay;
    std::optional<ExplicitBlockGraph> oracleReplayFinalRaw;
    std::optional<ExplicitBlockGraph> rewriteReplayFinalRaw;
    std::optional<CanonicalExplicitGraph> oracleReplayFinalCanonical;
    std::optional<CanonicalExplicitGraph> rewriteReplayFinalCanonical;
    std::optional<bool> oracleSolverVsReplayEqualRaw;
    std::optional<bool> oracleSolverVsReplayEqualCanonical;
    std::optional<bool> rewriteSolverVsReplayEqualRaw;
    std::optional<bool> rewriteSolverVsReplayEqualCanonical;
    std::optional<bool> oracleVsRewriteEqualCanonical;
    std::optional<bool> oracleVsRewriteEqualRaw;
    int oracleReplayTerminatedStep = -1;
    int rewriteReplayTerminatedStep = -1;
    std::string oracleSolverWhy;
    std::string rewriteSolverWhy;
    std::string oracleReplayWhy;
    std::string rewriteReplayWhy;
    std::string rewriteSolverOutputDebugTag;
    std::string rewriteTerminalAssemblyWhy;
};

enum class FinalCoreSeamKind : uint8_t {
    FCSK_NONE,
    FCSK_STEP_COUNT_DIFFER,
    FCSK_CHOSEN_R_DIFFER,
    FCSK_CHOSEN_X_DIFFER,
    FCSK_POST_NORMALIZE_EXPLICIT_DIFFER,
    FCSK_TERMINATION_DIFFER,
    FCSK_FINAL_CORE_EXPLICIT_DIFFER,
    FCSK_FINAL_CORE_METADATA_DIFFER,
    FCSK_OTHER
};

inline const char *finalCoreSeamKindName(FinalCoreSeamKind kind) {
    switch (kind) {
        case FinalCoreSeamKind::FCSK_NONE: return "FCSK_NONE";
        case FinalCoreSeamKind::FCSK_STEP_COUNT_DIFFER: return "FCSK_STEP_COUNT_DIFFER";
        case FinalCoreSeamKind::FCSK_CHOSEN_R_DIFFER: return "FCSK_CHOSEN_R_DIFFER";
        case FinalCoreSeamKind::FCSK_CHOSEN_X_DIFFER: return "FCSK_CHOSEN_X_DIFFER";
        case FinalCoreSeamKind::FCSK_POST_NORMALIZE_EXPLICIT_DIFFER:
            return "FCSK_POST_NORMALIZE_EXPLICIT_DIFFER";
        case FinalCoreSeamKind::FCSK_TERMINATION_DIFFER:
            return "FCSK_TERMINATION_DIFFER";
        case FinalCoreSeamKind::FCSK_FINAL_CORE_EXPLICIT_DIFFER:
            return "FCSK_FINAL_CORE_EXPLICIT_DIFFER";
        case FinalCoreSeamKind::FCSK_FINAL_CORE_METADATA_DIFFER:
            return "FCSK_FINAL_CORE_METADATA_DIFFER";
        case FinalCoreSeamKind::FCSK_OTHER: return "FCSK_OTHER";
    }
    return "FCSK_UNKNOWN";
}

struct FinalCoreStepTrace {
    int stepIndex = -1;
    int chosenR = -1;
    int chosenX = -1;
    CanonicalExplicitGraph canonicalExplicitBefore;
    CanonicalExplicitGraph canonicalExplicitAfterNormalize;
    bool terminated = false;
    std::string terminateReason;
};

struct FinalCoreSignature {
    int aliveNodeCount = 0;
    int aliveArcCount = 0;
    int root = -1;
    CanonicalExplicitGraph canonicalExplicit;
    std::vector<std::string> nodeSummaries;
};

struct RewriteTargetCandidate {
    int rNode = -1;
    int x = -1;
    int scorePrimary = 0;
    int scoreSecondary = 0;
    std::string sourceTag;
};

inline bool operator==(const RewriteTargetCandidate &lhs,
                       const RewriteTargetCandidate &rhs) {
    return lhs.rNode == rhs.rNode && lhs.x == rhs.x &&
           lhs.scorePrimary == rhs.scorePrimary &&
           lhs.scoreSecondary == rhs.scoreSecondary &&
           lhs.sourceTag == rhs.sourceTag;
}

struct RewriteTargetSnapshot {
    int stepIndex = -1;
    std::vector<int> aliveRNodes;
    std::vector<RewriteTargetCandidate> candidates;
    bool hasNextTarget = false;
    int chosenR = -1;
    int chosenX = -1;
    std::string noTargetReason;
};

struct SemanticTargetSnapshot {
    int stepIndex = -1;
    std::string side;
    CanonicalExplicitGraph canonicalExplicitAfterNormalize;
    std::vector<int> aliveRNodes;
    std::vector<RewriteTargetCandidate> candidates;
    bool hasNextTarget = false;
    int chosenR = -1;
    int chosenX = -1;
    std::string noTargetReason;
};

struct SolverSemanticTargetReplayBundle {
    std::string caseName;
    std::string manifestPath;
    uint64_t seed = 0;
    int tc = -1;
    std::optional<int> targetStep;
    bool topLevelOk = false;
    ExplicitBlockGraph explicitInput;
    SemanticTargetSeamKind semanticTargetSeamKind = SemanticTargetSeamKind::STSK_NONE;
    int divergenceStepIndex = -1;
    std::string semanticTargetSeamWhy;
    SemanticTargetSnapshot oracleTargetSnapshot;
    SemanticTargetSnapshot rewriteTargetSnapshot;
    SemanticTargetSnapshot shadowTargetSnapshot;
    std::vector<SemanticStepTrace> oracleSemanticTrace;
    std::vector<SemanticStepTrace> rewriteSemanticTrace;
    std::string oracleWhy;
    std::string rewriteWhy;
    std::string shadowWhy;
};

enum class TransitionSeamKind : uint8_t {
    TRSK_NONE,
    TRSK_CHOICE_DIFFER,
    TRSK_ORACLE_MATCHES_SHADOW_DELETE_REWRITE_DIFFERS,
    TRSK_REWRITE_MATCHES_SHADOW_DELETE_ORACLE_DIFFERS,
    TRSK_BOTH_DIFFER_FROM_SHADOW_DELETE,
    TRSK_DELETE_EQUAL_STEP_DIFFER,
    TRSK_ORACLE_SIDE_STEP_SEMANTICS_DIFFER,
    TRSK_OTHER
};

inline const char *transitionSeamKindName(TransitionSeamKind kind) {
    switch (kind) {
        case TransitionSeamKind::TRSK_NONE: return "TRSK_NONE";
        case TransitionSeamKind::TRSK_CHOICE_DIFFER:
            return "TRSK_CHOICE_DIFFER";
        case TransitionSeamKind::
            TRSK_ORACLE_MATCHES_SHADOW_DELETE_REWRITE_DIFFERS:
            return "TRSK_ORACLE_MATCHES_SHADOW_DELETE_REWRITE_DIFFERS";
        case TransitionSeamKind::
            TRSK_REWRITE_MATCHES_SHADOW_DELETE_ORACLE_DIFFERS:
            return "TRSK_REWRITE_MATCHES_SHADOW_DELETE_ORACLE_DIFFERS";
        case TransitionSeamKind::TRSK_BOTH_DIFFER_FROM_SHADOW_DELETE:
            return "TRSK_BOTH_DIFFER_FROM_SHADOW_DELETE";
        case TransitionSeamKind::TRSK_DELETE_EQUAL_STEP_DIFFER:
            return "TRSK_DELETE_EQUAL_STEP_DIFFER";
        case TransitionSeamKind::TRSK_ORACLE_SIDE_STEP_SEMANTICS_DIFFER:
            return "TRSK_ORACLE_SIDE_STEP_SEMANTICS_DIFFER";
        case TransitionSeamKind::TRSK_OTHER: return "TRSK_OTHER";
    }
    return "TRSK_UNKNOWN";
}

struct TransitionReplaySnapshot {
    int sourceStep = -1;
    CanonicalExplicitGraph explicitBefore;
    int chosenR = -1;
    int chosenX = -1;
    CanonicalExplicitGraph explicitAfterDelete;
    CanonicalExplicitGraph explicitAfterStep;
    bool terminated = false;
    std::string terminateReason;
    bool ok = false;
    std::string why;
};

struct SolverSemanticTransitionReplayBundle {
    std::string caseName;
    std::string manifestPath;
    uint64_t seed = 0;
    int tc = -1;
    std::optional<int> targetStep;
    int sourceStep = -1;
    bool topLevelOk = false;
    TransitionSeamKind transitionSeamKind = TransitionSeamKind::TRSK_OTHER;
    std::string transitionSeamWhy;
    ExplicitBlockGraph sharedExplicitBefore;
    CanonicalExplicitGraph sharedExplicitBeforeCanonical;
    TransitionReplaySnapshot oracleTransitionSnapshot;
    TransitionReplaySnapshot rewriteTransitionSnapshot;
    CanonicalExplicitGraph shadowDeleteExplicit;
    std::string sharedExplicitWhy;
    std::string oracleWhy;
    std::string rewriteWhy;
    std::string shadowWhy;
};

enum class StepHandoffSourceKind : uint8_t {
    SHSK_NONE_TERMINATE,
    SHSK_FROM_DELETE,
    SHSK_FROM_NORMALIZE,
    SHSK_OTHER
};

inline const char *stepHandoffSourceKindName(StepHandoffSourceKind kind) {
    switch (kind) {
        case StepHandoffSourceKind::SHSK_NONE_TERMINATE:
            return "SHSK_NONE_TERMINATE";
        case StepHandoffSourceKind::SHSK_FROM_DELETE:
            return "SHSK_FROM_DELETE";
        case StepHandoffSourceKind::SHSK_FROM_NORMALIZE:
            return "SHSK_FROM_NORMALIZE";
        case StepHandoffSourceKind::SHSK_OTHER: return "SHSK_OTHER";
    }
    return "SHSK_UNKNOWN";
}

enum class HandoffSeamKind : uint8_t {
    HSK_NONE,
    HSK_ORACLE_DELETE_REWRITE_NORMALIZE,
    HSK_ORACLE_DELETE_REWRITE_TERMINATE,
    HSK_ORACLE_TERMINATE_REWRITE_NORMALIZE,
    HSK_BOTH_SAME_SOURCE_BUT_NEXT_INPUT_DIFFERS,
    HSK_SHARED_INPUT_EXTRACTION_BUG,
    HSK_OTHER
};

inline const char *handoffSeamKindName(HandoffSeamKind kind) {
    switch (kind) {
        case HandoffSeamKind::HSK_NONE: return "HSK_NONE";
        case HandoffSeamKind::HSK_ORACLE_DELETE_REWRITE_NORMALIZE:
            return "HSK_ORACLE_DELETE_REWRITE_NORMALIZE";
        case HandoffSeamKind::HSK_ORACLE_DELETE_REWRITE_TERMINATE:
            return "HSK_ORACLE_DELETE_REWRITE_TERMINATE";
        case HandoffSeamKind::HSK_ORACLE_TERMINATE_REWRITE_NORMALIZE:
            return "HSK_ORACLE_TERMINATE_REWRITE_NORMALIZE";
        case HandoffSeamKind::HSK_BOTH_SAME_SOURCE_BUT_NEXT_INPUT_DIFFERS:
            return "HSK_BOTH_SAME_SOURCE_BUT_NEXT_INPUT_DIFFERS";
        case HandoffSeamKind::HSK_SHARED_INPUT_EXTRACTION_BUG:
            return "HSK_SHARED_INPUT_EXTRACTION_BUG";
        case HandoffSeamKind::HSK_OTHER: return "HSK_OTHER";
    }
    return "HSK_UNKNOWN";
}

struct StepHandoffSnapshot {
    int stepIndex = -1;
    std::string side;
    CanonicalExplicitGraph explicitBefore;
    int chosenR = -1;
    int chosenX = -1;
    CanonicalExplicitGraph explicitAfterDelete;
    CanonicalExplicitGraph explicitAfterNormalize;
    CanonicalExplicitGraph nextInputExplicit;
    StepHandoffSourceKind nextInputSourceKind =
        StepHandoffSourceKind::SHSK_OTHER;
    bool terminated = false;
    std::string terminateReason;
};

struct SolverHandoffReplayBundle {
    std::string caseName;
    std::string manifestPath;
    uint64_t seed = 0;
    int tc = -1;
    std::optional<int> targetStep;
    bool topLevelOk = false;
    int handoffStepIndex = -1;
    HandoffSeamKind handoffSeamKind = HandoffSeamKind::HSK_OTHER;
    std::string handoffSeamWhy;
    ExplicitBlockGraph transitionSharedExplicit;
    CanonicalExplicitGraph transitionSharedExplicitCanonical;
    bool oracleNextMatchesTransitionShared = false;
    bool rewriteNextMatchesTransitionShared = false;
    StepHandoffSnapshot oracleHandoffSnapshot;
    StepHandoffSnapshot rewriteHandoffSnapshot;
    std::vector<StepHandoffSnapshot> oracleHandoffTrace;
    std::vector<StepHandoffSnapshot> rewriteHandoffTrace;
    std::string transitionSharedWhy;
    std::string oracleWhy;
    std::string rewriteWhy;
};

struct SolverHandoffPolicyReplayBundle {
    std::string caseName;
    std::string manifestPath;
    uint64_t seed = 0;
    int tc = -1;
    std::optional<int> targetStep;
    bool topLevelOk = false;
    OracleHandoffPolicy oracleHandoffPolicy =
        OracleHandoffPolicy::OHP_DELETE_EXPLICIT;
    CanonicalExplicitGraph oracleDeletePolicyFinalExplicit;
    CanonicalExplicitGraph oracleNormalizePolicyFinalExplicit;
    CanonicalExplicitGraph rewriteFinalExplicit;
    bool deleteVsRewriteCanonicalEqual = false;
    bool normalizeVsRewriteCanonicalEqual = false;
    bool deleteVsNormalizeCanonicalEqual = false;
    std::vector<StepHandoffSnapshot> oracleDeleteTrace;
    std::vector<StepHandoffSnapshot> oracleNormalizeTrace;
    std::vector<StepHandoffSnapshot> rewriteTrace;
    std::string oracleDeleteWhy;
    std::string oracleNormalizeWhy;
    std::string rewriteWhy;
};

enum class StepTransitionSeamKind : uint8_t {
    STSK_STEP1_HANDOFF_EXPLICIT_DIFFER,
    STSK_STEP1_HANDOFF_EQUAL_STEP2_LIVE_CORE_DIFFER,
    STSK_STEP1_HANDOFF_EQUAL_STEP2_SHADOW_MATCHES_REPLAY,
    STSK_STEP1_HANDOFF_EQUAL_STEP2_SHADOW_MATCHES_SOLVER,
    STSK_STEP1_HANDOFF_EQUAL_BOTH_DIFFER_FROM_SHADOW,
    STSK_OTHER
};

inline const char *stepTransitionSeamKindName(StepTransitionSeamKind kind) {
    switch (kind) {
        case StepTransitionSeamKind::STSK_STEP1_HANDOFF_EXPLICIT_DIFFER:
            return "STSK_STEP1_HANDOFF_EXPLICIT_DIFFER";
        case StepTransitionSeamKind::STSK_STEP1_HANDOFF_EQUAL_STEP2_LIVE_CORE_DIFFER:
            return "STSK_STEP1_HANDOFF_EQUAL_STEP2_LIVE_CORE_DIFFER";
        case StepTransitionSeamKind::STSK_STEP1_HANDOFF_EQUAL_STEP2_SHADOW_MATCHES_REPLAY:
            return "STSK_STEP1_HANDOFF_EQUAL_STEP2_SHADOW_MATCHES_REPLAY";
        case StepTransitionSeamKind::STSK_STEP1_HANDOFF_EQUAL_STEP2_SHADOW_MATCHES_SOLVER:
            return "STSK_STEP1_HANDOFF_EQUAL_STEP2_SHADOW_MATCHES_SOLVER";
        case StepTransitionSeamKind::STSK_STEP1_HANDOFF_EQUAL_BOTH_DIFFER_FROM_SHADOW:
            return "STSK_STEP1_HANDOFF_EQUAL_BOTH_DIFFER_FROM_SHADOW";
        case StepTransitionSeamKind::STSK_OTHER: return "STSK_OTHER";
    }
    return "STSK_UNKNOWN";
}

struct StepTransitionSnapshot {
    std::string side;
    int sourceStep = -1;
    CanonicalExplicitGraph explicitBeforeStep;
    int chosenR = -1;
    int chosenX = -1;
    CanonicalExplicitGraph explicitAfterDelete;
    CanonicalExplicitGraph explicitAfterNormalize;
    CanonicalExplicitGraph nextInputExplicit;
    ExplicitBlockGraph nextInputExplicitRaw;
    std::string nextInputSourceTag;
    bool terminatedAfterStep = false;
    std::string terminateReason;
    std::vector<int> aliveRNodesForNextStep;
    std::vector<RewriteTargetCandidate> nextStepCandidates;
    RewriteTargetSnapshot nextStepTargetSnapshot;
};

struct SolverStepTransitionReplayBundle {
    std::string caseName;
    std::string manifestPath;
    uint64_t seed = 0;
    int tc = -1;
    std::optional<int> targetStep;
    bool topLevelOk = false;
    int sourceStep = 1;
    StepTransitionSeamKind stepTransitionSeamKind =
        StepTransitionSeamKind::STSK_OTHER;
    std::string stepTransitionSeamWhy;
    StepTransitionSnapshot solverStep1Snapshot;
    StepTransitionSnapshot rewriteStep1Snapshot;
    bool hasShadowStep2Snapshot = false;
    StepTransitionSnapshot shadowStep2Snapshot;
    std::string solverWhy;
    std::string rewriteWhy;
    std::string shadowWhy;
};

enum class TargetSearchSeamKind : uint8_t {
    TSK_NONE,
    TSK_ALIVE_R_SET_DIFFER,
    TSK_CANDIDATE_SET_DIFFER,
    TSK_CHOSEN_TARGET_DIFFER,
    TSK_NO_TARGET_SOLVER_ONLY,
    TSK_NO_TARGET_REPLAY_ONLY,
    TSK_TERMINATION_POLICY_DIFFER,
    TSK_OTHER
};

inline const char *targetSearchSeamKindName(TargetSearchSeamKind kind) {
    switch (kind) {
        case TargetSearchSeamKind::TSK_NONE: return "TSK_NONE";
        case TargetSearchSeamKind::TSK_ALIVE_R_SET_DIFFER:
            return "TSK_ALIVE_R_SET_DIFFER";
        case TargetSearchSeamKind::TSK_CANDIDATE_SET_DIFFER:
            return "TSK_CANDIDATE_SET_DIFFER";
        case TargetSearchSeamKind::TSK_CHOSEN_TARGET_DIFFER:
            return "TSK_CHOSEN_TARGET_DIFFER";
        case TargetSearchSeamKind::TSK_NO_TARGET_SOLVER_ONLY:
            return "TSK_NO_TARGET_SOLVER_ONLY";
        case TargetSearchSeamKind::TSK_NO_TARGET_REPLAY_ONLY:
            return "TSK_NO_TARGET_REPLAY_ONLY";
        case TargetSearchSeamKind::TSK_TERMINATION_POLICY_DIFFER:
            return "TSK_TERMINATION_POLICY_DIFFER";
        case TargetSearchSeamKind::TSK_OTHER: return "TSK_OTHER";
    }
    return "TSK_UNKNOWN";
}

struct SolverFinalCoreReplayStatsSnapshot {
    bool success = false;
    bool reachedFixpoint = false;
    bool hadSequenceFallback = false;
    bool maxStepReached = false;
    int completedSteps = 0;
    uint64_t solverShadowResyncAttemptCount = 0;
    uint64_t solverShadowResyncAppliedCount = 0;
    uint64_t solverShadowResyncNoopCount = 0;
    uint64_t solverShadowResyncNoTargetToHasTargetCount = 0;
    uint64_t solverShadowResyncAliveRSetDifferCount = 0;
};

struct SolverFinalCoreReplayBundle {
    std::string caseName;
    std::string manifestPath;
    uint64_t seed = 0;
    int tc = -1;
    std::optional<int> targetStep;
    bool topLevelOk = false;
    FinalCoreSeamKind finalCoreSeamKind = FinalCoreSeamKind::FCSK_NONE;
    int firstDivergenceStep = -1;
    std::string finalCoreSeamWhy;
    FinalCoreSignature solverFinalCoreSignature;
    FinalCoreSignature replayFinalCoreSignature;
    std::vector<FinalCoreStepTrace> solverTrace;
    std::vector<FinalCoreStepTrace> replayTrace;
    std::vector<RewriteTargetSnapshot> solverPostStepTargetSnapshots;
    std::vector<RewriteTargetSnapshot> replayPostStepTargetSnapshots;
    int targetSearchComparedStep = -1;
    TargetSearchSeamKind targetSearchSeamKind = TargetSearchSeamKind::TSK_NONE;
    std::string targetSearchSeamWhy;
    SolverFinalCoreReplayStatsSnapshot solverStats;
    std::string solverWhy;
    std::string replayWhy;
};

enum class CoreShapeSeamKind : uint8_t {
    CSS_NONE,
    CSS_SOLVER_DIFFERS_SHADOW_REPLAY_MATCHES,
    CSS_REPLAY_DIFFERS_SHADOW_SOLVER_MATCHES,
    CSS_SOLVER_AND_REPLAY_BOTH_DIFFER_FROM_SHADOW,
    CSS_ALL_THREE_DIFFER,
    CSS_OTHER
};

inline const char *coreShapeSeamKindName(CoreShapeSeamKind kind) {
    switch (kind) {
        case CoreShapeSeamKind::CSS_NONE: return "CSS_NONE";
        case CoreShapeSeamKind::CSS_SOLVER_DIFFERS_SHADOW_REPLAY_MATCHES:
            return "CSS_SOLVER_DIFFERS_SHADOW_REPLAY_MATCHES";
        case CoreShapeSeamKind::CSS_REPLAY_DIFFERS_SHADOW_SOLVER_MATCHES:
            return "CSS_REPLAY_DIFFERS_SHADOW_SOLVER_MATCHES";
        case CoreShapeSeamKind::CSS_SOLVER_AND_REPLAY_BOTH_DIFFER_FROM_SHADOW:
            return "CSS_SOLVER_AND_REPLAY_BOTH_DIFFER_FROM_SHADOW";
        case CoreShapeSeamKind::CSS_ALL_THREE_DIFFER:
            return "CSS_ALL_THREE_DIFFER";
        case CoreShapeSeamKind::CSS_OTHER: return "CSS_OTHER";
    }
    return "CSS_UNKNOWN";
}

struct CoreShapeSnapshot {
    int stepIndex = -1;
    std::string side;
    int root = -1;
    int aliveNodeCount = 0;
    int aliveArcCount = 0;
    int rNodeCount = 0;
    int sNodeCount = 0;
    int pNodeCount = 0;
    std::vector<int> aliveRNodes;
    RewriteTargetSnapshot targetSnapshot;
    std::vector<std::string> nodeSummaries;
    CanonicalExplicitGraph canonicalExplicit;
};

struct SolverShapeReplayBundle {
    std::string caseName;
    std::string manifestPath;
    uint64_t seed = 0;
    int tc = -1;
    std::optional<int> targetStep;
    bool topLevelOk = false;
    int seamStepIndex = -1;
    CoreShapeSeamKind coreShapeSeamKind = CoreShapeSeamKind::CSS_NONE;
    std::string coreShapeSeamWhy;
    CoreShapeSnapshot solverShapeSnapshot;
    CoreShapeSnapshot replayShapeSnapshot;
    CoreShapeSnapshot shadowShapeSnapshot;
    std::string solverWhy;
    std::string replayWhy;
    std::string shadowWhy;
};

enum class BuilderPipelineSeamKind : uint8_t {
    BPSK_NONE,
    BPSK_RAW_TYPE_DIVERGENCE,
    BPSK_MINI_BEFORE_NORMALIZE_DIVERGENCE,
    BPSK_MINI_AFTER_NORMALIZE_DIVERGENCE,
    BPSK_CORE_AFTER_MATERIALIZE_DIVERGENCE,
    BPSK_CORE_AFTER_FINAL_NORMALIZE_DIVERGENCE,
    BPSK_OTHER
};

inline const char *builderPipelineSeamKindName(BuilderPipelineSeamKind kind) {
    switch (kind) {
        case BuilderPipelineSeamKind::BPSK_NONE: return "BPSK_NONE";
        case BuilderPipelineSeamKind::BPSK_RAW_TYPE_DIVERGENCE:
            return "BPSK_RAW_TYPE_DIVERGENCE";
        case BuilderPipelineSeamKind::BPSK_MINI_BEFORE_NORMALIZE_DIVERGENCE:
            return "BPSK_MINI_BEFORE_NORMALIZE_DIVERGENCE";
        case BuilderPipelineSeamKind::BPSK_MINI_AFTER_NORMALIZE_DIVERGENCE:
            return "BPSK_MINI_AFTER_NORMALIZE_DIVERGENCE";
        case BuilderPipelineSeamKind::BPSK_CORE_AFTER_MATERIALIZE_DIVERGENCE:
            return "BPSK_CORE_AFTER_MATERIALIZE_DIVERGENCE";
        case BuilderPipelineSeamKind::BPSK_CORE_AFTER_FINAL_NORMALIZE_DIVERGENCE:
            return "BPSK_CORE_AFTER_FINAL_NORMALIZE_DIVERGENCE";
        case BuilderPipelineSeamKind::BPSK_OTHER: return "BPSK_OTHER";
    }
    return "BPSK_UNKNOWN";
}

struct BuilderStageSnapshot {
    std::string stage;
    CanonicalExplicitGraph canonicalExplicit;
    int aliveNodeCount = 0;
    int aliveArcCount = 0;
    int rNodeCount = 0;
    int sNodeCount = 0;
    int pNodeCount = 0;
    int root = -1;
    std::vector<std::string> nodeSummaries;
};

struct ExplicitCoreBuilderReplayBundle {
    std::string caseName;
    std::string manifestPath;
    uint64_t seed = 0;
    int tc = -1;
    std::optional<int> targetStep;
    int sourceStep = -1;
    std::string sourceKind = "step";
    std::string sourceSide;
    bool topLevelOk = false;
    BuilderPipelineSeamKind builderPipelineSeamKind = BuilderPipelineSeamKind::BPSK_NONE;
    std::string builderPipelineSeamWhy;
    ExplicitBlockGraph inputExplicit;
    StepTransitionSnapshot solverStep1Snapshot;
    StepTransitionSnapshot rewriteStep1Snapshot;
    CoreShapeSnapshot sourceLiveCore;
    CoreShapeSnapshot solverLiveCore;
    CoreShapeSnapshot replayLiveCore;
    BuilderStageSnapshot rawSnapshot;
    BuilderStageSnapshot miniBeforeSnapshot;
    BuilderStageSnapshot miniAfterSnapshot;
    BuilderStageSnapshot coreAfterMaterializeSnapshot;
    BuilderStageSnapshot coreAfterNormalizeSnapshot;
    std::string sourceWhy;
    std::string builderWhy;
};

enum class RawReplaySeamKind : uint8_t {
    RRSK_EXPLICIT_TO_COMPACT_DIFFER,
    RRSK_RAW_NODETYPE_ONLY_DIFFER,
    RRSK_RAW_STRUCTURE_DIFFER,
    RRSK_RAW_SPLITS_SINGLE_R_TO_S_PLUS_R,
    RRSK_RAW_SPLITS_SINGLE_R_TO_P_PLUS_R,
    RRSK_OTHER
};

inline const char *rawReplaySeamKindName(RawReplaySeamKind kind) {
    switch (kind) {
        case RawReplaySeamKind::RRSK_EXPLICIT_TO_COMPACT_DIFFER:
            return "RRSK_EXPLICIT_TO_COMPACT_DIFFER";
        case RawReplaySeamKind::RRSK_RAW_NODETYPE_ONLY_DIFFER:
            return "RRSK_RAW_NODETYPE_ONLY_DIFFER";
        case RawReplaySeamKind::RRSK_RAW_STRUCTURE_DIFFER:
            return "RRSK_RAW_STRUCTURE_DIFFER";
        case RawReplaySeamKind::RRSK_RAW_SPLITS_SINGLE_R_TO_S_PLUS_R:
            return "RRSK_RAW_SPLITS_SINGLE_R_TO_S_PLUS_R";
        case RawReplaySeamKind::RRSK_RAW_SPLITS_SINGLE_R_TO_P_PLUS_R:
            return "RRSK_RAW_SPLITS_SINGLE_R_TO_P_PLUS_R";
        case RawReplaySeamKind::RRSK_OTHER: return "RRSK_OTHER";
    }
    return "RRSK_UNKNOWN";
}

struct RawReplaySnapshot {
    CanonicalExplicitGraph inputExplicit;
    CompactGraph compactGraph;
    CompactPrecheckSummary precheckSummary;
    std::string compactGraphCanonicalSummary;
    std::vector<std::string> compactEdgeSummaries;
    std::vector<std::string> rawNodeSummaries;
    std::vector<std::string> rawTreeEdgeSummaries;
    int rawNodeCount = 0;
    int rawTreeEdgeCount = 0;
    int rawRNodeCount = 0;
    int rawSNodeCount = 0;
    int rawPNodeCount = 0;
    std::string why;
    bool ok = false;
};

struct HandoffRawReplayBundle {
    std::string caseName;
    std::string manifestPath;
    uint64_t seed = 0;
    int tc = -1;
    std::optional<int> targetStep;
    int sourceStep = -1;
    bool topLevelOk = false;
    RawReplaySeamKind rawReplaySeamKind = RawReplaySeamKind::RRSK_OTHER;
    std::string rawReplaySeamWhy;
    ExplicitBlockGraph sharedHandoffExplicit;
    CoreShapeSnapshot sourceLiveCoreSnapshot;
    RawReplaySnapshot rawReplaySnapshot;
    std::string sourceWhy;
    std::string rawWhy;
};

enum class CoreMaterializeSubphase : uint8_t {
    CMS_ALLOCATE_NODE,
    CMS_INSTALL_INPUT_SLOTS,
    CMS_CONNECT_INTERNAL_ARCS,
    CMS_POST_MAT_METADATA,
    CMS_FINAL_NORMALIZE
};

inline const char *coreMaterializeSubphaseName(CoreMaterializeSubphase phase) {
    switch (phase) {
        case CoreMaterializeSubphase::CMS_ALLOCATE_NODE:
            return "CMS_ALLOCATE_NODE";
        case CoreMaterializeSubphase::CMS_INSTALL_INPUT_SLOTS:
            return "CMS_INSTALL_INPUT_SLOTS";
        case CoreMaterializeSubphase::CMS_CONNECT_INTERNAL_ARCS:
            return "CMS_CONNECT_INTERNAL_ARCS";
        case CoreMaterializeSubphase::CMS_POST_MAT_METADATA:
            return "CMS_POST_MAT_METADATA";
        case CoreMaterializeSubphase::CMS_FINAL_NORMALIZE:
            return "CMS_FINAL_NORMALIZE";
    }
    return "CMS_UNKNOWN";
}

enum class CoreMaterializeSubphaseSeamKind : uint8_t {
    CMSS_NONE,
    CMSS_TYPE_COERCION_AT_ALLOC,
    CMSS_TYPE_COERCION_AT_POST_METADATA,
    CMSS_STRUCTURE_DIFFER_BEFORE_METADATA,
    CMSS_STRUCTURE_DIFFER_AFTER_METADATA,
    CMSS_OTHER
};

inline const char *coreMaterializeSubphaseSeamKindName(
    CoreMaterializeSubphaseSeamKind kind) {
    switch (kind) {
        case CoreMaterializeSubphaseSeamKind::CMSS_NONE: return "CMSS_NONE";
        case CoreMaterializeSubphaseSeamKind::CMSS_TYPE_COERCION_AT_ALLOC:
            return "CMSS_TYPE_COERCION_AT_ALLOC";
        case CoreMaterializeSubphaseSeamKind::CMSS_TYPE_COERCION_AT_POST_METADATA:
            return "CMSS_TYPE_COERCION_AT_POST_METADATA";
        case CoreMaterializeSubphaseSeamKind::CMSS_STRUCTURE_DIFFER_BEFORE_METADATA:
            return "CMSS_STRUCTURE_DIFFER_BEFORE_METADATA";
        case CoreMaterializeSubphaseSeamKind::CMSS_STRUCTURE_DIFFER_AFTER_METADATA:
            return "CMSS_STRUCTURE_DIFFER_AFTER_METADATA";
        case CoreMaterializeSubphaseSeamKind::CMSS_OTHER: return "CMSS_OTHER";
    }
    return "CMSS_UNKNOWN";
}

struct MaterializeCoreSubphaseSnapshot {
    std::string subphase;
    int aliveNodeCount = 0;
    int aliveArcCount = 0;
    int rNodeCount = 0;
    int sNodeCount = 0;
    int pNodeCount = 0;
    int liveSlotCount = 0;
    std::vector<std::string> nodeSummaries;
    CanonicalExplicitGraph canonicalExplicit;
};

struct MaterializeCoreReplayBundle {
    std::string caseName;
    std::string manifestPath;
    uint64_t seed = 0;
    int tc = -1;
    std::optional<int> targetStep;
    int sourceStep = -1;
    std::string sourceSide;
    bool topLevelOk = false;
    CoreMaterializeSubphaseSeamKind seamKind =
        CoreMaterializeSubphaseSeamKind::CMSS_NONE;
    std::string seamWhy;
    ExplicitBlockGraph inputExplicit;
    CoreShapeSnapshot sourceLiveCore;
    BuilderStageSnapshot miniAfterSnapshot;
    MaterializeCoreSubphaseSnapshot allocateSnapshot;
    MaterializeCoreSubphaseSnapshot installSlotsSnapshot;
    MaterializeCoreSubphaseSnapshot connectArcsSnapshot;
    MaterializeCoreSubphaseSnapshot postMetadataSnapshot;
    MaterializeCoreSubphaseSnapshot finalNormalizeSnapshot;
    std::string sourceWhy;
    std::string builderWhy;
};

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
    std::optional<int> targetTcIndex;
    std::optional<int> targetStep;
    std::optional<int> stepIndex;
    std::optional<int> sequenceLengthSoFar;
    std::optional<GraftPostcheckSubtype> postcheckSubtype;
    std::optional<ReducedSPQRCore> actualAfterNormalize;
    std::optional<ExplicitBlockGraph> explicitAfterNormalize;
    std::optional<bool> normalizeOk;
    std::optional<bool> actualInvariantOk;
    std::optional<bool> oracleBuildOk;
    std::optional<bool> oracleEquivalentOk;
    std::string normalizeWhy;
    std::string actualInvariantWhy;
    std::string oracleWhy;
    std::vector<GraftTrace::ReplayNodeSnapshotPhase> oldNodeSnapshotsByPhase;
    std::vector<GraftTrace::ReplayNodeSnapshotPhase> affectedNodeSnapshotsByPhase;
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
    std::optional<HarnessBundle> bundle;
};

struct RunConfig {
    uint64_t seed = 1;
    int rounds = 1;
    int tcIndex = -1;
    int targetStep = -1;
    bool manualOnly = false;
    std::string mode = "static";
    std::string dumpDir = "dumps";
    std::string manifestPath;
    std::string caseName;
    std::string baselineMode = "legacy";
    std::string oracleHandoff = "delete";
    std::string semanticStop = "raw";
    int sourceStep = -1;
    std::string sourceKind = "step";
    std::string sourceSide = "replay";
    std::string source = "auto";
    bool stopBeforeOgdf = false;
    bool runChild = true;
};

inline std::pair<VertexId, VertexId> canonPole(VertexId a, VertexId b) {
    if (a > b) std::swap(a, b);
    return {a, b};
}

} // namespace harness
