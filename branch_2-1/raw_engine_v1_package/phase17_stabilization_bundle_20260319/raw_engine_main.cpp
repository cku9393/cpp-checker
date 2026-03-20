#include "test_harness.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;

namespace {

string require_value(int& i, int argc, char** argv, const string& flag) {
    if (i + 1 >= argc) {
        throw runtime_error("missing value for " + flag);
    }
    ++i;
    return argv[i];
}

bool next_is_value(int i, int argc, char** argv) {
    return i + 1 < argc && argv[i + 1] != nullptr && string(argv[i + 1]).rfind("--", 0) != 0;
}

void append_keep_occ(vector<OccID>& keepOcc, const string& csv) {
    size_t start = 0;
    while (start < csv.size()) {
        const size_t comma = csv.find(',', start);
        const string token = csv.substr(start, comma == string::npos ? string::npos : comma - start);
        if (!token.empty()) {
            keepOcc.push_back(static_cast<OccID>(stoul(token)));
        }
        if (comma == string::npos) {
            break;
        }
        start = comma + 1;
    }
}

optional<OracleMode> parse_oracle_mode(const string& text) {
    if (text == "primitive") {
        return OracleMode::PRIMITIVE;
    }
    if (text == "planner") {
        return OracleMode::PLANNER;
    }
    if (text == "all") {
        return OracleMode::ALL;
    }
    return nullopt;
}

optional<TraceLevel> parse_trace_level(const string& text) {
    if (text == "none") {
        return TraceLevel::NONE;
    }
    if (text == "summary") {
        return TraceLevel::SUMMARY;
    }
    if (text == "full") {
        return TraceLevel::FULL;
    }
    return nullopt;
}

optional<WeightProfile> parse_weight_profile(const string& text) {
    if (text == "random") {
        return WeightProfile::RANDOM;
    }
    if (text == "weighted_split_heavy") {
        return WeightProfile::WEIGHTED_SPLIT_HEAVY;
    }
    if (text == "weighted_join_heavy") {
        return WeightProfile::WEIGHTED_JOIN_HEAVY;
    }
    if (text == "weighted_integrate_heavy") {
        return WeightProfile::WEIGHTED_INTEGRATE_HEAVY;
    }
    if (text == "artifact_heavy") {
        return WeightProfile::ARTIFACT_HEAVY;
    }
    if (text == "multiedge_heavy") {
        return WeightProfile::MULTIEDGE_HEAVY;
    }
    return nullopt;
}

optional<PreconditionBiasProfile> parse_precondition_bias_profile(const string& text) {
    if (text == "default") {
        return PreconditionBiasProfile::DEFAULT;
    }
    if (text == "balanced") {
        return PreconditionBiasProfile::BALANCED;
    }
    if (text == "split_heavy") {
        return PreconditionBiasProfile::SPLIT_HEAVY;
    }
    if (text == "join_heavy") {
        return PreconditionBiasProfile::JOIN_HEAVY;
    }
    if (text == "integrate_heavy") {
        return PreconditionBiasProfile::INTEGRATE_HEAVY;
    }
    if (text == "artifact_heavy") {
        return PreconditionBiasProfile::ARTIFACT_HEAVY;
    }
    if (text == "structural") {
        return PreconditionBiasProfile::STRUCTURAL;
    }
    return nullopt;
}

optional<ScenarioFamily> parse_scenario_family(const string& text) {
    if (text == "random") {
        return ScenarioFamily::RANDOM;
    }
    if (text == "split_ready") {
        return ScenarioFamily::SPLIT_READY;
    }
    if (text == "split_with_boundary_artifact") {
        return ScenarioFamily::SPLIT_WITH_BOUNDARY_ARTIFACT;
    }
    if (text == "split_with_keepOcc_sibling") {
        return ScenarioFamily::SPLIT_WITH_KEEPOCC_SIBLING;
    }
    if (text == "split_with_join_and_integrate") {
        return ScenarioFamily::SPLIT_WITH_JOIN_AND_INTEGRATE;
    }
    if (text == "planner_mixed_targeted") {
        return ScenarioFamily::PLANNER_MIXED_TARGETED;
    }
    if (text == "join_ready") {
        return ScenarioFamily::JOIN_READY;
    }
    if (text == "integrate_ready") {
        return ScenarioFamily::INTEGRATE_READY;
    }
    if (text == "planner_mixed_structural") {
        return ScenarioFamily::PLANNER_MIXED_STRUCTURAL;
    }
    if (text == "split_tie_ready") {
        return ScenarioFamily::SPLIT_TIE_READY;
    }
    if (text == "split_tie_structural") {
        return ScenarioFamily::SPLIT_TIE_STRUCTURAL;
    }
    if (text == "planner_tie_mixed") {
        return ScenarioFamily::PLANNER_TIE_MIXED;
    }
    if (text == "split_tie_symmetric_large") {
        return ScenarioFamily::SPLIT_TIE_SYMMETRIC_LARGE;
    }
    if (text == "planner_tie_mixed_symmetric") {
        return ScenarioFamily::PLANNER_TIE_MIXED_SYMMETRIC;
    }
    if (text == "canonical_collision_probe") {
        return ScenarioFamily::CANONICAL_COLLISION_PROBE;
    }
    if (text == "split_tie_organic_symmetric") {
        return ScenarioFamily::SPLIT_TIE_ORGANIC_SYMMETRIC;
    }
    if (text == "planner_tie_mixed_organic") {
        return ScenarioFamily::PLANNER_TIE_MIXED_ORGANIC;
    }
    if (text == "planner_tie_mixed_organic_compare_ready") {
        return ScenarioFamily::PLANNER_TIE_MIXED_ORGANIC_COMPARE_READY;
    }
    if (text == "automorphism_probe_large") {
        return ScenarioFamily::AUTOMORPHISM_PROBE_LARGE;
    }
    return nullopt;
}

optional<SplitChoicePolicyMode> parse_split_choice_policy_mode(const string& text) {
    if (text == "fast") {
        return SplitChoicePolicyMode::FAST;
    }
    if (text == "exact_shadow") {
        return SplitChoicePolicyMode::EXACT_SHADOW;
    }
    if (text == "exact_full") {
        return SplitChoicePolicyMode::EXACT_FULL;
    }
    return nullopt;
}

optional<SplitChoiceCompareMode> parse_split_choice_compare_mode(const string& text) {
    if (text == "none") {
        return SplitChoiceCompareMode::NONE;
    }
    if (text == "exact_full") {
        return SplitChoiceCompareMode::EXACT_FULL;
    }
    return nullopt;
}

optional<CorpusPolicy> parse_corpus_policy(const string& text) {
    if (text == "best") {
        return CorpusPolicy::BEST;
    }
    if (text == "append") {
        return CorpusPolicy::APPEND;
    }
    if (text == "replace") {
        return CorpusPolicy::REPLACE;
    }
    return nullopt;
}

optional<ExhaustiveFamily> parse_exhaustive_family(const string& text) {
    if (text == "split_ready") {
        return ExhaustiveFamily::SPLIT_READY;
    }
    if (text == "join_ready") {
        return ExhaustiveFamily::JOIN_READY;
    }
    if (text == "integrate_ready") {
        return ExhaustiveFamily::INTEGRATE_READY;
    }
    if (text == "mixed") {
        return ExhaustiveFamily::MIXED;
    }
    if (text == "split_tie_ready") {
        return ExhaustiveFamily::SPLIT_TIE_READY;
    }
    if (text == "split_tie_structural") {
        return ExhaustiveFamily::SPLIT_TIE_STRUCTURAL;
    }
    if (text == "planner_tie_mixed") {
        return ExhaustiveFamily::PLANNER_TIE_MIXED;
    }
    if (text == "split_tie_symmetric_large") {
        return ExhaustiveFamily::SPLIT_TIE_SYMMETRIC_LARGE;
    }
    if (text == "planner_tie_mixed_symmetric") {
        return ExhaustiveFamily::PLANNER_TIE_MIXED_SYMMETRIC;
    }
    if (text == "canonical_collision_probe") {
        return ExhaustiveFamily::CANONICAL_COLLISION_PROBE;
    }
    if (text == "split_tie_organic_symmetric") {
        return ExhaustiveFamily::SPLIT_TIE_ORGANIC_SYMMETRIC;
    }
    if (text == "planner_tie_mixed_organic") {
        return ExhaustiveFamily::PLANNER_TIE_MIXED_ORGANIC;
    }
    if (text == "automorphism_probe_large") {
        return ExhaustiveFamily::AUTOMORPHISM_PROBE_LARGE;
    }
    if (text == "all") {
        return ExhaustiveFamily::ALL;
    }
    return nullopt;
}

} // namespace

