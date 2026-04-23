# Contract Equivalent Support Coordinates Operation Semantics 90

## operation

`contract_equivalent_support_coordinates` forms a quotient witness by identifying a nontrivial equivalence class of support coordinates.

The operation applies only when the accepted equivalence relation is defined, at least one class has size greater than one, and payload/canonical/status fields are congruent enough to build the quotient object. If these preconditions fail, contraction is not performed and the case routes to the next support-reduction operation or named escape.

## final status

`coordinate_contraction_semantics_contract_ready`.

Runtime table: `branch_4/90/runtime/contract_equivalent_support_coordinates_operation_semantics_90.tsv`.
