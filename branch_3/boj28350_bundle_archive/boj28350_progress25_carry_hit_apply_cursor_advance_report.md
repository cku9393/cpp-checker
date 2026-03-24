# Progress25 carry hit apply and cursor advance report

## Status

This package is still partial. In this follow-up I completed the clean LOCAL 512 before/after matrix authoritatively and preserved the previously confirmed smoke and gate reruns. I did **not** complete the clean 1024 RELEASE reruns or the authoritative clean 4096 representative reruns in this environment.

Source artifact: `boj28350_literature_progress25_carry_hit_apply_cursor_advance.cpp`

## What changed in this follow-up

I reused the already patched source with the `ENABLE_CARRY_HIT_APPLY_OPT` flag and the `time_chit_*` and `chit_*` families. The new work in this follow-up was to rerun the full clean LOCAL 512 before/after matrix authoritatively and package those results so the carry-hit bucket can be judged from clean LOCAL data instead of the earlier smoke and gate-only partial package.

## Reused smoke and clean gate reruns

| case | validator_ok | elapsed_sec |
| --- | --- | ---: |
| smoke_p25_multi64 | True | 2.3 |
| gate_connector_only_dense_256_after | True | 3.99 |
| gate_both_on_dense_256_after | True | 4.59 |
| gate_both_on_multi_512_after | True | 6.68 |

## Authoritative clean LOCAL 512 matrix

| case | profile | delta | validator_ok | elapsed_sec |
| --- | --- | --- | --- | ---: |
| before_connector_only_dense_512_base | PROFILE_BASE | connector_only | True | 67.55 |
| before_both_on_dense_512_base | PROFILE_BASE | both_on | True | 68.54 |
| before_connector_only_dense_512_sampled | PROFILE_SAMPLED | connector_only | True | 66.23 |
| before_both_on_dense_512_sampled | PROFILE_SAMPLED | both_on | True | 67.06 |
| before_both_on_multi_512_sampled | PROFILE_SAMPLED | both_on | True | 9.49 |
| after_connector_only_dense_512_base | PROFILE_BASE | connector_only | True | 66.55 |
| after_both_on_dense_512_base | PROFILE_BASE | both_on | True | 65.75 |
| after_connector_only_dense_512_sampled | PROFILE_SAMPLED | connector_only | True | 66.19 |
| after_both_on_dense_512_sampled | PROFILE_SAMPLED | both_on | True | 65.24 |
| after_both_on_multi_512_sampled | PROFILE_SAMPLED | both_on | True | 8.56 |

## Carry-hit grouped aggregate from clean LOCAL sampled

### Before sampled aggregate

| category | aggregate_ms | share_pct |
| --- | ---: | ---: |
| prev state writeback and scalar store | 34.061 | 56.3 |
| cursor advance and next carry prepare | 13.237 | 21.9 |
| hit bookkeeping and metadata commit | 6.503 | 10.8 |
| fastpath guard and skip path suppression | 6.674 | 11.0 |

### After sampled aggregate

| category | aggregate_ms | share_pct |
| --- | ---: | ---: |
| prev state writeback and scalar store | 34.009 | 56.2 |
| cursor advance and next carry prepare | 13.269 | 21.9 |
| hit bookkeeping and metadata commit | 6.576 | 10.9 |
| fastpath guard and skip path suppression | 6.626 | 11.0 |

Clean LOCAL sampled aggregate after rerun shows a clear largest residual in `prev-state writeback and scalar store` at 56.2 percent of the carry-hit bucket. `cursor advance and next-carry prepare` is second at 21.9 percent, while `hit bookkeeping and metadata commit` and `fastpath guard and skip-path suppression` are both about 11 percent.

## Per-case clean LOCAL sampled grouped breakdown

### after_connector_only_dense_512_sampled

| category | ms | share_pct |
| --- | ---: | ---: |
| prev-state writeback and scalar store | 0.007 | 50.7 |
| cursor advance and next-carry prepare | 0.005 | 34.4 |
| hit bookkeeping and metadata commit | 0.001 | 7.3 |
| fastpath guard and skip-path suppression | 0.001 | 7.6 |

### after_both_on_dense_512_sampled

| category | ms | share_pct |
| --- | ---: | ---: |
| prev-state writeback and scalar store | 29.704 | 56.2 |
| cursor advance and next-carry prepare | 11.586 | 21.9 |
| hit bookkeeping and metadata commit | 5.745 | 10.9 |
| fastpath guard and skip-path suppression | 5.784 | 11.0 |

### after_both_on_multi_512_sampled

| category | ms | share_pct |
| --- | ---: | ---: |
| prev-state writeback and scalar store | 4.298 | 56.2 |
| cursor advance and next-carry prepare | 1.679 | 22.0 |
| hit bookkeeping and metadata commit | 0.829 | 10.8 |
| fastpath guard and skip-path suppression | 0.841 | 11.0 |

## Release and representative status

These reruns are still missing from the authoritative package in this environment:

- `both_on, comb_rect_dense 1024 RELEASE`
- `both_on, comb_rect_dense 1024 RELEASE` repeat stability
- `both_on, multi_comb_rect 1024 RELEASE` clean rerun for progress25
- authoritative clean `both_on, comb_rect_dense 4096 RELEASE`
- authoritative clean `both_on, multi_comb_rect 4096 RELEASE`

## Current authoritative clean LOCAL conclusion

`next pivot after carry-hit round: prev-state writeback and scalar store`