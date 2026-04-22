# Minimal Counterexample Witness Normal Form 90

## purpose

The normal form defines how a candidate counterexample can be compared to the current finite runtime package.

## normal-form language

A witness is represented as a canonical support/schema/frontier/family-chain object with:

- support set
- antecedent and consequent atoms
- shell index
- tail descriptor
- frontier/candidate row link
- family-chain component, if relevant
- survivor/unsupported/obstruction-chain status
- canonical fingerprint/rank

## current status

The runtime has canonicalization and rowset fingerprint machinery. For the limited statement, this is enough to define a proof-ready normal-form contract. The mathematical proof that every admissible general witness preserves counterexample status under normalization remains proof-sketch-only.
