# 다음 할 일 (Static 우선)

기준 경로:
`C:\Users\acsmi\.gemini\antigravity\scratch\c++ 체커\lca_tree_stress_v3\lca_tree_stress_v2\branch_1\ogdf_local_harness_bundle_v2`

## 1) static 5개 실제 심볼 연결

`src/project_hooks_real.cpp`는 현재 아래 free-function 심볼을 호출하도록 되어 있습니다.

- `validateRawSpqrDecomp(H, raw, why)`
- `materializeMiniCore(H, raw, mini, why)`
- `normalizeWholeMiniCore(mini)`
- `checkOwnershipConsistency(mini, H, why)`
- `checkReducedInvariant(mini, H, why)`

실제 프로젝트 코드에서 위 5개를 동일 시그니처로 제공해야 합니다.

## 2) 빌드 옵션 켜기

`CMakeLists.txt`에 옵션이 추가되어 있으므로 configure 시 아래를 켭니다.

- `-DHARNESS_PROJECT_USE_FREE_FUNCTION_HOOKS=ON`

필요 시 OGDF도 함께:

- `-DUSE_OGDF=ON`
- `-DOGDF_ROOT=<your-ogdf-root>`

## 3) static만 재실행

초기 검증은 dummy로 올라가지 말고 static/manual-only로 제한:

```bash
./build/rewrite_r_harness --backend ogdf --mode static --manual-only --dump-dir dumps/static
```

## 4) 결과 판정

- `RAW_VALIDATE_FAIL`이면 훅 연결/시그니처/링크 문제를 우선 해결
- `MINI_*`로 올라가면 정상 진전
- static manual 통과 후에만 dummy 훅 9개를 순차 연결

## 참고

- 현재 실행 환경에서는 `cmake`가 PATH에 없어 빌드 실행은 미검증 상태입니다.
