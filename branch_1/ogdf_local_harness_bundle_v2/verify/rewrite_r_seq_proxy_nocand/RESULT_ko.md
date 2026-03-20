# rewrite_r_seq_proxy_nocand

- correctness: green
- runCount: 32
- seqResolvedOldArcRepairAttemptCount: 12468
- seqResolvedOldArcRepairSuccessCount: 0
- seqResolvedOldArcRepairFailCount: 12468
- dominant no-candidate subtype: `PNC_SAME_POLES_BUT_OTHER_OUTSIDE = 7650`
- dominant remaining trigger: `RFT_GRAFT_REWIRE_FAIL = 4818`

## Comparison To Baseline
- seqFallbackCaseCount: 6894 -> 6894 (delta +0)
- seqRewriteWholeCoreFallbackCount: 8829 -> 8829 (delta +0)
- GRB_OLDARC_DEAD: 4818 -> 4818 (delta +0)
- seqResolvedOldArcRepairAttemptCount: 12468 -> 12468 (delta +0)

## No-Candidate Breakdown
- PNC_SAME_POLES_BUT_OTHER_OUTSIDE: 7650
- PNC_OLDNODE_NO_LIVE_ARCS: 4818
- PNC_CANDIDATE_SLOT_ARCID_MISMATCH: 0
- PNC_CANDIDATE_SLOT_NOT_VIRTUAL: 0
- PNC_MULTI_WEAK_CANDIDATES: 0
- PNC_NO_ARC_TO_OUTSIDENODE: 0
- PNC_OLDNODE_DEAD: 0
- PNC_OTHER: 0
- PNC_OUTSIDENODE_DEAD: 0
- PNC_TO_OUTSIDENODE_BUT_WRONG_POLES: 0

## First Sample Dumps
- PNC_OLDNODE_NO_LIVE_ARCS: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_1/ogdf_local_harness_bundle_v2/dumps/rewrite_r_seq_proxy_nocand/seq_proxy_nocand_PNC_OLDNODE_NO_LIVE_ARCS_seed1_tc40_step1.txt
- PNC_SAME_POLES_BUT_OTHER_OUTSIDE: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_1/ogdf_local_harness_bundle_v2/dumps/rewrite_r_seq_proxy_nocand/seq_proxy_nocand_PNC_SAME_POLES_BUT_OTHER_OUTSIDE_seed1_tc56_step1.txt

## Next Target
- outsideNode drift tolerant repair 검토
