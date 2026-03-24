# progress20 interrupted local findings

Status:
container reset interrupted the authoritative progress20 round before release and clean 4096 reruns completed.
The results below are from clean LOCAL reruns that were completed before the reset.

## Completed scope

Gate reruns all passed.

- connector_only, comb_rect_dense 256 LOCAL, PROFILE_NONE, all opts on: validator OK, 3.46s
- both_on, comb_rect_dense 256 LOCAL, PROFILE_NONE, all opts on: validator OK, 3.76s
- both_on, multi_comb_rect 512 LOCAL, PROFILE_SAMPLED, all opts on: validator OK, 5.16s

All three preserved:

- local_active_mismatch=0
- local_active_partition_mismatch=0
- debug_touched_missing_classes=0
- piece_materialize_fallback_calls=0
- support_rebuild_fallback_calls=0
- unanimous_baseline_path_calls=0

Tiny sampled smoke also confirmed the new `time_tscan_*` and `tscan_*` keys were present in stderr and result export.

## Clean LOCAL 512 reruns

Before:

- before_connector_only_dense_512_base: validator OK, 49.86s
- before_both_on_dense_512_base: validator OK, 48.82s
- before_connector_only_dense_512_sampled: validator OK, 49.28s
- before_both_on_dense_512_sampled: validator OK, 48.45s
- before_both_on_multi_512_sampled: validator OK, 7.00s

After:

- after_connector_only_dense_512_base: validator OK, 47.34s
- after_both_on_dense_512_base: validator OK, 49.42s
- after_connector_only_dense_512_sampled: validator OK, 45.89s
- after_both_on_dense_512_sampled: validator OK, 51.32s
- after_both_on_multi_512_sampled: validator OK, 6.97s

## Authoritative LOCAL sampled aggregate for one-pass scan bucket

After sampled grouped aggregate:

- boundary seeded scan window narrowing: 1.986 ms, 15.3%
- transition-state branch and state load core: 10.353 ms, 79.8%
- run boundary commit and count update: 0.022 ms, 0.2%
- tail stop and early exit reuse: 0.605 ms, 4.7%

Per case:

connector_only, comb_rect_dense 512 LOCAL, PROFILE_SAMPLED

- boundary seeded scan window narrowing: 0.860 ms, 56.3%
- transition-state branch and state load core: 0.397 ms, 26.0%
- run boundary commit and count update: 0.010 ms, 0.7%
- tail stop and early exit reuse: 0.261 ms, 17.1%

both_on, comb_rect_dense 512 LOCAL, PROFILE_SAMPLED

- boundary seeded scan window narrowing: 1.012 ms, 10.1%
- transition-state branch and state load core: 8.692 ms, 86.8%
- run boundary commit and count update: 0.010 ms, 0.1%
- tail stop and early exit reuse: 0.303 ms, 3.0%

both_on, multi_comb_rect 512 LOCAL, PROFILE_SAMPLED

- boundary seeded scan window narrowing: 0.115 ms, 8.1%
- transition-state branch and state load core: 1.265 ms, 88.9%
- run boundary commit and count update: 0.002 ms, 0.1%
- tail stop and early exit reuse: 0.042 ms, 2.9%

## Current clean-local conclusion before reset

The progress19 authoritative conclusion that the next pivot should stay inside the one-pass scan bucket was preserved.
After re-splitting that bucket in progress20 local clean reruns, the next pivot became:

`next pivot after one-pass-scan round: transition-state branch and state load core`

## Missing because of reset

These were not completed authoritatively before the environment reset:

- both_on, comb_rect_dense 1024 RELEASE clean rerun
- both_on, comb_rect_dense 1024 RELEASE repeat stability
- both_on, multi_comb_rect 1024 RELEASE clean rerun
- authoritative clean 4096 representative reruns
- authoritative progress20 source packaging
- authoritative progress20 merged json packaging
