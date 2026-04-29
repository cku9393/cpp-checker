# Inactive Support Locality Semantics 90

## status

`inactive_support_locality_semantics_contract_ready`

## semantics

Inactive-support locality is a two-layer condition.

Payload locality says that deleting `S \ active_support(W)` does not delete
payload carriers declared active by the notation contract. This side is
available under the selected contract.

Counterexample-status locality is stronger. It requires not only payload
locality, but also complete status/certificate dependency extraction,
normal-form preservation under `normalize(restrict(W,A))`, and invariance of the
status predicate domain after projection. If the projected object exits the
source status domain but remains a valid counterexample or reduced obstruction,
the branch is a reduction, not preservation. If neither preservation nor valid
reduction is established, the branch is a named project-to-active blocker or a
deferred higher-support escape after operation proofs close.

Runtime table:
`branch_4/90/runtime/inactive_support_locality_semantics_90.tsv`.
