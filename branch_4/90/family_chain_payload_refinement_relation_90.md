# Family-Chain Payload Refinement Relation 90

## purpose

This document defines the payload relation used by phase2. It separates exact equality, canonical quotient equality, obstruction-preserving refinement, counterexample-preserving refinement, and counterexample-reducing refinement.

## selected relation

The selected working relation is `layerwise_payload_refinement`.

It is weaker than literal row equality and stronger than an unconstrained semantic similarity claim. It requires every recognized source payload component to map to a target layer payload component with the same canonical obstruction descriptor or a declared refinement that preserves obstruction meaning.

## status

The relation is well-defined under the current payload semantics. Proving that every recognized lifted payload satisfies the relation remains proof-sketch and depends on layer projection payload-preservation sublemmas.

Runtime table: `branch_4/90/runtime/family_chain_payload_refinement_relation_90.tsv`.
