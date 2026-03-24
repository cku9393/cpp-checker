# Progress22 state load materialization report

## Scope completed so far

This package is partial authoritative. It includes clean gate reruns, full clean LOCAL 512 before and after, tiny smoke confirmation that the new `time_sload_*` and `sload_*` keys are live, and a clean multi 1024 release rerun. Dense 1024 clean rerun was started again but had not completed at packaging time. Authoritative clean 4096 representative reruns are still missing.

Source artifact: `boj28350_literature_progress22_state_load_materialization_collapse.cpp`

## Clean gate reruns

| case | validator_ok | elapsed_sec |
| --- | --- | ---: |
| gate_connector_only_dense_256_after | True | 3.43 |
| gate_both_on_dense_256_after | True | 3.58 |
| gate_both_on_multi_512_after | True | 4.76 |

## Tiny sampled smoke

Sampled smoke `smoke_p22_multi64` was validator OK with elapsed 2.10s and confirmed that `time_sload_*` and `sload_*` keys are present in `result.json`.

## Clean LOCAL 512 before and after elapsed

| case | elapsed_sec |
| --- | ---: |
| before_connector_only_dense_512_base | 28.54 |
| before_both_on_dense_512_base | 27.24 |
| before_connector_only_dense_512_sampled | 27.80 |
| before_both_on_dense_512_sampled | 28.05 |
| before_both_on_multi_512_sampled | 4.96 |
| after_connector_only_dense_512_base | 27.19 |
| after_both_on_dense_512_base | 27.91 |
| after_connector_only_dense_512_sampled | 28.05 |
| after_both_on_dense_512_sampled | 27.61 |
| after_both_on_multi_512_sampled | 4.72 |

## Authoritative LOCAL sampled aggregate for state-load bucket

| category | before_ms | after_ms | after_share_pct |
| --- | ---: | ---: | ---: |
| previous-state seed and carry reuse | 14.879 | 15.102 | 60.0 |
| current-state direct load and source lookup | 5.103 | 5.040 | 20.0 |
| state-pair materialization and normalize | 5.012 | 5.042 | 20.0 |
| route-local invariant hoist and source pinning | 5.021 | 0.000 | 0.0 |

## After sampled per-case state-load breakdown

| case | prev_seed_ms | curr_source_ms | pair_pack_ms | invariant_hoist_miss_ms | total_ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| after_connector_only_dense_512_sampled | 0.008 | 0.002 | 0.005 | 0.000 | 0.015 |
| after_both_on_dense_512_sampled | 13.116 | 4.384 | 4.377 | 0.000 | 21.877 |
| after_both_on_multi_512_sampled | 1.978 | 0.654 | 0.660 | 0.000 | 3.292 |

## Current authoritative interpretation

Clean LOCAL sampled aggregate keeps the progress21 conclusion and sharpens it. Strict dominant is now present inside the state-load bucket: `previous-state seed and carry reuse` is 60.0 percent of the after aggregate. `current-state direct load and source lookup` and `state-pair materialization and normalize` each sit near 20 percent, while `route-local invariant hoist and source pinning` is effectively 0 after the current optimization path.

## Release reruns completed so far

| case | rc | timed_out | validator_ok | elapsed_sec |
| --- | ---: | --- | --- | ---: |
| after_both_on_multi_1024_release | 0 | False | True | 18.72 |
| after_both_on_dense_1024_release | missing | missing | missing | missing |

`after_both_on_multi_1024_release` was cleanly recovered with validator OK. `after_both_on_dense_1024_release` had been started again but had not completed at packaging time, so it remains missing in this package.

## Slow deletion qualitative excerpt

Representative top slow deletion lines from the clean after sampled reruns show the new `sload_*` fields are threaded through slow deletion export.

`after_both_on_dense_512_sampled`

