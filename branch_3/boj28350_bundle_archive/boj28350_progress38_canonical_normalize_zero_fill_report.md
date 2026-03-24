# boj28350 progress38 canonical normalize and zero-fill

## What changed

Progress37 base에 `ENABLE_CANONICAL_NORMALIZE_OPT` 브랜치를 추가했고 `time_cnorm_*`, `cnorm_*` 계열을 source와 LOCAL summary에 연결했다.

새 direct subaxis는 아래다.

`time_cnorm_same_layout_reuse_check_ns`  
`time_cnorm_zero_span_elision_ns`  
`time_cnorm_rule_dispatch_ns`  
`time_cnorm_field_normalize_apply_ns`  
`time_cnorm_zero_fill_span_write_ns`  
`time_cnorm_tail_zero_clear_ns`  
`time_cnorm_connector_hotpath_apply_ns`

support artifact도 progress38 이름으로 분리했다.

`run_progress38_case_supervised.py`  
`progress38_finalize_case.py`  
`merge_progress38_results.py`  
`progress38_case_journal.jsonl`  
`progress38_resume_remaining.sh`

## Smoke and gate

`smoke_both_dense_256_sampled_after` validator OK, elapsed 3.739s  
`gate_connector_only_dense_256_after` validator OK, elapsed 3.432s  
`gate_both_on_dense_256_after` validator OK, elapsed 3.378s  
`gate_both_on_multi_512_after` validator OK, elapsed 5.004s

## LOCAL 512 before and after elapsed

| case | before base | after base | before sampled | after sampled |
| --- | ---: | ---: | ---: | ---: |
| connector_only, comb_rect_dense 512 LOCAL | 30.011s | 29.243s | 30.012s | 30.011s |
| both_on, comb_rect_dense 512 LOCAL | 27.373s | 33.246s | 32.400s | 33.305s |
| both_on, multi_comb_rect 512 LOCAL | n.a. | n.a. | 5.005s | 5.004s |

## Direct canonical normalize and zero-fill aggregate

| category | aggregate_ms | share_pct |
| --- | ---: | ---: |
| same-layout reuse and zero-span elision | 0.492479 | 39.3447 |
| normalize rule dispatch and field apply core | 0.379585 | 30.3255 |
| zero-fill span write and tail clear | 0.379586 | 30.3255 |
| connector hotpath normalize apply | 0.000054 | 0.0043 |

## 1024 RELEASE

`both_on, multi_comb_rect 1024 RELEASE` validator OK, elapsed 25.011s  
`both_on, comb_rect_dense 1024 RELEASE` missing  
`both_on, comb_rect_dense 1024 RELEASE repeat` missing

## Representative 4096

`both_on, comb_rect_dense 4096 RELEASE` missing  
`both_on, multi_comb_rect 4096 RELEASE` missing

## Current conclusion

현재 authoritative clean LOCAL direct aggregate 기준 largest residual은 `same-layout reuse and zero-span elision`이다. strict dominant는 아니고 aggregate share는 39.3447퍼센트다.

`next pivot after canonical-normalize round: same-layout reuse and zero-span elision`

## Remaining missing rows

`progress37_authoritative_close_not_completed`  
`both_on_dense_1024_release`  
`both_on_dense_1024_release_repeat`  
`both_on_dense_4096_release`  
`both_on_multi_4096_release`

## Current package state

LOCAL 512 matrix는 authoritative하게 다시 회수했다.  
`both_on, multi_comb_rect 1024 RELEASE`는 authoritative 1회 확보했다.  
하지만 dense 1024 release and repeat, 4096 representative는 여전히 partial authoritative 상태다.  
execution layer는 short and medium case terminal row를 남기지만 long run terminal row persistence는 아직 완전히 닫히지 않았다.
