# OGDF local harness bundle

이 묶음은 아래 반복 루프를 로컬에서 바로 시작할 수 있게 만든 최소 실행 뼈대다.

- OGDF wrapper 채우기
- static runner 실행
- 첫 failure bundle 수집
- earliest-stage만 디버깅
- static pipeline 통과 후 dummy graft runner 실행
- 다시 첫 failure bundle 수집
- 다시 earliest-stage만 디버깅

## 포함 파일

- `CMakeLists.txt`
  - 로컬 빌드용 최소 CMake
- `cmake/DetectOGDFFeatures.cmake`
  - OGDF feature detection
- `include/ogdf_feature_config.hpp.in`
  - configure_file 출력 템플릿
- `src/rewrite_r_harness_main.cpp`
  - 로컬 실행용 main() 드라이버 인자 파서
- `docs/ogdf_wrapper_fill_checklist_ko.md`
  - wrapper 5개를 채울 때 체크리스트
- `docs/failure_bundle_reading_example_ko.md`
  - 첫 failure bundle 읽는 순서 예시
- `docs/integration_notes_ko.md`
  - 실제 하네스 코드와 연결하는 위치
- `prompts/local_execution_prompt_ko.txt`
  - 로컬 실행용 프롬프트
- `scripts/run_static_then_dummy.sh`
  - static -> dummy 순차 실행 예시

## 권장 실행 순서

1. OGDF include/lib 경로를 로컬 환경에 맞게 지정한다.
2. wrapper 5개를 채운다.
3. `runStaticPipelineCaseDumpAware`를 manual-only로 먼저 실행한다.
4. 첫 실패 bundle을 저장한다.
5. 가장 이른 stage만 디버깅한다.
6. static pipeline이 통과하면 `runDummyGraftCaseDumpAware`를 실행한다.
7. 다시 첫 실패 bundle을 저장한다.
8. 가장 이른 stage만 디버깅한다.
9. 이 둘이 안정화된 뒤에만 `rewriteR_fallback + normalizeTouchedRegion`로 올린다.

## 로컬 빌드 예시

```bash
cmake -S . -B build \
  -DUSE_OGDF=ON \
  -DOGDF_ROOT=/path/to/ogdf
cmake --build build -j
```

또는 include/lib를 직접 넘길 수도 있다.

```bash
cmake -S . -B build \
  -DUSE_OGDF=ON \
  -DOGDF_INCLUDE_DIR=/path/to/ogdf/include \
  -DOGDF_LIBRARY=/path/to/libOGDF.so
cmake --build build -j
```

## 실행 예시

먼저 static-only:

```bash
./build/rewrite_r_harness \
  --backend ogdf \
  --mode static \
  --seed 1 \
  --rounds 1 \
  --manual-only \
  --dump-dir dumps/static
```

그다음 random static:

```bash
./build/rewrite_r_harness \
  --backend ogdf \
  --mode static \
  --seed 1 \
  --rounds 1000 \
  --dump-dir dumps/static
```

그다음 dummy graft:

```bash
./build/rewrite_r_harness \
  --backend ogdf \
  --mode dummy \
  --seed 1 \
  --rounds 1 \
  --manual-only \
  --dump-dir dumps/dummy
```

## 중요 원칙

- 항상 첫 실패 케이스 1개만 본다.
- 항상 가장 이른 stage만 디버깅한다.
- `RAW_*`가 깨졌으면 mini/actual은 보지 않는다.
- `MINI_*`가 깨졌으면 actual/graft는 보지 않는다.
- `DUMMY_PROXY_REWIRE_FAIL`이면 backend/raw/mini는 이미 통과했다고 보고 actual/trace만 본다.

## 참고

OGDF는 개발/검증용 backend로 쓰고, 최종 제출용은 self-contained backend로 분리하는 전략을 권장한다.