```
slow_del[0] idx=31 x=30 touched=235 terms=7728 skelV=19978 unreg=149 reg=92 splitV=3737 gdfsE=44362 qscan=185 total_ns=164805266 t_gdfs_ns=330672 t_skel_ns=6606927 t_unreg_ns=1366075 t_reg_ns=13445 t_split_ns=1010280 t_qscan_ns=15819 dispatch_candidate_cids=464 publish_preserved_handles=52019 publish_connector_handles=52019 publish_posmap_builds=1606 publish_full_rescan_calls=862 publish_noop_calls=0 reuse_route=delta_preserved_then_skeleton reuse_keepmask_removed_handles=8376 reuse_preserved_direct_retag_handles=3737 reuse_connector_direct_retag_handles=19978 reuse_attachment_retargets=0 reuse_patch_vertices=44459 reuse_patch_handles_added=92 reuse_prepublish_preserved_annotate_calls=0 reuse_prepublish_connector_annotate_calls=0 reuse_final_publish_noop_calls=0 reuse_final_publish_skipped_calls=0 reuse_total_ns=93083962 wscan_route=delta_preserved_then_skeleton wscan_preserved_handles_scanned=52088 wscan_connector_handles_scanned=28179 wscan_existing_connector_set_handles_scanned=19886 wscan_retain_removed_handles=8376 wscan_retain_slotpos_fixups=8368 wscan_duplicate_full_scan_passes=83 retain_removed_handles=8376 retain_sparse_removed_entries=8376 retain_moved_entry_count=8368 retain_owner_lookup_calls=8368 retain_owner_lookup_misses=0 retain_slotpos_fixups=8368 retain_kept_handles_copied=0 retain_handleidx_fixups=10576 scomp_first_removed_index=18350 scomp_removed_run_count=315 scomp_kept_run_count=426 scomp_prefix_skipped_handles=18350 scomp_block_copied_handles=10576 scomp_elementwise_emitted_handles=0 scomp_suffix_only_calls=38 scomp_single_middle_run_calls=111 scomp_scratch_capacity_reuse_calls=194 plan_route=connector_skeleton plan_first_removed_index=18350 plan_removed_run_count=315 plan_kept_run_count=426 plan_adjacent_merge_hits=0 plan_descriptor_count=83 plan_dst_index_updates=83 plan_single_middle_shortcircuit_hits=111 plan_small_inline_hits=194 plan_total_ns=33868 rdisc_route=delta_preserved_then_skeleton rdisc_first_removed_index=18350 rdisc_removed_run_count=315 rdisc_kept_run_count=426 rdisc_boundary_reuse_hits=38 rdisc_suffix_only_hits=38 rdisc_single_middle_hits=111 rdisc_fused_onepass_calls=232 rdisc_small_runlist_inline_hits=0 rdisc_total_ns=4603252 fclass_route=delta_preserved_then_skeleton fclass_suffix_only_hits=38 fclass_single_middle_hits=111 fclass_fused_onepass_calls=232 fclass_transition_steps=8376 fclass_removed_to_kept_transitions=83 fclass_kept_to_removed_transitions=83 fclass_small_inline_hits=194 fclass_total_ns=4618337 tbranch_route=delta_preserved_then_skeleton tbranch_prev_state_materialize_calls=8144 tbranch_curr_state_load_calls=8144 tbranch_compare_ops=8144 tbranch_removed_to_kept_taken=83 tbranch_kept_to_removed_taken=83 tbranch_no_transition_steps=8061 tbranch_window_clipped_calls=0 tbranch_total_ns=3007221 sload_route=delta_preserved_then_skeleton sload_prev_seed_calls=8144 sload_prev_carry_reuse_hits=8144 sload_curr_direct_load_calls=8144 sload_curr_source_lookup_calls=0 sload_curr_source_lookup_misses=0 sload_state_pair_pack_calls=0 sload_invariant_ref_hoist_hits=8144 sload_total_ns=1065744 bcopy_route=connector_skeleton bcopy_single_middle_run_calls=222 bcopy_removed_run_count=315 bcopy_kept_run_count=426 bcopy_copy_plan_entries=83 bcopy_direct_memmove_calls=111 bcopy_direct_memmoved_handles=6449 bcopy_block_copied_handles=10576 bcopy_elementwise_fallback_handles=0 bcopy_overlap_staging_calls=0 bcopy_total_ns=55331 scomp_total_ns=294991 retain_total_ns=1993293 wscan_total_ns=13340767
```

`after_connector_only_dense_512_sampled`

