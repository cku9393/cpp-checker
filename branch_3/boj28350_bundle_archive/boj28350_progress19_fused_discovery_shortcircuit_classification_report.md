# Progress19 fused discovery and shortcircuit classification report
This report is reconstructed from surviving progress18 artifacts and the completed progress19 session note after a later environment reset removed the live progress19 source, binaries, and raw run directories. Numeric LOCAL sampled before and after values below are preserved from the session note. Missing release and representative rows are marked explicitly.
## Reconstruction status
Reconstructed from session note: yes.
Authoritative progress19 source file currently present: no.
Authoritative progress19 raw run directories currently present: no.
This package is therefore a best-effort reconstructed report rather than a raw-artifact-backed final submission.
## Correctness gate
- `gate_connector_only_dense_256_after`: validator OK, elapsed 2.05s.
- `gate_both_on_dense_256_after`: validator OK, elapsed 2.16s.
- `gate_both_on_multi_512_after`: validator OK, elapsed 6.13s.

All preserved correctness conditions remained satisfied in the completed session note: `local_active_mismatch=0`, `local_active_partition_mismatch=0`, `debug_touched_missing_classes=0`, `piece_materialize_fallback_calls=0`, `support_rebuild_fallback_calls=0`, `unanimous_baseline_path_calls=0`.
## 512 LOCAL elapsed summary
| case | before | after | delta |
| --- | ---: | ---: | ---: |
| connector_only, comb_rect_dense 512 LOCAL, PROFILE_BASE | 45.81s | 48.26s | +2.45s |
| both_on, comb_rect_dense 512 LOCAL, PROFILE_BASE | 45.67s | 42.54s | -3.13s |
| connector_only, comb_rect_dense 512 LOCAL, PROFILE_SAMPLED | 45.58s | 41.66s | -3.92s |
| both_on, comb_rect_dense 512 LOCAL, PROFILE_SAMPLED | 43.99s | 42.32s | -1.67s |
| both_on, multi_comb_rect 512 LOCAL, PROFILE_SAMPLED | 5.76s | 5.90s | +0.14s |

## Sampled grouped after aggregate
| category | aggregate_ns | share_pct |
| --- | ---: | ---: |
| run transition emit and count finalize | 1677722 | 44.3 |
| transition-state one-pass scan core | 1021249 | 27.0 |
| shortcircuit classification fast path | 782341 | 20.7 |
| small-runlist inline materialization | 302742 | 8.0 |

## Sampled grouped before aggregate
| category | aggregate_ns | share_pct |
| --- | ---: | ---: |
| run transition emit and count finalize | 1727285 | 45.4 |
| transition-state one-pass scan core | 1010473 | 26.6 |
| shortcircuit classification fast path | 773416 | 20.3 |
| small-runlist inline materialization | 294602 | 7.7 |

## Per-case sampled after grouped table
### after_connector_only_dense_512_sampled
| category | ns | share_pct |
| --- | ---: | ---: |
| run transition emit and count finalize | 734130 | 45.1 |
| transition-state one-pass scan core | 428790 | 26.3 |
| shortcircuit classification fast path | 337089 | 20.7 |
| small-runlist inline materialization | 127431 | 7.8 |
### after_both_on_dense_512_sampled
| category | ns | share_pct |
| --- | ---: | ---: |
| run transition emit and count finalize | 752948 | 44.4 |
| transition-state one-pass scan core | 478107 | 28.2 |
| shortcircuit classification fast path | 333989 | 19.7 |
| small-runlist inline materialization | 129663 | 7.7 |
### after_both_on_multi_512_sampled
| category | ns | share_pct |
| --- | ---: | ---: |
| run transition emit and count finalize | 190644 | 41.3 |
| transition-state one-pass scan core | 114352 | 24.8 |
| shortcircuit classification fast path | 111263 | 24.1 |
| small-runlist inline materialization | 45648 | 9.9 |

## Key sampled volume counters after
| case | fused_onepass_calls | suffix_only_hits | single_middle_hits | transition_steps | removed_to_kept | kept_to_removed | small_inline_hits |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| after_connector_only_dense_512_sampled | 71 | 0 | 2 | 239 | 184 | 193 | 71 |
| after_both_on_dense_512_sampled | 95 | 0 | 17 | 217 | 136 | 183 | 95 |
| after_both_on_multi_512_sampled | 51 | 0 | 0 | 2634 | 71 | 116 | 51 |

## Release and representative status
| case | status | validator | elapsed | notes |
| --- | --- | --- | ---: | --- |
| both_on, multi_comb_rect 1024 RELEASE | completed | OK | 32.48s | survived result from session note |
| both_on, comb_rect_dense 1024 RELEASE | incomplete | unknown | - | environment reset while the authoritative run was in progress; raw output lost |
| both_on, comb_rect_dense 1024 RELEASE repeat | not run | - | - | not reached before reset |
| both_on, comb_rect_dense 1024 RELEASE compact diag | not run | - | - | not reached before reset |
| both_on, comb_rect_dense 4096 RELEASE | not run | - | - | not reached before reset |
| both_on, multi_comb_rect 4096 RELEASE | not run | - | - | not reached before reset |

## Interpretation
The reconstructed after sampled aggregate does not show a strict dominant above 50 percent. The largest residual is `run transition emit and count finalize` at 44.3 percent. `transition-state one-pass scan core` is next at 27.0 percent, `shortcircuit classification fast path` is 20.7 percent, and `small-runlist inline materialization` is 8.0 percent. This means the next pivot after a reconstructed progress19 would most likely be `run transition emit and count finalize`, but this conclusion should be treated as reconstructed from session note rather than raw-artifact-backed.

## Final reconstructed conclusion
`next pivot after fused-classification round: run transition emit and count finalize`
