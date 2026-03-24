# Round45 Bundle Archive Index

이 디렉터리는 원래 `lca_tree_stress_v5`로 전달된 Round 45 hold bundle을 정리한 아카이브다.
현재 폴더 이름은 `round45_bundle_archive`지만, historical report와 manifest 안의 옛 경로 표기는 그대로 유지한다.

루트에서 바로 쓰는 canonical entrypoints:

- `README.md`
- `solve.cpp`
- `build.sh`
- `build_clang.sh`
- `gate.sh`
- `gate_boj3s.sh`
- `hunt.sh`
- `run_case.sh`
- `run_probe_list.py`
- `gen_case.py`
- `validator.py`
- `generators/`
- `suite_presets/`
- `probe_inputs/`
- `corr_inputs/`

정리된 보관 영역:

- `history/`
  - hold status 문서와 solver snapshot 보관
- `reports/`
  - round별 Markdown/JSON/TSV 결과
- `runs/`
  - smoke, sanity, probe-before 실행 산출물
- `build/`
  - 번들에 포함돼 있던 기존 binary/object 산출물
- `fixtures/`
  - round44/round45 stage, truth, panel TSV 입력

작업 재개용 현재 workspace는 이 아카이브가 아니라 `branch_2_2/round45_resume/`와 `branch_2_2/round45_resume.py`를 사용한다.

주의:

- shell wrapper와 일부 profiling helper는 GNU `time -f`, `timeout`, static link 전제를 그대로 가진다.
- 현재 macOS 환경에서는 구조 확인은 끝냈지만, archive root의 원형 shell flow 전체를 그대로 재현하지는 못했다.
