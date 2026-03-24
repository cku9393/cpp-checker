# BOJ 28350 progress14 kept vector compaction reduction report

## 결론

이번 라운드에서 `kept vector compaction reduction` 버킷을 다시 쪼개 보니 strict dominant가 생겼다. 세 sampled case 모두에서 실제 1순위 최적화 타깃은 `stable kept vector materialization and copy`였다.

after sampled 기준 grouped exclusive share는 dense connector_only에서 68.2퍼센트, dense both_on에서 65.3퍼센트, multi both_on에서 66.5퍼센트였다. `changed-only handleidx patch`는 두 번째 축으로 남았지만 31퍼센트에서 33.8퍼센트 수준이었고, `prefix or suffix fast path and scratch reuse`는 1퍼센트 안팎에 그쳤다.

실제 wall time도 줄었다. `both_on, comb_rect_dense 1024 RELEASE`는 progress13의 370.222초에서 progress14의 327.39초로 개선됐고, 같은 조건 repeat run은 328.72초로 차이가 0.41퍼센트여서 stable recovery와 timing stability good로 볼 수 있다. `both_on, multi_comb_rect 1024 RELEASE`는 32.651초에서 15.22초로 더 크게 내려갔다.

broader measured residual로는 `watch churn`이 여전히 더 크지만, 이번 라운드의 문제 설정은 retain bucket 내부 next pivot 판정이다. 그 기준에서 마지막 결론 한 줄은 이거다.

`next pivot after kept-vector round: in-place stable compaction`

## provenance

이번 report와 merged json은 completed progress14 session notes, surviving progress13 report and json, 그리고 current progress14 source를 합쳐 재구성했다. progress14 raw per-run directories 자체는 현재 filesystem에 남아 있지 않아서, before sampled의 detailed bucket rows와 top K per-deletion raw rows, broader after residual exact rows 중 일부는 복구된 session note나 explicit proxy baseline으로 패키징했다. proxy를 쓴 곳은 표와 json에 모두 명시했다.

현재 source에 대해서는 `g++ -std=gnu++17 -O2 -fsyntax-only` check를 다시 돌렸고 통과했다.

## 무엇을 바꿨는가

1. `retainClassWatchByKeepMask` 안의 kept-vector 구간을 `time_kvec_prefix_fastpath_check_ns`, `time_kvec_suffix_fastpath_check_ns`, `time_kvec_kept_count_scan_ns`, `time_kvec_scratch_prepare_ns`, `time_kvec_stable_emit_unchanged_prefix_ns`, `time_kvec_stable_emit_moved_suffix_ns`, `time_kvec_patchlist_build_ns`, `time_kvec_handleidx_patch_changed_only_ns`, `time_kvec_handleidx_patch_skip_same_index_ns`, `time_kvec_final_resize_or_swap_ns`로 다시 분해했다.
2. coarse umbrella인 `time_retain_kept_vector_build_ns`, `time_retain_kept_handle_copy_ns`, `time_retain_kept_handleidx_patch_ns`, `time_retain_final_swap_state_update_ns`의 scope를 다시 정리했다. build는 kept-region detection과 emit 준비까지만, copy는 실제 kept handle write or move까지만, patch는 changed-only patch 판단과 실제 patch까지만, final swap은 resize or swap and final state update까지만 포함하게 했다.
3. stable order를 유지하는 범위에서 in-place stable compaction fast path를 넣었다.
4. first removed index 이전의 unchanged prefix는 copy와 patch를 건너뛰게 정리했다.
5. removed suffix only 상황에는 resize fast path를 연결했다.
6. compaction 중 `(oldIdx,newIdx)` changed patchlist를 만들고 handleidx patch는 changed entries 위주로 줄였다.
7. function-scope scratch buffer and capacity reuse를 사용해 unnecessary scratch churn을 줄이도록 정리했다.
8. slow deletion summary에 kept-vector 관련 qualitative summary를 남길 수 있게 패키징 포맷을 정리했다.

## `time_retain_kept_vector_build_ns`, `time_retain_kept_handle_copy_ns`, `time_retain_kept_handleidx_patch_ns`, `time_retain_final_swap_state_update_ns` scope correction 여부와 결과

이번 라운드에서는 coarse timer 네 개를 다시 정의해 서로의 책임 범위를 겹치지 않게 정리했다. raw progress14 per-timer umbrella-gap rows는 남아 있지 않지만, surviving session note에서 recovered grouped exclusive subtotal이 kept-vector bucket total과 정확히 일치한다. 따라서 packed report에서는 kept-vector bucket total과 grouped exclusive subtotal의 gap을 0으로 기록한다.

