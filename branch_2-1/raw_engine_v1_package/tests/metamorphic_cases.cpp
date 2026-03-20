#include "exhaustive_cases.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "exhaustive_generator.hpp"
#include "stabilization_support.hpp"
#include "state_dump.hpp"

using namespace std;

namespace {

struct PlannerSnapshot {
    PlannerFinalStateCanonicalSignature signature;
    size_t traceSize = 0U;
};

struct MetamorphicSnapshot {
    optional<PrimitiveCanonicalSignature> primitive;
    PlannerSnapshot planner;
};

struct MatrixScenario {
    string familyName;
    ExhaustiveScenario scenario;
    bool requirePlannerMultistep = false;
};

struct MatrixEntry {
    size_t primitiveOnlyHits = 0U;
    size_t plannerStepHits = 0U;
    size_t plannerMultistepHits = 0U;
    size_t replayStepHits = 0U;
};

struct MatrixStats {
    map<pair<string, string>, MatrixEntry> entries;
};

using TransformFactory = function<RebuildTransform(const ExhaustiveScenario&)>;

struct TransformSpec {
    string invariantName;
    TransformFactory makeTransform;
};

vector<TransformSpec> transform_specs() {
    return {
        TransformSpec{
            "relabel",
            [](const ExhaustiveScenario& scenario) { return make_relabel_transform(scenario); },
        },
        TransformSpec{
            "occid_renumber",
            [](const ExhaustiveScenario&) { return make_occid_renumber_transform(); },
        },
        TransformSpec{
            "edge_order",
            [](const ExhaustiveScenario&) { return make_edge_order_transform(); },
        },
        TransformSpec{
            "vertex_order",
            [](const ExhaustiveScenario&) { return make_vertex_order_transform(); },
        },
        TransformSpec{
            "hosted_occ_order",
            [](const ExhaustiveScenario&) { return make_hosted_occ_order_transform(); },
        },
    };
}

vector<string> matrix_family_names() {
    return {
        "split_ready",
        "join_ready",
        "integrate_ready",
        "mixed",
        "split_tie_ready",
        "split_tie_structural",
        "planner_mixed_targeted",
        "planner_mixed_structural",
        "planner_tie_mixed",
    };
}

TestOptions matrix_sample_options(const TestOptions& options) {
    TestOptions sample = options;
    sample.maxReal = 6;
    sample.maxOcc = 3;
    sample.maxEdges = 9;
    sample.maxComponents = 3;
    sample.maxHostedOcc = 2;
    sample.maxStates = 24;
    sample.dedupeCanonical = true;
    return sample;
}

OccID first_live_occurrence(const RawEngine& RE) {
    for (OccID occ = 0; occ < RE.occ.a.size(); ++occ) {
        if (RE.occ.a[occ].alive) {
            return occ;
        }
    }
    throw runtime_error("state has no live occurrences");
}

bool live_occurrence_exists(const RawEngine& RE, OccID occ) {
    return occ < RE.occ.a.size() && RE.occ.a[occ].alive;
}

void run_primitive_plan(ExhaustiveScenario& scenario) {
    switch (scenario.primitivePlan.primitive) {
        case PrimitiveKind::NONE:
            return;
        case PrimitiveKind::ISOLATE:
            throw runtime_error("metamorphic primitive plan does not support isolate");
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

void require_valid_state(const RawEngine& RE, const string& caseName, const string& label) {
    string validationError;
    if (!validate_engine_state_soft(RE, &validationError)) {
        throw runtime_error(caseName + " invalid state label=" + label + " error=" + validationError);
    }
}

ExhaustiveScenario normalize_result_for_compare(
    const ExhaustiveScenario& scenario,
    const unordered_map<Vertex, Vertex>& inverseRelabel,
    bool requireTargetLive
) {
    ExhaustiveScenario comparable;
    comparable.RE = scenario.RE;
    comparable.family = scenario.family;
    comparable.label = scenario.label;
    comparable.primitivePlan.primitive = scenario.primitivePlan.primitive;

    if (live_occurrence_exists(scenario.RE, scenario.ctx.targetOcc)) {
        comparable.ctx.targetOcc = scenario.ctx.targetOcc;
    } else if (requireTargetLive) {
        throw runtime_error("metamorphic planner target occurrence disappeared");
    } else {
        comparable.ctx.targetOcc = first_live_occurrence(scenario.RE);
    }

    if (inverseRelabel.empty()) {
        return comparable;
    }

    RebuildTransform transform;
    transform.relabelOrig = inverseRelabel;
    ExhaustiveScenario normalized = rebuild_exhaustive_scenario(comparable, transform);
    normalized.primitivePlan.primitive = comparable.primitivePlan.primitive;
    return normalized;
}

MetamorphicSnapshot run_snapshot(
    const TestOptions& options,
    const ExhaustiveScenario& baseScenario,
    const unordered_map<Vertex, Vertex>& inverseRelabel
) {
    MetamorphicSnapshot snapshot;

    if (baseScenario.primitivePlan.primitive != PrimitiveKind::NONE) {
        ExhaustiveScenario primitiveScenario = baseScenario;
        run_primitive_plan(primitiveScenario);
        require_valid_state(primitiveScenario.RE, options.caseName, primitiveScenario.label + "_primitive");
        const ExhaustiveScenario primitiveComparable =
            normalize_result_for_compare(primitiveScenario, inverseRelabel, false);
        snapshot.primitive = capture_primitive_canonical_signature(
            primitiveComparable.primitivePlan.primitive,
            primitiveComparable.RE
        );
    }

    ExhaustiveScenario plannerScenario = baseScenario;
    const vector<UpdJob>* queue = plannerScenario.initialQueue.empty() ? nullptr : &plannerScenario.initialQueue;
    const PlannerExecutionResult plannerResult = run_planner_checked_capture(
        plannerScenario.RE,
        plannerScenario.ctx,
        plannerScenario.U,
        planner_run_options(options),
        queue,
        plannerScenario.label + "_planner"
    );
    require_valid_state(plannerScenario.RE, options.caseName, plannerScenario.label + "_planner");
    const ExhaustiveScenario plannerComparable =
        normalize_result_for_compare(plannerScenario, inverseRelabel, true);
    snapshot.planner.signature = capture_planner_final_state_canonical_signature(
        plannerComparable.RE,
        plannerComparable.ctx.targetOcc
    );
    snapshot.planner.traceSize = plannerResult.trace.size();
    return snapshot;
}

PlannerSnapshot run_planner_only_snapshot(const TestOptions& options, ExhaustiveScenario scenario) {
    const vector<UpdJob>* queue = scenario.initialQueue.empty() ? nullptr : &scenario.initialQueue;
    const PlannerExecutionResult result = run_planner_checked_capture(
        scenario.RE,
        scenario.ctx,
        scenario.U,
        planner_run_options(options),
        queue,
        scenario.label + "_planner"
    );
    require_valid_state(scenario.RE, options.caseName, scenario.label + "_planner");

    PlannerSnapshot snapshot;
    snapshot.signature = capture_planner_final_state_canonical_signature(scenario.RE, scenario.ctx.targetOcc);
    snapshot.traceSize = result.trace.size();
    return snapshot;
}

void require_same_snapshot(
    const string& caseName,
    const string& label,
    const MetamorphicSnapshot& expected,
    const MetamorphicSnapshot& actual
) {
    if (expected.primitive.has_value() != actual.primitive.has_value()) {
        throw runtime_error(caseName + " primitive presence mismatch label=" + label);
    }
    if (expected.primitive.has_value() && !(*expected.primitive == *actual.primitive)) {
        throw runtime_error(
            caseName + " primitive mismatch label=" + label +
            "\nexpected=" + describe_semantic_state_signature(expected.primitive->finalState) +
            "\nactual=" + describe_semantic_state_signature(actual.primitive->finalState)
        );
    }
    if (!(expected.planner.signature == actual.planner.signature)) {
        throw runtime_error(
            caseName + " planner mismatch label=" + label +
            "\nexpected=" + describe_semantic_state_signature(expected.planner.signature.finalState) +
            "\nactual=" + describe_semantic_state_signature(actual.planner.signature.finalState)
        );
    }
}

void record_stage_hit(
    MatrixStats* stats,
    const string& familyName,
    const string& invariantName,
    const string& stageName
) {
    if (stats == nullptr) {
        return;
    }
    MatrixEntry& entry = stats->entries[{familyName, invariantName}];
    if (stageName == "primitive_only") {
        ++entry.primitiveOnlyHits;
        return;
    }
    if (stageName == "planner_step") {
        ++entry.plannerStepHits;
        return;
    }
    if (stageName == "planner_multistep") {
        ++entry.plannerMultistepHits;
        return;
    }
    if (stageName == "replay_step") {
        ++entry.replayStepHits;
        return;
    }
    throw runtime_error("unknown matrix stage: " + stageName);
}

void write_matrix_stats(
    const TestOptions& options,
    const string& caseName,
    const MatrixStats& stats
) {
    const filesystem::path outPath = artifact_subdir(options, "logs") / (caseName + ".summary.txt");
    ofstream ofs(outPath);
    if (!ofs) {
        throw runtime_error("failed to write metamorphic summary: " + outPath.string());
    }

    for (const auto& [key, entry] : stats.entries) {
        ofs << "family=" << key.first
            << " invariant=" << key.second
            << " primitive_only_hits=" << entry.primitiveOnlyHits
            << " planner_step_hits=" << entry.plannerStepHits
            << " planner_multistep_hits=" << entry.plannerMultistepHits
            << " replay_step_hits=" << entry.replayStepHits
            << '\n';
    }
}

MatrixScenario first_exhaustive_scenario(const TestOptions& options, ExhaustiveFamily family, const string& familyName) {
    TestOptions sample = matrix_sample_options(options);
    vector<ExhaustiveScenario> scenarios = generate_exhaustive_scenarios(sample, family);
    if (scenarios.empty()) {
        throw runtime_error("metamorphic scenario set is empty for family=" + familyName);
    }
    return MatrixScenario{familyName, std::move(scenarios.front()), false};
}

vector<MatrixScenario> primitive_matrix_scenarios(const TestOptions& options) {
    vector<MatrixScenario> out = {
        first_exhaustive_scenario(options, ExhaustiveFamily::SPLIT_READY, "split_ready"),
        first_exhaustive_scenario(options, ExhaustiveFamily::JOIN_READY, "join_ready"),
        first_exhaustive_scenario(options, ExhaustiveFamily::INTEGRATE_READY, "integrate_ready"),
        first_exhaustive_scenario(options, ExhaustiveFamily::MIXED, "mixed"),
        MatrixScenario{
            "split_tie_ready",
            make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_READY, 880101U),
            false,
        },
        MatrixScenario{
            "split_tie_structural",
            make_targeted_planner_exhaustive_scenario(ScenarioFamily::SPLIT_TIE_STRUCTURAL, 880301U),
            false,
        },
    };
    return out;
}

vector<MatrixScenario> planner_matrix_scenarios(const TestOptions& options) {
    vector<MatrixScenario> out = primitive_matrix_scenarios(options);
    out.push_back(MatrixScenario{
        "planner_mixed_targeted",
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::PLANNER_MIXED_TARGETED, 830401U),
        true,
    });
    out.push_back(MatrixScenario{
        "planner_mixed_structural",
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::PLANNER_MIXED_STRUCTURAL, 840503U),
        false,
    });
    out.push_back(MatrixScenario{
        "planner_tie_mixed",
        make_targeted_planner_exhaustive_scenario(ScenarioFamily::PLANNER_TIE_MIXED, 880501U),
        true,
    });
    return out;
}

