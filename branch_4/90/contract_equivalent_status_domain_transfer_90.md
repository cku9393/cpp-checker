# Contract Equivalent Status Domain Transfer 90

## status

`contract_equivalent_status_domain_transfer_proof_ready_quotient_domain_open`

Status-domain transfer asks whether the source counterexample-status predicate
and the quotient witness status predicate are defined over compatible domains.
For coordinate contraction this is not automatic from payload congruence. The
quotient may identify active coordinates, so the status predicate domain can
change even if payload roles are mergeable.

The transfer is proof-ready but open on quotient status-domain invariance,
normal-form eligibility, and valid reduced-status fallback if the quotient
changes the domain.

Runtime table:
`branch_4/90/runtime/contract_equivalent_status_domain_transfer_90.tsv`.
