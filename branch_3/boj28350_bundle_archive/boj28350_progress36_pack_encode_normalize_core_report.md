# boj28350 progress36 pack encode and normalize core

## What changed

Progress35 base에 `ENABLE_PACK_ENCODE_NORMALIZE_OPT` 브랜치를 추가했고 `time_pnorm_*`, `pnorm_*` 계열을 source와 LOCAL summary에 연결했다.

기존 supervised runner와 finalize script를 progress36 버전으로 복사하고, zero-size input 재생성 guard를 추가했다.

새 direct subaxis는 아래다.

`time_pnorm_same_layout_skip_ns`  
`time_pnorm_bundle_gather_ns`  
`time_pnorm_staging_prepare_ns`  
`time_pnorm_encode_core_ns`  
`time_pnorm_normalize_core_ns`  
`time_pnorm_connector_hotpath_apply_ns`  

## Smoke and gate

`smoke_both_dense_256_sampled_after` validator OK, elapsed 3.528s  
`gate_connector_only_dense_256_after` validator OK, elapsed 3.475s  
`gate_both_on_dense_256_after` validator OK, elapsed 3.424s  
`gate_both_on_multi_512_after` validator OK, elapsed 4.929s  

## LOCAL 512 before and after elapsed

| case | before base | after base | before sampled | after sampled |
| --- | ---: | ---: | ---: | ---: |
| connector_only, comb_rect_dense 512 LOCAL | 27.255s | 25.013s | 27.093s | 25.010s |
| both_on, comb_rect_dense 512 LOCAL | 27.525s | 25.009s | 25.015s | 25.009s |
| both_on, multi_comb_rect 512 LOCAL | n.a. | n.a. | 5.006s | 5.006s |

## Direct pack encode and normalize aggregate

| category | aggregate_ms | share_pct |
| --- | ---: | ---: |
| same-layout reuse and pack skip | 0.000000 | 0.0000 |
| field bundle gather and staging | 0.573629 | 37.6472 |
| pack encode and normalize core | 0.950015 | 62.3493 |
| connector hotpath pack apply | 0.000054 | 0.0035 |

## 1024 RELEASE

`both_on, multi_comb_rect 1024 RELEASE` validator OK, elapsed 22.274s  
`both_on, comb_rect_dense 1024 RELEASE` missing  
`both_on, comb_rect_dense 1024 RELEASE repeat` missing  

## Representative 4096

`both_on, comb_rect_dense 4096 RELEASE` missing  
`both_on, multi_comb_rect 4096 RELEASE` missing  

## Current conclusion

현재 authoritative clean LOCAL direct aggregate 기준 largest residual은 `pack encode and normalize core`다. strict dominant는 없고 aggregate share는 62.3493퍼센트다.


`next pivot after pack-encode round: pack encode and normalize core`


## Remaining missing rows

`progress35_authoritative_close_not_completed`  
`both_on_dense_1024_release`  
`both_on_dense_1024_release_repeat`  
`both_on_dense_4096_release`  
`both_on_multi_4096_release`  

