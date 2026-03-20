# Rewrite Seq Freeze Result

## Status

- rewrite-r single-step: green
- rewrite-r-seq: green
- seqFallbackCaseCount: `0`
- seqRewriteWholeCoreFallbackCount: `0`
- rewriteSeqCalls (dedup bench aggregate): `110000`

## Regression

- manifest: `regressions/rewrite_seq_cases.json`
- totalCases: `6`
- passedCases: `6`
- failedCases: `0`
- case `same_type_sp_cleanup_tc40_step1`: pass, dump=`dumps/rewrite_r_seq_regression/SEQ_REPLAY_CAPTURE_seed1_tc40_step1.txt`
- case `xshared_loopshared_proxy_loop_real_tc56_step1`: pass, dump=`dumps/rewrite_r_seq_regression/SEQ_REPLAY_CAPTURE_seed1_tc56_step1.txt`
- case `selfloop_spqrready_tc328_step1`: pass, dump=`dumps/rewrite_r_seq_regression/SEQ_REPLAY_CAPTURE_seed1_tc328_step1.txt`
- case `selfloop_oneedge_tc444_step1`: pass, dump=`dumps/rewrite_r_seq_regression/SEQ_REPLAY_CAPTURE_seed2_tc444_step1.txt`
- case `xshared_spqrready_tc851_step1`: pass, dump=`dumps/rewrite_r_seq_regression/SEQ_REPLAY_CAPTURE_seed1_tc851_step1.txt`
- case `xincident_oneedge_tc33_step1`: pass, dump=`dumps/rewrite_r_seq_regression/SEQ_REPLAY_CAPTURE_seed1_tc33_step1.txt`

## Bench

- smoke `seed=1 rounds=1000`: avgCaseMs=`0.259429`, maxCaseMs=`1.481880`
- full `seed=1..10 rounds=1000`: totalCases=`10000`, avgCaseMs=`0.250990`
- full `seed=1..20 rounds=5000`: totalCases=`100000`, avgCaseMs=`0.255534`
- dedup aggregate totalCases=`110000`, totalRewriteCalls=`139855`, totalElapsedMs=`28063.342`
- dedup aggregate WHOLE_CORE_REBUILD=`0`
- internal scale ratio full/smoke avgCaseMs=`0.984988`

## Solver Prep

- feature flag: `USE_REWRITE_SEQ_ENGINE`
- API: `runRewriteSequenceToFixpoint(ReducedSPQRCore &, RewriteSeqStats &, std::string &)`
- next stage: `solver integration / end-to-end compare mode`

## References

- regression summary: `dumps/rewrite_r_seq_regression/summary.json`
- bench aggregate summary: `dumps/rewrite_r_seq_bench_freeze/aggregate_summary.json`
- freeze summary: `dumps/rewrite_r_seq_freeze/summary.json`
- freeze doc: `docs/rewrite_seq_engine_freeze_ko.md`
- solver checklist: `docs/rewrite_seq_solver_integration_checklist_ko.md`
- compare skeleton: `docs/rewrite_seq_solver_compare_mode_skeleton_ko.md`

## Verification

- verified: `true`
- benchSummaryCount: `31`
- problems: `0`
- verification summary: `dumps/rewrite_r_seq_freeze/verification.json`

## Checkpoint

- git HEAD: `179336d2faf130faa0da3d70b4c614209f0ba998`
- dirty worktree: `true`
- git tag는 만들지 않았고, freeze 기준은 zip artifact + summary로 고정
