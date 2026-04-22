# Support Bound Lemma Scope Memo 90

## selected statement

Selected for this round: `support_minimal_counterexample_reduces_to_support8_or_escape`.

Formal meaning: if a broader counterexample exists, choose a support-minimal normal witness. If its support is `<=8`, the already proved `limited_support8_shell16_boundary_bridge` applies. If its support is `>8` and no support-reduction step is available, the witness is not hidden; it is routed to `higher_support_escape`.

## non-claims

- This does not prove the full general theorem.
- This does not prove unrestricted support8 sufficiency.
- This does not run support9+ or higher-support scans.
- This does not prove that family-chain closure absorbs all support growth.

Runtime inventory: `branch_4/90/runtime/support_bound_lemma_scope_inventory_90.tsv`.
