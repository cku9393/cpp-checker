# Lower Frontier Ladder Dependency Graph 90

## support8 antecedent15 shell theorem

authoritative theorem object path:

- `support8_antecedent15_shell_theorem_data_()`
- `support8_antecedent14_shell_theorem_data_()`
- `support8_antecedent13_shell_theorem_data_()`

dependency classification:

- theorem-object dependency:
  - `support8_antecedent14_shell_theorem_data_()`
  - shell15 frontier pair
- theorem-object dependency inside shell14:
  - `support8_antecedent13_shell_theorem_data_()`
  - shell14 pair
- theorem-object dependency inside shell13:
  - `support_plus_one_frontier_theorem_data_()`
  - `antecedent_plus_one_frontier_theorem_data_()`
  - `mixed_outside_bounded_frontier_theorem_data_()`
  - shell5 pair
  - shell6 pair
  - shell7 pair
  - shell8 pair
  - shell9 pair
  - shell10 pair
  - shell13 pair

not a direct theorem-object dependency for shell15 theorem:

- shell11 pair
- shell12 pair

## support8 tail obstruction chain theorem

authoritative path:

- `validate_support8_tail_obstruction_chain_theorem_data_()`

dependency classification:

- theorem-object dependency:
  - `validate_support8_antecedent15_shell_theorem_data_()`
  - `validate_support8_outside_bounded_tail_pattern_theorem_data_()`
- audit dependency:
  - artifact completion audit
  - document completion audit
  - rerun completion audit
  - audit freshness
- note-only / reporting-only dependency:
  - none required for validation

즉 tail chain은 lower ladder를 직접 읽지 않고 shell theorem을 통해 간접 소비한다.

## support8 authoritative completion lock

authoritative path:

- `validate_support8_authoritative_completion_lock_data_()`

dependency classification:

- theorem-object dependency:
  - `validate_antecedent_shell15_frontier_theorem_data_("antecedent_plus_twelve_frontier", ...)`
  - `validate_antecedent_shell15_frontier_theorem_data_("support8_antecedent15_frontier", ...)`
  - `validate_support8_antecedent15_shell_theorem_data_()`
  - `validate_support8_outside_bounded_tail_pattern_theorem_data_()`
  - `validate_support8_tail_obstruction_chain_theorem_data_()`
- audit dependency:
  - artifact completion audit
  - document completion audit
  - rerun completion audit
  - audit freshness
  - stale audit eliminated
  - local test verified
  - release compile verified
- stats-only / reporting-only dependency:
  - current classification summaries

즉 completion lock도 lower ladder를 직접 읽지 않고 shell theorem 및 tail chain을 통해 간접 소비한다.

## minimal dependency conclusion

- `support8 antecedent15 shell theorem` direct blocker는 shell11/shell12가 아니라 shell13 base subset + shell14 pair다.
- `support8 tail obstruction chain theorem` direct blocker는 shell theorem freshization 여부다.
- `support8 authoritative completion lock` direct blocker는 shell theorem freshization과 tail chain reevaluation이다.
