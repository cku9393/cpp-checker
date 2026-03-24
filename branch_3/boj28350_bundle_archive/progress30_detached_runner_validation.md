# progress30 detached runner validation

- Restored stress suite `/mnt/data/lca_tree_stress_v5`.
- Rebuilt `p30_release` and `p30_local`.
- Verified that a detached `setsid` launch of `run_progress30_case_transactional.py` can survive the caller process and produce a terminal `result.json`.
- Validation case: `test_detached_256`
  - mode: `comb_rect_dense`
  - n: `256`
  - delta: `both_on`
  - profile: `PROFILE_NONE`
  - rc: `0`
  - validator_ok: `true`
  - elapsed_sec: `5.97`

This removes one orchestration uncertainty: the transactional runner itself can complete and finalize rows when launched detached. The remaining bottleneck is just waiting for long release rows to finish.
