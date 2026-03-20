#include "split_choice_oracle.hpp"

#include <algorithm>
#include <chrono>
#include <deque>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "exact_canonicalizer.hpp"
#include "exhaustive_generator.hpp"
#include "reference_model.hpp"
#include "stabilization_support.hpp"

using namespace std;

namespace {

struct EvaluatedSplitChoiceOutcome {
    SplitChoiceCandidateOutcome publicOutcome;
    string equivalenceKey;
    string fastClassKey;
    string exactOrderKey;
};

struct SplitChoiceCacheTelemetry {
    size_t stateHashHitCount = 0U;
    size_t stateHashMissCount = 0U;
    size_t candidateEvaluationCacheHitCount = 0U;
    size_t candidateEvaluationCacheMissCount = 0U;
    size_t exactCanonicalCacheHitCount = 0U;
    size_t exactCanonicalCacheMissCount = 0U;
};

unordered_map<string, string> g_splitChoiceStateHashCache;
unordered_map<string, string> g_splitChoiceExactCanonicalKeyCache;

uint64_t elapsed_nanos(const chrono::steady_clock::time_point begin) {
    return static_cast<uint64_t>(chrono::duration_cast<chrono::nanoseconds>(chrono::steady_clock::now() - begin).count());
}

string fast_split_choice_key(const SplitChoiceCandidateOutcome& outcome) {
    ostringstream oss;
    oss << "failed=" << (outcome.detail.empty() ? 0 : 1)
        << ";stop=" << (outcome.stopConditionSatisfied ? 1 : 0)
        << ";target=" << outcome.plannerFastTargetKey
        << ";final=" << outcome.plannerFastFinalKey
        << ";detail=" << outcome.detail;
    return oss.str();
}

string exact_full_order_key(const SplitChoiceCandidateOutcome& outcome) {
    ostringstream oss;
    oss << outcome.exactSplitChoiceKey
        << ";final=" << outcome.finalStateHash
        << ";target=" << outcome.targetPrepareHash
        << ";stop=" << (outcome.stopConditionSatisfied ? 1 : 0)
        << ";detail=" << stable_hash_text(outcome.detail)
        << ";pair=" << outcome.pair.first << ',' << outcome.pair.second;
    return oss.str();
}

size_t effective_split_choice_exact_cap(const TestOptions& options) {
    if (options.exactCanonicalCap != 0U) {
        return options.exactCanonicalCap;
    }
    if (options.splitChoicePolicyMode == SplitChoicePolicyMode::EXACT_FULL ||
        options.splitChoiceCompareMode == SplitChoiceCompareMode::EXACT_FULL) {
        return max<size_t>(8U, options.maxReal);
    }
    return 0U;
}

const SplitChoiceCandidateOutcome* find_outcome_by_pair(
    const vector<SplitChoiceCandidateOutcome>& outcomes,
    pair<Vertex, Vertex> pairValue
) {
    const auto it = find_if(
        outcomes.begin(),
        outcomes.end(),
        [&](const SplitChoiceCandidateOutcome& outcome) { return outcome.pair == pairValue; }
    );
    return it == outcomes.end() ? nullptr : &*it;
}

string build_multiclass_catalog_key(const SplitChoiceOracleRunResult& result) {
    map<string, size_t> classCounts;
    for (const SplitChoiceCandidateOutcome& outcome : result.outcomes) {
        ++classCounts[stable_hash_text(outcome.exactSplitChoiceKey)];
    }

    const SplitChoiceCandidateOutcome* exactShadow = find_outcome_by_pair(result.outcomes, result.exactShadowSelectedPair);
    const SplitChoiceCandidateOutcome* exactFull = find_outcome_by_pair(result.outcomes, result.exactFullSelectedPair);

    ostringstream oss;
    bool first = true;
    for (const auto& [classHash, count] : classCounts) {
        if (!first) {
            oss << '|';
        }
        first = false;
        oss << classHash << ':' << count;
    }
    oss << ";shadow=";
    if (exactShadow != nullptr) {
        oss << stable_hash_text(exactShadow->exactSplitChoiceKey);
    } else {
        oss << "none";
    }
    oss << ";full=";
    if (exactFull != nullptr) {
        oss << stable_hash_text(exactFull->exactSplitChoiceKey);
    } else {
        oss << "none";
    }
    return oss.str();
}

pair<Vertex, Vertex> selected_pair_for_mode(
    SplitChoicePolicyMode mode,
    pair<Vertex, Vertex> fastSelectedPair,
    pair<Vertex, Vertex> exactShadowSelectedPair,
    pair<Vertex, Vertex> exactFullSelectedPair
) {
    switch (mode) {
        case SplitChoicePolicyMode::FAST:
            return fastSelectedPair;
        case SplitChoicePolicyMode::EXACT_SHADOW:
            return exactShadowSelectedPair;
        case SplitChoicePolicyMode::EXACT_FULL:
            return exactFullSelectedPair;
    }
    return exactShadowSelectedPair;
}

bool live_occurrence_exists(const RawEngine& RE, OccID occ) {
    return occ < RE.occ.a.size() && RE.occ.a[occ].alive;
}

pair<Vertex, Vertex> ordered_pair(Vertex aOrig, Vertex bOrig) {
    if (aOrig > bOrig) {
        swap(aOrig, bOrig);
    }
    return {aOrig, bOrig};
}

pair<Vertex, Vertex> normalize_pair(
    pair<Vertex, Vertex> pairValue,
    const unordered_map<Vertex, Vertex>* inverseRelabel
) {
    if (inverseRelabel == nullptr) {
        return ordered_pair(pairValue.first, pairValue.second);
    }
    const auto map_vertex = [&](Vertex orig) {
        const auto it = inverseRelabel->find(orig);
        return it == inverseRelabel->end() ? orig : it->second;
    };
    return ordered_pair(map_vertex(pairValue.first), map_vertex(pairValue.second));
}

void normalize_engine_result(
    const RawEngine& RE,
    OccID targetOcc,
    ExhaustiveFamily family,
    const unordered_map<Vertex, Vertex>* inverseRelabel,
    RawEngine& normalizedEngine,
    OccID& normalizedTargetOcc
) {
    ExhaustiveScenario normalizedScenario;
    normalizedScenario.RE = RE;
    normalizedScenario.ctx.targetOcc = targetOcc;
    normalizedScenario.family = family;

    if (inverseRelabel != nullptr && !inverseRelabel->empty()) {
        RebuildTransform transform;
        transform.relabelOrig = *inverseRelabel;
        normalizedScenario = rebuild_exhaustive_scenario(normalizedScenario, transform);
    }

    normalizedEngine = normalizedScenario.RE;
    normalizedTargetOcc = normalizedScenario.ctx.targetOcc;
}

string planner_state_cache_hash(
    const RawEngine& RE,
    OccID targetOcc,
    SplitChoiceCacheTelemetry* telemetry
) {
    PlannerStateDump dump;
    dump.engine = RE;
    dump.targetOcc = targetOcc;
    dump.traceLevel = TraceLevel::SUMMARY;
    const string serialized = serialize_planner_state_dump(dump);
    const auto it = g_splitChoiceStateHashCache.find(serialized);
    if (it != g_splitChoiceStateHashCache.end()) {
        if (telemetry != nullptr) {
            ++telemetry->stateHashHitCount;
        }
        return it->second;
    }
    const string hash = stable_hash_text(serialized);
    g_splitChoiceStateHashCache.emplace(serialized, hash);
    if (telemetry != nullptr) {
        ++telemetry->stateHashMissCount;
    }
    return hash;
}

string planner_state_cache_hash(
    const PlannerStateDump& dump,
    SplitChoiceCacheTelemetry* telemetry
) {
    const string serialized = serialize_planner_state_dump(dump);
    const auto it = g_splitChoiceStateHashCache.find(serialized);
    if (it != g_splitChoiceStateHashCache.end()) {
        if (telemetry != nullptr) {
            ++telemetry->stateHashHitCount;
        }
        return it->second;
    }
    const string hash = stable_hash_text(serialized);
    g_splitChoiceStateHashCache.emplace(serialized, hash);
    if (telemetry != nullptr) {
        ++telemetry->stateHashMissCount;
    }
    return hash;
}

string inverse_relabel_fingerprint(const unordered_map<Vertex, Vertex>* inverseRelabel) {
    if (inverseRelabel == nullptr || inverseRelabel->empty()) {
        return "none";
    }
    vector<pair<Vertex, Vertex>> entries(inverseRelabel->begin(), inverseRelabel->end());
    sort(entries.begin(), entries.end());
    ostringstream oss;
    for (size_t i = 0; i < entries.size(); ++i) {
        if (i != 0U) {
            oss << ',';
        }
        oss << entries[i].first << "->" << entries[i].second;
    }
    return stable_hash_text(oss.str());
}

bool pair_has_boundary_only_followup(
    const SplitSeparationPairResult& result,
    const RawPlannerCtx& ctx
) {
    const int anchorIdx = choose_anchor_child(result, ctx);
    if (anchorIdx < 0) {
        return false;
    }
    for (int i = 0; i < static_cast<int>(result.child.size()); ++i) {
        if (i == anchorIdx) {
            continue;
        }
        if (result.child[static_cast<size_t>(i)].boundaryOnly) {
            return true;
        }
    }
    return false;
}

bool pair_has_keep_occ_sibling_followup(
    const SplitSeparationPairResult& result,
    const RawPlannerCtx& ctx
) {
    const int anchorIdx = choose_anchor_child(result, ctx);
    if (anchorIdx < 0) {
        return false;
    }
    for (int i = 0; i < static_cast<int>(result.child.size()); ++i) {
        if (i == anchorIdx) {
            continue;
        }
        const SplitChildInfo& child = result.child[static_cast<size_t>(i)];
        if (!child.boundaryOnly && child_intersects_keep_occ(child, ctx)) {
            return true;
        }
    }
    return false;
}

vector<pair<Vertex, Vertex>> bounded_split_pairs(
    const vector<pair<Vertex, Vertex>>& allPairs,
    const vector<pair<Vertex, Vertex>>& priorityPairs,
    size_t maxCandidates
) {
    if (maxCandidates == 0U || allPairs.size() <= maxCandidates) {
        return allPairs;
    }

    vector<pair<Vertex, Vertex>> chosen;
    chosen.reserve(maxCandidates);
    for (const pair<Vertex, Vertex>& pairValue : priorityPairs) {
        const pair<Vertex, Vertex> ordered = ordered_pair(pairValue.first, pairValue.second);
        if (find(chosen.begin(), chosen.end(), ordered) != chosen.end()) {
            continue;
        }
        chosen.push_back(ordered);
        if (chosen.size() >= maxCandidates) {
            sort(chosen.begin(), chosen.end());
            return chosen;
        }
    }
    for (const pair<Vertex, Vertex>& pairValue : allPairs) {
        if (find(chosen.begin(), chosen.end(), pairValue) != chosen.end()) {
            continue;
        }
        if (chosen.size() >= maxCandidates) {
            break;
        }
        chosen.push_back(pairValue);
    }
    sort(chosen.begin(), chosen.end());
    return chosen;
}

EvaluatedSplitChoiceOutcome evaluate_candidate_outcome(
    const TestOptions& options,
    const ExhaustiveScenario& scenario,
    pair<Vertex, Vertex> pairValue,
    const unordered_map<Vertex, Vertex>* inverseRelabel,
    const string& scenarioHash,
    unordered_map<string, EvaluatedSplitChoiceOutcome>& candidateEvaluationCache,
    SplitChoiceCacheTelemetry* telemetry,
    size_t& canonicalizationNanos
) {
    const TestOptions& previousOptions = active_test_options();
    TestOptions localOptions = options;
    localOptions.oracleMode = primitive_oracle_enabled(previousOptions) ? OracleMode::PRIMITIVE : OracleMode::NONE;
    set_active_test_options(&localOptions);

    EvaluatedSplitChoiceOutcome evaluated;
    evaluated.publicOutcome.pair = normalize_pair(pairValue, inverseRelabel);
    const string cacheKey =
        scenarioHash + ";pair=" + to_string(pairValue.first) + "," + to_string(pairValue.second) +
        ";family=" + string(exhaustive_family_name(scenario.family)) +
        ";inverse=" + inverse_relabel_fingerprint(inverseRelabel) +
        ";step=" + to_string(options.stepBudget) +
        ";max_eval=" + to_string(options.maxSplitChoiceEval) +
        ";policy=" + split_choice_policy_mode_name_string(options.splitChoicePolicyMode) +
        ";cap=" + to_string(effective_split_choice_exact_cap(options));
    if (const auto cacheIt = candidateEvaluationCache.find(cacheKey);
        cacheIt != candidateEvaluationCache.end()) {
        if (telemetry != nullptr) {
            ++telemetry->candidateEvaluationCacheHitCount;
        }
        set_active_test_options(&previousOptions);
        return cacheIt->second;
    }
    if (telemetry != nullptr) {
        ++telemetry->candidateEvaluationCacheMissCount;
    }

    try {
        ExhaustiveScenario working = scenario;
        const RawSkelID targetSid = working.RE.occ.get(working.ctx.targetOcc).hostSkel;
        const SplitSeparationPairResult splitResult = split_checked(
            working.RE,
            targetSid,
            pairValue.first,
            pairValue.second,
            working.U,
            working.label + "_split_choice_" + to_string(pairValue.first) + "_" + to_string(pairValue.second)
        );

        const RawUpdaterHooks hooks = make_basic_hooks_for_target(working.ctx);
        deque<UpdJob> q;
        hooks.afterSplit(splitResult, q);
        vector<UpdJob> queue(q.begin(), q.end());
        PlannerExecutionResult execution = run_planner_checked_capture(
            working.RE,
            working.ctx,
            working.U,
            planner_run_options(options),
            queue.empty() ? nullptr : &queue,
            working.label + "_split_choice_followup_" + to_string(pairValue.first) + "_" + to_string(pairValue.second)
        );

        string validationError;
        if (!validate_engine_state_soft(working.RE, &validationError)) {
            throw runtime_error("split-choice final validator failure: " + validationError);
        }

        RawEngine normalizedEngine;
        OccID normalizedTargetOcc = working.ctx.targetOcc;
        normalize_engine_result(
            working.RE,
            working.ctx.targetOcc,
            working.family,
            inverseRelabel,
            normalizedEngine,
            normalizedTargetOcc
        );

        const PlannerFinalStateCanonicalSignature finalSig =
            capture_planner_final_state_canonical_signature(normalizedEngine, normalizedTargetOcc);
        evaluated.publicOutcome.stopConditionSatisfied = finalSig.stopConditionSatisfied;
        evaluated.publicOutcome.plannerFastFinalKey = planner_fast_canonical_state_key(normalizedEngine);
        evaluated.publicOutcome.finalStateDescription = describe_semantic_state_signature(finalSig.finalState);
        evaluated.publicOutcome.finalStateHash = hash_semantic_state_signature(finalSig.finalState);

        optional<IsolatePrepared> targetPrepare;
        if (live_occurrence_exists(normalizedEngine, normalizedTargetOcc)) {
            targetPrepare = prepare_isolate_checked(
                normalizedEngine,
                normalizedEngine.occ.get(normalizedTargetOcc).hostSkel,
                normalizedTargetOcc,
                "split_choice_target_prepare"
            );
            const IsolatePreparedSignature targetPrepareSig = capture_isolate_signature(*targetPrepare);
            evaluated.publicOutcome.plannerFastTargetKey =
                planner_fast_canonical_isolate_key(normalizedEngine, *targetPrepare);
            evaluated.publicOutcome.targetPrepareDescription = describe_signature(targetPrepareSig);
            evaluated.publicOutcome.targetPrepareHash = hash_isolate_signature(targetPrepareSig);
        } else {
            evaluated.publicOutcome.detail = "target occurrence disappeared after split-choice continuation";
        }

        const size_t exactCap = effective_split_choice_exact_cap(options);
        if (exactCap != 0U) {
            const string normalizedStateHash =
                planner_state_cache_hash(normalizedEngine, normalizedTargetOcc, telemetry);
            const string exactCacheKey =
                normalizedStateHash +
                ";stop=" + to_string(finalSig.stopConditionSatisfied ? 1 : 0) +
                ";detail=" + stable_hash_text(evaluated.publicOutcome.detail) +
                ";target=" + evaluated.publicOutcome.targetPrepareHash +
                ";cap=" + to_string(exactCap);
            ExactCanonicalKey exactKey;
            if (const auto exactIt = g_splitChoiceExactCanonicalKeyCache.find(exactCacheKey);
                exactIt != g_splitChoiceExactCanonicalKeyCache.end()) {
                if (telemetry != nullptr) {
                    ++telemetry->exactCanonicalCacheHitCount;
                }
                exactKey.key = exactIt->second;
            } else {
                if (telemetry != nullptr) {
                    ++telemetry->exactCanonicalCacheMissCount;
                }
                const auto canonicalBegin = chrono::steady_clock::now();
                exactKey = compute_exact_split_choice_canonical_key(
                    normalizedEngine,
                    targetPrepare.has_value() ? &*targetPrepare : nullptr,
                    finalSig.stopConditionSatisfied,
                    evaluated.publicOutcome.detail,
                    exactCap
                );
                canonicalizationNanos += static_cast<size_t>(elapsed_nanos(canonicalBegin));
                if (!exactKey.skipped) {
                    g_splitChoiceExactCanonicalKeyCache.emplace(exactCacheKey, exactKey.key);
                }
            }
            if (!exactKey.skipped) {
                evaluated.publicOutcome.exactSplitChoiceKey = exactKey.key;
            }
        }

        evaluated.publicOutcome.traceHash = hash_planner_trace_prefix(execution.trace, execution.trace.size());
        evaluated.publicOutcome.trace = std::move(execution.trace);
        evaluated.equivalenceKey = string("ok:") +
            (evaluated.publicOutcome.stopConditionSatisfied ? "1" : "0") + ':' +
            evaluated.publicOutcome.finalStateHash + ':' +
            evaluated.publicOutcome.targetPrepareHash;
        evaluated.fastClassKey = fast_split_choice_key(evaluated.publicOutcome);
        evaluated.exactOrderKey = exact_full_order_key(evaluated.publicOutcome);
    } catch (const exception& ex) {
        evaluated.publicOutcome.detail = ex.what();
        evaluated.publicOutcome.grade = SplitChoiceGrade::SEMANTIC_MISMATCH;
        evaluated.equivalenceKey = string("failure:") + stable_hash_text(ex.what());
        evaluated.fastClassKey = fast_split_choice_key(evaluated.publicOutcome);
        evaluated.exactOrderKey = exact_full_order_key(evaluated.publicOutcome);
    }

    set_active_test_options(&previousOptions);
    candidateEvaluationCache.emplace(cacheKey, evaluated);
    return evaluated;
}

SplitChoiceGrade classify_grade(
    const SplitChoiceCandidateOutcome& baseline,
    const SplitChoiceCandidateOutcome& candidate
) {
    if (candidate.detail.empty() != baseline.detail.empty()) {
        return SplitChoiceGrade::SEMANTIC_MISMATCH;
    }
    if (!candidate.detail.empty()) {
        return candidate.detail == baseline.detail
            ? SplitChoiceGrade::FULLY_EQUIVALENT
            : SplitChoiceGrade::SEMANTIC_MISMATCH;
    }
    if (candidate.stopConditionSatisfied != baseline.stopConditionSatisfied) {
        return SplitChoiceGrade::SEMANTIC_MISMATCH;
    }
    if (candidate.targetPrepareHash != baseline.targetPrepareHash) {
        return SplitChoiceGrade::SEMANTIC_MISMATCH;
    }
    if (candidate.finalStateHash != baseline.finalStateHash) {
        return SplitChoiceGrade::SAME_STOP_DIFFERENT_FINAL_STATE;
    }
    return SplitChoiceGrade::FULLY_EQUIVALENT;
}

pair<const SplitChoiceCandidateOutcome*, const SplitChoiceCandidateOutcome*> select_reference_and_mismatch(
    const SplitChoiceOracleRunResult& result
) {
    const SplitChoiceCandidateOutcome* selected = nullptr;
    const SplitChoiceCandidateOutcome* mismatch = nullptr;
    for (const SplitChoiceCandidateOutcome& outcome : result.outcomes) {
        if (outcome.selectedByPlanner) {
            selected = &outcome;
            continue;
        }
        if (outcome.grade != SplitChoiceGrade::FULLY_EQUIVALENT && mismatch == nullptr) {
            mismatch = &outcome;
        }
    }
    return {selected, mismatch};
}

pair<const SplitChoiceCandidateOutcome*, const SplitChoiceCandidateOutcome*> select_compare_reference_and_mismatch(
    const SplitChoiceOracleRunResult& result
) {
    const SplitChoiceCandidateOutcome* exactShadow = nullptr;
    const SplitChoiceCandidateOutcome* exactFull = nullptr;
    for (const SplitChoiceCandidateOutcome& outcome : result.outcomes) {
        if (outcome.selectedByExactShadow) {
            exactShadow = &outcome;
        } else if (outcome.selectedByExactFull) {
            exactFull = &outcome;
        }
    }
    return {exactShadow, exactFull};
}

FailureMismatchKind mismatch_kind_from_outcomes(
    const SplitChoiceCandidateOutcome& selected,
    const SplitChoiceCandidateOutcome& mismatch
) {
    if (!mismatch.detail.empty() || !selected.detail.empty()) {
        return FailureMismatchKind::FINAL_STATE;
    }
    if (selected.stopConditionSatisfied != mismatch.stopConditionSatisfied) {
        return FailureMismatchKind::STOP_CONDITION;
    }
    if (selected.targetPrepareHash != mismatch.targetPrepareHash) {
        return FailureMismatchKind::TARGET_PREPARE;
    }
    return FailureMismatchKind::FINAL_STATE;
}

bool same_semantic_projection(
    const SplitChoiceCandidateOutcome& lhs,
    const SplitChoiceCandidateOutcome& rhs
) {
    return lhs.detail == rhs.detail &&
           lhs.stopConditionSatisfied == rhs.stopConditionSatisfied &&
           lhs.finalStateHash == rhs.finalStateHash &&
           lhs.targetPrepareHash == rhs.targetPrepareHash;
}

bool same_exact_audit_projection(
    const SplitChoiceCandidateOutcome& lhs,
    const SplitChoiceCandidateOutcome& rhs
) {
    if (!lhs.exactSplitChoiceKey.empty() && !rhs.exactSplitChoiceKey.empty()) {
        return lhs.exactSplitChoiceKey == rhs.exactSplitChoiceKey;
    }
    return same_semantic_projection(lhs, rhs);
}

SplitChoiceRepresentativeShiftKind classify_representative_shift(
    const SplitChoiceCandidateOutcome& selected,
    const SplitChoiceCandidateOutcome& exactRepresentative
) {
    if (selected.pair == exactRepresentative.pair) {
        return SplitChoiceRepresentativeShiftKind::NONE;
    }
    if (!same_exact_audit_projection(selected, exactRepresentative)) {
        return SplitChoiceRepresentativeShiftKind::SEMANTIC;
    }
    if (selected.traceHash != exactRepresentative.traceHash) {
        return SplitChoiceRepresentativeShiftKind::TRACE_ONLY;
    }
    return SplitChoiceRepresentativeShiftKind::HARMLESS;
}

} // namespace

