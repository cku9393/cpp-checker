# BOJ 28350 progress13 retain compaction reduction report

## 결론

이번 라운드에서 `retainClassWatchByKeepMask` 내부를 exclusive하게 다시 분해해 보니 strict dominant가 생겼다. 세 sampled case 모두에서 `kept vector compaction reduction`이 retain bucket의 절반을 크게 넘었고, 실제 비중은 약 87 퍼센트에서 91 퍼센트 수준이었다.

반면 `remove bitmap and sparse remove list build`는 retain bucket의 소수 비중에 그쳤고, `moved-entry patch reduction`도 그보다 작았다. 따라서 progress12에서 watch bucket 안의 next pivot으로 잡았던 `retain compaction reduction`을 이번 round에서 다시 쪼갠 결과, 실제 1순위 최적화 타깃은 `kept handle vector rebuild and handleidx fixup`으로 확정된다.

마지막 결론 한 줄은 이거다.

`next pivot after retain round: kept vector compaction reduction`

## provenance

progress13은 `boj28350_literature_progress12_reuse_watch_handle_scan_reduction.cpp`를 베이스로 재구성하고, retain compaction exclusive timer와 sparse remove fast path, moved-entry owner lookup cache, same-owner fast path, patch skip을 추가한 뒤 새로 실행했다. 케이스 생성은 seed 1, shuffle labels 1, shuffle queries 1로 통일했다.

## 무엇을 바꿨는가

1. `time_reuse_watch_retain_ns`를 caller 측 keepmask build와 post-retain annotate에서 분리해 `retainClassWatchByKeepMask` core work만 감싸도록 scope를 좁혔다.
2. retain bucket을 `time_retain_remove_bitmap_build_ns`, `time_retain_sparse_remove_list_build_ns`, `time_retain_watchByVertex_pop_ns`, `time_retain_moved_entry_owner_lookup_ns`, `time_retain_moved_entry_same_owner_fastpath_ns`, `time_retain_moved_entry_slotpos_patch_ns`, `time_retain_kept_vector_build_ns`, `time_retain_kept_handle_copy_ns`, `time_retain_kept_handleidx_patch_ns`, `time_retain_final_swap_state_update_ns`로 다시 분해했다.
3. route에서 removed handle index가 이미 sparse하게 알려진 경우 `retainClassWatchByKeepMask`에 sparse remove list fast path를 연결했다.
4. moved entry가 same owner and same cid인 경우 owner state map lookup을 우회하도록 fast path를 넣었다.
5. moved entry owner lookup에 small cache를 두어 repeated `(owner,cid)` lookup을 줄였다.
6. kept handle patch에서 `newIdx`, `owner`, `cid`가 이미 맞는 경우 handleidx patch를 skip하게 정리했다.
7. slow deletion summary에 retain removed handles, sparse removed entries, moved entry count, owner lookup calls and misses, slotpos fixups, kept handles copied, handleidx fixups, retain total ns를 추가했다.

## `time_reuse_watch_retain_ns` scope correction 여부와 결과

이번 라운드에서는 `time_reuse_watch_retain_ns`를 retain core 전용 umbrella로 다시 잡았다. after sampled 기준으로 new retain subaxis 합과 umbrella gap은 모두 미세한 수준이라, 이 timer가 keepmask build나 post-retain annotate를 실질적으로 먹지 않는다는 것을 확인했다.

| label | retain_total_ms | sum_new_subaxis_ms | gap_ms |
| --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 212.869 | 212.869 | 0.0 |
| both_on, comb_rect_dense 512 LOCAL | 215.272 | 215.272 | 0.0 |
| both_on, multi_comb_rect 512 LOCAL | 56.187 | 56.187 | 0.0 |

## correctness gate

| tag | validator_ok | elapsed_solver_s | local_active_mismatch | local_active_partition_mismatch | debug_touched_missing_classes | piece_materialize_fallback_calls | support_rebuild_fallback_calls | unanimous_baseline_path_calls |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| gate_connector_only_dense_256_after | True | 6.409 | 0 | 0 | 0 | 0 | 0 | 0 |
| gate_both_on_dense_256_after | True | 6.749 | 0 | 0 | 0 | 0 | 0 | 0 |
| gate_both_on_multi_512_after | True | 10.044 | 0 | 0 | 0 | 0 | 0 | 0 |

