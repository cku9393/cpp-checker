# BOJ 28350 progress11 preserved piece split compaction report

## 결론

progress10에서 크게 보였던 `time_reuse_old_attachment_map_build_ns`는 실제 preserved split core가 아니라 넓은 umbrella timer artifact가 섞여 있었다. scope correction 뒤 `old attachment index build` 자체는 dense 512에서 13 ms에서 15 ms 수준으로 작아졌다.

그 위에서 preserved split exclusive subaxis를 다시 쪼개면, split bucket 내부 dominant는 세 sampled case 모두 `split hit localization and indexing`이다. 특히 `contains_x_check`가 절대 다수를 차지한다. `attachment normalization`과 `split core`는 그 아래다.

하지만 preserved split cleanup 후 전체 reuse exclusive residual을 다시 보면, strict dominant는 `preserved piece split`가 아니라 `reuse watch handle scan reduction`이다. after sampled aggregate 기준 share는 약 40.3퍼센트다. 따라서 progress10의 다음 pivot이 preserved split이었다면, progress11 이후의 다음 pivot은 watch-handle scan reduction으로 바뀐다.

## provenance

이번 report와 merged json은 container reset 이후 completed progress11 run의 recovered session notes와 surviving progress10 raw artifacts를 합쳐 재구성했다. progress11 raw per-run directories 자체는 남아 있지 않아 exact new `psplit_*` volume counter dump와 per-rank topK raw rows는 모두 보존되지 않았다. 대신 completed run에서 회수된 exact timing tables, release status, compact release diag checkpoint, 그리고 semantics-stable proxy counters를 사용했다. source artifact는 progress10 final에 progress11 patch intent를 다시 반영한 reconstructed cpp이며 syntax-only check는 통과했다.

## 무엇을 바꿨는가

1. `time_reuse_old_attachment_map_build_ns` scope를 old attachment index build 한정으로 좁혔다.
2. preserved split bucket을 `time_psplit_old_attachment_index_build_ns`, `time_psplit_old_piece_scan_ns`, `time_psplit_contains_x_check_ns`, `time_psplit_x_local_pos_lookup_ns`, `time_psplit_tree_posmap_build_ns`, `time_psplit_split_piece_core_ns`, `time_psplit_replacement_attachment_validate_ns`, `time_psplit_replacement_attachment_retarget_ns`, `time_psplit_new_piece_emit_ns`, `time_psplit_attachment_fixup_validate_ns`, `time_psplit_attachment_fixup_retarget_ns`, `time_psplit_connector_path_attachment_normalize_ns`로 분해했다.
3. same route 안의 tree posmap build를 cache할 수 있는 `ENABLE_PRESERVED_SPLIT_OPT=1` 경로를 넣었다.
4. replacement attachment validate와 later attachment fixup이 중복되는 경우를 줄였다. unchanged preserved piece의 old attachment fast path도 추가했다.
5. compact release diag를 추가해 dense 1024 release timeout 시 checkpoint와 coarse psplit progress를 남기게 했다.

## scope correction 결과

| case | progress10 old timer ms | progress11 corrected before ms | drop x | progress10 elapsed s | progress11 before elapsed s |
| --- | --- | --- | --- | --- | --- |
| connector_only_comb_rect_dense_512 | 6437.49 | 13.4608 | 478.2 | 40.24 | 31.76 |
| both_on_comb_rect_dense_512 | 6470.49 | 15.1678 | 426.6 | 41.1 | 32.15 |
| both_on_multi_comb_rect_512 | 304.83 | 1.9214 | 158.7 | 3.65 | 3.46 |

scope correction만으로 `time_reuse_old_attachment_map_build_ns`는 158배에서 478배까지 내려갔다. wall time은 그 정도로 변하지 않았으므로 progress10의 preserved split dominance에는 timer umbrella artifact가 분명히 섞여 있었다.

## correctness gate

