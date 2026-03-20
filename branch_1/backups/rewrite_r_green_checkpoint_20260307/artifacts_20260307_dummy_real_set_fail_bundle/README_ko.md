# Dummy Real Set Fail Bundle

이 번들은 `buildDummyActualCoreEnvelope`를 수정해
REAL edge가 `oldR`에만 남도록 만든 뒤,
clean rebuild와 `dummy --manual-only` 재실행 결과를 묶은 산출물입니다.

핵심 상태:
- 이전 핵심 실패 이유: `ACTUAL_INVARIANT_FAIL / actual ownership: duplicate alive REAL slot for same real edge`
- 현재 earliest stage: `DUMMY_REAL_SET_FAIL`
- 현재 실패 이유: `ProjectHarnessOps: unwired hook: checkEquivalentExplicitGraphs (expected symbol: checkEquivalentExplicitGraphs(got, exp, why))`
- 확인된 변화: `ActualBeforeGraft`에서 node 0만 REAL slot(`1 2 3`)을 보유하고 stub REAL ownership은 사라짐

포함 내용:
- `CMakeLists.txt`
- `include/harness/project_static_adapter.hpp`
- `include/harness/types.hpp`
- `src/project_static_adapter.cpp`
- `src/dump.cpp`
- `src/project_hook_shims.cpp`
- `src/project_hooks_real.cpp`
- `verify/RESULT_ko.md`
- `verify/dumps/DUMMY_REAL_SET_FAIL_seed1_tc0.txt`

다음 작업 원칙:
1. earliest-stage 원칙 유지
2. 다음 단계는 `checkEquivalentExplicitGraphs` hook만 본다
3. `buildDummyActualCoreEnvelope` 쪽 ownership 회귀는 다시 건드리지 않는다
