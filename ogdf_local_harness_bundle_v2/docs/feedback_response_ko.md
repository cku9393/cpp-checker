# 실행 결과 피드백 요약

현재 실패 원인은 알고리즘이 아니라 **백엔드 미연결**입니다.

- 생성된 bundle의 stage가 `RAW_BACKEND_FAIL`
- `where=backend.buildRaw`
- `why=USE_OGDF not enabled`

즉 wrapper/runner 파일은 있는 상태이고, 다음 단계는 OGDF를 실제로 설치/탐지해서 `USE_OGDF=ON`으로 static pipeline을 돌리는 것입니다.

권장 순서:
1. `./scripts/configure_with_local_ogdf.sh`
2. `./build/rewrite_r_harness --backend ogdf --mode static --manual-only --dump-dir dumps/static`
3. 첫 failure bundle 수집
4. earliest-stage 디버깅
5. static 통과 후 dummy mode 실행