const char* split_choice_grade_name(SplitChoiceGrade grade) {
    switch (grade) {
        case SplitChoiceGrade::FULLY_EQUIVALENT:
            return "fully_equivalent";
        case SplitChoiceGrade::SAME_STOP_DIFFERENT_FINAL_STATE:
            return "same_stop_different_final_state";
        case SplitChoiceGrade::SEMANTIC_MISMATCH:
            return "semantic_mismatch";
    }
    return "unknown";
}

const char* compare_eligibility_reason_name(CompareEligibilityReason reason) {
    switch (reason) {
        case CompareEligibilityReason::ELIGIBLE:
            return "eligible";
        case CompareEligibilityReason::NO_SPLIT_READY:
            return "no_split_ready";
        case CompareEligibilityReason::SINGLE_ADMISSIBLE_PAIR:
            return "single_admissible_pair";
        case CompareEligibilityReason::TIE_BUT_NO_BOUNDARY_ARTIFACT:
            return "tie_but_no_boundary_artifact";
        case CompareEligibilityReason::TIE_BUT_NO_KEEPOCC_SIBLING:
            return "tie_but_no_keepOcc_sibling";
        case CompareEligibilityReason::PLANNER_MIXED_FOLLOWUP_ABSENT:
            return "planner_mixed_followup_absent";
    }
    return "unknown";
}

