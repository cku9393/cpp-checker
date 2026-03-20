# Dummy Actual Invariant Reason Bundle

이 번들은 `checkReducedInvariantActual` hook을 연결한 뒤
`dummy --manual-only`를 다시 실행해
`ACTUAL_INVARIANT_FAIL`의 실제 실패 이유를 얻은 상태를 묶은 산출물입니다.

핵심 상태:
- earliest stage: `ACTUAL_INVARIANT_FAIL`
- 실제 실패 이유: `actual ownership: duplicate alive REAL slot for same real edge`

포함 내용:
- `include/harness/project_static_adapter.hpp`
- `include/harness/types.hpp`
- `src/project_static_adapter.cpp`
- `src/dump.cpp`
- `src/project_hook_shims.cpp`
- `src/project_hooks_real.cpp`
- `verify/RESULT_ko.md`
- `verify/dumps/ACTUAL_INVARIANT_FAIL_seed1_tc0.txt`

다음 작업 원칙:
1. earliest-stage 원칙 유지
2. 다음 단계는 ownership duplication 원인만 수정
3. 우선 `buildDummyActualCoreEnvelope`에서 REAL edge가 root/stub에 동시에 남는지 확인
