# Shell16 Attempt Protocol 90

## preconditions

- release compile and LOCAL_TEST compile remain verified.
- pass1/pass2/pass3 remain `support8_authoritative_completion_locked`.
- required docs remain `39/39` and required artifacts remain `8/8`.
- top-level provenance remains fresh `16`, imported `0`, mixed `0`, archival `3`.
- family-chain lower layers remain total `7`, fresh `7`, imported `0`.
- shell16 artifact, document, audit, feasibility, and guardrail contracts are present.

## required code paths used by the attempt

- pass1 builder: `scan_shell16_frontier_non_promoting_attempt_`.
- candidate enumerator: shell16-specific enumerator, not a shell15 artifact copy.
- pass2/pass3 cache loader: `load_current_shell16_runtime_artifact_`.
- audit writer: shell16-specific writer that records constructor, cache path, fallback visibility, fingerprints, and survivor audits.

## allowed outcomes

| outcome | classification |
| --- | --- |
| shell16 theorem-preserving survivors zero after validated shell16 scan | may close the theorem-preserving `shell16_escape` component only after rowset, fingerprint, fallback, stale-artifact, local-exact visibility, and pass rerun gates |
| shell16 theorem-preserving survivors nonzero | opens named shell16 survivor blocker; no theorem promotion |
| scan timeout or too expensive | opens cost/pruning blocker; no theorem promotion |
| canonicalization mismatch | opens canonicalization blocker; no theorem promotion |
| fallback hit | opens provenance blocker; no theorem promotion |
| stale artifact | opens stale-artifact blocker; no theorem promotion |

## pass behavior

Pass1 must run the shell16 builder if a future attempt is selected. Pass2/pass3 must use a stable validated shell16 cache-load path. If either path falls back to imported or shell15 data, shell16 remains unproved.

## current attempt result

- classification: `shell16_probe_completed_local_exact_survivors_present_no_theorem_preserving_survivors`
- candidate universe: `4`
- raw/canonical/outside-bounded: `8/4/4`
- local exact/plus-one/theorem-preserving survivors: `2/0/0`
- fingerprint: `981:4479772858934799504`
- fallback reachable/hit: `0/0`
- stale artifact status: `fresh`

## tail bridge update rule

This attempt and promotion review supplied reviewed shell16 boundary facts. The subsequent limited bridge theorem proof updates the tail bridge shell16 escape to `tail_escape_closed_for_limited_bridge_theorem`. It does not close full tail monotonicity; arbitrary extension absorption still requires a separate proof if needed.

## rollback and non-regression

Any shell16 attempt failure must leave support8 lock, top-level provenance, family-chain closure, and lower-frontier inventory-only decisions unchanged. The failing gate must be recorded before any theorem promotion is considered.