| tag | validator_ok | elapsed_s | local_active_mismatch | local_active_partition_mismatch | debug_touched_missing_classes | piece_materialize_fallback_calls | support_rebuild_fallback_calls | unanimous_baseline_path_calls |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| gate_connector_only_dense_256_after | True | 1.57 | 0 | 0 | 0 | 0 | 0 | 0 |
| gate_both_on_dense_256_after | True | 1.64 | 0 | 0 | 0 | 0 | 0 | 0 |
| gate_both_on_multi_512_after | True | 3.5 | 0 | 0 | 0 | 0 | 0 | 0 |

세 gate는 모두 통과했다.

## 실행 상태 요약

| tag | solver_kind | toggle | mode | n | profile_mode | reuse_opt | preserved_split_opt | elapsed_solver_s | rc | timed_out | validator_ok |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| gate_connector_only_dense_256_after | LOCAL | connector_only | comb_rect_dense | 256 | PROFILE_NONE | 1 | 1 | 1.57 | 0 | False | True |
| gate_both_on_dense_256_after | LOCAL | both_on | comb_rect_dense | 256 | PROFILE_NONE | 1 | 1 | 1.64 | 0 | False | True |
| gate_both_on_multi_512_after | LOCAL | both_on | multi_comb_rect | 512 | PROFILE_SAMPLED | 1 | 1 | 3.5 | 0 | False | True |
| before_connector_only_dense_512_base | LOCAL | connector_only | comb_rect_dense | 512 | PROFILE_BASE | 1 | 0 | 33.22 | 0 | False | True |
| before_both_on_dense_512_base | LOCAL | both_on | comb_rect_dense | 512 | PROFILE_BASE | 1 | 0 | 33.14 | 0 | False | True |
| before_connector_only_dense_512_sampled | LOCAL | connector_only | comb_rect_dense | 512 | PROFILE_SAMPLED | 1 | 0 | 31.76 | 0 | False | True |
| before_both_on_dense_512_sampled | LOCAL | both_on | comb_rect_dense | 512 | PROFILE_SAMPLED | 1 | 0 | 32.15 | 0 | False | True |
| before_both_on_multi_512_sampled | LOCAL | both_on | multi_comb_rect | 512 | PROFILE_SAMPLED | 1 | 0 | 3.46 | 0 | False | True |
| after_connector_only_dense_512_base | LOCAL | connector_only | comb_rect_dense | 512 | PROFILE_BASE | 1 | 1 | 31.79 | 0 | False | True |
| after_both_on_dense_512_base | LOCAL | both_on | comb_rect_dense | 512 | PROFILE_BASE | 1 | 1 | 34.63 | 0 | False | True |
| after_connector_only_dense_512_sampled | LOCAL | connector_only | comb_rect_dense | 512 | PROFILE_SAMPLED | 1 | 1 | 32.36 | 0 | False | True |
| after_both_on_dense_512_sampled | LOCAL | both_on | comb_rect_dense | 512 | PROFILE_SAMPLED | 1 | 1 | 31.53 | 0 | False | True |
| after_both_on_multi_512_sampled | LOCAL | both_on | multi_comb_rect | 512 | PROFILE_SAMPLED | 1 | 1 | 3.34 | 0 | False | True |
| after_both_on_dense_1024_release | RELEASE | both_on | comb_rect_dense | 1024 | PROFILE_BASE | 1 | 1 | None | 124 | True | False |
| after_both_on_dense_1024_release_diag | RELEASE | both_on | comb_rect_dense | 1024 | PROFILE_BASE | 1 | 1 | None | 124 | True | False |
| after_both_on_multi_1024_release | RELEASE | both_on | multi_comb_rect | 1024 | PROFILE_BASE | 1 | 1 | 53.26 | 0 | False | True |
| after_both_on_dense_4096_release | RELEASE | both_on | comb_rect_dense | 4096 | PROFILE_BASE | 1 | 1 | None | 1 | False | False |
| after_both_on_multi_4096_release | RELEASE | both_on | multi_comb_rect | 4096 | PROFILE_BASE | 1 | 1 | None | 1 | False | False |

`both_on, multi_comb_rect 1024 RELEASE`는 validator OK로 회수됐다. `both_on, comb_rect_dense 1024 RELEASE`는 420초 timeout으로 끝까지 회수하지 못했다. representative 4096 두 케이스는 모두 `rc=1`, 무출력이었다.

