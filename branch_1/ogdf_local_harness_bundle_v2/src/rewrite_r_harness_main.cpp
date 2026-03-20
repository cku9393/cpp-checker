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
        else if (a == "--manual-only") cfg.manualOnly = true;
        else if (a == "--mode") cfg.mode = next("--mode");
        else if (a == "--dump-dir") cfg.dumpDir = next("--dump-dir");
        else if (a == "--backend") { (void)next("--backend"); }
        else if (a == "--help") {
            std::cout << "--backend ogdf --mode {static|dummy|rewrite-r|rewrite-r-seq|rewrite-seq|rewrite-r-seq-replay|rewrite-r-seq-regression|rewrite-r-seq-bench} "
                         "--seed N --rounds N --tc-index N --target-step N --manifest PATH --manual-only --dump-dir DIR\n";
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
        out.push_back(std::move(spec));
    }
    return out;
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
            cfg.mode == "rewrite-r-seq-bench") {
            resetRewriteRStats();
        }

        int tc = 0;
        auto printRewriteStats = [&]() {
            if (cfg.mode != "rewrite-r" &&
                cfg.mode != "rewrite-r-seq" &&
                cfg.mode != "rewrite-seq" &&
                cfg.mode != "rewrite-r-seq-replay" &&
                cfg.mode != "rewrite-r-seq-regression" &&
                cfg.mode != "rewrite-r-seq-bench") {
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
                const ExplicitBlockGraph G =
                    makeRandomRewriteCase(spec.seed, spec.tcIndex);
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
