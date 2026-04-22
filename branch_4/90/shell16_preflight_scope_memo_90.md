# Shell16 Preflight Scope Memo 90

## purpose

This memo fixes the scope of `shell16_preflight_refactor`. The current round does not run shell16 exhaustion, does not promote a shell16 theorem, and does not assert shell16 survivor counts. It only installs the contract needed to attempt shell16 safely in a later round.

## selected scope

Selected scope: `shell16_candidate_universe_dry_run`.

This means the next shell boundary is first-class as a preflight target: paths, row schemas, audit gates, failure gates, and the tail-escape integration are specified before any expensive scan or theorem promotion is allowed.

## scope candidates

| scope | allowed this round | decision |
| --- | ---: | --- |
| `shell16_contract_only` | 1 | acceptable but weaker than selected dry-run contract |
| `shell16_candidate_universe_dry_run` | 1 | selected |
| `shell16_bounded_preflight_probe` | 0 | deferred until this contract is reviewed |
| `shell16_full_attempt` | 0 | out of scope |

## guardrails

- `shell16_full_attempt` remains out of scope.
- shell16 theorem promotion is forbidden in this round.
- shell16 survivor-zero claims are forbidden in this round.
- support8 lock, required doc/artifact gates, provenance audit, and family-chain closure remain unchanged.
- Existing shell15 artifacts are reference patterns only; they are not shell16 results.

## relationship to tail bridge

The tail bridge already separates checked tail absorption from the first unchecked boundary. This preflight round turns that boundary into a concrete shell16 attempt protocol. It does not close the shell16 escape.

