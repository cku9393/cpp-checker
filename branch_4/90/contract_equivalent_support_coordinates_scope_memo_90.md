# Contract Equivalent Support Coordinates Scope Memo 90

## selected target

This round selects `contract_equivalent_support_coordinates`.

The operation is distinct from `delete_redundant_support_coordinate` and `project_to_active_support`: it does not delete an unused coordinate and does not restrict to active support. It identifies a nontrivial equivalence class of still-active support coordinates and replaces the class by one quotient coordinate.

## status

`contract_equivalent_support_coordinates` is a valid formal target under the current runtime inventory. The target is attempted only under an explicit coordinate-equivalence precondition. It is not promoted to a full support-reduction theorem, and it does not prove that every support `>8` witness has equivalent coordinates.

Runtime inventory: `branch_4/90/runtime/contract_equivalent_support_coordinates_scope_inventory_90.tsv`.