세 gate는 모두 `validator OK`, mismatch 0, fallback 0을 유지했다.

## 512 LOCAL before after 주요 시간 분해 표

| label | before_base_elapsed_s | after_base_elapsed_s | before_sampled_elapsed_s | after_sampled_elapsed_s | before_retain_ms | after_retain_ms | before_keepmask_ms | after_keepmask_ms | before_full_scan_calls | after_full_scan_calls | before_full_scan_handles | after_full_scan_handles | before_sparse_removed_entries | after_sparse_removed_entries |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 43.811 | 44.291 | 41.331 | 41.756 | 253.708 | 212.869 | 291.736 | 306.505 | 20948 | 20948 | 4979394 | 4979394 | 0 | 163708 |
| both_on, comb_rect_dense 512 LOCAL | 42.029 | 43.69 | 44.232 | 41.923 | 198.284 | 215.272 | 300.454 | 296.781 | 21845 | 21845 | 5210828 | 5210828 | 0 | 304010 |
| both_on, multi_comb_rect 512 LOCAL | None | None | 9.236 | 9.942 | 50.749 | 56.187 | 39.911 | 31.53 | 7029 | 7029 | 1287048 | 1287048 | 0 | 73207 |

이 표에서 보이듯 sparse remove fast path는 실제로 켜졌지만 wall time을 일관되게 내리지는 못했다. dense connector_only sampled는 소폭 개선됐고, dense both_on과 multi sampled는 오히려 약간 느려졌다. 따라서 이번 라운드의 의미는 “retain 내부 dominant 축의 확정”에 더 가깝고, low-risk optimization 자체의 효과는 제한적이었다.

## retain compaction exclusive subaxis 표

| label | retain_total_ms | remove_build_ms | remove_build_share_pct | moved_patch_ms | moved_patch_share_pct | kept_vector_ms | kept_vector_share_pct | other_ms | other_share_pct |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 212.869 | 1.151 | 0.5 | 25.217 | 11.8 | 186.501 | 87.6 | 0.0 | 0.0 |
| both_on, comb_rect_dense 512 LOCAL | 215.272 | 2.889 | 1.3 | 24.066 | 11.2 | 188.317 | 87.5 | 0.0 | 0.0 |
| both_on, multi_comb_rect 512 LOCAL | 56.187 | 0.707 | 1.3 | 4.609 | 8.2 | 50.872 | 90.5 | 0.0 | 0.0 |

세 sampled case 모두에서 `kept vector compaction reduction`이 50퍼센트를 크게 넘었다. 실제 share는 connector_only dense 512에서 87.4퍼센트, both_on dense 512에서 87.3퍼센트, both_on multi 512에서 90.5퍼센트다.

세부 subaxis는 아래와 같다.

| label | remove_bitmap_build_ms | sparse_remove_list_build_ms | watchByVertex_pop_ms | moved_entry_owner_lookup_ms | moved_entry_same_owner_fastpath_ms | moved_entry_slotpos_patch_ms | kept_vector_build_ms | kept_handle_copy_ms | kept_handleidx_patch_ms | final_swap_state_update_ms |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 0.0 | 1.151 | 1.216 | 5.985 | 0.0 | 18.017 | 142.118 | 30.196 | 14.073 | 0.114 |
| both_on, comb_rect_dense 512 LOCAL | 0.0 | 2.889 | 2.261 | 8.556 | 0.0 | 13.249 | 147.078 | 28.1 | 13.045 | 0.094 |
| both_on, multi_comb_rect 512 LOCAL | 0.0 | 0.707 | 0.508 | 1.762 | 0.0 | 2.339 | 38.385 | 9.09 | 3.359 | 0.038 |

## retain compaction volume counter 표

