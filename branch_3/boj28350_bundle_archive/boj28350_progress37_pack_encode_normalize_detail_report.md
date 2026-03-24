# boj28350 progress37 pack encode and normalize detail

## What changed

Progress36 base에 `ENABLE_PACK_ENCODE_NORMALIZE_CORE_OPT` 브랜치를 추가했고 `time_pencore_*`, `pencore_*` 계열을 source와 LOCAL summary에 연결했다.

새 direct subaxis는 아래다.

`time_pencore_descriptor_encode_ns`  
`time_pencore_payload_pack_write_ns`  
`time_pencore_normalize_rule_apply_ns`  
`time_pencore_postnormalize_compare_ns`  
`time_pencore_same_layout_skip_ns`  
`time_pencore_connector_hotpath_apply_ns`

## Smoke and gate

`smoke_both_dense_256_sampled_after` validator OK, elapsed 3.557s  
`gate_connector_only_dense_256_after` validator OK, elapsed 3.426s  
`gate_both_on_dense_256_after` validator OK, elapsed 3.501s  
`gate_both_on_multi_512_after` validator OK, elapsed 5.004s

## LOCAL 512 before and after elapsed

| case | before base | after base | before sampled | after sampled |
| --- | ---: | ---: | ---: | ---: |
| connector_only, comb_rect_dense 512 LOCAL | 28.998s | 28.173s | 29.468s | 29.163s |
| both_on, comb_rect_dense 512 LOCAL | 29.192s | 28.812s | 29.685s | 28.964s |
| both_on, multi_comb_rect 512 LOCAL | n.a. | n.a. | 5.004s | 5.004s |

## Direct pack encode and normalize core aggregate

| category | aggregate_ms | share_pct |
| --- | ---: | ---: |
| descriptor encode and field-order resolve | 0.190462 | 19.9990 |
| payload pack write core | 0.197307 | 20.7178 |
| canonical normalize and zero-fill | 0.374885 | 39.3639 |
| post-normalize compare and no-op suppression | 0.189703 | 19.9193 |
| connector hotpath apply | 0.000054 | 0.0057 |

## 1024 RELEASE

`both_on, multi_comb_rect 1024 RELEASE` validator OK, elapsed 20.009s  
`both_on, comb_rect_dense 1024 RELEASE` missing  
`both_on, comb_rect_dense 1024 RELEASE repeat` missing

## Representative 4096

`both_on, comb_rect_dense 4096 RELEASE` missing  
`both_on, multi_comb_rect 4096 RELEASE` missing

## Current conclusion

현재 authoritative clean LOCAL direct aggregate 기준 largest residual은 `canonical normalize and zero-fill`다. strict dominant는 없고 aggregate share는 39.3639퍼센트다.


`next pivot after encode-normalize-detail round: canonical normalize and zero-fill`


## Remaining missing rows

`progress36_authoritative_close_not_completed`  
`both_on_dense_1024_release`  
`both_on_dense_1024_release_repeat`  
`both_on_dense_4096_release`  
`both_on_multi_4096_release`  
