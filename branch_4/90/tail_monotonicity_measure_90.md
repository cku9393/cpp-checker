# Tail Monotonicity Measure 90

## selected measure

`lexicographic_tail_extension_tuple`

The tuple is:

1. support overflow flag,
2. shell-boundary index,
3. distance beyond checked tail bound,
4. obstruction-chain depth,
5. survivor status rank,
6. canonical motif rank.

Inside the checked range, the measure is already at distance `0` beyond the checked bound and the survivor rank is zero. For extension witnesses, a true absorption step must decrease the distance or survivor rank. If no such step is available, the witness is routed to `shell16_escape`.

## status

The measure is proof-ready for classification and shell16 escape routing. It is not yet a proof of full monotone absorption.
