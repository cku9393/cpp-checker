# Canonical Compression Status Domain Transfer 90

## status

`canonical_compression_status_domain_transfer_proof_ready_motif_domain_open`

Status-domain transfer asks whether the source counterexample-status predicate
and the compressed witness status predicate are defined over compatible domains.
For canonical motif compression this is not automatic from payload congruence.
The motif rewrite can refine, remove, or reinterpret status/certificate roles
even when payload roles are compatible.

The transfer is proof-ready but open on motif status-domain invariance or
refinement, compressed normal-form eligibility, and valid reduced-status
fallback if the compressed domain changes.

Runtime table:
`branch_4/90/runtime/canonical_compression_status_domain_transfer_90.tsv`.
