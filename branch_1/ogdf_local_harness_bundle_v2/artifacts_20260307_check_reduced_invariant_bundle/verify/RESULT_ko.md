# RESULT

실행 명령:

```bash
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

- `checkOwnershipConsistency`는 adapter 경로로 통과
- earliest failure는 여전히 `MINI_PRECHECK_FAIL`
- 실패 이유는 `project_hook_shims.cpp: TODO wire checkReducedInvariant -> project::<checkReducedInvariant>`

실패 요약:

```text
[FAIL] tc=0 where=mini precheck why=project_hook_shims.cpp: TODO wire checkReducedInvariant -> project::<checkReducedInvariant>
bundle=dumps/static/MINI_PRECHECK_FAIL_seed1_tc0.txt
```
