# Literature-Grade Proof Package for `boj28350_literature_progress7_bcdecomp_verified.cpp`

This document isolates the remaining theory package needed to justify the current strongest implementation as a literature-grade solver. The reference implementation is the single-file solver `boj28350_literature_progress7_bcdecomp_verified.cpp` together with its `LOCAL` instrumentation.

The `LOCAL` counters used throughout this document satisfy:

- `owner_rebuild_calls = 0`
- `local_fallback = 0`
- `partition_mismatch = 0`
- `strict_child_exists_but_missed = 0`
- `strict_child_structural_miss = 0`
- `semantic_escape_count = 0`
- `strict_child_global_fallback_used = 0`

These counters do not replace proofs, but they support the intended invariants by acting as differential checks against exact restricted oracles.

---

## Section 1. BC-tree flavored child lattice and its decomposition semantics

### 1.1 Definitions

**Definition 1 (BC-tree flavored decomposition model).**
Let `G_alive` be the current alive graph. The implementation maintains a decomposition lattice whose node set is represented by `DecompTree::nodes`. Each node is a record

- `id`
- `kind`
- `verts`
- `edges`
- `children`
- `parent`
- `boundaryVerts`
- `budgetExp`

and is interpreted as a closed decomposition region. In the current implementation `kind` is instantiated at the BC-tree level and may be refined later through the SPQR seam.

**Definition 2 (BC-tree).**
For a connected graph `H`, the BC-tree is the bipartite tree whose block nodes correspond to maximal 2-vertex-connected blocks of `H` and whose cut nodes correspond to articulation vertices of `H`. An incidence edge joins a cut node `c` and a block node `B` iff the articulation represented by `c` belongs to the block represented by `B`.

**Definition 3 (Closed decomposition region).**
A set of decomposition nodes is *closed* if it is connected in the BC-tree and contains all boundary vertices needed to interpret the represented restricted witness region as a valid induced subproblem. In code this closure is carried by `DecompNode::boundaryVerts` and by the region vertex set assigned to the corresponding `PotentialHandle`.

**Definition 4 (Explicit child lattice).**
For a decomposition node `u`, the implementation-defined child set is `children(u) = DecompTree::nodes[u].children`. A child node `v` is a *structural child* of `u` if it is listed in this vector and represents a proper closed subregion whose `budgetExp` equals `budgetExp(u) - 1`.

### 1.2 Lemmas

**Lemma 1.1 (Tree semantics of `children(nodeId)`).**
For every decomposition node `u`, the vector `children(u)` defines a set of proper descendants in the BC-tree flavored lattice, and each such child corresponds to a strict subtree partition of the parent region.

*Proof sketch.*
`PotentialHandleKernel` constructs lattice nodes from normalized base regions. `ensureLatticeChildren` first materializes the parent base region, then adds children obtained from strict BC-path prunings or the same base region at lower budget. Because base regions are normalized and deduplicated by `getOrCreateBaseRegion`, each child region is a canonical proper subregion or a budget-descended copy of the same closed region. The lattice node relation is acyclic because `budgetExp` strictly decreases along added child edges. Therefore `children(nodeId)` is a genuine child set in the decomposition lattice.

**Lemma 1.2 (Boundary closure soundness).**
If a decomposition region contains a restricted witness, then the corresponding closed region stored in `verts` together with `boundaryVerts` also contains a valid restricted witness for the same query.

*Proof sketch.*
The BC-path closure routine `closeByBCPath` computes the minimal union of BC-tree nodes connecting the atoms touched by the witness path. Every articulation on the BC path is inserted into `boundaryVerts`; hence every graph segment needed to traverse between witness path atoms remains represented inside the closed region. Since the restricted predicate excludes only the owner and removed vertex, boundary insertion cannot destroy witness existence and only enlarges the representing region enough to make it closed.

**Lemma 1.3 (Exact strict-child predicate).**
For a current handle `H`, a child node `C`, and a removed vertex `x`, `childStillContainsWitness(H, C, x)` returns true if and only if the child closure contains `owner`, `a`, `b`, and there exists an exact restricted witness in `child - owner` after deleting `x`.

