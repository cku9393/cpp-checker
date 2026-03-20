# Verify Result

실행 요약:
- clean rebuild 수행
- configure:
  `cmake -S . -B build -DUSE_OGDF=ON -DOGDF_ROOT="/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_1/ogdf_local_harness_bundle_v2/.deps/ogdf-install" -DHARNESS_PROJECT_USE_FREE_FUNCTION_HOOKS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`
- build:
  `cmake --build build -j --verbose`
- run:
  `./build/rewrite_r_harness --backend ogdf --mode dummy --manual-only --dump-dir dumps/dummy`

실행 결과:
- `[FAIL] tc=0 where=checkDummyProxyRewire why=ProjectHarnessOps: unwired hook: checkDummyProxyRewire (expected symbol: checkDummyProxyRewire(env, mini, trace, why))`
- dump: `dumps/dummy/DUMMY_PROXY_REWIRE_FAIL_seed1_tc0.txt`

확인 사항:
- `DUMMY_REAL_SET_FAIL` with unwired reason 은 더 이상 나오지 않음
- `ExplicitExpected`와 `ExplicitGot`가 tc=0에서 동일하게 `id=1 (1,2)`, `id=2 (2,3)`, `id=3 (1,3)`로 맞음
- earliest stage가 `DUMMY_PROXY_REWIRE_FAIL`까지 진행됨
