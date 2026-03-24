# BOJ 28350 progress12 reuse watch handle scan reduction report

## 결론

progress11에서 next pivot으로 올렸던 `reuse watch handle scan reduction`은 이번 progress12에서 tighter timer scope로 다시 분해해 보니 일부 umbrella-timer artifact가 섞여 있었다. aggregate 기준 reported share는 progress11 after의 40.3퍼센트에서 progress12 corrected after의 16.8퍼센트로 내려갔다.

그럼에도 low-risk 최적화 자체는 실제로 먹혔다. 세 sampled case 모두에서 duplicate full scan pass는 사실상 제거됐고, full-scan calls와 scanned handles는 약 74퍼센트 줄었다. 그 결과 `both_on, comb_rect_dense 1024 RELEASE`도 이번 라운드에서 validator OK로 회수됐다.

watch bucket 내부에서는 strict dominant가 없었고, 세 sampled case 모두에서 largest residual은 `retain compaction reduction`이었다. broader measured residual로는 `watch churn`이 여전히 더 크지만, 이번 라운드 프롬프트의 기준에 따라 다음 pivot은 `retain compaction reduction`으로 잡는 것이 맞다.

마지막 결론 한 줄은 이거다.

`next pivot after watch scan round: retain compaction reduction`

## provenance

이번 report와 merged json은 completed progress12 session notes와 surviving progress11 report/json, 그리고 reconstructed progress12 source를 합쳐 재구성했다. progress12 raw per-run directories 자체는 현재 filesystem에 남아 있지 않아서, 일부 local gate와 512 before/after run의 exact elapsed와 full topK raw row는 보존되지 않았다. 대신 완료 세션에서 명시적으로 회수된 watch-scan timing, counter delta, release status, and final pivot 판단을 패키징했다.

source syntax-only check는 통과했다.

## 무엇을 바꿨는가

1. `time_reuse_keepmask_scan_ns`와 `time_reuse_watch_retain_ns` scope를 core work 쪽으로 좁혔다.
2. watch scan bucket을 `time_wscan_preserved_keepstamp_build_ns`, `time_wscan_preserved_keepmask_decision_ns`, `time_wscan_preserved_stamp_mark_ns`, `time_wscan_connector_desired_set_build_ns`, `time_wscan_connector_keepmask_decision_ns`, `time_wscan_connector_existing_set_build_ns`, `time_wscan_connector_addverts_diff_ns`, `time_wscan_retain_remove_entries_ns`, `time_wscan_retain_compact_handles_ns`, `time_wscan_retain_slotpos_fixup_ns`, `time_wscan_retain_handleidx_fixup_ns`, `time_wscan_retain_owner_lookup_ns`로 다시 분해했다.
3. connector skeleton route에서 `st.connectorWatchEntryIds` fast path를 써서 connector keepmask decision과 existing connector set build의 full scan을 줄였다.
4. preserved keepmask decision과 stamp mark 쪽 중복 pass를 줄였다.
5. keepmask 결과가 거의 no-op이면 retain 쪽 heavy compaction을 가능한 한 건너뛰게 정리했다.
6. slow deletion summary에 `wscan_route`, `wscan_preserved_handles_scanned`, `wscan_connector_handles_scanned`, `wscan_existing_connector_set_handles_scanned`, `wscan_retain_removed_handles`, `wscan_retain_slotpos_fixups`, `wscan_duplicate_full_scan_passes`, `wscan_total_ns`를 추가했다.

## scope correction 결과

| case | progress11 reported watch scan ms | progress11 reported share pct | progress12 corrected watch scan ms | progress12 corrected reuse total ms | progress12 corrected share pct |
| --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 3086.77 | 40.0 | 248.98 | 1365.04 | 18.2 |
| both_on, comb_rect_dense 512 LOCAL | 3188.09 | 40.4 | 235.54 | 1355.61 | 17.4 |
| both_on, multi_comb_rect 512 LOCAL | 241.95 | 41.6 | 50.72 | 459.16 | 11.0 |

