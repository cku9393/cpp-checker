# Shell15 Frontier Feasibility 90

## semantics

- `raw 4` counts the uncollapsed shell15 candidate hits per target frontier tag
- `canonical 2` counts the canonicalized frontier rows after current normalization
- `outside-bounded 2` counts the canonical rows that remain outside the bounded shell theorem scope
- `local exact survivors 0`, `plus-one survivors 0`, `theorem-preserving survivors 0` mean the current caches close without any surviving counterexample row

## equality requirements

fresh shell15 frontier promotion was treated as successful only when all of the following matched the legacy `84` output.

1. frontier theorem counts match
2. canonical candidate row-set matches
3. local exact cache contents match
4. plus-one cache contents match
5. consumer-visible theorem stats remain unchanged

## feasibility result

- current runtime fresh derivation is feasible
- pass1 can rebuild the frontier pair by direct shell15 scan
- pass2/pass3 can reuse the authoritative runtime cache path
- imported `84` data is still reachable as fallback but is not used on the successful current run

## truthful boundary

- `antecedent plus twelve frontier`: fresh current-runtime generated
- `support8 antecedent15 frontier`: fresh current-runtime generated
- `support8 antecedent15 shell theorem`: fresh current-runtime generated over the fresh shell15 frontier pair and the fresh direct lower-frontier dependency subset
- `support8 outside-bounded tail pattern theorem`: promoted to fresh because its direct shell15 frontier dependency is now fresh