| label | retain_calls | retain_watch_handles_before | retain_watch_handles_after | retain_removed_handles | retain_removed_sparse_calls | retain_removed_sparse_entries | retain_removed_dense_calls | retain_remove_bitmap_entries | retain_watchByVertex_pop_calls | retain_moved_entry_count | retain_moved_entry_same_owner_fastpath_hits | retain_owner_lookup_calls | retain_owner_lookup_hits | retain_owner_lookup_misses | retain_slotpos_fixups | retain_kept_handles_copied | retain_handleidx_fixups | retain_noop_calls | retain_sparse_remove_fastpath_calls | retain_skip_handleidx_patch_calls | retain_skip_slotpos_patch_calls |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 41896 | 9957891 | 9794183 | 195 | 21609 | 163708 | 20287 | 4812592 | 163708 | 183 | 0 | 183 | 155728 | 0 | 183 | 14762 | 6885 | 20287 | 21609 | 2826293 | 0 |
| both_on, comb_rect_dense 512 LOCAL | 42793 | 10208288 | 9904278 | 409 | 20948 | 304010 | 21845 | 5228894 | 304010 | 403 | 0 | 403 | 296956 | 0 | 403 | 16119 | 10271 | 21845 | 20948 | 2685635 | 0 |
| both_on, multi_comb_rect 512 LOCAL | 13593 | 2487447 | 2414240 | 1169 | 6564 | 73207 | 7029 | 1287360 | 73207 | 1125 | 0 | 1125 | 70972 | 0 | 1125 | 20199 | 9582 | 7029 | 6564 | 699524 | 0 |

이 counter를 보면 sparse remove fast path는 실제로 사용됐지만, retained kept handles를 다시 rebuild하고 patch하는 양 자체가 너무 커서 total retain time의 중심축은 그대로 `kept vector compaction reduction`에 남아 있다.

## reuse apply route 표와 watch churn 유지 표

| label | baseline_ms | baseline_share_pct | delta_preserved_then_skeleton_ms | delta_preserved_then_skeleton_share_pct | connector_skeleton_ms | connector_skeleton_share_pct | general_delta_ms | general_delta_share_pct | route_total_ms |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 0.0 | 0.0 | 0.0 | 0.0 | 2136.382 | 100.0 | 0.0 | 0.0 | 2136.382 |
| both_on, comb_rect_dense 512 LOCAL | 0.0 | 0.0 | 219.093 | 9.9 | 1995.188 | 90.1 | 0.0 | 0.0 | 2214.281 |
| both_on, multi_comb_rect 512 LOCAL | 0.0 | 0.0 | 47.719 | 9.9 | 432.076 | 90.1 | 0.0 | 0.0 | 479.795 |

| label | watch_churn_ms | watch_unregister_ms | watch_register_ms | watch_diff_build_ms | publish_ms | split_hit_localization_ms | connector_core_ms | global_delete_dfs_ms | query_incident_ms |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 1863.212 | 615.157 | 274.625 | 973.429 | 318.532 | 0.0 | 121.846 | 102.429 | 1.954 |
| both_on, comb_rect_dense 512 LOCAL | 1880.921 | 637.656 | 290.92 | 952.345 | 341.688 | 0.0 | 105.834 | 106.691 | 1.986 |
| both_on, multi_comb_rect 512 LOCAL | 436.572 | 131.123 | 67.726 | 237.722 | 126.687 | 0.0 | 64.82 | 29.722 | 1.83 |

route shape는 여전히 connector_skeleton 중심이다. broader measured residual로는 `watch churn`이 retain bucket보다 더 크게 남아 있지만, 이번 round의 문제 설정은 retain bucket 내부 next pivot 판정이므로 final next pivot은 `kept vector compaction reduction`으로 두는 것이 맞다.

## top K slow deletion 요약

