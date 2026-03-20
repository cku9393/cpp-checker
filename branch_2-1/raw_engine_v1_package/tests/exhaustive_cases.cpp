#include "exhaustive_cases.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#include "exact_canonicalizer.hpp"
#include "exhaustive_generator.hpp"
#include "multiclass_catalog.hpp"
#include "split_choice_oracle.hpp"
#include "stabilization_support.hpp"
#include "state_dump.hpp"

using namespace std;

namespace {

struct DedupeCollisionSample {
    ExhaustiveFamily family = ExhaustiveFamily::ALL;
    string hash;
    ExhaustiveScenario first;
    ExhaustiveScenario duplicate;
};

struct ExactAuditSample {
    string fastKey;
    string exactKey;
    ExhaustiveScenario first;
    ExhaustiveScenario second;
};

vector<ExhaustiveFamily> selected_families(ExhaustiveFamily family) {
    if (family == ExhaustiveFamily::ALL) {
        return {
            ExhaustiveFamily::SPLIT_READY,
            ExhaustiveFamily::JOIN_READY,
            ExhaustiveFamily::INTEGRATE_READY,
            ExhaustiveFamily::MIXED,
            ExhaustiveFamily::SPLIT_TIE_READY,
            ExhaustiveFamily::SPLIT_TIE_STRUCTURAL,
            ExhaustiveFamily::PLANNER_TIE_MIXED,
            ExhaustiveFamily::SPLIT_TIE_SYMMETRIC_LARGE,
            ExhaustiveFamily::PLANNER_TIE_MIXED_SYMMETRIC,
            ExhaustiveFamily::CANONICAL_COLLISION_PROBE,
            ExhaustiveFamily::SPLIT_TIE_ORGANIC_SYMMETRIC,
            ExhaustiveFamily::PLANNER_TIE_MIXED_ORGANIC,
            ExhaustiveFamily::AUTOMORPHISM_PROBE_LARGE,
        };
    }
    return {family};
}

string json_histogram_object(const unordered_map<string, size_t>& values) {
    ostringstream oss;
    oss << '{';
    bool first = true;
    for (const auto& [key, count] : values) {
        if (!first) {
            oss << ',';
        }
        first = false;
        oss << '"' << key << "\":" << count;
    }
    oss << '}';
    return oss.str();
}

void append_regression_candidate(
    const TestOptions& options,
    const string& caseName,
    const string& kind,
    const filesystem::path& dumpPath,
    const optional<filesystem::path>& reducedPath
) {
    const filesystem::path outPath = artifact_subdir(options, "logs") / "regression_candidates.txt";
    ofstream ofs(outPath, ios::app);
    if (!ofs) {
        throw runtime_error("failed to append regression candidate: " + outPath.string());
    }
    ofs << "case=" << caseName
        << " kind=" << kind
        << " dump=" << dumpPath.string();
    if (reducedPath.has_value()) {
        ofs << " reduced=" << reducedPath->string();
    }
    ofs << '\n';
}

optional<filesystem::path> maybe_reduce_pending_primitive_dump(
    const TestOptions& options,
    PrimitiveKind primitive
) {
    const optional<filesystem::path> dumpPath = pending_dump_path();
    if (!dumpPath.has_value()) {
        return nullopt;
    }
    try {
        return reduce_state_dump_file(options, *dumpPath, primitive, true);
    } catch (const exception&) {
        return nullopt;
    }
}

optional<filesystem::path> maybe_reduce_pending_planner_dump(const TestOptions& options) {
    const optional<filesystem::path> dumpPath = pending_dump_path();
    if (!dumpPath.has_value()) {
        return nullopt;
    }
    try {
        return reduce_planner_state_dump_file(options, *dumpPath, true);
    } catch (const exception&) {
        return nullopt;
    }
}

void run_primitive_plan(ExhaustiveScenario& scenario) {
    switch (scenario.primitivePlan.primitive) {
        case PrimitiveKind::NONE:
            throw runtime_error("exhaustive primitive plan missing primitive kind");
        case PrimitiveKind::ISOLATE:
            throw runtime_error("exhaustive primitive plan does not support isolate");
        case PrimitiveKind::SPLIT:
            (void)split_checked(
                scenario.RE,
                scenario.primitivePlan.sid,
                scenario.primitivePlan.aOrig,
                scenario.primitivePlan.bOrig,
                scenario.U,
                scenario.label + "_primitive_split"
            );
            return;
        case PrimitiveKind::JOIN:
            (void)join_checked(
                scenario.RE,
                scenario.primitivePlan.leftSid,
                scenario.primitivePlan.rightSid,
                scenario.primitivePlan.aOrig,
                scenario.primitivePlan.bOrig,
                scenario.U,
                scenario.label + "_primitive_join"
            );
            return;
        case PrimitiveKind::INTEGRATE:
            (void)integrate_checked(
                scenario.RE,
                scenario.primitivePlan.parentSid,
                scenario.primitivePlan.childSid,
                scenario.primitivePlan.boundaryMap,
                scenario.U,
                scenario.label + "_primitive_integrate"
            );
            return;
    }
}

void write_exhaustive_stats(
    const TestOptions& options,
    const string& caseName,
    const ExhaustiveRunStats& stats
) {
    const filesystem::path outPath = artifact_subdir(options, "logs") / (caseName + ".summary.txt");
    ofstream ofs(outPath);
    if (!ofs) {
        throw runtime_error("failed to write exhaustive summary: " + outPath.string());
    }
    ofs << "generated_state_count=" << stats.generatedStateCount << '\n';
    ofs << "canonical_unique_state_count=" << stats.canonicalUniqueStateCount << '\n';
    ofs << "dedupe_drop_count=" << stats.dedupeDropCount << '\n';
    ofs << "collision_spot_check_count=" << stats.collisionSpotCheckCount << '\n';
    ofs << "organic_duplicate_example_count=" << stats.organicDuplicateExampleCount << '\n';
    ofs << "exact_audited_state_count=" << stats.exactAuditedStateCount << '\n';
    ofs << "exact_audit_skipped_cap_count=" << stats.exactAuditSkippedCapCount << '\n';
    ofs << "exact_audit_skipped_budget_count=" << stats.exactAuditSkippedBudgetCount << '\n';
    ofs << "exact_audit_skipped_sample_count=" << stats.exactAuditSkippedSampleCount << '\n';
    ofs << "fast_unique_count=" << stats.fastUniqueCount << '\n';
    ofs << "exact_unique_count=" << stats.exactUniqueCount << '\n';
    ofs << "fast_vs_exact_disagreement_count=" << stats.fastVsExactDisagreementCount << '\n';
    ofs << "false_merge_count=" << stats.falseMergeCount << '\n';
    ofs << "false_split_count=" << stats.falseSplitCount << '\n';
    ofs << "split_choice_compare_state_count=" << stats.splitChoiceCompareStateCount << '\n';
    ofs << "compare_eligible_state_count=" << stats.compareEligibleStateCount << '\n';
    ofs << "compare_ineligible_state_count=" << stats.compareIneligibleStateCount << '\n';
    ofs << "compare_completed_state_count=" << stats.compareCompletedStateCount << '\n';
    ofs << "compare_partial_state_count=" << stats.comparePartialStateCount << '\n';
    ofs << "split_choice_same_representative_count=" << stats.splitChoiceSameRepresentativeCount << '\n';
    ofs << "split_choice_same_semantic_class_count=" << stats.splitChoiceSameSemanticClassCount << '\n';
    ofs << "split_choice_same_final_state_count=" << stats.splitChoiceSameFinalStateCount << '\n';
    ofs << "split_choice_semantic_disagreement_count=" << stats.splitChoiceSemanticDisagreementCount << '\n';
    ofs << "split_choice_exact_full_eval_count=" << stats.splitChoiceExactFullEvalCount << '\n';
    ofs << "split_choice_exact_shadow_eval_count=" << stats.splitChoiceExactShadowEvalCount << '\n';
    ofs << "split_choice_fallback_count=" << stats.splitChoiceFallbackCount << '\n';
    ofs << "split_choice_cap_hit_count=" << stats.splitChoiceCapHitCount << '\n';
    ofs << "split_choice_harmless_compare_count=" << stats.splitChoiceHarmlessCompareCount << '\n';
    ofs << "split_choice_trace_only_compare_count=" << stats.splitChoiceTraceOnlyCompareCount << '\n';
    ofs << "multiclass_cluster_count=" << stats.multiclassClusterCount << '\n';
    ofs << "harmless_multiclass_count=" << stats.harmlessMulticlassCount << '\n';
    ofs << "trace_only_multiclass_count=" << stats.traceOnlyMulticlassCount << '\n';
    ofs << "semantic_shift_multiclass_count=" << stats.semanticShiftMulticlassCount << '\n';
    ofs << "multiclass_catalog_histogram=" << json_histogram_object(stats.multiclassCatalogHistogram) << '\n';
    ofs << "compare_ineligible_reason_histogram=" << json_histogram_object(stats.compareIneligibleReasonHistogram) << '\n';
    ofs << "build_order_duplicate_count=" << stats.buildOrderDuplicateCount << '\n';
    ofs << "commit_order_duplicate_count=" << stats.commitOrderDuplicateCount << '\n';
    ofs << "hosted_occ_order_duplicate_count=" << stats.hostedOccOrderDuplicateCount << '\n';
    ofs << "relabel_duplicate_count=" << stats.relabelDuplicateCount << '\n';
    ofs << "occid_duplicate_count=" << stats.occidDuplicateCount << '\n';
    ofs << "symmetric_structure_duplicate_count=" << stats.symmetricStructureDuplicateCount << '\n';
    ofs << "mixed_duplicate_count=" << stats.mixedDuplicateCount << '\n';
    ofs << "unknown_duplicate_count=" << stats.unknownDuplicateCount << '\n';
    const double totalRatio = stats.generatedStateCount == 0U
        ? 0.0
        : 1.0 - (static_cast<double>(stats.canonicalUniqueStateCount) / static_cast<double>(stats.generatedStateCount));
    ofs << "dedupe_ratio=" << totalRatio << '\n';
    for (ExhaustiveFamily family : selected_families(options.exhaustiveFamily)) {
        const auto it = stats.familyStats.find(family);
        if (it == stats.familyStats.end()) {
            continue;
        }
        const ExhaustiveFamilyStats& familyStats = it->second;
        const double ratio = familyStats.generated == 0U
            ? 0.0
            : 1.0 - (static_cast<double>(familyStats.unique) / static_cast<double>(familyStats.generated));
        ofs << "family=" << exhaustive_family_name(family)
            << " generated=" << familyStats.generated
            << " unique=" << familyStats.unique
            << " dedupe_drops=" << familyStats.dedupeDrops
            << " dedupe_ratio=" << ratio
            << " validator_checks=" << familyStats.validatorChecks
            << " primitive_checks=" << familyStats.primitiveChecks
            << " planner_checks=" << familyStats.plannerChecks
            << " split_choice_compare_state=" << familyStats.splitChoiceCompareStateCount
            << " compare_eligible_state=" << familyStats.compareEligibleStateCount
            << " compare_ineligible_state=" << familyStats.compareIneligibleStateCount
            << " compare_completed_state=" << familyStats.compareCompletedStateCount
            << " compare_partial_state=" << familyStats.comparePartialStateCount
            << " split_choice_exact_shadow_eval=" << familyStats.splitChoiceExactShadowEvalCount
            << " split_choice_exact_full_eval=" << familyStats.splitChoiceExactFullEvalCount
            << " split_choice_fallback=" << familyStats.splitChoiceFallbackCount
            << " split_choice_same_rep=" << familyStats.splitChoiceSameRepresentativeCount
            << " split_choice_same_class=" << familyStats.splitChoiceSameSemanticClassCount
            << " split_choice_same_final=" << familyStats.splitChoiceSameFinalStateCount
            << " split_choice_semantic_disagreement=" << familyStats.splitChoiceSemanticDisagreementCount
            << " split_choice_cap_hit=" << familyStats.splitChoiceCapHitCount
            << " split_choice_queue_normalized=" << familyStats.splitChoiceQueueNormalizedCount
            << " representative_shift=" << familyStats.representativeShiftCount
            << " harmless_shift=" << familyStats.harmlessShiftCount
            << " trace_only_shift=" << familyStats.traceOnlyShiftCount
            << " semantic_shift=" << familyStats.semanticShiftCount
            << " multiclass_clusters=" << familyStats.multiclassClusterCount
            << " harmless_multiclass=" << familyStats.harmlessMulticlassCount
            << " trace_only_multiclass=" << familyStats.traceOnlyMulticlassCount
            << " semantic_shift_multiclass=" << familyStats.semanticShiftMulticlassCount
            << " compare_ineligible_reason_histogram=" << json_histogram_object(familyStats.compareIneligibleReasonHistogram)
            << '\n';
    }

    const filesystem::path dupPath = artifact_subdir(options, "logs") / (caseName + ".duplicates.txt");
    ofstream dupOfs(dupPath);
    if (!dupOfs) {
        throw runtime_error("failed to write exhaustive duplicate summary: " + dupPath.string());
    }
    for (const OrganicDuplicateExample& example : stats.organicDuplicateExamples) {
        dupOfs << "family=" << exhaustive_family_name(example.family)
               << " hash=" << example.hash
               << " first=" << example.firstLabel
               << " duplicate=" << example.duplicateLabel
               << " cause=" << example.cause
               << '\n';
    }

}

size_t collision_spot_check_budget(const TestOptions& options) {
#ifndef NDEBUG
    return options.collisionSpotCheckCount == 0U ? 4U : options.collisionSpotCheckCount;
#else
    return options.collisionSpotCheckCount;
#endif
}

size_t choose2(size_t n) {
    return n < 2U ? 0U : (n * (n - 1U)) / 2U;
}

string normalize_duplicate_label(string label) {
    for (const string& marker : {"_tf0", "_tf1", "_crf0", "_crf1", "_ccf0", "_ccf1", "_outer0", "_outer1"}) {
        const size_t pos = label.find(marker);
        if (pos != string::npos) {
            label.erase(pos, marker.size());
        }
    }
    return label;
}

string classify_duplicate_cause(
    const TestOptions& options,
    const ExhaustiveScenario& first,
    const ExhaustiveScenario& duplicate
) {
    const size_t classificationCap = max<size_t>(options.exactCanonicalCap, max<size_t>(6U, options.maxReal));
    const ExactCanonicalKey duplicateExact = compute_exact_explorer_canonical_key(duplicate, classificationCap);
    const auto matches_transform = [&](const RebuildTransform& transform) {
        if (duplicateExact.skipped) {
            return false;
        }
        const ExactCanonicalKey transformedExact =
            compute_exact_explorer_canonical_key(rebuild_exhaustive_scenario(first, transform), classificationCap);
        return !transformedExact.skipped && transformedExact.key == duplicateExact.key;
    };

    vector<string> causes;
    if (matches_transform(make_occid_renumber_transform())) {
        causes.push_back("occid");
    }
    if (matches_transform(make_relabel_transform(first))) {
        causes.push_back("relabel");
    }
    if (matches_transform(make_edge_order_transform()) || matches_transform(make_vertex_order_transform()) ||
        normalize_duplicate_label(first.label) == normalize_duplicate_label(duplicate.label)) {
        causes.push_back("build_order");
    }
    if (matches_transform(make_hosted_occ_order_transform())) {
        causes.push_back("hosted_occ_order");
    }
    {
        RebuildTransform commitOrderTransform;
        commitOrderTransform.reverseSkeletonOrder = true;
        if (matches_transform(commitOrderTransform)) {
            causes.push_back("commit_order");
        }
    }
    switch (first.family) {
        case ExhaustiveFamily::SPLIT_TIE_ORGANIC_SYMMETRIC:
        case ExhaustiveFamily::PLANNER_TIE_MIXED_ORGANIC:
        case ExhaustiveFamily::AUTOMORPHISM_PROBE_LARGE:
            if (!causes.empty()) {
                return "mixed";
            }
            return "symmetric_structure";
        case ExhaustiveFamily::SPLIT_TIE_STRUCTURAL:
        case ExhaustiveFamily::SPLIT_TIE_SYMMETRIC_LARGE:
        case ExhaustiveFamily::PLANNER_TIE_MIXED_SYMMETRIC:
        case ExhaustiveFamily::CANONICAL_COLLISION_PROBE:
            return "symmetric_structure";
        default:
            break;
    }
    if (causes.size() > 1U) {
        return "mixed";
    }
    if (!causes.empty()) {
        return causes.front();
    }
    return "unknown";
}

void record_duplicate_cause(ExhaustiveRunStats& stats, const string& cause) {
    if (cause == "build_order") {
        ++stats.buildOrderDuplicateCount;
        return;
    }
    if (cause == "commit_order") {
        ++stats.commitOrderDuplicateCount;
        return;
    }
    if (cause == "hosted_occ_order") {
        ++stats.hostedOccOrderDuplicateCount;
        return;
    }
    if (cause == "relabel") {
        ++stats.relabelDuplicateCount;
        return;
    }
    if (cause == "occid") {
        ++stats.occidDuplicateCount;
        return;
    }
    if (cause == "symmetric_structure") {
        ++stats.symmetricStructureDuplicateCount;
        return;
    }
    if (cause == "mixed") {
        ++stats.mixedDuplicateCount;
        return;
    }
    ++stats.unknownDuplicateCount;
}

void persist_exact_audit_failure(
    const TestOptions& options,
    const string& caseName,
    const string& kind,
    const ExactAuditSample& sample
) {
    const filesystem::path dumpPath = artifact_subdir(options, "counterexamples") / (caseName + "_" + kind + "_input.txt");
    PlannerStateDump dump;
    dump.engine = sample.second.RE;
    dump.caseName = caseName + "_" + kind;
    dump.targetOcc = sample.second.ctx.targetOcc;
    dump.keepOcc.assign(sample.second.ctx.keepOcc.begin(), sample.second.ctx.keepOcc.end());
    sort(dump.keepOcc.begin(), dump.keepOcc.end());
    dump.stepBudget = options.stepBudget;
    dump.tracePrefixLength = 0U;
    dump.initialQueue = sample.second.initialQueue;
    dump.traceLevel = TraceLevel::FULL;
    save_planner_state_dump(dumpPath, dump);
    set_pending_dump_path(dumpPath);

    optional<filesystem::path> reducedPath;
    try {
        reducedPath = reduce_planner_state_dump_file(options, dumpPath, false);
    } catch (const exception&) {
        reducedPath = nullopt;
    }
    append_regression_candidate(options, caseName, kind, dumpPath, reducedPath);

    const filesystem::path detailPath = artifact_subdir(options, "logs") / (caseName + "_" + kind + ".failure.txt");
    ofstream ofs(detailPath);
    if (!ofs) {
        throw runtime_error("failed to write exact audit failure summary: " + detailPath.string());
    }
    ofs << "fast_key=" << sample.fastKey << '\n';
    ofs << "exact_key=" << sample.exactKey << '\n';
    ofs << "first_label=" << sample.first.label << '\n';
    ofs << "second_label=" << sample.second.label << '\n';
}

void run_collision_spot_check(
    const TestOptions& options,
    const string& caseName,
    const DedupeCollisionSample& sample
) {
    const ExplorerStateCanonicalSignature firstSig = capture_explorer_state_signature(sample.first);
    const ExplorerStateCanonicalSignature duplicateSig = capture_explorer_state_signature(sample.duplicate);
    if (!(firstSig == duplicateSig)) {
        throw runtime_error(
            caseName + " canonical collision guard mismatch family=" + string(exhaustive_family_name(sample.family)) +
            " hash=" + sample.hash +
            "\nfirst=" + describe_explorer_state_signature(firstSig) +
            "\nduplicate=" + describe_explorer_state_signature(duplicateSig)
        );
    }

    string validationError;
    if (!validate_engine_state_soft(sample.first.RE, &validationError)) {
        throw runtime_error(caseName + " collision guard first invalid state: " + validationError);
    }
    if (!validate_engine_state_soft(sample.duplicate.RE, &validationError)) {
        throw runtime_error(caseName + " collision guard duplicate invalid state: " + validationError);
    }

    if (sample.first.primitivePlan.primitive != PrimitiveKind::NONE) {
        ExhaustiveScenario firstPrimitive = sample.first;
        ExhaustiveScenario duplicatePrimitive = sample.duplicate;
        run_primitive_plan(firstPrimitive);
        run_primitive_plan(duplicatePrimitive);

        const PrimitiveCanonicalSignature firstPrimitiveSig =
            capture_primitive_canonical_signature(firstPrimitive.primitivePlan.primitive, firstPrimitive.RE);
        const PrimitiveCanonicalSignature duplicatePrimitiveSig =
            capture_primitive_canonical_signature(duplicatePrimitive.primitivePlan.primitive, duplicatePrimitive.RE);
        if (!(firstPrimitiveSig == duplicatePrimitiveSig)) {
            throw runtime_error(
                caseName + " collision guard primitive mismatch family=" + string(exhaustive_family_name(sample.family)) +
                " hash=" + sample.hash
            );
        }
    }

    ExhaustiveScenario firstPlanner = sample.first;
    ExhaustiveScenario duplicatePlanner = sample.duplicate;
    const vector<UpdJob>* firstQueue = firstPlanner.initialQueue.empty() ? nullptr : &firstPlanner.initialQueue;
    const vector<UpdJob>* duplicateQueue = duplicatePlanner.initialQueue.empty() ? nullptr : &duplicatePlanner.initialQueue;
    (void)run_planner_checked_capture(
        firstPlanner.RE,
        firstPlanner.ctx,
        firstPlanner.U,
        planner_run_options(options),
        firstQueue,
        firstPlanner.label + "_collision_guard_first"
    );
    (void)run_planner_checked_capture(
        duplicatePlanner.RE,
        duplicatePlanner.ctx,
        duplicatePlanner.U,
        planner_run_options(options),
        duplicateQueue,
        duplicatePlanner.label + "_collision_guard_duplicate"
    );

    const PlannerFinalStateCanonicalSignature firstPlannerSig =
        capture_planner_final_state_canonical_signature(firstPlanner.RE, firstPlanner.ctx.targetOcc);
    const PlannerFinalStateCanonicalSignature duplicatePlannerSig =
        capture_planner_final_state_canonical_signature(duplicatePlanner.RE, duplicatePlanner.ctx.targetOcc);
    if (!(firstPlannerSig == duplicatePlannerSig)) {
        throw runtime_error(
            caseName + " collision guard planner mismatch family=" + string(exhaustive_family_name(sample.family)) +
            " hash=" + sample.hash
        );
    }
}

bool split_choice_compare_requested(const TestOptions& options) {
    return options.splitChoiceCompareMode == SplitChoiceCompareMode::EXACT_FULL;
}

bool should_sample_exhaustive_compare(const TestOptions& options, size_t ordinal) {
    if (!split_choice_compare_requested(options)) {
        return false;
    }
    const double sampleRate = options.compareSampleRate;
    if (sampleRate >= 1.0) {
        const size_t stride = static_cast<size_t>(sampleRate);
        return stride == 0U || ordinal % stride == 0U;
    }
    const double scaled = sampleRate * 1000.0;
    const size_t threshold = static_cast<size_t>(scaled);
    return (ordinal % 1000U) < max<size_t>(1U, threshold);
}

ExhaustiveRunStats run_exhaustive_impl(const TestOptions& options, const string& caseName) {
    TestOptions oracleOptions = options;
    oracleOptions.caseName = caseName;
    oracleOptions.oracleMode = OracleMode::ALL;
    oracleOptions.dumpOnFail = true;
    set_active_test_options(&oracleOptions);
    set_pending_dump_path(nullopt);

    ExhaustiveRunStats stats;
    unordered_set<string> seen;
    unordered_map<string, ExhaustiveScenario> firstScenarioByHash;
    unordered_map<string, ExhaustiveScenario> firstScenarioByFastAuditKey;
    unordered_map<string, ExhaustiveScenario> firstScenarioByExactAuditKey;
    unordered_map<string, string> firstExactByFastKey;
    unordered_map<string, string> firstFastByExactKey;
    unordered_map<string, unordered_map<string, size_t>> fastToExactCounts;
    unordered_map<string, unordered_map<string, size_t>> exactToFastCounts;
    unordered_set<string> multiclassClusters;
    optional<ExactAuditSample> falseMergeSample;
    optional<ExactAuditSample> falseSplitSample;
    vector<DedupeCollisionSample> collisionSamples;
    const size_t spotCheckBudget = collision_spot_check_budget(options);
    size_t classifiedDuplicateCount = 0U;

    try {
        for (ExhaustiveFamily family : selected_families(options.exhaustiveFamily)) {
            const vector<ExhaustiveScenario> scenarios = generate_exhaustive_scenarios(options, family);
            ExhaustiveFamilyStats& familyStats = stats.familyStats[family];
            for (const ExhaustiveScenario& baseScenario : scenarios) {
                ++stats.generatedStateCount;
                ++familyStats.generated;
                const ExplorerStateCanonicalSignature dedupeSig = capture_explorer_state_signature(baseScenario);
                const string dedupeHash = hash_explorer_state_signature(dedupeSig);

                if (options.exactCanonicalCap != 0U &&
                    options.exactAuditBudget != 0U &&
                    stats.exactAuditedStateCount >= options.exactAuditBudget) {
                    ++stats.exactAuditSkippedBudgetCount;
                } else if (options.exactCanonicalCap != 0U &&
                           !should_sample_exact_canonical(options, stats.generatedStateCount - 1U)) {
                    ++stats.exactAuditSkippedSampleCount;
                } else if (should_sample_exact_canonical(options, stats.generatedStateCount - 1U)) {
                    const string fastStateKey = planner_fast_canonical_state_key(baseScenario.RE);
                    const ExactCanonicalKey exactKey =
                        compute_exact_state_canonical_key(baseScenario.RE, options.exactCanonicalCap);
                    if (!exactKey.skipped) {
                        ++stats.exactAuditedStateCount;
                        ++fastToExactCounts[fastStateKey][exactKey.key];
                        ++exactToFastCounts[exactKey.key][fastStateKey];

                        const auto fastIt = firstExactByFastKey.find(fastStateKey);
                        if (fastIt == firstExactByFastKey.end()) {
                            firstExactByFastKey.emplace(fastStateKey, exactKey.key);
                            firstScenarioByFastAuditKey.emplace(fastStateKey, baseScenario);
                        } else if (fastIt->second != exactKey.key && !falseMergeSample.has_value()) {
                            falseMergeSample = ExactAuditSample{
                                fastStateKey,
                                exactKey.key,
                                firstScenarioByFastAuditKey.at(fastStateKey),
                                baseScenario,
                            };
                        }

                        const auto exactIt = firstFastByExactKey.find(exactKey.key);
                        if (exactIt == firstFastByExactKey.end()) {
                            firstFastByExactKey.emplace(exactKey.key, fastStateKey);
                            firstScenarioByExactAuditKey.emplace(exactKey.key, baseScenario);
                        } else if (exactIt->second != fastStateKey && !falseSplitSample.has_value()) {
                            falseSplitSample = ExactAuditSample{
                                fastStateKey,
                                exactKey.key,
                                firstScenarioByExactAuditKey.at(exactKey.key),
                                baseScenario,
                            };
                        }
                    } else {
                        ++stats.exactAuditSkippedCapCount;
                    }
                }

                if (options.dedupeCanonical) {
                    if (!seen.insert(dedupeHash).second) {
                        ++stats.dedupeDropCount;
                        ++familyStats.dedupeDrops;
                        const auto firstIt = firstScenarioByHash.find(dedupeHash);
                        if (firstIt != firstScenarioByHash.end()) {
                            string cause = "unknown";
                            if (classifiedDuplicateCount < 64U) {
                                cause = classify_duplicate_cause(oracleOptions, firstIt->second, baseScenario);
                                record_duplicate_cause(stats, cause);
                                ++classifiedDuplicateCount;
                            }
                            if (stats.organicDuplicateExamples.size() < 16U) {
                                stats.organicDuplicateExamples.push_back(OrganicDuplicateExample{
                                    family,
                                    dedupeHash,
                                    firstIt->second.label,
                                    baseScenario.label,
                                    cause,
                                });
                            }
                            const ExplorerStateCanonicalSignature firstSig =
                                capture_explorer_state_signature(firstIt->second);
                            if (!(firstSig == dedupeSig)) {
                                throw runtime_error(
                                    caseName + " canonical hash collision family=" + string(exhaustive_family_name(family)) +
                                    " hash=" + dedupeHash +
                                    "\nfirst=" + describe_explorer_state_signature(firstSig) +
                                    "\nduplicate=" + describe_explorer_state_signature(dedupeSig)
                                );
                            }
                            if (collisionSamples.size() < spotCheckBudget) {
                                collisionSamples.push_back(DedupeCollisionSample{
                                    family,
                                    dedupeHash,
                                    firstIt->second,
                                    baseScenario,
                                });
                            }
                        }
                        continue;
                    }
                    firstScenarioByHash.emplace(dedupeHash, baseScenario);
                }

                ++stats.canonicalUniqueStateCount;
                ++familyStats.unique;

                const bool compareRequested = split_choice_compare_requested(oracleOptions);
                optional<CompareEligibilityInfo> compareEligibility;
                if (compareRequested) {
                    compareEligibility = analyze_compare_eligibility(baseScenario, oracleOptions.maxSplitPairCandidates);
                    if (compareEligibility->compareEligible) {
                        ++stats.compareEligibleStateCount;
                        ++familyStats.compareEligibleStateCount;
                    } else {
                        ++stats.compareIneligibleStateCount;
                        ++familyStats.compareIneligibleStateCount;
                        ++stats.compareIneligibleReasonHistogram[
                            compare_eligibility_reason_name(compareEligibility->reason)
                        ];
                        ++familyStats.compareIneligibleReasonHistogram[
                            compare_eligibility_reason_name(compareEligibility->reason)
                        ];
                    }
                }
                if (compareRequested &&
                    compareEligibility.has_value() &&
                    compareEligibility->compareEligible &&
                    (oracleOptions.compareBudget == 0U ||
                     stats.splitChoiceCompareStateCount < oracleOptions.compareBudget) &&
                    should_sample_exhaustive_compare(oracleOptions, stats.canonicalUniqueStateCount - 1U)) {
                    ExhaustiveScenario compareScenario = baseScenario;
                    if (!compareScenario.initialQueue.empty()) {
                        compareScenario.initialQueue.clear();
                        ++familyStats.splitChoiceQueueNormalizedCount;
                    }
                    TestOptions compareOptions = oracleOptions;
                    compareOptions.exactCanonicalCap = max<size_t>(compareOptions.exactCanonicalCap, 8U);
                    const SplitChoiceOracleRunResult compareResult =
                        run_split_choice_oracle(compareOptions, compareScenario, nullptr);
                    if (compareResult.exactAuditAvailable) {
                        ++stats.compareCompletedStateCount;
                        ++familyStats.compareCompletedStateCount;
                        stats.splitChoiceCompareStateCount += compareResult.compareStateCount;
                        stats.splitChoiceSameRepresentativeCount += compareResult.sameRepresentativeCount;
                        stats.splitChoiceSameSemanticClassCount += compareResult.sameSemanticClassCount;
                        stats.splitChoiceSameFinalStateCount += compareResult.sameFinalStateCount;
                        stats.splitChoiceSemanticDisagreementCount += compareResult.semanticDisagreementCount;
                        stats.splitChoiceExactFullEvalCount += compareResult.exactFullEvalCount;
                        stats.splitChoiceExactShadowEvalCount += compareResult.exactShadowEvalCount;
                        stats.splitChoiceFallbackCount += compareResult.fallbackCount;
                        stats.splitChoiceCapHitCount += compareResult.capHitCount;
                        stats.splitChoiceHarmlessCompareCount += compareResult.harmlessCompareCount;
                        stats.splitChoiceTraceOnlyCompareCount += compareResult.traceOnlyCompareCount;
                        familyStats.splitChoiceCompareStateCount += compareResult.compareStateCount;
                        familyStats.splitChoiceExactFullEvalCount += compareResult.exactFullEvalCount;
                        familyStats.splitChoiceExactShadowEvalCount += compareResult.exactShadowEvalCount;
                        familyStats.splitChoiceFallbackCount += compareResult.fallbackCount;
                        familyStats.splitChoiceSameRepresentativeCount += compareResult.sameRepresentativeCount;
                        familyStats.splitChoiceSameSemanticClassCount += compareResult.sameSemanticClassCount;
                        familyStats.splitChoiceSameFinalStateCount += compareResult.sameFinalStateCount;
                        familyStats.splitChoiceSemanticDisagreementCount += compareResult.semanticDisagreementCount;
                        familyStats.splitChoiceCapHitCount += compareResult.capHitCount;
                        familyStats.representativeShiftCount += compareResult.representativeShiftCount;
                        familyStats.harmlessShiftCount += compareResult.harmlessShiftCount;
                        familyStats.traceOnlyShiftCount += compareResult.traceOnlyShiftCount;
                        familyStats.semanticShiftCount += compareResult.semanticShiftCount;
                        if (const optional<MulticlassCatalogEntry> catalogEntry =
                                build_multiclass_catalog_entry(
                                    exhaustive_family_name(family),
                                    compareScenario,
                                    compareResult
                                );
                            catalogEntry.has_value()) {
                            write_multiclass_catalog_entry(
                                compareOptions,
                                caseName,
                                compareScenario,
                                *catalogEntry,
                                compareResult
                            );
                            if (multiclassClusters.insert(catalogEntry->clusterKey).second) {
                                ++stats.multiclassClusterCount;
                                ++stats.multiclassCatalogHistogram[multiclass_catalog_category_name(catalogEntry->category)];
                                ++familyStats.multiclassClusterCount;
                                switch (catalogEntry->category) {
                                    case MulticlassCatalogCategory::NONE:
                                        break;
                                    case MulticlassCatalogCategory::HARMLESS:
                                        ++stats.harmlessMulticlassCount;
                                        ++familyStats.harmlessMulticlassCount;
                                        break;
                                    case MulticlassCatalogCategory::TRACE_ONLY:
                                        ++stats.traceOnlyMulticlassCount;
                                        ++familyStats.traceOnlyMulticlassCount;
                                        break;
                                    case MulticlassCatalogCategory::SEMANTIC_SHIFT:
                                        ++stats.semanticShiftMulticlassCount;
                                        ++familyStats.semanticShiftMulticlassCount;
                                        break;
                                }
                            }
                        }
                        if (compareResult.semanticDisagreementCount != 0U) {
                            const filesystem::path dumpPath =
                                artifact_subdir(compareOptions, "counterexamples") /
                                (caseName + "_exact_shadow_vs_exact_full_input.txt");
                            PlannerStateDump dump;
                            dump.engine = compareScenario.RE;
                            dump.caseName = caseName + "_exact_shadow_vs_exact_full";
                            dump.targetOcc = compareScenario.ctx.targetOcc;
                            dump.keepOcc.assign(compareScenario.ctx.keepOcc.begin(), compareScenario.ctx.keepOcc.end());
                            sort(dump.keepOcc.begin(), dump.keepOcc.end());
                            dump.stepBudget = compareOptions.stepBudget;
                            dump.tracePrefixLength = 0U;
                            dump.initialQueue = compareScenario.initialQueue;
                            dump.traceLevel = TraceLevel::FULL;
                            save_planner_state_dump(dumpPath, dump);
                            set_pending_dump_path(dumpPath);
                            const optional<filesystem::path> reducedPath =
                                maybe_reduce_pending_planner_dump(compareOptions);
                            append_regression_candidate(
                                compareOptions,
                                caseName,
                                "exact_shadow_vs_exact_full",
                                dumpPath,
                                reducedPath
                            );
                            throw runtime_error(caseName + " exact_shadow vs exact_full semantic disagreement");
                        }
                    } else {
                        ++stats.comparePartialStateCount;
                        ++stats.splitChoiceCapHitCount;
                        stats.splitChoiceFallbackCount += compareResult.fallbackCount;
                        ++familyStats.comparePartialStateCount;
                        ++familyStats.splitChoiceCapHitCount;
                        familyStats.splitChoiceFallbackCount += compareResult.fallbackCount;
                    }
                }

                string validationError;
                ++familyStats.validatorChecks;
                if (!validate_engine_state_soft(baseScenario.RE, &validationError)) {
                    throw runtime_error(caseName + " invalid generated state: " + validationError);
                }

                {
                    ExhaustiveScenario primitiveScenario = baseScenario;
                    try {
                        run_primitive_plan(primitiveScenario);
                    } catch (...) {
                        const optional<filesystem::path> dumpPath = pending_dump_path();
                        const optional<filesystem::path> reducedPath =
                            maybe_reduce_pending_primitive_dump(oracleOptions, primitiveScenario.primitivePlan.primitive);
                        if (dumpPath.has_value()) {
                            append_regression_candidate(
                                oracleOptions,
                                caseName,
                                "primitive",
                                *dumpPath,
                                reducedPath
                            );
                        }
                        throw;
                    }

                    ++familyStats.primitiveChecks;
                    if (!validate_engine_state_soft(primitiveScenario.RE, &validationError)) {
                        throw runtime_error(caseName + " primitive validation failed: " + validationError);
                    }
                    (void)capture_primitive_canonical_signature(primitiveScenario.primitivePlan.primitive, primitiveScenario.RE);
                }

                {
                    ExhaustiveScenario plannerScenario = baseScenario;
                    try {
                        const vector<UpdJob>* queue =
                            plannerScenario.initialQueue.empty() ? nullptr : &plannerScenario.initialQueue;
                        (void)run_planner_checked_capture(
                            plannerScenario.RE,
                            plannerScenario.ctx,
                            plannerScenario.U,
                            planner_run_options(options),
                            queue,
                            plannerScenario.label + "_planner"
                        );
                    } catch (...) {
                        const optional<filesystem::path> dumpPath = pending_dump_path();
                        const optional<filesystem::path> reducedPath =
                            maybe_reduce_pending_planner_dump(oracleOptions);
                        if (dumpPath.has_value()) {
                            append_regression_candidate(
                                oracleOptions,
                                caseName,
                                "planner",
                                *dumpPath,
                                reducedPath
                            );
                        }
                        throw;
                    }

                    ++familyStats.plannerChecks;
                    if (!validate_engine_state_soft(plannerScenario.RE, &validationError)) {
                        throw runtime_error(caseName + " planner validation failed: " + validationError);
                    }
                    (void)capture_planner_final_state_canonical_signature(
                        plannerScenario.RE,
                        plannerScenario.ctx.targetOcc
                    );
                }
            }
        }

        for (const DedupeCollisionSample& sample : collisionSamples) {
            run_collision_spot_check(oracleOptions, caseName, sample);
            ++stats.collisionSpotCheckCount;
        }

        stats.fastUniqueCount = fastToExactCounts.size();
        stats.exactUniqueCount = exactToFastCounts.size();
        for (const auto& [_, exactBuckets] : fastToExactCounts) {
            if (exactBuckets.size() > 1U) {
                ++stats.falseMergeCount;
            }
            size_t total = 0U;
            size_t preserved = 0U;
            for (const auto& [__, count] : exactBuckets) {
                total += count;
                preserved += choose2(count);
            }
            stats.fastVsExactDisagreementCount += choose2(total) - preserved;
        }
        for (const auto& [_, fastBuckets] : exactToFastCounts) {
            if (fastBuckets.size() > 1U) {
                ++stats.falseSplitCount;
            }
            size_t total = 0U;
            size_t preserved = 0U;
            for (const auto& [__, count] : fastBuckets) {
                total += count;
                preserved += choose2(count);
            }
            stats.fastVsExactDisagreementCount += choose2(total) - preserved;
        }
        stats.organicDuplicateExampleCount = stats.organicDuplicateExamples.size();

        const bool strictExactDedupeAudit = !split_choice_compare_requested(options);
        if (strictExactDedupeAudit && (stats.falseMergeCount != 0U || stats.falseSplitCount != 0U)) {
            if (falseMergeSample.has_value()) {
                persist_exact_audit_failure(oracleOptions, caseName, "exact_false_merge", *falseMergeSample);
            }
            if (falseSplitSample.has_value()) {
                persist_exact_audit_failure(oracleOptions, caseName, "exact_false_split", *falseSplitSample);
            }
            throw runtime_error(caseName + " fast/exact canonical audit disagreement");
        }
    } catch (...) {
        stats.organicDuplicateExampleCount = stats.organicDuplicateExamples.size();
        stats.fastUniqueCount = fastToExactCounts.size();
        stats.exactUniqueCount = exactToFastCounts.size();
        write_exhaustive_stats(oracleOptions, caseName, stats);
        set_active_test_options(&options);
        throw;
    }

    stats.organicDuplicateExampleCount = stats.organicDuplicateExamples.size();
    write_exhaustive_stats(oracleOptions, caseName, stats);
    set_active_test_options(&options);
    return stats;
}

TestOptions exhaustive_smoke_options(const TestOptions& options, ExhaustiveFamily family, size_t maxStates) {
    TestOptions smoke = options;
    smoke.caseName = "exhaustive";
    smoke.exhaustiveFamily = family;
    smoke.maxReal = 8;
    smoke.maxOcc = 3;
    smoke.maxEdges = 14;
    smoke.maxComponents = 5;
    smoke.maxHostedOcc = 2;
    smoke.maxStates = maxStates;
    smoke.dedupeCanonical = true;
    return smoke;
}

} // namespace

