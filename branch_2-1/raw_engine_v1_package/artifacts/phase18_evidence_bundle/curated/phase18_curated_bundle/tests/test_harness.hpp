#pragma once

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

#include "raw_engine/raw_engine.hpp"
#include "reference_planner.hpp"

enum class OracleMode : u8 {
    NONE = 0,
    PRIMITIVE = 1,
    PLANNER = 2,
    ALL = 3,
};

enum class TraceLevel : u8 {
    NONE = 0,
    SUMMARY = 1,
    FULL = 2,
};

enum class WeightProfile : u8 {
    RANDOM = 0,
    WEIGHTED_SPLIT_HEAVY = 1,
    WEIGHTED_JOIN_HEAVY = 2,
    WEIGHTED_INTEGRATE_HEAVY = 3,
    ARTIFACT_HEAVY = 4,
    MULTIEDGE_HEAVY = 5,
};

enum class PreconditionBiasProfile : u8 {
    DEFAULT = 0,
    BALANCED = 1,
    SPLIT_HEAVY = 2,
    JOIN_HEAVY = 3,
    INTEGRATE_HEAVY = 4,
    ARTIFACT_HEAVY = 5,
    STRUCTURAL = 6,
};

enum class ScenarioFamily : u8 {
    RANDOM = 0,
    SPLIT_READY = 1,
    SPLIT_WITH_BOUNDARY_ARTIFACT = 2,
    SPLIT_WITH_KEEPOCC_SIBLING = 3,
    SPLIT_WITH_JOIN_AND_INTEGRATE = 4,
    PLANNER_MIXED_TARGETED = 5,
    JOIN_READY = 6,
    INTEGRATE_READY = 7,
    PLANNER_MIXED_STRUCTURAL = 8,
    SPLIT_TIE_READY = 9,
    SPLIT_TIE_STRUCTURAL = 10,
    PLANNER_TIE_MIXED = 11,
    SPLIT_TIE_SYMMETRIC_LARGE = 12,
    PLANNER_TIE_MIXED_SYMMETRIC = 13,
    CANONICAL_COLLISION_PROBE = 14,
    SPLIT_TIE_ORGANIC_SYMMETRIC = 15,
    PLANNER_TIE_MIXED_ORGANIC = 16,
    AUTOMORPHISM_PROBE_LARGE = 17,
    PLANNER_TIE_MIXED_ORGANIC_COMPARE_READY = 18,
};

enum class SplitChoicePolicyMode : u8 {
    FAST = 0,
    EXACT_SHADOW = 1,
    EXACT_FULL = 2,
};

enum class SplitChoiceCompareMode : u8 {
    NONE = 0,
    EXACT_FULL = 1,
};

enum class CorpusPolicy : u8 {
    BEST = 0,
    APPEND = 1,
    REPLACE = 2,
};

enum class ExhaustiveFamily : u8 {
    SPLIT_READY = 0,
    JOIN_READY = 1,
    INTEGRATE_READY = 2,
    MIXED = 3,
    SPLIT_TIE_READY = 4,
    SPLIT_TIE_STRUCTURAL = 5,
    PLANNER_TIE_MIXED = 6,
    SPLIT_TIE_SYMMETRIC_LARGE = 7,
    PLANNER_TIE_MIXED_SYMMETRIC = 8,
    CANONICAL_COLLISION_PROBE = 9,
    SPLIT_TIE_ORGANIC_SYMMETRIC = 10,
    PLANNER_TIE_MIXED_ORGANIC = 11,
    AUTOMORPHISM_PROBE_LARGE = 12,
    ALL = 13,
};

struct FailureContextSnapshot {
    std::string caseName = "none";
    std::optional<u32> seed;
    int iter = -1;
};

