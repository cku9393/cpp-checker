rewrite-r manual-only local harness를 추가하고 clean rebuild 후 첫 earliest-stage를 수집했다.

- 실행 명령:
  `./build/rewrite_r_harness --backend ogdf --mode rewrite-r --manual-only --dump-dir dumps/rewrite_r_manual`
- 결과:
  `tc=0` 에서 `LOCAL_REWRITE_R_FAIL`
- reason:
  `project_hook_shims.cpp: TODO wire rewriteR_fallback -> project::rewriteR_fallback(core, rNode, x, why)`
- first bundle:
  `verify/dumps/LOCAL_REWRITE_R_FAIL_seed1_tc0.txt`

bundle에는 새 `rewrite-r` mode/runner/hook surface 변경 파일과 시작 checkpoint를 함께 넣었다.