void run_exhaustive_case(const TestOptions& options) {
    (void)run_exhaustive_impl(options, "exhaustive");
}

void run_exhaustive_split_ready_smoke_case(const TestOptions& options) {
    (void)run_exhaustive_impl(exhaustive_smoke_options(options, ExhaustiveFamily::SPLIT_READY, 16), "exhaustive_split_ready_smoke");
}

void run_exhaustive_join_ready_smoke_case(const TestOptions& options) {
    (void)run_exhaustive_impl(exhaustive_smoke_options(options, ExhaustiveFamily::JOIN_READY, 16), "exhaustive_join_ready_smoke");
}

void run_exhaustive_integrate_ready_smoke_case(const TestOptions& options) {
    (void)run_exhaustive_impl(
        exhaustive_smoke_options(options, ExhaustiveFamily::INTEGRATE_READY, 24),
        "exhaustive_integrate_ready_smoke"
    );
}

void run_exhaustive_mixed_smoke_case(const TestOptions& options) {
    (void)run_exhaustive_impl(exhaustive_smoke_options(options, ExhaustiveFamily::MIXED, 24), "exhaustive_mixed_smoke");
}

void run_planner_tie_mixed_exhaustive_smoke_case(const TestOptions& options) {
    TestOptions smoke = exhaustive_smoke_options(options, ExhaustiveFamily::PLANNER_TIE_MIXED, 16);
    smoke.maxReal = 6;
    smoke.maxEdges = 9;
    smoke.maxComponents = 4;
    const ExhaustiveRunStats stats = run_exhaustive_impl(smoke, "planner_tie_mixed_exhaustive_smoke");
    const auto it = stats.familyStats.find(ExhaustiveFamily::PLANNER_TIE_MIXED);
    if (it == stats.familyStats.end() ||
        it->second.generated == 0U ||
        it->second.unique == 0U ||
        it->second.validatorChecks == 0U ||
        it->second.primitiveChecks == 0U ||
        it->second.plannerChecks == 0U) {
        throw runtime_error("planner_tie_mixed_exhaustive_smoke expected non-zero planner_tie_mixed coverage");
    }
}

