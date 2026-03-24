# progress30 current attempt note

이번 시도에서 `/mnt/data/lca_tree_stress_v5`를 다시 복구했고,
`/mnt/data/p30_local`, `/mnt/data/p30_release`를 다시 빌드했다.

그 다음 `run_progress30_case_transactional.py`를 이용해
`both_on_dense_1024_release`를 직접 실행했지만,
호출 세션이 반환된 뒤 runner 본체가 정상 finish state로 가지 못하고
`status.json`은 `running`, `heartbeat.json`은 stale 상태로 남았으며,
`result.json`은 생성되지 않았다.

관찰 시점 기준:
- outdir: `/mnt/data/p30_release_runs/both_on_dense_1024_release`
- `status.json` state: `running`
- `heartbeat.json` last update: stale
- `result.json`: missing
- solver child `/usr/bin/time` pid 1307: `<defunct>`

즉 current transactional runner는 direct foreground invocation에서
parent session 종료나 exec 반환과의 상호작용 때문에
terminal row를 남기지 못하는 failure mode가 여전히 있다.

따라서 이번 시도에서 authoritative release row는 추가하지 않았고,
기존 progress30 authoritative package를 유지한다.
