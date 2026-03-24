# Progress23 previous-state seed and carry reuse report

## Status

This package is partial and partly reconstructed. The source branch was rebuilt from the progress22 base, the new `time_pcarry_*` and `pcarry_*` metrics were wired into LOCAL summary and slow deletion export, and a tiny sampled smoke plus clean gate reruns were executed in the current session. Full clean LOCAL 512 matrix and the aggregate bucket conclusion below come from the interrupted progress23 session note and are carried forward explicitly as reconstructed data.

Source artifact: `boj28350_literature_progress23_prev_state_seed_carry_reuse.cpp`

## Tiny sampled smoke

Sampled smoke `smoke_p23_multi64_full` was validator OK with elapsed 1.81s.

Confirmed live keys:

`time_pcarry_boundary_seed_bootstrap_ns=2893`
`time_pcarry_prev_scalar_materialize_ns=0`
`time_pcarry_prev_scalar_pack_ns=0`
`time_pcarry_carry_reuse_check_ns=0`
`time_pcarry_carry_reuse_hit_apply_ns=0`
`time_pcarry_carry_invalidate_transition_ns=0`
`time_pcarry_reseed_after_invalidate_ns=0`
`pcarry_prev_seed_calls=5`
`pcarry_carry_reuse_hits=0`

## Clean gate reruns

| case | validator_ok | elapsed_sec |
| --- | --- | ---: |
| gate_connector_only_dense_256_after | True | 3.92 |
| gate_both_on_dense_256_after | True | 3.88 |
| gate_both_on_multi_512_after | True | 6.66 |

All rerun gates preserved the required invariants.

## Clean LOCAL 512 elapsed carried from interrupted session note

| case | elapsed_sec |
| --- | ---: |
| before_connector_only_dense_512_base | 49.21 |
| before_both_on_dense_512_base | 46.22 |
| before_connector_only_dense_512_sampled | 47.73 |
| before_both_on_dense_512_sampled | 47.5 |
| before_both_on_multi_512_sampled | 8.17 |
| after_connector_only_dense_512_base | 48.53 |
| after_both_on_dense_512_base | 48.91 |
| after_connector_only_dense_512_sampled | 48.1 |
| after_both_on_dense_512_sampled | 48.38 |
| after_both_on_multi_512_sampled | 8.41 |

## Current clean LOCAL sampled aggregate for prev-state seed and carry bucket

| category | after_ms | share_pct |
| --- | ---: | ---: |
| boundary seed bootstrap and refresh | 1.177 | 2.2 |
| previous-state scalar materialize and pack | 25.275 | 48.1 |
| carry reuse check and hit fast path | 25.851 | 49.2 |
| carry invalidation and reseed fallback | 0.291 | 0.6 |

There is no strict dominant, but the largest residual is `carry reuse check and hit fast path`.

## Release status

Current session could not complete authoritative clean release recovery. The interrupted progress23 note recorded `both_on, multi_comb_rect 1024 RELEASE` as validator OK at 35.13s, while `both_on, comb_rect_dense 1024 RELEASE` was still in progress when the environment reset. No authoritative progress23 dense 1024 or 4096 representative rows survive in this package.

## Current conclusion

`next pivot after prev-state round: carry reuse check and hit fast path`