void run_exhaustive_canonical_dedupe_smoke_case(const TestOptions& options) {
    TestOptions smoke = exhaustive_smoke_options(options, ExhaustiveFamily::MIXED, 96);
    vector<ExhaustiveScenario> scenarios = generate_exhaustive_scenarios(smoke, ExhaustiveFamily::MIXED);
    const size_t baseCount = min<size_t>(8U, scenarios.size());
    scenarios.resize(baseCount);
    for (size_t i = 0; i < baseCount; ++i) {
        ExhaustiveScenario duplicate = rebuild_exhaustive_scenario(scenarios[i], make_occid_renumber_transform());
        duplicate.family = ExhaustiveFamily::MIXED;
        duplicate.label += "_occid_duplicate";
        scenarios.push_back(std::move(duplicate));
    }

    unordered_set<string> seen;
    size_t dedupeDrops = 0U;
    for (const ExhaustiveScenario& scenario : scenarios) {
        const string hash = hash_explorer_state_signature(capture_explorer_state_signature(scenario));
        if (!seen.insert(hash).second) {
            ++dedupeDrops;
        }
    }

    ExhaustiveRunStats stats;
    stats.generatedStateCount = scenarios.size();
    stats.canonicalUniqueStateCount = seen.size();
    ExhaustiveFamilyStats& familyStats = stats.familyStats[ExhaustiveFamily::MIXED];
    familyStats.generated = scenarios.size();
    familyStats.unique = seen.size();
    familyStats.dedupeDrops = dedupeDrops;
    write_exhaustive_stats(smoke, "exhaustive_canonical_dedupe_smoke", stats);

    if (dedupeDrops == 0U) {
        throw runtime_error("exhaustive_canonical_dedupe_smoke expected canonical dedupe drops");
    }
}

