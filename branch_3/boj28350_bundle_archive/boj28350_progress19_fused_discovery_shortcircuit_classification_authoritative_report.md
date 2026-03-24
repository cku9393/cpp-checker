# Progress19 fused discovery shortcircuit classification authoritative report

## What changed

Progress19 authoritative keeps all progress18 routes and optimizations fixed, and fully threads the fused-classification metrics into LOCAL summary, slow deletion export, and result packaging. The newly authoritative exclusive subaxis is split into suffix-only gate, single-middle gate, one-pass transition scan, transition emit and count finalize, and small-runlist inline materialization.

## Scope correction

`time_rdisc_*` remains the discovery-fusion umbrella. The new `time_fclass_*` counters isolate the fused classification internals: `time_fclass_onepass_transition_scan_ns`, `time_fclass_suffix_only_gate_ns`, `time_fclass_single_middle_gate_ns`, `time_fclass_transition_emit_runs_ns`, `time_fclass_run_count_finalize_ns`, and `time_fclass_small_runlist_inline_ns`. In before runs with `ENABLE_FUSED_DISCOVERY_CLASSIFY_OPT=0`, these stay zero. In after runs they are populated and the authoritative clean rerun can determine the next pivot inside the fused-classification bucket.

## Correctness gate

- `gate_connector_only_dense_256_after`: validator_ok=True, elapsed_sec=3.86
- `gate_both_on_dense_256_after`: validator_ok=True, elapsed_sec=3.78
- `gate_both_on_multi_512_after`: validator_ok=True, elapsed_sec=5.29

All reused or rerun gate results keep `local_active_mismatch=0`, `local_active_partition_mismatch=0`, `debug_touched_missing_classes=0`, `piece_materialize_fallback_calls=0`, `support_rebuild_fallback_calls=0`, and `unanimous_baseline_path_calls=0` where the authoritative result exists.

## 512 LOCAL before and after elapsed

| case | before_sec | after_sec | delta_sec |
| --- | ---: | ---: | ---: |
| connector_only dense 512 BASE | 34.45 | 38.07 | +3.62 |
| both_on dense 512 BASE | 38.78 | 35.96 | -2.82 |
| connector_only dense 512 SAMPLED | 37.67 | 35.62 | -2.05 |
| both_on dense 512 SAMPLED | 39.34 | 37.78 | -1.56 |
| both_on multi 512 SAMPLED | 5.28 | 5.51 | +0.23 |

## Fused classification exclusive subaxis, sampled aggregate after

| category | aggregate_ms | share_pct |
| --- | ---: | ---: |
| transition-state one-pass scan core | 1.836186 | 37.5 |
| shortcircuit classification fast path | 1.336143 | 27.3 |
| run transition emit and count finalize | 0.631831 | 12.9 |
| small-runlist inline materialization | 1.096421 | 22.4 |

## Fused classification exclusive subaxis, sampled aggregate before

| category | aggregate_ms | share_pct |
| --- | ---: | ---: |
| transition-state one-pass scan core | 0.0 | 0.0 |
| shortcircuit classification fast path | 0.0 | 0.0 |
| run transition emit and count finalize | 0.0 | 0.0 |
| small-runlist inline materialization | 0.0 | 0.0 |

## Per-case sampled after grouped table

### after_connector_only_dense_512_sampled

| category | ms | share_pct |
| --- | ---: | ---: |
| transition-state one-pass scan core | 0.714423 | 35.2 |
| shortcircuit classification fast path | 0.606151 | 29.9 |
| run transition emit and count finalize | 0.241643 | 11.9 |
| small-runlist inline materialization | 0.466682 | 23.0 |

### after_both_on_dense_512_sampled

| category | ms | share_pct |
| --- | ---: | ---: |
| transition-state one-pass scan core | 0.983453 | 39.2 |
| shortcircuit classification fast path | 0.647096 | 25.8 |
| run transition emit and count finalize | 0.333468 | 13.3 |
| small-runlist inline materialization | 0.546698 | 21.8 |

### after_both_on_multi_512_sampled

| category | ms | share_pct |
| --- | ---: | ---: |
| transition-state one-pass scan core | 0.13831 | 38.3 |
| shortcircuit classification fast path | 0.082896 | 23.0 |
| run transition emit and count finalize | 0.05672 | 15.7 |
| small-runlist inline materialization | 0.083041 | 23.0 |

## Fused classification volume counters, sampled after

| case | fclass_calls | suffix_only_hits | single_middle_hits | fused_onepass_calls | transition_steps | removed_to_kept | kept_to_removed | small_inline_hits |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| after_connector_only_dense_512_sampled | 30887 | 67 | 111 | 178 | 178 | 111 | 178 | 111 |
| after_both_on_dense_512_sampled | 30887 | 55 | 125 | 230 | 4873 | 175 | 280 | 175 |
| after_both_on_multi_512_sampled | 7581 | 10 | 56 | 111 | 2354 | 101 | 156 | 101 |

## Discovery-fusion umbrella and watch churn carry forward

