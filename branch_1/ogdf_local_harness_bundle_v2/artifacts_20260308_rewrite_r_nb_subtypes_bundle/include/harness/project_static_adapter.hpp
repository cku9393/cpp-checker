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

struct RewriteRStats {
    uint64_t rewriteCalls = 0;
    uint64_t compactReadyCount = 0;
    uint64_t compactRejectedFallbackCount = 0;
    uint64_t backendBuildRawDirectCount = 0;
    uint64_t backendBuildRawFallbackCount = 0;
    uint64_t rewriteManualPassCount = 0;
    uint64_t rewriteRandomPassCount = 0;
    std::array<uint64_t, kCompactRejectReasonCount> compactRejectReasonCounts{};
    std::array<std::string, kCompactRejectReasonCount> firstRejectDumpPaths{};
    std::array<uint64_t, kNotBiconnectedSubtypeCount> compactNotBiconnectedSubtypeCounts{};
    std::array<std::string, kNotBiconnectedSubtypeCount> firstNotBiconnectedSubtypeDumpPaths{};
};

const char *compactRejectReasonName(CompactRejectReason reason);
const char *notBiconnectedSubtypeName(NotBiconnectedSubtype subtype);

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

} // namespace harness
