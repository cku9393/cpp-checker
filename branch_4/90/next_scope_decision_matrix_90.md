# Next Scope Decision Matrix 90

## recommendation

Top recommendation: `general_gap_bridge_formalization`.

Second recommendation: `shell16_preflight_then_attempt`.

Third recommendation: `boj_bridge_formalization`.

## scoring

| candidate | readiness | proof value | engineering cost | risk | dependency clarity | expected progress | order | final recommendation |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `broader_general_gap_theorem` | 78 | 95 | 45 | 55 | 80 | 90 | 1 | `general_gap_bridge_formalization` |
| `shell16_readiness` | 62 | 75 | 70 | 60 | 65 | 78 | 2 | `shell16_preflight_then_attempt` |
| `BOJ_solver_bridge` | 58 | 65 | 50 | 50 | 60 | 62 | 3 | `boj_bridge_formalization` |
| `higher_support_necessity` | 35 | 70 | 90 | 80 | 35 | 45 | 4 | `higher_support_bound_formalization` |
| `archive_wide_history_provenance_cleanup` | 80 | 35 | 30 | 20 | 85 | 35 | 5 | `archive_cleanup_before_expansion` |

## rationale

`general_gap_bridge_formalization` is first because it decides the proof obligation that shell16 and higher-support would serve. Running shell16 first would generate more finite evidence, but it would not answer whether shell16 is necessary or sufficient for a broader theorem. Higher-support is deferred because no current bridge or bound proves it is necessary. BOJ bridge is useful but should not become solver implementation until the proof-to-construction bridge is specified. Archive cleanup is nonblocking.

## previous next exact target

`general_gap_bridge_formalization`

## after bridge formalization

The bridge formalization round refined this target into `prove_minimal_counterexample_reduction`; the minimal-counterexample round advanced that target to a proof-ready skeleton, and the tail bridge round proved checked-tail absorption while routing extension tails to shell16.

After shell16 preflight, the next action order is:

1. `shell16_attempt`
2. `prove_or_refine_tail_absorption_step`
3. `limited_bridge_theorem_proof_attempt`

The preflight contract is ready, but shell16 has not been attempted and no shell16 survivor count is claimed.
