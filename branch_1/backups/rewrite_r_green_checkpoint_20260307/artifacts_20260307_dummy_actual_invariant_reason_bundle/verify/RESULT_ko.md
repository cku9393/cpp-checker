# RESULT

실행 순서:

1. actual-core invariant checker free function 구현
2. `checkActualReducedInvariant`를 free-function 경로로 연결
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

- `ACTUAL_INVARIANT_FAIL`는 더 이상 unwired hook이 아님
- 실제 invariant failure reason이 출력됨
- 현재 earliest stage는 계속 `ACTUAL_INVARIANT_FAIL`

실패 요약:

```text
[FAIL] tc=0 where=checkActualReducedInvariant why=actual ownership: duplicate alive REAL slot for same real edge
bundle=dumps/dummy/ACTUAL_INVARIANT_FAIL_seed1_tc0.txt
```

포함 항목:

- cumulative baseline
  - `include/harness/project_static_adapter.hpp`
  - `src/project_static_adapter.cpp`
- actual invariant checker and trace/dump support
  - `include/harness/types.hpp`
  - `src/dump.cpp`
  - `src/project_hook_shims.cpp`
  - `src/project_hooks_real.cpp`
- 최신 dummy failure dump
  - `verify/dumps/ACTUAL_INVARIANT_FAIL_seed1_tc0.txt`
