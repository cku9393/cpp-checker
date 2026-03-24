# Progress23 interrupted local findings

This is a reconstructed note from an interrupted progress23 session. The environment reset before authoritative progress23 source and result artifacts were persisted.

## What was completed before reset

The intended base was `boj28350_literature_progress22_state_load_materialization_collapse.cpp`.

A progress23 branch was patched to target the `previous-state seed and carry reuse` bucket.
The following new metrics were wired and verified in a tiny sampled smoke before the reset:

`time_pcarry_boundary_seed_bootstrap_ns`
`time_pcarry_prev_scalar_materialize_ns`
`time_pcarry_prev_scalar_pack_ns`
`time_pcarry_carry_reuse_check_ns`
`time_pcarry_carry_reuse_hit_apply_ns`
`time_pcarry_carry_invalidate_transition_ns`
`time_pcarry_carry_invalidate_window_clip_ns`
`time_pcarry_reseed_after_invalidate_ns`

and corresponding `pcarry_*` volume counters.

Three clean gates were rerun and all were validator OK.

`connector_only, comb_rect_dense 256 LOCAL, PROFILE_NONE, all opts on`
`both_on, comb_rect_dense 256 LOCAL, PROFILE_NONE, all opts on`
`both_on, multi_comb_rect 512 LOCAL, PROFILE_SAMPLED, all opts on`

## Clean LOCAL 512 reruns completed before reset

The following clean LOCAL matrix was completed before the reset.

Before

`before_connector_only_dense_512_base`
`before_both_on_dense_512_base`
`before_connector_only_dense_512_sampled`
`before_both_on_dense_512_sampled`
`before_both_on_multi_512_sampled`

After

`after_connector_only_dense_512_base`
`after_both_on_dense_512_base`
`after_connector_only_dense_512_sampled`
`after_both_on_dense_512_sampled`
`after_both_on_multi_512_sampled`

## Clean LOCAL 512 elapsed observed before reset

`connector_only, comb_rect_dense 512, PROFILE_BASE`
before 49.21s
after 48.53s

`both_on, comb_rect_dense 512, PROFILE_BASE`
before 46.22s
after 48.91s

`connector_only, comb_rect_dense 512, PROFILE_SAMPLED`
before 47.73s
after 48.10s

`both_on, comb_rect_dense 512, PROFILE_SAMPLED`
before 47.50s
after 48.38s

`both_on, multi_comb_rect 512, PROFILE_SAMPLED`
before 8.17s
after 8.41s

## Clean LOCAL sampled aggregate for prev-state seed and carry bucket

After sampled aggregate reconstructed from the interrupted session:

`boundary seed bootstrap and refresh` 1.177ms
`previous-state scalar materialize and pack` 25.275ms
`carry reuse check and hit fast path` 25.851ms
`carry invalidation and reseed fallback` 0.291ms

This implies no strict dominant, but the largest residual was:

`carry reuse check and hit fast path`

## Release status observed before reset

`both_on, multi_comb_rect 1024 RELEASE`
validator OK, 35.13s

`both_on, comb_rect_dense 1024 RELEASE`
was already in progress when the environment reset and did not produce a preserved authoritative row.

## Reconstructed current conclusion

`next pivot after prev-state round: carry reuse check and hit fast path`

## Safe resume point

The safest resume point is still progress22 base.
Recreate the progress23 branch from `boj28350_literature_progress22_state_load_materialization_collapse.cpp`, reapply the `pcarry` patch, verify the tiny smoke, rerun the clean gate 3, then rerun the LOCAL 512 matrix, then rerun `both_on, multi_comb_rect 1024 RELEASE` and `both_on, comb_rect_dense 1024 RELEASE` cleanly.
