# clean rebuild + hooks OFF 검증 결과

기준 경로:
`C:\Users\acsmi\.gemini\antigravity\scratch\c++ 체커\lca_tree_stress_v3\lca_tree_stress_v2\branch_1\ogdf_local_harness_bundle_v2`

## 수행한 핵심 단계

1. `build`, `dumps/static`, `dumps/dummy` 초기화
2. OGDF 라이브러리 준비 (`/tmp/codex/ogdf-install/lib/libOGDF.a`, `libCOIN.a`)
3. 하네스 clean rebuild (WSL/Ninja)
   - `USE_OGDF=ON`
   - `HARNESS_PROJECT_USE_FREE_FUNCTION_HOOKS=OFF`
4. static/manual-only 실행

## 소스/빌드 확인

- `src/rewrite_r_harness_main.cpp`는 `ProjectHarnessOps ops;` 사용
- `compile_commands.json`에 `project_hooks_real.cpp` 컴파일 항목 존재

## 실행 결과 (핵심)

실패 번들:
- `dumps/static/RAW_VALIDATE_FAIL_seed1_tc0.txt`

검증 포인트:
- `stage=RAW_VALIDATE_FAIL`
- `where=validateRawSpqrDecomp`
- `why=ProjectHarnessOps: unwired hook: validateRawSpqrDecomp (expected symbol: validateRawSpqrDecomp(H, raw, why))`

결론:
- 더 이상 `StubHarnessOps...`가 아니라 `ProjectHarnessOps...` 경로를 타고 있음이 확인됨.
- 따라서 stale binary 의심은 해소됨.
- 다음 병목은 static 5 free-function 심볼 미연결 상태.

## 다음 작업 (확정)

1. static 5 shim/free-function 구현
   - `validateRawSpqrDecomp`
   - `materializeMiniCore`
   - `normalizeWholeMiniCore`
   - `checkOwnershipConsistency`
   - `checkReducedInvariant`
2. `-DHARNESS_PROJECT_USE_FREE_FUNCTION_HOOKS=ON`으로 rebuild
3. static/manual-only 재실행
4. `RAW_VALIDATE_FAIL` 탈출 확인 후 MINI 단계 디버깅
