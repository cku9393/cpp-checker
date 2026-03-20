# raw adapter verify

## 결과

- `validateRawSpqrDecomp`는 이제 `src/project_hook_shims.cpp`의 TODO stub가 아니라
  `ProjectRawSnapshot` adapter 경로를 탄다.
- static/manual-only 재실행 결과는 `RAW_VALIDATE_FAIL`가 아니라
  `MINI_MATERIALIZE_FAIL`로 상승했다.

## 확인된 것

- 로컬 `CMakeLists.txt`에 `src/project_hook_shims.cpp`와
  `src/project_static_adapter.cpp`가 `rewrite_r_harness_core` target에 포함돼 있다.
- build log에서 아래 파일 컴파일을 확인했다.
  - `src/project_hooks_real.cpp`
  - `src/project_hook_shims.cpp`
  - `src/project_static_adapter.cpp`

## 실행 결과

- stage: `MINI_MATERIALIZE_FAIL`
- where: `materializeMiniCore`
- why: `project_hook_shims.cpp: TODO wire materializeMiniCore -> project::<materializeMiniCore>`

## 비고

- WSL에서 원본 경로(`/mnt/c/.../c++ 체커/...`)로 직접 configure하면
  경로/인코딩 문제로 불안정해서, 동일 소스를 `/tmp/codex/harness-src-copy` ASCII mirror로 복사해 build/run 검증했다.
- 소스 변경은 원본 워크스페이스에 반영되어 있고, 여기 폴더에는 검증 산출물만 복사해 두었다.
