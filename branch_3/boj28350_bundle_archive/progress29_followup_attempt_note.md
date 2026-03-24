# progress29 follow-up attempt note

이번 후속 시도에서 clean LOCAL 512 missing rows를 순차 rerun해 관찰된 elapsed를 기록했다.
이 값들은 세션 중 stdout에서 확인한 값이며, 환경 리셋 이후 raw run 디렉토리가 남아 있지 않아 authoritative result row로 승격하지는 않았다.

## session-observed LOCAL 512 rows

- before_connector_only_dense_512_base: validator_ok=True, elapsed_sec=27.59
- before_both_on_dense_512_base: validator_ok=True, elapsed_sec=26.66
- before_connector_only_dense_512_sampled: validator_ok=True, elapsed_sec=27.24
- before_both_on_dense_512_sampled: validator_ok=True, elapsed_sec=27.33
- after_connector_only_dense_512_base: validator_ok=True, elapsed_sec=27.58
- after_both_on_dense_512_base: validator_ok=True, elapsed_sec=27.29
- after_connector_only_dense_512_sampled: validator_ok=True, elapsed_sec=27.71
- after_both_on_dense_512_sampled: validator_ok=True, elapsed_sec=27.25

## status

progress29 main package는 여전히 partial authoritative 상태다. 위 값들은 resume guidance 용이다.
