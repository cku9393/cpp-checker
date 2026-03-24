# Rewrite Seq Regression Policy

hard gate:

- source: `regressions/rewrite_seq_cases.json`
- case count: `6`
- role: behavior regression 방지용 immutable gate
- input policy:
  - `seed/tcIndex/targetStep`는 traceability 용도
  - 실제 hard input은 manifest의 `inputExplicit` snapshot이 source of truth

diagnostic-only:

- source: `diagnostics/rewrite_seq_diagnostic_cases.json`
- role: exploratory replay/random 추적용
- rule:
  - hard gate 실패 기준에 포함하지 않음
  - 새로운 디버깅 케이스는 여기에 먼저 추가
  - hard gate 6개는 늘리지 않음