void run_exhaustive_natural_dedupe_smoke_case(const TestOptions& options) {
    TestOptions smoke = exhaustive_smoke_options(options, ExhaustiveFamily::SPLIT_TIE_STRUCTURAL, 16);
    const ExhaustiveRunStats stats = run_exhaustive_impl(smoke, "exhaustive_natural_dedupe_smoke");
    const auto it = stats.familyStats.find(ExhaustiveFamily::SPLIT_TIE_STRUCTURAL);
    if (it == stats.familyStats.end() || it->second.dedupeDrops == 0U) {
        throw runtime_error("exhaustive_natural_dedupe_smoke expected split_tie_structural natural dedupe");
    }
}

void run_exhaustive_family_sweep_smoke_case(const TestOptions& options) {
    const vector<ExhaustiveFamily> families = {
        ExhaustiveFamily::SPLIT_READY,
        ExhaustiveFamily::JOIN_READY,
        ExhaustiveFamily::INTEGRATE_READY,
        ExhaustiveFamily::MIXED,
        ExhaustiveFamily::SPLIT_TIE_READY,
        ExhaustiveFamily::SPLIT_TIE_STRUCTURAL,
        ExhaustiveFamily::PLANNER_TIE_MIXED,
        ExhaustiveFamily::SPLIT_TIE_ORGANIC_SYMMETRIC,
        ExhaustiveFamily::AUTOMORPHISM_PROBE_LARGE,
    };
    for (ExhaustiveFamily family : families) {
        TestOptions smoke = exhaustive_smoke_options(options, family, 2);
        smoke.caseName = "exhaustive_family_sweep_smoke";
        switch (family) {
            case ExhaustiveFamily::SPLIT_TIE_SYMMETRIC_LARGE:
            case ExhaustiveFamily::PLANNER_TIE_MIXED_SYMMETRIC:
            case ExhaustiveFamily::CANONICAL_COLLISION_PROBE:
            case ExhaustiveFamily::SPLIT_TIE_ORGANIC_SYMMETRIC:
            case ExhaustiveFamily::PLANNER_TIE_MIXED_ORGANIC:
            case ExhaustiveFamily::AUTOMORPHISM_PROBE_LARGE:
                smoke.maxReal = 8;
                smoke.maxEdges = 14;
                smoke.maxComponents = 5;
                smoke.maxStates = (family == ExhaustiveFamily::PLANNER_TIE_MIXED_SYMMETRIC) ? 2U : 1U;
                break;
            default:
                smoke.maxReal = 6;
                smoke.maxEdges = 9;
                smoke.maxComponents = 3;
                break;
        }

        const string caseName = string("exhaustive_family_sweep_smoke_") + exhaustive_family_name(family);
        const ExhaustiveRunStats stats = run_exhaustive_impl(smoke, caseName);
        const auto it = stats.familyStats.find(family);
        if (it == stats.familyStats.end()) {
            throw runtime_error("exhaustive_family_sweep_smoke missing family stats");
        }
        const ExhaustiveFamilyStats& familyStats = it->second;
        if (familyStats.generated == 0U || familyStats.unique == 0U ||
            familyStats.validatorChecks == 0U || familyStats.primitiveChecks == 0U ||
            familyStats.plannerChecks == 0U) {
            throw runtime_error(
                string("exhaustive_family_sweep_smoke incomplete family coverage family=") +
                exhaustive_family_name(family)
            );
        }
    }
}

