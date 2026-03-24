# BOJ 28350 progress15 in-place stable compaction report

## 결론

이번 라운드에서는 `in-place stable compaction` 버킷을 다시 쪼갰고, LOCAL sampled 기준으로 strict dominant는 없지만 largest residual은 `contiguous run block copy compaction`으로 정리됐다.

after sampled grouped aggregate는 `run discovery coalescing` 2.625ms, `contiguous run block copy compaction` 2.659ms, `scratch reuse and allocation avoidance` 0.703ms, `tail cleanup and final resize fast path` 0.767ms였다. share로 보면 각각 38.9퍼센트, 39.4퍼센트, 10.4퍼센트, 11.4퍼센트다.

broader measured residual로는 `watch churn`이 여전히 더 큰 proxy bucket이다. 하지만 이번 라운드의 pivot 규칙은 stable-compaction bucket 내부 next pivot 판정이므로, 현재까지 확보된 근거상 마지막 결론 한 줄은 이거다.

`next pivot after stable-compaction round: contiguous run block copy compaction`

## provenance

이번 report와 merged json은 completed progress15 session note, surviving progress14 report and json, 그리고 current progress15 source를 합쳐 재구성했다. progress15 raw per-run directories 자체는 현재 filesystem에 남아 있지 않다. 그래서 gate와 512 LOCAL before after는 completed note 기준으로 복구했고, dense 1024 release 이후의 compact diag rerun과 remaining release or representative artifacts는 unrecovered 상태로 표기했다.

현재 source에 대해서는 `g++ -std=gnu++17 -O2 -fsyntax-only` check를 다시 돌렸고 통과했다.

## 무엇을 바꿨는가

1. `retainClassWatchByKeepMask` 안의 stable-compaction 구간을 `time_scomp_first_removed_seek_ns`, `time_scomp_suffix_only_check_ns`, `time_scomp_kept_count_scan_ns`, `time_scomp_kept_run_partition_build_ns`, `time_scomp_prefix_skip_ns`, `time_scomp_contiguous_run_block_copy_ns`, `time_scomp_elementwise_emit_ns`, `time_scomp_scratch_prepare_ns`, `time_scomp_tail_cleanup_ns`, `time_scomp_final_resize_swap_ns`로 다시 분해했다.

2. coarse umbrella인 `time_kvec_kept_count_scan_ns`, `time_kvec_scratch_prepare_ns`, `time_kvec_stable_emit_unchanged_prefix_ns`, `time_kvec_stable_emit_moved_suffix_ns`, `time_kvec_final_resize_or_swap_ns`의 scope를 정리했다.

3. unchanged prefix skip, suffix-only resize fast path, single contiguous middle run block-move 성격 fast path, kept-run block copy 우선 경로를 넣었다.

4. stable compaction slow deletion summary에 first removed index, removed run count, kept run count, prefix skipped handles, block copied handles, elementwise emitted handles, suffix-only 여부, single-middle-run 여부, scratch capacity reuse 여부를 남기도록 포맷을 확장했다.

## stable-compaction scope correction 여부와 결과

recovered grouped subtotal 기준으로 after sampled stable-compaction subtotal은 각 case의 recovered grouped sum과 일치한다. finer raw umbrella-gap row는 surviving artifacts에 남지 않아 grouped subtotal 기준으로 gap 0만 보고한다.

| label | scomp_total_ms | sum_grouped_exclusive_ms | gap_ms |
| --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 2.998 | 2.998 | 0.0 |
| both_on, comb_rect_dense 512 LOCAL | 2.927 | 2.927 | 0.0 |
| both_on, multi_comb_rect 512 LOCAL | 0.829 | 0.829 | 0.0 |

## correctness gate

| tag | validator_ok | local_active_mismatch | local_active_partition_mismatch | debug_touched_missing_classes | piece_materialize_fallback_calls | support_rebuild_fallback_calls | unanimous_baseline_path_calls |
| --- | --- | --- | --- | --- | --- | --- | --- |
| gate_connector_only_dense_256_after | True | 0 | 0 | 0 | 0 | 0 | 0 |
| gate_both_on_dense_256_after | True | 0 | 0 | 0 | 0 | 0 | 0 |
| gate_both_on_multi_512_after | True | 0 | 0 | 0 | 0 | 0 | 0 |

세 gate 모두 `validator OK`, mismatch 0, fallback 0을 유지했다.

## 실행 상태 요약

