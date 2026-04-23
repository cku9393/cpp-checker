# Support Reduction Selected Operation Smaller Witness 90

## construction

For `delete_redundant_support_coordinate`, the smaller witness is obtained by deleting one redundant coordinate from the support set and restricting every payload/certificate component to the remaining coordinates.

The strict decrease is in the first coordinate of the selected support measure:

`support_size(W') = support_size(W) - 1`.

This is a conditional operation proof. It does not prove that a redundant coordinate always exists.

Runtime table: `branch_4/90/runtime/support_reduction_selected_operation_smaller_witness_90.tsv`.
