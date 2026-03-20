# OGDF local harness bundle v2

이 번들은 이전 압축과 달리 **실제 파일**을 포함합니다.

포함 내용:
- `src/ogdf_wrapper.cpp`: OGDF raw backend wrapper 구현 스켈레톤
- `src/runners.cpp`: `runStaticPipelineCaseDumpAware`, `runDummyGraftCaseDumpAware`
- `src/rewrite_r_harness_main.cpp`: 로컬 실행용 CLI 드라이버
- `src/project_hooks_stub.cpp`: 프로젝트 통합 전까지 컴파일 가능한 stub hooks
- `src/dump.cpp`: bundle dump 출력기
- `scripts/install_ogdf_local.sh`: OGDF 로컬 설치 스크립트
- `scripts/find_ogdf_root.sh`: OGDF_ROOT 후보 탐색 스크립트

## 중요
- 이 번들은 **wrapper/runner 코드가 들어 있습니다.**
- 다만 실제 프로젝트 연동 함수는 `StubHarnessOps`로 남아 있으므로,
  프로젝트의 실제 구현으로 교체해야 진짜 harness가 동작합니다.
- 즉 "드라이버/러너/백엔드 경계"는 제공되고, "프로젝트 고유 알고리즘 연결"만 남았습니다.

## 빌드
### OGDF 없이 (stub 확인)
```bash
cmake -S . -B build -DUSE_OGDF=OFF
cmake --build build -j
./build/rewrite_r_harness --mode static --manual-only --dump-dir dumps/static
```

### OGDF 설치 후
```bash
./scripts/install_ogdf_local.sh
OGDF_ROOT=$(./scripts/find_ogdf_root.sh .deps/ogdf-install | head -n1)
cmake -S . -B build -DUSE_OGDF=ON -DOGDF_ROOT="$OGDF_ROOT"
cmake --build build -j
```

## 바로 다음 순서
1. wrapper 5개 채우기
2. static runner 실행
3. 첫 failure bundle 저장
4. earliest-stage만 디버깅
5. static 통과 후 dummy graft runner 실행


## 피드백 반영
- `include/harness/dump.hpp`에 `<sstream>` 누락을 수정했습니다.
- `scripts/configure_with_local_ogdf.sh`를 추가했습니다. 이 스크립트는 OGDF 소스 clone/build/install 후 하네스를 `USE_OGDF=ON`으로 바로 configure/build 합니다.
- `find_ogdf_root.sh`는 install prefix와 build-tree 둘 다 후보로 탐색하도록 보강했습니다.

### 가장 빠른 로컬 실행
```bash
./scripts/configure_with_local_ogdf.sh
./build/rewrite_r_harness --backend ogdf --mode static --manual-only --dump-dir dumps/static
```
