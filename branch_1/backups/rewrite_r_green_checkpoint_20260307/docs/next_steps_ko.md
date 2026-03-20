# 다음 단계

1. OGDF wrapper 5개 채우기
2. `runStaticPipelineCaseDumpAware` manual-only 실행
3. 첫 failure bundle 저장
4. 가장 이른 stage만 디버깅
5. static 통과 후 `runDummyGraftCaseDumpAware` 실행
6. 첫 failure bundle 저장
7. 다시 가장 이른 stage만 디버깅
8. 마지막에만 `rewriteR_fallback + normalizeTouchedRegion`

주의: 이 번들은 wrapper/runner와 dump-aware 스켈레톤을 제공합니다. 프로젝트 고유 함수는 `StubHarnessOps`를 실제 구현으로 교체해야 합니다.
