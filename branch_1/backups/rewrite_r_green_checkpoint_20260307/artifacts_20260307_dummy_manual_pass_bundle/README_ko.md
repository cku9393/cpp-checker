# Dummy Manual Pass Bundle

이 번들은 `checkDummyProxyRewire`까지 연결한 뒤,
clean rebuild와 `dummy --manual-only` 재실행 결과를 묶은 산출물입니다.

핵심 상태:
- 이전 earliest stage: `DUMMY_PROXY_REWIRE_FAIL`
- 현재 상태: `dummy/manual-only` 통과
- 실행 결과: `[OK] completed tc=2`
- 확인된 변화: tc=0은 더 이상 `DUMMY_PROXY_REWIRE_FAIL`에 걸리지 않고, manual dummy 파이프라인이 끝까지 진행됨

포함 내용:
- `CMakeLists.txt`
- `include/harness/project_static_adapter.hpp`
- `include/harness/types.hpp`
- `src/project_static_adapter.cpp`
- `src/dump.cpp`
- `src/project_hook_shims.cpp`
- `src/project_hooks_real.cpp`
- `verify/RESULT_ko.md`

비고:
- 현재 `dumps/dummy` 아래에 실패 dump는 없음