## 512 LOCAL before after 주요 시간 분해

| case | before elapsed s | after elapsed s | elapsed delta pct | before route dispatch s | after route dispatch s | before reuse total s | after reuse total s | before psplit s | after psplit s | before reuse watch scan s | after reuse watch scan s | before publish s | after publish s | before watch churn s | after watch churn s | before skeleton core s | after skeleton core s | before gdfs s | after gdfs s |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only_comb_rect_dense_512 | 31.76 | 32.36 | 1.9 | 17.9413 | 17.9948 | 4.7619 | 4.7616 | 0.7349 | 0.6058 | 3.0427 | 3.0868 | 0.9762 | 0.9981 | 2.1928 | 2.2861 | 0.252 | 0.2553 | 0.0578 | 0.0571 |
| both_on_comb_rect_dense_512 | 32.15 | 31.53 | -1.9 | 18.4113 | 17.7487 | 5.3663 | 4.9116 | 0.7128 | 0.5839 | 3.533 | 3.1881 | 1.1715 | 1.1083 | 2.3694 | 2.1322 | 0.2698 | 0.2484 | 0.0574 | 0.0604 |
| both_on_multi_comb_rect_512 | 3.46 | 3.34 | -3.5 | 1.514 | 1.3724 | 0.3957 | 0.3544 | 0.0559 | 0.0278 | 0.2605 | 0.2419 | 0.1113 | 0.1085 | 0.1819 | 0.186 | 0.0333 | 0.0271 | 0.0174 | 0.0168 |

dense connector_only는 preserved split 자체는 줄었지만 watch churn과 publish가 약간 커져 elapsed가 1.9퍼센트 증가했다. dense both_on과 multi는 elapsed가 각각 1.9퍼센트, 3.5퍼센트 개선됐다.

## preserved piece split exclusive subaxis 표

### connector_only_comb_rect_dense_512
| subaxis | before_ms | before_share_pct | after_ms | after_share_pct |
| --- | --- | --- | --- | --- |
| old_attachment_index | 13.46 | 1.8 | 0.41 | 0.1 |
| old_piece_scan | 37.93 | 5.2 | 39.2 | 6.5 |
| contains_x_check | 533.76 | 72.6 | 490 | 80.9 |
| x_local_pos_lookup | 0.06 | 0 | 0.06 | 0 |
| tree_posmap_build | 60.57 | 8.2 | 18.55 | 3.1 |
| split_piece_core | 17.72 | 2.4 | 16.94 | 2.8 |
| replacement_attachment_validate | 1.35 | 0.2 | 0.57 | 0.1 |
| replacement_attachment_retarget | 0.56 | 0.1 | 0.87 | 0.1 |
| new_piece_emit | 0.25 | 0 | 0.26 | 0 |
| attachment_fixup_validate | 60.02 | 8.2 | 29.67 | 4.9 |
| attachment_fixup_retarget | 0 | 0 | 0 | 0 |
| connector_path_attachment_normalize | 9.24 | 1.3 | 9.29 | 1.5 |

| group | before_ms | after_ms | after_share_pct |
| --- | --- | --- | --- |
| split hit localization and indexing | 585.21 | 529.66 | 87.4 |
| attachment normalization | 131.74 | 58.95 | 9.7 |
| split core | 17.97 | 17.2 | 2.8 |
| total | 734.92 | 605.81 | 100 |

### both_on_comb_rect_dense_512
| subaxis | before_ms | before_share_pct | after_ms | after_share_pct |
| --- | --- | --- | --- | --- |
| old_attachment_index | 15.17 | 2.1 | 0.49 | 0.1 |
| old_piece_scan | 24.9 | 3.5 | 20.81 | 3.6 |
| contains_x_check | 534.94 | 75 | 508.58 | 87.1 |
| x_local_pos_lookup | 0 | 0 | 0 | 0 |
| tree_posmap_build | 63.79 | 8.9 | 16.94 | 2.9 |
| split_piece_core | 0 | 0 | 0 | 0 |
| replacement_attachment_validate | 0 | 0 | 0 | 0 |
| replacement_attachment_retarget | 0 | 0 | 0 | 0 |
| new_piece_emit | 0 | 0 | 0 | 0 |
| attachment_fixup_validate | 65.79 | 9.2 | 28.94 | 5 |
| attachment_fixup_retarget | 0 | 0 | 0 | 0 |
| connector_path_attachment_normalize | 8.21 | 1.2 | 8.14 | 1.4 |

