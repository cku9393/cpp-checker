# RESULT

실행 명령:

```bash
cmake --fresh -S . -B build \
  -DUSE_OGDF=ON \
  -DOGDF_ROOT="$PWD/.deps/ogdf-install" \
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

- `materializeMiniCore`는 adapter 경로로 성공
- 실패 stage는 `MINI_PRECHECK_FAIL`로 이동
- 실패 이유는 아직 TODO인 `checkOwnershipConsistency` shim

실패 요약:

```text
[FAIL] tc=0 where=mini precheck why=project_hook_shims.cpp: TODO wire checkOwnershipConsistency -> project::<checkOwnershipConsistency>
bundle=dumps/static/MINI_PRECHECK_FAIL_seed1_tc0.txt
```
