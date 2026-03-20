# Dummy Graft Fail Bundle

이 번들은 `chooseKeepMiniNode`를 adapter/shim 경로에 연결한 뒤,
`dummy --manual-only`를 다시 실행해 `KEEP_SELECT_FAIL`을 벗어난 결과를 묶은 산출물입니다.

핵심 상태:
- 이전 earliest stage: `KEEP_SELECT_FAIL`
- 현재 earliest stage: `GRAFT_FAIL`
- 현재 실패 이유: `graftMiniCoreIntoPlace` unwired

포함 내용:
- `include/harness/project_static_adapter.hpp`
- `src/project_static_adapter.cpp`
- `src/project_hook_shims.cpp`
- `src/project_hooks_real.cpp`
- `verify/RESULT_ko.md`
- `verify/dumps/GRAFT_FAIL_seed1_tc0.txt`

다음 작업 원칙:
1. earliest-stage 원칙 유지
2. 다음 단계는 `graftMiniCoreIntoPlace`만 수정
3. 다른 dummy hook은 아직 건드리지 않음
