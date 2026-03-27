# BOJ28350 현재 상태 요약

## 최신 기준 패키지

가장 앞선 기준 패키지는 `progress40`이다.

기준 파일

`boj28350_literature_progress40_layout_signature_reuse_gate.cpp`
`boj28350_progress40_layout_signature_reuse_gate_report.md`
`boj28350_progress40_results_merged.json`

## pre-rewrite review checkpoint

`2026-03-26` 기준, 다음 major solver rewrite 또는 pivot decision을 열기 전에 아래 두 source set review completion이 모두 확인됐다.

1. source set A: `branch_3` working set review 완료
   `boj28350_resume/README.md`, `boj28350_resume/current_state_summary.md`, `boj28350_resume/next_session_briefing.md`, `boj28350_complete_master_document_partA_raw.md`, `boj28350_integrated_technical_history.md`, `boj28350_literature_progress7_bcdecomp_report.md`, `literature_grade_proof_package.md`, `boj28350_resume/boj28350_branch_3_solver.cpp`
2. source set B: bundled `progress40` authoritative materials review 완료
   `boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp`, `boj28350_bundle_archive/boj28350_progress40_layout_signature_reuse_gate_report.md`, `boj28350_bundle_archive/boj28350_progress40_results_merged.json`

세부 evidence는 `boj28350_resume/next_session_briefing.md`의 6.1, 6.2, 6.2.1, 6.2.2에 남겼고, 다음 rewrite/pivot은 이 두 source set review completion이 선행 확인된 상태에서만 진행한다.

## 현재 authoritative 상태

현재 확보된 범위는 아래와 같다.

`execution layer validation` 완료
`smoke and gate` 완료
`authoritative LOCAL 512 matrix` 완료
`both_on_multi_1024_release` 1회 완료

## 최신 direct aggregate 결론

progress40 authoritative clean LOCAL sampled aggregate 기준으로는 아래와 같다.

`signature source load and materialize` 0.378774ms, 24.9643퍼센트
`layout signature compare and reuse gate core` 0.379830ms, 25.0339퍼센트
`zero-span eligibility and fastpath commit` 0.758605ms, 49.9983퍼센트
`connector hotpath normalize reuse` 0.000054ms, 0.0036퍼센트

strict dominant는 없지만 largest residual은 `zero-span eligibility and fastpath commit`이다.

현재 가장 안전한 결론은 아래 한 줄이다.

`next pivot after layout-gate round: zero-span eligibility and fastpath commit`

## 현재 남아 있는 partial authoritative 범위

`missing_runs` 기준 미완료 항목은 아래와 같다.
`progress38_authoritative_close_not_completed`
`both_on_dense_1024_release`
`both_on_dense_1024_release_repeat`
`both_on_dense_4096_release`
`both_on_multi_4096_release`

## 핵심 병목

첫째, 운영 병목은 여전히 long run terminal row persistence failure다. 짧은 케이스와 512 sampled one-off에서는 terminal `result.json` 생성이 확인됐지만, dense 1024 release and repeat, 4096 representative는 아직 안정적으로 닫히지 않았다.

둘째, 알고리즘 병목은 `zero-span eligibility and fastpath commit`이다. 다만 share가 49.9983퍼센트라 매우 근접한 largest residual이고, 다음 라운드에서는 이 축을 authoritative basis에서 다시 검증해야 한다.

## 이번 번들에 포함한 것

이 zip에는 `/mnt/data` 아래의 현재 프로젝트 관련 파일을 최대한 넓게 포함했다.

포함 기준

`boj28350_*`
`progress*`
`run_*`
`merge_*`
`reconcile_*`
`lca_tree_stress_v5.zip`

기존 zip 출력물과 이번 새 zip 자체는 중복 방지를 위해 제외했다.
