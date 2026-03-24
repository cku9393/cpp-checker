# BOJ 28350 progress10 reuse apply attribution report

## 결론

이번 라운드의 low risk optimization은 제대로 먹혔다. `reuse_duplicate_preserved_annotate_passes`와 `reuse_prepublish_preserved_annotate_calls`는 세 sampled case 모두에서 0으로 내려갔고, elapsed는 `connector_only dense 512`에서 44.83초→40.24초, `both_on dense 512`에서 46.20초→41.10초, `both_on multi 512`에서 5.39초→3.65초로 줄었다.

하지만 reuse apply 내부에서 질문한 네 축 중 어느 하나도 50퍼센트를 넘는 strict dominant로 올라오지 않았다. after sampled 기준 가장 큰 단일 잔여 축은 세 case 모두 `preserved piece split`이며, 핵심 하위 원인은 `time_reuse_old_attachment_map_build_ns`다. watch-handle scan은 두 번째다. 따라서 이번 라운드의 최종 pivot은 `preserved piece split`으로 잡는 것이 맞다.

## 무엇을 바꿨는가

`boj28350_literature_progress10_reuse_apply_attribution_final.cpp` 기준으로 다음을 반영했다.

1. reuse apply route timer와 call counter를 추가했다. `baseline`, `delta_preserved_then_skeleton`, `connector_skeleton`, `general_delta` 네 route를 따로 기록한다.
2. reuse apply subaxis timer를 추가했다. old attachment map build, piece split apply, keepmask scan, watch retain, direct retag, attachment fixup, patch tree build, prepublish annotate, final publish commit을 분리했다.
3. reuse volume counter와 slow deletion extended fields를 추가했다. reuse route, keepmask removed handles, direct retag handles, patch vertices, patch handles added, final publish noop or skipped 여부를 남긴다.
4. LOCAL summary 출력부에 reuse route line, reuse subaxis line, reuse volume line을 추가했다.
5. `ENABLE_REUSE_APPLY_OPT=1`일 때 duplicate preserved annotate pass를 제거했다. 구체적으로 connector skeleton rebuild path에서 prepublish preserved annotate를 생략하고 final publish 쪽 한 번으로 정리했다. progress9의 publish compaction은 유지했고 되돌리지 않았다.

## correctness gate

| tag | validator_ok | elapsed_s | local_active_mismatch | local_active_partition_mismatch | debug_touched_missing_classes | piece_materialize_fallback_calls | support_rebuild_fallback_calls | unanimous_baseline_path_calls |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| gate_connector_only_dense_256_after | True | 1.57 | 0 | 0 | 0 | 0 | 0 | 0 |
| gate_both_on_dense_256_after | True | 1.82 | 0 | 0 | 0 | 0 | 0 | 0 |
| gate_both_on_multi_512_after | True | 3.65 | 0 | 0 | 0 | 0 | 0 | 0 |

세 gate는 모두 통과했다.

## 실행 상태 요약