void run_exhaustive_collision_guard_smoke_case(const TestOptions& options) {
    TestOptions smoke = exhaustive_smoke_options(options, ExhaustiveFamily::SPLIT_TIE_STRUCTURAL, 16);
    smoke.collisionSpotCheckCount = 4U;
    const ExhaustiveRunStats stats = run_exhaustive_impl(smoke, "exhaustive_collision_guard_smoke");
    if (stats.dedupeDropCount == 0U || stats.collisionSpotCheckCount == 0U) {
        throw runtime_error("exhaustive_collision_guard_smoke expected sampled dedupe collision checks");
    }
}

void run_exhaustive_natural_dedupe_large_smoke_case(const TestOptions& options) {
    TestOptions smoke = exhaustive_smoke_options(options, ExhaustiveFamily::CANONICAL_COLLISION_PROBE, 4);
    smoke.collisionSpotCheckCount = 8U;
    const ExhaustiveRunStats stats = run_exhaustive_impl(smoke, "exhaustive_natural_dedupe_large_smoke");
    const auto it = stats.familyStats.find(ExhaustiveFamily::CANONICAL_COLLISION_PROBE);
    if (it == stats.familyStats.end() || it->second.dedupeDrops == 0U) {
        throw runtime_error("exhaustive_natural_dedupe_large_smoke expected canonical_collision_probe dedupe");
    }
}

