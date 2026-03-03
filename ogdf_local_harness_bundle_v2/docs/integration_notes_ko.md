# integration notes

이 번들은 실제 wrapper/runner 소스 파일을 포함합니다.

- `src/ogdf_wrapper.cpp`: OGDF -> RawSpqrDecomp 복원
- `src/runners.cpp`: dump-aware static/dummy runner
- `src/project_hooks_stub.cpp`: 컴파일 가능한 기본 stub hooks
- `src/project_hooks_example_adapter.cpp`: 프로젝트 실제 함수로 교체할 어댑터 예시

즉 이전 번들의 문제였던 "wrapper/runner가 없음"은 해소했고,
이제 남은 것은 **프로젝트 고유 함수 연결**입니다.
