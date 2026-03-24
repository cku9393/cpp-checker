#include <ogdf/basic/Graph.h>
#include <ogdf/decomposition/StaticSPQRTree.h>
#include <ogdf/decomposition/StaticSkeleton.h>

int main(){ ogdf::StaticSkeleton *S = nullptr; ogdf::edge e = nullptr; auto te = S->treeEdge(e); (void)te; return 0; }