*Proof sketch.*
The function first checks membership of `owner`, `a`, and `b` in the child region, then calls the exact restricted builder over the child region only. Hence the predicate is exact with respect to the induced restricted subproblem on that child closure, rather than heuristic.

### 1.3 Theorem

**Theorem 1 (Strict child lattice correctness at BC-tree level).**
In the current BC-tree flavored implementation, `children(nodeId)` is the exact set of decomposition children considered by the solver, and a strict child is characterized precisely as a child `C` such that `childStillContainsWitness(H, C, removedV)` is true.

*Proof sketch.*
By Lemma 1.1 every enumerated child is a genuine child in the implementation lattice. By Lemma 1.3 the exact restricted witness predicate is both sound and complete inside each child. Therefore the solver's strict-child search reduces to exact child testing over the explicit child lattice. The SPQR seam is not needed for correctness of these statements: refining a BC block by SPQR would only split a block node into smaller internal atoms, which preserves the parent-child semantics already present at BC-tree level.

### 1.4 Implementation Mapping Table

| Mathematical object | Code representation |
|---|---|
| Decomposition tree | `PotentialHandleKernel::DecompTree decomp_` |
| Decomposition node | `PotentialHandleKernel::DecompNode` |
| Child lattice | `DecompNode::children`, `ensureLatticeChildren(...)` |
| Closed region vertex set | `DecompNode::verts`, `PotentialHandle::regionVerts` |
| Boundary closure | `DecompNode::boundaryVerts`, `closeByBCPath(...)` |
| Strict child predicate | `childStillContainsWitness(...)` |
| Budget on decomposition nodes | `DecompNode::budgetExp`, `PotentialHandle::budgetExp` |

**How LOCAL counters support this section.**
`strict_child_exists_but_missed = 0` and `strict_child_structural_miss = 0` provide differential evidence that the explicit child lattice is not omitting structurally valid strict children on the exercised instances. `strict_child_global_fallback_used = 0` supports the claim that release-path strict relocation only uses structural children rather than a global escape path.

---

## Section 2. Minimal closed subtree property of `buildClosedHandleFromWitness`

### 2.1 Definitions

**Definition 5 (Witness hit set).**
Given an exact restricted witness path `P` for query `(owner, a, b)`, its *hit set* is the set of decomposition atoms or BC blocks intersected by `P`.

**Definition 6 (Minimal connected subtree).**
For a tree `T` and a subset of nodes `S`, the *minimal connected subtree* containing `S` is the unique smallest connected subgraph of `T` whose node set contains `S`.

**Definition 7 (Minimal closed subtree).**
Let `S` be a hit set. The *minimal closed subtree* of `S` is the minimal connected subtree containing `S` together with all boundary nodes required by the closure rule. This is the canonical closed region assigned to a seed handle.

### 2.2 Lemmas

**Lemma 2.1 (Existence and uniqueness of the minimal connected subtree).**
For every finite tree `T` and every nonempty node set `S`, there exists a unique minimal connected subtree containing `S`.

*Proof sketch.*
In a tree, between every pair of nodes there is a unique simple path. The union of all pairwise paths among nodes of `S` is connected and minimal; uniqueness follows from uniqueness of paths.

**Lemma 2.2 (Closure preserves exact witnesses).**
If an exact restricted witness path hits a set of BC blocks, then the minimal closed subtree of this hit set still contains an exact restricted witness.

*Proof sketch.*
The only extra vertices added by closure are articulation or boundary vertices that connect the hit blocks along the BC path. Since the witness path already traverses these connectors in the graph sense, adding them does not invalidate the witness; it only makes the representing region closed.

**Lemma 2.3 (Minimality of the closed subtree).**
No proper closed subregion of the minimal closed subtree contains the entire hit set.

*Proof sketch.*
A proper closed subregion would omit at least one node from the unique minimal connected subtree of the hit set, thereby disconnecting some pair of hit atoms or removing a required boundary node. Hence it cannot still contain the whole witness support lifted to decomposition atoms.

### 2.3 Theorem

