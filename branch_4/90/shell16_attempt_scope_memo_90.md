# Shell16 Attempt Scope Memo 90

## selected scope

Selected attempt scope: `shell16_survivor_probe_attempt`.

This scope generated the shell16 candidate universe and ran the local-exact, plus-one, and theorem-preserving survivor probes. It did not promote a shell16 theorem and did not claim a general theorem.

## scope decision

| scope | selected | decision |
| --- | ---: | --- |
| `shell16_candidate_universe_attempt` | 0 | candidate-only fallback was not needed because the candidate count was small |
| `shell16_survivor_probe_attempt` | 1 | selected and completed |
| `shell16_full_scan_no_promotion` | 0 | not needed as a distinct broader scope; survivor probe completed under the preflight guard |
| `shell16_attempt_aborted_by_precondition` | 0 | all required preconditions passed in the validated run |

## result

- raw candidates: `8`
- canonical candidates: `4`
- outside-bounded candidates: `4`
- local exact survivors: `2`
- plus-one survivors: `0`
- theorem-preserving survivors: `0`
- classification: `shell16_probe_completed_local_exact_survivors_present_no_theorem_preserving_survivors`
- theorem promotion: `0`

Promotion review completed: runtime facts and limited boundary lemmas are promotable; full shell16 theorem promotion is not. The selected limited bridge theorem proof is complete under current scope, and support-bound formalization plus support-growth partition plus partial family-chain lift are installed. The next required action is `family_chain_lift_map_refinement`.
