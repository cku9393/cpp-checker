#pragma once

#include <array>
#include "harness/types.hpp"

namespace harness {

struct ProjectRawSlot {
    bool alive = true;
    RawSlotKind kind = RawSlotKind::INPUT_EDGE;
    int inputEdgeId = -1;
    int treeEdgeId = -1;
    VertexId poleA = -1;
    VertexId poleB = -1;
};

struct ProjectRawNode {
    bool alive = true;
    SPQRType type = SPQRType::R_NODE;
    std::vector<ProjectRawSlot> slots;
    std::vector<int> cycleSlots;
    std::optional<RawPShape> pShape;
    std::optional<RawRShape> rShape;
};

struct ProjectRawTreeEdge {
    bool alive = true;
    int a = -1;
    int b = -1;
    int slotInA = -1;
    int slotInB = -1;
    VertexId poleA = -1;
    VertexId poleB = -1;
};

struct ProjectRawSnapshot {
    BlockId block = -1;
    NodeId ownerR = -1;
    VertexId deletedX = -1;
    bool valid = false;
    RawDecompError error = RawDecompError::NONE;
    int inputEdgeCount = 0;
    std::vector<ProjectRawNode> nodes;
    std::vector<ProjectRawTreeEdge> treeEdges;
    std::vector<std::pair<int,int>> ownerOfInputEdge;
};

struct ProjectMiniSlot {
    MiniSlotKind kind = MiniSlotKind::REAL_INPUT;
    bool alive = true;
    VertexId poleA = -1;
    VertexId poleB = -1;
    int inputEdgeId = -1;
    EdgeId realEdge = -1;
    int miniArcId = -1;
};

struct ProjectMiniNode {
    bool alive = true;
    SPQRType type = SPQRType::R_NODE;
    std::vector<ProjectMiniSlot> slots;
    std::vector<int> adjArcs;
    Agg localAgg;
    Agg payloadAgg;
};

struct ProjectMiniArc {
    bool alive = true;
    int a = -1;
    int b = -1;
    int slotInA = -1;
    int slotInB = -1;
    VertexId poleA = -1;
    VertexId poleB = -1;
};

struct ProjectMiniCore {
    bool valid = false;
    CoreKind kind = CoreKind::REDUCED_SPQR;
    std::vector<ProjectMiniNode> nodes;
    std::vector<ProjectMiniArc> arcs;
    std::vector<std::pair<int,int>> ownerOfInputEdge;
};

struct ProjectExplicitEdge {
    EdgeId id = -1;
    VertexId u = -1;
    VertexId v = -1;
};

struct ProjectExplicitBlockGraph {
    std::vector<ProjectExplicitEdge> edges;
    std::vector<VertexId> vertices;
};

enum class CompactRejectReason : uint8_t {
    EMPTY_AFTER_DELETE,
    TOO_SMALL_FOR_SPQR,
    NOT_BICONNECTED,
    X_INCIDENT_VIRTUAL_UNSUPPORTED,
    SELF_LOOP,
    OWNER_NOT_R,
    X_NOT_PRESENT_IN_R,
    OTHER,
    COUNT
};

inline constexpr size_t kCompactRejectReasonCount =
    static_cast<size_t>(CompactRejectReason::COUNT);

struct CompactBCBlock {
    std::vector<int> edgeIds;
    std::vector<VertexId> vertices;
    int realEdgeCnt = 0;
    int proxyEdgeCnt = 0;
    int payloadEdgeCnt = 0;
    int payloadVertexCnt = 0;
};

struct CompactBCResult {
    bool valid = false;
    std::vector<CompactBCBlock> blocks;
    std::unordered_map<int,int> blockOfEdge;
    std::unordered_map<VertexId, std::vector<int>> blocksOfVertex;
    std::vector<VertexId> articulationVertices;
    std::vector<std::vector<int>> bcAdj;
};

enum class NotBiconnectedSubtype : uint8_t {
    NB_DISCONNECTED,
    NB_SINGLE_CUT_TWO_BLOCKS,
    NB_PATH_OF_BLOCKS,
    NB_STAR_AROUND_ONE_CUT,
    NB_COMPLEX_MULTI_CUT,
    NB_BLOCKS_ALL_TINY,
    NB_OTHER,
    COUNT
};

inline constexpr size_t kNotBiconnectedSubtypeCount =
    static_cast<size_t>(NotBiconnectedSubtype::COUNT);

enum class TooSmallSubtype : uint8_t {
    TS_EMPTY,
    TS_ONE_EDGE,
    TS_TWO_PARALLEL,
    TS_TWO_PATH,
    TS_TWO_DISCONNECTED,
    TS_TWO_OTHER,
    TS_OTHER,
    COUNT
};

inline constexpr size_t kTooSmallSubtypeCount =
    static_cast<size_t>(TooSmallSubtype::COUNT);

enum class TooSmallOtherSubtype : uint8_t {
    TSO_LOOP_PLUS_EDGE_SHARED,
    TSO_LOOP_PLUS_EDGE_DISJOINT,
    TSO_TWO_LOOPS_SAME_VERTEX,
    TSO_TWO_LOOPS_DIFF_VERTEX,
    TSO_TWO_NONLOOP_UNCLASSIFIED,
    TSO_KIND_MIXED_ANOMALY,
    TSO_OTHER,
    COUNT
};

inline constexpr size_t kTooSmallOtherSubtypeCount =
    static_cast<size_t>(TooSmallOtherSubtype::COUNT);

enum class SeqFallbackReason : uint8_t {
    PROXY_METADATA,
    GRAFT_REWIRE,
    OTHER,
    COUNT
};

inline constexpr size_t kSeqFallbackReasonCount =
    static_cast<size_t>(SeqFallbackReason::COUNT);

enum class RewriteFallbackTrigger : uint8_t {
    RFT_NONE,
    RFT_COMPACT_BUILD_FAIL,
    RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED,
    RFT_COMPACT_EMPTY_AFTER_DELETE,
    RFT_COMPACT_NOT_BICONNECTED_UNHANDLED,
    RFT_COMPACT_TOO_SMALL_UNHANDLED,
    RFT_SINGLE_CUT_BUILDER_FAIL,
    RFT_PATH_OF_BLOCKS_BUILDER_FAIL,
    RFT_TWO_PATH_BUILDER_FAIL,
    RFT_BACKEND_BUILDRAW_S_LT3,
    RFT_BACKEND_BUILDRAW_NOT_BICONNECTED,
    RFT_BACKEND_BUILDRAW_OTHER,
    RFT_RAW_VALIDATE_FAIL,
    RFT_MATERIALIZE_MINI_FAIL,
    RFT_CHOOSE_KEEP_FAIL,
    RFT_PROXY_METADATA_INVALID,
    RFT_GRAFT_REWIRE_FAIL,
    RFT_GRAFT_OTHER,
    RFT_OTHER,
    COUNT
};

inline constexpr size_t kRewriteFallbackTriggerCount =
    static_cast<size_t>(RewriteFallbackTrigger::COUNT);

enum class CompactBuildFailSubtype : uint8_t {
    CBF_NODE_DEAD,
    CBF_NODE_NOT_R,
    CBF_X_NOT_PRESENT_IN_R,
    CBF_REAL_ENDPOINT_NOT_MAPPED,
    CBF_PROXY_OLDARC_INVALID,
    CBF_PROXY_OUTSIDENODE_INVALID,
    CBF_PROXY_OLDSLOT_INVALID,
    CBF_PROXY_OLDSLOT_ARC_MISMATCH,
    CBF_DUPLICATE_COMPACT_EDGE,
    CBF_EMPTY_VERTEX_SET,
    CBF_SELF_LOOP_PRECHECK,
    CBF_OTHER,
    COUNT
};

inline constexpr size_t kCompactBuildFailSubtypeCount =
    static_cast<size_t>(CompactBuildFailSubtype::COUNT);

enum class SelfLoopBuildFailSubtype : uint8_t {
    SL_REAL_LOOP_PRESENT,
    SL_MIXED_REAL_PROXY_LOOP,
    SL_PROXY_ONLY_REMAINDER_EMPTY,
    SL_PROXY_ONLY_REMAINDER_SPQR_READY,
    SL_PROXY_ONLY_REMAINDER_TWO_PATH,
    SL_PROXY_ONLY_REMAINDER_SINGLE_CUT,
    SL_PROXY_ONLY_REMAINDER_PATH_OF_BLOCKS,
    SL_PROXY_ONLY_REMAINDER_OTHER_NOT_BICONNECTED,
    SL_PROXY_ONLY_REMAINDER_OTHER,
    SL_OTHER,
    COUNT
};

inline constexpr size_t kSelfLoopBuildFailSubtypeCount =
    static_cast<size_t>(SelfLoopBuildFailSubtype::COUNT);

enum class XIncidentVirtualSubtype : uint8_t {
    XIV_ZERO_PAYLOAD,
    XIV_ONE_POS_PROXY,
    XIV_MULTI_POS_PROXY,
    XIV_SHARED_WITH_LOOP,
    XIV_MIXED_OTHER,
    COUNT
};

inline constexpr size_t kXIncidentVirtualSubtypeCount =
    static_cast<size_t>(XIncidentVirtualSubtype::COUNT);

enum class XSharedResidualSubtype : uint8_t {
    XSR_HAFTER_BUILD_FAIL,
    XSR_HAFTER_EMPTY,
    XSR_HAFTER_ONE_EDGE,
    XSR_HAFTER_TWO_PARALLEL,
    XSR_HAFTER_TWO_PATH,
    XSR_HAFTER_LOOP_SHARED,
    XSR_HAFTER_SPQR_READY,
    XSR_HAFTER_SINGLE_CUT,
    XSR_HAFTER_PATH_OF_BLOCKS,
    XSR_HAFTER_OTHER_NOT_BICONNECTED,
    XSR_HAFTER_OTHER,
    COUNT
};

inline constexpr size_t kXSharedResidualSubtypeCount =
    static_cast<size_t>(XSharedResidualSubtype::COUNT);

enum class XSharedBridgeBailout : uint8_t {
    XSB_NONE,
    XSB_HAFTER_BUILD_FAIL,
    XSB_UNSUPPORTED_HAFTER_SUBTYPE,
    XSB_LOOP_SHARED_BUILDER_FAIL,
    XSB_TWO_PATH_BUILDER_FAIL,
    XSB_CHOOSE_KEEP_FAIL,
    XSB_GRAFT_FAIL,
    XSB_METADATA_REFRESH_FAIL,
    XSB_OTHER,
    COUNT
};

inline constexpr size_t kXSharedBridgeBailoutCount =
    static_cast<size_t>(XSharedBridgeBailout::COUNT);

enum class ProxyArcNoCandidateSubtype : uint8_t {
    PNC_OLDNODE_DEAD,
    PNC_OUTSIDENODE_DEAD,
    PNC_OLDNODE_NO_LIVE_ARCS,
    PNC_NO_ARC_TO_OUTSIDENODE,
    PNC_TO_OUTSIDENODE_BUT_WRONG_POLES,
    PNC_SAME_POLES_BUT_OTHER_OUTSIDE,
    PNC_CANDIDATE_SLOT_NOT_VIRTUAL,
    PNC_CANDIDATE_SLOT_ARCID_MISMATCH,
    PNC_MULTI_WEAK_CANDIDATES,
    PNC_OTHER,
    COUNT
};

inline constexpr size_t kProxyArcNoCandidateSubtypeCount =
    static_cast<size_t>(ProxyArcNoCandidateSubtype::COUNT);

inline constexpr size_t kGraftRewireBailoutSubtypeCount =
    static_cast<size_t>(GraftRewireBailoutSubtype::COUNT);

enum class RewritePathTaken : uint8_t {
    DIRECT_SPQR,
    SPECIAL_SINGLE_CUT,
    SPECIAL_ONE_EDGE,
    SPECIAL_TWO_PATH,
    SPECIAL_PATH,
    SPECIAL_LOOP_SHARED,
    SPECIAL_SELF_LOOP_TWO_PATH,
    WHOLE_CORE_REBUILD,
    COUNT
};

inline constexpr size_t kRewritePathTakenCount =
    static_cast<size_t>(RewritePathTaken::COUNT);
inline constexpr size_t kRewriteSeqTrackedSteps = 64;
inline constexpr size_t kRewriteSeqLengthHistogramSize = kRewriteSeqTrackedSteps + 1;

struct RewriteRStats {
    uint64_t rewriteCalls = 0;
    uint64_t rewriteSeqCalls = 0;
    uint64_t rewriteSeqSucceededCases = 0;
    uint64_t rewriteSeqFailedCases = 0;
    uint64_t rewriteSeqMaxStepReachedCount = 0;
    uint64_t compactReadyCount = 0;
    uint64_t compactRejectedFallbackCount = 0;
    uint64_t backendBuildRawDirectCount = 0;
    uint64_t backendBuildRawFallbackCount = 0;
    uint64_t compactSingleCutTwoBlocksHandled = 0;
    uint64_t compactPathOfBlocksHandled = 0;
    uint64_t compactTooSmallHandledCount = 0;
    uint64_t compactTooSmallTwoPathHandledCount = 0;
    uint64_t rewriteFallbackWholeCoreCount = 0;
    uint64_t rewriteFallbackSpecialCaseCount = 0;
    uint64_t seqProxyMetadataFallbackCount = 0;
    uint64_t seqGraftRewireFallbackCount = 0;
    uint64_t seqRewriteWholeCoreFallbackCount = 0;
    uint64_t seqFallbackCaseCount = 0;
    uint64_t seqResolvedProxySnapshotCount = 0;
    uint64_t seqResolvedProxySnapshotFailCount = 0;
    uint64_t seqResolvedProxyRepairUsedCount = 0;
    uint64_t seqClearPreserveRequestedCount = 0;
    uint64_t seqClearPreserveArcCount = 0;
    uint64_t seqClearPreserveCrossNodeRewireCount = 0;
    uint64_t seqClearPreserveSameNodeRehomeCount = 0;
    uint64_t seqClearPreserveFallbackCount = 0;
    uint64_t seqAdjRepairUsedCount = 0;
    uint64_t seqAdjRepairAffectedNodeCount = 0;
    uint64_t seqAdjRepairOldNodeCount = 0;
    uint64_t seqAdjRepairOutsideNodeCount = 0;
    uint64_t seqResolvedOldArcRepairAttemptCount = 0;
    uint64_t seqResolvedOldArcRepairSuccessCount = 0;
    uint64_t seqResolvedOldArcRepairFailCount = 0;
    uint64_t seqResolvedOldArcRepairUsedCount = 0;
    uint64_t seqResolvedOldArcWeakRepairAttemptCount = 0;
    uint64_t seqResolvedOldArcWeakRepairSuccessCount = 0;
    uint64_t seqResolvedOldArcWeakRepairFailCount = 0;
    uint64_t seqResolvedOldArcWeakRepairUsedCount = 0;
    uint64_t seqTooSmallOtherHandledCount = 0;
    uint64_t seqLoopPlusEdgeSharedHandledCount = 0;
    uint64_t seqSelfLoopRemainderTwoPathHandledCount = 0;
    uint64_t seqXIncidentSharedWithLoopHandledCount = 0;
    uint64_t seqXIncidentOneEdgeHandledCount = 0;
    uint64_t seqXIncidentOneEdgeRealHandledCount = 0;
    uint64_t seqXIncidentOneEdgeUnsupportedProxyCount = 0;
    uint64_t rewriteManualPassCount = 0;
    uint64_t rewriteRandomPassCount = 0;
    std::array<uint64_t, kCompactRejectReasonCount> compactRejectReasonCounts{};
    std::array<std::string, kCompactRejectReasonCount> firstRejectDumpPaths{};
    std::array<uint64_t, kNotBiconnectedSubtypeCount> compactNotBiconnectedSubtypeCounts{};
    std::array<std::string, kNotBiconnectedSubtypeCount> firstNotBiconnectedSubtypeDumpPaths{};
    std::array<uint64_t, kTooSmallSubtypeCount> compactTooSmallSubtypeCounts{};
    std::array<std::string, kTooSmallSubtypeCount> firstTooSmallSubtypeDumpPaths{};
    std::array<uint64_t, kTooSmallSubtypeCount> seqTooSmallSubtypeCounts{};
    std::array<uint64_t, kTooSmallOtherSubtypeCount> seqTooSmallOtherSubtypeCounts{};
    std::array<uint64_t, kTooSmallOtherSubtypeCount> seqTooSmallCaseCountsBySubtype{};
    std::array<std::string, kTooSmallOtherSubtypeCount> firstTooSmallOtherDumpPaths{};
    std::array<uint64_t, kRewriteSeqTrackedSteps> seqFallbackAtStepCounts{};
    std::array<uint64_t, kSeqFallbackReasonCount> seqFallbackReasonCounts{};
    std::array<std::string, kSeqFallbackReasonCount> firstSeqFallbackDumpPaths{};
    std::array<uint64_t, kRewriteFallbackTriggerCount> rewriteFallbackTriggerCounts{};
    std::array<std::array<uint64_t, kRewriteFallbackTriggerCount>, kRewriteSeqTrackedSteps>
        rewriteFallbackTriggerAtStepCounts{};
    std::array<uint64_t, kRewriteFallbackTriggerCount> rewriteFallbackCaseCountsByTrigger{};
    std::array<std::string, kRewriteFallbackTriggerCount> firstFallbackTriggerDumpPaths{};
    std::array<uint64_t, kCompactBuildFailSubtypeCount> seqCompactBuildFailSubtypeCounts{};
    std::array<uint64_t, kCompactBuildFailSubtypeCount> seqCompactBuildFailCaseCountsBySubtype{};
    std::array<std::array<uint64_t, kCompactBuildFailSubtypeCount>, kRewriteSeqTrackedSteps>
        seqCompactBuildFailAtStepCounts{};
    std::array<std::string, kCompactBuildFailSubtypeCount> firstCompactBuildFailDumpPaths{};
    std::array<uint64_t, kSelfLoopBuildFailSubtypeCount> seqSelfLoopSubtypeCounts{};
    std::array<uint64_t, kSelfLoopBuildFailSubtypeCount> seqSelfLoopCaseCountsBySubtype{};
    std::array<std::array<uint64_t, kSelfLoopBuildFailSubtypeCount>, kRewriteSeqTrackedSteps>
        seqSelfLoopAtStepCounts{};
    std::array<std::string, kSelfLoopBuildFailSubtypeCount> firstSelfLoopDumpPaths{};
    std::array<uint64_t, kXIncidentVirtualSubtypeCount> seqXIncidentVirtualSubtypeCounts{};
    std::array<uint64_t, kXIncidentVirtualSubtypeCount> seqXIncidentVirtualCaseCountsBySubtype{};
    std::array<std::array<uint64_t, kXIncidentVirtualSubtypeCount>, kRewriteSeqTrackedSteps>
        seqXIncidentVirtualAtStepCounts{};
    std::array<std::string, kXIncidentVirtualSubtypeCount> firstXIncidentVirtualDumpPaths{};
    std::array<uint64_t, kXSharedResidualSubtypeCount> seqXIncidentResidualSubtypeCounts{};
    std::array<uint64_t, kXSharedResidualSubtypeCount> seqXIncidentResidualCaseCountsBySubtype{};
    std::array<std::string, kXSharedResidualSubtypeCount> firstXIncidentResidualDumpPaths{};
    std::array<uint64_t, kXSharedBridgeBailoutCount> seqXIncidentBridgeBailoutCounts{};
    std::array<std::array<uint64_t, kXSharedBridgeBailoutCount>, kRewriteSeqTrackedSteps>
        seqXIncidentBridgeBailoutAtStepCounts{};
    std::array<std::string, kXSharedBridgeBailoutCount> firstXIncidentBridgeBailoutDumpPaths{};
    std::array<uint64_t, kGraftRewireBailoutSubtypeCount> seqGraftRewireSubtypeCounts{};
    std::array<uint64_t, kGraftRewireBailoutSubtypeCount> seqGraftRewireCaseCountsBySubtype{};
    std::array<std::array<uint64_t, kGraftRewireBailoutSubtypeCount>, kRewriteSeqTrackedSteps>
        seqGraftRewireAtStepCounts{};
    std::array<std::string, kGraftRewireBailoutSubtypeCount> firstGraftRewireDumpPaths{};
    std::array<uint64_t, kGraftOtherSubtypeCount> seqGraftOtherSubtypeCounts{};
    std::array<uint64_t, kGraftOtherSubtypeCount> seqGraftOtherCaseCountsBySubtype{};
    std::array<std::array<uint64_t, kGraftOtherSubtypeCount>, kRewriteSeqTrackedSteps>
        seqGraftOtherAtStepCounts{};
    std::array<std::string, kGraftOtherSubtypeCount> firstGraftOtherDumpPaths{};
    std::array<uint64_t, kProxyArcRepairOutcomeCount> seqResolvedOldArcRepairOutcomeCounts{};
    std::array<uint64_t, kProxyArcRepairOutcomeCount> seqResolvedOldArcWeakRepairOutcomeCounts{};
    std::array<uint64_t, kProxyArcLifecyclePhaseCount> seqProxyArcLifecycleCounts{};
    std::array<uint64_t, kProxyArcLifecyclePhaseCount> seqProxyArcFirstBadPhaseCounts{};
    std::array<uint64_t, kProxyArcLifecyclePhaseCount> seqProxyArcCaseCountsByFirstBadPhase{};
    std::array<uint64_t, kWeakRepairGateSubtypeCount> seqWeakRepairGateCounts{};
    std::array<uint64_t, kWeakRepairCandidateSubtypeCount> seqWeakRepairCandidateCounts{};
    std::array<uint64_t, kWeakRepairCommitOutcomeCount> seqWeakRepairCommitCounts{};
    uint64_t seqWeakRepairEnteredCount = 0;
    uint64_t seqWeakRepairTentativeSuccessCount = 0;
    uint64_t seqWeakRepairCommittedCount = 0;
    uint64_t seqWeakRepairRollbackCount = 0;
    std::string firstOldArcWeakRepairSuccessDumpPath;
    std::array<std::string, kProxyArcRepairOutcomeCount> firstOldArcWeakRepairFailDumpPaths{};
    std::array<std::string, kWeakRepairGateSubtypeCount> firstWeakRepairGateDumpPaths{};
    std::array<std::string, kWeakRepairCandidateSubtypeCount> firstWeakRepairCandidateDumpPaths{};
    std::array<std::string, kWeakRepairCommitOutcomeCount> firstWeakRepairCommitDumpPaths{};
    std::array<std::string, kProxyArcLifecyclePhaseCount> firstProxyArcPhaseDumpPaths{};
    std::array<uint64_t, kProxyArcNoCandidateSubtypeCount>
        seqProxyRepairNoCandidateSubtypeCounts{};
    std::array<uint64_t, kProxyArcNoCandidateSubtypeCount>
        seqProxyRepairNoCandidateCaseCountsBySubtype{};
    std::array<std::array<uint64_t, kProxyArcNoCandidateSubtypeCount>, kRewriteSeqTrackedSteps>
        seqProxyRepairNoCandidateAtStepCounts{};
    std::array<std::string, kProxyArcNoCandidateSubtypeCount>
        firstProxyRepairNoCandidateDumpPaths{};
    std::array<uint64_t, kRewritePathTakenCount> rewritePathTakenCounts{};
    std::array<uint64_t, kRewriteSeqLengthHistogramSize> sequenceLengthHistogram{};
};

const char *compactRejectReasonName(CompactRejectReason reason);
const char *notBiconnectedSubtypeName(NotBiconnectedSubtype subtype);
const char *tooSmallSubtypeName(TooSmallSubtype subtype);
const char *tooSmallOtherSubtypeName(TooSmallOtherSubtype subtype);
const char *seqFallbackReasonName(SeqFallbackReason reason);
const char *rewriteFallbackTriggerName(RewriteFallbackTrigger trigger);
const char *compactBuildFailSubtypeName(CompactBuildFailSubtype subtype);
const char *selfLoopBuildFailSubtypeName(SelfLoopBuildFailSubtype subtype);
const char *xIncidentVirtualSubtypeName(XIncidentVirtualSubtype subtype);
const char *xSharedResidualSubtypeName(XSharedResidualSubtype subtype);
const char *xSharedBridgeBailoutName(XSharedBridgeBailout bailout);
const char *graftRewireBailoutSubtypeName(GraftRewireBailoutSubtype subtype);
const char *graftOtherSubtypeName(GraftOtherSubtype subtype);
const char *proxyArcRepairOutcomeName(ProxyArcRepairOutcome outcome);
const char *proxyArcNoCandidateSubtypeName(ProxyArcNoCandidateSubtype subtype);
const char *proxyArcLifecyclePhaseName(ProxyArcLifecyclePhase phase);
const char *weakRepairGateSubtypeName(WeakRepairGateSubtype subtype);
const char *weakRepairCandidateSubtypeName(WeakRepairCandidateSubtype subtype);
const char *weakRepairCommitOutcomeName(WeakRepairCommitOutcome outcome);
const char *rewritePathTakenName(RewritePathTaken path);

bool buildProjectRawSnapshot(const CompactGraph &H,
                             const RawSpqrDecomp &raw,
                             ProjectRawSnapshot &out,
                             std::string &why);

bool checkProjectRawSnapshot(const ProjectRawSnapshot &snap,
                             std::string &why);

bool buildProjectMiniCore(const CompactGraph &H,
                          const ProjectRawSnapshot &snap,
                          ProjectMiniCore &out,
                          std::string &why);

bool exportProjectMiniCore(const ProjectMiniCore &in,
                           StaticMiniCore &out,
                           std::string &why);

bool importStaticMiniCore(const StaticMiniCore &in,
                          ProjectMiniCore &out,
                          std::string &why);

bool chooseProjectKeepMiniNode(const ProjectMiniCore &mini,
                               int &keep,
                               std::string &why);

bool checkProjectMiniOwnershipConsistency(const CompactGraph &H,
                                          const ProjectMiniCore &mini,
                                          std::string &why);

bool checkProjectMiniReducedInvariant(const CompactGraph &H,
                                      const ProjectMiniCore &mini,
                                      std::string &why);

void materializeProjectWholeCoreExplicit(const ReducedSPQRCore &core,
                                         ProjectExplicitBlockGraph &out);

void materializeProjectCompactRealProjection(const CompactGraph &H,
                                             ProjectExplicitBlockGraph &out);

void exportProjectExplicitBlockGraph(const ProjectExplicitBlockGraph &in,
                                     ExplicitBlockGraph &out);

void importExplicitBlockGraph(const ExplicitBlockGraph &in,
                              ProjectExplicitBlockGraph &out);

bool checkProjectEquivalentExplicitGraphs(const ProjectExplicitBlockGraph &got,
                                          const ProjectExplicitBlockGraph &exp,
                                          std::string &why);

bool checkProjectDummyProxyRewire(const DummyActualEnvelope &env,
                                  const StaticMiniCore &mini,
                                  const GraftTrace &trace,
                                  std::string &why);

bool buildProjectCompactLocalViewFromR(const ReducedSPQRCore &core,
                                       NodeId rNode,
                                       VertexId x,
                                       CompactGraph &out,
                                       CompactRejectReason *reason,
                                       CompactBuildFailSubtype *subtype,
                                       std::string &why);

bool validateProxyMetadataForRewrite(const ReducedSPQRCore &core,
                                     NodeId oldNode,
                                     const CompactGraph &H,
                                     std::string &why);

bool resolveLiveProxyEndpointsForGraft(const ReducedSPQRCore &core,
                                       NodeId oldNode,
                                       const CompactGraph &H,
                                       std::vector<ResolvedProxyEndpoint> &out,
                                       GraftRewireBailoutSubtype *subtype,
                                       std::string &why);

bool collectPreservedProxyArcsBeforeClear(const ReducedSPQRCore &core,
                                          NodeId oldNode,
                                          const std::vector<ResolvedProxyEndpoint> &resolved,
                                          std::vector<PreservedProxyArc> &out,
                                          std::string &why);

bool clearNodeKeepIdForGraftPreserveResolvedProxyArcs(
    ReducedSPQRCore &core,
    NodeId oldNode,
    const std::vector<PreservedProxyArc> &preserved,
    SPQRType newType,
    std::string &why);

bool rehomePreservedProxyArcOnSameNode(ReducedSPQRCore &core,
                                       const PreservedProxyArc &p,
                                       int newSlot,
                                       std::string &why);

std::vector<ArcId> collectAuthoritativeLiveAdjArcs(const ReducedSPQRCore &core,
                                                   NodeId u);

void rebuildAdjacencyForNodeFromArcs(ReducedSPQRCore &core,
                                     NodeId u);

void rebuildAdjacencyForAffectedNodesAfterGraft(ReducedSPQRCore &core,
                                                const std::vector<NodeId> &nodes);

std::vector<NodeId> collectAffectedNodesForAdjRepair(
    const GraftTrace &trace,
    NodeId oldNode,
    const std::vector<PreservedProxyArc> &preserved);

bool resolveLiveProxyArcForSequenceRepair(const ReducedSPQRCore &core,
                                          NodeId oldNode,
                                          const CompactGraph &H,
                                          const CompactEdge &proxyEdge,
                                          RepairedProxyArcInfo &out,
                                          std::string &why);

bool inspectResolvedProxyArcPhase(const ReducedSPQRCore &core,
                                  const RepairedProxyArcInfo &info,
                                  ProxyArcLifecyclePhase alivePhase,
                                  ProxyArcLifecyclePhase deadPhase,
                                  ProxyArcLifecyclePhase notIncidentPhase,
                                  ProxyArcLifecyclePhase slotInvalidPhase,
                                  ProxyArcLifecyclePhase &outPhase,
                                  std::string &why);

ProxyArcNoCandidateSubtype classifyProxyRepairNoCandidate(const ReducedSPQRCore &core,
                                                          NodeId oldNode,
                                                          const CompactGraph &H,
                                                          const CompactEdge &proxyEdge,
                                                          std::string &why);

bool stripSelfLoopsForAnalysis(const CompactGraph &H,
                               CompactGraph &remainder,
                               std::vector<int> &removedLoopEdgeIds,
                               std::string &why);

SelfLoopBuildFailSubtype classifySelfLoopBuildFailDetailed(const CompactGraph &H,
                                                           std::string &why);

bool isCompactGraphSpqrReady(const CompactGraph &H,
                             CompactRejectReason *reason,
                             std::string &why);

bool decomposeCompactIntoBC(const CompactGraph &H,
                            CompactBCResult &out,
                            std::string &why);

bool validateCompactBC(const CompactGraph &H,
                       const CompactBCResult &bc,
                       std::string &why);

NotBiconnectedSubtype classifyNotBiconnected(const CompactGraph &H,
                                             const CompactBCResult &bc);

TooSmallSubtype classifyTooSmallCompact(const CompactGraph &H);
TooSmallOtherSubtype classifyTooSmallOtherDetailed(const CompactGraph &H);
XSharedResidualSubtype classifyXSharedResidualAfterDelete(const ReducedSPQRCore &core,
                                                          NodeId rNode,
                                                          VertexId x,
                                                          CompactGraph &Hafter,
                                                          std::string &why);

bool buildCompactAfterDeletingXForSharedLoop(const ReducedSPQRCore &core,
                                             NodeId rNode,
                                             VertexId x,
                                             CompactGraph &Hafter,
                                             std::string &why);

bool buildSyntheticMiniForOneEdgeRemainder(const CompactGraph &H,
                                           StaticMiniCore &mini,
                                           std::string &why);

bool buildSyntheticMiniForLoopPlusEdgeShared(const CompactGraph &H,
                                             StaticMiniCore &mini,
                                             std::string &why);

bool buildSyntheticMiniForSelfLoopRemainderTwoPath(const CompactGraph &H,
                                                   StaticMiniCore &mini,
                                                   std::string &why);

bool buildSyntheticMiniForTooSmallTwoPath(const CompactGraph &H,
                                          StaticMiniCore &mini,
                                          std::string &why);

bool buildSyntheticMiniForPathOfBlocks(const CompactGraph &H,
                                       const CompactBCResult &bc,
                                       StaticMiniCore &mini,
                                       std::string &why);

bool buildSyntheticMiniForSingleCutTwoBlocks(const CompactGraph &H,
                                             const CompactBCResult &bc,
                                             StaticMiniCore &mini,
                                             std::string &why);

bool buildExplicitAfterDeletingVertex(const ReducedSPQRCore &core,
                                      VertexId x,
                                      ExplicitBlockGraph &out,
                                      std::string &why);

bool rebuildWholeCoreFromExplicit(const ExplicitBlockGraph &G,
                                  ReducedSPQRCore &out,
                                  std::string &why);

bool rebuildWholeCoreAfterDeletingX(ReducedSPQRCore &core,
                                    VertexId x,
                                    std::string &why);

bool rewriteProjectRFallback(ReducedSPQRCore &core,
                             NodeId rNode,
                             VertexId x,
                             std::string &why);

bool normalizeProjectTouchedRegion(ReducedSPQRCore &core,
                                   std::string &why);

void rebuildAllOccurrencesActual(ReducedSPQRCore &core);

void resetRewriteRStats();
RewriteRStats getRewriteRStats();
void recordRewriteRPass(bool manualCase);
void setRewriteRCaseContext(uint64_t seed, int tc);
void setRewriteRSequenceMode(bool enabled);
void setRewriteRSequenceStepContext(int stepIndex, int sequenceLengthSoFar);
void noteRewriteSeqCaseStart();
void noteRewriteSeqCaseFinish(bool success,
                              int stepsTaken,
                              bool hadFallback,
                              bool maxStepReached);
void noteSequenceProxyArcLifecyclePhase(const ReducedSPQRCore &core,
                                        NodeId oldNode,
                                        const CompactGraph *compact,
                                        RepairedProxyArcInfo &info,
                                        ProxyArcLifecyclePhase phase,
                                        const std::string &why,
                                        const GraftTrace *trace);

void noteSequenceClearPreserveRequested(size_t arcCount);
void noteSequenceClearPreserveCrossNodeRewire();
void noteSequenceClearPreserveSameNodeRehome();
void noteSequenceClearPreserveFallback();
void noteRewriteRWeakRepairCommitOutcome(WeakRepairCommitOutcome outcome,
                                         const ReducedSPQRCore *afterCore,
                                         std::string why);

} // namespace harness