| tag | solver_kind | toggle | mode | n | profile_mode | reuse_opt | elapsed_solver_s | rc | timed_out | validator_ok |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| after_both_on_dense_1024_release | RELEASE | both_on | comb_rect_dense | 1024 | PROFILE_BASE | 1 | None | 124 | True | False |
| after_both_on_dense_4096_release | RELEASE | both_on | comb_rect_dense | 4096 | PROFILE_BASE | 1 | None | 1 | False | False |
| after_both_on_dense_512_base | LOCAL | both_on | comb_rect_dense | 512 | PROFILE_BASE | 1 | 40.42 | 0 | False | True |
| after_both_on_dense_512_sampled | LOCAL | both_on | comb_rect_dense | 512 | PROFILE_SAMPLED | 1 | 41.1 | 0 | False | True |
| after_both_on_multi_1024_release | RELEASE | both_on | multi_comb_rect | 1024 | PROFILE_BASE | 1 | 74.29 | 0 | False | True |
| after_both_on_multi_4096_release | RELEASE | both_on | multi_comb_rect | 4096 | PROFILE_BASE | 1 | None | 1 | False | False |
| after_both_on_multi_512_sampled | LOCAL | both_on | multi_comb_rect | 512 | PROFILE_SAMPLED | 1 | 3.65 | 0 | False | True |
| after_connector_only_dense_512_base | LOCAL | connector_only | comb_rect_dense | 512 | PROFILE_BASE | 1 | 40.05 | 0 | False | True |
| after_connector_only_dense_512_sampled | LOCAL | connector_only | comb_rect_dense | 512 | PROFILE_SAMPLED | 1 | 40.24 | 0 | False | True |
| before_both_on_dense_512_base | LOCAL | both_on | comb_rect_dense | 512 | PROFILE_BASE | 0 | 46.19 | 0 | False | True |
| before_both_on_dense_512_sampled | LOCAL | both_on | comb_rect_dense | 512 | PROFILE_SAMPLED | 0 | 46.2 | 0 | False | True |
| before_both_on_multi_512_sampled | LOCAL | both_on | multi_comb_rect | 512 | PROFILE_SAMPLED | 0 | 5.39 | 0 | False | True |
| before_connector_only_dense_512_base | LOCAL | connector_only | comb_rect_dense | 512 | PROFILE_BASE | 0 | 45.35 | 0 | False | True |
| before_connector_only_dense_512_sampled | LOCAL | connector_only | comb_rect_dense | 512 | PROFILE_SAMPLED | 0 | 44.83 | 0 | False | True |
| gate_both_on_dense_256_after | LOCAL | both_on | comb_rect_dense | 256 | PROFILE_NONE | 1 | 1.82 | 0 | False | True |
| gate_both_on_multi_512_after | LOCAL | both_on | multi_comb_rect | 512 | PROFILE_SAMPLED | 1 | 3.65 | 0 | False | True |
| gate_connector_only_dense_256_after | LOCAL | connector_only | comb_rect_dense | 256 | PROFILE_NONE | 1 | 1.57 | 0 | False | True |

dense 1024 release는 420초 timeout으로 끝까지 회수하지 못했다. `rc=1` 무출력은 아니었기 때문에 compact release diag rerun 조건은 충족하지 않았다. multi 1024 release는 validator OK로 회수됐다. representative 4096 두 케이스는 둘 다 `rc=1`, 무출력이었다.

## 512 LOCAL before after 주요 시간 분해

| case | before_elapsed_s | after_elapsed_s | elapsed_delta_pct | before_time_route_dispatch_s | after_time_route_dispatch_s | before_reuse_apply_total_s | after_reuse_apply_total_s | before_publish_group_s | after_publish_group_s | before_watch_diff_build_s | after_watch_diff_build_s | before_connector_skeleton_build_s | after_connector_skeleton_build_s | before_global_delete_dfs_s | after_global_delete_dfs_s | before_preserved_piece_split_s | after_preserved_piece_split_s | before_query_incident_scan_s | after_query_incident_scan_s | before_reuse_exclusive_total_s | after_reuse_exclusive_total_s | duplicate_preserved_annotate_before | duplicate_preserved_annotate_after |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only_comb_rect_dense_512 | 44.83 | 40.24 | -10.2 | 29.163 | 24.004 | 8.659 | 6.578 | 0.547 | 1.395 | 1.694 | 2.697 | 0.936 | 0.914 | 0.094 | 0.107 | 0.055 | 0.079 | 0.001 | 0.002 | 23.117 | 17.322 | 30887 | 0 |
| both_on_comb_rect_dense_512 | 46.2 | 41.1 | -11.0 | 30.22 | 24.837 | 9.343 | 7.291 | 0.74 | 1.553 | 1.754 | 2.893 | 0.997 | 0.898 | 0.091 | 0.093 | 0.036 | 0.052 | 0.001 | 0.002 | 24.314 | 18.577 | 30887 | 0 |
| both_on_multi_comb_rect_512 | 5.39 | 3.65 | -32.3 | 3.289 | 1.702 | 0.953 | 0.426 | 0.046 | 0.123 | 0.054 | 0.142 | 0.154 | 0.132 | 0.028 | 0.021 | 0.007 | 0.007 | 0.001 | 0.001 | 2.476 | 0.983 | 7581 | 0 |

