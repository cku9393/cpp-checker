# BOJ28350 progress17 run coalescing and copy-plan build report

## 결론

이번 라운드의 clean LOCAL sampled 기준으로 `run coalescing and copy-plan build` 버킷 내부 strict dominant는 `removed and kept run discovery fusion`이다. after sampled aggregate에서 discovery 계열은 6.386ms로 63.5퍼센트를 차지했고, `destination index and descriptor emit fusion`은 2.478ms로 24.6퍼센트, `small-plan inline buffer and scratch reuse`는 0.881ms로 8.8퍼센트, `adjacent-run coalescing and plan normalization`은 0.315ms로 3.1퍼센트였다.

따라서 이번 라운드의 next pivot은 `removed and kept run discovery fusion`이다. progress16에서 기대했던 `adjacent-run coalescing and plan normalization`은 실제로 거의 비용이 없었고, `plan_adjacent_merge_hits`도 sampled 3케이스 모두 0이었다.

## 무엇을 바꿨는가

`time_bcopy_run_coalesce_build_ns`를 generic umbrella로 남기고, 그 내부를 `time_plan_*` 계열의 exclusive subaxis로 분해했다. 추가된 핵심 timer는 `time_plan_first_removed_seek_ns`, `time_plan_removed_run_discovery_ns`, `time_plan_kept_run_discovery_ns`, `time_plan_adjacent_run_coalesce_ns`, `time_plan_single_middle_shortcircuit_eligibility_ns`, `time_plan_dst_index_accumulate_ns`, `time_plan_descriptor_emit_ns`, `time_plan_small_inline_buffer_prepare_ns`다. 또 `plan_*` volume counter와 route timer, slow deletion summary의 plan 필드를 추가했다.

저위험 최적화로는 removed-run discovery와 kept-run discovery의 one-pass화, single middle-run과 suffix-only case의 generic descriptor plan short-circuit, running write cursor 기반 dst index 계산, inline small buffer 경로 도입을 적용했다. semantics는 유지했고, gate 3개는 전부 validator OK였다.

## correctness gate

| case | validator | elapsed_s | local_active_mismatch | local_active_partition_mismatch | debug_touched_missing_classes | piece_materialize_fallback_calls | support_rebuild_fallback_calls | unanimous_baseline_path_calls |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 256 LOCAL, PROFILE_NONE, all opts on | OK | 6.200 | 0 | 0 | 0 | 0 | 0 | 0 |
| both_on, comb_rect_dense 256 LOCAL, PROFILE_NONE, all opts on | OK | 6.360 | 0 | 0 | 0 | 0 | 0 | 0 |
| both_on, multi_comb_rect 512 LOCAL, PROFILE_SAMPLED, all opts on | OK | 11.130 | 0 | 0 | 0 | 0 | 0 | 0 |

## umbrella와 subaxis scope correction 결과

`time_bcopy_run_coalesce_build_ns`는 이제 copy-plan build umbrella만 담당한다. `time_bcopy_direct_suffix_memmove_ns`, `time_bcopy_multi_run_block_copy_ns`, `time_bcopy_short_fragment_elementwise_fallback_ns`, `time_bcopy_overlap_safe_staging_ns`는 actual copy core와 fallback core만 포함한다. after sampled aggregate 기준으로 umbrella는 10.060ms였고, 새 `time_plan_*` subaxis 합과 일치하는 checksum 역할을 했다.

## 512 LOCAL before와 after 주요 시간 분해

