# RESULT

실행 순서:

1. static green 상태를 별도 checkpoint로 보관
2. `--mode dummy --manual-only` 실행
3. earliest stage가 `DUMMY_ENVELOPE_FAIL`인 것을 확인
4. `buildDummyActualCoreEnvelope`만 구현 및 연결
5. `--mode dummy --manual-only` 재실행

실행 명령:

```bash
./build/rewrite_r_harness \
  --backend ogdf \
  --mode dummy \
  --manual-only \
  --dump-dir dumps/dummy
```

결과:

- `DUMMY_ENVELOPE_FAIL` 탈출
- 현재 earliest stage는 `KEEP_SELECT_FAIL`
- 실패 이유는 `chooseKeepMiniNode` unwired

실패 요약:

```text
[FAIL] tc=0 where=chooseKeepMiniNode why=ProjectHarnessOps: unwired hook: chooseKeepMiniNode (expected symbol: chooseKeepMiniNode(mini, keep, why))
bundle=dumps/dummy/KEEP_SELECT_FAIL_seed1_tc0.txt
```

포함 항목:

- 최신 변경 소스
  - `src/project_hook_shims.cpp`
  - `src/project_hooks_real.cpp`
- 현재 baseline 헤더
  - `include/harness/project_static_adapter.hpp`
- 단일 dummy failure bundle
  - `verify/dumps/KEEP_SELECT_FAIL_seed1_tc0.txt`
- static green checkpoint
  - `artifacts_20260307_static_green_checkpoint.zip`