핵심 변화는 duplicate preserved annotate 제거다. 그 결과 reuse exclusive total은 dense 두 case에서 약 24퍼센트, multi에서 약 60퍼센트 감소했다.

## reuse apply route 표 after

### connector_only_comb_rect_dense_512

| route | time_s | calls | share_pct |
| --- | --- | --- | --- |
| baseline | 0.0 | 0 | 0.0 |
| delta_preserved_then_skeleton | 0.0 | 0 | 0.0 |
| connector_skeleton | 6.512 | 30887 | 100.0 |
| general_delta | 0.0 | 0 | 0.0 |

### both_on_comb_rect_dense_512

| route | time_s | calls | share_pct |
| --- | --- | --- | --- |
| baseline | 0.0 | 0 | 0.0 |
| delta_preserved_then_skeleton | 1.476 | 1897 | 20.4 |
| connector_skeleton | 5.754 | 28990 | 79.6 |
| general_delta | 0.0 | 0 | 0.0 |

### both_on_multi_comb_rect_512

| route | time_s | calls | share_pct |
| --- | --- | --- | --- |
| baseline | 0.0 | 0 | 0.0 |
| delta_preserved_then_skeleton | 0.19 | 650 | 48.7 |
| connector_skeleton | 0.2 | 6931 | 51.3 |
| general_delta | 0.0 | 0 | 0.0 |

after 기준 route dominant는 세 case 모두 `connector_skeleton`이다. `both_on` 계열에서는 `delta_preserved_then_skeleton`도 남지만, dense 512에서는 약 20.4퍼센트, multi 512에서는 약 48.7퍼센트 수준이다.

## reuse apply exclusive subaxis 표 after

### connector_only_comb_rect_dense_512

| subaxis | time_s | share_pct |
| --- | --- | --- |
| old_attachment_map_build | 6.437 | 37.2 |
| piece_split_apply | 0.52 | 3.0 |
| connector_split_apply | 0.0 | 0.0 |
| keepmask_scan | 4.329 | 25.0 |
| watch_retain | 0.252 | 1.5 |
| preserved_direct_retag | 0.0 | 0.0 |
| connector_direct_retag | 2.58 | 14.9 |
| attachment_fixup | 0.585 | 3.4 |
| patch_vertex_collect | 0.0 | 0.0 |
| patch_tree_build | 0.534 | 3.1 |
| prepublish_preserved_annotate | 0.0 | 0.0 |
| prepublish_connector_annotate | 0.0 | 0.0 |
| final_publish_commit | 2.084 | 12.0 |

### both_on_comb_rect_dense_512

| subaxis | time_s | share_pct |
| --- | --- | --- |
| old_attachment_map_build | 6.47 | 34.8 |
| piece_split_apply | 0.519 | 2.8 |
| connector_split_apply | 0.0 | 0.0 |
| keepmask_scan | 4.912 | 26.4 |
| watch_retain | 0.315 | 1.7 |
| preserved_direct_retag | 0.002 | 0.0 |
| connector_direct_retag | 2.734 | 14.7 |
| attachment_fixup | 0.583 | 3.1 |
| patch_vertex_collect | 0.0 | 0.0 |
| patch_tree_build | 0.751 | 4.0 |
| prepublish_preserved_annotate | 0.0 | 0.0 |
| prepublish_connector_annotate | 0.0 | 0.0 |
| final_publish_commit | 2.291 | 12.3 |

### both_on_multi_comb_rect_512

