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
- `[OK] completed tc=2`

확인 사항:
- `DUMMY_PROXY_REWIRE_FAIL` unwired reason 은 더 이상 나오지 않음
- `dummy/manual-only`가 manual case 전체를 끝까지 통과함
- `dumps/dummy`에는 실패 dump 파일이 생성되지 않음
