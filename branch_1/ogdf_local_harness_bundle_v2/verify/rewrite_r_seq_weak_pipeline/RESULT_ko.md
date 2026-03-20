# rewrite-r-seq weak pipeline 계측 결과

- baseline: `verify/rewrite_r_seq_oldarcweak/summary.json`
- current: `verify/rewrite_r_seq_weak_pipeline/summary.json`
- full campaign: `32` runs, green=`True`

## 핵심 수치
- `rewriteSeqCalls`: `111100`
- `seqFallbackCaseCount`: `6894`
- `seqRewriteWholeCoreFallbackCount`: `8829`
- `seqResolvedOldArcRepairAttemptCount`: `0`
- `seqResolvedOldArcRepairSuccessCount`: `0`
- `seqResolvedOldArcRepairFailCount`: `0`
- `seqResolvedOldArcWeakRepairAttemptCount`: `0`
- `seqResolvedOldArcWeakRepairSuccessCount`: `0`
- `seqResolvedOldArcWeakRepairFailCount`: `0`

## weak pipeline breakdown
- `seqWeakRepairGateCounts`: `{'WRG_NOT_NEEDED_STRONG_LIVE': 12468, 'WRG_ENTER_PNC_SAME_POLES_BUT_OTHER_OUTSIDE': 0, 'WRG_SKIP_PNC_OLDNODE_NO_LIVE_ARCS': 0, 'WRG_SKIP_OTHER_PNC': 0, 'WRG_OTHER': 0}`
- `seqWeakRepairCandidateCounts`: `{'WRC_ZERO_SAME_POLE_CANDIDATES': 0, 'WRC_ONE_SAME_POLE_CANDIDATE': 0, 'WRC_MULTI_SAME_POLE_CANDIDATES': 0, 'WRC_SLOT_INVALID': 0, 'WRC_OTHER': 0}`
- `seqWeakRepairCommitCounts`: `{'WCO_NOT_ATTEMPTED': 0, 'WCO_FAILED_BEFORE_GRAFT': 0, 'WCO_GRAFT_FAIL': 0, 'WCO_NORMALIZE_FAIL': 0, 'WCO_ACTUAL_INVARIANT_FAIL': 0, 'WCO_ORACLE_FAIL': 0, 'WCO_COMMITTED': 0}`
- `seqWeakRepairEnteredCount`: `0`
- `seqWeakRepairTentativeSuccessCount`: `0`
- `seqWeakRepairCommittedCount`: `0`
- `seqWeakRepairRollbackCount`: `0`

## 결론
- dominant 원인은 `WRG_NOT_NEEDED_STRONG_LIVE=12468` 입니다.
- `WRG_ENTER_PNC_SAME_POLES_BUT_OTHER_OUTSIDE=0` 이고, candidate/commit 단계는 전부 `0`입니다.
- 따라서 다음 safe target 은 weak accept rule 완화가 아니라, resolver 이후 graft 단계에서 `oldArc`가 stale/dead 로 바뀌는 경로를 추적하는 쪽입니다.

## 산출물
- `summary.json`
- `comparison.json`
- `run_manifest.json`