```
slow_del[0] idx=71 x=70 touched=216 terms=8106 skelV=16807 unreg=189 reg=203 splitV=1888 gdfsE=38306 qscan=0 total_ns=168937394 t_gdfs_ns=346219 t_skel_ns=6856311 t_unreg_ns=175340 t_reg_ns=21112 t_split_ns=899554 t_qscan_ns=0 dispatch_candidate_cids=432 publish_preserved_handles=31664 publish_connector_handles=31664 publish_posmap_builds=1684 publish_full_rescan_calls=648 publish_noop_calls=0 reuse_route=none reuse_keepmask_removed_handles=217 reuse_preserved_direct_retag_handles=0 reuse_connector_direct_retag_handles=16807 reuse_attachment_retargets=0 reuse_patch_vertices=27953 reuse_patch_handles_added=203 reuse_prepublish_preserved_annotate_calls=0 reuse_prepublish_connector_annotate_calls=0 reuse_final_publish_noop_calls=0 reuse_final_publish_skipped_calls=0 reuse_total_ns=0 wscan_route=none wscan_preserved_handles_scanned=31678 wscan_connector_handles_scanned=16793 wscan_existing_connector_set_handles_scanned=16604 wscan_retain_removed_handles=217 wscan_retain_slotpos_fixups=212 wscan_duplicate_full_scan_passes=0 retain_removed_handles=217 retain_sparse_removed_entries=217 retain_moved_entry_count=212 retain_owner_lookup_calls=212 retain_owner_lookup_misses=0 retain_slotpos_fixups=212 retain_kept_handles_copied=0 retain_handleidx_fixups=3885 scomp_first_removed_index=27576 scomp_removed_run_count=216 scomp_kept_run_count=373 scomp_prefix_skipped_handles=27576 scomp_block_copied_handles=3885 scomp_elementwise_emitted_handles=0 scomp_suffix_only_calls=59 scomp_single_middle_run_calls=157 scomp_scratch_capacity_reuse_calls=157 plan_route=none plan_first_removed_index=27576 plan_removed_run_count=216 plan_kept_run_count=373 plan_adjacent_merge_hits=0 plan_descriptor_count=0 plan_dst_index_updates=0 plan_single_middle_shortcircuit_hits=157 plan_small_inline_hits=157 plan_total_ns=0 rdisc_route=none rdisc_first_removed_index=27576 rdisc_removed_run_count=216 rdisc_kept_run_count=373 rdisc_boundary_reuse_hits=59 rdisc_suffix_only_hits=59 rdisc_single_middle_hits=157 rdisc_fused_onepass_calls=216 rdisc_small_runlist_inline_hits=0 rdisc_total_ns=0 fclass_route=none fclass_suffix_only_hits=59 fclass_single_middle_hits=157 fclass_fused_onepass_calls=216 fclass_transition_steps=217 fclass_removed_to_kept_transitions=0 fclass_kept_to_removed_transitions=0 fclass_small_inline_hits=157 fclass_total_ns=0 tbranch_route=none tbranch_prev_state_materialize_calls=1 tbranch_curr_state_load_calls=1 tbranch_compare_ops=1 tbranch_removed_to_kept_taken=0 tbranch_kept_to_removed_taken=0 tbranch_no_transition_steps=1 tbranch_window_clipped_calls=0 tbranch_total_ns=0 sload_route=none sload_prev_seed_calls=1 sload_prev_carry_reuse_hits=1 sload_curr_direct_load_calls=1 sload_curr_source_lookup_calls=0 sload_curr_source_lookup_misses=0 sload_state_pair_pack_calls=0 sload_invariant_ref_hoist_hits=1 sload_total_ns=0 bcopy_route=none bcopy_single_middle_run_calls=314 bcopy_removed_run_count=216 bcopy_kept_run_count=373 bcopy_copy_plan_entries=0 bcopy_direct_memmove_calls=157 bcopy_direct_memmoved_handles=3885 bcopy_block_copied_handles=3885 bcopy_elementwise_fallback_handles=0 bcopy_overlap_staging_calls=0 bcopy_total_ns=0 scomp_total_ns=0 retain_total_ns=0 wscan_total_ns=0
```

`after_both_on_multi_512_sampled`