| label | kvec_total_ms | sum_grouped_exclusive_ms | gap_ms |
| --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 48.29 | 48.29 | 0 |
| both_on, comb_rect_dense 512 LOCAL | 40.836 | 40.836 | 0 |
| both_on, multi_comb_rect 512 LOCAL | 11.121 | 11.121 | 0 |

주의. 여기의 gap 0은 recovered grouped subtotals 기준 결과다. finer ten-subaxis raw rows는 surviving artifacts에 남지 않아, exact ten-way timer table 대신 grouped exclusive category table을 보고한다.

## correctness gate

| tag | validator_ok | elapsed_solver_s | local_active_mismatch | local_active_partition_mismatch | debug_touched_missing_classes | piece_materialize_fallback_calls | support_rebuild_fallback_calls | unanimous_baseline_path_calls |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| gate_connector_only_dense_256_after | True | None | 0 | 0 | 0 | 0 | 0 | 0 |
| gate_both_on_dense_256_after | True | None | 0 | 0 | 0 | 0 | 0 | 0 |
| gate_both_on_multi_512_after | True | None | 0 | 0 | 0 | 0 | 0 | 0 |

세 gate 모두 `validator OK`, mismatch 0, fallback 0을 유지했다. exact gate elapsed는 surviving raw artifacts에 남아 있지 않아 `None`으로 둔다.

## 실행 상태 요약

| tag | solver | toggle | mode | n | profile_mode | kept_vector_opt | elapsed_solver_s | rc | timed_out | validator_ok |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| gate_connector_only_dense_256_after | LOCAL | connector_only | comb_rect_dense | 256 | PROFILE_NONE | 1 | None | 0 | False | True |
| gate_both_on_dense_256_after | LOCAL | both_on | comb_rect_dense | 256 | PROFILE_NONE | 1 | None | 0 | False | True |
| gate_both_on_multi_512_after | LOCAL | both_on | multi_comb_rect | 512 | PROFILE_SAMPLED | 1 | None | 0 | False | True |
| before_connector_only_dense_512_base | LOCAL | connector_only | comb_rect_dense | 512 | PROFILE_BASE | 0 | 19.67 | 0 | False | True |
| after_connector_only_dense_512_base | LOCAL | connector_only | comb_rect_dense | 512 | PROFILE_BASE | 1 | 19.03 | 0 | False | True |
| before_both_on_dense_512_base | LOCAL | both_on | comb_rect_dense | 512 | PROFILE_BASE | 0 | 21.1 | 0 | False | True |
| after_both_on_dense_512_base | LOCAL | both_on | comb_rect_dense | 512 | PROFILE_BASE | 1 | 19.63 | 0 | False | True |
| before_connector_only_dense_512_sampled | LOCAL | connector_only | comb_rect_dense | 512 | PROFILE_SAMPLED | 0 | 20.21 | 0 | False | True |
| after_connector_only_dense_512_sampled | LOCAL | connector_only | comb_rect_dense | 512 | PROFILE_SAMPLED | 1 | 20.03 | 0 | False | True |
| before_both_on_dense_512_sampled | LOCAL | both_on | comb_rect_dense | 512 | PROFILE_SAMPLED | 0 | 22.51 | 0 | False | True |
| after_both_on_dense_512_sampled | LOCAL | both_on | comb_rect_dense | 512 | PROFILE_SAMPLED | 1 | 20.64 | 0 | False | True |
| before_both_on_multi_512_sampled | LOCAL | both_on | multi_comb_rect | 512 | PROFILE_SAMPLED | 0 | 3.94 | 0 | False | True |
| after_both_on_multi_512_sampled | LOCAL | both_on | multi_comb_rect | 512 | PROFILE_SAMPLED | 1 | 3.75 | 0 | False | True |
| after_both_on_dense_1024_release | RELEASE | both_on | comb_rect_dense | 1024 | PROFILE_BASE | 1 | 327.39 | 0 | False | True |
| after_both_on_dense_1024_release_repeat | RELEASE | both_on | comb_rect_dense | 1024 | PROFILE_BASE | 1 | 328.72 | 0 | False | True |
| after_both_on_multi_1024_release | RELEASE | both_on | multi_comb_rect | 1024 | PROFILE_BASE | 1 | 15.22 | 0 | False | True |
| after_both_on_dense_4096_release | RELEASE | both_on | comb_rect_dense | 4096 | PROFILE_BASE | 1 | 206.89 | 1 | False | False |
| after_both_on_multi_4096_release | RELEASE | both_on | multi_comb_rect | 4096 | PROFILE_BASE | 1 | None | 124 | True | False |

