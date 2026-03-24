# SHIP READY DECISION

- default solver path = rewrite-seq
- hard compare status = green
- random sanity status = green
- direct smoke status = green
- legacy path = diagnostic-only
- release bundle path = `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_1/ogdf_local_harness_bundle_v2/artifacts/rewrite_seq_release_delivery_20260324_103121.zip`

## Rollback Policy
- 운영 rollback은 legacy를 default로 되돌리는 변경이 아니라 diagnostic-only `--mode rewrite-r` 경로로 제한한다.
- compare rollback 진단은 `--baseline legacy`를 opt-in diagnostic 모드로만 사용한다.

## Open Risk / Known Limitation
- large campaign mismatch가 다시 나오면 earliest case 1개만 고정해서 replay 체인으로 내린다.
- summary artifact는 atomic + validated 규칙을 유지해야 한다.

## Decision
- 현재 release gate 기준으로 ship-ready 판단은 `yes`다.
- 다음 action은 merge/tag/hand-off다.
