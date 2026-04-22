# Shell16 Result Semantics Review 90

## purpose

This review separates the three survivor classes in the shell16 attempt. The prior active label was ambiguous because it said zero survivors while the local-exact survivor count is `2`.

## corrected final label

`shell16_probe_completed_local_exact_survivors_present_no_theorem_preserving_survivors`

## survivor class split

- local exact survivors: `2`
- plus-one survivors: `0`
- theorem-preserving survivors: `0`

The local exact survivors are real shell16 witness rows and must stay visible. They do not by themselves block the current tail/minimal bridge, because the bridge escape needs plus-one or theorem-preserving survival under the current proof package. The two local exact rows fail the plus-one survivor probe and therefore cannot reach theorem-preserving status in this attempt.

## promotion decision

The shell16 result is promotable as current verified runtime facts and as a limited bridge lemma saying the first shell16 boundary has no theorem-preserving escape under the current shell16 probe.

It is not promoted as a full shell16 theorem, full tail monotonicity theorem, full support bound, or full general theorem.