struct PlannerCoverageSummary {
    std::size_t splitReadyCount = 0;
    std::size_t boundaryOnlyChildCount = 0;
    std::size_t joinCandidateCount = 0;
    std::size_t integrateCandidateCount = 0;
    std::size_t actualSplitHits = 0;
    std::size_t actualJoinHits = 0;
    std::size_t actualIntegrateHits = 0;
    std::size_t splitChoiceCandidateCount = 0;
    std::size_t splitChoiceEvalCount = 0;
    std::size_t splitChoiceTieCount = 0;
    std::size_t splitChoiceMulticlassCount = 0;
    std::size_t splitChoiceFallbackCount = 0;
    std::unordered_map<std::size_t, std::size_t> splitChoiceEquivClassCountHistogram;
};

struct PlannerExecutionResult {
    std::vector<PlannerTraceEntry> trace;
    PlannerCoverageSummary coverage;
};

struct TestOptions {
    std::string caseName = "all";
    std::optional<u32> seed;
    std::optional<int> iters;
    std::optional<std::string> reproFile;
    std::optional<std::string> stateFile;
    std::optional<std::string> primitiveName;
    std::optional<std::string> artifactDir;
    std::optional<OccID> targetOcc;
    std::vector<OccID> keepOcc;
    std::string executablePath;
    std::size_t stepBudget = 200000;
    std::size_t checkpointEvery = 0;
    std::size_t maxWallSeconds = 0;
    std::size_t targetComparedStates = 0;
    std::size_t targetEligibleStates = 0;
    std::size_t targetLineageSamples = 0;
    std::size_t maxPartialRuns = 0;
    std::size_t maxArtifacts = 0;
    int repeat = 1;
    int iterStart = 0;
    bool verbose = false;
    bool dumpOnFail = false;
    bool reduce = false;
    bool dumpTrace = false;
    bool stats = false;
    bool stopAfterCheckpoint = false;
    bool stopWhenGatePasses = false;
    bool keepOnlyFailures = true;
    bool compressArtifacts = false;
    OracleMode oracleMode = OracleMode::NONE;
    TraceLevel traceLevel = TraceLevel::SUMMARY;
    WeightProfile weightProfile = WeightProfile::RANDOM;
    PreconditionBiasProfile preconditionBiasProfile = PreconditionBiasProfile::DEFAULT;
    ScenarioFamily scenarioFamily = ScenarioFamily::RANDOM;
    CorpusPolicy corpusPolicy = CorpusPolicy::BEST;
    int biasSplit = -1;
    int biasJoin = -1;
    int biasIntegrate = -1;
    std::size_t maxReal = 5;
    std::size_t maxOcc = 3;
    std::size_t maxEdges = 8;
    std::size_t maxComponents = 0;
    std::size_t maxHostedOcc = 0;
    std::size_t maxStates = 5000;
    std::size_t collisionSpotCheckCount = 0;
    std::size_t maxSplitPairCandidates = 0;
    std::size_t maxSplitChoiceEval = 0;
    std::size_t exactCanonicalCap = 0;
    std::size_t exactCanonicalSampleRate = 1;
    double exactAuditSampleRate = 1.0;
    std::size_t exactAuditBudget = 0;
    std::optional<ScenarioFamily> exactAuditFamily;
    SplitChoicePolicyMode splitChoicePolicyMode = SplitChoicePolicyMode::EXACT_SHADOW;
    SplitChoiceCompareMode splitChoiceCompareMode = SplitChoiceCompareMode::NONE;
    double compareSampleRate = 1.0;
    double targetApplicabilityConfidence = 0.0;
    std::size_t compareBudget = 0;
    bool dedupeCanonical = false;
    ExhaustiveFamily exhaustiveFamily = ExhaustiveFamily::ALL;
    std::optional<std::string> statsFile;
    std::optional<std::string> saveCorpusDir;
    std::optional<std::string> loadCorpusDir;
    std::optional<std::string> campaignConfig;
    std::optional<std::string> checkpointDir;
    std::optional<std::string> resumeFrom;
    std::optional<std::string> policyManifest;
    std::optional<std::string> gateFamily;
    std::optional<std::string> gateOutput;
    bool gateStrict = false;
};

void install_failure_handlers();
void print_usage(std::ostream& os);
int run_test_suite(const TestOptions& options);

