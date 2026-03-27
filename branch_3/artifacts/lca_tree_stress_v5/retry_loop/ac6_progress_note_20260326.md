## AC6 progress note (2026-03-26)

Context:
- Goal remains AC6 closure for `./lca_boj3s_gate.sh` twice on the same tree.
- A fresh formal gate run before these edits failed badly on `correctness_smoke`, `hard_scaling_strict`, `boj_3s_large_adversarial`, and `boj_3s_large_mix`.
- The representative blocking rows were:
  - `comb_rect_dense 512 seed=1 L0 Q0` at the `1.5s` `correctness_smoke` cap
  - `comb_core 16384 seed=1 L1 Q1` at the `3.0s` `hard_scaling_strict` cap

Solver changes made:
1. Connector-skeleton publish direct-commit path
- File: `boj28350_resume/boj28350_branch_3_solver.cpp`
- Targeted axis: progress40-aligned zero-span / state-materialization follow-through inside the connector-skeleton route
- Change:
  - Added direct canonical connector-state commit helper
  - Skipped redundant connector publish rescans/watch-id rebuilds/canonical rebuilds when the route already had the final connector handle set
  - Skipped preserved publish rescans when preserved metadata had not changed
- Measured effect:
  - `comb_rect_dense 512`: about `9.32s -> 7.07s`
  - `comb_core 16384`: about `7.23s -> 7.50s` (slight regression)
- Diagnostic result on `comb_rect_dense 512`:
  - `dispatch_publish_watch_id_rebuild_calls=0`
  - `dispatch_publish_canonical_rebuild_calls=0`
  - `dispatch_publish_noop_calls=30887`
  - `reuse_final_publish_noop_calls=30887`
  - `reuse_final_publish_skipped_calls=30887`
  - `time_route_dispatch_ns` fell from about `13.16s` to `11.26s`

2. Preserve-piece split/fixup bypass when `info.pieceHits` is empty
- File: `boj28350_resume/boj28350_branch_3_solver.cpp`
- Targeted axis: eliminate unnecessary preserved-piece work on connector-only unanimous updates
- Change:
  - When the delete does not touch preserved-piece handles, reuse existing preserved pieces/attachments directly
  - Skip full preserved-piece scanning, split probing, and attachment-fixup loops in that common case
- Measured effect:
  - `comb_rect_dense 512`: about `7.07s -> 5.19s`
  - `comb_core 16384`: about `7.50s -> 7.41s`
- Sampled LOCAL profile on `comb_rect_dense 512` after this cut showed the next dense-row bottlenecks were still inside the connector-skeleton route:
  - `time_reuse_connector_direct_retag_ns=2482895577`
  - `time_watch_diff_build_ns=2488330082`
  - `time_state_publish_ns=2472610983`
  - `time_reuse_final_publish_commit_ns=2471424557`
  - `time_reuse_keepmask_scan_ns=1930894225`
  - `time_reuse_piece_split_apply_ns=1930062046`
  - `time_wscan_preserved_keepstamp_build_ns=1810715008`
  - `time_reuse_patch_tree_build_ns=1066554900`

3. Queryless-mode fast path
- File: `boj28350_resume/boj28350_branch_3_solver.cpp`
- Targeted axis: comb-core startup path, not the connector reuse line
- Change:
  - If `activeQueryTotal_ == 0` after init, skip the owner/rebuild sweep entirely
  - In `eraseVertex`, delegate directly to `topo_.deleteVertexAndSplit(...)` with empty `changes`
- Measured effect:
  - `comb_core 16384`: about `7.41s -> 6.95s`
  - `comb_rect_dense 512`: about `5.19s -> 5.03s`
- Sampled LOCAL profile on `comb_core 16384` showed a distinct mode:
  - `active_query_peak=0`
  - `class_split_events=0`
  - all reuse/connector-skeleton counters stayed zero
  - runtime is therefore not blocked by the progress40 reuse route on that case

Current measured state:
- `comb_rect_dense 512 seed=1 L0 Q0`
  - still times out at `1.5s`
  - latest measured release runtime: `5.029267s`
- `comb_core 16384 seed=1 L1 Q1`
  - still times out at `3.0s`
  - latest measured release runtime: `6.946457s`

Why no formal `boj3s` rerun was done:
- Both representative blocking rows still miss their gate caps by wide margins.
- A fresh full `./lca_boj3s_gate.sh` run would be informative only after another material cut.

Best next retry directions:
1. Dense-row / primary-axis continuation
- Stay inside `applyConnectorSkeletonRebuildForClass(...)`
- Attack `watch-diff`, connector direct-retag, and final state-commit work that still dominates the dense sampled profile

2. Comb-core / secondary-axis continuation
- Treat `comb_core 16384` as a startup/topology-only path
- Investigate additional queryless-mode initialization/deletion shortcuts instead of more connector-reuse edits

Files changed in this session:
- `boj28350_resume/boj28350_branch_3_solver.cpp`