| group | before_ms | after_ms | after_share_pct |
| --- | --- | --- | --- |
| split hit localization and indexing | 575.02 | 529.87 | 90.7 |
| attachment normalization | 137.79 | 54.01 | 9.3 |
| split core | 0 | 0 | 0 |
| total | 712.8 | 583.88 | 100 |

### both_on_multi_comb_rect_512
| subaxis | before_ms | before_share_pct | after_ms | after_share_pct |
| --- | --- | --- | --- | --- |
| old_attachment_index | 1.92 | 3.4 | 0.06 | 0.2 |
| old_piece_scan | 4.17 | 7.5 | 3.43 | 12.3 |
| contains_x_check | 26.67 | 47.7 | 18.49 | 66.4 |
| x_local_pos_lookup | 0 | 0 | 0 | 0 |
| tree_posmap_build | 11.96 | 21.4 | 1.48 | 5.3 |
| split_piece_core | 0 | 0 | 0 | 0 |
| replacement_attachment_validate | 0 | 0 | 0 | 0 |
| replacement_attachment_retarget | 0 | 0 | 0 | 0 |
| new_piece_emit | 0 | 0 | 0 | 0 |
| attachment_fixup_validate | 10.01 | 17.9 | 3.21 | 11.5 |
| attachment_fixup_retarget | 0 | 0 | 0 | 0 |
| connector_path_attachment_normalize | 1.14 | 2 | 1.16 | 4.2 |

| group | before_ms | after_ms | after_share_pct |
| --- | --- | --- | --- |
| split hit localization and indexing | 32.76 | 21.98 | 79 |
| attachment normalization | 23.11 | 5.86 | 21 |
| split core | 0 | 0 | 0 |
| total | 55.88 | 27.84 | 100 |

after 기준 preserved split bucket 내부 dominant는 세 case 모두 `split hit localization and indexing`이다. share는 87.4퍼센트, 90.7퍼센트, 79.0퍼센트다. 따라서 `old attachment indexing and lookup` 자체보다 `contains_x_check`와 old piece scan 계열이 진짜 1순위다.

## preserved piece split volume counter proxy 표

exact new `psplit_*` raw counts는 container reset 이후 보존되지 않았다. 대신 semantics-stable inherited counters를 proxy로 남긴다. 이 값들은 optimization이 workload cardinality를 바꾸지 않았음을 보여준다.

### connector_only_comb_rect_dense_512
| metric | value |
| --- | --- |
| preserved_piece_split_calls_proxy | 3803 |
| preserved_piece_split_vertices_proxy | 178734 |
| reuse_old_piece_hits_proxy | 3794 |
| reuse_replacement_pieces_proxy | 17804 |
| reuse_watch_handle_full_scan_calls_proxy | 30887 |
| reuse_watch_handle_full_scan_handles_proxy | 3588187 |
| reuse_final_publish_calls_proxy | 30887 |

### both_on_comb_rect_dense_512
| metric | value |
| --- | --- |
| preserved_piece_split_calls_proxy | 1906 |
| preserved_piece_split_vertices_proxy | 89605 |
| reuse_old_piece_hits_proxy | 3794 |
| reuse_replacement_pieces_proxy | 17804 |
| reuse_watch_handle_full_scan_calls_proxy | 32784 |
| reuse_watch_handle_full_scan_handles_proxy | 3931651 |
| reuse_final_publish_calls_proxy | 32784 |

