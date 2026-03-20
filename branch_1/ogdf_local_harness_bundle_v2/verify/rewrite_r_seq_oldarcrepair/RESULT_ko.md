# rewrite-r-seq oldArc repair result

- correctness: green (`rewriteSeqFailedCases=0`, `rewriteSeqCalls=111100`)
- dominant remaining trigger: `RFT_GRAFT_REWIRE_FAIL = 4818`
- dominant graft-rewire subtype: `GRB_OLDARC_DEAD = 4818`
- oldArc repair attempts: `seqResolvedOldArcRepairAttemptCount=12468`
- oldArc repair success/fail: `success=0`, `fail=12468`, `used=0`
- oldArc repair outcomes: `PAR_FAIL_NO_CANDIDATE = 12468`

## baseline comparison
- `seqFallbackCaseCount`: 6894 -> 6894 (delta 0)
- `seqRewriteWholeCoreFallbackCount`: 8829 -> 8829 (delta 0)
- `GRB_OLDARC_DEAD`: 4818 -> 4818 (delta 0)
- `GRB_OLDSLOT_INVALID`: 0 -> 0 (delta 0)

## interpretation
- strong-match repair `outsideNode + poles` never found a usable replacement arc: `PAR_MATCH_BY_OUTSIDENODE_AND_POLES=0`.
- every oldArc repair attempt ended as `PAR_FAIL_NO_CANDIDATE`, so `GRB_OLDARC_DEAD` stayed unchanged at `4818`.
- fallback totals also stayed unchanged, so this repair rule is too strict or the live replacement arc genuinely does not exist under the current sequence state.

## sample
- `GRB_OLDARC_DEAD`: `dumps/rewrite_r_seq_graftbreakdown/seq_fallback_GRB_OLDARC_DEAD_seed1_tc817_step1.txt`
