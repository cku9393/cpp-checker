# Progress21 transition-state branch and state load core report

## Scope completed in this session

This session rebuilt progress21 from the progress20 authoritative base, wired `time_tbranch_*` and `tbranch_*` through LOCAL summary and slow-deletion export, reran clean gate 3 cases, and reran the full clean LOCAL 512 before and after matrix. Long RELEASE reruns were started but not authoritatively closed in this package.

Source artifact:
`boj28350_literature_progress21_transition_state_branch_state_load_core.cpp`

## Clean gate reruns

| case | validator_ok | elapsed_sec | local_active_mismatch | local_active_partition_mismatch | debug_touched_missing_classes | piece_materialize_fallback_calls | support_rebuild_fallback_calls | unanimous_baseline_path_calls |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| gate_connector_only_dense_256_after | True | 3.73 | ? | ? | ? | ? | ? | ? |
| gate_both_on_dense_256_after | True | 4.44 | ? | ? | ? | ? | ? | ? |
| gate_both_on_multi_512_after | True | 6.66 | ? | ? | ? | ? | ? | ? |

## Clean LOCAL 512 before and after elapsed

| case | elapsed_sec |
| --- | ---: |
| before_connector_only_dense_512_base | 45.12 |
| before_both_on_dense_512_base | 46.73 |
| before_connector_only_dense_512_sampled | 46.23 |
| before_both_on_dense_512_sampled | 47.09 |
| before_both_on_multi_512_sampled | 6.29 |
| after_connector_only_dense_512_base | 46.46 |
| after_both_on_dense_512_base | 47.28 |
| after_connector_only_dense_512_sampled | 45.07 |
| after_both_on_dense_512_sampled | 47.49 |
| after_both_on_multi_512_sampled | 6.08 |

## Authoritative LOCAL sampled aggregate for branch-state bucket

| category | aggregate_ms | share_pct |
| --- | ---: | ---: |
| state load materialization collapse | 12.314 | 39.8 |
| transition compare and classify fusion | 6.223 | 20.1 |
| dual-transition branch dispatch compaction | 0.220 | 0.7 |
| no-transition fast path and loop-carried state update | 12.203 | 39.4 |

## After sampled per-case branch-state breakdown

| case | state load ms | compare and classify ms | dual dispatch ms | no-transition ms | total ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| after_connector_only_dense_512_sampled | 0.007 | 0.002 | 0.004 | 0.006 | 0.019 |
| after_both_on_dense_512_sampled | 10.731 | 5.432 | 0.176 | 10.657 | 26.996 |
| after_both_on_multi_512_sampled | 1.576 | 0.789 | 0.040 | 1.539 | 3.944 |

## After sampled volume proxy

| case | prev_state_calls | curr_state_calls | compare_ops | removed_to_kept_taken | kept_to_removed_taken | no_transition_steps |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| after_connector_only_dense_512_sampled | 132 | 132 | 132 | 73 | 73 | 59 |
| after_both_on_dense_512_sampled | 188983 | 188983 | 188983 | 1966 | 1966 | 187017 |
| after_both_on_multi_512_sampled | 27629 | 27629 | 27629 | 630 | 630 | 26999 |

## Progress20 conclusion versus current clean rerun

Progress20 authoritative next pivot was `transition-state branch and state load core`.
This clean LOCAL rerun keeps that outer conclusion. Inside the branch-state bucket, strict dominant does not emerge, but the largest residual is `state load materialization collapse` by a narrow margin over `no-transition fast path and loop-carried state update`.


## Clean RELEASE reruns completed in this session

| case | rc | timed_out | validator_ok | elapsed_sec |
| --- | ---: | --- | --- | ---: |
| after_both_on_multi_1024_release_clean | 0 | False | True | 33.76 |

Dense 1024 clean rerun was started but did not complete in time for authoritative packaging. It is excluded from the final conclusion here.

## RELEASE and authoritative clean 4096 status

Dense 1024 clean rerun and authoritative clean 4096 reruns were not completed in this session. Existing rows under `progress21_runs/after_both_on_dense_1024_release*` are stale or in-progress and are excluded from the final conclusion here.

## Top K qualitative carry-forward

- after_connector_only_dense_512_sampled: `slow_del[0] idx=130 x=130 touched=186 terms=6479 skelV=11962 unreg=186 reg=200 splitV=0 gdfsE=30006 qscan=0 total_ns=295941698 t_gdfs_ns=821373 t_skel_ns=10147224 t_unreg_ns=440441 t_reg_ns=42945 t_split_ns=0 t_qscan_ns=0 dispatch_candidate_cids=372 publish_preserved_handles=23832 publish_connector_...`
- after_both_on_dense_512_sampled: `slow_del[0] idx=104 x=104 touched=200 terms=7205 skelV=13816 unreg=193 reg=166 splitV=192 gdfsE=33500 qscan=98 total_ns=273928934 t_gdfs_ns=724120 t_skel_ns=12152285 t_unreg_ns=1228850 t_reg_ns=38698 t_split_ns=200978 t_qscan_ns=8748 dispatch_candidate_cids=398 publish_preserved_handles=27646 publis...`
- after_both_on_multi_512_sampled: `slow_del[0] idx=37 x=34 touched=117 terms=4395 skelV=4356 unreg=62 reg=42 splitV=3033 gdfsE=28746 qscan=112 total_ns=72889663 t_gdfs_ns=350133 t_skel_ns=3276555 t_unreg_ns=261884 t_reg_ns=4955 t_split_ns=546574 t_qscan_ns=30918 dispatch_candidate_cids=230 publish_preserved_handles=21725 publish_conn...`

## Final current conclusion

`next pivot after branch-state round: state load materialization collapse`