| tag | solver | toggle | mode | n | profile_mode | stable_compaction_opt | elapsed_solver_s | rc | timed_out | validator_ok |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| gate_connector_only_dense_256_after | LOCAL | connector_only | comb_rect_dense | 256 | PROFILE_NONE | 1 | None | 0 | False | True |
| gate_both_on_dense_256_after | LOCAL | both_on | comb_rect_dense | 256 | PROFILE_NONE | 1 | None | 0 | False | True |
| gate_both_on_multi_512_after | LOCAL | both_on | multi_comb_rect | 512 | PROFILE_SAMPLED | 1 | None | 0 | False | True |
| before_connector_only_dense_512_base | LOCAL | connector_only | comb_rect_dense | 512 | PROFILE_BASE | 0 | 57.9 | 0 | False | True |
| after_connector_only_dense_512_base | LOCAL | connector_only | comb_rect_dense | 512 | PROFILE_BASE | 1 | 59.52 | 0 | False | True |
| before_both_on_dense_512_base | LOCAL | both_on | comb_rect_dense | 512 | PROFILE_BASE | 0 | 61.96 | 0 | False | True |
| after_both_on_dense_512_base | LOCAL | both_on | comb_rect_dense | 512 | PROFILE_BASE | 1 | 62.8 | 0 | False | True |
| before_connector_only_dense_512_sampled | LOCAL | connector_only | comb_rect_dense | 512 | PROFILE_SAMPLED | 0 | 63.23 | 0 | False | True |
| after_connector_only_dense_512_sampled | LOCAL | connector_only | comb_rect_dense | 512 | PROFILE_SAMPLED | 1 | 59.9 | 0 | False | True |
| before_both_on_dense_512_sampled | LOCAL | both_on | comb_rect_dense | 512 | PROFILE_SAMPLED | 0 | 64.31 | 0 | False | True |
| after_both_on_dense_512_sampled | LOCAL | both_on | comb_rect_dense | 512 | PROFILE_SAMPLED | 1 | 63.33 | 0 | False | True |
| before_both_on_multi_512_sampled | LOCAL | both_on | multi_comb_rect | 512 | PROFILE_SAMPLED | 0 | 6.99 | 0 | False | True |
| after_both_on_multi_512_sampled | LOCAL | both_on | multi_comb_rect | 512 | PROFILE_SAMPLED | 1 | 7.03 | 0 | False | True |
| after_both_on_dense_1024_release | RELEASE | both_on | comb_rect_dense | 1024 | PROFILE_BASE | 1 | None | -9 | True | False |
| after_both_on_dense_1024_release_diag | RELEASE | both_on | comb_rect_dense | 1024 | PROFILE_BASE | 1 | None | None | None | None |
| after_both_on_multi_1024_release | RELEASE | both_on | multi_comb_rect | 1024 | PROFILE_BASE | 1 | None | None | None | None |
| after_both_on_dense_4096_release | RELEASE | both_on | comb_rect_dense | 4096 | PROFILE_BASE | 1 | None | None | None | None |
| after_both_on_multi_4096_release | RELEASE | both_on | multi_comb_rect | 4096 | PROFILE_BASE | 1 | None | None | None | None |

## `connector_only 512 LOCAL`, `both_on 512 LOCAL`, `both_on multi_comb_rect 512 LOCAL` before와 after 주요 시간 분해 표

before stable-compaction total은 progress14 after rows를 semantic baseline proxy로 사용했다. after stable-compaction total은 completed progress15 session note에서 recovered한 grouped exclusive subtotal이다.

| label | before_base_elapsed_s | after_base_elapsed_s | before_sampled_elapsed_s | after_sampled_elapsed_s | before_in_place_stable_compaction_ms_proxy_from_progress14 | after_in_place_stable_compaction_ms | stable_compaction_drop_pct |
| --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 57.9 | 59.52 | 63.23 | 59.9 | 32.955 | 2.998 | 90.9 |
| both_on, comb_rect_dense 512 LOCAL | 61.96 | 62.8 | 64.31 | 63.33 | 26.668 | 2.927 | 89.0 |
| both_on, multi_comb_rect 512 LOCAL | None | None | 6.99 | 7.03 | 7.399 | 0.829 | 88.8 |

## in-place stable compaction exclusive subaxis 표

| label | after_stable_compaction_ms | after_run_discovery_coalescing_ms | after_run_discovery_coalescing_share_pct | after_contiguous_run_block_copy_compaction_ms | after_contiguous_run_block_copy_compaction_share_pct | after_scratch_reuse_and_allocation_avoidance_ms | after_scratch_reuse_and_allocation_avoidance_share_pct | after_tail_cleanup_and_final_resize_fast_path_ms | after_tail_cleanup_and_final_resize_fast_path_share_pct |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 2.998 | 1.15 | 38.4 | 1.2 | 40.0 | 0.316 | 10.5 | 0.332 | 11.1 |
| both_on, comb_rect_dense 512 LOCAL | 2.927 | 1.156 | 39.5 | 1.17 | 40.0 | 0.278 | 9.5 | 0.323 | 11.0 |
| both_on, multi_comb_rect 512 LOCAL | 0.829 | 0.319 | 38.5 | 0.289 | 34.9 | 0.109 | 13.1 | 0.112 | 13.5 |

aggregate after sampled 기준 grouped subtotal은 아래와 같다.

| scomp_total_ms | run_discovery_coalescing_ms | run_discovery_coalescing_share_pct | contiguous_run_block_copy_compaction_ms | contiguous_run_block_copy_compaction_share_pct | scratch_reuse_and_allocation_avoidance_ms | scratch_reuse_and_allocation_avoidance_share_pct | tail_cleanup_and_final_resize_fast_path_ms | tail_cleanup_and_final_resize_fast_path_share_pct | next_pivot_after_stable_compaction_round |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 6.754 | 2.625 | 38.9 | 2.659 | 39.4 | 0.703 | 10.4 | 0.767 | 11.4 | contiguous run block copy compaction |

