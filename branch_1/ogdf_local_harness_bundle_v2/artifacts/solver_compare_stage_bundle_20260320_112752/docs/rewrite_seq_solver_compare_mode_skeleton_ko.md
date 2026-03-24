# Rewrite Seq Solver Compare Mode Skeleton

```cpp
struct SolverCompareResult {
    bool ok = true;
    std::string why;
    ExplicitBlockGraph explicitBefore;
    ExplicitBlockGraph explicitAfterLegacy;
    ExplicitBlockGraph explicitAfterSeq;
};

SolverCompareResult runSolverCompareMode(const ExplicitBlockGraph &input) {
    SolverCompareResult result;

    ReducedSPQRCore legacyCore = buildSolverCore(input);
    ReducedSPQRCore seqCore = legacyCore;

    std::string why;
    RewriteSeqStats stats;
    if (!runRewriteSequenceToFixpoint(seqCore, stats, why)) {
        result.ok = false;
        result.why = "runRewriteSequenceToFixpoint failed: " + why;
        return result;
    }

    result.explicitBefore = materializeWholeCoreExplicit(legacyCore);
    result.explicitAfterLegacy = materializeWholeCoreExplicit(runLegacySolverPath(legacyCore));
    result.explicitAfterSeq = materializeWholeCoreExplicit(seqCore);

    if (!checkEquivalentExplicitGraphs(result.explicitAfterSeq,
                                       result.explicitAfterLegacy,
                                       why)) {
        result.ok = false;
        result.why = "legacy/seq explicit mismatch: " + why;
    }
    return result;
}
```

초기 compare mode 원칙:

- 기본 경로는 legacy 유지
- `USE_REWRITE_SEQ_ENGINE`가 켜져도 먼저 compare-only로 운영
- mismatch bundle과 explicit diff를 바로 dump
- compare mode가 충분히 green일 때만 default-on 검토

권장 compare mode shape:

- mode name:
  - `solver-compare-rewrite-seq`
- required outputs:
  - legacy explicit
  - rewrite-seq explicit
  - `RewriteSeqStats`
  - mismatch reason
  - dump path
