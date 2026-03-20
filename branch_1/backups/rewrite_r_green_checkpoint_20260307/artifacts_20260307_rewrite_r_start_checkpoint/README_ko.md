# Static + Dummy Green Checkpoint

이 번들은 현재 green 상태를 백업한 산출물입니다.

현재 확인된 상태:
- `static/manual-only` 통과
- `static random` 통과
- `dummy/manual-only` 통과

포함 내용:
- 현재 소스 스냅샷
  - `CMakeLists.txt`
  - `include/harness/project_static_adapter.hpp`
  - `include/harness/types.hpp`
  - `src/dump.cpp`
  - `src/project_hook_shims.cpp`
  - `src/project_hooks_real.cpp`
  - `src/project_static_adapter.cpp`
- 기존 검증 문서 보관
  - `verify/static/STATIC_MANUAL_RESULT_ko.md`
  - `verify/static/STATIC_RANDOM_RESULT_ko.md`
  - `verify/dummy/DUMMY_MANUAL_RESULT_ko.md`

다음 단계:
- `dummy` random을 점진적으로 실행해 첫 실제 랜덤 병목 earliest stage를 수집