| case | seek_boundary_ms | removed_scan_ms | kept_scan_ms | fused_bucket_ms | watch_churn_proxy_ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| after_connector_only_dense_512_sampled | 0.54387 | 0.0 | 0.0 | 1.0283 | 1983.296514 |
| after_both_on_dense_512_sampled | 0.556261 | 0.0 | 0.0 | 1.318507 | 2195.350797 |
| after_both_on_multi_512_sampled | 0.079014 | 0.0 | 0.0 | 0.180247 | 123.232436 |

## Top K slow deletion summary

### after_connector_only_dense_512_sampled

- idx=116, total_ms=194.265, fclass_suffix_only_hits=65, fclass_single_middle_hits=126, fclass_fused_onepass_calls=193, fclass_transition_steps=195, fclass_removed_to_kept_transitions=128, fclass_kept_to_removed_transitions=195, fclass_small_inline_hits=128
- idx=115, total_ms=192.566, fclass_suffix_only_hits=70, fclass_single_middle_hits=124, fclass_fused_onepass_calls=194, fclass_transition_steps=194, fclass_removed_to_kept_transitions=124, fclass_kept_to_removed_transitions=194, fclass_small_inline_hits=124
- idx=134, total_ms=189.273, fclass_suffix_only_hits=57, fclass_single_middle_hits=127, fclass_fused_onepass_calls=184, fclass_transition_steps=186, fclass_removed_to_kept_transitions=127, fclass_kept_to_removed_transitions=184, fclass_small_inline_hits=127

### after_both_on_dense_512_sampled

- idx=47, total_ms=222.884, fclass_suffix_only_hits=47, fclass_single_middle_hits=147, fclass_fused_onepass_calls=226, fclass_transition_steps=3545, fclass_removed_to_kept_transitions=179, fclass_kept_to_removed_transitions=258, fclass_small_inline_hits=179
- idx=158, total_ms=217.581, fclass_suffix_only_hits=55, fclass_single_middle_hits=115, fclass_fused_onepass_calls=170, fclass_transition_steps=170, fclass_removed_to_kept_transitions=115, fclass_kept_to_removed_transitions=170, fclass_small_inline_hits=115
- idx=112, total_ms=216.262, fclass_suffix_only_hits=51, fclass_single_middle_hits=142, fclass_fused_onepass_calls=195, fclass_transition_steps=296, fclass_removed_to_kept_transitions=144, fclass_kept_to_removed_transitions=197, fclass_small_inline_hits=144

### after_both_on_multi_512_sampled

- idx=25, total_ms=54.324, fclass_suffix_only_hits=9, fclass_single_middle_hits=60, fclass_fused_onepass_calls=116, fclass_transition_steps=2144, fclass_removed_to_kept_transitions=107, fclass_kept_to_removed_transitions=163, fclass_small_inline_hits=107
- idx=37, total_ms=53.73, fclass_suffix_only_hits=14, fclass_single_middle_hits=49, fclass_fused_onepass_calls=115, fclass_transition_steps=2380, fclass_removed_to_kept_transitions=101, fclass_kept_to_removed_transitions=167, fclass_small_inline_hits=101
- idx=49, total_ms=52.641, fclass_suffix_only_hits=10, fclass_single_middle_hits=75, fclass_fused_onepass_calls=112, fclass_transition_steps=1387, fclass_removed_to_kept_transitions=102, fclass_kept_to_removed_transitions=139, fclass_small_inline_hits=102

## Release and representative

| case | rc | timed_out | validator_ok | elapsed_sec | stdout_empty | stderr_empty | note |
| --- | --- | --- | --- | ---: | --- | --- | --- |
| after_both_on_dense_1024_release | 0 | False | True | 848.57 | False | True |  |
| after_both_on_dense_1024_release_repeat | missing | None | None | - | None | None | not run or not finished |
| after_both_on_multi_1024_release | 0 | False | True | 29.64 | False | True |  |
| after_both_on_dense_4096_release | missing | None | None | - | None | None | not run or not finished |
| after_both_on_multi_4096_release | missing | None | None | - | None | None | not run or not finished |

## Dense 1024 repeat stability

Missing or non-successful repeat runs.

## Reconstructed progress19 conclusion versus clean rerun

The reconstructed progress19 package suggested `run transition emit and count finalize` as the largest residual inside the fused-classification bucket. The authoritative clean rerun overturns that: sampled aggregate now shows `transition-state one-pass scan core` as the largest residual, with `shortcircuit classification fast path` second, `small-runlist inline materialization` third, and `run transition emit and count finalize` clearly smaller.

## Final residual cost judgement

Inside the round-defined bucket, authoritative clean LOCAL sampled runs give the following aggregate ordering:

- `transition-state one-pass scan core`: 1.836186 ms, 37.5%
- `shortcircuit classification fast path`: 1.336143 ms, 27.3%
- `small-runlist inline materialization`: 1.096421 ms, 22.4%
- `run transition emit and count finalize`: 0.631831 ms, 12.9%

Strict dominant does not appear because no category exceeds 50%. Therefore, by the round rule, the next pivot is the largest residual.

## Final conclusion

`next pivot after fused-classification round: transition-state one-pass scan core`