# RESULT

실행 순서:

1. `chooseKeepMiniNode`를 harness/adapter 쪽에 직접 구현
2. `src/project_hook_shims.cpp`에서 `chooseKeepMiniNode` body만 연결
3. clean rebuild 수행
4. `--mode dummy --manual-only` 재실행

실행 명령:

```bash
rm -rf build dumps/dummy
mkdir -p dumps/dummy

export OGDF_ROOT="${PWD}/.deps/ogdf-install"

cmake -S . -B build \
  -DUSE_OGDF=ON \
  -DOGDF_ROOT="${OGDF_ROOT}" \
  -DHARNESS_PROJECT_USE_FREE_FUNCTION_HOOKS=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build -j --verbose

./build/rewrite_r_harness \
  --backend ogdf \
  --mode dummy \
  --manual-only \
  --dump-dir dumps/dummy
```

결과:

- `KEEP_SELECT_FAIL`는 더 이상 발생하지 않음
- 현재 earliest stage는 `GRAFT_FAIL`
- 실패 이유는 `graftMiniCoreIntoPlace` unwired

실패 요약:

```text
[FAIL] tc=0 where=graftMiniCoreIntoPlace why=ProjectHarnessOps: unwired hook: graftMiniCoreIntoPlace (expected symbol: graftMiniCoreIntoPlace(core, oldR, H, mini, keep, q, trace, why))
bundle=dumps/dummy/GRAFT_FAIL_seed1_tc0.txt
```

포함 항목:

- keep selector 선언/구현
  - `include/harness/project_static_adapter.hpp`
  - `src/project_static_adapter.cpp`
- shim 및 hook 연결
  - `src/project_hook_shims.cpp`
  - `src/project_hooks_real.cpp`
- 최신 dummy failure dump
  - `verify/dumps/GRAFT_FAIL_seed1_tc0.txt`