const char* split_choice_representative_shift_name(SplitChoiceRepresentativeShiftKind kind) {
    switch (kind) {
        case SplitChoiceRepresentativeShiftKind::NONE:
            return "none";
        case SplitChoiceRepresentativeShiftKind::HARMLESS:
            return "harmless_shift";
        case SplitChoiceRepresentativeShiftKind::TRACE_ONLY:
            return "trace_only_shift";
        case SplitChoiceRepresentativeShiftKind::SEMANTIC:
            return "semantic_shift";
    }
    return "unknown";
}

bool split_choice_has_mismatch(const SplitChoiceOracleRunResult& result) {
    return result.sameStopDifferentFinalStateCount != 0U || result.semanticMismatchCount != 0U;
}

bool split_choice_has_exact_audit_disagreement(const SplitChoiceOracleRunResult& result) {
    return result.exactAuditAvailable &&
           (result.fastClassCount != result.exactClassCount ||
            result.exactVsFastClassDisagreementCount != 0U ||
            result.representativeShifted ||
            result.semanticDisagreementCount != 0U);
}

CompareEligibilityInfo analyze_compare_eligibility(
    const ExhaustiveScenario& scenario,
    size_t maxSplitPairCandidates
) {
    CompareEligibilityInfo info;
    const vector<pair<Vertex, Vertex>> allPairs = enumerate_valid_split_pairs(scenario.RE, scenario.ctx.targetOcc);
    info.admissibleSplitPairCount = allPairs.size();
    info.hasSplitReady = !allPairs.empty();
    if (allPairs.empty()) {
        info.reason = CompareEligibilityReason::NO_SPLIT_READY;
        return info;
    }

    info.hasTie = allPairs.size() >= 2U;
    if (!info.hasTie) {
        info.reason = CompareEligibilityReason::SINGLE_ADMISSIBLE_PAIR;
        return info;
    }

    const vector<pair<Vertex, Vertex>> candidatePairs = bounded_split_pairs(allPairs, {}, maxSplitPairCandidates);
    info.evaluatedPairCount = candidatePairs.size();
    for (const pair<Vertex, Vertex>& pairValue : candidatePairs) {
        ExhaustiveScenario candidate = scenario;
        candidate.initialQueue.clear();
        const RawSkelID sid = candidate.RE.occ.get(candidate.ctx.targetOcc).hostSkel;
        const SplitSeparationPairResult splitResult = split_checked(
            candidate.RE,
            sid,
            pairValue.first,
            pairValue.second,
            candidate.U,
            "compare_eligibility_probe"
        );
        info.hasBoundaryOnlyChildAfterSplit =
            info.hasBoundaryOnlyChildAfterSplit || pair_has_boundary_only_followup(splitResult, candidate.ctx);
        info.hasKeepOccSiblingAfterSplit =
            info.hasKeepOccSiblingAfterSplit || pair_has_keep_occ_sibling_followup(splitResult, candidate.ctx);

        deque<UpdJob> q;
        make_basic_hooks_for_target(candidate.ctx).afterSplit(splitResult, q);
        const bool hasJoin = any_of(q.begin(), q.end(), [](const UpdJob& job) {
            return job.kind == UpdJobKind::JOIN_PAIR;
        });
        const bool hasIntegrate = any_of(q.begin(), q.end(), [](const UpdJob& job) {
            return job.kind == UpdJobKind::INTEGRATE_CHILD;
        });
        info.hasPlannerMixedFollowup = info.hasPlannerMixedFollowup || (hasJoin && hasIntegrate);
        if (info.hasBoundaryOnlyChildAfterSplit &&
            info.hasKeepOccSiblingAfterSplit &&
            info.hasPlannerMixedFollowup) {
            break;
        }
    }

    info.compareEligible = true;
    if (!info.hasBoundaryOnlyChildAfterSplit) {
        info.reason = CompareEligibilityReason::TIE_BUT_NO_BOUNDARY_ARTIFACT;
        return info;
    }
    if (!info.hasKeepOccSiblingAfterSplit) {
        info.reason = CompareEligibilityReason::TIE_BUT_NO_KEEPOCC_SIBLING;
        return info;
    }
    if (!info.hasPlannerMixedFollowup) {
        info.reason = CompareEligibilityReason::PLANNER_MIXED_FOLLOWUP_ABSENT;
        return info;
    }

    info.mixedCompareEligible = true;
    info.reason = CompareEligibilityReason::ELIGIBLE;
    return info;
}

