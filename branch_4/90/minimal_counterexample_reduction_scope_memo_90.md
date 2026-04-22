# Minimal Counterexample Reduction Scope Memo 90

## purpose

This memo separates candidate statements for `minimal_counterexample_reduction`. It does not prove the full general theorem.

## selected statement

The selected statement for this round is `limited_support8_minimal_reduction`.

It says: inside the current support8 shell15/tail/family-chain finite proof package, any candidate counterexample can be put into a canonical finite witness form, and then it either lands in the verified finite package or exits through a named escape obligation.

## non-selected statements

- `bounded_shell_minimal_reduction`: useful after tail monotonicity and shell16 preflight clarify the next shell boundary.
- `full_general_minimal_reduction`: long-term target; it requires support-bound and shell/tail absorption lemmas.

## status

The limited statement is proof-ready as a skeleton, not completed as a proof. The missing blocker is tail monotonicity/absorption for witnesses that do not remain inside the current finite tail package.