void print_usage(ostream& os) {
    os << "Usage: raw_engine_tests [options]\n";
    os << "  --case <name>\n";
    os << "  --seed <u32>\n";
    os << "  --iters <int>\n";
    os << "  --repro-file <path>\n";
    os << "  --state-file <path>\n";
    os << "  --primitive isolate|split|join|integrate\n";
    os << "  --target-occ <id>\n";
    os << "  --keep-occ <csv>\n";
    os << "  --artifact-dir <path>\n";
    os << "  --save-corpus <dir>\n";
    os << "  --load-corpus <dir>\n";
    os << "  --corpus-policy best|append|replace\n";
    os << "  --campaign-config <json|txt>\n";
    os << "  --checkpoint-dir <path>\n";
    os << "  --checkpoint-every <N>\n";
    os << "  --resume-from <path>\n";
    os << "  --max-wall-seconds <sec>\n";
    os << "  --target-compared-states <N>\n";
    os << "  --target-eligible-states <N>\n";
    os << "  --target-lineage-samples <N>\n";
    os << "  --target-applicability-confidence <0..1>\n";
    os << "  --stop-when-gate-passes\n";
    os << "  --max-partial-runs <N>\n";
    os << "  --stop-after-checkpoint\n";
    os << "  --max-real <N>\n";
    os << "  --max-occ <N>\n";
    os << "  --max-edges <N>\n";
    os << "  --max-components <N>\n";
    os << "  --max-hosted-occ <N>\n";
    os << "  --max-states <N>\n";
    os << "  --collision-spot-checks <N>\n";
    os << "  --max-split-pair-candidates <N>\n";
    os << "  --max-split-choice-eval <N>\n";
    os << "  --exact-canonical-cap <N>\n";
    os << "  --exact-canonical-sample-rate <N>\n";
    os << "  --exact-audit-sample-rate <N>\n";
    os << "  --exact-audit-budget <N>\n";
    os << "  --exact-audit-family <scenario-family>\n";
    os << "  --split-choice-policy fast|exact_shadow|exact_full (default: exact_shadow)\n";
    os << "  --compare-against none|exact_full\n";
    os << "  --compare-sample-rate <double>\n";
    os << "  --compare-budget <N>\n";
    os << "  --dedupe-canonical\n";
    os << "  --family split_ready|join_ready|integrate_ready|mixed|split_tie_ready|split_tie_structural|planner_tie_mixed|split_tie_symmetric_large|planner_tie_mixed_symmetric|canonical_collision_probe|split_tie_organic_symmetric|planner_tie_mixed_organic|automorphism_probe_large|all\n";
    os << "  --stats\n";
    os << "  --stats-file <path>\n";
    os << "  --fuzz-mode random|weighted_split_heavy|weighted_join_heavy|weighted_integrate_heavy|artifact_heavy|multiedge_heavy|split_ready|split_with_boundary_artifact|split_with_keepOcc_sibling|split_with_join_and_integrate|planner_mixed_targeted|join_ready|integrate_ready|planner_mixed_structural|split_tie_ready|split_tie_structural|planner_tie_mixed|split_tie_symmetric_large|planner_tie_mixed_symmetric|canonical_collision_probe|split_tie_organic_symmetric|planner_tie_mixed_organic|planner_tie_mixed_organic_compare_ready|automorphism_probe_large\n";
    os << "  --weight-profile random|weighted_split_heavy|weighted_join_heavy|weighted_integrate_heavy|artifact_heavy|multiedge_heavy\n";
    os << "  --precondition-bias-profile default|balanced|split_heavy|join_heavy|integrate_heavy|artifact_heavy|structural\n";
    os << "  --bias-split <0..8>\n";
    os << "  --bias-join <0..8>\n";
    os << "  --bias-integrate <0..8>\n";
    os << "  --scenario-family random|split_ready|split_with_boundary_artifact|split_with_keepOcc_sibling|split_with_join_and_integrate|planner_mixed_targeted|join_ready|integrate_ready|planner_mixed_structural|split_tie_ready|split_tie_structural|planner_tie_mixed|split_tie_symmetric_large|planner_tie_mixed_symmetric|canonical_collision_probe|split_tie_organic_symmetric|planner_tie_mixed_organic|planner_tie_mixed_organic_compare_ready|automorphism_probe_large\n";
    os << "  --max-artifacts <N>\n";
    os << "  --keep-only-failures\n";
    os << "  --trace-level none|summary|full\n";
    os << "  --compress-artifacts\n";
    os << "  --step-budget <size_t>\n";
    os << "  --repeat <int>\n";
    os << "  --dump-on-fail\n";
    os << "  --oracle [primitive|planner|all]\n";
    os << "  --dump-trace\n";
    os << "  --reduce\n";
    os << "  --verbose\n";
    os << "  --help\n";
}

