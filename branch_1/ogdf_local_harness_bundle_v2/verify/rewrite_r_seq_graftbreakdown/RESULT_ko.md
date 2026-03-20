# rewrite-r-seq graft rewire breakdown

- rewriteSeqCalls: 111100
- rewriteSeqSucceededCases: 111100
- rewriteSeqFailedCases: 0
- seqFallbackCaseCount: 6894
- seqRewriteWholeCoreFallbackCount: 8829
- seqGraftRewireFallbackCount: 4818

## Trigger
- dominant remaining trigger in this breakdown task: RFT_GRAFT_REWIRE_FAIL = 4818

## Graft Rewire Subtypes
- GRB_OLDARC_DEAD: count=798, caseCount=798, firstDump=dumps/rewrite_r_seq_graftbreakdown/seq_fallback_GRB_OLDARC_DEAD_seed1_tc817_step1.txt
- GRB_OLDSLOT_INVALID: count=4020, caseCount=4020, firstDump=dumps/rewrite_r_seq_graftbreakdown/seq_fallback_GRB_OLDSLOT_INVALID_seed1_tc40_step1.txt

## Dominant
- dominant graft-rewire bailout subtype: GRB_OLDSLOT_INVALID = 4020
- testcase case count: 4020
- first sample: dumps/rewrite_r_seq_graftbreakdown/seq_fallback_GRB_OLDSLOT_INVALID_seed1_tc40_step1.txt
