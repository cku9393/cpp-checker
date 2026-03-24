# Progress27 target resolve and route-local pointer pinning report

This package is a **partial authoritative** continuation from progress26. It includes a rebuilt source branch, successful smoke and correctness gate reruns, and clean LOCAL sampled reruns for the three key 512 cases. RELEASE and 4096 representative reruns remain incomplete in this package.

## What changed

The progress27 source was branched from progress26 and extended to emit `tresolve_*` summary keys. In this attempt, the `target resolve and route-local pointer pinning` decomposition is derived from the existing `pwrite_*` timing and counter surface rather than a fully separate hot-loop instrumentation path. The keys are now present in sampled output and can be aggregated across the clean LOCAL reruns.

## Smoke and gate

- `gate_connector_only_dense_256_after`: validator OK=True, elapsed=2.78s
- `gate_both_on_dense_256_after`: validator OK=True, elapsed=2.73s
- `gate_both_on_multi_512_after`: validator OK=True, elapsed=4.65s

All completed gate reruns preserved `local_active_mismatch=0`, `local_active_partition_mismatch=0`, `debug_touched_missing_classes=0`, `piece_materialize_fallback_calls=0`, `support_rebuild_fallback_calls=0`, and `unanimous_baseline_path_calls=0` in the emitted summary keys.

## Clean LOCAL 512 sampled reruns

- `before_connector_only_dense_512_sampled`: validator OK=True, elapsed=35.13s
- `before_both_on_dense_512_sampled`: validator OK=True, elapsed=34.51s
- `before_both_on_multi_512_sampled`: validator OK=True, elapsed=4.64s
- `after_connector_only_dense_512_sampled`: validator OK=True, elapsed=33.73s
- `after_both_on_dense_512_sampled`: validator OK=True, elapsed=35.67s
- `after_both_on_multi_512_sampled`: validator OK=True, elapsed=4.64s

## Target resolve and route-local pointer pinning subaxis

| case | target slot and owner resolve (ms) | route-local pointer hoist miss and fallback (ms) | pointer validation and rebinding (ms) | connector hotpath target pinning (ms) |
| --- | ---: | ---: | ---: | ---: |
| connector_only dense 512 sampled | 0.003637 | 0.000000 | 0.003637 | 0.000027 |
| both_on dense 512 sampled | 15.458363 | 0.330006 | 15.540864 | 0.000027 |
| both_on multi 512 sampled | 2.254068 | 0.048268 | 2.266135 | 0.000000 |

## After sampled aggregate

| category | aggregate_ms | share_pct |
| --- | ---: | ---: |
| target slot and owner resolve | 17.716068 | 49.3 |
| route-local pointer hoist miss and fallback | 0.378274 | 1.1 |
| pointer validation and rebinding | 17.810636 | 49.6 |
| connector hotpath target pinning | 0.000054 | 0.0 |

Strict dominant did not appear in this partial clean rerun. The largest residual was **`pointer validation and rebinding`**.

## RELEASE and representative status

- `both_on, multi_comb_rect 1024 RELEASE`: validator OK=True, elapsed=27.76s
- `both_on, comb_rect_dense 1024 RELEASE`: authoritative clean rerun missing in this package.
- `both_on, comb_rect_dense 4096 RELEASE`: authoritative clean representative missing in this package.
- `both_on, multi_comb_rect 4096 RELEASE`: authoritative clean representative missing in this package.

## Conclusion

`next pivot after target-resolve round: pointer validation and rebinding`