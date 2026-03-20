#include "test_harness.hpp"

#include <array>
#include <cctype>
#include <csignal>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "state_dump.hpp"

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
    int firstSplitIter = -1;
    int firstJoinIter = -1;
    int firstIntegrateIter = -1;
    size_t multiEdgeCount = 0;
    size_t reducerInvocationCount = 0;
    unordered_map<string, size_t> tracePrefixHistogram;
    unordered_map<string, size_t> primitiveMultisetHistogram;
};

struct FuzzStats {
    string caseName;
    string weightProfile;
    string preconditionBiasProfile;
    string scenarioFamily;
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
    int firstSplitIter = -1;
    int firstJoinIter = -1;
    int firstIntegrateIter = -1;
    size_t multiEdgeCount = 0;
    size_t reducerInvocationCount = 0;
    unordered_map<string, size_t> sequenceHistogram;
    unordered_map<string, size_t> tracePrefixHistogram;
    unordered_map<string, size_t> primitiveMultisetHistogram;
    unordered_map<string, size_t> oracleMismatchCount;
    vector<SeedFuzzSummary> seedSummaries;
};

struct IterationOrdinal {
    size_t global = 0;
    size_t seed = 0;
};

FailureContext g_failureContext;
optional<FuzzStats> g_fuzzStats;
optional<size_t> g_activeSeedSummary;

