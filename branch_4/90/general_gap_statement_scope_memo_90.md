# General Gap Statement Scope Memo 90

## purpose

The phrase "general gap theorem" is split into three candidate statements. None of these is marked proved in this round.

## candidate statements

### `limited_support8_gap_statement`

This is the closest statement to the current verified package. It is limited to the current support8, shell15/tail, bounded family-chain, and runtime-audited frontier scope. The current package now has a minimal-counterexample proof-ready skeleton, a checked-tail bridge, and a non-promoting shell16 first-boundary attempt. Shell16 result promotion review and the finite-exhaustion-to-structural bridge still need to be written.

### `bounded_shell_gap_statement`

This statement keeps an explicit finite shell/tail bound. It asks whether shell15 plus the current tail pattern and the shell16 no-promotion attempt result are sufficient for a bounded shell gap statement.

### `full_general_gap_statement`

This statement removes the support/shell bounds or quantifies over them generally. It requires a support-bound theorem and a shell/tail absorption theorem. It is not ready for a proof attempt.

## conclusion

The selected `limited_bridge_theorem_proof_attempt` is complete under current scope, and support-bound formalization plus support-growth partition plus partial family-chain lift are now installed as proof-ready skeletons. The next target should be `family_chain_lift_map_refinement`, not the full general theorem. The shell16 first-boundary result is reviewed but not promoted as a full theorem.