aggregate reported share는 40.3퍼센트에서 corrected share 16.8퍼센트로 내려갔다. 이건 progress11의 watch-scan dominance 판단에 umbrella timer artifact가 일부 섞여 있었다는 뜻이다.

## correctness gate

| tag | validator_ok | elapsed_s | local_active_mismatch | local_active_partition_mismatch | debug_touched_missing_classes | piece_materialize_fallback_calls | support_rebuild_fallback_calls | unanimous_baseline_path_calls |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| gate_connector_only_dense_256_after | True | None | 0 | 0 | 0 | 0 | 0 | 0 |
| gate_both_on_dense_256_after | True | None | 0 | 0 | 0 | 0 | 0 | 0 |
| gate_both_on_multi_512_after | True | None | 0 | 0 | 0 | 0 | 0 | 0 |

세 gate는 모두 통과했다. exact elapsed는 surviving raw artifacts에 남지 않아 `null`로 둔다.

## 실행 상태 요약

| tag | solver_kind | toggle | mode | n | profile_mode | reuse_opt | preserved_split_opt | watch_scan_opt | completed | elapsed_solver_s | rc | timed_out | validator_ok |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| gate_connector_only_dense_256_after | LOCAL | connector_only | comb_rect_dense | 256 | PROFILE_NONE | 1 | 1 | 1 | True | None | 0 | False | True |
| gate_both_on_dense_256_after | LOCAL | both_on | comb_rect_dense | 256 | PROFILE_NONE | 1 | 1 | 1 | True | None | 0 | False | True |
| gate_both_on_multi_512_after | LOCAL | both_on | multi_comb_rect | 512 | PROFILE_SAMPLED | 1 | 1 | 1 | True | None | 0 | False | True |
| before_connector_only_dense_512_base | LOCAL | connector_only | comb_rect_dense | 512 | PROFILE_BASE | 1 | 1 | 0 | True | None | 0 | False | True |
| before_both_on_dense_512_base | LOCAL | both_on | comb_rect_dense | 512 | PROFILE_BASE | 1 | 1 | 0 | True | None | 0 | False | True |
| before_connector_only_dense_512_sampled | LOCAL | connector_only | comb_rect_dense | 512 | PROFILE_SAMPLED | 1 | 1 | 0 | True | None | 0 | False | True |
| before_both_on_dense_512_sampled | LOCAL | both_on | comb_rect_dense | 512 | PROFILE_SAMPLED | 1 | 1 | 0 | True | None | 0 | False | True |
| before_both_on_multi_512_sampled | LOCAL | both_on | multi_comb_rect | 512 | PROFILE_SAMPLED | 1 | 1 | 0 | True | None | 0 | False | True |
| after_connector_only_dense_512_base | LOCAL | connector_only | comb_rect_dense | 512 | PROFILE_BASE | 1 | 1 | 1 | True | None | 0 | False | True |
| after_both_on_dense_512_base | LOCAL | both_on | comb_rect_dense | 512 | PROFILE_BASE | 1 | 1 | 1 | True | None | 0 | False | True |
| after_connector_only_dense_512_sampled | LOCAL | connector_only | comb_rect_dense | 512 | PROFILE_SAMPLED | 1 | 1 | 1 | True | None | 0 | False | True |
| after_both_on_dense_512_sampled | LOCAL | both_on | comb_rect_dense | 512 | PROFILE_SAMPLED | 1 | 1 | 1 | True | None | 0 | False | True |
| after_both_on_multi_512_sampled | LOCAL | both_on | multi_comb_rect | 512 | PROFILE_SAMPLED | 1 | 1 | 1 | True | None | 0 | False | True |
| after_both_on_dense_1024_release | RELEASE | both_on | comb_rect_dense | 1024 | PROFILE_BASE | 1 | 1 | 1 | True | 335.23 | 0 | False | True |
| after_both_on_multi_1024_release | RELEASE | both_on | multi_comb_rect | 1024 | PROFILE_BASE | 1 | 1 | 1 | True | 18.3 | 0 | False | True |
| after_both_on_dense_4096_release | RELEASE | both_on | comb_rect_dense | 4096 | PROFILE_BASE | 1 | 1 | 1 | True | 240.16 | 1 | False | False |
| after_both_on_multi_4096_release | RELEASE | both_on | multi_comb_rect | 4096 | PROFILE_BASE | 1 | 1 | 1 | True | 240.19 | 124 | True | False |

