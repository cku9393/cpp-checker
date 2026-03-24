# Progress24 carry reuse check and hit fast path report

## Status

This package is partial authoritative. It includes clean gate reruns, a tiny sampled smoke confirming that the new `time_creuse_*` and `creuse_*` keys are live, full clean LOCAL 512 before and after, and a clean multi 1024 release rerun. Authoritative dense 1024 release and authoritative clean 4096 representative reruns are still missing in this package.

Source artifact: `boj28350_literature_progress24_carry_reuse_check_hit_fastpath.cpp`

## Tiny sampled smoke

Sampled smoke `smoke_p24_multi64` was validator OK with elapsed 2.69s.

Confirmed live keys:

`time_creuse_eligibility_gate_ns=0`
`time_creuse_prev_scalar_eq_check_ns=0`
`time_creuse_transition_consistency_check_ns=0`
`time_creuse_hit_apply_cursor_advance_ns=0`
`time_creuse_miss_classify_ns=0`
`time_creuse_reseed_handoff_ns=0`
`creuse_checks=0`
`creuse_hits=0`
`creuse_misses=0`

## Clean gate reruns

| case | validator_ok | elapsed_sec |
| --- | --- | ---: |
| gate_connector_only_dense_256_after | True | 3.83 |
| gate_both_on_dense_256_after | True | 3.86 |
| gate_both_on_multi_512_after | True | 5.43 |

All rerun gates preserved the required invariants.

## Clean LOCAL 512 elapsed

| case | elapsed_sec |
| --- | ---: |
| before_connector_only_dense_512_base | 25.84 |
| before_both_on_dense_512_base | 28.93 |
| before_connector_only_dense_512_sampled | 29.01 |
| before_both_on_dense_512_sampled | 29.21 |
| before_both_on_multi_512_sampled | 5.41 |
| after_connector_only_dense_512_base | 29.09 |
| after_both_on_dense_512_base | 29.21 |
| after_connector_only_dense_512_sampled | 28.56 |
| after_both_on_dense_512_sampled | 28.83 |
| after_both_on_multi_512_sampled | 5.35 |

## Authoritative LOCAL sampled aggregate for carry reuse check and hit fast path bucket

| category | before_ms | after_ms | after_share_pct |
| --- | ---: | ---: | ---: |
| eligibility gate and boundary validity | 10.890 | 10.898 | 24.9 |
| previous-state equality compare and transition consistency | 10.716 | 10.681 | 24.4 |
| carry hit apply and loop cursor advance | 16.402 | 21.865 | 50.0 |
| miss classify and reseed handoff | 22.003 | 0.285 | 0.7 |

There is no strict dominant under the 50 percent rule, but the largest residual is `carry hit apply and loop cursor advance` at 50.0 percent of the clean after aggregate.

## After sampled per-case carry-reuse breakdown

| case | eligibility_ms | compare_ms | hit_apply_ms | miss_ms | total_ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| connector_only, comb_rect_dense 512 LOCAL | 0.005 | 0.004 | 0.012 | 0.005 | 0.026 |
| both_on, comb_rect_dense 512 LOCAL | 9.499 | 9.339 | 19.128 | 0.215 | 38.181 |
| both_on, multi_comb_rect 512 LOCAL | 1.394 | 1.338 | 2.725 | 0.065 | 5.522 |

## Clean LOCAL sampled aggregate volume excerpt

| key | value |
| --- | ---: |
| creuse_checks | 216744 |
| creuse_hits | 214075 |
| creuse_misses | 2669 |
| creuse_boundary_validity_hits | 216744 |
| creuse_prev_scalar_eq_hits | 216744 |
| creuse_transition_consistency_hits | 214075 |
| creuse_hit_apply_cursor_advance_calls | 189191 |
| creuse_reseed_handoff_calls | 2221 |
| creuse_route_connector_skeleton_calls | 133616 |
| creuse_route_delta_preserved_then_skeleton_calls | 7641 |

## Release reruns completed so far

| case | rc | timed_out | validator_ok | elapsed_sec |
| --- | ---: | --- | --- | ---: |
| after_both_on_multi_1024_release | 0 | False | True | 19.77 |
| after_both_on_dense_1024_release | missing | missing | missing | missing |

## Remaining missing

- `after_both_on_dense_1024_release` authoritative clean completion
- dense 1024 repeat stability
- `after_both_on_dense_4096_release` authoritative clean rerun
- `after_both_on_multi_4096_release` authoritative clean rerun

## Current authoritative conclusion

`next pivot after carry-reuse round: carry hit apply and loop cursor advance`