bool split_choice_same_exact_projection(
    const SplitChoiceCandidateOutcome& lhs,
    const SplitChoiceCandidateOutcome& rhs
) {
    return same_exact_audit_projection(lhs, rhs);
}

vector<pair<Vertex, Vertex>> enumerate_valid_split_pairs(const RawEngine& RE, OccID occ) {
    vector<pair<Vertex, Vertex>> out;
    if (!live_occurrence_exists(RE, occ)) {
        return out;
    }

    const RawOccRecord& O = RE.occ.get(occ);
    if (!RE.skel.a[O.hostSkel].alive) {
        return out;
    }
    const RawSkeleton& S = RE.skel.get(O.hostSkel);
    if (S.hostedOcc.size() != 1U || S.hostedOcc[0] != occ) {
        return out;
    }

    const vector<RawVID> support = collect_support_vertices(RE, occ);
    vector<pair<Vertex, RawVID>> realVertices;
    realVertices.reserve(S.verts.size());
    for (RawVID vid : S.verts) {
        const RawVertex& vertex = RE.V.get(vid);
        if (vertex.kind == RawVertexKind::REAL) {
            realVertices.push_back({vertex.orig, vid});
        }
    }
    sort(realVertices.begin(), realVertices.end());

    for (size_t i = 0; i < realVertices.size(); ++i) {
        for (size_t j = i + 1U; j < realVertices.size(); ++j) {
            if (valid_split_pair_for_support(RE, S, realVertices[i].second, realVertices[j].second, support)) {
                out.push_back(ordered_pair(realVertices[i].first, realVertices[j].first));
            }
        }
    }

    sort(out.begin(), out.end());
    out.erase(unique(out.begin(), out.end()), out.end());
    return out;
}