void run_transform_matrix(
    const TestOptions& options,
    const string& caseName,
    const vector<MatrixScenario>& scenarios,
    const vector<TransformSpec>& transforms,
    MatrixStats* stats
) {
    for (const MatrixScenario& matrixScenario : scenarios) {
        for (const TransformSpec& transformSpec : transforms) {
            const string label =
                matrixScenario.familyName + ":" + transformSpec.invariantName + ":" + matrixScenario.scenario.label;
            const MetamorphicSnapshot baseline = run_snapshot(options, matrixScenario.scenario, {});
            if (matrixScenario.requirePlannerMultistep && baseline.planner.traceSize <= 1U) {
                throw runtime_error(caseName + " expected multi-step planner trace label=" + label);
            }

            const RebuildTransform transform = transformSpec.makeTransform(matrixScenario.scenario);
            const unordered_map<Vertex, Vertex> inverseRelabel = invert_relabel_map(transform.relabelOrig);
            const ExhaustiveScenario rebuilt = rebuild_exhaustive_scenario(matrixScenario.scenario, transform);
            const MetamorphicSnapshot transformed = run_snapshot(options, rebuilt, inverseRelabel);
            require_same_snapshot(caseName, label, baseline, transformed);

            if (baseline.primitive.has_value()) {
                record_stage_hit(stats, matrixScenario.familyName, transformSpec.invariantName, "primitive_only");
            }
            record_stage_hit(
                stats,
                matrixScenario.familyName,
                transformSpec.invariantName,
                baseline.planner.traceSize > 1U ? "planner_multistep" : "planner_step"
            );
        }
    }
}

