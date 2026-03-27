# boj28350_literature_progress7_bcdecomp verification report

## Files
- Solver: `boj28350_literature_progress7_bcdecomp_verified.cpp`

## Build / run checks
- Release build: OK
- LOCAL build: OK
- Example output: `0 1 2 3`

## LOCAL counters
- owner_rebuild_calls = 0
- local_updates = 1
- local_fallback = 0
- partition_mismatch = 0
- fallback_breakdown = all 0
- topology_zone_bfs_vertices = 20
- topology_zone_bfs_edges = 38
- strict_child_found = 149
- strict_child_exists_but_missed = 0
- strict_child_structural_miss = 0
- semantic_escape_count = 0
- strict_child_rebuild_used = 149
- strict_child_global_fallback_used = 0
- strict_child_depth_sum = 149
- strict_child_rebuild_vertices = 700
- strict_child_rebuild_edges = 1694

## Summary
This version uses an explicit BC-tree-flavored decomposition lattice (`nodeId`, `children(nodeId)`, `boundaryVerts`, `budgetExp`) and `buildClosedHandleFromWitness(...)` to lift exact witnesses into closed decomposition subtrees. `relocateToStrictChild(...)` enumerates only explicit decomposition children, checks them with an exact restricted predicate, rebuilds only inside the chosen child, and always decreases `budgetExp` by 1. Topology updates are refinement-only local updates with no release-path exact rebuild calls.
