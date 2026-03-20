#include "test_harness.hpp"

#include <array>
#include <cctype>
#include <csignal>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "exhaustive_cases.hpp"
#include "exhaustive_generator.hpp"
#include "multiclass_catalog.hpp"
#include "compare_ready_lineage.hpp"
#include "family_applicability_audit.hpp"
#include "split_choice_oracle.hpp"
#include "state_dump.hpp"
#include "stabilization_support.hpp"

using namespace std;

namespace {

struct FailureContext {
    string caseName = "none";
    optional<u32> seed;
    int iter = -1;
};

struct NormalizedPrep {
    Vertex orig = NIL_U32;
    vector<OccID> allocNbr;
    vector<tuple<int, Vertex, BridgeRef, u8>> ports;
    vector<pair<Vertex, Vertex>> core;

    bool operator==(const NormalizedPrep& rhs) const {
        return orig == rhs.orig && allocNbr == rhs.allocNbr && ports == rhs.ports && core == rhs.core;
    }
};

struct OccPatchSignature {
    vector<OccID> allocNbr;
    vector<pair<Vertex, Vertex>> core;

    bool operator==(const OccPatchSignature& rhs) const {
        return allocNbr == rhs.allocNbr && core == rhs.core;
    }
};

struct GeneratedCase {
    RawSkelID sid = NIL_U32;
    vector<OccID> occs;
    bool hasDirectAB = false;
    unordered_map<OccID, NormalizedPrep> expected;
};

struct SplitFixture {
    RawEngine RE;
    RawUpdateCtx U;
    RawSkelID sid = NIL_U32;
    OccID occ31 = 0;
    OccID occ44 = 0;
};

enum class FuzzMode : u8 {
    ISOLATE_ONLY = 0,
    SPLIT_ONLY = 1,
    JOIN_ONLY = 2,
    INTEGRATE_ONLY = 3,
    ISOLATE_THEN_SPLIT = 4,
    SPLIT_THEN_JOIN = 5,
    SPLIT_THEN_INTEGRATE = 6,
    MIXED_PLANNER = 7,
};

struct FuzzSpec {
    FuzzMode mode = FuzzMode::MIXED_PLANNER;
    u32 seed = 0;
    int branchCount = 3;
    int occCount = 3;
    int boundaryOnlyCount = 1;
    int maxPathLen = 2;
    int maxOccPerBranch = 2;
    int directABCount = 1;
    int multiEdgeCount = 1;
    int sharedOrigPairs = 0;
    int keepOccCount = 1;
    int opCount = 1;
    int biasSplit = 0;
    int biasJoin = 0;
    int biasIntegrate = 0;
    size_t stepBudget = 200000;
};

struct PreconditionBiasConfig {
    int split = 0;
    int join = 0;
    int integrate = 0;
};

struct SeedFuzzSummary {
    u32 seed = 0;
    size_t iterations = 0;
    array<size_t, 4> primitiveHits{{0, 0, 0, 0}};
    size_t splitSuccessCount = 0;
    size_t splitRejectedCount = 0;
    size_t directABArtifactCount = 0;
    size_t boundaryOnlyChildCount = 0;
    size_t splitReadyCount = 0;
    size_t joinCandidateCount = 0;
    size_t integrateCandidateCount = 0;
    size_t actualSplitHits = 0;
    size_t actualJoinHits = 0;
    size_t actualIntegrateHits = 0;
    size_t splitChoiceCandidateCount = 0;
    size_t splitChoiceEvalCount = 0;
    size_t splitChoiceTieCount = 0;
    size_t splitChoiceMulticlassCount = 0;
    size_t splitChoiceFallbackCount = 0;
    size_t exactAuditedStateCount = 0;
    size_t exactAuditedPairCount = 0;
    size_t fastVsExactClassDisagreementCount = 0;
    size_t splitChoiceCompareStateCount = 0;
    size_t splitReadyStateCount = 0;
    size_t tieReadyStateCount = 0;
    size_t compareEligibleStateCount = 0;
    size_t compareIneligibleStateCount = 0;
    size_t compareCompletedStateCount = 0;
    size_t comparePartialStateCount = 0;
    size_t splitChoiceExactFullEvalCount = 0;
    size_t splitChoiceExactShadowEvalCount = 0;
    size_t splitChoiceSameRepresentativeCount = 0;
    size_t splitChoiceSameSemanticClassCount = 0;
    size_t splitChoiceSameFinalStateCount = 0;
    size_t splitChoiceSemanticDisagreementCount = 0;
    size_t splitChoiceCapHitCount = 0;
    size_t splitChoiceHarmlessCompareCount = 0;
    size_t splitChoiceTraceOnlyCompareCount = 0;
    size_t representativeShiftCount = 0;
    size_t representativeShiftSameClassCount = 0;
    size_t representativeShiftSemanticDivergenceCount = 0;
    size_t representativeShiftFollowupDivergenceCount = 0;
    size_t representativeShiftTraceDivergenceCount = 0;
    size_t harmlessShiftCount = 0;
    size_t traceOnlyShiftCount = 0;
    size_t semanticShiftCount = 0;
    size_t compareCandidateEnumerationNanos = 0;
    size_t compareExactShadowEvaluationNanos = 0;
    size_t compareExactFullEvaluationNanos = 0;
    size_t compareCanonicalizationNanos = 0;
    size_t compareMulticlassCatalogNanos = 0;
    size_t compareStateHashCacheHitCount = 0;
    size_t compareStateHashCacheMissCount = 0;
    size_t compareCandidateEvaluationCacheHitCount = 0;
    size_t compareCandidateEvaluationCacheMissCount = 0;
    size_t compareExactCanonicalCacheHitCount = 0;
    size_t compareExactCanonicalCacheMissCount = 0;
    size_t exactAuditSkippedCapCount = 0;
    size_t exactAuditSkippedBudgetCount = 0;
    size_t exactAuditSkippedSampleCount = 0;
    size_t exactAuditSkippedFamilyCount = 0;
    size_t exactAuditSkippedNonTieCount = 0;
    int firstSplitIter = -1;
    int firstJoinIter = -1;
    int firstIntegrateIter = -1;
    int firstSplitChoiceTieIter = -1;
    size_t multiEdgeCount = 0;
    size_t reducerInvocationCount = 0;
    map<size_t, size_t> splitChoiceEquivClassCountHistogram;
    unordered_map<string, size_t> compareIneligibleReasonHistogram;
    unordered_map<string, size_t> tracePrefixHistogram;
    unordered_map<string, size_t> primitiveMultisetHistogram;
};

struct FuzzStats {
    string caseName;
    string weightProfile;
    string preconditionBiasProfile;
    string scenarioFamily;
    string splitChoicePolicyMode;
    string splitChoiceCompareMode;
    int biasSplit = 0;
    int biasJoin = 0;
    int biasIntegrate = 0;
    array<size_t, 4> primitiveHits{{0, 0, 0, 0}};
    size_t totalIterations = 0;
    size_t splitSuccessCount = 0;
    size_t splitRejectedCount = 0;
    size_t directABArtifactCount = 0;
    size_t boundaryOnlyChildCount = 0;
    size_t splitReadyCount = 0;
    size_t joinCandidateCount = 0;
    size_t integrateCandidateCount = 0;
    size_t actualSplitHits = 0;
    size_t actualJoinHits = 0;
    size_t actualIntegrateHits = 0;
    size_t splitChoiceCandidateCount = 0;
    size_t splitChoiceEvalCount = 0;
    size_t splitChoiceTieCount = 0;
    size_t splitChoiceMulticlassCount = 0;
    size_t splitChoiceFallbackCount = 0;
    size_t exactAuditedStateCount = 0;
    size_t exactAuditedPairCount = 0;
    size_t fastVsExactClassDisagreementCount = 0;
    size_t splitChoiceCompareStateCount = 0;
    size_t splitReadyStateCount = 0;
    size_t tieReadyStateCount = 0;
    size_t compareEligibleStateCount = 0;
    size_t compareIneligibleStateCount = 0;
    size_t compareCompletedStateCount = 0;
    size_t comparePartialStateCount = 0;
    size_t splitChoiceExactFullEvalCount = 0;
    size_t splitChoiceExactShadowEvalCount = 0;
    size_t splitChoiceSameRepresentativeCount = 0;
    size_t splitChoiceSameSemanticClassCount = 0;
    size_t splitChoiceSameFinalStateCount = 0;
    size_t splitChoiceSemanticDisagreementCount = 0;
    size_t splitChoiceCapHitCount = 0;
    size_t splitChoiceHarmlessCompareCount = 0;
    size_t splitChoiceTraceOnlyCompareCount = 0;
    size_t representativeShiftCount = 0;
    size_t representativeShiftSameClassCount = 0;
    size_t representativeShiftSemanticDivergenceCount = 0;
    size_t representativeShiftFollowupDivergenceCount = 0;
    size_t representativeShiftTraceDivergenceCount = 0;
    size_t harmlessShiftCount = 0;
    size_t traceOnlyShiftCount = 0;
    size_t semanticShiftCount = 0;
    size_t compareCandidateEnumerationNanos = 0;
    size_t compareExactShadowEvaluationNanos = 0;
    size_t compareExactFullEvaluationNanos = 0;
    size_t compareCanonicalizationNanos = 0;
    size_t compareMulticlassCatalogNanos = 0;
    size_t compareStateHashCacheHitCount = 0;
    size_t compareStateHashCacheMissCount = 0;
    size_t compareCandidateEvaluationCacheHitCount = 0;
    size_t compareCandidateEvaluationCacheMissCount = 0;
    size_t compareExactCanonicalCacheHitCount = 0;
    size_t compareExactCanonicalCacheMissCount = 0;
    size_t exactAuditSkippedCapCount = 0;
    size_t exactAuditSkippedBudgetCount = 0;
    size_t exactAuditSkippedSampleCount = 0;
    size_t exactAuditSkippedFamilyCount = 0;
    size_t exactAuditSkippedNonTieCount = 0;
    int firstSplitIter = -1;
    int firstJoinIter = -1;
    int firstIntegrateIter = -1;
    int firstSplitChoiceTieIter = -1;
    size_t multiEdgeCount = 0;
    size_t reducerInvocationCount = 0;
    size_t multiclassCatalogClusterCount = 0;
    size_t multiclassHarmlessClusterCount = 0;
    size_t multiclassTraceOnlyClusterCount = 0;
    size_t multiclassSemanticShiftClusterCount = 0;
    map<size_t, size_t> splitChoiceEquivClassCountHistogram;
    unordered_map<string, size_t> compareIneligibleReasonHistogram;
    unordered_map<string, size_t> sequenceHistogram;
    unordered_map<string, size_t> tracePrefixHistogram;
    unordered_map<string, size_t> primitiveMultisetHistogram;
    unordered_map<string, size_t> oracleMismatchCount;
    unordered_map<string, size_t> multiclassCatalogHistogram;
    vector<SeedFuzzSummary> seedSummaries;
};

void write_seed_summary_line(ostream& ofs, const SeedFuzzSummary& seedSummary);
void write_fuzz_summary_body(ostream& ofs, const FuzzStats& stats);

struct CorpusEntry {
    string category;
    string modeKey;
    u32 seed = 0;
    string weightProfile;
    string preconditionBiasProfile;
    string scenarioFamily;
    int biasSplit = 0;
    int biasJoin = 0;
    int biasIntegrate = 0;
    int iters = 0;
    size_t stepBudget = 0U;
    size_t actualSplitHits = 0;
    size_t actualJoinHits = 0;
    size_t actualIntegrateHits = 0;
    size_t splitReadyCount = 0;
    size_t joinCandidateCount = 0;
    size_t integrateCandidateCount = 0;
    size_t uniqueTracePrefixCount = 0;
    size_t uniquePrimitiveMultisetCount = 0;
};

struct CampaignRunConfig {
    string caseName = "planner_oracle_fuzz";
    string mode;
    vector<u32> seeds;
    int iters = 0;
    size_t stepBudget = 0U;
    string artifactDir;
    string statsFile;
    optional<size_t> exactCanonicalCap;
    optional<double> compareSampleRate;
    optional<size_t> compareBudget;
    optional<SplitChoicePolicyMode> splitChoicePolicyMode;
    optional<SplitChoiceCompareMode> splitChoiceCompareMode;
};

struct CampaignConfig {
    vector<CampaignRunConfig> runs;
    string aggregateStatsFile;
    string aggregateSummaryFile;
    optional<string> saveCorpusDir;
    optional<string> loadCorpusDir;
    CorpusPolicy corpusPolicy = CorpusPolicy::BEST;
};

struct CampaignCheckpointChunk {
    size_t runIndex = 0U;
    size_t seedIndex = 0U;
    u32 seed = 0U;
    int iterStart = 0;
    int iterCount = 0;
    size_t totalIterations = 0U;
    size_t compareStateCount = 0U;
    size_t semanticDisagreementCount = 0U;
    size_t fallbackCount = 0U;
    string caseName;
    string mode;
    string artifactDir;
    string statsFile;
    string summaryFile;
};

struct CampaignCheckpointManifest {
    filesystem::path checkpointDir;
    filesystem::path campaignConfigSnapshot;
    SplitChoicePolicyMode splitChoicePolicyMode = SplitChoicePolicyMode::EXACT_SHADOW;
    SplitChoiceCompareMode splitChoiceCompareMode = SplitChoiceCompareMode::NONE;
    double compareSampleRate = 1.0;
    size_t compareBudget = 0U;
    size_t exactCanonicalCap = 0U;
    size_t targetComparedStates = 0U;
    size_t targetEligibleStates = 0U;
    size_t targetLineageSamples = 0U;
    size_t maxPartialRuns = 0U;
    bool stopWhenGatePasses = false;
    double targetApplicabilityConfidence = 0.0;
    optional<string> saveCorpusDir;
    optional<string> loadCorpusDir;
    CorpusPolicy corpusPolicy = CorpusPolicy::BEST;
    size_t checkpointEvery = 0U;
};

enum class PolicyGraduationGateStatus : u8 {
    PASS = 0,
    PROXY_PASS = 1,
    NON_APPLICABLE = 2,
    FAIL = 3,
};

struct PolicyGraduationThreshold {
    string family;
    size_t comparedStates = 0U;
    size_t eligibleStates = 0U;
    size_t completedStates = 0U;
    size_t lineageSamples = 0U;
    size_t applicabilityStates = 0U;
    double nonApplicableMaxCompareRelevance = 0.0;
    double nonApplicableMaxSplitReadyRelevance = 0.0;
    double nonApplicableMinReasonConfidence = 0.0;
    string proxyFamily;
};

struct PolicyGraduationEvaluation {
    PolicyGraduationGateStatus status = PolicyGraduationGateStatus::FAIL;
    PolicyGraduationThreshold threshold;
    size_t effectiveComparedTarget = 0U;
    size_t effectiveEligibleTarget = 0U;
    size_t effectiveCompletedTarget = 0U;
    size_t effectiveLineageTarget = 0U;
    size_t effectiveApplicabilityTarget = 0U;
    double effectiveApplicabilityConfidence = 0.0;
    string rationale;
};

enum class CampaignStopReason : u8 {
    COMPLETED = 0,
    GATE_PASSED = 1,
    STOP_AFTER_CHECKPOINT = 2,
    MAX_WALL_SECONDS = 3,
    MAX_PARTIAL_RUNS = 4,
};

struct IterationOrdinal {
    size_t global = 0;
    size_t seed = 0;
};

enum class ExactAuditSkipReason : u8 {
    NONE = 0,
    SAMPLE = 1,
    BUDGET = 2,
    FAMILY = 3,
    NON_TIE = 4,
    CAP = 5,
};

struct FaultDetectionRow {
    string faultName;
    bool validatorDetected = false;
    bool primitiveOracleDetected = false;
    bool plannerOracleDetected = false;
    string detail;
};

FailureContext g_failureContext;
optional<FuzzStats> g_fuzzStats;
optional<size_t> g_activeSeedSummary;
constexpr const char* kCampaignCheckpointManifestVersion = "phase17";

[[noreturn]] void fail_test(const string& message) {
    throw runtime_error(message);
}

void require_test(bool condition, const string& message) {
    if (!condition) {
        fail_test(message);
    }
}

string format_occ_patch_signature(const OccPatchSignature& sig) {
    ostringstream oss;
    oss << "alloc=[";
    for (OccID occ : sig.allocNbr) {
        oss << occ << ';';
    }
    oss << "] core=[";
    for (const auto& [a, b] : sig.core) {
        oss << a << '-' << b << ';';
    }
    oss << ']';
    return oss.str();
}

string format_normalized_prep(const NormalizedPrep& prep) {
    ostringstream oss;
    oss << "orig=" << prep.orig << " alloc=[";
    for (OccID occ : prep.allocNbr) {
        oss << occ << ';';
    }
    oss << "] ports=[";
    for (const auto& [kind, attachOrig, br, side] : prep.ports) {
        oss << kind << ':' << attachOrig << ':' << br << ':' << static_cast<int>(side) << ';';
    }
    oss << "] core=[";
    for (const auto& [a, b] : prep.core) {
        oss << a << '-' << b << ';';
    }
    oss << ']';
    return oss.str();
}

void maybe_save_corpus_from_stats(const TestOptions& options, const string& caseName, const FuzzStats& stats);
filesystem::path resolve_corpus_dir_for_read(const TestOptions& options);
void run_micro_suite(const TestOptions& options);
void run_regression_44001(const TestOptions& options);
void run_regression_isolate_split_no_sep(const TestOptions& options);
void run_artifact_retention_smoke_case(const TestOptions& options);
void write_compare_profile_summary(const filesystem::path& jsonPath, const FuzzStats& stats);

string sanitize_stem_token(const string& text) {
    string out;
    out.reserve(text.size());
    for (char ch : text) {
        const unsigned char uch = static_cast<unsigned char>(ch);
        if (isalnum(uch) || ch == '_' || ch == '-') {
            out.push_back(static_cast<char>(ch));
        } else {
            out.push_back('_');
        }
    }
    if (out.empty()) {
        out = "case";
    }
    return out;
}

string make_failure_stem(const FailureContext& ctx) {
    ostringstream oss;
    oss << sanitize_stem_token(ctx.caseName);
    if (ctx.seed.has_value()) {
        oss << "_seed" << *ctx.seed;
    }
    if (ctx.iter >= 0) {
        oss << "_iter" << ctx.iter;
    }
    return oss.str();
}

void print_failure_context() {
    cerr << "failure_context case=" << g_failureContext.caseName;
    if (g_failureContext.seed.has_value()) {
        cerr << " seed=" << *g_failureContext.seed;
    }
    if (g_failureContext.iter >= 0) {
        cerr << " iter=" << g_failureContext.iter;
    }
    if (const optional<filesystem::path> dumpPath = pending_dump_path(); dumpPath.has_value()) {
        cerr << " dump=" << dumpPath->string();
    }
    cerr << '\n';
}

[[noreturn]] void signal_fail_exit(int signum) {
    cerr << "raw_engine_tests fatal signal=" << signum << '\n';
    print_failure_context();
    std::_Exit(128 + signum);
}

void signal_handler(int signum) {
    signal_fail_exit(signum);
}

void raw_engine_terminate_handler() {
    cerr << "raw_engine_tests terminate called\n";
    print_failure_context();
    std::_Exit(1);
}

void set_failure_context(const string& caseName, optional<u32> seed, int iter) {
    g_failureContext.caseName = caseName;
    g_failureContext.seed = seed;
    g_failureContext.iter = iter;
}

void maybe_log(const TestOptions& options, const string& caseName, optional<u32> seed, int iter) {
    if (!options.verbose) {
        return;
    }
    cerr << "[run] case=" << caseName;
    if (seed.has_value()) {
        cerr << " seed=" << *seed;
    }
    if (iter >= 0) {
        cerr << " iter=" << iter;
    }
    cerr << '\n';
}

string shell_quote(const string& in) {
#ifdef _WIN32
    string out = "\"";
    for (char ch : in) {
        if (ch == '"') {
            out += "\\\"";
        } else {
            out += ch;
        }
    }
    out += '"';
    return out;
#else
    string out = "'";
    for (char ch : in) {
        if (ch == '\'') {
            out += "'\\''";
        } else {
            out += ch;
        }
    }
    out += '\'';
    return out;
#endif
}

string slurp_text_file(const filesystem::path& path) {
    ifstream ifs(path);
    if (!ifs) {
        return {};
    }
    ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

const char* fuzz_mode_name(FuzzMode mode) {
    switch (mode) {
        case FuzzMode::ISOLATE_ONLY:
            return "isolate_only";
        case FuzzMode::SPLIT_ONLY:
            return "split_only";
        case FuzzMode::JOIN_ONLY:
            return "join_only";
        case FuzzMode::INTEGRATE_ONLY:
            return "integrate_only";
        case FuzzMode::ISOLATE_THEN_SPLIT:
            return "isolate_split";
        case FuzzMode::SPLIT_THEN_JOIN:
            return "split_join";
        case FuzzMode::SPLIT_THEN_INTEGRATE:
            return "split_integrate";
        case FuzzMode::MIXED_PLANNER:
            return "mixed_planner";
    }
    return "unknown";
}

size_t planner_primitive_index(PlannerPrimitiveKind primitive) {
    switch (primitive) {
        case PlannerPrimitiveKind::ISOLATE_VERTEX:
            return 0U;
        case PlannerPrimitiveKind::SPLIT:
            return 1U;
        case PlannerPrimitiveKind::JOIN:
            return 2U;
        case PlannerPrimitiveKind::INTEGRATE:
            return 3U;
    }
    return 0U;
}

int clamp_bias_value(int value) {
    return max(0, min(value, 8));
}

PreconditionBiasConfig bias_profile_defaults(PreconditionBiasProfile profile) {
    switch (profile) {
        case PreconditionBiasProfile::DEFAULT:
            break;
        case PreconditionBiasProfile::BALANCED:
            return {2, 2, 2};
        case PreconditionBiasProfile::SPLIT_HEAVY:
            return {5, 2, 3};
        case PreconditionBiasProfile::JOIN_HEAVY:
            return {3, 5, 2};
        case PreconditionBiasProfile::INTEGRATE_HEAVY:
            return {3, 2, 5};
        case PreconditionBiasProfile::ARTIFACT_HEAVY:
            return {3, 1, 6};
        case PreconditionBiasProfile::STRUCTURAL:
            return {4, 4, 4};
    }
    return {0, 0, 0};
}

PreconditionBiasConfig weight_profile_bias_defaults(WeightProfile profile) {
    switch (profile) {
        case WeightProfile::RANDOM:
            return bias_profile_defaults(PreconditionBiasProfile::BALANCED);
        case WeightProfile::WEIGHTED_SPLIT_HEAVY:
            return bias_profile_defaults(PreconditionBiasProfile::SPLIT_HEAVY);
        case WeightProfile::WEIGHTED_JOIN_HEAVY:
            return bias_profile_defaults(PreconditionBiasProfile::JOIN_HEAVY);
        case WeightProfile::WEIGHTED_INTEGRATE_HEAVY:
            return bias_profile_defaults(PreconditionBiasProfile::INTEGRATE_HEAVY);
        case WeightProfile::ARTIFACT_HEAVY:
            return bias_profile_defaults(PreconditionBiasProfile::ARTIFACT_HEAVY);
        case WeightProfile::MULTIEDGE_HEAVY:
            return bias_profile_defaults(PreconditionBiasProfile::BALANCED);
    }
    return {0, 0, 0};
}

PreconditionBiasConfig resolve_precondition_bias(const TestOptions& options) {
    PreconditionBiasConfig bias = weight_profile_bias_defaults(options.weightProfile);
    if (options.preconditionBiasProfile != PreconditionBiasProfile::DEFAULT) {
        bias = bias_profile_defaults(options.preconditionBiasProfile);
    }
    if (options.biasSplit >= 0) {
        bias.split = options.biasSplit;
    }
    if (options.biasJoin >= 0) {
        bias.join = options.biasJoin;
    }
    if (options.biasIntegrate >= 0) {
        bias.integrate = options.biasIntegrate;
    }
    bias.split = clamp_bias_value(bias.split);
    bias.join = clamp_bias_value(bias.join);
    bias.integrate = clamp_bias_value(bias.integrate);
    return bias;
}

void begin_fuzz_stats_case(const TestOptions& options, const string& caseName) {
    if (!options.stats) {
        g_fuzzStats.reset();
        g_activeSeedSummary.reset();
        return;
    }

    FuzzStats stats;
    stats.caseName = caseName;
    stats.weightProfile = weight_profile_name_string(options.weightProfile);
    stats.preconditionBiasProfile = precondition_bias_profile_name_string(options.preconditionBiasProfile);
    stats.scenarioFamily = scenario_family_name_string(options.scenarioFamily);
    stats.splitChoicePolicyMode = split_choice_policy_mode_name_string(options.splitChoicePolicyMode);
    stats.splitChoiceCompareMode = split_choice_compare_mode_name_string(options.splitChoiceCompareMode);
    const PreconditionBiasConfig bias = resolve_precondition_bias(options);
    stats.biasSplit = bias.split;
    stats.biasJoin = bias.join;
    stats.biasIntegrate = bias.integrate;
    g_fuzzStats = stats;
    g_activeSeedSummary.reset();
    reset_multiclass_catalog();
}

void begin_fuzz_seed_summary(u32 seed) {
    if (!g_fuzzStats.has_value()) {
        return;
    }
    g_fuzzStats->seedSummaries.push_back(SeedFuzzSummary{});
    g_fuzzStats->seedSummaries.back().seed = seed;
    g_activeSeedSummary = g_fuzzStats->seedSummaries.size() - 1U;
}

void end_fuzz_seed_summary() {
    g_activeSeedSummary.reset();
}

void sync_multiclass_catalog_stats() {
    if (!g_fuzzStats.has_value()) {
        return;
    }
    const MulticlassCatalogStats& catalogStats = multiclass_catalog_stats();
    g_fuzzStats->multiclassCatalogClusterCount = catalogStats.clusterCount;
    g_fuzzStats->multiclassHarmlessClusterCount = catalogStats.harmlessClusterCount;
    g_fuzzStats->multiclassTraceOnlyClusterCount = catalogStats.traceOnlyClusterCount;
    g_fuzzStats->multiclassSemanticShiftClusterCount = catalogStats.semanticShiftClusterCount;
    g_fuzzStats->multiclassCatalogHistogram = catalogStats.familyCategoryHistogram;
}

SeedFuzzSummary* active_seed_summary() {
    if (!g_fuzzStats.has_value() || !g_activeSeedSummary.has_value()) {
        return nullptr;
    }
    return &g_fuzzStats->seedSummaries[*g_activeSeedSummary];
}

IterationOrdinal begin_fuzz_iteration_stats() {
    IterationOrdinal ordinal;
    if (!g_fuzzStats.has_value()) {
        return ordinal;
    }

    ordinal.global = g_fuzzStats->totalIterations;
    ++g_fuzzStats->totalIterations;
    if (SeedFuzzSummary* seedSummary = active_seed_summary(); seedSummary != nullptr) {
        ordinal.seed = seedSummary->iterations;
        ++seedSummary->iterations;
    }
    return ordinal;
}

IterationOrdinal current_fuzz_iteration_ordinal() {
    IterationOrdinal ordinal;
    if (!g_fuzzStats.has_value() || g_fuzzStats->totalIterations == 0U) {
        return ordinal;
    }
    ordinal.global = g_fuzzStats->totalIterations - 1U;
    if (SeedFuzzSummary* seedSummary = active_seed_summary();
        seedSummary != nullptr && seedSummary->iterations != 0U) {
        ordinal.seed = seedSummary->iterations - 1U;
    }
    return ordinal;
}

void record_exact_audit_skip(ExactAuditSkipReason reason) {
    if (!g_fuzzStats.has_value()) {
        return;
    }
    auto apply = [&](auto& summary) {
        switch (reason) {
            case ExactAuditSkipReason::NONE:
                break;
            case ExactAuditSkipReason::SAMPLE:
                ++summary.exactAuditSkippedSampleCount;
                break;
            case ExactAuditSkipReason::BUDGET:
                ++summary.exactAuditSkippedBudgetCount;
                break;
            case ExactAuditSkipReason::FAMILY:
                ++summary.exactAuditSkippedFamilyCount;
                break;
            case ExactAuditSkipReason::NON_TIE:
                ++summary.exactAuditSkippedNonTieCount;
                break;
            case ExactAuditSkipReason::CAP:
                ++summary.exactAuditSkippedCapCount;
                break;
        }
    };
    apply(*g_fuzzStats);
    if (SeedFuzzSummary* seedSummary = active_seed_summary(); seedSummary != nullptr) {
        apply(*seedSummary);
    }
}

bool compare_eligibility_counts_as_direct_evidence(ScenarioFamily family, const CompareEligibilityInfo& info) {
    switch (family) {
        case ScenarioFamily::PLANNER_TIE_MIXED:
        case ScenarioFamily::PLANNER_TIE_MIXED_SYMMETRIC:
        case ScenarioFamily::PLANNER_TIE_MIXED_ORGANIC:
            return info.mixedCompareEligible;
        default:
            return info.compareEligible;
    }
}

void record_compare_eligibility_probe(ScenarioFamily family, const CompareEligibilityInfo& info) {
    if (!g_fuzzStats.has_value()) {
        return;
    }
    const bool directEvidenceEligible = compare_eligibility_counts_as_direct_evidence(family, info);
    auto apply = [&](auto& summary) {
        if (info.hasSplitReady) {
            ++summary.splitReadyStateCount;
        }
        if (info.hasTie) {
            ++summary.tieReadyStateCount;
        }
        if (directEvidenceEligible) {
            ++summary.compareEligibleStateCount;
            return;
        }
        ++summary.compareIneligibleStateCount;
        ++summary.compareIneligibleReasonHistogram[compare_eligibility_reason_name(info.reason)];
    };
    apply(*g_fuzzStats);
    if (SeedFuzzSummary* seedSummary = active_seed_summary(); seedSummary != nullptr) {
        apply(*seedSummary);
    }
}

void record_compare_completion(bool completed) {
    if (!g_fuzzStats.has_value()) {
        return;
    }
    auto apply = [&](auto& summary) {
        if (completed) {
            ++summary.compareCompletedStateCount;
        } else {
            ++summary.comparePartialStateCount;
        }
    };
    apply(*g_fuzzStats);
    if (SeedFuzzSummary* seedSummary = active_seed_summary(); seedSummary != nullptr) {
        apply(*seedSummary);
    }
}

bool split_choice_compare_enabled(const TestOptions& options) {
    return options.splitChoiceCompareMode == SplitChoiceCompareMode::EXACT_FULL;
}

double split_choice_compare_sample_rate(const TestOptions& options) {
    return split_choice_compare_enabled(options) ? options.compareSampleRate : options.exactAuditSampleRate;
}

size_t split_choice_compare_budget(const TestOptions& options) {
    return split_choice_compare_enabled(options) ? options.compareBudget : options.exactAuditBudget;
}

const char* policy_graduation_gate_status_name(PolicyGraduationGateStatus status) {
    switch (status) {
        case PolicyGraduationGateStatus::PASS:
            return "PASS";
        case PolicyGraduationGateStatus::PROXY_PASS:
            return "PROXY_PASS";
        case PolicyGraduationGateStatus::NON_APPLICABLE:
            return "NON_APPLICABLE";
        case PolicyGraduationGateStatus::FAIL:
            return "FAIL";
    }
    return "FAIL";
}

const char* campaign_stop_reason_name(CampaignStopReason reason) {
    switch (reason) {
        case CampaignStopReason::COMPLETED:
            return "completed";
        case CampaignStopReason::GATE_PASSED:
            return "gate_passed";
        case CampaignStopReason::STOP_AFTER_CHECKPOINT:
            return "stop_after_checkpoint";
        case CampaignStopReason::MAX_WALL_SECONDS:
            return "max_wall_seconds";
        case CampaignStopReason::MAX_PARTIAL_RUNS:
            return "max_partial_runs";
    }
    return "completed";
}

optional<PolicyGraduationThreshold> policy_graduation_threshold_for_family(const string& family) {
    if (family == "split_tie_ready") {
        PolicyGraduationThreshold threshold;
        threshold.family = family;
        threshold.comparedStates = 16U;
        threshold.eligibleStates = 16U;
        threshold.completedStates = 16U;
        return threshold;
    }
    if (family == "split_tie_organic_symmetric") {
        PolicyGraduationThreshold threshold;
        threshold.family = family;
        threshold.comparedStates = 32U;
        threshold.eligibleStates = 32U;
        threshold.completedStates = 32U;
        return threshold;
    }
    if (family == "planner_tie_mixed_organic_compare_ready") {
        PolicyGraduationThreshold threshold;
        threshold.family = family;
        threshold.comparedStates = 32U;
        threshold.eligibleStates = 32U;
        threshold.completedStates = 32U;
        return threshold;
    }
    if (family == "automorphism_probe_large") {
        PolicyGraduationThreshold threshold;
        threshold.family = family;
        threshold.comparedStates = 32U;
        threshold.eligibleStates = 32U;
        threshold.completedStates = 32U;
        return threshold;
    }
    if (family == "planner_tie_mixed_organic") {
        PolicyGraduationThreshold threshold;
        threshold.family = family;
        threshold.lineageSamples = 16U;
        threshold.applicabilityStates = 48U;
        threshold.nonApplicableMaxCompareRelevance = 0.02;
        threshold.nonApplicableMaxSplitReadyRelevance = 0.05;
        threshold.nonApplicableMinReasonConfidence = 0.90;
        threshold.proxyFamily = "planner_tie_mixed_organic_compare_ready";
        return threshold;
    }
    return nullopt;
}

size_t compare_audited_state_count(const FuzzStats& stats) {
    return stats.compareEligibleStateCount + stats.compareIneligibleStateCount;
}

double compare_ineligible_reason_confidence(const FuzzStats& stats, const string& reason) {
    if (stats.compareIneligibleStateCount == 0U) {
        return 0.0;
    }
    const auto it = stats.compareIneligibleReasonHistogram.find(reason);
    const size_t count = it == stats.compareIneligibleReasonHistogram.end() ? 0U : it->second;
    return static_cast<double>(count) / static_cast<double>(stats.compareIneligibleStateCount);
}

bool policy_gate_satisfied(PolicyGraduationGateStatus status) {
    return status == PolicyGraduationGateStatus::PASS ||
        status == PolicyGraduationGateStatus::PROXY_PASS ||
        status == PolicyGraduationGateStatus::NON_APPLICABLE;
}

PolicyGraduationEvaluation evaluate_policy_graduation_gate(const TestOptions& options, const FuzzStats& stats) {
    PolicyGraduationEvaluation evaluation;
    const optional<PolicyGraduationThreshold> configured =
        policy_graduation_threshold_for_family(stats.scenarioFamily);
    if (!configured.has_value()) {
        evaluation.threshold.family = stats.scenarioFamily;
        evaluation.rationale = "no graduation threshold configured";
        return evaluation;
    }

    evaluation.threshold = *configured;
    evaluation.effectiveComparedTarget = max(evaluation.threshold.comparedStates, options.targetComparedStates);
    evaluation.effectiveEligibleTarget = max(evaluation.threshold.eligibleStates, options.targetEligibleStates);
    evaluation.effectiveCompletedTarget = max(
        evaluation.threshold.completedStates,
        max(options.targetComparedStates, options.targetEligibleStates)
    );
    evaluation.effectiveLineageTarget =
        options.targetLineageSamples != 0U ? options.targetLineageSamples : evaluation.threshold.lineageSamples;
    evaluation.effectiveApplicabilityTarget = evaluation.threshold.applicabilityStates;
    evaluation.effectiveApplicabilityConfidence =
        options.targetApplicabilityConfidence > 0.0
            ? options.targetApplicabilityConfidence
            : evaluation.threshold.nonApplicableMinReasonConfidence;

    if (stats.splitChoiceSemanticDisagreementCount != 0U ||
        stats.splitChoiceFallbackCount != 0U ||
        stats.semanticShiftCount != 0U) {
        evaluation.status = PolicyGraduationGateStatus::FAIL;
        evaluation.rationale = "semantic disagreement, fallback, or semantic shift detected";
        return evaluation;
    }

    if (evaluation.threshold.comparedStates != 0U ||
        evaluation.threshold.eligibleStates != 0U ||
        evaluation.threshold.completedStates != 0U) {
        if (stats.splitChoiceCompareStateCount < evaluation.effectiveComparedTarget ||
            stats.compareEligibleStateCount < evaluation.effectiveEligibleTarget ||
            stats.compareCompletedStateCount < evaluation.effectiveCompletedTarget) {
            evaluation.status = PolicyGraduationGateStatus::FAIL;
            evaluation.rationale = "insufficient direct compare evidence";
            return evaluation;
        }
        evaluation.status = PolicyGraduationGateStatus::PASS;
        evaluation.rationale = "direct compare evidence satisfied";
        return evaluation;
    }

    const size_t auditedStates = compare_audited_state_count(stats);
    if (auditedStates < evaluation.effectiveApplicabilityTarget) {
        evaluation.status = PolicyGraduationGateStatus::FAIL;
        evaluation.rationale = "insufficient applicability evidence";
        return evaluation;
    }

    const double compareRelevance = auditedStates == 0U
        ? 0.0
        : static_cast<double>(stats.compareEligibleStateCount) / static_cast<double>(auditedStates);
    const double splitReadyRelevance = auditedStates == 0U
        ? 0.0
        : static_cast<double>(stats.splitReadyStateCount) / static_cast<double>(auditedStates);
    const double noSplitReadyConfidence = compare_ineligible_reason_confidence(stats, "no_split_ready");
    if (compareRelevance <= evaluation.threshold.nonApplicableMaxCompareRelevance &&
        splitReadyRelevance <= evaluation.threshold.nonApplicableMaxSplitReadyRelevance &&
        noSplitReadyConfidence >= evaluation.effectiveApplicabilityConfidence &&
        stats.actualJoinHits != 0U &&
        stats.actualIntegrateHits != 0U) {
        evaluation.status = PolicyGraduationGateStatus::NON_APPLICABLE;
        evaluation.rationale = "family remains mixed-followup heavy and split-choice compare is stably no_split_ready";
        return evaluation;
    }

    evaluation.status = PolicyGraduationGateStatus::FAIL;
    evaluation.rationale = "applicability audit remains inconclusive or indicates under-generation";
    return evaluation;
}

PolicyGraduationEvaluation evaluate_proxy_policy_graduation_gate(
    const TestOptions& options,
    const FamilyApplicabilitySummary& applicability,
    const CompareReadyLineageSummary& lineage,
    const FuzzStats& proxyStats
) {
    PolicyGraduationEvaluation evaluation;
    const optional<PolicyGraduationThreshold> configured =
        policy_graduation_threshold_for_family("planner_tie_mixed_organic");
    if (!configured.has_value()) {
        evaluation.threshold.family = "planner_tie_mixed_organic";
        evaluation.rationale = "planner_tie_mixed_organic threshold missing";
        return evaluation;
    }

    evaluation.threshold = *configured;
    const optional<PolicyGraduationThreshold> proxyThreshold =
        policy_graduation_threshold_for_family(proxyStats.scenarioFamily);
    if (!proxyThreshold.has_value()) {
        evaluation.status = PolicyGraduationGateStatus::FAIL;
        evaluation.rationale = "proxy compare family threshold missing";
        return evaluation;
    }
    evaluation.effectiveComparedTarget = max(options.targetComparedStates, proxyThreshold->comparedStates);
    evaluation.effectiveEligibleTarget = max(options.targetEligibleStates, proxyThreshold->eligibleStates);
    evaluation.effectiveCompletedTarget = max(
        max(options.targetComparedStates, options.targetEligibleStates),
        proxyThreshold->completedStates
    );
    evaluation.effectiveLineageTarget =
        options.targetLineageSamples != 0U ? options.targetLineageSamples : evaluation.threshold.lineageSamples;
    evaluation.effectiveApplicabilityTarget = evaluation.threshold.applicabilityStates;
    evaluation.effectiveApplicabilityConfidence =
        options.targetApplicabilityConfidence > 0.0
            ? options.targetApplicabilityConfidence
            : evaluation.threshold.nonApplicableMinReasonConfidence;

    const PolicyGraduationEvaluation proxyGate = evaluate_policy_graduation_gate(options, proxyStats);
    if (proxyGate.status != PolicyGraduationGateStatus::PASS) {
        evaluation.status = PolicyGraduationGateStatus::FAIL;
        evaluation.rationale = "proxy compare family did not PASS";
        return evaluation;
    }
    if (lineage.lineageSampleCount < evaluation.effectiveLineageTarget ||
        lineage.sameSemanticClassCount < lineage.lineageSampleCount ||
        lineage.sameFinalStateCount < lineage.lineageSampleCount ||
        lineage.followupPatternPreservedCount < lineage.lineageSampleCount ||
        lineage.structureOnlyEnrichmentCount < lineage.lineageSampleCount) {
        evaluation.status = PolicyGraduationGateStatus::FAIL;
        evaluation.rationale = "proxy lineage evidence below threshold";
        return evaluation;
    }
    if (applicability.classification == FamilyApplicabilityClassification::NON_APPLICABLE) {
        evaluation.status = PolicyGraduationGateStatus::NON_APPLICABLE;
        evaluation.rationale = "direct family is non-applicable; proxy compare family still PASSes";
        return evaluation;
    }
    if (applicability.classification == FamilyApplicabilityClassification::UNDER_GENERATED) {
        evaluation.status = PolicyGraduationGateStatus::PROXY_PASS;
        evaluation.rationale = "direct family stays under-generated but proxy lineage and compare evidence are sufficient";
        return evaluation;
    }
    evaluation.status = PolicyGraduationGateStatus::PASS;
    evaluation.rationale = "direct family remains directly applicable and proxy evidence is additive";
    return evaluation;
}

PolicyGraduationGateStatus combine_policy_graduation_status(
    PolicyGraduationGateStatus lhs,
    PolicyGraduationGateStatus rhs
) {
    if (lhs == PolicyGraduationGateStatus::FAIL || rhs == PolicyGraduationGateStatus::FAIL) {
        return PolicyGraduationGateStatus::FAIL;
    }
    if (lhs == PolicyGraduationGateStatus::PROXY_PASS || rhs == PolicyGraduationGateStatus::PROXY_PASS) {
        return PolicyGraduationGateStatus::PROXY_PASS;
    }
    if (lhs == PolicyGraduationGateStatus::PASS || rhs == PolicyGraduationGateStatus::PASS) {
        return PolicyGraduationGateStatus::PASS;
    }
    return PolicyGraduationGateStatus::NON_APPLICABLE;
}

bool should_sample_split_choice_compare(const TestOptions& options, const IterationOrdinal& ordinal) {
    if (options.exactCanonicalCap == 0U && !split_choice_compare_enabled(options)) {
        return false;
    }
    const double sampleRate = split_choice_compare_sample_rate(options);
    if (sampleRate >= 1.0) {
        const size_t stride = static_cast<size_t>(sampleRate);
        return stride == 0U || ordinal.global % stride == 0U;
    }
    const double scaled = sampleRate * 1000.0;
    const size_t threshold = static_cast<size_t>(scaled);
    return (ordinal.global % 1000U) < max<size_t>(1U, threshold);
}

bool exact_audit_family_matches(const TestOptions& options, ScenarioFamily family) {
    return !options.exactAuditFamily.has_value() || *options.exactAuditFamily == family;
}

void record_split_choice_audit_result(const SplitChoiceOracleRunResult& result) {
    if (!g_fuzzStats.has_value()) {
        return;
    }
    auto apply = [&](auto& summary) {
        ++summary.exactAuditedStateCount;
        ++summary.compareCompletedStateCount;
        summary.exactAuditedPairCount += result.comparedPairCount;
        summary.splitChoiceCompareStateCount += result.compareStateCount;
        summary.splitChoiceExactFullEvalCount += result.exactFullEvalCount;
        summary.splitChoiceExactShadowEvalCount += result.exactShadowEvalCount;
        summary.splitChoiceFallbackCount += result.fallbackCount;
        summary.splitChoiceSameRepresentativeCount += result.sameRepresentativeCount;
        summary.splitChoiceSameSemanticClassCount += result.sameSemanticClassCount;
        summary.splitChoiceSameFinalStateCount += result.sameFinalStateCount;
        summary.splitChoiceSemanticDisagreementCount += result.semanticDisagreementCount;
        summary.splitChoiceCapHitCount += result.capHitCount;
        summary.splitChoiceHarmlessCompareCount += result.harmlessCompareCount;
        summary.splitChoiceTraceOnlyCompareCount += result.traceOnlyCompareCount;
        summary.fastVsExactClassDisagreementCount += result.exactVsFastClassDisagreementCount;
        summary.representativeShiftCount += result.representativeShiftCount;
        summary.representativeShiftSameClassCount += result.representativeShiftSameClassCount;
        summary.representativeShiftSemanticDivergenceCount += result.representativeShiftSemanticDivergenceCount;
        summary.representativeShiftFollowupDivergenceCount += result.representativeShiftFollowupDivergenceCount;
        summary.representativeShiftTraceDivergenceCount += result.representativeShiftTraceDivergenceCount;
        summary.harmlessShiftCount += result.harmlessShiftCount;
        summary.traceOnlyShiftCount += result.traceOnlyShiftCount;
        summary.semanticShiftCount += result.semanticShiftCount;
        summary.compareCandidateEnumerationNanos += result.candidateEnumerationNanos;
        summary.compareExactShadowEvaluationNanos += result.exactShadowEvaluationNanos;
        summary.compareExactFullEvaluationNanos += result.exactFullEvaluationNanos;
        summary.compareCanonicalizationNanos += result.canonicalizationNanos;
        summary.compareMulticlassCatalogNanos += result.multiclassCatalogNanos;
        summary.compareStateHashCacheHitCount += result.stateHashCacheHitCount;
        summary.compareStateHashCacheMissCount += result.stateHashCacheMissCount;
        summary.compareCandidateEvaluationCacheHitCount += result.candidateEvaluationCacheHitCount;
        summary.compareCandidateEvaluationCacheMissCount += result.candidateEvaluationCacheMissCount;
        summary.compareExactCanonicalCacheHitCount += result.exactCanonicalCacheHitCount;
        summary.compareExactCanonicalCacheMissCount += result.exactCanonicalCacheMissCount;
    };
    apply(*g_fuzzStats);
    if (SeedFuzzSummary* seedSummary = active_seed_summary(); seedSummary != nullptr) {
        apply(*seedSummary);
    }
}

void record_generated_artifact_metadata(bool hasDirectAB, size_t multiEdgeCount) {
    if (!g_fuzzStats.has_value()) {
        return;
    }
    g_fuzzStats->directABArtifactCount += (hasDirectAB ? 1U : 0U);
    g_fuzzStats->multiEdgeCount += multiEdgeCount;
    if (SeedFuzzSummary* seedSummary = active_seed_summary(); seedSummary != nullptr) {
        seedSummary->directABArtifactCount += (hasDirectAB ? 1U : 0U);
        seedSummary->multiEdgeCount += multiEdgeCount;
    }
}

void note_first_hit(int& slot, size_t iterIndex, size_t hitCount) {
    if (slot < 0 && hitCount > 0U) {
        slot = static_cast<int>(iterIndex);
    }
}

string trace_prefix_key(const vector<PlannerTraceEntry>& trace, size_t maxEntries = 4U) {
    if (trace.empty()) {
        return "empty";
    }
    ostringstream oss;
    const size_t limit = min(maxEntries, trace.size());
    for (size_t i = 0; i < limit; ++i) {
        if (i != 0U) {
            oss << ">";
        }
        oss << planner_primitive_name(trace[i].primitive);
    }
    return oss.str();
}

string primitive_multiset_key(const vector<PlannerTraceEntry>& trace) {
    array<size_t, 4> counts{{0, 0, 0, 0}};
    for (const PlannerTraceEntry& entry : trace) {
        ++counts[planner_primitive_index(entry.primitive)];
    }
    ostringstream oss;
    oss << "isolate=" << counts[0]
        << ",split=" << counts[1]
        << ",join=" << counts[2]
        << ",integrate=" << counts[3];
    return oss.str();
}

void record_planner_execution_stats(const PlannerExecutionResult& result, const IterationOrdinal& ordinal) {
    if (!g_fuzzStats.has_value()) {
        return;
    }

    ostringstream seq;
    for (size_t i = 0; i < result.trace.size(); ++i) {
        if (i != 0U) {
            seq << ">";
        }
        seq << planner_primitive_name(result.trace[i].primitive);
        const size_t idx = planner_primitive_index(result.trace[i].primitive);
        ++g_fuzzStats->primitiveHits[idx];
        if (SeedFuzzSummary* seedSummary = active_seed_summary(); seedSummary != nullptr) {
            ++seedSummary->primitiveHits[idx];
        }
    }

    ++g_fuzzStats->sequenceHistogram[result.trace.empty() ? string("empty") : seq.str()];
    ++g_fuzzStats->tracePrefixHistogram[trace_prefix_key(result.trace)];
    ++g_fuzzStats->primitiveMultisetHistogram[primitive_multiset_key(result.trace)];
    if (result.coverage.actualSplitHits != 0U) {
        ++g_fuzzStats->splitSuccessCount;
        if (SeedFuzzSummary* seedSummary = active_seed_summary(); seedSummary != nullptr) {
            ++seedSummary->splitSuccessCount;
        }
    } else {
        ++g_fuzzStats->splitRejectedCount;
        if (SeedFuzzSummary* seedSummary = active_seed_summary(); seedSummary != nullptr) {
            ++seedSummary->splitRejectedCount;
        }
    }

    g_fuzzStats->splitReadyCount += result.coverage.splitReadyCount;
    g_fuzzStats->boundaryOnlyChildCount += result.coverage.boundaryOnlyChildCount;
    g_fuzzStats->joinCandidateCount += result.coverage.joinCandidateCount;
    g_fuzzStats->integrateCandidateCount += result.coverage.integrateCandidateCount;
    g_fuzzStats->actualSplitHits += result.coverage.actualSplitHits;
    g_fuzzStats->actualJoinHits += result.coverage.actualJoinHits;
    g_fuzzStats->actualIntegrateHits += result.coverage.actualIntegrateHits;
    g_fuzzStats->splitChoiceCandidateCount += result.coverage.splitChoiceCandidateCount;
    g_fuzzStats->splitChoiceEvalCount += result.coverage.splitChoiceEvalCount;
    g_fuzzStats->splitChoiceTieCount += result.coverage.splitChoiceTieCount;
    g_fuzzStats->splitChoiceMulticlassCount += result.coverage.splitChoiceMulticlassCount;
    g_fuzzStats->splitChoiceFallbackCount += result.coverage.splitChoiceFallbackCount;
    note_first_hit(g_fuzzStats->firstSplitIter, ordinal.global, result.coverage.actualSplitHits);
    note_first_hit(g_fuzzStats->firstJoinIter, ordinal.global, result.coverage.actualJoinHits);
    note_first_hit(g_fuzzStats->firstIntegrateIter, ordinal.global, result.coverage.actualIntegrateHits);
    note_first_hit(g_fuzzStats->firstSplitChoiceTieIter, ordinal.global, result.coverage.splitChoiceTieCount);
    for (const auto& [classCount, seen] : result.coverage.splitChoiceEquivClassCountHistogram) {
        g_fuzzStats->splitChoiceEquivClassCountHistogram[classCount] += seen;
    }

    if (SeedFuzzSummary* seedSummary = active_seed_summary(); seedSummary != nullptr) {
        seedSummary->splitReadyCount += result.coverage.splitReadyCount;
        seedSummary->boundaryOnlyChildCount += result.coverage.boundaryOnlyChildCount;
        seedSummary->joinCandidateCount += result.coverage.joinCandidateCount;
        seedSummary->integrateCandidateCount += result.coverage.integrateCandidateCount;
        seedSummary->actualSplitHits += result.coverage.actualSplitHits;
        seedSummary->actualJoinHits += result.coverage.actualJoinHits;
        seedSummary->actualIntegrateHits += result.coverage.actualIntegrateHits;
        seedSummary->splitChoiceCandidateCount += result.coverage.splitChoiceCandidateCount;
        seedSummary->splitChoiceEvalCount += result.coverage.splitChoiceEvalCount;
        seedSummary->splitChoiceTieCount += result.coverage.splitChoiceTieCount;
        seedSummary->splitChoiceMulticlassCount += result.coverage.splitChoiceMulticlassCount;
        seedSummary->splitChoiceFallbackCount += result.coverage.splitChoiceFallbackCount;
        note_first_hit(seedSummary->firstSplitIter, ordinal.seed, result.coverage.actualSplitHits);
        note_first_hit(seedSummary->firstJoinIter, ordinal.seed, result.coverage.actualJoinHits);
        note_first_hit(seedSummary->firstIntegrateIter, ordinal.seed, result.coverage.actualIntegrateHits);
        note_first_hit(seedSummary->firstSplitChoiceTieIter, ordinal.seed, result.coverage.splitChoiceTieCount);
        for (const auto& [classCount, seen] : result.coverage.splitChoiceEquivClassCountHistogram) {
            seedSummary->splitChoiceEquivClassCountHistogram[classCount] += seen;
        }
        ++seedSummary->tracePrefixHistogram[trace_prefix_key(result.trace)];
        ++seedSummary->primitiveMultisetHistogram[primitive_multiset_key(result.trace)];
    }
}

void record_reducer_invocation() {
    if (!g_fuzzStats.has_value()) {
        return;
    }
    ++g_fuzzStats->reducerInvocationCount;
    if (SeedFuzzSummary* seedSummary = active_seed_summary(); seedSummary != nullptr) {
        ++seedSummary->reducerInvocationCount;
    }
}

void record_failure_stat(const string& key) {
    if (!g_fuzzStats.has_value()) {
        return;
    }
    ++g_fuzzStats->oracleMismatchCount[key];
}

string json_escape(const string& text) {
    string out;
    out.reserve(text.size() + 8U);
    for (char ch : text) {
        switch (static_cast<unsigned char>(ch)) {
            case '\\':
                out += "\\\\";
                break;
            case '"':
                out += "\\\"";
                break;
            case '\n':
                out += "\\n";
                break;
            default:
                out.push_back(static_cast<char>(ch));
                break;
        }
    }
    return out;
}

template <size_t N>
string json_array_from_counts(const array<size_t, N>& counts, const array<const char*, N>& labels) {
    ostringstream oss;
    oss << '{';
    for (size_t i = 0; i < N; ++i) {
        if (i != 0U) {
            oss << ',';
        }
        oss << '"' << labels[i] << "\":" << counts[i];
    }
    oss << '}';
    return oss.str();
}

string json_object_from_map(const unordered_map<string, size_t>& values) {
    vector<pair<string, size_t>> ordered(values.begin(), values.end());
    sort(ordered.begin(), ordered.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
    });
    ostringstream oss;
    oss << '{';
    for (size_t i = 0; i < ordered.size(); ++i) {
        if (i != 0U) {
            oss << ',';
        }
        oss << '"' << json_escape(ordered[i].first) << "\":" << ordered[i].second;
    }
    oss << '}';
    return oss.str();
}

string json_object_from_numeric_map(const map<size_t, size_t>& values) {
    ostringstream oss;
    oss << '{';
    bool first = true;
    for (const auto& [key, value] : values) {
        if (!first) {
            oss << ',';
        }
        first = false;
        oss << '"' << key << "\":" << value;
    }
    oss << '}';
    return oss.str();
}

double safe_ratio(size_t numerator, size_t denominator) {
    return static_cast<double>(numerator) / static_cast<double>(max<size_t>(1U, denominator));
}

string json_number(double value) {
    ostringstream oss;
    oss << fixed << setprecision(6) << value;
    return oss.str();
}

size_t total_primitive_hits(const array<size_t, 4>& primitiveHits) {
    return accumulate(primitiveHits.begin(), primitiveHits.end(), 0U);
}

string json_conversion_object(
    size_t actualSplit,
    size_t splitReady,
    size_t actualJoin,
    size_t joinCandidate,
    size_t actualIntegrate,
    size_t integrateCandidate
) {
    ostringstream oss;
    oss << '{'
        << "\"split_conversion\":" << json_number(safe_ratio(actualSplit, splitReady)) << ','
        << "\"join_conversion\":" << json_number(safe_ratio(actualJoin, joinCandidate)) << ','
        << "\"integrate_conversion\":" << json_number(safe_ratio(actualIntegrate, integrateCandidate))
        << '}';
    return oss.str();
}

string json_diversity_object(
    const unordered_map<string, size_t>& tracePrefixHistogram,
    const unordered_map<string, size_t>& primitiveMultisetHistogram
) {
    ostringstream oss;
    oss << '{'
        << "\"unique_trace_prefix_count\":" << tracePrefixHistogram.size() << ','
        << "\"unique_primitive_multiset_count\":" << primitiveMultisetHistogram.size()
        << '}';
    return oss.str();
}

string json_coverage_summary_object(
    const array<size_t, 4>& primitiveHits,
    size_t iterations,
    size_t actualSplitHits,
    size_t actualJoinHits,
    size_t actualIntegrateHits
) {
    const size_t primitiveTotal = total_primitive_hits(primitiveHits);
    ostringstream oss;
    oss << '{'
        << "\"primitive_total\":" << primitiveTotal << ','
        << "\"isolate_heavy_ratio\":" << json_number(safe_ratio(primitiveHits[0], primitiveTotal)) << ','
        << "\"split_hit_density\":" << json_number(safe_ratio(actualSplitHits, iterations)) << ','
        << "\"join_hit_density\":" << json_number(safe_ratio(actualJoinHits, iterations)) << ','
        << "\"integrate_hit_density\":" << json_number(safe_ratio(actualIntegrateHits, iterations))
        << '}';
    return oss.str();
}

filesystem::path stats_output_path(const TestOptions& options, const string& caseName) {
    if (options.statsFile.has_value()) {
        return filesystem::absolute(*options.statsFile);
    }
    return artifact_subdir(options, "logs") / (sanitize_stem_token(caseName) + "_stats.json");
}

filesystem::path stats_summary_output_path(const TestOptions& options, const string& caseName) {
    const filesystem::path jsonPath = stats_output_path(options, caseName);
    return jsonPath.parent_path() / (jsonPath.stem().string() + ".summary.txt");
}

void flush_fuzz_summary_text(const TestOptions& options, const string& caseName) {
    if (!g_fuzzStats.has_value()) {
        return;
    }
    sync_multiclass_catalog_stats();

    const filesystem::path outPath = stats_summary_output_path(options, caseName);
    filesystem::create_directories(outPath.parent_path());

    ofstream ofs(outPath);
    if (!ofs) {
        throw runtime_error("failed to write stats summary file: " + outPath.string());
    }
    write_fuzz_summary_body(ofs, *g_fuzzStats);
}

void flush_fuzz_stats(const TestOptions& options, const string& caseName) {
    if (!g_fuzzStats.has_value()) {
        return;
    }
    sync_multiclass_catalog_stats();

    const array<const char*, 4> primitiveLabels = {{"isolate", "split", "join", "integrate"}};
    const filesystem::path outPath = stats_output_path(options, caseName);
    filesystem::create_directories(outPath.parent_path());

    ofstream ofs(outPath);
    if (!ofs) {
        throw runtime_error("failed to write stats file: " + outPath.string());
    }

    ofs << "{\n";
    ofs << "\"case\":\"" << json_escape(g_fuzzStats->caseName) << "\",\n";
    ofs << "\"weight_profile\":\"" << json_escape(g_fuzzStats->weightProfile) << "\",\n";
    ofs << "\"precondition_bias_profile\":\"" << json_escape(g_fuzzStats->preconditionBiasProfile) << "\",\n";
    ofs << "\"scenario_family\":\"" << json_escape(g_fuzzStats->scenarioFamily) << "\",\n";
    ofs << "\"split_choice_policy_mode\":\"" << json_escape(g_fuzzStats->splitChoicePolicyMode) << "\",\n";
    ofs << "\"split_choice_compare_mode\":\"" << json_escape(g_fuzzStats->splitChoiceCompareMode) << "\",\n";
    ofs << "\"bias_split\":" << g_fuzzStats->biasSplit << ",\n";
    ofs << "\"bias_join\":" << g_fuzzStats->biasJoin << ",\n";
    ofs << "\"bias_integrate\":" << g_fuzzStats->biasIntegrate << ",\n";
    ofs << "\"total_iterations\":" << g_fuzzStats->totalIterations << ",\n";
    ofs << "\"primitive_hits\":" << json_array_from_counts(g_fuzzStats->primitiveHits, primitiveLabels) << ",\n";
    ofs << "\"sequence_histogram\":" << json_object_from_map(g_fuzzStats->sequenceHistogram) << ",\n";
    ofs << "\"trace_prefix_histogram\":" << json_object_from_map(g_fuzzStats->tracePrefixHistogram) << ",\n";
    ofs << "\"primitive_multiset_histogram\":" << json_object_from_map(g_fuzzStats->primitiveMultisetHistogram) << ",\n";
    ofs << "\"split_success_count\":" << g_fuzzStats->splitSuccessCount << ",\n";
    ofs << "\"split_rejected_count\":" << g_fuzzStats->splitRejectedCount << ",\n";
    ofs << "\"direct_ab_artifact_count\":" << g_fuzzStats->directABArtifactCount << ",\n";
    ofs << "\"split_ready_count\":" << g_fuzzStats->splitReadyCount << ",\n";
    ofs << "\"boundary_only_child_count\":" << g_fuzzStats->boundaryOnlyChildCount << ",\n";
    ofs << "\"join_candidate_count\":" << g_fuzzStats->joinCandidateCount << ",\n";
    ofs << "\"integrate_candidate_count\":" << g_fuzzStats->integrateCandidateCount << ",\n";
    ofs << "\"actual_split_hits\":" << g_fuzzStats->actualSplitHits << ",\n";
    ofs << "\"actual_join_hits\":" << g_fuzzStats->actualJoinHits << ",\n";
    ofs << "\"actual_integrate_hits\":" << g_fuzzStats->actualIntegrateHits << ",\n";
    ofs << "\"split_choice_candidate_count\":" << g_fuzzStats->splitChoiceCandidateCount << ",\n";
    ofs << "\"split_choice_eval_count\":" << g_fuzzStats->splitChoiceEvalCount << ",\n";
    ofs << "\"split_choice_tie_count\":" << g_fuzzStats->splitChoiceTieCount << ",\n";
    ofs << "\"split_choice_multiclass_count\":" << g_fuzzStats->splitChoiceMulticlassCount << ",\n";
    ofs << "\"split_choice_fallback_count\":" << g_fuzzStats->splitChoiceFallbackCount << ",\n";
    ofs << "\"split_choice_compare_state_count\":" << g_fuzzStats->splitChoiceCompareStateCount << ",\n";
    ofs << "\"split_ready_state_count\":" << g_fuzzStats->splitReadyStateCount << ",\n";
    ofs << "\"tie_ready_state_count\":" << g_fuzzStats->tieReadyStateCount << ",\n";
    ofs << "\"compare_eligible_state_count\":" << g_fuzzStats->compareEligibleStateCount << ",\n";
    ofs << "\"compare_ineligible_state_count\":" << g_fuzzStats->compareIneligibleStateCount << ",\n";
    ofs << "\"compare_completed_state_count\":" << g_fuzzStats->compareCompletedStateCount << ",\n";
    ofs << "\"compare_partial_state_count\":" << g_fuzzStats->comparePartialStateCount << ",\n";
    ofs << "\"split_choice_exact_full_eval_count\":" << g_fuzzStats->splitChoiceExactFullEvalCount << ",\n";
    ofs << "\"split_choice_exact_shadow_eval_count\":" << g_fuzzStats->splitChoiceExactShadowEvalCount << ",\n";
    ofs << "\"split_choice_same_representative_count\":" << g_fuzzStats->splitChoiceSameRepresentativeCount << ",\n";
    ofs << "\"split_choice_same_semantic_class_count\":" << g_fuzzStats->splitChoiceSameSemanticClassCount << ",\n";
    ofs << "\"split_choice_same_final_state_count\":" << g_fuzzStats->splitChoiceSameFinalStateCount << ",\n";
    ofs << "\"split_choice_semantic_disagreement_count\":" << g_fuzzStats->splitChoiceSemanticDisagreementCount << ",\n";
    ofs << "\"split_choice_cap_hit_count\":" << g_fuzzStats->splitChoiceCapHitCount << ",\n";
    ofs << "\"split_choice_harmless_compare_count\":" << g_fuzzStats->splitChoiceHarmlessCompareCount << ",\n";
    ofs << "\"split_choice_trace_only_compare_count\":" << g_fuzzStats->splitChoiceTraceOnlyCompareCount << ",\n";
    ofs << "\"exact_audited_state_count\":" << g_fuzzStats->exactAuditedStateCount << ",\n";
    ofs << "\"exact_audited_pair_count\":" << g_fuzzStats->exactAuditedPairCount << ",\n";
    ofs << "\"fast_vs_exact_class_disagreement_count\":" << g_fuzzStats->fastVsExactClassDisagreementCount << ",\n";
    ofs << "\"representative_shift_count\":" << g_fuzzStats->representativeShiftCount << ",\n";
    ofs << "\"representative_shift_same_class_count\":" << g_fuzzStats->representativeShiftSameClassCount << ",\n";
    ofs << "\"representative_shift_semantic_divergence_count\":" << g_fuzzStats->representativeShiftSemanticDivergenceCount << ",\n";
    ofs << "\"representative_shift_followup_divergence_count\":" << g_fuzzStats->representativeShiftFollowupDivergenceCount << ",\n";
    ofs << "\"representative_shift_trace_divergence_count\":" << g_fuzzStats->representativeShiftTraceDivergenceCount << ",\n";
    ofs << "\"harmless_shift_count\":" << g_fuzzStats->harmlessShiftCount << ",\n";
    ofs << "\"trace_only_shift_count\":" << g_fuzzStats->traceOnlyShiftCount << ",\n";
    ofs << "\"semantic_shift_count\":" << g_fuzzStats->semanticShiftCount << ",\n";
    ofs << "\"compare_candidate_enumeration_ns\":" << g_fuzzStats->compareCandidateEnumerationNanos << ",\n";
    ofs << "\"compare_exact_shadow_evaluation_ns\":" << g_fuzzStats->compareExactShadowEvaluationNanos << ",\n";
    ofs << "\"compare_exact_full_evaluation_ns\":" << g_fuzzStats->compareExactFullEvaluationNanos << ",\n";
    ofs << "\"compare_canonicalization_ns\":" << g_fuzzStats->compareCanonicalizationNanos << ",\n";
    ofs << "\"compare_multiclass_catalog_ns\":" << g_fuzzStats->compareMulticlassCatalogNanos << ",\n";
    ofs << "\"compare_state_hash_cache_hit_count\":" << g_fuzzStats->compareStateHashCacheHitCount << ",\n";
    ofs << "\"compare_state_hash_cache_miss_count\":" << g_fuzzStats->compareStateHashCacheMissCount << ",\n";
    ofs << "\"compare_candidate_evaluation_cache_hit_count\":" << g_fuzzStats->compareCandidateEvaluationCacheHitCount << ",\n";
    ofs << "\"compare_candidate_evaluation_cache_miss_count\":" << g_fuzzStats->compareCandidateEvaluationCacheMissCount << ",\n";
    ofs << "\"compare_exact_canonical_cache_hit_count\":" << g_fuzzStats->compareExactCanonicalCacheHitCount << ",\n";
    ofs << "\"compare_exact_canonical_cache_miss_count\":" << g_fuzzStats->compareExactCanonicalCacheMissCount << ",\n";
    ofs << "\"exact_audit_skipped_cap_count\":" << g_fuzzStats->exactAuditSkippedCapCount << ",\n";
    ofs << "\"exact_audit_skipped_budget_count\":" << g_fuzzStats->exactAuditSkippedBudgetCount << ",\n";
    ofs << "\"exact_audit_skipped_sample_count\":" << g_fuzzStats->exactAuditSkippedSampleCount << ",\n";
    ofs << "\"exact_audit_skipped_family_count\":" << g_fuzzStats->exactAuditSkippedFamilyCount << ",\n";
    ofs << "\"exact_audit_skipped_non_tie_count\":" << g_fuzzStats->exactAuditSkippedNonTieCount << ",\n";
    ofs << "\"first_split_iter\":" << g_fuzzStats->firstSplitIter << ",\n";
    ofs << "\"first_join_iter\":" << g_fuzzStats->firstJoinIter << ",\n";
    ofs << "\"first_integrate_iter\":" << g_fuzzStats->firstIntegrateIter << ",\n";
    ofs << "\"first_split_choice_tie_iter\":" << g_fuzzStats->firstSplitChoiceTieIter << ",\n";
    ofs << "\"multi_edge_count\":" << g_fuzzStats->multiEdgeCount << ",\n";
    ofs << "\"reducer_invocation_count\":" << g_fuzzStats->reducerInvocationCount << ",\n";
    ofs << "\"multiclass_catalog_cluster_count\":" << g_fuzzStats->multiclassCatalogClusterCount << ",\n";
    ofs << "\"multiclass_harmless_cluster_count\":" << g_fuzzStats->multiclassHarmlessClusterCount << ",\n";
    ofs << "\"multiclass_trace_only_cluster_count\":" << g_fuzzStats->multiclassTraceOnlyClusterCount << ",\n";
    ofs << "\"multiclass_semantic_shift_cluster_count\":" << g_fuzzStats->multiclassSemanticShiftClusterCount << ",\n";
    ofs << "\"multiclass_catalog_histogram\":" << json_object_from_map(g_fuzzStats->multiclassCatalogHistogram)
        << ",\n";
    ofs << "\"split_choice_equiv_class_count_histogram\":"
        << json_object_from_numeric_map(g_fuzzStats->splitChoiceEquivClassCountHistogram) << ",\n";
    ofs << "\"compare_ineligible_reason_histogram\":"
        << json_object_from_map(g_fuzzStats->compareIneligibleReasonHistogram) << ",\n";
    ofs << "\"precondition_to_actual\":" << json_conversion_object(
        g_fuzzStats->actualSplitHits,
        g_fuzzStats->splitReadyCount,
        g_fuzzStats->actualJoinHits,
        g_fuzzStats->joinCandidateCount,
        g_fuzzStats->actualIntegrateHits,
        g_fuzzStats->integrateCandidateCount
    ) << ",\n";
    ofs << "\"diversity\":" << json_diversity_object(
        g_fuzzStats->tracePrefixHistogram,
        g_fuzzStats->primitiveMultisetHistogram
    ) << ",\n";
    ofs << "\"coverage_summary\":" << json_coverage_summary_object(
        g_fuzzStats->primitiveHits,
        g_fuzzStats->totalIterations,
        g_fuzzStats->actualSplitHits,
        g_fuzzStats->actualJoinHits,
        g_fuzzStats->actualIntegrateHits
    ) << ",\n";
    ofs << "\"oracle_mismatch_count\":" << json_object_from_map(g_fuzzStats->oracleMismatchCount) << ",\n";
    ofs << "\"seed_summaries\":[";
    for (size_t i = 0; i < g_fuzzStats->seedSummaries.size(); ++i) {
        if (i != 0U) {
            ofs << ',';
        }
        const SeedFuzzSummary& seedSummary = g_fuzzStats->seedSummaries[i];
        ofs << "{";
        ofs << "\"seed\":" << seedSummary.seed << ',';
        ofs << "\"iterations\":" << seedSummary.iterations << ',';
        ofs << "\"primitive_hits\":" << json_array_from_counts(seedSummary.primitiveHits, primitiveLabels) << ',';
        ofs << "\"trace_prefix_histogram\":" << json_object_from_map(seedSummary.tracePrefixHistogram) << ',';
        ofs << "\"primitive_multiset_histogram\":" << json_object_from_map(seedSummary.primitiveMultisetHistogram) << ',';
        ofs << "\"split_success_count\":" << seedSummary.splitSuccessCount << ',';
        ofs << "\"split_rejected_count\":" << seedSummary.splitRejectedCount << ',';
        ofs << "\"direct_ab_artifact_count\":" << seedSummary.directABArtifactCount << ',';
        ofs << "\"split_ready_count\":" << seedSummary.splitReadyCount << ',';
        ofs << "\"boundary_only_child_count\":" << seedSummary.boundaryOnlyChildCount << ',';
        ofs << "\"join_candidate_count\":" << seedSummary.joinCandidateCount << ',';
        ofs << "\"integrate_candidate_count\":" << seedSummary.integrateCandidateCount << ',';
        ofs << "\"actual_split_hits\":" << seedSummary.actualSplitHits << ',';
        ofs << "\"actual_join_hits\":" << seedSummary.actualJoinHits << ',';
        ofs << "\"actual_integrate_hits\":" << seedSummary.actualIntegrateHits << ',';
        ofs << "\"split_choice_candidate_count\":" << seedSummary.splitChoiceCandidateCount << ',';
        ofs << "\"split_choice_eval_count\":" << seedSummary.splitChoiceEvalCount << ',';
        ofs << "\"split_choice_tie_count\":" << seedSummary.splitChoiceTieCount << ',';
        ofs << "\"split_choice_multiclass_count\":" << seedSummary.splitChoiceMulticlassCount << ',';
        ofs << "\"split_choice_fallback_count\":" << seedSummary.splitChoiceFallbackCount << ',';
        ofs << "\"split_choice_compare_state_count\":" << seedSummary.splitChoiceCompareStateCount << ',';
        ofs << "\"split_ready_state_count\":" << seedSummary.splitReadyStateCount << ',';
        ofs << "\"tie_ready_state_count\":" << seedSummary.tieReadyStateCount << ',';
        ofs << "\"compare_eligible_state_count\":" << seedSummary.compareEligibleStateCount << ',';
        ofs << "\"compare_ineligible_state_count\":" << seedSummary.compareIneligibleStateCount << ',';
        ofs << "\"compare_completed_state_count\":" << seedSummary.compareCompletedStateCount << ',';
        ofs << "\"compare_partial_state_count\":" << seedSummary.comparePartialStateCount << ',';
        ofs << "\"split_choice_exact_full_eval_count\":" << seedSummary.splitChoiceExactFullEvalCount << ',';
        ofs << "\"split_choice_exact_shadow_eval_count\":" << seedSummary.splitChoiceExactShadowEvalCount << ',';
        ofs << "\"split_choice_same_representative_count\":" << seedSummary.splitChoiceSameRepresentativeCount << ',';
        ofs << "\"split_choice_same_semantic_class_count\":" << seedSummary.splitChoiceSameSemanticClassCount << ',';
        ofs << "\"split_choice_same_final_state_count\":" << seedSummary.splitChoiceSameFinalStateCount << ',';
        ofs << "\"split_choice_semantic_disagreement_count\":" << seedSummary.splitChoiceSemanticDisagreementCount << ',';
        ofs << "\"split_choice_cap_hit_count\":" << seedSummary.splitChoiceCapHitCount << ',';
        ofs << "\"split_choice_harmless_compare_count\":" << seedSummary.splitChoiceHarmlessCompareCount << ',';
        ofs << "\"split_choice_trace_only_compare_count\":" << seedSummary.splitChoiceTraceOnlyCompareCount << ',';
        ofs << "\"exact_audited_state_count\":" << seedSummary.exactAuditedStateCount << ',';
        ofs << "\"exact_audited_pair_count\":" << seedSummary.exactAuditedPairCount << ',';
        ofs << "\"fast_vs_exact_class_disagreement_count\":" << seedSummary.fastVsExactClassDisagreementCount << ',';
        ofs << "\"representative_shift_count\":" << seedSummary.representativeShiftCount << ',';
        ofs << "\"representative_shift_same_class_count\":" << seedSummary.representativeShiftSameClassCount << ',';
        ofs << "\"representative_shift_semantic_divergence_count\":" << seedSummary.representativeShiftSemanticDivergenceCount << ',';
        ofs << "\"representative_shift_followup_divergence_count\":" << seedSummary.representativeShiftFollowupDivergenceCount << ',';
        ofs << "\"representative_shift_trace_divergence_count\":" << seedSummary.representativeShiftTraceDivergenceCount << ',';
        ofs << "\"harmless_shift_count\":" << seedSummary.harmlessShiftCount << ',';
        ofs << "\"trace_only_shift_count\":" << seedSummary.traceOnlyShiftCount << ',';
        ofs << "\"semantic_shift_count\":" << seedSummary.semanticShiftCount << ',';
        ofs << "\"exact_audit_skipped_cap_count\":" << seedSummary.exactAuditSkippedCapCount << ',';
        ofs << "\"exact_audit_skipped_budget_count\":" << seedSummary.exactAuditSkippedBudgetCount << ',';
        ofs << "\"exact_audit_skipped_sample_count\":" << seedSummary.exactAuditSkippedSampleCount << ',';
        ofs << "\"exact_audit_skipped_family_count\":" << seedSummary.exactAuditSkippedFamilyCount << ',';
        ofs << "\"exact_audit_skipped_non_tie_count\":" << seedSummary.exactAuditSkippedNonTieCount << ',';
        ofs << "\"first_split_iter\":" << seedSummary.firstSplitIter << ',';
        ofs << "\"first_join_iter\":" << seedSummary.firstJoinIter << ',';
        ofs << "\"first_integrate_iter\":" << seedSummary.firstIntegrateIter << ',';
        ofs << "\"first_split_choice_tie_iter\":" << seedSummary.firstSplitChoiceTieIter << ',';
        ofs << "\"multi_edge_count\":" << seedSummary.multiEdgeCount << ',';
        ofs << "\"reducer_invocation_count\":" << seedSummary.reducerInvocationCount << ',';
        ofs << "\"split_choice_equiv_class_count_histogram\":"
            << json_object_from_numeric_map(seedSummary.splitChoiceEquivClassCountHistogram) << ',';
        ofs << "\"compare_ineligible_reason_histogram\":"
            << json_object_from_map(seedSummary.compareIneligibleReasonHistogram) << ',';
        ofs << "\"precondition_to_actual\":" << json_conversion_object(
            seedSummary.actualSplitHits,
            seedSummary.splitReadyCount,
            seedSummary.actualJoinHits,
            seedSummary.joinCandidateCount,
            seedSummary.actualIntegrateHits,
            seedSummary.integrateCandidateCount
        ) << ',';
        ofs << "\"diversity\":" << json_diversity_object(
            seedSummary.tracePrefixHistogram,
            seedSummary.primitiveMultisetHistogram
        ) << ',';
        ofs << "\"coverage_summary\":" << json_coverage_summary_object(
            seedSummary.primitiveHits,
            seedSummary.iterations,
            seedSummary.actualSplitHits,
            seedSummary.actualJoinHits,
            seedSummary.actualIntegrateHits
        );
        ofs << "}";
    }
    ofs << "]\n}\n";
    flush_fuzz_summary_text(options, caseName);
    write_compare_profile_summary(outPath, *g_fuzzStats);
    maybe_save_corpus_from_stats(options, caseName, *g_fuzzStats);
}

string active_corpus_mode_key(const TestOptions& options) {
    const PreconditionBiasConfig bias = resolve_precondition_bias(options);
    ostringstream oss;
    if (options.scenarioFamily != ScenarioFamily::RANDOM) {
        oss << "family_" << scenario_family_name_string(options.scenarioFamily);
    } else {
        oss << "weight_" << weight_profile_name_string(options.weightProfile)
            << "_bias_" << precondition_bias_profile_name_string(options.preconditionBiasProfile);
    }
    oss << "_s" << bias.split
        << "_j" << bias.join
        << "_i" << bias.integrate;
    return sanitize_stem_token(oss.str());
}

filesystem::path resolve_corpus_dir(const TestOptions& options, const optional<string>& explicitDir) {
    if (explicitDir.has_value()) {
        const filesystem::path dir = filesystem::absolute(*explicitDir);
        filesystem::create_directories(dir);
        return dir;
    }
    return artifact_subdir(options, "corpus");
}

filesystem::path corpus_manifest_path(const filesystem::path& dir) {
    return dir / "corpus_manifest_v1.txt";
}

size_t corpus_ratio_key(size_t actual, size_t candidate) {
    return static_cast<size_t>(safe_ratio(actual, candidate) * 1000000.0);
}

bool corpus_entry_is_better(const CorpusEntry& lhs, const CorpusEntry& rhs) {
    if (lhs.category == "best_split") {
        return make_tuple(
                   lhs.actualSplitHits,
                   corpus_ratio_key(lhs.actualSplitHits, lhs.splitReadyCount),
                   lhs.uniqueTracePrefixCount,
                   lhs.uniquePrimitiveMultisetCount,
                   lhs.seed
               ) >
               make_tuple(
                   rhs.actualSplitHits,
                   corpus_ratio_key(rhs.actualSplitHits, rhs.splitReadyCount),
                   rhs.uniqueTracePrefixCount,
                   rhs.uniquePrimitiveMultisetCount,
                   rhs.seed
               );
    }
    if (lhs.category == "best_join") {
        return make_tuple(
                   lhs.actualJoinHits,
                   corpus_ratio_key(lhs.actualJoinHits, lhs.joinCandidateCount),
                   lhs.uniqueTracePrefixCount,
                   lhs.uniquePrimitiveMultisetCount,
                   lhs.seed
               ) >
               make_tuple(
                   rhs.actualJoinHits,
                   corpus_ratio_key(rhs.actualJoinHits, rhs.joinCandidateCount),
                   rhs.uniqueTracePrefixCount,
                   rhs.uniquePrimitiveMultisetCount,
                   rhs.seed
               );
    }
    if (lhs.category == "best_integrate") {
        return make_tuple(
                   lhs.actualIntegrateHits,
                   corpus_ratio_key(lhs.actualIntegrateHits, lhs.integrateCandidateCount),
                   lhs.uniqueTracePrefixCount,
                   lhs.uniquePrimitiveMultisetCount,
                   lhs.seed
               ) >
               make_tuple(
                   rhs.actualIntegrateHits,
                   corpus_ratio_key(rhs.actualIntegrateHits, rhs.integrateCandidateCount),
                   rhs.uniqueTracePrefixCount,
                   rhs.uniquePrimitiveMultisetCount,
                   rhs.seed
               );
    }
    if (lhs.category == "diverse_trace") {
        return make_tuple(
                   lhs.uniqueTracePrefixCount,
                   lhs.uniquePrimitiveMultisetCount,
                   lhs.actualSplitHits + lhs.actualJoinHits + lhs.actualIntegrateHits,
                   lhs.seed
               ) >
               make_tuple(
                   rhs.uniqueTracePrefixCount,
                   rhs.uniquePrimitiveMultisetCount,
                   rhs.actualSplitHits + rhs.actualJoinHits + rhs.actualIntegrateHits,
                   rhs.seed
               );
    }
    return make_tuple(
               lhs.actualSplitHits,
               lhs.actualJoinHits,
               lhs.actualIntegrateHits,
               lhs.uniqueTracePrefixCount,
               lhs.uniquePrimitiveMultisetCount,
               lhs.seed
           ) >
           make_tuple(
               rhs.actualSplitHits,
               rhs.actualJoinHits,
               rhs.actualIntegrateHits,
               rhs.uniqueTracePrefixCount,
               rhs.uniquePrimitiveMultisetCount,
               rhs.seed
           );
}

CorpusEntry make_corpus_entry(
    const TestOptions& options,
    const SeedFuzzSummary& seedSummary,
    const string& category
) {
    CorpusEntry entry;
    entry.category = category;
    entry.modeKey = active_corpus_mode_key(options);
    entry.seed = seedSummary.seed;
    entry.weightProfile = weight_profile_name_string(options.weightProfile);
    entry.preconditionBiasProfile = precondition_bias_profile_name_string(options.preconditionBiasProfile);
    entry.scenarioFamily = scenario_family_name_string(options.scenarioFamily);
    entry.iters = static_cast<int>(seedSummary.iterations);
    entry.stepBudget = options.stepBudget;
    const PreconditionBiasConfig bias = resolve_precondition_bias(options);
    entry.biasSplit = bias.split;
    entry.biasJoin = bias.join;
    entry.biasIntegrate = bias.integrate;
    entry.actualSplitHits = seedSummary.actualSplitHits;
    entry.actualJoinHits = seedSummary.actualJoinHits;
    entry.actualIntegrateHits = seedSummary.actualIntegrateHits;
    entry.splitReadyCount = seedSummary.splitReadyCount;
    entry.joinCandidateCount = seedSummary.joinCandidateCount;
    entry.integrateCandidateCount = seedSummary.integrateCandidateCount;
    entry.uniqueTracePrefixCount = seedSummary.tracePrefixHistogram.size();
    entry.uniquePrimitiveMultisetCount = seedSummary.primitiveMultisetHistogram.size();
    return entry;
}

vector<CorpusEntry> select_corpus_entries(const TestOptions& options, const FuzzStats& stats) {
    vector<CorpusEntry> out;
    if (stats.seedSummaries.empty()) {
        return out;
    }

    const auto select_best = [&](const string& category, auto&& better) {
        const SeedFuzzSummary* best = nullptr;
        for (const SeedFuzzSummary& seedSummary : stats.seedSummaries) {
            if (best == nullptr || better(seedSummary, *best)) {
                best = &seedSummary;
            }
        }
        if (best != nullptr) {
            out.push_back(make_corpus_entry(options, *best, category));
        }
    };

    select_best("best_split", [](const SeedFuzzSummary& lhs, const SeedFuzzSummary& rhs) {
        return make_tuple(
                   lhs.actualSplitHits,
                   corpus_ratio_key(lhs.actualSplitHits, lhs.splitReadyCount),
                   lhs.tracePrefixHistogram.size(),
                   lhs.primitiveMultisetHistogram.size(),
                   lhs.seed
               ) >
               make_tuple(
                   rhs.actualSplitHits,
                   corpus_ratio_key(rhs.actualSplitHits, rhs.splitReadyCount),
                   rhs.tracePrefixHistogram.size(),
                   rhs.primitiveMultisetHistogram.size(),
                   rhs.seed
               );
    });
    select_best("best_join", [](const SeedFuzzSummary& lhs, const SeedFuzzSummary& rhs) {
        return make_tuple(
                   lhs.actualJoinHits,
                   corpus_ratio_key(lhs.actualJoinHits, lhs.joinCandidateCount),
                   lhs.tracePrefixHistogram.size(),
                   lhs.primitiveMultisetHistogram.size(),
                   lhs.seed
               ) >
               make_tuple(
                   rhs.actualJoinHits,
                   corpus_ratio_key(rhs.actualJoinHits, rhs.joinCandidateCount),
                   rhs.tracePrefixHistogram.size(),
                   rhs.primitiveMultisetHistogram.size(),
                   rhs.seed
               );
    });
    select_best("best_integrate", [](const SeedFuzzSummary& lhs, const SeedFuzzSummary& rhs) {
        return make_tuple(
                   lhs.actualIntegrateHits,
                   corpus_ratio_key(lhs.actualIntegrateHits, lhs.integrateCandidateCount),
                   lhs.tracePrefixHistogram.size(),
                   lhs.primitiveMultisetHistogram.size(),
                   lhs.seed
               ) >
               make_tuple(
                   rhs.actualIntegrateHits,
                   corpus_ratio_key(rhs.actualIntegrateHits, rhs.integrateCandidateCount),
                   rhs.tracePrefixHistogram.size(),
                   rhs.primitiveMultisetHistogram.size(),
                   rhs.seed
               );
    });
    select_best("diverse_trace", [](const SeedFuzzSummary& lhs, const SeedFuzzSummary& rhs) {
        return make_tuple(
                   lhs.tracePrefixHistogram.size(),
                   lhs.primitiveMultisetHistogram.size(),
                   lhs.actualSplitHits + lhs.actualJoinHits + lhs.actualIntegrateHits,
                   lhs.seed
               ) >
               make_tuple(
                   rhs.tracePrefixHistogram.size(),
                   rhs.primitiveMultisetHistogram.size(),
                   rhs.actualSplitHits + rhs.actualJoinHits + rhs.actualIntegrateHits,
                   rhs.seed
               );
    });
    return out;
}

vector<CorpusEntry> load_corpus_entries_from_dir(const filesystem::path& dir) {
    const filesystem::path manifestPath = corpus_manifest_path(dir);
    ifstream ifs(manifestPath);
    if (!ifs) {
        return {};
    }

    string header;
    if (!(ifs >> header) || header != "raw_corpus_v1") {
        throw runtime_error("invalid corpus manifest: " + manifestPath.string());
    }
    string tag;
    size_t count = 0U;
    if (!(ifs >> tag >> count) || tag != "entries") {
        throw runtime_error("invalid corpus manifest count: " + manifestPath.string());
    }

    vector<CorpusEntry> entries;
    entries.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        CorpusEntry entry;
        if (!(ifs >> tag) || tag != "entry") {
            throw runtime_error("invalid corpus manifest entry: " + manifestPath.string());
        }
        ifs >> entry.category
            >> entry.modeKey
            >> entry.seed
            >> entry.weightProfile
            >> entry.preconditionBiasProfile
            >> entry.scenarioFamily
            >> entry.biasSplit
            >> entry.biasJoin
            >> entry.biasIntegrate
            >> entry.iters
            >> entry.stepBudget
            >> entry.actualSplitHits
            >> entry.actualJoinHits
            >> entry.actualIntegrateHits
            >> entry.splitReadyCount
            >> entry.joinCandidateCount
            >> entry.integrateCandidateCount
            >> entry.uniqueTracePrefixCount
            >> entry.uniquePrimitiveMultisetCount;
        if (!ifs) {
            throw runtime_error("truncated corpus manifest entry: " + manifestPath.string());
        }
        entries.push_back(entry);
    }
    return entries;
}

void store_corpus_entries(
    const filesystem::path& dir,
    const vector<CorpusEntry>& entries,
    CorpusPolicy policy
) {
    filesystem::create_directories(dir);
    vector<CorpusEntry> merged;
    if (policy != CorpusPolicy::REPLACE) {
        merged = load_corpus_entries_from_dir(dir);
    }

    if (policy == CorpusPolicy::APPEND) {
        merged.insert(merged.end(), entries.begin(), entries.end());
    } else {
        unordered_map<string, CorpusEntry> bestByKey;
        for (const CorpusEntry& existing : merged) {
            bestByKey[existing.modeKey + "|" + existing.category] = existing;
        }
        for (const CorpusEntry& entry : entries) {
            const string key = entry.modeKey + "|" + entry.category;
            const auto it = bestByKey.find(key);
            if (it == bestByKey.end() || corpus_entry_is_better(entry, it->second)) {
                bestByKey[key] = entry;
            }
        }
        merged.clear();
        for (auto& [_, value] : bestByKey) {
            merged.push_back(value);
        }
        sort(merged.begin(), merged.end(), [](const CorpusEntry& lhs, const CorpusEntry& rhs) {
            return tie(lhs.modeKey, lhs.category, lhs.seed) < tie(rhs.modeKey, rhs.category, rhs.seed);
        });
    }

    const filesystem::path manifestPath = corpus_manifest_path(dir);
    ofstream ofs(manifestPath);
    if (!ofs) {
        throw runtime_error("failed to write corpus manifest: " + manifestPath.string());
    }
    ofs << "raw_corpus_v1\n";
    ofs << "entries " << merged.size() << '\n';
    for (const CorpusEntry& entry : merged) {
        ofs << "entry "
            << entry.category << ' '
            << entry.modeKey << ' '
            << entry.seed << ' '
            << entry.weightProfile << ' '
            << entry.preconditionBiasProfile << ' '
            << entry.scenarioFamily << ' '
            << entry.biasSplit << ' '
            << entry.biasJoin << ' '
            << entry.biasIntegrate << ' '
            << entry.iters << ' '
            << entry.stepBudget << ' '
            << entry.actualSplitHits << ' '
            << entry.actualJoinHits << ' '
            << entry.actualIntegrateHits << ' '
            << entry.splitReadyCount << ' '
            << entry.joinCandidateCount << ' '
            << entry.integrateCandidateCount << ' '
            << entry.uniqueTracePrefixCount << ' '
            << entry.uniquePrimitiveMultisetCount << '\n';
    }
}

void maybe_save_corpus_from_stats(const TestOptions& options, const string& caseName, const FuzzStats& stats) {
    (void)caseName;
    if (!options.saveCorpusDir.has_value()) {
        return;
    }
    const vector<CorpusEntry> entries = select_corpus_entries(options, stats);
    if (entries.empty()) {
        return;
    }
    store_corpus_entries(resolve_corpus_dir(options, options.saveCorpusDir), entries, options.corpusPolicy);
}

vector<CorpusEntry> load_corpus_entries(const filesystem::path& dir) {
    return load_corpus_entries_from_dir(dir);
}

vector<u32> resolve_seed_list(const TestOptions& options, const vector<u32>& baseSeeds) {
    if (options.seed.has_value()) {
        return {*options.seed};
    }
    if (!options.loadCorpusDir.has_value()) {
        return baseSeeds;
    }

    vector<u32> corpusSeeds;
    unordered_set<u32> seen;
    const string modeKey = active_corpus_mode_key(options);
    for (const CorpusEntry& entry : load_corpus_entries_from_dir(resolve_corpus_dir_for_read(options))) {
        if (entry.modeKey == modeKey && seen.insert(entry.seed).second) {
            corpusSeeds.push_back(entry.seed);
        }
    }
    if (corpusSeeds.empty()) {
        throw runtime_error("load-corpus found no seeds for mode: " + modeKey);
    }

    if (options.corpusPolicy == CorpusPolicy::APPEND) {
        vector<u32> merged = baseSeeds;
        unordered_set<u32> mergedSeen(merged.begin(), merged.end());
        for (u32 seed : corpusSeeds) {
            if (mergedSeen.insert(seed).second) {
                merged.push_back(seed);
            }
        }
        return merged;
    }
    return corpusSeeds;
}

string trim_copy(const string& text) {
    size_t begin = 0U;
    while (begin < text.size() && isspace(static_cast<unsigned char>(text[begin])) != 0) {
        ++begin;
    }
    size_t end = text.size();
    while (end > begin && isspace(static_cast<unsigned char>(text[end - 1U])) != 0) {
        --end;
    }
    return text.substr(begin, end - begin);
}

vector<u32> parse_u32_csv(const string& text) {
    vector<u32> out;
    size_t start = 0U;
    while (start < text.size()) {
        const size_t comma = text.find(',', start);
        const string token = trim_copy(text.substr(start, comma == string::npos ? string::npos : comma - start));
        if (!token.empty()) {
            out.push_back(static_cast<u32>(stoul(token)));
        }
        if (comma == string::npos) {
            break;
        }
        start = comma + 1U;
    }
    return out;
}

optional<WeightProfile> parse_weight_profile_token(const string& text) {
    if (text == "random") return WeightProfile::RANDOM;
    if (text == "weighted_split_heavy") return WeightProfile::WEIGHTED_SPLIT_HEAVY;
    if (text == "weighted_join_heavy") return WeightProfile::WEIGHTED_JOIN_HEAVY;
    if (text == "weighted_integrate_heavy") return WeightProfile::WEIGHTED_INTEGRATE_HEAVY;
    if (text == "artifact_heavy") return WeightProfile::ARTIFACT_HEAVY;
    if (text == "multiedge_heavy") return WeightProfile::MULTIEDGE_HEAVY;
    return nullopt;
}

optional<PreconditionBiasProfile> parse_precondition_bias_profile_token(const string& text) {
    if (text == "default") return PreconditionBiasProfile::DEFAULT;
    if (text == "balanced") return PreconditionBiasProfile::BALANCED;
    if (text == "split_heavy") return PreconditionBiasProfile::SPLIT_HEAVY;
    if (text == "join_heavy") return PreconditionBiasProfile::JOIN_HEAVY;
    if (text == "integrate_heavy") return PreconditionBiasProfile::INTEGRATE_HEAVY;
    if (text == "artifact_heavy") return PreconditionBiasProfile::ARTIFACT_HEAVY;
    if (text == "structural") return PreconditionBiasProfile::STRUCTURAL;
    return nullopt;
}

optional<ScenarioFamily> parse_scenario_family_token(const string& text) {
    if (text == "random") return ScenarioFamily::RANDOM;
    if (text == "split_ready") return ScenarioFamily::SPLIT_READY;
    if (text == "split_with_boundary_artifact") return ScenarioFamily::SPLIT_WITH_BOUNDARY_ARTIFACT;
    if (text == "split_with_keepOcc_sibling") return ScenarioFamily::SPLIT_WITH_KEEPOCC_SIBLING;
    if (text == "split_with_join_and_integrate") return ScenarioFamily::SPLIT_WITH_JOIN_AND_INTEGRATE;
    if (text == "planner_mixed_targeted") return ScenarioFamily::PLANNER_MIXED_TARGETED;
    if (text == "join_ready") return ScenarioFamily::JOIN_READY;
    if (text == "integrate_ready") return ScenarioFamily::INTEGRATE_READY;
    if (text == "planner_mixed_structural") return ScenarioFamily::PLANNER_MIXED_STRUCTURAL;
    if (text == "split_tie_ready") return ScenarioFamily::SPLIT_TIE_READY;
    if (text == "split_tie_structural") return ScenarioFamily::SPLIT_TIE_STRUCTURAL;
    if (text == "planner_tie_mixed") return ScenarioFamily::PLANNER_TIE_MIXED;
    if (text == "split_tie_symmetric_large") return ScenarioFamily::SPLIT_TIE_SYMMETRIC_LARGE;
    if (text == "planner_tie_mixed_symmetric") return ScenarioFamily::PLANNER_TIE_MIXED_SYMMETRIC;
    if (text == "canonical_collision_probe") return ScenarioFamily::CANONICAL_COLLISION_PROBE;
    if (text == "split_tie_organic_symmetric") return ScenarioFamily::SPLIT_TIE_ORGANIC_SYMMETRIC;
    if (text == "planner_tie_mixed_organic") return ScenarioFamily::PLANNER_TIE_MIXED_ORGANIC;
    if (text == "planner_tie_mixed_organic_compare_ready") return ScenarioFamily::PLANNER_TIE_MIXED_ORGANIC_COMPARE_READY;
    if (text == "automorphism_probe_large") return ScenarioFamily::AUTOMORPHISM_PROBE_LARGE;
    return nullopt;
}

optional<CorpusPolicy> parse_corpus_policy_token(const string& text) {
    if (text == "best") return CorpusPolicy::BEST;
    if (text == "append") return CorpusPolicy::APPEND;
    if (text == "replace") return CorpusPolicy::REPLACE;
    return nullopt;
}

optional<SplitChoicePolicyMode> parse_split_choice_policy_mode_token(const string& text) {
    if (text == "fast") return SplitChoicePolicyMode::FAST;
    if (text == "exact_shadow") return SplitChoicePolicyMode::EXACT_SHADOW;
    if (text == "exact_full") return SplitChoicePolicyMode::EXACT_FULL;
    return nullopt;
}

optional<SplitChoiceCompareMode> parse_split_choice_compare_mode_token(const string& text) {
    if (text == "none") return SplitChoiceCompareMode::NONE;
    if (text == "exact_full") return SplitChoiceCompareMode::EXACT_FULL;
    return nullopt;
}

void apply_mode_to_options(TestOptions& options, const string& modeText) {
    if (const optional<WeightProfile> profile = parse_weight_profile_token(modeText); profile.has_value()) {
        options.weightProfile = *profile;
        options.scenarioFamily = ScenarioFamily::RANDOM;
        return;
    }
    if (const optional<ScenarioFamily> family = parse_scenario_family_token(modeText); family.has_value()) {
        options.scenarioFamily = *family;
        return;
    }
    throw runtime_error("unknown campaign/corpus mode: " + modeText);
}

struct CampaignJsonValue {
    enum class Kind : u8 {
        NULL_VALUE = 0,
        BOOL = 1,
        NUMBER = 2,
        STRING = 3,
        ARRAY = 4,
        OBJECT = 5,
    } kind = Kind::NULL_VALUE;

    bool boolValue = false;
    double numberValue = 0.0;
    string stringValue;
    vector<CampaignJsonValue> arrayValue;
    map<string, CampaignJsonValue> objectValue;
};

class CampaignJsonParser {
public:
    explicit CampaignJsonParser(const string& text) : text_(text) {}

    CampaignJsonValue parse() {
        skip_ws();
        CampaignJsonValue value = parse_value();
        skip_ws();
        if (pos_ != text_.size()) {
            throw runtime_error("unexpected trailing JSON content in campaign config");
        }
        return value;
    }

private:
    const string& text_;
    size_t pos_ = 0U;

    void skip_ws() {
        while (pos_ < text_.size() && isspace(static_cast<unsigned char>(text_[pos_])) != 0) {
            ++pos_;
        }
    }

    bool consume(char ch) {
        skip_ws();
        if (pos_ < text_.size() && text_[pos_] == ch) {
            ++pos_;
            return true;
        }
        return false;
    }

    void expect(char ch) {
        if (!consume(ch)) {
            throw runtime_error(string("expected '") + ch + "' in campaign config JSON");
        }
    }

    bool match_keyword(string_view keyword) {
        skip_ws();
        if (text_.compare(pos_, keyword.size(), keyword) != 0) {
            return false;
        }
        pos_ += keyword.size();
        return true;
    }

    CampaignJsonValue parse_value() {
        skip_ws();
        if (pos_ >= text_.size()) {
            throw runtime_error("unexpected end of campaign config JSON");
        }

        if (text_[pos_] == '{') {
            return parse_object();
        }
        if (text_[pos_] == '[') {
            return parse_array();
        }
        if (text_[pos_] == '"') {
            CampaignJsonValue value;
            value.kind = CampaignJsonValue::Kind::STRING;
            value.stringValue = parse_string();
            return value;
        }
        if (text_[pos_] == '-' || isdigit(static_cast<unsigned char>(text_[pos_])) != 0) {
            return parse_number();
        }
        if (match_keyword("true")) {
            CampaignJsonValue value;
            value.kind = CampaignJsonValue::Kind::BOOL;
            value.boolValue = true;
            return value;
        }
        if (match_keyword("false")) {
            CampaignJsonValue value;
            value.kind = CampaignJsonValue::Kind::BOOL;
            return value;
        }
        if (match_keyword("null")) {
            return {};
        }
        throw runtime_error("invalid JSON value in campaign config");
    }

    CampaignJsonValue parse_object() {
        CampaignJsonValue value;
        value.kind = CampaignJsonValue::Kind::OBJECT;
        expect('{');
        skip_ws();
        if (consume('}')) {
            return value;
        }
        while (true) {
            const string key = parse_string();
            expect(':');
            value.objectValue.emplace(key, parse_value());
            skip_ws();
            if (consume('}')) {
                break;
            }
            expect(',');
        }
        return value;
    }

    CampaignJsonValue parse_array() {
        CampaignJsonValue value;
        value.kind = CampaignJsonValue::Kind::ARRAY;
        expect('[');
        skip_ws();
        if (consume(']')) {
            return value;
        }
        while (true) {
            value.arrayValue.push_back(parse_value());
            skip_ws();
            if (consume(']')) {
                break;
            }
            expect(',');
        }
        return value;
    }

    string parse_string() {
        skip_ws();
        if (pos_ >= text_.size() || text_[pos_] != '"') {
            throw runtime_error("expected JSON string in campaign config");
        }
        ++pos_;
        string out;
        while (pos_ < text_.size()) {
            const char ch = text_[pos_++];
            if (ch == '"') {
                return out;
            }
            if (ch == '\\') {
                if (pos_ >= text_.size()) {
                    throw runtime_error("unterminated JSON escape in campaign config");
                }
                const char esc = text_[pos_++];
                switch (esc) {
                    case '"':
                    case '\\':
                    case '/':
                        out.push_back(esc);
                        break;
                    case 'b':
                        out.push_back('\b');
                        break;
                    case 'f':
                        out.push_back('\f');
                        break;
                    case 'n':
                        out.push_back('\n');
                        break;
                    case 'r':
                        out.push_back('\r');
                        break;
                    case 't':
                        out.push_back('\t');
                        break;
                    default:
                        throw runtime_error("unsupported JSON escape in campaign config");
                }
                continue;
            }
            out.push_back(ch);
        }
        throw runtime_error("unterminated JSON string in campaign config");
    }

    CampaignJsonValue parse_number() {
        skip_ws();
        const size_t begin = pos_;
        if (text_[pos_] == '-') {
            ++pos_;
        }
        while (pos_ < text_.size() && isdigit(static_cast<unsigned char>(text_[pos_])) != 0) {
            ++pos_;
        }
        if (pos_ < text_.size() && text_[pos_] == '.') {
            ++pos_;
            while (pos_ < text_.size() && isdigit(static_cast<unsigned char>(text_[pos_])) != 0) {
                ++pos_;
            }
        }
        CampaignJsonValue value;
        value.kind = CampaignJsonValue::Kind::NUMBER;
        value.numberValue = stod(text_.substr(begin, pos_ - begin));
        return value;
    }
};

const CampaignJsonValue& campaign_json_require_field(
    const CampaignJsonValue& object,
    const string& key
) {
    if (object.kind != CampaignJsonValue::Kind::OBJECT) {
        throw runtime_error("expected campaign config JSON object");
    }
    const auto it = object.objectValue.find(key);
    if (it == object.objectValue.end()) {
        throw runtime_error("missing campaign config field: " + key);
    }
    return it->second;
}

const CampaignJsonValue* campaign_json_find_field(
    const CampaignJsonValue& object,
    const string& key
) {
    if (object.kind != CampaignJsonValue::Kind::OBJECT) {
        return nullptr;
    }
    const auto it = object.objectValue.find(key);
    return it == object.objectValue.end() ? nullptr : &it->second;
}

string campaign_json_require_string(const CampaignJsonValue& value, const string& key) {
    if (value.kind != CampaignJsonValue::Kind::STRING) {
        throw runtime_error("campaign config field must be a string: " + key);
    }
    return value.stringValue;
}

optional<string> campaign_json_optional_string(const CampaignJsonValue& object, const string& key) {
    const CampaignJsonValue* value = campaign_json_find_field(object, key);
    if (value == nullptr || value->kind == CampaignJsonValue::Kind::NULL_VALUE) {
        return nullopt;
    }
    return campaign_json_require_string(*value, key);
}

size_t campaign_json_require_size_t(const CampaignJsonValue& value, const string& key) {
    if (value.kind != CampaignJsonValue::Kind::NUMBER) {
        throw runtime_error("campaign config field must be numeric: " + key);
    }
    return static_cast<size_t>(value.numberValue);
}

optional<size_t> campaign_json_optional_size_t(const CampaignJsonValue& object, const string& key) {
    const CampaignJsonValue* value = campaign_json_find_field(object, key);
    if (value == nullptr || value->kind == CampaignJsonValue::Kind::NULL_VALUE) {
        return nullopt;
    }
    return campaign_json_require_size_t(*value, key);
}

double campaign_json_require_double(const CampaignJsonValue& value, const string& key) {
    if (value.kind != CampaignJsonValue::Kind::NUMBER) {
        throw runtime_error("campaign config field must be numeric: " + key);
    }
    return value.numberValue;
}

optional<double> campaign_json_optional_double(const CampaignJsonValue& object, const string& key) {
    const CampaignJsonValue* value = campaign_json_find_field(object, key);
    if (value == nullptr || value->kind == CampaignJsonValue::Kind::NULL_VALUE) {
        return nullopt;
    }
    return campaign_json_require_double(*value, key);
}

vector<u32> campaign_json_require_u32_array(const CampaignJsonValue& value, const string& key) {
    if (value.kind != CampaignJsonValue::Kind::ARRAY) {
        throw runtime_error("campaign config field must be an array: " + key);
    }
    vector<u32> out;
    out.reserve(value.arrayValue.size());
    for (const CampaignJsonValue& entry : value.arrayValue) {
        if (entry.kind != CampaignJsonValue::Kind::NUMBER) {
            throw runtime_error("campaign config seed list must contain numbers");
        }
        out.push_back(static_cast<u32>(entry.numberValue));
    }
    return out;
}

CampaignConfig load_campaign_config_json(const filesystem::path& path, const string& text) {
    const CampaignJsonValue root = CampaignJsonParser(text).parse();
    CampaignConfig config;

    if (const optional<string> aggregateStatsFile = campaign_json_optional_string(root, "aggregateStatsFile");
        aggregateStatsFile.has_value()) {
        config.aggregateStatsFile = *aggregateStatsFile;
    }
    if (const optional<string> aggregateSummaryFile = campaign_json_optional_string(root, "aggregateSummaryFile");
        aggregateSummaryFile.has_value()) {
        config.aggregateSummaryFile = *aggregateSummaryFile;
    }
    config.saveCorpusDir = campaign_json_optional_string(root, "saveCorpusDir");
    config.loadCorpusDir = campaign_json_optional_string(root, "loadCorpusDir");
    if (const CampaignJsonValue* policyValue = campaign_json_find_field(root, "corpusPolicy"); policyValue != nullptr) {
        const optional<CorpusPolicy> policy = parse_corpus_policy_token(campaign_json_require_string(*policyValue, "corpusPolicy"));
        if (!policy.has_value()) {
            throw runtime_error("invalid campaign corpusPolicy in JSON: " + path.string());
        }
        config.corpusPolicy = *policy;
    }

    const CampaignJsonValue& runsValue = campaign_json_require_field(root, "runs");
    if (runsValue.kind != CampaignJsonValue::Kind::ARRAY) {
        throw runtime_error("campaign config 'runs' must be an array");
    }
    for (const CampaignJsonValue& runValue : runsValue.arrayValue) {
        CampaignRunConfig run;
        if (const optional<string> caseName = campaign_json_optional_string(runValue, "caseName"); caseName.has_value()) {
            run.caseName = *caseName;
        }
        run.mode = campaign_json_require_string(campaign_json_require_field(runValue, "mode"), "mode");
        run.seeds = campaign_json_require_u32_array(campaign_json_require_field(runValue, "seeds"), "seeds");
        run.iters = static_cast<int>(campaign_json_require_size_t(campaign_json_require_field(runValue, "iters"), "iters"));
        run.stepBudget = campaign_json_require_size_t(campaign_json_require_field(runValue, "stepBudget"), "stepBudget");
        run.artifactDir = campaign_json_require_string(campaign_json_require_field(runValue, "artifactDir"), "artifactDir");
        run.statsFile = campaign_json_require_string(campaign_json_require_field(runValue, "statsFile"), "statsFile");
        run.exactCanonicalCap = campaign_json_optional_size_t(runValue, "exactCanonicalCap");
        run.compareSampleRate = campaign_json_optional_double(runValue, "compareSampleRate");
        run.compareBudget = campaign_json_optional_size_t(runValue, "compareBudget");
        if (const optional<string> policyText = campaign_json_optional_string(runValue, "splitChoicePolicyMode");
            policyText.has_value()) {
            const optional<SplitChoicePolicyMode> policy = parse_split_choice_policy_mode_token(*policyText);
            if (!policy.has_value()) {
                throw runtime_error("invalid campaign splitChoicePolicyMode in JSON: " + path.string());
            }
            run.splitChoicePolicyMode = *policy;
        }
        if (const optional<string> compareText = campaign_json_optional_string(runValue, "splitChoiceCompareMode");
            compareText.has_value()) {
            const optional<SplitChoiceCompareMode> compare = parse_split_choice_compare_mode_token(*compareText);
            if (!compare.has_value()) {
                throw runtime_error("invalid campaign splitChoiceCompareMode in JSON: " + path.string());
            }
            run.splitChoiceCompareMode = *compare;
        }
        config.runs.push_back(run);
    }
    if (config.runs.empty()) {
        throw runtime_error("campaign config has no runs: " + path.string());
    }
    return config;
}

CampaignConfig load_campaign_config(const filesystem::path& path) {
    const string text = trim_copy(slurp_text_file(path));
    if (text.empty()) {
        throw runtime_error("failed to read campaign config: " + path.string());
    }

    if (!text.empty() && text.front() == '{') {
        return load_campaign_config_json(path, text);
    }

    CampaignConfig config;
    istringstream iss(text);
    string line;
    while (getline(iss, line)) {
        line = trim_copy(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        if (line.rfind("aggregateStatsFile=", 0) == 0) {
            config.aggregateStatsFile = line.substr(19);
            continue;
        }
        if (line.rfind("aggregateSummaryFile=", 0) == 0) {
            config.aggregateSummaryFile = line.substr(21);
            continue;
        }
        if (line.rfind("saveCorpusDir=", 0) == 0) {
            config.saveCorpusDir = line.substr(14);
            continue;
        }
        if (line.rfind("loadCorpusDir=", 0) == 0) {
            config.loadCorpusDir = line.substr(14);
            continue;
        }
        if (line.rfind("corpusPolicy=", 0) == 0) {
            const optional<CorpusPolicy> policy = parse_corpus_policy_token(line.substr(13));
            if (!policy.has_value()) {
                throw runtime_error("invalid campaign corpusPolicy");
            }
            config.corpusPolicy = *policy;
            continue;
        }
        if (line.rfind("run ", 0) != 0) {
            throw runtime_error("invalid campaign config line: " + line);
        }

        CampaignRunConfig run;
        istringstream ls(line.substr(4));
        string token;
        while (ls >> token) {
            const size_t eq = token.find('=');
            if (eq == string::npos) {
                throw runtime_error("invalid campaign token: " + token);
            }
            const string key = token.substr(0, eq);
            const string value = token.substr(eq + 1U);
            if (key == "caseName") {
                run.caseName = value;
            } else if (key == "mode") {
                run.mode = value;
            } else if (key == "seeds") {
                run.seeds = parse_u32_csv(value);
            } else if (key == "iters") {
                run.iters = stoi(value);
            } else if (key == "stepBudget") {
                run.stepBudget = static_cast<size_t>(stoull(value));
            } else if (key == "artifactDir") {
                run.artifactDir = value;
            } else if (key == "statsFile") {
                run.statsFile = value;
            } else if (key == "exactCanonicalCap") {
                run.exactCanonicalCap = static_cast<size_t>(stoull(value));
            } else if (key == "compareSampleRate") {
                run.compareSampleRate = stod(value);
            } else if (key == "compareBudget") {
                run.compareBudget = static_cast<size_t>(stoull(value));
            } else if (key == "splitChoicePolicyMode") {
                const optional<SplitChoicePolicyMode> policy = parse_split_choice_policy_mode_token(value);
                if (!policy.has_value()) {
                    throw runtime_error("invalid campaign splitChoicePolicyMode");
                }
                run.splitChoicePolicyMode = *policy;
            } else if (key == "splitChoiceCompareMode") {
                const optional<SplitChoiceCompareMode> compare = parse_split_choice_compare_mode_token(value);
                if (!compare.has_value()) {
                    throw runtime_error("invalid campaign splitChoiceCompareMode");
                }
                run.splitChoiceCompareMode = *compare;
            } else {
                throw runtime_error("unknown campaign token: " + key);
            }
        }
        config.runs.push_back(run);
    }

    if (config.runs.empty()) {
        throw runtime_error("campaign config has no runs: " + path.string());
    }
    return config;
}

string checkpoint_chunk_stem(size_t runIndex, size_t seedIndex, u32 seed, int iterStart, int iterCount) {
    ostringstream oss;
    oss << "run" << setw(3) << setfill('0') << runIndex
        << "_seed" << setw(3) << seedIndex
        << "_s" << seed
        << "_iter" << setw(6) << iterStart
        << "_count" << setw(6) << iterCount;
    return oss.str();
}

filesystem::path checkpoint_manifest_path(const filesystem::path& dir) {
    return dir / "latest.chk";
}

filesystem::path checkpoint_chunks_dir(const filesystem::path& dir) {
    return dir / "chunks";
}

unordered_map<string, string> read_key_value_file(const filesystem::path& path) {
    ifstream ifs(path);
    if (!ifs) {
        throw runtime_error("failed to read checkpoint file: " + path.string());
    }
    unordered_map<string, string> values;
    string line;
    while (getline(ifs, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const size_t eq = line.find('=');
        if (eq == string::npos) {
            continue;
        }
        values.emplace(line.substr(0, eq), line.substr(eq + 1U));
    }
    return values;
}

string require_kv_value(
    const unordered_map<string, string>& values,
    const string& key,
    const filesystem::path& path
) {
    const auto it = values.find(key);
    if (it == values.end()) {
        throw runtime_error("missing checkpoint key '" + key + "' in " + path.string());
    }
    return it->second;
}

unordered_map<string, size_t> parse_string_count_object(const string& text) {
    unordered_map<string, size_t> out;
    const string trimmed = trim_copy(text);
    string_view view(trimmed);
    if (view == "{}" || view.empty()) {
        return out;
    }
    if (view.size() < 2U || view.front() != '{' || view.back() != '}') {
        return out;
    }
    size_t pos = 1U;
    while (pos + 1U < view.size()) {
        while (pos < view.size() && (view[pos] == ' ' || view[pos] == ',')) {
            ++pos;
        }
        if (pos >= view.size() || view[pos] == '}') {
            break;
        }
        if (view[pos] != '"') {
            break;
        }
        ++pos;
        const size_t keyBegin = pos;
        while (pos < view.size() && view[pos] != '"') {
            ++pos;
        }
        if (pos >= view.size()) {
            break;
        }
        const string key(view.substr(keyBegin, pos - keyBegin));
        ++pos;
        while (pos < view.size() && (view[pos] == ' ' || view[pos] == ':')) {
            ++pos;
        }
        const size_t valueBegin = pos;
        while (pos < view.size() && isdigit(static_cast<unsigned char>(view[pos])) != 0) {
            ++pos;
        }
        if (valueBegin != pos) {
            out[key] = static_cast<size_t>(stoull(string(view.substr(valueBegin, pos - valueBegin))));
        }
        while (pos < view.size() && view[pos] != ',' && view[pos] != '}') {
            ++pos;
        }
    }
    return out;
}

map<size_t, size_t> parse_numeric_count_object(const string& text) {
    map<size_t, size_t> out;
    const unordered_map<string, size_t> raw = parse_string_count_object(text);
    for (const auto& [key, value] : raw) {
        out[static_cast<size_t>(stoull(key))] += value;
    }
    return out;
}

vector<string> read_text_lines(const filesystem::path& path) {
    ifstream ifs(path);
    if (!ifs) {
        throw runtime_error("failed to read checkpoint file: " + path.string());
    }
    vector<string> lines;
    string line;
    while (getline(ifs, line)) {
        lines.push_back(line);
    }
    return lines;
}

string primitive_hits_text(const array<size_t, 4>& primitiveHits) {
    ostringstream oss;
    for (size_t i = 0; i < primitiveHits.size(); ++i) {
        if (i != 0U) {
            oss << ',';
        }
        oss << primitiveHits[i];
    }
    return oss.str();
}

array<size_t, 4> parse_primitive_hits_text(const string& text) {
    array<size_t, 4> out{{0U, 0U, 0U, 0U}};
    if (text.empty()) {
        return out;
    }
    size_t begin = 0U;
    size_t index = 0U;
    while (begin <= text.size() && index < out.size()) {
        const size_t end = text.find(',', begin);
        const string token = end == string::npos ? text.substr(begin) : text.substr(begin, end - begin);
        if (!token.empty()) {
            out[index] = static_cast<size_t>(stoull(token));
        }
        ++index;
        if (end == string::npos) {
            break;
        }
        begin = end + 1U;
    }
    return out;
}

unordered_map<string, string> parse_summary_record_tokens(const string& text) {
    unordered_map<string, string> out;
    string_view view(text);
    size_t pos = 0U;
    while (pos < view.size()) {
        while (pos < view.size() && view[pos] == ' ') {
            ++pos;
        }
        if (pos >= view.size()) {
            break;
        }
        const size_t eq = view.find('=', pos);
        if (eq == string_view::npos) {
            break;
        }
        const string key(view.substr(pos, eq - pos));
        pos = eq + 1U;
        size_t end = pos;
        while (end < view.size() && view[end] != ' ') {
            ++end;
        }
        out[key] = string(view.substr(pos, end - pos));
        pos = end;
    }
    return out;
}

void write_seed_summary_line(ostream& ofs, const SeedFuzzSummary& seedSummary) {
    ofs << "seed_summary="
        << "seed=" << seedSummary.seed
        << " iterations=" << seedSummary.iterations
        << " primitive_hits=" << primitive_hits_text(seedSummary.primitiveHits)
        << " split_success=" << seedSummary.splitSuccessCount
        << " split_rejected=" << seedSummary.splitRejectedCount
        << " direct_ab_artifact=" << seedSummary.directABArtifactCount
        << " boundary_only_child=" << seedSummary.boundaryOnlyChildCount
        << " split_ready=" << seedSummary.splitReadyCount
        << " join_candidate=" << seedSummary.joinCandidateCount
        << " integrate_candidate=" << seedSummary.integrateCandidateCount
        << " actual_split=" << seedSummary.actualSplitHits
        << " actual_join=" << seedSummary.actualJoinHits
        << " actual_integrate=" << seedSummary.actualIntegrateHits
        << " split_choice_candidate=" << seedSummary.splitChoiceCandidateCount
        << " split_choice_eval=" << seedSummary.splitChoiceEvalCount
        << " split_choice_tie=" << seedSummary.splitChoiceTieCount
        << " split_choice_multiclass=" << seedSummary.splitChoiceMulticlassCount
        << " split_choice_fallback=" << seedSummary.splitChoiceFallbackCount
        << " split_choice_compare_state=" << seedSummary.splitChoiceCompareStateCount
        << " split_ready_state=" << seedSummary.splitReadyStateCount
        << " tie_ready_state=" << seedSummary.tieReadyStateCount
        << " compare_eligible_state=" << seedSummary.compareEligibleStateCount
        << " compare_ineligible_state=" << seedSummary.compareIneligibleStateCount
        << " compare_completed_state=" << seedSummary.compareCompletedStateCount
        << " compare_partial_state=" << seedSummary.comparePartialStateCount
        << " split_choice_exact_full_eval=" << seedSummary.splitChoiceExactFullEvalCount
        << " split_choice_exact_shadow_eval=" << seedSummary.splitChoiceExactShadowEvalCount
        << " split_choice_same_representative=" << seedSummary.splitChoiceSameRepresentativeCount
        << " split_choice_same_semantic_class=" << seedSummary.splitChoiceSameSemanticClassCount
        << " split_choice_same_final_state=" << seedSummary.splitChoiceSameFinalStateCount
        << " split_choice_semantic_disagreement=" << seedSummary.splitChoiceSemanticDisagreementCount
        << " split_choice_cap_hit=" << seedSummary.splitChoiceCapHitCount
        << " split_choice_harmless_compare=" << seedSummary.splitChoiceHarmlessCompareCount
        << " split_choice_trace_only_compare=" << seedSummary.splitChoiceTraceOnlyCompareCount
        << " exact_audited_state=" << seedSummary.exactAuditedStateCount
        << " exact_audited_pair=" << seedSummary.exactAuditedPairCount
        << " fast_vs_exact_class_disagreement=" << seedSummary.fastVsExactClassDisagreementCount
        << " representative_shift=" << seedSummary.representativeShiftCount
        << " representative_shift_same_class=" << seedSummary.representativeShiftSameClassCount
        << " representative_shift_semantic_divergence=" << seedSummary.representativeShiftSemanticDivergenceCount
        << " representative_shift_followup_divergence=" << seedSummary.representativeShiftFollowupDivergenceCount
        << " representative_shift_trace_divergence=" << seedSummary.representativeShiftTraceDivergenceCount
        << " harmless_shift=" << seedSummary.harmlessShiftCount
        << " trace_only_shift=" << seedSummary.traceOnlyShiftCount
        << " semantic_shift=" << seedSummary.semanticShiftCount
        << " compare_candidate_enumeration_ns=" << seedSummary.compareCandidateEnumerationNanos
        << " compare_exact_shadow_evaluation_ns=" << seedSummary.compareExactShadowEvaluationNanos
        << " compare_exact_full_evaluation_ns=" << seedSummary.compareExactFullEvaluationNanos
        << " compare_canonicalization_ns=" << seedSummary.compareCanonicalizationNanos
        << " compare_multiclass_catalog_ns=" << seedSummary.compareMulticlassCatalogNanos
        << " compare_state_hash_cache_hit_count=" << seedSummary.compareStateHashCacheHitCount
        << " compare_state_hash_cache_miss_count=" << seedSummary.compareStateHashCacheMissCount
        << " compare_candidate_evaluation_cache_hit_count=" << seedSummary.compareCandidateEvaluationCacheHitCount
        << " compare_candidate_evaluation_cache_miss_count=" << seedSummary.compareCandidateEvaluationCacheMissCount
        << " compare_exact_canonical_cache_hit_count=" << seedSummary.compareExactCanonicalCacheHitCount
        << " compare_exact_canonical_cache_miss_count=" << seedSummary.compareExactCanonicalCacheMissCount
        << " exact_audit_skipped_cap=" << seedSummary.exactAuditSkippedCapCount
        << " exact_audit_skipped_budget=" << seedSummary.exactAuditSkippedBudgetCount
        << " exact_audit_skipped_sample=" << seedSummary.exactAuditSkippedSampleCount
        << " exact_audit_skipped_family=" << seedSummary.exactAuditSkippedFamilyCount
        << " exact_audit_skipped_non_tie=" << seedSummary.exactAuditSkippedNonTieCount
        << " first_split_iter=" << seedSummary.firstSplitIter
        << " first_join_iter=" << seedSummary.firstJoinIter
        << " first_integrate_iter=" << seedSummary.firstIntegrateIter
        << " first_split_choice_tie_iter=" << seedSummary.firstSplitChoiceTieIter
        << " multi_edge_count=" << seedSummary.multiEdgeCount
        << " reducer_invocation_count=" << seedSummary.reducerInvocationCount
        << " split_choice_equiv_class_count_histogram=" << json_object_from_numeric_map(seedSummary.splitChoiceEquivClassCountHistogram)
        << " compare_ineligible_reason_histogram=" << json_object_from_map(seedSummary.compareIneligibleReasonHistogram)
        << " trace_prefix_histogram=" << json_object_from_map(seedSummary.tracePrefixHistogram)
        << " primitive_multiset_histogram=" << json_object_from_map(seedSummary.primitiveMultisetHistogram)
        << '\n';
}

void write_fuzz_summary_body(ostream& ofs, const FuzzStats& stats) {
    ofs << "case=" << stats.caseName << '\n';
    ofs << "weight_profile=" << stats.weightProfile << '\n';
    ofs << "precondition_bias_profile=" << stats.preconditionBiasProfile << '\n';
    ofs << "scenario_family=" << stats.scenarioFamily << '\n';
    ofs << "split_choice_policy_mode=" << stats.splitChoicePolicyMode << '\n';
    ofs << "split_choice_compare_mode=" << stats.splitChoiceCompareMode << '\n';
    ofs << "bias_split=" << stats.biasSplit << '\n';
    ofs << "bias_join=" << stats.biasJoin << '\n';
    ofs << "bias_integrate=" << stats.biasIntegrate << '\n';
    ofs << "primitive_hits=" << primitive_hits_text(stats.primitiveHits) << '\n';
    ofs << "iterations=" << stats.totalIterations << '\n';
    ofs << "split_success_count=" << stats.splitSuccessCount << '\n';
    ofs << "split_rejected_count=" << stats.splitRejectedCount << '\n';
    ofs << "direct_ab_artifact_count=" << stats.directABArtifactCount << '\n';
    ofs << "boundary_only_child_count=" << stats.boundaryOnlyChildCount << '\n';
    ofs << "split_ready_count=" << stats.splitReadyCount << '\n';
    ofs << "join_candidate_count=" << stats.joinCandidateCount << '\n';
    ofs << "integrate_candidate_count=" << stats.integrateCandidateCount << '\n';
    ofs << "actual_split_hits=" << stats.actualSplitHits << '\n';
    ofs << "actual_join_hits=" << stats.actualJoinHits << '\n';
    ofs << "actual_integrate_hits=" << stats.actualIntegrateHits << '\n';
    ofs << "split_choice_candidate_count=" << stats.splitChoiceCandidateCount << '\n';
    ofs << "split_choice_eval_count=" << stats.splitChoiceEvalCount << '\n';
    ofs << "split_choice_tie_count=" << stats.splitChoiceTieCount << '\n';
    ofs << "split_choice_multiclass_count=" << stats.splitChoiceMulticlassCount << '\n';
    ofs << "split_choice_fallback_count=" << stats.splitChoiceFallbackCount << '\n';
    ofs << "split_choice_compare_state_count=" << stats.splitChoiceCompareStateCount << '\n';
    ofs << "split_ready_state_count=" << stats.splitReadyStateCount << '\n';
    ofs << "tie_ready_state_count=" << stats.tieReadyStateCount << '\n';
    ofs << "compare_eligible_state_count=" << stats.compareEligibleStateCount << '\n';
    ofs << "compare_ineligible_state_count=" << stats.compareIneligibleStateCount << '\n';
    ofs << "compare_completed_state_count=" << stats.compareCompletedStateCount << '\n';
    ofs << "compare_partial_state_count=" << stats.comparePartialStateCount << '\n';
    ofs << "split_choice_exact_full_eval_count=" << stats.splitChoiceExactFullEvalCount << '\n';
    ofs << "split_choice_exact_shadow_eval_count=" << stats.splitChoiceExactShadowEvalCount << '\n';
    ofs << "split_choice_same_representative_count=" << stats.splitChoiceSameRepresentativeCount << '\n';
    ofs << "split_choice_same_semantic_class_count=" << stats.splitChoiceSameSemanticClassCount << '\n';
    ofs << "split_choice_same_final_state_count=" << stats.splitChoiceSameFinalStateCount << '\n';
    ofs << "split_choice_semantic_disagreement_count=" << stats.splitChoiceSemanticDisagreementCount << '\n';
    ofs << "split_choice_cap_hit_count=" << stats.splitChoiceCapHitCount << '\n';
    ofs << "split_choice_harmless_compare_count=" << stats.splitChoiceHarmlessCompareCount << '\n';
    ofs << "split_choice_trace_only_compare_count=" << stats.splitChoiceTraceOnlyCompareCount << '\n';
    ofs << "exact_audited_state_count=" << stats.exactAuditedStateCount << '\n';
    ofs << "exact_audited_pair_count=" << stats.exactAuditedPairCount << '\n';
    ofs << "fast_vs_exact_class_disagreement_count=" << stats.fastVsExactClassDisagreementCount << '\n';
    ofs << "representative_shift_count=" << stats.representativeShiftCount << '\n';
    ofs << "representative_shift_same_class_count=" << stats.representativeShiftSameClassCount << '\n';
    ofs << "representative_shift_semantic_divergence_count=" << stats.representativeShiftSemanticDivergenceCount << '\n';
    ofs << "representative_shift_followup_divergence_count=" << stats.representativeShiftFollowupDivergenceCount << '\n';
    ofs << "representative_shift_trace_divergence_count=" << stats.representativeShiftTraceDivergenceCount << '\n';
    ofs << "harmless_shift_count=" << stats.harmlessShiftCount << '\n';
    ofs << "trace_only_shift_count=" << stats.traceOnlyShiftCount << '\n';
    ofs << "semantic_shift_count=" << stats.semanticShiftCount << '\n';
    ofs << "compare_candidate_enumeration_ns=" << stats.compareCandidateEnumerationNanos << '\n';
    ofs << "compare_exact_shadow_evaluation_ns=" << stats.compareExactShadowEvaluationNanos << '\n';
    ofs << "compare_exact_full_evaluation_ns=" << stats.compareExactFullEvaluationNanos << '\n';
    ofs << "compare_canonicalization_ns=" << stats.compareCanonicalizationNanos << '\n';
    ofs << "compare_multiclass_catalog_ns=" << stats.compareMulticlassCatalogNanos << '\n';
    ofs << "compare_state_hash_cache_hit_count=" << stats.compareStateHashCacheHitCount << '\n';
    ofs << "compare_state_hash_cache_miss_count=" << stats.compareStateHashCacheMissCount << '\n';
    ofs << "compare_candidate_evaluation_cache_hit_count=" << stats.compareCandidateEvaluationCacheHitCount << '\n';
    ofs << "compare_candidate_evaluation_cache_miss_count=" << stats.compareCandidateEvaluationCacheMissCount << '\n';
    ofs << "compare_exact_canonical_cache_hit_count=" << stats.compareExactCanonicalCacheHitCount << '\n';
    ofs << "compare_exact_canonical_cache_miss_count=" << stats.compareExactCanonicalCacheMissCount << '\n';
    ofs << "exact_audit_skipped_cap_count=" << stats.exactAuditSkippedCapCount << '\n';
    ofs << "exact_audit_skipped_budget_count=" << stats.exactAuditSkippedBudgetCount << '\n';
    ofs << "exact_audit_skipped_sample_count=" << stats.exactAuditSkippedSampleCount << '\n';
    ofs << "exact_audit_skipped_family_count=" << stats.exactAuditSkippedFamilyCount << '\n';
    ofs << "exact_audit_skipped_non_tie_count=" << stats.exactAuditSkippedNonTieCount << '\n';
    ofs << "first_split_iter=" << stats.firstSplitIter << '\n';
    ofs << "first_join_iter=" << stats.firstJoinIter << '\n';
    ofs << "first_integrate_iter=" << stats.firstIntegrateIter << '\n';
    ofs << "first_split_choice_tie_iter=" << stats.firstSplitChoiceTieIter << '\n';
    ofs << "multi_edge_count=" << stats.multiEdgeCount << '\n';
    ofs << "reducer_invocation_count=" << stats.reducerInvocationCount << '\n';
    ofs << "multiclass_catalog_cluster_count=" << stats.multiclassCatalogClusterCount << '\n';
    ofs << "multiclass_harmless_cluster_count=" << stats.multiclassHarmlessClusterCount << '\n';
    ofs << "multiclass_trace_only_cluster_count=" << stats.multiclassTraceOnlyClusterCount << '\n';
    ofs << "multiclass_semantic_shift_cluster_count=" << stats.multiclassSemanticShiftClusterCount << '\n';
    ofs << "multiclass_catalog_histogram=" << json_object_from_map(stats.multiclassCatalogHistogram) << '\n';
    ofs << "split_choice_equiv_class_count_histogram="
        << json_object_from_numeric_map(stats.splitChoiceEquivClassCountHistogram) << '\n';
    ofs << "compare_ineligible_reason_histogram=" << json_object_from_map(stats.compareIneligibleReasonHistogram) << '\n';
    ofs << "sequence_histogram=" << json_object_from_map(stats.sequenceHistogram) << '\n';
    ofs << "trace_prefix_histogram=" << json_object_from_map(stats.tracePrefixHistogram) << '\n';
    ofs << "primitive_multiset_histogram=" << json_object_from_map(stats.primitiveMultisetHistogram) << '\n';
    ofs << "oracle_mismatch_count=" << json_object_from_map(stats.oracleMismatchCount) << '\n';
    ofs << "split_conversion=" << json_number(safe_ratio(stats.actualSplitHits, stats.splitReadyCount)) << '\n';
    ofs << "join_conversion=" << json_number(safe_ratio(stats.actualJoinHits, stats.joinCandidateCount)) << '\n';
    ofs << "integrate_conversion=" << json_number(safe_ratio(stats.actualIntegrateHits, stats.integrateCandidateCount)) << '\n';
    ofs << "unique_trace_prefix_count=" << stats.tracePrefixHistogram.size() << '\n';
    ofs << "unique_primitive_multiset_count=" << stats.primitiveMultisetHistogram.size() << '\n';
    ofs << "isolate_heavy_ratio=" << json_number(safe_ratio(stats.primitiveHits[0], total_primitive_hits(stats.primitiveHits))) << '\n';
    ofs << "split_hit_density=" << json_number(safe_ratio(stats.actualSplitHits, stats.totalIterations)) << '\n';
    ofs << "join_hit_density=" << json_number(safe_ratio(stats.actualJoinHits, stats.totalIterations)) << '\n';
    ofs << "integrate_hit_density=" << json_number(safe_ratio(stats.actualIntegrateHits, stats.totalIterations)) << '\n';
    for (const SeedFuzzSummary& seedSummary : stats.seedSummaries) {
        write_seed_summary_line(ofs, seedSummary);
    }
}

FuzzStats load_fuzz_stats_from_summary_file(const filesystem::path& path) {
    const unordered_map<string, string> values = read_key_value_file(path);
    const vector<string> lines = read_text_lines(path);
    FuzzStats stats;
    auto read_size = [&](const string& key) -> size_t {
        const auto it = values.find(key);
        return it == values.end() ? 0U : static_cast<size_t>(stoull(it->second));
    };
    auto read_int = [&](const string& key) -> int {
        const auto it = values.find(key);
        return it == values.end() ? 0 : stoi(it->second);
    };
    auto read_string = [&](const string& key) -> string {
        const auto it = values.find(key);
        return it == values.end() ? string{} : it->second;
    };

    stats.caseName = read_string("case");
    stats.weightProfile = read_string("weight_profile");
    stats.preconditionBiasProfile = read_string("precondition_bias_profile");
    stats.scenarioFamily = read_string("scenario_family");
    stats.splitChoicePolicyMode = read_string("split_choice_policy_mode");
    stats.splitChoiceCompareMode = read_string("split_choice_compare_mode");
    stats.biasSplit = read_int("bias_split");
    stats.biasJoin = read_int("bias_join");
    stats.biasIntegrate = read_int("bias_integrate");
    stats.primitiveHits = parse_primitive_hits_text(read_string("primitive_hits"));
    stats.totalIterations = read_size("iterations");
    stats.splitSuccessCount = read_size("split_success_count");
    stats.splitRejectedCount = read_size("split_rejected_count");
    stats.directABArtifactCount = read_size("direct_ab_artifact_count");
    stats.boundaryOnlyChildCount = read_size("boundary_only_child_count");
    stats.splitReadyCount = read_size("split_ready_count");
    stats.joinCandidateCount = read_size("join_candidate_count");
    stats.integrateCandidateCount = read_size("integrate_candidate_count");
    stats.actualSplitHits = read_size("actual_split_hits");
    stats.actualJoinHits = read_size("actual_join_hits");
    stats.actualIntegrateHits = read_size("actual_integrate_hits");
    stats.splitChoiceCandidateCount = read_size("split_choice_candidate_count");
    stats.splitChoiceEvalCount = read_size("split_choice_eval_count");
    stats.splitChoiceTieCount = read_size("split_choice_tie_count");
    stats.splitChoiceMulticlassCount = read_size("split_choice_multiclass_count");
    stats.splitChoiceFallbackCount = read_size("split_choice_fallback_count");
    stats.splitChoiceCompareStateCount = read_size("split_choice_compare_state_count");
    stats.splitReadyStateCount = read_size("split_ready_state_count");
    stats.tieReadyStateCount = read_size("tie_ready_state_count");
    stats.compareEligibleStateCount = read_size("compare_eligible_state_count");
    stats.compareIneligibleStateCount = read_size("compare_ineligible_state_count");
    stats.compareCompletedStateCount = read_size("compare_completed_state_count");
    stats.comparePartialStateCount = read_size("compare_partial_state_count");
    stats.splitChoiceExactFullEvalCount = read_size("split_choice_exact_full_eval_count");
    stats.splitChoiceExactShadowEvalCount = read_size("split_choice_exact_shadow_eval_count");
    stats.splitChoiceSameRepresentativeCount = read_size("split_choice_same_representative_count");
    stats.splitChoiceSameSemanticClassCount = read_size("split_choice_same_semantic_class_count");
    stats.splitChoiceSameFinalStateCount = read_size("split_choice_same_final_state_count");
    stats.splitChoiceSemanticDisagreementCount = read_size("split_choice_semantic_disagreement_count");
    stats.splitChoiceCapHitCount = read_size("split_choice_cap_hit_count");
    stats.splitChoiceHarmlessCompareCount = read_size("split_choice_harmless_compare_count");
    stats.splitChoiceTraceOnlyCompareCount = read_size("split_choice_trace_only_compare_count");
    stats.exactAuditedStateCount = read_size("exact_audited_state_count");
    stats.exactAuditedPairCount = read_size("exact_audited_pair_count");
    stats.fastVsExactClassDisagreementCount = read_size("fast_vs_exact_class_disagreement_count");
    stats.representativeShiftCount = read_size("representative_shift_count");
    stats.representativeShiftSameClassCount = read_size("representative_shift_same_class_count");
    stats.representativeShiftSemanticDivergenceCount = read_size("representative_shift_semantic_divergence_count");
    stats.representativeShiftFollowupDivergenceCount = read_size("representative_shift_followup_divergence_count");
    stats.representativeShiftTraceDivergenceCount = read_size("representative_shift_trace_divergence_count");
    stats.harmlessShiftCount = read_size("harmless_shift_count");
    stats.traceOnlyShiftCount = read_size("trace_only_shift_count");
    stats.semanticShiftCount = read_size("semantic_shift_count");
    stats.compareCandidateEnumerationNanos = read_size("compare_candidate_enumeration_ns");
    stats.compareExactShadowEvaluationNanos = read_size("compare_exact_shadow_evaluation_ns");
    stats.compareExactFullEvaluationNanos = read_size("compare_exact_full_evaluation_ns");
    stats.compareCanonicalizationNanos = read_size("compare_canonicalization_ns");
    stats.compareMulticlassCatalogNanos = read_size("compare_multiclass_catalog_ns");
    stats.compareStateHashCacheHitCount = read_size("compare_state_hash_cache_hit_count");
    stats.compareStateHashCacheMissCount = read_size("compare_state_hash_cache_miss_count");
    stats.compareCandidateEvaluationCacheHitCount = read_size("compare_candidate_evaluation_cache_hit_count");
    stats.compareCandidateEvaluationCacheMissCount = read_size("compare_candidate_evaluation_cache_miss_count");
    stats.compareExactCanonicalCacheHitCount = read_size("compare_exact_canonical_cache_hit_count");
    stats.compareExactCanonicalCacheMissCount = read_size("compare_exact_canonical_cache_miss_count");
    stats.exactAuditSkippedCapCount = read_size("exact_audit_skipped_cap_count");
    stats.exactAuditSkippedBudgetCount = read_size("exact_audit_skipped_budget_count");
    stats.exactAuditSkippedSampleCount = read_size("exact_audit_skipped_sample_count");
    stats.exactAuditSkippedFamilyCount = read_size("exact_audit_skipped_family_count");
    stats.exactAuditSkippedNonTieCount = read_size("exact_audit_skipped_non_tie_count");
    stats.firstSplitIter = read_int("first_split_iter");
    stats.firstJoinIter = read_int("first_join_iter");
    stats.firstIntegrateIter = read_int("first_integrate_iter");
    stats.firstSplitChoiceTieIter = read_int("first_split_choice_tie_iter");
    stats.multiEdgeCount = read_size("multi_edge_count");
    stats.reducerInvocationCount = read_size("reducer_invocation_count");
    stats.multiclassCatalogClusterCount = read_size("multiclass_catalog_cluster_count");
    stats.multiclassHarmlessClusterCount = read_size("multiclass_harmless_cluster_count");
    stats.multiclassTraceOnlyClusterCount = read_size("multiclass_trace_only_cluster_count");
    stats.multiclassSemanticShiftClusterCount = read_size("multiclass_semantic_shift_cluster_count");
    if (const auto it = values.find("multiclass_catalog_histogram"); it != values.end()) {
        stats.multiclassCatalogHistogram = parse_string_count_object(it->second);
    }
    if (const auto it = values.find("split_choice_equiv_class_count_histogram"); it != values.end()) {
        stats.splitChoiceEquivClassCountHistogram = parse_numeric_count_object(it->second);
    }
    if (const auto it = values.find("compare_ineligible_reason_histogram"); it != values.end()) {
        stats.compareIneligibleReasonHistogram = parse_string_count_object(it->second);
    }
    if (const auto it = values.find("sequence_histogram"); it != values.end()) {
        stats.sequenceHistogram = parse_string_count_object(it->second);
    }
    if (const auto it = values.find("trace_prefix_histogram"); it != values.end()) {
        stats.tracePrefixHistogram = parse_string_count_object(it->second);
    }
    if (const auto it = values.find("primitive_multiset_histogram"); it != values.end()) {
        stats.primitiveMultisetHistogram = parse_string_count_object(it->second);
    }
    if (const auto it = values.find("oracle_mismatch_count"); it != values.end()) {
        stats.oracleMismatchCount = parse_string_count_object(it->second);
    }

    auto load_seed_summary = [&](const string& recordText) {
        const unordered_map<string, string> record = parse_summary_record_tokens(recordText);
        const auto seedIt = record.find("seed");
        if (seedIt == record.end()) {
            return;
        }
        auto read_seed_size = [&](const string& key) -> size_t {
            const auto it = record.find(key);
            return it == record.end() ? 0U : static_cast<size_t>(stoull(it->second));
        };
        auto read_seed_int = [&](const string& key) -> int {
            const auto it = record.find(key);
            return it == record.end() ? 0 : stoi(it->second);
        };

        SeedFuzzSummary seedSummary;
        seedSummary.seed = static_cast<u32>(stoul(seedIt->second));
        if (const auto it = record.find("primitive_hits"); it != record.end()) {
            seedSummary.primitiveHits = parse_primitive_hits_text(it->second);
        }
        seedSummary.iterations = read_seed_size("iterations");
        seedSummary.splitSuccessCount = read_seed_size("split_success");
        seedSummary.splitRejectedCount = read_seed_size("split_rejected");
        seedSummary.directABArtifactCount = read_seed_size("direct_ab_artifact");
        seedSummary.boundaryOnlyChildCount = read_seed_size("boundary_only_child");
        seedSummary.splitReadyCount = read_seed_size("split_ready");
        seedSummary.joinCandidateCount = read_seed_size("join_candidate");
        seedSummary.integrateCandidateCount = read_seed_size("integrate_candidate");
        seedSummary.actualSplitHits = read_seed_size("actual_split");
        seedSummary.actualJoinHits = read_seed_size("actual_join");
        seedSummary.actualIntegrateHits = read_seed_size("actual_integrate");
        seedSummary.splitChoiceCandidateCount = read_seed_size("split_choice_candidate");
        seedSummary.splitChoiceEvalCount = read_seed_size("split_choice_eval");
        seedSummary.splitChoiceTieCount = read_seed_size("split_choice_tie");
        seedSummary.splitChoiceMulticlassCount = read_seed_size("split_choice_multiclass");
        seedSummary.splitChoiceFallbackCount = read_seed_size("split_choice_fallback");
        seedSummary.exactAuditedStateCount = read_seed_size("exact_audited_state");
        seedSummary.exactAuditedPairCount = read_seed_size("exact_audited_pair");
        seedSummary.fastVsExactClassDisagreementCount = read_seed_size("fast_vs_exact_class_disagreement");
        seedSummary.splitChoiceCompareStateCount = read_seed_size("split_choice_compare_state");
        seedSummary.splitReadyStateCount = read_seed_size("split_ready_state");
        seedSummary.tieReadyStateCount = read_seed_size("tie_ready_state");
        seedSummary.compareEligibleStateCount = read_seed_size("compare_eligible_state");
        seedSummary.compareIneligibleStateCount = read_seed_size("compare_ineligible_state");
        seedSummary.compareCompletedStateCount = read_seed_size("compare_completed_state");
        seedSummary.comparePartialStateCount = read_seed_size("compare_partial_state");
        seedSummary.splitChoiceExactFullEvalCount = read_seed_size("split_choice_exact_full_eval");
        seedSummary.splitChoiceExactShadowEvalCount = read_seed_size("split_choice_exact_shadow_eval");
        seedSummary.splitChoiceSameRepresentativeCount = read_seed_size("split_choice_same_representative");
        seedSummary.splitChoiceSameSemanticClassCount = read_seed_size("split_choice_same_semantic_class");
        seedSummary.splitChoiceSameFinalStateCount = read_seed_size("split_choice_same_final_state");
        seedSummary.splitChoiceSemanticDisagreementCount = read_seed_size("split_choice_semantic_disagreement");
        seedSummary.splitChoiceCapHitCount = read_seed_size("split_choice_cap_hit");
        seedSummary.splitChoiceHarmlessCompareCount = read_seed_size("split_choice_harmless_compare");
        seedSummary.splitChoiceTraceOnlyCompareCount = read_seed_size("split_choice_trace_only_compare");
        seedSummary.representativeShiftCount = read_seed_size("representative_shift");
        seedSummary.representativeShiftSameClassCount = read_seed_size("representative_shift_same_class");
        seedSummary.representativeShiftSemanticDivergenceCount = read_seed_size("representative_shift_semantic_divergence");
        seedSummary.representativeShiftFollowupDivergenceCount = read_seed_size("representative_shift_followup_divergence");
        seedSummary.representativeShiftTraceDivergenceCount = read_seed_size("representative_shift_trace_divergence");
        seedSummary.harmlessShiftCount = read_seed_size("harmless_shift");
        seedSummary.traceOnlyShiftCount = read_seed_size("trace_only_shift");
        seedSummary.semanticShiftCount = read_seed_size("semantic_shift");
        seedSummary.compareCandidateEnumerationNanos = read_seed_size("compare_candidate_enumeration_ns");
        seedSummary.compareExactShadowEvaluationNanos = read_seed_size("compare_exact_shadow_evaluation_ns");
        seedSummary.compareExactFullEvaluationNanos = read_seed_size("compare_exact_full_evaluation_ns");
        seedSummary.compareCanonicalizationNanos = read_seed_size("compare_canonicalization_ns");
        seedSummary.compareMulticlassCatalogNanos = read_seed_size("compare_multiclass_catalog_ns");
        seedSummary.compareStateHashCacheHitCount = read_seed_size("compare_state_hash_cache_hit_count");
        seedSummary.compareStateHashCacheMissCount = read_seed_size("compare_state_hash_cache_miss_count");
        seedSummary.compareCandidateEvaluationCacheHitCount = read_seed_size("compare_candidate_evaluation_cache_hit_count");
        seedSummary.compareCandidateEvaluationCacheMissCount = read_seed_size("compare_candidate_evaluation_cache_miss_count");
        seedSummary.compareExactCanonicalCacheHitCount = read_seed_size("compare_exact_canonical_cache_hit_count");
        seedSummary.compareExactCanonicalCacheMissCount = read_seed_size("compare_exact_canonical_cache_miss_count");
        seedSummary.exactAuditSkippedCapCount = read_seed_size("exact_audit_skipped_cap");
        seedSummary.exactAuditSkippedBudgetCount = read_seed_size("exact_audit_skipped_budget");
        seedSummary.exactAuditSkippedSampleCount = read_seed_size("exact_audit_skipped_sample");
        seedSummary.exactAuditSkippedFamilyCount = read_seed_size("exact_audit_skipped_family");
        seedSummary.exactAuditSkippedNonTieCount = read_seed_size("exact_audit_skipped_non_tie");
        seedSummary.firstSplitIter = read_seed_int("first_split_iter");
        seedSummary.firstJoinIter = read_seed_int("first_join_iter");
        seedSummary.firstIntegrateIter = read_seed_int("first_integrate_iter");
        seedSummary.firstSplitChoiceTieIter = read_seed_int("first_split_choice_tie_iter");
        seedSummary.multiEdgeCount = read_seed_size("multi_edge_count");
        seedSummary.reducerInvocationCount = read_seed_size("reducer_invocation_count");
        if (const auto it = record.find("split_choice_equiv_class_count_histogram"); it != record.end()) {
            seedSummary.splitChoiceEquivClassCountHistogram = parse_numeric_count_object(it->second);
        }
        if (const auto it = record.find("compare_ineligible_reason_histogram"); it != record.end()) {
            seedSummary.compareIneligibleReasonHistogram = parse_string_count_object(it->second);
        }
        if (const auto it = record.find("trace_prefix_histogram"); it != record.end()) {
            seedSummary.tracePrefixHistogram = parse_string_count_object(it->second);
        }
        if (const auto it = record.find("primitive_multiset_histogram"); it != record.end()) {
            seedSummary.primitiveMultisetHistogram = parse_string_count_object(it->second);
        }
        stats.seedSummaries.push_back(std::move(seedSummary));
    };

    for (const string& line : lines) {
        if (line.rfind("seed_summary=", 0) == 0U) {
            load_seed_summary(line.substr(string("seed_summary=").size()));
        } else if (line.rfind("seed=", 0) == 0U) {
            load_seed_summary(line);
        }
    }
    return stats;
}

void merge_seed_fuzz_summary_into(SeedFuzzSummary& dst, const SeedFuzzSummary& src) {
    dst.iterations += src.iterations;
    for (size_t i = 0; i < dst.primitiveHits.size(); ++i) {
        dst.primitiveHits[i] += src.primitiveHits[i];
    }
    dst.splitSuccessCount += src.splitSuccessCount;
    dst.splitRejectedCount += src.splitRejectedCount;
    dst.directABArtifactCount += src.directABArtifactCount;
    dst.boundaryOnlyChildCount += src.boundaryOnlyChildCount;
    dst.splitReadyCount += src.splitReadyCount;
    dst.joinCandidateCount += src.joinCandidateCount;
    dst.integrateCandidateCount += src.integrateCandidateCount;
    dst.actualSplitHits += src.actualSplitHits;
    dst.actualJoinHits += src.actualJoinHits;
    dst.actualIntegrateHits += src.actualIntegrateHits;
    dst.splitChoiceCandidateCount += src.splitChoiceCandidateCount;
    dst.splitChoiceEvalCount += src.splitChoiceEvalCount;
    dst.splitChoiceTieCount += src.splitChoiceTieCount;
    dst.splitChoiceMulticlassCount += src.splitChoiceMulticlassCount;
    dst.splitChoiceFallbackCount += src.splitChoiceFallbackCount;
    dst.exactAuditedStateCount += src.exactAuditedStateCount;
    dst.exactAuditedPairCount += src.exactAuditedPairCount;
    dst.fastVsExactClassDisagreementCount += src.fastVsExactClassDisagreementCount;
    dst.splitChoiceCompareStateCount += src.splitChoiceCompareStateCount;
    dst.splitReadyStateCount += src.splitReadyStateCount;
    dst.tieReadyStateCount += src.tieReadyStateCount;
    dst.compareEligibleStateCount += src.compareEligibleStateCount;
    dst.compareIneligibleStateCount += src.compareIneligibleStateCount;
    dst.compareCompletedStateCount += src.compareCompletedStateCount;
    dst.comparePartialStateCount += src.comparePartialStateCount;
    dst.splitChoiceExactFullEvalCount += src.splitChoiceExactFullEvalCount;
    dst.splitChoiceExactShadowEvalCount += src.splitChoiceExactShadowEvalCount;
    dst.splitChoiceSameRepresentativeCount += src.splitChoiceSameRepresentativeCount;
    dst.splitChoiceSameSemanticClassCount += src.splitChoiceSameSemanticClassCount;
    dst.splitChoiceSameFinalStateCount += src.splitChoiceSameFinalStateCount;
    dst.splitChoiceSemanticDisagreementCount += src.splitChoiceSemanticDisagreementCount;
    dst.splitChoiceCapHitCount += src.splitChoiceCapHitCount;
    dst.splitChoiceHarmlessCompareCount += src.splitChoiceHarmlessCompareCount;
    dst.splitChoiceTraceOnlyCompareCount += src.splitChoiceTraceOnlyCompareCount;
    dst.representativeShiftCount += src.representativeShiftCount;
    dst.representativeShiftSameClassCount += src.representativeShiftSameClassCount;
    dst.representativeShiftSemanticDivergenceCount += src.representativeShiftSemanticDivergenceCount;
    dst.representativeShiftFollowupDivergenceCount += src.representativeShiftFollowupDivergenceCount;
    dst.representativeShiftTraceDivergenceCount += src.representativeShiftTraceDivergenceCount;
    dst.harmlessShiftCount += src.harmlessShiftCount;
    dst.traceOnlyShiftCount += src.traceOnlyShiftCount;
    dst.semanticShiftCount += src.semanticShiftCount;
    dst.compareCandidateEnumerationNanos += src.compareCandidateEnumerationNanos;
    dst.compareExactShadowEvaluationNanos += src.compareExactShadowEvaluationNanos;
    dst.compareExactFullEvaluationNanos += src.compareExactFullEvaluationNanos;
    dst.compareCanonicalizationNanos += src.compareCanonicalizationNanos;
    dst.compareMulticlassCatalogNanos += src.compareMulticlassCatalogNanos;
    dst.compareStateHashCacheHitCount += src.compareStateHashCacheHitCount;
    dst.compareStateHashCacheMissCount += src.compareStateHashCacheMissCount;
    dst.compareCandidateEvaluationCacheHitCount += src.compareCandidateEvaluationCacheHitCount;
    dst.compareCandidateEvaluationCacheMissCount += src.compareCandidateEvaluationCacheMissCount;
    dst.compareExactCanonicalCacheHitCount += src.compareExactCanonicalCacheHitCount;
    dst.compareExactCanonicalCacheMissCount += src.compareExactCanonicalCacheMissCount;
    dst.exactAuditSkippedCapCount += src.exactAuditSkippedCapCount;
    dst.exactAuditSkippedBudgetCount += src.exactAuditSkippedBudgetCount;
    dst.exactAuditSkippedSampleCount += src.exactAuditSkippedSampleCount;
    dst.exactAuditSkippedFamilyCount += src.exactAuditSkippedFamilyCount;
    dst.exactAuditSkippedNonTieCount += src.exactAuditSkippedNonTieCount;
    if (dst.firstSplitIter < 0 || (src.firstSplitIter >= 0 && src.firstSplitIter < dst.firstSplitIter)) {
        dst.firstSplitIter = src.firstSplitIter;
    }
    if (dst.firstJoinIter < 0 || (src.firstJoinIter >= 0 && src.firstJoinIter < dst.firstJoinIter)) {
        dst.firstJoinIter = src.firstJoinIter;
    }
    if (dst.firstIntegrateIter < 0 || (src.firstIntegrateIter >= 0 && src.firstIntegrateIter < dst.firstIntegrateIter)) {
        dst.firstIntegrateIter = src.firstIntegrateIter;
    }
    if (dst.firstSplitChoiceTieIter < 0 ||
        (src.firstSplitChoiceTieIter >= 0 && src.firstSplitChoiceTieIter < dst.firstSplitChoiceTieIter)) {
        dst.firstSplitChoiceTieIter = src.firstSplitChoiceTieIter;
    }
    dst.multiEdgeCount += src.multiEdgeCount;
    dst.reducerInvocationCount += src.reducerInvocationCount;
    for (const auto& [key, value] : src.splitChoiceEquivClassCountHistogram) {
        dst.splitChoiceEquivClassCountHistogram[key] += value;
    }
    for (const auto& [key, value] : src.compareIneligibleReasonHistogram) {
        dst.compareIneligibleReasonHistogram[key] += value;
    }
    for (const auto& [key, value] : src.tracePrefixHistogram) {
        dst.tracePrefixHistogram[key] += value;
    }
    for (const auto& [key, value] : src.primitiveMultisetHistogram) {
        dst.primitiveMultisetHistogram[key] += value;
    }
}

void merge_fuzz_stats_into(FuzzStats& dst, const FuzzStats& src) {
    if (dst.caseName.empty()) {
        dst.caseName = src.caseName;
    }
    if (dst.weightProfile.empty()) {
        dst.weightProfile = src.weightProfile;
    }
    if (dst.preconditionBiasProfile.empty()) {
        dst.preconditionBiasProfile = src.preconditionBiasProfile;
    }
    if (dst.scenarioFamily.empty()) {
        dst.scenarioFamily = src.scenarioFamily;
    }
    if (dst.splitChoicePolicyMode.empty()) {
        dst.splitChoicePolicyMode = src.splitChoicePolicyMode;
    }
    if (dst.splitChoiceCompareMode.empty()) {
        dst.splitChoiceCompareMode = src.splitChoiceCompareMode;
    }
    if (dst.biasSplit == 0 && dst.biasJoin == 0 && dst.biasIntegrate == 0) {
        dst.biasSplit = src.biasSplit;
        dst.biasJoin = src.biasJoin;
        dst.biasIntegrate = src.biasIntegrate;
    }

    dst.totalIterations += src.totalIterations;
    dst.splitReadyCount += src.splitReadyCount;
    dst.boundaryOnlyChildCount += src.boundaryOnlyChildCount;
    dst.joinCandidateCount += src.joinCandidateCount;
    dst.integrateCandidateCount += src.integrateCandidateCount;
    dst.actualSplitHits += src.actualSplitHits;
    dst.actualJoinHits += src.actualJoinHits;
    dst.actualIntegrateHits += src.actualIntegrateHits;
    dst.splitChoiceCandidateCount += src.splitChoiceCandidateCount;
    dst.splitChoiceEvalCount += src.splitChoiceEvalCount;
    dst.splitChoiceTieCount += src.splitChoiceTieCount;
    dst.splitChoiceMulticlassCount += src.splitChoiceMulticlassCount;
    dst.splitChoiceFallbackCount += src.splitChoiceFallbackCount;
    dst.splitChoiceCompareStateCount += src.splitChoiceCompareStateCount;
    dst.splitReadyStateCount += src.splitReadyStateCount;
    dst.tieReadyStateCount += src.tieReadyStateCount;
    dst.compareEligibleStateCount += src.compareEligibleStateCount;
    dst.compareIneligibleStateCount += src.compareIneligibleStateCount;
    dst.compareCompletedStateCount += src.compareCompletedStateCount;
    dst.comparePartialStateCount += src.comparePartialStateCount;
    dst.splitChoiceExactFullEvalCount += src.splitChoiceExactFullEvalCount;
    dst.splitChoiceExactShadowEvalCount += src.splitChoiceExactShadowEvalCount;
    dst.splitChoiceSameRepresentativeCount += src.splitChoiceSameRepresentativeCount;
    dst.splitChoiceSameSemanticClassCount += src.splitChoiceSameSemanticClassCount;
    dst.splitChoiceSameFinalStateCount += src.splitChoiceSameFinalStateCount;
    dst.splitChoiceSemanticDisagreementCount += src.splitChoiceSemanticDisagreementCount;
    dst.splitChoiceCapHitCount += src.splitChoiceCapHitCount;
    dst.splitChoiceHarmlessCompareCount += src.splitChoiceHarmlessCompareCount;
    dst.splitChoiceTraceOnlyCompareCount += src.splitChoiceTraceOnlyCompareCount;
    dst.exactAuditedStateCount += src.exactAuditedStateCount;
    dst.exactAuditedPairCount += src.exactAuditedPairCount;
    dst.fastVsExactClassDisagreementCount += src.fastVsExactClassDisagreementCount;
    dst.representativeShiftCount += src.representativeShiftCount;
    dst.representativeShiftSameClassCount += src.representativeShiftSameClassCount;
    dst.representativeShiftSemanticDivergenceCount += src.representativeShiftSemanticDivergenceCount;
    dst.representativeShiftFollowupDivergenceCount += src.representativeShiftFollowupDivergenceCount;
    dst.representativeShiftTraceDivergenceCount += src.representativeShiftTraceDivergenceCount;
    dst.harmlessShiftCount += src.harmlessShiftCount;
    dst.traceOnlyShiftCount += src.traceOnlyShiftCount;
    dst.semanticShiftCount += src.semanticShiftCount;
    dst.compareCandidateEnumerationNanos += src.compareCandidateEnumerationNanos;
    dst.compareExactShadowEvaluationNanos += src.compareExactShadowEvaluationNanos;
    dst.compareExactFullEvaluationNanos += src.compareExactFullEvaluationNanos;
    dst.compareCanonicalizationNanos += src.compareCanonicalizationNanos;
    dst.compareMulticlassCatalogNanos += src.compareMulticlassCatalogNanos;
    dst.compareStateHashCacheHitCount += src.compareStateHashCacheHitCount;
    dst.compareStateHashCacheMissCount += src.compareStateHashCacheMissCount;
    dst.compareCandidateEvaluationCacheHitCount += src.compareCandidateEvaluationCacheHitCount;
    dst.compareCandidateEvaluationCacheMissCount += src.compareCandidateEvaluationCacheMissCount;
    dst.compareExactCanonicalCacheHitCount += src.compareExactCanonicalCacheHitCount;
    dst.compareExactCanonicalCacheMissCount += src.compareExactCanonicalCacheMissCount;
    dst.exactAuditSkippedCapCount += src.exactAuditSkippedCapCount;
    dst.exactAuditSkippedBudgetCount += src.exactAuditSkippedBudgetCount;
    dst.exactAuditSkippedSampleCount += src.exactAuditSkippedSampleCount;
    dst.exactAuditSkippedFamilyCount += src.exactAuditSkippedFamilyCount;
    dst.exactAuditSkippedNonTieCount += src.exactAuditSkippedNonTieCount;
    dst.splitSuccessCount += src.splitSuccessCount;
    dst.splitRejectedCount += src.splitRejectedCount;
    dst.directABArtifactCount += src.directABArtifactCount;
    dst.multiEdgeCount += src.multiEdgeCount;
    dst.reducerInvocationCount += src.reducerInvocationCount;
    dst.multiclassCatalogClusterCount += src.multiclassCatalogClusterCount;
    dst.multiclassHarmlessClusterCount += src.multiclassHarmlessClusterCount;
    dst.multiclassTraceOnlyClusterCount += src.multiclassTraceOnlyClusterCount;
    dst.multiclassSemanticShiftClusterCount += src.multiclassSemanticShiftClusterCount;
    if (dst.firstSplitChoiceTieIter < 0 ||
        (src.firstSplitChoiceTieIter >= 0 && src.firstSplitChoiceTieIter < dst.firstSplitChoiceTieIter)) {
        dst.firstSplitChoiceTieIter = src.firstSplitChoiceTieIter;
    }
    for (size_t i = 0; i < dst.primitiveHits.size(); ++i) {
        dst.primitiveHits[i] += src.primitiveHits[i];
    }
    for (const auto& [key, value] : src.splitChoiceEquivClassCountHistogram) {
        dst.splitChoiceEquivClassCountHistogram[key] += value;
    }
    for (const auto& [key, value] : src.compareIneligibleReasonHistogram) {
        dst.compareIneligibleReasonHistogram[key] += value;
    }
    for (const auto& [key, value] : src.multiclassCatalogHistogram) {
        dst.multiclassCatalogHistogram[key] += value;
    }
    for (const auto& [key, value] : src.oracleMismatchCount) {
        dst.oracleMismatchCount[key] += value;
    }
    for (const auto& [key, value] : src.tracePrefixHistogram) {
        dst.tracePrefixHistogram[key] += value;
    }
    for (const auto& [key, value] : src.primitiveMultisetHistogram) {
        dst.primitiveMultisetHistogram[key] += value;
    }
    for (const auto& [key, value] : src.sequenceHistogram) {
        dst.sequenceHistogram[key] += value;
    }
    for (const SeedFuzzSummary& srcSeedSummary : src.seedSummaries) {
        const auto it = find_if(
            dst.seedSummaries.begin(),
            dst.seedSummaries.end(),
            [&](const SeedFuzzSummary& candidate) { return candidate.seed == srcSeedSummary.seed; }
        );
        if (it == dst.seedSummaries.end()) {
            dst.seedSummaries.push_back(srcSeedSummary);
        } else {
            merge_seed_fuzz_summary_into(*it, srcSeedSummary);
        }
    }
}

void write_compare_profile_summary(const filesystem::path& jsonPath, const FuzzStats& stats) {
    const filesystem::path outPath = jsonPath.parent_path() / (jsonPath.stem().string() + ".compare_profile.summary.txt");
    filesystem::create_directories(outPath.parent_path());
    ofstream ofs(outPath);
    if (!ofs) {
        throw runtime_error("failed to write compare profile summary file: " + outPath.string());
    }
    const double avgCompareTimePerState = stats.splitChoiceCompareStateCount == 0U
        ? 0.0
        : static_cast<double>(
              stats.compareCandidateEnumerationNanos +
              stats.compareExactShadowEvaluationNanos +
              stats.compareExactFullEvaluationNanos +
              stats.compareCanonicalizationNanos +
              stats.compareMulticlassCatalogNanos
          ) / static_cast<double>(stats.splitChoiceCompareStateCount);
    const double avgExactFullEvalTimePerPair = stats.splitChoiceExactFullEvalCount == 0U
        ? 0.0
        : static_cast<double>(stats.compareExactFullEvaluationNanos) /
              static_cast<double>(stats.splitChoiceExactFullEvalCount);
    const double scenarioHashCacheHitRatio = safe_ratio(
        stats.compareStateHashCacheHitCount,
        stats.compareStateHashCacheHitCount + stats.compareStateHashCacheMissCount
    );
    const double pairEvalCacheHitRatio = safe_ratio(
        stats.compareCandidateEvaluationCacheHitCount,
        stats.compareCandidateEvaluationCacheHitCount + stats.compareCandidateEvaluationCacheMissCount
    );
    const double exactCanonicalCacheHitRatio = safe_ratio(
        stats.compareExactCanonicalCacheHitCount,
        stats.compareExactCanonicalCacheHitCount + stats.compareExactCanonicalCacheMissCount
    );
    ofs << "case=" << stats.caseName << '\n';
    ofs << "split_choice_policy_mode=" << stats.splitChoicePolicyMode << '\n';
    ofs << "split_choice_compare_mode=" << stats.splitChoiceCompareMode << '\n';
    ofs << "split_choice_compare_state_count=" << stats.splitChoiceCompareStateCount << '\n';
    ofs << "compare_eligible_state_count=" << stats.compareEligibleStateCount << '\n';
    ofs << "compare_ineligible_state_count=" << stats.compareIneligibleStateCount << '\n';
    ofs << "compare_completed_state_count=" << stats.compareCompletedStateCount << '\n';
    ofs << "compare_partial_state_count=" << stats.comparePartialStateCount << '\n';
    ofs << "split_choice_exact_shadow_eval_count=" << stats.splitChoiceExactShadowEvalCount << '\n';
    ofs << "split_choice_exact_full_eval_count=" << stats.splitChoiceExactFullEvalCount << '\n';
    ofs << "compare_candidate_enumeration_ns=" << stats.compareCandidateEnumerationNanos << '\n';
    ofs << "compare_exact_shadow_evaluation_ns=" << stats.compareExactShadowEvaluationNanos << '\n';
    ofs << "compare_exact_full_evaluation_ns=" << stats.compareExactFullEvaluationNanos << '\n';
    ofs << "compare_canonicalization_ns=" << stats.compareCanonicalizationNanos << '\n';
    ofs << "compare_multiclass_catalog_ns=" << stats.compareMulticlassCatalogNanos << '\n';
    ofs << "compare_state_hash_cache_hit_count=" << stats.compareStateHashCacheHitCount << '\n';
    ofs << "compare_state_hash_cache_miss_count=" << stats.compareStateHashCacheMissCount << '\n';
    ofs << "compare_candidate_evaluation_cache_hit_count=" << stats.compareCandidateEvaluationCacheHitCount << '\n';
    ofs << "compare_candidate_evaluation_cache_miss_count=" << stats.compareCandidateEvaluationCacheMissCount << '\n';
    ofs << "compare_exact_canonical_cache_hit_count=" << stats.compareExactCanonicalCacheHitCount << '\n';
    ofs << "compare_exact_canonical_cache_miss_count=" << stats.compareExactCanonicalCacheMissCount << '\n';
    ofs << "avg_compare_time_per_state_ns=" << json_number(avgCompareTimePerState) << '\n';
    ofs << "avg_exact_full_eval_time_per_pair_ns=" << json_number(avgExactFullEvalTimePerPair) << '\n';
    ofs << "scenario_hash_cache_hit_ratio=" << json_number(scenarioHashCacheHitRatio) << '\n';
    ofs << "exact_full_pair_evaluation_cache_hit_ratio=" << json_number(pairEvalCacheHitRatio) << '\n';
    ofs << "exact_canonical_key_cache_hit_ratio=" << json_number(exactCanonicalCacheHitRatio) << '\n';
}

void write_fuzz_stats_outputs_from_stats(const filesystem::path& jsonPath, const FuzzStats& stats) {
    const filesystem::path summaryPath = jsonPath.parent_path() / (jsonPath.stem().string() + ".summary.txt");
    filesystem::create_directories(jsonPath.parent_path());
    filesystem::create_directories(summaryPath.parent_path());

    ofstream summary(summaryPath);
    if (!summary) {
        throw runtime_error("failed to write merged stats summary file: " + summaryPath.string());
    }
    write_fuzz_summary_body(summary, stats);

    ofstream json(jsonPath);
    if (!json) {
        throw runtime_error("failed to write merged stats json file: " + jsonPath.string());
    }
    json << "{\n";
    json << "\"case\":\"" << json_escape(stats.caseName) << "\",\n";
    json << "\"weight_profile\":\"" << json_escape(stats.weightProfile) << "\",\n";
    json << "\"precondition_bias_profile\":\"" << json_escape(stats.preconditionBiasProfile) << "\",\n";
    json << "\"scenario_family\":\"" << json_escape(stats.scenarioFamily) << "\",\n";
    json << "\"split_choice_policy_mode\":\"" << json_escape(stats.splitChoicePolicyMode) << "\",\n";
    json << "\"split_choice_compare_mode\":\"" << json_escape(stats.splitChoiceCompareMode) << "\",\n";
    json << "\"iterations\":" << stats.totalIterations << ",\n";
    json << "\"split_ready_count\":" << stats.splitReadyCount << ",\n";
    json << "\"join_candidate_count\":" << stats.joinCandidateCount << ",\n";
    json << "\"integrate_candidate_count\":" << stats.integrateCandidateCount << ",\n";
    json << "\"actual_split_hits\":" << stats.actualSplitHits << ",\n";
    json << "\"actual_join_hits\":" << stats.actualJoinHits << ",\n";
    json << "\"actual_integrate_hits\":" << stats.actualIntegrateHits << ",\n";
    json << "\"split_choice_compare_state_count\":" << stats.splitChoiceCompareStateCount << ",\n";
    json << "\"split_ready_state_count\":" << stats.splitReadyStateCount << ",\n";
    json << "\"tie_ready_state_count\":" << stats.tieReadyStateCount << ",\n";
    json << "\"compare_eligible_state_count\":" << stats.compareEligibleStateCount << ",\n";
    json << "\"compare_ineligible_state_count\":" << stats.compareIneligibleStateCount << ",\n";
    json << "\"compare_completed_state_count\":" << stats.compareCompletedStateCount << ",\n";
    json << "\"compare_partial_state_count\":" << stats.comparePartialStateCount << ",\n";
    json << "\"split_choice_exact_full_eval_count\":" << stats.splitChoiceExactFullEvalCount << ",\n";
    json << "\"split_choice_exact_shadow_eval_count\":" << stats.splitChoiceExactShadowEvalCount << ",\n";
    json << "\"split_choice_same_representative_count\":" << stats.splitChoiceSameRepresentativeCount << ",\n";
    json << "\"split_choice_same_semantic_class_count\":" << stats.splitChoiceSameSemanticClassCount << ",\n";
    json << "\"split_choice_same_final_state_count\":" << stats.splitChoiceSameFinalStateCount << ",\n";
    json << "\"split_choice_semantic_disagreement_count\":" << stats.splitChoiceSemanticDisagreementCount << ",\n";
    json << "\"split_choice_fallback_count\":" << stats.splitChoiceFallbackCount << ",\n";
    json << "\"split_choice_cap_hit_count\":" << stats.splitChoiceCapHitCount << ",\n";
    json << "\"representative_shift_count\":" << stats.representativeShiftCount << ",\n";
    json << "\"harmless_shift_count\":" << stats.harmlessShiftCount << ",\n";
    json << "\"trace_only_shift_count\":" << stats.traceOnlyShiftCount << ",\n";
    json << "\"semantic_shift_count\":" << stats.semanticShiftCount << ",\n";
    json << "\"compare_ineligible_reason_histogram\":" << json_object_from_map(stats.compareIneligibleReasonHistogram) << ",\n";
    json << "\"compare_candidate_enumeration_ns\":" << stats.compareCandidateEnumerationNanos << ",\n";
    json << "\"compare_exact_shadow_evaluation_ns\":" << stats.compareExactShadowEvaluationNanos << ",\n";
    json << "\"compare_exact_full_evaluation_ns\":" << stats.compareExactFullEvaluationNanos << ",\n";
    json << "\"compare_canonicalization_ns\":" << stats.compareCanonicalizationNanos << ",\n";
    json << "\"compare_multiclass_catalog_ns\":" << stats.compareMulticlassCatalogNanos << ",\n";
    json << "\"compare_state_hash_cache_hit_count\":" << stats.compareStateHashCacheHitCount << ",\n";
    json << "\"compare_state_hash_cache_miss_count\":" << stats.compareStateHashCacheMissCount << ",\n";
    json << "\"compare_candidate_evaluation_cache_hit_count\":" << stats.compareCandidateEvaluationCacheHitCount << ",\n";
    json << "\"compare_candidate_evaluation_cache_miss_count\":" << stats.compareCandidateEvaluationCacheMissCount << ",\n";
    json << "\"compare_exact_canonical_cache_hit_count\":" << stats.compareExactCanonicalCacheHitCount << ",\n";
    json << "\"compare_exact_canonical_cache_miss_count\":" << stats.compareExactCanonicalCacheMissCount << ",\n";
    json << "\"multiclass_catalog_cluster_count\":" << stats.multiclassCatalogClusterCount << ",\n";
    json << "\"multiclass_catalog_histogram\":" << json_object_from_map(stats.multiclassCatalogHistogram) << ",\n";
    json << "\"split_choice_equiv_class_count_histogram\":"
         << json_object_from_numeric_map(stats.splitChoiceEquivClassCountHistogram) << "\n";
    json << "}\n";
    write_compare_profile_summary(jsonPath, stats);
}

void write_campaign_checkpoint_manifest(const CampaignCheckpointManifest& manifest) {
    filesystem::create_directories(checkpoint_chunks_dir(manifest.checkpointDir));
    ofstream ofs(checkpoint_manifest_path(manifest.checkpointDir));
    if (!ofs) {
        throw runtime_error("failed to write checkpoint manifest: " + checkpoint_manifest_path(manifest.checkpointDir).string());
    }
    ofs << "version=" << kCampaignCheckpointManifestVersion << '\n';
    ofs << "checkpoint_dir=" << manifest.checkpointDir.string() << '\n';
    ofs << "campaign_config_snapshot=" << manifest.campaignConfigSnapshot.string() << '\n';
    ofs << "split_choice_policy_mode=" << split_choice_policy_mode_name_string(manifest.splitChoicePolicyMode) << '\n';
    ofs << "split_choice_compare_mode=" << split_choice_compare_mode_name_string(manifest.splitChoiceCompareMode) << '\n';
    ofs << "compare_sample_rate=" << manifest.compareSampleRate << '\n';
    ofs << "compare_budget=" << manifest.compareBudget << '\n';
    ofs << "exact_canonical_cap=" << manifest.exactCanonicalCap << '\n';
    ofs << "target_compared_states=" << manifest.targetComparedStates << '\n';
    ofs << "target_eligible_states=" << manifest.targetEligibleStates << '\n';
    ofs << "target_lineage_samples=" << manifest.targetLineageSamples << '\n';
    ofs << "max_partial_runs=" << manifest.maxPartialRuns << '\n';
    ofs << "stop_when_gate_passes=" << (manifest.stopWhenGatePasses ? 1 : 0) << '\n';
    ofs << "target_applicability_confidence=" << json_number(manifest.targetApplicabilityConfidence) << '\n';
    ofs << "checkpoint_every=" << manifest.checkpointEvery << '\n';
    ofs << "save_corpus_dir=" << (manifest.saveCorpusDir.has_value() ? *manifest.saveCorpusDir : string()) << '\n';
    ofs << "load_corpus_dir=" << (manifest.loadCorpusDir.has_value() ? *manifest.loadCorpusDir : string()) << '\n';
    ofs << "corpus_policy=" << corpus_policy_name_string(manifest.corpusPolicy) << '\n';
}

CampaignCheckpointManifest load_campaign_checkpoint_manifest(const filesystem::path& resumePath) {
    filesystem::path manifestPath = filesystem::absolute(resumePath);
    if (filesystem::is_directory(manifestPath)) {
        manifestPath = checkpoint_manifest_path(manifestPath);
    }
    const unordered_map<string, string> values = read_key_value_file(manifestPath);
    const string version = require_kv_value(values, "version", manifestPath);
    if (version != kCampaignCheckpointManifestVersion) {
        throw runtime_error(
            "unsupported checkpoint manifest version: path=" + manifestPath.string() +
            " expected=" + kCampaignCheckpointManifestVersion +
            " actual=" + version
        );
    }
    CampaignCheckpointManifest manifest;
    manifest.checkpointDir = filesystem::absolute(require_kv_value(values, "checkpoint_dir", manifestPath));
    manifest.campaignConfigSnapshot = filesystem::absolute(require_kv_value(values, "campaign_config_snapshot", manifestPath));
    if (const optional<SplitChoicePolicyMode> policy =
            parse_split_choice_policy_mode_token(require_kv_value(values, "split_choice_policy_mode", manifestPath));
        policy.has_value()) {
        manifest.splitChoicePolicyMode = *policy;
    }
    if (const optional<SplitChoiceCompareMode> compare =
            parse_split_choice_compare_mode_token(require_kv_value(values, "split_choice_compare_mode", manifestPath));
        compare.has_value()) {
        manifest.splitChoiceCompareMode = *compare;
    }
    manifest.compareSampleRate = stod(require_kv_value(values, "compare_sample_rate", manifestPath));
    manifest.compareBudget = static_cast<size_t>(stoull(require_kv_value(values, "compare_budget", manifestPath)));
    manifest.exactCanonicalCap = static_cast<size_t>(stoull(require_kv_value(values, "exact_canonical_cap", manifestPath)));
    if (const auto it = values.find("target_compared_states"); it != values.end()) {
        manifest.targetComparedStates = static_cast<size_t>(stoull(it->second));
    }
    if (const auto it = values.find("target_eligible_states"); it != values.end()) {
        manifest.targetEligibleStates = static_cast<size_t>(stoull(it->second));
    }
    if (const auto it = values.find("target_lineage_samples"); it != values.end()) {
        manifest.targetLineageSamples = static_cast<size_t>(stoull(it->second));
    }
    if (const auto it = values.find("max_partial_runs"); it != values.end()) {
        manifest.maxPartialRuns = static_cast<size_t>(stoull(it->second));
    }
    if (const auto it = values.find("stop_when_gate_passes"); it != values.end()) {
        manifest.stopWhenGatePasses = (it->second == "1" || it->second == "true");
    }
    if (const auto it = values.find("target_applicability_confidence"); it != values.end()) {
        manifest.targetApplicabilityConfidence = stod(it->second);
    }
    manifest.checkpointEvery = static_cast<size_t>(stoull(require_kv_value(values, "checkpoint_every", manifestPath)));
    const string saveCorpus = require_kv_value(values, "save_corpus_dir", manifestPath);
    if (!saveCorpus.empty()) {
        manifest.saveCorpusDir = saveCorpus;
    }
    const string loadCorpus = require_kv_value(values, "load_corpus_dir", manifestPath);
    if (!loadCorpus.empty()) {
        manifest.loadCorpusDir = loadCorpus;
    }
    if (const optional<CorpusPolicy> policy =
            parse_corpus_policy_token(require_kv_value(values, "corpus_policy", manifestPath));
        policy.has_value()) {
        manifest.corpusPolicy = *policy;
    }
    return manifest;
}

void write_campaign_checkpoint_chunk(
    const filesystem::path& checkpointDir,
    const CampaignCheckpointChunk& chunk
) {
    const filesystem::path outPath = checkpoint_chunks_dir(checkpointDir) /
        (checkpoint_chunk_stem(chunk.runIndex, chunk.seedIndex, chunk.seed, chunk.iterStart, chunk.iterCount) + ".chk");
    filesystem::create_directories(outPath.parent_path());
    ofstream ofs(outPath);
    if (!ofs) {
        throw runtime_error("failed to write checkpoint chunk: " + outPath.string());
    }
    ofs << "run_index=" << chunk.runIndex << '\n';
    ofs << "seed_index=" << chunk.seedIndex << '\n';
    ofs << "seed=" << chunk.seed << '\n';
    ofs << "iter_start=" << chunk.iterStart << '\n';
    ofs << "iter_count=" << chunk.iterCount << '\n';
    ofs << "total_iterations=" << chunk.totalIterations << '\n';
    ofs << "compare_state_count=" << chunk.compareStateCount << '\n';
    ofs << "semantic_disagreement_count=" << chunk.semanticDisagreementCount << '\n';
    ofs << "fallback_count=" << chunk.fallbackCount << '\n';
    ofs << "case_name=" << chunk.caseName << '\n';
    ofs << "mode=" << chunk.mode << '\n';
    ofs << "artifact_dir=" << chunk.artifactDir << '\n';
    ofs << "stats_file=" << chunk.statsFile << '\n';
    ofs << "summary_file=" << chunk.summaryFile << '\n';
}

vector<CampaignCheckpointChunk> load_campaign_checkpoint_chunks(const filesystem::path& checkpointDir) {
    vector<CampaignCheckpointChunk> chunks;
    const filesystem::path dir = checkpoint_chunks_dir(checkpointDir);
    if (!filesystem::exists(dir)) {
        return chunks;
    }
    for (const filesystem::directory_entry& entry : filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".chk") {
            continue;
        }
        const unordered_map<string, string> values = read_key_value_file(entry.path());
        CampaignCheckpointChunk chunk;
        chunk.runIndex = static_cast<size_t>(stoull(require_kv_value(values, "run_index", entry.path())));
        chunk.seedIndex = static_cast<size_t>(stoull(require_kv_value(values, "seed_index", entry.path())));
        chunk.seed = static_cast<u32>(stoul(require_kv_value(values, "seed", entry.path())));
        chunk.iterStart = stoi(require_kv_value(values, "iter_start", entry.path()));
        chunk.iterCount = stoi(require_kv_value(values, "iter_count", entry.path()));
        if (const auto it = values.find("total_iterations"); it != values.end()) {
            chunk.totalIterations = static_cast<size_t>(stoull(it->second));
        }
        if (const auto it = values.find("compare_state_count"); it != values.end()) {
            chunk.compareStateCount = static_cast<size_t>(stoull(it->second));
        }
        if (const auto it = values.find("semantic_disagreement_count"); it != values.end()) {
            chunk.semanticDisagreementCount = static_cast<size_t>(stoull(it->second));
        }
        if (const auto it = values.find("fallback_count"); it != values.end()) {
            chunk.fallbackCount = static_cast<size_t>(stoull(it->second));
        }
        chunk.caseName = require_kv_value(values, "case_name", entry.path());
        chunk.mode = require_kv_value(values, "mode", entry.path());
        chunk.artifactDir = require_kv_value(values, "artifact_dir", entry.path());
        chunk.statsFile = require_kv_value(values, "stats_file", entry.path());
        chunk.summaryFile = require_kv_value(values, "summary_file", entry.path());
        chunks.push_back(std::move(chunk));
    }
    sort(chunks.begin(), chunks.end(), [](const CampaignCheckpointChunk& lhs, const CampaignCheckpointChunk& rhs) {
        return tie(lhs.runIndex, lhs.seedIndex, lhs.iterStart) < tie(rhs.runIndex, rhs.seedIndex, rhs.iterStart);
    });
    return chunks;
}

void validate_campaign_checkpoint_chunks(
    const filesystem::path& checkpointDir,
    const vector<CampaignCheckpointChunk>& chunks
) {
    for (size_t i = 1; i < chunks.size(); ++i) {
        const CampaignCheckpointChunk& prev = chunks[i - 1];
        const CampaignCheckpointChunk& curr = chunks[i];
        if (prev.runIndex != curr.runIndex || prev.seedIndex != curr.seedIndex || prev.seed != curr.seed) {
            continue;
        }
        if (curr.iterStart < prev.iterStart + prev.iterCount) {
            throw runtime_error(
                "overlapping checkpoint chunks detected in " + checkpointDir.string() +
                "; use --resume-from with the original checkpoint settings or a fresh checkpoint dir"
            );
        }
    }
}

void validate_campaign_checkpoint_manifest_compatibility(
    const filesystem::path& checkpointDir,
    const CampaignCheckpointManifest& prior,
    const CampaignCheckpointManifest& current,
    bool haveExistingChunks
) {
    if (!haveExistingChunks) {
        return;
    }
    auto fail_mismatch = [&](const string& field, const string& priorValue, const string& currentValue) {
        throw runtime_error(
            "checkpoint manifest mismatch for " + checkpointDir.string() +
            " field=" + field +
            " prior=" + priorValue +
            " current=" + currentValue +
            "; use --resume-from with the original settings or a fresh checkpoint dir"
        );
    };
    if (prior.checkpointEvery != current.checkpointEvery) {
        fail_mismatch("checkpoint_every", to_string(prior.checkpointEvery), to_string(current.checkpointEvery));
    }
    if (prior.splitChoicePolicyMode != current.splitChoicePolicyMode) {
        fail_mismatch(
            "split_choice_policy_mode",
            split_choice_policy_mode_name_string(prior.splitChoicePolicyMode),
            split_choice_policy_mode_name_string(current.splitChoicePolicyMode)
        );
    }
    if (prior.splitChoiceCompareMode != current.splitChoiceCompareMode) {
        fail_mismatch(
            "split_choice_compare_mode",
            split_choice_compare_mode_name_string(prior.splitChoiceCompareMode),
            split_choice_compare_mode_name_string(current.splitChoiceCompareMode)
        );
    }
    if (prior.compareSampleRate != current.compareSampleRate) {
        fail_mismatch("compare_sample_rate", json_number(prior.compareSampleRate), json_number(current.compareSampleRate));
    }
    if (prior.compareBudget != current.compareBudget) {
        fail_mismatch("compare_budget", to_string(prior.compareBudget), to_string(current.compareBudget));
    }
    if (prior.exactCanonicalCap != current.exactCanonicalCap) {
        fail_mismatch("exact_canonical_cap", to_string(prior.exactCanonicalCap), to_string(current.exactCanonicalCap));
    }
    if (prior.targetComparedStates != current.targetComparedStates) {
        fail_mismatch(
            "target_compared_states",
            to_string(prior.targetComparedStates),
            to_string(current.targetComparedStates)
        );
    }
    if (prior.targetEligibleStates != current.targetEligibleStates) {
        fail_mismatch(
            "target_eligible_states",
            to_string(prior.targetEligibleStates),
            to_string(current.targetEligibleStates)
        );
    }
    if (prior.targetLineageSamples != current.targetLineageSamples) {
        fail_mismatch(
            "target_lineage_samples",
            to_string(prior.targetLineageSamples),
            to_string(current.targetLineageSamples)
        );
    }
    if (prior.maxPartialRuns != current.maxPartialRuns) {
        fail_mismatch("max_partial_runs", to_string(prior.maxPartialRuns), to_string(current.maxPartialRuns));
    }
    if (prior.stopWhenGatePasses != current.stopWhenGatePasses) {
        fail_mismatch(
            "stop_when_gate_passes",
            prior.stopWhenGatePasses ? "1" : "0",
            current.stopWhenGatePasses ? "1" : "0"
        );
    }
    if (prior.targetApplicabilityConfidence != current.targetApplicabilityConfidence) {
        fail_mismatch(
            "target_applicability_confidence",
            json_number(prior.targetApplicabilityConfidence),
            json_number(current.targetApplicabilityConfidence)
        );
    }
}

optional<FuzzMode> parse_fuzz_mode(const string& name) {
    static const array<pair<const char*, FuzzMode>, 8> modes = {{
        {"isolate_only", FuzzMode::ISOLATE_ONLY},
        {"split_only", FuzzMode::SPLIT_ONLY},
        {"join_only", FuzzMode::JOIN_ONLY},
        {"integrate_only", FuzzMode::INTEGRATE_ONLY},
        {"isolate_split", FuzzMode::ISOLATE_THEN_SPLIT},
        {"split_join", FuzzMode::SPLIT_THEN_JOIN},
        {"split_integrate", FuzzMode::SPLIT_THEN_INTEGRATE},
        {"mixed_planner", FuzzMode::MIXED_PLANNER},
    }};
    for (const auto& entry : modes) {
        if (name == entry.first) {
            return entry.second;
        }
    }
    return nullopt;
}

u32 mix_seed(u32 baseSeed, int iter) {
    u64 x = (static_cast<u64>(baseSeed) << 32U) ^ static_cast<u32>(iter);
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27U)) * 0x94D049BB133111EBULL;
    x ^= (x >> 31U);
    return static_cast<u32>(x & 0xFFFFFFFFULL);
}

OccID new_occ(RawEngine& RE, Vertex orig) {
    const OccID id = RE.occ.alloc(make_occ_record(orig));
    RE.occOfOrig[orig].push_back(id);
    return id;
}

RawSkelID new_skeleton(RawEngine& RE) {
    return RE.skel.alloc(RawSkeleton{});
}

void assert_engine_ok(const RawEngine& RE, const vector<RawSkelID>& sids, const vector<OccID>& occs) {
    string validationError;
    for (RawSkelID sid : sids) {
        if (sid != NIL_U32) {
            if (!validate_skeleton_wellformed(RE, sid, &validationError)) {
                fail_test("engine skeleton validation failed sid=" + to_string(sid) + " error=" + validationError);
            }
        }
    }
    for (OccID occ : occs) {
        if (!validate_occ_patch_consistent(RE, occ, &validationError)) {
            fail_test("engine occurrence validation failed occ=" + to_string(occ) + " error=" + validationError);
        }
    }
}

vector<pair<Vertex, Vertex>> normalized_core_pairs(const RawEngine& RE, const vector<RawEID>& eids) {
    vector<pair<Vertex, Vertex>> out;
    out.reserve(eids.size());
    for (RawEID eid : eids) {
        const RawEdge& e = RE.E.get(eid);
        Vertex a = RE.V.get(e.a).orig;
        Vertex b = RE.V.get(e.b).orig;
        if (a > b) {
            swap(a, b);
        }
        out.push_back({a, b});
    }
    sort(out.begin(), out.end());
    return out;
}

OccPatchSignature capture_occ_patch_signature(const RawEngine& RE, OccID occ) {
    const RawOccRecord& O = RE.occ.get(occ);
    OccPatchSignature sig;
    sig.allocNbr = O.allocNbr;
    sort(sig.allocNbr.begin(), sig.allocNbr.end());
    sig.core = normalized_core_pairs(RE, O.corePatchEdges);
    return sig;
}

unordered_map<OccID, OccPatchSignature> capture_occ_signatures(const RawEngine& RE, const vector<OccID>& occs);
void assert_target_prepare_equivalent(
    const RawEngine& RE,
    RawSkelID sid,
    OccID occ,
    const NormalizedPrep& expected
);

NormalizedPrep normalize_prep(const IsolatePrepared& p) {
    NormalizedPrep out;
    out.orig = p.orig;
    out.allocNbr = p.allocNbr;
    sort(out.allocNbr.begin(), out.allocNbr.end());

    for (const IsoPort& port : p.ports) {
        out.ports.push_back({static_cast<int>(port.kind), port.attachOrig, port.br, port.side});
    }
    sort(out.ports.begin(), out.ports.end());

    for (const TinyEdge& e : p.core.edges) {
        Vertex a = p.core.orig[e.a];
        Vertex b = p.core.orig[e.b];
        if (a > b) {
            swap(a, b);
        }
        out.core.push_back({a, b});
    }
    sort(out.core.begin(), out.core.end());
    return out;
}

bool has_mode_min_boundary(FuzzMode mode) {
    return mode == FuzzMode::INTEGRATE_ONLY ||
           mode == FuzzMode::SPLIT_THEN_INTEGRATE ||
           mode == FuzzMode::MIXED_PLANNER;
}

int mode_min_occ(FuzzMode mode) {
    switch (mode) {
        case FuzzMode::ISOLATE_ONLY:
        case FuzzMode::SPLIT_ONLY:
        case FuzzMode::JOIN_ONLY:
        case FuzzMode::ISOLATE_THEN_SPLIT:
        case FuzzMode::SPLIT_THEN_JOIN:
        case FuzzMode::MIXED_PLANNER:
            return 2;
        case FuzzMode::INTEGRATE_ONLY:
        case FuzzMode::SPLIT_THEN_INTEGRATE:
            return 1;
    }
    return 1;
}

bool same_fuzz_spec(const FuzzSpec& lhs, const FuzzSpec& rhs) {
    return lhs.mode == rhs.mode &&
           lhs.seed == rhs.seed &&
           lhs.branchCount == rhs.branchCount &&
           lhs.occCount == rhs.occCount &&
           lhs.boundaryOnlyCount == rhs.boundaryOnlyCount &&
           lhs.maxPathLen == rhs.maxPathLen &&
           lhs.maxOccPerBranch == rhs.maxOccPerBranch &&
           lhs.directABCount == rhs.directABCount &&
           lhs.multiEdgeCount == rhs.multiEdgeCount &&
           lhs.sharedOrigPairs == rhs.sharedOrigPairs &&
           lhs.keepOccCount == rhs.keepOccCount &&
           lhs.opCount == rhs.opCount &&
           lhs.biasSplit == rhs.biasSplit &&
           lhs.biasJoin == rhs.biasJoin &&
           lhs.biasIntegrate == rhs.biasIntegrate &&
           lhs.stepBudget == rhs.stepBudget;
}

FuzzSpec normalize_fuzz_spec(FuzzSpec spec) {
    spec.branchCount = max(spec.branchCount, 2);
    spec.maxPathLen = max(spec.maxPathLen, 1);
    spec.maxOccPerBranch = max(spec.maxOccPerBranch, 1);
    spec.opCount = max(spec.opCount, 1);

    const int minBoundary = has_mode_min_boundary(spec.mode) ? 1 : 0;
    spec.boundaryOnlyCount = max(spec.boundaryOnlyCount, minBoundary);
    spec.boundaryOnlyCount = min(spec.boundaryOnlyCount, spec.branchCount - 1);

    const int minOcc = mode_min_occ(spec.mode);
    const int occCapacity = max((spec.branchCount - spec.boundaryOnlyCount) * spec.maxOccPerBranch, minOcc);
    spec.occCount = max(spec.occCount, minOcc);
    spec.occCount = min(spec.occCount, occCapacity);
    spec.keepOccCount = max(spec.keepOccCount, 1);
    spec.keepOccCount = min(spec.keepOccCount, spec.occCount);
    spec.sharedOrigPairs = max(spec.sharedOrigPairs, 0);
    spec.sharedOrigPairs = min(spec.sharedOrigPairs, spec.occCount / 2);
    spec.directABCount = max(spec.directABCount, 0);
    spec.multiEdgeCount = max(spec.multiEdgeCount, 0);
    spec.biasSplit = clamp_bias_value(spec.biasSplit);
    spec.biasJoin = clamp_bias_value(spec.biasJoin);
    spec.biasIntegrate = clamp_bias_value(spec.biasIntegrate);

    return spec;
}

string fuzz_spec_to_string(const FuzzSpec& spec) {
    ostringstream oss;
    oss << "mode=" << fuzz_mode_name(spec.mode) << '\n';
    oss << "seed=" << spec.seed << '\n';
    oss << "branch_count=" << spec.branchCount << '\n';
    oss << "occ_count=" << spec.occCount << '\n';
    oss << "boundary_only_count=" << spec.boundaryOnlyCount << '\n';
    oss << "max_path_len=" << spec.maxPathLen << '\n';
    oss << "max_occ_per_branch=" << spec.maxOccPerBranch << '\n';
    oss << "direct_ab_count=" << spec.directABCount << '\n';
    oss << "multi_edge_count=" << spec.multiEdgeCount << '\n';
    oss << "shared_orig_pairs=" << spec.sharedOrigPairs << '\n';
    oss << "keep_occ_count=" << spec.keepOccCount << '\n';
    oss << "op_count=" << spec.opCount << '\n';
    oss << "bias_split=" << spec.biasSplit << '\n';
    oss << "bias_join=" << spec.biasJoin << '\n';
    oss << "bias_integrate=" << spec.biasIntegrate << '\n';
    oss << "step_budget=" << spec.stepBudget << '\n';
    return oss.str();
}

FuzzSpec parse_fuzz_spec(istream& is) {
    unordered_map<string, string> kv;
    string line;
    while (getline(is, line)) {
        if (line.empty()) {
            continue;
        }
        const size_t eq = line.find('=');
        if (eq == string::npos) {
            throw runtime_error("invalid repro line: " + line);
        }
        kv.emplace(line.substr(0, eq), line.substr(eq + 1));
    }

    const auto get = [&](const string& key) -> string {
        const auto it = kv.find(key);
        if (it == kv.end()) {
            throw runtime_error("missing repro key: " + key);
        }
        return it->second;
    };

    const optional<FuzzMode> mode = parse_fuzz_mode(get("mode"));
    if (!mode.has_value()) {
        throw runtime_error("unknown repro mode");
    }

    FuzzSpec spec;
    spec.mode = *mode;
    spec.seed = static_cast<u32>(stoul(get("seed")));
    spec.branchCount = stoi(get("branch_count"));
    spec.occCount = stoi(get("occ_count"));
    spec.boundaryOnlyCount = stoi(get("boundary_only_count"));
    spec.maxPathLen = stoi(get("max_path_len"));
    spec.maxOccPerBranch = stoi(get("max_occ_per_branch"));
    spec.directABCount = stoi(get("direct_ab_count"));
    spec.multiEdgeCount = stoi(get("multi_edge_count"));
    spec.sharedOrigPairs = stoi(get("shared_orig_pairs"));
    spec.keepOccCount = stoi(get("keep_occ_count"));
    spec.opCount = stoi(get("op_count"));
    spec.biasSplit = kv.count("bias_split") != 0U ? stoi(get("bias_split")) : 0;
    spec.biasJoin = kv.count("bias_join") != 0U ? stoi(get("bias_join")) : 0;
    spec.biasIntegrate = kv.count("bias_integrate") != 0U ? stoi(get("bias_integrate")) : 0;
    spec.stepBudget = static_cast<size_t>(stoull(get("step_budget")));
    return normalize_fuzz_spec(spec);
}

FuzzSpec load_fuzz_spec(const filesystem::path& path) {
    ifstream ifs(path);
    if (!ifs) {
        throw runtime_error("failed to open repro file: " + path.string());
    }
    return parse_fuzz_spec(ifs);
}

filesystem::path save_fuzz_spec(const FuzzSpec& spec, const string& stem) {
    const filesystem::path outDir = artifact_subdir(active_test_options(), "counterexamples");
    const filesystem::path outPath = outDir / (stem + ".txt");
    ofstream ofs(outPath);
    if (!ofs) {
        throw runtime_error("failed to write repro file: " + outPath.string());
    }
    ofs << fuzz_spec_to_string(spec);
    return outPath;
}

vector<OccID> collect_live_occurrences(const RawEngine& RE) {
    vector<OccID> out;
    for (u32 occ = 0; occ < RE.occ.a.size(); ++occ) {
        if (RE.occ.a[occ].alive) {
            out.push_back(occ);
        }
    }
    return out;
}

void assert_occ_signature_equivalent(const RawEngine& RE, OccID occ, const OccPatchSignature& expected) {
    const OccPatchSignature got = capture_occ_patch_signature(RE, occ);
    if (!(got == expected)) {
        fail_test(
            "occ signature mismatch occ=" + to_string(occ) +
            " expected=" + format_occ_patch_signature(expected) +
            " got=" + format_occ_patch_signature(got)
        );
    }
}

void assert_engine_bookkeeping_sane(const RawEngine& RE) {
    unordered_map<RawVID, int> vertexOwnerCnt;
    unordered_map<RawEID, int> edgeOwnerCnt;
    unordered_map<OccID, int> occHostCnt;

    for (RawSkelID sid = 0; sid < RE.skel.a.size(); ++sid) {
        if (!RE.skel.a[sid].alive) {
            continue;
        }

        const RawSkeleton& S = RE.skel.get(sid);
        string validationError;
        if (!validate_skeleton_wellformed(RE, sid, &validationError)) {
            fail_test("bookkeeping skeleton validation failed sid=" + to_string(sid) + " error=" + validationError);
        }

        unordered_set<RawVID> localV;
        unordered_set<RawEID> localE;
        unordered_set<OccID> localOcc;

        for (RawVID v : S.verts) {
            if (!localV.insert(v).second || !RE.V.a[v].alive) {
                fail_test("bookkeeping vertex ownership violation sid=" + to_string(sid) + " vid=" + to_string(v));
            }
            ++vertexOwnerCnt[v];
        }

        for (RawEID eid : S.edges) {
            if (!localE.insert(eid).second || !RE.E.a[eid].alive) {
                fail_test("bookkeeping edge ownership violation sid=" + to_string(sid) + " eid=" + to_string(eid));
            }
            ++edgeOwnerCnt[eid];
        }

        for (OccID occ : S.hostedOcc) {
            if (!localOcc.insert(occ).second || !RE.occ.a[occ].alive) {
                fail_test("bookkeeping hosted occurrence violation sid=" + to_string(sid) + " occ=" + to_string(occ));
            }
            ++occHostCnt[occ];
            if (!validate_occ_patch_consistent(RE, occ, &validationError)) {
                fail_test("bookkeeping occurrence validation failed occ=" + to_string(occ) + " error=" + validationError);
            }
            const RawOccRecord& O = RE.occ.get(occ);
            if (O.hostSkel != sid) {
                fail_test(
                    "bookkeeping host mismatch occ=" + to_string(occ) +
                    " hostSkel=" + to_string(O.hostSkel) +
                    " expectedSid=" + to_string(sid)
                );
            }
        }
    }

    for (u32 vid = 0; vid < RE.V.a.size(); ++vid) {
        if (!RE.V.a[vid].alive) {
            continue;
        }
        if (vertexOwnerCnt[vid] != 1) {
            fail_test("bookkeeping vertex owner count mismatch vid=" + to_string(vid));
        }
    }

    for (u32 eid = 0; eid < RE.E.a.size(); ++eid) {
        if (!RE.E.a[eid].alive) {
            continue;
        }
        if (edgeOwnerCnt[eid] != 1) {
            fail_test("bookkeeping edge owner count mismatch eid=" + to_string(eid));
        }
    }

    unordered_map<OccID, int> occOrigCnt;
    for (const auto& entry : RE.occOfOrig) {
        unordered_set<OccID> uniq(entry.second.begin(), entry.second.end());
        if (uniq.size() != entry.second.size()) {
            fail_test("bookkeeping duplicate occOfOrig entry orig=" + to_string(entry.first));
        }
        for (OccID occ : entry.second) {
            if (!RE.occ.a[occ].alive || RE.occ.get(occ).orig != entry.first) {
                fail_test(
                    "bookkeeping occOfOrig mismatch orig=" + to_string(entry.first) +
                    " occ=" + to_string(occ)
                );
            }
            ++occOrigCnt[occ];
        }
    }

    for (u32 occ = 0; occ < RE.occ.a.size(); ++occ) {
        if (!RE.occ.a[occ].alive) {
            continue;
        }
        const RawOccRecord& O = RE.occ.get(occ);
        if (occHostCnt[occ] != 1 || occOrigCnt[occ] != 1) {
            fail_test("bookkeeping occurrence count mismatch occ=" + to_string(occ));
        }
        if (O.hostSkel >= RE.skel.a.size() || !RE.skel.a[O.hostSkel].alive) {
            fail_test("bookkeeping dead host skeleton occ=" + to_string(occ) + " hostSkel=" + to_string(O.hostSkel));
        }
    }
}

void assert_planner_stop_condition(const RawEngine& RE, OccID target) {
    const RawOccRecord& O = RE.occ.get(target);
    const RawSkeleton& S = RE.skel.get(O.hostSkel);
    if (S.hostedOcc.size() != 1U || S.hostedOcc[0] != target) {
        fail_test("planner stop condition hosted occurrence mismatch target=" + to_string(target));
    }
    if (discover_split_pair_from_support(RE, target).has_value()) {
        fail_test("planner stop condition split pair still available target=" + to_string(target));
    }
    string validationError;
    if (!validate_occ_patch_consistent(RE, target, &validationError)) {
        fail_test("planner stop condition invalid target occurrence target=" + to_string(target) + " error=" + validationError);
    }
}

vector<BoundaryMapEntry> identity_boundary_map(Vertex aOrig = 2, Vertex bOrig = 8) {
    return {
        BoundaryMapEntry{aOrig, aOrig},
        BoundaryMapEntry{bOrig, bOrig},
    };
}

UpdJob make_join_queue_job(RawSkelID leftSid, RawSkelID rightSid, Vertex aOrig = 2, Vertex bOrig = 8) {
    UpdJob job;
    job.kind = UpdJobKind::JOIN_PAIR;
    job.leftSid = leftSid;
    job.rightSid = rightSid;
    job.aOrig = aOrig;
    job.bOrig = bOrig;
    return job;
}

UpdJob make_integrate_queue_job(RawSkelID parentSid, RawSkelID childSid, Vertex aOrig = 2, Vertex bOrig = 8) {
    UpdJob job;
    job.kind = UpdJobKind::INTEGRATE_CHILD;
    job.parentSid = parentSid;
    job.childSid = childSid;
    job.bm = identity_boundary_map(aOrig, bOrig);
    return job;
}

SplitFixture make_split_fixture() {
    SplitFixture fixture;
    fixture.occ31 = new_occ(fixture.RE, 9);
    fixture.occ44 = new_occ(fixture.RE, 12);
    fixture.sid = new_skeleton(fixture.RE);

    RawSkeletonBuilder B;
    B.V = {
        make_builder_vertex(RawVertexKind::REAL, 2, 0),
        make_builder_vertex(RawVertexKind::REAL, 8, 0),
        make_builder_vertex(RawVertexKind::OCC_CENTER, 9, fixture.occ31),
        make_builder_vertex(RawVertexKind::REAL, 6, 0),
        make_builder_vertex(RawVertexKind::OCC_CENTER, 12, fixture.occ44),
        make_builder_vertex(RawVertexKind::REAL, 10, 0),
    };
    B.E.push_back(make_builder_edge(2, 3, RawEdgeKind::REAL_PORT));
    B.E.push_back(make_builder_edge(0, 3, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(3, 1, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(4, 5, RawEdgeKind::REAL_PORT));
    B.E.push_back(make_builder_edge(0, 5, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(5, 1, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(0, 1, RawEdgeKind::CORE_REAL));
    B.allocNbr[fixture.occ31] = {};
    B.allocNbr[fixture.occ44] = {};
    B.corePatchLocalEids[fixture.occ31] = {1, 2};
    B.corePatchLocalEids[fixture.occ44] = {4, 5};
    commit_skeleton(fixture.RE, fixture.sid, std::move(B), fixture.U);
    assert_engine_ok(fixture.RE, {fixture.sid}, {fixture.occ31, fixture.occ44});
    return fixture;
}

void classify_split_children(
    const SplitSeparationPairResult& sp,
    OccID firstOcc,
    OccID secondOcc,
    RawSkelID& sidFirst,
    RawSkelID& sidSecond,
    RawSkelID& sidBoundary
) {
    sidFirst = NIL_U32;
    sidSecond = NIL_U32;
    sidBoundary = NIL_U32;
    for (const SplitChildInfo& child : sp.child) {
        if (child_contains_occ(child, firstOcc)) {
            sidFirst = child.sid;
        } else if (child_contains_occ(child, secondOcc)) {
            sidSecond = child.sid;
        } else if (child.boundaryOnly) {
            sidBoundary = child.sid;
        }
    }
}

struct SplitLayout {
    RawSkelID anchor = NIL_U32;
    vector<RawSkelID> boundaryOnly;
    vector<RawSkelID> otherOccChildren;
};

struct PlannerTargetedScenario {
    RawEngine RE;
    RawUpdateCtx U;
    RawPlannerCtx ctx;
    OccID targetOcc = 0;
    vector<UpdJob> initialQueue;
    PlannerCoverageSummary preseedCoverage;
    unordered_map<OccID, OccPatchSignature> expectedSig;
    NormalizedPrep expectedTargetPrep;
    bool hasDirectAB = false;
};

void finalize_planner_scenario(PlannerTargetedScenario& scenario, const string& label);

SplitLayout collect_split_layout(const SplitSeparationPairResult& sp, OccID target) {
    SplitLayout layout;
    for (const SplitChildInfo& child : sp.child) {
        if (child_contains_occ(child, target)) {
            layout.anchor = child.sid;
        } else if (child.boundaryOnly) {
            layout.boundaryOnly.push_back(child.sid);
        } else {
            layout.otherOccChildren.push_back(child.sid);
        }
    }
    return layout;
}

void accumulate_coverage_summary(PlannerCoverageSummary& dst, const PlannerCoverageSummary& src) {
    dst.splitReadyCount += src.splitReadyCount;
    dst.boundaryOnlyChildCount += src.boundaryOnlyChildCount;
    dst.joinCandidateCount += src.joinCandidateCount;
    dst.integrateCandidateCount += src.integrateCandidateCount;
    dst.actualSplitHits += src.actualSplitHits;
    dst.actualJoinHits += src.actualJoinHits;
    dst.actualIntegrateHits += src.actualIntegrateHits;
    dst.splitChoiceCandidateCount += src.splitChoiceCandidateCount;
    dst.splitChoiceEvalCount += src.splitChoiceEvalCount;
    dst.splitChoiceTieCount += src.splitChoiceTieCount;
    dst.splitChoiceMulticlassCount += src.splitChoiceMulticlassCount;
    dst.splitChoiceFallbackCount += src.splitChoiceFallbackCount;
    for (const auto& [classCount, seen] : src.splitChoiceEquivClassCountHistogram) {
        dst.splitChoiceEquivClassCountHistogram[classCount] += seen;
    }
}

struct BranchLayout {
    vector<u32> coreLocalEids;
    u32 attachLocalV = NIL_U32;
};

BranchLayout append_core_branch(RawSkeletonBuilder& B, u32 sepA, u32 sepB, Vertex origBase, int pathLen) {
    BranchLayout branch;
    u32 prev = sepA;
    for (int step = 0; step < pathLen; ++step) {
        const u32 localV = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, origBase + static_cast<Vertex>(step), 0));
        if (branch.attachLocalV == NIL_U32) {
            branch.attachLocalV = localV;
        }
        branch.coreLocalEids.push_back(static_cast<u32>(B.E.size()));
        B.E.push_back(make_builder_edge(prev, localV, RawEdgeKind::CORE_REAL));
        prev = localV;
    }
    branch.coreLocalEids.push_back(static_cast<u32>(B.E.size()));
    B.E.push_back(make_builder_edge(prev, sepB, RawEdgeKind::CORE_REAL));
    return branch;
}

bool branch_orig_ranges_overlap(Vertex lhsBase, int lhsPathLen, Vertex rhsBase, int rhsPathLen) {
    const Vertex lhsEnd = lhsBase + static_cast<Vertex>(max(lhsPathLen, 1));
    const Vertex rhsEnd = rhsBase + static_cast<Vertex>(max(rhsPathLen, 1));
    return !(lhsEnd <= rhsBase || rhsEnd <= lhsBase);
}

Vertex disjoint_branch_base(Vertex preferredBase, int preferredPathLen, Vertex occupiedBase, int occupiedPathLen) {
    if (!branch_orig_ranges_overlap(preferredBase, preferredPathLen, occupiedBase, occupiedPathLen)) {
        return preferredBase;
    }
    return occupiedBase + static_cast<Vertex>(max(occupiedPathLen, 1)) + 8U;
}

void add_occ_center_edge(RawSkeletonBuilder& B, u32 center, u32 attach, bool bridgePort, BridgeRef bridgeRef, u8 side) {
    if (bridgePort) {
        B.E.push_back(make_builder_edge(center, attach, RawEdgeKind::BRIDGE_PORT, bridgeRef, side));
    } else {
        B.E.push_back(make_builder_edge(center, attach, RawEdgeKind::REAL_PORT));
    }
}

PlannerTargetedScenario make_split_ready_targeted_scenario(u32 seed, bool includeBoundaryArtifact) {
    PlannerTargetedScenario scenario;
    scenario.targetOcc = new_occ(scenario.RE, static_cast<Vertex>(500U + (seed % 97U)));
    const RawSkelID sid = new_skeleton(scenario.RE);

    RawSkeletonBuilder B;
    const u32 sepA = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 2, 0));
    const u32 sepB = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 8, 0));

    const int targetPathLen = 1 + static_cast<int>((seed >> 1U) & 1U);
    const int siblingPathLen = 1 + static_cast<int>((seed >> 2U) & 1U);
    const Vertex targetBase = 1000U + static_cast<Vertex>((seed % 211U) * 8U);
    const Vertex siblingBase = disjoint_branch_base(
        2000U + static_cast<Vertex>((seed % 233U) * 8U),
        siblingPathLen,
        targetBase,
        targetPathLen
    );

    const BranchLayout targetBranch = append_core_branch(B, sepA, sepB, targetBase, targetPathLen);
    const BranchLayout siblingBranch = append_core_branch(B, sepA, sepB, siblingBase, siblingPathLen);

    const u32 center = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, scenario.RE.occ.get(scenario.targetOcc).orig, scenario.targetOcc));
    add_occ_center_edge(
        B,
        center,
        targetBranch.attachLocalV,
        (seed & 1U) != 0U,
        7000U + (seed % 1024U),
        static_cast<u8>((seed >> 3U) & 1U)
    );

    vector<u32> targetCore = targetBranch.coreLocalEids;
    if (includeBoundaryArtifact) {
        B.E.push_back(make_builder_edge(sepA, sepB, RawEdgeKind::CORE_REAL));
        scenario.hasDirectAB = true;
    }

    B.allocNbr[scenario.targetOcc] = {};
    B.corePatchLocalEids[scenario.targetOcc] = std::move(targetCore);
    commit_skeleton(scenario.RE, sid, std::move(B), scenario.U);

    scenario.ctx.targetOcc = scenario.targetOcc;
    scenario.ctx.keepOcc = {scenario.targetOcc};
    scenario.expectedSig = capture_occ_signatures(scenario.RE, collect_live_occurrences(scenario.RE));
    scenario.expectedTargetPrep =
        normalize_prep(prepare_isolate_checked(scenario.RE, scenario.RE.occ.get(scenario.targetOcc).hostSkel, scenario.targetOcc, "targeted_split_ready_prepare"));
    require_test(
        discover_split_pair_from_support(scenario.RE, scenario.targetOcc).has_value(),
        "targeted_split_ready_prepare expected split-ready target"
    );
    return scenario;
}

PlannerTargetedScenario make_post_split_targeted_scenario(u32 seed, bool includeBoundaryArtifact) {
    PlannerTargetedScenario scenario;
    scenario.targetOcc = new_occ(scenario.RE, static_cast<Vertex>(800U + (seed % 91U)));
    const OccID keepOcc = new_occ(scenario.RE, static_cast<Vertex>(900U + (seed % 89U)));
    const RawSkelID sid = new_skeleton(scenario.RE);

    RawSkeletonBuilder B;
    const u32 sepA = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 2, 0));
    const u32 sepB = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 8, 0));

    const BranchLayout targetBranch = append_core_branch(
        B,
        sepA,
        sepB,
        3000U + static_cast<Vertex>((seed % 173U) * 6U),
        1 + static_cast<int>((seed >> 1U) & 1U)
    );
    const int keepPathLen = 1 + static_cast<int>((seed >> 2U) & 1U);
    const Vertex keepBase = disjoint_branch_base(
        4000U + static_cast<Vertex>((seed % 181U) * 6U),
        keepPathLen,
        3000U + static_cast<Vertex>((seed % 173U) * 6U),
        1 + static_cast<int>((seed >> 1U) & 1U)
    );
    const BranchLayout keepBranch = append_core_branch(
        B,
        sepA,
        sepB,
        keepBase,
        keepPathLen
    );

    const u32 targetCenter = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, scenario.RE.occ.get(scenario.targetOcc).orig, scenario.targetOcc));
    add_occ_center_edge(
        B,
        targetCenter,
        targetBranch.attachLocalV,
        (seed & 1U) != 0U,
        9000U + (seed % 2048U),
        static_cast<u8>((seed >> 3U) & 1U)
    );

    const u32 keepCenter = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, scenario.RE.occ.get(keepOcc).orig, keepOcc));
    add_occ_center_edge(
        B,
        keepCenter,
        keepBranch.attachLocalV,
        ((seed >> 4U) & 1U) != 0U,
        11000U + (seed % 2048U),
        static_cast<u8>((seed >> 5U) & 1U)
    );

    if (includeBoundaryArtifact) {
        B.E.push_back(make_builder_edge(sepA, sepB, RawEdgeKind::CORE_REAL));
        scenario.hasDirectAB = true;
    }

    B.allocNbr[scenario.targetOcc] = {};
    B.allocNbr[keepOcc] = {};
    B.corePatchLocalEids[scenario.targetOcc] = targetBranch.coreLocalEids;
    B.corePatchLocalEids[keepOcc] = keepBranch.coreLocalEids;
    commit_skeleton(scenario.RE, sid, std::move(B), scenario.U);

    scenario.ctx.targetOcc = scenario.targetOcc;
    scenario.ctx.keepOcc = {keepOcc};

    const SplitSeparationPairResult sp = split_checked(
        scenario.RE,
        sid,
        2,
        8,
        scenario.U,
        includeBoundaryArtifact ? "targeted_join_integrate_setup" : "targeted_join_setup"
    );
    deque<UpdJob> q;
    make_basic_hooks_for_target(scenario.ctx).afterSplit(sp, q);
    scenario.initialQueue.assign(q.begin(), q.end());
    for (const SplitChildInfo& child : sp.child) {
        if (child.boundaryOnly) {
            ++scenario.preseedCoverage.boundaryOnlyChildCount;
        }
    }

    const bool hasJoinJob = any_of(scenario.initialQueue.begin(), scenario.initialQueue.end(), [](const UpdJob& job) {
        return job.kind == UpdJobKind::JOIN_PAIR;
    });
    const bool hasIntegrateJob = any_of(scenario.initialQueue.begin(), scenario.initialQueue.end(), [](const UpdJob& job) {
        return job.kind == UpdJobKind::INTEGRATE_CHILD;
    });
    if (!hasJoinJob) {
        throw runtime_error("targeted join scenario did not produce JOIN_PAIR");
    }
    if (includeBoundaryArtifact && !hasIntegrateJob) {
        throw runtime_error("targeted integrate scenario did not produce INTEGRATE_CHILD");
    }

    scenario.expectedSig = capture_occ_signatures(scenario.RE, collect_live_occurrences(scenario.RE));
    scenario.expectedTargetPrep =
        normalize_prep(prepare_isolate_checked(scenario.RE, scenario.RE.occ.get(scenario.targetOcc).hostSkel, scenario.targetOcc, "targeted_join_prepare"));
    return scenario;
}

void finalize_planner_scenario(PlannerTargetedScenario& scenario, const string& label) {
    scenario.expectedSig = capture_occ_signatures(scenario.RE, collect_live_occurrences(scenario.RE));
    scenario.expectedTargetPrep = normalize_prep(
        prepare_isolate_checked(
            scenario.RE,
            scenario.RE.occ.get(scenario.targetOcc).hostSkel,
            scenario.targetOcc,
            label
        )
    );
}

void require_tie_ready_target(const RawEngine& RE, OccID targetOcc, const string& label) {
    const vector<pair<Vertex, Vertex>> pairs = enumerate_valid_split_pairs(RE, targetOcc);
    require_test(pairs.size() >= 2U, label + " expected at least two admissible split pairs");
}

PlannerTargetedScenario make_split_tie_targeted_scenario(u32 seed, bool structuralSymmetry, bool mixedFollowup) {
    PlannerTargetedScenario scenario;
    const Vertex targetOrig = structuralSymmetry
        ? 2201U
        : static_cast<Vertex>(2200U + (seed % 181U));
    scenario.targetOcc = new_occ(scenario.RE, targetOrig);
    (void)mixedFollowup;
    const RawSkelID sid = new_skeleton(scenario.RE);

    const Vertex cutAOrig = 4;
    const Vertex cutBOrig = 6;
    const Vertex outerOrig = structuralSymmetry
        ? 3001U
        : static_cast<Vertex>(3000U + (seed % 97U) * 5U);
    const Vertex targetMidOrig = structuralSymmetry
        ? 4001U
        : static_cast<Vertex>(4000U + (seed % 101U) * 5U);

    const bool outerFirst = ((seed >> 1U) & 1U) != 0U;
    const bool bridgePort = ((seed >> 3U) & 1U) != 0U;
    const BridgeRef targetBridgeRef = 26000U + (seed % 2048U);

    RawSkeletonBuilder B;
    const u32 sepA = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 2, 0));
    const u32 sepB = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 8, 0));
    const u32 cutA = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, cutAOrig, 0));
    const u32 cutB = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, cutBOrig, 0));
    B.E.push_back(make_builder_edge(sepA, cutA, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(cutB, sepB, RawEdgeKind::CORE_REAL));

    auto append_outer_lane = [&]() {
        const u32 outer = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, outerOrig, 0));
        B.E.push_back(make_builder_edge(sepA, outer, RawEdgeKind::CORE_REAL));
        B.E.push_back(make_builder_edge(outer, sepB, RawEdgeKind::CORE_REAL));
    };

    auto append_cut_lane = [&](Vertex midOrig, optional<OccID> occ, vector<u32>* coreLocalEids, bool bridge) {
        const u32 mid = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, midOrig, 0));

        const u32 e0 = static_cast<u32>(B.E.size());
        B.E.push_back(make_builder_edge(cutA, mid, RawEdgeKind::CORE_REAL));
        const u32 e1 = static_cast<u32>(B.E.size());
        B.E.push_back(make_builder_edge(mid, cutB, RawEdgeKind::CORE_REAL));

        if (coreLocalEids != nullptr) {
            coreLocalEids->push_back(e0);
            coreLocalEids->push_back(e1);
        }

        if (!occ.has_value()) {
            return;
        }

        const u32 center = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, scenario.RE.occ.get(*occ).orig, *occ));
        add_occ_center_edge(
            B,
            center,
            mid,
            bridge,
            targetBridgeRef + static_cast<BridgeRef>(midOrig % 17U),
            static_cast<u8>((seed + midOrig) & 1U)
        );
        B.allocNbr[*occ] = {};
        if (coreLocalEids != nullptr) {
            B.corePatchLocalEids[*occ] = *coreLocalEids;
        }
    };

    vector<u32> targetCore;
    if (outerFirst) {
        append_outer_lane();
    }
    append_cut_lane(targetMidOrig, scenario.targetOcc, &targetCore, bridgePort);
    if (!outerFirst) {
        append_outer_lane();
    }

    commit_skeleton(scenario.RE, sid, std::move(B), scenario.U);
    scenario.ctx.targetOcc = scenario.targetOcc;
    scenario.ctx.keepOcc = {scenario.targetOcc};
    finalize_planner_scenario(
        scenario,
        mixedFollowup ? "planner_tie_mixed_prepare" :
        (structuralSymmetry ? "split_tie_structural_prepare" : "split_tie_ready_prepare")
    );
    require_tie_ready_target(
        scenario.RE,
        scenario.targetOcc,
        mixedFollowup ? "planner_tie_mixed_prepare" :
        (structuralSymmetry ? "split_tie_structural_prepare" : "split_tie_ready_prepare")
    );
    return scenario;
}

PlannerTargetedScenario make_planner_tie_mixed_targeted_scenario(u32 seed) {
    PlannerTargetedScenario scenario;
    scenario.targetOcc = new_occ(scenario.RE, static_cast<Vertex>(2200U + (seed % 181U)));
    const OccID keepOcc = new_occ(scenario.RE, static_cast<Vertex>(2400U + (seed % 173U)));
    const RawSkelID sid = new_skeleton(scenario.RE);

    const Vertex outerOrig = 3001U;
    const Vertex targetMidOrig = 4001U;
    const bool outerFirst = ((seed >> 1U) & 1U) != 0U;

    RawSkeletonBuilder B;
    const u32 sepA = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 2, 0));
    const u32 sepB = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 8, 0));
    const u32 cutA = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 4, 0));
    const u32 cutB = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 6, 0));
    B.E.push_back(make_builder_edge(sepA, cutA, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(cutB, sepB, RawEdgeKind::CORE_REAL));

    auto append_outer_lane = [&](optional<OccID> occ, vector<u32>* coreLocalEids) {
        const u32 outer = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, outerOrig, 0));
        const u32 e0 = static_cast<u32>(B.E.size());
        B.E.push_back(make_builder_edge(sepA, outer, RawEdgeKind::CORE_REAL));
        const u32 e1 = static_cast<u32>(B.E.size());
        B.E.push_back(make_builder_edge(outer, sepB, RawEdgeKind::CORE_REAL));
        if (coreLocalEids != nullptr) {
            coreLocalEids->push_back(e0);
            coreLocalEids->push_back(e1);
        }
        if (!occ.has_value()) {
            return;
        }
        const u32 center = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, scenario.RE.occ.get(*occ).orig, *occ));
        add_occ_center_edge(B, center, outer, false, 0, 0);
        B.allocNbr[*occ] = {};
        if (coreLocalEids != nullptr) {
            B.corePatchLocalEids[*occ] = *coreLocalEids;
        }
    };

    auto append_lane = [&](Vertex midOrig, optional<OccID> occ, vector<u32>* coreLocalEids) {
        const u32 mid = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, midOrig, 0));
        const u32 e0 = static_cast<u32>(B.E.size());
        B.E.push_back(make_builder_edge(cutA, mid, RawEdgeKind::CORE_REAL));
        const u32 e1 = static_cast<u32>(B.E.size());
        B.E.push_back(make_builder_edge(mid, cutB, RawEdgeKind::CORE_REAL));
        if (coreLocalEids != nullptr) {
            coreLocalEids->push_back(e0);
            coreLocalEids->push_back(e1);
        }
        if (!occ.has_value()) {
            return;
        }
        const u32 center = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, scenario.RE.occ.get(*occ).orig, *occ));
        add_occ_center_edge(B, center, mid, false, 0, 0);
        B.allocNbr[*occ] = {};
        if (coreLocalEids != nullptr) {
            B.corePatchLocalEids[*occ] = *coreLocalEids;
        }
    };

    vector<u32> targetCore;
    vector<u32> keepCore;
    if (outerFirst) {
        append_outer_lane(keepOcc, &keepCore);
    }
    append_lane(targetMidOrig, scenario.targetOcc, &targetCore);
    B.E.push_back(make_builder_edge(cutA, cutB, RawEdgeKind::CORE_REAL));
    if (!outerFirst) {
        append_outer_lane(keepOcc, &keepCore);
    }

    commit_skeleton(scenario.RE, sid, std::move(B), scenario.U);
    scenario.ctx.targetOcc = scenario.targetOcc;
    scenario.ctx.keepOcc = {keepOcc};
    finalize_planner_scenario(scenario, "planner_tie_mixed_prepare");

    const SplitSeparationPairResult splitResult = split_checked(
        scenario.RE,
        sid,
        4,
        6,
        scenario.U,
        "planner_tie_mixed_setup"
    );
    deque<UpdJob> q;
    make_basic_hooks_for_target(scenario.ctx).afterSplit(splitResult, q);
    scenario.initialQueue.assign(q.begin(), q.end());
    for (const SplitChildInfo& child : splitResult.child) {
        if (child.boundaryOnly) {
            ++scenario.preseedCoverage.boundaryOnlyChildCount;
        }
    }

    const bool hasJoinJob = any_of(scenario.initialQueue.begin(), scenario.initialQueue.end(), [](const UpdJob& job) {
        return job.kind == UpdJobKind::JOIN_PAIR;
    });
    const bool hasIntegrateJob = any_of(scenario.initialQueue.begin(), scenario.initialQueue.end(), [](const UpdJob& job) {
        return job.kind == UpdJobKind::INTEGRATE_CHILD;
    });
    require_test(hasJoinJob, "planner_tie_mixed_setup expected JOIN_PAIR");
    require_test(hasIntegrateJob, "planner_tie_mixed_setup expected INTEGRATE_CHILD");
    return scenario;
}

PlannerTargetedScenario make_split_tie_symmetric_large_targeted_scenario(u32 seed) {
    PlannerTargetedScenario scenario;
    scenario.targetOcc = new_occ(scenario.RE, 5201U);
    const RawSkelID sid = new_skeleton(scenario.RE);

    const bool outerFirst = ((seed >> 1U) & 1U) != 0U;
    const bool middleFirst = ((seed >> 2U) & 1U) != 0U;

    RawSkeletonBuilder B;
    const u32 sepA = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 2, 0));
    const u32 sepB = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 8, 0));
    const u32 cutA = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 4, 0));
    const u32 cutB = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 6, 0));
    B.E.push_back(make_builder_edge(sepA, cutA, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(cutB, sepB, RawEdgeKind::CORE_REAL));

    auto append_outer_lane = [&](Vertex outerOrig) {
        const u32 outer = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, outerOrig, 0));
        B.E.push_back(make_builder_edge(sepA, outer, RawEdgeKind::CORE_REAL));
        B.E.push_back(make_builder_edge(outer, sepB, RawEdgeKind::CORE_REAL));
    };

    auto append_cut_lane = [&](Vertex midOrig, optional<OccID> occ, vector<u32>* coreLocalEids) {
        const u32 mid = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, midOrig, 0));
        const u32 e0 = static_cast<u32>(B.E.size());
        B.E.push_back(make_builder_edge(cutA, mid, RawEdgeKind::CORE_REAL));
        const u32 e1 = static_cast<u32>(B.E.size());
        B.E.push_back(make_builder_edge(mid, cutB, RawEdgeKind::CORE_REAL));
        if (coreLocalEids != nullptr) {
            coreLocalEids->push_back(e0);
            coreLocalEids->push_back(e1);
        }
        if (!occ.has_value()) {
            return;
        }
        const u32 center = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, scenario.RE.occ.get(*occ).orig, *occ));
        add_occ_center_edge(B, center, mid, false, 0, 0);
        B.allocNbr[*occ] = {};
        if (coreLocalEids != nullptr) {
            B.corePatchLocalEids[*occ] = *coreLocalEids;
        }
    };

    vector<u32> targetCore;
    if (outerFirst) {
        append_outer_lane(3001U);
        append_outer_lane(3002U);
    }
    if (middleFirst) {
        append_cut_lane(4002U, nullopt, nullptr);
        append_cut_lane(4001U, scenario.targetOcc, &targetCore);
    } else {
        append_cut_lane(4001U, scenario.targetOcc, &targetCore);
        append_cut_lane(4002U, nullopt, nullptr);
    }
    if (!outerFirst) {
        append_outer_lane(3001U);
        append_outer_lane(3002U);
    }

    commit_skeleton(scenario.RE, sid, std::move(B), scenario.U);
    scenario.ctx.targetOcc = scenario.targetOcc;
    scenario.ctx.keepOcc = {scenario.targetOcc};
    finalize_planner_scenario(scenario, "split_tie_symmetric_large_prepare");
    const vector<pair<Vertex, Vertex>> pairs = enumerate_valid_split_pairs(scenario.RE, scenario.targetOcc);
    require_test(pairs.size() >= 4U, "split_tie_symmetric_large_prepare expected >=4 admissible split pairs");
    return scenario;
}

PlannerTargetedScenario make_planner_tie_mixed_symmetric_targeted_scenario(u32 seed) {
    PlannerTargetedScenario scenario;
    scenario.targetOcc = new_occ(scenario.RE, 5301U);
    const OccID keepOcc = new_occ(scenario.RE, 5302U);
    const RawSkelID sid = new_skeleton(scenario.RE);
    const bool outerFirst = ((seed >> 1U) & 1U) != 0U;

    RawSkeletonBuilder B;
    const u32 sepA = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 2, 0));
    const u32 sepB = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 8, 0));
    const u32 cutA = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 4, 0));
    const u32 cutB = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 6, 0));
    B.E.push_back(make_builder_edge(sepA, cutA, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(cutB, sepB, RawEdgeKind::CORE_REAL));

    auto append_outer_lane = [&](Vertex outerOrig, optional<OccID> occ, vector<u32>* coreLocalEids) {
        const u32 outer = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, outerOrig, 0));
        const u32 e0 = static_cast<u32>(B.E.size());
        B.E.push_back(make_builder_edge(sepA, outer, RawEdgeKind::CORE_REAL));
        const u32 e1 = static_cast<u32>(B.E.size());
        B.E.push_back(make_builder_edge(outer, sepB, RawEdgeKind::CORE_REAL));
        if (coreLocalEids != nullptr) {
            coreLocalEids->push_back(e0);
            coreLocalEids->push_back(e1);
        }
        if (!occ.has_value()) {
            return;
        }
        const u32 center = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, scenario.RE.occ.get(*occ).orig, *occ));
        add_occ_center_edge(B, center, outer, false, 0, 0);
        B.allocNbr[*occ] = {};
        if (coreLocalEids != nullptr) {
            B.corePatchLocalEids[*occ] = *coreLocalEids;
        }
    };

    auto append_cut_lane = [&](Vertex midOrig, optional<OccID> occ, vector<u32>* coreLocalEids) {
        const u32 mid = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, midOrig, 0));
        const u32 e0 = static_cast<u32>(B.E.size());
        B.E.push_back(make_builder_edge(cutA, mid, RawEdgeKind::CORE_REAL));
        const u32 e1 = static_cast<u32>(B.E.size());
        B.E.push_back(make_builder_edge(mid, cutB, RawEdgeKind::CORE_REAL));
        if (coreLocalEids != nullptr) {
            coreLocalEids->push_back(e0);
            coreLocalEids->push_back(e1);
        }
        if (!occ.has_value()) {
            return;
        }
        const u32 center = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, scenario.RE.occ.get(*occ).orig, *occ));
        add_occ_center_edge(B, center, mid, false, 0, 0);
        B.allocNbr[*occ] = {};
        if (coreLocalEids != nullptr) {
            B.corePatchLocalEids[*occ] = *coreLocalEids;
        }
    };

    vector<u32> targetCore;
    vector<u32> keepCore;
    if (outerFirst) {
        append_outer_lane(3001U, keepOcc, &keepCore);
        append_outer_lane(3002U, nullopt, nullptr);
    }
    append_cut_lane(4001U, scenario.targetOcc, &targetCore);
    append_cut_lane(4002U, nullopt, nullptr);
    B.E.push_back(make_builder_edge(cutA, cutB, RawEdgeKind::CORE_REAL));
    if (!outerFirst) {
        append_outer_lane(3001U, keepOcc, &keepCore);
        append_outer_lane(3002U, nullopt, nullptr);
    }

    commit_skeleton(scenario.RE, sid, std::move(B), scenario.U);
    scenario.ctx.targetOcc = scenario.targetOcc;
    scenario.ctx.keepOcc = {keepOcc};
    finalize_planner_scenario(scenario, "planner_tie_mixed_symmetric_prepare");

    const SplitSeparationPairResult splitResult = split_checked(
        scenario.RE,
        sid,
        4,
        6,
        scenario.U,
        "planner_tie_mixed_symmetric_setup"
    );
    deque<UpdJob> q;
    make_basic_hooks_for_target(scenario.ctx).afterSplit(splitResult, q);
    scenario.initialQueue.assign(q.begin(), q.end());
    for (const SplitChildInfo& child : splitResult.child) {
        if (child.boundaryOnly) {
            ++scenario.preseedCoverage.boundaryOnlyChildCount;
        }
    }

    const bool hasJoinJob = any_of(scenario.initialQueue.begin(), scenario.initialQueue.end(), [](const UpdJob& job) {
        return job.kind == UpdJobKind::JOIN_PAIR;
    });
    const bool hasIntegrateJob = any_of(scenario.initialQueue.begin(), scenario.initialQueue.end(), [](const UpdJob& job) {
        return job.kind == UpdJobKind::INTEGRATE_CHILD;
    });
    require_test(hasJoinJob, "planner_tie_mixed_symmetric_setup expected JOIN_PAIR");
    require_test(hasIntegrateJob, "planner_tie_mixed_symmetric_setup expected INTEGRATE_CHILD");
    return scenario;
}

PlannerTargetedScenario make_canonical_collision_probe_targeted_scenario(u32 seed) {
    PlannerTargetedScenario scenario;
    scenario.targetOcc = new_occ(scenario.RE, 6201U);
    const RawSkelID sid = new_skeleton(scenario.RE);
    const bool outerFirst = ((seed >> 1U) & 1U) != 0U;
    const bool targetLaneFirst = ((seed >> 2U) & 1U) != 0U;
    const bool reverseOuterOrder = ((seed >> 3U) & 1U) != 0U;

    RawSkeletonBuilder B;
    const u32 sepA = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 2, 0));
    const u32 sepB = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 8, 0));
    const u32 cutA = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 4, 0));
    const u32 cutB = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 6, 0));
    B.E.push_back(make_builder_edge(sepA, cutA, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(cutB, sepB, RawEdgeKind::CORE_REAL));

    auto append_outer_lane = [&](Vertex outerOrig) {
        const u32 outer = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, outerOrig, 0));
        B.E.push_back(make_builder_edge(sepA, outer, RawEdgeKind::CORE_REAL));
        B.E.push_back(make_builder_edge(outer, sepB, RawEdgeKind::CORE_REAL));
    };

    auto append_cut_lane = [&](Vertex midOrig, optional<OccID> occ, vector<u32>* coreLocalEids) {
        const u32 mid = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, midOrig, 0));
        const u32 e0 = static_cast<u32>(B.E.size());
        B.E.push_back(make_builder_edge(cutA, mid, RawEdgeKind::CORE_REAL));
        const u32 e1 = static_cast<u32>(B.E.size());
        B.E.push_back(make_builder_edge(mid, cutB, RawEdgeKind::CORE_REAL));
        if (coreLocalEids != nullptr) {
            coreLocalEids->push_back(e0);
            coreLocalEids->push_back(e1);
        }
        if (!occ.has_value()) {
            return;
        }
        const u32 center = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, scenario.RE.occ.get(*occ).orig, *occ));
        add_occ_center_edge(B, center, mid, false, 0, 0);
        B.allocNbr[*occ] = {};
        if (coreLocalEids != nullptr) {
            B.corePatchLocalEids[*occ] = *coreLocalEids;
        }
    };

    vector<u32> targetCore;
    auto append_outer_block = [&]() {
        if (reverseOuterOrder) {
            append_outer_lane(3002U);
            append_outer_lane(3001U);
        } else {
            append_outer_lane(3001U);
            append_outer_lane(3002U);
        }
    };
    if (outerFirst) {
        append_outer_block();
    }
    if (targetLaneFirst) {
        append_cut_lane(4001U, scenario.targetOcc, &targetCore);
        append_cut_lane(4002U, nullopt, nullptr);
    } else {
        append_cut_lane(4002U, nullopt, nullptr);
        append_cut_lane(4001U, scenario.targetOcc, &targetCore);
    }
    if (!outerFirst) {
        append_outer_block();
    }

    commit_skeleton(scenario.RE, sid, std::move(B), scenario.U);
    scenario.ctx.targetOcc = scenario.targetOcc;
    scenario.ctx.keepOcc = {scenario.targetOcc};
    finalize_planner_scenario(scenario, "canonical_collision_probe_prepare");
    require_tie_ready_target(scenario.RE, scenario.targetOcc, "canonical_collision_probe_prepare");
    return scenario;
}

PlannerTargetedScenario make_split_tie_organic_symmetric_targeted_scenario(u32 seed) {
    return make_split_tie_symmetric_large_targeted_scenario(seed ^ 0x13579U);
}

PlannerTargetedScenario make_planner_tie_mixed_organic_targeted_scenario(u32 seed) {
    return make_planner_tie_mixed_symmetric_targeted_scenario(seed ^ 0x2468U);
}

PlannerTargetedScenario make_planner_tie_mixed_organic_compare_ready_targeted_scenario(u32 seed) {
    return make_split_tie_organic_symmetric_targeted_scenario(seed ^ 0x2468U);
}

PlannerTargetedScenario make_automorphism_probe_large_targeted_scenario(u32 seed) {
    return make_canonical_collision_probe_targeted_scenario(seed ^ 0xabcdeU);
}

PlannerTargetedScenario make_join_ready_targeted_scenario(u32 seed) {
    PlannerTargetedScenario scenario;
    scenario.targetOcc = new_occ(scenario.RE, static_cast<Vertex>(1200U + (seed % 101U)));
    const OccID keepOcc = new_occ(scenario.RE, static_cast<Vertex>(1400U + (seed % 113U)));
    const RawSkelID leftSid = new_skeleton(scenario.RE);
    const RawSkelID rightSid = new_skeleton(scenario.RE);

    {
        RawSkeletonBuilder B;
        const u32 sepA = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 2, 0));
        const u32 sepB = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 8, 0));
        const BranchLayout targetBranch = append_core_branch(
            B,
            sepA,
            sepB,
            5000U + static_cast<Vertex>((seed % 127U) * 7U),
            1 + static_cast<int>((seed >> 1U) & 1U)
        );
        const u32 center = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, scenario.RE.occ.get(scenario.targetOcc).orig, scenario.targetOcc));
        add_occ_center_edge(
            B,
            center,
            targetBranch.attachLocalV,
            (seed & 1U) != 0U,
            13000U + (seed % 2048U),
            static_cast<u8>((seed >> 2U) & 1U)
        );
        B.allocNbr[scenario.targetOcc] = {};
        B.corePatchLocalEids[scenario.targetOcc] = targetBranch.coreLocalEids;
        commit_skeleton(scenario.RE, leftSid, std::move(B), scenario.U);
    }

    {
        RawSkeletonBuilder B;
        const u32 sepA = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 2, 0));
        const u32 sepB = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 8, 0));
        const BranchLayout keepBranch = append_core_branch(
            B,
            sepA,
            sepB,
            6000U + static_cast<Vertex>((seed % 131U) * 7U),
            1 + static_cast<int>((seed >> 3U) & 1U)
        );
        const u32 center = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, scenario.RE.occ.get(keepOcc).orig, keepOcc));
        add_occ_center_edge(
            B,
            center,
            keepBranch.attachLocalV,
            ((seed >> 4U) & 1U) != 0U,
            15000U + (seed % 2048U),
            static_cast<u8>((seed >> 5U) & 1U)
        );
        B.allocNbr[keepOcc] = {};
        B.corePatchLocalEids[keepOcc] = keepBranch.coreLocalEids;
        commit_skeleton(scenario.RE, rightSid, std::move(B), scenario.U);
    }

    scenario.ctx.targetOcc = scenario.targetOcc;
    scenario.ctx.keepOcc = {keepOcc};
    scenario.initialQueue.push_back(make_join_queue_job(leftSid, rightSid, 2, 8));
    scenario.preseedCoverage.joinCandidateCount = 1U;
    finalize_planner_scenario(scenario, "join_ready_prepare");
    return scenario;
}

PlannerTargetedScenario make_integrate_ready_targeted_scenario(u32 seed) {
    PlannerTargetedScenario scenario;
    scenario.targetOcc = new_occ(scenario.RE, static_cast<Vertex>(1600U + (seed % 137U)));
    const RawSkelID parentSid = new_skeleton(scenario.RE);
    const RawSkelID childSid = new_skeleton(scenario.RE);
    const bool boundaryOnlyChild = ((seed >> 1U) & 1U) == 0U;

    {
        RawSkeletonBuilder B;
        const u32 sepA = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 2, 0));
        const u32 sepB = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 8, 0));
        const BranchLayout targetBranch = append_core_branch(
            B,
            sepA,
            sepB,
            7000U + static_cast<Vertex>((seed % 149U) * 5U),
            1 + static_cast<int>((seed >> 2U) & 1U)
        );
        const u32 center = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, scenario.RE.occ.get(scenario.targetOcc).orig, scenario.targetOcc));
        add_occ_center_edge(
            B,
            center,
            targetBranch.attachLocalV,
            (seed & 1U) != 0U,
            17000U + (seed % 2048U),
            static_cast<u8>((seed >> 3U) & 1U)
        );
        B.allocNbr[scenario.targetOcc] = {};
        B.corePatchLocalEids[scenario.targetOcc] = targetBranch.coreLocalEids;
        commit_skeleton(scenario.RE, parentSid, std::move(B), scenario.U);
    }

    {
        RawSkeletonBuilder B;
        const u32 sepA = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 2, 0));
        const u32 sepB = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 8, 0));
        if (boundaryOnlyChild) {
            B.E.push_back(make_builder_edge(sepA, sepB, RawEdgeKind::CORE_REAL));
            scenario.preseedCoverage.boundaryOnlyChildCount = 1U;
            scenario.hasDirectAB = true;
        } else {
            (void)append_core_branch(
                B,
                sepA,
                sepB,
                8000U + static_cast<Vertex>((seed % 151U) * 5U),
                1 + static_cast<int>((seed >> 4U) & 1U)
            );
        }
        commit_skeleton(scenario.RE, childSid, std::move(B), scenario.U);
    }

    scenario.ctx.targetOcc = scenario.targetOcc;
    scenario.ctx.keepOcc = {scenario.targetOcc};
    scenario.initialQueue.push_back(make_integrate_queue_job(parentSid, childSid, 2, 8));
    scenario.preseedCoverage.integrateCandidateCount = 1U;
    finalize_planner_scenario(scenario, "integrate_ready_prepare");
    return scenario;
}

PlannerTargetedScenario make_integrate_keepocc_ready_targeted_scenario(u32 seed) {
    PlannerTargetedScenario scenario;
    scenario.targetOcc = new_occ(scenario.RE, static_cast<Vertex>(1800U + (seed % 137U)));
    const OccID keepOcc = new_occ(scenario.RE, static_cast<Vertex>(2000U + (seed % 149U)));
    const RawSkelID parentSid = new_skeleton(scenario.RE);
    const RawSkelID childSid = new_skeleton(scenario.RE);

    {
        RawSkeletonBuilder B;
        const u32 sepA = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 2, 0));
        const u32 sepB = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 8, 0));
        const BranchLayout targetBranch = append_core_branch(
            B,
            sepA,
            sepB,
            9000U + static_cast<Vertex>((seed % 163U) * 5U),
            1 + static_cast<int>((seed >> 1U) & 1U)
        );
        const u32 center = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, scenario.RE.occ.get(scenario.targetOcc).orig, scenario.targetOcc));
        add_occ_center_edge(
            B,
            center,
            targetBranch.attachLocalV,
            (seed & 1U) != 0U,
            19000U + (seed % 2048U),
            static_cast<u8>((seed >> 2U) & 1U)
        );
        B.allocNbr[scenario.targetOcc] = {};
        B.corePatchLocalEids[scenario.targetOcc] = targetBranch.coreLocalEids;
        commit_skeleton(scenario.RE, parentSid, std::move(B), scenario.U);
    }

    {
        RawSkeletonBuilder B;
        const u32 sepA = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 2, 0));
        const u32 sepB = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 8, 0));
        const BranchLayout keepBranch = append_core_branch(
            B,
            sepA,
            sepB,
            10000U + static_cast<Vertex>((seed % 173U) * 5U),
            1 + static_cast<int>((seed >> 3U) & 1U)
        );
        const u32 center = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, scenario.RE.occ.get(keepOcc).orig, keepOcc));
        add_occ_center_edge(
            B,
            center,
            keepBranch.attachLocalV,
            ((seed >> 4U) & 1U) != 0U,
            21000U + (seed % 2048U),
            static_cast<u8>((seed >> 5U) & 1U)
        );
        B.allocNbr[keepOcc] = {};
        B.corePatchLocalEids[keepOcc] = keepBranch.coreLocalEids;
        commit_skeleton(scenario.RE, childSid, std::move(B), scenario.U);
    }

    scenario.ctx.targetOcc = scenario.targetOcc;
    scenario.ctx.keepOcc = {keepOcc};
    scenario.initialQueue.push_back(make_integrate_queue_job(parentSid, childSid, 2, 8));
    scenario.preseedCoverage.integrateCandidateCount = 1U;
    finalize_planner_scenario(scenario, "integrate_keepocc_ready_prepare");
    return scenario;
}

ScenarioFamily choose_targeted_family(u32 seed, int iter) {
    static const array<ScenarioFamily, 4> families = {{
        ScenarioFamily::SPLIT_READY,
        ScenarioFamily::SPLIT_WITH_BOUNDARY_ARTIFACT,
        ScenarioFamily::SPLIT_WITH_KEEPOCC_SIBLING,
        ScenarioFamily::SPLIT_WITH_JOIN_AND_INTEGRATE,
    }};
    const size_t offset = static_cast<size_t>(seed % families.size());
    return families[(offset + static_cast<size_t>(iter)) % families.size()];
}

ScenarioFamily choose_structural_family(u32 seed, int iter) {
    static const array<ScenarioFamily, 4> families = {{
        ScenarioFamily::JOIN_READY,
        ScenarioFamily::INTEGRATE_READY,
        ScenarioFamily::SPLIT_READY,
        ScenarioFamily::SPLIT_WITH_BOUNDARY_ARTIFACT,
    }};
    const size_t offset = static_cast<size_t>((seed >> 1U) % families.size());
    return families[(offset + static_cast<size_t>(iter)) % families.size()];
}

PlannerTargetedScenario make_targeted_planner_scenario(ScenarioFamily family, u32 seed) {
    switch (family) {
        case ScenarioFamily::SPLIT_READY:
            return make_split_ready_targeted_scenario(seed, false);
        case ScenarioFamily::SPLIT_WITH_BOUNDARY_ARTIFACT:
            return make_split_ready_targeted_scenario(seed, true);
        case ScenarioFamily::SPLIT_WITH_KEEPOCC_SIBLING:
            return make_post_split_targeted_scenario(seed, false);
        case ScenarioFamily::SPLIT_WITH_JOIN_AND_INTEGRATE:
            return make_post_split_targeted_scenario(seed, true);
        case ScenarioFamily::PLANNER_MIXED_TARGETED:
            return make_targeted_planner_scenario(choose_targeted_family(seed, 0), seed);
        case ScenarioFamily::JOIN_READY:
            return make_join_ready_targeted_scenario(seed);
        case ScenarioFamily::INTEGRATE_READY:
            return make_integrate_ready_targeted_scenario(seed);
        case ScenarioFamily::PLANNER_MIXED_STRUCTURAL:
            return make_targeted_planner_scenario(choose_structural_family(seed, 0), seed);
        case ScenarioFamily::SPLIT_TIE_READY:
            return make_split_tie_targeted_scenario(seed, false, false);
        case ScenarioFamily::SPLIT_TIE_STRUCTURAL:
            return make_split_tie_targeted_scenario(seed, true, false);
        case ScenarioFamily::PLANNER_TIE_MIXED:
            return make_planner_tie_mixed_targeted_scenario(seed);
        case ScenarioFamily::SPLIT_TIE_SYMMETRIC_LARGE:
            return make_split_tie_symmetric_large_targeted_scenario(seed);
        case ScenarioFamily::PLANNER_TIE_MIXED_SYMMETRIC:
            return make_planner_tie_mixed_symmetric_targeted_scenario(seed);
        case ScenarioFamily::CANONICAL_COLLISION_PROBE:
            return make_canonical_collision_probe_targeted_scenario(seed);
        case ScenarioFamily::SPLIT_TIE_ORGANIC_SYMMETRIC:
            return make_split_tie_organic_symmetric_targeted_scenario(seed);
        case ScenarioFamily::PLANNER_TIE_MIXED_ORGANIC:
            return make_planner_tie_mixed_organic_targeted_scenario(seed);
        case ScenarioFamily::PLANNER_TIE_MIXED_ORGANIC_COMPARE_READY:
            return make_planner_tie_mixed_organic_compare_ready_targeted_scenario(seed);
        case ScenarioFamily::AUTOMORPHISM_PROBE_LARGE:
            return make_automorphism_probe_large_targeted_scenario(seed);
        case ScenarioFamily::RANDOM:
            break;
    }
    throw runtime_error("random scenario family is not a targeted generator");
}

PlannerExecutionResult run_targeted_planner_iteration(
    PlannerTargetedScenario& scenario,
    const RawUpdaterRunOptions& runOptions,
    const string& label
) {
    PlannerExecutionResult result = run_planner_checked_capture(
        scenario.RE,
        scenario.ctx,
        scenario.U,
        runOptions,
        scenario.initialQueue.empty() ? nullptr : &scenario.initialQueue,
        label
    );
    accumulate_coverage_summary(result.coverage, scenario.preseedCoverage);
    assert_engine_bookkeeping_sane(scenario.RE);
    assert_planner_stop_condition(scenario.RE, scenario.targetOcc);
    assert_target_prepare_equivalent(
        scenario.RE,
        scenario.RE.occ.get(scenario.targetOcc).hostSkel,
        scenario.targetOcc,
        scenario.expectedTargetPrep
    );
    for (const auto& [occ, expected] : scenario.expectedSig) {
        require_test(scenario.RE.occ.a[occ].alive, "targeted planner expected live occurrence disappeared");
        assert_occ_signature_equivalent(scenario.RE, occ, expected);
    }
    return result;
}

GeneratedCase make_random_split_case(RawEngine& RE, RawUpdateCtx& U, mt19937& rng) {
    uniform_int_distribution<int> sideCntDist(2, 4);
    uniform_int_distribution<int> lenDist(1, 2);
    uniform_int_distribution<int> portKindDist(0, 1);
    bernoulli_distribution directABDist(0.5);

    GeneratedCase gc;
    gc.sid = new_skeleton(RE);
    gc.hasDirectAB = directABDist(rng);

    RawSkeletonBuilder B;
    const u32 sepA = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 2, 0));
    const u32 sepB = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 8, 0));

    const int sideCnt = sideCntDist(rng);
    const u32 bridgeRefBase = 1000;

    for (int i = 0; i < sideCnt; ++i) {
        const Vertex occOrig = static_cast<Vertex>(100 + i);
        const OccID occ = new_occ(RE, occOrig);
        gc.occs.push_back(occ);

        const Vertex xOrig = static_cast<Vertex>(1000 + i * 10 + 1);
        const Vertex yOrig = static_cast<Vertex>(1000 + i * 10 + 2);

        const u32 cx = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::REAL, xOrig, 0));
        u32 cy = NIL_U32;
        const bool len2 = (lenDist(rng) == 2);
        if (len2) {
            cy = static_cast<u32>(B.V.size());
            B.V.push_back(make_builder_vertex(RawVertexKind::REAL, yOrig, 0));
        }
        const u32 cc = static_cast<u32>(B.V.size());
        B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, occOrig, occ));

        const bool useBridgePort = (portKindDist(rng) == 1);
        const u32 attach = (len2 ? cy : cx);
        if (useBridgePort) {
            B.E.push_back(make_builder_edge(
                cc,
                attach,
                RawEdgeKind::BRIDGE_PORT,
                bridgeRefBase + static_cast<u32>(i),
                static_cast<u8>(i & 1)
            ));
        } else {
            B.E.push_back(make_builder_edge(cc, attach, RawEdgeKind::REAL_PORT));
        }

        vector<u32> coreE;
        u32 e = static_cast<u32>(B.E.size());
        B.E.push_back(make_builder_edge(sepA, cx, RawEdgeKind::CORE_REAL));
        coreE.push_back(e);
        if (len2) {
            e = static_cast<u32>(B.E.size());
            B.E.push_back(make_builder_edge(cx, cy, RawEdgeKind::CORE_REAL));
            coreE.push_back(e);
            e = static_cast<u32>(B.E.size());
            B.E.push_back(make_builder_edge(cy, sepB, RawEdgeKind::CORE_REAL));
            coreE.push_back(e);
        } else {
            e = static_cast<u32>(B.E.size());
            B.E.push_back(make_builder_edge(cx, sepB, RawEdgeKind::CORE_REAL));
            coreE.push_back(e);
        }

        B.allocNbr[occ] = {};
        B.corePatchLocalEids[occ] = coreE;
    }

    if (gc.hasDirectAB) {
        B.E.push_back(make_builder_edge(sepA, sepB, RawEdgeKind::CORE_REAL));
    }

    commit_skeleton(RE, gc.sid, std::move(B), U);
    assert_engine_ok(RE, {gc.sid}, gc.occs);

    for (OccID occ : gc.occs) {
        gc.expected.emplace(occ, normalize_prep(prepare_isolate_checked(RE, gc.sid, occ, "generated_prepare")));
    }
    return gc;
}

GeneratedCase make_generated_case_from_spec(RawEngine& RE, RawUpdateCtx& U, const FuzzSpec& rawSpec) {
    const FuzzSpec spec = normalize_fuzz_spec(rawSpec);
    mt19937 rng(spec.seed);
    bernoulli_distribution useBridgePort(0.4);
    bernoulli_distribution useBridgeSide(0.5);

    GeneratedCase gc;
    gc.sid = new_skeleton(RE);
    gc.hasDirectAB = (spec.directABCount > 0);

    RawSkeletonBuilder B;
    const u32 sepA = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 2, 0));
    const u32 sepB = static_cast<u32>(B.V.size());
    B.V.push_back(make_builder_vertex(RawVertexKind::REAL, 8, 0));

    vector<int> branchOrder(static_cast<size_t>(spec.branchCount));
    for (int i = 0; i < spec.branchCount; ++i) {
        branchOrder[static_cast<size_t>(i)] = i;
    }
    shuffle(branchOrder.begin(), branchOrder.end(), rng);

    vector<bool> isBoundary(static_cast<size_t>(spec.branchCount), false);
    for (int i = 0; i < spec.boundaryOnlyCount; ++i) {
        isBoundary[static_cast<size_t>(branchOrder[static_cast<size_t>(i)])] = true;
    }

    vector<int> occPerBranch(static_cast<size_t>(spec.branchCount), 0);
    vector<int> nonBoundary;
    for (int branch = 0; branch < spec.branchCount; ++branch) {
        if (!isBoundary[static_cast<size_t>(branch)]) {
            nonBoundary.push_back(branch);
        }
    }

    int occRemaining = spec.occCount;
    for (int branch : nonBoundary) {
        if (occRemaining == 0) {
            break;
        }
        ++occPerBranch[static_cast<size_t>(branch)];
        --occRemaining;
    }

    vector<int> extraSlots;
    for (int branch : nonBoundary) {
        for (int slot = 1; slot < spec.maxOccPerBranch; ++slot) {
            extraSlots.push_back(branch);
        }
    }
    shuffle(extraSlots.begin(), extraSlots.end(), rng);
    for (int branch : extraSlots) {
        if (occRemaining == 0) {
            break;
        }
        ++occPerBranch[static_cast<size_t>(branch)];
        --occRemaining;
    }
    if (occRemaining != 0) {
        throw runtime_error("normalized fuzz spec could not place all occurrences");
    }

    vector<Vertex> occOrigPool(static_cast<size_t>(spec.occCount));
    for (int i = 0; i < spec.occCount; ++i) {
        occOrigPool[static_cast<size_t>(i)] = static_cast<Vertex>(100U + static_cast<u32>(i));
    }
    for (int pairIdx = 0; pairIdx < spec.sharedOrigPairs; ++pairIdx) {
        occOrigPool[static_cast<size_t>(pairIdx * 2 + 1)] = occOrigPool[static_cast<size_t>(pairIdx * 2)];
    }
    shuffle(occOrigPool.begin(), occOrigPool.end(), rng);

    struct BranchBuild {
        vector<u32> pathVerts;
        vector<u32> coreLocalEids;
        vector<OccID> occs;
    };

    vector<BranchBuild> branches(static_cast<size_t>(spec.branchCount));
    const u32 bridgeRefBase = 70000U;
    int occCursor = 0;

    for (int branch = 0; branch < spec.branchCount; ++branch) {
        const int pathLen = uniform_int_distribution<int>(1, spec.maxPathLen)(rng);
        BranchBuild& info = branches[static_cast<size_t>(branch)];
        info.pathVerts.reserve(static_cast<size_t>(pathLen));

        for (int step = 0; step < pathLen; ++step) {
            const Vertex orig = static_cast<Vertex>(1000U + static_cast<u32>(branch) * 32U + static_cast<u32>(step));
            info.pathVerts.push_back(static_cast<u32>(B.V.size()));
            B.V.push_back(make_builder_vertex(RawVertexKind::REAL, orig, 0));
        }

        u32 prev = sepA;
        for (u32 localV : info.pathVerts) {
            info.coreLocalEids.push_back(static_cast<u32>(B.E.size()));
            B.E.push_back(make_builder_edge(prev, localV, RawEdgeKind::CORE_REAL));
            prev = localV;
        }
        info.coreLocalEids.push_back(static_cast<u32>(B.E.size()));
        B.E.push_back(make_builder_edge(prev, sepB, RawEdgeKind::CORE_REAL));

        for (int occIdx = 0; occIdx < occPerBranch[static_cast<size_t>(branch)]; ++occIdx) {
            const Vertex occOrig = occOrigPool[static_cast<size_t>(occCursor++)];
            const OccID occ = new_occ(RE, occOrig);
            gc.occs.push_back(occ);
            info.occs.push_back(occ);

            const u32 center = static_cast<u32>(B.V.size());
            B.V.push_back(make_builder_vertex(RawVertexKind::OCC_CENTER, occOrig, occ));
            const u32 attach = info.pathVerts[static_cast<size_t>(
                uniform_int_distribution<int>(0, static_cast<int>(info.pathVerts.size()) - 1)(rng)
            )];

            if (useBridgePort(rng)) {
                B.E.push_back(make_builder_edge(
                    center,
                    attach,
                    RawEdgeKind::BRIDGE_PORT,
                    bridgeRefBase + static_cast<u32>(gc.occs.size()),
                    static_cast<u8>(useBridgeSide(rng) ? 1U : 0U)
                ));
            } else {
                B.E.push_back(make_builder_edge(center, attach, RawEdgeKind::REAL_PORT));
            }
        }
    }

    for (int i = 0; i < spec.directABCount; ++i) {
        B.E.push_back(make_builder_edge(sepA, sepB, RawEdgeKind::CORE_REAL));
    }

    for (int i = 0; i < spec.multiEdgeCount; ++i) {
        const int branch = uniform_int_distribution<int>(0, spec.branchCount - 1)(rng);
        BranchBuild& info = branches[static_cast<size_t>(branch)];
        if (info.coreLocalEids.empty()) {
            continue;
        }
        const u32 srcLocalEid = info.coreLocalEids[static_cast<size_t>(
            uniform_int_distribution<int>(0, static_cast<int>(info.coreLocalEids.size()) - 1)(rng)
        )];
        const auto& src = B.E[static_cast<size_t>(srcLocalEid)];
        info.coreLocalEids.push_back(static_cast<u32>(B.E.size()));
        B.E.push_back(make_builder_edge(src.a, src.b, src.kind, src.br, src.side));
    }

    unordered_map<Vertex, vector<OccID>> occByOrig;
    for (int branch = 0; branch < spec.branchCount; ++branch) {
        BranchBuild& info = branches[static_cast<size_t>(branch)];
        for (OccID occ : info.occs) {
            B.allocNbr[occ] = {};
            B.corePatchLocalEids[occ] = info.coreLocalEids;
            occByOrig[RE.occ.get(occ).orig].push_back(occ);
        }
    }
    for (const auto& entry : occByOrig) {
        vector<OccID> sortedGroup = entry.second;
        sort(sortedGroup.begin(), sortedGroup.end());
        for (OccID occ : sortedGroup) {
            vector<OccID>& nbr = B.allocNbr[occ];
            for (OccID other : sortedGroup) {
                if (other != occ) {
                    nbr.push_back(other);
                }
            }
        }
    }

    assert_builder_basic(B);
    commit_skeleton(RE, gc.sid, std::move(B), U);
    assert_engine_ok(RE, {gc.sid}, gc.occs);
    assert_engine_bookkeeping_sane(RE);

    for (OccID occ : gc.occs) {
        gc.expected.emplace(occ, normalize_prep(prepare_isolate_checked(RE, gc.sid, occ, "generated_prepare")));
    }
    return gc;
}

void assert_target_prepare_equivalent(
    const RawEngine& RE,
    RawSkelID sid,
    OccID occ,
    const NormalizedPrep& expected
) {
    const NormalizedPrep got = normalize_prep(prepare_isolate_checked(RE, sid, occ, "assert_prepare"));
    if (!(got == expected)) {
        fail_test(
            "target prepare mismatch sid=" + to_string(sid) +
            " occ=" + to_string(occ) +
            " expected=" + format_normalized_prep(expected) +
            " got=" + format_normalized_prep(got)
        );
    }
}

void test_isolate_microcase() {
    RawEngine RE;
    RawUpdateCtx U;

    const OccID occ17 = new_occ(RE, 5);
    const OccID occ31 = new_occ(RE, 9);
    const OccID occ23 = new_occ(RE, 5);
    const RawSkelID sid = new_skeleton(RE);

    RawSkeletonBuilder B;
    B.V = {
        make_builder_vertex(RawVertexKind::REAL, 2, 0),
        make_builder_vertex(RawVertexKind::REAL, 8, 0),
        make_builder_vertex(RawVertexKind::REAL, 7, 0),
        make_builder_vertex(RawVertexKind::OCC_CENTER, 5, occ17),
        make_builder_vertex(RawVertexKind::REAL, 6, 0),
        make_builder_vertex(RawVertexKind::OCC_CENTER, 9, occ31),
    };
    B.E.push_back(make_builder_edge(3, 0, RawEdgeKind::REAL_PORT));
    B.E.push_back(make_builder_edge(3, 1, RawEdgeKind::BRIDGE_PORT, 41, 0));
    B.E.push_back(make_builder_edge(0, 2, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(2, 1, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(0, 1, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(5, 4, RawEdgeKind::REAL_PORT));
    B.E.push_back(make_builder_edge(4, 1, RawEdgeKind::CORE_REAL));
    B.allocNbr[occ17] = {occ23};
    B.allocNbr[occ31] = {};
    B.corePatchLocalEids[occ17] = {2, 3, 4};
    B.corePatchLocalEids[occ31] = {6};

    assert_builder_basic(B);
    commit_skeleton(RE, sid, std::move(B), U);
    assert_engine_ok(RE, {sid}, {occ17, occ31});

    const IsolatePrepared prep = prepare_isolate_checked(RE, sid, occ17, "isolate_micro_prepare");
    require_test(prep.occ == occ17, "isolate_micro prepare occ mismatch");
    require_test(prep.orig == 5, "isolate_micro prepare orig mismatch");
    require_test(prep.allocNbr.size() == 1U && prep.allocNbr[0] == occ23, "isolate_micro allocNbr mismatch");
    require_test(prep.ports.size() == 2U, "isolate_micro port count mismatch");
    require_test(prep.core.orig.size() == 3U, "isolate_micro core vertex count mismatch");
    require_test(prep.core.edges.size() == 3U, "isolate_micro core edge count mismatch");

    const IsolateVertexResult res = isolate_vertex(RE, sid, occ17, U);
    require_test(res.residualSkel == sid, "isolate_micro residual skeleton mismatch");
    require_test(res.occSkel != sid, "isolate_micro occurrence skeleton should differ");
    assert_engine_ok(RE, {sid, res.occSkel}, {occ17, occ31});
    require_test(
        RE.skel.get(sid).hostedOcc.size() == 1U && RE.skel.get(sid).hostedOcc[0] == occ31,
        "isolate_micro residual hosted occurrence mismatch"
    );
    require_test(
        RE.skel.get(res.occSkel).hostedOcc.size() == 1U && RE.skel.get(res.occSkel).hostedOcc[0] == occ17,
        "isolate_micro isolated hosted occurrence mismatch"
    );
}

void test_isolate_multiedge_microcase() {
    RawEngine RE;
    RawUpdateCtx U;

    const OccID occ = new_occ(RE, 5);
    const RawSkelID sid = new_skeleton(RE);

    RawSkeletonBuilder B;
    B.V = {
        make_builder_vertex(RawVertexKind::REAL, 2, 0),
        make_builder_vertex(RawVertexKind::REAL, 8, 0),
        make_builder_vertex(RawVertexKind::OCC_CENTER, 5, occ),
    };
    B.E.push_back(make_builder_edge(2, 0, RawEdgeKind::REAL_PORT));
    B.E.push_back(make_builder_edge(0, 1, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(0, 1, RawEdgeKind::CORE_REAL));
    B.allocNbr[occ] = {};
    B.corePatchLocalEids[occ] = {1, 2};
    commit_skeleton(RE, sid, std::move(B), U);

    const IsolatePrepared p = prepare_isolate_checked(RE, sid, occ, "isolate_multiedge_prepare");
    require_test(p.core.orig.size() == 2U, "isolate_multiedge_prepare core vertex count mismatch");
    require_test(p.core.edges.size() == 2U, "isolate_multiedge_prepare core edge count mismatch");
}

void test_split_microcase() {
    SplitFixture fixture = make_split_fixture();
    const SplitSeparationPairResult sp = split_checked(fixture.RE, fixture.sid, 2, 8, fixture.U, "split_micro");
    require_test(sp.child.size() == 3U, "split_micro unexpected child count");

    vector<RawSkelID> sids;
    for (const SplitChildInfo& child : sp.child) {
        sids.push_back(child.sid);
    }
    assert_engine_ok(fixture.RE, sids, {fixture.occ31, fixture.occ44});

    RawSkelID sid31 = NIL_U32;
    RawSkelID sid44 = NIL_U32;
    RawSkelID sidAB = NIL_U32;
    classify_split_children(sp, fixture.occ31, fixture.occ44, sid31, sid44, sidAB);
    require_test(sid31 != NIL_U32, "split_micro missing sid31");
    require_test(sid44 != NIL_U32, "split_micro missing sid44");
    require_test(sidAB != NIL_U32, "split_micro missing sidAB");
}

void test_join_microcase() {
    SplitFixture fixture = make_split_fixture();
    const NormalizedPrep expected = normalize_prep(
        prepare_isolate_checked(fixture.RE, fixture.sid, fixture.occ31, "join_micro_prepare")
    );

    const SplitSeparationPairResult sp = split_checked(fixture.RE, fixture.sid, 2, 8, fixture.U, "join_micro_split");
    RawSkelID sid31 = NIL_U32;
    RawSkelID sid44 = NIL_U32;
    RawSkelID sidAB = NIL_U32;
    classify_split_children(sp, fixture.occ31, fixture.occ44, sid31, sid44, sidAB);
    require_test(sid31 != NIL_U32 && sid44 != NIL_U32 && sidAB != NIL_U32, "join_micro missing split children");

    require_test(
        join_checked(fixture.RE, sid31, sid44, 2, 8, fixture.U, "join_micro").mergedSid == sid31,
        "join_micro mergedSid mismatch"
    );
    assert_engine_ok(fixture.RE, {sid31, sidAB}, {fixture.occ31, fixture.occ44});
    require_test(fixture.RE.skel.get(sid31).hostedOcc.size() == 2U, "join_micro hosted occurrence count mismatch");
    assert_target_prepare_equivalent(fixture.RE, sid31, fixture.occ31, expected);
}

void test_integrate_microcase() {
    RawEngine RE;
    RawUpdateCtx U;

    const OccID occ31 = new_occ(RE, 9);
    const OccID occ44 = new_occ(RE, 14);
    const RawSkelID parentSid = new_skeleton(RE);
    const RawSkelID childSid = new_skeleton(RE);

    {
        RawSkeletonBuilder B;
        B.V = {
            make_builder_vertex(RawVertexKind::REAL, 6, 0),
            make_builder_vertex(RawVertexKind::REAL, 8, 0),
            make_builder_vertex(RawVertexKind::REAL, 2, 0),
            make_builder_vertex(RawVertexKind::OCC_CENTER, 9, occ31),
        };
        B.E.push_back(make_builder_edge(3, 2, RawEdgeKind::REAL_PORT));
        B.E.push_back(make_builder_edge(2, 0, RawEdgeKind::CORE_REAL));
        B.E.push_back(make_builder_edge(0, 1, RawEdgeKind::CORE_REAL));
        B.allocNbr[occ31] = {};
        B.corePatchLocalEids[occ31] = {1, 2};
        commit_skeleton(RE, parentSid, std::move(B), U);
    }

    {
        RawSkeletonBuilder B;
        B.V = {
            make_builder_vertex(RawVertexKind::REAL, 6, 0),
            make_builder_vertex(RawVertexKind::REAL, 8, 0),
            make_builder_vertex(RawVertexKind::REAL, 11, 0),
            make_builder_vertex(RawVertexKind::OCC_CENTER, 14, occ44),
        };
        B.E.push_back(make_builder_edge(3, 2, RawEdgeKind::REAL_PORT));
        B.E.push_back(make_builder_edge(2, 0, RawEdgeKind::CORE_REAL));
        B.E.push_back(make_builder_edge(2, 1, RawEdgeKind::CORE_REAL));
        B.allocNbr[occ44] = {};
        B.corePatchLocalEids[occ44] = {1, 2};
        commit_skeleton(RE, childSid, std::move(B), U);
    }

    const OccPatchSignature sig31 = capture_occ_patch_signature(RE, occ31);
    const OccPatchSignature sig44 = capture_occ_patch_signature(RE, occ44);

    assert_engine_ok(RE, {parentSid, childSid}, {occ31, occ44});
    require_test(
        integrate_checked(RE, parentSid, childSid, identity_boundary_map(6, 8), U, "integrate_micro").mergedSid == parentSid,
        "integrate_micro mergedSid mismatch"
    );
    assert_engine_ok(RE, {parentSid}, {occ31, occ44});
    require_test(RE.skel.get(parentSid).hostedOcc.size() == 2U, "integrate_micro hosted occurrence count mismatch");
    require_test(capture_occ_patch_signature(RE, occ31) == sig31, "integrate_micro target signature mismatch");
    require_test(capture_occ_patch_signature(RE, occ44) == sig44, "integrate_micro keep signature mismatch");
}

void test_planner_microcase(const TestOptions& options) {
    RawEngine RE;
    RawUpdateCtx U;

    const OccID occ31 = new_occ(RE, 9);
    const OccID occ44 = new_occ(RE, 12);
    const RawSkelID sid = new_skeleton(RE);

    RawSkeletonBuilder B;
    B.V = {
        make_builder_vertex(RawVertexKind::REAL, 2, 0),
        make_builder_vertex(RawVertexKind::REAL, 8, 0),
        make_builder_vertex(RawVertexKind::OCC_CENTER, 9, occ31),
        make_builder_vertex(RawVertexKind::REAL, 6, 0),
        make_builder_vertex(RawVertexKind::OCC_CENTER, 12, occ44),
        make_builder_vertex(RawVertexKind::REAL, 10, 0),
    };
    B.E.push_back(make_builder_edge(2, 3, RawEdgeKind::REAL_PORT));
    B.E.push_back(make_builder_edge(0, 3, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(3, 1, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(4, 5, RawEdgeKind::REAL_PORT));
    B.E.push_back(make_builder_edge(0, 5, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(5, 1, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(0, 1, RawEdgeKind::CORE_REAL));
    B.allocNbr[occ31] = {};
    B.allocNbr[occ44] = {};
    B.corePatchLocalEids[occ31] = {1, 2};
    B.corePatchLocalEids[occ44] = {4, 5};
    commit_skeleton(RE, sid, std::move(B), U);

    RawPlannerCtx ctx;
    ctx.targetOcc = occ31;
    ctx.keepOcc = {occ31};
    const RawUpdaterRunOptions runOptions{options.stepBudget};
    run_planner_checked(RE, ctx, U, runOptions, "planner_micro");
    assert_occ_patch_consistent(RE, occ31);
}

FuzzSpec make_random_spec(FuzzMode mode, u32 seed, const TestOptions& options) {
    mt19937 rng(seed);
    const PreconditionBiasConfig bias = resolve_precondition_bias(options);

    FuzzSpec spec;
    spec.mode = mode;
    spec.seed = seed;
    spec.stepBudget = options.stepBudget;
    spec.maxOccPerBranch = 2;
    spec.biasSplit = bias.split;
    spec.biasJoin = bias.join;
    spec.biasIntegrate = bias.integrate;

    int branchMin = 2;
    int branchMax = 6;
    int boundaryMax = 1;
    int maxPathLen = 3;
    int directABMax = 1;
    int multiEdgeMax = 2;
    int sharedOrigMax = 1;
    int keepMax = 2;
    int opMin = 1;
    int opMax = 2;

    switch (mode) {
        case FuzzMode::ISOLATE_ONLY:
            branchMax = 6;
            boundaryMax = 1;
            opMax = 3;
            break;
        case FuzzMode::SPLIT_ONLY:
            branchMax = 7;
            boundaryMax = 2;
            directABMax = 2;
            multiEdgeMax = 3;
            break;
        case FuzzMode::JOIN_ONLY:
            branchMin = 3;
            branchMax = 7;
            boundaryMax = 2;
            opMax = 2;
            directABMax = 2;
            multiEdgeMax = 3;
            break;
        case FuzzMode::INTEGRATE_ONLY:
            branchMin = 3;
            branchMax = 7;
            boundaryMax = 3;
            directABMax = 2;
            multiEdgeMax = 3;
            break;
        case FuzzMode::ISOLATE_THEN_SPLIT:
            branchMin = 3;
            branchMax = 7;
            boundaryMax = 2;
            opMax = 2;
            directABMax = 2;
            multiEdgeMax = 3;
            break;
        case FuzzMode::SPLIT_THEN_JOIN:
            branchMin = 3;
            branchMax = 7;
            boundaryMax = 2;
            opMax = 3;
            directABMax = 2;
            multiEdgeMax = 3;
            sharedOrigMax = 2;
            break;
        case FuzzMode::SPLIT_THEN_INTEGRATE:
            branchMin = 3;
            branchMax = 7;
            boundaryMax = 3;
            opMax = 3;
            directABMax = 2;
            multiEdgeMax = 3;
            break;
        case FuzzMode::MIXED_PLANNER:
            branchMin = 4;
            branchMax = 8;
            boundaryMax = 3;
            maxPathLen = 4;
            directABMax = 2;
            multiEdgeMax = 4;
            sharedOrigMax = 2;
            keepMax = 3;
            opMin = 2;
            opMax = 4;
            break;
    }

    spec.branchCount = uniform_int_distribution<int>(branchMin, branchMax)(rng);
    const int minNonBoundary =
        (mode == FuzzMode::INTEGRATE_ONLY || mode == FuzzMode::SPLIT_THEN_INTEGRATE) ? 1 : 2;
    const int boundaryCap = min(boundaryMax, spec.branchCount - minNonBoundary);
    spec.boundaryOnlyCount = uniform_int_distribution<int>(has_mode_min_boundary(mode) ? 1 : 0, max(boundaryCap, has_mode_min_boundary(mode) ? 1 : 0))(rng);
    spec.maxPathLen = uniform_int_distribution<int>(1, maxPathLen)(rng);

    const int occCapacity = (spec.branchCount - spec.boundaryOnlyCount) * spec.maxOccPerBranch;
    const int minOcc =
        (mode == FuzzMode::INTEGRATE_ONLY || mode == FuzzMode::SPLIT_THEN_INTEGRATE) ? 1 : 2;
    spec.occCount = uniform_int_distribution<int>(minOcc, occCapacity)(rng);
    spec.directABCount = uniform_int_distribution<int>(0, directABMax)(rng);
    spec.multiEdgeCount = uniform_int_distribution<int>(0, multiEdgeMax)(rng);
    spec.sharedOrigPairs = uniform_int_distribution<int>(0, min(sharedOrigMax, spec.occCount / 2))(rng);
    spec.keepOccCount = uniform_int_distribution<int>(1, min(keepMax, spec.occCount))(rng);
    spec.opCount = uniform_int_distribution<int>(opMin, opMax)(rng);

    switch (options.weightProfile) {
        case WeightProfile::RANDOM:
            break;
        case WeightProfile::WEIGHTED_SPLIT_HEAVY:
            spec.occCount = max(spec.occCount, occCapacity - 1);
            spec.keepOccCount = 1;
            spec.opCount = max(spec.opCount, opMax);
            spec.boundaryOnlyCount = max(has_mode_min_boundary(mode) ? 1 : 0, spec.boundaryOnlyCount / 2);
            break;
        case WeightProfile::WEIGHTED_JOIN_HEAVY:
            spec.occCount = occCapacity;
            spec.keepOccCount = 1;
            spec.sharedOrigPairs = min(sharedOrigMax, spec.occCount / 2);
            spec.boundaryOnlyCount = max(has_mode_min_boundary(mode) ? 1 : 0, min(spec.boundaryOnlyCount, 1));
            spec.opCount = max(spec.opCount, opMax);
            break;
        case WeightProfile::WEIGHTED_INTEGRATE_HEAVY:
            spec.boundaryOnlyCount = boundaryCap;
            spec.opCount = max(spec.opCount, opMax);
            spec.directABCount = max(spec.directABCount, directABMax);
            break;
        case WeightProfile::ARTIFACT_HEAVY:
            spec.boundaryOnlyCount = boundaryCap;
            spec.directABCount = directABMax;
            spec.opCount = max(spec.opCount, opMax);
            break;
        case WeightProfile::MULTIEDGE_HEAVY:
            spec.multiEdgeCount = multiEdgeMax;
            spec.directABCount = max(spec.directABCount, directABMax);
            break;
    }

    if (mode == FuzzMode::MIXED_PLANNER) {
        spec.directABCount = max(spec.directABCount, spec.biasIntegrate / 3);
        spec.sharedOrigPairs = min(spec.occCount / 2, spec.sharedOrigPairs + spec.biasJoin / 4);
        spec.keepOccCount = min(spec.occCount, max(spec.keepOccCount, 1 + max(spec.biasJoin, spec.biasIntegrate) / 3));
    }
    return normalize_fuzz_spec(spec);
}

unordered_map<OccID, OccPatchSignature> capture_occ_signatures(const RawEngine& RE, const vector<OccID>& occs) {
    unordered_map<OccID, OccPatchSignature> out;
    for (OccID occ : occs) {
        out.emplace(occ, capture_occ_patch_signature(RE, occ));
    }
    return out;
}

vector<OccID> choose_keep_occ(const vector<OccID>& occs, int keepCount, OccID target, mt19937& rng) {
    vector<OccID> order = occs;
    shuffle(order.begin(), order.end(), rng);

    vector<OccID> keep;
    keep.push_back(target);
    for (OccID occ : order) {
        if (occ == target) {
            continue;
        }
        if (static_cast<int>(keep.size()) >= keepCount) {
            break;
        }
        keep.push_back(occ);
    }
    return keep;
}

PlannerTargetedScenario make_legacy_generated_planner_scenario(const FuzzSpec& spec) {
    mt19937 rng(spec.seed ^ 0xC0DEC0DEU);
    PlannerTargetedScenario scenario;
    GeneratedCase gc = make_generated_case_from_spec(scenario.RE, scenario.U, spec);
    scenario.hasDirectAB = gc.hasDirectAB;

    const OccID target = gc.occs[static_cast<size_t>(
        uniform_int_distribution<int>(0, static_cast<int>(gc.occs.size()) - 1)(rng)
    )];
    scenario.targetOcc = target;
    scenario.ctx.targetOcc = target;
    for (OccID occ : choose_keep_occ(gc.occs, min(spec.keepOccCount, static_cast<int>(gc.occs.size())), target, rng)) {
        scenario.ctx.keepOcc.insert(occ);
    }

    scenario.expectedSig = capture_occ_signatures(scenario.RE, gc.occs);
    scenario.expectedTargetPrep = gc.expected.at(target);
    return scenario;
}

int choose_weighted_index(mt19937& rng, const vector<int>& weights) {
    const int totalWeight = accumulate(weights.begin(), weights.end(), 0);
    if (totalWeight <= 0) {
        return 0;
    }
    int pick = uniform_int_distribution<int>(1, totalWeight)(rng);
    for (size_t i = 0; i < weights.size(); ++i) {
        pick -= weights[i];
        if (pick <= 0) {
            return static_cast<int>(i);
        }
    }
    return static_cast<int>(weights.size() - 1U);
}

PlannerTargetedScenario make_non_targeted_planner_scenario(const FuzzSpec& spec) {
    mt19937 rng(spec.seed ^ 0x91A2B3C4U);
    const int legacyWeight = max(4, 14 - (spec.biasSplit + spec.biasJoin + spec.biasIntegrate));
    const vector<int> weights = {
        legacyWeight,
        max(1, spec.biasSplit),
        max(0, (spec.biasSplit + spec.biasIntegrate) / 2),
        max(0, spec.biasJoin),
        max(0, spec.biasIntegrate),
        max(0, max(spec.biasJoin, spec.biasIntegrate) / 2),
    };

    switch (choose_weighted_index(rng, weights)) {
        case 0:
            return make_legacy_generated_planner_scenario(spec);
        case 1:
            return make_split_ready_targeted_scenario(mix_seed(spec.seed, 11), false);
        case 2:
            return make_split_ready_targeted_scenario(mix_seed(spec.seed, 17), true);
        case 3:
            return make_join_ready_targeted_scenario(mix_seed(spec.seed, 23));
        case 4:
            return make_integrate_ready_targeted_scenario(mix_seed(spec.seed, 29));
        case 5:
            return make_targeted_planner_scenario(choose_structural_family(spec.seed, spec.opCount), mix_seed(spec.seed, 31));
    }
    return make_legacy_generated_planner_scenario(spec);
}

void assert_occurrences_match_initial(
    const RawEngine& RE,
    const vector<OccID>& occs,
    const unordered_map<OccID, NormalizedPrep>& expectedPrep,
    const unordered_map<OccID, OccPatchSignature>& expectedSig
) {
    for (OccID occ : occs) {
        const RawSkelID sid = RE.occ.get(occ).hostSkel;
        assert_target_prepare_equivalent(RE, sid, occ, expectedPrep.at(occ));
        assert_occ_signature_equivalent(RE, occ, expectedSig.at(occ));
    }
}

void run_isolate_only_iteration(const FuzzSpec& spec) {
    mt19937 rng(spec.seed ^ 0xA51C0DEU);
    RawEngine RE;
    RawUpdateCtx U;
    GeneratedCase gc = make_generated_case_from_spec(RE, U, spec);
    const auto sigs = capture_occ_signatures(RE, gc.occs);

    for (int step = 0; step < spec.opCount; ++step) {
        vector<OccID> candidates;
        for (OccID occ : collect_live_occurrences(RE)) {
            const RawSkelID sid = RE.occ.get(occ).hostSkel;
            if (RE.skel.get(sid).hostedOcc.size() > 1U) {
                candidates.push_back(occ);
            }
        }
        if (candidates.empty()) {
            break;
        }

        const OccID target = candidates[static_cast<size_t>(
            uniform_int_distribution<int>(0, static_cast<int>(candidates.size()) - 1)(rng)
        )];
        const RawSkelID sid = RE.occ.get(target).hostSkel;
        assert_engine_bookkeeping_sane(RE);
        isolate_vertex(RE, sid, target, U);
        assert_engine_bookkeeping_sane(RE);
        assert_target_prepare_equivalent(RE, RE.occ.get(target).hostSkel, target, gc.expected.at(target));
        assert_occ_signature_equivalent(RE, target, sigs.at(target));
    }
}

void run_split_only_iteration(const FuzzSpec& spec) {
    RawEngine RE;
    RawUpdateCtx U;
    GeneratedCase gc = make_generated_case_from_spec(RE, U, spec);
    const auto sigs = capture_occ_signatures(RE, gc.occs);

    const SplitSeparationPairResult sp = split_checked(RE, gc.sid, 2, 8, U, "split_only");
    if (sp.child.size() < 2U) {
        fail_test("split_only expected at least two split children");
    }
    assert_engine_bookkeeping_sane(RE);
    assert_occurrences_match_initial(RE, gc.occs, gc.expected, sigs);
}

void run_join_only_iteration(const FuzzSpec& spec) {
    mt19937 rng(spec.seed ^ 0x501A2U);
    RawEngine RE;
    RawUpdateCtx U;
    GeneratedCase gc = make_generated_case_from_spec(RE, U, spec);
    const auto sigs = capture_occ_signatures(RE, gc.occs);
    const OccID target = gc.occs.front();

    const SplitSeparationPairResult sp = split_checked(RE, gc.sid, 2, 8, U, "join_only_split");
    SplitLayout layout = collect_split_layout(sp, target);
    if (layout.anchor == NIL_U32) {
        fail_test("join_only missing anchor split child");
    }

    shuffle(layout.otherOccChildren.begin(), layout.otherOccChildren.end(), rng);
    const int joinCount = min(spec.opCount, static_cast<int>(layout.otherOccChildren.size()));
    for (int i = 0; i < joinCount; ++i) {
        join_checked(RE, layout.anchor, layout.otherOccChildren[static_cast<size_t>(i)], 2, 8, U, "join_only");
        assert_engine_bookkeeping_sane(RE);
        const vector<OccID> hosted = RE.skel.get(layout.anchor).hostedOcc;
        assert_occurrences_match_initial(RE, hosted, gc.expected, sigs);
    }
}

void run_integrate_only_iteration(const FuzzSpec& spec) {
    mt19937 rng(spec.seed ^ 0x1A7E6U);
    RawEngine RE;
    RawUpdateCtx U;
    GeneratedCase gc = make_generated_case_from_spec(RE, U, spec);
    const auto sigs = capture_occ_signatures(RE, gc.occs);
    const OccID target = gc.occs.front();

    const SplitSeparationPairResult sp = split_checked(RE, gc.sid, 2, 8, U, "integrate_only_split");
    SplitLayout layout = collect_split_layout(sp, target);
    if (layout.anchor == NIL_U32) {
        fail_test("integrate_only missing anchor split child");
    }

    shuffle(layout.boundaryOnly.begin(), layout.boundaryOnly.end(), rng);
    const int integrateCount = min(spec.opCount, static_cast<int>(layout.boundaryOnly.size()));
    for (int i = 0; i < integrateCount; ++i) {
        integrate_checked(RE, layout.anchor, layout.boundaryOnly[static_cast<size_t>(i)], identity_boundary_map(), U, "integrate_only");
        assert_engine_bookkeeping_sane(RE);
        const vector<OccID> hosted = RE.skel.get(layout.anchor).hostedOcc;
        assert_occurrences_match_initial(RE, hosted, gc.expected, sigs);
    }
}

void run_isolate_split_iteration(const FuzzSpec& spec) {
    RawEngine RE;
    RawUpdateCtx U;
    GeneratedCase gc = make_generated_case_from_spec(RE, U, spec);
    const auto sigs = capture_occ_signatures(RE, gc.occs);
    const OccID target = gc.occs.back();

    const RawSkelID sid = RE.occ.get(target).hostSkel;
    const IsolateVertexResult ir = isolate_vertex(RE, sid, target, U);
    assert_engine_bookkeeping_sane(RE);
    assert_target_prepare_equivalent(RE, ir.occSkel, target, gc.expected.at(target));
    assert_occ_signature_equivalent(RE, target, sigs.at(target));

    if (ir.residualSkel == NIL_U32 || RE.skel.get(ir.residualSkel).hostedOcc.size() < 2U) {
        return;
    }

    const OccID residualProbe = RE.skel.get(ir.residualSkel).hostedOcc.front();
    const optional<pair<Vertex, Vertex>> sep = discover_split_pair_from_support(RE, residualProbe);
    if (!sep.has_value()) {
        return;
    }

    const SplitSeparationPairResult sp = split_checked(RE, ir.residualSkel, sep->first, sep->second, U, "isolate_split");
    if (sp.child.size() < 2U) {
        fail_test("isolate_split expected at least two split children");
    }
    assert_engine_bookkeeping_sane(RE);

    vector<OccID> residualOcc = RE.skel.get(ir.residualSkel).hostedOcc;
    for (const SplitChildInfo& child : sp.child) {
        for (OccID occ : child.hostedOcc) {
            residualOcc.push_back(occ);
        }
    }
    sort(residualOcc.begin(), residualOcc.end());
    residualOcc.erase(unique(residualOcc.begin(), residualOcc.end()), residualOcc.end());
    for (OccID occ : residualOcc) {
        assert_target_prepare_equivalent(RE, RE.occ.get(occ).hostSkel, occ, gc.expected.at(occ));
        assert_occ_signature_equivalent(RE, occ, sigs.at(occ));
    }
}

void run_split_join_iteration(const FuzzSpec& spec) {
    RawEngine RE;
    RawUpdateCtx U;
    GeneratedCase gc = make_generated_case_from_spec(RE, U, spec);
    const auto sigs = capture_occ_signatures(RE, gc.occs);
    const OccID target = gc.occs.front();

    const SplitSeparationPairResult sp = split_checked(RE, gc.sid, 2, 8, U, "split_join_split");
    SplitLayout layout = collect_split_layout(sp, target);
    if (layout.anchor == NIL_U32) {
        fail_test("split_join missing anchor split child");
    }

    for (RawSkelID sidAB : layout.boundaryOnly) {
        integrate_checked(RE, layout.anchor, sidAB, identity_boundary_map(), U, "split_join_integrate");
    }
    for (RawSkelID sidX : layout.otherOccChildren) {
        join_checked(RE, layout.anchor, sidX, 2, 8, U, "split_join_join");
    }

    assert_engine_bookkeeping_sane(RE);
    if (RE.skel.get(layout.anchor).hostedOcc.size() != gc.occs.size()) {
        fail_test("split_join final hosted occurrence count mismatch");
    }
    assert_occurrences_match_initial(RE, gc.occs, gc.expected, sigs);
}

void run_split_integrate_iteration(const FuzzSpec& spec) {
    RawEngine RE;
    RawUpdateCtx U;
    GeneratedCase gc = make_generated_case_from_spec(RE, U, spec);
    const auto sigs = capture_occ_signatures(RE, gc.occs);
    const OccID target = gc.occs.front();

    const SplitSeparationPairResult sp = split_checked(RE, gc.sid, 2, 8, U, "split_integrate_split");
    SplitLayout layout = collect_split_layout(sp, target);
    if (layout.anchor == NIL_U32) {
        fail_test("split_integrate missing anchor split child");
    }

    for (RawSkelID sidAB : layout.boundaryOnly) {
        integrate_checked(RE, layout.anchor, sidAB, identity_boundary_map(), U, "split_integrate");
    }
    assert_engine_bookkeeping_sane(RE);
    assert_occurrences_match_initial(RE, RE.skel.get(layout.anchor).hostedOcc, gc.expected, sigs);
}

void run_mixed_planner_iteration(const FuzzSpec& spec) {
    PlannerTargetedScenario scenario = make_non_targeted_planner_scenario(spec);
    record_generated_artifact_metadata(scenario.hasDirectAB, 0U);
    const PlannerExecutionResult result = run_targeted_planner_iteration(
        scenario,
        planner_run_options(spec.stepBudget, active_test_options()),
        "mixed_planner"
    );
    record_planner_execution_stats(result, current_fuzz_iteration_ordinal());
}

void run_fuzz_spec(const FuzzSpec& spec) {
    switch (spec.mode) {
        case FuzzMode::ISOLATE_ONLY:
            run_isolate_only_iteration(spec);
            return;
        case FuzzMode::SPLIT_ONLY:
            run_split_only_iteration(spec);
            return;
        case FuzzMode::JOIN_ONLY:
            run_join_only_iteration(spec);
            return;
        case FuzzMode::INTEGRATE_ONLY:
            run_integrate_only_iteration(spec);
            return;
        case FuzzMode::ISOLATE_THEN_SPLIT:
            run_isolate_split_iteration(spec);
            return;
        case FuzzMode::SPLIT_THEN_JOIN:
            run_split_join_iteration(spec);
            return;
        case FuzzMode::SPLIT_THEN_INTEGRATE:
            run_split_integrate_iteration(spec);
            return;
        case FuzzMode::MIXED_PLANNER:
            run_mixed_planner_iteration(spec);
            return;
    }
}

void run_roundtrip_iteration(mt19937& rng) {
    RawEngine RE;
    RawUpdateCtx U;
    GeneratedCase gc = make_random_split_case(RE, U, rng);
    const auto sigs = capture_occ_signatures(RE, gc.occs);

    const SplitSeparationPairResult sp = split_checked(RE, gc.sid, 2, 8, U, "roundtrip_split");
    require_test(sp.child.size() >= 2U, "roundtrip expected at least two split children");

    const SplitLayout layout = collect_split_layout(sp, gc.occs[0]);
    require_test(layout.anchor != NIL_U32, "roundtrip missing anchor split child");

    for (RawSkelID sidAB : layout.boundaryOnly) {
        const IntegrateResult ir = integrate_checked(RE, layout.anchor, sidAB, identity_boundary_map(), U, "roundtrip_integrate");
        if (ir.mergedSid != layout.anchor) {
            fail_test("roundtrip integrate mergedSid mismatch");
        }
    }

    for (RawSkelID sidX : layout.otherOccChildren) {
        const JoinSeparationPairResult jr = join_checked(RE, layout.anchor, sidX, 2, 8, U, "roundtrip_join");
        if (jr.mergedSid != layout.anchor) {
            fail_test("roundtrip join mergedSid mismatch");
        }
    }

    assert_engine_bookkeeping_sane(RE);
    assert_skeleton_wellformed(RE, layout.anchor);
    require_test(RE.skel.get(layout.anchor).hostedOcc.size() == gc.occs.size(), "roundtrip hosted occurrence count mismatch");
    for (OccID occ : gc.occs) {
        require_test(RE.occ.get(occ).hostSkel == layout.anchor, "roundtrip host skeleton mismatch");
        assert_occ_patch_consistent(RE, occ);
        assert_target_prepare_equivalent(RE, layout.anchor, occ, gc.expected.at(occ));
        assert_occ_signature_equivalent(RE, occ, sigs.at(occ));
    }
}

void run_split_integrate_ensure_iteration(mt19937& rng, const TestOptions& options) {
    RawEngine RE;
    RawUpdateCtx U;
    GeneratedCase gc = make_random_split_case(RE, U, rng);
    const auto sigs = capture_occ_signatures(RE, gc.occs);
    const OccID target = gc.occs[0];

    const SplitSeparationPairResult sp = split_checked(RE, gc.sid, 2, 8, U, "split_integrate_ensure_split");
    require_test(sp.child.size() >= 2U, "split_integrate_ensure expected at least two split children");

    const SplitLayout layout = collect_split_layout(sp, target);
    require_test(layout.anchor != NIL_U32, "split_integrate_ensure missing anchor split child");

    for (RawSkelID sidAB : layout.boundaryOnly) {
        const IntegrateResult ir = integrate_checked(RE, layout.anchor, sidAB, identity_boundary_map(), U, "split_integrate_ensure_integrate");
        if (ir.mergedSid != layout.anchor) {
            fail_test("split_integrate_ensure integrate mergedSid mismatch");
        }
    }

    RawPlannerCtx ctx;
    ctx.targetOcc = target;
    ctx.keepOcc = {target};
    run_planner_checked(RE, ctx, U, planner_run_options(options), "split_integrate_ensure_planner");

    assert_engine_bookkeeping_sane(RE);
    assert_planner_stop_condition(RE, target);
    assert_target_prepare_equivalent(RE, RE.occ.get(target).hostSkel, target, gc.expected.at(target));
    assert_occ_signature_equivalent(RE, target, sigs.at(target));
}

void run_planner_iteration(mt19937& rng, int iter, const TestOptions& options) {
    (void)iter;
    const u32 specSeed = uniform_int_distribution<u32>(0U, 0xFFFFFFFFU)(rng);
    const FuzzSpec spec = make_random_spec(FuzzMode::MIXED_PLANNER, specSeed, options);
    PlannerTargetedScenario scenario = make_non_targeted_planner_scenario(spec);
    const PlannerExecutionResult result = run_targeted_planner_iteration(
        scenario,
        planner_run_options(options),
        "planner_iteration"
    );
    record_planner_execution_stats(result, current_fuzz_iteration_ordinal());
}

FuzzMode choose_weighted_fuzz_mode(mt19937& rng, WeightProfile profile) {
    static const array<FuzzMode, 8> modes = {{
        FuzzMode::ISOLATE_ONLY,
        FuzzMode::SPLIT_ONLY,
        FuzzMode::JOIN_ONLY,
        FuzzMode::INTEGRATE_ONLY,
        FuzzMode::ISOLATE_THEN_SPLIT,
        FuzzMode::SPLIT_THEN_JOIN,
        FuzzMode::SPLIT_THEN_INTEGRATE,
        FuzzMode::MIXED_PLANNER,
    }};
    array<int, 8> weights{{1, 1, 1, 1, 1, 1, 1, 1}};
    switch (profile) {
        case WeightProfile::RANDOM:
            break;
        case WeightProfile::WEIGHTED_SPLIT_HEAVY:
            weights = {{1, 4, 1, 1, 3, 2, 2, 3}};
            break;
        case WeightProfile::WEIGHTED_JOIN_HEAVY:
            weights = {{1, 1, 4, 1, 1, 3, 1, 3}};
            break;
        case WeightProfile::WEIGHTED_INTEGRATE_HEAVY:
            weights = {{1, 1, 1, 4, 1, 1, 3, 3}};
            break;
        case WeightProfile::ARTIFACT_HEAVY:
            weights = {{1, 2, 2, 2, 1, 2, 2, 4}};
            break;
        case WeightProfile::MULTIEDGE_HEAVY:
            weights = {{1, 2, 2, 2, 1, 2, 2, 4}};
            break;
    }
    const int totalWeight = accumulate(weights.begin(), weights.end(), 0);
    int pick = uniform_int_distribution<int>(1, totalWeight)(rng);
    for (size_t i = 0; i < modes.size(); ++i) {
        pick -= weights[i];
        if (pick <= 0) {
            return modes[i];
        }
    }
    return modes.back();
}

void run_fuzz_iteration(mt19937& rng, const TestOptions& options) {
    const FuzzMode mode = choose_weighted_fuzz_mode(rng, options.weightProfile);
    const u32 specSeed = uniform_int_distribution<u32>(0U, 0xFFFFFFFFU)(rng);
    run_fuzz_spec(make_random_spec(mode, specSeed, options));
}

template <class Fn>
void run_seeded_case(
    const TestOptions& options,
    const string& caseName,
    const vector<u32>& defaultSeeds,
    int defaultIters,
    Fn&& fn
) {
    const vector<u32> seeds = resolve_seed_list(
        options,
        options.seed.has_value() ? vector<u32>{*options.seed} : defaultSeeds
    );
    const int iters = options.iters.value_or(defaultIters);
    const int iterBase = max(0, options.iterStart);

    for (u32 seed : seeds) {
        begin_fuzz_seed_summary(seed);
        mt19937 rng(seed);
        for (int offset = 0; offset < iters; ++offset) {
            const int iter = iterBase + offset;
            set_failure_context(caseName, seed, iter);
            maybe_log(options, caseName, seed, iter);
            fn(rng, iter);
        }
        end_fuzz_seed_summary();
    }
}

void run_fuzz_mode_case(
    const TestOptions& options,
    const string& caseName,
    FuzzMode mode,
    const vector<u32>& defaultSeeds,
    int defaultIters
) {
    const vector<u32> seeds = resolve_seed_list(
        options,
        options.seed.has_value() ? vector<u32>{*options.seed} : defaultSeeds
    );
    const int iters = options.iters.value_or(defaultIters);
    const int iterBase = max(0, options.iterStart);

    for (u32 seed : seeds) {
        begin_fuzz_seed_summary(seed);
        for (int offset = 0; offset < iters; ++offset) {
            const int iter = iterBase + offset;
            const u32 specSeed = mix_seed(seed, iter);
            set_failure_context(caseName, specSeed, iter);
            maybe_log(options, caseName, specSeed, iter);
            const FuzzSpec spec = make_random_spec(mode, specSeed, options);
            (void)begin_fuzz_iteration_stats();
            if (mode != FuzzMode::MIXED_PLANNER) {
                record_generated_artifact_metadata(spec.directABCount > 0, static_cast<size_t>(max(spec.multiEdgeCount, 0)));
            }
            run_fuzz_spec(spec);
        }
        end_fuzz_seed_summary();
    }
}

void run_targeted_planner_case(
    const TestOptions& options,
    const string& caseName,
    ScenarioFamily family,
    const vector<u32>& defaultSeeds,
    int defaultIters
) {
    if (family == ScenarioFamily::RANDOM) {
        throw runtime_error("random scenario family is not a targeted planner case");
    }

    const vector<u32> seeds = resolve_seed_list(
        options,
        options.seed.has_value() ? vector<u32>{*options.seed} : defaultSeeds
    );
    const int iters = options.iters.value_or(defaultIters);
    const int iterBase = max(0, options.iterStart);

    auto append_split_choice_regression_candidate = [&](const string& stem, const filesystem::path& dumpPath, const optional<filesystem::path>& reducedPath) {
        const filesystem::path outPath = artifact_subdir(options, "logs") / "regression_candidates.txt";
        ofstream ofs(outPath, ios::app);
        if (!ofs) {
            throw runtime_error("failed to append sampled split-choice regression candidate: " + outPath.string());
        }
        ofs << "case=" << stem << " kind=split_choice_audit dump=" << dumpPath.string();
        if (reducedPath.has_value()) {
            ofs << " reduced=" << reducedPath->string();
        }
        ofs << '\n';
    };

    auto append_targeted_planner_exception_candidate = [&](const string& stem, const filesystem::path& dumpPath) {
        const filesystem::path outPath = artifact_subdir(options, "logs") / "regression_candidates.txt";
        ofstream ofs(outPath, ios::app);
        if (!ofs) {
            throw runtime_error("failed to append targeted planner exception candidate: " + outPath.string());
        }
        ofs << "case=" << stem << " kind=planner_exception dump=" << dumpPath.string() << '\n';
    };

    auto dump_targeted_planner_exception = [&](ScenarioFamily activeFamily,
                                              u32 scenarioSeed,
                                              int iter,
                                              const PlannerTargetedScenario& scenario,
                                              const exception& ex) {
        PlannerStateDump dump;
        dump.engine = scenario.RE;
        dump.caseName = caseName + "_planner_exception";
        dump.seed = scenarioSeed;
        dump.iter = iter;
        dump.targetOcc = scenario.ctx.targetOcc;
        dump.keepOcc.assign(scenario.ctx.keepOcc.begin(), scenario.ctx.keepOcc.end());
        sort(dump.keepOcc.begin(), dump.keepOcc.end());
        dump.stepBudget = options.stepBudget;
        dump.tracePrefixLength = 0U;
        dump.traceLevel = TraceLevel::FULL;
        dump.initialQueue = scenario.initialQueue;

        const string stem =
            caseName + "_planner_exception_" + scenario_family_name_string(activeFamily) + "_seed" +
            to_string(scenarioSeed) + "_iter" + to_string(iter);
        const filesystem::path dumpPath = artifact_subdir(options, "counterexamples") / (stem + ".txt");
        save_planner_state_dump(dumpPath, dump);
        set_pending_dump_path(dumpPath);

        const filesystem::path summaryPath = artifact_subdir(options, "logs") / (stem + ".summary.txt");
        ofstream summary(summaryPath);
        if (!summary) {
            throw runtime_error("failed to write targeted planner exception summary: " + summaryPath.string());
        }
        summary << "case=" << caseName << '\n';
        summary << "family=" << scenario_family_name_string(activeFamily) << '\n';
        summary << "seed=" << scenarioSeed << '\n';
        summary << "iter=" << iter << '\n';
        summary << "split_choice_policy_mode=" << split_choice_policy_mode_name_string(options.splitChoicePolicyMode) << '\n';
        summary << "exception=" << ex.what() << '\n';

        append_targeted_planner_exception_candidate(stem, dumpPath);
    };

    auto maybe_run_split_choice_exact_audit = [&](ScenarioFamily activeFamily, u32 scenarioSeed, int iter, const PlannerTargetedScenario& scenario, const IterationOrdinal& ordinal) {
        if (!split_choice_compare_enabled(options) && options.exactCanonicalCap == 0U) {
            return;
        }
        ExhaustiveScenario auditScenario;
        auditScenario.RE = scenario.RE;
        auditScenario.ctx = scenario.ctx;
        auditScenario.U = scenario.U;
        auditScenario.label = scenario_family_name_string(activeFamily) + "_seed" + to_string(scenarioSeed) +
            "_iter" + to_string(iter);
        auditScenario.family = ExhaustiveFamily::MIXED;

        const CompareEligibilityInfo eligibility =
            analyze_compare_eligibility(auditScenario, options.maxSplitPairCandidates);
        record_compare_eligibility_probe(activeFamily, eligibility);

        if (!exact_audit_family_matches(options, activeFamily)) {
            record_exact_audit_skip(ExactAuditSkipReason::FAMILY);
            return;
        }
        const size_t auditBudget = split_choice_compare_budget(options);
        if (auditBudget != 0U &&
            g_fuzzStats.has_value() &&
            (split_choice_compare_enabled(options)
                 ? g_fuzzStats->splitChoiceCompareStateCount
                 : g_fuzzStats->exactAuditedStateCount) >= auditBudget) {
            record_exact_audit_skip(ExactAuditSkipReason::BUDGET);
            return;
        }
        if (!should_sample_split_choice_compare(options, ordinal)) {
            record_exact_audit_skip(ExactAuditSkipReason::SAMPLE);
            return;
        }
        if (!compare_eligibility_counts_as_direct_evidence(activeFamily, eligibility)) {
            record_exact_audit_skip(ExactAuditSkipReason::NON_TIE);
            return;
        }

        TestOptions auditOptions = options;
        auditOptions.caseName = caseName;
        auditOptions.oracleMode = OracleMode::ALL;
        auditOptions.dumpOnFail = true;
        if (split_choice_compare_enabled(auditOptions)) {
            auditOptions.exactCanonicalCap = max<size_t>(auditOptions.exactCanonicalCap, 8U);
        }
        set_active_test_options(&auditOptions);
        const SplitChoiceOracleRunResult auditResult = run_split_choice_oracle(auditOptions, auditScenario, nullptr);
        set_active_test_options(&options);

        if (!auditResult.exactAuditAvailable) {
            record_compare_completion(false);
            record_exact_audit_skip(ExactAuditSkipReason::CAP);
            return;
        }
        record_split_choice_audit_result(auditResult);
        note_multiclass_catalog_observation(auditOptions, caseName, activeFamily, auditScenario, auditResult);

        if (auditResult.exactVsFastClassDisagreementCount == 0U &&
            auditResult.semanticShiftCount == 0U &&
            auditResult.semanticDisagreementCount == 0U) {
            return;
        }

        PlannerStateDump dump;
        dump.engine = auditScenario.RE;
        dump.caseName = caseName + "_split_choice_audit";
        dump.seed = scenarioSeed;
        dump.iter = iter;
        dump.targetOcc = auditScenario.ctx.targetOcc;
        dump.keepOcc.assign(auditScenario.ctx.keepOcc.begin(), auditScenario.ctx.keepOcc.end());
        sort(dump.keepOcc.begin(), dump.keepOcc.end());
        dump.stepBudget = options.stepBudget;
        dump.tracePrefixLength = 0U;
        dump.traceLevel = TraceLevel::FULL;

        const string stem = caseName + "_split_choice_audit_seed" + to_string(scenarioSeed) + "_iter" + to_string(iter);
        const filesystem::path dumpPath = artifact_subdir(options, "counterexamples") / (stem + ".txt");
        save_planner_state_dump(dumpPath, dump);
        set_pending_dump_path(dumpPath);

        const filesystem::path summaryPath = artifact_subdir(options, "logs") / (stem + ".summary.txt");
        ofstream summary(summaryPath);
        if (!summary) {
            throw runtime_error("failed to write sampled split-choice audit summary: " + summaryPath.string());
        }
        summary << describe_split_choice_oracle_result(auditResult);

        optional<filesystem::path> reducedPath;
        try {
            reducedPath = reduce_planner_state_dump_file(options, dumpPath, false);
        } catch (const exception&) {
            reducedPath = nullopt;
        }
        append_split_choice_regression_candidate(stem, dumpPath, reducedPath);
    };

    for (u32 seed : seeds) {
        begin_fuzz_seed_summary(seed);
        for (int offset = 0; offset < iters; ++offset) {
            const int iter = iterBase + offset;
            const u32 scenarioSeed = mix_seed(seed, iter);
            ScenarioFamily activeFamily = family;
            if (family == ScenarioFamily::PLANNER_MIXED_TARGETED) {
                activeFamily = choose_targeted_family(seed, iter);
            } else if (family == ScenarioFamily::PLANNER_MIXED_STRUCTURAL) {
                activeFamily = choose_structural_family(seed, iter);
            }
            set_failure_context(caseName, scenarioSeed, iter);
            maybe_log(options, caseName, scenarioSeed, iter);

            const IterationOrdinal ordinal = begin_fuzz_iteration_stats();
            try {
                PlannerTargetedScenario scenario = make_targeted_planner_scenario(activeFamily, scenarioSeed);
                record_generated_artifact_metadata(scenario.hasDirectAB, 0U);
                maybe_run_split_choice_exact_audit(activeFamily, scenarioSeed, iter, scenario, ordinal);
                const PlannerExecutionResult result = run_targeted_planner_iteration(
                    scenario,
                    planner_run_options(options),
                    "targeted_" + scenario_family_name_string(activeFamily)
                );
                record_planner_execution_stats(result, ordinal);
            } catch (const exception& ex) {
                try {
                    PlannerTargetedScenario dumpScenario = make_targeted_planner_scenario(activeFamily, scenarioSeed);
                    dump_targeted_planner_exception(activeFamily, scenarioSeed, iter, dumpScenario, ex);
                } catch (...) {
                }
                throw;
            }
        }
        end_fuzz_seed_summary();
    }
}

filesystem::path write_temp_repro_spec(const FuzzSpec& spec) {
    const filesystem::path outDir = artifact_subdir(active_test_options(), "logs");
    const filesystem::path outPath = outDir / (current_failure_stem() + "_repro_tmp.txt");
    ofstream ofs(outPath);
    if (!ofs) {
        throw runtime_error("failed to write temp repro file");
    }
    ofs << fuzz_spec_to_string(spec);
    return outPath;
}

bool failing_child_repro(const TestOptions& options, const FuzzSpec& spec) {
    if (options.executablePath.empty()) {
        throw runtime_error("missing executable path for shrinker");
    }

    const filesystem::path reproPath = write_temp_repro_spec(spec);
    ostringstream cmd;
    cmd << shell_quote(options.executablePath)
        << " --case repro --repro-file " << shell_quote(reproPath.string())
        << " --step-budget " << spec.stepBudget
        << " --artifact-dir " << shell_quote(resolve_artifact_dir(options).string());
    return std::system(cmd.str().c_str()) != 0;
}

bool accept_shrink_candidate(const TestOptions& options, FuzzSpec& best, FuzzSpec candidate) {
    candidate = normalize_fuzz_spec(candidate);
    if (same_fuzz_spec(candidate, best)) {
        return false;
    }
    if (!failing_child_repro(options, candidate)) {
        return false;
    }
    best = candidate;
    cerr << "[shrink] " << fuzz_mode_name(best.mode)
         << " seed=" << best.seed
         << " branch_count=" << best.branchCount
         << " occ_count=" << best.occCount
         << " boundary_only_count=" << best.boundaryOnlyCount
         << " max_path_len=" << best.maxPathLen
         << " direct_ab_count=" << best.directABCount
         << " multi_edge_count=" << best.multiEdgeCount
         << " keep_occ_count=" << best.keepOccCount
         << " op_count=" << best.opCount
         << '\n';
    return true;
}

FuzzSpec shrink_failing_spec(const TestOptions& options, FuzzSpec spec) {
    spec = normalize_fuzz_spec(spec);
    bool changed = true;
    while (changed) {
        changed = false;

        while (spec.opCount > 1) {
            FuzzSpec candidate = spec;
            --candidate.opCount;
            if (!accept_shrink_candidate(options, spec, candidate)) {
                break;
            }
            changed = true;
        }

        while (spec.keepOccCount > 1) {
            FuzzSpec candidate = spec;
            --candidate.keepOccCount;
            if (!accept_shrink_candidate(options, spec, candidate)) {
                break;
            }
            changed = true;
        }

        while (spec.sharedOrigPairs > 0) {
            FuzzSpec candidate = spec;
            --candidate.sharedOrigPairs;
            if (!accept_shrink_candidate(options, spec, candidate)) {
                break;
            }
            changed = true;
        }

        while (spec.multiEdgeCount > 0) {
            FuzzSpec candidate = spec;
            --candidate.multiEdgeCount;
            if (!accept_shrink_candidate(options, spec, candidate)) {
                break;
            }
            changed = true;
        }

        while (spec.directABCount > 0) {
            FuzzSpec candidate = spec;
            --candidate.directABCount;
            if (!accept_shrink_candidate(options, spec, candidate)) {
                break;
            }
            changed = true;
        }

        while (spec.maxPathLen > 1) {
            FuzzSpec candidate = spec;
            --candidate.maxPathLen;
            if (!accept_shrink_candidate(options, spec, candidate)) {
                break;
            }
            changed = true;
        }

        while (spec.boundaryOnlyCount > (has_mode_min_boundary(spec.mode) ? 1 : 0)) {
            FuzzSpec candidate = spec;
            --candidate.boundaryOnlyCount;
            if (!accept_shrink_candidate(options, spec, candidate)) {
                break;
            }
            changed = true;
        }

        while (spec.occCount > mode_min_occ(spec.mode)) {
            FuzzSpec candidate = spec;
            --candidate.occCount;
            if (!accept_shrink_candidate(options, spec, candidate)) {
                break;
            }
            changed = true;
        }

        while (spec.branchCount > 2) {
            FuzzSpec candidate = spec;
            --candidate.branchCount;
            if (!accept_shrink_candidate(options, spec, candidate)) {
                break;
            }
            changed = true;
        }

        while (spec.maxOccPerBranch > 1) {
            FuzzSpec candidate = spec;
            --candidate.maxOccPerBranch;
            if (!accept_shrink_candidate(options, spec, candidate)) {
                break;
            }
            changed = true;
        }
    }
    return spec;
}

void run_repro_case(const TestOptions& options) {
    if (!options.reproFile.has_value()) {
        throw runtime_error("--repro-file is required for --case repro");
    }
    const FuzzSpec spec = load_fuzz_spec(*options.reproFile);
    set_failure_context(string("repro_") + fuzz_mode_name(spec.mode), spec.seed, 0);
    maybe_log(options, string("repro_") + fuzz_mode_name(spec.mode), spec.seed, 0);
    run_fuzz_spec(spec);
}

void run_counterexample_search(const TestOptions& options) {
    static const array<FuzzMode, 8> modes = {{
        FuzzMode::ISOLATE_ONLY,
        FuzzMode::SPLIT_ONLY,
        FuzzMode::JOIN_ONLY,
        FuzzMode::INTEGRATE_ONLY,
        FuzzMode::ISOLATE_THEN_SPLIT,
        FuzzMode::SPLIT_THEN_JOIN,
        FuzzMode::SPLIT_THEN_INTEGRATE,
        FuzzMode::MIXED_PLANNER,
    }};
    const vector<u32> seeds = options.seed.has_value()
        ? vector<u32>{*options.seed}
        : vector<u32>{730001U, 730002U, 730003U, 730004U, 730005U};
    const int iters = options.iters.value_or(200);

    for (FuzzMode mode : modes) {
        for (u32 seed : seeds) {
            for (int iter = 0; iter < iters; ++iter) {
                const u32 specSeed = mix_seed(seed, iter);
                const string caseName = string("search_") + fuzz_mode_name(mode);
                set_failure_context(caseName, specSeed, iter);
                maybe_log(options, caseName, specSeed, iter);

                const FuzzSpec spec = make_random_spec(mode, specSeed, options);
                if (!failing_child_repro(options, spec)) {
                    continue;
                }

                cerr << "counterexample_found case=" << fuzz_mode_name(mode)
                     << " seed=" << spec.seed
                     << " iter=" << iter
                     << '\n';
                const FuzzSpec minimized = shrink_failing_spec(options, spec);
                const filesystem::path saved = save_fuzz_spec(
                    minimized,
                    string("reduced_") + fuzz_mode_name(mode) + "_seed" + to_string(minimized.seed)
                );
                cerr << "counterexample_saved " << saved.string() << '\n';
                throw runtime_error("counterexample found; see saved repro");
            }
        }
    }
}

PrimitiveKind resolve_primitive_or_throw(const TestOptions& options, PrimitiveKind fallback) {
    if (!options.primitiveName.has_value()) {
        return fallback;
    }
    const optional<PrimitiveKind> primitive = parse_primitive_kind(*options.primitiveName);
    if (!primitive.has_value()) {
        throw runtime_error("unknown primitive: " + *options.primitiveName);
    }
    return *primitive;
}

bool primitive_replay_oracle_enabled(const TestOptions& options) {
    return primitive_oracle_enabled(options);
}

bool planner_replay_oracle_enabled(const TestOptions& options) {
    return planner_oracle_enabled(options);
}

void maybe_reduce_pending_planner_state(const TestOptions& options) {
    if (!options.dumpOnFail) {
        return;
    }
    const optional<filesystem::path> dumpPath = pending_dump_path();
    if (!dumpPath.has_value()) {
        return;
    }
    try {
        record_reducer_invocation();
        const filesystem::path reduced =
            reduce_planner_state_dump_file(options, *dumpPath, planner_replay_oracle_enabled(options));
        cerr << "reduced_planner_state=" << reduced.string() << '\n';
    } catch (const exception& ex) {
        cerr << "planner_reduce_failed " << ex.what() << '\n';
    }
}

void print_failure_signature(const FailureSignature& failure) {
    write_failure_signature_machine(cout, failure);
}

LiveStateDump make_default_replay_state() {
    LiveStateDump dump = make_reducer_smoke_state();
    dump.invocation.boundaryMap = {
        BoundaryMapEntry{6, 6},
        BoundaryMapEntry{8, 8},
    };
    dump.invocation.keepOcc.clear();
    dump.invocation.throughBranches.clear();
    dump.invocation.sequence.clear();
    return dump;
}

PlannerStateDump make_default_replay_planner_state() {
    PlannerStateDump dump = make_planner_reducer_smoke_state();
    dump.stepBudget = 200000;
    dump.traceLevel = TraceLevel::FULL;
    return dump;
}

void run_replay_state_case(const TestOptions& options) {
    const LiveStateDump dump = options.stateFile.has_value()
        ? load_state_dump(*options.stateFile)
        : make_default_replay_state();
    const PrimitiveKind primitive = resolve_primitive_or_throw(options, dump.invocation.primitive);
    FailureSignature failure;
    const bool ok = replay_state_dump(options, dump, primitive, primitive_replay_oracle_enabled(options), &failure);
    print_failure_signature(failure);
    if (!ok) {
        throw runtime_error(failure.detail.empty() ? "replay_state failed" : failure.detail);
    }
}

void run_reduce_state_case(const TestOptions& options) {
    filesystem::path inputPath;
    if (options.stateFile.has_value()) {
        inputPath = *options.stateFile;
    } else {
        inputPath = artifact_subdir(options, "counterexamples") / "reducer_smoke_input.txt";
        save_state_dump(inputPath, make_reducer_smoke_state());
    }

    const LiveStateDump dump = load_state_dump(inputPath);
    const PrimitiveKind primitive = resolve_primitive_or_throw(options, dump.invocation.primitive);
    const filesystem::path reduced =
        reduce_state_dump_file(options, inputPath, primitive, primitive_replay_oracle_enabled(options));
    if (!filesystem::exists(reduced)) {
        throw runtime_error("reduced state file was not written");
    }
}

void run_reducer_smoke_case(const TestOptions& options) {
    TestOptions reduceOptions = options;
    reduceOptions.oracleMode = OracleMode::PRIMITIVE;
    run_reduce_state_case(reduceOptions);
}

void run_replay_planner_state_case(const TestOptions& options) {
    PlannerStateDump dump = options.stateFile.has_value()
        ? load_planner_state_dump(*options.stateFile)
        : make_default_replay_planner_state();
    if (options.targetOcc.has_value()) {
        dump.targetOcc = *options.targetOcc;
    }
    if (!options.keepOcc.empty()) {
        dump.keepOcc = options.keepOcc;
    }
    if (options.stepBudget != 200000U || dump.stepBudget == 0U) {
        dump.stepBudget = options.stepBudget;
    }
    if (options.dumpTrace) {
        dump.traceLevel = TraceLevel::FULL;
    } else if (trace_enabled(options)) {
        dump.traceLevel = options.traceLevel;
    }

    FailureSignature failure;
    const bool ok = replay_planner_state_dump(options, dump, planner_replay_oracle_enabled(options), &failure);
    print_failure_signature(failure);
    if (!ok) {
        throw runtime_error(failure.detail.empty() ? "replay_planner_state failed" : failure.detail);
    }
}

void run_reduce_planner_state_case(const TestOptions& options) {
    filesystem::path inputPath;
    if (options.stateFile.has_value()) {
        inputPath = *options.stateFile;
    } else {
        inputPath = artifact_subdir(options, "counterexamples") / "planner_reducer_smoke_input.txt";
        save_planner_state_dump(inputPath, make_planner_reducer_smoke_state());
    }

    const filesystem::path reduced =
        reduce_planner_state_dump_file(options, inputPath, planner_replay_oracle_enabled(options));
    if (!filesystem::exists(reduced)) {
        throw runtime_error("reduced planner state file was not written");
    }
}

void run_reduce_planner_state_smoke_case(const TestOptions& options) {
    TestOptions reduceOptions = options;
    reduceOptions.oracleMode = OracleMode::PLANNER;
    run_reduce_planner_state_case(reduceOptions);
}

void run_failure_signature_smoke_case(const TestOptions& options) {
    const FailureSignature primitiveFailure =
        make_failure_signature(FailureClass::PRIMITIVE_ORACLE_MISMATCH, "primitive_mismatch_smoke");
    const FailureSignature primitiveFailureSame =
        make_failure_signature(FailureClass::PRIMITIVE_ORACLE_MISMATCH, "primitive_mismatch_smoke");
    const string primitiveHash = failure_signature_hash(primitiveFailure);
    if (!primitiveFailure.same_failure(primitiveFailureSame)) {
        throw runtime_error("failure_signature_smoke unstable primitive failure signature");
    }
    if (primitiveFailure.failureClass != FailureClass::PRIMITIVE_ORACLE_MISMATCH || primitiveHash.empty()) {
        throw runtime_error("failure_signature_smoke primitive classification mismatch");
    }

    FailureSignature plannerFailureA;
    FailureSignature plannerFailureB;
    const PlannerStateDump plannerDump = make_planner_reducer_smoke_state();
    const bool plannerOkA = replay_planner_state_dump(options, plannerDump, true, &plannerFailureA);
    const bool plannerOkB = replay_planner_state_dump(options, plannerDump, true, &plannerFailureB);
    if (plannerOkA || plannerOkB) {
        throw runtime_error("failure_signature_smoke expected planner replay failure");
    }
    if (!plannerFailureA.same_failure(plannerFailureB)) {
        throw runtime_error("failure_signature_smoke unstable planner failure signature");
    }
}

void run_planner_failure_signature_smoke_case(const TestOptions& options) {
    FailureSignature plannerFailureA;
    FailureSignature plannerFailureB;
    PlannerStateDump plannerDump = make_planner_reducer_smoke_state();
    plannerDump.tracePrefixLength = 5U;

    const bool plannerOkA = replay_planner_state_dump(options, plannerDump, false, &plannerFailureA);
    const bool plannerOkB = replay_planner_state_dump(options, plannerDump, false, &plannerFailureB);
    if (plannerOkA || plannerOkB) {
        throw runtime_error("planner_failure_signature_smoke expected replay failure");
    }
    if (!plannerFailureA.same_failure(plannerFailureB)) {
        throw runtime_error("planner_failure_signature_smoke unstable planner failure signature");
    }
    if (plannerFailureA.failureClass != FailureClass::STEP_BUDGET_EXCEEDED ||
        plannerFailureA.stage != FailureStage::REPLAY ||
        plannerFailureA.mismatchKind != FailureMismatchKind::STEP_BUDGET ||
        plannerFailureA.targetOcc != plannerDump.targetOcc) {
        throw runtime_error("planner_failure_signature_smoke classification mismatch");
    }
}

void run_reduce_planner_state_step_budget_case(const TestOptions& options) {
    const filesystem::path inputPath = artifact_subdir(options, "counterexamples") / "planner_step_budget_input.txt";
    PlannerStateDump dump = make_planner_reducer_smoke_state();
    dump.stepBudget = 64U;
    dump.initialQueue.clear();
    for (int i = 0; i < 96; ++i) {
        UpdJob job;
        job.kind = UpdJobKind::ENSURE_SOLE;
        job.occ = dump.targetOcc;
        dump.initialQueue.push_back(job);
    }
    save_planner_state_dump(inputPath, dump);

    const filesystem::path reduced =
        reduce_planner_state_dump_file(options, inputPath, planner_replay_oracle_enabled(options));
    const PlannerStateDump reducedDump = load_planner_state_dump(reduced);
    if (reducedDump.stepBudget >= dump.stepBudget) {
        throw runtime_error("reduce_planner_state_step_budget did not shrink stepBudget");
    }
}

void run_reduce_planner_state_trace_prefix_case(const TestOptions& options) {
    const filesystem::path inputPath = artifact_subdir(options, "counterexamples") / "planner_trace_prefix_input.txt";
    PlannerStateDump dump = make_planner_reducer_smoke_state();
    dump.tracePrefixLength = 32U;
    save_planner_state_dump(inputPath, dump);

    const filesystem::path reduced =
        reduce_planner_state_dump_file(options, inputPath, planner_replay_oracle_enabled(options));
    const PlannerStateDump reducedDump = load_planner_state_dump(reduced);
    if (reducedDump.tracePrefixLength >= dump.tracePrefixLength) {
        throw runtime_error("reduce_planner_state_trace_prefix did not shrink trace prefix");
    }
}

const FuzzStats& require_fuzz_stats_or_throw(const string& caseName) {
    sync_multiclass_catalog_stats();
    if (!g_fuzzStats.has_value()) {
        throw runtime_error(caseName + " missing fuzz stats");
    }
    return *g_fuzzStats;
}

void require_stats_keys(const filesystem::path& statsPath, initializer_list<const char*> keys, const string& caseName) {
    const string statsText = slurp_text_file(statsPath);
    if (!filesystem::exists(statsPath)) {
        throw runtime_error(caseName + " did not emit stats file");
    }
    for (const char* key : keys) {
        if (statsText.find(key) == string::npos) {
            throw runtime_error(caseName + " missing stats key: " + string(key));
        }
    }
}

void require_coverage_at_least(
    const FuzzStats& stats,
    const string& caseName,
    size_t minSplitReady,
    size_t minBoundaryOnly,
    size_t minJoinCandidate,
    size_t minIntegrateCandidate,
    size_t minActualSplit,
    size_t minActualJoin,
    size_t minActualIntegrate,
    const optional<filesystem::path>& statsPath = nullopt
) {
    if (stats.splitReadyCount < minSplitReady ||
        stats.boundaryOnlyChildCount < minBoundaryOnly ||
        stats.joinCandidateCount < minJoinCandidate ||
        stats.integrateCandidateCount < minIntegrateCandidate ||
        stats.actualSplitHits < minActualSplit ||
        stats.actualJoinHits < minActualJoin ||
        stats.actualIntegrateHits < minActualIntegrate) {
        ostringstream oss;
        oss << caseName
            << " coverage gate failed"
            << " split_ready=" << stats.splitReadyCount
            << " boundary_only=" << stats.boundaryOnlyChildCount
            << " join_candidate=" << stats.joinCandidateCount
            << " integrate_candidate=" << stats.integrateCandidateCount
            << " actual_split=" << stats.actualSplitHits
            << " actual_join=" << stats.actualJoinHits
            << " actual_integrate=" << stats.actualIntegrateHits;
        if (statsPath.has_value()) {
            oss << " stats_file=" << statsPath->string();
        }
        throw runtime_error(oss.str());
    }
}

void require_conversion_at_least(
    const FuzzStats& stats,
    const string& caseName,
    double minSplitConversion,
    double minJoinConversion,
    double minIntegrateConversion,
    const optional<filesystem::path>& statsPath = nullopt
) {
    const double splitConversion = safe_ratio(stats.actualSplitHits, stats.splitReadyCount);
    const double joinConversion = safe_ratio(stats.actualJoinHits, stats.joinCandidateCount);
    const double integrateConversion = safe_ratio(stats.actualIntegrateHits, stats.integrateCandidateCount);
    if (splitConversion < minSplitConversion ||
        joinConversion < minJoinConversion ||
        integrateConversion < minIntegrateConversion) {
        ostringstream oss;
        oss << caseName
            << " conversion gate failed"
            << " split_conversion=" << json_number(splitConversion)
            << " join_conversion=" << json_number(joinConversion)
            << " integrate_conversion=" << json_number(integrateConversion);
        if (statsPath.has_value()) {
            oss << " stats_file=" << statsPath->string();
        }
        throw runtime_error(oss.str());
    }
}

void run_planner_oracle_fuzz_stats_case(const TestOptions& options) {
    TestOptions fuzzOptions = options;
    fuzzOptions.oracleMode = OracleMode::PLANNER;
    fuzzOptions.stats = true;
    if (!fuzzOptions.statsFile.has_value()) {
        fuzzOptions.statsFile = (artifact_subdir(options, "logs") / "planner_oracle_fuzz_stats.json").string();
    }
    begin_fuzz_stats_case(fuzzOptions, "planner_oracle_fuzz_stats");
    set_active_test_options(&fuzzOptions);
    if (fuzzOptions.scenarioFamily == ScenarioFamily::RANDOM) {
        run_fuzz_mode_case(
            fuzzOptions,
            "planner_oracle_fuzz_stats",
            FuzzMode::MIXED_PLANNER,
            {821901U, 821902U},
            fuzzOptions.iters.value_or(24)
        );
    } else {
        run_targeted_planner_case(
            fuzzOptions,
            "planner_oracle_fuzz_stats",
            fuzzOptions.scenarioFamily,
            {821901U, 821902U},
            fuzzOptions.iters.value_or(8)
        );
    }
    flush_fuzz_stats(fuzzOptions, "planner_oracle_fuzz_stats");
    set_active_test_options(&options);

    const filesystem::path statsPath = stats_output_path(fuzzOptions, "planner_oracle_fuzz_stats");
    require_stats_keys(
        statsPath,
        {
            "\"primitive_hits\"",
            "\"sequence_histogram\"",
            "\"trace_prefix_histogram\"",
            "\"primitive_multiset_histogram\"",
            "\"precondition_bias_profile\"",
            "\"precondition_to_actual\"",
            "\"diversity\"",
            "\"coverage_summary\"",
            "\"split_ready_count\"",
            "\"join_candidate_count\"",
            "\"integrate_candidate_count\"",
            "\"actual_split_hits\"",
            "\"actual_join_hits\"",
            "\"actual_integrate_hits\"",
            "\"split_choice_candidate_count\"",
            "\"split_choice_eval_count\"",
            "\"split_choice_tie_count\"",
            "\"split_choice_multiclass_count\"",
            "\"split_choice_fallback_count\"",
            "\"split_choice_equiv_class_count_histogram\"",
            "\"split_choice_policy_mode\"",
            "\"exact_audited_state_count\"",
            "\"exact_audited_pair_count\"",
            "\"fast_vs_exact_class_disagreement_count\"",
            "\"representative_shift_count\"",
            "\"representative_shift_same_class_count\"",
            "\"representative_shift_semantic_divergence_count\"",
            "\"representative_shift_followup_divergence_count\"",
            "\"representative_shift_trace_divergence_count\"",
            "\"harmless_shift_count\"",
            "\"trace_only_shift_count\"",
            "\"semantic_shift_count\"",
            "\"exact_audit_skipped_cap_count\"",
            "\"exact_audit_skipped_budget_count\"",
            "\"exact_audit_skipped_sample_count\"",
            "\"exact_audit_skipped_family_count\"",
            "\"exact_audit_skipped_non_tie_count\"",
            "\"first_split_choice_tie_iter\"",
            "\"first_split_iter\"",
            "\"first_join_iter\"",
            "\"first_integrate_iter\""
        },
        "planner_oracle_fuzz_stats"
    );
    if (!filesystem::exists(stats_summary_output_path(fuzzOptions, "planner_oracle_fuzz_stats"))) {
        throw runtime_error("planner_oracle_fuzz_stats missing summary text file");
    }
}

void run_planner_targeted_split_smoke_case(const TestOptions& options) {
    TestOptions smokeOptions = options;
    smokeOptions.oracleMode = OracleMode::PLANNER;
    smokeOptions.stats = true;
    smokeOptions.scenarioFamily = ScenarioFamily::SPLIT_READY;
    begin_fuzz_stats_case(smokeOptions, "planner_targeted_split_smoke");
    set_active_test_options(&smokeOptions);
    run_targeted_planner_case(smokeOptions, "planner_targeted_split_smoke", smokeOptions.scenarioFamily, {830101U}, 2);
    flush_fuzz_stats(smokeOptions, "planner_targeted_split_smoke");
    set_active_test_options(&options);
    const filesystem::path statsPath = stats_output_path(smokeOptions, "planner_targeted_split_smoke");
    require_coverage_at_least(require_fuzz_stats_or_throw("planner_targeted_split_smoke"), "planner_targeted_split_smoke", 1, 0, 0, 0, 1, 0, 0, statsPath);
    require_conversion_at_least(require_fuzz_stats_or_throw("planner_targeted_split_smoke"), "planner_targeted_split_smoke", 0.50, 0.00, 0.00, statsPath);
}

void run_planner_targeted_join_smoke_case(const TestOptions& options) {
    TestOptions smokeOptions = options;
    smokeOptions.oracleMode = OracleMode::PLANNER;
    smokeOptions.stats = true;
    smokeOptions.scenarioFamily = ScenarioFamily::SPLIT_WITH_KEEPOCC_SIBLING;
    begin_fuzz_stats_case(smokeOptions, "planner_targeted_join_smoke");
    set_active_test_options(&smokeOptions);
    run_targeted_planner_case(smokeOptions, "planner_targeted_join_smoke", smokeOptions.scenarioFamily, {830201U}, 2);
    flush_fuzz_stats(smokeOptions, "planner_targeted_join_smoke");
    set_active_test_options(&options);
    const filesystem::path statsPath = stats_output_path(smokeOptions, "planner_targeted_join_smoke");
    require_coverage_at_least(require_fuzz_stats_or_throw("planner_targeted_join_smoke"), "planner_targeted_join_smoke", 0, 0, 1, 0, 0, 1, 0, statsPath);
    require_conversion_at_least(require_fuzz_stats_or_throw("planner_targeted_join_smoke"), "planner_targeted_join_smoke", 0.00, 0.50, 0.00, statsPath);
}

void run_planner_targeted_integrate_smoke_case(const TestOptions& options) {
    TestOptions smokeOptions = options;
    smokeOptions.oracleMode = OracleMode::PLANNER;
    smokeOptions.stats = true;
    smokeOptions.scenarioFamily = ScenarioFamily::INTEGRATE_READY;
    begin_fuzz_stats_case(smokeOptions, "planner_targeted_integrate_smoke");
    set_active_test_options(&smokeOptions);
    run_targeted_planner_case(smokeOptions, "planner_targeted_integrate_smoke", smokeOptions.scenarioFamily, {840401U}, 2);
    flush_fuzz_stats(smokeOptions, "planner_targeted_integrate_smoke");
    set_active_test_options(&options);
    const filesystem::path statsPath = stats_output_path(smokeOptions, "planner_targeted_integrate_smoke");
    require_coverage_at_least(require_fuzz_stats_or_throw("planner_targeted_integrate_smoke"), "planner_targeted_integrate_smoke", 0, 0, 0, 1, 0, 0, 1, statsPath);
    require_conversion_at_least(require_fuzz_stats_or_throw("planner_targeted_integrate_smoke"), "planner_targeted_integrate_smoke", 0.00, 0.00, 0.50, statsPath);
}

void run_planner_targeted_mixed_smoke_case(const TestOptions& options) {
    TestOptions smokeOptions = options;
    smokeOptions.oracleMode = OracleMode::PLANNER;
    smokeOptions.stats = true;
    smokeOptions.scenarioFamily = ScenarioFamily::PLANNER_MIXED_TARGETED;
    begin_fuzz_stats_case(smokeOptions, "planner_targeted_mixed_smoke");
    set_active_test_options(&smokeOptions);
    run_targeted_planner_case(smokeOptions, "planner_targeted_mixed_smoke", smokeOptions.scenarioFamily, {830401U}, 4);
    flush_fuzz_stats(smokeOptions, "planner_targeted_mixed_smoke");
    set_active_test_options(&options);
    const filesystem::path statsPath = stats_output_path(smokeOptions, "planner_targeted_mixed_smoke");
    require_coverage_at_least(require_fuzz_stats_or_throw("planner_targeted_mixed_smoke"), "planner_targeted_mixed_smoke", 2, 1, 2, 1, 2, 2, 1, statsPath);
    require_conversion_at_least(require_fuzz_stats_or_throw("planner_targeted_mixed_smoke"), "planner_targeted_mixed_smoke", 0.25, 0.25, 0.25, statsPath);
}

void run_split_ready_unique_orig_regression_case(const TestOptions& options) {
    TestOptions oracleOptions = options;
    oracleOptions.caseName = "split_ready_unique_orig_regression";
    oracleOptions.oracleMode = OracleMode::ALL;
    set_active_test_options(&oracleOptions);

    PlannerTargetedScenario scenario = make_split_ready_targeted_scenario(2979151749U, false);
    unordered_set<Vertex> seenRealOrig;
    for (const auto& slot : scenario.RE.V.a) {
        if (!slot.alive || slot.val.kind != RawVertexKind::REAL) {
            continue;
        }
        if (!seenRealOrig.insert(slot.val.orig).second) {
            throw runtime_error("split_ready_unique_orig_regression found duplicate live real orig");
        }
    }

    string validationError;
    if (!validate_engine_state_soft(scenario.RE, &validationError)) {
        throw runtime_error("split_ready_unique_orig_regression invalid initial state: " + validationError);
    }

    (void)run_targeted_planner_iteration(
        scenario,
        planner_run_options(oracleOptions),
        "split_ready_unique_orig_regression"
    );
    set_active_test_options(&options);
}

void run_planner_tie_mixed_smoke_case(const TestOptions& options) {
    TestOptions smokeOptions = options;
    smokeOptions.oracleMode = OracleMode::PLANNER;
    smokeOptions.stats = true;
    smokeOptions.scenarioFamily = ScenarioFamily::PLANNER_TIE_MIXED;
    begin_fuzz_stats_case(smokeOptions, "planner_tie_mixed_smoke");
    set_active_test_options(&smokeOptions);
    run_targeted_planner_case(smokeOptions, "planner_tie_mixed_smoke", smokeOptions.scenarioFamily, {880201U}, 2);
    flush_fuzz_stats(smokeOptions, "planner_tie_mixed_smoke");
    set_active_test_options(&options);
    const filesystem::path statsPath = stats_output_path(smokeOptions, "planner_tie_mixed_smoke");
    require_coverage_at_least(
        require_fuzz_stats_or_throw("planner_tie_mixed_smoke"),
        "planner_tie_mixed_smoke",
        0,
        1,
        1,
        1,
        0,
        1,
        1,
        statsPath
    );
    require_conversion_at_least(
        require_fuzz_stats_or_throw("planner_tie_mixed_smoke"),
        "planner_tie_mixed_smoke",
        0.00,
        0.50,
        0.50,
        statsPath
    );
}

void run_planner_tie_mixed_exact_shadow_smoke_case(const TestOptions& options) {
    TestOptions smokeOptions = options;
    smokeOptions.oracleMode = OracleMode::PLANNER;
    smokeOptions.stats = true;
    smokeOptions.scenarioFamily = ScenarioFamily::PLANNER_TIE_MIXED;
    smokeOptions.splitChoicePolicyMode = SplitChoicePolicyMode::EXACT_SHADOW;
    begin_fuzz_stats_case(smokeOptions, "planner_tie_mixed_exact_shadow_smoke");
    set_active_test_options(&smokeOptions);
    run_targeted_planner_case(
        smokeOptions,
        "planner_tie_mixed_exact_shadow_smoke",
        smokeOptions.scenarioFamily,
        {880201U},
        2
    );
    flush_fuzz_stats(smokeOptions, "planner_tie_mixed_exact_shadow_smoke");
    set_active_test_options(&options);
    const filesystem::path statsPath = stats_output_path(smokeOptions, "planner_tie_mixed_exact_shadow_smoke");
    const FuzzStats& stats = require_fuzz_stats_or_throw("planner_tie_mixed_exact_shadow_smoke");
    require_coverage_at_least(
        stats,
        "planner_tie_mixed_exact_shadow_smoke",
        0,
        1,
        1,
        1,
        0,
        1,
        1,
        statsPath
    );
    require_conversion_at_least(
        stats,
        "planner_tie_mixed_exact_shadow_smoke",
        0.00,
        0.50,
        0.50,
        statsPath
    );
    if (stats.splitChoiceFallbackCount != 0U || stats.semanticShiftCount != 0U) {
        throw runtime_error(
            "planner_tie_mixed_exact_shadow_smoke expected split-choice fallback_count=0 and semantic_shift_count=0"
        );
    }
}

void run_planner_tie_symmetric_smoke_case(const TestOptions& options) {
    TestOptions smokeOptions = options;
    smokeOptions.oracleMode = OracleMode::PLANNER;
    smokeOptions.stats = true;
    smokeOptions.scenarioFamily = ScenarioFamily::PLANNER_TIE_MIXED_SYMMETRIC;
    begin_fuzz_stats_case(smokeOptions, "planner_tie_symmetric_smoke");
    set_active_test_options(&smokeOptions);
    run_targeted_planner_case(smokeOptions, "planner_tie_symmetric_smoke", smokeOptions.scenarioFamily, {890301U}, 2);
    flush_fuzz_stats(smokeOptions, "planner_tie_symmetric_smoke");
    set_active_test_options(&options);
    const filesystem::path statsPath = stats_output_path(smokeOptions, "planner_tie_symmetric_smoke");
    require_coverage_at_least(
        require_fuzz_stats_or_throw("planner_tie_symmetric_smoke"),
        "planner_tie_symmetric_smoke",
        0,
        1,
        1,
        1,
        0,
        1,
        1,
        statsPath
    );
    require_conversion_at_least(
        require_fuzz_stats_or_throw("planner_tie_symmetric_smoke"),
        "planner_tie_symmetric_smoke",
        0.00,
        0.50,
        0.50,
        statsPath
    );
}

void run_planner_coverage_smoke_case(const TestOptions& options) {
    TestOptions smokeOptions = options;
    smokeOptions.oracleMode = OracleMode::PLANNER;
    smokeOptions.stats = true;
    smokeOptions.scenarioFamily = ScenarioFamily::PLANNER_MIXED_TARGETED;
    if (!smokeOptions.statsFile.has_value()) {
        smokeOptions.statsFile = (artifact_subdir(options, "logs") / "planner_coverage_smoke.json").string();
    }
    begin_fuzz_stats_case(smokeOptions, "planner_coverage_smoke");
    set_active_test_options(&smokeOptions);
    run_targeted_planner_case(smokeOptions, "planner_coverage_smoke", smokeOptions.scenarioFamily, {830501U}, 4);
    flush_fuzz_stats(smokeOptions, "planner_coverage_smoke");
    set_active_test_options(&options);

    const FuzzStats& stats = require_fuzz_stats_or_throw("planner_coverage_smoke");
    const filesystem::path statsPath = stats_output_path(smokeOptions, "planner_coverage_smoke");
    require_coverage_at_least(stats, "planner_coverage_smoke", 2, 1, 2, 1, 2, 2, 1, statsPath);
    require_conversion_at_least(stats, "planner_coverage_smoke", 0.25, 0.25, 0.25, statsPath);
    require_stats_keys(
        statsPath,
        {
            "\"scenario_family\"",
            "\"precondition_to_actual\"",
            "\"diversity\"",
            "\"coverage_summary\"",
            "\"split_ready_count\"",
            "\"boundary_only_child_count\"",
            "\"join_candidate_count\"",
            "\"integrate_candidate_count\"",
            "\"actual_split_hits\"",
            "\"actual_join_hits\"",
            "\"actual_integrate_hits\"",
            "\"first_split_iter\"",
            "\"first_join_iter\"",
            "\"first_integrate_iter\""
        },
        "planner_coverage_smoke"
    );
}

void run_planner_random_coverage_smoke_case(const TestOptions& options) {
    TestOptions smokeOptions = options;
    smokeOptions.oracleMode = OracleMode::PLANNER;
    smokeOptions.stats = true;
    smokeOptions.weightProfile = WeightProfile::RANDOM;
    smokeOptions.preconditionBiasProfile = PreconditionBiasProfile::BALANCED;
    smokeOptions.scenarioFamily = ScenarioFamily::RANDOM;
    if (!smokeOptions.statsFile.has_value()) {
        smokeOptions.statsFile = (artifact_subdir(options, "logs") / "planner_random_coverage_smoke.json").string();
    }
    begin_fuzz_stats_case(smokeOptions, "planner_random_coverage_smoke");
    set_active_test_options(&smokeOptions);
    run_fuzz_mode_case(smokeOptions, "planner_random_coverage_smoke", FuzzMode::MIXED_PLANNER, {840101U}, 24);
    flush_fuzz_stats(smokeOptions, "planner_random_coverage_smoke");
    set_active_test_options(&options);

    const filesystem::path statsPath = stats_output_path(smokeOptions, "planner_random_coverage_smoke");
    const FuzzStats& stats = require_fuzz_stats_or_throw("planner_random_coverage_smoke");
    require_coverage_at_least(stats, "planner_random_coverage_smoke", 1, 0, 0, 1, 1, 0, 1, statsPath);
    require_conversion_at_least(stats, "planner_random_coverage_smoke", 0.05, 0.00, 0.05, statsPath);
}

void run_planner_weighted_coverage_smoke_case(const TestOptions& options) {
    const auto run_profile = [&](WeightProfile profile, const string& caseName, size_t minSplit, size_t minJoin, size_t minIntegrate) {
        TestOptions smokeOptions = options;
        smokeOptions.oracleMode = OracleMode::PLANNER;
        smokeOptions.stats = true;
        smokeOptions.weightProfile = profile;
        smokeOptions.preconditionBiasProfile = PreconditionBiasProfile::DEFAULT;
        smokeOptions.scenarioFamily = ScenarioFamily::RANDOM;
        smokeOptions.statsFile = (artifact_subdir(options, "logs") / (caseName + ".json")).string();
        begin_fuzz_stats_case(smokeOptions, caseName);
        set_active_test_options(&smokeOptions);
        run_fuzz_mode_case(smokeOptions, caseName, FuzzMode::MIXED_PLANNER, {840201U}, 16);
        flush_fuzz_stats(smokeOptions, caseName);
        set_active_test_options(&options);

        const filesystem::path statsPath = stats_output_path(smokeOptions, caseName);
        const FuzzStats& stats = require_fuzz_stats_or_throw(caseName);
        require_coverage_at_least(stats, caseName, 0, 0, 0, 0, minSplit, minJoin, minIntegrate, statsPath);
        require_conversion_at_least(
            stats,
            caseName,
            minSplit != 0U ? 0.05 : 0.00,
            minJoin != 0U ? 0.05 : 0.00,
            minIntegrate != 0U ? 0.05 : 0.00,
            statsPath
        );
    };

    run_profile(WeightProfile::WEIGHTED_SPLIT_HEAVY, "planner_weighted_split_coverage_smoke", 1, 0, 0);
    run_profile(WeightProfile::WEIGHTED_JOIN_HEAVY, "planner_weighted_join_coverage_smoke", 0, 1, 0);
    run_profile(WeightProfile::WEIGHTED_INTEGRATE_HEAVY, "planner_weighted_integrate_coverage_smoke", 0, 0, 1);
}

void run_planner_join_ready_smoke_case(const TestOptions& options) {
    TestOptions smokeOptions = options;
    smokeOptions.oracleMode = OracleMode::PLANNER;
    smokeOptions.stats = true;
    smokeOptions.scenarioFamily = ScenarioFamily::JOIN_READY;
    smokeOptions.statsFile = (artifact_subdir(options, "logs") / "planner_join_ready_smoke.json").string();
    begin_fuzz_stats_case(smokeOptions, "planner_join_ready_smoke");
    set_active_test_options(&smokeOptions);
    run_targeted_planner_case(smokeOptions, "planner_join_ready_smoke", smokeOptions.scenarioFamily, {840301U}, 2);
    flush_fuzz_stats(smokeOptions, "planner_join_ready_smoke");
    set_active_test_options(&options);

    const filesystem::path statsPath = stats_output_path(smokeOptions, "planner_join_ready_smoke");
    const FuzzStats& stats = require_fuzz_stats_or_throw("planner_join_ready_smoke");
    require_coverage_at_least(stats, "planner_join_ready_smoke", 0, 0, 1, 0, 0, 1, 0, statsPath);
    require_conversion_at_least(stats, "planner_join_ready_smoke", 0.00, 0.50, 0.00, statsPath);
}

void run_planner_integrate_ready_smoke_case(const TestOptions& options) {
    TestOptions smokeOptions = options;
    smokeOptions.oracleMode = OracleMode::PLANNER;
    smokeOptions.stats = true;
    smokeOptions.scenarioFamily = ScenarioFamily::INTEGRATE_READY;
    smokeOptions.statsFile = (artifact_subdir(options, "logs") / "planner_integrate_ready_smoke.json").string();
    begin_fuzz_stats_case(smokeOptions, "planner_integrate_ready_smoke");
    set_active_test_options(&smokeOptions);
    run_targeted_planner_case(smokeOptions, "planner_integrate_ready_smoke", smokeOptions.scenarioFamily, {840401U}, 2);
    flush_fuzz_stats(smokeOptions, "planner_integrate_ready_smoke");
    set_active_test_options(&options);

    const filesystem::path statsPath = stats_output_path(smokeOptions, "planner_integrate_ready_smoke");
    const FuzzStats& stats = require_fuzz_stats_or_throw("planner_integrate_ready_smoke");
    require_coverage_at_least(stats, "planner_integrate_ready_smoke", 0, 0, 0, 1, 0, 0, 1, statsPath);
    require_conversion_at_least(stats, "planner_integrate_ready_smoke", 0.00, 0.00, 0.50, statsPath);
}

void run_planner_structural_mixed_smoke_case(const TestOptions& options) {
    TestOptions smokeOptions = options;
    smokeOptions.oracleMode = OracleMode::PLANNER;
    smokeOptions.stats = true;
    smokeOptions.scenarioFamily = ScenarioFamily::PLANNER_MIXED_STRUCTURAL;
    smokeOptions.statsFile = (artifact_subdir(options, "logs") / "planner_structural_mixed_smoke.json").string();
    begin_fuzz_stats_case(smokeOptions, "planner_structural_mixed_smoke");
    set_active_test_options(&smokeOptions);
    run_targeted_planner_case(smokeOptions, "planner_structural_mixed_smoke", smokeOptions.scenarioFamily, {840501U}, 6);
    flush_fuzz_stats(smokeOptions, "planner_structural_mixed_smoke");
    set_active_test_options(&options);

    const filesystem::path statsPath = stats_output_path(smokeOptions, "planner_structural_mixed_smoke");
    const FuzzStats& stats = require_fuzz_stats_or_throw("planner_structural_mixed_smoke");
    require_coverage_at_least(stats, "planner_structural_mixed_smoke", 1, 0, 1, 1, 1, 1, 1, statsPath);
    require_conversion_at_least(stats, "planner_structural_mixed_smoke", 0.10, 0.10, 0.10, statsPath);
    if (stats.tracePrefixHistogram.size() < 3U || stats.primitiveMultisetHistogram.size() < 3U) {
        throw runtime_error("planner_structural_mixed_smoke diversity gate failed stats_file=" + statsPath.string());
    }
}

struct IsolateFaultFixture {
    RawEngine RE;
    RawSkelID sid = NIL_U32;
    OccID targetOcc = NIL_U32;
};

IsolateFaultFixture make_isolate_fault_fixture() {
    IsolateFaultFixture fixture;
    RawUpdateCtx U;

    const OccID occ17 = new_occ(fixture.RE, 5);
    const OccID occ31 = new_occ(fixture.RE, 9);
    const OccID occ23 = new_occ(fixture.RE, 5);
    fixture.sid = new_skeleton(fixture.RE);
    fixture.targetOcc = occ17;

    RawSkeletonBuilder B;
    B.V = {
        make_builder_vertex(RawVertexKind::REAL, 2, 0),
        make_builder_vertex(RawVertexKind::REAL, 8, 0),
        make_builder_vertex(RawVertexKind::REAL, 7, 0),
        make_builder_vertex(RawVertexKind::OCC_CENTER, 5, occ17),
        make_builder_vertex(RawVertexKind::REAL, 6, 0),
        make_builder_vertex(RawVertexKind::OCC_CENTER, 9, occ31),
    };
    B.E.push_back(make_builder_edge(3, 0, RawEdgeKind::REAL_PORT));
    B.E.push_back(make_builder_edge(3, 1, RawEdgeKind::BRIDGE_PORT, 41, 0));
    B.E.push_back(make_builder_edge(0, 2, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(2, 1, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(0, 1, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(5, 4, RawEdgeKind::REAL_PORT));
    B.E.push_back(make_builder_edge(4, 1, RawEdgeKind::CORE_REAL));
    B.allocNbr[occ17] = {occ23};
    B.allocNbr[occ31] = {};
    B.corePatchLocalEids[occ17] = {2, 3, 4};
    B.corePatchLocalEids[occ31] = {6};
    commit_skeleton(fixture.RE, fixture.sid, std::move(B), U);
    return fixture;
}

struct SplitFaultFixture {
    RawEngine before;
    RawEngine after;
    PrimitiveInvocation invocation;
    SplitSeparationPairResult result;
};

SplitFaultFixture make_split_fault_fixture() {
    SplitFixture fixture = make_split_fixture();

    SplitFaultFixture out;
    out.before = fixture.RE;
    out.after = fixture.RE;
    out.invocation.primitive = PrimitiveKind::SPLIT;
    out.invocation.sid = fixture.sid;
    out.invocation.occ = fixture.occ31;
    out.invocation.aOrig = 2;
    out.invocation.bOrig = 8;
    out.result = split_separation_pair(out.after, fixture.sid, 2, 8, fixture.U);
    return out;
}

RawSkelID choose_faulty_child_sid(const SplitSeparationPairResult& result, const RawEngine& RE) {
    for (const SplitChildInfo& child : result.child) {
        if (!RE.skel.get(child.sid).edges.empty()) {
            return child.sid;
        }
    }
    return result.child.empty() ? NIL_U32 : result.child.front().sid;
}

FaultDetectionRow detect_prepare_fault(PrimitiveFaultKind fault) {
    FaultDetectionRow row;
    row.faultName = primitive_fault_name_string(fault);

    IsolateFaultFixture fixture = make_isolate_fault_fixture();
    if (fault == PrimitiveFaultKind::DROP_ALLOC_NBR ||
        fault == PrimitiveFaultKind::DUPLICATE_ALLOC_NBR ||
        fault == PrimitiveFaultKind::DROP_CORE_PATCH_EDGE) {
        IsolatePrepared mutated = prepare_isolate_neighborhood(fixture.RE, fixture.sid, fixture.targetOcc);
        if (fault == PrimitiveFaultKind::DROP_ALLOC_NBR && !mutated.allocNbr.empty()) {
            mutated.allocNbr.erase(mutated.allocNbr.begin());
        } else if (fault == PrimitiveFaultKind::DUPLICATE_ALLOC_NBR && !mutated.allocNbr.empty()) {
            mutated.allocNbr.push_back(mutated.allocNbr.front());
        } else if (fault == PrimitiveFaultKind::DROP_CORE_PATCH_EDGE && !mutated.core.edges.empty()) {
            mutated.core.edges.pop_back();
        }
        string failure;
        row.primitiveOracleDetected = !check_prepare_isolate_oracle(
            fixture.RE,
            fixture.sid,
            fixture.targetOcc,
            mutated,
            &failure
        );
        row.detail = failure;
    }

    RawEngine validatorState = fixture.RE;
    inject_primitive_fault(validatorState, fault, fixture.targetOcc);
    string validationError;
    row.validatorDetected = !validate_engine_state_soft(validatorState, &validationError);
    if (row.detail.empty()) {
        row.detail = validationError;
    }
    return row;
}

FaultDetectionRow detect_split_fault(PrimitiveFaultKind fault) {
    FaultDetectionRow row;
    row.faultName = primitive_fault_name_string(fault);

    SplitFaultFixture fixture = make_split_fault_fixture();
    if (fault == PrimitiveFaultKind::FLIP_BOUNDARY_ONLY) {
        if (!fixture.result.child.empty()) {
            fixture.result.child.front().boundaryOnly = !fixture.result.child.front().boundaryOnly;
        }
        string failure;
        row.primitiveOracleDetected = !check_split_oracle(
            fixture.before,
            fixture.after,
            fixture.invocation,
            fixture.result,
            &failure
        );
        row.detail = failure;
        return row;
    }

    const RawSkelID sid = choose_faulty_child_sid(fixture.result, fixture.after);
    if (sid != NIL_U32 && !fixture.after.skel.get(sid).hostedOcc.empty()) {
        inject_primitive_fault(fixture.after, fault, fixture.after.skel.get(sid).hostedOcc.front());
    }

    string validationError;
    row.validatorDetected = !validate_engine_state_soft(fixture.after, &validationError);
    string failure;
    row.primitiveOracleDetected = !check_split_oracle(
        fixture.before,
        fixture.after,
        fixture.invocation,
        fixture.result,
        &failure
    );
    row.detail = !failure.empty() ? failure : validationError;
    return row;
}

FaultDetectionRow detect_planner_fault(PlannerFaultKind fault, ScenarioFamily family, u32 seed) {
    FaultDetectionRow row;
    row.faultName = planner_fault_name_string(fault);
    const RawUpdaterRunOptions runOptions{200000U};

    try {
        const auto detect_planner_signature_mismatch = [&](
            const RawEngine& before,
            const PlannerTargetedScenario& scenario,
            const PlannerExecutionResult& result,
            const vector<UpdJob>* expectedQueue
        ) {
            const PlannerRunResult reference = run_reference_planner(
                before,
                scenario.ctx,
                runOptions,
                true,
                expectedQueue
            );
            const PlannerResultSignature actual =
                capture_planner_signature(scenario.RE, scenario.ctx.targetOcc, result.trace);
            if (!(reference.signature == actual)) {
                row.plannerOracleDetected = true;
                return string("planner signature mismatch\nexpected:\n") +
                    describe_planner_signature(reference.signature) +
                    "\nactual:\n" +
                    describe_planner_signature(actual);
            }

            string oracleFailure;
            if (!check_planner_oracle(before, scenario.RE, scenario.ctx, runOptions, result.trace, expectedQueue, &oracleFailure)) {
                row.plannerOracleDetected = true;
                return oracleFailure;
            }
            row.plannerOracleDetected = false;
            return string{};
        };

        const auto finalize_detection = [&](const string& oracleFailure, const string& validationError) {
            row.detail = !oracleFailure.empty()
                ? oracleFailure
                : (validationError.empty() ? string("undetected") : validationError);
        };

        auto run_fault_injected = [&](PlannerTargetedScenario scenario, bool useInitialQueue) {
            const RawEngine before = scenario.RE;
            const vector<UpdJob>* initialQueue =
                useInitialQueue && !scenario.initialQueue.empty() ? &scenario.initialQueue : nullptr;
            const PlannerExecutionResult result = run_planner_checked_capture_fault_injected(
                scenario.RE,
                scenario.ctx,
                scenario.U,
                runOptions,
                fault,
                initialQueue,
                row.faultName
            );
            const string failure = detect_planner_signature_mismatch(before, scenario, result, initialQueue);
            string validationError;
            row.validatorDetected = !validate_engine_state_soft(scenario.RE, &validationError);
            finalize_detection(failure, validationError);
        };

        auto run_with_actual_queue = [&](
            PlannerTargetedScenario scenario,
            vector<UpdJob> actualQueue,
            const vector<UpdJob>& expectedQueue
        ) {
            const RawEngine before = scenario.RE;
            const vector<UpdJob>* actualQueuePtr = actualQueue.empty() ? nullptr : &actualQueue;
            const vector<UpdJob>* expectedQueuePtr = expectedQueue.empty() ? nullptr : &expectedQueue;
            const PlannerExecutionResult result = run_planner_checked_capture(
                scenario.RE,
                scenario.ctx,
                scenario.U,
                runOptions,
                actualQueuePtr,
                row.faultName
            );
            const string failure =
                detect_planner_signature_mismatch(before, scenario, result, expectedQueuePtr);
            string validationError;
            row.validatorDetected = !validate_engine_state_soft(scenario.RE, &validationError);
            finalize_detection(failure, validationError);
        };

        if (fault == PlannerFaultKind::OMIT_AFTER_SPLIT_JOIN) {
            PlannerTargetedScenario scenario = make_post_split_targeted_scenario(seed, false);
            const vector<UpdJob> expectedQueue = scenario.initialQueue;
            vector<UpdJob> actualQueue = expectedQueue;
            actualQueue.erase(
                remove_if(actualQueue.begin(), actualQueue.end(), [](const UpdJob& job) {
                    return job.kind == UpdJobKind::JOIN_PAIR;
                }),
                actualQueue.end()
            );
            run_with_actual_queue(std::move(scenario), std::move(actualQueue), expectedQueue);
        } else if (fault == PlannerFaultKind::OMIT_AFTER_SPLIT_INTEGRATE) {
            PlannerTargetedScenario scenario = make_post_split_targeted_scenario(seed, true);
            const vector<UpdJob> expectedQueue = scenario.initialQueue;
            vector<UpdJob> actualQueue = expectedQueue;
            actualQueue.erase(
                remove_if(actualQueue.begin(), actualQueue.end(), [](const UpdJob& job) {
                    return job.kind == UpdJobKind::INTEGRATE_CHILD;
                }),
                actualQueue.end()
            );
            run_with_actual_queue(std::move(scenario), std::move(actualQueue), expectedQueue);
        } else if (fault == PlannerFaultKind::OMIT_ENSURE_SOLE_AFTER_INTEGRATE) {
            for (u32 offset = 0; offset < 64U; ++offset) {
                PlannerTargetedScenario scenario = make_integrate_keepocc_ready_targeted_scenario(seed + offset);
                const vector<UpdJob>* initialQueue = scenario.initialQueue.empty() ? nullptr : &scenario.initialQueue;
                const RawEngine before = scenario.RE;
                const PlannerExecutionResult result = run_planner_checked_capture_fault_injected(
                    scenario.RE,
                    scenario.ctx,
                    scenario.U,
                    runOptions,
                    fault,
                    initialQueue,
                    row.faultName
                );
                const string failure =
                    detect_planner_signature_mismatch(before, scenario, result, initialQueue);
                string validationError;
                row.validatorDetected = !validate_engine_state_soft(scenario.RE, &validationError);
                finalize_detection(failure, validationError);
                if (row.validatorDetected || row.plannerOracleDetected) {
                    return row;
                }
            }
            row.detail = "undetected";
        } else if (fault == PlannerFaultKind::WRONG_BRANCH_ROUTING) {
            PlannerTargetedScenario scenario = make_post_split_targeted_scenario(seed, true);
            const vector<UpdJob> expectedQueue = scenario.initialQueue;
            vector<UpdJob> actualQueue = expectedQueue;
            optional<RawSkelID> originalAnchorSid;
            optional<RawSkelID> keepOccSid;
            for (const UpdJob& job : expectedQueue) {
                if (job.kind == UpdJobKind::JOIN_PAIR) {
                    originalAnchorSid = job.leftSid;
                    keepOccSid = job.rightSid;
                    break;
                }
            }
            if (!originalAnchorSid.has_value() || !keepOccSid.has_value()) {
                throw runtime_error("wrong_branch_routing requires JOIN_PAIR");
            }
            for (UpdJob& job : actualQueue) {
                if (job.kind == UpdJobKind::JOIN_PAIR) {
                    job.leftSid = *keepOccSid;
                    job.rightSid = *originalAnchorSid;
                } else if (job.kind == UpdJobKind::INTEGRATE_CHILD) {
                    job.parentSid = *keepOccSid;
                }
            }
            run_with_actual_queue(std::move(scenario), std::move(actualQueue), expectedQueue);
        } else {
            PlannerTargetedScenario scenario = make_targeted_planner_scenario(family, seed);
            run_fault_injected(std::move(scenario), true);
        }
    } catch (const exception& ex) {
        row.detail = ex.what();
    }
    return row;
}

vector<FaultDetectionRow> collect_fault_detection_rows() {
    vector<FaultDetectionRow> rows;
    rows.push_back(detect_prepare_fault(PrimitiveFaultKind::DROP_ALLOC_NBR));
    rows.push_back(detect_prepare_fault(PrimitiveFaultKind::DUPLICATE_ALLOC_NBR));
    rows.push_back(detect_prepare_fault(PrimitiveFaultKind::WRONG_HOST_SKEL));
    rows.push_back(detect_prepare_fault(PrimitiveFaultKind::WRONG_CENTER_V));
    rows.push_back(detect_prepare_fault(PrimitiveFaultKind::DROP_CORE_PATCH_EDGE));
    rows.push_back(detect_prepare_fault(PrimitiveFaultKind::ADD_CENTER_INCIDENT_CORE_EDGE));
    rows.push_back(detect_split_fault(PrimitiveFaultKind::DROP_SKELETON_EDGE));
    rows.push_back(detect_split_fault(PrimitiveFaultKind::REWIRE_SKELETON_EDGE));
    rows.push_back(detect_split_fault(PrimitiveFaultKind::FLIP_BOUNDARY_ONLY));
    rows.push_back(detect_planner_fault(PlannerFaultKind::OMIT_AFTER_SPLIT_JOIN, ScenarioFamily::SPLIT_WITH_KEEPOCC_SIBLING, 930101U));
    rows.push_back(detect_planner_fault(PlannerFaultKind::OMIT_AFTER_SPLIT_INTEGRATE, ScenarioFamily::SPLIT_WITH_BOUNDARY_ARTIFACT, 930201U));
    rows.push_back(detect_planner_fault(PlannerFaultKind::OMIT_ENSURE_SOLE_AFTER_JOIN, ScenarioFamily::JOIN_READY, 930301U));
    rows.push_back(detect_planner_fault(PlannerFaultKind::OMIT_ENSURE_SOLE_AFTER_INTEGRATE, ScenarioFamily::INTEGRATE_READY, 930401U));
    rows.push_back(detect_planner_fault(PlannerFaultKind::WRONG_BRANCH_ROUTING, ScenarioFamily::SPLIT_WITH_JOIN_AND_INTEGRATE, 930501U));
    return rows;
}

void write_fault_detection_matrix(const TestOptions& options, const string& caseName, const vector<FaultDetectionRow>& rows) {
    const filesystem::path outPath = artifact_subdir(options, "logs") / (caseName + ".matrix.txt");
    ofstream ofs(outPath);
    if (!ofs) {
        throw runtime_error("failed to write fault matrix: " + outPath.string());
    }
    ofs << "fault validator primitive_oracle planner_oracle detail\n";
    for (const FaultDetectionRow& row : rows) {
        ofs << row.faultName << ' '
            << (row.validatorDetected ? 1 : 0) << ' '
            << (row.primitiveOracleDetected ? 1 : 0) << ' '
            << (row.plannerOracleDetected ? 1 : 0) << ' '
            << row.detail << '\n';
    }
}

void require_fault_detected(const FaultDetectionRow& row, const string& caseName) {
    if (!row.validatorDetected && !row.primitiveOracleDetected && !row.plannerOracleDetected) {
        throw runtime_error(caseName + " undetected fault: " + row.faultName);
    }
}

PlannerStateDump planner_dump_from_scenario(
    const PlannerTargetedScenario& scenario,
    const TestOptions& options,
    const string& caseName
) {
    PlannerStateDump dump;
    dump.engine = scenario.RE;
    dump.caseName = caseName;
    dump.targetOcc = scenario.ctx.targetOcc;
    dump.keepOcc.assign(scenario.ctx.keepOcc.begin(), scenario.ctx.keepOcc.end());
    sort(dump.keepOcc.begin(), dump.keepOcc.end());
    dump.stepBudget = options.stepBudget;
    dump.tracePrefixLength = 0U;
    dump.initialQueue = scenario.initialQueue;
    dump.traceLevel = TraceLevel::FULL;
    return dump;
}

void run_planner_fixpoint_idempotence_case(const TestOptions& options) {
    for (const pair<ScenarioFamily, u32>& entry :
         vector<pair<ScenarioFamily, u32>>{{ScenarioFamily::SPLIT_WITH_JOIN_AND_INTEGRATE, 940101U},
                                           {ScenarioFamily::JOIN_READY, 940201U},
                                           {ScenarioFamily::INTEGRATE_READY, 940301U}}) {
        PlannerTargetedScenario scenario = make_targeted_planner_scenario(entry.first, entry.second);
        const RawUpdaterRunOptions runOptions{options.stepBudget};
        (void)run_targeted_planner_iteration(scenario, runOptions, "fixpoint_first");
        const EngineStateSignature afterFirst = capture_engine_state_signature(scenario.RE);
        const RawEngine beforeSecond = scenario.RE;
        const PlannerExecutionResult second = run_planner_checked_capture(
            scenario.RE,
            scenario.ctx,
            scenario.U,
            runOptions,
            nullptr,
            "fixpoint_second"
        );
        const EngineStateSignature afterSecond = capture_engine_state_signature(scenario.RE);
        if (!(afterFirst == afterSecond)) {
            throw runtime_error("planner_fixpoint_idempotence final state changed on second run");
        }
        string oracleFailure;
        if (!check_planner_oracle(beforeSecond, scenario.RE, scenario.ctx, runOptions, second.trace, nullptr, &oracleFailure)) {
            throw runtime_error("planner_fixpoint_idempotence oracle mismatch\n" + oracleFailure);
        }
    }
}

struct PlannerReplaySnapshot {
    bool ok = false;
    EngineStateSignature finalState;
    vector<PlannerTraceEntry> trace;
    FailureSignature failure;
    bool oracleOk = false;
};

PlannerReplaySnapshot replay_planner_dump_snapshot(const PlannerStateDump& dump) {
    PlannerReplaySnapshot snapshot;
    try {
        RawEngine RE = dump.engine;
        RawUpdateCtx U;
        RawPlannerCtx ctx;
        ctx.targetOcc = dump.targetOcc;
        ctx.keepOcc.insert(dump.keepOcc.begin(), dump.keepOcc.end());
        const RawUpdaterRunOptions runOptions{dump.stepBudget};
        execute_planner_capture(
            RE,
            ctx,
            U,
            runOptions,
            dump.initialQueue.empty() ? nullptr : &dump.initialQueue,
            snapshot.trace,
            nullptr
        );
        snapshot.finalState = capture_engine_state_signature(RE);
        snapshot.ok = true;
        snapshot.oracleOk = check_planner_oracle(
            dump.engine,
            RE,
            ctx,
            runOptions,
            snapshot.trace,
            dump.initialQueue.empty() ? nullptr : &dump.initialQueue,
            nullptr
        );
    } catch (const exception& ex) {
        snapshot.failure = make_failure_signature(FailureClass::CRASH, ex.what());
    }
    return snapshot;
}

void run_planner_replay_determinism_case(const TestOptions& options) {
    {
        PlannerTargetedScenario scenario = make_targeted_planner_scenario(ScenarioFamily::SPLIT_WITH_JOIN_AND_INTEGRATE, 941001U);
        const filesystem::path dumpPath = artifact_subdir(options, "logs") / "planner_replay_determinism_input.txt";
        save_planner_state_dump(dumpPath, planner_dump_from_scenario(scenario, options, "planner_replay_determinism"));
        const PlannerStateDump dumpA = load_planner_state_dump(dumpPath);
        const PlannerStateDump dumpB = load_planner_state_dump(dumpPath);
        const PlannerReplaySnapshot first = replay_planner_dump_snapshot(dumpA);
        const PlannerReplaySnapshot second = replay_planner_dump_snapshot(dumpB);
        if (!first.ok || !second.ok) {
            throw runtime_error("planner_replay_determinism passing replay unexpectedly failed");
        }
        if (!(first.finalState == second.finalState) || first.trace != second.trace || first.oracleOk != second.oracleOk) {
            throw runtime_error("planner_replay_determinism passing replay diverged");
        }
    }

    {
        const filesystem::path dumpPath = artifact_subdir(options, "counterexamples") / "planner_replay_determinism_fail_input.txt";
        save_planner_state_dump(dumpPath, make_planner_reducer_smoke_state());
        const PlannerStateDump dumpA = load_planner_state_dump(dumpPath);
        const PlannerStateDump dumpB = load_planner_state_dump(dumpPath);
        FailureSignature failureA;
        FailureSignature failureB;
        const bool okA = replay_planner_state_dump(options, dumpA, false, &failureA);
        const bool okB = replay_planner_state_dump(options, dumpB, false, &failureB);
        if (okA || okB) {
            throw runtime_error("planner_replay_determinism failing replay unexpectedly passed");
        }
        if (!failureA.same_failure(failureB) || failure_signature_hash(failureA) != failure_signature_hash(failureB)) {
            throw runtime_error("planner_replay_determinism failing replay produced unstable signature");
        }
    }
}

void run_reducer_determinism_smoke_case(const TestOptions& options) {
    const filesystem::path inputPath = artifact_subdir(options, "counterexamples") / "reducer_determinism_input.txt";
    save_planner_state_dump(inputPath, make_planner_reducer_smoke_state());
    const filesystem::path reducedA = reduce_planner_state_dump_file(options, inputPath, false);
    const PlannerStateDump dumpA = load_planner_state_dump(reducedA);
    const filesystem::path reducedB = reduce_planner_state_dump_file(options, inputPath, false);
    const PlannerStateDump dumpB = load_planner_state_dump(reducedB);

    const PlannerReplaySnapshot replayA = replay_planner_dump_snapshot(dumpA);
    const PlannerReplaySnapshot replayB = replay_planner_dump_snapshot(dumpB);
    if (replayA.ok != replayB.ok) {
        throw runtime_error("reducer_determinism_smoke replay outcome diverged");
    }
    if (!replayA.ok && !replayA.failure.same_failure(replayB.failure)) {
        throw runtime_error("reducer_determinism_smoke failure signatures diverged");
    }
    if (replayA.ok &&
        (!(replayA.finalState == replayB.finalState) || replayA.oracleOk != replayB.oracleOk)) {
        throw runtime_error("reducer_determinism_smoke replay final state diverged");
    }
}

void run_primitive_fault_detection_smoke_case(const TestOptions& options) {
    const vector<FaultDetectionRow> rows = {
        detect_prepare_fault(PrimitiveFaultKind::DROP_ALLOC_NBR),
        detect_prepare_fault(PrimitiveFaultKind::DUPLICATE_ALLOC_NBR),
        detect_prepare_fault(PrimitiveFaultKind::WRONG_HOST_SKEL),
        detect_prepare_fault(PrimitiveFaultKind::WRONG_CENTER_V),
        detect_prepare_fault(PrimitiveFaultKind::DROP_CORE_PATCH_EDGE),
        detect_prepare_fault(PrimitiveFaultKind::ADD_CENTER_INCIDENT_CORE_EDGE),
        detect_split_fault(PrimitiveFaultKind::DROP_SKELETON_EDGE),
        detect_split_fault(PrimitiveFaultKind::REWIRE_SKELETON_EDGE),
        detect_split_fault(PrimitiveFaultKind::FLIP_BOUNDARY_ONLY),
    };
    write_fault_detection_matrix(options, "primitive_fault_detection_smoke", rows);
    for (const FaultDetectionRow& row : rows) {
        require_fault_detected(row, "primitive_fault_detection_smoke");
    }
}

void run_planner_fault_detection_smoke_case(const TestOptions& options) {
    const vector<FaultDetectionRow> rows = {
        detect_planner_fault(PlannerFaultKind::OMIT_AFTER_SPLIT_JOIN, ScenarioFamily::SPLIT_WITH_KEEPOCC_SIBLING, 942101U),
        detect_planner_fault(PlannerFaultKind::OMIT_AFTER_SPLIT_INTEGRATE, ScenarioFamily::SPLIT_WITH_BOUNDARY_ARTIFACT, 942201U),
        detect_planner_fault(PlannerFaultKind::OMIT_ENSURE_SOLE_AFTER_JOIN, ScenarioFamily::JOIN_READY, 942301U),
        detect_planner_fault(PlannerFaultKind::OMIT_ENSURE_SOLE_AFTER_INTEGRATE, ScenarioFamily::INTEGRATE_READY, 942401U),
        detect_planner_fault(PlannerFaultKind::WRONG_BRANCH_ROUTING, ScenarioFamily::SPLIT_WITH_JOIN_AND_INTEGRATE, 942501U),
    };
    write_fault_detection_matrix(options, "planner_fault_detection_smoke", rows);
    for (const FaultDetectionRow& row : rows) {
        require_fault_detected(row, "planner_fault_detection_smoke");
    }
}

void run_mutation_matrix_smoke_case(const TestOptions& options) {
    const vector<FaultDetectionRow> rows = collect_fault_detection_rows();
    write_fault_detection_matrix(options, "mutation_matrix_smoke", rows);
    for (const FaultDetectionRow& row : rows) {
        require_fault_detected(row, "mutation_matrix_smoke");
    }
}

void apply_mode_text_to_options(TestOptions& options, const string& modeText) {
    try {
        apply_mode_to_options(options, modeText);
        return;
    } catch (const exception&) {
    }

    const auto parse_bias_suffix = [&](const string& text, string& base) -> bool {
        const size_t sPos = text.rfind("_s");
        const size_t jPos = text.rfind("_j");
        const size_t iPos = text.rfind("_i");
        if (sPos == string::npos || jPos == string::npos || iPos == string::npos ||
            !(sPos < jPos && jPos < iPos)) {
            base = text;
            return false;
        }
        base = text.substr(0, sPos);
        options.biasSplit = stoi(text.substr(sPos + 2U, jPos - (sPos + 2U)));
        options.biasJoin = stoi(text.substr(jPos + 2U, iPos - (jPos + 2U)));
        options.biasIntegrate = stoi(text.substr(iPos + 2U));
        return true;
    };

    string base;
    const bool hasBiasSuffix = parse_bias_suffix(modeText, base);
    if (base.rfind("family_", 0) == 0) {
        const optional<ScenarioFamily> family = parse_scenario_family_token(base.substr(7));
        if (!family.has_value()) {
            throw runtime_error("unknown campaign/corpus mode: " + modeText);
        }
        options.scenarioFamily = *family;
        if (!hasBiasSuffix) {
            options.biasSplit = -1;
            options.biasJoin = -1;
            options.biasIntegrate = -1;
        }
        return;
    }

    if (base.rfind("weight_", 0) == 0) {
        string weightAndBias = base.substr(7);
        const size_t biasPos = weightAndBias.find("_bias_");
        if (biasPos == string::npos) {
            throw runtime_error("unknown campaign/corpus mode: " + modeText);
        }
        const optional<WeightProfile> profile = parse_weight_profile_token(weightAndBias.substr(0, biasPos));
        const optional<PreconditionBiasProfile> biasProfile =
            parse_precondition_bias_profile_token(weightAndBias.substr(biasPos + 6U));
        if (!profile.has_value() || !biasProfile.has_value()) {
            throw runtime_error("unknown campaign/corpus mode: " + modeText);
        }
        options.weightProfile = *profile;
        options.preconditionBiasProfile = *biasProfile;
        options.scenarioFamily = ScenarioFamily::RANDOM;
        if (!hasBiasSuffix) {
            options.biasSplit = -1;
            options.biasJoin = -1;
            options.biasIntegrate = -1;
        }
        return;
    }

    throw runtime_error("unknown campaign/corpus mode: " + modeText);
}

filesystem::path resolve_corpus_dir_for_read(const TestOptions& options) {
    if (!options.loadCorpusDir.has_value()) {
        throw runtime_error("corpus replay requires --load-corpus");
    }
    const filesystem::path dir = filesystem::absolute(*options.loadCorpusDir);
    if (!filesystem::exists(dir) || !filesystem::is_directory(dir)) {
        throw runtime_error("corpus directory does not exist: " + dir.string());
    }
    return dir;
}

FuzzStats run_planner_oracle_fuzz_entry(
    const TestOptions& options,
    const string& caseName,
    const vector<u32>& defaultSeeds,
    int defaultIters
) {
    TestOptions oracleOptions = options;
    oracleOptions.oracleMode = OracleMode::PLANNER;
    oracleOptions.stats = true;
    begin_fuzz_stats_case(oracleOptions, caseName);
    set_active_test_options(&oracleOptions);
    try {
        if (oracleOptions.scenarioFamily == ScenarioFamily::RANDOM) {
            run_fuzz_mode_case(oracleOptions, caseName, FuzzMode::MIXED_PLANNER, defaultSeeds, defaultIters);
        } else {
            run_targeted_planner_case(oracleOptions, caseName, oracleOptions.scenarioFamily, defaultSeeds, defaultIters);
        }
    } catch (...) {
        record_failure_stat(caseName + "_exception");
        maybe_reduce_pending_planner_state(oracleOptions);
        flush_fuzz_stats(oracleOptions, caseName);
        set_active_test_options(&options);
        throw;
    }
    FuzzStats stats;
    if (g_fuzzStats.has_value()) {
        stats = *g_fuzzStats;
    } else {
        stats.caseName = caseName;
    }
    flush_fuzz_stats(oracleOptions, caseName);
    set_active_test_options(&options);
    return stats;
}

filesystem::path ensure_campaign_config_snapshot(
    const filesystem::path& checkpointDir,
    const filesystem::path& configPath
) {
    filesystem::create_directories(checkpointDir);
    const filesystem::path snapshotPath =
        checkpointDir / ("campaign_config_snapshot" + configPath.extension().string());
    if (!filesystem::exists(snapshotPath) || slurp_text_file(snapshotPath) != slurp_text_file(configPath)) {
        filesystem::copy_file(
            configPath,
            snapshotPath,
            filesystem::copy_options::overwrite_existing
        );
    }
    return filesystem::absolute(snapshotPath);
}

TestOptions make_campaign_run_options(
    const TestOptions& options,
    const CampaignConfig& config,
    const CampaignRunConfig& entry
) {
    TestOptions runOptions = options;
    runOptions.caseName = entry.caseName;
    runOptions.artifactDir = filesystem::absolute(entry.artifactDir).string();
    runOptions.dumpOnFail = true;
    runOptions.stats = true;
    runOptions.saveCorpusDir = options.saveCorpusDir.has_value()
        ? optional<string>(filesystem::absolute(*options.saveCorpusDir).string())
        : (config.saveCorpusDir.has_value() ? optional<string>(filesystem::absolute(*config.saveCorpusDir).string()) : nullopt);
    runOptions.loadCorpusDir = options.loadCorpusDir.has_value()
        ? optional<string>(filesystem::absolute(*options.loadCorpusDir).string())
        : (config.loadCorpusDir.has_value() ? optional<string>(filesystem::absolute(*config.loadCorpusDir).string()) : nullopt);
    runOptions.corpusPolicy = options.corpusPolicy == CorpusPolicy::BEST ? config.corpusPolicy : options.corpusPolicy;
    runOptions.seed.reset();
    runOptions.iterStart = 0;
    runOptions.iters = entry.iters;
    runOptions.stepBudget = entry.stepBudget;
    apply_mode_text_to_options(runOptions, entry.mode);
    if (entry.exactCanonicalCap.has_value()) {
        runOptions.exactCanonicalCap = *entry.exactCanonicalCap;
    }
    if (entry.compareSampleRate.has_value()) {
        runOptions.compareSampleRate = *entry.compareSampleRate;
    }
    if (entry.compareBudget.has_value()) {
        runOptions.compareBudget = *entry.compareBudget;
    }
    if (entry.splitChoicePolicyMode.has_value()) {
        runOptions.splitChoicePolicyMode = *entry.splitChoicePolicyMode;
    }
    if (entry.splitChoiceCompareMode.has_value()) {
        runOptions.splitChoiceCompareMode = *entry.splitChoiceCompareMode;
    }
    return runOptions;
}

TestOptions campaign_checkpoint_template_options(
    const TestOptions& options,
    const CampaignConfig& config
) {
    if (config.runs.empty()) {
        return options;
    }
    return make_campaign_run_options(options, config, config.runs.front());
}

void backfill_campaign_stats_metadata(FuzzStats& stats, const TestOptions& templateOptions, const string& caseName) {
    if (stats.caseName.empty()) {
        stats.caseName = caseName;
    }
    if (stats.weightProfile.empty()) {
        stats.weightProfile = weight_profile_name_string(templateOptions.weightProfile);
    }
    if (stats.preconditionBiasProfile.empty()) {
        stats.preconditionBiasProfile = precondition_bias_profile_name_string(templateOptions.preconditionBiasProfile);
    }
    if (stats.scenarioFamily.empty()) {
        stats.scenarioFamily = scenario_family_name_string(templateOptions.scenarioFamily);
    }
    if (stats.splitChoicePolicyMode.empty()) {
        stats.splitChoicePolicyMode = split_choice_policy_mode_name_string(templateOptions.splitChoicePolicyMode);
    }
    if (stats.splitChoiceCompareMode.empty()) {
        stats.splitChoiceCompareMode = split_choice_compare_mode_name_string(templateOptions.splitChoiceCompareMode);
    }
    stats.biasSplit = templateOptions.biasSplit;
    stats.biasJoin = templateOptions.biasJoin;
    stats.biasIntegrate = templateOptions.biasIntegrate;
}

bool campaign_wall_time_exceeded(
    const chrono::steady_clock::time_point& startedAt,
    size_t maxWallSeconds
) {
    if (maxWallSeconds == 0U) {
        return false;
    }
    return chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - startedAt).count() >=
           static_cast<long long>(maxWallSeconds);
}

bool checkpoint_chunk_matches(
    const CampaignCheckpointChunk& chunk,
    size_t runIndex,
    size_t seedIndex,
    u32 seed,
    int iterStart,
    int iterCount
) {
    return chunk.runIndex == runIndex &&
           chunk.seedIndex == seedIndex &&
           chunk.seed == seed &&
           chunk.iterStart == iterStart &&
           chunk.iterCount == iterCount;
}

size_t remaining_campaign_compare_budget(const TestOptions& options, const FuzzStats& aggregate) {
    if (split_choice_compare_enabled(options)) {
        if (options.compareBudget == 0U) {
            return 0U;
        }
        return options.compareBudget > aggregate.splitChoiceCompareStateCount
            ? options.compareBudget - aggregate.splitChoiceCompareStateCount
            : 0U;
    }
    if (options.exactAuditBudget == 0U) {
        return 0U;
    }
    return options.exactAuditBudget > aggregate.exactAuditedStateCount
        ? options.exactAuditBudget - aggregate.exactAuditedStateCount
        : 0U;
}

bool campaign_gate_passed(
    const TestOptions& options,
    const vector<pair<CampaignRunConfig, FuzzStats>>& runs
) {
    bool sawThreshold = false;
    for (const auto& run : runs) {
        const PolicyGraduationEvaluation gate = evaluate_policy_graduation_gate(options, run.second);
        if (gate.threshold.family.empty()) {
            return false;
        }
        sawThreshold = true;
        if (!policy_gate_satisfied(gate.status)) {
            return false;
        }
    }
    return sawThreshold;
}

optional<CampaignStopReason> campaign_checkpoint_stop_reason(
    const TestOptions& options,
    size_t checkpointChunkCount,
    const vector<pair<CampaignRunConfig, FuzzStats>>& runs
) {
    if (options.stopWhenGatePasses && campaign_gate_passed(options, runs)) {
        return CampaignStopReason::GATE_PASSED;
    }
    if (options.maxPartialRuns != 0U && checkpointChunkCount >= options.maxPartialRuns) {
        return CampaignStopReason::MAX_PARTIAL_RUNS;
    }
    return nullopt;
}

void write_campaign_aggregate_outputs(
    const CampaignConfig& config,
    const TestOptions& options,
    const vector<pair<CampaignRunConfig, FuzzStats>>& runs,
    optional<CampaignStopReason> stopReason = nullopt
) {
    if (runs.empty()) {
        return;
    }

    FuzzStats aggregate;
    aggregate.caseName = "campaign_aggregate";
    unordered_set<string> modes;
    PolicyGraduationGateStatus aggregateGateStatus = PolicyGraduationGateStatus::FAIL;
    bool haveGateStatus = false;
    vector<PolicyGraduationEvaluation> runGateEvaluations;
    vector<FuzzStats> effectiveRunStats;
    effectiveRunStats.reserve(runs.size());
    for (const auto& run : runs) {
        FuzzStats aggregateRunStats = run.second;
        const filesystem::path persistedStatsPath = filesystem::absolute(run.first.statsFile);
        const filesystem::path persistedSummaryPath =
            persistedStatsPath.parent_path() / (persistedStatsPath.stem().string() + ".summary.txt");
        if (filesystem::exists(persistedSummaryPath)) {
            FuzzStats persistedRunStats = load_fuzz_stats_from_summary_file(persistedSummaryPath);
            const TestOptions runTemplateOptions = make_campaign_run_options(options, config, run.first);
            backfill_campaign_stats_metadata(
                persistedRunStats,
                runTemplateOptions,
                "campaign_" + sanitize_stem_token(run.first.mode)
            );
            if (persistedRunStats.totalIterations > aggregateRunStats.totalIterations) {
                aggregateRunStats = persistedRunStats;
            }
        }
        effectiveRunStats.push_back(aggregateRunStats);
        modes.insert(run.first.mode);
        merge_fuzz_stats_into(aggregate, aggregateRunStats);
        const PolicyGraduationEvaluation runGate = evaluate_policy_graduation_gate(options, aggregateRunStats);
        runGateEvaluations.push_back(runGate);
        if (!runGate.threshold.family.empty()) {
            aggregateGateStatus = haveGateStatus
                ? combine_policy_graduation_status(aggregateGateStatus, runGate.status)
                : runGate.status;
            haveGateStatus = true;
        }
    }
    if (!haveGateStatus) {
        aggregateGateStatus = PolicyGraduationGateStatus::FAIL;
    }
    const double avgCompareTimePerState = aggregate.splitChoiceCompareStateCount == 0U
        ? 0.0
        : static_cast<double>(
              aggregate.compareCandidateEnumerationNanos +
              aggregate.compareExactShadowEvaluationNanos +
              aggregate.compareExactFullEvaluationNanos +
              aggregate.compareCanonicalizationNanos +
              aggregate.compareMulticlassCatalogNanos
          ) / static_cast<double>(aggregate.splitChoiceCompareStateCount);
    const double avgExactFullEvalTimePerPair = aggregate.splitChoiceExactFullEvalCount == 0U
        ? 0.0
        : static_cast<double>(aggregate.compareExactFullEvaluationNanos) /
              static_cast<double>(aggregate.splitChoiceExactFullEvalCount);
    const double scenarioHashCacheHitRatio = safe_ratio(
        aggregate.compareStateHashCacheHitCount,
        aggregate.compareStateHashCacheHitCount + aggregate.compareStateHashCacheMissCount
    );
    const double pairEvalCacheHitRatio = safe_ratio(
        aggregate.compareCandidateEvaluationCacheHitCount,
        aggregate.compareCandidateEvaluationCacheHitCount + aggregate.compareCandidateEvaluationCacheMissCount
    );
    const double exactCanonicalCacheHitRatio = safe_ratio(
        aggregate.compareExactCanonicalCacheHitCount,
        aggregate.compareExactCanonicalCacheHitCount + aggregate.compareExactCanonicalCacheMissCount
    );

    const filesystem::path statsPath =
        filesystem::absolute(
            config.aggregateStatsFile.empty()
                ? (filesystem::path(runs.front().first.artifactDir) / "logs" / "campaign_aggregate.json").string()
                : config.aggregateStatsFile
        );
    const filesystem::path summaryPath =
        filesystem::absolute(
            config.aggregateSummaryFile.empty()
                ? (statsPath.parent_path() / "campaign_aggregate.summary.txt").string()
                : config.aggregateSummaryFile
        );
    filesystem::create_directories(statsPath.parent_path());
    filesystem::create_directories(summaryPath.parent_path());

    ofstream json(statsPath);
    if (!json) {
        throw runtime_error("failed to write campaign aggregate stats: " + statsPath.string());
    }
    json << "{\n";
    json << "\"case\":\"campaign_aggregate\",\n";
    json << "\"split_choice_policy_mode\":\"" << json_escape(aggregate.splitChoicePolicyMode) << "\",\n";
    json << "\"split_choice_compare_mode\":\"" << json_escape(aggregate.splitChoiceCompareMode) << "\",\n";
    json << "\"run_count\":" << runs.size() << ",\n";
    json << "\"mode_count\":" << modes.size() << ",\n";
    json << "\"target_compared_states\":" << options.targetComparedStates << ",\n";
    json << "\"target_eligible_states\":" << options.targetEligibleStates << ",\n";
    json << "\"target_lineage_samples\":" << options.targetLineageSamples << ",\n";
    json << "\"target_applicability_confidence\":" << json_number(options.targetApplicabilityConfidence) << ",\n";
    json << "\"total_iterations\":" << aggregate.totalIterations << ",\n";
    json << "\"split_ready_count\":" << aggregate.splitReadyCount << ",\n";
    json << "\"join_candidate_count\":" << aggregate.joinCandidateCount << ",\n";
    json << "\"integrate_candidate_count\":" << aggregate.integrateCandidateCount << ",\n";
    json << "\"actual_split_hits\":" << aggregate.actualSplitHits << ",\n";
    json << "\"actual_join_hits\":" << aggregate.actualJoinHits << ",\n";
    json << "\"actual_integrate_hits\":" << aggregate.actualIntegrateHits << ",\n";
    json << "\"split_choice_candidate_count\":" << aggregate.splitChoiceCandidateCount << ",\n";
    json << "\"split_choice_eval_count\":" << aggregate.splitChoiceEvalCount << ",\n";
    json << "\"split_choice_tie_count\":" << aggregate.splitChoiceTieCount << ",\n";
    json << "\"split_choice_multiclass_count\":" << aggregate.splitChoiceMulticlassCount << ",\n";
    json << "\"split_choice_fallback_count\":" << aggregate.splitChoiceFallbackCount << ",\n";
    json << "\"split_choice_compare_state_count\":" << aggregate.splitChoiceCompareStateCount << ",\n";
    json << "\"split_ready_state_count\":" << aggregate.splitReadyStateCount << ",\n";
    json << "\"tie_ready_state_count\":" << aggregate.tieReadyStateCount << ",\n";
    json << "\"compare_eligible_state_count\":" << aggregate.compareEligibleStateCount << ",\n";
    json << "\"compare_ineligible_state_count\":" << aggregate.compareIneligibleStateCount << ",\n";
    json << "\"compare_completed_state_count\":" << aggregate.compareCompletedStateCount << ",\n";
    json << "\"compare_partial_state_count\":" << aggregate.comparePartialStateCount << ",\n";
    json << "\"split_choice_exact_full_eval_count\":" << aggregate.splitChoiceExactFullEvalCount << ",\n";
    json << "\"split_choice_exact_shadow_eval_count\":" << aggregate.splitChoiceExactShadowEvalCount << ",\n";
    json << "\"split_choice_same_representative_count\":" << aggregate.splitChoiceSameRepresentativeCount << ",\n";
    json << "\"split_choice_same_semantic_class_count\":" << aggregate.splitChoiceSameSemanticClassCount << ",\n";
    json << "\"split_choice_same_final_state_count\":" << aggregate.splitChoiceSameFinalStateCount << ",\n";
    json << "\"split_choice_semantic_disagreement_count\":" << aggregate.splitChoiceSemanticDisagreementCount << ",\n";
    json << "\"split_choice_cap_hit_count\":" << aggregate.splitChoiceCapHitCount << ",\n";
    json << "\"split_choice_harmless_compare_count\":" << aggregate.splitChoiceHarmlessCompareCount << ",\n";
    json << "\"split_choice_trace_only_compare_count\":" << aggregate.splitChoiceTraceOnlyCompareCount << ",\n";
    json << "\"exact_audited_state_count\":" << aggregate.exactAuditedStateCount << ",\n";
    json << "\"exact_audited_pair_count\":" << aggregate.exactAuditedPairCount << ",\n";
    json << "\"fast_vs_exact_class_disagreement_count\":" << aggregate.fastVsExactClassDisagreementCount << ",\n";
    json << "\"representative_shift_count\":" << aggregate.representativeShiftCount << ",\n";
    json << "\"representative_shift_same_class_count\":" << aggregate.representativeShiftSameClassCount << ",\n";
    json << "\"representative_shift_semantic_divergence_count\":" << aggregate.representativeShiftSemanticDivergenceCount << ",\n";
    json << "\"representative_shift_followup_divergence_count\":" << aggregate.representativeShiftFollowupDivergenceCount << ",\n";
    json << "\"representative_shift_trace_divergence_count\":" << aggregate.representativeShiftTraceDivergenceCount << ",\n";
    json << "\"harmless_shift_count\":" << aggregate.harmlessShiftCount << ",\n";
    json << "\"trace_only_shift_count\":" << aggregate.traceOnlyShiftCount << ",\n";
    json << "\"semantic_shift_count\":" << aggregate.semanticShiftCount << ",\n";
    json << "\"compare_candidate_enumeration_ns\":" << aggregate.compareCandidateEnumerationNanos << ",\n";
    json << "\"compare_exact_shadow_evaluation_ns\":" << aggregate.compareExactShadowEvaluationNanos << ",\n";
    json << "\"compare_exact_full_evaluation_ns\":" << aggregate.compareExactFullEvaluationNanos << ",\n";
    json << "\"compare_canonicalization_ns\":" << aggregate.compareCanonicalizationNanos << ",\n";
    json << "\"compare_multiclass_catalog_ns\":" << aggregate.compareMulticlassCatalogNanos << ",\n";
    json << "\"compare_state_hash_cache_hit_count\":" << aggregate.compareStateHashCacheHitCount << ",\n";
    json << "\"compare_state_hash_cache_miss_count\":" << aggregate.compareStateHashCacheMissCount << ",\n";
    json << "\"compare_candidate_evaluation_cache_hit_count\":" << aggregate.compareCandidateEvaluationCacheHitCount << ",\n";
    json << "\"compare_candidate_evaluation_cache_miss_count\":" << aggregate.compareCandidateEvaluationCacheMissCount << ",\n";
    json << "\"compare_exact_canonical_cache_hit_count\":" << aggregate.compareExactCanonicalCacheHitCount << ",\n";
    json << "\"compare_exact_canonical_cache_miss_count\":" << aggregate.compareExactCanonicalCacheMissCount << ",\n";
    json << "\"avg_compare_time_per_state_ns\":" << json_number(avgCompareTimePerState) << ",\n";
    json << "\"avg_exact_full_eval_time_per_pair_ns\":" << json_number(avgExactFullEvalTimePerPair) << ",\n";
    json << "\"scenario_hash_cache_hit_ratio\":" << json_number(scenarioHashCacheHitRatio) << ",\n";
    json << "\"exact_full_pair_evaluation_cache_hit_ratio\":" << json_number(pairEvalCacheHitRatio) << ",\n";
    json << "\"exact_canonical_key_cache_hit_ratio\":" << json_number(exactCanonicalCacheHitRatio) << ",\n";
    json << "\"gate_status\":\"" << policy_graduation_gate_status_name(aggregateGateStatus) << "\",\n";
    json << "\"stop_reason\":\""
         << (stopReason.has_value() ? campaign_stop_reason_name(*stopReason) : "pending") << "\",\n";
    json << "\"exact_audit_skipped_cap_count\":" << aggregate.exactAuditSkippedCapCount << ",\n";
    json << "\"exact_audit_skipped_budget_count\":" << aggregate.exactAuditSkippedBudgetCount << ",\n";
    json << "\"exact_audit_skipped_sample_count\":" << aggregate.exactAuditSkippedSampleCount << ",\n";
    json << "\"exact_audit_skipped_family_count\":" << aggregate.exactAuditSkippedFamilyCount << ",\n";
    json << "\"exact_audit_skipped_non_tie_count\":" << aggregate.exactAuditSkippedNonTieCount << ",\n";
    json << "\"multiclass_catalog_cluster_count\":" << aggregate.multiclassCatalogClusterCount << ",\n";
    json << "\"multiclass_harmless_cluster_count\":" << aggregate.multiclassHarmlessClusterCount << ",\n";
    json << "\"multiclass_trace_only_cluster_count\":" << aggregate.multiclassTraceOnlyClusterCount << ",\n";
    json << "\"multiclass_semantic_shift_cluster_count\":" << aggregate.multiclassSemanticShiftClusterCount << ",\n";
    json << "\"multiclass_catalog_histogram\":" << json_object_from_map(aggregate.multiclassCatalogHistogram) << ",\n";
    json << "\"split_choice_equiv_class_count_histogram\":"
         << json_object_from_numeric_map(aggregate.splitChoiceEquivClassCountHistogram) << ",\n";
    json << "\"compare_ineligible_reason_histogram\":"
         << json_object_from_map(aggregate.compareIneligibleReasonHistogram) << ",\n";
    json << "\"trace_prefix_histogram\":" << json_object_from_map(aggregate.tracePrefixHistogram) << ",\n";
    json << "\"primitive_multiset_histogram\":" << json_object_from_map(aggregate.primitiveMultisetHistogram) << ",\n";
    json << "\"oracle_mismatch_count\":" << json_object_from_map(aggregate.oracleMismatchCount) << "\n";
    json << "}\n";

    ofstream summary(summaryPath);
    if (!summary) {
        throw runtime_error("failed to write campaign aggregate summary: " + summaryPath.string());
    }
    summary << "runs=" << runs.size() << '\n';
    summary << "modes=" << modes.size() << '\n';
    summary << "split_choice_policy_mode=" << aggregate.splitChoicePolicyMode << '\n';
    summary << "split_choice_compare_mode=" << aggregate.splitChoiceCompareMode << '\n';
    summary << "target_compared_states=" << options.targetComparedStates << '\n';
    summary << "target_eligible_states=" << options.targetEligibleStates << '\n';
    summary << "target_lineage_samples=" << options.targetLineageSamples << '\n';
    summary << "target_applicability_confidence=" << json_number(options.targetApplicabilityConfidence) << '\n';
    summary << "total_iterations=" << aggregate.totalIterations << '\n';
    summary << "split_ready_count=" << aggregate.splitReadyCount << '\n';
    summary << "join_candidate_count=" << aggregate.joinCandidateCount << '\n';
    summary << "integrate_candidate_count=" << aggregate.integrateCandidateCount << '\n';
    summary << "actual_split_hits=" << aggregate.actualSplitHits << '\n';
    summary << "actual_join_hits=" << aggregate.actualJoinHits << '\n';
    summary << "actual_integrate_hits=" << aggregate.actualIntegrateHits << '\n';
    summary << "split_choice_candidate_count=" << aggregate.splitChoiceCandidateCount << '\n';
    summary << "split_choice_eval_count=" << aggregate.splitChoiceEvalCount << '\n';
    summary << "split_choice_tie_count=" << aggregate.splitChoiceTieCount << '\n';
    summary << "split_choice_multiclass_count=" << aggregate.splitChoiceMulticlassCount << '\n';
    summary << "split_choice_fallback_count=" << aggregate.splitChoiceFallbackCount << '\n';
    summary << "split_choice_compare_state_count=" << aggregate.splitChoiceCompareStateCount << '\n';
    summary << "split_ready_state_count=" << aggregate.splitReadyStateCount << '\n';
    summary << "tie_ready_state_count=" << aggregate.tieReadyStateCount << '\n';
    summary << "compare_eligible_state_count=" << aggregate.compareEligibleStateCount << '\n';
    summary << "compare_ineligible_state_count=" << aggregate.compareIneligibleStateCount << '\n';
    summary << "compare_completed_state_count=" << aggregate.compareCompletedStateCount << '\n';
    summary << "compare_partial_state_count=" << aggregate.comparePartialStateCount << '\n';
    summary << "split_choice_exact_full_eval_count=" << aggregate.splitChoiceExactFullEvalCount << '\n';
    summary << "split_choice_exact_shadow_eval_count=" << aggregate.splitChoiceExactShadowEvalCount << '\n';
    summary << "split_choice_same_representative_count=" << aggregate.splitChoiceSameRepresentativeCount << '\n';
    summary << "split_choice_same_semantic_class_count=" << aggregate.splitChoiceSameSemanticClassCount << '\n';
    summary << "split_choice_same_final_state_count=" << aggregate.splitChoiceSameFinalStateCount << '\n';
    summary << "split_choice_semantic_disagreement_count=" << aggregate.splitChoiceSemanticDisagreementCount << '\n';
    summary << "split_choice_cap_hit_count=" << aggregate.splitChoiceCapHitCount << '\n';
    summary << "split_choice_harmless_compare_count=" << aggregate.splitChoiceHarmlessCompareCount << '\n';
    summary << "split_choice_trace_only_compare_count=" << aggregate.splitChoiceTraceOnlyCompareCount << '\n';
    summary << "exact_audited_state_count=" << aggregate.exactAuditedStateCount << '\n';
    summary << "exact_audited_pair_count=" << aggregate.exactAuditedPairCount << '\n';
    summary << "fast_vs_exact_class_disagreement_count=" << aggregate.fastVsExactClassDisagreementCount << '\n';
    summary << "representative_shift_count=" << aggregate.representativeShiftCount << '\n';
    summary << "representative_shift_same_class_count=" << aggregate.representativeShiftSameClassCount << '\n';
    summary << "representative_shift_semantic_divergence_count=" << aggregate.representativeShiftSemanticDivergenceCount << '\n';
    summary << "representative_shift_followup_divergence_count=" << aggregate.representativeShiftFollowupDivergenceCount << '\n';
    summary << "representative_shift_trace_divergence_count=" << aggregate.representativeShiftTraceDivergenceCount << '\n';
    summary << "harmless_shift_count=" << aggregate.harmlessShiftCount << '\n';
    summary << "trace_only_shift_count=" << aggregate.traceOnlyShiftCount << '\n';
    summary << "semantic_shift_count=" << aggregate.semanticShiftCount << '\n';
    summary << "compare_candidate_enumeration_ns=" << aggregate.compareCandidateEnumerationNanos << '\n';
    summary << "compare_exact_shadow_evaluation_ns=" << aggregate.compareExactShadowEvaluationNanos << '\n';
    summary << "compare_exact_full_evaluation_ns=" << aggregate.compareExactFullEvaluationNanos << '\n';
    summary << "compare_canonicalization_ns=" << aggregate.compareCanonicalizationNanos << '\n';
    summary << "compare_multiclass_catalog_ns=" << aggregate.compareMulticlassCatalogNanos << '\n';
    summary << "compare_state_hash_cache_hit_count=" << aggregate.compareStateHashCacheHitCount << '\n';
    summary << "compare_state_hash_cache_miss_count=" << aggregate.compareStateHashCacheMissCount << '\n';
    summary << "compare_candidate_evaluation_cache_hit_count=" << aggregate.compareCandidateEvaluationCacheHitCount << '\n';
    summary << "compare_candidate_evaluation_cache_miss_count=" << aggregate.compareCandidateEvaluationCacheMissCount << '\n';
    summary << "compare_exact_canonical_cache_hit_count=" << aggregate.compareExactCanonicalCacheHitCount << '\n';
    summary << "compare_exact_canonical_cache_miss_count=" << aggregate.compareExactCanonicalCacheMissCount << '\n';
    summary << "avg_compare_time_per_state_ns=" << json_number(avgCompareTimePerState) << '\n';
    summary << "avg_exact_full_eval_time_per_pair_ns=" << json_number(avgExactFullEvalTimePerPair) << '\n';
    summary << "scenario_hash_cache_hit_ratio=" << json_number(scenarioHashCacheHitRatio) << '\n';
    summary << "exact_full_pair_evaluation_cache_hit_ratio=" << json_number(pairEvalCacheHitRatio) << '\n';
    summary << "exact_canonical_key_cache_hit_ratio=" << json_number(exactCanonicalCacheHitRatio) << '\n';
    summary << "gate_status=" << policy_graduation_gate_status_name(aggregateGateStatus) << '\n';
    summary << "stop_reason="
            << (stopReason.has_value() ? campaign_stop_reason_name(*stopReason) : "pending") << '\n';
    summary << "exact_audit_skipped_cap_count=" << aggregate.exactAuditSkippedCapCount << '\n';
    summary << "exact_audit_skipped_budget_count=" << aggregate.exactAuditSkippedBudgetCount << '\n';
    summary << "exact_audit_skipped_sample_count=" << aggregate.exactAuditSkippedSampleCount << '\n';
    summary << "exact_audit_skipped_family_count=" << aggregate.exactAuditSkippedFamilyCount << '\n';
    summary << "exact_audit_skipped_non_tie_count=" << aggregate.exactAuditSkippedNonTieCount << '\n';
    summary << "multiclass_catalog_cluster_count=" << aggregate.multiclassCatalogClusterCount << '\n';
    summary << "multiclass_harmless_cluster_count=" << aggregate.multiclassHarmlessClusterCount << '\n';
    summary << "multiclass_trace_only_cluster_count=" << aggregate.multiclassTraceOnlyClusterCount << '\n';
    summary << "multiclass_semantic_shift_cluster_count=" << aggregate.multiclassSemanticShiftClusterCount << '\n';
    summary << "multiclass_catalog_histogram=" << json_object_from_map(aggregate.multiclassCatalogHistogram) << '\n';
    summary << "split_choice_equiv_class_count_histogram="
            << json_object_from_numeric_map(aggregate.splitChoiceEquivClassCountHistogram) << '\n';
    summary << "compare_ineligible_reason_histogram="
            << json_object_from_map(aggregate.compareIneligibleReasonHistogram) << '\n';
    for (size_t i = 0; i < runs.size(); ++i) {
        const PolicyGraduationEvaluation& runGate = runGateEvaluations[i];
        summary << "run_gate="
                << "mode=" << runs[i].first.mode
                << " family=" << effectiveRunStats[i].scenarioFamily
                << " gate_status=" << policy_graduation_gate_status_name(runGate.status)
                << " compared_state_count=" << effectiveRunStats[i].splitChoiceCompareStateCount
                << " split_ready_state_count=" << effectiveRunStats[i].splitReadyStateCount
                << " tie_ready_state_count=" << effectiveRunStats[i].tieReadyStateCount
                << " eligible_state_count=" << effectiveRunStats[i].compareEligibleStateCount
                << " completed_state_count=" << effectiveRunStats[i].compareCompletedStateCount
                << " target_compared_states=" << runGate.effectiveComparedTarget
                << " target_eligible_states=" << runGate.effectiveEligibleTarget
                << " target_completed_states=" << runGate.effectiveCompletedTarget
                << " target_lineage_samples=" << runGate.effectiveLineageTarget
                << " target_applicability_states=" << runGate.effectiveApplicabilityTarget
                << " target_applicability_confidence=" << json_number(runGate.effectiveApplicabilityConfidence)
                << " rationale=" << runGate.rationale
                << '\n';
    }
    write_compare_profile_summary(statsPath, aggregate);
}

#if 0
void run_campaign_case(const TestOptions& options) {
    optional<CampaignCheckpointManifest> resumeManifest;
    filesystem::path configPath;
    if (options.resumeFrom.has_value()) {
        resumeManifest = load_campaign_checkpoint_manifest(*options.resumeFrom);
        configPath = options.campaignConfig.has_value()
            ? filesystem::absolute(*options.campaignConfig)
            : resumeManifest->campaignConfigSnapshot;
    } else if (options.campaignConfig.has_value()) {
        configPath = filesystem::absolute(*options.campaignConfig);
    } else {
        throw runtime_error("campaign requires --campaign-config or --resume-from");
    }

    TestOptions effectiveOptions = options;
    if (resumeManifest.has_value()) {
        if (!effectiveOptions.campaignConfig.has_value()) {
            effectiveOptions.campaignConfig = configPath.string();
        }
        if (!effectiveOptions.checkpointDir.has_value()) {
            effectiveOptions.checkpointDir = resumeManifest->checkpointDir.string();
        }
        effectiveOptions.checkpointEvery = resumeManifest->checkpointEvery;
        effectiveOptions.compareSampleRate = resumeManifest->compareSampleRate;
        effectiveOptions.compareBudget = resumeManifest->compareBudget;
        effectiveOptions.exactCanonicalCap = resumeManifest->exactCanonicalCap;
        if (effectiveOptions.targetComparedStates == 0U) {
            effectiveOptions.targetComparedStates = resumeManifest->targetComparedStates;
        }
        if (effectiveOptions.targetEligibleStates == 0U) {
            effectiveOptions.targetEligibleStates = resumeManifest->targetEligibleStates;
        }
        if (effectiveOptions.maxPartialRuns == 0U) {
            effectiveOptions.maxPartialRuns = resumeManifest->maxPartialRuns;
        }
        effectiveOptions.stopWhenGatePasses =
            effectiveOptions.stopWhenGatePasses || resumeManifest->stopWhenGatePasses;
        effectiveOptions.splitChoiceCompareMode = resumeManifest->splitChoiceCompareMode;
        effectiveOptions.splitChoicePolicyMode = resumeManifest->splitChoicePolicyMode;
        effectiveOptions.corpusPolicy = resumeManifest->corpusPolicy;
        effectiveOptions.saveCorpusDir = resumeManifest->saveCorpusDir;
        effectiveOptions.loadCorpusDir = resumeManifest->loadCorpusDir;
    }

    const CampaignConfig config = load_campaign_config(configPath);
    filesystem::path checkpointDir;
    if (effectiveOptions.checkpointDir.has_value()) {
        checkpointDir = filesystem::absolute(*effectiveOptions.checkpointDir);
    } else if (effectiveOptions.checkpointEvery != 0U || effectiveOptions.stopAfterCheckpoint ||
               effectiveOptions.maxWallSeconds != 0U || effectiveOptions.stopWhenGatePasses ||
               effectiveOptions.targetComparedStates != 0U || effectiveOptions.targetEligibleStates != 0U ||
               effectiveOptions.maxPartialRuns != 0U) {
        checkpointDir = filesystem::absolute(
            filesystem::path(config.runs.front().artifactDir) / "checkpoints" / sanitize_stem_token(configPath.stem().string())
        );
    }
    const bool checkpointingEnabled = !checkpointDir.empty();
    const size_t effectiveCheckpointEvery =
        checkpointingEnabled && effectiveOptions.checkpointEvery == 0U ? 100U : effectiveOptions.checkpointEvery;

    vector<CampaignCheckpointChunk> existingChunks;
    if (checkpointingEnabled) {
        optional<CampaignCheckpointManifest> priorManifest;
        if (filesystem::exists(checkpoint_manifest_path(checkpointDir))) {
            priorManifest = load_campaign_checkpoint_manifest(checkpointDir);
        }
        const TestOptions checkpointTemplateOptions =
            campaign_checkpoint_template_options(effectiveOptions, config);
        CampaignCheckpointManifest manifest;
        manifest.checkpointDir = checkpointDir;
        manifest.campaignConfigSnapshot = ensure_campaign_config_snapshot(checkpointDir, configPath);
        manifest.splitChoicePolicyMode = checkpointTemplateOptions.splitChoicePolicyMode;
        manifest.splitChoiceCompareMode = checkpointTemplateOptions.splitChoiceCompareMode;
        manifest.compareSampleRate = checkpointTemplateOptions.compareSampleRate;
        manifest.compareBudget = checkpointTemplateOptions.compareBudget;
        manifest.exactCanonicalCap = checkpointTemplateOptions.exactCanonicalCap;
        manifest.targetComparedStates = effectiveOptions.targetComparedStates;
        manifest.targetEligibleStates = effectiveOptions.targetEligibleStates;
        manifest.maxPartialRuns = effectiveOptions.maxPartialRuns;
        manifest.stopWhenGatePasses = effectiveOptions.stopWhenGatePasses;
        manifest.saveCorpusDir = checkpointTemplateOptions.saveCorpusDir;
        manifest.loadCorpusDir = checkpointTemplateOptions.loadCorpusDir;
        manifest.corpusPolicy = checkpointTemplateOptions.corpusPolicy;
        manifest.checkpointEvery = effectiveCheckpointEvery;
        existingChunks = load_campaign_checkpoint_chunks(checkpointDir);
        validate_campaign_checkpoint_chunks(checkpointDir, existingChunks);
        if (priorManifest.has_value()) {
            validate_campaign_checkpoint_manifest_compatibility(
                checkpointDir,
                *priorManifest,
                manifest,
                !existingChunks.empty()
            );
        }
        write_campaign_checkpoint_manifest(manifest);
    }

    vector<pair<CampaignRunConfig, FuzzStats>> runStats;
    runStats.reserve(config.runs.size());

        for (size_t runIndex = 0; runIndex < config.runs.size(); ++runIndex) {
            const CampaignRunConfig& entry = config.runs[runIndex];
            if (entry.caseName != "planner_oracle_fuzz") {
                throw runtime_error("campaign currently supports planner_oracle_fuzz only: " + entry.caseName);
            }

            const TestOptions runTemplateOptions = make_campaign_run_options(effectiveOptions, config, entry);
            const filesystem::path runStatsPath = filesystem::absolute(entry.statsFile);
            FuzzStats aggregate;
            unordered_set<string> completedChunkKeys;
            for (const CampaignCheckpointChunk& chunk : existingChunks) {
                if (chunk.runIndex != runIndex || !filesystem::exists(chunk.summaryFile)) {
                continue;
            }
            const string chunkKey =
                checkpoint_chunk_stem(chunk.runIndex, chunk.seedIndex, chunk.seed, chunk.iterStart, chunk.iterCount);
            if (!completedChunkKeys.insert(chunkKey).second) {
                continue;
                }
                merge_fuzz_stats_into(aggregate, load_fuzz_stats_from_summary_file(chunk.summaryFile));
            }
            backfill_campaign_stats_metadata(
                aggregate,
                runTemplateOptions,
                "campaign_" + sanitize_stem_token(entry.mode)
            );
            runStats.push_back({entry, aggregate});
            if (aggregate.totalIterations != 0U) {
                write_fuzz_stats_outputs_from_stats(runStatsPath, aggregate);
            }
    }
    write_campaign_aggregate_outputs(config, runStats);

    const chrono::steady_clock::time_point startedAt = chrono::steady_clock::now();

    try {
        for (size_t runIndex = 0; runIndex < config.runs.size(); ++runIndex) {
            const CampaignRunConfig& entry = config.runs[runIndex];
            const TestOptions runTemplateOptions = make_campaign_run_options(effectiveOptions, config, entry);

            const filesystem::path runStatsPath = filesystem::absolute(entry.statsFile);
            const int totalIters = max(0, entry.iters);
            const int checkpointChunkSize = effectiveCheckpointEvery == 0U
                ? totalIters
                : static_cast<int>(effectiveCheckpointEvery);

            for (size_t seedIndex = 0; seedIndex < entry.seeds.size(); ++seedIndex) {
                const u32 seed = entry.seeds[seedIndex];
                for (int iterStart = 0; iterStart < totalIters; iterStart += max(1, checkpointChunkSize)) {
                    const int iterCount = min(totalIters - iterStart, max(1, checkpointChunkSize));
                    const auto existingIt = find_if(
                        existingChunks.begin(),
                        existingChunks.end(),
                        [&](const CampaignCheckpointChunk& chunk) {
                            return checkpoint_chunk_matches(chunk, runIndex, seedIndex, seed, iterStart, iterCount);
                        }
                    );
                    if (existingIt != existingChunks.end() &&
                        filesystem::exists(existingIt->summaryFile)) {
                        continue;
                    }

                    if (campaign_wall_time_exceeded(startedAt, effectiveOptions.maxWallSeconds)) {
                        write_campaign_aggregate_outputs(config, runStats);
                        return;
                    }

                    TestOptions runOptions = runTemplateOptions;
                    runOptions.seed = seed;
                    runOptions.iterStart = iterStart;
                    runOptions.iters = iterCount;
                    const size_t remainingBudget = remaining_campaign_compare_budget(runOptions, runStats[runIndex].second);
                    if (split_choice_compare_enabled(runOptions)) {
                        if (runOptions.compareBudget != 0U) {
                            if (remainingBudget == 0U) {
                                runOptions.splitChoiceCompareMode = SplitChoiceCompareMode::NONE;
                            } else {
                                runOptions.compareBudget = remainingBudget;
                            }
                        }
                    } else if (runOptions.exactAuditBudget != 0U) {
                        if (remainingBudget == 0U) {
                            runOptions.exactCanonicalCap = 0U;
                        } else {
                            runOptions.exactAuditBudget = remainingBudget;
                        }
                    }

                    const string chunkStem = checkpoint_chunk_stem(runIndex, seedIndex, seed, iterStart, iterCount);
                    const filesystem::path chunkStatsPath = checkpointingEnabled
                        ? (checkpoint_chunks_dir(checkpointDir) / (chunkStem + ".json"))
                        : (runStatsPath.parent_path() / (chunkStem + ".json"));
                    runOptions.statsFile = filesystem::absolute(chunkStatsPath).string();

                    const string chunkCaseName =
                        "campaign_" + sanitize_stem_token(entry.mode) + "_seed" + to_string(seed) +
                        "_iter" + to_string(iterStart);
                    const FuzzStats chunkStats =
                        run_planner_oracle_fuzz_entry(runOptions, chunkCaseName, {seed}, iterCount);

                    if (checkpointingEnabled) {
                        CampaignCheckpointChunk chunk;
                        chunk.runIndex = runIndex;
                        chunk.seedIndex = seedIndex;
                        chunk.seed = seed;
                        chunk.iterStart = iterStart;
                        chunk.iterCount = iterCount;
                        chunk.totalIterations = chunkStats.totalIterations;
                        chunk.compareStateCount = chunkStats.splitChoiceCompareStateCount;
                        chunk.semanticDisagreementCount = chunkStats.splitChoiceSemanticDisagreementCount;
                        chunk.fallbackCount = chunkStats.splitChoiceFallbackCount;
                        chunk.caseName = entry.caseName;
                        chunk.mode = entry.mode;
                        chunk.artifactDir = runOptions.artifactDir.value_or(string());
                        chunk.statsFile = filesystem::absolute(chunkStatsPath).string();
                        chunk.summaryFile = filesystem::absolute(
                            chunkStatsPath.parent_path() / (chunkStatsPath.stem().string() + ".summary.txt")
                        ).string();
                        write_campaign_checkpoint_chunk(checkpointDir, chunk);
                        existingChunks.push_back(chunk);
                    }

                    merge_fuzz_stats_into(runStats[runIndex].second, chunkStats);
                    backfill_campaign_stats_metadata(
                        runStats[runIndex].second,
                        runTemplateOptions,
                        "campaign_" + sanitize_stem_token(entry.mode)
                    );
                    write_fuzz_stats_outputs_from_stats(runStatsPath, runStats[runIndex].second);
                    write_campaign_aggregate_outputs(config, runStats);

                    if (effectiveOptions.stopAfterCheckpoint ||
                        campaign_wall_time_exceeded(startedAt, effectiveOptions.maxWallSeconds)) {
                        return;
                    }
                }
            }
        }
    } catch (...) {
        write_campaign_aggregate_outputs(config, runStats);
        throw;
    }

    write_campaign_aggregate_outputs(config, runStats);
}

#endif

void run_campaign_case(const TestOptions& options) {
    optional<CampaignCheckpointManifest> resumeManifest;
    filesystem::path configPath;
    if (options.resumeFrom.has_value()) {
        resumeManifest = load_campaign_checkpoint_manifest(*options.resumeFrom);
        configPath = options.campaignConfig.has_value()
            ? filesystem::absolute(*options.campaignConfig)
            : resumeManifest->campaignConfigSnapshot;
    } else if (options.campaignConfig.has_value()) {
        configPath = filesystem::absolute(*options.campaignConfig);
    } else {
        throw runtime_error("campaign requires --campaign-config or --resume-from");
    }

    TestOptions effectiveOptions = options;
    if (resumeManifest.has_value()) {
        if (!effectiveOptions.campaignConfig.has_value()) {
            effectiveOptions.campaignConfig = configPath.string();
        }
        if (!effectiveOptions.checkpointDir.has_value()) {
            effectiveOptions.checkpointDir = resumeManifest->checkpointDir.string();
        }
        effectiveOptions.checkpointEvery = resumeManifest->checkpointEvery;
        effectiveOptions.compareSampleRate = resumeManifest->compareSampleRate;
        effectiveOptions.compareBudget = resumeManifest->compareBudget;
        effectiveOptions.exactCanonicalCap = resumeManifest->exactCanonicalCap;
        effectiveOptions.targetComparedStates = resumeManifest->targetComparedStates;
        effectiveOptions.targetEligibleStates = resumeManifest->targetEligibleStates;
        effectiveOptions.targetLineageSamples = resumeManifest->targetLineageSamples;
        effectiveOptions.maxPartialRuns = resumeManifest->maxPartialRuns;
        effectiveOptions.stopWhenGatePasses = resumeManifest->stopWhenGatePasses;
        effectiveOptions.targetApplicabilityConfidence = resumeManifest->targetApplicabilityConfidence;
        effectiveOptions.splitChoiceCompareMode = resumeManifest->splitChoiceCompareMode;
        effectiveOptions.splitChoicePolicyMode = resumeManifest->splitChoicePolicyMode;
        effectiveOptions.corpusPolicy = resumeManifest->corpusPolicy;
        effectiveOptions.saveCorpusDir = resumeManifest->saveCorpusDir;
        effectiveOptions.loadCorpusDir = resumeManifest->loadCorpusDir;
    }

    const CampaignConfig config = load_campaign_config(configPath);
    filesystem::path checkpointDir;
    if (effectiveOptions.checkpointDir.has_value()) {
        checkpointDir = filesystem::absolute(*effectiveOptions.checkpointDir);
    } else if (effectiveOptions.checkpointEvery != 0U || effectiveOptions.stopAfterCheckpoint ||
               effectiveOptions.maxWallSeconds != 0U || effectiveOptions.stopWhenGatePasses ||
               effectiveOptions.targetComparedStates != 0U || effectiveOptions.targetEligibleStates != 0U ||
               effectiveOptions.targetLineageSamples != 0U ||
               effectiveOptions.targetApplicabilityConfidence != 0.0 ||
               effectiveOptions.maxPartialRuns != 0U) {
        checkpointDir = filesystem::absolute(
            filesystem::path(config.runs.front().artifactDir) / "checkpoints" / sanitize_stem_token(configPath.stem().string())
        );
    }
    const bool checkpointingEnabled = !checkpointDir.empty();
    const size_t effectiveCheckpointEvery =
        checkpointingEnabled && effectiveOptions.checkpointEvery == 0U ? 100U : effectiveOptions.checkpointEvery;

    vector<CampaignCheckpointChunk> existingChunks;
    if (checkpointingEnabled) {
        optional<CampaignCheckpointManifest> priorManifest;
        if (filesystem::exists(checkpoint_manifest_path(checkpointDir))) {
            priorManifest = load_campaign_checkpoint_manifest(checkpointDir);
        }
        const TestOptions checkpointTemplateOptions =
            campaign_checkpoint_template_options(effectiveOptions, config);
        CampaignCheckpointManifest manifest;
        manifest.checkpointDir = checkpointDir;
        manifest.campaignConfigSnapshot = ensure_campaign_config_snapshot(checkpointDir, configPath);
        manifest.splitChoicePolicyMode = checkpointTemplateOptions.splitChoicePolicyMode;
        manifest.splitChoiceCompareMode = checkpointTemplateOptions.splitChoiceCompareMode;
        manifest.compareSampleRate = checkpointTemplateOptions.compareSampleRate;
        manifest.compareBudget = checkpointTemplateOptions.compareBudget;
        manifest.exactCanonicalCap = checkpointTemplateOptions.exactCanonicalCap;
        manifest.targetComparedStates = effectiveOptions.targetComparedStates;
        manifest.targetEligibleStates = effectiveOptions.targetEligibleStates;
        manifest.targetLineageSamples = effectiveOptions.targetLineageSamples;
        manifest.maxPartialRuns = effectiveOptions.maxPartialRuns;
        manifest.stopWhenGatePasses = effectiveOptions.stopWhenGatePasses;
        manifest.targetApplicabilityConfidence = effectiveOptions.targetApplicabilityConfidence;
        manifest.saveCorpusDir = checkpointTemplateOptions.saveCorpusDir;
        manifest.loadCorpusDir = checkpointTemplateOptions.loadCorpusDir;
        manifest.corpusPolicy = checkpointTemplateOptions.corpusPolicy;
        manifest.checkpointEvery = effectiveCheckpointEvery;
        existingChunks = load_campaign_checkpoint_chunks(checkpointDir);
        validate_campaign_checkpoint_chunks(checkpointDir, existingChunks);
        if (priorManifest.has_value()) {
            validate_campaign_checkpoint_manifest_compatibility(
                checkpointDir,
                *priorManifest,
                manifest,
                !existingChunks.empty()
            );
        }
        write_campaign_checkpoint_manifest(manifest);
    }

    vector<pair<CampaignRunConfig, FuzzStats>> runStats;
    runStats.reserve(config.runs.size());
    for (size_t runIndex = 0; runIndex < config.runs.size(); ++runIndex) {
        const CampaignRunConfig& entry = config.runs[runIndex];
        if (entry.caseName != "planner_oracle_fuzz") {
            throw runtime_error("campaign currently supports planner_oracle_fuzz only: " + entry.caseName);
        }

        const TestOptions runTemplateOptions = make_campaign_run_options(effectiveOptions, config, entry);
        const filesystem::path runStatsPath = filesystem::absolute(entry.statsFile);
        FuzzStats aggregate;
        unordered_set<string> completedChunkKeys;
        for (const CampaignCheckpointChunk& chunk : existingChunks) {
            if (chunk.runIndex != runIndex || !filesystem::exists(chunk.summaryFile)) {
                continue;
            }
            const string chunkKey =
                checkpoint_chunk_stem(chunk.runIndex, chunk.seedIndex, chunk.seed, chunk.iterStart, chunk.iterCount);
            if (!completedChunkKeys.insert(chunkKey).second) {
                continue;
            }
            merge_fuzz_stats_into(aggregate, load_fuzz_stats_from_summary_file(chunk.summaryFile));
        }
        backfill_campaign_stats_metadata(
            aggregate,
            runTemplateOptions,
            "campaign_" + sanitize_stem_token(entry.mode)
        );
        runStats.push_back({entry, aggregate});
        if (aggregate.totalIterations != 0U) {
            write_fuzz_stats_outputs_from_stats(runStatsPath, aggregate);
        }
    }
    write_campaign_aggregate_outputs(config, effectiveOptions, runStats);
    if (const optional<CampaignStopReason> stopReason =
            campaign_checkpoint_stop_reason(effectiveOptions, existingChunks.size(), runStats);
        stopReason.has_value()) {
        write_campaign_aggregate_outputs(config, effectiveOptions, runStats, *stopReason);
        return;
    }

    const chrono::steady_clock::time_point startedAt = chrono::steady_clock::now();
    try {
        for (size_t runIndex = 0; runIndex < config.runs.size(); ++runIndex) {
            const CampaignRunConfig& entry = config.runs[runIndex];
            const TestOptions runTemplateOptions = make_campaign_run_options(effectiveOptions, config, entry);
            const filesystem::path runStatsPath = filesystem::absolute(entry.statsFile);
            const int totalIters = max(0, entry.iters);
            const int checkpointChunkSize = effectiveCheckpointEvery == 0U
                ? totalIters
                : static_cast<int>(effectiveCheckpointEvery);

            for (size_t seedIndex = 0; seedIndex < entry.seeds.size(); ++seedIndex) {
                const u32 seed = entry.seeds[seedIndex];
                for (int iterStart = 0; iterStart < totalIters; iterStart += max(1, checkpointChunkSize)) {
                    const int iterCount = min(totalIters - iterStart, max(1, checkpointChunkSize));
                    const auto existingIt = find_if(
                        existingChunks.begin(),
                        existingChunks.end(),
                        [&](const CampaignCheckpointChunk& chunk) {
                            return checkpoint_chunk_matches(chunk, runIndex, seedIndex, seed, iterStart, iterCount);
                        }
                    );
                    if (existingIt != existingChunks.end() && filesystem::exists(existingIt->summaryFile)) {
                        continue;
                    }

                    if (campaign_wall_time_exceeded(startedAt, effectiveOptions.maxWallSeconds)) {
                        write_campaign_aggregate_outputs(
                            config,
                            effectiveOptions,
                            runStats,
                            CampaignStopReason::MAX_WALL_SECONDS
                        );
                        return;
                    }

                    TestOptions runOptions = runTemplateOptions;
                    runOptions.seed = seed;
                    runOptions.iterStart = iterStart;
                    runOptions.iters = iterCount;

                    const size_t remainingBudget =
                        remaining_campaign_compare_budget(runOptions, runStats[runIndex].second);
                    if (split_choice_compare_enabled(runOptions)) {
                        if (runOptions.compareBudget != 0U) {
                            if (remainingBudget == 0U) {
                                runOptions.splitChoiceCompareMode = SplitChoiceCompareMode::NONE;
                            } else {
                                runOptions.compareBudget = remainingBudget;
                            }
                        }
                    } else if (runOptions.exactAuditBudget != 0U) {
                        if (remainingBudget == 0U) {
                            runOptions.exactCanonicalCap = 0U;
                        } else {
                            runOptions.exactAuditBudget = remainingBudget;
                        }
                    }

                    const string chunkStem = checkpoint_chunk_stem(runIndex, seedIndex, seed, iterStart, iterCount);
                    const filesystem::path chunkStatsPath = checkpointingEnabled
                        ? (checkpoint_chunks_dir(checkpointDir) / (chunkStem + ".json"))
                        : (runStatsPath.parent_path() / (chunkStem + ".json"));
                    runOptions.statsFile = filesystem::absolute(chunkStatsPath).string();

                    const string chunkCaseName =
                        "campaign_" + sanitize_stem_token(entry.mode) + "_seed" + to_string(seed) +
                        "_iter" + to_string(iterStart);
                    const FuzzStats chunkStats =
                        run_planner_oracle_fuzz_entry(runOptions, chunkCaseName, {seed}, iterCount);

                    if (checkpointingEnabled) {
                        CampaignCheckpointChunk chunk;
                        chunk.runIndex = runIndex;
                        chunk.seedIndex = seedIndex;
                        chunk.seed = seed;
                        chunk.iterStart = iterStart;
                        chunk.iterCount = iterCount;
                        chunk.totalIterations = chunkStats.totalIterations;
                        chunk.compareStateCount = chunkStats.splitChoiceCompareStateCount;
                        chunk.semanticDisagreementCount = chunkStats.splitChoiceSemanticDisagreementCount;
                        chunk.fallbackCount = chunkStats.splitChoiceFallbackCount;
                        chunk.caseName = entry.caseName;
                        chunk.mode = entry.mode;
                        chunk.artifactDir = runOptions.artifactDir.value_or(string());
                        chunk.statsFile = filesystem::absolute(chunkStatsPath).string();
                        chunk.summaryFile = filesystem::absolute(
                            chunkStatsPath.parent_path() / (chunkStatsPath.stem().string() + ".summary.txt")
                        ).string();
                        write_campaign_checkpoint_chunk(checkpointDir, chunk);
                        existingChunks.push_back(chunk);
                    }

                    merge_fuzz_stats_into(runStats[runIndex].second, chunkStats);
                    backfill_campaign_stats_metadata(
                        runStats[runIndex].second,
                        runTemplateOptions,
                        "campaign_" + sanitize_stem_token(entry.mode)
                    );
                    write_fuzz_stats_outputs_from_stats(runStatsPath, runStats[runIndex].second);
                    if (const optional<CampaignStopReason> stopReason =
                            campaign_checkpoint_stop_reason(effectiveOptions, existingChunks.size(), runStats);
                        stopReason.has_value()) {
                        write_campaign_aggregate_outputs(config, effectiveOptions, runStats, *stopReason);
                        return;
                    }
                    if (effectiveOptions.stopAfterCheckpoint) {
                        write_campaign_aggregate_outputs(
                            config,
                            effectiveOptions,
                            runStats,
                            CampaignStopReason::STOP_AFTER_CHECKPOINT
                        );
                        return;
                    }
                    if (campaign_wall_time_exceeded(startedAt, effectiveOptions.maxWallSeconds)) {
                        write_campaign_aggregate_outputs(
                            config,
                            effectiveOptions,
                            runStats,
                            CampaignStopReason::MAX_WALL_SECONDS
                        );
                        return;
                    }
                    write_campaign_aggregate_outputs(config, effectiveOptions, runStats);
                }
            }
        }
    } catch (...) {
        write_campaign_aggregate_outputs(config, effectiveOptions, runStats);
        throw;
    }

    write_campaign_aggregate_outputs(config, effectiveOptions, runStats, CampaignStopReason::COMPLETED);
}

void run_corpus_replay_case(const TestOptions& options) {
    const vector<CorpusEntry> entries = load_corpus_entries(resolve_corpus_dir_for_read(options));
    if (entries.empty()) {
        throw runtime_error("corpus_replay found no corpus entries");
    }

    for (const CorpusEntry& entry : entries) {
        TestOptions replayOptions = options;
        replayOptions.caseName = "planner_oracle_fuzz";
        replayOptions.loadCorpusDir.reset();
        replayOptions.saveCorpusDir.reset();
        replayOptions.stats = true;
        replayOptions.iters = max(1, entry.iters);
        replayOptions.stepBudget = entry.stepBudget;
        if (const optional<WeightProfile> profile = parse_weight_profile_token(entry.weightProfile); profile.has_value()) {
            replayOptions.weightProfile = *profile;
        }
        if (const optional<PreconditionBiasProfile> biasProfile =
                parse_precondition_bias_profile_token(entry.preconditionBiasProfile);
            biasProfile.has_value()) {
            replayOptions.preconditionBiasProfile = *biasProfile;
        }
        if (const optional<ScenarioFamily> family = parse_scenario_family_token(entry.scenarioFamily); family.has_value()) {
            replayOptions.scenarioFamily = *family;
        }
        replayOptions.biasSplit = entry.biasSplit;
        replayOptions.biasJoin = entry.biasJoin;
        replayOptions.biasIntegrate = entry.biasIntegrate;
        (void)run_planner_oracle_fuzz_entry(
            replayOptions,
            "corpus_replay",
            {entry.seed},
            replayOptions.iters.value_or(1)
        );
    }
}

void write_compare_ready_lineage_outputs(
    const TestOptions& options,
    const string& caseName,
    const CompareReadyLineageSummary& summary
) {
    const filesystem::path summaryPath = artifact_subdir(options, "logs") / (caseName + ".summary.txt");
    ofstream summaryOfs(summaryPath);
    if (!summaryOfs) {
        throw runtime_error("failed to write compare-ready lineage summary: " + summaryPath.string());
    }
    summaryOfs << compare_ready_lineage_summary_text(summary);

    const filesystem::path logPath = artifact_subdir(options, "logs") / (caseName + ".lineage.txt");
    ofstream logOfs(logPath);
    if (!logOfs) {
        throw runtime_error("failed to write compare-ready lineage log: " + logPath.string());
    }
    logOfs << compare_ready_lineage_log_text(summary);
}

void write_family_applicability_outputs(
    const TestOptions& options,
    const string& caseName,
    const FamilyApplicabilitySummary& summary
) {
    const filesystem::path summaryPath = artifact_subdir(options, "logs") / (caseName + ".summary.txt");
    ofstream summaryOfs(summaryPath);
    if (!summaryOfs) {
        throw runtime_error("failed to write family applicability summary: " + summaryPath.string());
    }
    summaryOfs << family_applicability_summary_text(summary);

    const filesystem::path logPath = artifact_subdir(options, "logs") / (caseName + ".log.txt");
    ofstream logOfs(logPath);
    if (!logOfs) {
        throw runtime_error("failed to write family applicability log: " + logPath.string());
    }
    logOfs << family_applicability_log_text(summary);
}

vector<u32> collect_lineage_scenario_seeds(
    const TestOptions& options,
    const vector<u32>& defaultSeeds,
    int defaultIters
) {
    const vector<u32> seeds = resolve_seed_list(
        options,
        options.seed.has_value() ? vector<u32>{*options.seed} : defaultSeeds
    );
    const int iters = options.iters.value_or(defaultIters);
    const int iterBase = max(0, options.iterStart);

    vector<u32> scenarioSeeds;
    scenarioSeeds.reserve(seeds.size() * static_cast<size_t>(max(1, iters)));
    for (u32 seed : seeds) {
        for (int offset = 0; offset < iters; ++offset) {
            scenarioSeeds.push_back(mix_seed(seed, iterBase + offset));
        }
    }
    return scenarioSeeds;
}

void run_compare_ready_lineage_audit_case(const TestOptions& options) {
    const vector<u32> scenarioSeeds = collect_lineage_scenario_seeds(options, {950101U, 950102U}, 8);
    const CompareReadyLineageSummary summary = audit_compare_ready_lineage(options, scenarioSeeds);
    write_compare_ready_lineage_outputs(options, "compare_ready_lineage_audit", summary);

    if (summary.generatedStateCount == 0U || summary.compareReadyStateCount == 0U) {
        throw runtime_error("compare_ready_lineage_audit expected paired lineage states");
    }
    if (summary.derivedFromBaseStateCount == 0U || summary.derivedFromBaseStateCount != summary.pairReplayCount) {
        throw runtime_error("compare_ready_lineage_audit expected verified paired lineage replays");
    }
    if (summary.compareCompletedStateCount == 0U) {
        throw runtime_error("compare_ready_lineage_audit expected compare-ready exact compare completions");
    }
    if (summary.sameSemanticClassCount != summary.compareCompletedStateCount ||
        summary.sameFinalStateCount != summary.compareCompletedStateCount ||
        summary.sameTraceClassCount != summary.compareCompletedStateCount) {
        throw runtime_error("compare_ready_lineage_audit expected exact_shadow vs exact_full agreement");
    }
}

void run_compare_ready_lineage_smoke_case(const TestOptions& options) {
    TestOptions smokeOptions = options;
    if (!smokeOptions.seed.has_value()) {
        smokeOptions.seed = 950101U;
    }
    if (!smokeOptions.iters.has_value()) {
        smokeOptions.iters = 2;
    }
    run_compare_ready_lineage_audit_case(smokeOptions);
}

void run_planner_tie_organic_applicability_audit_case(const TestOptions& options) {
    TestOptions auditOptions = options;
    if (auditOptions.iters.value_or(0) <= 0) {
        auditOptions.iters = 24;
    }
    const vector<u32> scenarioSeeds = collect_lineage_scenario_seeds(auditOptions, {970101U, 970102U}, 24);
    const FamilyApplicabilitySummary summary = audit_family_applicability(
        auditOptions,
        ScenarioFamily::PLANNER_TIE_MIXED_ORGANIC,
        scenarioSeeds
    );
    write_family_applicability_outputs(auditOptions, "planner_tie_mixed_organic_applicability_audit", summary);

    if (summary.generatedStateCount == 0U) {
        throw runtime_error("planner_tie_mixed_organic_applicability_audit expected generated states");
    }
    if (summary.actualJoinHits == 0U || summary.actualIntegrateHits == 0U) {
        throw runtime_error("planner_tie_mixed_organic_applicability_audit expected mixed follow-up hits");
    }
}

void run_planner_tie_organic_applicability_smoke_case(const TestOptions& options) {
    TestOptions smokeOptions = options;
    smokeOptions.iters = 24;
    const vector<u32> scenarioSeeds = collect_lineage_scenario_seeds(smokeOptions, {970101U, 970102U}, 24);
    const FamilyApplicabilitySummary summary = audit_family_applicability(
        smokeOptions,
        ScenarioFamily::PLANNER_TIE_MIXED_ORGANIC,
        scenarioSeeds
    );
    write_family_applicability_outputs(smokeOptions, "planner_tie_mixed_organic_applicability_smoke", summary);

    require_test(
        summary.classification == FamilyApplicabilityClassification::NON_APPLICABLE,
        "planner_tie organic applicability smoke expected NON_APPLICABLE"
    );
    require_test(summary.compareEligibleStateCount == 0U, "planner_tie organic applicability smoke expected zero eligible states");
    require_test(
        summary.dominantIneligibleReason == "no_split_ready",
        "planner_tie organic applicability smoke expected dominant no_split_ready classification"
    );
}

void run_planner_tie_organic_proxy_pass_smoke_case(const TestOptions& options) {
    TestOptions proxyOptions = options;
    proxyOptions.targetLineageSamples = 4U;

    CompareReadyLineageSummary lineage;
    lineage.lineageSampleCount = 4U;
    lineage.sameSemanticClassCount = 4U;
    lineage.sameFinalStateCount = 4U;
    lineage.sameTraceClassCount = 4U;
    lineage.followupPatternPreservedCount = 4U;
    lineage.structureOnlyEnrichmentCount = 4U;

    FamilyApplicabilitySummary applicability;
    applicability.family = "planner_tie_mixed_organic";
    applicability.classification = FamilyApplicabilityClassification::UNDER_GENERATED;

    FuzzStats proxyStats;
    proxyStats.scenarioFamily = "planner_tie_mixed_organic_compare_ready";
    proxyStats.splitChoiceCompareStateCount = 32U;
    proxyStats.compareEligibleStateCount = 32U;
    proxyStats.compareCompletedStateCount = 32U;
    PolicyGraduationEvaluation gate =
        evaluate_proxy_policy_graduation_gate(proxyOptions, applicability, lineage, proxyStats);
    require_test(
        gate.status == PolicyGraduationGateStatus::PROXY_PASS,
        "planner_tie organic proxy smoke expected PROXY_PASS"
    );
}

filesystem::path phase16_resume_gate_smoke_config_path() {
    return filesystem::path(__FILE__).parent_path() / "campaigns" / "phase16_resume_gate_smoke.txt";
}

void run_campaign_phase16_resume_gate_smoke_case(const TestOptions& options) {
    const filesystem::path configPath = phase16_resume_gate_smoke_config_path();
    const CampaignConfig config = load_campaign_config(configPath);
    const filesystem::path aggregateSummaryPath = filesystem::absolute(config.aggregateSummaryFile);
    const filesystem::path aggregateRoot = aggregateSummaryPath.parent_path().parent_path();
    const filesystem::path checkpointDir = artifact_subdir(options, "checkpoints") / "phase16_resume_gate_smoke";
    filesystem::remove_all(checkpointDir);
    filesystem::remove_all(aggregateRoot);

    TestOptions initial = options;
    initial.caseName = "campaign";
    initial.campaignConfig = filesystem::absolute(configPath).string();
    initial.checkpointDir = checkpointDir.string();
    initial.checkpointEvery = 4U;
    initial.maxPartialRuns = 1U;
    initial.stopAfterCheckpoint = true;
    run_campaign_case(initial);

    const size_t chunkCountBeforeResume = load_campaign_checkpoint_chunks(checkpointDir).size();
    require_test(chunkCountBeforeResume == 1U, "phase16 resume gate smoke expected one checkpoint chunk");

    TestOptions resume = options;
    resume.caseName = "campaign";
    resume.resumeFrom = (checkpointDir / "latest.chk").string();
    run_campaign_case(resume);

    const size_t chunkCountAfterResume = load_campaign_checkpoint_chunks(checkpointDir).size();
    require_test(
        chunkCountAfterResume == chunkCountBeforeResume,
        "phase16 resume gate smoke expected resume preflight to avoid an extra checkpoint chunk"
    );

    const unordered_map<string, string> values = read_key_value_file(aggregateSummaryPath);
    require_test(
        require_kv_value(values, "stop_reason", aggregateSummaryPath) == "max_partial_runs",
        "phase16 resume gate smoke expected max_partial_runs on resume preflight"
    );

    TestOptions gateOptions = options;
    gateOptions.stopWhenGatePasses = true;
    gateOptions.targetComparedStates = 32U;
    gateOptions.targetEligibleStates = 32U;
    CampaignRunConfig gateRun;
    gateRun.mode = "split_tie_organic_symmetric";
    FuzzStats gateStats;
    gateStats.scenarioFamily = "split_tie_organic_symmetric";
    gateStats.splitChoiceCompareStateCount = 32U;
    gateStats.compareEligibleStateCount = 32U;
    gateStats.compareCompletedStateCount = 32U;
    const optional<CampaignStopReason> gateStopReason =
        campaign_checkpoint_stop_reason(gateOptions, 1U, {{gateRun, gateStats}});
    require_test(
        gateStopReason.has_value() && *gateStopReason == CampaignStopReason::GATE_PASSED,
        "phase16 resume gate smoke expected gate preflight stop reason"
    );
}

void run_campaign_phase16_manifest_compatibility_smoke_case(const TestOptions& options) {
    const filesystem::path configPath = phase16_resume_gate_smoke_config_path();
    const CampaignConfig config = load_campaign_config(configPath);
    const filesystem::path aggregateSummaryPath = filesystem::absolute(config.aggregateSummaryFile);
    const filesystem::path aggregateRoot = aggregateSummaryPath.parent_path().parent_path();
    const filesystem::path checkpointDir =
        artifact_subdir(options, "checkpoints") / "phase16_manifest_compatibility_smoke";
    filesystem::remove_all(checkpointDir);
    filesystem::remove_all(aggregateRoot);

    TestOptions seedRun = options;
    seedRun.caseName = "campaign";
    seedRun.campaignConfig = filesystem::absolute(configPath).string();
    seedRun.checkpointDir = checkpointDir.string();
    seedRun.checkpointEvery = 4U;
    seedRun.targetComparedStates = 4U;
    seedRun.targetEligibleStates = 4U;
    seedRun.targetLineageSamples = 4U;
    seedRun.maxPartialRuns = 3U;
    seedRun.stopWhenGatePasses = true;
    seedRun.targetApplicabilityConfidence = 0.95;
    seedRun.stopAfterCheckpoint = true;
    run_campaign_case(seedRun);

    const auto expect_manifest_mismatch =
        [&](const string& expectedField, const function<void(TestOptions&)>& mutate) {
            TestOptions mismatch = seedRun;
            mismatch.stopAfterCheckpoint = false;
            mutate(mismatch);
            try {
                run_campaign_case(mismatch);
                fail_test("phase16 manifest compatibility smoke expected mismatch for " + expectedField);
            } catch (const runtime_error& ex) {
                require_test(
                    string(ex.what()).find("field=" + expectedField) != string::npos,
                    "phase16 manifest compatibility smoke expected mismatch field " + expectedField
                );
            }
        };

    expect_manifest_mismatch("target_compared_states", [](TestOptions& testOptions) {
        testOptions.targetComparedStates = 5U;
    });
    expect_manifest_mismatch("target_eligible_states", [](TestOptions& testOptions) {
        testOptions.targetEligibleStates = 5U;
    });
    expect_manifest_mismatch("target_lineage_samples", [](TestOptions& testOptions) {
        testOptions.targetLineageSamples = 5U;
    });
    expect_manifest_mismatch("max_partial_runs", [](TestOptions& testOptions) {
        testOptions.maxPartialRuns = 2U;
    });
    expect_manifest_mismatch("stop_when_gate_passes", [](TestOptions& testOptions) {
        testOptions.stopWhenGatePasses = false;
    });
    expect_manifest_mismatch("target_applicability_confidence", [](TestOptions& testOptions) {
        testOptions.targetApplicabilityConfidence = 0.90;
    });

    const filesystem::path manifestPath = checkpoint_manifest_path(checkpointDir);
    const string versionPrefix = "version=" + string(kCampaignCheckpointManifestVersion);
    string manifestText = slurp_text_file(manifestPath);
    const size_t versionPos = manifestText.find(versionPrefix);
    require_test(
        versionPos != string::npos,
        "phase16 manifest compatibility smoke expected checkpoint manifest version"
    );
    manifestText.replace(versionPos, versionPrefix.size(), "version=phase16");
    ofstream manifestOfs(manifestPath);
    require_test(
        static_cast<bool>(manifestOfs),
        "phase16 manifest compatibility smoke expected writable checkpoint manifest"
    );
    manifestOfs << manifestText;
    manifestOfs.close();

    TestOptions versionResume = options;
    versionResume.caseName = "campaign";
    versionResume.resumeFrom = (checkpointDir / "latest.chk").string();
    try {
        run_campaign_case(versionResume);
        fail_test("phase16 manifest compatibility smoke expected version mismatch");
    } catch (const runtime_error& ex) {
        require_test(
            string(ex.what()).find("unsupported checkpoint manifest version") != string::npos,
            "phase16 manifest compatibility smoke expected manifest version rejection"
        );
    }
}

void run_split_choice_policy_graduation_smoke_case(const TestOptions& options) {
    FuzzStats passStats;
    passStats.scenarioFamily = "planner_tie_mixed_organic_compare_ready";
    passStats.splitChoiceCompareStateCount = 32U;
    passStats.compareEligibleStateCount = 32U;
    passStats.compareCompletedStateCount = 32U;
    PolicyGraduationEvaluation gate = evaluate_policy_graduation_gate(options, passStats);
    require_test(gate.status == PolicyGraduationGateStatus::PASS, "policy graduation smoke expected PASS");

    FuzzStats nonApplicableStats;
    nonApplicableStats.scenarioFamily = "planner_tie_mixed_organic";
    nonApplicableStats.compareIneligibleStateCount = 48U;
    nonApplicableStats.splitReadyStateCount = 0U;
    nonApplicableStats.actualJoinHits = 48U;
    nonApplicableStats.actualIntegrateHits = 48U;
    nonApplicableStats.compareIneligibleReasonHistogram["no_split_ready"] = 48U;
    gate = evaluate_policy_graduation_gate(options, nonApplicableStats);
    require_test(
        gate.status == PolicyGraduationGateStatus::NON_APPLICABLE,
        "policy graduation smoke expected NON_APPLICABLE"
    );

    passStats.semanticShiftCount = 1U;
    gate = evaluate_policy_graduation_gate(options, passStats);
    require_test(gate.status == PolicyGraduationGateStatus::FAIL, "policy graduation smoke expected FAIL");
}

void run_compare_evidence_threshold_smoke_case(const TestOptions& options) {
    TestOptions thresholdOptions = options;
    thresholdOptions.targetComparedStates = 32U;
    thresholdOptions.targetEligibleStates = 32U;

    FuzzStats insufficient;
    insufficient.scenarioFamily = "split_tie_organic_symmetric";
    insufficient.splitChoiceCompareStateCount = 31U;
    insufficient.compareEligibleStateCount = 32U;
    insufficient.compareCompletedStateCount = 32U;
    PolicyGraduationEvaluation gate = evaluate_policy_graduation_gate(thresholdOptions, insufficient);
    require_test(
        gate.status == PolicyGraduationGateStatus::FAIL,
        "compare evidence threshold smoke expected FAIL for insufficient evidence"
    );

    insufficient.splitChoiceCompareStateCount = 32U;
    gate = evaluate_policy_graduation_gate(thresholdOptions, insufficient);
    require_test(gate.status == PolicyGraduationGateStatus::PASS, "compare evidence threshold smoke expected PASS");
}

void run_policy_graduation_gate_smoke_case(const TestOptions& options) {
    run_split_choice_policy_graduation_smoke_case(options);

    CompareReadyLineageSummary lineage;
    lineage.lineageSampleCount = 4U;
    lineage.sameSemanticClassCount = 4U;
    lineage.sameFinalStateCount = 4U;
    lineage.sameTraceClassCount = 4U;
    lineage.followupPatternPreservedCount = 4U;
    lineage.structureOnlyEnrichmentCount = 4U;

    FamilyApplicabilitySummary applicability;
    applicability.family = "planner_tie_mixed_organic";
    applicability.classification = FamilyApplicabilityClassification::UNDER_GENERATED;

    FuzzStats proxyStats;
    proxyStats.scenarioFamily = "planner_tie_mixed_organic_compare_ready";
    proxyStats.splitChoiceCompareStateCount = 32U;
    proxyStats.compareEligibleStateCount = 32U;
    proxyStats.compareCompletedStateCount = 32U;

    TestOptions proxyOptions = options;
    proxyOptions.targetLineageSamples = 4U;
    PolicyGraduationEvaluation proxyGate =
        evaluate_proxy_policy_graduation_gate(proxyOptions, applicability, lineage, proxyStats);
    require_test(
        proxyGate.status == PolicyGraduationGateStatus::PROXY_PASS,
        "policy graduation gate smoke expected PROXY_PASS"
    );
}

void run_corpus_roundtrip_smoke_case(const TestOptions& options) {
    const filesystem::path corpusDir = artifact_subdir(options, "corpus") / "roundtrip";
    filesystem::remove_all(corpusDir);

    TestOptions saveOptions = options;
    saveOptions.caseName = "planner_oracle_fuzz";
    saveOptions.scenarioFamily = ScenarioFamily::PLANNER_MIXED_TARGETED;
    saveOptions.saveCorpusDir = corpusDir.string();
    saveOptions.stats = true;
    saveOptions.iters = 4;
    saveOptions.corpusPolicy = CorpusPolicy::BEST;
    (void)run_planner_oracle_fuzz_entry(saveOptions, "corpus_roundtrip_smoke", {943101U, 943102U}, 4);

    const vector<CorpusEntry> entries = load_corpus_entries(corpusDir);
    if (entries.empty()) {
        throw runtime_error("corpus_roundtrip_smoke did not persist corpus entries");
    }

    TestOptions loadOptions = options;
    loadOptions.loadCorpusDir = corpusDir.string();
    loadOptions.scenarioFamily = saveOptions.scenarioFamily;
    const vector<u32> loaded = resolve_seed_list(loadOptions, {999999U});
    if (loaded.empty() || (loaded.size() == 1U && loaded.front() == 999999U)) {
        throw runtime_error("corpus_roundtrip_smoke failed to reload seeds");
    }
}

void run_corpus_replay_smoke_case(const TestOptions& options) {
    const filesystem::path corpusDir = artifact_subdir(options, "corpus") / "replay";
    filesystem::remove_all(corpusDir);

    TestOptions saveOptions = options;
    saveOptions.caseName = "planner_oracle_fuzz";
    saveOptions.scenarioFamily = ScenarioFamily::PLANNER_MIXED_STRUCTURAL;
    saveOptions.saveCorpusDir = corpusDir.string();
    saveOptions.stats = true;
    saveOptions.iters = 4;
    saveOptions.corpusPolicy = CorpusPolicy::BEST;
    (void)run_planner_oracle_fuzz_entry(saveOptions, "corpus_replay_seed_save", {943201U, 943202U}, 4);

    const vector<CorpusEntry> entries = load_corpus_entries(corpusDir);
    if (entries.empty()) {
        throw runtime_error("corpus_replay_smoke missing saved corpus");
    }

    TestOptions replayOptions = options;
    replayOptions.caseName = "planner_oracle_fuzz";
    replayOptions.loadCorpusDir = corpusDir.string();
    replayOptions.saveCorpusDir.reset();
    replayOptions.stats = true;
    replayOptions.iters = 3;
    replayOptions.scenarioFamily = saveOptions.scenarioFamily;
    const FuzzStats replayStats =
        run_planner_oracle_fuzz_entry(replayOptions, "corpus_replay_smoke", {1U}, replayOptions.iters.value_or(3));

    unordered_set<u32> expectedSeeds;
    for (const CorpusEntry& entry : entries) {
        if (entry.modeKey == active_corpus_mode_key(replayOptions)) {
            expectedSeeds.insert(entry.seed);
        }
    }
    unordered_set<u32> replayedSeeds;
    for (const SeedFuzzSummary& seedSummary : replayStats.seedSummaries) {
        replayedSeeds.insert(seedSummary.seed);
    }
    if (replayedSeeds.empty() || replayedSeeds != expectedSeeds || replayedSeeds.count(1U) != 0U) {
        throw runtime_error("corpus_replay_smoke did not replay the saved corpus seed set");
    }
}

void run_micro_suite(const TestOptions& options) {
    set_failure_context("isolate_micro", nullopt, -1);
    maybe_log(options, "isolate_micro", nullopt, -1);
    test_isolate_microcase();
    set_failure_context("isolate_multiedge_micro", nullopt, -1);
    maybe_log(options, "isolate_multiedge_micro", nullopt, -1);
    test_isolate_multiedge_microcase();
    set_failure_context("split_micro", nullopt, -1);
    maybe_log(options, "split_micro", nullopt, -1);
    test_split_microcase();
    set_failure_context("join_micro", nullopt, -1);
    maybe_log(options, "join_micro", nullopt, -1);
    test_join_microcase();
    set_failure_context("integrate_micro", nullopt, -1);
    maybe_log(options, "integrate_micro", nullopt, -1);
    test_integrate_microcase();
    set_failure_context("planner_micro", nullopt, -1);
    maybe_log(options, "planner_micro", nullopt, -1);
    test_planner_microcase(options);
}

void run_regression_44001(const TestOptions& options) {
    set_failure_context("regression_44001", 44001U, 0);
    maybe_log(options, "regression_44001", 44001U, 0);

    mt19937 rng(44001);
    {
        RawEngine RE;
        RawUpdateCtx U;
        (void)make_random_split_case(RE, U, rng);
    }

    RawEngine RE;
    RawUpdateCtx U;
    GeneratedCase gc = make_random_split_case(RE, U, rng);
    const OccID target = gc.occs[0];

    const SplitSeparationPairResult sp = split_checked(RE, gc.sid, 2, 8, U, "regression_44001_split");
    RawSkelID anchor = NIL_U32;
    vector<RawSkelID> boundaryOnly;
    for (const SplitChildInfo& child : sp.child) {
        if (child_contains_occ(child, target)) {
            anchor = child.sid;
        } else if (child.boundaryOnly) {
            boundaryOnly.push_back(child.sid);
        }
    }
    require_test(anchor != NIL_U32, "regression_44001 missing anchor split child");

    for (RawSkelID sidAB : boundaryOnly) {
        const IntegrateResult ir = integrate_checked(
            RE,
            anchor,
            sidAB,
            identity_boundary_map(),
            U,
            "regression_44001_integrate"
        );
        if (ir.mergedSid != anchor) {
            fail_test("regression_44001 integrate mergedSid mismatch");
        }
    }

    RawPlannerCtx ctx;
    ctx.targetOcc = target;
    ctx.keepOcc = {target};
    const RawUpdaterRunOptions runOptions{options.stepBudget};
    run_planner_checked(RE, ctx, U, runOptions, "regression_44001_planner");

    const RawSkelID hostSid = RE.occ.get(target).hostSkel;
    if (RE.skel.get(hostSid).hostedOcc.size() != 1U || RE.skel.get(hostSid).hostedOcc[0] != target) {
        fail_test("regression_44001 planner stop condition hosted occurrence mismatch");
    }
    require_test(!discover_split_pair_from_support(RE, target).has_value(), "regression_44001 split pair still available");
}

void run_regression_isolate_split_no_sep(const TestOptions& options) {
    set_failure_context("regression_isolate_split_no_sep", 3736675150U, 0);
    maybe_log(options, "regression_isolate_split_no_sep", 3736675150U, 0);

    FuzzSpec spec;
    spec.mode = FuzzMode::ISOLATE_THEN_SPLIT;
    spec.seed = 3736675150U;
    spec.branchCount = 3;
    spec.occCount = 3;
    spec.boundaryOnlyCount = 1;
    spec.maxPathLen = 1;
    spec.maxOccPerBranch = 2;
    spec.directABCount = 0;
    spec.multiEdgeCount = 0;
    spec.sharedOrigPairs = 0;
    spec.keepOccCount = 1;
    spec.opCount = 1;
    spec.stepBudget = options.stepBudget;
    run_fuzz_spec(spec);
}

void run_artifact_retention_smoke_case(const TestOptions& options) {
    TestOptions retentionOptions = options;
    retentionOptions.maxArtifacts = 4U;

    const filesystem::path root = resolve_artifact_dir(retentionOptions);
    const array<string, 6> leaves = {{"traces", "logs", "counterexamples", "corpus", "reduced", "reduced_planner"}};
    for (const string& leaf : leaves) {
        filesystem::remove_all(root / leaf);
        filesystem::create_directories(root / leaf);
    }

    const array<pair<string, string>, 7> files = {{
        {"traces", "001_old_trace.log"},
        {"logs", "002_old_log.log"},
        {"counterexamples", "003_old_counterexample.txt"},
        {"corpus", "004_old_corpus.txt"},
        {"traces", "005_mid_trace.log"},
        {"reduced", "006_keep_reduced.txt"},
        {"reduced_planner", "007_keep_reduced_planner.txt"},
    }};
    for (size_t i = 0; i < files.size(); ++i) {
        const filesystem::path path = root / files[i].first / files[i].second;
        ofstream ofs(path);
        ofs << "artifact " << i << '\n';
        filesystem::last_write_time(path, filesystem::file_time_type::clock::now() + chrono::seconds(static_cast<int>(i)));
    }

    enforce_artifact_retention(retentionOptions);

    size_t fileCount = 0U;
    for (const string& leaf : leaves) {
        for (const auto& entry : filesystem::directory_iterator(root / leaf)) {
            fileCount += entry.is_regular_file() ? 1U : 0U;
        }
    }
    if (fileCount > retentionOptions.maxArtifacts) {
        throw runtime_error("artifact_retention_smoke exceeded maxArtifacts");
    }
    if (!filesystem::exists(root / "reduced" / "006_keep_reduced.txt") ||
        !filesystem::exists(root / "reduced_planner" / "007_keep_reduced_planner.txt")) {
        throw runtime_error("artifact_retention_smoke pruned reduced artifacts first");
    }
}

void run_named_case(const TestOptions& options) {
    if (options.caseName == "all") {
        run_micro_suite(options);
        run_seeded_case(options, "roundtrip", {123456U, 123457U, 700001U}, 160, [&](mt19937& rng, int) {
            run_roundtrip_iteration(rng);
        });
        run_seeded_case(options, "split_integrate_ensure", {44001U, 44002U, 44003U, 44004U}, 120, [&](mt19937& rng, int) {
            run_split_integrate_ensure_iteration(rng, options);
        });
        run_regression_44001(options);
        run_regression_isolate_split_no_sep(options);
        run_seeded_case(options, "planner", {987654U, 987655U, 987656U}, 180, [&](mt19937& rng, int iter) {
            run_planner_iteration(rng, iter, options);
        });
        run_fuzz_mode_case(options, "isolate_fuzz", FuzzMode::ISOLATE_ONLY, {710001U, 710002U}, 80);
        run_fuzz_mode_case(options, "split_fuzz", FuzzMode::SPLIT_ONLY, {711001U, 711002U}, 80);
        run_fuzz_mode_case(options, "join_fuzz", FuzzMode::JOIN_ONLY, {712001U, 712002U}, 80);
        run_fuzz_mode_case(options, "integrate_fuzz", FuzzMode::INTEGRATE_ONLY, {713001U, 713002U}, 80);
        run_fuzz_mode_case(options, "isolate_split_fuzz", FuzzMode::ISOLATE_THEN_SPLIT, {714001U, 714002U}, 80);
        run_fuzz_mode_case(options, "split_join_fuzz", FuzzMode::SPLIT_THEN_JOIN, {715001U, 715002U}, 80);
        run_fuzz_mode_case(options, "split_integrate_fuzz", FuzzMode::SPLIT_THEN_INTEGRATE, {716001U, 716002U}, 80);
        run_fuzz_mode_case(options, "planner_mixed_fuzz", FuzzMode::MIXED_PLANNER, {717001U, 717002U}, 80);
        run_seeded_case(options, "fuzz", {424242U, 424243U, 424244U, 424245U}, 120, [&](mt19937& rng, int) {
            run_fuzz_iteration(rng, options);
        });
        return;
    }

    if (options.reduce && options.caseName == "replay_state") {
        run_reduce_state_case(options);
        return;
    }

    if (options.caseName == "micro") {
        run_micro_suite(options);
        return;
    }
    if (options.caseName == "isolate") {
        test_isolate_microcase();
        test_isolate_multiedge_microcase();
        return;
    }
    if (options.caseName == "split") {
        test_split_microcase();
        return;
    }
    if (options.caseName == "join") {
        test_join_microcase();
        return;
    }
    if (options.caseName == "integrate") {
        test_integrate_microcase();
        return;
    }
    if (options.caseName == "planner_micro") {
        test_planner_microcase(options);
        return;
    }
    if (options.caseName == "roundtrip") {
        run_seeded_case(options, "roundtrip", {123456U, 123457U, 700001U}, 160, [&](mt19937& rng, int) {
            run_roundtrip_iteration(rng);
        });
        return;
    }
    if (options.caseName == "split_integrate_ensure") {
        run_seeded_case(options, "split_integrate_ensure", {44001U, 44002U, 44003U, 44004U}, 120, [&](mt19937& rng, int) {
            run_split_integrate_ensure_iteration(rng, options);
        });
        return;
    }
    if (options.caseName == "planner") {
        run_seeded_case(options, "planner", {987654U, 987655U, 987656U}, 180, [&](mt19937& rng, int iter) {
            run_planner_iteration(rng, iter, options);
        });
        return;
    }
    if (options.caseName == "isolate_fuzz") {
        run_fuzz_mode_case(options, "isolate_fuzz", FuzzMode::ISOLATE_ONLY, {710001U, 710002U, 710003U}, 120);
        return;
    }
    if (options.caseName == "split_fuzz") {
        run_fuzz_mode_case(options, "split_fuzz", FuzzMode::SPLIT_ONLY, {711001U, 711002U, 711003U}, 120);
        return;
    }
    if (options.caseName == "join_fuzz") {
        run_fuzz_mode_case(options, "join_fuzz", FuzzMode::JOIN_ONLY, {712001U, 712002U, 712003U}, 120);
        return;
    }
    if (options.caseName == "integrate_fuzz") {
        run_fuzz_mode_case(options, "integrate_fuzz", FuzzMode::INTEGRATE_ONLY, {713001U, 713002U, 713003U}, 120);
        return;
    }
    if (options.caseName == "isolate_split_fuzz") {
        run_fuzz_mode_case(options, "isolate_split_fuzz", FuzzMode::ISOLATE_THEN_SPLIT, {714001U, 714002U, 714003U}, 120);
        return;
    }
    if (options.caseName == "split_join_fuzz") {
        run_fuzz_mode_case(options, "split_join_fuzz", FuzzMode::SPLIT_THEN_JOIN, {715001U, 715002U, 715003U}, 120);
        return;
    }
    if (options.caseName == "split_integrate_fuzz") {
        run_fuzz_mode_case(options, "split_integrate_fuzz", FuzzMode::SPLIT_THEN_INTEGRATE, {716001U, 716002U, 716003U}, 120);
        return;
    }
    if (options.caseName == "planner_mixed_fuzz") {
        run_fuzz_mode_case(options, "planner_mixed_fuzz", FuzzMode::MIXED_PLANNER, {717001U, 717002U, 717003U}, 120);
        return;
    }
    if (options.caseName == "fuzz_matrix") {
        run_fuzz_mode_case(options, "isolate_fuzz", FuzzMode::ISOLATE_ONLY, {710001U, 710002U}, 80);
        run_fuzz_mode_case(options, "split_fuzz", FuzzMode::SPLIT_ONLY, {711001U, 711002U}, 80);
        run_fuzz_mode_case(options, "join_fuzz", FuzzMode::JOIN_ONLY, {712001U, 712002U}, 80);
        run_fuzz_mode_case(options, "integrate_fuzz", FuzzMode::INTEGRATE_ONLY, {713001U, 713002U}, 80);
        run_fuzz_mode_case(options, "isolate_split_fuzz", FuzzMode::ISOLATE_THEN_SPLIT, {714001U, 714002U}, 80);
        run_fuzz_mode_case(options, "split_join_fuzz", FuzzMode::SPLIT_THEN_JOIN, {715001U, 715002U}, 80);
        run_fuzz_mode_case(options, "split_integrate_fuzz", FuzzMode::SPLIT_THEN_INTEGRATE, {716001U, 716002U}, 80);
        run_fuzz_mode_case(options, "planner_mixed_fuzz", FuzzMode::MIXED_PLANNER, {717001U, 717002U}, 80);
        return;
    }
    if (options.caseName == "fuzz") {
        run_seeded_case(options, "fuzz", {424242U, 424243U, 424244U, 424245U}, 120, [&](mt19937& rng, int) {
            run_fuzz_iteration(rng, options);
        });
        return;
    }
    if (options.caseName == "isolate_oracle") {
        TestOptions oracleOptions = options;
        oracleOptions.oracleMode = OracleMode::PRIMITIVE;
        set_active_test_options(&oracleOptions);
        run_fuzz_mode_case(oracleOptions, "isolate_oracle", FuzzMode::ISOLATE_ONLY, {810001U, 810002U, 810003U}, 120);
        set_active_test_options(&options);
        return;
    }
    if (options.caseName == "split_oracle") {
        TestOptions oracleOptions = options;
        oracleOptions.oracleMode = OracleMode::PRIMITIVE;
        set_active_test_options(&oracleOptions);
        run_fuzz_mode_case(oracleOptions, "split_oracle", FuzzMode::SPLIT_ONLY, {811001U, 811002U, 811003U}, 120);
        set_active_test_options(&options);
        return;
    }
    if (options.caseName == "join_oracle") {
        TestOptions oracleOptions = options;
        oracleOptions.oracleMode = OracleMode::PRIMITIVE;
        set_active_test_options(&oracleOptions);
        run_fuzz_mode_case(oracleOptions, "join_oracle", FuzzMode::JOIN_ONLY, {812001U, 812002U, 812003U}, 120);
        set_active_test_options(&options);
        return;
    }
    if (options.caseName == "integrate_oracle") {
        TestOptions oracleOptions = options;
        oracleOptions.oracleMode = OracleMode::PRIMITIVE;
        set_active_test_options(&oracleOptions);
        run_fuzz_mode_case(oracleOptions, "integrate_oracle", FuzzMode::INTEGRATE_ONLY, {813001U, 813002U, 813003U}, 120);
        set_active_test_options(&options);
        return;
    }
    if (options.caseName == "planner_oracle") {
        TestOptions oracleOptions = options;
        oracleOptions.oracleMode = OracleMode::PLANNER;
        set_active_test_options(&oracleOptions);
        run_seeded_case(
            oracleOptions,
            "planner_oracle",
            {820001U, 820002U, 820003U},
            120,
            [&](mt19937& rng, int iter) { run_planner_iteration(rng, iter, oracleOptions); }
        );
        set_active_test_options(&options);
        return;
    }
    if (options.caseName == "planner_oracle_fuzz") {
        (void)run_planner_oracle_fuzz_entry(options, "planner_oracle_fuzz", {821001U, 821002U, 821003U}, 80);
        return;
    }
    if (options.caseName == "campaign") {
        run_campaign_case(options);
        return;
    }
    if (options.caseName == "repro") {
        run_repro_case(options);
        return;
    }
    if (options.caseName == "counterexample_search") {
        run_counterexample_search(options);
        return;
    }
    if (options.caseName == "replay_state") {
        run_replay_state_case(options);
        return;
    }
    if (options.caseName == "reduce_state") {
        run_reduce_state_case(options);
        return;
    }
    if (options.caseName == "replay_planner_state") {
        run_replay_planner_state_case(options);
        return;
    }
    if (options.caseName == "reduce_planner_state") {
        run_reduce_planner_state_case(options);
        return;
    }
    if (options.caseName == "reducer_smoke") {
        run_reducer_smoke_case(options);
        return;
    }
    if (options.caseName == "reduce_planner_state_smoke") {
        run_reduce_planner_state_smoke_case(options);
        return;
    }
    if (options.caseName == "failure_signature_smoke") {
        run_failure_signature_smoke_case(options);
        return;
    }
    if (options.caseName == "planner_failure_signature_smoke") {
        run_planner_failure_signature_smoke_case(options);
        return;
    }
    if (options.caseName == "reduce_planner_state_step_budget") {
        run_reduce_planner_state_step_budget_case(options);
        return;
    }
    if (options.caseName == "reduce_planner_state_trace_prefix") {
        run_reduce_planner_state_trace_prefix_case(options);
        return;
    }
    if (options.caseName == "planner_oracle_fuzz_stats") {
        run_planner_oracle_fuzz_stats_case(options);
        return;
    }
    if (options.caseName == "primitive_fault_detection_smoke") {
        run_primitive_fault_detection_smoke_case(options);
        return;
    }
    if (options.caseName == "planner_fault_detection_smoke") {
        run_planner_fault_detection_smoke_case(options);
        return;
    }
    if (options.caseName == "mutation_matrix_smoke") {
        run_mutation_matrix_smoke_case(options);
        return;
    }
    if (options.caseName == "planner_fixpoint_idempotence") {
        run_planner_fixpoint_idempotence_case(options);
        return;
    }
    if (options.caseName == "planner_replay_determinism") {
        run_planner_replay_determinism_case(options);
        return;
    }
    if (options.caseName == "reducer_determinism_smoke") {
        run_reducer_determinism_smoke_case(options);
        return;
    }
    if (options.caseName == "corpus_roundtrip_smoke") {
        run_corpus_roundtrip_smoke_case(options);
        return;
    }
    if (options.caseName == "corpus_replay_smoke") {
        run_corpus_replay_smoke_case(options);
        return;
    }
    if (options.caseName == "corpus_replay") {
        run_corpus_replay_case(options);
        return;
    }
    if (options.caseName == "compare_ready_lineage_audit") {
        run_compare_ready_lineage_audit_case(options);
        return;
    }
    if (options.caseName == "compare_ready_lineage_smoke") {
        run_compare_ready_lineage_smoke_case(options);
        return;
    }
    if (options.caseName == "planner_tie_organic_applicability_audit") {
        run_planner_tie_organic_applicability_audit_case(options);
        return;
    }
    if (options.caseName == "planner_tie_organic_applicability_smoke") {
        run_planner_tie_organic_applicability_smoke_case(options);
        return;
    }
    if (options.caseName == "planner_tie_organic_proxy_pass_smoke") {
        run_planner_tie_organic_proxy_pass_smoke_case(options);
        return;
    }
    if (options.caseName == "campaign_phase16_resume_gate_smoke") {
        run_campaign_phase16_resume_gate_smoke_case(options);
        return;
    }
    if (options.caseName == "campaign_phase16_manifest_compatibility_smoke") {
        run_campaign_phase16_manifest_compatibility_smoke_case(options);
        return;
    }
    if (options.caseName == "split_choice_policy_graduation_smoke") {
        run_split_choice_policy_graduation_smoke_case(options);
        return;
    }
    if (options.caseName == "policy_graduation_gate_smoke") {
        run_policy_graduation_gate_smoke_case(options);
        return;
    }
    if (options.caseName == "compare_evidence_threshold_smoke") {
        run_compare_evidence_threshold_smoke_case(options);
        return;
    }
    if (options.caseName == "exhaustive") {
        run_exhaustive_case(options);
        return;
    }
    if (options.caseName == "exhaustive_split_ready_smoke") {
        run_exhaustive_split_ready_smoke_case(options);
        return;
    }
    if (options.caseName == "exhaustive_join_ready_smoke") {
        run_exhaustive_join_ready_smoke_case(options);
        return;
    }
    if (options.caseName == "exhaustive_integrate_ready_smoke") {
        run_exhaustive_integrate_ready_smoke_case(options);
        return;
    }
    if (options.caseName == "exhaustive_mixed_smoke") {
        run_exhaustive_mixed_smoke_case(options);
        return;
    }
    if (options.caseName == "planner_tie_mixed_exhaustive_smoke") {
        run_planner_tie_mixed_exhaustive_smoke_case(options);
        return;
    }
    if (options.caseName == "exhaustive_canonical_dedupe_smoke") {
        run_exhaustive_canonical_dedupe_smoke_case(options);
        return;
    }
    if (options.caseName == "exhaustive_natural_dedupe_smoke") {
        run_exhaustive_natural_dedupe_smoke_case(options);
        return;
    }
    if (options.caseName == "exhaustive_family_sweep_smoke") {
        run_exhaustive_family_sweep_smoke_case(options);
        return;
    }
    if (options.caseName == "exhaustive_collision_guard_smoke") {
        run_exhaustive_collision_guard_smoke_case(options);
        return;
    }
    if (options.caseName == "exhaustive_natural_dedupe_large_smoke") {
        run_exhaustive_natural_dedupe_large_smoke_case(options);
        return;
    }
    if (options.caseName == "exhaustive_organic_duplicate_examples_smoke") {
        run_exhaustive_organic_duplicate_examples_smoke_case(options);
        return;
    }
    if (options.caseName == "exact_canonicalizer_smoke") {
        run_exact_canonicalizer_smoke_case(options);
        return;
    }
    if (options.caseName == "fast_vs_exact_canonical_dedupe_smoke") {
        run_fast_vs_exact_canonical_dedupe_smoke_case(options);
        return;
    }
    if (options.caseName == "metamorphic_relabel_invariance") {
        run_metamorphic_relabel_invariance_case(options);
        return;
    }
    if (options.caseName == "metamorphic_occid_invariance") {
        run_metamorphic_occid_invariance_case(options);
        return;
    }
    if (options.caseName == "metamorphic_edge_order_invariance") {
        run_metamorphic_edge_order_invariance_case(options);
        return;
    }
    if (options.caseName == "metamorphic_vertex_order_invariance") {
        run_metamorphic_vertex_order_invariance_case(options);
        return;
    }
    if (options.caseName == "replay_serialization_invariance") {
        run_replay_serialization_invariance_case(options);
        return;
    }
    if (options.caseName == "metamorphic_family_matrix_smoke") {
        run_metamorphic_family_matrix_smoke_case(options);
        return;
    }
    if (options.caseName == "metamorphic_planner_multistep_smoke") {
        run_metamorphic_planner_multistep_smoke_case(options);
        return;
    }
    if (options.caseName == "metamorphic_replay_matrix_smoke") {
        run_metamorphic_replay_matrix_smoke_case(options);
        return;
    }
    if (options.caseName == "split_choice_oracle_smoke") {
        run_split_choice_oracle_smoke_case(options);
        return;
    }
    if (options.caseName == "split_choice_relabel_invariance") {
        run_split_choice_relabel_invariance_case(options);
        return;
    }
    if (options.caseName == "split_choice_edge_order_invariance") {
        run_split_choice_edge_order_invariance_case(options);
        return;
    }
    if (options.caseName == "split_choice_vertex_order_invariance") {
        run_split_choice_vertex_order_invariance_case(options);
        return;
    }
    if (options.caseName == "split_choice_oracle_regression") {
        run_split_choice_oracle_regression_case(options);
        return;
    }
    if (options.caseName == "split_choice_policy_smoke") {
        run_split_choice_policy_smoke_case(options);
        return;
    }
    if (options.caseName == "split_choice_policy_relabel_invariance") {
        run_split_choice_policy_relabel_invariance_case(options);
        return;
    }
    if (options.caseName == "split_choice_policy_edge_order_invariance") {
        run_split_choice_policy_edge_order_invariance_case(options);
        return;
    }
    if (options.caseName == "split_choice_policy_vertex_order_invariance") {
        run_split_choice_policy_vertex_order_invariance_case(options);
        return;
    }
    if (options.caseName == "split_choice_policy_occid_invariance") {
        run_split_choice_policy_occid_invariance_case(options);
        return;
    }
    if (options.caseName == "split_choice_policy_multiclass_smoke") {
        run_split_choice_policy_multiclass_smoke_case(options);
        return;
    }
    if (options.caseName == "split_choice_exact_class_smoke") {
        run_split_choice_exact_class_smoke_case(options);
        return;
    }
    if (options.caseName == "split_choice_exact_relabel_invariance") {
        run_split_choice_exact_relabel_invariance_case(options);
        return;
    }
    if (options.caseName == "split_choice_exact_vertex_order_invariance") {
        run_split_choice_exact_vertex_order_invariance_case(options);
        return;
    }
    if (options.caseName == "split_choice_exact_edge_order_invariance") {
        run_split_choice_exact_edge_order_invariance_case(options);
        return;
    }
    if (options.caseName == "split_choice_representative_shift_smoke") {
        run_split_choice_representative_shift_smoke_case(options);
        return;
    }
    if (options.caseName == "split_choice_harmless_shift_smoke") {
        run_split_choice_harmless_shift_smoke_case(options);
        return;
    }
    if (options.caseName == "split_choice_semantic_shift_smoke") {
        run_split_choice_semantic_shift_smoke_case(options);
        return;
    }
    if (options.caseName == "split_choice_semantic_shift_regression") {
        run_split_choice_semantic_shift_regression_case(options);
        return;
    }
    if (options.caseName == "split_choice_policy_exact_shadow_smoke") {
        run_split_choice_policy_exact_shadow_smoke_case(options);
        return;
    }
    if (options.caseName == "split_choice_policy_fast_vs_exact_shadow_compare") {
        run_split_choice_policy_fast_vs_exact_shadow_compare_case(options);
        return;
    }
    if (options.caseName == "split_choice_exact_shadow_relabel_invariance") {
        run_split_choice_exact_shadow_relabel_invariance_case(options);
        return;
    }
    if (options.caseName == "split_choice_exact_shadow_vertex_order_invariance") {
        run_split_choice_exact_shadow_vertex_order_invariance_case(options);
        return;
    }
    if (options.caseName == "split_choice_exact_shadow_edge_order_invariance") {
        run_split_choice_exact_shadow_edge_order_invariance_case(options);
        return;
    }
    if (options.caseName == "split_choice_fallback_zero_smoke") {
        run_split_choice_fallback_zero_smoke_case(options);
        return;
    }
    if (options.caseName == "planner_relabel_structural_regression") {
        run_planner_relabel_structural_regression_case(options);
        return;
    }
    if (options.caseName == "planner_targeted_split_smoke") {
        run_planner_targeted_split_smoke_case(options);
        return;
    }
    if (options.caseName == "planner_targeted_join_smoke") {
        run_planner_targeted_join_smoke_case(options);
        return;
    }
    if (options.caseName == "planner_targeted_integrate_smoke") {
        run_planner_targeted_integrate_smoke_case(options);
        return;
    }
    if (options.caseName == "planner_targeted_mixed_smoke") {
        run_planner_targeted_mixed_smoke_case(options);
        return;
    }
    if (options.caseName == "split_ready_unique_orig_regression") {
        run_split_ready_unique_orig_regression_case(options);
        return;
    }
    if (options.caseName == "planner_tie_mixed_smoke") {
        run_planner_tie_mixed_smoke_case(options);
        return;
    }
    if (options.caseName == "planner_tie_mixed_exact_shadow_smoke") {
        run_planner_tie_mixed_exact_shadow_smoke_case(options);
        return;
    }
    if (options.caseName == "planner_tie_symmetric_smoke") {
        run_planner_tie_symmetric_smoke_case(options);
        return;
    }
    if (options.caseName == "canonical_collision_probe_smoke") {
        run_canonical_collision_probe_smoke_case(options);
        return;
    }
    if (options.caseName == "split_tie_organic_symmetric_smoke") {
        run_split_tie_organic_symmetric_smoke_case(options);
        return;
    }
    if (options.caseName == "planner_tie_mixed_organic_smoke") {
        run_planner_tie_mixed_organic_smoke_case(options);
        return;
    }
    if (options.caseName == "planner_tie_mixed_organic_compare_ready_smoke") {
        run_planner_tie_mixed_organic_compare_ready_smoke_case(options);
        return;
    }
    if (options.caseName == "automorphism_probe_large_smoke") {
        run_automorphism_probe_large_smoke_case(options);
        return;
    }
    if (options.caseName == "sampled_exact_audit_smoke") {
        run_sampled_exact_audit_smoke_case(options);
        return;
    }
    if (options.caseName == "duplicate_attribution_smoke") {
        run_duplicate_attribution_smoke_case(options);
        return;
    }
    if (options.caseName == "planner_coverage_smoke") {
        run_planner_coverage_smoke_case(options);
        return;
    }
    if (options.caseName == "planner_random_coverage_smoke") {
        run_planner_random_coverage_smoke_case(options);
        return;
    }
    if (options.caseName == "planner_weighted_coverage_smoke") {
        run_planner_weighted_coverage_smoke_case(options);
        return;
    }
    if (options.caseName == "planner_join_ready_smoke") {
        run_planner_join_ready_smoke_case(options);
        return;
    }
    if (options.caseName == "planner_integrate_ready_smoke") {
        run_planner_integrate_ready_smoke_case(options);
        return;
    }
    if (options.caseName == "planner_structural_mixed_smoke") {
        run_planner_structural_mixed_smoke_case(options);
        return;
    }
    if (options.caseName == "artifact_retention_smoke") {
        run_artifact_retention_smoke_case(options);
        return;
    }
    if (options.caseName == "regression_44001") {
        run_regression_44001(options);
        return;
    }
    if (options.caseName == "regression_isolate_split_no_sep") {
        run_regression_isolate_split_no_sep(options);
        return;
    }

    throw runtime_error("unknown test case: " + options.caseName);
}

} // namespace

ExhaustiveScenario make_targeted_planner_exhaustive_scenario(ScenarioFamily family, u32 seed) {
    PlannerTargetedScenario source = make_targeted_planner_scenario(family, seed);

    ExhaustiveScenario scenario;
    scenario.RE = source.RE;
    scenario.ctx = source.ctx;
    scenario.U = source.U;
    scenario.initialQueue = source.initialQueue;
    scenario.family = ExhaustiveFamily::MIXED;
    scenario.label = scenario_family_name_string(family) + string("_seed") + to_string(seed);

    if (!scenario.initialQueue.empty()) {
        const UpdJob& job = scenario.initialQueue.front();
        if (job.kind == UpdJobKind::JOIN_PAIR) {
            scenario.primitivePlan.primitive = PrimitiveKind::JOIN;
            scenario.primitivePlan.leftSid = job.leftSid;
            scenario.primitivePlan.rightSid = job.rightSid;
            scenario.primitivePlan.aOrig = job.aOrig;
            scenario.primitivePlan.bOrig = job.bOrig;
            return scenario;
        }
        if (job.kind == UpdJobKind::INTEGRATE_CHILD) {
            scenario.primitivePlan.primitive = PrimitiveKind::INTEGRATE;
            scenario.primitivePlan.parentSid = job.parentSid;
            scenario.primitivePlan.childSid = job.childSid;
            scenario.primitivePlan.aOrig = job.aOrig;
            scenario.primitivePlan.bOrig = job.bOrig;
            scenario.primitivePlan.boundaryMap = job.bm;
            return scenario;
        }
    }

    if (const optional<pair<Vertex, Vertex>> splitPair =
            discover_split_pair_from_support(scenario.RE, scenario.ctx.targetOcc);
        splitPair.has_value()) {
        scenario.primitivePlan.primitive = PrimitiveKind::SPLIT;
        scenario.primitivePlan.sid = scenario.RE.occ.get(scenario.ctx.targetOcc).hostSkel;
        scenario.primitivePlan.aOrig = splitPair->first;
        scenario.primitivePlan.bOrig = splitPair->second;
    }

    return scenario;
}

FailureContextSnapshot current_failure_context() {
    FailureContextSnapshot snapshot;
    snapshot.caseName = g_failureContext.caseName;
    snapshot.seed = g_failureContext.seed;
    snapshot.iter = g_failureContext.iter;
    return snapshot;
}

string current_failure_stem() {
    return make_failure_stem(g_failureContext);
}

void install_failure_handlers() {
    std::set_terminate(raw_engine_terminate_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGILL, signal_handler);
    signal(SIGFPE, signal_handler);
}

int run_test_suite(const TestOptions& options) {
    set_active_test_options(&options);
    set_pending_dump_path(nullopt);
    for (int rep = 0; rep < options.repeat; ++rep) {
        set_failure_context(options.caseName, options.seed, -1);
        maybe_log(options, options.caseName, options.seed, rep);
        run_named_case(options);
    }
    set_active_test_options(nullptr);

    if (options.caseName != "repro") {
        cout << "raw_engine_v1 tests passed case=" << options.caseName << '\n';
    }
    return 0;
}