| case | phase | elapsed_s | discovery_ms | adjacent_ms | dst_desc_ms | small_inline_ms | plan_total_ms |
| --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | before | 49.770 | 2.049 | 0.399 | 1.403 | 0.307 | 4.157 |
| connector_only, comb_rect_dense 512 LOCAL | after | 48.990 | 2.153 | 0.141 | 1.140 | 0.511 | 3.946 |
| both_on, comb_rect_dense 512 LOCAL | before | 51.910 | 3.260 | 0.350 | 1.240 | 0.308 | 5.158 |
| both_on, comb_rect_dense 512 LOCAL | after | 51.240 | 3.323 | 0.128 | 1.077 | 0.251 | 4.779 |
| both_on, multi_comb_rect 512 LOCAL | before | 11.070 | 4.889 | 0.121 | 0.290 | 0.098 | 5.398 |
| both_on, multi_comb_rect 512 LOCAL | after | 11.310 | 0.910 | 0.046 | 0.261 | 0.119 | 1.335 |

## run coalescing and copy-plan build exclusive subaxis aggregate

| category | aggregate_before_ms | aggregate_after_ms | after_share_pct |
| --- | --- | --- | --- |
| removed and kept run discovery fusion | 10.198 | 6.386 | 63.476 |
| adjacent-run coalescing and plan normalization | 0.869 | 0.315 | 3.133 |
| destination index and descriptor emit fusion | 2.933 | 2.478 | 24.632 |
| small-plan inline buffer and scratch reuse | 0.713 | 0.881 | 8.760 |

after sampled aggregate 기준으로 `removed and kept run discovery fusion`이 63.5퍼센트로 50퍼센트를 넘었으므로 strict dominant다.

## run coalescing and copy-plan build volume counter

| case | plan_removed_run_count_sum | plan_kept_run_count_sum | plan_adjacent_merge_hits | plan_descriptor_count | plan_dst_index_updates | plan_single_middle_shortcircuit_hits | plan_suffix_only_shortcircuit_hits | plan_small_inline_hits | plan_small_inline_capacity_reuse_hits | plan_heap_plan_build_calls |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 85297 | 97782 | 0 | 73100 | 73100 | 3073 | 92 | 21412 | 21512 | 105 |
| both_on, comb_rect_dense 512 LOCAL | 81869 | 93331 | 0 | 69843 | 69843 | 2540 | 64 | 20805 | 20880 | 79 |
| both_on, multi_comb_rect 512 LOCAL | 20114 | 23815 | 0 | 16538 | 16538 | 714 | 38 | 6524 | 6525 | 2 |

핵심 관찰은 두 가지다. 첫째, `plan_adjacent_merge_hits`는 sampled 3케이스 모두 0이었다. 둘째, `plan_descriptor_count`와 `plan_dst_index_updates`는 여전히 크지만, `plan_small_inline_hits`도 매우 많이 올라가 있다. 이 말은 heap plan build는 많이 줄였지만, run discovery 자체가 여전히 가장 큰 축이라는 뜻이다.

## block-copy umbrella와 watch churn 유지 표

| case | copy_plan_umbrella_ms | block_copy_bucket_ms | watch_churn_ms | plan_route_connector_skeleton_ms | plan_route_delta_preserved_then_skeleton_ms | plan_route_baseline_ms |
| --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 3.946 | 5.323 | 1415.766 | 3.946 | 0.000 | 0.000 |
| both_on, comb_rect_dense 512 LOCAL | 4.779 | 5.913 | 1564.543 | 3.439 | 1.339 | 0.000 |
| both_on, multi_comb_rect 512 LOCAL | 1.335 | 1.635 | 312.300 | 0.860 | 0.475 | 0.000 |

broader measured residual에서는 `watch churn`이 여전히 훨씬 크다. 하지만 이번 라운드의 목표는 block-copy bucket 내부 next pivot 판정이므로, 내부 pivot은 discovery fusion 쪽으로 잡는 것이 맞다.

## top K slow deletion 요약

