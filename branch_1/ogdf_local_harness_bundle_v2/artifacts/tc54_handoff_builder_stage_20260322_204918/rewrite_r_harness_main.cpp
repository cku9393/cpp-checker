#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <variant>
#include <vector>
#include "harness/ogdf_wrapper.hpp"
#include "harness/project_hooks.hpp"
#include "harness/project_static_adapter.hpp"
#include "harness/runners.hpp"

using namespace harness;

namespace {
RunConfig parseArgs(int argc, char **argv) {
    RunConfig cfg;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](const char *name) -> std::string {
            if (i + 1 >= argc) throw std::runtime_error(std::string("missing value for ") + name);
            return argv[++i];
        };
        if (a == "--seed") cfg.seed = std::stoull(next("--seed"));
        else if (a == "--rounds") cfg.rounds = std::stoi(next("--rounds"));
        else if (a == "--tc-index") cfg.tcIndex = std::stoi(next("--tc-index"));
        else if (a == "--target-step") cfg.targetStep = std::stoi(next("--target-step"));
        else if (a == "--manifest") cfg.manifestPath = next("--manifest");
        else if (a == "--case-name") cfg.caseName = next("--case-name");
        else if (a == "--baseline") cfg.baselineMode = next("--baseline");
        else if (a == "--oracle-handoff") cfg.oracleHandoff = next("--oracle-handoff");
        else if (a == "--semantic-stop") cfg.semanticStop = next("--semantic-stop");
        else if (a == "--source-step") cfg.sourceStep = std::stoi(next("--source-step"));
        else if (a == "--source-kind") cfg.sourceKind = next("--source-kind");
        else if (a == "--source-side") cfg.sourceSide = next("--source-side");
        else if (a == "--source") cfg.source = next("--source");
        else if (a == "--stop-before-ogdf") cfg.stopBeforeOgdf = std::stoi(next("--stop-before-ogdf")) != 0;
        else if (a == "--run-child") cfg.runChild = std::stoi(next("--run-child")) != 0;
        else if (a == "--manual-only") cfg.manualOnly = true;
        else if (a == "--mode") cfg.mode = next("--mode");
        else if (a == "--dump-dir") cfg.dumpDir = next("--dump-dir");
        else if (a == "--backend") { (void)next("--backend"); }
        else if (a == "--help") {
            std::cout << "--backend ogdf --mode {static|dummy|rewrite-r|rewrite-r-seq|rewrite-seq|rewrite-r-seq-replay|rewrite-r-seq-regression|rewrite-r-seq-bench|solver-compare|solver-baseline-replay|solver-semantic-replay|solver-semantic-target-replay|solver-semantic-transition-replay|solver-handoff-replay|solver-step-transition-replay|solver-handoff-policy-replay|solver-compare-replay|solver-finalcore-replay|solver-shape-replay|explicit-core-builder-replay|materialize-core-replay|ogdf-raw-crash-replay} "
                         "--seed N --rounds N --tc-index N --target-step N --manifest PATH --case-name NAME --baseline {legacy|oracle|both} --oracle-handoff {delete|normalize} --semantic-stop {raw|canonical|end} --source-step N --source-kind {step|handoff} --source-side {replay|solver|shadow} --source {oracle|rewrite|baseline|auto} --stop-before-ogdf {0|1} --run-child {0|1} --manual-only --dump-dir DIR\n";
            std::exit(0);
        } else {
            throw std::runtime_error("unknown argument: " + a);
        }
    }
    return cfg;
}

using Clock = std::chrono::steady_clock;

struct JsonValue {
    using Object = std::map<std::string, JsonValue>;
    using Array = std::vector<JsonValue>;

    std::variant<std::nullptr_t, bool, int64_t, std::string, Object, Array> data;

    bool isNull() const { return std::holds_alternative<std::nullptr_t>(data); }
    bool isBool() const { return std::holds_alternative<bool>(data); }
    bool isInt() const { return std::holds_alternative<int64_t>(data); }
    bool isString() const { return std::holds_alternative<std::string>(data); }
    bool isObject() const { return std::holds_alternative<Object>(data); }
    bool isArray() const { return std::holds_alternative<Array>(data); }

    const bool &asBool() const { return std::get<bool>(data); }
    const int64_t &asInt() const { return std::get<int64_t>(data); }
    const std::string &asString() const { return std::get<std::string>(data); }
    const Object &asObject() const { return std::get<Object>(data); }
    const Array &asArray() const { return std::get<Array>(data); }
};

class JsonParser {
public:
    explicit JsonParser(std::string_view text) : text_(text) {}

    JsonValue parse() {
        JsonValue value = parseValue();
        skipWs();
        if (pos_ != text_.size()) {
            throw std::runtime_error("unexpected trailing characters in manifest JSON");
        }
        return value;
    }

private:
    void skipWs() {
        while (pos_ < text_.size() &&
               std::isspace(static_cast<unsigned char>(text_[pos_])) != 0) {
            ++pos_;
        }
    }

    char peek() const {
        if (pos_ >= text_.size()) return '\0';
        return text_[pos_];
    }

    char take() {
        if (pos_ >= text_.size()) {
            throw std::runtime_error("unexpected end of manifest JSON");
        }
        return text_[pos_++];
    }

    void expect(char c) {
        skipWs();
        if (take() != c) {
            throw std::runtime_error(std::string("expected '") + c + "' in manifest JSON");
        }
    }

    JsonValue parseValue() {
        skipWs();
        switch (peek()) {
        case '{':
            return JsonValue{parseObject()};
        case '[':
            return JsonValue{parseArray()};
        case '"':
            return JsonValue{parseString()};
        case 't':
            parseLiteral("true");
            return JsonValue{true};
        case 'f':
            parseLiteral("false");
            return JsonValue{false};
        case 'n':
            parseLiteral("null");
            return JsonValue{nullptr};
        default:
            if (peek() == '-' || std::isdigit(static_cast<unsigned char>(peek())) != 0) {
                return JsonValue{parseInt()};
            }
            throw std::runtime_error("unexpected token in manifest JSON");
        }
    }

    JsonValue::Object parseObject() {
        JsonValue::Object out;
        expect('{');
        skipWs();
        if (peek() == '}') {
            ++pos_;
            return out;
        }
        while (true) {
            skipWs();
            const std::string key = parseString();
            expect(':');
            out.emplace(key, parseValue());
            skipWs();
            if (peek() == '}') {
                ++pos_;
                return out;
            }
            expect(',');
        }
    }

    JsonValue::Array parseArray() {
        JsonValue::Array out;
        expect('[');
        skipWs();
        if (peek() == ']') {
            ++pos_;
            return out;
        }
        while (true) {
            out.push_back(parseValue());
            skipWs();
            if (peek() == ']') {
                ++pos_;
                return out;
            }
            expect(',');
        }
    }

    std::string parseString() {
        expect('"');
        std::string out;
        while (true) {
            const char c = take();
            if (c == '"') break;
            if (c == '\\') {
                const char esc = take();
                switch (esc) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                default:
                    throw std::runtime_error("unsupported escape in manifest JSON");
                }
                continue;
            }
            out.push_back(c);
        }
        return out;
    }

    int64_t parseInt() {
        skipWs();
        const size_t start = pos_;
        if (peek() == '-') ++pos_;
        if (std::isdigit(static_cast<unsigned char>(peek())) == 0) {
            throw std::runtime_error("expected integer in manifest JSON");
        }
        while (std::isdigit(static_cast<unsigned char>(peek())) != 0) ++pos_;
        return std::stoll(std::string(text_.substr(start, pos_ - start)));
    }

    void parseLiteral(std::string_view literal) {
        if (text_.substr(pos_, literal.size()) != literal) {
            throw std::runtime_error("invalid literal in manifest JSON");
        }
        pos_ += literal.size();
    }

    std::string_view text_;
    size_t pos_ = 0;
};

struct RegressionCaseSpec {
    std::string name;
    uint64_t seed = 1;
    int tcIndex = -1;
    int targetStep = -1;
    bool expectedTopLevelOk = true;
    bool expectedActualInvariantOk = true;
    bool expectedOracleEquivalentOk = true;
    std::optional<std::string> expectedPostcheckSubtype;
    std::vector<std::string> expectedSpecialPathTags;
    std::optional<ExplicitBlockGraph> inputExplicit;
};

struct RegressionCaseResult {
    std::string name;
    uint64_t seed = 1;
    int tcIndex = -1;
    int targetStep = -1;
    bool passed = false;
    double elapsedMs = 0.0;
    std::string dumpPath;
    std::vector<std::string> failures;
};

OracleHandoffPolicy parseOracleHandoffPolicyArg(const std::string &value) {
    if (value == "delete") {
        return OracleHandoffPolicy::OHP_DELETE_EXPLICIT;
    }
    if (value == "normalize") {
        return OracleHandoffPolicy::OHP_NORMALIZE_EXPLICIT;
    }
    throw std::runtime_error("unknown --oracle-handoff value: " + value);
}

struct SolverCompareCaseResult {
    std::string name;
    uint64_t seed = 1;
    int tcIndex = -1;
    std::optional<int> targetStep;
    bool passed = false;
    bool legacyOk = false;
    bool oracleOk = false;
    bool rewriteSeqOk = false;
    std::optional<bool> legacyVsRewriteRawExplicitEquivalent;
    std::optional<bool> oracleVsRewriteRawExplicitEquivalent;
    std::optional<bool> legacyVsOracleRawExplicitEquivalent;
    std::optional<bool> legacyVsRewriteCanonicalExplicitEquivalent;
    std::optional<bool> oracleVsRewriteCanonicalExplicitEquivalent;
    std::optional<bool> legacyVsOracleCanonicalExplicitEquivalent;
    std::optional<bool> legacyVsRewriteEquivalent;
    std::optional<bool> oracleVsRewriteEquivalent;
    std::optional<bool> legacyVsOracleEquivalent;
    std::optional<bool> legacyVsRewriteParentEquivalent;
    std::optional<bool> oracleVsRewriteParentEquivalent;
    std::optional<bool> legacyVsOracleParentEquivalent;
    double legacyElapsedMs = 0.0;
    double oracleElapsedMs = 0.0;
    double rewriteSeqElapsedMs = 0.0;
    std::string dumpPath;
    std::vector<std::string> failures;
};

