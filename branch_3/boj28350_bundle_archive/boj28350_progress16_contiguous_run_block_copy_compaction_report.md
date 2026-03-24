# BOJ28350 progress16 contiguous run block copy compaction report

## 결론

이번 라운드의 clean LOCAL sampled 기준으로는 `contiguous run block copy compaction` 버킷 내부에 strict dominant가 생겼다. after sampled aggregate에서 `run coalescing and copy-plan build`가 51.6퍼센트로 50퍼센트를 넘었고, `multi-run block copy core`는 39.7퍼센트, `single middle-run direct shift`는 2.7퍼센트, `elementwise fallback and overlap-safe staging`은 6.0퍼센트였다.

따라서 이번 라운드의 최종 pivot 판정은 `run coalescing and copy-plan build`다. broader measured residual로는 `watch churn`이 여전히 더 크지만, 이번 round의 문제 설정은 stable-compaction bucket 내부 next pivot 확정이었다.

## 무엇을 바꿨는가

`retainClassWatchByKeepMask` 안의 block-copy bucket을 더 세밀하게 쪼개고, 실제 copy core와 plan-build, direct memmove, fallback을 분리했다. 추가한 exclusive timer는 `time_bcopy_single_middle_run_detect_ns`, `time_bcopy_run_coalesce_build_ns`, `time_bcopy_direct_suffix_memmove_ns`, `time_bcopy_multi_run_block_copy_ns`, `time_bcopy_short_fragment_elementwise_fallback_ns`, `time_bcopy_overlap_safe_staging_ns`다. route partition timer와 `bcopy_*` volume counter도 같이 넣었다.

저위험 최적화는 세 가지였다. single contiguous middle run이면 suffix를 direct move 계열 fast path로 보내고, kept runs가 적으면 runwise block copy를 우선하며, elementwise fallback은 더 작은 fragment에만 남겼다. adjacent run coalescing과 overlap-safe direct path도 같이 계측했다.

## correctness gate

| case | validator | elapsed_s | local_active_mismatch | local_active_partition_mismatch | debug_touched_missing_classes | piece_materialize_fallback_calls | support_rebuild_fallback_calls | unanimous_baseline_path_calls |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 256 LOCAL, PROFILE_NONE, all opts on | OK | 6.854 | 0 | 0 | 0 | 0 | 0 | 0 |
| both_on, comb_rect_dense 256 LOCAL, PROFILE_NONE, all opts on | OK | 6.833 | 0 | 0 | 0 | 0 | 0 | 0 |
| both_on, multi_comb_rect 512 LOCAL, PROFILE_SAMPLED, all opts on | OK | 9.129 | 0 | 0 | 0 | 0 | 0 | 0 |

## scope correction 결과

`time_scomp_contiguous_run_block_copy_ns`는 actual runwise copy 또는 direct memmove core만 포함하게 정리했다. `time_scomp_elementwise_emit_ns`는 contiguous copy로 못 보낸 fallback emit만 담게 했고, `time_scomp_kept_run_partition_build_ns`는 removed run과 kept run partition build까지만 담게 했다. `time_scomp_scratch_prepare_ns`는 scratch 준비와 capacity decision만 담고, `time_scomp_tail_cleanup_ns`와 `time_scomp_final_resize_swap_ns`는 copy 이후 finalization만 담게 했다.

그 결과 after sampled aggregate에서 strict dominant가 생겼다. progress15의 묶음에서는 block-copy bucket 내부 largest residual 정도였지만, progress16의 clean before and after와 tighter scope에서는 `run coalescing and copy-plan build`가 51.6퍼센트로 50퍼센트를 넘겼다.

## 512 LOCAL before and after 주요 시간 분해

| case | phase | elapsed_s | run_discovery_ms | single_shift_ms | multi_run_core_ms | fallback_staging_ms | block_copy_total_ms |
| --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | before | 40.137 | 0.680 | 0.067 | 0.600 | 0.224 | 1.569 |
| connector_only, comb_rect_dense 512 LOCAL | after | 40.052 | 0.866 | 0.072 | 0.700 | 0.104 | 1.742 |
| both_on, comb_rect_dense 512 LOCAL | before | 41.304 | 0.688 | 0.025 | 0.654 | 0.214 | 1.581 |
| both_on, comb_rect_dense 512 LOCAL | after | 46.640 | 0.942 | 0.023 | 0.697 | 0.108 | 1.771 |
| both_on, multi_comb_rect 512 LOCAL | before | 9.743 | 0.166 | 0.017 | 0.162 | 0.036 | 0.381 |
| both_on, multi_comb_rect 512 LOCAL | after | 9.834 | 0.245 | 0.012 | 0.185 | 0.028 | 0.470 |

## contiguous run block copy exclusive subaxis aggregate

| category | aggregate_before_ms | aggregate_after_ms | after_share_pct |
| --- | --- | --- | --- |
| run discovery coalescing | 1.534 | 2.053 | 51.6 |
| single middle-run direct shift | 0.108 | 0.107 | 2.7 |
| multi-run block copy core | 1.416 | 1.583 | 39.7 |
| elementwise fallback and overlap-safe staging | 0.474 | 0.240 | 6.0 |

이 표가 이번 라운드의 핵심이다. after sampled aggregate 기준으로 `run discovery coalescing`이 51.6퍼센트라 strict dominant다.

## contiguous run block copy volume counter

