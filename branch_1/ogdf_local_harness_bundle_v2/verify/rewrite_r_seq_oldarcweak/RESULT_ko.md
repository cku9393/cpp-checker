# RESULT

- baseline: `verify/rewrite_r_seq_proxy_nocand/summary.json`
- runCount: `32`
- rewriteSeqCalls: `111100`
- rewriteSeqSucceededCases: `111100`
- rewriteSeqFailedCases: `0`

## 핵심 비교
- `seqFallbackCaseCount`: `6894 -> 6894`
- `seqRewriteWholeCoreFallbackCount`: `8829 -> 8829`
- `GRB_OLDARC_DEAD`: `4818 -> 4818`
- `seqResolvedOldArcRepairAttemptCount`: `12468 -> 0`
- `seqResolvedOldArcRepairSuccessCount`: `0 -> 0`
- `seqResolvedOldArcRepairFailCount`: `12468 -> 0`
- `seqResolvedOldArcWeakRepairAttemptCount`: `0 -> 0`
- `seqResolvedOldArcWeakRepairSuccessCount`: `0 -> 0`
- `seqResolvedOldArcWeakRepairFailCount`: `0 -> 0`

## Weak Repair Outcome
- 모든 weak repair outcome count가 `0`이다.

## 결론
- weak repair path never activated in the full campaign, so the oldArc-dead fallback stayed unchanged