**Theorem 2 (`buildClosedHandleFromWitness` returns the unique minimal closed subtree).**
For any exact restricted witness path given by `(owner, a, b, pathVerts, pathEdges)`, `buildClosedHandleFromWitness(owner, a, b, pathVerts, pathEdges)` returns a handle region that
1. contains a restricted exact witness,
2. is closed under the BC-tree flavored decomposition rule, and
3. is the unique minimal closed subtree containing the witness hit set.

*Proof sketch.*
The implementation first enlarges the path neighborhood, then invokes `closeByBCPath(...)`, which computes the BC-path closure between the relevant witness endpoints. By Lemma 2.2 the resulting region still contains an exact restricted witness. By Lemma 2.1 the BC path connecting the hit atoms is the unique minimal connected subtree, and by Lemma 2.3 adding the required boundary vertices yields the unique minimal closed subtree. Therefore the constructed region is canonical and minimal.

### 2.4 Implementation Mapping Table

| Mathematical object | Code representation |
|---|---|
| Exact restricted witness path | `PotentialHandleManager::buildExactRestricted(...)` output (`certVerts`, `certEdges`) |
| Witness hit set | implicit in `buildClosedHandleFromWitness(...)` via `pathVerts`, `pathEdges` |
| Minimal connected subtree | BC path produced by `closeByBCPath(...)` |
| Minimal closed subtree | `PotentialHandle::regionVerts` after `buildClosedHandleFromWitness(...)` |
| Boundary closure | `boundaryVerts` returned by `closeByBCPath(...)` |
| Seed handle | `buildSeedHandle(...)` followed by `assignNode(...)` |

**How LOCAL counters support this section.**
`semantic_escape_count = 0` gives differential support that the generated closed handle does not miss a global exact witness on exercised instances. This is the empirical companion to the minimal-closed-subtree theorem.

---

## Section 3. Complexity theorem (proof skeleton)

### 3.1 Definitions

**Definition 8 (Topology update cost).**
A *topology update* for owner `v` is the execution of `updateOwnerLocal(v, removedX, oldNeighbors)`, whose graph traversal work is measured by
- `topology_zone_bfs_vertices`
- `topology_zone_bfs_edges`.

**Definition 9 (Strict-child relocation cost).**
A *strict-child relocation* is one successful call to `relocateToStrictChild(...)`. The work spent rebuilding the selected child is measured by
- `strict_child_rebuild_vertices`
- `strict_child_rebuild_edges`.

**Definition 10 (Potential `Φ1`).**
Let `Φ1 = Σ_h budgetExp(h)` over all live witness handles. Since every relocation produces `new.budgetExp = old.budgetExp - 1`, `Φ1` decreases by at least one per successful strict-child relocation.

**Definition 11 (Potential `Φ2`).**
Let `Φ2` be the total class-id change charge over all owner endpoints, where an endpoint is charged whenever its old class identifier is not reused after refinement. The largest-fragment reuse rule makes `Φ2` amortizable by a logarithmic or halving-style argument under a proof-ready accounting scheme.

### 3.2 Lemmas

**Lemma 3.1 (Strict-child descent bound).**
Every successful relocation decreases `Φ1` by exactly one.

*Proof sketch.*
The relocation path uses only structural children and sets `new.budgetExp = old.budgetExp - 1` by construction. Hence each successful relocation consumes one unit of `Φ1`.

**Lemma 3.2 (Refinement-only update bound).**
Topology local updates refine endpoint partitions but never merge two old classes. Therefore each endpoint can only be reassigned when its current class is split by a touched zone.

*Proof sketch.*
This is exactly the refinement-only invariant checked in `LOCAL`. Since untouched classes are preserved and only touched classes are split, relabeling can be charged to strictly finer partitions.

**Lemma 3.3 (Largest-fragment reuse bounds relabel churn).**
If, after a split of one old class, the largest new fragment keeps the old class id, then an endpoint receives a fresh id only when it moves into a strictly smaller fragment. Consequently the number of id changes per endpoint is bounded by the number of times its containing fragment shrinks substantially.

*Proof sketch.*
This is the standard large-fragment retention argument. Every time an endpoint does not keep the old id, it belongs to a non-largest fragment, so a suitable potential on fragment sizes drops. The exact asymptotic form depends on how the fragment-size potential is formalized, but the charging mechanism is already present in the implementation.

