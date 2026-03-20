# rewrite-r-seq Gate Result

## Status
- correctness: `green`
- behavior change: `none (instrumentation only)`

## Current Gate
- dominant remaining trigger: `RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED`
- trigger count: `6247`
- testcase count: `6247`
- first sample dump: `dumps/rewrite_r_seq_fallbacks/seq_fallback_RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED_seed1_tc0_step1.txt`

## Current Stats
- rewriteSeqCalls: `111100` delta=`0`
- rewriteSeqSucceededCases: `111100` delta=`0`
- rewriteSeqFailedCases: `0` delta=`0`
- seqFallbackCaseCount: `11631` delta=`0`
- seqRewriteWholeCoreFallbackCount: `14892` delta=`0`
- rewriteFallbackSpecialCaseCount: `93150` delta=`0`
- seqGraftRewireFallbackCount: `4818` delta=`0`
- seqProxyMetadataFallbackCount: `0` delta=`0`

## X-Incident Breakdown
- XIV_SHARED_WITH_LOOP: `6247` cases=`6247` dump=`dumps/rewrite_r_seq_xincident/seq_fallback_XIV_SHARED_WITH_LOOP_seed1_tc0_step1.txt`
- XIV_MIXED_OTHER: `0` cases=`0` dump=`-`
- XIV_MULTI_POS_PROXY: `0` cases=`0` dump=`-`
- XIV_ONE_POS_PROXY: `0` cases=`0` dump=`-`
- XIV_ZERO_PAYLOAD: `0` cases=`0` dump=`-`

## Next Safe Fix Target
- `sequence x-incident loop-sharing handling`

## Files
- `summary.json`: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_1/ogdf_local_harness_bundle_v2/verify/rewrite_r_seq_gate/summary.json`
- `comparison.json`: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_1/ogdf_local_harness_bundle_v2/verify/rewrite_r_seq_gate/comparison.json`