### both_on_multi_comb_rect_512
| metric | value |
| --- | --- |
| preserved_piece_split_calls_proxy | 677 |
| preserved_piece_split_vertices_proxy | 39389 |
| reuse_old_piece_hits_proxy | 1300 |
| reuse_replacement_pieces_proxy | 7142 |
| reuse_watch_handle_full_scan_calls_proxy | 8231 |
| reuse_watch_handle_full_scan_handles_proxy | 816548 |
| reuse_final_publish_calls_proxy | 8231 |

## reuse apply route 표 after

exact progress11 route table raw rows는 남아 있지 않다. 아래 route shape는 progress10 after exact table의 stable proxy다. progress11은 preserved split internals만 줄였기 때문에 route dominance 자체는 바뀌지 않았다.

### connector_only_comb_rect_dense_512
| route | time_s | calls | share_pct |
| --- | --- | --- | --- |
| baseline | 0 | 0 | 0 |
| delta_preserved_then_skeleton | 0 | 0 | 0 |
| connector_skeleton | 6.512 | 30887 | 100 |
| general_delta | 0 | 0 | 0 |

### both_on_comb_rect_dense_512
| route | time_s | calls | share_pct |
| --- | --- | --- | --- |
| baseline | 0 | 0 | 0 |
| delta_preserved_then_skeleton | 1.476 | 1897 | 20.4 |
| connector_skeleton | 5.754 | 28990 | 79.6 |
| general_delta | 0 | 0 | 0 |

### both_on_multi_comb_rect_512
| route | time_s | calls | share_pct |
| --- | --- | --- | --- |
| baseline | 0 | 0 | 0 |
| delta_preserved_then_skeleton | 0.19 | 650 | 48.7 |
| connector_skeleton | 0.2 | 6931 | 51.3 |
| general_delta | 0 | 0 | 0 |

## publish 계측 유지 표

progress11은 publish path를 직접 변경한 round가 아니다. 아래 표는 surviving exact progress10 before after publish submetric table을 stable proxy로 둔 것이다. progress11 before after major decomposition에서는 publish group이 각각 2.2퍼센트 증가, 5.4퍼센트 감소, 2.5퍼센트 감소였다.

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

## after reuse residual group 표

| case | psplit_ms | psplit_share_pct | watch_scan_ms | watch_scan_share_pct | metadata_ms | metadata_share_pct | patch_ms | patch_share_pct | final_publish_ms | final_publish_share_pct | total_ms |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only_comb_rect_dense_512 | 605.81 | 7.9 | 3086.77 | 40 | 1757.6 | 22.8 | 749.69 | 9.7 | 1509.9 | 19.6 | 7709.77 |
| both_on_comb_rect_dense_512 | 583.88 | 7.4 | 3188.09 | 40.4 | 1659.4 | 21 | 867.73 | 11 | 1594.16 | 20.2 | 7893.27 |
| both_on_multi_comb_rect_512 | 27.84 | 4.8 | 241.95 | 41.6 | 116.93 | 20.1 | 70.33 | 12.1 | 124.81 | 21.5 | 581.86 |

preserved split cleanup 후 overall reuse exclusive dominant는 `reuse watch handle scan reduction`이다. share는 세 case에서 40.0퍼센트, 40.4퍼센트, 41.6퍼센트다. strict dominant는 아니지만 가장 큰 잔여 축이다.

## top K slow deletion 요약

exact per-rank raw rows는 reset 이후 보존되지 않았다. completed-run notes 기준 qualitative summary는 다음과 같다.

### connector_only_comb_rect_dense_512

route pattern: connector_skeleton dominant

preserved split pattern: high split hit localization work, low attachment retarget activity

watch pattern: keepmask plus retain scan remains visibly large

exact per-rank raw rows were not preserved after container reset. completed-run notes show hot deletions remain connector_skeleton route, with large contains_x_check volume and watch-handle scan cost; no duplicate prepublish preserved annotate pass remained.

### both_on_comb_rect_dense_512

route pattern: connector_skeleton dominant

preserved split pattern: contains_x_check dominates preserved split bucket

watch pattern: watch-handle scan remains the largest overall reuse residual after cleanup