| subaxis | time_s | share_pct |
| --- | --- | --- |
| old_attachment_map_build | 0.305 | 31.0 |
| piece_split_apply | 0.021 | 2.2 |
| connector_split_apply | 0.0 | 0.0 |
| keepmask_scan | 0.268 | 27.3 |
| watch_retain | 0.018 | 1.8 |
| preserved_direct_retag | 0.001 | 0.1 |
| connector_direct_retag | 0.139 | 14.1 |
| attachment_fixup | 0.028 | 2.9 |
| patch_vertex_collect | 0.0 | 0.0 |
| patch_tree_build | 0.058 | 5.9 |
| prepublish_preserved_annotate | 0.0 | 0.0 |
| prepublish_connector_annotate | 0.0 | 0.0 |
| final_publish_commit | 0.145 | 14.7 |

after sampled 세 case 모두에서 가장 큰 단일 subaxis는 `old_attachment_map_build`다. 이를 `piece_split_apply`와 합치면 `preserved piece split` 버킷이 33.2퍼센트에서 40.2퍼센트로 가장 크다. 그다음은 `keepmask_scan + watch_retain`으로 구성된 watch-handle scan이다. metadata retag or annotate, patch build, final publish commit은 모두 그 아래다.

## publish 계측 유지 표

### connector_only_comb_rect_dense_512

| metric | before | after |
| --- | --- | --- |
| time_dispatch_publish_preserved_annotate_ns | 0 | 791878496 |
| time_dispatch_publish_connector_annotate_ns | 213088694 | 176757521 |
| time_dispatch_publish_watch_id_rebuild_ns | 4207057 | 4234790 |
| time_dispatch_publish_canonical_rebuild_ns | 329979259 | 399037726 |
| time_dispatch_publish_posmap_build_ns | 0 | 23573758 |
| dispatch_publish_noop_calls | 0 | 0 |
| dispatch_publish_full_rescan_calls | 61774 | 92661 |

### both_on_comb_rect_dense_512

| metric | before | after |
| --- | --- | --- |
| time_dispatch_publish_preserved_annotate_ns | 132069874 | 914056402 |
| time_dispatch_publish_connector_annotate_ns | 248604583 | 203622569 |
| time_dispatch_publish_watch_id_rebuild_ns | 4443756 | 3924545 |
| time_dispatch_publish_canonical_rebuild_ns | 351549262 | 402007535 |
| time_dispatch_publish_posmap_build_ns | 3289142 | 29221959 |
| dispatch_publish_noop_calls | 0 | 0 |
| dispatch_publish_full_rescan_calls | 65568 | 96455 |

### both_on_multi_comb_rect_512

| metric | before | after |
| --- | --- | --- |
| time_dispatch_publish_preserved_annotate_ns | 29713605 | 106657120 |
| time_dispatch_publish_connector_annotate_ns | 3809235 | 3652864 |
| time_dispatch_publish_watch_id_rebuild_ns | 404120 | 343745 |
| time_dispatch_publish_canonical_rebuild_ns | 11025667 | 10504042 |
| time_dispatch_publish_posmap_build_ns | 567701 | 1800034 |
| dispatch_publish_noop_calls | 0 | 0 |
| dispatch_publish_full_rescan_calls | 16462 | 24043 |

progress10의 low risk optimization은 prepublish preserved annotate를 final publish 쪽으로 이동시키는 성격이 있으므로, `time_dispatch_publish_preserved_annotate_ns`와 `dispatch_publish_full_rescan_calls`는 일부 case에서 오히려 증가한다. 대신 reuse route total과 reuse exclusive total이 줄었고 elapsed도 개선됐다. 즉 이번 최적화는 publish work를 없앤 것이 아니라 duplicate prepublish pass를 제거해 reuse path 전체를 줄인 것이다.

## reuse volume counter 표

### connector_only_comb_rect_dense_512

