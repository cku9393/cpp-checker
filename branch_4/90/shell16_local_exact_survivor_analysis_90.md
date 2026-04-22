# Shell16 Local Exact Survivor Analysis 90

## result

The shell16 attempt found two local-exact survivors. Both are on `support8_antecedent16_frontier`. Both fail the plus-one survivor probe, and therefore neither is theorem-preserving under the current shell16 attempt.

## survivor rows

1. `support8_antecedent16_frontier`, conclusion `(2,3->3)`
2. `support8_antecedent16_frontier`, conclusion `(2,4->1)`

Both share the same antecedent pattern:

`(1,2->1) (2,3->1) (2,3->2) (2,3->4) (2,4->2) (2,4->4) (3,4->1) (3,4->5)`

## bridge interpretation

The pair is not hidden. It is promoted as a runtime fact that local-exact survivors exist at shell16. The pair is nonblocking for the current tail/minimal bridge because the current escape condition is theorem-preserving survival; plus-one survivors and theorem-preserving survivors are both zero.

This does not prove arbitrary extension-tail absorption and does not promote a full shell16 theorem.

