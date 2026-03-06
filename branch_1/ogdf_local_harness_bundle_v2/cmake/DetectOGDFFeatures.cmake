include(CheckCXXSourceCompiles)

set(OGDF_INCLUDE_DIRS "")
set(OGDF_LIBRARY_DIRS "")

if(USE_OGDF)
  if(OGDF_ROOT)
    # install prefix layout
    if(EXISTS "${OGDF_ROOT}/include")
      list(APPEND OGDF_INCLUDE_DIRS "${OGDF_ROOT}/include")
    endif()
    # OGDF installs generated headers (e.g. config_autogen.h) under include/ogdf-release
    if(EXISTS "${OGDF_ROOT}/include/ogdf-release")
      list(APPEND OGDF_INCLUDE_DIRS "${OGDF_ROOT}/include/ogdf-release")
    endif()
    if(EXISTS "${OGDF_ROOT}/lib")
      list(APPEND OGDF_LIBRARY_DIRS "${OGDF_ROOT}/lib")
    endif()
    if(EXISTS "${OGDF_ROOT}/lib64")
      list(APPEND OGDF_LIBRARY_DIRS "${OGDF_ROOT}/lib64")
    endif()

    # build-tree layout from official build guide
    if(EXISTS "${OGDF_ROOT}/include")
      list(APPEND OGDF_INCLUDE_DIRS "${OGDF_ROOT}/include")
    endif()
    if(EXISTS "${OGDF_ROOT}/include/ogdf-release")
      list(APPEND OGDF_INCLUDE_DIRS "${OGDF_ROOT}/include/ogdf-release")
    endif()
    if(EXISTS "${OGDF_ROOT}/../ogdf-build/include/ogdf-release")
      list(APPEND OGDF_INCLUDE_DIRS "${OGDF_ROOT}/../ogdf-build/include/ogdf-release")
    endif()
    if(EXISTS "${OGDF_ROOT}/../OGDF/include")
      list(APPEND OGDF_INCLUDE_DIRS "${OGDF_ROOT}/../OGDF/include")
    endif()
    if(EXISTS "${OGDF_ROOT}")
      list(APPEND OGDF_LIBRARY_DIRS "${OGDF_ROOT}")
    endif()
  endif()
  list(REMOVE_DUPLICATES OGDF_INCLUDE_DIRS)
  list(REMOVE_DUPLICATES OGDF_LIBRARY_DIRS)
endif()

set(_ogdf_required_headers [[
#include <ogdf/basic/Graph.h>
#include <ogdf/decomposition/StaticSPQRTree.h>
#include <ogdf/decomposition/StaticSkeleton.h>
]])

set(CMAKE_REQUIRED_INCLUDES ${OGDF_INCLUDE_DIRS})

check_cxx_source_compiles("${_ogdf_required_headers}
int main(){ ogdf::Graph G; return 0; }" OGDF_HEADERS_OK)

check_cxx_source_compiles("${_ogdf_required_headers}
int main(){ ogdf::Graph G; for(ogdf::node v : G.nodes) {(void)v;} return 0; }" OGDF_HAS_RANGE_GRAPH_ITER)

check_cxx_source_compiles("${_ogdf_required_headers}
int main(){ ogdf::Graph G; auto v = G.newNode(); for(ogdf::adjEntry a : v->adjEntries){(void)a;} return 0; }" OGDF_HAS_NODE_ADJENTRIES)

check_cxx_source_compiles("${_ogdf_required_headers}
int main(){ ogdf::Graph G; auto e = G.newEdge(G.newNode(), G.newNode()); ogdf::StaticSPQRTree *T = nullptr; (void)T; return 0; }" OGDF_HAS_STATICSPQRTREE)

check_cxx_source_compiles("${_ogdf_required_headers}
int main(){ ogdf::Graph G; ogdf::StaticSkeleton *S = nullptr; (void)S; return 0; }" OGDF_HAS_STATICSKELETON)

check_cxx_source_compiles("${_ogdf_required_headers}
int main(){ ogdf::StaticSkeleton *S = nullptr; ogdf::edge e = nullptr; auto te = S->treeEdge(e); (void)te; return 0; }" OGDF_HAS_STATICSKELETON_TREEEDGE)

unset(CMAKE_REQUIRED_INCLUDES)
