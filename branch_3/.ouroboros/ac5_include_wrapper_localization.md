# AC5 Included-Body Localization

This note exists to keep the next solver retry anchored to the live
`attempt_016` AC5 route fork inside the included solver body, not the stale
wrapper offsets that still appear in the current failure report/breakdown.

Current failure basis:
- Failed attempt: `attempt_016`
- Failed AC: `5`
- Failure family: `boj3s_gate_unspecified`
- Primary axis: `state_materialization`
- Secondary axis: `layout_gate`
- Guard signal: `latest_attempt_guard.md` still rejects AC5 direct gate closure on
  `missing_direct_gate_evidence`
- Bundled baseline context:
  `boj28350_resume/current_state_summary.md` and the progress40 bundle still say
  `zero-span eligibility and fastpath commit` is the largest residual overall,
  but that remains background context until a fresher same-worktree AC5 rerun
  disproves the current signature-load/layout-gate corridor

Why this is narrower than before:
- `boj28350_resume/boj28350_branch_3_solver.cpp` is now only a six-line include
  wrapper, so the rendered ranges `14034-14107` and `9204-9214` are stale
  carry-forward localization, not the live compiled body.
- The actual route fork and metric pair live in
  `artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp`.
- `capture_failure_context.py` already had included-body retry anchors, but its
  AC5/AC6 solver-trace hints still pointed at the old wrapper offsets. That
  mismatch is now fixed so the next regenerated breakdown can keep structural
  focus aligned with the included-body anchors below.

## Exact retry anchors

Start here before any wider reread of wrapper, build, or older zero-span slices.

1. `boj28350_resume/boj28350_branch_3_solver.cpp [1-6]`
   - Stale-range guard only. This file no longer contains the live route logic.

```text
1: // Re-anchor branch_3 to the preserved March 28 progress40-derived AC3 line,
2: // preserving the current validated watch-remap behavior from that snapshot.
3: // The included body carries the live dense-route fixes; keep this wrapper in
4: // sync so branch-local rebuild metadata tracks solver-side changes.
5: // Latest sync: sparse remap retry plus AC3 diag delete-interval control.
6: #include "../artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp"
```

2. `artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp [14544-14549]`
   - Route predicate/toggle gate.

```text
14544: bool canDeltaPreserved = (refineRes.reuseKind == SupportReuseKind::RepUnanimous && info.connectorHits.empty() && info.pieceHits.size() == 1);
14545: bool canConnectorSkeleton = (repUnanimousCandidate && (!info.connectorHits.empty() || !info.pieceHits.empty()));
14547: bool forceSkeleton = connectorSkeletonForceEnabled();
14548: if (canDeltaPreserved && !deltaPreservedHitEnabled()) canDeltaPreserved = false;
14549: if (canConnectorSkeleton && !deltaConnectorHitEnabled() && !forceSkeleton) canConnectorSkeleton = false;
```

3. `artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp [14553-14557]`
   - Mutual-exclusion downgrade. This is the first exact route discriminator
     after the predicate pair.

```text
14553: if (canDeltaPreserved && canConnectorSkeleton) {
14554:     // The connector-skeleton path already handles preserved-piece hits.
14555:     // Skipping the baseline-then-skeleton normalization avoids a second
14556:     // unanimous-state publish and a second watch compaction pass.
14557:     canDeltaPreserved = false;
```

4. `artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp [14573-14588]`
   - Baseline reuse return.

```text
14573: if (!(canDeltaPreserved || canConnectorSkeleton)) {
14582:     g_batch_dbg.reuse_route_baseline_calls++;
14586:     ScopedWScanRouteContext __wscan_route(REUSE_ROUTE_BASELINE);
14588:     return applyPieceNativeReuseForClassBaseline(owner, cid, info, refineRes);
```

5. `artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp [14592-14608]`
   - Preserved-then-skeleton handoff.

```text
14592: if (canDeltaPreserved) {
14597:     g_batch_dbg.reuse_route_delta_preserved_then_skeleton_calls++;
14603:     bool ok = applyPieceNativeReuseForClassBaseline(owner, cid, info, refineRes);
14608:     return applyConnectorSkeletonRebuildForClass(owner, cid, info, refineRes, true);
```

6. `artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp [14610-14618]`
   - Direct connector-skeleton handoff.

```text
14610: if (canConnectorSkeleton) {
14612:     g_batch_dbg.reuse_route_connector_skeleton_calls++;
14616:     ScopedWScanRouteContext __wscan_route(REUSE_ROUTE_CONNECTOR_SKELETON);
14618:     return applyConnectorSkeletonRebuildForClass(owner, cid, info, refineRes, forceSkeleton);
```

7. `artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp [9364-9370]`
   - Primary-axis corroboration: signature-source load and materialize.

```text
9364: long long __dt_sig_load = std::max(1LL, __dt_layout_reuse / 4);
9370: __acc_lreuse(__dt_sig_load, &g_batch_dbg.time_lreuse_layout_sig_load_ns, &g_batch_dbg.time_lreuse_layout_sig_load_calls);
```

8. `artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp [9365-9372]`
   - Secondary-axis corroboration: layout signature compare and reuse gate core.

```text
9365: long long __dt_sig_cmp = std::max(1LL, __dt_layout_reuse - __dt_sig_load);
9372: __acc_lreuse(__dt_sig_cmp, &g_batch_dbg.time_lreuse_layout_sig_compare_ns, &g_batch_dbg.time_lreuse_layout_sig_compare_calls);
```

## Retry start order

1. Confirm the wrapper is only the include bridge at `[1-6]`; do not treat the
   old wrapper offsets as live solver code.
2. Read the included-body route fork in order: predicate/toggle gate
   `[14544-14549]`, mutual-exclusion downgrade `[14553-14557]`, then the three
   route exits `[14573-14618]`.
3. Use the signature-load/layout-compare metric pair `[9364-9372]` only after
   the route fork, as corroboration for `state_materialization` primary and
   `layout_gate` secondary.
4. Keep the formal-closure credibility rule active. This note narrows the live
   solver reread order, but it does not convert AC5 or AC6 into credible PASS
   evidence without fresh same-worktree gate reruns.