void run_exhaustive_organic_duplicate_examples_smoke_case(const TestOptions& options) {
    TestOptions smoke = exhaustive_smoke_options(options, ExhaustiveFamily::AUTOMORPHISM_PROBE_LARGE, 4);
    const ExhaustiveRunStats stats = run_exhaustive_impl(smoke, "exhaustive_organic_duplicate_examples_smoke");
    if (stats.organicDuplicateExampleCount == 0U) {
        throw runtime_error("exhaustive_organic_duplicate_examples_smoke expected logged duplicate examples");
    }
}

void run_exact_canonicalizer_smoke_case(const TestOptions& options) {
    const ExhaustiveScenario base =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::CANONICAL_COLLISION_PROBE, 890501U);
    const ExhaustiveScenario relabeled = rebuild_exhaustive_scenario(base, make_relabel_transform(base));
    const ExhaustiveScenario occidRenumbered = rebuild_exhaustive_scenario(base, make_occid_renumber_transform());

    const size_t cap = max<size_t>(options.exactCanonicalCap, 8U);
    const ExactCanonicalKey stateKey = compute_exact_state_canonical_key(base.RE, cap);
    const ExactCanonicalKey relabeledStateKey = compute_exact_state_canonical_key(relabeled.RE, cap);
    const ExactCanonicalKey occidStateKey = compute_exact_state_canonical_key(occidRenumbered.RE, cap);
    if (stateKey.skipped || relabeledStateKey.skipped || occidStateKey.skipped) {
        throw runtime_error("exact_canonicalizer_smoke expected exact state canonicalization");
    }
    if (stateKey.key != relabeledStateKey.key || stateKey.key != occidStateKey.key) {
        throw runtime_error("exact_canonicalizer_smoke exact state canonicalization mismatch");
    }

    const IsolatePrepared prep = prepare_isolate_checked(
        base.RE,
        base.RE.occ.get(base.ctx.targetOcc).hostSkel,
        base.ctx.targetOcc,
        "exact_canonicalizer_smoke_prepare"
    );
    const IsolatePrepared relabeledPrep = prepare_isolate_checked(
        relabeled.RE,
        relabeled.RE.occ.get(relabeled.ctx.targetOcc).hostSkel,
        relabeled.ctx.targetOcc,
        "exact_canonicalizer_smoke_prepare_relabel"
    );
    const ExactCanonicalKey isolateKey = compute_exact_isolate_canonical_key(base.RE, prep, cap);
    const ExactCanonicalKey relabeledIsolateKey = compute_exact_isolate_canonical_key(relabeled.RE, relabeledPrep, cap);
    if (isolateKey.skipped || relabeledIsolateKey.skipped || isolateKey.key != relabeledIsolateKey.key) {
        throw runtime_error("exact_canonicalizer_smoke exact isolate canonicalization mismatch");
    }
}

