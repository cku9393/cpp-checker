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
- `[FAIL] tc=0 where=checkEquivalentExplicitGraphs why=ProjectHarnessOps: unwired hook: checkEquivalentExplicitGraphs (expected symbol: checkEquivalentExplicitGraphs(got, exp, why))`
- dump: `dumps/dummy/DUMMY_REAL_SET_FAIL_seed1_tc0.txt`

확인 사항:
- `ACTUAL_INVARIANT_FAIL / duplicate alive REAL slot for same real edge`는 더 이상 나오지 않음
- `ActualBeforeGraft`에서 node 0만 REAL slot을 소유하고, stub node의 REAL slot 보유는 없음
- earliest stage가 `DUMMY_REAL_SET_FAIL`까지 진행됨
