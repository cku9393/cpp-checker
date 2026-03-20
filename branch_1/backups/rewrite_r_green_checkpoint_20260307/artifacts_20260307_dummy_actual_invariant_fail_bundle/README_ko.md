# Dummy Actual Invariant Fail Bundle

이 번들은 `graftMiniCoreIntoPlace`를 구현한 뒤
`dummy --manual-only`를 다시 실행해 `GRAFT_FAIL`을 벗어난 현재 상태를 묶은 산출물입니다.

핵심 상태:
- 이전 earliest stage: `GRAFT_FAIL`
- 현재 earliest stage: `ACTUAL_INVARIANT_FAIL`
- 현재 실패 이유: `checkActualReducedInvariant` unwired

포함 내용:
- `include/harness/project_static_adapter.hpp`
- `include/harness/types.hpp`
- `src/project_static_adapter.cpp`
- `src/dump.cpp`
- `src/project_hook_shims.cpp`
- `src/project_hooks_real.cpp`
- `verify/RESULT_ko.md`
- `verify/dumps/ACTUAL_INVARIANT_FAIL_seed1_tc0.txt`

다음 작업 원칙:
1. earliest-stage 원칙 유지
2. 다음 단계는 `actual invariant`만 수정
3. REAL projection / proxy rewire는 아직 건드리지 않음
