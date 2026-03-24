# progress30 rebind commit and metadata patch report
이번 continuation에서는 progress30 partial 패키지에 authoritative LOCAL 512 dense before와 after rows를 채워 넣고, clean multi 1024 release 1회를 반영했다. release and representative는 아직 partial authoritative 상태다.

## 이번에 실제로 바꾼 것
`ENABLE_REBIND_COMMIT_OPT` 브랜치는 유지했다. `time_rcommit_*`, `rcommit_*` direct subaxis와 transactional runner support artifact는 그대로 유지했다.

## support artifact
`run_progress30_case_transactional.py`
`progress30_case_journal.jsonl`
`merge_progress30_results.py`
`progress30_resume_remaining.sh`

## smoke와 gate
- smoke_p30_multi64_after: validator_ok=True, elapsed_sec=4.0
- gate_connector_only_dense_256_after: validator_ok=True, elapsed_sec=5.46
- gate_both_on_dense_256_after: validator_ok=True, elapsed_sec=5.55
- gate_both_on_multi_512_after: validator_ok=True, elapsed_sec=8.35

모든 gate rerun은 `validator OK`, `local_active_mismatch=0`, `local_active_partition_mismatch=0`, `debug_touched_missing_classes=0`, `piece_materialize_fallback_calls=0`, `support_rebuild_fallback_calls=0`, `unanimous_baseline_path_calls=0`를 유지했다.

## authoritative LOCAL 512 dense before와 after
| case | before base sec | after base sec | before sampled sec | after sampled sec |
| --- | ---: | ---: | ---: | ---: |
| connector_only, comb_rect_dense 512 LOCAL | 28.20 | 27.19 | 26.81 | 28.33 |
| both_on, comb_rect_dense 512 LOCAL | 26.63 | 27.32 | 27.66 | 28.33 |
| both_on, multi_comb_rect 512 LOCAL, sampled | carry-forward | carry-forward | 8.33 | carry-forward evidence |

## clean multi 1024 release
| case | validator | elapsed sec |
| --- | --- | ---: |
| both_on, multi_comb_rect 1024 RELEASE | True | 19.65 |

## direct rebind-commit sampled evidence
| run | same-target fastpath ms | install+swap ms | patch+slot-owner ms | connector commit ms | elapsed sec | note |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| after_connector_only_dense_512_sampled | 0.000000 | 0.001524 | 0.003288 | 0.000027 | 28.33 | authoritative |
| after_both_on_dense_512_sampled | 0.000000 | 8.612490 | 13.488693 | 0.000027 | 28.33 | authoritative |
| after_both_on_multi_512_sampled | 0.000000 | 1.561898 | 2.400871 | 0.000000 | 8.51 | carry-forward evidence from prior partial progress30 package |

## direct sampled aggregate
| subaxis | aggregate_ms | share_pct |
| --- | ---: | ---: |
| same-target fastpath and patch suppression | 0.000000 | 0.0 |
| rebind target install and pointer swap | 10.175912 | 39.0 |
| metadata patch apply and slot-owner update | 15.892852 | 61.0 |
| connector hotpath commit apply | 0.000054 | 0.0 |


이 aggregate 기준으로 현재까지의 가장 안전한 partial direct conclusion은 `metadata patch apply and slot-owner update`가 largest residual이라는 것이다.

## 아직 미완료인 authoritative row
- progress29 close
  - both_on, comb_rect_dense 1024 RELEASE
  - both_on, comb_rect_dense 1024 RELEASE repeat
  - both_on, comb_rect_dense 4096 RELEASE
  - both_on, multi_comb_rect 4096 RELEASE
- progress30 release and representative
  - both_on, comb_rect_dense 1024 RELEASE
  - both_on, comb_rect_dense 1024 RELEASE repeat
  - both_on, comb_rect_dense 4096 RELEASE
  - both_on, multi_comb_rect 4096 RELEASE

## caveat
이번 progress30은 LOCAL 512 dense matrix는 authoritative하게 회수했지만, direct aggregate에는 `after_both_on_multi_512_sampled` carry-forward evidence가 포함되어 있다. 따라서 current direct conclusion은 improved but still partial authoritative conclusion이다.

## 마지막 결론
`next pivot after rebind-commit round: metadata patch apply and slot-owner update`