PlannerStateDump planner_dump_from_scenario(
    const ExhaustiveScenario& scenario,
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

ExhaustiveScenario scenario_from_planner_dump(
    const MatrixScenario& prototype,
    const PlannerStateDump& dump,
    const string& label
) {
    ExhaustiveScenario scenario;
    scenario.RE = dump.engine;
    scenario.ctx.targetOcc = dump.targetOcc;
    scenario.ctx.keepOcc.insert(dump.keepOcc.begin(), dump.keepOcc.end());
    scenario.initialQueue = dump.initialQueue;
    scenario.primitivePlan = prototype.scenario.primitivePlan;
    scenario.family = prototype.scenario.family;
    scenario.label = label;
    return scenario;
}

void run_replay_matrix(
    const TestOptions& options,
    const string& caseName,
    const vector<MatrixScenario>& scenarios,
    MatrixStats* stats
) {
    size_t index = 0U;
    for (const MatrixScenario& matrixScenario : scenarios) {
        const PlannerStateDump originalDump = planner_dump_from_scenario(matrixScenario.scenario, options, caseName);
        const filesystem::path dumpPath =
            artifact_subdir(options, "logs") / (caseName + "_" + to_string(index++) + ".txt");
        save_planner_state_dump(dumpPath, originalDump);
        const PlannerStateDump loadedDump = load_planner_state_dump(dumpPath);
        ExhaustiveScenario loadedScenario =
            scenario_from_planner_dump(matrixScenario, loadedDump, matrixScenario.scenario.label + "_loaded");

        if (!(capture_explorer_state_signature(matrixScenario.scenario) == capture_explorer_state_signature(loadedScenario))) {
            throw runtime_error(caseName + " dump roundtrip changed initial scenario family=" + matrixScenario.familyName);
        }

        const PlannerSnapshot baseline = run_planner_only_snapshot(options, matrixScenario.scenario);
        if (matrixScenario.requirePlannerMultistep && baseline.traceSize <= 1U) {
            throw runtime_error(caseName + " expected multi-step replay baseline family=" + matrixScenario.familyName);
        }
        const PlannerSnapshot replayed = run_planner_only_snapshot(options, loadedScenario);
        if (!(baseline.signature == replayed.signature)) {
            throw runtime_error(
                caseName + " planner replay mismatch family=" + matrixScenario.familyName +
                "\nexpected=" + describe_semantic_state_signature(baseline.signature.finalState) +
                "\nactual=" + describe_semantic_state_signature(replayed.signature.finalState)
            );
        }
        record_stage_hit(stats, matrixScenario.familyName, "replay_serialization", "replay_step");
    }
}

void require_matrix_coverage(
    const string& caseName,
    const MatrixStats& stats,
    const vector<string>& families,
    const vector<string>& invariants,
    bool requirePrimitiveCoverage,
    bool requirePlannerMultistepCoverage,
    bool requireReplayCoverage
) {
    for (const string& familyName : families) {
        for (const string& invariantName : invariants) {
            const auto it = stats.entries.find({familyName, invariantName});
            if (it == stats.entries.end()) {
                throw runtime_error(caseName + " missing matrix entry family=" + familyName + " invariant=" + invariantName);
            }
            if (requirePrimitiveCoverage && it->second.primitiveOnlyHits == 0U &&
                familyName != "planner_mixed_targeted" &&
                familyName != "planner_mixed_structural" &&
                familyName != "planner_tie_mixed" &&
                invariantName != "replay_serialization") {
                throw runtime_error(caseName + " missing primitive coverage family=" + familyName + " invariant=" + invariantName);
            }
            if (requirePlannerMultistepCoverage &&
                (familyName.find("planner_mixed_") == 0 || familyName == "planner_tie_mixed") &&
                invariantName != "replay_serialization" &&
                it->second.plannerMultistepHits == 0U) {
                throw runtime_error(caseName + " missing planner multistep coverage family=" + familyName + " invariant=" + invariantName);
            }
            if (requireReplayCoverage && invariantName == "replay_serialization" && it->second.replayStepHits == 0U) {
                throw runtime_error(caseName + " missing replay coverage family=" + familyName);
            }
        }
    }
}

void run_transform_case(
    const TestOptions& options,
    const string& caseName,
    const TransformSpec& transformSpec
) {
    TestOptions oracleOptions = options;
    oracleOptions.caseName = caseName;
    oracleOptions.oracleMode = OracleMode::ALL;
    set_active_test_options(&oracleOptions);
    set_pending_dump_path(nullopt);

    MatrixStats stats;
    try {
        run_transform_matrix(
            oracleOptions,
            caseName,
            primitive_matrix_scenarios(options),
            {transformSpec},
            &stats
        );
        write_matrix_stats(oracleOptions, caseName, stats);
    } catch (...) {
        write_matrix_stats(oracleOptions, caseName, stats);
        set_active_test_options(&options);
        throw;
    }

    set_active_test_options(&options);
}

void run_replay_case(
    const TestOptions& options,
    const string& caseName,
    const vector<MatrixScenario>& scenarios
) {
    TestOptions oracleOptions = options;
    oracleOptions.caseName = caseName;
    oracleOptions.oracleMode = OracleMode::ALL;
    set_active_test_options(&oracleOptions);
    set_pending_dump_path(nullopt);

    MatrixStats stats;
    try {
        run_replay_matrix(oracleOptions, caseName, scenarios, &stats);
        write_matrix_stats(oracleOptions, caseName, stats);
    } catch (...) {
        write_matrix_stats(oracleOptions, caseName, stats);
        set_active_test_options(&options);
        throw;
    }

    set_active_test_options(&options);
}

} // namespace