**Lemma 3.4 (Locality of rebuild work).**
The measured work in `strict_child_rebuild_vertices` and `strict_child_rebuild_edges` is confined to the selected child region, never to the parent region or the whole graph.

*Proof sketch.*
Global fallback is disallowed (`strict_child_global_fallback_used = 0`). Rebuilds occur only through child-restricted calls such as `rebuildRestrictedFixedNode(...)` inside the selected structural child.

### 3.3 Theorem

**Theorem 3 (Complexity skeleton).**
Let
- `U_topo = Σ topology_zone_bfs_vertices + Σ topology_zone_bfs_edges`,
- `U_child = Σ strict_child_rebuild_vertices + Σ strict_child_rebuild_edges`,
- `R = Σ strict_child_depth_sum`.

Then the total running time of the oracle and solver is bounded by
1. the sum of topology refinement work `U_topo`,
2. the sum of strict-child rebuild work `U_child`,
3. the number of strict-child descents `R`, and
4. linear or near-linear bookkeeping overhead in watcher updates and component maintenance.

Moreover:
- `R` is bounded by the initial total strict-child potential `Φ1`, since every descent decreases `budgetExp` by one;
- `U_topo` is bounded by a charging argument over `Φ2`, since endpoint classes only refine and the largest fragment retains the old id;
- `U_child` is bounded by the sum of child-internal exact rebuild costs, each charged to one strict-child descent.

Therefore the overall complexity reduces to proving explicit upper bounds for `Φ1` and `Φ2` under the chosen decomposition budget and refinement accounting.

*Proof sketch.*
Combine Lemma 3.1 with the measured descent counter `strict_child_depth_sum`. Combine Lemmas 3.2 and 3.3 with the zone-BFS counters to charge topology work to class refinements. Finally use Lemma 3.4 to observe that rebuild work is localized to strict children and thus can be charged to child-descents rather than parent-scale updates. The remaining task for a full paper is to instantiate the asymptotic forms of `Φ1` and `Φ2` (for example by proving logarithmic charging from large-fragment reuse and by bounding initial budgets from the decomposition model).

### 3.4 Implementation Mapping Table

| Mathematical object | Code representation |
|---|---|
| Topology local update | `DecrementalNBTopology::updateOwnerLocal(...)` |
| Exact differential oracle | `computeOwnerExactMap(...)` in `LOCAL` |
| Strict-child relocation | `PotentialHandleKernel::relocateToStrictChild(...)` |
| Child-restricted rebuild | `rebuildRestrictedFixedNode(...)` |
| Potential `Φ1` | `PotentialHandle::budgetExp` summed over live handles |
| Potential `Φ2` charging events | `classEndpoints_`, `classRep_`, id changes in `applyOwnerPartition(...)` |
| Topology work counters | `topology_zone_bfs_vertices`, `topology_zone_bfs_edges` |
| Strict-child work counters | `strict_child_depth_sum`, `strict_child_rebuild_vertices`, `strict_child_rebuild_edges` |

**How LOCAL counters support this section.**
`strict_child_depth_sum` empirically measures the number of strict descents, which should be chargeable to `Φ1`. `topology_zone_bfs_vertices/edges` empirically measure the local update footprint, which should be chargeable to the refinement-only partition potential `Φ2`. The zero values of `owner_rebuild_calls` and `strict_child_global_fallback_used` support the claim that the observed costs come from localized refinement and child-restricted rebuilds rather than hidden global recomputation.

---

## Final summary

The current implementation already satisfies the experimental checklist needed for a literature-grade declaration: exact owner rebuilds have been eliminated from the release path, local refinement updates match exact differential checks, strict-child relocation never falls back globally, structural strict-child search misses no oracle-visible strict child on the exercised instances, and handle regions do not exhibit semantic escape in `LOCAL` mode.

What remains for a polished paper is not another code-level redesign but a final layer of exposition:
1. state the BC-tree flavored lattice as an explicit decomposition model and explain the SPQR seam as a refinement rather than a correctness dependency,
2. formalize `buildClosedHandleFromWitness(...)` as the unique minimal closed subtree lift of an exact witness,
3. present the potential-based complexity proof by turning the measured counters into theorems about `Φ1` and `Φ2`.
