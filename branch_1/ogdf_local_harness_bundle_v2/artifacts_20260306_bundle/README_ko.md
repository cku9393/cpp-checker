# Bundle Artifacts

기준 폴더:
`C:\Users\acsmi\.gemini\antigravity\scratch\c++ 체커\lca_tree_stress_v3\lca_tree_stress_v2\branch_1\ogdf_local_harness_bundle_v2`

이 폴더는 현재 시점 기준 핵심 산출물을 한곳에 모은 묶음이다.

포함 내용:

- `project_hooks/`
  - 현재 코드 기준 소스/헤더/CMake 변경본
  - `ProjectHarnessOps`, `project_hook_shims.cpp`, template, 심볼 매핑 문서
- `verify_off/`
  - `HARNESS_PROJECT_USE_FREE_FUNCTION_HOOKS=OFF` 검증 결과
  - `ProjectHarnessOps: unwired hook`까지 확인한 번들
- `verify_on/`
  - `HARNESS_PROJECT_USE_FREE_FUNCTION_HOOKS=ON` 검증 결과
  - `project_hook_shims.cpp` placeholder shim까지 진입한 번들

현재 핵심 상태:

- stale binary 문제는 끝남
- wrapper/raw는 첫 manual 케이스 기준으로 살아 있음
- 다음 병목은 `project_hook_shims.cpp`의 static 5 body를 실제 프로젝트 함수로 바꾸는 작업