| sampled_case | topk_rows_used | nonzero_plan_rows | avg_plan_first_removed_index | avg_plan_removed_run_count | avg_plan_kept_run_count | avg_plan_descriptor_count | avg_plan_dst_index_updates | avg_plan_adjacent_merge_hits | avg_plan_single_middle_shortcircuit_hits | avg_plan_small_inline_hits | avg_plan_total_ms |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 10 | 0 | 12947.100 | 352.300 | 419.400 | 272.000 | 272.000 | 0.000 | 27.600 | 119.700 | 0.000 |
| both_on, comb_rect_dense 512 LOCAL | 1 | 1 | 8181.000 | 527.000 | 597.000 | 473.000 | 473.000 | 0.000 | 2.000 | 122.000 | 0.263 |
| both_on, multi_comb_rect 512 LOCAL | 4 | 4 | 12988.250 | 244.750 | 308.250 | 189.500 | 189.500 | 0.000 | 22.250 | 96.500 | 0.152 |

slow deletion row 중 많은 행은 total deletion 기준 topK이지만 deep copy-plan 계측은 sampled deletion에만 들어가므로, 일부 케이스는 `nonzero_plan_rows`가 적다. 그래도 nonzero rows를 보면 `descriptor_count`와 `dst_index_updates`는 존재하지만 `adjacent_merge_hits`는 0이고, single-middle shortcircuit은 보조적이다. 이 역시 discovery 쪽이 주범이라는 결론과 맞는다.

## release와 representative

| case | rc | timed_out | validator_ok | elapsed_sec | maxrss_kb | stdout_empty | stderr_empty | compact_release_diag |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| both_on, comb_rect_dense 1024 RELEASE, plan opt on | 1 | False | False | 195.500 | 963864 | True | True | 0 |
| both_on, comb_rect_dense 1024 RELEASE repeat, plan opt on | 1 | False | False | 267.060 | 962088 | True | True | 0 |
| both_on, comb_rect_dense 1024 RELEASE compact diag rerun, plan opt on | 1 | False | False | 203.470 | 962036 | True | True | 1 |
| both_on, multi_comb_rect 1024 RELEASE, plan opt on | 0 | False | True | 35.000 | 271516 | False | True | 0 |
| both_on, comb_rect_dense 4096 RELEASE, plan opt on | 1 | False | False | 60.560 | 973936 | True | True | 0 |
| both_on, multi_comb_rect 4096 RELEASE, plan opt on | 1 | False | False | 105.360 | 971352 | True | True | 0 |

dense 1024 plain release 두 번은 모두 `rc=1`과 무출력이었고, compact release diag rerun도 `rc=1` 무출력으로 끝났다. `stderr.txt`에는 `[release_diag]` line이 남지 않았다. 따라서 progress17에서는 dense 1024 release를 더 안정적으로 회수하지 못했다.

### dense 1024 repeat stability

| run1_sec | run2_sec | elapsed_diff_pct | stable_recovery | timing_stability_good |
| --- | --- | --- | --- | --- |
| 195.500 | 267.060 | 30.941 | False | False |

representative 4096 두 케이스도 clean recovery에 실패했다. dense 4096은 `rc=1` 무출력, multi 4096도 `rc=1` 무출력이었다. time.txt 기준 wall time은 각각 약 60.56초와 105.36초였지만 validator 단계로 가지 못했다.

## 최종 residual cost 판정

문제 설정상 이번 round는 block-copy bucket 내부 next pivot 판정이 목표다. 그 기준에서 after sampled grouped aggregate는 아래와 같다.

| category | aggregate_ms |
| --- | --- |
| removed and kept run discovery fusion | 6.386 |
| adjacent-run coalescing and plan normalization | 0.315 |
| destination index and descriptor emit fusion | 2.478 |
| small-plan inline buffer and scratch reuse | 0.881 |
| watch churn broader proxy | 3292.609 |

broader measured residual로는 `watch churn`이 여전히 더 크다. 하지만 이번 라운드의 pivot 규칙은 copy-plan build bucket 내부 strict dominant 판정이다. 그 기준에서 `removed and kept run discovery fusion`이 aggregate 63.5퍼센트로 분명한 1순위다.

## 마지막 결론

`next pivot after copy-plan round: removed and kept run discovery fusion`