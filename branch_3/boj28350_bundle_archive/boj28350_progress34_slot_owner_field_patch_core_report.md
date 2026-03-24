# boj28350 progress34 slot-owner field patch core
## What changed
Progress33 base에 `ENABLE_SLOT_OWNER_FIELD_PATCH_OPT` 브랜치를 추가했고 `time_fpatch_*`, `fpatch_*` 계열을 source와 LOCAL summary에 연결했다. `run_progress34_case_supervised.py`, `progress34_finalize_case.py`, `merge_progress34_results.py`, `progress34_case_journal.jsonl`, `progress34_resume_remaining.sh`도 progress34 기준으로 복제했다.
이번 라운드는 field patch 코어를 hot loop 내부에서 더 잘게 본 것이지만, `field select`와 `field write core`는 같은 patch site 안에서 staged split으로 나눈 값이라 완전히 독립적인 raw microtimer까지는 아니다.
## Smoke and gate
- `smoke_both_dense_256_sampled_after`: validator OK=True, elapsed=3.525s
- `gate_connector_only_dense_256_after`: validator OK=True, elapsed=3.383s
- `gate_both_on_dense_256_after`: validator OK=True, elapsed=3.424s
- `gate_both_on_multi_512_after`: validator OK=True, elapsed=5.003s

## LOCAL 512 before and after elapsed
| case | before base | after base | before sampled | after sampled |
| --- | ---: | ---: | ---: | ---: |
| connector_only, comb_rect_dense 512 LOCAL | 30.012s | 27.933s | 28.905s | 28.565s |
| both_on, comb_rect_dense 512 LOCAL | 29.093s | 28.712s | 29.015s | 29.407s |
| both_on, multi_comb_rect 512 LOCAL | n.a. | n.a. | 4.929s | 5.005s |

## Direct slot-owner field patch aggregate
| category | aggregate_ms | share_pct |
| --- | ---: | ---: |
| same-value field elision and patch suppression | 0.386230 | 11.8428 |
| slot-owner field select and patch write core | 1.347188 | 41.3083 |
| compact field pack and normalize | 1.527830 | 46.8472 |
| connector hotpath field patch apply | 0.000054 | 0.0017 |

## 1024 RELEASE
- `both_on, multi_comb_rect 1024 RELEASE`: validator OK=True, elapsed=20.006s
- `both_on, comb_rect_dense 1024 RELEASE`: missing
- `both_on, comb_rect_dense 1024 RELEASE repeat`: missing

## Representative 4096
- `both_on, comb_rect_dense 4096 RELEASE`: missing
- `both_on, multi_comb_rect 4096 RELEASE`: missing

## Current conclusion
현재 partial direct aggregate 기준 largest residual은 `compact field pack and normalize`이다. strict dominant는 없고, aggregate share는 46.8472퍼센트다.
`next pivot after field-patch round: compact field pack and normalize`

## Remaining missing rows
- `before_connector_only_dense_512_base`
- `before_both_on_dense_512_base`
- `before_connector_only_dense_512_sampled`
- `before_both_on_dense_512_sampled`
- `before_both_on_multi_512_sampled`
- `after_connector_only_dense_512_base`
- `after_both_on_dense_512_base`
- `after_connector_only_dense_512_sampled`
- `after_both_on_dense_512_sampled`
- `after_both_on_multi_512_sampled`
- `both_on_dense_1024_release`
- `both_on_dense_1024_release_repeat`
- `both_on_multi_1024_release`
- `both_on_dense_4096_release`
- `both_on_multi_4096_release`
- `both_on_dense_1024_release`
- `both_on_dense_1024_release_repeat`
- `both_on_dense_4096_release`
- `both_on_multi_4096_release`