void run_metamorphic_relabel_invariance_case(const TestOptions& options) {
    run_transform_case(
        options,
        "metamorphic_relabel_invariance",
        TransformSpec{
            "relabel",
            [](const ExhaustiveScenario& scenario) { return make_relabel_transform(scenario); },
        }
    );
}

void run_metamorphic_occid_invariance_case(const TestOptions& options) {
    run_transform_case(
        options,
        "metamorphic_occid_invariance",
        TransformSpec{
            "occid_renumber",
            [](const ExhaustiveScenario&) { return make_occid_renumber_transform(); },
        }
    );
}

void run_metamorphic_edge_order_invariance_case(const TestOptions& options) {
    run_transform_case(
        options,
        "metamorphic_edge_order_invariance",
        TransformSpec{
            "edge_order",
            [](const ExhaustiveScenario&) { return make_edge_order_transform(); },
        }
    );
}

void run_metamorphic_vertex_order_invariance_case(const TestOptions& options) {
    TestOptions oracleOptions = options;
    oracleOptions.caseName = "metamorphic_vertex_order_invariance";
    oracleOptions.oracleMode = OracleMode::ALL;
    set_active_test_options(&oracleOptions);
    set_pending_dump_path(nullopt);

    MatrixStats stats;
    try {
        run_transform_matrix(
            oracleOptions,
            "metamorphic_vertex_order_invariance",
            primitive_matrix_scenarios(options),
            {
                TransformSpec{
                    "vertex_order",
                    [](const ExhaustiveScenario&) { return make_vertex_order_transform(); },
                },
                TransformSpec{
                    "hosted_occ_order",
                    [](const ExhaustiveScenario&) { return make_hosted_occ_order_transform(); },
                },
            },
            &stats
        );
        write_matrix_stats(oracleOptions, "metamorphic_vertex_order_invariance", stats);
    } catch (...) {
        write_matrix_stats(oracleOptions, "metamorphic_vertex_order_invariance", stats);
        set_active_test_options(&options);
        throw;
    }

    set_active_test_options(&options);
}

