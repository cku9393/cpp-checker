# RESULT

static random 검증 요약:

- `--mode static --seed 1 --rounds 100` 통과
- `--mode static --seed 1 --rounds 1000` 통과
- `--mode static --seed 1..10 --rounds 1000` 전부 통과
- `--mode static --seed 1..20 --rounds 5000` 전부 통과

결론:

- earliest random failure는 관측되지 않음
- 총 무실패 케이스 수는 `111100`
- static 경로는 현재 단계 기준으로 충분히 안정적

보관 목적:

- dummy 단계 진행 전 static green 기준선 백업
- `dumps/static` 디렉터리는 이 checkpoint 안에 빈 상태로 보관