int main(int argc, char** argv) {
    TestOptions options;
    if (argc > 0 && argv[0] != nullptr) {
        options.executablePath = argv[0];
    }

    try {
        install_failure_handlers();

        for (int i = 1; i < argc; ++i) {
            const string arg = argv[i];
            if (arg == "--case") {
                options.caseName = require_value(i, argc, argv, arg);
            } else if (arg == "--seed") {
                options.seed = static_cast<u32>(stoul(require_value(i, argc, argv, arg)));
            } else if (arg == "--iters") {
                options.iters = stoi(require_value(i, argc, argv, arg));
            } else if (arg == "--repro-file") {
                options.reproFile = require_value(i, argc, argv, arg);
            } else if (arg == "--state-file") {
                options.stateFile = require_value(i, argc, argv, arg);
            } else if (arg == "--primitive") {
                options.primitiveName = require_value(i, argc, argv, arg);
            } else if (arg == "--target-occ") {
                options.targetOcc = static_cast<OccID>(stoul(require_value(i, argc, argv, arg)));
            } else if (arg == "--keep-occ") {
                append_keep_occ(options.keepOcc, require_value(i, argc, argv, arg));
            } else if (arg == "--artifact-dir") {
                options.artifactDir = require_value(i, argc, argv, arg);
            } else if (arg == "--save-corpus") {
                options.saveCorpusDir = require_value(i, argc, argv, arg);
            } else if (arg == "--load-corpus") {
                options.loadCorpusDir = require_value(i, argc, argv, arg);
            } else if (arg == "--corpus-policy") {
                const string valueText = require_value(i, argc, argv, arg);
                const optional<CorpusPolicy> policy = parse_corpus_policy(valueText);
                if (!policy.has_value()) {
                    throw runtime_error("unknown corpus policy: " + valueText);
                }
                options.corpusPolicy = *policy;
            } else if (arg == "--campaign-config") {
                options.campaignConfig = require_value(i, argc, argv, arg);
            } else if (arg == "--checkpoint-dir") {
                options.checkpointDir = require_value(i, argc, argv, arg);
            } else if (arg == "--checkpoint-every") {
                options.checkpointEvery = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--resume-from") {
                options.resumeFrom = require_value(i, argc, argv, arg);
            } else if (arg == "--max-wall-seconds") {
                options.maxWallSeconds = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--target-compared-states") {
                options.targetComparedStates = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--target-eligible-states") {
                options.targetEligibleStates = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--target-lineage-samples") {
                options.targetLineageSamples = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--target-applicability-confidence") {
                options.targetApplicabilityConfidence = stod(require_value(i, argc, argv, arg));
                if (options.targetApplicabilityConfidence < 0.0 || options.targetApplicabilityConfidence > 1.0) {
                    throw runtime_error("target applicability confidence must be in [0,1]");
                }
            } else if (arg == "--stop-when-gate-passes") {
                options.stopWhenGatePasses = true;
            } else if (arg == "--max-partial-runs") {
                options.maxPartialRuns = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--stop-after-checkpoint") {
                options.stopAfterCheckpoint = true;
            } else if (arg == "--max-real") {
                options.maxReal = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--max-occ") {
                options.maxOcc = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--max-edges") {
                options.maxEdges = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--max-components") {
                options.maxComponents = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--max-hosted-occ") {
                options.maxHostedOcc = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--max-states") {
                options.maxStates = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--collision-spot-checks") {
                options.collisionSpotCheckCount = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--max-split-pair-candidates") {
                options.maxSplitPairCandidates = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--max-split-choice-eval") {
                options.maxSplitChoiceEval = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--exact-canonical-cap") {
                options.exactCanonicalCap = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--exact-canonical-sample-rate") {
                options.exactCanonicalSampleRate = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--exact-audit-sample-rate") {
                options.exactAuditSampleRate = stod(require_value(i, argc, argv, arg));
            } else if (arg == "--exact-audit-budget") {
                options.exactAuditBudget = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--exact-audit-family") {
                const string valueText = require_value(i, argc, argv, arg);
                const optional<ScenarioFamily> family = parse_scenario_family(valueText);
                if (!family.has_value()) {
                    throw runtime_error("unknown exact audit family: " + valueText);
                }
                options.exactAuditFamily = *family;
            } else if (arg == "--split-choice-policy") {
                const string valueText = require_value(i, argc, argv, arg);
                const optional<SplitChoicePolicyMode> mode = parse_split_choice_policy_mode(valueText);
                if (!mode.has_value()) {
                    throw runtime_error("unknown split choice policy mode: " + valueText);
                }
                options.splitChoicePolicyMode = *mode;
            } else if (arg == "--compare-against") {
                const string valueText = require_value(i, argc, argv, arg);
                const optional<SplitChoiceCompareMode> mode = parse_split_choice_compare_mode(valueText);
                if (!mode.has_value()) {
                    throw runtime_error("unknown split-choice compare mode: " + valueText);
                }
                options.splitChoiceCompareMode = *mode;
            } else if (arg == "--compare-sample-rate") {
                options.compareSampleRate = stod(require_value(i, argc, argv, arg));
            } else if (arg == "--compare-budget") {
                options.compareBudget = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--dedupe-canonical") {
                options.dedupeCanonical = true;
            } else if (arg == "--family") {
                const string valueText = require_value(i, argc, argv, arg);
                const optional<ExhaustiveFamily> family = parse_exhaustive_family(valueText);
                if (!family.has_value()) {
                    throw runtime_error("unknown exhaustive family: " + valueText);
                }
                options.exhaustiveFamily = *family;
            } else if (arg == "--stats") {
                options.stats = true;
            } else if (arg == "--stats-file") {
                options.statsFile = require_value(i, argc, argv, arg);
                options.stats = true;
            } else if (arg == "--fuzz-mode") {
                const string valueText = require_value(i, argc, argv, arg);
                const optional<WeightProfile> profile = parse_weight_profile(valueText);
                if (profile.has_value()) {
                    options.weightProfile = *profile;
                    options.scenarioFamily = ScenarioFamily::RANDOM;
                    continue;
                }
                const optional<ScenarioFamily> family = parse_scenario_family(valueText);
                if (!family.has_value()) {
                    throw runtime_error("unknown fuzz mode: " + valueText);
                }
                options.scenarioFamily = *family;
            } else if (arg == "--weight-profile") {
                const string valueText = require_value(i, argc, argv, arg);
                const optional<WeightProfile> profile = parse_weight_profile(valueText);
                if (!profile.has_value()) {
                    throw runtime_error("unknown weight profile: " + valueText);
                }
                options.weightProfile = *profile;
            } else if (arg == "--precondition-bias-profile") {
                const string valueText = require_value(i, argc, argv, arg);
                const optional<PreconditionBiasProfile> profile = parse_precondition_bias_profile(valueText);
                if (!profile.has_value()) {
                    throw runtime_error("unknown precondition bias profile: " + valueText);
                }
                options.preconditionBiasProfile = *profile;
            } else if (arg == "--bias-split") {
                options.biasSplit = stoi(require_value(i, argc, argv, arg));
            } else if (arg == "--bias-join") {
                options.biasJoin = stoi(require_value(i, argc, argv, arg));
            } else if (arg == "--bias-integrate") {
                options.biasIntegrate = stoi(require_value(i, argc, argv, arg));
            } else if (arg == "--scenario-family") {
                const string valueText = require_value(i, argc, argv, arg);
                const optional<ScenarioFamily> family = parse_scenario_family(valueText);
                if (!family.has_value()) {
                    throw runtime_error("unknown scenario family: " + valueText);
                }
                options.scenarioFamily = *family;
            } else if (arg == "--max-artifacts") {
                options.maxArtifacts = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--keep-only-failures") {
                options.keepOnlyFailures = true;
            } else if (arg == "--trace-level") {
                const string valueText = require_value(i, argc, argv, arg);
                const optional<TraceLevel> traceLevel = parse_trace_level(valueText);
                if (!traceLevel.has_value()) {
                    throw runtime_error("unknown trace level: " + valueText);
                }
                options.traceLevel = *traceLevel;
            } else if (arg == "--step-budget") {
                options.stepBudget = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--repeat") {
                options.repeat = stoi(require_value(i, argc, argv, arg));
            } else if (arg == "--dump-on-fail") {
                options.dumpOnFail = true;
            } else if (arg == "--oracle") {
                if (next_is_value(i, argc, argv)) {
                    const string modeText = require_value(i, argc, argv, arg);
                    const optional<OracleMode> mode = parse_oracle_mode(modeText);
                    if (!mode.has_value()) {
                        throw runtime_error("unknown oracle mode: " + modeText);
                    }
                    options.oracleMode = *mode;
                } else {
                    options.oracleMode = OracleMode::PRIMITIVE;
                }
            } else if (arg == "--dump-trace") {
                options.dumpTrace = true;
                options.traceLevel = TraceLevel::FULL;
            } else if (arg == "--compress-artifacts") {
                options.compressArtifacts = true;
            } else if (arg == "--reduce") {
                options.reduce = true;
            } else if (arg == "--verbose") {
                options.verbose = true;
            } else if (arg == "--help") {
                print_usage(cout);
                return 0;
            } else {
                throw runtime_error("unknown argument: " + arg);
            }
        }

        if (options.repeat <= 0) {
            throw runtime_error("--repeat must be positive");
        }
        if (options.exactCanonicalSampleRate == 0U) {
            throw runtime_error("--exact-canonical-sample-rate must be positive");
        }
        if (!(options.exactAuditSampleRate > 0.0)) {
            throw runtime_error("--exact-audit-sample-rate must be positive");
        }
        if (!(options.compareSampleRate > 0.0)) {
            throw runtime_error("--compare-sample-rate must be positive");
        }
        if (options.iters.has_value() && *options.iters <= 0) {
            throw runtime_error("--iters must be positive");
        }
        if ((options.biasSplit != -1 && (options.biasSplit < 0 || options.biasSplit > 8)) ||
            (options.biasJoin != -1 && (options.biasJoin < 0 || options.biasJoin > 8)) ||
            (options.biasIntegrate != -1 && (options.biasIntegrate < 0 || options.biasIntegrate > 8))) {
            throw runtime_error("--bias-* values must be in [0,8]");
        }

        return run_test_suite(options);
    } catch (const exception& ex) {
        cerr << "raw_engine_tests error: " << ex.what() << '\n';
        print_usage(cerr);
        return 1;
    }
}
