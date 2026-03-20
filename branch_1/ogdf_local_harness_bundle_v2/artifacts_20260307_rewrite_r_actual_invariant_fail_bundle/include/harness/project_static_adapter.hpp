#pragma once

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
                                       std::string &why);

bool rewriteProjectRFallback(ReducedSPQRCore &core,
                             NodeId rNode,
                             VertexId x,
                             std::string &why);

bool normalizeProjectTouchedRegion(ReducedSPQRCore &core,
                                   std::string &why);

} // namespace harness
