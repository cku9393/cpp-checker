# Minimal Counterexample Reduction Limited Skeleton 90

## theorem name

`limited_support8_minimal_counterexample_reduction`

## exact statement

Assume a counterexample witness exists inside the current support8 shell15/tail/family-chain finite proof package. With the lexicographic measure defined in `minimal_counterexample_measure_90`, choose a minimal normal-form witness. Then either:

1. the witness is captured by the current finite package and contradicts the support8 closure certificate, or
2. it exits through a named escape class: checked tail is now handled by the tail bridge, while the first shell16 extension boundary has reviewed limited boundary facts, or it routes to `higher_support_escape`, `boj_constructivity_escape`, or `archive_independence_escape`.

## assumptions

- current support8 closure certificate is valid
- family-chain lower layers remain total `7`, fresh `7`, imported `0`
- lower-frontier inventory-only rows remain nonblocking
- witness belongs to the selected limited support8 finite package or has a named escape trigger

## verified inputs used

- `current_support8_closure_certificate_90`
- `general_gap_bridge_input_package_90`
- `family_chain_lower_layers_fingerprint_90`
- `lower_frontier_inventory_only_decision_90`
- `provenance_audit_fingerprint_90`

## measure used

`lexicographic_minimal_counterexample_tuple`.

## normal form used

The normal form in `minimal_counterexample_witness_normal_form_90`: support set, antecedent/consequent atoms, shell index, tail descriptor, frontier row link, family-chain component, survivor status, and canonical fingerprint.

## proof steps

1. Existence: the selected limited witness class is finite/current-admissible, so the lexicographic measure has a minimal element.
2. Normalize: convert the witness to canonical runtime normal form. This is proof-sketch-ready and depends on canonicalization soundness.
3. Classify scope: if the witness leaves support8/shell15/current-tail scope, route it to a named escape class.
4. Capture: if in scope, map it to frontier/family/tail package rows.
5. Contradict: if captured in the finite package, support8 completion lock and finite survivor counts contradict the witness.
6. Escape: if not captured, the escape interface records the exact missing obligation.

## current status

- final status: `limited_reduction_used_in_limited_bridge_theorem`
- completed sublemmas under current scope: existence, family-chain capture, finite-package contradiction, named/disjoint escape taxonomy
- proof-sketch-only sublemmas: normalization, support escape, shell escape, tail absorption/extension routing, frontier capture, obstruction-chain capture
- blocked sublemma: none inside the minimal-reduction skeleton after tail bridge reclassification; limited bridge theorem proof remains the next external obligation

## caveat

This is not a full proof of the general theorem. The skeleton has now been consumed by the selected limited theorem proof. The current shell16 result remains candidate universe `4`, raw/canonical/outside-bounded `8/4/4`, local exact/plus-one/theorem-preserving survivors `2/0/0`, fingerprint `981:4479772858934799504`; support-bound formalization, support-growth partition, and partial family-chain lift are installed, and the next broader blocker is `family_chain_lift_map_refinement`.
