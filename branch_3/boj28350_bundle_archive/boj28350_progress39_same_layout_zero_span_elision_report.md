# boj28350 progress39 same-layout reuse and zero-span elision

## What changed

Progress38 base에 `ENABLE_LAYOUT_REUSE_ZERO_ELISION_OPT` 브랜치를 추가했고 `time_lreuse_*`, `lreuse_*` 계열을 source와 LOCAL summary까지 연결했다.

새 direct subaxis는 아래다.

`time_lreuse_layout_sig_load_ns`  
`time_lreuse_layout_sig_compare_ns`  
`time_lreuse_zero_span_scan_ns`  
`time_lreuse_zero_span_segment_reuse_ns`  
`time_lreuse_skip_apply_commit_ns`  
`time_lreuse_noop_fastpath_commit_ns`  
`time_lreuse_connector_hotpath_reuse_ns`

support artifact도 progress39 이름으로 분리했다.

`run_progress39_case_supervised.py`  
`progress39_finalize_case.py`  
`merge_progress39_results.py`  
`progress39_case_journal.jsonl`  
`progress39_resume_remaining.sh`

## Execution layer status

Detached supervised launch로 terminal `result.json`을 남기는 경로는 짧은 케이스와 sampled one-off에서 확인했다.

검증된 케이스는 아래다.

`smoke_both_dense_256_sampled_after` validator OK, elapsed 29.773s  
`gate_both_on_multi_512_after` validator OK, elapsed 62.376s

다만 long run terminal row persistence는 아직 authoritative하게 닫히지 않았다. dense 1024 release and repeat, 4096 representative는 여전히 missing이다.

## Smoke and gate

`smoke_both_dense_256_sampled_after` validator OK, elapsed 29.773s  
`gate_connector_only_dense_256_after` validator OK, elapsed 29.175s  
`gate_both_on_dense_256_after` validator OK, elapsed 29.886s  
`gate_both_on_multi_512_after` validator OK, elapsed 62.376s

## Direct same-layout reuse and zero-span elision aggregate

현재 direct aggregate는 아래 3개 sampled after row 기준이다.

`direct_connector_only_dense_256_sampled_after`  
`smoke_both_dense_256_sampled_after`  
`gate_both_on_multi_512_after`

| category | aggregate_ms | share_pct |
| --- | ---: | ---: |
| layout signature compare and reuse gate | 0.083838 | 33.9776 |
| zero-span boundary detect and segment reuse | 0.081822 | 33.1606 |
| skip apply commit and no-op fast path | 0.081077 | 32.8586 |
| connector hotpath normalize reuse | 0.000008 | 0.0032 |

## Current conclusion

현재 partial direct aggregate 기준 strict dominant는 없지만 largest residual은 `layout signature compare and reuse gate`다.

`next pivot after layout-reuse round: layout signature compare and reuse gate`

## Remaining missing rows

`progress38_authoritative_close_not_completed`  
`progress39_local_512_matrix_not_completed`  
`both_on_dense_1024_release`  
`both_on_dense_1024_release_repeat`  
`both_on_multi_1024_release`  
`both_on_dense_4096_release`  
`both_on_multi_4096_release`

## Current package state

Progress39 source patch, support artifact, smoke, and gate는 확보했다.  
Direct layout reuse aggregate도 partial sampled basis로 계산했다.  
하지만 progress38 carry-forward close, progress39 full LOCAL 512 matrix, dense 1024 release and repeat, 4096 representative는 아직 partial authoritative 상태다.