LOCAL 512 before/after와 release/representative는 completed session에서 모두 끝까지 회수됐지만, local 512의 exact elapsed는 패키징 시점의 surviving raw artifacts에는 남아 있지 않았다.

## 512 LOCAL watch-scan before after 핵심 표

| case | dup scan passes before | dup scan passes after | full scan calls before | full scan calls after | calls drop pct | scanned handles before | scanned handles after | handles drop pct | connectorWatchEntryIds fastpath calls after | preservedHandleIdxs fastpath calls after |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 324 | 0 | 83,792 | 20,948 | 75.0 | 19,752,074 | 4,979,394 | 74.8 | 20,948 | 20,948 |
| both_on, comb_rect_dense 512 LOCAL | 306 | 0 | 84,689 | 21,845 | 74.2 | 20,045,778 | 5,210,828 | 74.0 | 21,845 | 21,845 |
| both_on, multi_comb_rect 512 LOCAL | 288 | 1 | 26,721 | 7,029 | 73.7 | 4,859,171 | 1,287,048 | 73.5 | 6,999 | 7,029 |

세 case 모두 duplicate full scan pass는 거의 0으로 내려갔고, full-scan calls와 scanned handles는 약 74퍼센트 줄었다.

## corrected after watch handle scan exclusive subaxis 표

### connector_only, comb_rect_dense 512 LOCAL
| subaxis | ms | reported share pct |
| --- | --- | --- |
| watch pass fusion and subset indexing | 40.23 | 16.2 |
| retain compaction reduction | 92.56 | 37.2 |
| connector watch set reuse | 60.76 | 24.4 |
| other or unclassified | 55.43 | 22.3 |

total corrected watch-scan bucket는 248.98 ms다.

### both_on, comb_rect_dense 512 LOCAL
| subaxis | ms | reported share pct |
| --- | --- | --- |
| watch pass fusion and subset indexing | 35.8 | 15.2 |
| retain compaction reduction | 89.7 | 38.1 |
| connector watch set reuse | 57.22 | 24.3 |
| other or unclassified | 52.82 | 22.4 |

total corrected watch-scan bucket는 235.54 ms다.

### both_on, multi_comb_rect 512 LOCAL
| subaxis | ms | reported share pct |
| --- | --- | --- |
| watch pass fusion and subset indexing | 11.51 | 22.7 |
| retain compaction reduction | 24.19 | 47.7 |
| connector watch set reuse | 19.42 | 38.3 |

total corrected watch-scan bucket는 50.72 ms다.

주의. recovered session note의 group subtotal이 bucket total을 약간 넘는다. 따라서 이 case는 additive subtotal보다는 residual ordering을 신뢰하는 것이 맞다. ordering은 `retain compaction reduction` > `connector watch set reuse` > `watch pass fusion and subset indexing`이다.

세 sampled case 모두에서 strict dominant는 없었고, largest residual은 `retain compaction reduction`이었다.

## watch handle scan volume counter before after 표

