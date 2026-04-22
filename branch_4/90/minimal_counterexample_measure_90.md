# Minimal Counterexample Measure 90

## selected measure

The selected measure is `lexicographic_minimal_counterexample_tuple`.

It orders candidate witnesses by:

1. support size
2. antecedent count
3. shell index
4. tail position
5. frontier rank
6. family-chain depth
7. obstruction/witness size
8. canonical motif rank

## well-foundedness

For the selected limited support8 statement, every coordinate ranges over a finite or natural-number ordered set, and lexicographic order over a finite product of well-founded orders is well-founded.

For full general use, the measure still needs a proof that the candidate witness class is admissible and that each reduction step decreases the tuple or exits through a named escape obligation.

## caveat

The measure is proof-ready for the limited skeleton, not a completed full-general reduction proof.