void run_fast_vs_exact_canonical_dedupe_smoke_case(const TestOptions& options) {
    TestOptions smoke = exhaustive_smoke_options(options, ExhaustiveFamily::CANONICAL_COLLISION_PROBE, 2);
    smoke.exactCanonicalCap = max<size_t>(smoke.exactCanonicalCap, 8U);
    smoke.exactCanonicalSampleRate = 1U;
    const ExhaustiveRunStats stats = run_exhaustive_impl(smoke, "fast_vs_exact_canonical_dedupe_smoke");
    if (stats.exactAuditedStateCount == 0U) {
        throw runtime_error("fast_vs_exact_canonical_dedupe_smoke expected exact canonical audit coverage");
    }
    if (stats.fastVsExactDisagreementCount != 0U || stats.falseMergeCount != 0U || stats.falseSplitCount != 0U) {
        throw runtime_error("fast_vs_exact_canonical_dedupe_smoke expected fast/exact agreement");
    }
}

void run_canonical_collision_probe_smoke_case(const TestOptions& options) {
    TestOptions smoke = exhaustive_smoke_options(options, ExhaustiveFamily::CANONICAL_COLLISION_PROBE, 12);
    smoke.exactCanonicalCap = max<size_t>(smoke.exactCanonicalCap, 8U);
    smoke.exactCanonicalSampleRate = 4U;
    const ExhaustiveRunStats stats = run_exhaustive_impl(smoke, "canonical_collision_probe_smoke");
    const auto it = stats.familyStats.find(ExhaustiveFamily::CANONICAL_COLLISION_PROBE);
    if (it == stats.familyStats.end() || it->second.generated == 0U || it->second.unique == 0U) {
        throw runtime_error("canonical_collision_probe_smoke expected canonical_collision_probe coverage");
    }
    if (stats.dedupeDropCount == 0U) {
        throw runtime_error("canonical_collision_probe_smoke expected duplicate probe coverage");
    }
}