std::string readTextFile(const std::string &path) {
    std::ifstream ifs(path);
    if (!ifs) {
        throw std::runtime_error("failed to open file: " + path);
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

std::string jsonEscape(const std::string &s) {
    std::ostringstream oss;
    for (const char c : s) {
        switch (c) {
        case '\\': oss << "\\\\"; break;
        case '"': oss << "\\\""; break;
        case '\n': oss << "\\n"; break;
        case '\r': oss << "\\r"; break;
        case '\t': oss << "\\t"; break;
        default: oss << c; break;
        }
    }
    return oss.str();
}

const JsonValue *findField(const JsonValue::Object &obj, const std::string &key) {
    const auto it = obj.find(key);
    return it == obj.end() ? nullptr : &it->second;
}

std::string requiredStringField(const JsonValue::Object &obj, const std::string &key) {
    const JsonValue *value = findField(obj, key);
    if (!value || !value->isString()) {
        throw std::runtime_error("manifest case missing string field: " + key);
    }
    return value->asString();
}

int requiredIntField(const JsonValue::Object &obj, const std::string &key) {
    const JsonValue *value = findField(obj, key);
    if (!value || !value->isInt()) {
        throw std::runtime_error("manifest case missing integer field: " + key);
    }
    return static_cast<int>(value->asInt());
}

bool optionalBoolField(const JsonValue::Object &obj,
                       const std::string &key,
                       bool defaultValue) {
    const JsonValue *value = findField(obj, key);
    if (!value) return defaultValue;
    if (!value->isBool()) {
        throw std::runtime_error("manifest case field must be bool: " + key);
    }
    return value->asBool();
}

std::optional<std::string> optionalStringField(const JsonValue::Object &obj,
                                               const std::string &key) {
    const JsonValue *value = findField(obj, key);
    if (!value || value->isNull()) return std::nullopt;
    if (!value->isString()) {
        throw std::runtime_error("manifest case field must be string: " + key);
    }
    return value->asString();
}

std::vector<std::string> optionalStringListField(const JsonValue::Object &obj,
                                                 const std::string &key) {
    std::vector<std::string> out;
    const JsonValue *value = findField(obj, key);
    if (!value || value->isNull()) return out;
    if (value->isString()) {
        out.push_back(value->asString());
        return out;
    }
    if (!value->isArray()) {
        throw std::runtime_error("manifest case field must be string or array: " + key);
    }
    for (const auto &item : value->asArray()) {
        if (!item.isString()) {
            throw std::runtime_error("manifest case string list contains non-string item: " + key);
        }
        out.push_back(item.asString());
    }
    return out;
}

ExplicitBlockGraph normalizeExplicitBlockGraph(ExplicitBlockGraph G) {
    std::sort(G.vertices.begin(), G.vertices.end());
    G.vertices.erase(std::unique(G.vertices.begin(), G.vertices.end()), G.vertices.end());
    for (auto &edge : G.edges) {
        if (edge.u > edge.v) std::swap(edge.u, edge.v);
    }
    std::sort(G.edges.begin(),
              G.edges.end(),
              [](const ExplicitEdge &lhs, const ExplicitEdge &rhs) {
                  return std::tie(lhs.id, lhs.u, lhs.v) <
                         std::tie(rhs.id, rhs.u, rhs.v);
              });
    return G;
}

ExplicitBlockGraph parseExplicitBlockGraphValue(const JsonValue &value) {
    if (!value.isObject()) {
        throw std::runtime_error("inputExplicit must be an object");
    }
    const auto &obj = value.asObject();
    const JsonValue *verticesValue = findField(obj, "vertices");
    const JsonValue *edgesValue = findField(obj, "edges");
    if (!verticesValue || !verticesValue->isArray()) {
        throw std::runtime_error("inputExplicit.vertices must be an array");
    }
    if (!edgesValue || !edgesValue->isArray()) {
        throw std::runtime_error("inputExplicit.edges must be an array");
    }

    ExplicitBlockGraph G;
    for (const auto &item : verticesValue->asArray()) {
        if (!item.isInt()) {
            throw std::runtime_error("inputExplicit.vertices contains non-integer");
        }
        G.vertices.push_back(static_cast<int>(item.asInt()));
    }
    for (const auto &item : edgesValue->asArray()) {
        if (!item.isObject()) {
            throw std::runtime_error("inputExplicit.edges contains non-object");
        }
        const auto &edgeObj = item.asObject();
        ExplicitEdge edge;
        edge.id = requiredIntField(edgeObj, "id");
        edge.u = requiredIntField(edgeObj, "u");
        edge.v = requiredIntField(edgeObj, "v");
        G.edges.push_back(edge);
    }
    return normalizeExplicitBlockGraph(std::move(G));
}

std::vector<RegressionCaseSpec> loadRegressionManifest(const std::string &path) {
    const JsonValue root = JsonParser(readTextFile(path)).parse();
    const JsonValue::Array *cases = nullptr;
    if (root.isArray()) {
        cases = &root.asArray();
    } else if (root.isObject()) {
        const JsonValue *value = findField(root.asObject(), "cases");
        if (!value || !value->isArray()) {
            throw std::runtime_error("manifest top-level object must contain array field: cases");
        }
        cases = &value->asArray();
    } else {
        throw std::runtime_error("manifest must be a JSON array or object with cases");
    }

    std::vector<RegressionCaseSpec> out;
    out.reserve(cases->size());
    for (const auto &item : *cases) {
        if (!item.isObject()) {
            throw std::runtime_error("manifest case must be an object");
        }
        const auto &obj = item.asObject();
        RegressionCaseSpec spec;
        spec.name = requiredStringField(obj, "name");
        spec.seed = static_cast<uint64_t>(requiredIntField(obj, "seed"));
        spec.tcIndex = requiredIntField(obj, "tcIndex");
        spec.targetStep = requiredIntField(obj, "targetStep");
        spec.expectedTopLevelOk =
            optionalBoolField(obj, "expectedTopLevelOk", true);
        spec.expectedActualInvariantOk =
            optionalBoolField(obj, "expectedActualInvariantOk", true);
        spec.expectedOracleEquivalentOk =
            optionalBoolField(obj, "expectedOracleEquivalentOk", true);
        spec.expectedPostcheckSubtype =
            optionalStringField(obj, "expectedPostcheckSubtype");
        spec.expectedSpecialPathTags =
            optionalStringListField(obj, "expectedSpecialPathTags");
        if (spec.expectedSpecialPathTags.empty()) {
            spec.expectedSpecialPathTags =
                optionalStringListField(obj, "expectedSpecialPathTag");
        }
        if (const JsonValue *inputExplicit = findField(obj, "inputExplicit");
            inputExplicit != nullptr && !inputExplicit->isNull()) {
            spec.inputExplicit = parseExplicitBlockGraphValue(*inputExplicit);
        }
        out.push_back(std::move(spec));
    }
    return out;
}

std::string sanitizePathComponent(std::string value) {
    for (char &ch : value) {
        const unsigned char uch = static_cast<unsigned char>(ch);
        if (std::isalnum(uch) != 0 || ch == '-' || ch == '_' || ch == '.') continue;
        ch = '_';
    }
    if (value.empty()) value = "case";
    return value;
}

uint64_t statDeltaForSpecialPathTag(const RewriteRStats &before,
                                    const RewriteRStats &after,
                                    const std::string &tag,
                                    std::string &why) {
    auto delta = [](uint64_t lhs, uint64_t rhs) -> uint64_t {
        return rhs >= lhs ? (rhs - lhs) : 0;
    };
    auto pathDelta = [&](RewritePathTaken path) -> uint64_t {
        const size_t idx = static_cast<size_t>(path);
        return delta(before.rewritePathTakenCounts[idx], after.rewritePathTakenCounts[idx]);
    };

    if (tag == "DIRECT_SPQR") return pathDelta(RewritePathTaken::DIRECT_SPQR);
    if (tag == "SPECIAL_SINGLE_CUT") return pathDelta(RewritePathTaken::SPECIAL_SINGLE_CUT);
    if (tag == "SPECIAL_ONE_EDGE") return pathDelta(RewritePathTaken::SPECIAL_ONE_EDGE);
    if (tag == "SPECIAL_TWO_PATH") return pathDelta(RewritePathTaken::SPECIAL_TWO_PATH);
    if (tag == "SPECIAL_PATH") return pathDelta(RewritePathTaken::SPECIAL_PATH);
    if (tag == "SPECIAL_LOOP_SHARED") return pathDelta(RewritePathTaken::SPECIAL_LOOP_SHARED);
    if (tag == "SPECIAL_SELF_LOOP_TWO_PATH") {
        return pathDelta(RewritePathTaken::SPECIAL_SELF_LOOP_TWO_PATH);
    }
    if (tag == "SPECIAL_SELF_LOOP_SPQR_READY") {
        return pathDelta(RewritePathTaken::SPECIAL_SELF_LOOP_SPQR_READY);
    }
    if (tag == "SPECIAL_SELF_LOOP_ONE_EDGE") {
        return pathDelta(RewritePathTaken::SPECIAL_SELF_LOOP_ONE_EDGE);
    }
    if (tag == "SEQ_SP_CLEANUP") {
        return delta(before.seqSameTypeSPCleanupSuccessCount,
                     after.seqSameTypeSPCleanupSuccessCount);
    }
    if (tag == "SEQ_ADJ_REPAIR") {
        return delta(before.seqAdjRepairUsedCount, after.seqAdjRepairUsedCount);
    }
    if (tag == "SEQ_CLEAR_PRESERVE") {
        return delta(before.seqClearPreserveRequestedCount,
                     after.seqClearPreserveRequestedCount);
    }
    if (tag == "SEQ_XSHARED_PROXY_LOOP_REAL") {
        return delta(before.seqXSharedLoopSharedProxyLoopRealHandledCount,
                     after.seqXSharedLoopSharedProxyLoopRealHandledCount);
    }
    if (tag == "SEQ_ONE_EDGE_REAL_NONLOOP") {
        return delta(before.seqTooSmallOneEdgeRealNonLoopHandledCount,
                     after.seqTooSmallOneEdgeRealNonLoopHandledCount);
    }
    if (tag == "SEQ_XINCIDENT_ONE_EDGE") {
        return delta(before.seqXIncidentOneEdgeHandledCount,
                     after.seqXIncidentOneEdgeHandledCount);
    }
    if (tag == "SEQ_XINCIDENT_SPQR_READY") {
        return delta(before.seqXIncidentSpqrReadyHandledCount,
                     after.seqXIncidentSpqrReadyHandledCount);
    }
    why = "unknown expectedSpecialPathTag: " + tag;
    return 0;
}
}

int main(int argc, char **argv) {
    try {
        RunConfig cfg = parseArgs(argc, argv);
        std::filesystem::create_directories(cfg.dumpDir);

        OgdfRawSpqrBackend backend;
        ProjectHarnessOps ops;
        if (cfg.mode == "rewrite-r" ||
            cfg.mode == "rewrite-r-seq" ||
            cfg.mode == "rewrite-seq" ||
            cfg.mode == "rewrite-r-seq-replay" ||
            cfg.mode == "rewrite-r-seq-regression" ||
            cfg.mode == "rewrite-r-seq-bench" ||
            cfg.mode == "solver-compare" ||
            cfg.mode == "solver-baseline-replay" ||
            cfg.mode == "solver-semantic-replay" ||
            cfg.mode == "solver-semantic-target-replay" ||
            cfg.mode == "solver-semantic-transition-replay" ||
            cfg.mode == "solver-handoff-replay" ||
            cfg.mode == "solver-step-transition-replay" ||
            cfg.mode == "solver-handoff-policy-replay" ||
            cfg.mode == "solver-compare-replay" ||
            cfg.mode == "solver-finalcore-replay" ||
            cfg.mode == "solver-shape-replay" ||
            cfg.mode == "explicit-core-builder-replay" ||
            cfg.mode == "materialize-core-replay" ||
            cfg.mode == "ogdf-raw-crash-replay") {
            resetRewriteRStats();
        }

        int tc = 0;
        auto printRewriteStats = [&]() {
            if (cfg.mode != "rewrite-r" &&
                cfg.mode != "rewrite-r-seq" &&
                cfg.mode != "rewrite-seq" &&
                cfg.mode != "rewrite-r-seq-replay" &&
                cfg.mode != "rewrite-r-seq-regression" &&
                cfg.mode != "rewrite-r-seq-bench" &&
                cfg.mode != "solver-compare" &&
                cfg.mode != "solver-baseline-replay" &&
                cfg.mode != "solver-semantic-replay" &&
                cfg.mode != "solver-semantic-target-replay" &&
                cfg.mode != "solver-semantic-transition-replay" &&
                cfg.mode != "solver-handoff-replay" &&
                cfg.mode != "solver-step-transition-replay" &&
                cfg.mode != "solver-handoff-policy-replay" &&
                cfg.mode != "solver-compare-replay" &&
                cfg.mode != "solver-finalcore-replay" &&
                cfg.mode != "solver-shape-replay" &&
                cfg.mode != "explicit-core-builder-replay" &&
                cfg.mode != "materialize-core-replay" &&
                cfg.mode != "ogdf-raw-crash-replay") {
                return;
            }
            const RewriteRStats stats = getRewriteRStats();
            std::cout << "[REWRITE_R_STATS] "
                      << "rewriteCalls=" << stats.rewriteCalls
                      << " rewriteSeqCalls=" << stats.rewriteSeqCalls
                      << " rewriteSeqSucceededCases=" << stats.rewriteSeqSucceededCases
                      << " rewriteSeqFailedCases=" << stats.rewriteSeqFailedCases
                      << " rewriteSeqMaxStepReachedCount=" << stats.rewriteSeqMaxStepReachedCount
                      << " compactReadyCount=" << stats.compactReadyCount
                      << " compactRejectedFallbackCount=" << stats.compactRejectedFallbackCount
                      << " backendBuildRawDirectCount=" << stats.backendBuildRawDirectCount
                      << " backendBuildRawFallbackCount=" << stats.backendBuildRawFallbackCount
                      << " compareDirectRawCallCount=" << stats.compareDirectRawCallCount
                      << " compareDirectRawBlockedCount=" << stats.compareDirectRawBlockedCount
                      << " compareSharedDispatchFallbackCount="
                      << stats.compareSharedDispatchFallbackCount
                      << " solverShadowResyncAttemptCount="
                      << stats.solverShadowResyncAttemptCount
                      << " solverShadowResyncAppliedCount="
                      << stats.solverShadowResyncAppliedCount
                      << " solverShadowResyncNoopCount="
                      << stats.solverShadowResyncNoopCount
                      << " solverShadowResyncNoTargetToHasTargetCount="
                      << stats.solverShadowResyncNoTargetToHasTargetCount
                      << " solverShadowResyncAliveRSetDifferCount="
                      << stats.solverShadowResyncAliveRSetDifferCount
                      << " compactSingleCutTwoBlocksHandled=" << stats.compactSingleCutTwoBlocksHandled
                      << " compactPathOfBlocksHandled=" << stats.compactPathOfBlocksHandled
                      << " compactTooSmallHandledCount=" << stats.compactTooSmallHandledCount
                      << " compactTooSmallTwoPathHandledCount=" << stats.compactTooSmallTwoPathHandledCount
                      << " rewriteFallbackWholeCoreCount=" << stats.rewriteFallbackWholeCoreCount
                      << " rewriteFallbackSpecialCaseCount=" << stats.rewriteFallbackSpecialCaseCount
                      << " seqProxyMetadataFallbackCount=" << stats.seqProxyMetadataFallbackCount
                      << " seqGraftRewireFallbackCount=" << stats.seqGraftRewireFallbackCount
                      << " seqRewriteWholeCoreFallbackCount=" << stats.seqRewriteWholeCoreFallbackCount
                      << " seqFallbackCaseCount=" << stats.seqFallbackCaseCount
                      << " seqResolvedProxySnapshotCount="
                      << stats.seqResolvedProxySnapshotCount
                      << " seqResolvedProxySnapshotFailCount="
                      << stats.seqResolvedProxySnapshotFailCount
                      << " seqResolvedProxyRepairUsedCount="
                      << stats.seqResolvedProxyRepairUsedCount
                      << " seqClearPreserveRequestedCount="
                      << stats.seqClearPreserveRequestedCount
                      << " seqClearPreserveArcCount="
                      << stats.seqClearPreserveArcCount
                      << " seqClearPreserveCrossNodeRewireCount="
                      << stats.seqClearPreserveCrossNodeRewireCount
                      << " seqClearPreserveSameNodeRehomeCount="
                      << stats.seqClearPreserveSameNodeRehomeCount
                      << " seqClearPreserveFallbackCount="
                      << stats.seqClearPreserveFallbackCount
                      << " seqAdjRepairUsedCount="
                      << stats.seqAdjRepairUsedCount
                      << " seqAdjRepairAffectedNodeCount="
                      << stats.seqAdjRepairAffectedNodeCount
                      << " seqAdjRepairOldNodeCount="
                      << stats.seqAdjRepairOldNodeCount
                      << " seqAdjRepairOutsideNodeCount="
                      << stats.seqAdjRepairOutsideNodeCount
                      << " seqResolvedOldArcRepairAttemptCount="
                      << stats.seqResolvedOldArcRepairAttemptCount
                      << " seqResolvedOldArcRepairSuccessCount="
                      << stats.seqResolvedOldArcRepairSuccessCount
                      << " seqResolvedOldArcRepairFailCount="
                      << stats.seqResolvedOldArcRepairFailCount
                      << " seqResolvedOldArcRepairUsedCount="
                      << stats.seqResolvedOldArcRepairUsedCount
                      << " seqTooSmallOtherHandledCount=" << stats.seqTooSmallOtherHandledCount
                      << " seqTooSmallOneEdgeHandledCount="
                      << stats.seqTooSmallOneEdgeHandledCount
                      << " seqTooSmallOneEdgeRealNonLoopHandledCount="
                      << stats.seqTooSmallOneEdgeRealNonLoopHandledCount
                      << " seqTooSmallOneEdgeFallbackCount="
                      << stats.seqTooSmallOneEdgeFallbackCount
                      << " seqLoopPlusEdgeSharedHandledCount=" << stats.seqLoopPlusEdgeSharedHandledCount
                      << " seqSelfLoopRemainderTwoPathHandledCount="
                      << stats.seqSelfLoopRemainderTwoPathHandledCount
                      << " seqSelfLoopRemainderSpqrReadyAttemptCount="
                      << stats.seqSelfLoopRemainderSpqrReadyAttemptCount
                      << " seqSelfLoopRemainderSpqrReadyHandledCount="
                      << stats.seqSelfLoopRemainderSpqrReadyHandledCount
                      << " seqSelfLoopRemainderSpqrReadyFallbackCount="
                      << stats.seqSelfLoopRemainderSpqrReadyFallbackCount
                      << " seqSelfLoopRemainderOneEdgeAttemptCount="
                      << stats.seqSelfLoopRemainderOneEdgeAttemptCount
                      << " seqSelfLoopRemainderOneEdgeHandledCount="
                      << stats.seqSelfLoopRemainderOneEdgeHandledCount
                      << " seqSelfLoopRemainderOneEdgeFallbackCount="
                      << stats.seqSelfLoopRemainderOneEdgeFallbackCount
                      << " seqXIncidentSharedWithLoopHandledCount="
                      << stats.seqXIncidentSharedWithLoopHandledCount
                      << " seqXSharedLoopSharedProxyLoopRealAttemptCount="
                      << stats.seqXSharedLoopSharedProxyLoopRealAttemptCount
                      << " seqXSharedLoopSharedProxyLoopRealHandledCount="
                      << stats.seqXSharedLoopSharedProxyLoopRealHandledCount
                      << " seqXSharedLoopSharedProxyLoopRealFallbackCount="
                      << stats.seqXSharedLoopSharedProxyLoopRealFallbackCount
                      << " seqXIncidentSpqrReadyAttemptCount="
                      << stats.seqXIncidentSpqrReadyAttemptCount
                      << " seqXIncidentSpqrReadyHandledCount="
                      << stats.seqXIncidentSpqrReadyHandledCount
                      << " seqXIncidentSpqrReadyFallbackCount="
                      << stats.seqXIncidentSpqrReadyFallbackCount
                      << " seqXIncidentOneEdgeHandledCount="
                      << stats.seqXIncidentOneEdgeHandledCount
                      << " seqXIncidentOneEdgeRealHandledCount="
                      << stats.seqXIncidentOneEdgeRealHandledCount
                      << " seqXIncidentOneEdgeUnsupportedProxyCount="
                      << stats.seqXIncidentOneEdgeUnsupportedProxyCount
                      << " rewriteManualPassCount=" << stats.rewriteManualPassCount
                      << " rewriteRandomPassCount=" << stats.rewriteRandomPassCount
                      << "\n";
            std::cout << "[REWRITE_R_REJECT_COUNTS]";
            for (size_t i = 0; i < kCompactRejectReasonCount; ++i) {
                const auto reason = static_cast<CompactRejectReason>(i);
                std::cout << ' ' << compactRejectReasonName(reason)
                          << '=' << stats.compactRejectReasonCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_REJECT_DUMPS]";
            for (size_t i = 0; i < kCompactRejectReasonCount; ++i) {
                const auto reason = static_cast<CompactRejectReason>(i);
                std::cout << ' ' << compactRejectReasonName(reason)
                          << '='
                          << (stats.firstRejectDumpPaths[i].empty() ? "-" : stats.firstRejectDumpPaths[i]);
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_NB_SUBTYPE_COUNTS]";
            for (size_t i = 0; i < kNotBiconnectedSubtypeCount; ++i) {
                const auto subtype = static_cast<NotBiconnectedSubtype>(i);
                std::cout << ' ' << notBiconnectedSubtypeName(subtype)
                          << '=' << stats.compactNotBiconnectedSubtypeCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_NB_SUBTYPE_DUMPS]";
            for (size_t i = 0; i < kNotBiconnectedSubtypeCount; ++i) {
                const auto subtype = static_cast<NotBiconnectedSubtype>(i);
                std::cout << ' ' << notBiconnectedSubtypeName(subtype)
                          << '='
                          << (stats.firstNotBiconnectedSubtypeDumpPaths[i].empty()
                                  ? "-"
                                  : stats.firstNotBiconnectedSubtypeDumpPaths[i]);
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_TS_SUBTYPE_COUNTS]";
            for (size_t i = 0; i < kTooSmallSubtypeCount; ++i) {
                const auto subtype = static_cast<TooSmallSubtype>(i);
                std::cout << ' ' << tooSmallSubtypeName(subtype)
                          << '=' << stats.compactTooSmallSubtypeCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_TS_SUBTYPE_DUMPS]";
            for (size_t i = 0; i < kTooSmallSubtypeCount; ++i) {
                const auto subtype = static_cast<TooSmallSubtype>(i);
                std::cout << ' ' << tooSmallSubtypeName(subtype)
                          << '='
                          << (stats.firstTooSmallSubtypeDumpPaths[i].empty()
                                  ? "-"
                                  : stats.firstTooSmallSubtypeDumpPaths[i]);
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_TS_SUBTYPE_COUNTS]";
            for (size_t i = 0; i < kTooSmallSubtypeCount; ++i) {
                const auto subtype = static_cast<TooSmallSubtype>(i);
                std::cout << ' ' << tooSmallSubtypeName(subtype)
                          << '=' << stats.seqTooSmallSubtypeCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_TSO_SUBTYPE_COUNTS]";
            for (size_t i = 0; i < kTooSmallOtherSubtypeCount; ++i) {
                const auto subtype = static_cast<TooSmallOtherSubtype>(i);
                std::cout << ' ' << tooSmallOtherSubtypeName(subtype)
                          << '=' << stats.seqTooSmallOtherSubtypeCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_TSO_CASE_COUNTS]";
            for (size_t i = 0; i < kTooSmallOtherSubtypeCount; ++i) {
                const auto subtype = static_cast<TooSmallOtherSubtype>(i);
                std::cout << ' ' << tooSmallOtherSubtypeName(subtype)
                          << '=' << stats.seqTooSmallCaseCountsBySubtype[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_TSO_DUMPS]";
            for (size_t i = 0; i < kTooSmallOtherSubtypeCount; ++i) {
                const auto subtype = static_cast<TooSmallOtherSubtype>(i);
                std::cout << ' ' << tooSmallOtherSubtypeName(subtype)
                          << '='
                          << (stats.firstTooSmallOtherDumpPaths[i].empty()
                                  ? "-"
                                  : stats.firstTooSmallOtherDumpPaths[i]);
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_ONEEDGE_SUBTYPE_COUNTS]";
            for (size_t i = 0; i < kSequenceOneEdgeSubtypeCount; ++i) {
                const auto subtype = static_cast<SequenceOneEdgeSubtype>(i);
                std::cout << ' ' << sequenceOneEdgeSubtypeName(subtype)
                          << '=' << stats.seqTooSmallOneEdgeSubtypeCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_ONEEDGE_CASE_COUNTS]";
            for (size_t i = 0; i < kSequenceOneEdgeSubtypeCount; ++i) {
                const auto subtype = static_cast<SequenceOneEdgeSubtype>(i);
                std::cout << ' ' << sequenceOneEdgeSubtypeName(subtype)
                          << '=' << stats.seqTooSmallOneEdgeCaseCountsBySubtype[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_ONEEDGE_DUMPS]";
            for (size_t i = 0; i < kSequenceOneEdgeSubtypeCount; ++i) {
                const auto subtype = static_cast<SequenceOneEdgeSubtype>(i);
                std::cout << ' ' << sequenceOneEdgeSubtypeName(subtype)
                          << '='
                          << (stats.firstTooSmallOneEdgeDumpPaths[i].empty()
                                  ? "-"
                                  : stats.firstTooSmallOneEdgeDumpPaths[i]);
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_FALLBACK_STEP_COUNTS]";
            for (size_t i = 0; i < kRewriteSeqTrackedSteps; ++i) {
                std::cout << " step" << i << '=' << stats.seqFallbackAtStepCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_FALLBACK_REASON_COUNTS]";
            for (size_t i = 0; i < kSeqFallbackReasonCount; ++i) {
                const auto reason = static_cast<SeqFallbackReason>(i);
                std::cout << ' ' << seqFallbackReasonName(reason)
                          << '=' << stats.seqFallbackReasonCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_FALLBACK_REASON_DUMPS]";
            for (size_t i = 0; i < kSeqFallbackReasonCount; ++i) {
                const auto reason = static_cast<SeqFallbackReason>(i);
                std::cout << ' ' << seqFallbackReasonName(reason)
                          << '='
                          << (stats.firstSeqFallbackDumpPaths[i].empty()
                                  ? "-"
                                  : stats.firstSeqFallbackDumpPaths[i]);
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_FALLBACK_TRIGGER_COUNTS]";
            for (size_t i = 0; i < kRewriteFallbackTriggerCount; ++i) {
                const auto trigger = static_cast<RewriteFallbackTrigger>(i);
                std::cout << ' ' << rewriteFallbackTriggerName(trigger)
                          << '=' << stats.rewriteFallbackTriggerCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_FALLBACK_TRIGGER_CASE_COUNTS]";
            for (size_t i = 0; i < kRewriteFallbackTriggerCount; ++i) {
                const auto trigger = static_cast<RewriteFallbackTrigger>(i);
                std::cout << ' ' << rewriteFallbackTriggerName(trigger)
                          << '=' << stats.rewriteFallbackCaseCountsByTrigger[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_FALLBACK_TRIGGER_DUMPS]";
            for (size_t i = 0; i < kRewriteFallbackTriggerCount; ++i) {
                const auto trigger = static_cast<RewriteFallbackTrigger>(i);
                std::cout << ' ' << rewriteFallbackTriggerName(trigger)
                          << '='
                          << (stats.firstFallbackTriggerDumpPaths[i].empty()
                                  ? "-"
                                  : stats.firstFallbackTriggerDumpPaths[i]);
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_FALLBACK_TRIGGER_STEP_COUNTS]";
            for (size_t step = 0; step < kRewriteSeqTrackedSteps; ++step) {
                for (size_t i = 0; i < kRewriteFallbackTriggerCount; ++i) {
                    if (stats.rewriteFallbackTriggerAtStepCounts[step][i] == 0) continue;
                    const auto trigger = static_cast<RewriteFallbackTrigger>(i);
                    std::cout << ' ' << "step" << step << '_'
                              << rewriteFallbackTriggerName(trigger)
                              << '='
                              << stats.rewriteFallbackTriggerAtStepCounts[step][i];
                }
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_BUILDFAIL_SUBTYPE_COUNTS]";
            for (size_t i = 0; i < kCompactBuildFailSubtypeCount; ++i) {
                const auto subtype = static_cast<CompactBuildFailSubtype>(i);
                std::cout << ' ' << compactBuildFailSubtypeName(subtype)
                          << '=' << stats.seqCompactBuildFailSubtypeCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_BUILDFAIL_SUBTYPE_CASE_COUNTS]";
            for (size_t i = 0; i < kCompactBuildFailSubtypeCount; ++i) {
                const auto subtype = static_cast<CompactBuildFailSubtype>(i);
                std::cout << ' ' << compactBuildFailSubtypeName(subtype)
                          << '=' << stats.seqCompactBuildFailCaseCountsBySubtype[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_BUILDFAIL_SUBTYPE_DUMPS]";
            for (size_t i = 0; i < kCompactBuildFailSubtypeCount; ++i) {
                const auto subtype = static_cast<CompactBuildFailSubtype>(i);
                std::cout << ' ' << compactBuildFailSubtypeName(subtype)
                          << '='
                          << (stats.firstCompactBuildFailDumpPaths[i].empty()
                                  ? "-"
                                  : stats.firstCompactBuildFailDumpPaths[i]);
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_BUILDFAIL_SUBTYPE_STEP_COUNTS]";
            for (size_t step = 0; step < kRewriteSeqTrackedSteps; ++step) {
                for (size_t i = 0; i < kCompactBuildFailSubtypeCount; ++i) {
                    if (stats.seqCompactBuildFailAtStepCounts[step][i] == 0) continue;
                    const auto subtype = static_cast<CompactBuildFailSubtype>(i);
                    std::cout << ' ' << "step" << step << '_'
                              << compactBuildFailSubtypeName(subtype)
                              << '='
                              << stats.seqCompactBuildFailAtStepCounts[step][i];
                }
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_SELFLOOP_SUBTYPE_COUNTS]";
            for (size_t i = 0; i < kSelfLoopBuildFailSubtypeCount; ++i) {
                const auto subtype = static_cast<SelfLoopBuildFailSubtype>(i);
                std::cout << ' ' << selfLoopBuildFailSubtypeName(subtype)
                          << '=' << stats.seqSelfLoopSubtypeCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_SELFLOOP_SUBTYPE_CASE_COUNTS]";
            for (size_t i = 0; i < kSelfLoopBuildFailSubtypeCount; ++i) {
                const auto subtype = static_cast<SelfLoopBuildFailSubtype>(i);
                std::cout << ' ' << selfLoopBuildFailSubtypeName(subtype)
                          << '=' << stats.seqSelfLoopCaseCountsBySubtype[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_SELFLOOP_SUBTYPE_DUMPS]";
            for (size_t i = 0; i < kSelfLoopBuildFailSubtypeCount; ++i) {
                const auto subtype = static_cast<SelfLoopBuildFailSubtype>(i);
                std::cout << ' ' << selfLoopBuildFailSubtypeName(subtype)
                          << '='
                          << (stats.firstSelfLoopDumpPaths[i].empty()
                                  ? "-"
                                  : stats.firstSelfLoopDumpPaths[i]);
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_SELFLOOP_SUBTYPE_STEP_COUNTS]";
            for (size_t step = 0; step < kRewriteSeqTrackedSteps; ++step) {
                for (size_t i = 0; i < kSelfLoopBuildFailSubtypeCount; ++i) {
                    if (stats.seqSelfLoopAtStepCounts[step][i] == 0) continue;
                    const auto subtype = static_cast<SelfLoopBuildFailSubtype>(i);
                    std::cout << ' ' << "step" << step << '_'
                              << selfLoopBuildFailSubtypeName(subtype)
                              << '='
                              << stats.seqSelfLoopAtStepCounts[step][i];
                }
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_SELFLOOP_OTHERNB_SUBTYPE_COUNTS]";
            for (size_t i = 0; i < kSelfLoopRemainderOtherNBSubtypeCount; ++i) {
                const auto subtype = static_cast<SelfLoopRemainderOtherNBSubtype>(i);
                std::cout << ' ' << selfLoopRemainderOtherNBSubtypeName(subtype)
                          << '=' << stats.seqSelfLoopOtherNBSubtypeCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_SELFLOOP_OTHERNB_CASE_COUNTS]";
            for (size_t i = 0; i < kSelfLoopRemainderOtherNBSubtypeCount; ++i) {
                const auto subtype = static_cast<SelfLoopRemainderOtherNBSubtype>(i);
                std::cout << ' ' << selfLoopRemainderOtherNBSubtypeName(subtype)
                          << '=' << stats.seqSelfLoopOtherNBCaseCountsBySubtype[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_SELFLOOP_OTHERNB_DUMPS]";
            for (size_t i = 0; i < kSelfLoopRemainderOtherNBSubtypeCount; ++i) {
                const auto subtype = static_cast<SelfLoopRemainderOtherNBSubtype>(i);
                std::cout << ' ' << selfLoopRemainderOtherNBSubtypeName(subtype)
                          << '='
                          << (stats.firstSelfLoopOtherNBDumpPaths[i].empty()
                                  ? "-"
                                  : stats.firstSelfLoopOtherNBDumpPaths[i]);
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_SELFLOOP_OTHERNB_STEP_COUNTS]";
            for (size_t step = 0; step < kRewriteSeqTrackedSteps; ++step) {
                for (size_t i = 0; i < kSelfLoopRemainderOtherNBSubtypeCount; ++i) {
                    if (stats.seqSelfLoopOtherNBAtStepCounts[step][i] == 0) continue;
                    const auto subtype = static_cast<SelfLoopRemainderOtherNBSubtype>(i);
                    std::cout << ' ' << "step" << step << '_'
                              << selfLoopRemainderOtherNBSubtypeName(subtype)
                              << '='
                              << stats.seqSelfLoopOtherNBAtStepCounts[step][i];
                }
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_XINCIDENT_SUBTYPE_COUNTS]";
            for (size_t i = 0; i < kXIncidentVirtualSubtypeCount; ++i) {
                const auto subtype = static_cast<XIncidentVirtualSubtype>(i);
                std::cout << ' ' << xIncidentVirtualSubtypeName(subtype)
                          << '=' << stats.seqXIncidentVirtualSubtypeCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_XINCIDENT_CASE_COUNTS]";
            for (size_t i = 0; i < kXIncidentVirtualSubtypeCount; ++i) {
                const auto subtype = static_cast<XIncidentVirtualSubtype>(i);
                std::cout << ' ' << xIncidentVirtualSubtypeName(subtype)
                          << '=' << stats.seqXIncidentVirtualCaseCountsBySubtype[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_XINCIDENT_DUMPS]";
            for (size_t i = 0; i < kXIncidentVirtualSubtypeCount; ++i) {
                const auto subtype = static_cast<XIncidentVirtualSubtype>(i);
                std::cout << ' ' << xIncidentVirtualSubtypeName(subtype)
                          << '='
                          << (stats.firstXIncidentVirtualDumpPaths[i].empty()
                                  ? "-"
                                  : stats.firstXIncidentVirtualDumpPaths[i]);
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_XINCIDENT_STEP_COUNTS]";
            for (size_t step = 0; step < kRewriteSeqTrackedSteps; ++step) {
                for (size_t i = 0; i < kXIncidentVirtualSubtypeCount; ++i) {
                    if (stats.seqXIncidentVirtualAtStepCounts[step][i] == 0) continue;
                    const auto subtype = static_cast<XIncidentVirtualSubtype>(i);
                    std::cout << ' ' << "step" << step << '_'
                              << xIncidentVirtualSubtypeName(subtype)
                              << '='
                              << stats.seqXIncidentVirtualAtStepCounts[step][i];
                }
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_XSHARED_RESIDUAL_SUBTYPE_COUNTS]";
            for (size_t i = 0; i < kXSharedResidualSubtypeCount; ++i) {
                const auto subtype = static_cast<XSharedResidualSubtype>(i);
                std::cout << ' ' << xSharedResidualSubtypeName(subtype)
                          << '=' << stats.seqXIncidentResidualSubtypeCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_XSHARED_RESIDUAL_CASE_COUNTS]";
            for (size_t i = 0; i < kXSharedResidualSubtypeCount; ++i) {
                const auto subtype = static_cast<XSharedResidualSubtype>(i);
                std::cout << ' ' << xSharedResidualSubtypeName(subtype)
                          << '=' << stats.seqXIncidentResidualCaseCountsBySubtype[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_XSHARED_RESIDUAL_DUMPS]";
            for (size_t i = 0; i < kXSharedResidualSubtypeCount; ++i) {
                const auto subtype = static_cast<XSharedResidualSubtype>(i);
                std::cout << ' ' << xSharedResidualSubtypeName(subtype)
                          << '='
                          << (stats.firstXIncidentResidualDumpPaths[i].empty()
                                  ? "-"
                                  : stats.firstXIncidentResidualDumpPaths[i]);
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_XSHARED_LOOPSHARED_INPUT_SUBTYPE_COUNTS]";
            for (size_t i = 0; i < kXSharedLoopSharedInputSubtypeCount; ++i) {
                const auto subtype = static_cast<XSharedLoopSharedInputSubtype>(i);
                std::cout << ' ' << xSharedLoopSharedInputSubtypeName(subtype)
                          << '=' << stats.seqXSharedLoopSharedInputSubtypeCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_XSHARED_LOOPSHARED_INPUT_CASE_COUNTS]";
            for (size_t i = 0; i < kXSharedLoopSharedInputSubtypeCount; ++i) {
                const auto subtype = static_cast<XSharedLoopSharedInputSubtype>(i);
                std::cout << ' ' << xSharedLoopSharedInputSubtypeName(subtype)
                          << '=' << stats.seqXSharedLoopSharedCaseCountsByInputSubtype[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_XSHARED_LOOPSHARED_INPUT_DUMPS]";
            for (size_t i = 0; i < kXSharedLoopSharedInputSubtypeCount; ++i) {
                const auto subtype = static_cast<XSharedLoopSharedInputSubtype>(i);
                std::cout << ' ' << xSharedLoopSharedInputSubtypeName(subtype)
                          << '='
                          << (stats.firstXSharedLoopSharedInputDumpPaths[i].empty()
                                  ? "-"
                                  : stats.firstXSharedLoopSharedInputDumpPaths[i]);
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_XSHARED_LOOPSHARED_BAILOUT_COUNTS]";
            for (size_t i = 0; i < kXSharedLoopSharedBailoutCount; ++i) {
                const auto bailout = static_cast<XSharedLoopSharedBailout>(i);
                std::cout << ' ' << xSharedLoopSharedBailoutName(bailout)
                          << '=' << stats.seqXSharedLoopSharedBailoutCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_XSHARED_LOOPSHARED_BAILOUT_DUMPS]";
            for (size_t i = 0; i < kXSharedLoopSharedBailoutCount; ++i) {
                const auto bailout = static_cast<XSharedLoopSharedBailout>(i);
                std::cout << ' ' << xSharedLoopSharedBailoutName(bailout)
                          << '='
                          << (stats.firstXSharedLoopSharedBailoutDumpPaths[i].empty()
                                  ? "-"
                                  : stats.firstXSharedLoopSharedBailoutDumpPaths[i]);
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_XSHARED_LOOPSHARED_BAILOUT_STEP_COUNTS]";
            for (size_t step = 0; step < kRewriteSeqTrackedSteps; ++step) {
                for (size_t i = 0; i < kXSharedLoopSharedBailoutCount; ++i) {
                    if (stats.seqXSharedLoopSharedBailoutAtStepCounts[step][i] == 0) continue;
                    const auto bailout = static_cast<XSharedLoopSharedBailout>(i);
                    std::cout << ' ' << "step" << step << '_'
                              << xSharedLoopSharedBailoutName(bailout)
                              << '='
                              << stats.seqXSharedLoopSharedBailoutAtStepCounts[step][i];
                }
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_XSHARED_BAILOUT_COUNTS]";
            for (size_t i = 0; i < kXSharedBridgeBailoutCount; ++i) {
                const auto bailout = static_cast<XSharedBridgeBailout>(i);
                std::cout << ' ' << xSharedBridgeBailoutName(bailout)
                          << '=' << stats.seqXIncidentBridgeBailoutCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_XSHARED_BAILOUT_DUMPS]";
            for (size_t i = 0; i < kXSharedBridgeBailoutCount; ++i) {
                const auto bailout = static_cast<XSharedBridgeBailout>(i);
                std::cout << ' ' << xSharedBridgeBailoutName(bailout)
                          << '='
                          << (stats.firstXIncidentBridgeBailoutDumpPaths[i].empty()
                                  ? "-"
                                  : stats.firstXIncidentBridgeBailoutDumpPaths[i]);
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_XSHARED_BAILOUT_STEP_COUNTS]";
            for (size_t step = 0; step < kRewriteSeqTrackedSteps; ++step) {
                for (size_t i = 0; i < kXSharedBridgeBailoutCount; ++i) {
                    if (stats.seqXIncidentBridgeBailoutAtStepCounts[step][i] == 0) continue;
                    const auto bailout = static_cast<XSharedBridgeBailout>(i);
                    std::cout << ' ' << "step" << step << '_'
                              << xSharedBridgeBailoutName(bailout)
                              << '='
                              << stats.seqXIncidentBridgeBailoutAtStepCounts[step][i];
                }
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_GRAFT_REWIRE_SUBTYPE_COUNTS]";
            for (size_t i = 0; i < kGraftRewireBailoutSubtypeCount; ++i) {
                const auto subtype = static_cast<GraftRewireBailoutSubtype>(i);
                std::cout << ' ' << graftRewireBailoutSubtypeName(subtype)
                          << '=' << stats.seqGraftRewireSubtypeCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_GRAFT_REWIRE_SUBTYPE_CASE_COUNTS]";
            for (size_t i = 0; i < kGraftRewireBailoutSubtypeCount; ++i) {
                const auto subtype = static_cast<GraftRewireBailoutSubtype>(i);
                std::cout << ' ' << graftRewireBailoutSubtypeName(subtype)
                          << '=' << stats.seqGraftRewireCaseCountsBySubtype[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_GRAFT_REWIRE_SUBTYPE_DUMPS]";
            for (size_t i = 0; i < kGraftRewireBailoutSubtypeCount; ++i) {
                const auto subtype = static_cast<GraftRewireBailoutSubtype>(i);
                std::cout << ' ' << graftRewireBailoutSubtypeName(subtype)
                          << '='
                          << (stats.firstGraftRewireDumpPaths[i].empty()
                                  ? "-"
                                  : stats.firstGraftRewireDumpPaths[i]);
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_GRAFT_REWIRE_SUBTYPE_STEP_COUNTS]";
            for (size_t step = 0; step < kRewriteSeqTrackedSteps; ++step) {
                for (size_t i = 0; i < kGraftRewireBailoutSubtypeCount; ++i) {
                    if (stats.seqGraftRewireAtStepCounts[step][i] == 0) continue;
                    const auto subtype = static_cast<GraftRewireBailoutSubtype>(i);
                    std::cout << ' ' << "step" << step << '_'
                              << graftRewireBailoutSubtypeName(subtype)
                              << '='
                              << stats.seqGraftRewireAtStepCounts[step][i];
                }
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_GRAFT_OTHER_SUBTYPE_COUNTS]";
            for (size_t i = 0; i < kGraftOtherSubtypeCount; ++i) {
                const auto subtype = static_cast<GraftOtherSubtype>(i);
                std::cout << ' ' << graftOtherSubtypeName(subtype)
                          << '=' << stats.seqGraftOtherSubtypeCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_GRAFT_OTHER_SUBTYPE_CASE_COUNTS]";
            for (size_t i = 0; i < kGraftOtherSubtypeCount; ++i) {
                const auto subtype = static_cast<GraftOtherSubtype>(i);
                std::cout << ' ' << graftOtherSubtypeName(subtype)
                          << '=' << stats.seqGraftOtherCaseCountsBySubtype[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_GRAFT_OTHER_SUBTYPE_DUMPS]";
            for (size_t i = 0; i < kGraftOtherSubtypeCount; ++i) {
                const auto subtype = static_cast<GraftOtherSubtype>(i);
                std::cout << ' ' << graftOtherSubtypeName(subtype)
                          << '='
                          << (stats.firstGraftOtherDumpPaths[i].empty()
                                  ? "-"
                                  : stats.firstGraftOtherDumpPaths[i]);
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_GRAFT_OTHER_SUBTYPE_STEP_COUNTS]";
            for (size_t step = 0; step < kRewriteSeqTrackedSteps; ++step) {
                for (size_t i = 0; i < kGraftOtherSubtypeCount; ++i) {
                    if (stats.seqGraftOtherAtStepCounts[step][i] == 0) continue;
                    const auto subtype = static_cast<GraftOtherSubtype>(i);
                    std::cout << ' ' << "step" << step << '_'
                              << graftOtherSubtypeName(subtype)
                              << '='
                              << stats.seqGraftOtherAtStepCounts[step][i];
                }
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_POSTCHECK_SUBTYPE_COUNTS]";
            for (size_t i = 0; i < kGraftPostcheckSubtypeCount; ++i) {
                const auto subtype = static_cast<GraftPostcheckSubtype>(i);
                std::cout << ' ' << graftPostcheckSubtypeName(subtype)
                          << '=' << stats.seqPostcheckSubtypeCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_POSTCHECK_SUBTYPE_CASE_COUNTS]";
            for (size_t i = 0; i < kGraftPostcheckSubtypeCount; ++i) {
                const auto subtype = static_cast<GraftPostcheckSubtype>(i);
                std::cout << ' ' << graftPostcheckSubtypeName(subtype)
                          << '=' << stats.seqPostcheckCaseCountsBySubtype[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_POSTCHECK_SUBTYPE_DUMPS]";
            for (size_t i = 0; i < kGraftPostcheckSubtypeCount; ++i) {
                const auto subtype = static_cast<GraftPostcheckSubtype>(i);
                std::cout << ' ' << graftPostcheckSubtypeName(subtype)
                          << '='
                          << (stats.firstPostcheckSubtypeDumpPaths[i].empty()
                                  ? "-"
                                  : stats.firstPostcheckSubtypeDumpPaths[i]);
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_DEFER_SAME_TYPE_SP]"
                      << " count=" << stats.seqDeferredSameTypeSPCount
                      << " caseCount=" << stats.seqDeferredSameTypeSPCaseCount
                      << " dump="
                      << (stats.firstDeferredSameTypeSPDumpPath.empty()
                              ? "-"
                              : stats.firstDeferredSameTypeSPDumpPath)
                      << "\n";
            std::cout << "[REWRITE_R_SEQ_DEFER_SAME_TYPE_SP_STEP_COUNTS]";
            for (size_t step = 0; step < kRewriteSeqTrackedSteps; ++step) {
                if (stats.seqDeferredSameTypeSPAtStepCounts[step] == 0) continue;
                std::cout << ' ' << "step" << step
                          << '=' << stats.seqDeferredSameTypeSPAtStepCounts[step];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_SP_CLEANUP]"
                      << " attemptCount=" << stats.seqSameTypeSPCleanupAttemptCount
                      << " mergeCount=" << stats.seqSameTypeSPCleanupMergeCount
                      << " successCount=" << stats.seqSameTypeSPCleanupSuccessCount
                      << " failCount=" << stats.seqSameTypeSPCleanupFailCount
                      << " caseCount=" << stats.seqSameTypeSPCleanupCaseCount
                      << " dump="
                      << (stats.firstSameTypeSPCleanupDumpPath.empty()
                              ? "-"
                              : stats.firstSameTypeSPCleanupDumpPath)
                      << "\n";
            std::cout << "[REWRITE_R_SEQ_OLDARC_REPAIR_OUTCOME_COUNTS]";
            for (size_t i = 0; i < kProxyArcRepairOutcomeCount; ++i) {
                const auto outcome = static_cast<ProxyArcRepairOutcome>(i);
                std::cout << ' ' << proxyArcRepairOutcomeName(outcome)
                          << '=' << stats.seqResolvedOldArcRepairOutcomeCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_PROXY_ARC_LIFECYCLE_COUNTS]";
            for (size_t i = 0; i < kProxyArcLifecyclePhaseCount; ++i) {
                const auto phase = static_cast<ProxyArcLifecyclePhase>(i);
                std::cout << ' ' << proxyArcLifecyclePhaseName(phase)
                          << '=' << stats.seqProxyArcLifecycleCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_PROXY_ARC_FIRST_BAD_PHASE_COUNTS]";
            for (size_t i = 0; i < kProxyArcLifecyclePhaseCount; ++i) {
                const auto phase = static_cast<ProxyArcLifecyclePhase>(i);
                std::cout << ' ' << proxyArcLifecyclePhaseName(phase)
                          << '=' << stats.seqProxyArcFirstBadPhaseCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_PROXY_ARC_CASE_COUNTS_BY_FIRST_BAD_PHASE]";
            for (size_t i = 0; i < kProxyArcLifecyclePhaseCount; ++i) {
                const auto phase = static_cast<ProxyArcLifecyclePhase>(i);
                std::cout << ' ' << proxyArcLifecyclePhaseName(phase)
                          << '=' << stats.seqProxyArcCaseCountsByFirstBadPhase[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_PROXY_ARC_PHASE_DUMPS]";
            for (size_t i = 0; i < kProxyArcLifecyclePhaseCount; ++i) {
                if (stats.firstProxyArcPhaseDumpPaths[i].empty()) continue;
                const auto phase = static_cast<ProxyArcLifecyclePhase>(i);
                std::cout << ' ' << proxyArcLifecyclePhaseName(phase)
                          << '=' << stats.firstProxyArcPhaseDumpPaths[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_OLDARC_WEAK_REPAIR_STATS]"
                      << " seqResolvedOldArcWeakRepairAttemptCount="
                      << stats.seqResolvedOldArcWeakRepairAttemptCount
                      << " seqResolvedOldArcWeakRepairSuccessCount="
                      << stats.seqResolvedOldArcWeakRepairSuccessCount
                      << " seqResolvedOldArcWeakRepairFailCount="
                      << stats.seqResolvedOldArcWeakRepairFailCount
                      << " seqResolvedOldArcWeakRepairUsedCount="
                      << stats.seqResolvedOldArcWeakRepairUsedCount << "\n";
            std::cout << "[REWRITE_R_SEQ_OLDARC_WEAK_REPAIR_OUTCOME_COUNTS]";
            for (size_t i = 0; i < kProxyArcRepairOutcomeCount; ++i) {
                const auto outcome = static_cast<ProxyArcRepairOutcome>(i);
                std::cout << ' ' << proxyArcRepairOutcomeName(outcome)
                          << '=' << stats.seqResolvedOldArcWeakRepairOutcomeCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_OLDARC_WEAK_REPAIR_DUMPS]"
                      << " success="
                      << (stats.firstOldArcWeakRepairSuccessDumpPath.empty()
                              ? "-"
                              : stats.firstOldArcWeakRepairSuccessDumpPath);
            for (size_t i = 0; i < kProxyArcRepairOutcomeCount; ++i) {
                if (stats.firstOldArcWeakRepairFailDumpPaths[i].empty()) continue;
                const auto outcome = static_cast<ProxyArcRepairOutcome>(i);
                std::cout << ' ' << proxyArcRepairOutcomeName(outcome)
                          << '=' << stats.firstOldArcWeakRepairFailDumpPaths[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_WEAK_REPAIR_STATS]"
                      << " seqWeakRepairEnteredCount="
                      << stats.seqWeakRepairEnteredCount
                      << " seqWeakRepairTentativeSuccessCount="
                      << stats.seqWeakRepairTentativeSuccessCount
                      << " seqWeakRepairCommittedCount="
                      << stats.seqWeakRepairCommittedCount
                      << " seqWeakRepairRollbackCount="
                      << stats.seqWeakRepairRollbackCount << "\n";
            std::cout << "[REWRITE_R_SEQ_WEAK_REPAIR_GATE_COUNTS]";
            for (size_t i = 0; i < kWeakRepairGateSubtypeCount; ++i) {
                const auto subtype = static_cast<WeakRepairGateSubtype>(i);
                std::cout << ' ' << weakRepairGateSubtypeName(subtype)
                          << '=' << stats.seqWeakRepairGateCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_WEAK_REPAIR_CANDIDATE_COUNTS]";
            for (size_t i = 0; i < kWeakRepairCandidateSubtypeCount; ++i) {
                const auto subtype = static_cast<WeakRepairCandidateSubtype>(i);
                std::cout << ' ' << weakRepairCandidateSubtypeName(subtype)
                          << '=' << stats.seqWeakRepairCandidateCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_WEAK_REPAIR_COMMIT_COUNTS]";
            for (size_t i = 0; i < kWeakRepairCommitOutcomeCount; ++i) {
                const auto outcome = static_cast<WeakRepairCommitOutcome>(i);
                std::cout << ' ' << weakRepairCommitOutcomeName(outcome)
                          << '=' << stats.seqWeakRepairCommitCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_WEAK_REPAIR_DUMPS]";
            for (size_t i = 0; i < kWeakRepairGateSubtypeCount; ++i) {
                if (stats.firstWeakRepairGateDumpPaths[i].empty()) continue;
                const auto subtype = static_cast<WeakRepairGateSubtype>(i);
                std::cout << ' ' << weakRepairGateSubtypeName(subtype)
                          << '=' << stats.firstWeakRepairGateDumpPaths[i];
            }
            for (size_t i = 0; i < kWeakRepairCandidateSubtypeCount; ++i) {
                if (stats.firstWeakRepairCandidateDumpPaths[i].empty()) continue;
                const auto subtype = static_cast<WeakRepairCandidateSubtype>(i);
                std::cout << ' ' << weakRepairCandidateSubtypeName(subtype)
                          << '=' << stats.firstWeakRepairCandidateDumpPaths[i];
            }
            for (size_t i = 0; i < kWeakRepairCommitOutcomeCount; ++i) {
                if (stats.firstWeakRepairCommitDumpPaths[i].empty()) continue;
                const auto outcome = static_cast<WeakRepairCommitOutcome>(i);
                std::cout << ' ' << weakRepairCommitOutcomeName(outcome)
                          << '=' << stats.firstWeakRepairCommitDumpPaths[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_PROXY_REPAIR_NOCAND_SUBTYPE_COUNTS]";
            for (size_t i = 0; i < kProxyArcNoCandidateSubtypeCount; ++i) {
                const auto subtype = static_cast<ProxyArcNoCandidateSubtype>(i);
                std::cout << ' ' << proxyArcNoCandidateSubtypeName(subtype)
                          << '=' << stats.seqProxyRepairNoCandidateSubtypeCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_PROXY_REPAIR_NOCAND_CASE_COUNTS]";
            for (size_t i = 0; i < kProxyArcNoCandidateSubtypeCount; ++i) {
                const auto subtype = static_cast<ProxyArcNoCandidateSubtype>(i);
                std::cout << ' ' << proxyArcNoCandidateSubtypeName(subtype)
                          << '=' << stats.seqProxyRepairNoCandidateCaseCountsBySubtype[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_PROXY_REPAIR_NOCAND_DUMPS]";
            for (size_t i = 0; i < kProxyArcNoCandidateSubtypeCount; ++i) {
                const auto subtype = static_cast<ProxyArcNoCandidateSubtype>(i);
                std::cout << ' ' << proxyArcNoCandidateSubtypeName(subtype)
                          << '='
                          << (stats.firstProxyRepairNoCandidateDumpPaths[i].empty()
                                  ? "-"
                                  : stats.firstProxyRepairNoCandidateDumpPaths[i]);
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_PROXY_REPAIR_NOCAND_STEP_COUNTS]";
            for (size_t step = 0; step < kRewriteSeqTrackedSteps; ++step) {
                for (size_t i = 0; i < kProxyArcNoCandidateSubtypeCount; ++i) {
                    if (stats.seqProxyRepairNoCandidateAtStepCounts[step][i] == 0) continue;
                    const auto subtype = static_cast<ProxyArcNoCandidateSubtype>(i);
                    std::cout << ' ' << "step" << step << '_'
                              << proxyArcNoCandidateSubtypeName(subtype)
                              << '='
                              << stats.seqProxyRepairNoCandidateAtStepCounts[step][i];
                }
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_PATH_TAKEN_COUNTS]";
            for (size_t i = 0; i < kRewritePathTakenCount; ++i) {
                const auto path = static_cast<RewritePathTaken>(i);
                std::cout << ' ' << rewritePathTakenName(path)
                          << '=' << stats.rewritePathTakenCounts[i];
            }
            std::cout << "\n";
            std::cout << "[REWRITE_R_SEQ_LENGTH_HIST]";
            for (size_t i = 0; i < kRewriteSeqLengthHistogramSize; ++i) {
                std::cout << " len" << i << '=' << stats.sequenceLengthHistogram[i];
            }
            std::cout << "\n";
        };
        auto runCompact = [&](const CompactGraph &H) -> bool {
            HarnessResult r;
            if (cfg.mode == "static") {
                r = runStaticPipelineCaseDumpAware(H, backend, ops, cfg.seed, tc, cfg.dumpDir);
            } else if (cfg.mode == "dummy") {
                r = runDummyGraftCaseDumpAware(H, backend, ops, cfg.seed, tc, cfg.dumpDir);
            } else {
                throw std::runtime_error("unknown mode: " + cfg.mode);
            }
            if (!r.ok) {
                std::cerr << "[FAIL] tc=" << tc << " where=" << r.where << " why=" << r.why << "\n";
                std::cerr << "bundle=" << r.dumpPath << "\n";
                return false;
            }
            ++tc;
            return true;
        };
        auto runRewrite = [&](const ExplicitBlockGraph &G) -> bool {
            HarnessResult r;
            if (cfg.mode == "rewrite-r") {
                r = runRewriteRFallbackCaseDumpAware(G, backend, ops, cfg.seed, tc, cfg.dumpDir);
            } else if (cfg.mode == "rewrite-r-seq" ||
                       cfg.mode == "rewrite-seq" ||
                       cfg.mode == "rewrite-r-seq-bench") {
                r = runRewriteRSequenceCaseDumpAware(G, backend, ops, cfg.seed, tc, cfg.dumpDir);
            } else if (cfg.mode == "rewrite-r-seq-replay") {
                r = runRewriteRSequenceReplayDumpAware(
                    G, backend, ops, cfg.seed, tc, cfg.targetStep, cfg.dumpDir);
            } else {
                throw std::runtime_error("unknown rewrite mode: " + cfg.mode);
            }
            if (!r.ok) {
                std::cerr << "[FAIL] tc=" << tc << " where=" << r.where << " why=" << r.why << "\n";
                std::cerr << "bundle=" << r.dumpPath << "\n";
                return false;
            }
            if (cfg.mode == "rewrite-r-seq-replay" && !r.dumpPath.empty()) {
                std::cout << "[REPLAY_CAPTURE] bundle=" << r.dumpPath << "\n";
            }
            if (cfg.mode != "rewrite-r-seq-replay") {
                recordRewriteRPass(cfg.manualOnly);
            }
            ++tc;
            return true;
        };
        auto runSolverCompareCase = [&](const std::string &caseName,
                                        uint64_t caseSeed,
                                        int caseTc,
                                        std::optional<int> targetStep,
                                        const ExplicitBlockGraph &G,
                                        SolverCompareCaseResult &result) {
            const bool useLegacy =
                cfg.baselineMode == "legacy" || cfg.baselineMode == "both";
            const bool useOracle =
                cfg.baselineMode == "oracle" || cfg.baselineMode == "both";
            const OracleHandoffPolicy oracleHandoffPolicy =
                parseOracleHandoffPolicyArg(cfg.oracleHandoff);
            const auto canonicalEqual = [](const CanonicalExplicitGraph &lhs,
                                           const CanonicalExplicitGraph &rhs) {
                return lhs.edges == rhs.edges && lhs.vertices == rhs.vertices;
            };

            result = {};
            result.name = caseName;
            result.seed = caseSeed;
            result.tcIndex = caseTc;
            result.targetStep = targetStep;

            SolverCompareBundle bundle;
            bundle.inputCaseId = caseName;
            bundle.seed = caseSeed;
            bundle.tc = caseTc;
            bundle.targetStep = targetStep;
            bundle.oracleHandoffPolicy = oracleHandoffPolicy;
            bundle.inputExplicit = G;

            SolverOutput legacyOutput;
            std::string legacyWhy;
            if (useLegacy) {
                setRewriteRCaseContext(caseSeed, caseTc);
                const auto legacyStartedAt = Clock::now();
                result.legacyOk =
                    solveWithBaselineRewriteSolver(G, legacyOutput, legacyWhy);
                result.legacyElapsedMs =
                    std::chrono::duration<double, std::milli>(Clock::now() - legacyStartedAt)
                        .count();
                bundle.legacyElapsedMs = result.legacyElapsedMs;
                bundle.legacyWhy = legacyWhy;
                if (result.legacyOk) {
                    bundle.legacyOutput = legacyOutput;
                    bundle.legacyCanonicalExplicit = legacyOutput.canonicalExplicitGraph;
                } else {
                    result.failures.push_back("legacy baseline failed: " + legacyWhy);
                }
            }

            SolverOutput oracleOutput;
            std::string oracleWhy;
            if (useOracle) {
                setRewriteRCaseContext(caseSeed, caseTc);
                const auto oracleStartedAt = Clock::now();
                result.oracleOk =
                    solveWithOracleFixpointBaseline(
                        G, oracleHandoffPolicy, oracleOutput, oracleWhy);
                result.oracleElapsedMs =
                    std::chrono::duration<double, std::milli>(Clock::now() - oracleStartedAt)
                        .count();
                bundle.oracleElapsedMs = result.oracleElapsedMs;
                bundle.oracleWhy = oracleWhy;
                if (result.oracleOk) {
                    bundle.oracleOutput = oracleOutput;
                    bundle.oracleCanonicalExplicit = oracleOutput.canonicalExplicitGraph;
                } else {
                    result.failures.push_back("oracle baseline failed: " + oracleWhy);
                }

                SolverOutput oracleDeleteOutput;
                std::string oracleDeleteWhy;
                if (oracleHandoffPolicy == OracleHandoffPolicy::OHP_DELETE_EXPLICIT) {
                    oracleDeleteOutput = oracleOutput;
                    oracleDeleteWhy = oracleWhy;
                } else {
                    setRewriteRCaseContext(caseSeed, caseTc);
                    solveWithOracleFixpointBaseline(G,
                                                    OracleHandoffPolicy::OHP_DELETE_EXPLICIT,
                                                    oracleDeleteOutput,
                                                    oracleDeleteWhy);
                }
                if (oracleDeleteOutput.valid) {
                    bundle.oracleDeletePolicyFinalExplicit =
                        oracleDeleteOutput.canonicalExplicitGraph;
                }

                SolverOutput oracleNormalizeOutput;
                std::string oracleNormalizeWhy;
                if (oracleHandoffPolicy == OracleHandoffPolicy::OHP_NORMALIZE_EXPLICIT) {
                    oracleNormalizeOutput = oracleOutput;
                    oracleNormalizeWhy = oracleWhy;
                } else {
                    setRewriteRCaseContext(caseSeed, caseTc);
                    solveWithOracleFixpointBaseline(
                        G,
                        OracleHandoffPolicy::OHP_NORMALIZE_EXPLICIT,
                        oracleNormalizeOutput,
                        oracleNormalizeWhy);
                }
                if (oracleNormalizeOutput.valid) {
                    bundle.oracleNormalizePolicyFinalExplicit =
                        oracleNormalizeOutput.canonicalExplicitGraph;
                }
            }

            SolverOutput rewriteSeqOutput;
            std::string rewriteSeqWhy;
            setRewriteRCaseContext(caseSeed, caseTc);
            const auto rewriteSeqStartedAt = Clock::now();
            result.rewriteSeqOk =
                solveWithRewriteSeqEngine(G, rewriteSeqOutput, rewriteSeqWhy);
            result.rewriteSeqElapsedMs =
                std::chrono::duration<double, std::milli>(Clock::now() - rewriteSeqStartedAt)
                    .count();
            bundle.rewriteSeqElapsedMs = result.rewriteSeqElapsedMs;
            bundle.rewriteSeqWhy = rewriteSeqWhy;
            if (result.rewriteSeqOk) {
                bundle.rewriteSeqOutput = rewriteSeqOutput;
                bundle.rewriteSeqCanonicalExplicit = rewriteSeqOutput.canonicalExplicitGraph;
                bundle.rewriteFinalExplicit = rewriteSeqOutput.canonicalExplicitGraph;
            } else {
                result.failures.push_back("rewrite-seq solver failed: " + rewriteSeqWhy);
            }

            if (bundle.oracleDeletePolicyFinalExplicit.has_value() &&
                bundle.rewriteFinalExplicit.has_value()) {
                bundle.deleteVsRewriteCanonicalEqual =
                    canonicalEqual(*bundle.oracleDeletePolicyFinalExplicit,
                                   *bundle.rewriteFinalExplicit);
            }
            if (bundle.oracleNormalizePolicyFinalExplicit.has_value() &&
                bundle.rewriteFinalExplicit.has_value()) {
                bundle.normalizeVsRewriteCanonicalEqual =
                    canonicalEqual(*bundle.oracleNormalizePolicyFinalExplicit,
                                   *bundle.rewriteFinalExplicit);
            }
            if (bundle.oracleDeletePolicyFinalExplicit.has_value() &&
                bundle.oracleNormalizePolicyFinalExplicit.has_value()) {
                bundle.deleteVsNormalizeCanonicalEqual =
                    canonicalEqual(*bundle.oracleDeletePolicyFinalExplicit,
                                   *bundle.oracleNormalizePolicyFinalExplicit);
            }

            auto compareOutputs =
                [&](const std::string &label,
                    const SolverOutput &lhs,
                    const SolverOutput &rhs,
                    std::optional<bool> &explicitEquivalentOut,
                    std::optional<bool> &rawExplicitEquivalentOut,
                    std::optional<bool> &canonicalExplicitEquivalentOut,
                    std::optional<bool> &parentEquivalentOut) {
                    std::string rawEqWhy;
                    rawExplicitEquivalentOut = ops.checkEquivalentExplicitGraphs(
                        lhs.explicitGraph, rhs.explicitGraph, rawEqWhy);

                    std::string canonicalEqWhy;
                    canonicalExplicitEquivalentOut = areCanonicalExplicitEqual(
                        lhs.explicitGraph, rhs.explicitGraph, canonicalEqWhy);
                    explicitEquivalentOut = canonicalExplicitEquivalentOut;
                    if (!*canonicalExplicitEquivalentOut) {
                        result.failures.push_back(label + " canonical explicit mismatch: " +
                                                  canonicalEqWhy);
                    }

                    if (!lhs.parent.empty() || !rhs.parent.empty()) {
                        parentEquivalentOut = lhs.parent == rhs.parent;
                        if (!*parentEquivalentOut) {
                            result.failures.push_back(label + " parent output mismatch");
                        }
                    }
                };

            if (useLegacy && result.legacyOk && result.rewriteSeqOk) {
                compareOutputs("legacy vs rewrite",
                               legacyOutput,
                               rewriteSeqOutput,
                               result.legacyVsRewriteEquivalent,
                               result.legacyVsRewriteRawExplicitEquivalent,
                               result.legacyVsRewriteCanonicalExplicitEquivalent,
                               result.legacyVsRewriteParentEquivalent);
                bundle.legacyVsRewriteEquivalent = result.legacyVsRewriteEquivalent;
                bundle.legacyVsRewriteRawExplicitEquivalent =
                    result.legacyVsRewriteRawExplicitEquivalent;
                bundle.legacyVsRewriteCanonicalExplicitEquivalent =
                    result.legacyVsRewriteCanonicalExplicitEquivalent;
                bundle.legacyVsRewriteParentEquivalent =
                    result.legacyVsRewriteParentEquivalent;
            }

            if (useOracle && result.oracleOk && result.rewriteSeqOk) {
                compareOutputs("oracle vs rewrite",
                               oracleOutput,
                               rewriteSeqOutput,
                               result.oracleVsRewriteEquivalent,
                               result.oracleVsRewriteRawExplicitEquivalent,
                               result.oracleVsRewriteCanonicalExplicitEquivalent,
                               result.oracleVsRewriteParentEquivalent);
                bundle.oracleVsRewriteEquivalent = result.oracleVsRewriteEquivalent;
                bundle.oracleVsRewriteRawExplicitEquivalent =
                    result.oracleVsRewriteRawExplicitEquivalent;
                bundle.oracleVsRewriteCanonicalExplicitEquivalent =
                    result.oracleVsRewriteCanonicalExplicitEquivalent;
                bundle.oracleVsRewriteParentEquivalent =
                    result.oracleVsRewriteParentEquivalent;
            }

            if (useLegacy && useOracle && result.legacyOk && result.oracleOk) {
                compareOutputs("legacy vs oracle",
                               legacyOutput,
                               oracleOutput,
                               result.legacyVsOracleEquivalent,
                               result.legacyVsOracleRawExplicitEquivalent,
                               result.legacyVsOracleCanonicalExplicitEquivalent,
                               result.legacyVsOracleParentEquivalent);
                bundle.legacyVsOracleEquivalent = result.legacyVsOracleEquivalent;
                bundle.legacyVsOracleRawExplicitEquivalent =
                    result.legacyVsOracleRawExplicitEquivalent;
                bundle.legacyVsOracleCanonicalExplicitEquivalent =
                    result.legacyVsOracleCanonicalExplicitEquivalent;
                bundle.legacyVsOracleParentEquivalent =
                    result.legacyVsOracleParentEquivalent;
            }

            result.passed = result.failures.empty();
            if (!result.passed) {
                bundle.firstMismatchDescription = result.failures.front();
                std::ostringstream oss;
                if (useLegacy && !result.legacyOk && (!useOracle || result.oracleOk) &&
                    result.rewriteSeqOk) {
                    oss << cfg.dumpDir << "/solver_compare_legacy_fail";
                } else if (useOracle && !result.oracleOk &&
                           (!useLegacy || result.legacyOk) && result.rewriteSeqOk) {
                    oss << cfg.dumpDir << "/solver_compare_oracle_fail";
                } else if ((useLegacy && !result.legacyOk) ||
                           (useOracle && !result.oracleOk)) {
                    oss << cfg.dumpDir << "/solver_compare_multi_fail";
                } else if (!result.rewriteSeqOk) {
                    oss << cfg.dumpDir << "/solver_compare_rewriteseq_fail";
                } else {
                    oss << cfg.dumpDir << "/solver_compare_mismatch";
                }
                oss << "_seed" << caseSeed << "_tc" << caseTc;
                if (targetStep.has_value()) oss << "_step" << *targetStep;
                oss << "_" << sanitizePathComponent(caseName) << ".txt";
                result.dumpPath = oss.str();
                dumpSolverCompareBundle(bundle, result.dumpPath);
            }
        };

        if (cfg.mode == "rewrite-r-seq-regression") {
            if (cfg.manualOnly) {
                throw std::runtime_error("rewrite-r-seq-regression does not support --manual-only");
            }
            if (cfg.manifestPath.empty()) {
                throw std::runtime_error("rewrite-r-seq-regression requires --manifest");
            }

            const auto cases = loadRegressionManifest(cfg.manifestPath);
            if (cases.empty()) {
                throw std::runtime_error("rewrite-r-seq-regression manifest contains no cases");
            }

            std::vector<RegressionCaseResult> results;
            results.reserve(cases.size());
            std::vector<std::string> failedCaseNames;

            for (const auto &spec : cases) {
                const auto caseStart = Clock::now();
                const RewriteRStats beforeStats = getRewriteRStats();
                const ExplicitBlockGraph G = spec.inputExplicit.has_value()
                                                ? *spec.inputExplicit
                                                : makeRandomRewriteCase(spec.seed, spec.tcIndex);
                HarnessResult r = runRewriteRSequenceReplayDumpAware(
                    G,
                    backend,
                    ops,
                    spec.seed,
                    spec.tcIndex,
                    spec.targetStep,
                    cfg.dumpDir);
                const RewriteRStats afterStats = getRewriteRStats();

                RegressionCaseResult result;
                result.name = spec.name;
                result.seed = spec.seed;
                result.tcIndex = spec.tcIndex;
                result.targetStep = spec.targetStep;
                result.elapsedMs = std::chrono::duration<double, std::milli>(
                                       Clock::now() - caseStart)
                                       .count();
                result.dumpPath = r.dumpPath;

                if (r.ok != spec.expectedTopLevelOk) {
                    std::ostringstream oss;
                    oss << "top-level ok mismatch: expected=" << spec.expectedTopLevelOk
                        << " actual=" << r.ok;
                    result.failures.push_back(oss.str());
                }

                if (!r.bundle.has_value()) {
                    result.failures.push_back("missing HarnessBundle in HarnessResult");
                } else {
                    const auto &bundle = *r.bundle;
                    const bool actualInvariantOk =
                        bundle.actualInvariantOk.value_or(false);
                    const bool oracleEquivalentOk =
                        bundle.oracleEquivalentOk.value_or(false);
                    if (actualInvariantOk != spec.expectedActualInvariantOk) {
                        std::ostringstream oss;
                        oss << "actualInvariantOk mismatch: expected="
                            << spec.expectedActualInvariantOk
                            << " actual=" << actualInvariantOk;
                        result.failures.push_back(oss.str());
                    }
                    if (oracleEquivalentOk != spec.expectedOracleEquivalentOk) {
                        std::ostringstream oss;
                        oss << "oracleEquivalentOk mismatch: expected="
                            << spec.expectedOracleEquivalentOk
                            << " actual=" << oracleEquivalentOk;
                        result.failures.push_back(oss.str());
                    }
                    if (spec.expectedPostcheckSubtype.has_value()) {
                        const std::string actualSubtype =
                            bundle.postcheckSubtype.has_value()
                                ? graftPostcheckSubtypeName(*bundle.postcheckSubtype)
                                : "NONE";
                        if (actualSubtype != *spec.expectedPostcheckSubtype) {
                            std::ostringstream oss;
                            oss << "postcheckSubtype mismatch: expected="
                                << *spec.expectedPostcheckSubtype
                                << " actual=" << actualSubtype;
                            result.failures.push_back(oss.str());
                        }
                    }
                }

                for (const auto &tag : spec.expectedSpecialPathTags) {
                    std::string tagWhy;
                    const uint64_t delta =
                        statDeltaForSpecialPathTag(beforeStats, afterStats, tag, tagWhy);
                    if (!tagWhy.empty()) {
                        result.failures.push_back(tagWhy);
                        continue;
                    }
                    if (delta == 0) {
                        result.failures.push_back(
                            "expected special path/stat tag not observed: " + tag);
                    }
                }

                result.passed = result.failures.empty();
                if (!result.passed) {
                    failedCaseNames.push_back(result.name);
                    std::cerr << "[REGRESSION_FAIL] case=" << result.name
                              << " bundle=" << result.dumpPath << "\n";
                    for (const auto &failure : result.failures) {
                        std::cerr << "  - " << failure << "\n";
                    }
                }
                results.push_back(std::move(result));
            }

            const std::string summaryPath = cfg.dumpDir + "/summary.json";
            std::ofstream ofs(summaryPath);
            ofs << "{\n";
            ofs << "  \"manifestPath\": \"" << jsonEscape(cfg.manifestPath) << "\",\n";
            ofs << "  \"outputDumpDir\": \"" << jsonEscape(cfg.dumpDir) << "\",\n";
            ofs << "  \"totalCases\": " << results.size() << ",\n";
            ofs << "  \"passedCases\": " << (results.size() - failedCaseNames.size()) << ",\n";
            ofs << "  \"failedCases\": " << failedCaseNames.size() << ",\n";
            ofs << "  \"failedCaseNames\": [";
            for (size_t i = 0; i < failedCaseNames.size(); ++i) {
                if (i != 0) ofs << ", ";
                ofs << "\"" << jsonEscape(failedCaseNames[i]) << "\"";
            }
            ofs << "],\n";
            ofs << "  \"cases\": [\n";
            for (size_t i = 0; i < results.size(); ++i) {
                const auto &result = results[i];
                ofs << "    {\n";
                ofs << "      \"name\": \"" << jsonEscape(result.name) << "\",\n";
                ofs << "      \"seed\": " << result.seed << ",\n";
                ofs << "      \"tcIndex\": " << result.tcIndex << ",\n";
                ofs << "      \"targetStep\": " << result.targetStep << ",\n";
                ofs << "      \"passed\": " << (result.passed ? "true" : "false") << ",\n";
                ofs << "      \"elapsedMs\": " << result.elapsedMs << ",\n";
                ofs << "      \"dumpPath\": \"" << jsonEscape(result.dumpPath) << "\",\n";
                ofs << "      \"failures\": [";
                for (size_t j = 0; j < result.failures.size(); ++j) {
                    if (j != 0) ofs << ", ";
                    ofs << "\"" << jsonEscape(result.failures[j]) << "\"";
                }
                ofs << "]\n";
                ofs << "    }" << (i + 1 == results.size() ? "\n" : ",\n");
            }
            ofs << "  ]\n";
            ofs << "}\n";

            if (!failedCaseNames.empty()) {
                printRewriteStats();
                std::cerr << "[FAIL] regression summary=" << summaryPath << "\n";
                return 1;
            }

            std::cout << "[REGRESSION] summary=" << summaryPath
                      << " totalCases=" << results.size()
                      << " passedCases=" << results.size()
                      << " failedCases=0\n";
            std::cout << "[OK] completed tc=" << results.size() << "\n";
            printRewriteStats();
            return 0;
        } else if (cfg.mode == "solver-baseline-replay") {
            if (cfg.manualOnly) {
                throw std::runtime_error("solver-baseline-replay does not support --manual-only");
            }
            if (cfg.manifestPath.empty()) {
                throw std::runtime_error("solver-baseline-replay requires --manifest");
            }
            if (cfg.caseName.empty()) {
                throw std::runtime_error("solver-baseline-replay requires --case-name");
            }

            const auto cases = loadRegressionManifest(cfg.manifestPath);
            auto it = std::find_if(cases.begin(),
                                   cases.end(),
                                   [&](const RegressionCaseSpec &spec) {
                                       return spec.name == cfg.caseName;
                                   });
            if (it == cases.end()) {
                throw std::runtime_error("solver-baseline-replay case not found in manifest: " +
                                         cfg.caseName);
            }
            if (!it->inputExplicit.has_value()) {
                throw std::runtime_error(
                    "solver-baseline-replay case is missing inputExplicit snapshot: " +
                    cfg.caseName);
            }

            SolverBaselineReplayBundle bundle;
            bundle.caseName = it->name;
            bundle.manifestPath = cfg.manifestPath;
            bundle.seed = it->seed;
            bundle.tc = it->tcIndex;
            bundle.targetStep = it->targetStep;

            std::string why;
            setRewriteRCaseContext(it->seed, it->tcIndex);
            const bool ok = runSolverBaselineReplay(*it->inputExplicit, bundle, why);

            std::ostringstream bundlePathStream;
            bundlePathStream << cfg.dumpDir << "/"
                             << solverBaselineStageName(bundle.baselineStage)
                             << "_seed" << it->seed
                             << "_tc" << it->tcIndex;
            if (bundle.stepIndex.has_value()) {
                bundlePathStream << "_step" << *bundle.stepIndex;
            } else if (it->targetStep >= 0) {
                bundlePathStream << "_step" << it->targetStep;
            }
            bundlePathStream << "_" << sanitizePathComponent(it->name) << ".txt";
            const std::string bundlePath = bundlePathStream.str();
            dumpSolverBaselineReplayBundle(bundle, bundlePath);

            const std::string summaryPath = cfg.dumpDir + "/summary.json";
            std::ofstream ofs(summaryPath);
            ofs << "{\n";
            ofs << "  \"mode\": \"solver-baseline-replay\",\n";
            ofs << "  \"manifestPath\": \"" << jsonEscape(cfg.manifestPath) << "\",\n";
            ofs << "  \"caseName\": \"" << jsonEscape(it->name) << "\",\n";
            ofs << "  \"outputDumpDir\": \"" << jsonEscape(cfg.dumpDir) << "\",\n";
            ofs << "  \"bundlePath\": \"" << jsonEscape(bundlePath) << "\",\n";
            ofs << "  \"baselineStage\": \""
                << solverBaselineStageName(bundle.baselineStage) << "\",\n";
            ofs << "  \"topLevelOk\": " << (ok ? "true" : "false") << ",\n";
            if (bundle.stepIndex.has_value()) {
                ofs << "  \"stepIndex\": " << *bundle.stepIndex << ",\n";
            }
            if (bundle.sequenceLengthSoFar.has_value()) {
                ofs << "  \"sequenceLengthSoFar\": " << *bundle.sequenceLengthSoFar
                    << ",\n";
            }
            if (bundle.chosenR.has_value()) {
                ofs << "  \"chosenR\": " << *bundle.chosenR << ",\n";
            }
            if (bundle.chosenX.has_value()) {
                ofs << "  \"chosenX\": " << *bundle.chosenX << ",\n";
            }
            if (bundle.actualInvariantOk.has_value()) {
                ofs << "  \"actualInvariantOk\": "
                    << (*bundle.actualInvariantOk ? "true" : "false") << ",\n";
            }
            ofs << "  \"actualInvariantWhy\": \""
                << jsonEscape(bundle.actualInvariantWhy) << "\",\n";
            ofs << "  \"actualInvariantDetailedSubtype\": \""
                << jsonEscape(bundle.actualInvariantDetailedSubtype) << "\",\n";
            if (bundle.oracleEquivalentOk.has_value()) {
                ofs << "  \"oracleEquivalentOk\": "
                    << (*bundle.oracleEquivalentOk ? "true" : "false") << ",\n";
            }
            ofs << "  \"oracleWhy\": \"" << jsonEscape(bundle.oracleWhy) << "\",\n";
            if (bundle.firstFailingNodeId.has_value()) {
                ofs << "  \"firstFailingNodeId\": " << *bundle.firstFailingNodeId
                    << ",\n";
            }
            ofs << "  \"firstFailingInvariantKind\": \""
                << solverBaselineInvariantKindName(bundle.firstFailingInvariantKind)
                << "\",\n";
            ofs << "  \"sameTypeSPPresent\": "
                << (bundle.sameTypeSPPresent ? "true" : "false") << ",\n";
            ofs << "  \"adjacencyMismatchPresent\": "
                << (bundle.adjacencyMismatchPresent ? "true" : "false") << ",\n";
            ofs << "  \"deadRelayCandidateNodes\": [";
            for (size_t i = 0; i < bundle.deadRelayCandidateNodes.size(); ++i) {
                if (i != 0) ofs << ", ";
                ofs << bundle.deadRelayCandidateNodes[i];
            }
            ofs << "]\n";
            ofs << "}\n";

            std::cout << "[BASELINE_REPLAY] case=" << it->name
                      << " stage=" << solverBaselineStageName(bundle.baselineStage)
                      << " bundle=" << bundlePath
                      << " summary=" << summaryPath << "\n";
            if (!why.empty()) {
                std::cout << "[BASELINE_REPLAY_WHY] " << why << "\n";
            }
            printRewriteStats();
            return 0;
        } else if (cfg.mode == "solver-semantic-replay") {
            if (cfg.manualOnly) {
                throw std::runtime_error("solver-semantic-replay does not support --manual-only");
            }
            if (cfg.manifestPath.empty()) {
                throw std::runtime_error("solver-semantic-replay requires --manifest");
            }
            if (cfg.caseName.empty()) {
                throw std::runtime_error("solver-semantic-replay requires --case-name");
            }

            const auto cases = loadRegressionManifest(cfg.manifestPath);
            auto it = std::find_if(cases.begin(),
                                   cases.end(),
                                   [&](const RegressionCaseSpec &spec) {
                                       return spec.name == cfg.caseName;
                                   });
            if (it == cases.end()) {
                throw std::runtime_error("solver-semantic-replay case not found in manifest: " +
                                         cfg.caseName);
            }
            if (!it->inputExplicit.has_value()) {
                throw std::runtime_error(
                    "solver-semantic-replay case is missing inputExplicit snapshot: " +
                    cfg.caseName);
            }

            SolverSemanticReplayBundle bundle;
            bundle.caseName = it->name;
            bundle.manifestPath = cfg.manifestPath;

            SemanticReplayStopPolicy stopPolicy = SemanticReplayStopPolicy::SRSP_RAW_FIRST_DIFF;
            if (cfg.semanticStop == "raw") {
                stopPolicy = SemanticReplayStopPolicy::SRSP_RAW_FIRST_DIFF;
            } else if (cfg.semanticStop == "canonical") {
                stopPolicy = SemanticReplayStopPolicy::SRSP_CANONICAL_FIRST_DIFF;
            } else if (cfg.semanticStop == "end") {
                stopPolicy = SemanticReplayStopPolicy::SRSP_RUN_TO_END;
            } else {
                throw std::runtime_error("solver-semantic-replay requires --semantic-stop raw|canonical|end");
            }

            std::string why;
            setRewriteRCaseContext(it->seed, it->tcIndex);
            const bool ok = runSolverSemanticReplay(*it->inputExplicit, stopPolicy, bundle, why);

            std::ostringstream bundlePathStream;
            bundlePathStream << cfg.dumpDir << "/"
                             << semanticDivergenceKindName(bundle.divergenceKind)
                             << "_seed" << it->seed
                             << "_tc" << it->tcIndex;
            if (bundle.divergenceStepIndex >= 0) {
                bundlePathStream << "_step" << bundle.divergenceStepIndex;
            } else if (it->targetStep >= 0) {
                bundlePathStream << "_step" << it->targetStep;
            }
            bundlePathStream << "_" << sanitizePathComponent(it->name) << ".txt";
            const std::string bundlePath = bundlePathStream.str();
            dumpSolverSemanticReplayBundle(bundle, bundlePath);

            const std::string summaryPath = cfg.dumpDir + "/summary.json";
            std::ofstream ofs(summaryPath);
            ofs << "{\n";
            ofs << "  \"mode\": \"solver-semantic-replay\",\n";
            ofs << "  \"manifestPath\": \"" << jsonEscape(cfg.manifestPath) << "\",\n";
            ofs << "  \"caseName\": \"" << jsonEscape(it->name) << "\",\n";
            ofs << "  \"outputDumpDir\": \"" << jsonEscape(cfg.dumpDir) << "\",\n";
            ofs << "  \"bundlePath\": \"" << jsonEscape(bundlePath) << "\",\n";
            ofs << "  \"stopPolicy\": \""
                << semanticReplayStopPolicyName(bundle.stopPolicy) << "\",\n";
            ofs << "  \"topLevelOk\": " << (bundle.topLevelOk ? "true" : "false")
                << ",\n";
            ofs << "  \"divergenceKind\": \""
                << semanticDivergenceKindName(bundle.divergenceKind) << "\",\n";
            ofs << "  \"divergenceStepIndex\": " << bundle.divergenceStepIndex
                << ",\n";
            ofs << "  \"divergenceWhy\": \"" << jsonEscape(bundle.divergenceWhy) << "\",\n";
            ofs << "  \"rawFirstDivergenceKind\": \""
                << semanticDivergenceKindName(bundle.rawFirstDivergenceKind) << "\",\n";
            ofs << "  \"rawFirstDivergenceStep\": " << bundle.rawFirstDivergenceStep << ",\n";
            ofs << "  \"rawFirstDivergenceWhy\": \""
                << jsonEscape(bundle.rawFirstDivergenceWhy) << "\",\n";
            ofs << "  \"canonicalFirstDivergenceKind\": \""
                << canonicalDivergenceKindName(bundle.canonicalFirstDivergenceKind)
                << "\",\n";
            ofs << "  \"canonicalFirstDivergenceStep\": "
                << bundle.canonicalFirstDivergenceStep << ",\n";
            ofs << "  \"canonicalFirstDivergenceWhy\": \""
                << jsonEscape(bundle.canonicalFirstDivergenceWhy) << "\",\n";
            ofs << "  \"firstOracleWhy\": \"" << jsonEscape(bundle.firstOracleWhy) << "\",\n";
            ofs << "  \"firstRewriteWhy\": \"" << jsonEscape(bundle.firstRewriteWhy) << "\",\n";
            ofs << "  \"oracleTerminatedStep\": " << bundle.oracleTerminatedStep << ",\n";
            ofs << "  \"rewriteTerminatedStep\": " << bundle.rewriteTerminatedStep << ",\n";
            ofs << "  \"canonicalEquivalent\": "
                << (bundle.canonicalEquivalent ? "true" : "false") << ",\n";
            ofs << "  \"canonicalWhy\": \"" << jsonEscape(bundle.canonicalWhy) << "\",\n";
            ofs << "  \"finalCanonicalEquivalent\": "
                << (bundle.finalCanonicalEquivalent ? "true" : "false") << ",\n";
            ofs << "  \"finalRawEquivalent\": "
                << (bundle.finalRawEquivalent ? "true" : "false") << ",\n";
            ofs << "  \"oracleTraceLength\": " << bundle.oracleTrace.size() << ",\n";
            ofs << "  \"rewriteTraceLength\": " << bundle.rewriteTrace.size() << "\n";
            ofs << "}\n";

            std::cout << "[SOLVER_SEMANTIC_REPLAY] case=" << it->name
                      << " stopPolicy=" << semanticReplayStopPolicyName(bundle.stopPolicy)
                      << " divergenceKind="
                      << semanticDivergenceKindName(bundle.divergenceKind)
                      << " divergenceStepIndex=" << bundle.divergenceStepIndex
                      << " bundle=" << bundlePath
                      << " summary=" << summaryPath << "\n";
            if (!why.empty()) {
                std::cout << "[SOLVER_SEMANTIC_REPLAY_WHY] " << why << "\n";
            }
            printRewriteStats();
            return ok ? 0 : 1;
        } else if (cfg.mode == "solver-semantic-target-replay") {
            if (cfg.manualOnly) {
                throw std::runtime_error(
                    "solver-semantic-target-replay does not support --manual-only");
            }
            if (cfg.manifestPath.empty()) {
                throw std::runtime_error(
                    "solver-semantic-target-replay requires --manifest");
            }
            if (cfg.caseName.empty()) {
                throw std::runtime_error(
                    "solver-semantic-target-replay requires --case-name");
            }

            const auto cases = loadRegressionManifest(cfg.manifestPath);
            auto it = std::find_if(cases.begin(),
                                   cases.end(),
                                   [&](const RegressionCaseSpec &spec) {
                                       return spec.name == cfg.caseName;
                                   });
            if (it == cases.end()) {
                throw std::runtime_error(
                    "solver-semantic-target-replay case not found in manifest: " +
                    cfg.caseName);
            }
            if (!it->inputExplicit.has_value()) {
                throw std::runtime_error(
                    "solver-semantic-target-replay case is missing inputExplicit snapshot: " +
                    cfg.caseName);
            }

            SolverSemanticTargetReplayBundle bundle;
            std::string why;
            setRewriteRCaseContext(it->seed, it->tcIndex);
            const bool ok = runSolverSemanticTargetReplayCaseDumpAware(
                *it->inputExplicit,
                cfg.manifestPath,
                it->name,
                it->seed,
                it->tcIndex,
                it->targetStep >= 0 ? std::optional<int>(it->targetStep) : std::nullopt,
                bundle,
                why);

            std::ostringstream bundlePathStream;
            bundlePathStream << cfg.dumpDir << "/"
                             << semanticTargetSeamKindName(bundle.semanticTargetSeamKind)
                             << "_seed" << it->seed
                             << "_tc" << it->tcIndex;
            if (bundle.divergenceStepIndex >= 0) {
                bundlePathStream << "_step" << bundle.divergenceStepIndex;
            } else if (it->targetStep >= 0) {
                bundlePathStream << "_step" << it->targetStep;
            }
            bundlePathStream << "_" << sanitizePathComponent(it->name) << ".txt";
            const std::string bundlePath = bundlePathStream.str();
            dumpSolverSemanticTargetReplayBundle(bundle, bundlePath);

            const std::string summaryPath = cfg.dumpDir + "/summary.json";
            std::ofstream ofs(summaryPath);
            ofs << "{\n";
            ofs << "  \"mode\": \"solver-semantic-target-replay\",\n";
            ofs << "  \"manifestPath\": \"" << jsonEscape(cfg.manifestPath) << "\",\n";
            ofs << "  \"caseName\": \"" << jsonEscape(it->name) << "\",\n";
            ofs << "  \"outputDumpDir\": \"" << jsonEscape(cfg.dumpDir) << "\",\n";
            ofs << "  \"bundlePath\": \"" << jsonEscape(bundlePath) << "\",\n";
            ofs << "  \"topLevelOk\": " << (bundle.topLevelOk ? "true" : "false")
                << ",\n";
            ofs << "  \"semanticTargetSeamKind\": \""
                << semanticTargetSeamKindName(bundle.semanticTargetSeamKind) << "\",\n";
            ofs << "  \"divergenceStepIndex\": " << bundle.divergenceStepIndex << ",\n";
            ofs << "  \"semanticTargetSeamWhy\": \""
                << jsonEscape(bundle.semanticTargetSeamWhy) << "\",\n";
            ofs << "  \"oracleHasNextTarget\": "
                << (bundle.oracleTargetSnapshot.hasNextTarget ? "true" : "false") << ",\n";
            ofs << "  \"rewriteHasNextTarget\": "
                << (bundle.rewriteTargetSnapshot.hasNextTarget ? "true" : "false") << ",\n";
            ofs << "  \"shadowHasNextTarget\": "
                << (bundle.shadowTargetSnapshot.hasNextTarget ? "true" : "false") << ",\n";
            ofs << "  \"oracleChosenR\": " << bundle.oracleTargetSnapshot.chosenR << ",\n";
            ofs << "  \"oracleChosenX\": " << bundle.oracleTargetSnapshot.chosenX << ",\n";
            ofs << "  \"rewriteChosenR\": " << bundle.rewriteTargetSnapshot.chosenR << ",\n";
            ofs << "  \"rewriteChosenX\": " << bundle.rewriteTargetSnapshot.chosenX << ",\n";
            ofs << "  \"shadowChosenR\": " << bundle.shadowTargetSnapshot.chosenR << ",\n";
            ofs << "  \"shadowChosenX\": " << bundle.shadowTargetSnapshot.chosenX << ",\n";
            ofs << "  \"oracleTraceLength\": " << bundle.oracleSemanticTrace.size() << ",\n";
            ofs << "  \"rewriteTraceLength\": " << bundle.rewriteSemanticTrace.size() << ",\n";
            ofs << "  \"oracleWhy\": \"" << jsonEscape(bundle.oracleWhy) << "\",\n";
            ofs << "  \"rewriteWhy\": \"" << jsonEscape(bundle.rewriteWhy) << "\",\n";
            ofs << "  \"shadowWhy\": \"" << jsonEscape(bundle.shadowWhy) << "\"\n";
            ofs << "}\n";

            std::cout << "[SOLVER_SEMANTIC_TARGET_REPLAY] case=" << it->name
                      << " seamKind="
                      << semanticTargetSeamKindName(bundle.semanticTargetSeamKind)
                      << " divergenceStepIndex=" << bundle.divergenceStepIndex
                      << " bundle=" << bundlePath
                      << " summary=" << summaryPath << "\n";
            if (!why.empty()) {
                std::cout << "[SOLVER_SEMANTIC_TARGET_REPLAY_WHY] " << why << "\n";
            }
            printRewriteStats();
            return ok ? 0 : 1;
        } else if (cfg.mode == "solver-semantic-transition-replay") {
            if (cfg.manualOnly) {
                throw std::runtime_error(
                    "solver-semantic-transition-replay does not support --manual-only");
            }
            if (cfg.manifestPath.empty()) {
                throw std::runtime_error(
                    "solver-semantic-transition-replay requires --manifest");
            }
            if (cfg.caseName.empty()) {
                throw std::runtime_error(
                    "solver-semantic-transition-replay requires --case-name");
            }
            if (cfg.sourceStep <= 0) {
                throw std::runtime_error(
                    "solver-semantic-transition-replay requires --source-step > 0");
            }

            const auto cases = loadRegressionManifest(cfg.manifestPath);
            auto it = std::find_if(cases.begin(),
                                   cases.end(),
                                   [&](const RegressionCaseSpec &spec) {
                                       return spec.name == cfg.caseName;
                                   });
            if (it == cases.end()) {
                throw std::runtime_error(
                    "solver-semantic-transition-replay case not found in manifest: " +
                    cfg.caseName);
            }
            if (!it->inputExplicit.has_value()) {
                throw std::runtime_error(
                    "solver-semantic-transition-replay case is missing inputExplicit snapshot: " +
                    cfg.caseName);
            }

            SolverSemanticTransitionReplayBundle bundle;
            std::string why;
            setRewriteRCaseContext(it->seed, it->tcIndex);
            const bool ok = runSolverSemanticTransitionReplayCaseDumpAware(
                *it->inputExplicit,
                cfg.manifestPath,
                it->name,
                it->seed,
                it->tcIndex,
                it->targetStep >= 0 ? std::optional<int>(it->targetStep) : std::nullopt,
                cfg.sourceStep,
                bundle,
                why);

            std::ostringstream bundlePathStream;
            bundlePathStream << cfg.dumpDir << "/"
                             << transitionSeamKindName(bundle.transitionSeamKind)
                             << "_seed" << it->seed
                             << "_tc" << it->tcIndex
                             << "_step" << cfg.sourceStep
                             << "_" << sanitizePathComponent(it->name) << ".txt";
            const std::string bundlePath = bundlePathStream.str();
            dumpSolverSemanticTransitionReplayBundle(bundle, bundlePath);

            const std::string summaryPath = cfg.dumpDir + "/summary.json";
            std::ofstream ofs(summaryPath);
            ofs << "{\n";
            ofs << "  \"mode\": \"solver-semantic-transition-replay\",\n";
            ofs << "  \"manifestPath\": \"" << jsonEscape(cfg.manifestPath) << "\",\n";
            ofs << "  \"caseName\": \"" << jsonEscape(it->name) << "\",\n";
            ofs << "  \"sourceStep\": " << cfg.sourceStep << ",\n";
            ofs << "  \"outputDumpDir\": \"" << jsonEscape(cfg.dumpDir) << "\",\n";
            ofs << "  \"bundlePath\": \"" << jsonEscape(bundlePath) << "\",\n";
            ofs << "  \"topLevelOk\": " << (bundle.topLevelOk ? "true" : "false")
                << ",\n";
            ofs << "  \"transitionSeamKind\": \""
                << transitionSeamKindName(bundle.transitionSeamKind) << "\",\n";
            ofs << "  \"transitionSeamWhy\": \""
                << jsonEscape(bundle.transitionSeamWhy) << "\",\n";
            ofs << "  \"oracleChosenR\": " << bundle.oracleTransitionSnapshot.chosenR << ",\n";
            ofs << "  \"oracleChosenX\": " << bundle.oracleTransitionSnapshot.chosenX << ",\n";
            ofs << "  \"rewriteChosenR\": " << bundle.rewriteTransitionSnapshot.chosenR << ",\n";
            ofs << "  \"rewriteChosenX\": " << bundle.rewriteTransitionSnapshot.chosenX << ",\n";
            ofs << "  \"sharedExplicitWhy\": \""
                << jsonEscape(bundle.sharedExplicitWhy) << "\",\n";
            ofs << "  \"oracleWhy\": \"" << jsonEscape(bundle.oracleWhy) << "\",\n";
            ofs << "  \"rewriteWhy\": \"" << jsonEscape(bundle.rewriteWhy) << "\",\n";
            ofs << "  \"shadowWhy\": \"" << jsonEscape(bundle.shadowWhy) << "\"\n";
            ofs << "}\n";

            std::cout << "[SOLVER_SEMANTIC_TRANSITION_REPLAY] case=" << it->name
                      << " sourceStep=" << cfg.sourceStep
                      << " seamKind=" << transitionSeamKindName(bundle.transitionSeamKind)
                      << " bundle=" << bundlePath
                      << " summary=" << summaryPath << "\n";
            if (!why.empty()) {
                std::cout << "[SOLVER_SEMANTIC_TRANSITION_REPLAY_WHY] " << why << "\n";
            }
            printRewriteStats();
            return ok ? 0 : 1;
        } else if (cfg.mode == "solver-handoff-replay") {
            if (cfg.manualOnly) {
                throw std::runtime_error(
                    "solver-handoff-replay does not support --manual-only");
            }
            if (cfg.manifestPath.empty()) {
                throw std::runtime_error("solver-handoff-replay requires --manifest");
            }
            if (cfg.caseName.empty()) {
                throw std::runtime_error("solver-handoff-replay requires --case-name");
            }

            const auto cases = loadRegressionManifest(cfg.manifestPath);
            auto it = std::find_if(cases.begin(),
                                   cases.end(),
                                   [&](const RegressionCaseSpec &spec) {
                                       return spec.name == cfg.caseName;
                                   });
            if (it == cases.end()) {
                throw std::runtime_error(
                    "solver-handoff-replay case not found in manifest: " +
                    cfg.caseName);
            }
            if (!it->inputExplicit.has_value()) {
                throw std::runtime_error(
                    "solver-handoff-replay case is missing inputExplicit snapshot: " +
                    cfg.caseName);
            }

            SolverHandoffReplayBundle bundle;
            std::string why;
            setRewriteRCaseContext(it->seed, it->tcIndex);
            const bool ok = runSolverHandoffReplayCaseDumpAware(
                *it->inputExplicit,
                cfg.manifestPath,
                it->name,
                it->seed,
                it->tcIndex,
                it->targetStep >= 0 ? std::optional<int>(it->targetStep) : std::nullopt,
                bundle,
                why);

            std::ostringstream bundlePathStream;
            bundlePathStream << cfg.dumpDir << "/"
                             << handoffSeamKindName(bundle.handoffSeamKind)
                             << "_seed" << it->seed
                             << "_tc" << it->tcIndex
                             << "_step" << bundle.handoffStepIndex
                             << "_" << sanitizePathComponent(it->name) << ".txt";
            const std::string bundlePath = bundlePathStream.str();
            dumpSolverHandoffReplayBundle(bundle, bundlePath);

            const std::string summaryPath = cfg.dumpDir + "/summary.json";
            std::ofstream ofs(summaryPath);
            ofs << "{\n";
            ofs << "  \"mode\": \"solver-handoff-replay\",\n";
            ofs << "  \"manifestPath\": \"" << jsonEscape(cfg.manifestPath) << "\",\n";
            ofs << "  \"caseName\": \"" << jsonEscape(it->name) << "\",\n";
            ofs << "  \"outputDumpDir\": \"" << jsonEscape(cfg.dumpDir) << "\",\n";
            ofs << "  \"bundlePath\": \"" << jsonEscape(bundlePath) << "\",\n";
            ofs << "  \"topLevelOk\": " << (bundle.topLevelOk ? "true" : "false")
                << ",\n";
            ofs << "  \"handoffStepIndex\": " << bundle.handoffStepIndex << ",\n";
            ofs << "  \"handoffSeamKind\": \""
                << handoffSeamKindName(bundle.handoffSeamKind) << "\",\n";
            ofs << "  \"handoffSeamWhy\": \""
                << jsonEscape(bundle.handoffSeamWhy) << "\",\n";
            ofs << "  \"oracleNextInputSourceKind\": \""
                << stepHandoffSourceKindName(
                       bundle.oracleHandoffSnapshot.nextInputSourceKind)
                << "\",\n";
            ofs << "  \"rewriteNextInputSourceKind\": \""
                << stepHandoffSourceKindName(
                       bundle.rewriteHandoffSnapshot.nextInputSourceKind)
                << "\",\n";
            ofs << "  \"oracleChosenR\": " << bundle.oracleHandoffSnapshot.chosenR << ",\n";
            ofs << "  \"oracleChosenX\": " << bundle.oracleHandoffSnapshot.chosenX << ",\n";
            ofs << "  \"rewriteChosenR\": " << bundle.rewriteHandoffSnapshot.chosenR
                << ",\n";
            ofs << "  \"rewriteChosenX\": " << bundle.rewriteHandoffSnapshot.chosenX
                << ",\n";
            ofs << "  \"oracleNextMatchesTransitionShared\": "
                << (bundle.oracleNextMatchesTransitionShared ? "true" : "false")
                << ",\n";
            ofs << "  \"rewriteNextMatchesTransitionShared\": "
                << (bundle.rewriteNextMatchesTransitionShared ? "true" : "false")
                << ",\n";
            ofs << "  \"transitionSharedWhy\": \""
                << jsonEscape(bundle.transitionSharedWhy) << "\",\n";
            ofs << "  \"oracleWhy\": \"" << jsonEscape(bundle.oracleWhy) << "\",\n";
            ofs << "  \"rewriteWhy\": \"" << jsonEscape(bundle.rewriteWhy) << "\"\n";
            ofs << "}\n";

            std::cout << "[SOLVER_HANDOFF_REPLAY] case=" << it->name
                      << " handoffStepIndex=" << bundle.handoffStepIndex
                      << " seamKind=" << handoffSeamKindName(bundle.handoffSeamKind)
                      << " bundle=" << bundlePath
                      << " summary=" << summaryPath << "\n";
            if (!why.empty()) {
                std::cout << "[SOLVER_HANDOFF_REPLAY_WHY] " << why << "\n";
            }
            printRewriteStats();
            return ok ? 0 : 1;
        } else if (cfg.mode == "solver-step-transition-replay") {
            if (cfg.manualOnly) {
                throw std::runtime_error(
                    "solver-step-transition-replay does not support --manual-only");
            }
            if (cfg.manifestPath.empty()) {
                throw std::runtime_error(
                    "solver-step-transition-replay requires --manifest");
            }
            if (cfg.caseName.empty()) {
                throw std::runtime_error(
                    "solver-step-transition-replay requires --case-name");
            }

            const auto cases = loadRegressionManifest(cfg.manifestPath);
            auto it = std::find_if(cases.begin(),
                                   cases.end(),
                                   [&](const RegressionCaseSpec &spec) {
                                       return spec.name == cfg.caseName;
                                   });
            if (it == cases.end()) {
                throw std::runtime_error(
                    "solver-step-transition-replay case not found in manifest: " +
                    cfg.caseName);
            }
            if (!it->inputExplicit.has_value()) {
                throw std::runtime_error(
                    "solver-step-transition-replay case is missing inputExplicit snapshot: " +
                    cfg.caseName);
            }

            SolverStepTransitionReplayBundle bundle;
            std::string why;
            setRewriteRCaseContext(it->seed, it->tcIndex);
            const bool ok = runSolverStepTransitionReplayCaseDumpAware(
                *it->inputExplicit,
                cfg.manifestPath,
                it->name,
                it->seed,
                it->tcIndex,
                it->targetStep >= 0 ? std::optional<int>(it->targetStep) : std::nullopt,
                bundle,
                why);

            std::ostringstream bundlePathStream;
            bundlePathStream << cfg.dumpDir << "/"
                             << stepTransitionSeamKindName(bundle.stepTransitionSeamKind)
                             << "_seed" << it->seed
                             << "_tc" << it->tcIndex
                             << "_step" << bundle.sourceStep
                             << "_" << sanitizePathComponent(it->name) << ".txt";
            const std::string bundlePath = bundlePathStream.str();
            dumpSolverStepTransitionReplayBundle(bundle, bundlePath);

            const std::string summaryPath = cfg.dumpDir + "/summary.json";
            std::ofstream ofs(summaryPath);
            ofs << "{\n";
            ofs << "  \"mode\": \"solver-step-transition-replay\",\n";
            ofs << "  \"manifestPath\": \"" << jsonEscape(cfg.manifestPath) << "\",\n";
            ofs << "  \"caseName\": \"" << jsonEscape(it->name) << "\",\n";
            ofs << "  \"outputDumpDir\": \"" << jsonEscape(cfg.dumpDir) << "\",\n";
            ofs << "  \"bundlePath\": \"" << jsonEscape(bundlePath) << "\",\n";
            ofs << "  \"topLevelOk\": " << (bundle.topLevelOk ? "true" : "false")
                << ",\n";
            ofs << "  \"sourceStep\": " << bundle.sourceStep << ",\n";
            ofs << "  \"stepTransitionSeamKind\": \""
                << stepTransitionSeamKindName(bundle.stepTransitionSeamKind) << "\",\n";
            ofs << "  \"stepTransitionSeamWhy\": \""
                << jsonEscape(bundle.stepTransitionSeamWhy) << "\",\n";
            ofs << "  \"solverChosenR\": " << bundle.solverStep1Snapshot.chosenR << ",\n";
            ofs << "  \"solverChosenX\": " << bundle.solverStep1Snapshot.chosenX << ",\n";
            ofs << "  \"rewriteChosenR\": " << bundle.rewriteStep1Snapshot.chosenR << ",\n";
            ofs << "  \"rewriteChosenX\": " << bundle.rewriteStep1Snapshot.chosenX << ",\n";
            ofs << "  \"solverNextInputSourceTag\": \""
                << jsonEscape(bundle.solverStep1Snapshot.nextInputSourceTag) << "\",\n";
            ofs << "  \"rewriteNextInputSourceTag\": \""
                << jsonEscape(bundle.rewriteStep1Snapshot.nextInputSourceTag) << "\",\n";
            ofs << "  \"hasShadowStep2Snapshot\": "
                << (bundle.hasShadowStep2Snapshot ? "true" : "false") << ",\n";
            ofs << "  \"solverWhy\": \"" << jsonEscape(bundle.solverWhy) << "\",\n";
            ofs << "  \"rewriteWhy\": \"" << jsonEscape(bundle.rewriteWhy) << "\",\n";
            ofs << "  \"shadowWhy\": \"" << jsonEscape(bundle.shadowWhy) << "\"\n";
            ofs << "}\n";

            std::cout << "[SOLVER_STEP_TRANSITION_REPLAY] case=" << it->name
                      << " seamKind="
                      << stepTransitionSeamKindName(bundle.stepTransitionSeamKind)
                      << " bundle=" << bundlePath
                      << " summary=" << summaryPath << "\n";
            if (!why.empty()) {
                std::cout << "[SOLVER_STEP_TRANSITION_REPLAY_WHY] " << why << "\n";
            }
            printRewriteStats();
            return ok ? 0 : 1;
        } else if (cfg.mode == "solver-handoff-policy-replay") {
            if (cfg.manualOnly) {
                throw std::runtime_error(
                    "solver-handoff-policy-replay does not support --manual-only");
            }
            if (cfg.manifestPath.empty()) {
                throw std::runtime_error(
                    "solver-handoff-policy-replay requires --manifest");
            }
            if (cfg.caseName.empty()) {
                throw std::runtime_error(
                    "solver-handoff-policy-replay requires --case-name");
            }
            const OracleHandoffPolicy oracleHandoffPolicy =
                parseOracleHandoffPolicyArg(cfg.oracleHandoff);

            const auto cases = loadRegressionManifest(cfg.manifestPath);
            auto it = std::find_if(cases.begin(),
                                   cases.end(),
                                   [&](const RegressionCaseSpec &spec) {
                                       return spec.name == cfg.caseName;
                                   });
            if (it == cases.end()) {
                throw std::runtime_error(
                    "solver-handoff-policy-replay case not found in manifest: " +
                    cfg.caseName);
            }
            if (!it->inputExplicit.has_value()) {
                throw std::runtime_error(
                    "solver-handoff-policy-replay case is missing inputExplicit snapshot: " +
                    cfg.caseName);
            }

            SolverHandoffPolicyReplayBundle bundle;
            std::string why;
            setRewriteRCaseContext(it->seed, it->tcIndex);
            const bool ok = runSolverHandoffPolicyReplayCaseDumpAware(
                *it->inputExplicit,
                cfg.manifestPath,
                it->name,
                it->seed,
                it->tcIndex,
                it->targetStep >= 0 ? std::optional<int>(it->targetStep) : std::nullopt,
                oracleHandoffPolicy,
                bundle,
                why);

            std::ostringstream bundlePathStream;
            bundlePathStream << cfg.dumpDir << "/"
                             << sanitizePathComponent(cfg.oracleHandoff)
                             << "_seed" << it->seed
                             << "_tc" << it->tcIndex
                             << "_" << sanitizePathComponent(it->name) << ".txt";
            const std::string bundlePath = bundlePathStream.str();
            dumpSolverHandoffPolicyReplayBundle(bundle, bundlePath);

            const std::string summaryPath = cfg.dumpDir + "/summary.json";
            std::ofstream ofs(summaryPath);
            ofs << "{\n";
            ofs << "  \"mode\": \"solver-handoff-policy-replay\",\n";
            ofs << "  \"manifestPath\": \"" << jsonEscape(cfg.manifestPath) << "\",\n";
            ofs << "  \"caseName\": \"" << jsonEscape(it->name) << "\",\n";
            ofs << "  \"outputDumpDir\": \"" << jsonEscape(cfg.dumpDir) << "\",\n";
            ofs << "  \"bundlePath\": \"" << jsonEscape(bundlePath) << "\",\n";
            ofs << "  \"topLevelOk\": " << (bundle.topLevelOk ? "true" : "false")
                << ",\n";
            ofs << "  \"oracleHandoffPolicy\": \""
                << oracleHandoffPolicyName(bundle.oracleHandoffPolicy) << "\",\n";
            ofs << "  \"deleteVsRewriteCanonicalEqual\": "
                << (bundle.deleteVsRewriteCanonicalEqual ? "true" : "false") << ",\n";
            ofs << "  \"normalizeVsRewriteCanonicalEqual\": "
                << (bundle.normalizeVsRewriteCanonicalEqual ? "true" : "false")
                << ",\n";
            ofs << "  \"deleteVsNormalizeCanonicalEqual\": "
                << (bundle.deleteVsNormalizeCanonicalEqual ? "true" : "false")
                << ",\n";
            ofs << "  \"oracleDeleteWhy\": \"" << jsonEscape(bundle.oracleDeleteWhy)
                << "\",\n";
            ofs << "  \"oracleNormalizeWhy\": \""
                << jsonEscape(bundle.oracleNormalizeWhy) << "\",\n";
            ofs << "  \"rewriteWhy\": \"" << jsonEscape(bundle.rewriteWhy) << "\"\n";
            ofs << "}\n";

            std::cout << "[SOLVER_HANDOFF_POLICY_REPLAY] case=" << it->name
                      << " oracleHandoffPolicy="
                      << oracleHandoffPolicyName(bundle.oracleHandoffPolicy)
                      << " bundle=" << bundlePath
                      << " summary=" << summaryPath << "\n";
            if (!why.empty()) {
                std::cout << "[SOLVER_HANDOFF_POLICY_REPLAY_WHY] " << why << "\n";
            }
            printRewriteStats();
            return ok ? 0 : 1;
        } else if (cfg.mode == "solver-compare-replay") {
            if (cfg.manualOnly) {
                throw std::runtime_error("solver-compare-replay does not support --manual-only");
            }
            if (cfg.manifestPath.empty()) {
                throw std::runtime_error("solver-compare-replay requires --manifest");
            }
            if (cfg.caseName.empty()) {
                throw std::runtime_error("solver-compare-replay requires --case-name");
            }

            const auto cases = loadRegressionManifest(cfg.manifestPath);
            auto it = std::find_if(cases.begin(),
                                   cases.end(),
                                   [&](const RegressionCaseSpec &spec) {
                                       return spec.name == cfg.caseName;
                                   });
            if (it == cases.end()) {
                throw std::runtime_error("solver-compare-replay case not found in manifest: " +
                                         cfg.caseName);
            }
            if (!it->inputExplicit.has_value()) {
                throw std::runtime_error(
                    "solver-compare-replay case is missing inputExplicit snapshot: " +
                    cfg.caseName);
            }

            SolverCompareReplayBundle bundle;
            std::string why;
            const bool ok = runSolverCompareReplayCaseDumpAware(*it->inputExplicit,
                                                                cfg.manifestPath,
                                                                it->name,
                                                                it->seed,
                                                                it->tcIndex,
                                                                it->targetStep >= 0
                                                                    ? std::optional<int>(it->targetStep)
                                                                    : std::nullopt,
                                                                bundle,
                                                                why);

            std::ostringstream bundlePathStream;
            bundlePathStream << cfg.dumpDir << "/"
                             << compareAssemblySeamKindName(bundle.compareAssemblySeamKind)
                             << "_seed" << it->seed
                             << "_tc" << it->tcIndex;
            if (it->targetStep >= 0) {
                bundlePathStream << "_step" << it->targetStep;
            }
            bundlePathStream << "_" << sanitizePathComponent(it->name) << ".txt";
            const std::string bundlePath = bundlePathStream.str();
            dumpSolverCompareReplayBundle(bundle, bundlePath);

            const std::string summaryPath = cfg.dumpDir + "/summary.json";
            std::ofstream ofs(summaryPath);
            ofs << "{\n";
            ofs << "  \"mode\": \"solver-compare-replay\",\n";
            ofs << "  \"manifestPath\": \"" << jsonEscape(cfg.manifestPath) << "\",\n";
            ofs << "  \"caseName\": \"" << jsonEscape(it->name) << "\",\n";
            ofs << "  \"outputDumpDir\": \"" << jsonEscape(cfg.dumpDir) << "\",\n";
            ofs << "  \"bundlePath\": \"" << jsonEscape(bundlePath) << "\",\n";
            ofs << "  \"topLevelOk\": " << (bundle.topLevelOk ? "true" : "false") << ",\n";
            ofs << "  \"compareAssemblySeamKind\": \""
                << compareAssemblySeamKindName(bundle.compareAssemblySeamKind) << "\",\n";
            ofs << "  \"compareAssemblyWhy\": \""
                << jsonEscape(bundle.compareAssemblyWhy) << "\",\n";
            ofs << "  \"firstMismatchDescription\": \""
                << jsonEscape(bundle.firstMismatchDescription) << "\",\n";
            if (bundle.oracleSolverVsReplayEqualRaw.has_value()) {
                ofs << "  \"oracleSolverVsReplayEqualRaw\": "
                    << (*bundle.oracleSolverVsReplayEqualRaw ? "true" : "false") << ",\n";
            }
            if (bundle.oracleSolverVsReplayEqualCanonical.has_value()) {
                ofs << "  \"oracleSolverVsReplayEqualCanonical\": "
                    << (*bundle.oracleSolverVsReplayEqualCanonical ? "true" : "false")
                    << ",\n";
            }
            if (bundle.rewriteSolverVsReplayEqualRaw.has_value()) {
                ofs << "  \"rewriteSolverVsReplayEqualRaw\": "
                    << (*bundle.rewriteSolverVsReplayEqualRaw ? "true" : "false")
                    << ",\n";
            }
            if (bundle.rewriteSolverVsReplayEqualCanonical.has_value()) {
                ofs << "  \"rewriteSolverVsReplayEqualCanonical\": "
                    << (*bundle.rewriteSolverVsReplayEqualCanonical ? "true" : "false")
                    << ",\n";
            }
            if (bundle.oracleVsRewriteEqualRaw.has_value()) {
                ofs << "  \"oracleVsRewriteEqualRaw\": "
                    << (*bundle.oracleVsRewriteEqualRaw ? "true" : "false") << ",\n";
            }
            if (bundle.oracleVsRewriteEqualCanonical.has_value()) {
                ofs << "  \"oracleVsRewriteEqualCanonical\": "
                    << (*bundle.oracleVsRewriteEqualCanonical ? "true" : "false")
                    << ",\n";
            }
            ofs << "  \"oracleReplayTerminatedStep\": " << bundle.oracleReplayTerminatedStep
                << ",\n";
            ofs << "  \"rewriteReplayTerminatedStep\": " << bundle.rewriteReplayTerminatedStep
                << ",\n";
            ofs << "  \"oracleSolverWhy\": \"" << jsonEscape(bundle.oracleSolverWhy)
                << "\",\n";
            ofs << "  \"rewriteSolverWhy\": \"" << jsonEscape(bundle.rewriteSolverWhy)
                << "\",\n";
            ofs << "  \"oracleReplayWhy\": \"" << jsonEscape(bundle.oracleReplayWhy)
                << "\",\n";
            ofs << "  \"rewriteReplayWhy\": \"" << jsonEscape(bundle.rewriteReplayWhy)
                << "\",\n";
            ofs << "  \"rewriteSolverOutputDebugTag\": \""
                << jsonEscape(bundle.rewriteSolverOutputDebugTag) << "\",\n";
            ofs << "  \"rewriteTerminalAssemblyWhy\": \""
                << jsonEscape(bundle.rewriteTerminalAssemblyWhy) << "\"\n";
            ofs << "}\n";

            std::cout << "[SOLVER_COMPARE_REPLAY] case=" << it->name
                      << " seamKind="
                      << compareAssemblySeamKindName(bundle.compareAssemblySeamKind)
                      << " bundle=" << bundlePath
                      << " summary=" << summaryPath << "\n";
            if (!why.empty()) {
                std::cout << "[SOLVER_COMPARE_REPLAY_WHY] " << why << "\n";
            }
            printRewriteStats();
            return ok ? 0 : 1;
        } else if (cfg.mode == "solver-finalcore-replay") {
            if (cfg.manualOnly) {
                throw std::runtime_error("solver-finalcore-replay does not support --manual-only");
            }
            if (cfg.manifestPath.empty()) {
                throw std::runtime_error("solver-finalcore-replay requires --manifest");
            }
            if (cfg.caseName.empty()) {
                throw std::runtime_error("solver-finalcore-replay requires --case-name");
            }

            const auto cases = loadRegressionManifest(cfg.manifestPath);
            auto it = std::find_if(cases.begin(),
                                   cases.end(),
                                   [&](const RegressionCaseSpec &spec) {
                                       return spec.name == cfg.caseName;
                                   });
            if (it == cases.end()) {
                throw std::runtime_error("solver-finalcore-replay case not found in manifest: " +
                                         cfg.caseName);
            }
            if (!it->inputExplicit.has_value()) {
                throw std::runtime_error(
                    "solver-finalcore-replay case is missing inputExplicit snapshot: " +
                    cfg.caseName);
            }

            SolverFinalCoreReplayBundle bundle;
            std::string why;
            const bool ok = runSolverFinalCoreReplayCaseDumpAware(
                *it->inputExplicit,
                cfg.manifestPath,
                it->name,
                it->seed,
                it->tcIndex,
                it->targetStep >= 0 ? std::optional<int>(it->targetStep) : std::nullopt,
                bundle,
                why);

            std::ostringstream bundlePathStream;
            bundlePathStream << cfg.dumpDir << "/"
                             << finalCoreSeamKindName(bundle.finalCoreSeamKind)
                             << "_seed" << it->seed
                             << "_tc" << it->tcIndex;
            if (bundle.firstDivergenceStep >= 0) {
                bundlePathStream << "_step" << bundle.firstDivergenceStep;
            } else if (it->targetStep >= 0) {
                bundlePathStream << "_step" << it->targetStep;
            }
            bundlePathStream << "_" << sanitizePathComponent(it->name) << ".txt";
            const std::string bundlePath = bundlePathStream.str();
            dumpSolverFinalCoreReplayBundle(bundle, bundlePath);

            const std::string summaryPath = cfg.dumpDir + "/summary.json";
            std::ofstream ofs(summaryPath);
            ofs << "{\n";
            ofs << "  \"mode\": \"solver-finalcore-replay\",\n";
            ofs << "  \"manifestPath\": \"" << jsonEscape(cfg.manifestPath) << "\",\n";
            ofs << "  \"caseName\": \"" << jsonEscape(it->name) << "\",\n";
            ofs << "  \"outputDumpDir\": \"" << jsonEscape(cfg.dumpDir) << "\",\n";
            ofs << "  \"bundlePath\": \"" << jsonEscape(bundlePath) << "\",\n";
            ofs << "  \"topLevelOk\": " << (bundle.topLevelOk ? "true" : "false") << ",\n";
            ofs << "  \"finalCoreSeamKind\": \""
                << finalCoreSeamKindName(bundle.finalCoreSeamKind) << "\",\n";
            ofs << "  \"firstDivergenceStep\": " << bundle.firstDivergenceStep << ",\n";
            ofs << "  \"finalCoreSeamWhy\": \""
                << jsonEscape(bundle.finalCoreSeamWhy) << "\",\n";
            ofs << "  \"targetSearchComparedStep\": "
                << bundle.targetSearchComparedStep << ",\n";
            ofs << "  \"targetSearchSeamKind\": \""
                << targetSearchSeamKindName(bundle.targetSearchSeamKind) << "\",\n";
            ofs << "  \"targetSearchSeamWhy\": \""
                << jsonEscape(bundle.targetSearchSeamWhy) << "\",\n";
            ofs << "  \"solverTraceLength\": " << bundle.solverTrace.size() << ",\n";
            ofs << "  \"replayTraceLength\": " << bundle.replayTrace.size() << ",\n";
            ofs << "  \"solverTargetSnapshotCount\": "
                << bundle.solverPostStepTargetSnapshots.size() << ",\n";
            ofs << "  \"replayTargetSnapshotCount\": "
                << bundle.replayPostStepTargetSnapshots.size() << ",\n";
            ofs << "  \"solverStatsCompletedSteps\": " << bundle.solverStats.completedSteps
                << ",\n";
            ofs << "  \"solverStatsReachedFixpoint\": "
                << (bundle.solverStats.reachedFixpoint ? "true" : "false") << ",\n";
            ofs << "  \"solverStatsHadSequenceFallback\": "
                << (bundle.solverStats.hadSequenceFallback ? "true" : "false") << ",\n";
            ofs << "  \"solverShadowResyncAttemptCount\": "
                << bundle.solverStats.solverShadowResyncAttemptCount << ",\n";
            ofs << "  \"solverShadowResyncAppliedCount\": "
                << bundle.solverStats.solverShadowResyncAppliedCount << ",\n";
            ofs << "  \"solverShadowResyncNoopCount\": "
                << bundle.solverStats.solverShadowResyncNoopCount << ",\n";
            ofs << "  \"solverShadowResyncNoTargetToHasTargetCount\": "
                << bundle.solverStats.solverShadowResyncNoTargetToHasTargetCount << ",\n";
            ofs << "  \"solverShadowResyncAliveRSetDifferCount\": "
                << bundle.solverStats.solverShadowResyncAliveRSetDifferCount << ",\n";
            ofs << "  \"solverWhy\": \"" << jsonEscape(bundle.solverWhy) << "\",\n";
            ofs << "  \"replayWhy\": \"" << jsonEscape(bundle.replayWhy) << "\"\n";
            ofs << "}\n";

            std::cout << "[SOLVER_FINALCORE_REPLAY] case=" << it->name
                      << " seamKind=" << finalCoreSeamKindName(bundle.finalCoreSeamKind)
                      << " targetSeamKind="
                      << targetSearchSeamKindName(bundle.targetSearchSeamKind)
                      << " firstDivergenceStep=" << bundle.firstDivergenceStep
                      << " bundle=" << bundlePath
                      << " summary=" << summaryPath << "\n";
            if (!why.empty()) {
                std::cout << "[SOLVER_FINALCORE_REPLAY_WHY] " << why << "\n";
            }
            printRewriteStats();
            return ok ? 0 : 1;
        } else if (cfg.mode == "solver-shape-replay") {
            if (cfg.manualOnly) {
                throw std::runtime_error("solver-shape-replay does not support --manual-only");
            }
            if (cfg.manifestPath.empty()) {
                throw std::runtime_error("solver-shape-replay requires --manifest");
            }
            if (cfg.caseName.empty()) {
                throw std::runtime_error("solver-shape-replay requires --case-name");
            }

            const auto cases = loadRegressionManifest(cfg.manifestPath);
            auto it = std::find_if(cases.begin(),
                                   cases.end(),
                                   [&](const RegressionCaseSpec &spec) {
                                       return spec.name == cfg.caseName;
                                   });
            if (it == cases.end()) {
                throw std::runtime_error("solver-shape-replay case not found in manifest: " +
                                         cfg.caseName);
            }
            if (!it->inputExplicit.has_value()) {
                throw std::runtime_error(
                    "solver-shape-replay case is missing inputExplicit snapshot: " +
                    cfg.caseName);
            }

            SolverShapeReplayBundle bundle;
            std::string why;
            const bool ok = runSolverShapeReplayCaseDumpAware(
                *it->inputExplicit,
                cfg.manifestPath,
                it->name,
                it->seed,
                it->tcIndex,
                it->targetStep >= 0 ? std::optional<int>(it->targetStep) : std::nullopt,
                bundle,
                why);

            std::ostringstream bundlePathStream;
            bundlePathStream << cfg.dumpDir << "/"
                             << coreShapeSeamKindName(bundle.coreShapeSeamKind)
                             << "_seed" << it->seed
                             << "_tc" << it->tcIndex;
            if (bundle.seamStepIndex >= 0) {
                bundlePathStream << "_step" << bundle.seamStepIndex;
            } else if (it->targetStep >= 0) {
                bundlePathStream << "_step" << it->targetStep;
            }
            bundlePathStream << "_" << sanitizePathComponent(it->name) << ".txt";
            const std::string bundlePath = bundlePathStream.str();
            dumpSolverShapeReplayBundle(bundle, bundlePath);

            const std::string summaryPath = cfg.dumpDir + "/summary.json";
            std::ofstream ofs(summaryPath);
            ofs << "{\n";
            ofs << "  \"mode\": \"solver-shape-replay\",\n";
            ofs << "  \"manifestPath\": \"" << jsonEscape(cfg.manifestPath) << "\",\n";
            ofs << "  \"caseName\": \"" << jsonEscape(it->name) << "\",\n";
            ofs << "  \"outputDumpDir\": \"" << jsonEscape(cfg.dumpDir) << "\",\n";
            ofs << "  \"bundlePath\": \"" << jsonEscape(bundlePath) << "\",\n";
            ofs << "  \"topLevelOk\": " << (bundle.topLevelOk ? "true" : "false") << ",\n";
            ofs << "  \"seamStepIndex\": " << bundle.seamStepIndex << ",\n";
            ofs << "  \"coreShapeSeamKind\": \""
                << coreShapeSeamKindName(bundle.coreShapeSeamKind) << "\",\n";
            ofs << "  \"coreShapeSeamWhy\": \""
                << jsonEscape(bundle.coreShapeSeamWhy) << "\",\n";
            ofs << "  \"solverWhy\": \"" << jsonEscape(bundle.solverWhy) << "\",\n";
            ofs << "  \"replayWhy\": \"" << jsonEscape(bundle.replayWhy) << "\",\n";
            ofs << "  \"shadowWhy\": \"" << jsonEscape(bundle.shadowWhy) << "\"\n";
            ofs << "}\n";

            std::cout << "[SOLVER_SHAPE_REPLAY] case=" << it->name
                      << " seamKind=" << coreShapeSeamKindName(bundle.coreShapeSeamKind)
                      << " seamStepIndex=" << bundle.seamStepIndex
                      << " bundle=" << bundlePath
                      << " summary=" << summaryPath << "\n";
            if (!why.empty()) {
                std::cout << "[SOLVER_SHAPE_REPLAY_WHY] " << why << "\n";
            }
            printRewriteStats();
            return ok ? 0 : 1;
        } else if (cfg.mode == "explicit-core-builder-replay") {
            if (cfg.manualOnly) {
                throw std::runtime_error(
                    "explicit-core-builder-replay does not support --manual-only");
            }
            if (cfg.manifestPath.empty()) {
                throw std::runtime_error("explicit-core-builder-replay requires --manifest");
            }
            if (cfg.caseName.empty()) {
                throw std::runtime_error("explicit-core-builder-replay requires --case-name");
            }
            if (cfg.sourceStep <= 0) {
                throw std::runtime_error("explicit-core-builder-replay requires --source-step > 0");
            }
            if (cfg.sourceKind != "step" && cfg.sourceKind != "handoff") {
                throw std::runtime_error(
                    "explicit-core-builder-replay requires --source-kind step|handoff");
            }
            if (cfg.sourceKind == "handoff") {
                if (cfg.sourceStep != 1) {
                    throw std::runtime_error(
                        "explicit-core-builder-replay with --source-kind handoff requires --source-step 1");
                }
            } else if (cfg.sourceSide != "replay" &&
                       cfg.sourceSide != "solver" &&
                       cfg.sourceSide != "shadow") {
                throw std::runtime_error(
                    "explicit-core-builder-replay requires --source-side replay|solver|shadow");
            }

            const auto cases = loadRegressionManifest(cfg.manifestPath);
            auto it = std::find_if(cases.begin(),
                                   cases.end(),
                                   [&](const RegressionCaseSpec &spec) {
                                       return spec.name == cfg.caseName;
                                   });
            if (it == cases.end()) {
                throw std::runtime_error(
                    "explicit-core-builder-replay case not found in manifest: " +
                    cfg.caseName);
            }
            if (!it->inputExplicit.has_value()) {
                throw std::runtime_error(
                    "explicit-core-builder-replay case is missing inputExplicit snapshot: " +
                    cfg.caseName);
            }

            ExplicitCoreBuilderReplayBundle bundle;
            std::string why;
            const bool ok = runExplicitCoreBuilderReplayCaseDumpAware(
                *it->inputExplicit,
                cfg.manifestPath,
                it->name,
                it->seed,
                it->tcIndex,
                it->targetStep >= 0 ? std::optional<int>(it->targetStep) : std::nullopt,
                cfg.sourceStep,
                cfg.sourceKind,
                cfg.sourceSide,
                bundle,
                why);

            const std::string sourceLabel =
                cfg.sourceKind == "handoff" ? "handoff" : cfg.sourceSide;
            std::ostringstream bundlePathStream;
            bundlePathStream << cfg.dumpDir << "/"
                             << builderPipelineSeamKindName(bundle.builderPipelineSeamKind)
                             << "_seed" << it->seed
                             << "_tc" << it->tcIndex
                             << "_step" << cfg.sourceStep
                             << "_" << sourceLabel
                             << "_" << sanitizePathComponent(it->name) << ".txt";
            const std::string bundlePath = bundlePathStream.str();
            dumpExplicitCoreBuilderReplayBundle(bundle, bundlePath);

            const std::string summaryPath = cfg.dumpDir + "/summary.json";
            std::ofstream ofs(summaryPath);
            ofs << "{\n";
            ofs << "  \"mode\": \"explicit-core-builder-replay\",\n";
            ofs << "  \"manifestPath\": \"" << jsonEscape(cfg.manifestPath) << "\",\n";
            ofs << "  \"caseName\": \"" << jsonEscape(it->name) << "\",\n";
            ofs << "  \"outputDumpDir\": \"" << jsonEscape(cfg.dumpDir) << "\",\n";
            ofs << "  \"bundlePath\": \"" << jsonEscape(bundlePath) << "\",\n";
            ofs << "  \"topLevelOk\": " << (bundle.topLevelOk ? "true" : "false") << ",\n";
            ofs << "  \"sourceStep\": " << cfg.sourceStep << ",\n";
            ofs << "  \"sourceKind\": \"" << jsonEscape(cfg.sourceKind) << "\",\n";
            ofs << "  \"sourceSide\": \"" << jsonEscape(sourceLabel) << "\",\n";
            ofs << "  \"builderPipelineSeamKind\": \""
                << builderPipelineSeamKindName(bundle.builderPipelineSeamKind) << "\",\n";
            ofs << "  \"builderPipelineSeamWhy\": \""
                << jsonEscape(bundle.builderPipelineSeamWhy) << "\",\n";
            ofs << "  \"sourceWhy\": \"" << jsonEscape(bundle.sourceWhy) << "\",\n";
            ofs << "  \"builderWhy\": \"" << jsonEscape(bundle.builderWhy) << "\"\n";
            ofs << "}\n";

            std::cout << "[EXPLICIT_CORE_BUILDER_REPLAY] case=" << it->name
                      << " seamKind="
                      << builderPipelineSeamKindName(bundle.builderPipelineSeamKind)
                      << " sourceStep=" << cfg.sourceStep
                      << " sourceSide=" << cfg.sourceSide
                      << " bundle=" << bundlePath
                      << " summary=" << summaryPath << "\n";
            if (!why.empty()) {
                std::cout << "[EXPLICIT_CORE_BUILDER_REPLAY_WHY] " << why << "\n";
            }
            printRewriteStats();
            return ok ? 0 : 1;
        } else if (cfg.mode == "materialize-core-replay") {
            if (cfg.manualOnly) {
                throw std::runtime_error("materialize-core-replay does not support --manual-only");
            }
            if (cfg.manifestPath.empty()) {
                throw std::runtime_error("materialize-core-replay requires --manifest");
            }
            if (cfg.caseName.empty()) {
                throw std::runtime_error("materialize-core-replay requires --case-name");
            }
            if (cfg.sourceStep <= 0) {
                throw std::runtime_error("materialize-core-replay requires --source-step > 0");
            }
            if (cfg.sourceSide != "replay" &&
                cfg.sourceSide != "solver" &&
                cfg.sourceSide != "shadow") {
                throw std::runtime_error(
                    "materialize-core-replay requires --source-side replay|solver|shadow");
            }

            const auto cases = loadRegressionManifest(cfg.manifestPath);
            auto it = std::find_if(cases.begin(),
                                   cases.end(),
                                   [&](const RegressionCaseSpec &spec) {
                                       return spec.name == cfg.caseName;
                                   });
            if (it == cases.end()) {
                throw std::runtime_error(
                    "materialize-core-replay case not found in manifest: " + cfg.caseName);
            }
            if (!it->inputExplicit.has_value()) {
                throw std::runtime_error(
                    "materialize-core-replay case is missing inputExplicit snapshot: " +
                    cfg.caseName);
            }

            MaterializeCoreReplayBundle bundle;
            std::string why;
            const bool ok = runMaterializeCoreReplayCaseDumpAware(
                *it->inputExplicit,
                cfg.manifestPath,
                it->name,
                it->seed,
                it->tcIndex,
                it->targetStep >= 0 ? std::optional<int>(it->targetStep) : std::nullopt,
                cfg.sourceStep,
                cfg.sourceSide,
                bundle,
                why);

            std::ostringstream bundlePathStream;
            bundlePathStream << cfg.dumpDir << "/"
                             << coreMaterializeSubphaseSeamKindName(bundle.seamKind)
                             << "_seed" << it->seed
                             << "_tc" << it->tcIndex
                             << "_step" << cfg.sourceStep
                             << "_" << cfg.sourceSide
                             << "_" << sanitizePathComponent(it->name) << ".txt";
            const std::string bundlePath = bundlePathStream.str();
            dumpMaterializeCoreReplayBundle(bundle, bundlePath);

            const std::string summaryPath = cfg.dumpDir + "/summary.json";
            std::ofstream ofs(summaryPath);
            ofs << "{\n";
            ofs << "  \"mode\": \"materialize-core-replay\",\n";
            ofs << "  \"manifestPath\": \"" << jsonEscape(cfg.manifestPath) << "\",\n";
            ofs << "  \"caseName\": \"" << jsonEscape(it->name) << "\",\n";
            ofs << "  \"outputDumpDir\": \"" << jsonEscape(cfg.dumpDir) << "\",\n";
            ofs << "  \"bundlePath\": \"" << jsonEscape(bundlePath) << "\",\n";
            ofs << "  \"topLevelOk\": " << (bundle.topLevelOk ? "true" : "false") << ",\n";
            ofs << "  \"sourceStep\": " << cfg.sourceStep << ",\n";
            ofs << "  \"sourceSide\": \"" << jsonEscape(cfg.sourceSide) << "\",\n";
            ofs << "  \"seamKind\": \""
                << coreMaterializeSubphaseSeamKindName(bundle.seamKind) << "\",\n";
            ofs << "  \"seamWhy\": \"" << jsonEscape(bundle.seamWhy) << "\",\n";
            ofs << "  \"sourceWhy\": \"" << jsonEscape(bundle.sourceWhy) << "\",\n";
            ofs << "  \"builderWhy\": \"" << jsonEscape(bundle.builderWhy) << "\"\n";
            ofs << "}\n";

            std::cout << "[MATERIALIZE_CORE_REPLAY] case=" << it->name
                      << " seamKind=" << coreMaterializeSubphaseSeamKindName(bundle.seamKind)
                      << " sourceStep=" << cfg.sourceStep
                      << " sourceSide=" << cfg.sourceSide
                      << " bundle=" << bundlePath
                      << " summary=" << summaryPath << "\n";
            if (!why.empty()) {
                std::cout << "[MATERIALIZE_CORE_REPLAY_WHY] " << why << "\n";
            }
            printRewriteStats();
            return ok ? 0 : 1;
        } else if (cfg.mode == "ogdf-raw-crash-replay") {
            if (cfg.manualOnly) {
                throw std::runtime_error("ogdf-raw-crash-replay does not support --manual-only");
            }
            if (cfg.manifestPath.empty()) {
                throw std::runtime_error("ogdf-raw-crash-replay requires --manifest");
            }
            if (cfg.caseName.empty()) {
                throw std::runtime_error("ogdf-raw-crash-replay requires --case-name");
            }

            const auto cases = loadRegressionManifest(cfg.manifestPath);
            auto it = std::find_if(cases.begin(),
                                   cases.end(),
                                   [&](const RegressionCaseSpec &spec) {
                                       return spec.name == cfg.caseName;
                                   });
            if (it == cases.end()) {
                throw std::runtime_error(
                    "ogdf-raw-crash-replay case not found in manifest: " + cfg.caseName);
            }
            if (!it->inputExplicit.has_value()) {
                throw std::runtime_error(
                    "ogdf-raw-crash-replay case is missing inputExplicit snapshot: " +
                    cfg.caseName);
            }

            OgdfRawCrashReplayBundle bundle;
            std::string why;
            const bool ok = runOgdfRawCrashReplayCaseDumpAware(
                *it->inputExplicit,
                cfg.manifestPath,
                it->name,
                it->seed,
                it->tcIndex,
                it->targetStep >= 0 ? std::optional<int>(it->targetStep) : std::nullopt,
                cfg.source,
                cfg.stopBeforeOgdf,
                cfg.runChild,
                cfg.dumpDir,
                bundle,
                why);

            std::ostringstream bundlePathStream;
            bundlePathStream << cfg.dumpDir << "/ogdf_raw_"
                             << sanitizePathComponent(bundle.sourceSide.empty()
                                                          ? cfg.source
                                                          : bundle.sourceSide)
                             << "_" << sanitizePathComponent(bundle.callSiteTag.empty()
                                                                  ? "NO_CALLSITE"
                                                                  : bundle.callSiteTag)
                             << "_seed" << it->seed
                             << "_tc" << it->tcIndex;
            if (bundle.stepIndex >= 0) {
                bundlePathStream << "_step" << bundle.stepIndex;
            } else if (it->targetStep >= 0) {
                bundlePathStream << "_step" << it->targetStep;
            }
            bundlePathStream << "_" << sanitizePathComponent(it->name) << ".txt";
            const std::string bundlePath = bundlePathStream.str();
            dumpOgdfRawCrashReplayBundle(bundle, bundlePath);

            const std::string summaryPath = cfg.dumpDir + "/summary.json";
            std::ofstream ofs(summaryPath);
            ofs << "{\n";
            ofs << "  \"mode\": \"ogdf-raw-crash-replay\",\n";
            ofs << "  \"manifestPath\": \"" << jsonEscape(cfg.manifestPath) << "\",\n";
            ofs << "  \"caseName\": \"" << jsonEscape(it->name) << "\",\n";
            ofs << "  \"outputDumpDir\": \"" << jsonEscape(cfg.dumpDir) << "\",\n";
            ofs << "  \"bundlePath\": \"" << jsonEscape(bundlePath) << "\",\n";
            ofs << "  \"topLevelOk\": " << (bundle.topLevelOk ? "true" : "false") << ",\n";
            ofs << "  \"requestedSource\": \""
                << ogdfRawCrashReplaySourceKindName(bundle.requestedSource) << "\",\n";
            ofs << "  \"sourceSide\": \"" << jsonEscape(bundle.sourceSide) << "\",\n";
            ofs << "  \"callSiteTag\": \"" << jsonEscape(bundle.callSiteTag) << "\",\n";
            ofs << "  \"phaseTag\": \"" << jsonEscape(bundle.phaseTag) << "\",\n";
            ofs << "  \"dispatchKind\": \"" << jsonEscape(bundle.dispatchKind) << "\",\n";
            ofs << "  \"directRawAllowed\": "
                << (bundle.directRawAllowed ? "true" : "false") << ",\n";
            ofs << "  \"directRawBlockedReason\": \""
                << jsonEscape(bundle.directRawBlockedReason) << "\",\n";
            ofs << "  \"usedSharedDispatchPath\": "
                << (bundle.usedSharedDispatchPath ? "true" : "false") << ",\n";
            ofs << "  \"usedWholeCoreFallback\": "
                << (bundle.usedWholeCoreFallback ? "true" : "false") << ",\n";
            ofs << "  \"stepIndex\": " << bundle.stepIndex << ",\n";
            ofs << "  \"chosenR\": " << bundle.chosenR << ",\n";
            ofs << "  \"chosenX\": " << bundle.chosenX << ",\n";
            ofs << "  \"compactGraphDumpPath\": \""
                << jsonEscape(bundle.compactGraphDumpPath) << "\",\n";
            ofs << "  \"childExitCode\": " << bundle.childExitCode << ",\n";
            ofs << "  \"childSignal\": " << bundle.childSignal << ",\n";
            ofs << "  \"crashed\": " << (bundle.crashed ? "true" : "false") << ",\n";
            ofs << "  \"crashWhy\": \"" << jsonEscape(bundle.crashWhy) << "\",\n";
            const RewriteRStats stats = getRewriteRStats();
            ofs << "  \"compareDirectRawCallCount\": "
                << stats.compareDirectRawCallCount << ",\n";
            ofs << "  \"compareDirectRawBlockedCount\": "
                << stats.compareDirectRawBlockedCount << ",\n";
            ofs << "  \"compareSharedDispatchFallbackCount\": "
                << stats.compareSharedDispatchFallbackCount << ",\n";
            ofs << "  \"compareSharedDispatchCounts\": {\n";
            for (size_t i = 0; i < kCompactDispatchKindCount; ++i) {
                const auto kind = static_cast<CompactDispatchKind>(i);
                ofs << "    \"" << compactDispatchKindName(kind) << "\": "
                    << stats.compareSharedDispatchCounts[i]
                    << (i + 1 == kCompactDispatchKindCount ? "\n" : ",\n");
            }
            ofs << "  },\n";
            ofs << "  \"notes\": \"" << jsonEscape(bundle.notes) << "\"\n";
            ofs << "}\n";

            std::cout << "[OGDF_RAW_CRASH_REPLAY] case=" << it->name
                      << " requestedSource="
                      << ogdfRawCrashReplaySourceKindName(bundle.requestedSource)
                      << " sourceSide=" << bundle.sourceSide
                      << " callSiteTag=" << bundle.callSiteTag
                      << " bundle=" << bundlePath
                      << " summary=" << summaryPath << "\n";
            if (!why.empty()) {
                std::cout << "[OGDF_RAW_CRASH_REPLAY_WHY] " << why << "\n";
            }
            printRewriteStats();
            return ok ? 0 : 1;
        } else if (cfg.mode == "solver-compare") {
            std::vector<SolverCompareCaseResult> results;
            SolverCompareStats compareStats;
            std::vector<std::string> failedCaseNames;
            const bool useLegacy =
                cfg.baselineMode == "legacy" || cfg.baselineMode == "both";
            const bool useOracle =
                cfg.baselineMode == "oracle" || cfg.baselineMode == "both";
            if (!useLegacy && !useOracle) {
                throw std::runtime_error(
                    "solver-compare requires --baseline legacy|oracle|both");
            }

            auto recordCompareResult = [&](SolverCompareCaseResult result) {
                ++compareStats.compareCases;
                compareStats.totalLegacyMs += result.legacyElapsedMs;
                compareStats.totalOracleMs += result.oracleElapsedMs;
                compareStats.totalRewriteSeqMs += result.rewriteSeqElapsedMs;
                compareStats.maxLegacyMs =
                    std::max(compareStats.maxLegacyMs, result.legacyElapsedMs);
                compareStats.maxOracleMs =
                    std::max(compareStats.maxOracleMs, result.oracleElapsedMs);
                compareStats.maxRewriteSeqMs =
                    std::max(compareStats.maxRewriteSeqMs, result.rewriteSeqElapsedMs);
                if (useLegacy && !result.legacyOk) ++compareStats.legacyFailCount;
                if (useOracle && !result.oracleOk) ++compareStats.oracleFailCount;
                if (!result.rewriteSeqOk) ++compareStats.rewriteSeqFailCount;

                if (result.legacyVsRewriteEquivalent.has_value()) {
                    if (*result.legacyVsRewriteEquivalent) {
                        ++compareStats.legacyVsRewritePassed;
                    } else {
                        ++compareStats.legacyVsRewriteMismatchCount;
                        ++compareStats.explicitMismatchCount;
                    }
                }
                if (result.oracleVsRewriteEquivalent.has_value()) {
                    if (*result.oracleVsRewriteEquivalent) {
                        ++compareStats.oracleVsRewritePassed;
                    } else {
                        ++compareStats.oracleVsRewriteMismatchCount;
                        ++compareStats.explicitMismatchCount;
                    }
                }
                if (result.legacyVsOracleEquivalent.has_value()) {
                    if (*result.legacyVsOracleEquivalent) {
                        ++compareStats.legacyVsOraclePassed;
                    } else {
                        ++compareStats.legacyVsOracleMismatchCount;
                        ++compareStats.explicitMismatchCount;
                    }
                }

                if (result.legacyVsRewriteParentEquivalent.has_value() &&
                    !*result.legacyVsRewriteParentEquivalent) {
                    ++compareStats.parentMismatchCount;
                }
                if (result.oracleVsRewriteParentEquivalent.has_value() &&
                    !*result.oracleVsRewriteParentEquivalent) {
                    ++compareStats.parentMismatchCount;
                }
                if (result.legacyVsOracleParentEquivalent.has_value() &&
                    !*result.legacyVsOracleParentEquivalent) {
                    ++compareStats.parentMismatchCount;
                }

                if (result.passed) {
                    ++compareStats.comparePassed;
                } else {
                    ++compareStats.compareFailed;
                    failedCaseNames.push_back(result.name);
                    std::cerr << "[COMPARE_FAIL] case=" << result.name;
                    if (!result.dumpPath.empty()) {
                        std::cerr << " bundle=" << result.dumpPath;
                    }
                    std::cerr << "\n";
                    for (const auto &failure : result.failures) {
                        std::cerr << "  - " << failure << "\n";
                    }
                }
                results.push_back(std::move(result));
            };

            if (cfg.manifestPath.empty()) {
                if (cfg.manualOnly) {
                    int manualIndex = 0;
                    for (const auto &G : buildManualRewriteCases()) {
                        SolverCompareCaseResult result;
                        runSolverCompareCase("manual_" + std::to_string(manualIndex),
                                             cfg.seed,
                                             manualIndex,
                                             std::nullopt,
                                             G,
                                             result);
                        recordCompareResult(std::move(result));
                        ++manualIndex;
                    }
                } else {
                    for (int i = 0; i < cfg.rounds; ++i) {
                        const ExplicitBlockGraph G = makeRandomRewriteCase(cfg.seed, i);
                        SolverCompareCaseResult result;
                        runSolverCompareCase("seed" + std::to_string(cfg.seed) +
                                                 "_tc" + std::to_string(i),
                                             cfg.seed,
                                             i,
                                             std::nullopt,
                                             G,
                                             result);
                        recordCompareResult(std::move(result));
                    }
                }
            } else {
                if (cfg.manualOnly) {
                    throw std::runtime_error("solver-compare does not support --manual-only together with --manifest");
                }
                const auto cases = loadRegressionManifest(cfg.manifestPath);
                if (cases.empty()) {
                    throw std::runtime_error("solver-compare manifest contains no cases");
                }
                for (const auto &spec : cases) {
                    const ExplicitBlockGraph G = spec.inputExplicit.has_value()
                                                    ? *spec.inputExplicit
                                                    : makeRandomRewriteCase(spec.seed, spec.tcIndex);
                    SolverCompareCaseResult result;
                    runSolverCompareCase(spec.name,
                                         spec.seed,
                                         spec.tcIndex,
                                         spec.targetStep,
                                         G,
                                         result);
                    recordCompareResult(std::move(result));
                }
            }

            if (compareStats.compareCases != 0) {
                compareStats.averageLegacyMs =
                    compareStats.totalLegacyMs / static_cast<double>(compareStats.compareCases);
                compareStats.averageOracleMs =
                    compareStats.totalOracleMs / static_cast<double>(compareStats.compareCases);
                compareStats.averageRewriteSeqMs =
                    compareStats.totalRewriteSeqMs / static_cast<double>(compareStats.compareCases);
            }

            const std::string summaryPath = cfg.dumpDir + "/summary.json";
            std::ofstream ofs(summaryPath);
            ofs << "{\n";
            ofs << "  \"mode\": \"solver-compare\",\n";
            ofs << "  \"baselineMode\": \"" << jsonEscape(cfg.baselineMode) << "\",\n";
            ofs << "  \"oracleHandoffPolicy\": \"" << jsonEscape(cfg.oracleHandoff)
                << "\",\n";
            if (!cfg.manifestPath.empty()) {
                ofs << "  \"manifestPath\": \"" << jsonEscape(cfg.manifestPath) << "\",\n";
            }
            ofs << "  \"outputDumpDir\": \"" << jsonEscape(cfg.dumpDir) << "\",\n";
            ofs << "  \"compareCases\": " << compareStats.compareCases << ",\n";
            ofs << "  \"comparePassed\": " << compareStats.comparePassed << ",\n";
            ofs << "  \"compareFailed\": " << compareStats.compareFailed << ",\n";
            ofs << "  \"legacyVsRewritePassed\": "
                << compareStats.legacyVsRewritePassed << ",\n";
            ofs << "  \"oracleVsRewritePassed\": "
                << compareStats.oracleVsRewritePassed << ",\n";
            ofs << "  \"legacyVsOraclePassed\": "
                << compareStats.legacyVsOraclePassed << ",\n";
            ofs << "  \"legacyFailCount\": " << compareStats.legacyFailCount << ",\n";
            ofs << "  \"oracleFailCount\": " << compareStats.oracleFailCount << ",\n";
            ofs << "  \"rewriteSeqFailCount\": " << compareStats.rewriteSeqFailCount << ",\n";
            ofs << "  \"legacyVsRewriteMismatchCount\": "
                << compareStats.legacyVsRewriteMismatchCount << ",\n";
            ofs << "  \"oracleVsRewriteMismatchCount\": "
                << compareStats.oracleVsRewriteMismatchCount << ",\n";
            ofs << "  \"legacyVsOracleMismatchCount\": "
                << compareStats.legacyVsOracleMismatchCount << ",\n";
            ofs << "  \"explicitMismatchCount\": " << compareStats.explicitMismatchCount << ",\n";
            ofs << "  \"parentMismatchCount\": " << compareStats.parentMismatchCount << ",\n";
            ofs << "  \"averageLegacyMs\": " << compareStats.averageLegacyMs << ",\n";
            ofs << "  \"averageOracleMs\": " << compareStats.averageOracleMs << ",\n";
            ofs << "  \"averageRewriteSeqMs\": " << compareStats.averageRewriteSeqMs << ",\n";
            ofs << "  \"maxLegacyMs\": " << compareStats.maxLegacyMs << ",\n";
            ofs << "  \"maxOracleMs\": " << compareStats.maxOracleMs << ",\n";
            ofs << "  \"maxRewriteSeqMs\": " << compareStats.maxRewriteSeqMs << ",\n";
            ofs << "  \"failedCaseNames\": [";
            for (size_t i = 0; i < failedCaseNames.size(); ++i) {
                if (i != 0) ofs << ", ";
                ofs << "\"" << jsonEscape(failedCaseNames[i]) << "\"";
            }
            ofs << "],\n";
            ofs << "  \"cases\": [\n";
            for (size_t i = 0; i < results.size(); ++i) {
                const auto &result = results[i];
                ofs << "    {\n";
                ofs << "      \"name\": \"" << jsonEscape(result.name) << "\",\n";
                ofs << "      \"seed\": " << result.seed << ",\n";
                ofs << "      \"tcIndex\": " << result.tcIndex << ",\n";
                if (result.targetStep.has_value()) {
                    ofs << "      \"targetStep\": " << *result.targetStep << ",\n";
                }
                ofs << "      \"passed\": " << (result.passed ? "true" : "false") << ",\n";
                if (useLegacy) {
                    ofs << "      \"legacyOk\": " << (result.legacyOk ? "true" : "false") << ",\n";
                }
                if (useOracle) {
                    ofs << "      \"oracleOk\": " << (result.oracleOk ? "true" : "false") << ",\n";
                }
                ofs << "      \"rewriteSeqOk\": " << (result.rewriteSeqOk ? "true" : "false") << ",\n";
                if (result.legacyVsRewriteEquivalent.has_value()) {
                    ofs << "      \"legacyVsRewriteEquivalent\": "
                        << (*result.legacyVsRewriteEquivalent ? "true" : "false") << ",\n";
                }
                if (result.legacyVsRewriteRawExplicitEquivalent.has_value()) {
                    ofs << "      \"legacyVsRewriteRawExplicitEquivalent\": "
                        << (*result.legacyVsRewriteRawExplicitEquivalent ? "true" : "false")
                        << ",\n";
                }
                if (result.legacyVsRewriteCanonicalExplicitEquivalent.has_value()) {
                    ofs << "      \"legacyVsRewriteCanonicalExplicitEquivalent\": "
                        << (*result.legacyVsRewriteCanonicalExplicitEquivalent ? "true" : "false")
                        << ",\n";
                }
                if (result.oracleVsRewriteEquivalent.has_value()) {
                    ofs << "      \"oracleVsRewriteEquivalent\": "
                        << (*result.oracleVsRewriteEquivalent ? "true" : "false") << ",\n";
                }
                if (result.oracleVsRewriteRawExplicitEquivalent.has_value()) {
                    ofs << "      \"oracleVsRewriteRawExplicitEquivalent\": "
                        << (*result.oracleVsRewriteRawExplicitEquivalent ? "true" : "false")
                        << ",\n";
                }
                if (result.oracleVsRewriteCanonicalExplicitEquivalent.has_value()) {
                    ofs << "      \"oracleVsRewriteCanonicalExplicitEquivalent\": "
                        << (*result.oracleVsRewriteCanonicalExplicitEquivalent ? "true" : "false")
                        << ",\n";
                }
                if (result.legacyVsOracleEquivalent.has_value()) {
                    ofs << "      \"legacyVsOracleEquivalent\": "
                        << (*result.legacyVsOracleEquivalent ? "true" : "false") << ",\n";
                }
                if (result.legacyVsOracleRawExplicitEquivalent.has_value()) {
                    ofs << "      \"legacyVsOracleRawExplicitEquivalent\": "
                        << (*result.legacyVsOracleRawExplicitEquivalent ? "true" : "false")
                        << ",\n";
                }
                if (result.legacyVsOracleCanonicalExplicitEquivalent.has_value()) {
                    ofs << "      \"legacyVsOracleCanonicalExplicitEquivalent\": "
                        << (*result.legacyVsOracleCanonicalExplicitEquivalent ? "true" : "false")
                        << ",\n";
                }
                if (useLegacy) {
                    ofs << "      \"legacyElapsedMs\": " << result.legacyElapsedMs << ",\n";
                }
                if (useOracle) {
                    ofs << "      \"oracleElapsedMs\": " << result.oracleElapsedMs << ",\n";
                }
                ofs << "      \"rewriteSeqElapsedMs\": " << result.rewriteSeqElapsedMs << ",\n";
                ofs << "      \"dumpPath\": \"" << jsonEscape(result.dumpPath) << "\",\n";
                ofs << "      \"failures\": [";
                for (size_t j = 0; j < result.failures.size(); ++j) {
                    if (j != 0) ofs << ", ";
                    ofs << "\"" << jsonEscape(result.failures[j]) << "\"";
                }
                ofs << "]\n";
                ofs << "    }" << (i + 1 == results.size() ? "\n" : ",\n");
            }
            ofs << "  ]\n";
            ofs << "}\n";

            std::cout << "[SOLVER_COMPARE] summary=" << summaryPath
                      << " compareCases=" << compareStats.compareCases
                      << " comparePassed=" << compareStats.comparePassed
                      << " compareFailed=" << compareStats.compareFailed
                      << " explicitMismatchCount=" << compareStats.explicitMismatchCount
                      << " parentMismatchCount=" << compareStats.parentMismatchCount
                      << "\n";
            if (!failedCaseNames.empty()) {
                printRewriteStats();
                return 1;
            }
            std::cout << "[OK] completed tc=" << compareStats.compareCases << "\n";
            printRewriteStats();
            return 0;
        } else if (cfg.mode == "rewrite-r-seq-bench") {
            if (cfg.manualOnly) {
                throw std::runtime_error("rewrite-r-seq-bench does not support --manual-only");
            }

            std::vector<double> caseElapsedMs;
            caseElapsedMs.reserve(static_cast<size_t>(std::max(cfg.rounds, 0)));
            const auto benchStart = Clock::now();
            for (int i = 0; i < cfg.rounds; ++i) {
                const auto caseStart = Clock::now();
                if (!runRewrite(makeRandomRewriteCase(cfg.seed, i))) {
                    printRewriteStats();
                    return 1;
                }
                caseElapsedMs.push_back(
                    std::chrono::duration<double, std::milli>(Clock::now() - caseStart)
                        .count());
            }

            const RewriteRStats stats = getRewriteRStats();
            const double totalElapsedMs =
                std::chrono::duration<double, std::milli>(Clock::now() - benchStart)
                    .count();
            const double avgCaseMs =
                caseElapsedMs.empty() ? 0.0 : totalElapsedMs / caseElapsedMs.size();
            const double avgRewriteMs =
                stats.rewriteCalls == 0
                    ? 0.0
                    : totalElapsedMs / static_cast<double>(stats.rewriteCalls);
            double maxCaseMs = 0.0;
            for (const double value : caseElapsedMs) {
                if (value > maxCaseMs) maxCaseMs = value;
            }
            size_t maxSequenceLength = 0;
            for (size_t i = 0; i < kRewriteSeqLengthHistogramSize; ++i) {
                if (stats.sequenceLengthHistogram[i] != 0) {
                    maxSequenceLength = i;
                }
            }

            const std::string summaryPath = cfg.dumpDir + "/summary.json";
            std::ofstream ofs(summaryPath);
            ofs << "{\n";
            ofs << "  \"seed\": " << cfg.seed << ",\n";
            ofs << "  \"rounds\": " << cfg.rounds << ",\n";
            ofs << "  \"outputDumpDir\": \"" << jsonEscape(cfg.dumpDir) << "\",\n";
            ofs << "  \"totalCases\": " << caseElapsedMs.size() << ",\n";
            ofs << "  \"totalRewriteCalls\": " << stats.rewriteCalls << ",\n";
            ofs << "  \"rewriteSeqCalls\": " << stats.rewriteSeqCalls << ",\n";
            ofs << "  \"totalElapsedMs\": " << totalElapsedMs << ",\n";
            ofs << "  \"avgCaseMs\": " << avgCaseMs << ",\n";
            ofs << "  \"avgRewriteMs\": " << avgRewriteMs << ",\n";
            ofs << "  \"maxCaseMs\": " << maxCaseMs << ",\n";
            ofs << "  \"maxSequenceLength\": " << maxSequenceLength << ",\n";
            ofs << "  \"seqFallbackCaseCount\": " << stats.seqFallbackCaseCount << ",\n";
            ofs << "  \"seqRewriteWholeCoreFallbackCount\": "
                << stats.seqRewriteWholeCoreFallbackCount << ",\n";
            ofs << "  \"rewriteFallbackSpecialCaseCount\": "
                << stats.rewriteFallbackSpecialCaseCount << ",\n";
            ofs << "  \"rewritePathTakenCounts\": {\n";
            for (size_t i = 0; i < kRewritePathTakenCount; ++i) {
                const auto path = static_cast<RewritePathTaken>(i);
                ofs << "    \"" << rewritePathTakenName(path) << "\": "
                    << stats.rewritePathTakenCounts[i]
                    << (i + 1 == kRewritePathTakenCount ? "\n" : ",\n");
            }
            ofs << "  },\n";
            ofs << "  \"rewriteFallbackTriggerCounts\": {\n";
            for (size_t i = 0; i < kRewriteFallbackTriggerCount; ++i) {
                const auto trigger = static_cast<RewriteFallbackTrigger>(i);
                ofs << "    \"" << rewriteFallbackTriggerName(trigger) << "\": "
                    << stats.rewriteFallbackTriggerCounts[i]
                    << (i + 1 == kRewriteFallbackTriggerCount ? "\n" : ",\n");
            }
            ofs << "  },\n";
            ofs << "  \"seqCompactBuildFailSubtypeCounts\": {\n";
            for (size_t i = 0; i < kCompactBuildFailSubtypeCount; ++i) {
                const auto subtype = static_cast<CompactBuildFailSubtype>(i);
                ofs << "    \"" << compactBuildFailSubtypeName(subtype) << "\": "
                    << stats.seqCompactBuildFailSubtypeCounts[i]
                    << (i + 1 == kCompactBuildFailSubtypeCount ? "\n" : ",\n");
            }
            ofs << "  },\n";
            ofs << "  \"seqSelfLoopSubtypeCounts\": {\n";
            for (size_t i = 0; i < kSelfLoopBuildFailSubtypeCount; ++i) {
                const auto subtype = static_cast<SelfLoopBuildFailSubtype>(i);
                ofs << "    \"" << selfLoopBuildFailSubtypeName(subtype) << "\": "
                    << stats.seqSelfLoopSubtypeCounts[i]
                    << (i + 1 == kSelfLoopBuildFailSubtypeCount ? "\n" : ",\n");
            }
            ofs << "  },\n";
            ofs << "  \"seqTooSmallSubtypeCounts\": {\n";
            for (size_t i = 0; i < kTooSmallSubtypeCount; ++i) {
                const auto subtype = static_cast<TooSmallSubtype>(i);
                ofs << "    \"" << tooSmallSubtypeName(subtype) << "\": "
                    << stats.seqTooSmallSubtypeCounts[i]
                    << (i + 1 == kTooSmallSubtypeCount ? "\n" : ",\n");
            }
            ofs << "  },\n";
            ofs << "  \"seqTooSmallOtherSubtypeCounts\": {\n";
            for (size_t i = 0; i < kTooSmallOtherSubtypeCount; ++i) {
                const auto subtype = static_cast<TooSmallOtherSubtype>(i);
                ofs << "    \"" << tooSmallOtherSubtypeName(subtype) << "\": "
                    << stats.seqTooSmallOtherSubtypeCounts[i]
                    << (i + 1 == kTooSmallOtherSubtypeCount ? "\n" : ",\n");
            }
            ofs << "  },\n";
            ofs << "  \"seqTooSmallOneEdgeSubtypeCounts\": {\n";
            for (size_t i = 0; i < kSequenceOneEdgeSubtypeCount; ++i) {
                const auto subtype = static_cast<SequenceOneEdgeSubtype>(i);
                ofs << "    \"" << sequenceOneEdgeSubtypeName(subtype) << "\": "
                    << stats.seqTooSmallOneEdgeSubtypeCounts[i]
                    << (i + 1 == kSequenceOneEdgeSubtypeCount ? "\n" : ",\n");
            }
            ofs << "  },\n";
            ofs << "  \"seqXIncidentVirtualSubtypeCounts\": {\n";
            for (size_t i = 0; i < kXIncidentVirtualSubtypeCount; ++i) {
                const auto subtype = static_cast<XIncidentVirtualSubtype>(i);
                ofs << "    \"" << xIncidentVirtualSubtypeName(subtype) << "\": "
                    << stats.seqXIncidentVirtualSubtypeCounts[i]
                    << (i + 1 == kXIncidentVirtualSubtypeCount ? "\n" : ",\n");
            }
            ofs << "  },\n";
            ofs << "  \"seqXIncidentResidualSubtypeCounts\": {\n";
            for (size_t i = 0; i < kXSharedResidualSubtypeCount; ++i) {
                const auto subtype = static_cast<XSharedResidualSubtype>(i);
                ofs << "    \"" << xSharedResidualSubtypeName(subtype) << "\": "
                    << stats.seqXIncidentResidualSubtypeCounts[i]
                    << (i + 1 == kXSharedResidualSubtypeCount ? "\n" : ",\n");
            }
            ofs << "  }\n";
            ofs << "}\n";

            std::cout << "[BENCH] summary=" << summaryPath
                      << " totalCases=" << caseElapsedMs.size()
                      << " totalElapsedMs=" << totalElapsedMs
                      << " avgCaseMs=" << avgCaseMs
                      << " avgRewriteMs=" << avgRewriteMs
                      << " maxCaseMs=" << maxCaseMs
                      << " maxSequenceLength=" << maxSequenceLength << "\n";
            std::cout << "[OK] completed tc=" << tc << "\n";
            printRewriteStats();
            return 0;
        } else if (cfg.mode == "rewrite-r-seq-replay") {
            if (cfg.manualOnly) {
                throw std::runtime_error("rewrite-r-seq-replay does not support --manual-only");
            }
            if (cfg.tcIndex < 0) {
                throw std::runtime_error("rewrite-r-seq-replay requires --tc-index");
            }
            if (cfg.targetStep < 0) {
                throw std::runtime_error("rewrite-r-seq-replay requires --target-step");
            }
            tc = cfg.tcIndex;
            if (!runRewrite(makeRandomRewriteCase(cfg.seed, cfg.tcIndex))) {
                printRewriteStats();
                return 1;
            }
        } else if (cfg.mode == "rewrite-r" ||
                   cfg.mode == "rewrite-r-seq" ||
                   cfg.mode == "rewrite-seq") {
            if (cfg.manualOnly) {
                for (const auto &G : buildManualRewriteCases()) {
                    if (!runRewrite(G)) {
                        printRewriteStats();
                        return 1;
                    }
                }
            } else {
                for (int i = 0; i < cfg.rounds; ++i) {
                    if (!runRewrite(makeRandomRewriteCase(cfg.seed, i))) {
                        printRewriteStats();
                        return 1;
                    }
                }
            }
        } else {
            if (cfg.manualOnly) {
                for (const auto &H : buildManualCases()) {
                    if (!runCompact(H)) return 1;
                }
            } else {
                for (int i = 0; i < cfg.rounds; ++i) {
                    if (!runCompact(makeRandomCompactGraph(cfg.seed, i))) return 1;
                }
            }
        }

        std::cout << "[OK] completed tc=" << tc << "\n";
        printRewriteStats();
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "[FATAL] " << e.what() << "\n";
        return 2;
    }
}