| metric | before | after |
| --- | --- | --- |
| reuse_old_piece_hits | 3794 | 3794 |
| reuse_old_connector_hits | 57980 | 57980 |
| reuse_replacement_pieces | 17804 | 17804 |
| reuse_keepmask_removed_handles | 31019 | 31019 |
| reuse_keepmask_removed_preserved_handles | 1897 | 1897 |
| reuse_keepmask_removed_connector_handles | 29122 | 29122 |
| reuse_preserved_direct_retag_handles | 0 | 0 |
| reuse_connector_direct_retag_handles | 1714002 | 1714002 |
| reuse_attachment_retargets | 0 | 0 |
| reuse_patch_vertices | 3030245 | 3030245 |
| reuse_patch_tree_build_calls | 30887 | 30887 |
| reuse_patch_handles_added | 30186 | 30186 |
| reuse_prepublish_preserved_annotate_calls | 30887 | 0 |

| metric | before | after |
| --- | --- | --- |
| reuse_prepublish_preserved_handles | 3586290 | 0 |
| reuse_prepublish_connector_annotate_calls | 0 | 0 |
| reuse_prepublish_connector_handles | 0 | 0 |
| reuse_full_connector_watch_id_rebuild_calls | 0 | 0 |
| reuse_incremental_connector_watch_id_update_calls | 0 | 0 |
| reuse_final_publish_calls | 30887 | 30887 |
| reuse_final_publish_noop_calls | 0 | 0 |
| reuse_final_publish_skipped_calls | 0 | 0 |
| reuse_watch_handle_full_scan_calls | 30887 | 30887 |
| reuse_watch_handle_full_scan_handles | 3588187 | 3588187 |
| reuse_duplicate_preserved_annotate_passes | 30887 | 0 |
| reuse_duplicate_connector_watch_id_rebuild_passes | 0 | 0 |
| reuse_state_commit_identical_calls | 0 | 0 |

### both_on_comb_rect_dense_512

| metric | before | after |
| --- | --- | --- |
| reuse_old_piece_hits | 3794 | 3794 |
| reuse_old_connector_hits | 57980 | 57980 |
| reuse_replacement_pieces | 17804 | 17804 |
| reuse_keepmask_removed_handles | 219870 | 219870 |
| reuse_keepmask_removed_preserved_handles | 1897 | 1897 |
| reuse_keepmask_removed_connector_handles | 217973 | 217973 |
| reuse_preserved_direct_retag_handles | 89129 | 89129 |
| reuse_connector_direct_retag_handles | 1714002 | 1714002 |
| reuse_attachment_retargets | 0 | 0 |
| reuse_patch_vertices | 3328474 | 3328474 |
| reuse_patch_tree_build_calls | 32784 | 32784 |
| reuse_patch_handles_added | 27451 | 27451 |
| reuse_prepublish_preserved_annotate_calls | 30887 | 0 |

| metric | before | after |
| --- | --- | --- |
| reuse_prepublish_preserved_handles | 3589025 | 0 |
| reuse_prepublish_connector_annotate_calls | 0 | 0 |
| reuse_prepublish_connector_handles | 0 | 0 |
| reuse_full_connector_watch_id_rebuild_calls | 0 | 0 |
| reuse_incremental_connector_watch_id_update_calls | 0 | 0 |
| reuse_final_publish_calls | 32784 | 32784 |
| reuse_final_publish_noop_calls | 0 | 0 |
| reuse_final_publish_skipped_calls | 0 | 0 |
| reuse_watch_handle_full_scan_calls | 32784 | 32784 |
| reuse_watch_handle_full_scan_handles | 3931651 | 3931651 |
| reuse_duplicate_preserved_annotate_passes | 30887 | 0 |
| reuse_duplicate_connector_watch_id_rebuild_passes | 0 | 0 |
| reuse_state_commit_identical_calls | 0 | 0 |

### both_on_multi_comb_rect_512