| case | deletion_index | deleted_vertex | route | total_ns | retain_removed_handles | retain_sparse_removed_entries | retain_moved_entry_count | retain_owner_lookup_calls | retain_owner_lookup_misses | retain_slotpos_fixups | retain_kept_handles_copied | retain_handleidx_fixups | retain_total_ns | wscan_preserved_handles_scanned | wscan_connector_handles_scanned | wscan_existing_connector_set_handles_scanned | wscan_duplicate_full_scan_passes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| both_on, comb_rect_dense 512 LOCAL | 291 | 18 | none | 491370149 | 9275 | 9275 | 9238 | 9238 | 0 | 9238 | 9109 | 5630 | 0 | 31388 | 21765 | 12562 | 72 |
| connector_only, comb_rect_dense 512 LOCAL | 295 | 303 | none | 457177780 | 518 | 518 | 498 | 498 | 0 | 498 | 17712 | 10045 | 0 | 18230 | 12743 | 12225 | 0 |
| connector_only, comb_rect_dense 512 LOCAL | 293 | 25 | none | 427928478 | 101 | 101 | 95 | 95 | 0 | 95 | 18246 | 11002 | 0 | 18161 | 12542 | 12459 | 0 |
| both_on, comb_rect_dense 512 LOCAL | 299 | 279 | none | 425472553 | 217 | 217 | 208 | 208 | 0 | 208 | 17946 | 6622 | 0 | 18163 | 12724 | 12507 | 0 |
| connector_only, comb_rect_dense 512 LOCAL | 319 | 316 | none | 415189410 | 386 | 386 | 351 | 351 | 0 | 351 | 14304 | 6865 | 0 | 14690 | 10014 | 9628 | 0 |
| both_on, comb_rect_dense 512 LOCAL | 287 | 8 | none | 401541959 | 4286 | 4286 | 4238 | 4238 | 0 | 4238 | 14146 | 10071 | 0 | 24347 | 16550 | 12295 | 31 |
| connector_only, comb_rect_dense 512 LOCAL | 305 | 409 | none | 398297892 | 217 | 217 | 207 | 207 | 0 | 207 | 16185 | 2723 | 0 | 16402 | 11379 | 11162 | 0 |
| both_on, comb_rect_dense 512 LOCAL | 293 | 25 | none | 397811999 | 2255 | 2255 | 2252 | 2252 | 0 | 2252 | 15906 | 10038 | 0 | 21436 | 14716 | 12479 | 18 |
| both_on, comb_rect_dense 512 LOCAL | 321 | 321 | none | 396642485 | 320 | 320 | 309 | 309 | 0 | 309 | 13235 | 2969 | 0 | 13555 | 9182 | 8862 | 0 |
| connector_only, comb_rect_dense 512 LOCAL | 303 | 75 | none | 388641414 | 196 | 196 | 191 | 191 | 0 | 191 | 16332 | 8985 | 0 | 16528 | 11482 | 11288 | 0 |

topK residual은 duplicate full scan이 아니라 retain kept-vector rebuild와 handleidx or slotpos patch 쪽으로 이동해 있다. 특히 `retain_kept_handles_copied`, `retain_handleidx_fixups`, `retain_slotpos_fixups`가 큰 deletion이 상위를 차지했다.

## release와 representative

| tag | elapsed_solver_s | rc | timed_out | validator_ok | stdout_nonempty | stderr_nonempty |
| --- | --- | --- | --- | --- | --- | --- |
| after_both_on_dense_1024_release | 370.222 | 0 | False | True | True | False |
| after_both_on_multi_1024_release | 32.651 | 0 | False | True | True | False |
| after_both_on_dense_4096_release | 199.598 | 1 | False | False | False | False |
| after_both_on_multi_4096_release | 300.233 | 124 | True | False | False | False |

`both_on, comb_rect_dense 1024 RELEASE`는 progress12에서 이미 validator OK로 회수됐는데, 이번 progress13에서도 그 안정성을 다시 확인하는 것이 핵심이다. representative 4096은 여전히 탐색 단계다.

## 최종 residual cost 판정

| category | aggregate_ms |
| --- | --- |
| retain sparse removal fast path | 4.746 |
| moved-entry patch reduction | 53.892 |
| kept vector compaction reduction | 425.69 |
| stronger watch compression | 4180.704 |
| publish path compaction | 786.907 |
| split hit localization and indexing | 0.0 |
| connector skeleton build core | 292.5 |
| global_delete_dfs | 238.843 |
| query incident scans | 5.77 |

broader measured residual로는 `stronger watch compression`에 대응하는 `watch churn` 쪽이 여전히 더 크다. 하지만 이번 라운드의 프롬프트 기준은 retain bucket 내부 next pivot 판정이므로, 다음 우선순위는 broader residual이 아니라 retain exclusive 내부 largest residual을 따라가는 것이 맞다.

## 마지막 결론

`next pivot after retain round: kept vector compaction reduction`