SplitChoiceOracleRunResult run_split_choice_oracle(
    const TestOptions& options,
    const ExhaustiveScenario& scenario,
    const unordered_map<Vertex, Vertex>* inverseRelabel
) {
    if (!scenario.initialQueue.empty()) {
        throw runtime_error("split-choice oracle expects an empty initial planner queue");
    }

    const RawUpdaterRunOptions fastRunOptions =
        planner_run_options_with(options.stepBudget, options.maxSplitChoiceEval, SplitChoicePolicyMode::FAST);
    const RawUpdaterRunOptions exactShadowRunOptions =
        planner_run_options_with(options.stepBudget, options.maxSplitChoiceEval, SplitChoicePolicyMode::EXACT_SHADOW);

    SplitChoicePolicyStats fastPolicyStats;
    SplitChoicePolicyStats exactShadowPolicyStats;
    SplitChoiceCacheTelemetry telemetry;
    const optional<pair<Vertex, Vertex>> fastSelectedPairRaw = discover_split_pair_from_support(
        scenario.RE,
        scenario.ctx.targetOcc,
        &scenario.ctx,
        fastRunOptions,
        &fastPolicyStats
    );
    const auto exactShadowBegin = chrono::steady_clock::now();
    const optional<pair<Vertex, Vertex>> exactShadowSelectedPairRaw = discover_split_pair_from_support(
        scenario.RE,
        scenario.ctx.targetOcc,
        &scenario.ctx,
        exactShadowRunOptions,
        &exactShadowPolicyStats
    );
    const size_t exactShadowEvaluationNanos = static_cast<size_t>(elapsed_nanos(exactShadowBegin));
    if (!fastSelectedPairRaw.has_value() || !exactShadowSelectedPairRaw.has_value()) {
        throw runtime_error("split-choice oracle expected both fast and exact-shadow selections");
    }
    const pair<Vertex, Vertex> fastSelectedPair =
        ordered_pair(fastSelectedPairRaw->first, fastSelectedPairRaw->second);
    const pair<Vertex, Vertex> exactShadowSelectedPair =
        ordered_pair(exactShadowSelectedPairRaw->first, exactShadowSelectedPairRaw->second);

    const auto enumerationBegin = chrono::steady_clock::now();
    const vector<pair<Vertex, Vertex>> allPairs = enumerate_valid_split_pairs(scenario.RE, scenario.ctx.targetOcc);
    const size_t candidateEnumerationNanos = static_cast<size_t>(elapsed_nanos(enumerationBegin));
    if (allPairs.size() < 2U) {
        throw runtime_error("split-choice oracle expected at least two admissible split pairs");
    }

    SplitChoiceOracleRunResult result;
    result.fastSelectedPair = normalize_pair(fastSelectedPair, inverseRelabel);
    result.exactShadowSelectedPair = normalize_pair(exactShadowSelectedPair, inverseRelabel);
    result.admissiblePairCount = allPairs.size();
    result.exactShadowEvalCount = exactShadowPolicyStats.evalCount;
    result.fallbackCount = exactShadowPolicyStats.fallbackCount;
    result.candidateEnumerationNanos = candidateEnumerationNanos;
    result.exactShadowEvaluationNanos = exactShadowEvaluationNanos;

    PlannerStateDump scenarioDump;
    scenarioDump.engine = scenario.RE;
    scenarioDump.targetOcc = scenario.ctx.targetOcc;
    scenarioDump.keepOcc.assign(scenario.ctx.keepOcc.begin(), scenario.ctx.keepOcc.end());
    sort(scenarioDump.keepOcc.begin(), scenarioDump.keepOcc.end());
    scenarioDump.initialQueue = scenario.initialQueue;
    scenarioDump.traceLevel = TraceLevel::SUMMARY;
    const string scenarioHash = planner_state_cache_hash(scenarioDump, &telemetry);

    const vector<pair<Vertex, Vertex>> comparedPairs =
        bounded_split_pairs(
            allPairs,
            {fastSelectedPair, exactShadowSelectedPair},
            options.maxSplitPairCandidates
        );
    if ((options.splitChoicePolicyMode == SplitChoicePolicyMode::EXACT_FULL ||
         options.splitChoiceCompareMode == SplitChoiceCompareMode::EXACT_FULL) &&
        comparedPairs.size() != allPairs.size()) {
        throw runtime_error("exact_full split-choice compare requires evaluating all admissible pairs");
    }
    result.comparedPairCount = comparedPairs.size();
    result.exactFullEvalCount = comparedPairs.size();

    vector<EvaluatedSplitChoiceOutcome> evaluated;
    unordered_map<string, EvaluatedSplitChoiceOutcome> candidateEvaluationCache;
    evaluated.reserve(comparedPairs.size());
    size_t canonicalizationNanos = 0U;
    const auto exactFullBegin = chrono::steady_clock::now();
    for (const pair<Vertex, Vertex>& pairValue : comparedPairs) {
        evaluated.push_back(evaluate_candidate_outcome(
            options,
            scenario,
            pairValue,
            inverseRelabel,
            scenarioHash,
            candidateEvaluationCache,
            &telemetry,
            canonicalizationNanos
        ));
    }
    result.exactFullEvaluationNanos = static_cast<size_t>(elapsed_nanos(exactFullBegin));
    result.canonicalizationNanos = canonicalizationNanos;
    result.stateHashCacheHitCount = telemetry.stateHashHitCount;
    result.stateHashCacheMissCount = telemetry.stateHashMissCount;
    result.candidateEvaluationCacheHitCount = telemetry.candidateEvaluationCacheHitCount;
    result.candidateEvaluationCacheMissCount = telemetry.candidateEvaluationCacheMissCount;
    result.exactCanonicalCacheHitCount = telemetry.exactCanonicalCacheHitCount;
    result.exactCanonicalCacheMissCount = telemetry.exactCanonicalCacheMissCount;

    unordered_map<string, size_t> classCounts;
    unordered_map<string, size_t> fastClassCounts;
    unordered_map<string, size_t> exactClassCounts;
    for (const EvaluatedSplitChoiceOutcome& entry : evaluated) {
        ++fastClassCounts[entry.fastClassKey];
        if (!entry.publicOutcome.exactSplitChoiceKey.empty()) {
            ++exactClassCounts[entry.publicOutcome.exactSplitChoiceKey];
        }
    }
    result.fastClassCount = fastClassCounts.size();
    result.exactAuditAvailable = !evaluated.empty();
    for (const EvaluatedSplitChoiceOutcome& entry : evaluated) {
        const SplitChoiceCandidateOutcome& outcome = entry.publicOutcome;
        if (outcome.exactSplitChoiceKey.empty()) {
            result.exactAuditAvailable = false;
            break;
        }
    }
    if (result.exactAuditAvailable) {
        result.exactClassCount = exactClassCounts.size();
        for (size_t i = 0; i < evaluated.size(); ++i) {
            for (size_t j = i + 1U; j < evaluated.size(); ++j) {
                const bool fastSame =
                    evaluated[i].fastClassKey ==
                    evaluated[j].fastClassKey;
                const bool exactSame =
                    evaluated[i].publicOutcome.exactSplitChoiceKey ==
                    evaluated[j].publicOutcome.exactSplitChoiceKey;
                if (fastSame != exactSame) {
                    ++result.exactVsFastClassDisagreementCount;
                }
            }
        }

        string bestExactKey;
        bool haveBestExact = false;
        const SplitChoiceCandidateOutcome* bestExactOutcome = nullptr;
        string bestExactOrderKey;
        for (const EvaluatedSplitChoiceOutcome& entry : evaluated) {
            const SplitChoiceCandidateOutcome& outcome = entry.publicOutcome;
            if (!haveBestExact || outcome.exactSplitChoiceKey < bestExactKey) {
                bestExactKey = outcome.exactSplitChoiceKey;
                haveBestExact = true;
            }
        }
        for (const EvaluatedSplitChoiceOutcome& entry : evaluated) {
            const SplitChoiceCandidateOutcome& outcome = entry.publicOutcome;
            if (outcome.exactSplitChoiceKey != bestExactKey) {
                continue;
            }
            const string& orderKey = entry.exactOrderKey;
            if (bestExactOutcome == nullptr || orderKey < bestExactOrderKey) {
                bestExactOutcome = &outcome;
                bestExactOrderKey = orderKey;
            }
        }
        if (bestExactOutcome != nullptr) {
            result.exactRepresentativePair = bestExactOutcome->pair;
            result.exactFullSelectedPair = bestExactOutcome->pair;
        }
    } else {
        result.exactFullSelectedPair = result.exactShadowSelectedPair;
    }

    if (result.exactFullSelectedPair.first == NIL_U32 && result.exactFullSelectedPair.second == NIL_U32) {
        result.exactFullSelectedPair = result.exactShadowSelectedPair;
    }
    result.selectedPair = selected_pair_for_mode(
        options.splitChoicePolicyMode,
        result.fastSelectedPair,
        result.exactShadowSelectedPair,
        result.exactFullSelectedPair
    );
    result.policySelectedPair = result.selectedPair;

    for (EvaluatedSplitChoiceOutcome& entry : evaluated) {
        entry.publicOutcome.selectedByPlanner = (entry.publicOutcome.pair == result.selectedPair);
        entry.publicOutcome.selectedByPolicy = entry.publicOutcome.selectedByPlanner;
        entry.publicOutcome.selectedByFast = (entry.publicOutcome.pair == result.fastSelectedPair);
        entry.publicOutcome.selectedByExactShadow = (entry.publicOutcome.pair == result.exactShadowSelectedPair);
        entry.publicOutcome.selectedByExactFull = (entry.publicOutcome.pair == result.exactFullSelectedPair);
    }

    auto selectedIt = find_if(
        evaluated.begin(),
        evaluated.end(),
        [](const EvaluatedSplitChoiceOutcome& entry) { return entry.publicOutcome.selectedByPlanner; }
    );
    if (selectedIt == evaluated.end()) {
        throw runtime_error("split-choice oracle did not evaluate the selected pair");
    }

    for (EvaluatedSplitChoiceOutcome& entry : evaluated) {
        entry.publicOutcome.grade = classify_grade(selectedIt->publicOutcome, entry.publicOutcome);
        ++classCounts[entry.equivalenceKey];
        switch (entry.publicOutcome.grade) {
            case SplitChoiceGrade::FULLY_EQUIVALENT:
                ++result.fullyEquivalentCount;
                break;
            case SplitChoiceGrade::SAME_STOP_DIFFERENT_FINAL_STATE:
                ++result.sameStopDifferentFinalStateCount;
                break;
            case SplitChoiceGrade::SEMANTIC_MISMATCH:
                ++result.semanticMismatchCount;
                break;
        }
        result.outcomes.push_back(std::move(entry.publicOutcome));
    }

    if (result.exactAuditAvailable) {
        const SplitChoiceCandidateOutcome* selectedOutcome = nullptr;
        const SplitChoiceCandidateOutcome* exactShadowOutcome = nullptr;
        const SplitChoiceCandidateOutcome* exactFullOutcome = nullptr;
        for (const SplitChoiceCandidateOutcome& outcome : result.outcomes) {
            if (outcome.selectedByPlanner) {
                selectedOutcome = &outcome;
            }
            if (outcome.selectedByExactShadow) {
                exactShadowOutcome = &outcome;
            }
            if (outcome.selectedByExactFull) {
                exactFullOutcome = &outcome;
            }
        }
        if (selectedOutcome != nullptr && exactFullOutcome != nullptr) {
            result.exactRepresentativeSelected = (selectedOutcome->pair == exactFullOutcome->pair);
        }
        if (selectedOutcome != nullptr && exactShadowOutcome != nullptr) {
            result.representativeShifted = (selectedOutcome->pair != exactShadowOutcome->pair);
            result.representativeShiftCount = result.representativeShifted ? 1U : 0U;
            if (result.representativeShifted) {
                if (selectedOutcome->exactSplitChoiceKey == exactShadowOutcome->exactSplitChoiceKey) {
                    result.representativeShiftSameClassCount = 1U;
                }
                result.representativeShiftKind =
                    classify_representative_shift(*selectedOutcome, *exactShadowOutcome);
                const bool traceDiverged = selectedOutcome->traceHash != exactShadowOutcome->traceHash;
                const bool followupDiverged =
                    traceDiverged || !same_semantic_projection(*selectedOutcome, *exactShadowOutcome);
                result.representativeShiftTraceDivergenceCount = traceDiverged ? 1U : 0U;
                result.representativeShiftFollowupDivergenceCount = followupDiverged ? 1U : 0U;
                switch (result.representativeShiftKind) {
                    case SplitChoiceRepresentativeShiftKind::NONE:
                        break;
                    case SplitChoiceRepresentativeShiftKind::HARMLESS:
                        result.harmlessShiftCount = 1U;
                        break;
                    case SplitChoiceRepresentativeShiftKind::TRACE_ONLY:
                        result.traceOnlyShiftCount = 1U;
                        break;
                    case SplitChoiceRepresentativeShiftKind::SEMANTIC:
                        result.semanticShiftCount = 1U;
                        result.representativeShiftSemanticDivergenceCount = 1U;
                        break;
                }
            }
        }
        if (exactShadowOutcome != nullptr && exactFullOutcome != nullptr) {
            result.compareStateCount = 1U;
            result.sameRepresentativeCount = exactShadowOutcome->pair == exactFullOutcome->pair ? 1U : 0U;
            result.sameSemanticClassCount =
                exactShadowOutcome->exactSplitChoiceKey == exactFullOutcome->exactSplitChoiceKey ? 1U : 0U;
            result.sameFinalStateCount =
                same_exact_audit_projection(*exactShadowOutcome, *exactFullOutcome) ? 1U : 0U;
            result.exactShadowVsExactFullShifted = exactShadowOutcome->pair != exactFullOutcome->pair;
            result.exactShadowVsExactFullShiftKind =
                classify_representative_shift(*exactShadowOutcome, *exactFullOutcome);
            switch (result.exactShadowVsExactFullShiftKind) {
                case SplitChoiceRepresentativeShiftKind::NONE:
                    break;
                case SplitChoiceRepresentativeShiftKind::HARMLESS:
                    result.harmlessCompareCount = 1U;
                    break;
                case SplitChoiceRepresentativeShiftKind::TRACE_ONLY:
                    result.traceOnlyCompareCount = 1U;
                    break;
                case SplitChoiceRepresentativeShiftKind::SEMANTIC:
                    result.semanticDisagreementCount = 1U;
                    break;
            }
        }
        if (result.exactClassCount > 1U) {
            const auto multiclassBegin = chrono::steady_clock::now();
            result.multiclassCatalogKey = build_multiclass_catalog_key(result);
            result.multiclassCatalogNanos = static_cast<size_t>(elapsed_nanos(multiclassBegin));
        }
    }

    if ((options.maxSplitPairCandidates != 0U && comparedPairs.size() < allPairs.size()) ||
        (options.maxSplitChoiceEval != 0U &&
         exactShadowPolicyStats.candidateCount > exactShadowPolicyStats.evalCount) ||
        !result.exactAuditAvailable) {
        result.capHitCount = 1U;
    }
    if (options.splitChoicePolicyMode == SplitChoicePolicyMode::EXACT_FULL && !result.exactAuditAvailable) {
        throw runtime_error("split-choice exact_full policy requires full exact audit availability");
    }

    for (SplitChoiceCandidateOutcome& outcome : result.outcomes) {
        outcome.selectedByPlanner = (outcome.pair == result.selectedPair);
        outcome.selectedByPolicy = outcome.selectedByPlanner;
        outcome.selectedByFast = (outcome.pair == result.fastSelectedPair);
        outcome.selectedByExactShadow = (outcome.pair == result.exactShadowSelectedPair);
        outcome.selectedByExactFull = (outcome.pair == result.exactFullSelectedPair);
    }

    const auto selectedClassIt = classCounts.find(selectedIt->equivalenceKey);
    result.selectedEquivalenceClassSize =
        selectedClassIt == classCounts.end() ? 0U : selectedClassIt->second;
    for (const auto& [_, count] : classCounts) {
        result.maxEquivalenceClassSize = max(result.maxEquivalenceClassSize, count);
    }
    result.selectedPairRepresentative =
        result.selectedEquivalenceClassSize != 0U &&
        result.selectedEquivalenceClassSize == result.maxEquivalenceClassSize;

    sort(
        result.outcomes.begin(),
        result.outcomes.end(),
        [](const SplitChoiceCandidateOutcome& lhs, const SplitChoiceCandidateOutcome& rhs) {
            return tie(
                       lhs.pair,
                       lhs.selectedByPlanner,
                       lhs.selectedByFast,
                       lhs.selectedByExactShadow,
                       lhs.selectedByExactFull
                   ) <
                   tie(
                       rhs.pair,
                       rhs.selectedByPlanner,
                       rhs.selectedByFast,
                       rhs.selectedByExactShadow,
                       rhs.selectedByExactFull
                   );
        }
    );
    return result;
}

