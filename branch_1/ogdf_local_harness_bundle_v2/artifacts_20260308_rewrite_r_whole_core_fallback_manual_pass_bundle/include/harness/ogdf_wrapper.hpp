#pragma once
#include "harness/assert.hpp"
#include "harness/types.hpp"
#include "ogdf_feature_config.hpp"

#if USE_OGDF
#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/Graph.h>
#if __has_include(<ogdf/basic/NodeArray.h>)
#include <ogdf/basic/NodeArray.h>
#endif
#include <ogdf/decomposition/StaticSPQRTree.h>
#include <ogdf/decomposition/StaticSkeleton.h>
#endif

namespace harness {

class IRawSpqrBackend {
public:
    virtual ~IRawSpqrBackend() = default;
    virtual const char *name() const = 0;
    virtual bool buildRaw(const CompactGraph &H,
                          RawSpqrDecomp &raw,
                          std::string &err) = 0;
};

class OgdfRawSpqrBackend final : public IRawSpqrBackend {
public:
    const char *name() const override { return "OGDF"; }
    bool buildRaw(const CompactGraph &H,
                  RawSpqrDecomp &raw,
                  std::string &err) override;
};

#if USE_OGDF
namespace ogdf_wrap {

struct BuildContext {
    ogdf::Graph G;
    std::vector<ogdf::node> cvNode;
    std::vector<ogdf::edge> inputEdge;
    std::unique_ptr<ogdf::EdgeArray<int>> inputIdOfEdge;
    std::unique_ptr<ogdf::NodeArray<VertexId>> origVertexOfGraphNode;
};

void buildOgdfGraph(const CompactGraph &H, BuildContext &B);

template <class F>
void forEachTreeNode(const ogdf::StaticSPQRTree &T, F fn) {
    const ogdf::Graph &TT = T.tree();
#if OGDF_HAS_RANGE_GRAPH_ITER
    for (ogdf::node v : TT.nodes) fn(v);
#else
    for (ogdf::node v = TT.firstNode(); v; v = v->succ()) fn(v);
#endif
}

template <class F>
void forEachTreeEdge(const ogdf::StaticSPQRTree &T, F fn) {
    const ogdf::Graph &TT = T.tree();
#if OGDF_HAS_RANGE_GRAPH_ITER
    for (ogdf::edge e : TT.edges) fn(e);
#else
    for (ogdf::edge e = TT.firstEdge(); e; e = e->succ()) fn(e);
#endif
}

template <class F>
void forEachSkeletonEdge(const ogdf::Skeleton &S, F fn) {
    const ogdf::Graph &GS = S.getGraph();
#if OGDF_HAS_RANGE_GRAPH_ITER
    for (ogdf::edge e : GS.edges) fn(e);
#else
    for (ogdf::edge e = GS.firstEdge(); e; e = e->succ()) fn(e);
#endif
}

SPQRType mapType(ogdf::SPQRTree::NodeType t);
VertexId originalOfSkeletonNode(const ogdf::Skeleton &S,
                                ogdf::node vSk,
                                const ogdf::NodeArray<VertexId> &origVertexOfGraphNode);
std::pair<VertexId,VertexId>
polesOfSkeletonEdge(const ogdf::Skeleton &S,
                    ogdf::edge eSk,
                    const ogdf::NodeArray<VertexId> &origVertexOfGraphNode);
bool computeCycleOrderFromSkeleton(
    const ogdf::Skeleton &S,
    const ogdf::EdgeArray<int> &slotOfSkel,
    std::vector<int> &out,
    std::string &why);
bool buildRShapeFromSkeleton(
    const ogdf::Skeleton &S,
    const ogdf::NodeArray<VertexId> &origVertexOfGraphNode,
    const ogdf::EdgeArray<int> &slotOfSkel,
    int numSlots,
    RawRShape &out,
    std::string &why);

struct SkelEdgeKey {
    const void *skelPtr = nullptr;
    const void *edgePtr = nullptr;
    bool operator==(const SkelEdgeKey &o) const {
        return skelPtr == o.skelPtr && edgePtr == o.edgePtr;
    }
};

struct SkelEdgeKeyHash {
    size_t operator()(const SkelEdgeKey &k) const {
        return std::hash<const void *>()(k.skelPtr) * 1315423911u
             ^ std::hash<const void *>()(k.edgePtr);
    }
};

inline SkelEdgeKey keyOf(const ogdf::Skeleton &S, ogdf::edge eSk) {
    return { (const void*)&S, (const void*)eSk };
}

} // namespace ogdf_wrap
#endif

bool buildRawSpqrDecompWithOgdf(const CompactGraph &H,
                                RawSpqrDecomp &raw,
                                std::string &why);

} // namespace harness
