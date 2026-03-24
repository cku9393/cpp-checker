# progress26 prev-state writeback and scalar store

이번 패키지는 partial authoritative 상태다. clean gate 3개와 clean LOCAL 512 before/after 전체, 그리고 `both_on, multi_comb_rect 1024 RELEASE` 1회를 authoritative하게 회수했다. 반면 `both_on, comb_rect_dense 1024 RELEASE`와 dense repeat, 4096 representative 두 케이스는 이번 패키지 시점에서 authoritative row를 만들지 못했다.

## 무엇을 바꿨는지

`time_chit_*` umbrella 안에서 `prev-state writeback and scalar store`를 다시 쪼개기 위해 아래 exclusive subaxis를 추가했다.

`time_pwrite_noop_guard_ns`, `time_pwrite_target_resolve_ns`, `time_pwrite_scalar_field_store_ns`, `time_pwrite_compact_pack_store_ns`, `time_pwrite_same_value_skip_ns`, `time_pwrite_route_pointer_hoist_miss_ns`

동시에 `pwrite_*` volume counter와 slow deletion export도 연결했다. 다만 current clean rerun에서는 `pwrite_calls`와 route call 계열이 일부 0으로 남아 있어, route 판정은 route time과 slow deletion row를 기준으로 읽는 편이 안전하다.

## correctness gate

| run | elapsed_sec | validator |
| --- | ---: | --- |
| gate_connector_only_dense_256_after | 3.81 | OK |
| gate_both_on_dense_256_after | 3.53 | OK |
| gate_both_on_multi_512_after | 5.15 | OK |

모든 gate에서 `local_active_mismatch=0`, `local_active_partition_mismatch=0`, `debug_touched_missing_classes=0`, `piece_materialize_fallback_calls=0`, `support_rebuild_fallback_calls=0`, `unanimous_baseline_path_calls=0`를 유지했다.

## 512 LOCAL before/after

| case | before_base_s | after_base_s | before_sampled_s | after_sampled_s |
| --- | ---: | ---: | ---: | ---: |
| connector_only, comb_rect_dense 512 | 36.01 | 36.85 | 37.88 | 37.40 |
| both_on, comb_rect_dense 512 | 36.30 | 37.99 | 38.49 | 39.04 |
| both_on, multi_comb_rect 512 | - | - | 5.73 | 5.66 |

## prev-state writeback and scalar store exclusive subaxis

| case | guard_ms | scalar_store_ms | compact_pack_ms | target_resolve_ms | total_ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| connector_only, comb_rect_dense 512 | 0.001 | 0.008 | 0.001 | 0.004 | 0.014 |
| both_on, comb_rect_dense 512 | 4.166 | 12.788 | 3.817 | 16.543 | 37.314 |
| both_on, multi_comb_rect 512 | 0.562 | 1.817 | 0.548 | 2.372 | 5.298 |

### after sampled aggregate

| category | aggregate_ms | share_pct |
| --- | ---: | ---: |
| writeback guard and same-value suppression | 4.730 | 11.1 |
| prev-state scalar field update core | 14.613 | 34.3 |
| compact pack and store normalize | 4.365 | 10.2 |
| target resolve and route-local pointer pinning | 18.919 | 44.4 |

strict dominant는 없지만 largest residual은 `target resolve and route-local pointer pinning`이다. 따라서 progress25 clean LOCAL 결론인 `prev-state writeback and scalar store`는 progress26 clean rerun에서 다시 안쪽으로 뒤집혔다.

## volume counter 관찰

| case | target_resolve_calls | target_resolve_cache_hits | scalar_field_store_calls | compact_pack_store_calls | same_value_skip_hits |
| --- | ---: | ---: | ---: | ---: | ---: |
| connector_only, comb_rect_dense 512 | 0 | 0 | 0 | 0 | 0 |
| both_on, comb_rect_dense 512 | 0 | 0 | 0 | 0 | 0 |
| both_on, multi_comb_rect 512 | 890 | 0 | 890 | 890 | 0 |

`same-value skip`는 현재 clean rerun에서 사실상 0으로 남았고, `target_resolve_calls`와 `scalar_field_store_calls`가 거의 같은 크기로 따라간다. 따라서 hit path에서 guard miss보다는 target resolve와 actual store 쪽이 시간의 중심이다.

## top K slow deletion 요약

| source_tag | idx | pwrite_total_ns | target_resolve_calls | scalar_field_store_calls | compact_pack_store_calls | same_value_skip_hits |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| after_both_on_dense_512_sampled | 33 | 1310774 | 5657 | 5657 | 5657 | 0 |
| after_both_on_multi_512_sampled | 21 | 544951 | 2480 | 2480 | 2480 | 0 |
| after_both_on_multi_512_sampled | 37 | 496949 | 2213 | 2213 | 2213 | 0 |
| after_both_on_multi_512_sampled | 17 | 472183 | 2198 | 2198 | 2198 | 0 |
| after_both_on_multi_512_sampled | 25 | 437883 | 1981 | 1981 | 1981 | 0 |

## release and representative

| run | status | elapsed_sec | note |
| --- | --- | ---: | --- |
| both_on, multi_comb_rect 1024 RELEASE | validator OK | 28.00 | clean rerun 1회 |
| both_on, comb_rect_dense 1024 RELEASE | missing | - | current session에서 authoritative row 생성 실패 |
| both_on, comb_rect_dense 1024 RELEASE repeat | missing | - | current session에서 authoritative row 생성 실패 |
| both_on, comb_rect_dense 4096 RELEASE | missing | - | current session에서 authoritative clean rerun 없음 |
| both_on, multi_comb_rect 4096 RELEASE | missing | - | current session에서 authoritative clean rerun 없음 |

## progress25 대비 결론 비교

progress25 clean LOCAL 결론은 `prev-state writeback and scalar store`였다.
이번 progress26 clean LOCAL rerun에서는 그 버킷 안을 다시 쪼개 보니 largest residual이 `target resolve and route-local pointer pinning`으로 바뀌었다.

## 최종 residual cost 판정

partial authoritative clean LOCAL 기준으로는 `target resolve and route-local pointer pinning`이 현재 가장 큰 residual이다. broader measured residual로는 여전히 `watch churn`이 더 크지만, 이번 라운드의 문제 설정은 carry-hit bucket 내부 next pivot 확정이다.

## 마지막 결론

`next pivot after prev-writeback round: target resolve and route-local pointer pinning`