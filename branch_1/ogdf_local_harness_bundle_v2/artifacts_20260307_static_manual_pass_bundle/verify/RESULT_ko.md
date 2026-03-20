# RESULT

실행 명령:

```bash
cmake -E remove_directory build
cmake -E remove_directory dumps/static
mkdir -p dumps/static

export OGDF_ROOT="${PWD}/.deps/ogdf-install"

cmake -S . -B build \
  -DUSE_OGDF=ON \
  -DOGDF_ROOT="${OGDF_ROOT}" \
  -DHARNESS_PROJECT_USE_FREE_FUNCTION_HOOKS=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build -j --verbose

./build/rewrite_r_harness \
  --backend ogdf \
  --mode static \
  --manual-only \
  --dump-dir dumps/static
```

결과:

- `checkReducedInvariant`가 adapter 경로로 연결됨
- `MINI_PRECHECK_FAIL`의 TODO 단계는 더 이상 발생하지 않음
- `static/manual-only`가 통과함

실행 요약:

```text
[OK] completed tc=2
```

포함 소스:

- `include/harness/project_static_adapter.hpp`
- `src/project_static_adapter.cpp`
- `src/project_hook_shims.cpp`
- `src/project_hooks_real.cpp`