string sanitize_stem_token(const string& text) {
    string out;
    out.reserve(text.size());
    for (unsigned char ch : text) {
        if (isalnum(ch) || ch == '_' || ch == '-') {
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
    const PreconditionBiasConfig bias = resolve_precondition_bias(options);
    stats.biasSplit = bias.split;
    stats.biasJoin = bias.join;
    stats.biasIntegrate = bias.integrate;
    g_fuzzStats = stats;
    g_activeSeedSummary.reset();
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
    note_first_hit(g_fuzzStats->firstSplitIter, ordinal.global, result.coverage.actualSplitHits);
    note_first_hit(g_fuzzStats->firstJoinIter, ordinal.global, result.coverage.actualJoinHits);
    note_first_hit(g_fuzzStats->firstIntegrateIter, ordinal.global, result.coverage.actualIntegrateHits);

    if (SeedFuzzSummary* seedSummary = active_seed_summary(); seedSummary != nullptr) {
        seedSummary->splitReadyCount += result.coverage.splitReadyCount;
        seedSummary->boundaryOnlyChildCount += result.coverage.boundaryOnlyChildCount;
        seedSummary->joinCandidateCount += result.coverage.joinCandidateCount;
        seedSummary->integrateCandidateCount += result.coverage.integrateCandidateCount;
        seedSummary->actualSplitHits += result.coverage.actualSplitHits;
        seedSummary->actualJoinHits += result.coverage.actualJoinHits;
        seedSummary->actualIntegrateHits += result.coverage.actualIntegrateHits;
        note_first_hit(seedSummary->firstSplitIter, ordinal.seed, result.coverage.actualSplitHits);
        note_first_hit(seedSummary->firstJoinIter, ordinal.seed, result.coverage.actualJoinHits);
        note_first_hit(seedSummary->firstIntegrateIter, ordinal.seed, result.coverage.actualIntegrateHits);
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
    for (unsigned char ch : text) {
        switch (ch) {
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

    const filesystem::path outPath = stats_summary_output_path(options, caseName);
    filesystem::create_directories(outPath.parent_path());

    ofstream ofs(outPath);
    if (!ofs) {
        throw runtime_error("failed to write stats summary file: " + outPath.string());
    }

    const FuzzStats& stats = *g_fuzzStats;
    ofs << "case=" << stats.caseName << '\n';
    ofs << "weight_profile=" << stats.weightProfile << '\n';
    ofs << "precondition_bias_profile=" << stats.preconditionBiasProfile << '\n';
    ofs << "scenario_family=" << stats.scenarioFamily << '\n';
    ofs << "bias_split=" << stats.biasSplit << '\n';
    ofs << "bias_join=" << stats.biasJoin << '\n';
    ofs << "bias_integrate=" << stats.biasIntegrate << '\n';
    ofs << "iterations=" << stats.totalIterations << '\n';
    ofs << "split_ready_count=" << stats.splitReadyCount << '\n';
    ofs << "join_candidate_count=" << stats.joinCandidateCount << '\n';
    ofs << "integrate_candidate_count=" << stats.integrateCandidateCount << '\n';
    ofs << "actual_split_hits=" << stats.actualSplitHits << '\n';
    ofs << "actual_join_hits=" << stats.actualJoinHits << '\n';
    ofs << "actual_integrate_hits=" << stats.actualIntegrateHits << '\n';
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
        ofs << "seed=" << seedSummary.seed
            << " iterations=" << seedSummary.iterations
            << " split_ready=" << seedSummary.splitReadyCount
            << " join_candidate=" << seedSummary.joinCandidateCount
            << " integrate_candidate=" << seedSummary.integrateCandidateCount
            << " actual_split=" << seedSummary.actualSplitHits
            << " actual_join=" << seedSummary.actualJoinHits
            << " actual_integrate=" << seedSummary.actualIntegrateHits
            << " split_conversion=" << json_number(safe_ratio(seedSummary.actualSplitHits, seedSummary.splitReadyCount))
            << " join_conversion=" << json_number(safe_ratio(seedSummary.actualJoinHits, seedSummary.joinCandidateCount))
            << " integrate_conversion=" << json_number(safe_ratio(seedSummary.actualIntegrateHits, seedSummary.integrateCandidateCount))
            << " unique_trace_prefix_count=" << seedSummary.tracePrefixHistogram.size()
            << " unique_primitive_multiset_count=" << seedSummary.primitiveMultisetHistogram.size()
            << '\n';
    }
}

void flush_fuzz_stats(const TestOptions& options, const string& caseName) {
    if (!g_fuzzStats.has_value()) {
        return;
    }

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
    ofs << "\"first_split_iter\":" << g_fuzzStats->firstSplitIter << ",\n";
    ofs << "\"first_join_iter\":" << g_fuzzStats->firstJoinIter << ",\n";
    ofs << "\"first_integrate_iter\":" << g_fuzzStats->firstIntegrateIter << ",\n";
    ofs << "\"multi_edge_count\":" << g_fuzzStats->multiEdgeCount << ",\n";
    ofs << "\"reducer_invocation_count\":" << g_fuzzStats->reducerInvocationCount << ",\n";
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
        ofs << "\"first_split_iter\":" << seedSummary.firstSplitIter << ',';
        ofs << "\"first_join_iter\":" << seedSummary.firstJoinIter << ',';
        ofs << "\"first_integrate_iter\":" << seedSummary.firstIntegrateIter << ',';
        ofs << "\"multi_edge_count\":" << seedSummary.multiEdgeCount << ',';
        ofs << "\"reducer_invocation_count\":" << seedSummary.reducerInvocationCount << ',';
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
    for (RawSkelID sid : sids) {
        if (sid != NIL_U32) {
            assert_skeleton_wellformed(RE, sid);
        }
    }
    for (OccID occ : occs) {
        assert_occ_patch_consistent(RE, occ);
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
    if (!(capture_occ_patch_signature(RE, occ) == expected)) {
        assert(false);
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
        assert_skeleton_wellformed(RE, sid);

        unordered_set<RawVID> localV;
        unordered_set<RawEID> localE;
        unordered_set<OccID> localOcc;

        for (RawVID v : S.verts) {
            if (!localV.insert(v).second || !RE.V.a[v].alive) {
                assert(false);
            }
            ++vertexOwnerCnt[v];
        }

        for (RawEID eid : S.edges) {
            if (!localE.insert(eid).second || !RE.E.a[eid].alive) {
                assert(false);
            }
            ++edgeOwnerCnt[eid];
        }

        for (OccID occ : S.hostedOcc) {
            if (!localOcc.insert(occ).second || !RE.occ.a[occ].alive) {
                assert(false);
            }
            ++occHostCnt[occ];
            assert_occ_patch_consistent(RE, occ);
            const RawOccRecord& O = RE.occ.get(occ);
            if (O.hostSkel != sid) {
                assert(false);
            }
        }
    }

    for (u32 vid = 0; vid < RE.V.a.size(); ++vid) {
        if (!RE.V.a[vid].alive) {
            continue;
        }
        if (vertexOwnerCnt[vid] != 1) {
            assert(false);
        }
    }

    for (u32 eid = 0; eid < RE.E.a.size(); ++eid) {
        if (!RE.E.a[eid].alive) {
            continue;
        }
        if (edgeOwnerCnt[eid] != 1) {
            assert(false);
        }
    }

    unordered_map<OccID, int> occOrigCnt;
    for (const auto& entry : RE.occOfOrig) {
        unordered_set<OccID> uniq(entry.second.begin(), entry.second.end());
        if (uniq.size() != entry.second.size()) {
            assert(false);
        }
        for (OccID occ : entry.second) {
            if (!RE.occ.a[occ].alive || RE.occ.get(occ).orig != entry.first) {
                assert(false);
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
            assert(false);
        }
        if (O.hostSkel >= RE.skel.a.size() || !RE.skel.a[O.hostSkel].alive) {
            assert(false);
        }
    }
}

void assert_planner_stop_condition(const RawEngine& RE, OccID target) {
    const RawOccRecord& O = RE.occ.get(target);
    const RawSkeleton& S = RE.skel.get(O.hostSkel);
    if (S.hostedOcc.size() != 1U || S.hostedOcc[0] != target) {
        assert(false);
    }
    if (discover_split_pair_from_support(RE, target).has_value()) {
        assert(false);
    }
    assert_occ_patch_consistent(RE, target);
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
    commit_skeleton(fixture.RE, fixture.sid, move(B), fixture.U);
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
    const Vertex siblingBase = 2000U + static_cast<Vertex>((seed % 233U) * 8U);

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
    B.corePatchLocalEids[scenario.targetOcc] = move(targetCore);
    commit_skeleton(scenario.RE, sid, move(B), scenario.U);

    scenario.ctx.targetOcc = scenario.targetOcc;
    scenario.ctx.keepOcc = {scenario.targetOcc};
    scenario.expectedSig = capture_occ_signatures(scenario.RE, collect_live_occurrences(scenario.RE));
    scenario.expectedTargetPrep =
        normalize_prep(prepare_isolate_checked(scenario.RE, scenario.RE.occ.get(scenario.targetOcc).hostSkel, scenario.targetOcc, "targeted_split_ready_prepare"));
    assert(discover_split_pair_from_support(scenario.RE, scenario.targetOcc).has_value());
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
    const BranchLayout keepBranch = append_core_branch(
        B,
        sepA,
        sepB,
        4000U + static_cast<Vertex>((seed % 181U) * 6U),
        1 + static_cast<int>((seed >> 2U) & 1U)
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
    commit_skeleton(scenario.RE, sid, move(B), scenario.U);

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
        commit_skeleton(scenario.RE, leftSid, move(B), scenario.U);
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
        commit_skeleton(scenario.RE, rightSid, move(B), scenario.U);
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
        commit_skeleton(scenario.RE, parentSid, move(B), scenario.U);
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
        commit_skeleton(scenario.RE, childSid, move(B), scenario.U);
    }

    scenario.ctx.targetOcc = scenario.targetOcc;
    scenario.ctx.keepOcc = {scenario.targetOcc};
    scenario.initialQueue.push_back(make_integrate_queue_job(parentSid, childSid, 2, 8));
    scenario.preseedCoverage.integrateCandidateCount = 1U;
    finalize_planner_scenario(scenario, "integrate_ready_prepare");
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
        assert(scenario.RE.occ.a[occ].alive);
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

    commit_skeleton(RE, gc.sid, move(B), U);
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
    commit_skeleton(RE, gc.sid, move(B), U);
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
        assert(false);
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
    commit_skeleton(RE, sid, move(B), U);
    assert_engine_ok(RE, {sid}, {occ17, occ31});

    const IsolatePrepared prep = prepare_isolate_checked(RE, sid, occ17, "isolate_micro_prepare");
    assert(prep.occ == occ17);
    assert(prep.orig == 5);
    assert(prep.allocNbr.size() == 1U && prep.allocNbr[0] == occ23);
    assert(prep.ports.size() == 2U);
    assert(prep.core.orig.size() == 3U);
    assert(prep.core.edges.size() == 3U);

    const IsolateVertexResult res = isolate_vertex(RE, sid, occ17, U);
    assert(res.residualSkel == sid);
    assert(res.occSkel != sid);
    assert_engine_ok(RE, {sid, res.occSkel}, {occ17, occ31});
    assert(RE.skel.get(sid).hostedOcc.size() == 1U && RE.skel.get(sid).hostedOcc[0] == occ31);
    assert(RE.skel.get(res.occSkel).hostedOcc.size() == 1U && RE.skel.get(res.occSkel).hostedOcc[0] == occ17);
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
    commit_skeleton(RE, sid, move(B), U);

    const IsolatePrepared p = prepare_isolate_checked(RE, sid, occ, "isolate_multiedge_prepare");
    assert(p.core.orig.size() == 2U);
    assert(p.core.edges.size() == 2U);
}

void test_split_microcase() {
    SplitFixture fixture = make_split_fixture();
    const SplitSeparationPairResult sp = split_checked(fixture.RE, fixture.sid, 2, 8, fixture.U, "split_micro");
    assert(sp.child.size() == 3U);

    vector<RawSkelID> sids;
    for (const SplitChildInfo& child : sp.child) {
        sids.push_back(child.sid);
    }
    assert_engine_ok(fixture.RE, sids, {fixture.occ31, fixture.occ44});

    RawSkelID sid31 = NIL_U32;
    RawSkelID sid44 = NIL_U32;
    RawSkelID sidAB = NIL_U32;
    classify_split_children(sp, fixture.occ31, fixture.occ44, sid31, sid44, sidAB);
    assert(sid31 != NIL_U32);
    assert(sid44 != NIL_U32);
    assert(sidAB != NIL_U32);
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
    assert(sid31 != NIL_U32 && sid44 != NIL_U32 && sidAB != NIL_U32);

    assert(join_checked(fixture.RE, sid31, sid44, 2, 8, fixture.U, "join_micro").mergedSid == sid31);
    assert_engine_ok(fixture.RE, {sid31, sidAB}, {fixture.occ31, fixture.occ44});
    assert(fixture.RE.skel.get(sid31).hostedOcc.size() == 2U);
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
        commit_skeleton(RE, parentSid, move(B), U);
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
        commit_skeleton(RE, childSid, move(B), U);
    }

    const OccPatchSignature sig31 = capture_occ_patch_signature(RE, occ31);
    const OccPatchSignature sig44 = capture_occ_patch_signature(RE, occ44);

    assert_engine_ok(RE, {parentSid, childSid}, {occ31, occ44});
    assert(integrate_checked(RE, parentSid, childSid, identity_boundary_map(6, 8), U, "integrate_micro").mergedSid == parentSid);
    assert_engine_ok(RE, {parentSid}, {occ31, occ44});
    assert(RE.skel.get(parentSid).hostedOcc.size() == 2U);
    assert(capture_occ_patch_signature(RE, occ31) == sig31);
    assert(capture_occ_patch_signature(RE, occ44) == sig44);
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
    commit_skeleton(RE, sid, move(B), U);

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
        assert(false);
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
        assert(false);
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
        assert(false);
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
        assert(false);
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
        assert(false);
    }

    for (RawSkelID sidAB : layout.boundaryOnly) {
        integrate_checked(RE, layout.anchor, sidAB, identity_boundary_map(), U, "split_join_integrate");
    }
    for (RawSkelID sidX : layout.otherOccChildren) {
        join_checked(RE, layout.anchor, sidX, 2, 8, U, "split_join_join");
    }

    assert_engine_bookkeeping_sane(RE);
    if (RE.skel.get(layout.anchor).hostedOcc.size() != gc.occs.size()) {
        assert(false);
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
        assert(false);
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
        RawUpdaterRunOptions{spec.stepBudget},
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
    assert(sp.child.size() >= 2U);

    const SplitLayout layout = collect_split_layout(sp, gc.occs[0]);
    assert(layout.anchor != NIL_U32);

    for (RawSkelID sidAB : layout.boundaryOnly) {
        const IntegrateResult ir = integrate_checked(RE, layout.anchor, sidAB, identity_boundary_map(), U, "roundtrip_integrate");
        if (ir.mergedSid != layout.anchor) {
            assert(false);
        }
    }

    for (RawSkelID sidX : layout.otherOccChildren) {
        const JoinSeparationPairResult jr = join_checked(RE, layout.anchor, sidX, 2, 8, U, "roundtrip_join");
        if (jr.mergedSid != layout.anchor) {
            assert(false);
        }
    }

    assert_engine_bookkeeping_sane(RE);
    assert_skeleton_wellformed(RE, layout.anchor);
    assert(RE.skel.get(layout.anchor).hostedOcc.size() == gc.occs.size());
    for (OccID occ : gc.occs) {
        assert(RE.occ.get(occ).hostSkel == layout.anchor);
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
    assert(sp.child.size() >= 2U);

    const SplitLayout layout = collect_split_layout(sp, target);
    assert(layout.anchor != NIL_U32);

    for (RawSkelID sidAB : layout.boundaryOnly) {
        const IntegrateResult ir = integrate_checked(RE, layout.anchor, sidAB, identity_boundary_map(), U, "split_integrate_ensure_integrate");
        if (ir.mergedSid != layout.anchor) {
            assert(false);
        }
    }

    RawPlannerCtx ctx;
    ctx.targetOcc = target;
    ctx.keepOcc = {target};
    const RawUpdaterRunOptions runOptions{options.stepBudget};
    run_planner_checked(RE, ctx, U, runOptions, "split_integrate_ensure_planner");

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
        RawUpdaterRunOptions{options.stepBudget},
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
    const vector<u32> seeds = options.seed.has_value() ? vector<u32>{*options.seed} : defaultSeeds;
    const int iters = options.iters.value_or(defaultIters);

    for (u32 seed : seeds) {
        begin_fuzz_seed_summary(seed);
        mt19937 rng(seed);
        for (int iter = 0; iter < iters; ++iter) {
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
    const vector<u32> seeds = options.seed.has_value() ? vector<u32>{*options.seed} : defaultSeeds;
    const int iters = options.iters.value_or(defaultIters);

    for (u32 seed : seeds) {
        begin_fuzz_seed_summary(seed);
        for (int iter = 0; iter < iters; ++iter) {
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

    const vector<u32> seeds = options.seed.has_value() ? vector<u32>{*options.seed} : defaultSeeds;
    const int iters = options.iters.value_or(defaultIters);

    for (u32 seed : seeds) {
        begin_fuzz_seed_summary(seed);
        for (int iter = 0; iter < iters; ++iter) {
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
            PlannerTargetedScenario scenario = make_targeted_planner_scenario(activeFamily, scenarioSeed);
            record_generated_artifact_metadata(scenario.hasDirectAB, 0U);
            const PlannerExecutionResult result = run_targeted_planner_iteration(
                scenario,
                RawUpdaterRunOptions{options.stepBudget},
                "targeted_" + scenario_family_name_string(activeFamily)
            );
            record_planner_execution_stats(result, ordinal);
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
        dump.initialQueue.push_back(UpdJob{UpdJobKind::ENSURE_SOLE, dump.targetOcc});
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
    smokeOptions.scenarioFamily = ScenarioFamily::SPLIT_WITH_BOUNDARY_ARTIFACT;
    begin_fuzz_stats_case(smokeOptions, "planner_targeted_integrate_smoke");
    set_active_test_options(&smokeOptions);
    run_targeted_planner_case(smokeOptions, "planner_targeted_integrate_smoke", smokeOptions.scenarioFamily, {830301U}, 2);
    flush_fuzz_stats(smokeOptions, "planner_targeted_integrate_smoke");
    set_active_test_options(&options);
    const filesystem::path statsPath = stats_output_path(smokeOptions, "planner_targeted_integrate_smoke");
    require_coverage_at_least(require_fuzz_stats_or_throw("planner_targeted_integrate_smoke"), "planner_targeted_integrate_smoke", 1, 1, 0, 1, 1, 0, 1, statsPath);
    require_conversion_at_least(require_fuzz_stats_or_throw("planner_targeted_integrate_smoke"), "planner_targeted_integrate_smoke", 0.50, 0.00, 0.50, statsPath);
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
    require_coverage_at_least(require_fuzz_stats_or_throw("planner_targeted_mixed_smoke"), "planner_targeted_mixed_smoke", 2, 2, 2, 2, 2, 2, 2, statsPath);
    require_conversion_at_least(require_fuzz_stats_or_throw("planner_targeted_mixed_smoke"), "planner_targeted_mixed_smoke", 0.25, 0.25, 0.25, statsPath);
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
    require_coverage_at_least(stats, "planner_coverage_smoke", 2, 2, 2, 2, 2, 2, 2, statsPath);
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

void run_artifact_retention_smoke_case(const TestOptions& options) {
    TestOptions retentionOptions = options;
    retentionOptions.maxArtifacts = 4U;

    const filesystem::path root = resolve_artifact_dir(retentionOptions);
    const array<string, 5> leaves = {{"traces", "logs", "counterexamples", "reduced", "reduced_planner"}};
    for (const string& leaf : leaves) {
        filesystem::remove_all(root / leaf);
        filesystem::create_directories(root / leaf);
    }

    const array<pair<string, string>, 6> files = {{
        {"traces", "001_old_trace.log"},
        {"logs", "002_old_log.log"},
        {"counterexamples", "003_old_counterexample.txt"},
        {"traces", "004_mid_trace.log"},
        {"reduced", "005_keep_reduced.txt"},
        {"reduced_planner", "006_keep_reduced_planner.txt"},
    }};
    for (size_t i = 0; i < files.size(); ++i) {
        const filesystem::path path = root / files[i].first / files[i].second;
        ofstream ofs(path);
        ofs << "artifact " << i << '\n';
        filesystem::last_write_time(path, filesystem::file_time_type::clock::now() + chrono::seconds(static_cast<int>(i)));
    }

    enforce_artifact_retention(retentionOptions);

    size_t fileCount = 0;
    for (const string& leaf : leaves) {
        for (const auto& entry : filesystem::directory_iterator(root / leaf)) {
            fileCount += entry.is_regular_file() ? 1U : 0U;
        }
    }
    if (fileCount > retentionOptions.maxArtifacts) {
        throw runtime_error("artifact_retention_smoke exceeded maxArtifacts");
    }
    if (!filesystem::exists(root / "reduced" / "005_keep_reduced.txt") ||
        !filesystem::exists(root / "reduced_planner" / "006_keep_reduced_planner.txt")) {
        throw runtime_error("artifact_retention_smoke pruned reduced artifacts first");
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
    assert(anchor != NIL_U32);

    for (RawSkelID sidAB : boundaryOnly) {
        const IntegrateResult ir = integrate_checked(RE, anchor, sidAB, identity_boundary_map(), U, "regression_44001_integrate");
        if (ir.mergedSid != anchor) {
            assert(false);
        }
    }

    RawPlannerCtx ctx;
    ctx.targetOcc = target;
    ctx.keepOcc = {target};
    const RawUpdaterRunOptions runOptions{options.stepBudget};
    run_planner_checked(RE, ctx, U, runOptions, "regression_44001_planner");

    const RawSkelID hostSid = RE.occ.get(target).hostSkel;
    if (RE.skel.get(hostSid).hostedOcc.size() != 1U || RE.skel.get(hostSid).hostedOcc[0] != target) {
        assert(false);
    }
    assert(!discover_split_pair_from_support(RE, target).has_value());
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
        TestOptions oracleOptions = options;
        oracleOptions.oracleMode = OracleMode::PLANNER;
        begin_fuzz_stats_case(oracleOptions, "planner_oracle_fuzz");
        set_active_test_options(&oracleOptions);
        try {
            if (oracleOptions.scenarioFamily == ScenarioFamily::RANDOM) {
                run_fuzz_mode_case(
                    oracleOptions,
                    "planner_oracle_fuzz",
                    FuzzMode::MIXED_PLANNER,
                    {821001U, 821002U, 821003U},
                    80
                );
            } else {
                run_targeted_planner_case(
                    oracleOptions,
                    "planner_oracle_fuzz",
                    oracleOptions.scenarioFamily,
                    {821001U, 821002U, 821003U},
                    80
                );
            }
        } catch (...) {
            record_failure_stat("planner_oracle_fuzz_exception");
            maybe_reduce_pending_planner_state(oracleOptions);
            flush_fuzz_stats(oracleOptions, "planner_oracle_fuzz");
            set_active_test_options(&options);
            throw;
        }
        flush_fuzz_stats(oracleOptions, "planner_oracle_fuzz");
        set_active_test_options(&options);
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