| metric | before | after |
| --- | --- | --- |
| reuse_old_piece_hits | 1300 | 1300 |
| reuse_old_connector_hits | 13862 | 13862 |
| reuse_replacement_pieces | 7142 | 7142 |
| reuse_keepmask_removed_handles | 35210 | 35210 |
| reuse_keepmask_removed_preserved_handles | 650 | 650 |
| reuse_keepmask_removed_connector_handles | 34560 | 34560 |
| reuse_preserved_direct_retag_handles | 38654 | 38654 |
| reuse_connector_direct_retag_handles | 196806 | 196806 |
| reuse_attachment_retargets | 0 | 0 |
| reuse_patch_vertices | 558382 | 558382 |
| reuse_patch_tree_build_calls | 8231 | 8231 |
| reuse_patch_handles_added | 6455 | 6455 |
| reuse_prepublish_preserved_annotate_calls | 7581 | 0 |

| metric | before | after |
| --- | --- | --- |
| reuse_prepublish_preserved_handles | 724549 | 0 |
| reuse_prepublish_connector_annotate_calls | 0 | 0 |
| reuse_prepublish_connector_handles | 0 | 0 |
| reuse_full_connector_watch_id_rebuild_calls | 0 | 0 |
| reuse_incremental_connector_watch_id_update_calls | 0 | 0 |
| reuse_final_publish_calls | 8231 | 8231 |
| reuse_final_publish_noop_calls | 0 | 0 |
| reuse_final_publish_skipped_calls | 0 | 0 |
| reuse_watch_handle_full_scan_calls | 8231 | 8231 |
| reuse_watch_handle_full_scan_handles | 816548 | 816548 |
| reuse_duplicate_preserved_annotate_passes | 7581 | 0 |
| reuse_duplicate_connector_watch_id_rebuild_passes | 0 | 0 |
| reuse_state_commit_identical_calls | 0 | 0 |

중요한 volume 신호는 세 가지다. `reuse_duplicate_preserved_annotate_passes`는 모두 0으로 떨어졌다. `reuse_prepublish_preserved_annotate_calls`도 모두 0이 됐다. 반면 `reuse_watch_handle_full_scan_calls`와 `reuse_watch_handle_full_scan_handles`는 여전히 크다. watch-handle scan이 두 번째 잔여 축으로 남는 이유다.

## top K slow deletion 요약

### connector_only_comb_rect_dense_512

| rank | idx | x | total_ms | reuse_route | reuse_total_ms | keepmask_removed | direct_retag_pres | direct_retag_conn | attachment_retargets | patch_vertices | patch_handles_added | prepub_pres_calls | prepub_conn_calls | final_noop | final_skipped |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 53 | 52 | 271.099 | connector_skeleton | 164.093 | 225 | 0 | 17856 | 0 | 29289 | 184 | 0 | 0 | 0 | 0 |
| 1 | 50 | 50 | 270.139 | connector_skeleton | 167.966 | 224 | 0 | 18244 | 0 | 29788 | 213 | 0 | 0 | 0 | 0 |
| 2 | 55 | 54 | 267.158 | connector_skeleton | 160.806 | 221 | 0 | 17674 | 0 | 29153 | 200 | 0 | 0 | 0 | 0 |
| 3 | 49 | 48 | 254.442 | connector_skeleton | 156.887 | 229 | 0 | 18577 | 0 | 30214 | 186 | 0 | 0 | 0 | 0 |
| 4 | 42 | 42 | 250.041 | connector_skeleton | 147.432 | 229 | 0 | 19151 | 0 | 30888 | 191 | 0 | 0 | 0 | 0 |
| 5 | 45 | 44 | 247.443 | connector_skeleton | 149.879 | 227 | 0 | 18802 | 0 | 30434 | 209 | 0 | 0 | 0 | 0 |
| 6 | 47 | 46 | 245.395 | connector_skeleton | 155.284 | 226 | 0 | 18588 | 0 | 30233 | 157 | 0 | 0 | 0 | 0 |
| 7 | 68 | 68 | 245.019 | none | 0.0 | 216 | 0 | 16793 | 0 | 27926 | 234 | 0 | 0 | 0 | 0 |
| 8 | 41 | 40 | 242.668 | connector_skeleton | 143.039 | 230 | 0 | 19321 | 0 | 31071 | 198 | 0 | 0 | 0 | 0 |
| 9 | 111 | 110 | 241.449 | none | 0.0 | 197 | 0 | 13474 | 0 | 23102 | 243 | 0 | 0 | 0 | 0 |