void run_replay_serialization_invariance_case(const TestOptions& options) {
    run_replay_case(options, "replay_serialization_invariance", primitive_matrix_scenarios(options));
}

void run_metamorphic_family_matrix_smoke_case(const TestOptions& options) {
    TestOptions oracleOptions = options;
    oracleOptions.caseName = "metamorphic_family_matrix_smoke";
    oracleOptions.oracleMode = OracleMode::ALL;
    set_active_test_options(&oracleOptions);
    set_pending_dump_path(nullopt);

    MatrixStats stats;
    try {
        const vector<MatrixScenario> scenarios = planner_matrix_scenarios(options);
        const vector<TransformSpec> transforms = transform_specs();
        run_transform_matrix(oracleOptions, oracleOptions.caseName, scenarios, transforms, &stats);
        write_matrix_stats(oracleOptions, oracleOptions.caseName, stats);
        vector<string> invariants;
        invariants.reserve(transforms.size());
        for (const TransformSpec& spec : transforms) {
            invariants.push_back(spec.invariantName);
        }
        require_matrix_coverage(
            oracleOptions.caseName,
            stats,
            matrix_family_names(),
            invariants,
            true,
            false,
            false
        );
    } catch (...) {
        write_matrix_stats(oracleOptions, oracleOptions.caseName, stats);
        set_active_test_options(&options);
        throw;
    }

    set_active_test_options(&options);
}