strict dominant는 없지만 largest residual은 `contiguous run block copy compaction`이다.

## in-place stable compaction volume counter 표

surviving session note에서 exact volume row가 남은 것은 dense both_on sampled after 하나뿐이다. 다른 두 case는 qualitative shift만 복구 가능했다.

| label | scomp_block_copied_handles | scomp_elementwise_emitted_handles | scomp_contiguous_middle_memmove_calls | scomp_single_middle_run_calls | scomp_suffix_only_calls | note |
| --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | None | None | None | None | None | Exact per-case volume row was not preserved in surviving progress15 artifacts; only qualitative shift is known. |
| both_on, comb_rect_dense 512 LOCAL | 1947857 | 41892 | 2540 | 2540 | 64 | None |
| both_on, multi_comb_rect 512 LOCAL | None | None | None | None | None | Exact per-case volume row was not preserved in surviving progress15 artifacts; only qualitative shift is known. |

특히 recovered dense both_on row에서 `scomp_block_copied_handles=1947857`, `scomp_elementwise_emitted_handles=41892`, `scomp_contiguous_middle_memmove_calls=2540`, `scomp_single_middle_run_calls=2540`, `scomp_suffix_only_calls=64`가 찍혔다. 이건 per-handle emit을 run 단위 block copy로 치환하는 fast path가 실제로 쓰였다는 강한 증거다.

## kept-vector umbrella 표와 watch churn 유지 표

broader measured residual exact row는 progress15 raw artifacts에 남아 있지 않아 progress14 carry-forward proxy를 함께 싣는다.

| label | watch_churn_ms_proxy_from_progress13 | watch_unregister_ms_proxy_from_progress13 | watch_register_ms_proxy_from_progress13 | watch_diff_build_ms_proxy_from_progress13 | publish_ms_proxy_from_progress13 | global_delete_dfs_ms_proxy_from_progress13 | query_incident_ms_proxy_from_progress13 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 1863.212 | 615.157 | 274.625 | 973.429 | 318.532 | 102.429 | 1.954 |
| both_on, comb_rect_dense 512 LOCAL | 1880.921 | 637.656 | 290.92 | 952.345 | 341.688 | 106.691 | 1.986 |
| both_on, multi_comb_rect 512 LOCAL | 436.572 | 131.123 | 67.726 | 237.722 | 126.687 | 29.722 | 1.83 |

proxy 기준 broader residual에서는 `watch churn`이 여전히 더 크다. 그러나 이번 라운드의 pivot 규칙은 stable-compaction bucket 내부다.

## dense 1024 release와 representative 상태

| tag | elapsed_solver_s | rc | timed_out | validator_ok | status | note |
| --- | --- | --- | --- | --- | --- | --- |
| after_both_on_dense_1024_release | None | -9 | True | False | None | Primary release attempt timed out. Compact diag rerun had been started later, but final artifact was not preserved. |
| after_both_on_multi_1024_release | None | None | None | None | pending_or_unrecovered | None |
| after_both_on_dense_4096_release | None | None | None | None | pending_or_unrecovered | None |
| after_both_on_multi_4096_release | None | None | None | None | pending_or_unrecovered | None |

현재 surviving evidence상 primary dense 1024 release는 timeout row까지만 확인 가능하고, compact diag rerun과 remaining release or representative rows는 unrecovered 상태다.

## top K slow deletion 요약

Exact per-deletion raw topK rows는 현재 filesystem에 남아 있지 않다. completed progress15 session note에 따르면, slow deletions의 residual pattern은 sparse remove build가 아니라 kept-run copy와 run partition 구조 쪽으로 이동했다.

dense 두 케이스는 `single contiguous middle run`이나 `few kept runs` 패턴에서 block-copy fast path가 자주 잡혔지만, remaining slow rows는 여전히 `contiguous run block copy`와 `run discovery` 사이의 경계에서 남았다. multi case는 block-copy와 run-discovery 비중이 더 비슷했다.

## 최종 residual cost 판정

문제 설정상 이번 round는 stable-compaction bucket 내부 next pivot 판정이 목표다. 그 기준에서 after sampled grouped aggregate는 아래와 같다.

| category | aggregate_ms |
| --- | --- |
| run discovery coalescing | 2.625 |
| contiguous run block copy compaction | 2.659 |
| scratch reuse and allocation avoidance | 0.703 |
| tail cleanup and final resize fast path | 0.767 |
| stronger watch compression (broader proxy from progress14 carry-forward) | 4180.705 |

`watch churn`의 broader proxy는 여전히 더 크다. 하지만 이번 라운드의 pivot 규칙은 stable-compaction bucket 내부 largest residual 판정이다. 그 기준에서 strict dominant는 없지만, `contiguous run block copy compaction`이 aggregate 39.4퍼센트로 가장 큰 residual이다.

## 마지막 결론

`next pivot after stable-compaction round: contiguous run block copy compaction`
