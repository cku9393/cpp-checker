# Failure Report: Attempt 1

- Timestamp: `2026-03-26 11:25:58 KST`
- Seed: `.ouroboros/seed_branch3_progress40_research_loop.yaml`
- Exit code: `1`
- Session ID: `orch_d7a962429eed`
- Execution ID: `exec_fcc14e7b961b`
- Analysis state file: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/.ouroboros/failure_analysis_state.json`
- Analysis state revision: `12`

## Result Summary

```text
╰──────────────────────────────────────────────────────╯
/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/structlog/_base.py:173: UserWarning: Remove `format_exc_info` from your processor chain if you want pretty exceptions.
  event_dict = proc(self._logger, method_name, event_dict)
2026-03-26T02:25:24.844909Z [error    ] orchestrator.session.create_failed error='Failed to append event: (sqlite3.OperationalError) attempt to write a readonly database\n[SQL: INSERT INTO events (id, aggregate_type, aggregate_id, event_type, payload, timestamp, consensus_id) VALUES (?, ?, ?, ?, ?, ?, ?)]\n[parameters: (\'c6e760e3-549c-4e09-b11a-1724b642cf3b\', \'session\', \'orch_d7a962429eed\', \'orchestrator.session.started\', \'{"execution_id": "exec_fcc14e7b961b", "seed_id": "seed_branch3_progress40_research_loop", "start_time": "2026-03-26T02:25:24.842087+00:00", "seed_goa ... (118 characters truncated) ...  the required lca_tree_stress_v5 standard, iterating after gate failures instead of treating the first failed acceptance as the end of useful work."}\', \'2026-03-26 02:25:24.842104\', None)]\n(Background on this error at: https://sqlalche.me/e/20/e3q8) (details: {\'event_id\': \'c6e760e3-549c-4e09-b11a-1724b642cf3b\', \'event_type\': \'orchestrator.session.started\'})' filename=session.py lineno=462 session_id=orch_d7a962429eed
Traceback (most recent call last):
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/engine/base.py", line 1967, in _exec_single_context
    self.dialect.do_execute(
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/engine/default.py", line 952, in do_execute
    cursor.execute(statement, parameters)
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/dialects/sqlite/aiosqlite.py", line 182, in execute
    self._adapt_connection._handle_exception(error)
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/dialects/sqlite/aiosqlite.py", line 342, in _handle_exception
    raise error
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/dialects/sqlite/aiosqlite.py", line 164, in execute
    self.await_(_cursor.execute(operation, parameters))
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/util/_concurrency_py3k.py", line 132, in await_only
    return current.parent.switch(awaitable)  # type: ignore[no-any-return,attr-defined] # noqa: E501
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/util/_concurrency_py3k.py", line 196, in greenlet_spawn
    value = await result
            ^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/aiosqlite/cursor.py", line 40, in execute
    await self._execute(self._cursor.execute, sql, parameters)
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/aiosqlite/cursor.py", line 32, in _execute
    return await self._conn._execute(fn, *args, **kwargs)
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/aiosqlite/core.py", line 160, in _execute
    return await future
           ^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/aiosqlite/core.py", line 63, in _connection_worker_thread
    result = function()
             ^^^^^^^^^^
sqlite3.OperationalError: attempt to write a readonly database

The above exception was the direct cause of the following exception:

Traceback (most recent call last):
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/ouroboros/persistence/event_store.py", line 185, in append
    await conn.execute(events_table.insert().values(**event.to_db_dict()))
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/ext/asyncio/engine.py", line 659, in execute
    result = await greenlet_spawn(
             ^^^^^^^^^^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/util/_concurrency_py3k.py", line 201, in greenlet_spawn
    result = context.throw(*sys.exc_info())
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/engine/base.py", line 1419, in execute
    return meth(
           ^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/sql/elements.py", line 527, in _execute_on_connection
    return connection._execute_clauseelement(
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/engine/base.py", line 1641, in _execute_clauseelement
    ret = self._execute_context(
          ^^^^^^^^^^^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/engine/base.py", line 1846, in _execute_context
    return self._exec_single_context(
           ^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/engine/base.py", line 1986, in _exec_single_context
    self._handle_dbapi_exception(
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/engine/base.py", line 2363, in _handle_dbapi_exception
    raise sqlalchemy_exception.with_traceback(exc_info[2]) from e
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/engine/base.py", line 1967, in _exec_single_context
    self.dialect.do_execute(
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/engine/default.py", line 952, in do_execute
    cursor.execute(statement, parameters)
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/dialects/sqlite/aiosqlite.py", line 182, in execute
    self._adapt_connection._handle_exception(error)
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/dialects/sqlite/aiosqlite.py", line 342, in _handle_exception
    raise error
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/dialects/sqlite/aiosqlite.py", line 164, in execute
    self.await_(_cursor.execute(operation, parameters))
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/util/_concurrency_py3k.py", line 132, in await_only
    return current.parent.switch(awaitable)  # type: ignore[no-any-return,attr-defined] # noqa: E501
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/util/_concurrency_py3k.py", line 196, in greenlet_spawn
    value = await result
            ^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/aiosqlite/cursor.py", line 40, in execute
    await self._execute(self._cursor.execute, sql, parameters)
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/aiosqlite/cursor.py", line 32, in _execute
    return await self._conn._execute(fn, *args, **kwargs)
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/aiosqlite/core.py", line 160, in _execute
    return await future
           ^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/aiosqlite/core.py", line 63, in _connection_worker_thread
    result = function()
             ^^^^^^^^^^
sqlalchemy.exc.OperationalError: (sqlite3.OperationalError) attempt to write a readonly database
[SQL: INSERT INTO events (id, aggregate_type, aggregate_id, event_type, payload, timestamp, consensus_id) VALUES (?, ?, ?, ?, ?, ?, ?)]
[parameters: ('c6e760e3-549c-4e09-b11a-1724b642cf3b', 'session', 'orch_d7a962429eed', 'orchestrator.session.started', '{"execution_id": "exec_fcc14e7b961b", "seed_id": "seed_branch3_progress40_research_loop", "start_time": "2026-03-26T02:25:24.842087+00:00", "seed_goa ... (118 characters truncated) ...  the required lca_tree_stress_v5 standard, iterating after gate failures instead of treating the first failed acceptance as the end of useful work."}', '2026-03-26 02:25:24.842104', None)]
(Background on this error at: https://sqlalche.me/e/20/e3q8)

The above exception was the direct cause of the following exception:

Traceback (most recent call last):
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/ouroboros/orchestrator/session.py", line 454, in create_session
    await self._event_store.append(event)
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/ouroboros/persistence/event_store.py", line 195, in append
    raise PersistenceError(
ouroboros.core.errors.PersistenceError: Failed to append event: (sqlite3.OperationalError) attempt to write a readonly database
[SQL: INSERT INTO events (id, aggregate_type, aggregate_id, event_type, payload, timestamp, consensus_id) VALUES (?, ?, ?, ?, ?, ?, ?)]
[parameters: ('c6e760e3-549c-4e09-b11a-1724b642cf3b', 'session', 'orch_d7a962429eed', 'orchestrator.session.started', '{"execution_id": "exec_fcc14e7b961b", "seed_id": "seed_branch3_progress40_research_loop", "start_time": "2026-03-26T02:25:24.842087+00:00", "seed_goa ... (118 characters truncated) ...  the required lca_tree_stress_v5 standard, iterating after gate failures instead of treating the first failed acceptance as the end of useful work."}', '2026-03-26 02:25:24.842104', None)]
(Background on this error at: https://sqlalche.me/e/20/e3q8) (details: {'event_id': 'c6e760e3-549c-4e09-b11a-1724b642cf3b', 'event_type': 'orchestrator.session.started'})
╭─────────────────────────────────── Error ────────────────────────────────────╮
│ Orchestrator error: Failed to create session: Failed to create session:      │
│ Failed to append event: (sqlite3.OperationalError) attempt to write a        │
│ readonly database                                                            │
│ [SQL: INSERT INTO events (id, aggregate_type, aggregate_id, event_type,      │
│ payload, timestamp, consensus_id) VALUES (?, ?, ?, ?, ?, ?, ?)]              │
│                                                                              │
│ (Background on this error at: https://sqlalche.me/e/20/e3q8) (details:       │
│ {'event_id': 'c6e760e3-549c-4e09-b11a-1724b642cf3b', 'event_type':           │
│ 'orchestrator.session.started'}) (details: {'session_id':                    │
│ 'orch_d7a962429eed'}) (details: {'execution_id': 'exec_fcc14e7b961b',        │
│ 'session_id': None})                                                         │
╰──────────────────────────────────────────────────────────────────────────────╯
artifacts/lca_tree_stress_v5/retry_loop/attempt_001_20260326_112454/attempt_guard.md
attempt guard passed
artifacts/lca_tree_stress_v5/retry_loop/attempt_001_20260326_112454/git_repo_health_post_failure.md
```

## Parsed AC Verdicts

- Failed ACs: none found
- Blocked ACs: none found
- Passed ACs: none found

## Git Status At Failure

```text
git status skipped: timed out after 10s
```

## Relevant Artifact Snapshots

### smoke

- Latest file: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/case05_smoke_random_recursive_mixed_n128_s1_L1_Q1_t3/run_case.stdout.txt`
- Latest mtime: `2026-03-26 09:11:38 KST`
- Summary file: `none`

### strong_gate

- Latest file: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore.latest_failure/certify.json`
- Latest mtime: `2026-03-26 07:26:33 KST`
- Summary file: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore.latest_failure/certify.json`

```text
{
  "verdict": "FAIL",
  "reasons": [
    "correctness_fuzz: 121 failing cases"
  ],
  "preset": "strong_gate",
  "stages": [
    {
      "name": "correctness_fuzz",
      "status": "FAIL",
      "cases": 900,
      "timeouts": 121,
      "re_wa": 0,
      "limit_scale": 1.0,
      "sec_max": null,
      "case_sec_max": null,
      "scale_fail": []
    }
  ]
}
```

### boj3s_gate

- Latest file: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1.latest_failure/certify.json`
- Latest mtime: `2026-03-26 10:15:54 KST`
- Summary file: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1.latest_failure/certify.json`

```text
{
  "verdict": "FAIL",
  "reasons": [
    "correctness_smoke: 72 failing cases",
    "hard_scaling_strict: 90 failing cases",
    "hard_scaling_strict: comb_core: alpha=2.029 > 1.350",
    "hard_scaling_strict: comb_core: ratio=4.082 > 2.600",
    "hard_scaling_strict: comb_plus_unary: alpha=2.051 > 1.350",
    "hard_scaling_strict: comb_plus_unary: ratio=4.145 > 2.600",
    "hard_scaling_strict: multi_comb_core: alpha=2.024 > 1.350",
    "hard_scaling_strict: multi_comb_core: ratio=4.067 > 2.600",
    "boj_3s_large_adversarial: 30 failing cases",
    "boj_3s_large_mix: 15 failing cases"
  ],
  "preset": "boj_3s_hard_gate",
  "stages": [
    {
      "name": "correctness_smoke",
      "status": "FAIL",
      "cases": 288,
      "timeouts": 72,
      "re_wa": 0,
      "limit_scale": 1.0,
      "sec_max": null,
      "case_sec_max": null,
      "scale_fail": []
    },
    {
      "name": "hard_scaling_strict",
      "status": "FAIL",
      "cases": 108,
      "timeouts": 90,
      "re_wa": 0,
      "limit_scale": 1.0,
      "sec_max": null,
      "case_sec_max": 2.7,
      "scale_fail": [
        "comb_core: alpha=2.029 > 1.350",
        "comb_core: ratio=4.082 > 2.600",
        "comb_plus_unary: alpha=2.051 > 1.350",
        "comb_plus_unary: ratio=4.145 > 2.600",
        "multi_comb_core: alpha=2.024 > 1.350",
        "multi_comb_core: ratio=4.067 > 2.600"
      ]
    },
    {
      "name": "boj_3s_large_adversarial",
      "status": "FAIL",
      "cases": 30,
      "timeouts": 30,
      "re_wa": 0,
      "limit_scale": 1.0,
      "sec_max": 2.55,
      "case_sec_max": 2.7,
      "scale_fail": []
    },
    {
      "name": "boj_3s_large_mix",
      "status": "FAIL",
      "cases": 18,
      "timeouts": 15,
      "re_wa": 0,
      "limit_scale": 1.0,
      "sec_max": null,
      "case_sec_max": 2.7,
      "scale_fail": []
    }
  ]
}
```

### hunt

- Latest file: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/hunt_summary.md`
- Latest mtime: `2026-03-25 03:18:57 KST`
- Summary file: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/hunt_summary.md`

```text
# Hardest-case hunt

상위 케이스는 현재 solver 기준으로 가장 느리게 측정된 조합이다. 느린 풀이를 반박하려면 이 목록에서 timeout/scale 문제가 없어야 한다.

| rank | mode | n | seed | L | Q | sec | rss_kb | val_ok | case_dir |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | caterpillar_rect_dense | 64 | 1 | 1 | 0 | 0.079 | 4496 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/caterpillar_rect_dense/n64/seed1_L1_Q0 |
| 2 | comb_rect_dense | 64 | 1 | 1 | 0 | 0.075 | 4400 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/comb_rect_dense/n64/seed1_L1_Q0 |
| 3 | comb_dense | 64 | 1 | 1 | 1 | 0.074 | 4464 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/comb_dense/n64/seed1_L1_Q1 |
| 4 | chain_unary | 64 | 1 | 1 | 1 | 0.073 | 2784 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/chain_unary/n64/seed1_L1_Q1 |
| 5 | comb_dense | 64 | 1 | 1 | 0 | 0.072 | 4432 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/comb_dense/n64/seed1_L1_Q0 |
| 6 | caterpillar_rect_dense | 64 | 1 | 1 | 1 | 0.066 | 4432 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/caterpillar_rect_dense/n64/seed1_L1_Q1 |
| 7 | comb_rect_dense | 64 | 1 | 1 | 1 | 0.061 | 4416 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/comb_rect_dense/n64/seed1_L1_Q1 |
| 8 | caterpillar_rect_dense | 64 | 1 | 0 | 1 | 0.059 | 3744 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/caterpillar_rect_dense/n64/seed1_L0_Q1 |
| 9 | comb_rect_dense | 64 | 1 | 0 | 1 | 0.054 | 3776 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/comb_rect_dense/n64/seed1_L0_Q1 |
| 10 | comb_dense | 64 | 1 | 0 | 0 | 0.050 | 3744 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/comb_dense/n64/seed1_L0_Q0 |
| 11 | caterpillar_mixed | 64 | 1 | 1 | 0 | 0.049 | 4464 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/caterpillar_mixed/n64/seed1_L1_Q0 |
| 12 | caterpillar_mixed | 64 | 1 | 1 | 1 | 0.047 | 4464 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/caterpillar_mixed/n64/seed1_L1_Q1 |
| 13 | balanced_dense | 64 | 1 | 1 | 0 | 0.046 | 2880 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/balanced_dense/n64/seed1_L1_Q0 |
| 14 | comb_rect_dense | 64 | 1 | 0 | 0 | 0.044 | 3728 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/comb_rect_dense/n64/seed1_L0_Q0 |
| 15 | balanced_sibling | 64 | 1 | 1 | 1 | 0.042 | 2928 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/balanced_sibling/n64/seed1_L1_Q1 |
| 16 | caterpillar_mixed | 64 | 1 | 0 | 1 | 0.040 | 3824 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/caterpillar_mixed/n64/seed1_L0_Q1 |
| 17 | multi_comb_rect | 64 | 1 | 0 | 1 | 0.039 | 3296 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/multi_comb_rect/n64/seed1_L0_Q1 |
| 18 | balanced_dense | 64 | 1 | 1 | 1 | 0.038 | 2912 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/balanced_dense/n64/seed1_L1_Q1 |
| 19 | broom_mixed | 64 | 1 | 0 | 0 | 0.038 | 3120 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/broom_mixed/n64/seed1_L0_Q0 |
| 20 | multi_comb_rect | 64 | 1 | 1 | 1 | 0.038 | 3520 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/multi_comb_rect/n64/seed1_L1_Q1 |
```

## Session Log Excerpt

```text
(no session-specific log lines found)
```

## Workflow Log Tail

```text
╰──────────────────────────────────────────────────────╯
/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/structlog/_base.py:173: UserWarning: Remove `format_exc_info` from your processor chain if you want pretty exceptions.
  event_dict = proc(self._logger, method_name, event_dict)
2026-03-26T02:25:24.844909Z [error    ] orchestrator.session.create_failed error='Failed to append event: (sqlite3.OperationalError) attempt to write a readonly database\n[SQL: INSERT INTO events (id, aggregate_type, aggregate_id, event_type, payload, timestamp, consensus_id) VALUES (?, ?, ?, ?, ?, ?, ?)]\n[parameters: (\'c6e760e3-549c-4e09-b11a-1724b642cf3b\', \'session\', \'orch_d7a962429eed\', \'orchestrator.session.started\', \'{"execution_id": "exec_fcc14e7b961b", "seed_id": "seed_branch3_progress40_research_loop", "start_time": "2026-03-26T02:25:24.842087+00:00", "seed_goa ... (118 characters truncated) ...  the required lca_tree_stress_v5 standard, iterating after gate failures instead of treating the first failed acceptance as the end of useful work."}\', \'2026-03-26 02:25:24.842104\', None)]\n(Background on this error at: https://sqlalche.me/e/20/e3q8) (details: {\'event_id\': \'c6e760e3-549c-4e09-b11a-1724b642cf3b\', \'event_type\': \'orchestrator.session.started\'})' filename=session.py lineno=462 session_id=orch_d7a962429eed
Traceback (most recent call last):
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/engine/base.py", line 1967, in _exec_single_context
    self.dialect.do_execute(
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/engine/default.py", line 952, in do_execute
    cursor.execute(statement, parameters)
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/dialects/sqlite/aiosqlite.py", line 182, in execute
    self._adapt_connection._handle_exception(error)
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/dialects/sqlite/aiosqlite.py", line 342, in _handle_exception
    raise error
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/dialects/sqlite/aiosqlite.py", line 164, in execute
    self.await_(_cursor.execute(operation, parameters))
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/util/_concurrency_py3k.py", line 132, in await_only
    return current.parent.switch(awaitable)  # type: ignore[no-any-return,attr-defined] # noqa: E501
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/util/_concurrency_py3k.py", line 196, in greenlet_spawn
    value = await result
            ^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/aiosqlite/cursor.py", line 40, in execute
    await self._execute(self._cursor.execute, sql, parameters)
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/aiosqlite/cursor.py", line 32, in _execute
    return await self._conn._execute(fn, *args, **kwargs)
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/aiosqlite/core.py", line 160, in _execute
    return await future
           ^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/aiosqlite/core.py", line 63, in _connection_worker_thread
    result = function()
             ^^^^^^^^^^
sqlite3.OperationalError: attempt to write a readonly database

The above exception was the direct cause of the following exception:

Traceback (most recent call last):
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/ouroboros/persistence/event_store.py", line 185, in append
    await conn.execute(events_table.insert().values(**event.to_db_dict()))
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/ext/asyncio/engine.py", line 659, in execute
    result = await greenlet_spawn(
             ^^^^^^^^^^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/util/_concurrency_py3k.py", line 201, in greenlet_spawn
    result = context.throw(*sys.exc_info())
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/engine/base.py", line 1419, in execute
    return meth(
           ^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/sql/elements.py", line 527, in _execute_on_connection
    return connection._execute_clauseelement(
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/engine/base.py", line 1641, in _execute_clauseelement
    ret = self._execute_context(
          ^^^^^^^^^^^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/engine/base.py", line 1846, in _execute_context
    return self._exec_single_context(
           ^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/engine/base.py", line 1986, in _exec_single_context
    self._handle_dbapi_exception(
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/engine/base.py", line 2363, in _handle_dbapi_exception
    raise sqlalchemy_exception.with_traceback(exc_info[2]) from e
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/engine/base.py", line 1967, in _exec_single_context
    self.dialect.do_execute(
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/engine/default.py", line 952, in do_execute
    cursor.execute(statement, parameters)
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/dialects/sqlite/aiosqlite.py", line 182, in execute
    self._adapt_connection._handle_exception(error)
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/dialects/sqlite/aiosqlite.py", line 342, in _handle_exception
    raise error
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/dialects/sqlite/aiosqlite.py", line 164, in execute
    self.await_(_cursor.execute(operation, parameters))
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/util/_concurrency_py3k.py", line 132, in await_only
    return current.parent.switch(awaitable)  # type: ignore[no-any-return,attr-defined] # noqa: E501
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/sqlalchemy/util/_concurrency_py3k.py", line 196, in greenlet_spawn
    value = await result
            ^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/aiosqlite/cursor.py", line 40, in execute
    await self._execute(self._cursor.execute, sql, parameters)
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/aiosqlite/cursor.py", line 32, in _execute
    return await self._conn._execute(fn, *args, **kwargs)
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/aiosqlite/core.py", line 160, in _execute
    return await future
           ^^^^^^^^^^^^
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/aiosqlite/core.py", line 63, in _connection_worker_thread
    result = function()
             ^^^^^^^^^^
sqlalchemy.exc.OperationalError: (sqlite3.OperationalError) attempt to write a readonly database
[SQL: INSERT INTO events (id, aggregate_type, aggregate_id, event_type, payload, timestamp, consensus_id) VALUES (?, ?, ?, ?, ?, ?, ?)]
[parameters: ('c6e760e3-549c-4e09-b11a-1724b642cf3b', 'session', 'orch_d7a962429eed', 'orchestrator.session.started', '{"execution_id": "exec_fcc14e7b961b", "seed_id": "seed_branch3_progress40_research_loop", "start_time": "2026-03-26T02:25:24.842087+00:00", "seed_goa ... (118 characters truncated) ...  the required lca_tree_stress_v5 standard, iterating after gate failures instead of treating the first failed acceptance as the end of useful work."}', '2026-03-26 02:25:24.842104', None)]
(Background on this error at: https://sqlalche.me/e/20/e3q8)

The above exception was the direct cause of the following exception:

Traceback (most recent call last):
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/ouroboros/orchestrator/session.py", line 454, in create_session
    await self._event_store.append(event)
  File "/Users/free_1/.local/share/uv/tools/ouroboros-ai/lib/python3.12/site-packages/ouroboros/persistence/event_store.py", line 195, in append
    raise PersistenceError(
ouroboros.core.errors.PersistenceError: Failed to append event: (sqlite3.OperationalError) attempt to write a readonly database
[SQL: INSERT INTO events (id, aggregate_type, aggregate_id, event_type, payload, timestamp, consensus_id) VALUES (?, ?, ?, ?, ?, ?, ?)]
[parameters: ('c6e760e3-549c-4e09-b11a-1724b642cf3b', 'session', 'orch_d7a962429eed', 'orchestrator.session.started', '{"execution_id": "exec_fcc14e7b961b", "seed_id": "seed_branch3_progress40_research_loop", "start_time": "2026-03-26T02:25:24.842087+00:00", "seed_goa ... (118 characters truncated) ...  the required lca_tree_stress_v5 standard, iterating after gate failures instead of treating the first failed acceptance as the end of useful work."}', '2026-03-26 02:25:24.842104', None)]
(Background on this error at: https://sqlalche.me/e/20/e3q8) (details: {'event_id': 'c6e760e3-549c-4e09-b11a-1724b642cf3b', 'event_type': 'orchestrator.session.started'})
╭─────────────────────────────────── Error ────────────────────────────────────╮
│ Orchestrator error: Failed to create session: Failed to create session:      │
│ Failed to append event: (sqlite3.OperationalError) attempt to write a        │
│ readonly database                                                            │
│ [SQL: INSERT INTO events (id, aggregate_type, aggregate_id, event_type,      │
│ payload, timestamp, consensus_id) VALUES (?, ?, ?, ?, ?, ?, ?)]              │
│                                                                              │
│ (Background on this error at: https://sqlalche.me/e/20/e3q8) (details:       │
│ {'event_id': 'c6e760e3-549c-4e09-b11a-1724b642cf3b', 'event_type':           │
│ 'orchestrator.session.started'}) (details: {'session_id':                    │
│ 'orch_d7a962429eed'}) (details: {'execution_id': 'exec_fcc14e7b961b',        │
│ 'session_id': None})                                                         │
╰──────────────────────────────────────────────────────────────────────────────╯
artifacts/lca_tree_stress_v5/retry_loop/attempt_001_20260326_112454/attempt_guard.md
attempt guard passed
artifacts/lca_tree_stress_v5/retry_loop/attempt_001_20260326_112454/git_repo_health_post_failure.md
```

See `failure_breakdown.md` for the per-AC phase split, structural hotspot analysis, and the refinement notes to carry into the next retry.