bool replay_split_choice_oracle_dump(
    const TestOptions& options,
    const PlannerStateDump& dump,
    FailureSignature* failure
) {
    ExhaustiveScenario scenario;
    scenario.RE = dump.engine;
    scenario.ctx.targetOcc = dump.targetOcc;
    scenario.ctx.keepOcc.insert(dump.keepOcc.begin(), dump.keepOcc.end());
    scenario.initialQueue = dump.initialQueue;
    scenario.label = dump.caseName;

    const SplitChoiceOracleRunResult result = run_split_choice_oracle(options, scenario, nullptr);
    if (!split_choice_has_mismatch(result) && !split_choice_has_exact_audit_disagreement(result)) {
        return true;
    }

    if (failure != nullptr) {
        const auto [selected, mismatch] =
            split_choice_has_mismatch(result)
                ? select_reference_and_mismatch(result)
                : select_compare_reference_and_mismatch(result);
        if (selected != nullptr && mismatch != nullptr) {
            FailureSignature sig;
            sig.failureClass = FailureClass::PLANNER_ORACLE_MISMATCH;
            sig.stage = FailureStage::REPLAY;
            sig.plannerPhase = PlannerPhase::SPLIT;
            sig.targetOcc = dump.targetOcc;
            sig.mismatchKind = mismatch_kind_from_outcomes(*selected, *mismatch);
            sig.canonicalStateHash = mismatch->finalStateHash;
            sig.oracleHash =
                sig.mismatchKind == FailureMismatchKind::TARGET_PREPARE
                    ? selected->targetPrepareHash
                    : selected->finalStateHash;
            sig.tracePrefixHash = mismatch->traceHash;
            sig.detail =
                string(split_choice_has_mismatch(result)
                           ? "split_choice_oracle_mismatch\n"
                           : "split_choice_compare_mismatch\n") +
                "selected_pair=" + to_string(selected->pair.first) + "," + to_string(selected->pair.second) +
                "\nmismatch_pair=" + to_string(mismatch->pair.first) + "," + to_string(mismatch->pair.second) +
                "\nresult=\n" + describe_split_choice_oracle_result(result);
            *failure = sig;
        } else {
            *failure = make_failure_signature(
                FailureClass::PLANNER_ORACLE_MISMATCH,
                "split-choice oracle mismatch without candidate details"
            );
        }
    }
    return false;
}

