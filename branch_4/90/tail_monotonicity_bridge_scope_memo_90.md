# Tail Monotonicity Bridge Scope Memo 90

## purpose

This memo separates candidate statements for `tail_monotonicity_bridge`. It does not prove the full general theorem and does not run shell16.

## selected statement

The selected statement for this round is `tail_monotonicity_to_shell16_escape`.

It says: a tail witness inside the current checked support8 tail range is captured by the current outside-bounded tail pattern theorem and tail obstruction chain. A tail witness that requires extending beyond the checked range is not silently absorbed; it is routed to the named `shell16_escape` unless a separate absorption step is later proved.

## candidate split

- `checked_tail_absorption_within_support8`: current verified checked-tail statement; proved under current scope.
- `bounded_tail_extension_absorption`: useful but still needs an absorption step.
- `tail_monotonicity_to_shell16_escape`: selected bridge statement; proof-ready with checked capture proved and extension routed to shell16 escape.
- `full_tail_monotonicity`: long-term statement only; not current verified.

## current decision

The tail bridge partially closes the previous tail escape. Checked tail absorption is current-scope proved. Bounded extension absorption is not proved. Shell16 preflight is contract-ready with no scan, so the next blocker is `shell16_attempt`, with `prove_or_refine_tail_absorption_step` as the proof-side alternative.