## `connector_only 512 LOCAL`, `both_on 512 LOCAL`, `both_on multi_comb_rect 512 LOCAL` before after 주요 시간 분해 표

before bucket 값은 progress13 final이 progress14 before의 semantic baseline이라는 점을 이용해 progress13 after rows를 explicit proxy로 사용했다. after 값은 completed progress14 session note에서 직접 recovered한 exact value다.

| label | before_base_elapsed_s | after_base_elapsed_s | before_sampled_elapsed_s | after_sampled_elapsed_s | before_kept_vector_ms_proxy_from_progress13 | after_kept_vector_ms | kept_vector_drop_pct | before_handle_copy_entries_proxy | after_handle_copy_entries | handle_copy_entries_drop_pct |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 19.67 | 19.03 | 20.21 | 20.03 | 186.501 | 48.29 | 74.1 | 4981591 | 2155298 | 56.7 |
| both_on, comb_rect_dense 512 LOCAL | 21.1 | 19.63 | 22.51 | 20.64 | 188.317 | 40.836 | 78.3 | 4675384 | 1989749 | 57.4 |
| both_on, multi_comb_rect 512 LOCAL | None | None | 3.94 | 3.75 | 50.872 | 11.121 | 78.1 | 1126880 | 427356 | 62.1 |

이 표에서 보이듯 dense two cases와 multi case 모두에서 kept-vector bucket total과 handle copy volume이 크게 줄었다. wall time 개선은 dense both_on sampled에서 22.51초에서 20.64초, multi sampled에서 3.94초에서 3.75초로 가장 선명했다.

## kept vector compaction exclusive subaxis 표

| label | after_kept_vector_ms | after_in_place_stable_compaction_ms | after_in_place_stable_compaction_share_pct | after_changed_only_handleidx_patch_ms | after_changed_only_handleidx_patch_share_pct | after_prefix_suffix_fastpath_and_scratch_reuse_ms | after_prefix_suffix_fastpath_and_scratch_reuse_share_pct |
| --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 48.29 | 32.955 | 68.2 | 14.948 | 31 | 0.387 | 0.8 |
| both_on, comb_rect_dense 512 LOCAL | 40.836 | 26.668 | 65.3 | 13.799 | 33.8 | 0.369 | 0.9 |
| both_on, multi_comb_rect 512 LOCAL | 11.121 | 7.399 | 66.5 | 3.601 | 32.4 | 0.121 | 1.1 |

aggregate after sampled 기준 grouped subtotal은 아래와 같다.

| kvec_total_ms | in_place_stable_compaction_ms | in_place_stable_compaction_share_pct | changed_only_handleidx_patch_ms | changed_only_handleidx_patch_share_pct | prefix_suffix_fastpath_and_scratch_reuse_ms | prefix_suffix_fastpath_and_scratch_reuse_share_pct | next_pivot_after_kept_vector_round |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 100.248 | 67.022 | 66.9 | 32.348 | 32.3 | 0.878 | 0.9 | in-place stable compaction |

세 sampled case 모두에서 `in-place stable compaction`이 50퍼센트를 크게 넘는다. 따라서 이번 라운드의 질문에 대한 답은 명확하다. 실제 1순위 최적화 타깃은 `stable kept vector materialization and copy`다.

## kept vector compaction volume counter 표

| label | before_handle_copy_entries_proxy | after_handle_copy_entries | handle_copy_entries_drop_pct | after_inplace_compact_calls | after_suffix_resize_fastpath_calls |
| --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 4981591 | 2155298 | 56.7 | 21517 | 92 |
| both_on, comb_rect_dense 512 LOCAL | 4675384 | 1989749 | 57.4 | 20884 | 64 |
| both_on, multi_comb_rect 512 LOCAL | 1126880 | 427356 | 62.1 | 6526 | 38 |

핵심은 `kvec_handle_copy_entries` 감소다. dense connector_only는 4,981,591에서 2,155,298, dense both_on은 4,675,384에서 1,989,749, multi both_on은 1,126,880에서 427,356으로 줄었다. 이건 full kept copy가 changed region 중심으로 줄었다는 뜻이다.

## retain compaction umbrella 표와 watch churn 유지 표

progress14 session notes에는 kept-vector delta가 정확히 남아 있지만 broader retain umbrella와 broader watch churn exact rows는 그대로 패키징되지 않았다. 따라서 아래 표는 progress13 after를 explicit proxy로 carry forward한 것이다.

### retain compaction umbrella proxy

| label | retain_total_ms_proxy_from_progress13 | kept_vector_ms_proxy_from_progress13 | kept_vector_share_pct_proxy_from_progress13 |
| --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 212.869 | 186.501 | 87.6 |
| both_on, comb_rect_dense 512 LOCAL | 215.272 | 188.317 | 87.5 |
| both_on, multi_comb_rect 512 LOCAL | 56.187 | 50.872 | 90.5 |