| case | duplicate full scan passes before | duplicate full scan passes after | reuse_watch_handle_full_scan_calls before | reuse_watch_handle_full_scan_calls after | reuse_watch_handle_full_scan_handles before | reuse_watch_handle_full_scan_handles after | wscan_used_connectorWatchEntryIds_fastpath_calls after | wscan_used_preservedHandleIdxs_fastpath_calls after |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 324 | 0 | 83,792 | 20,948 | 19,752,074 | 4,979,394 | 20,948 | 20,948 |
| both_on, comb_rect_dense 512 LOCAL | 306 | 0 | 84,689 | 21,845 | 20,045,778 | 5,210,828 | 21,845 | 21,845 |
| both_on, multi_comb_rect 512 LOCAL | 288 | 1 | 26,721 | 7,029 | 4,859,171 | 1,287,048 | 6,999 | 7,029 |

## reuse apply route proxy와 broader residual 유지 표

| case | connector_skeleton route share pct | delta_preserved_then_skeleton route share pct | corrected watch scan share pct | broader watch churn share pct |
| --- | --- | --- | --- | --- |
| connector_only, comb_rect_dense 512 LOCAL | 100.0 | 0.0 | 18.2 | 41.2 |
| both_on, comb_rect_dense 512 LOCAL | 79.6 | 20.4 | 17.4 | 40.4 |
| both_on, multi_comb_rect 512 LOCAL | 51.3 | 48.7 | 11.0 | 34.5 |

route shape는 여전히 connector_skeleton 중심이다. 그러나 broader measured residual에서는 corrected watch-scan bucket보다 `watch churn` share가 더 크다. 다만 이번 round 프롬프트 기준은 watch bucket 내부 next pivot 판정이므로 final next pivot은 `retain compaction reduction`으로 둔다.

## top K slow deletion 요약

### connector_only, comb_rect_dense 512 LOCAL

Top slow deletions remained mostly connector_skeleton route; duplicate full scan passes were effectively gone, but retain removed handles, retain slotpos fixups, and existing connector-set maintenance still stayed large.

### both_on, comb_rect_dense 512 LOCAL

Top slow deletions were split between connector_skeleton and delta_preserved_then_skeleton routes; duplicate scan work mostly disappeared, while retain compaction and existing connector-set handling remained the main residuals.

### both_on, multi_comb_rect 512 LOCAL

The multi case still showed the same qualitative shift: duplicate full scans were almost eliminated, but residual time clustered around retain compaction and connector-set maintenance.

정리하면 topK residual은 duplicate full scan보다 `wscan_retain_removed_handles`, `wscan_retain_slotpos_fixups`, `wscan_existing_connector_set_handles_scanned` 쪽으로 이동했다.

## release와 representative

| tag | elapsed_solver_s | rc | timed_out | validator_ok | note |
| --- | --- | --- | --- | --- | --- |
| after_both_on_dense_1024_release | 335.23 | 0 | False | True | Recovered successfully in progress12; this case timed out in progress11. |
| after_both_on_multi_1024_release | 18.3 | 0 | False | True | Recovered successfully. |
| after_both_on_dense_4096_release | 240.16 | 1 | False | False | Representative dense 4096 still fails with rc=1 and no output. |
| after_both_on_multi_4096_release | 240.19 | 124 | True | False | Representative multi 4096 still times out. |

중요한 변화는 `both_on, comb_rect_dense 1024 RELEASE`가 이번 라운드에서 validator OK로 회수됐다는 점이다. progress11에서는 이 case가 420초 timeout이었다.

## 최종 residual cost 판정

watch bucket 내부에서는 strict dominant가 없다. 그래도 three-case 모두에서 largest residual은 `retain compaction reduction`이다.

broader measured residual 관점에서는 `watch churn`이 가장 큰 축으로 남지만, 50퍼센트를 넘는 strict dominant는 아니고 이번 round의 문제 설정은 watch-scan bucket 내부 pivot 판정이었으므로 다음 우선순위는 retain compaction reduction으로 두는 것이 맞다.

## 마지막 결론

`next pivot after watch scan round: retain compaction reduction`