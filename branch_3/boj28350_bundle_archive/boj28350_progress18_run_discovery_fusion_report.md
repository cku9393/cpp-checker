# BOJ28350 progress18 run discovery fusion report

## What changed

Progress18 keeps all progress17 routes and semantics fixed, and adds exclusive discovery-fusion timers and counters inside the copy-plan bucket. The main code change is a fused one-pass discovery path behind `ENABLE_RUN_DISCOVERY_FUSION_OPT`, plus boundary reuse and shortcircuit classification counters. Existing block-copy and stable-compaction optimizations remain enabled.

## Scope correction

`time_plan_first_removed_seek_ns`, `time_plan_removed_run_discovery_ns`, `time_plan_kept_run_discovery_ns`, `time_plan_single_middle_shortcircuit_eligibility_ns`, and `time_plan_small_inline_buffer_prepare_ns` were narrowed so that `time_plan_*` remains the umbrella and the new `time_rdisc_*` timers carry the exclusive attribution. In the before path, `time_rdisc_removed_run_scan_ns` and `time_rdisc_kept_run_scan_ns` are non-zero and `time_rdisc_fused_onepass_scan_ns` is zero. In the after path, removed and kept scan timers collapse to zero while `time_rdisc_fused_onepass_scan_ns` becomes the dominant discovery timer.

## Correctness gate

- gate_connector_only_dense_256_after: validator_ok=True, elapsed_sec=2.07

- gate_both_on_dense_256_after: validator_ok=True, elapsed_sec=2.1

- gate_both_on_multi_512_after: validator_ok=True, elapsed_sec=5.72


All three gate runs kept `local_active_mismatch=0`, `local_active_partition_mismatch=0`, `debug_touched_missing_classes=0`, `piece_materialize_fallback_calls=0`, `support_rebuild_fallback_calls=0`, and `unanimous_baseline_path_calls=0`.

## 512 LOCAL before and after elapsed

| case | before_sec | after_sec | delta_sec |
| --- | --- | --- | --- |
| connector_only dense 512 BASE | 43.35 | 42.66 | -0.69 |
| both_on dense 512 BASE | 43.42 | 44.19 | 0.77 |
| connector_only dense 512 SAMPLED | 42.51 | 45.18 | 2.67 |
| both_on dense 512 SAMPLED | 42.46 | 43.61 | 1.15 |
| both_on multi 512 SAMPLED | 6.18 | 5.89 | -0.29 |


## Run discovery fusion exclusive subaxis, sampled aggregate

| category | aggregate_ms | share_pct |
| --- | --- | --- |
| first removed seek and boundary reuse | 0.616371 | 46.3 |
| removed-run discovery scan | 0.0 | 0.0 |
| kept-run discovery scan | 0.0 | 0.0 |
| single-pass fused discovery and shortcircuit classification | 0.714619 | 53.7 |


## Sampled case breakdown after

| case | seek_boundary_ms | removed_scan_ms | kept_scan_ms | fused_shortcircuit_ms | dominant_share_pct |
| --- | --- | --- | --- | --- | --- |
| connector_only comb_rect_dense 512 | 0.271914 | 0.0 | 0.0 | 0.30545 | 52.9 |
| both_on comb_rect_dense 512 | 0.251682 | 0.0 | 0.0 | 0.313415 | 55.5 |
| both_on multi_comb_rect 512 | 0.092775 | 0.0 | 0.0 | 0.095754 | 50.8 |


## Sampled case breakdown before

| case | seek_boundary_ms | removed_scan_ms | kept_scan_ms | fused_shortcircuit_ms | dominant_share_pct |
| --- | --- | --- | --- | --- | --- |
| connector_only comb_rect_dense 512 | 0.264711 | 0.291913 | 0.296634 | 0.016993 | 34.1 |
| both_on comb_rect_dense 512 | 0.250606 | 0.280935 | 0.310021 | 0.008529 | 36.5 |
| both_on multi_comb_rect 512 | 0.093663 | 0.089964 | 0.093314 | 0.004631 | 33.3 |


## Discovery fusion volume counters, sampled after

| case | rdisc_calls | boundary_reuse_hits | suffix_only_hits | single_middle_hits | two_pass_removed_calls | two_pass_kept_calls | fused_onepass_calls | removed_scan_steps | kept_scan_steps | fused_scan_steps |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only comb_rect_dense 512 | 21609 | 92 | 92 | 3073 | 0 | 0 | 21609 | 0 | 0 | 163708 |
| both_on comb_rect_dense 512 | 20948 | 64 | 64 | 2540 | 0 | 0 | 20948 | 0 | 0 | 304010 |
| both_on multi_comb_rect 512 | 6564 | 38 | 38 | 714 | 0 | 0 | 6564 | 0 | 0 | 73207 |