string describe_split_choice_oracle_result(const SplitChoiceOracleRunResult& result) {
    ostringstream oss;
    oss << "selected_pair=" << result.selectedPair.first << ',' << result.selectedPair.second << '\n';
    oss << "fast_selected_pair=" << result.fastSelectedPair.first << ',' << result.fastSelectedPair.second << '\n';
    oss << "policy_selected_pair=" << result.policySelectedPair.first << ',' << result.policySelectedPair.second << '\n';
    oss << "exact_shadow_selected_pair=" << result.exactShadowSelectedPair.first << ',' << result.exactShadowSelectedPair.second << '\n';
    oss << "exact_full_selected_pair=" << result.exactFullSelectedPair.first << ',' << result.exactFullSelectedPair.second << '\n';
    oss << "exact_representative_pair=" << result.exactRepresentativePair.first << ',' << result.exactRepresentativePair.second << '\n';
    oss << "admissible_pair_count=" << result.admissiblePairCount << '\n';
    oss << "compared_pair_count=" << result.comparedPairCount << '\n';
    oss << "fully_equivalent_count=" << result.fullyEquivalentCount << '\n';
    oss << "same_stop_different_final_state_count=" << result.sameStopDifferentFinalStateCount << '\n';
    oss << "semantic_mismatch_count=" << result.semanticMismatchCount << '\n';
    oss << "selected_equivalence_class_size=" << result.selectedEquivalenceClassSize << '\n';
    oss << "max_equivalence_class_size=" << result.maxEquivalenceClassSize << '\n';
    oss << "selected_pair_representative=" << (result.selectedPairRepresentative ? 1 : 0) << '\n';
    oss << "exact_audit_available=" << (result.exactAuditAvailable ? 1 : 0) << '\n';
    oss << "fast_class_count=" << result.fastClassCount << '\n';
    oss << "exact_class_count=" << result.exactClassCount << '\n';
    oss << "exact_vs_fast_class_disagreement_count=" << result.exactVsFastClassDisagreementCount << '\n';
    oss << "representative_shift_count=" << result.representativeShiftCount << '\n';
    oss << "representative_shift_same_class_count=" << result.representativeShiftSameClassCount << '\n';
    oss << "representative_shift_semantic_divergence_count=" << result.representativeShiftSemanticDivergenceCount << '\n';
    oss << "representative_shift_followup_divergence_count=" << result.representativeShiftFollowupDivergenceCount << '\n';
    oss << "representative_shift_trace_divergence_count=" << result.representativeShiftTraceDivergenceCount << '\n';
    oss << "harmless_shift_count=" << result.harmlessShiftCount << '\n';
    oss << "trace_only_shift_count=" << result.traceOnlyShiftCount << '\n';
    oss << "semantic_shift_count=" << result.semanticShiftCount << '\n';
    oss << "representative_shift_kind=" << split_choice_representative_shift_name(result.representativeShiftKind) << '\n';
    oss << "representative_shifted=" << (result.representativeShifted ? 1 : 0) << '\n';
    oss << "exact_representative_selected=" << (result.exactRepresentativeSelected ? 1 : 0) << '\n';
    oss << "split_choice_compare_state_count=" << result.compareStateCount << '\n';
    oss << "split_choice_exact_shadow_eval_count=" << result.exactShadowEvalCount << '\n';
    oss << "split_choice_exact_full_eval_count=" << result.exactFullEvalCount << '\n';
    oss << "split_choice_fallback_count=" << result.fallbackCount << '\n';
    oss << "split_choice_same_representative_count=" << result.sameRepresentativeCount << '\n';
    oss << "split_choice_same_semantic_class_count=" << result.sameSemanticClassCount << '\n';
    oss << "split_choice_same_final_state_count=" << result.sameFinalStateCount << '\n';
    oss << "split_choice_semantic_disagreement_count=" << result.semanticDisagreementCount << '\n';
    oss << "split_choice_cap_hit_count=" << result.capHitCount << '\n';
    oss << "split_choice_harmless_compare_count=" << result.harmlessCompareCount << '\n';
    oss << "split_choice_trace_only_compare_count=" << result.traceOnlyCompareCount << '\n';
    oss << "candidate_enumeration_ns=" << result.candidateEnumerationNanos << '\n';
    oss << "exact_shadow_evaluation_ns=" << result.exactShadowEvaluationNanos << '\n';
    oss << "exact_full_evaluation_ns=" << result.exactFullEvaluationNanos << '\n';
    oss << "canonicalization_ns=" << result.canonicalizationNanos << '\n';
    oss << "multiclass_catalog_ns=" << result.multiclassCatalogNanos << '\n';
    oss << "state_hash_cache_hit_count=" << result.stateHashCacheHitCount << '\n';
    oss << "state_hash_cache_miss_count=" << result.stateHashCacheMissCount << '\n';
    oss << "candidate_evaluation_cache_hit_count=" << result.candidateEvaluationCacheHitCount << '\n';
    oss << "candidate_evaluation_cache_miss_count=" << result.candidateEvaluationCacheMissCount << '\n';
    oss << "exact_canonical_cache_hit_count=" << result.exactCanonicalCacheHitCount << '\n';
    oss << "exact_canonical_cache_miss_count=" << result.exactCanonicalCacheMissCount << '\n';
    oss << "split_choice_exact_shadow_vs_exact_full_shifted=" << (result.exactShadowVsExactFullShifted ? 1 : 0)
        << '\n';
    oss << "split_choice_exact_shadow_vs_exact_full_shift_kind="
        << split_choice_representative_shift_name(result.exactShadowVsExactFullShiftKind) << '\n';
    if (!result.multiclassCatalogKey.empty()) {
        oss << "multiclass_catalog_key=" << stable_hash_text(result.multiclassCatalogKey) << '\n';
    }
    for (const SplitChoiceCandidateOutcome& outcome : result.outcomes) {
        oss << "pair=" << outcome.pair.first << ',' << outcome.pair.second
            << " selected=" << (outcome.selectedByPlanner ? 1 : 0)
            << " fast_selected=" << (outcome.selectedByFast ? 1 : 0)
            << " policy_selected=" << (outcome.selectedByPolicy ? 1 : 0)
            << " exact_shadow_selected=" << (outcome.selectedByExactShadow ? 1 : 0)
            << " exact_full_selected=" << (outcome.selectedByExactFull ? 1 : 0)
            << " grade=" << split_choice_grade_name(outcome.grade)
            << " stop=" << (outcome.stopConditionSatisfied ? 1 : 0)
            << " final=" << outcome.finalStateHash
            << " target_prepare=" << outcome.targetPrepareHash
            << " trace=" << outcome.traceHash
            << " fast_class=" << stable_hash_text(fast_split_choice_key(outcome));
        if (!outcome.exactSplitChoiceKey.empty()) {
            oss << " exact_class=" << stable_hash_text(outcome.exactSplitChoiceKey);
        }
        if (!outcome.detail.empty()) {
            oss << " detail=" << stable_hash_text(outcome.detail);
        }
        oss << '\n';
    }
    return oss.str();
}