void run_metamorphic_planner_multistep_smoke_case(const TestOptions& options) {
    TestOptions oracleOptions = options;
    oracleOptions.caseName = "metamorphic_planner_multistep_smoke";
    oracleOptions.oracleMode = OracleMode::ALL;
    set_active_test_options(&oracleOptions);
    set_pending_dump_path(nullopt);

    MatrixStats stats;
    try {
        const vector<TransformSpec> transforms = transform_specs();
        vector<MatrixScenario> scenarios = {
            MatrixScenario{
                "planner_mixed_targeted",
                make_targeted_planner_exhaustive_scenario(ScenarioFamily::PLANNER_MIXED_TARGETED, 830401U),
                true,
            },
            MatrixScenario{
                "planner_mixed_structural",
                make_targeted_planner_exhaustive_scenario(ScenarioFamily::PLANNER_MIXED_STRUCTURAL, 840503U),
                false,
            },
            MatrixScenario{
                "planner_tie_mixed",
                make_targeted_planner_exhaustive_scenario(ScenarioFamily::PLANNER_TIE_MIXED, 880501U),
                true,
            },
        };
        run_transform_matrix(oracleOptions, oracleOptions.caseName, scenarios, transforms, &stats);
        write_matrix_stats(oracleOptions, oracleOptions.caseName, stats);
        vector<string> invariants;
        invariants.reserve(transforms.size());
        for (const TransformSpec& spec : transforms) {
            invariants.push_back(spec.invariantName);
        }
        require_matrix_coverage(
            oracleOptions.caseName,
            stats,
            {"planner_mixed_targeted", "planner_tie_mixed"},
            invariants,
            false,
            true,
            false
        );
    } catch (...) {
        write_matrix_stats(oracleOptions, oracleOptions.caseName, stats);
        set_active_test_options(&options);
        throw;
    }

    set_active_test_options(&options);
}

void run_metamorphic_replay_matrix_smoke_case(const TestOptions& options) {
    TestOptions oracleOptions = options;
    oracleOptions.caseName = "metamorphic_replay_matrix_smoke";
    oracleOptions.oracleMode = OracleMode::ALL;
    set_active_test_options(&oracleOptions);
    set_pending_dump_path(nullopt);

    MatrixStats stats;
    try {
        const vector<MatrixScenario> scenarios = planner_matrix_scenarios(options);
        run_replay_matrix(oracleOptions, oracleOptions.caseName, scenarios, &stats);
        write_matrix_stats(oracleOptions, oracleOptions.caseName, stats);
        require_matrix_coverage(
            oracleOptions.caseName,
            stats,
            matrix_family_names(),
            {"replay_serialization"},
            false,
            false,
            true
        );
    } catch (...) {
        write_matrix_stats(oracleOptions, oracleOptions.caseName, stats);
        set_active_test_options(&options);
        throw;
    }

    set_active_test_options(&options);
}

void run_planner_relabel_structural_regression_case(const TestOptions& options) {
    TestOptions oracleOptions = options;
    oracleOptions.caseName = "planner_relabel_structural_regression";
    oracleOptions.oracleMode = OracleMode::ALL;
    set_active_test_options(&oracleOptions);
    set_pending_dump_path(nullopt);

    MatrixStats stats;
    try {
        run_transform_matrix(
            oracleOptions,
            oracleOptions.caseName,
            {
                MatrixScenario{
                    "planner_mixed_structural",
                    make_targeted_planner_exhaustive_scenario(ScenarioFamily::PLANNER_MIXED_STRUCTURAL, 840503U),
                    false,
                },
            },
            {
                TransformSpec{
                    "relabel",
                    [](const ExhaustiveScenario& scenario) { return make_relabel_transform(scenario); },
                },
            },
            &stats
        );
        write_matrix_stats(oracleOptions, oracleOptions.caseName, stats);
    } catch (...) {
        write_matrix_stats(oracleOptions, oracleOptions.caseName, stats);
        set_active_test_options(&options);
        throw;
    }

    set_active_test_options(&options);
}
