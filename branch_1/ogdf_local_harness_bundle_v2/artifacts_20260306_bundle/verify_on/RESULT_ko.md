# hooks ON rebuild 검증 결과

기준 경로:
`C:\Users\acsmi\.gemini\antigravity\scratch\c++ 체커\lca_tree_stress_v3\lca_tree_stress_v2\branch_1\ogdf_local_harness_bundle_v2`

## 수행 내용

1. `src/project_hook_shims.cpp` 추가
2. `src/project_hook_shims_template.cpp` 추가
3. `CMakeLists.txt`에 `src/project_hook_shims.cpp`를 빌드 타깃으로 추가
4. `HARNESS_PROJECT_USE_FREE_FUNCTION_HOOKS=ON`으로 rebuild
5. static/manual-only 재실행

## 빌드 확인

`compile_commands.json`에서 아래 두 파일이 모두 컴파일됨을 확인했다.

- `src/project_hooks_real.cpp`
- `src/project_hook_shims.cpp`

## 실행 결과

실패 번들:

- `dumps/static/RAW_VALIDATE_FAIL_seed1_tc0.txt`

핵심 라인:

- `stage=RAW_VALIDATE_FAIL`
- `where=validateRawSpqrDecomp`
- `why=project_hook_shims.cpp: TODO wire validateRawSpqrDecomp -> project::<validateRawSpqrDecomp>`

## 결론

- `HARNESS_PROJECT_USE_FREE_FUNCTION_HOOKS=ON` 경로는 정상 동작한다.
- 실패 원인은 더 이상 `ProjectHarnessOps: unwired hook`이 아니다.
- 다음 병목은 `project_hook_shims.cpp`의 static 5 body를 실제 프로젝트 함수로 교체하는 일이다.
