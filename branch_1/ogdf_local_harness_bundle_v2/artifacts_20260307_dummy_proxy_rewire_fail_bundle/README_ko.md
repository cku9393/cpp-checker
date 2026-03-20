# Dummy Proxy Rewire Fail Bundle

이 번들은 `REAL projection / explicit graph compare` 경로를 연결한 뒤,
clean rebuild와 `dummy --manual-only` 재실행 결과를 묶은 산출물입니다.

핵심 상태:
- 이전 earliest stage: `DUMMY_REAL_SET_FAIL`
- 현재 earliest stage: `DUMMY_PROXY_REWIRE_FAIL`
- 현재 실패 이유: `ProjectHarnessOps: unwired hook: checkDummyProxyRewire (expected symbol: checkDummyProxyRewire(env, mini, trace, why))`
- 확인된 변화: `ExplicitExpected`와 `ExplicitGot`가 tc=0에서 동일하게 맞음

포함 내용:
- `CMakeLists.txt`
- `include/harness/project_static_adapter.hpp`
- `include/harness/types.hpp`
- `src/project_static_adapter.cpp`
- `src/dump.cpp`
- `src/project_hook_shims.cpp`
- `src/project_hooks_real.cpp`
- `verify/RESULT_ko.md`
- `verify/dumps/DUMMY_PROXY_REWIRE_FAIL_seed1_tc0.txt`

다음 작업 원칙:
1. earliest-stage 원칙 유지
2. 다음 단계는 `checkDummyProxyRewire`만 본다
3. `REAL projection / explicit graph compare` 경로는 다시 건드리지 않는다
