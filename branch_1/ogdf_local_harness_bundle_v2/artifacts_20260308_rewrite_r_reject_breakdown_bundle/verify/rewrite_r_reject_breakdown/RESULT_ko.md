rewrite-r reject reason breakdown collection and one safe optimization pass completed.

Baseline:
- rewriteCalls: 111100
- compactReadyCount: 63667 (57.31%)
- compactRejectedFallbackCount: 47433 (42.69%)
- backendBuildRawFallbackCount: 0
- reject breakdown:
  - NOT_BICONNECTED: 27004
  - TOO_SMALL_FOR_SPQR: 20429
  - others: 0
- first dumps:
  - NOT_BICONNECTED: dumps/rewrite_r_rejects/reject_NOT_BICONNECTED_seed1_tc2.txt
  - TOO_SMALL_FOR_SPQR: dumps/rewrite_r_rejects/reject_TOO_SMALL_FOR_SPQR_seed1_tc3.txt

Selected reason:
- NOT_BICONNECTED (largest reject reason, safe to probe first)

Optimization tried:
- explicit safe prune pass before SPQR precheck
- remove zero-payload PROXY edges
- remove isolated vertices
- keep existing 2-vertex multi-edge special case behavior

Optimized replay:
- rewriteCalls: 111100
- compactReadyCount: 63667 (57.31%)
- compactRejectedFallbackCount: 47433 (42.69%)
- backendBuildRawFallbackCount: 0
- reject breakdown:
  - NOT_BICONNECTED: 27004
  - TOO_SMALL_FOR_SPQR: 20429
  - others: 0

Comparison:
- compactReadyCount delta: 0
- compactRejectedFallbackCount delta: 0
- compactReadyPct delta: 0.00 pp
- compactRejectedFallbackPct delta: 0.00 pp

Conclusion:
- correctness stayed green across the full staged rewrite-r random campaign
- reject reason breakdown is now available
- NOT_BICONNECTED is the dominant reject reason
- this safe prune pass did not reduce fallback usage, so the next improvement needs deeper local compact handling for non-biconnected post-delete graphs rather than more superficial pruning