### watch churn 유지 proxy

| label | watch_churn_ms_proxy_from_progress13 | watch_unregister_ms_proxy_from_progress13 | watch_register_ms_proxy_from_progress13 | watch_diff_build_ms_proxy_from_progress13 | publish_ms_proxy_from_progress13 |
| --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 1863.212 | 615.157 | 274.625 | 973.429 | 318.532 |
| both_on, comb_rect_dense 512 LOCAL | 1880.921 | 637.656 | 290.92 | 952.345 | 341.688 |
| both_on, multi_comb_rect 512 LOCAL | 436.572 | 131.123 | 67.726 | 237.722 | 126.687 |

broader measured residual로는 `watch churn`이 여전히 더 크다는 판단은 이 proxy table과 completed progress14 session note를 함께 봐도 유지된다. 다만 이번 라운드의 pivot 판정 기준은 retain bucket 내부다.

## dense 1024 release와 stability

| tag | elapsed_solver_s | rc | timed_out | validator_ok |
| --- | --- | --- | --- | --- |
| after_both_on_dense_1024_release | 327.39 | 0 | False | True |
| after_both_on_dense_1024_release_repeat | 328.72 | 0 | False | True |
| after_both_on_multi_1024_release | 15.22 | 0 | False | True |

| progress13_dense_1024_elapsed_s_reference | progress14_dense_1024_elapsed_s | progress14_dense_1024_repeat_elapsed_s | dense_1024_improvement_vs_progress13_pct | dense_1024_repeat_delta_pct | stable_recovery | timing_stability_good | progress13_multi_1024_elapsed_s_reference | progress14_multi_1024_elapsed_s | multi_1024_improvement_vs_progress13_pct |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 370.222 | 327.39 | 328.72 | 11.6 | 0.41 | True | True | 32.651 | 15.22 | 53.4 |

dense 1024는 두 번 모두 validator OK였고 elapsed 차이가 15퍼센트보다 훨씬 작은 0.41퍼센트였다. 따라서 stable recovery와 timing stability good로 적는 것이 맞다.

## representative 4096 두 케이스 결과

| tag | elapsed_solver_s | rc | timed_out | validator_ok | note |
| --- | --- | --- | --- | --- | --- |
| after_both_on_dense_4096_release | 206.89 | 1 | False | False | Representative dense 4096 still exits with rc=1 and no output. |
| after_both_on_multi_4096_release | None | 124 | True | False | Representative multi 4096 still times out; exact elapsed was not preserved because time.txt was empty in surviving artifacts. |

4096 representative는 아직 열리지 않았다. dense는 `rc=1` 무출력이고, multi는 timeout이다.

## top K slow deletion 요약

### connector_only, comb_rect_dense 512 LOCAL

Top slow deletions no longer centered on duplicate full scans or sparse remove build. Residual time clustered around kept handle copy volume and handleidx patch volume, especially when unchanged prefix was shorter and the moved suffix remained long.

### both_on, comb_rect_dense 512 LOCAL

Dense both_on showed the same shift. Connector-skeleton heavy slow deletions still spent most retain time on stable kept-vector materialization, with handleidx patch trailing as the second term.

### both_on, multi_comb_rect 512 LOCAL

The multi case improved markedly, but the surviving slow deletions still concentrated on kept-vector work rather than prefix-suffix checks or final swap.

raw per-deletion topK rows는 surviving artifacts에 남아 있지 않다. 그래서 이번 report에서는 completed session note에서 명시된 qualitative shift만 정리했다. 공통점은 duplicate full scan이나 sparse remove build가 아니라 kept-vector rebuild와 changed-only handleidx patch 쪽으로 residual이 이동했다는 점이다.

## 최종 residual cost 판정

문제 설정상 이번 round는 retain bucket 내부 next pivot 판정이 목표다. 그 기준에서 after sampled grouped aggregate는 아래와 같다.

| category | aggregate_ms |
| --- | --- |
| in-place stable compaction | 67.022 |
| changed-only handleidx patch | 32.348 |
| prefix-suffix fast path and scratch reuse | 0.878 |
| stronger watch compression (broader proxy) | 4180.705 |

broader measured residual로는 `watch churn`이 여전히 더 크다. 하지만 이번 라운드의 pivot 규칙은 kept-vector bucket 내부 strict dominant 판정이다. 그 기준에서 `in-place stable compaction`이 aggregate 66.9퍼센트로 분명한 1순위다.

## 마지막 결론

`next pivot after kept-vector round: in-place stable compaction`
