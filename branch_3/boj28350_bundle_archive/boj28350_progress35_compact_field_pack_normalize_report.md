# boj28350 progress35 compact field pack and normalize

## What changed
Progress34 base에 `ENABLE_COMPACT_FIELD_PACK_OPT` 브랜치를 추가했고 `time_cpack_*`, `cpack_*` 계열을 source와 LOCAL summary에 연결했다.

새 direct subaxis는 아래다.

`time_cpack_same_layout_check_ns`
`time_cpack_same_layout_skip_apply_ns`
`time_cpack_field_gather_ns`
`time_cpack_staging_buffer_prepare_ns`
`time_cpack_pack_encode_ns`
`time_cpack_pack_normalize_ns`
`time_cpack_connector_hotpath_pack_ns`

support artifact도 progress35 기준으로 만들었다.

`run_progress35_case_supervised.py`
`progress35_finalize_case.py`
`merge_progress35_results.py`
`progress35_case_journal.jsonl`
`progress35_resume_remaining.sh`

현재 분해는 actual pack site에서 직접 계측하지만, `field gather`, `pack encode`, `normalize`는 같은 hot loop 안에서 staged split으로 나눈 값이다. 즉 완전히 서로 다른 raw microkernel timer까지는 아니다.

## Smoke and gate
`smoke_both_dense_256_sampled_after` validator OK, elapsed 3.380s
`gate_connector_only_dense_256_after` validator OK, elapsed 3.486s
`gate_both_on_dense_256_after` validator OK, elapsed 3.377s
`gate_both_on_multi_512_after` validator OK, elapsed 4.996s

## LOCAL 512 before and after elapsed
| case | before base | after base | before sampled | after sampled |
| --- | ---: | ---: | ---: | ---: |
| connector_only, comb_rect_dense 512 LOCAL | 27.619s | 31.397s | 28.936s | 31.583s |
| both_on, comb_rect_dense 512 LOCAL | 31.146s | 32.413s | 32.199s | 32.088s |
| both_on, multi_comb_rect 512 LOCAL | n.a. | n.a. | 8.748s | 8.990s |

## Direct compact field pack and normalize aggregate
| category | aggregate_ms | share_pct |
| --- | ---: | ---: |
| same-layout reuse and pack skip | 0.408083 | 20.6040 |
| field bundle gather and staging | 0.601163 | 30.3526 |
| pack encode and normalize core | 0.971298 | 49.0406 |
| connector hotpath pack apply | 0.000054 | 0.0027 |

## 1024 RELEASE
`both_on, multi_comb_rect 1024 RELEASE` validator OK, elapsed 24.201s
`both_on, comb_rect_dense 1024 RELEASE` missing
`both_on, comb_rect_dense 1024 RELEASE repeat` missing

## Representative 4096
`both_on, comb_rect_dense 4096 RELEASE` missing
`both_on, multi_comb_rect 4096 RELEASE` missing

## Current conclusion
현재 authoritative clean LOCAL direct aggregate 기준 largest residual은 `pack encode and normalize core`다. strict dominant는 없고 aggregate share는 49.0406퍼센트다.

`next pivot after compact-pack round: pack encode and normalize core`

## Remaining missing rows
`progress34_authoritative_close_not_completed`
`both_on_dense_1024_release`
`both_on_dense_1024_release_repeat`
`both_on_dense_4096_release`
`both_on_multi_4096_release`
