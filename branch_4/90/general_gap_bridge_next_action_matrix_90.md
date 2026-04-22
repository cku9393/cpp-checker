# General Gap Bridge Next Action Matrix 90

## recommendation after lift-map refinement phase2

Top recommendation: `family_chain_lift_phase3_if_needed`.

Second recommendation: `prove_support_reduction_operation_sublemma`.

Third recommendation: `higher_support_necessity_recheck`.

## scoring

| action | readiness | proof value | engineering cost | risk | dependency clarity | expected progress | order | final recommendation |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `family_chain_lift_phase3_if_needed` | 80 | 89 | 70 | 72 | 90 | 88 | 1 | `family_chain_lift_phase3_if_needed` |
| `prove_support_reduction_operation_sublemma` | 78 | 88 | 76 | 76 | 86 | 86 | 2 | `prove_support_reduction_operation_sublemma` |
| `higher_support_necessity_recheck` | 72 | 84 | 58 | 64 | 88 | 80 | 3 | `higher_support_necessity_recheck` |
| `limited_to_broader_generalization_plan` | 65 | 80 | 55 | 60 | 82 | 71 | 4 | `limited_to_broader_generalization_plan` |
| `support_bound_completion` | 60 | 78 | 62 | 68 | 78 | 69 | 5 | `support_bound_completion` |
| `boj_problem_bridge_formalization` | 42 | 60 | 50 | 50 | 58 | 52 | 6 | `boj_problem_bridge_formalization` |

## rationale

Phase2 made the payload refinement relation and source-target correspondence first-class and proved recognized correspondence totality. It did not prove layer projection payload preservation, canonical lift soundness, or counterexample-status preservation. The sharp next target is therefore `family_chain_lift_phase3_if_needed`.

This remains a bridge/planning state, not a full general theorem.
