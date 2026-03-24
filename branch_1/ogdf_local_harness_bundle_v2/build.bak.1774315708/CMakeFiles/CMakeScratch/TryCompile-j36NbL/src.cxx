#include <ogdf/basic/Graph.h>
#include <ogdf/decomposition/StaticSPQRTree.h>
#include <ogdf/decomposition/StaticSkeleton.h>

int main(){ ogdf::Graph G; auto v = G.newNode(); for(ogdf::adjEntry a : v->adjEntries){(void)a;} return 0; }
