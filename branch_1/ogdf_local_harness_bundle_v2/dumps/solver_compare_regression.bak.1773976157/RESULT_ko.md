# Solver Compare Result

## Status

- hard gate immutable: `True`
- rewrite-r-seq regression: `6/6` pass
- solver-compare regression: `0/6` pass
- random compare run: `false`

## Earliest Compare Failure

- case: `same_type_sp_cleanup_tc40_step1`
- reason: `baseline solver failed: actual reduced: dead relay forbidden`
- dump: `dumps/solver_compare_regression/solver_compare_baseline_fail_seed1_tc40_step1_same_type_sp_cleanup_tc40_step1.txt`

## Notes

- 이번 단계는 integration + compare 구현 단계다.
- hard regression compare가 실패했으므로 random compare는 의도적으로 실행하지 않았다.
- 다음 디버그 타깃은 earliest compare failure 1건만 본다.
