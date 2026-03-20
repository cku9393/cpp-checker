#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "state_dump.hpp"
#include "test_harness.hpp"

struct ExhaustiveScenario;

enum class CompareEligibilityReason : u8 {
    ELIGIBLE = 0,
    NO_SPLIT_READY = 1,
    SINGLE_ADMISSIBLE_PAIR = 2,
    TIE_BUT_NO_BOUNDARY_ARTIFACT = 3,
    TIE_BUT_NO_KEEPOCC_SIBLING = 4,
    PLANNER_MIXED_FOLLOWUP_ABSENT = 5,
};

enum class SplitChoiceGrade : u8 {
    FULLY_EQUIVALENT = 0,
    SAME_STOP_DIFFERENT_FINAL_STATE = 1,
    SEMANTIC_MISMATCH = 2,
};

enum class SplitChoiceRepresentativeShiftKind : u8 {
    NONE = 0,
    HARMLESS = 1,
    TRACE_ONLY = 2,
    SEMANTIC = 3,
};

struct SplitChoiceCandidateOutcome {
    std::pair<Vertex, Vertex> pair{NIL_U32, NIL_U32};
    bool selectedByPlanner = false;
    bool selectedByFast = false;
    bool selectedByPolicy = false;
    bool selectedByExactShadow = false;
    bool selectedByExactFull = false;
    SplitChoiceGrade grade = SplitChoiceGrade::FULLY_EQUIVALENT;
    bool stopConditionSatisfied = false;
    std::string plannerFastFinalKey;
    std::string plannerFastTargetKey;
    std::string exactSplitChoiceKey;
    std::string finalStateHash;
    std::string finalStateDescription;
    std::string targetPrepareHash;
    std::string targetPrepareDescription;
    std::string traceHash;
    std::vector<PlannerTraceEntry> trace;
    std::string detail;
};

struct SplitChoiceOracleRunResult {
    std::pair<Vertex, Vertex> selectedPair{NIL_U32, NIL_U32};
    std::pair<Vertex, Vertex> fastSelectedPair{NIL_U32, NIL_U32};
    std::pair<Vertex, Vertex> policySelectedPair{NIL_U32, NIL_U32};
    std::pair<Vertex, Vertex> exactShadowSelectedPair{NIL_U32, NIL_U32};
    std::pair<Vertex, Vertex> exactFullSelectedPair{NIL_U32, NIL_U32};
    std::pair<Vertex, Vertex> exactRepresentativePair{NIL_U32, NIL_U32};
    std::size_t admissiblePairCount = 0;
    std::size_t comparedPairCount = 0;
    std::size_t fullyEquivalentCount = 0;
    std::size_t sameStopDifferentFinalStateCount = 0;
    std::size_t semanticMismatchCount = 0;
    std::size_t selectedEquivalenceClassSize = 0;
    std::size_t maxEquivalenceClassSize = 0;
    bool selectedPairRepresentative = false;
    bool exactAuditAvailable = false;
    std::size_t fastClassCount = 0;
    std::size_t exactClassCount = 0;
    std::size_t exactVsFastClassDisagreementCount = 0;
    std::size_t representativeShiftCount = 0;
    std::size_t representativeShiftSameClassCount = 0;
    std::size_t representativeShiftSemanticDivergenceCount = 0;
    std::size_t representativeShiftFollowupDivergenceCount = 0;
    std::size_t representativeShiftTraceDivergenceCount = 0;
    std::size_t harmlessShiftCount = 0;
    std::size_t traceOnlyShiftCount = 0;
    std::size_t semanticShiftCount = 0;
    bool representativeShifted = false;
    bool exactRepresentativeSelected = false;
    SplitChoiceRepresentativeShiftKind representativeShiftKind = SplitChoiceRepresentativeShiftKind::NONE;
    std::size_t compareStateCount = 0;
    std::size_t exactFullEvalCount = 0;
    std::size_t exactShadowEvalCount = 0;
    std::size_t fallbackCount = 0;
    std::size_t sameRepresentativeCount = 0;
    std::size_t sameSemanticClassCount = 0;
    std::size_t sameFinalStateCount = 0;
    std::size_t semanticDisagreementCount = 0;
    std::size_t capHitCount = 0;
    std::size_t harmlessCompareCount = 0;
    std::size_t traceOnlyCompareCount = 0;
    std::size_t candidateEnumerationNanos = 0;
    std::size_t exactShadowEvaluationNanos = 0;
    std::size_t exactFullEvaluationNanos = 0;
    std::size_t canonicalizationNanos = 0;
    std::size_t multiclassCatalogNanos = 0;
    std::size_t stateHashCacheHitCount = 0;
    std::size_t stateHashCacheMissCount = 0;
    std::size_t candidateEvaluationCacheHitCount = 0;
    std::size_t candidateEvaluationCacheMissCount = 0;
    std::size_t exactCanonicalCacheHitCount = 0;
    std::size_t exactCanonicalCacheMissCount = 0;
    bool exactShadowVsExactFullShifted = false;
    SplitChoiceRepresentativeShiftKind exactShadowVsExactFullShiftKind = SplitChoiceRepresentativeShiftKind::NONE;
    std::string multiclassCatalogKey;
    std::vector<SplitChoiceCandidateOutcome> outcomes;
};

struct CompareEligibilityInfo {
    bool hasSplitReady = false;
    std::size_t admissibleSplitPairCount = 0;
    bool hasTie = false;
    bool hasBoundaryOnlyChildAfterSplit = false;
    bool hasKeepOccSiblingAfterSplit = false;
    bool hasPlannerMixedFollowup = false;
    bool compareEligible = false;
    bool mixedCompareEligible = false;
    CompareEligibilityReason reason = CompareEligibilityReason::NO_SPLIT_READY;
    std::size_t evaluatedPairCount = 0;
};

const char* compare_eligibility_reason_name(CompareEligibilityReason reason);
const char* split_choice_grade_name(SplitChoiceGrade grade);
const char* split_choice_representative_shift_name(SplitChoiceRepresentativeShiftKind kind);
bool split_choice_has_mismatch(const SplitChoiceOracleRunResult& result);
bool split_choice_has_exact_audit_disagreement(const SplitChoiceOracleRunResult& result);
bool split_choice_same_exact_projection(
    const SplitChoiceCandidateOutcome& lhs,
    const SplitChoiceCandidateOutcome& rhs
);

std::vector<std::pair<Vertex, Vertex>> enumerate_valid_split_pairs(const RawEngine& RE, OccID occ);
CompareEligibilityInfo analyze_compare_eligibility(const ExhaustiveScenario& scenario, std::size_t maxSplitPairCandidates);

SplitChoiceOracleRunResult run_split_choice_oracle(
    const TestOptions& options,
    const ExhaustiveScenario& scenario,
    const std::unordered_map<Vertex, Vertex>* inverseRelabel = nullptr
);

bool replay_split_choice_oracle_dump(
    const TestOptions& options,
    const PlannerStateDump& dump,
    FailureSignature* failure
);

std::string describe_split_choice_oracle_result(const SplitChoiceOracleRunResult& result);
