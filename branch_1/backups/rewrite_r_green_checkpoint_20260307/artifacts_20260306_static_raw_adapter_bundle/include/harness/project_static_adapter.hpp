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

bool buildProjectRawSnapshot(const CompactGraph &H,
                             const RawSpqrDecomp &raw,
                             ProjectRawSnapshot &out,
                             std::string &why);

bool checkProjectRawSnapshot(const ProjectRawSnapshot &snap,
                             std::string &why);

} // namespace harness
