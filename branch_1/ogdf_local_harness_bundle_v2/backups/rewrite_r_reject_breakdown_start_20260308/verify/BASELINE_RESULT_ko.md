rewrite-r random staged campaign finished with no failures.

- total rewriteCalls: 111100
- compactReadyCount: 63667 (57.31%)
- compactRejectedFallbackCount: 47433 (42.69%)
- backendBuildRawDirectCount: 63667 (57.31%)
- backendBuildRawFallbackCount: 0 (0.00%)
- rewriteRandomPassCount: 111100

Commands covered:
- seed=1 rounds=100
- seed=1 rounds=1000
- seed=1..10 rounds=1000
- seed=1..20 rounds=5000

Interpretation:
- No earliest-stage failure was found in staged random.
- Direct local compact path is active in more than half of rewrite calls.
- Whole-core rebuild fallback is still used frequently enough that local compact path optimization remains worthwhile.