```
slow_del[0] idx=37 x=34 touched=117 terms=4395 skelV=4356 unreg=62 reg=42 splitV=3033 gdfsE=28746 qscan=112 total_ns=51211889 t_gdfs_ns=211386 t_skel_ns=2186004 t_unreg_ns=284109 t_reg_ns=4204 t_split_ns=458648 t_qscan_ns=11431 dispatch_candidate_cids=230 publish_preserved_handles=21725 publish_connector_handles=21725 publish_posmap_builds=765 publish_full_rescan_calls=451 publish_noop_calls=0 reuse_route=delta_preserved_then_skeleton reuse_keepmask_removed_handles=2380 reuse_preserved_direct_retag_handles=2966 reuse_connector_direct_retag_handles=4356 reuse_attachment_retargets=0 reuse_patch_vertices=15416 reuse_patch_handles_added=42 reuse_prepublish_preserved_annotate_calls=0 reuse_prepublish_connector_annotate_calls=0 reuse_final_publish_noop_calls=0 reuse_final_publish_skipped_calls=0 reuse_total_ns=28194459 wscan_route=delta_preserved_then_skeleton wscan_preserved_handles_scanned=21744 wscan_connector_handles_scanned=6641 wscan_existing_connector_set_handles_scanned=4314 wscan_retain_removed_handles=2380 wscan_retain_slotpos_fixups=2375 wscan_duplicate_full_scan_passes=53 retain_removed_handles=2380 retain_sparse_removed_entries=2380 retain_moved_entry_count=2375 retain_owner_lookup_calls=2375 retain_owner_lookup_misses=0 retain_slotpos_fixups=2375 retain_kept_handles_copied=0 retain_handleidx_fixups=4629 scomp_first_removed_index=7388 scomp_removed_run_count=167 scomp_kept_run_count=216 scomp_prefix_skipped_handles=7388 scomp_block_copied_handles=4629 scomp_elementwise_emitted_handles=0 scomp_suffix_only_calls=14 scomp_single_middle_run_calls=49 scomp_scratch_capacity_reuse_calls=101 plan_route=delta_preserved_then_skeleton plan_first_removed_index=7388 plan_removed_run_count=167 plan_kept_run_count=216 plan_adjacent_merge_hits=0 plan_descriptor_count=52 plan_dst_index_updates=52 plan_single_middle_shortcircuit_hits=49 plan_small_inline_hits=101 plan_total_ns=18652 rdisc_route=delta_preserved_then_skeleton rdisc_first_removed_index=7388 rdisc_removed_run_count=167 rdisc_kept_run_count=216 rdisc_boundary_reuse_hits=14 rdisc_suffix_only_hits=14 rdisc_single_middle_hits=49 rdisc_fused_onepass_calls=115 rdisc_small_runlist_inline_hits=0 rdisc_total_ns=1352388 fclass_route=delta_preserved_then_skeleton fclass_suffix_only_hits=14 fclass_single_middle_hits=49 fclass_fused_onepass_calls=115 fclass_transition_steps=2380 fclass_removed_to_kept_transitions=52 fclass_kept_to_removed_transitions=52 fclass_small_inline_hits=101 fclass_total_ns=1370350 tbranch_route=delta_preserved_then_skeleton tbranch_prev_state_materialize_calls=2265 tbranch_curr_state_load_calls=2265 tbranch_compare_ops=2265 tbranch_removed_to_kept_taken=52 tbranch_kept_to_removed_taken=52 tbranch_no_transition_steps=2213 tbranch_window_clipped_calls=0 tbranch_total_ns=881022 sload_route=delta_preserved_then_skeleton sload_prev_seed_calls=2265 sload_prev_carry_reuse_hits=2265 sload_curr_direct_load_calls=2265 sload_curr_source_lookup_calls=0 sload_curr_source_lookup_misses=0 sload_state_pair_pack_calls=0 sload_invariant_ref_hoist_hits=2265 sload_total_ns=316363 bcopy_route=delta_preserved_then_skeleton bcopy_single_middle_run_calls=98 bcopy_removed_run_count=167 bcopy_kept_run_count=216 bcopy_copy_plan_entries=52 bcopy_direct_memmove_calls=49 bcopy_direct_memmoved_handles=1266 bcopy_block_copied_handles=4629 bcopy_elementwise_fallback_handles=0 bcopy_overlap_staging_calls=0 bcopy_total_ns=29872 scomp_total_ns=104311 retain_total_ns=679305 wscan_total_ns=2732410
```

## Remaining missing

- `after_both_on_dense_1024_release` authoritative clean completion
- `after_both_on_dense_1024_release_repeat`
- `after_both_on_dense_4096_release` authoritative clean rerun
- `after_both_on_multi_4096_release` authoritative clean rerun

## Current authoritative conclusion

`next pivot after state-load round: previous-state seed and carry reuse`