## Copy-plan umbrella and watch churn carry-forward

Progress17 had `removed and kept run discovery fusion` as the dominant copy-plan bucket category at 63.5%. Progress18 confirms that inside this bucket, the fused one-pass plus shortcircuit classification subaxis becomes the new dominant residual in LOCAL sampled runs. Broader measured residual outside the bucket still leaves watch churn larger overall, but by the round rule the next pivot stays inside discovery fusion.

## Top K slow deletion summary

### connector_only comb_rect_dense 512

- idx=343, total_ms=510.337, first_removed=7258, removed_runs=208, kept_runs=237, boundary_reuse_hits=0, suffix_only_hits=0, single_middle_hits=9, fused_onepass_calls=78, small_runlist_inline_hits=0

- idx=321, total_ms=456.461, first_removed=9258, removed_runs=263, kept_runs=317, boundary_reuse_hits=0, suffix_only_hits=0, single_middle_hits=4, fused_onepass_calls=86, small_runlist_inline_hits=0

- idx=291, total_ms=431.886, first_removed=13506, removed_runs=374, kept_runs=512, boundary_reuse_hits=0, suffix_only_hits=0, single_middle_hits=93, fused_onepass_calls=168, small_runlist_inline_hits=0

### both_on comb_rect_dense 512

- idx=301, total_ms=561.721, first_removed=10588, removed_runs=355, kept_runs=420, boundary_reuse_hits=0, suffix_only_hits=0, single_middle_hits=2, fused_onepass_calls=100, small_runlist_inline_hits=0

- idx=291, total_ms=550.818, first_removed=3479, removed_runs=230, kept_runs=254, boundary_reuse_hits=0, suffix_only_hits=0, single_middle_hits=4, fused_onepass_calls=101, small_runlist_inline_hits=0

- idx=293, total_ms=456.645, first_removed=5868, removed_runs=118, kept_runs=200, boundary_reuse_hits=0, suffix_only_hits=0, single_middle_hits=82, fused_onepass_calls=100, small_runlist_inline_hits=0

### both_on multi_comb_rect 512

- idx=1, total_ms=128.366, first_removed=0, removed_runs=0, kept_runs=0, boundary_reuse_hits=0, suffix_only_hits=0, single_middle_hits=0, fused_onepass_calls=0, small_runlist_inline_hits=0

- idx=17, total_ms=103.974, first_removed=7564, removed_runs=320, kept_runs=364, boundary_reuse_hits=0, suffix_only_hits=0, single_middle_hits=3, fused_onepass_calls=100, small_runlist_inline_hits=0

- idx=9, total_ms=98.889, first_removed=8373, removed_runs=254, kept_runs=318, boundary_reuse_hits=0, suffix_only_hits=0, single_middle_hits=10, fused_onepass_calls=84, small_runlist_inline_hits=0



## Release and representative

| case | rc | timed_out | validator_ok | elapsed_sec | stdout_empty | stderr_empty | note |
| --- | --- | --- | --- | --- | --- | --- | --- |
| both_on comb_rect_dense 1024 release | 0 | False | True | 442.36 | False | True |  |
| both_on comb_rect_dense 1024 release repeat | 0 | False | True | 449.36 | False | True |  |
| both_on multi_comb_rect 1024 release | 0 | False | True | 34.02 | False | True |  |
| both_on comb_rect_dense 4096 representative | 127 | False | False | 0.1189243090002492 | True | False | stale rc=127 from missing release binary |
| both_on multi_comb_rect 4096 representative | 127 | False | False | 0.10112760399988474 | True | False | stale rc=127 from missing release binary |


## Dense 1024 repeat stability

| run1_sec | run2_sec | diff_pct | stable_recovery | timing_stability_good |
| --- | --- | --- | --- | --- |
| 442.36 | 449.36 | 1.57 | True | True |


## Final residual cost judgement

Inside the round-defined bucket, progress18 moves the dominant share away from separate removed and kept scans into fused one-pass discovery plus shortcircuit classification. Aggregate after sampled share is 53.7% for `single-pass fused discovery and shortcircuit classification`, 46.3% for `first removed seek and boundary reuse`, and 0% for separate removed-run or kept-run scans. Therefore the next pivot inside the discovery-fusion bucket is the fused one-pass and shortcircuit path, not boundary reuse alone and not kept-run scan.

## Final conclusion

`next pivot after discovery-fusion round: single-pass fused discovery and shortcircuit classification`
