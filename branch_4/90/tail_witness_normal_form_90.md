# Tail Witness Normal Form 90

## purpose

Tail monotonicity and absorption require a normal-form language for tail witnesses. This file separates the checked runtime form from unproved extension forms.

## current normal form

A checked tail witness is represented by:

- support label: support7 or support8 checked tail side.
- shell/tail bounds: `tail_start=9`, `tail_end=15`, `shell_count=7`.
- candidate fingerprint: the canonical outside-bounded frontier row.
- survivor status: local exact, plus-one, and theorem-preserving survivor counts.
- obstruction-chain status: captured by the support8 tail obstruction chain.

## extension normal form

An extension tail witness adds a distance-beyond-checked-bound coordinate. It is not current verified unless an absorption step decreases that coordinate or a shell16 preflight materializes the first extension boundary.

## caveat

Runtime row equality gives a stable checked normal form. A full mathematical equivalence proof for arbitrary tail extension remains a separate bridge obligation.
