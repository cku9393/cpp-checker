# Collected Artifacts

기준 폴더:
`C:\Users\acsmi\.gemini\antigravity\scratch\c++ 체커\lca_tree_stress_v3\lca_tree_stress_v2\branch_1\ogdf_local_harness_bundle_v2`

이 폴더는 지금까지 만든 주요 산출물을 한곳에 모아둔 묶음이다.

포함 내용:

- `project_hooks/`
  - `ProjectHarnessOps` 전환과 관련된 소스/헤더/CMake 변경본
  - 심볼 매핑 문서, 다음 작업 문서, static 5 shim 파일
- `rebuild_verify/`
  - clean rebuild + `HARNESS_PROJECT_USE_FREE_FUNCTION_HOOKS=OFF` 검증 결과
  - `ProjectHarnessOps: unwired hook`까지 확인한 결과 문서와 번들
- `hooks_on_verify/`
  - `HARNESS_PROJECT_USE_FREE_FUNCTION_HOOKS=ON` 재빌드 검증 결과
  - `project_hook_shims.cpp` 경로로 실패가 이동했음을 보여주는 결과 문서와 번들
- `current_dumps/static/`
  - 현재 최신 static 실행 번들

현재 기준 핵심 상태:

- stale binary 문제는 해소됨
- 최신 실패는 `StubHarnessOps`가 아니라 `project_hook_shims.cpp` placeholder shim
- 다음 병목은 static 5 shim body를 실제 프로젝트 함수로 바꾸는 작업

참고:

- `dumps` 아래의 깨진 이름 디렉터리(`static` 이외)는 복사하지 않았다.
- 정상 경로의 최신 산출물만 이 폴더에 모았다.