### both_on_comb_rect_dense_512

| rank | idx | x | total_ms | reuse_route | reuse_total_ms | keepmask_removed | direct_retag_pres | direct_retag_conn | attachment_retargets | patch_vertices | patch_handles_added | prepub_pres_calls | prepub_conn_calls | final_noop | final_skipped |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 42 | 42 | 299.929 | connector_skeleton | 205.816 | 5126 | 2004 | 19151 | 0 | 38734 | 140 | 0 | 0 | 0 | 0 |
| 1 | 47 | 46 | 277.4 | connector_skeleton | 182.985 | 3545 | 1289 | 18588 | 0 | 35551 | 145 | 0 | 0 | 0 | 0 |
| 2 | 45 | 44 | 272.157 | connector_skeleton | 173.629 | 4572 | 1756 | 18802 | 0 | 37336 | 181 | 0 | 0 | 0 | 0 |
| 3 | 55 | 54 | 269.667 | connector_skeleton | 187.315 | 2769 | 1258 | 17674 | 0 | 33433 | 174 | 0 | 0 | 0 | 0 |
| 4 | 138 | 138 | 267.382 | none | 0.0 | 182 | 0 | 11107 | 0 | 19607 | 165 | 0 | 0 | 0 | 0 |
| 5 | 50 | 50 | 266.642 | connector_skeleton | 181.577 | 3818 | 1530 | 18244 | 0 | 35734 | 172 | 0 | 0 | 0 | 0 |
| 6 | 53 | 52 | 264.274 | connector_skeleton | 181.393 | 3124 | 1022 | 17856 | 0 | 33879 | 157 | 0 | 0 | 0 | 0 |
| 7 | 49 | 48 | 262.162 | connector_skeleton | 167.794 | 3300 | 1273 | 18577 | 0 | 35133 | 147 | 0 | 0 | 0 | 0 |
| 8 | 38 | 38 | 261.422 | connector_skeleton | 169.798 | 5593 | 1999 | 19303 | 0 | 39345 | 163 | 0 | 0 | 0 | 0 |
| 9 | 71 | 70 | 258.79 | none | 0.0 | 2538 | 944 | 16807 | 0 | 31953 | 166 | 0 | 0 | 0 | 0 |

### both_on_multi_comb_rect_512

| rank | idx | x | total_ms | reuse_route | reuse_total_ms | keepmask_removed | direct_retag_pres | direct_retag_conn | attachment_retargets | patch_vertices | patch_handles_added | prepub_pres_calls | prepub_conn_calls | final_noop | final_skipped |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 37 | 34 | 64.19 | delta_preserved_then_skeleton | 35.055 | 2380 | 2966 | 4356 | 0 | 15416 | 42 | 0 | 0 | 0 | 0 |
| 1 | 25 | 22 | 61.842 | connector_skeleton | 33.489 | 2144 | 2910 | 4558 | 0 | 14893 | 40 | 0 | 0 | 0 | 0 |
| 2 | 81 | 78 | 61.596 | none | 0.0 | 436 | 465 | 3705 | 0 | 9741 | 83 | 0 | 0 | 0 | 0 |
| 3 | 29 | 26 | 61.292 | delta_preserved_then_skeleton | 28.846 | 2013 | 2779 | 4515 | 0 | 14859 | 49 | 0 | 0 | 0 | 0 |
| 4 | 89 | 86 | 58.962 | none | 0.0 | 293 | 264 | 3574 | 0 | 9104 | 103 | 0 | 0 | 0 | 0 |
| 5 | 77 | 74 | 58.744 | none | 0.0 | 337 | 320 | 3782 | 0 | 9662 | 92 | 0 | 0 | 0 | 0 |
| 6 | 49 | 46 | 58.554 | connector_skeleton | 31.882 | 1387 | 1543 | 4192 | 0 | 12959 | 61 | 0 | 0 | 0 | 0 |
| 7 | 41 | 38 | 58.386 | connector_skeleton | 26.751 | 1261 | 1715 | 4329 | 0 | 12975 | 67 | 0 | 0 | 0 | 0 |
| 8 | 61 | 58 | 57.817 | connector_skeleton | 26.216 | 754 | 801 | 3973 | 0 | 11012 | 71 | 0 | 0 | 0 | 0 |
| 9 | 73 | 70 | 56.167 | none | 0.0 | 503 | 555 | 3849 | 0 | 10277 | 86 | 0 | 0 | 0 | 0 |

