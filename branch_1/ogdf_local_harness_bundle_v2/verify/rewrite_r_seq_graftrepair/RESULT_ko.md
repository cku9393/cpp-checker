# rewrite-r-seq graftrepair result

- correctness: green (`rewriteSeqFailedCases=0`, `rewriteSeqCalls=111100`)
- sequence proxy snapshot: `seqResolvedProxySnapshotCount=135260`, `seqResolvedProxySnapshotFailCount=0`, `seqResolvedProxyRepairUsedCount=12468`
- dominant remaining trigger: `RFT_GRAFT_REWIRE_FAIL = 4818`
- dominant graft-rewire subtype: `GRB_OLDARC_DEAD = 4818`

## baseline comparison
- `seqFallbackCaseCount`: 6894 -> 6894 (delta 0)
- `seqRewriteWholeCoreFallbackCount`: 8829 -> 8829 (delta 0)
- `RFT_GRAFT_REWIRE_FAIL`: 4818 -> 4818 (delta 0)
- `GRB_OLDSLOT_INVALID`: 4020 -> 0 (delta -4020)
- `GRB_OLDARC_DEAD`: 798 -> 4818 (delta 4020)

## interpretation
- sequence-only old-slot snapshot repair removed stale `oldSlotInU` failures: `GRB_OLDSLOT_INVALID` is now `0`.
- overall fallback volume did not improve: `seqFallbackCaseCount` and `seqRewriteWholeCoreFallbackCount` are unchanged versus baseline.
- the graft bailout mass moved to `GRB_OLDARC_DEAD`, so the next safe fix target is `sequence cumulative oldArc liveness repair`.

## sample
- `GRB_OLDARC_DEAD`: `dumps/rewrite_r_seq_graftbreakdown/seq_fallback_GRB_OLDARC_DEAD_seed1_tc817_step1.txt`
