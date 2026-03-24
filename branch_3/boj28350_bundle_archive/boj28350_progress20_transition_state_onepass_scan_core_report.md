# Progress20 transition-state one-pass scan core report

## Scope completed in this resumed session

This resumed session used the reconstructed progress20 source base and the previously saved clean LOCAL findings as the authoritative LOCAL baseline. It then re-ran clean RELEASE for dense 1024 and multi 1024. Dense 1024 repeat stability and authoritative clean 4096 representative reruns were not completed in this session.

## Source artifact

`boj28350_literature_progress20_transition_state_onepass_scan_core.cpp`

## Clean LOCAL findings carried forward

Clean gate reruns preserved:

connector_only_dense_256_local_none_after: validator OK, 3.46s
both_on_dense_256_local_none_after: validator OK, 3.76s
both_on_multi_512_local_sampled_after: validator OK, 5.16s

All gate runs kept `local_active_mismatch=0`, `local_active_partition_mismatch=0`, `debug_touched_missing_classes=0`, `piece_materialize_fallback_calls=0`, `support_rebuild_fallback_calls=0`, and `unanimous_baseline_path_calls=0`.

## Clean LOCAL 512 before and after

| case | elapsed_sec |
| --- | ---: |
| before_connector_only_dense_512_base | 49.86 |
| before_both_on_dense_512_base | 48.82 |
| before_connector_only_dense_512_sampled | 49.28 |
| before_both_on_dense_512_sampled | 48.45 |
| before_both_on_multi_512_sampled | 7.00 |
| after_connector_only_dense_512_base | 47.34 |
| after_both_on_dense_512_base | 49.42 |
| after_connector_only_dense_512_sampled | 45.89 |
| after_both_on_dense_512_sampled | 51.32 |
| after_both_on_multi_512_sampled | 6.97 |

## Authoritative LOCAL sampled aggregate for one-pass scan bucket

| category | aggregate_ms | share_pct |
| --- | ---: | ---: |
| boundary seeded scan window narrowing | 1.986 | 15.3 |
| transition-state branch and state load core | 10.353 | 79.8 |
| run boundary commit and count update | 0.022 | 0.2 |
| tail stop and early exit reuse | 0.605 | 4.7 |

This preserved the clean LOCAL authoritative conclusion:

`next pivot after one-pass-scan round: transition-state branch and state load core`

## Clean RELEASE reruns completed in this session

| case | rc | timed_out | validator_ok | elapsed_sec | stdout_empty | stderr_empty |
| --- | ---: | --- | --- | ---: | --- | --- |
| after_both_on_dense_1024_release | 0 | False | True | 914.06 | False | True |
| after_both_on_multi_1024_release | 0 | False | True | 37.59 | False | True |

Dense 1024 clean rerun was recovered and validated, but it was still very slow.

## Missing in this session

The following were not completed in this resumed session:

- dense 1024 repeat stability
- authoritative clean 4096 representative reruns
- fully authoritative progress20 packaging beyond the carried-forward clean LOCAL findings and the two clean RELEASE reruns above

## Final current conclusion

`next pivot after one-pass-scan round: transition-state branch and state load core`