| case | single_middle_run_calls | suffix_only_calls | copy_plan_entries | direct_memmove_calls | direct_memmoved_handles | runwise_block_copy_calls | runwise_block_copied_handles | elementwise_fallback_calls | elementwise_fallback_handles | prefix_skipped_handles | suffix_skipped_handles |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 20 | 92 | 167 | 10 | 482 | 62797 | 2141922 | 13376 | 15 | 2826293 | 1138 |
| both_on, comb_rect_dense 512 LOCAL | 0 | 64 | 276 | 0 | 0 | 59714 | 1977080 | 12669 | 32 | 2685635 | 1532 |
| both_on, multi_comb_rect 512 LOCAL | 148 | 38 | 70 | 74 | 5647 | 14809 | 424913 | 2443 | 0 | 699524 | 217 |

해석은 간단하다. per-handle emit은 이미 많이 줄었고, block copy도 충분히 많이 쓰고 있다. 그런데도 time이 남는 건 `copy-plan build`와 `run coalescing` 쪽이 아직 크기 때문이다. 특히 dense both_on에서는 `bcopy_copy_plan_entries=276`, `bcopy_runwise_block_copy_calls=59714`처럼 run 관리 오버헤드가 작지 않다.

## stable-compaction umbrella와 watch churn 유지 표

| case | stable_compaction_bucket_ms | block_copy_bucket_ms | watch_churn_ms | bcopy_route_connector_skeleton_ms | bcopy_route_delta_preserved_then_skeleton_ms |
| --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 2.564 | 1.742 | 1064.760 | 1.742 | 0.000 |
| both_on, comb_rect_dense 512 LOCAL | 2.575 | 1.771 | 1072.562 | 1.694 | 0.077 |
| both_on, multi_comb_rect 512 LOCAL | 0.766 | 0.470 | 272.991 | 0.438 | 0.032 |

route는 여전히 connector_skeleton이 지배적이다. 그래서 이번 round의 copy-plan 최적화도 주로 connector_skeleton route에 효과를 내야 한다.

## top K slow deletion 요약

slow deletion raw row에는 `bcopy_*` counter가 붙어 있다. 다만 `bcopy_total_ns`는 현재도 0으로 남아서 per-deletion timed subtotal로는 못 쓴다. 그래서 아래는 qualitative summary로 본다.

| sampled_case | topk_rows | avg_removed_run_count | avg_kept_run_count | avg_copy_plan_entries | avg_direct_memmove_calls | avg_direct_memmoved_handles | avg_block_copied_handles | avg_elementwise_fallback_handles |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| after_connector_only_dense_512_sampled | 10 | 251.6 | 331.3 | 196.2 | 30.9 | 3248.0 | 8070.1 | 27.0 |
| after_both_on_dense_512_sampled | 10 | 225.5 | 283.2 | 169.7 | 21.1 | 1809.5 | 6820.7 | 20.2 |
| after_both_on_multi_512_sampled | 10 | 232.6 | 279.5 | 186.5 | 9.9 | 815.5 | 7608.9 | 17.9 |

top K는 direct memmove miss보다 `removed run`과 `copy-plan entries`가 계속 크다는 점을 보여 준다. block copied handles는 큰데 elementwise fallback handles는 매우 작다. 그래서 다음 pivot을 `elementwise fallback`이 아니라 `run coalescing and copy-plan build`로 잡는 것이 맞다.

## 1024 RELEASE와 4096 representative

| case | elapsed_s | rc | timed_out | validator_ok |
| --- | --- | --- | --- | --- |
| both_on, comb_rect_dense 1024 RELEASE, BLOCK_COPY_OPT=1 | 373.384 | 0 | False | True |
| both_on, comb_rect_dense 1024 RELEASE repeat, BLOCK_COPY_OPT=1 | 382.796 | 0 | False | True |
| both_on, multi_comb_rect 1024 RELEASE, BLOCK_COPY_OPT=1 | 27.675 | 0 | False | True |
| both_on, comb_rect_dense 4096 RELEASE, BLOCK_COPY_OPT=1 | 248.678 | -9 | True | False |
| both_on, multi_comb_rect 4096 RELEASE, BLOCK_COPY_OPT=1 | 240.170 | -9 | True | False |

dense 1024는 두 번 모두 validator OK라 stable recovery는 유지된다. 다만 elapsed는 373.384초와 382.796초로 progress14의 약 327초대보다 느리다. 즉 axis는 더 잘 밝혔지만, 이번 라운드 최적화는 dense 1024 wall time 개선에는 아직 부족했다.

### dense 1024 repeat stability

| run1_elapsed_s | run2_elapsed_s | difference_pct | stable_recovery | timing_stability |
| --- | --- | --- | --- | --- |
| 373.384 | 382.796 | 2.49 | yes | good |

representative 4096은 둘 다 열리지 않았다. dense는 248.678초에서 timeout, multi도 240.170초에서 timeout이었다.

## 최종 residual cost 판정

이번 round의 문제 설정은 stable-compaction bucket 내부 next pivot 판정이다. 그 기준에서 after sampled grouped aggregate는 아래와 같다.

| category | aggregate_after_ms | share_pct |
| --- | --- | --- |
| run discovery coalescing | 2.053 | 51.6 |
| single middle-run direct shift | 0.107 | 2.7 |
| multi-run block copy core | 1.583 | 39.7 |
| elementwise fallback and overlap-safe staging | 0.240 | 6.0 |

broader measured residual로는 `watch churn`이 여전히 더 크다. 하지만 이번 라운드의 pivot 규칙은 block-copy bucket 내부 strict dominant 판정이다. 그 기준에서 `run discovery coalescing`이 aggregate 51.6퍼센트로 50퍼센트를 넘는다.

## 마지막 결론

`next pivot after block-copy round: run coalescing and copy-plan build`