void set_active_test_options(const TestOptions* options);
const TestOptions& active_test_options();
bool primitive_oracle_enabled(const TestOptions& options);
bool planner_oracle_enabled(const TestOptions& options);
bool active_primitive_oracle_enabled();
bool active_planner_oracle_enabled();
FailureContextSnapshot current_failure_context();
std::string current_failure_stem();
bool trace_enabled(const TestOptions& options);
bool active_trace_enabled();
std::string trace_level_name_string(TraceLevel traceLevel);
std::string weight_profile_name_string(WeightProfile profile);
std::string precondition_bias_profile_name_string(PreconditionBiasProfile profile);
std::string scenario_family_name_string(ScenarioFamily family);
std::string split_choice_policy_mode_name_string(SplitChoicePolicyMode mode);
std::string split_choice_compare_mode_name_string(SplitChoiceCompareMode mode);
std::string corpus_policy_name_string(CorpusPolicy policy);
const std::vector<PlannerTraceEntry>& last_planner_trace();

inline RawSplitChoicePolicyMode raw_split_choice_policy_mode(SplitChoicePolicyMode mode) {
    switch (mode) {
        case SplitChoicePolicyMode::FAST:
            return RawSplitChoicePolicyMode::FAST;
        case SplitChoicePolicyMode::EXACT_SHADOW:
        case SplitChoicePolicyMode::EXACT_FULL:
            return RawSplitChoicePolicyMode::EXACT_SHADOW;
    }
    return RawSplitChoicePolicyMode::EXACT_SHADOW;
}

inline RawUpdaterRunOptions planner_run_options_with(
    std::size_t stepBudget,
    std::size_t maxSplitChoiceEval,
    SplitChoicePolicyMode mode
) {
    RawUpdaterRunOptions runOptions;
    runOptions.stepBudget = stepBudget;
    runOptions.maxSplitChoiceEval = maxSplitChoiceEval;
    runOptions.splitChoicePolicy = raw_split_choice_policy_mode(mode);
    return runOptions;
}

inline RawUpdaterRunOptions planner_run_options(const TestOptions& options) {
    return planner_run_options_with(options.stepBudget, options.maxSplitChoiceEval, options.splitChoicePolicyMode);
}

inline RawUpdaterRunOptions planner_run_options(std::size_t stepBudget, const TestOptions& options) {
    return planner_run_options_with(stepBudget, options.maxSplitChoiceEval, options.splitChoicePolicyMode);
}

IsolatePrepared prepare_isolate_checked(const RawEngine& RE, RawSkelID sid, OccID occ, const std::string& label = {});
SplitSeparationPairResult split_checked(
    RawEngine& RE,
    RawSkelID sid,
    Vertex saOrig,
    Vertex sbOrig,
    RawUpdateCtx& U,
    const std::string& label = {}
);
JoinSeparationPairResult join_checked(
    RawEngine& RE,
    RawSkelID leftSid,
    RawSkelID rightSid,
    Vertex saOrig,
    Vertex sbOrig,
    RawUpdateCtx& U,
    const std::string& label = {}
);
IntegrateResult integrate_checked(
    RawEngine& RE,
    RawSkelID parentSid,
    RawSkelID childSid,
    const std::vector<BoundaryMapEntry>& bm,
    RawUpdateCtx& U,
    const std::string& label = {}
);

void run_planner_checked(
    RawEngine& RE,
    const RawPlannerCtx& ctx,
    RawUpdateCtx& U,
    const RawUpdaterRunOptions& runOptions,
    const std::string& label = {}
);

PlannerExecutionResult run_planner_checked_capture(
    RawEngine& RE,
    const RawPlannerCtx& ctx,
    RawUpdateCtx& U,
    const RawUpdaterRunOptions& runOptions,
    const std::vector<UpdJob>* initialQueue = nullptr,
    const std::string& label = {}
);

void execute_planner_capture(
    RawEngine& RE,
    const RawPlannerCtx& ctx,
    RawUpdateCtx& U,
    const RawUpdaterRunOptions& runOptions,
    const std::vector<UpdJob>* initialQueue,
    std::vector<PlannerTraceEntry>& trace,
    PlannerPhase* activePhase = nullptr
);