void run_split_tie_organic_symmetric_smoke_case(const TestOptions& options) {
    TestOptions auditOptions = options;
    auditOptions.oracleMode = OracleMode::ALL;
    auditOptions.exactCanonicalCap = max<size_t>(auditOptions.exactCanonicalCap, 8U);
    set_active_test_options(&auditOptions);
    const ExhaustiveScenario scenario =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_ORGANIC_SYMMETRIC, 910101U);
    const SplitChoiceOracleRunResult result = run_split_choice_oracle(auditOptions, scenario, nullptr);
    set_active_test_options(&options);
    if (result.comparedPairCount < 2U || !result.exactAuditAvailable) {
        throw runtime_error("split_tie_organic_symmetric_smoke expected exact split-choice audit coverage");
    }
}

void run_planner_tie_mixed_organic_smoke_case(const TestOptions& options) {
    TestOptions plannerOptions = options;
    plannerOptions.oracleMode = OracleMode::PLANNER;
    set_active_test_options(&plannerOptions);
    ExhaustiveScenario scenario =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::PLANNER_TIE_MIXED_ORGANIC, 910201U);
    const vector<UpdJob>* queue = scenario.initialQueue.empty() ? nullptr : &scenario.initialQueue;
    const PlannerExecutionResult result = run_planner_checked_capture(
        scenario.RE,
        scenario.ctx,
        scenario.U,
        planner_run_options(options),
        queue,
        "planner_tie_mixed_organic_smoke"
    );
    set_active_test_options(&options);
    if (result.coverage.actualJoinHits == 0U || result.coverage.actualIntegrateHits == 0U) {
        throw runtime_error("planner_tie_mixed_organic_smoke expected join/integrate follow-up coverage");
    }
}

void run_planner_tie_mixed_organic_compare_ready_smoke_case(const TestOptions& options) {
    TestOptions auditOptions = options;
    auditOptions.oracleMode = OracleMode::ALL;
    auditOptions.exactCanonicalCap = max<size_t>(auditOptions.exactCanonicalCap, 8U);
    auditOptions.splitChoicePolicyMode = SplitChoicePolicyMode::EXACT_SHADOW;
    auditOptions.splitChoiceCompareMode = SplitChoiceCompareMode::EXACT_FULL;
    set_active_test_options(&auditOptions);

    const ExhaustiveScenario mixedScenario =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::PLANNER_TIE_MIXED_ORGANIC, 910201U);
    const CompareEligibilityInfo mixedInfo =
        analyze_compare_eligibility(mixedScenario, auditOptions.maxSplitPairCandidates);

    const ExhaustiveScenario compareReadyScenario = make_targeted_planner_exhaustive_scenario(
        ScenarioFamily::PLANNER_TIE_MIXED_ORGANIC_COMPARE_READY,
        950001U
    );
    const CompareEligibilityInfo compareReadyInfo =
        analyze_compare_eligibility(compareReadyScenario, auditOptions.maxSplitPairCandidates);
    const SplitChoiceOracleRunResult result = run_split_choice_oracle(auditOptions, compareReadyScenario, nullptr);
    set_active_test_options(&options);

    if (mixedInfo.compareEligible || (mixedInfo.hasSplitReady && mixedInfo.hasTie)) {
        throw runtime_error(
            "planner_tie_mixed_organic_compare_ready_smoke expected original mixed family to remain compare-ineligible"
        );
    }
    if (!compareReadyInfo.hasSplitReady || !compareReadyInfo.hasTie || !compareReadyInfo.compareEligible) {
        throw runtime_error(
            "planner_tie_mixed_organic_compare_ready_smoke expected compare-ready family to produce exact-compare tie states"
        );
    }
    if (!result.exactAuditAvailable || result.comparedPairCount < 2U) {
        throw runtime_error("planner_tie_mixed_organic_compare_ready_smoke expected exact compare coverage");
    }
    if (result.semanticDisagreementCount != 0U || result.fallbackCount != 0U) {
        throw runtime_error(
            "planner_tie_mixed_organic_compare_ready_smoke expected exact_shadow vs exact_full agreement"
        );
    }
}

void run_automorphism_probe_large_smoke_case(const TestOptions& options) {
    TestOptions auditOptions = options;
    auditOptions.oracleMode = OracleMode::ALL;
    auditOptions.exactCanonicalCap = max<size_t>(auditOptions.exactCanonicalCap, 8U);
    set_active_test_options(&auditOptions);
    const ExhaustiveScenario scenario =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::AUTOMORPHISM_PROBE_LARGE, 910301U);
    const SplitChoiceOracleRunResult result = run_split_choice_oracle(auditOptions, scenario, nullptr);
    set_active_test_options(&options);
    if (!result.exactAuditAvailable || result.fastClassCount == 0U) {
        throw runtime_error("automorphism_probe_large_smoke expected exact automorphism audit coverage");
    }
}

void run_sampled_exact_audit_smoke_case(const TestOptions& options) {
    TestOptions smoke = options;
    smoke.exactCanonicalCap = max<size_t>(smoke.exactCanonicalCap, 8U);
    smoke.exactCanonicalSampleRate = 2U;
    smoke.exactAuditBudget = 2U;
    size_t audited = 0U;
    size_t sampleSkipped = 0U;
    for (size_t i = 0; i < 4U; ++i) {
        const ExhaustiveScenario scenario = make_targeted_planner_exhaustive_scenario(
            ScenarioFamily::AUTOMORPHISM_PROBE_LARGE,
            910301U + static_cast<u32>(i)
        );
        if (smoke.exactAuditBudget != 0U && audited >= smoke.exactAuditBudget) {
            break;
        }
        if (!should_sample_exact_canonical(smoke, i)) {
            ++sampleSkipped;
            continue;
        }
        const ExactCanonicalKey key = compute_exact_explorer_canonical_key(scenario, smoke.exactCanonicalCap);
        if (!key.skipped) {
            ++audited;
        }
    }
    if (audited == 0U || sampleSkipped == 0U) {
        throw runtime_error("sampled_exact_audit_smoke expected sampled exact audit coverage and skips");
    }
}

void run_duplicate_attribution_smoke_case(const TestOptions& options) {
    TestOptions smoke = options;
    smoke.exactCanonicalCap = max<size_t>(smoke.exactCanonicalCap, 8U);
    const ExhaustiveScenario base =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::AUTOMORPHISM_PROBE_LARGE, 910301U);
    const ExhaustiveScenario duplicate =
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::AUTOMORPHISM_PROBE_LARGE, 910302U);

    const string baseHash = hash_explorer_state_signature(capture_explorer_state_signature(base));
    const string duplicateHash = hash_explorer_state_signature(capture_explorer_state_signature(duplicate));
    if (baseHash != duplicateHash) {
        throw runtime_error("duplicate_attribution_smoke expected canonical duplicate pair");
    }

    const string cause = classify_duplicate_cause(smoke, base, duplicate);
    if (cause != "mixed" && cause != "symmetric_structure") {
        throw runtime_error("duplicate_attribution_smoke expected structural duplicate attribution, saw " + cause);
    }
}
