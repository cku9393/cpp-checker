include(CheckCXXSourceCompiles)

set(CMAKE_REQUIRED_INCLUDES ${OGDF_INCLUDE_DIR})
set(CMAKE_REQUIRED_LIBRARIES ${OGDF_LIBRARY})

check_cxx_source_compiles([[ 
#include <ogdf/basic/Graph.h>
int main() {
    ogdf::Graph G;
    for (ogdf::node v : G.nodes) { (void)v; }
    for (ogdf::edge e : G.edges) { (void)e; }
    return 0;
}
]] OGDF_HAS_RANGE_GRAPH_ITER)

check_cxx_source_compiles([[ 
#include <ogdf/basic/Graph.h>
int main() {
    ogdf::Graph G;
    ogdf::node v = G.newNode();
    for (ogdf::adjEntry adj : v->adjEntries) { (void)adj; }
    return 0;
}
]] OGDF_HAS_NODE_ADJENTRIES)

check_cxx_source_compiles([[ 
#include <ogdf/basic/Graph.h>
#include <ogdf/decomposition/StaticSkeleton.h>
int main() {
    // compile-only probe
    return 0;
}
]] OGDF_HAS_STATICSKELETON_HEADER)

check_cxx_source_compiles([[ 
#include <ogdf/basic/Graph.h>
#include <ogdf/decomposition/StaticSPQRTree.h>
#include <ogdf/decomposition/StaticSkeleton.h>
int main() {
    ogdf::Graph G;
    auto a = G.newNode();
    auto b = G.newNode();
    auto e1 = G.newEdge(a,b);
    auto e2 = G.newEdge(a,b);
    auto e3 = G.newEdge(a,b);
    ogdf::StaticSPQRTree T(G, e1);
    ogdf::node tv = T.tree().firstNode();
    auto &S = static_cast<ogdf::StaticSkeleton&>(T.skeleton(tv));
    (void)S;
    return 0;
}
]] OGDF_CAN_BUILD_BASIC_STATICSPQR)

message(STATUS "OGDF_HAS_RANGE_GRAPH_ITER=${OGDF_HAS_RANGE_GRAPH_ITER}")
message(STATUS "OGDF_HAS_NODE_ADJENTRIES=${OGDF_HAS_NODE_ADJENTRIES}")
message(STATUS "OGDF_HAS_STATICSKELETON_HEADER=${OGDF_HAS_STATICSKELETON_HEADER}")
message(STATUS "OGDF_CAN_BUILD_BASIC_STATICSPQR=${OGDF_CAN_BUILD_BASIC_STATICSPQR}")
