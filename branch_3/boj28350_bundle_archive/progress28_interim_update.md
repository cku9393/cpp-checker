# progress28 interim update

이번 시점까지 추가로 authoritative row가 생긴 LOCAL 512 rerun은 아래 두 개다.

| run | elapsed_sec | validator |
| --- | ---: | --- |
| before_both_on_dense_512_base | 123.94 | OK |
| before_connector_only_dense_512_sampled | 119.23 | OK |

나머지 LOCAL 512 rerun의 현재 진행 흔적은 아래와 같다.

```
before_both_on_dense_512_sampled: [progress] phase=deletion_checkpoint run_tag=before_both_on_dense_512_sampled delta_mode=both_on profile_mode=PROFILE_SAMPLED sampled=yes sample_stride=8 sample_warmup=64 progress_stride=16 deletion=320/512 x=321 touched=0 detailed_sampled=1 elapsed_ms=103329
before_both_on_multi_512_sampled: not started yet
after_both_on_dense_512_base: not started yet
after_both_on_dense_512_sampled: not started yet
after_both_on_multi_512_sampled: not started yet
```

RELEASE rerun과 4096 representative authoritative close는 아직 시작하지 않았거나 완료 row가 없다.