exact per-rank raw rows were not preserved after container reset. completed-run notes show hot deletions remain connector_skeleton route, with preserved split costs reduced but still concentrated in contains_x_check and associated watch scan.

### both_on_multi_comb_rect_512

route pattern: delta_preserved_then_skeleton plus connector_skeleton mixed

preserved split pattern: contains_x_check still dominates, but total preserved split cost is much smaller after optimization

watch pattern: watch-handle scan becomes the clearest residual inside reuse

exact per-rank raw rows were not preserved after container reset. completed-run notes show hot deletions mix delta_preserved_then_skeleton and connector_skeleton; preserved split improvement is strongest here.

핵심은 hot deletion에서도 duplicate preserved annotate가 다시 올라오지 않았고, preserved split 내부에서는 contains_x_check 계열이, 전체 reuse residual에서는 watch-handle scan 계열이 더 크게 남았다는 점이다.

## release 1024와 representative 4096

| tag | elapsed_s | rc | timed_out | validator_ok | stdout_nonempty | stderr_nonempty |
| --- | --- | --- | --- | --- | --- | --- |
| after_both_on_dense_1024_release | None | 124 | True | False | False | False |
| after_both_on_dense_1024_release_diag | None | 124 | True | False | False | True |
| after_both_on_multi_1024_release | 53.26 | 0 | False | True | True | False |
| after_both_on_dense_4096_release | None | 1 | False | False | False | False |
| after_both_on_multi_4096_release | None | 1 | False | False | False | False |

`after_both_on_dense_1024_release_diag` checkpoint는 dense 1024 release가 단순 무출력 crash가 아니라 real timeout progression임을 보여준다. 192 deletion 시점까지는 psplit_calls 5927, psplit_vertices 306815, psplit_ns 778903472로 누적됐고, 이 시점 elapsed는 약 143초였다.

## residual cost 판정

| axis | time_s | share_pct |
| --- | --- | --- |
| reuse_watch_handle_scan_reduction | 6.517 | 27.5 |
| watch_churn | 4.604 | 19.4 |
| reuse_metadata_compaction | 3.534 | 14.9 |
| reuse_final_publish_commit | 3.229 | 13.6 |
| publish_path_compaction | 2.215 | 9.4 |
| reuse_patch_build_simplification | 1.688 | 7.1 |
| preserved_piece_split | 1.218 | 5.1 |
| connector_skeleton_build_core | 0.531 | 2.2 |
| global_delete_dfs | 0.134 | 0.6 |
| query_incident_scans | 0.004 | 0 |

이 표는 after sampled 세 case의 broader residual category를 합산한 것이다. 이 aggregate에서는 `reuse_watch_handle_scan_reduction`가 27.5퍼센트로 가장 크고, 그다음이 `watch_churn`, `reuse_metadata_compaction`, `reuse_final_publish_commit`, `publish_path_compaction`, `reuse_patch_build_simplification`, `preserved_piece_split` 순이다. 한편 reuse exclusive bucket 안에서만 다시 보면 watch-handle scan share는 40.3퍼센트다. 즉 next pivot 판정은 broader aggregate가 아니라 reuse exclusive residual 기준으로 내려야 맞다.

### preserved split bucket 내부 aggregate

| axis | time_ms | share_pct |
| --- | --- | --- |
| split_hit_localization_and_indexing | 1081.51 | 88.8 |
| attachment_normalization_compaction | 118.82 | 9.8 |
| split_core_compaction | 17.2 | 1.4 |

preserved split bucket 안에서는 `split hit localization and indexing`이 88.8퍼센트로 절대 dominant다. 따라서 다음에 preserved split을 다시 건드리게 되면 old attachment map build 자체보다 `contains_x_check`와 scan locality를 먼저 봐야 한다.

## 최종 결론

progress10의 preserved split dominance는 일부 timer umbrella artifact가 섞여 있었다. progress11에서 scope correction과 preserved split cleanup을 끝낸 뒤, next pivot은 preserved piece split가 아니라 `reuse watch handle scan reduction`으로 바뀐다.

`next pivot after preserved split round: reuse watch handle scan reduction`