dense 512의 느린 deletion은 거의 모두 `connector_skeleton` route에서 나온다. multi 512는 `delta_preserved_then_skeleton`과 `connector_skeleton`이 섞인다. after 기준 slow deletion에서도 `prepub_pres_calls=0`이 유지되므로 duplicate preserved annotate 제거는 실제 hot deletion에서도 적용되고 있다.

## release 1024와 representative 4096

| tag | elapsed_s | rc | timed_out | validator_ok | stdout_nonempty | stderr_nonempty |
| --- | --- | --- | --- | --- | --- | --- |
| after_both_on_dense_1024_release | None | 124 | True | False | False | False |
| after_both_on_multi_1024_release | 74.29 | 0 | False | True | True | False |
| after_both_on_dense_4096_release | None | 1 | False | False | False | False |
| after_both_on_multi_4096_release | None | 1 | False | False | False | False |

`both_on, multi_comb_rect 1024 RELEASE`는 validator OK로 회수됐다. `both_on, comb_rect_dense 1024 RELEASE`는 420초 timeout으로 끝났고, 이번 라운드 최적화만으로는 아직 회수되지 않았다. representative 4096 두 케이스는 모두 `rc=1`, 무출력이었다.

## residual cost 판정

| axis | time_s | share_pct |
| --- | --- | --- |
| preserved_piece_split | 14.412 | 29.8 |
| reuse_watch_handle_scan_reduction | 10.095 | 20.9 |
| watch_churn | 7.338 | 15.2 |
| reuse_metadata_compaction | 5.456 | 11.3 |
| reuse_final_publish_commit | 4.519 | 9.4 |
| publish_path_compaction | 3.071 | 6.4 |
| reuse_patch_build_simplification | 2.539 | 5.3 |
| connector_skeleton_build_core | 0.675 | 1.4 |
| global_delete_dfs | 0.221 | 0.5 |
| query_incident_scans | 0.004 | 0.0 |

이 표는 after sampled 세 case의 residual category를 합산한 것이다. `preserved_piece_split`가 가장 크고, 그다음이 `reuse_watch_handle_scan_reduction`, 그다음이 `watch_churn`이다. `reuse_metadata_compaction`은 duplicate preserved annotate 제거 이후 4순위권으로 내려왔다. 따라서 이번 라운드의 질문에 대한 답은 명확하다. 남은 비용은 더 이상 metadata retag and annotate가 1순위가 아니다.

정리하면 다음과 같다.

1. `duplicate metadata retag and annotate`는 이번 라운드 최적화로 큰 폭으로 줄었다.
2. `watch-handle keep and retain scan`은 여전히 크지만 1순위는 아니다.
3. `connector patch build and attachment retarget`은 5퍼센트에서 9퍼센트 수준으로 1순위가 아니다.
4. `final publish commit inside reuse apply`도 10퍼센트에서 15퍼센트 수준으로 1순위가 아니다.
5. 실제 1순위 잔여 축은 `preserved piece split`, 그중에서도 `old_attachment_map_build`다.

## 최종 결론

`next pivot after reuse round: preserved piece split`