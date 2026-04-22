# Family Chain Constructor Scope Memo 90

## target

- target subset: `family_chain_output_57`
- target items:
  - `bounded family-chain theorem`
  - `family-chain self verification`
- in-scope: authoritative theorem object layer only
- out of scope: shell15 frontier fresh scan, shell16, higher-support expansion, BOJ solver

## A. imported theorem object constructor

- `support_bounded_schema_universe_obstruction_theorem_()`
  - fixed theorem object constructor
  - source claim: imported `52` output baseline
  - fields were previously hardcoded as `single=7`, `pair=21`, survivor totals `0`
- `unified_bounded_schema_universe_obstruction_theorem_()`
  - fixed theorem object constructor
  - source claim: imported `57` output baseline
  - fields were previously hardcoded as `7/21/35/35/21/7/1`, survivor totals `0`
- before this round, `family_chain_output_57` authoritative path stopped here
  - theorem object identity itself was imported
  - downstream validation only checked consistency

## B. current runtime rebuildable downstream expansion

- `ensure_triple_family_expansion_ready_fast_()`
  - loads validated triple-family theorem data and region summaries into current globals
- `ensure_quadruple_family_expansion_ready_fast_()`
  - builds or loads current-runtime quadruple theorem data 55 into current globals
- `ensure_quintuple_family_expansion_ready_fast_()`
  - current path now builds or loads current-runtime quintuple theorem data 57
- `ensure_sextuple_family_expansion_ready_fast_()`
  - current path now builds or loads current-runtime sextuple theorem data 57
- `ensure_septuple_family_expansion_ready_fast_()`
  - current path now builds or loads current-runtime septuple theorem data 57
- `ensure_high_family_expansion_ready_fast_()`
  - fast path still installs the imported high-family aggregate theorem data
  - next target is to rebuild the high-family aggregate from current sextuple/septuple payloads

relation to `family_chain_output_57`

- `family_chain_output_57` historically bundled:
  - support-bounded obstruction theorem object
  - unified bounded schema-universe obstruction theorem object
  - quintuple / sextuple / septuple / high-family aggregate counts
- the current code already had enough lower-layer runtime state to validate these counts
- what was missing was the final current constructor that reassembled the authoritative theorem objects from that state

## C. validation-only wrapper

- `ensure_unified_bounded_schema_universe_obstruction_ready_()`
  - validates imported theorem-data chain
  - installs `g_last_unified_bounded_schema_universe_obstruction_theorem` from imported constructor
- `ensure_family_chain_theorem_ready_fast_()`
  - previously ensured support/triple/quadruple/quintuple/high/unified readiness only
  - it did not replace theorem object identity with a current constructor

## D. provenance / audit emitter

- `build_family_chain_theorem_audit_stats_()`
  - computes family-chain theorem audit summary and provenance label
- `build_theorem_data_provenance_inventory_()`
  - maps `bounded family-chain theorem` / `family-chain self verification` to machine-readable provenance
- `write_basis_family_generation_audit_files_()`
  - emits `runtime/family_chain_generation_audit_90.tsv`
  - now also emits constructor-specific TSVs

## current round constructor decision

- new authoritative current constructor path:
  - `build_current_family_chain_output_57_theorem_objects_()`
- internal split:
  - `build_current_support_bounded_schema_universe_obstruction_theorem_()`
  - `build_current_unified_bounded_schema_universe_obstruction_theorem_()`

what these constructors do

- they do not claim a fresh frontier scan
- they do rebuild the top theorem objects in the current runtime from:
  - current pool fingerprint / pool size
  - current family summary set
  - current bounded region spec counts
  - current pair52 and triple53 constructor/cache outputs
  - current quad55 constructor/cache output
  - current quintuple57 constructor/cache output
  - currently loaded high-family theorem data
- therefore the theorem object layer is now current-constructed
- remaining 57 sublayers remain validated imported inputs unless separately rederived

## truthful provenance reading

- `bounded family-chain theorem`
  - current constructor path: yes
  - lower-layer inputs fully fresh: partially; pair52, triple53, quad55, and quintuple57 are fresh, remaining 57 sublayers remain imported lower layers
- `family-chain self verification`
  - current constructor path: yes
  - lower-layer inputs fully fresh: partially; pair52, triple53, quad55, and quintuple57 are fresh, remaining 57 sublayers remain imported lower layers
- `exact minimal basis size = 96`
  - current-runtime exact basis payload constructor/cache path is fresh current-runtime generated
- `antecedent plus twelve frontier` / `support8 antecedent15 frontier`
  - current-runtime shell15 frontier constructor/cache path is fresh current-runtime generated

## outcome criterion for this round

- success means:
  - family-chain authoritative theorem objects are no longer imported constructors
  - pass2/pass3 still reach `support8_authoritative_completion_locked`
  - provenance inventory shows family-chain items as fresh current-runtime generated with an explicit lower-layer caveat
