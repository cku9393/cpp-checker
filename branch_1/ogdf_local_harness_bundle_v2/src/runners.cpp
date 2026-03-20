#include "harness/runners.hpp"
#include "harness/project_static_adapter.hpp"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace harness {

static CompactGraph makeTriangle(int offset) {
    CompactGraph H;
    H.block = offset;
    H.origOfCv = {offset + 1, offset + 2, offset + 3};
    for (int i = 0; i < 3; ++i) H.cvOfOrig[H.origOfCv[i]] = i;
    H.touchedVertices = H.origOfCv;
    H.edges = {
        {0, CompactEdgeKind::REAL, 0, 1, offset * 10 + 1},
        {1, CompactEdgeKind::REAL, 1, 2, offset * 10 + 2},
        {2, CompactEdgeKind::REAL, 2, 0, offset * 10 + 3},
    };
    return H;
}

static ExplicitBlockGraph makeExplicitGraph(std::vector<VertexId> vertices,
                                            std::vector<ExplicitEdge> edges) {
    std::sort(vertices.begin(), vertices.end());
    vertices.erase(std::unique(vertices.begin(), vertices.end()), vertices.end());

    for (auto &edge : edges) {
        if (edge.u > edge.v) std::swap(edge.u, edge.v);
    }
    std::sort(edges.begin(), edges.end(), [](const ExplicitEdge &lhs, const ExplicitEdge &rhs) {
        return std::tie(lhs.id, lhs.u, lhs.v) < std::tie(rhs.id, rhs.u, rhs.v);
    });

    ExplicitBlockGraph G;
    G.vertices = std::move(vertices);
    G.edges = std::move(edges);
    return G;
}

static bool buildWholeCoreForTestingLocal(const ExplicitBlockGraph &G,
                                          IRawSpqrBackend &,
                                          ReducedSPQRCore &core,
                                          std::string &why) {
    core = {};
    core.blockId = 0;
    core.root = 0;
    core.nodes.resize(1);

    auto &root = core.nodes[0];
    root.alive = true;
    root.type = SPQRType::R_NODE;

    std::unordered_set<VertexId> vertexSet(G.vertices.begin(), G.vertices.end());
    std::unordered_set<VertexId> usedVertices;
    std::unordered_set<EdgeId> seenEdgeIds;

    for (const auto &edge : G.edges) {
        if (edge.id < 0) {
            why = "buildWholeCoreForTesting: explicit edge id must be non-negative";
            return false;
        }
        if (edge.u < 0 || edge.v < 0) {
            why = "buildWholeCoreForTesting: explicit edge endpoint must be non-negative";
            return false;
        }
        if (!seenEdgeIds.insert(edge.id).second) {
            why = "buildWholeCoreForTesting: duplicate explicit edge id";
            return false;
        }
        if (!vertexSet.empty() &&
            (vertexSet.count(edge.u) == 0 || vertexSet.count(edge.v) == 0)) {
            why = "buildWholeCoreForTesting: explicit edge endpoint missing from vertex set";
            return false;
        }

        const auto [u, v] = canonPole(edge.u, edge.v);
        usedVertices.insert(u);
        usedVertices.insert(v);
        const int slotId = static_cast<int>(root.slots.size());
        root.slots.push_back({true, u, v, false, edge.id, -1});
        root.realEdgesHere.push_back(edge.id);
        core.ownerNodeOfRealEdge[edge.id] = 0;
        core.ownerSlotOfRealEdge[edge.id] = slotId;
        core.occ[u].push_back({0, slotId});
        if (u != v) core.occ[v].push_back({0, slotId});
    }

    std::sort(root.realEdgesHere.begin(), root.realEdgesHere.end());
    root.localAgg.edgeCnt = static_cast<int>(G.edges.size());
    root.localAgg.vertexCnt = static_cast<int>(usedVertices.size());
    root.localAgg.incCnt = static_cast<int>(G.edges.size()) * 2;
    if (!G.edges.empty()) root.localAgg.repEdge = G.edges.front().id;
    if (!usedVertices.empty()) root.localAgg.repVertex = *std::min_element(usedVertices.begin(), usedVertices.end());
    root.subAgg = root.localAgg;
    core.totalAgg = root.localAgg;
    why.clear();
    return true;
}

static bool chooseDeterministicRewriteTarget(const ReducedSPQRCore &core,
                                             NodeId &chosenR,
                                             VertexId &chosenX,
                                             std::string &why) {
    chosenR = -1;
    chosenX = -1;

    for (NodeId nodeId = 0; nodeId < static_cast<NodeId>(core.nodes.size()); ++nodeId) {
        const auto &node = core.nodes[nodeId];
        if (!node.alive || node.type != SPQRType::R_NODE) continue;
        chosenR = nodeId;
        break;
    }
    if (chosenR < 0) {
        why = "chooseDeterministicRewriteTarget: no alive R-node";
        return false;
    }

    std::unordered_map<VertexId, int> countByVertex;
    const auto &node = core.nodes[chosenR];
    for (const auto &slot : node.slots) {
        if (!slot.alive) continue;
        ++countByVertex[slot.poleA];
        if (slot.poleB != slot.poleA) ++countByVertex[slot.poleB];
    }

    for (const auto &[vertex, count] : countByVertex) {
        if (count < 2) continue;
        if (chosenX < 0 || vertex < chosenX) chosenX = vertex;
    }
    if (chosenX < 0) {
        why = "chooseDeterministicRewriteTarget: no vertex with occurrence count >= 2 on chosen R-node";
        return false;
    }
    why.clear();
    return true;
}

enum class LocalSequenceChooseStatus : uint8_t {
    FOUND,
    NONE,
    ERROR
};

static LocalSequenceChooseStatus chooseDeterministicSequenceRewriteTargetLocal(
    const ReducedSPQRCore &core,
    NodeId &chosenR,
    VertexId &chosenX,
    std::string &why) {
    chosenR = -1;
    chosenX = -1;
    why.clear();

    for (NodeId nodeId = 0; nodeId < static_cast<NodeId>(core.nodes.size()); ++nodeId) {
        const auto &node = core.nodes[nodeId];
        if (!node.alive || node.type != SPQRType::R_NODE) continue;

        std::unordered_map<VertexId, int> realCountByVertex;
        for (const auto &slot : node.slots) {
            if (!slot.alive || slot.isVirtual) continue;
            if (slot.poleA < 0 || slot.poleB < 0) {
                why = "chooseDeterministicSequenceRewriteTarget: invalid REAL slot pole";
                return LocalSequenceChooseStatus::ERROR;
            }
            ++realCountByVertex[slot.poleA];
            if (slot.poleB != slot.poleA) ++realCountByVertex[slot.poleB];
        }

        VertexId bestX = -1;
        int bestCount = -1;
        for (const auto &[vertex, count] : realCountByVertex) {
            if (count < 2) continue;
            if (count > bestCount || (count == bestCount && (bestX < 0 || vertex < bestX))) {
                bestX = vertex;
                bestCount = count;
            }
        }

        if (bestX >= 0) {
            chosenR = nodeId;
            chosenX = bestX;
            return LocalSequenceChooseStatus::FOUND;
        }
    }

    return LocalSequenceChooseStatus::NONE;
}

static void captureRewriteAfter(HarnessBundle &B,
                                IHarnessOps &ops,
                                const ReducedSPQRCore &core) {
    B.actualAfterRewrite = core;
    B.explicitAfter = ops.materializeWholeCoreExplicit(core);
}

static HarnessResult succeedAndDump(HarnessBundle &B,
                                    const std::string &dumpDir) {
    HarnessResult R;
    R.ok = true;
    R.where = B.where;
    R.why = B.why;
    R.dumpPath = makeBundlePath(dumpDir, B);
    R.bundle = B;
    dumpHarnessBundle(B, R.dumpPath);
    return R;
}

using JsonObject = std::map<std::string, struct JsonValue>;
using JsonArray = std::vector<struct JsonValue>;

struct JsonValue {
    using Storage = std::variant<std::nullptr_t, bool, double, std::string, JsonArray, JsonObject>;
    Storage value;

    bool isNull() const { return std::holds_alternative<std::nullptr_t>(value); }
    bool isBool() const { return std::holds_alternative<bool>(value); }
    bool isNumber() const { return std::holds_alternative<double>(value); }
    bool isString() const { return std::holds_alternative<std::string>(value); }
    bool isArray() const { return std::holds_alternative<JsonArray>(value); }
    bool isObject() const { return std::holds_alternative<JsonObject>(value); }
};

class JsonParser {
public:
    explicit JsonParser(const std::string &text) : m_text(text) {}

    bool parse(JsonValue &out, std::string &why) {
        skipWs();
        if (!parseValue(out, why)) return false;
        skipWs();
        if (m_pos != m_text.size()) {
            why = "json parse: trailing characters";
            return false;
        }
        return true;
    }

private:
    const std::string &m_text;
    size_t m_pos = 0;

    void skipWs() {
        while (m_pos < m_text.size() &&
               std::isspace(static_cast<unsigned char>(m_text[m_pos])) != 0) {
            ++m_pos;
        }
    }

    bool parseValue(JsonValue &out, std::string &why) {
        skipWs();
        if (m_pos >= m_text.size()) {
            why = "json parse: unexpected end of input";
            return false;
        }
        const char ch = m_text[m_pos];
        if (ch == '{') return parseObject(out, why);
        if (ch == '[') return parseArray(out, why);
        if (ch == '"') return parseString(out, why);
        if (ch == 't' || ch == 'f') return parseBool(out, why);
        if (ch == 'n') return parseNull(out, why);
        if (ch == '-' || (ch >= '0' && ch <= '9')) return parseNumber(out, why);
        why = "json parse: unexpected token";
        return false;
    }

    bool parseObject(JsonValue &out, std::string &why) {
        JsonObject obj;
        ++m_pos; // {
        skipWs();
        if (m_pos < m_text.size() && m_text[m_pos] == '}') {
            ++m_pos;
            out.value = std::move(obj);
            return true;
        }
        while (m_pos < m_text.size()) {
            JsonValue keyValue;
            if (!parseString(keyValue, why)) return false;
            const std::string &key = std::get<std::string>(keyValue.value);
            skipWs();
            if (m_pos >= m_text.size() || m_text[m_pos] != ':') {
                why = "json parse: expected ':'";
                return false;
            }
            ++m_pos;
            JsonValue value;
            if (!parseValue(value, why)) return false;
            obj[key] = std::move(value);
            skipWs();
            if (m_pos >= m_text.size()) {
                why = "json parse: unexpected end in object";
                return false;
            }
            if (m_text[m_pos] == '}') {
                ++m_pos;
                out.value = std::move(obj);
                return true;
            }
            if (m_text[m_pos] != ',') {
                why = "json parse: expected ',' in object";
                return false;
            }
            ++m_pos;
            skipWs();
        }
        why = "json parse: unterminated object";
        return false;
    }

    bool parseArray(JsonValue &out, std::string &why) {
        JsonArray arr;
        ++m_pos; // [
        skipWs();
        if (m_pos < m_text.size() && m_text[m_pos] == ']') {
            ++m_pos;
            out.value = std::move(arr);
            return true;
        }
        while (m_pos < m_text.size()) {
            JsonValue value;
            if (!parseValue(value, why)) return false;
            arr.push_back(std::move(value));
            skipWs();
            if (m_pos >= m_text.size()) {
                why = "json parse: unexpected end in array";
                return false;
            }
            if (m_text[m_pos] == ']') {
                ++m_pos;
                out.value = std::move(arr);
                return true;
            }
            if (m_text[m_pos] != ',') {
                why = "json parse: expected ',' in array";
                return false;
            }
            ++m_pos;
            skipWs();
        }
        why = "json parse: unterminated array";
        return false;
    }

    bool parseString(JsonValue &out, std::string &why) {
        if (m_pos >= m_text.size() || m_text[m_pos] != '"') {
            why = "json parse: expected string";
            return false;
        }
        ++m_pos;
        std::string s;
        while (m_pos < m_text.size()) {
            const char ch = m_text[m_pos++];
            if (ch == '"') {
                out.value = std::move(s);
                return true;
            }
            if (ch == '\\') {
                if (m_pos >= m_text.size()) {
                    why = "json parse: bad escape";
                    return false;
                }
                const char esc = m_text[m_pos++];
                switch (esc) {
                    case '"': s.push_back('"'); break;
                    case '\\': s.push_back('\\'); break;
                    case '/': s.push_back('/'); break;
                    case 'b': s.push_back('\b'); break;
                    case 'f': s.push_back('\f'); break;
                    case 'n': s.push_back('\n'); break;
                    case 'r': s.push_back('\r'); break;
                    case 't': s.push_back('\t'); break;
                    default:
                        why = "json parse: unsupported escape";
                        return false;
                }
                continue;
            }
            s.push_back(ch);
        }
        why = "json parse: unterminated string";
        return false;
    }

    bool parseBool(JsonValue &out, std::string &why) {
        if (m_text.compare(m_pos, 4, "true") == 0) {
            m_pos += 4;
            out.value = true;
            return true;
        }
        if (m_text.compare(m_pos, 5, "false") == 0) {
            m_pos += 5;
            out.value = false;
            return true;
        }
        why = "json parse: invalid boolean";
        return false;
    }

    bool parseNull(JsonValue &out, std::string &why) {
        if (m_text.compare(m_pos, 4, "null") != 0) {
            why = "json parse: invalid null";
            return false;
        }
        m_pos += 4;
        out.value = nullptr;
        return true;
    }

    bool parseNumber(JsonValue &out, std::string &why) {
        const size_t start = m_pos;
        if (m_text[m_pos] == '-') ++m_pos;
        while (m_pos < m_text.size() &&
               std::isdigit(static_cast<unsigned char>(m_text[m_pos])) != 0) {
            ++m_pos;
        }
        if (m_pos < m_text.size() && m_text[m_pos] == '.') {
            ++m_pos;
            while (m_pos < m_text.size() &&
                   std::isdigit(static_cast<unsigned char>(m_text[m_pos])) != 0) {
                ++m_pos;
            }
        }
        const std::string token = m_text.substr(start, m_pos - start);
        try {
            out.value = std::stod(token);
        } catch (...) {
            why = "json parse: invalid number";
            return false;
        }
        return true;
    }
};

std::string jsonEscape(const std::string &s) {
    std::ostringstream oss;
    for (const char ch : s) {
        switch (ch) {
            case '\\': oss << "\\\\"; break;
            case '"': oss << "\\\""; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default: oss << ch; break;
        }
    }
    return oss.str();
}

std::string sanitizePathComponent(std::string s) {
    for (char &ch : s) {
        const unsigned char uch = static_cast<unsigned char>(ch);
        if (std::isalnum(uch) == 0 && ch != '-' && ch != '_') ch = '_';
    }
    if (s.empty()) s = "case";
    return s;
}

const JsonObject *jsonObject(const JsonValue &value) {
    return value.isObject() ? &std::get<JsonObject>(value.value) : nullptr;
}

const JsonArray *jsonArray(const JsonValue &value) {
    return value.isArray() ? &std::get<JsonArray>(value.value) : nullptr;
}

bool jsonBoolOrDefault(const JsonObject &obj,
                       const std::string &key,
                       bool defaultValue,
                       bool &out,
                       std::string &why) {
    const auto it = obj.find(key);
    if (it == obj.end()) {
        out = defaultValue;
        return true;
    }
    if (!it->second.isBool()) {
        why = "manifest: key '" + key + "' must be bool";
        return false;
    }
    out = std::get<bool>(it->second.value);
    return true;
}

bool jsonIntField(const JsonObject &obj,
                  const std::string &key,
                  int &out,
                  std::string &why) {
    const auto it = obj.find(key);
    if (it == obj.end() || !it->second.isNumber()) {
        why = "manifest: key '" + key + "' must be number";
        return false;
    }
    out = static_cast<int>(std::llround(std::get<double>(it->second.value)));
    return true;
}

bool jsonUint64Field(const JsonObject &obj,
                     const std::string &key,
                     uint64_t &out,
                     std::string &why) {
    const auto it = obj.find(key);
    if (it == obj.end() || !it->second.isNumber()) {
        why = "manifest: key '" + key + "' must be number";
        return false;
    }
    out = static_cast<uint64_t>(std::llround(std::get<double>(it->second.value)));
    return true;
}

bool jsonStringField(const JsonObject &obj,
                     const std::string &key,
                     std::string &out,
                     std::string &why,
                     bool required) {
    const auto it = obj.find(key);
    if (it == obj.end()) {
        if (!required) {
            out.clear();
            return true;
        }
        why = "manifest: missing key '" + key + "'";
        return false;
    }
    if (it->second.isNull() && !required) {
        out.clear();
        return true;
    }
    if (!it->second.isString()) {
        why = "manifest: key '" + key + "' must be string";
        return false;
    }
    out = std::get<std::string>(it->second.value);
    return true;
}

uint64_t deltaCount(uint64_t before, uint64_t after) {
    return after >= before ? (after - before) : 0;
}

bool hasExpectedSpecialPathTagDelta(const RewriteRStats &before,
                                    const RewriteRStats &after,
                                    const std::string &tag) {
    if (tag.empty()) return true;
    for (size_t i = 0; i < kRewritePathTakenCount; ++i) {
        const auto path = static_cast<RewritePathTaken>(i);
        if (tag == rewritePathTakenName(path)) {
            return deltaCount(before.rewritePathTakenCounts[i],
                              after.rewritePathTakenCounts[i]) > 0;
        }
    }
    if (tag == "SEQ_SP_CLEANUP") {
        return deltaCount(before.seqSameTypeSPCleanupSuccessCount,
                          after.seqSameTypeSPCleanupSuccessCount) > 0;
    }
    if (tag == "SEQ_XSHARED_PROXY_LOOP_REAL") {
        return deltaCount(before.seqXSharedLoopSharedProxyLoopRealHandledCount,
                          after.seqXSharedLoopSharedProxyLoopRealHandledCount) > 0;
    }
    if (tag == "SEQ_SELF_LOOP_SPQR_READY") {
        return deltaCount(before.seqSelfLoopRemainderSpqrReadyHandledCount,
                          after.seqSelfLoopRemainderSpqrReadyHandledCount) > 0;
    }
    if (tag == "SEQ_SELF_LOOP_ONE_EDGE") {
        return deltaCount(before.seqSelfLoopRemainderOneEdgeHandledCount,
                          after.seqSelfLoopRemainderOneEdgeHandledCount) > 0;
    }
    if (tag == "SEQ_XINCIDENT_SPQR_READY") {
        return deltaCount(before.seqXIncidentSpqrReadyHandledCount,
                          after.seqXIncidentSpqrReadyHandledCount) > 0;
    }
    if (tag == "SEQ_ONE_EDGE_REAL_NONLOOP") {
        return deltaCount(before.seqTooSmallOneEdgeRealNonLoopHandledCount,
                          after.seqTooSmallOneEdgeRealNonLoopHandledCount) > 0;
    }
    return false;
}

void writeRewriteSeqRegressionSummaryJson(const RewriteSeqRegressionSummary &summary,
                                          const std::string &path) {
    std::ofstream ofs(path);
    ofs << "{\n";
    ofs << "  \"totalCases\": " << summary.totalCases << ",\n";
    ofs << "  \"passedCases\": " << summary.passedCases << ",\n";
    ofs << "  \"failedCases\": " << summary.failedCases << ",\n";
    ofs << "  \"failedCaseNames\": [";
    for (size_t i = 0; i < summary.failedCaseNames.size(); ++i) {
        if (i != 0) ofs << ", ";
        ofs << '"' << jsonEscape(summary.failedCaseNames[i]) << '"';
    }
    ofs << "],\n";
    ofs << "  \"manifestPath\": \"" << jsonEscape(summary.manifestPath) << "\",\n";
    ofs << "  \"dumpDir\": \"" << jsonEscape(summary.dumpDir) << "\",\n";
    ofs << "  \"caseResults\": [\n";
    for (size_t i = 0; i < summary.caseResults.size(); ++i) {
        const auto &r = summary.caseResults[i];
        ofs << "    {\n";
        ofs << "      \"name\": \"" << jsonEscape(r.name) << "\",\n";
        ofs << "      \"ok\": " << (r.ok ? "true" : "false") << ",\n";
        ofs << "      \"elapsedMs\": " << r.elapsedMs << ",\n";
        ofs << "      \"why\": \"" << jsonEscape(r.why) << "\",\n";
        ofs << "      \"dumpPath\": \"" << jsonEscape(r.dumpPath) << "\"\n";
        ofs << "    }";
        if (i + 1 != summary.caseResults.size()) ofs << ',';
        ofs << "\n";
    }
    ofs << "  ]\n";
    ofs << "}\n";
}

std::vector<CompactGraph> buildManualCases() {
    std::vector<CompactGraph> out;
    out.push_back(makeTriangle(0));
    CompactGraph H = makeTriangle(10);
    H.edges.push_back({3, CompactEdgeKind::PROXY, 0, 1, -1, -1, -1, Agg{1, 2, 0, 0, -1, H.origOfCv[0]}, -1});
    out.push_back(H);
    return out;
}

std::vector<ExplicitBlockGraph> buildManualRewriteCases() {
    std::vector<ExplicitBlockGraph> out;
    out.push_back(makeExplicitGraph({1, 2, 3, 4},
                                    {{1, 1, 2}, {2, 1, 3}, {3, 1, 4},
                                     {4, 2, 3}, {5, 2, 4}, {6, 3, 4}}));
    out.push_back(makeExplicitGraph({1, 2, 3, 4, 5},
                                    {{10, 1, 2}, {11, 1, 3}, {12, 1, 4}, {13, 2, 3}, {14, 2, 4},
                                     {15, 2, 5}, {16, 3, 4}, {17, 3, 5}, {18, 4, 5}}));
    out.push_back(makeExplicitGraph({1, 2, 3, 4, 5},
                                    {{20, 1, 2}, {21, 2, 3}, {22, 3, 4}, {23, 4, 5}, {24, 1, 5},
                                     {25, 1, 3}, {26, 1, 4}, {27, 3, 5}}));
    return out;
}

ExplicitBlockGraph makeRandomRewriteCase(uint64_t seed, int caseIndex) {
    std::mt19937_64 rng(seed + caseIndex * 32452843ULL);
    const int n = 4 + static_cast<int>(rng() % 3);

    std::vector<VertexId> vertices;
    vertices.reserve(n);
    for (int i = 0; i < n; ++i) vertices.push_back(i + 1);

    std::set<std::pair<int, int>> seenPairs;
    std::vector<ExplicitEdge> edges;
    auto addEdge = [&](VertexId u, VertexId v) {
        auto poles = canonPole(u, v);
        if (!seenPairs.insert(poles).second) return;
        edges.push_back({static_cast<EdgeId>(edges.size()), poles.first, poles.second});
    };

    for (int i = 0; i < n; ++i) {
        addEdge(vertices[i], vertices[(i + 1) % n]);
    }

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            auto poles = canonPole(vertices[i], vertices[j]);
            if (seenPairs.count(poles) != 0) continue;
            if ((rng() % 100) < 45) addEdge(poles.first, poles.second);
        }
    }

    if (edges.size() == static_cast<size_t>(n)) {
        for (int i = 0; i < n; ++i) {
            const VertexId u = vertices[i];
            const VertexId v = vertices[(i + 2) % n];
            if (u == v) continue;
            addEdge(u, v);
            if (edges.size() > static_cast<size_t>(n)) break;
        }
    }

    return makeExplicitGraph(std::move(vertices), std::move(edges));
}

CompactGraph makeRandomCompactGraph(uint64_t seed, int caseIndex) {
    std::mt19937_64 rng(seed + caseIndex * 239017ULL);
    int n = 4 + static_cast<int>(rng() % 3);
    CompactGraph H;
    H.block = caseIndex;
    for (int i = 0; i < n; ++i) {
        H.origOfCv.push_back(i + 1);
        H.cvOfOrig[i + 1] = i;
        H.touchedVertices.push_back(i + 1);
    }
    int id = 0;
    for (int i = 0; i < n; ++i) {
        int j = (i + 1) % n;
        H.edges.push_back({id++, CompactEdgeKind::REAL, i, j, id});
    }
    if (rng() & 1ULL) {
        H.edges.push_back({id++, CompactEdgeKind::PROXY, 0, 2, -1, -1, -1, Agg{1, 2, 0, 0, -1, H.origOfCv[0]}, -1});
    }
    return H;
}

HarnessResult runStaticPipelineCaseDumpAware(const CompactGraph &H,
                                             IRawSpqrBackend &backend,
                                             IHarnessOps &ops,
                                             uint64_t seed,
                                             int tc,
                                             const std::string &dumpDir) {
    HarnessBundle B = makeBaseBundle(seed, tc, backend.name(), H);

    RawSpqrDecomp raw;
    std::string err;
    if (!backend.buildRaw(H, raw, err)) {
        setFailure(B, HarnessStage::RAW_BACKEND_FAIL, "backend.buildRaw", err.empty() ? "raw backend failed" : err);
        return failAndDump(B, dumpDir);
    }
    B.raw = raw;

    std::string why;
    if (!ops.validateRawSpqrDecomp(H, raw, why)) {
        setFailure(B, HarnessStage::RAW_VALIDATE_FAIL, "validateRawSpqrDecomp", why);
        return failAndDump(B, dumpDir);
    }

    StaticMiniCore mini;
    if (!ops.materializeMiniCore(H, raw, mini, why)) {
        B.miniBeforeNormalize = mini;
        setFailure(B, HarnessStage::MINI_MATERIALIZE_FAIL, "materializeMiniCore", why);
        return failAndDump(B, dumpDir);
    }
    B.miniBeforeNormalize = mini;

    if (!ops.checkMiniOwnershipConsistency(mini, H, why) ||
        !ops.checkMiniReducedInvariant(mini, H, why)) {
        setFailure(B, HarnessStage::MINI_PRECHECK_FAIL, "mini precheck", why);
        return failAndDump(B, dumpDir);
    }

    try {
        ops.normalizeWholeMiniCore(mini);
    } catch (const std::exception &e) {
        B.miniAfterNormalize = mini;
        setFailure(B, HarnessStage::MINI_NORMALIZE_THROW, "normalizeWholeMiniCore", e.what());
        return failAndDump(B, dumpDir);
    } catch (...) {
        B.miniAfterNormalize = mini;
        setFailure(B, HarnessStage::MINI_NORMALIZE_THROW, "normalizeWholeMiniCore", "unknown exception");
        return failAndDump(B, dumpDir);
    }
    B.miniAfterNormalize = mini;

    if (!ops.checkMiniOwnershipConsistency(mini, H, why) ||
        !ops.checkMiniReducedInvariant(mini, H, why)) {
        setFailure(B, HarnessStage::MINI_POSTCHECK_FAIL, "mini postcheck", why);
        return failAndDump(B, dumpDir);
    }

    return {};
}

HarnessResult runDummyGraftCaseDumpAware(const CompactGraph &H,
                                         IRawSpqrBackend &backend,
                                         IHarnessOps &ops,
                                         uint64_t seed,
                                         int tc,
                                         const std::string &dumpDir) {
    HarnessBundle B = makeBaseBundle(seed, tc, backend.name(), H);

    RawSpqrDecomp raw;
    std::string err;
    if (!backend.buildRaw(H, raw, err)) {
        setFailure(B, HarnessStage::RAW_BACKEND_FAIL, "backend.buildRaw", err.empty() ? "raw backend failed" : err);
        return failAndDump(B, dumpDir);
    }
    B.raw = raw;

    std::string why;
    if (!ops.validateRawSpqrDecomp(H, raw, why)) {
        setFailure(B, HarnessStage::RAW_VALIDATE_FAIL, "validateRawSpqrDecomp", why);
        return failAndDump(B, dumpDir);
    }

    StaticMiniCore mini;
    if (!ops.materializeMiniCore(H, raw, mini, why)) {
        B.miniBeforeNormalize = mini;
        setFailure(B, HarnessStage::MINI_MATERIALIZE_FAIL, "materializeMiniCore", why);
        return failAndDump(B, dumpDir);
    }
    B.miniBeforeNormalize = mini;

    try {
        ops.normalizeWholeMiniCore(mini);
    } catch (const std::exception &e) {
        B.miniAfterNormalize = mini;
        setFailure(B, HarnessStage::MINI_NORMALIZE_THROW, "normalizeWholeMiniCore", e.what());
        return failAndDump(B, dumpDir);
    } catch (...) {
        B.miniAfterNormalize = mini;
        setFailure(B, HarnessStage::MINI_NORMALIZE_THROW, "normalizeWholeMiniCore", "unknown exception");
        return failAndDump(B, dumpDir);
    }
    B.miniAfterNormalize = mini;

    if (!ops.checkMiniOwnershipConsistency(mini, H, why) ||
        !ops.checkMiniReducedInvariant(mini, H, why)) {
        setFailure(B, HarnessStage::MINI_POSTCHECK_FAIL, "mini postcheck", why);
        return failAndDump(B, dumpDir);
    }

    DummyActualEnvelope env;
    if (!ops.buildDummyActualCoreEnvelope(H, env, why)) {
        setFailure(B, HarnessStage::DUMMY_ENVELOPE_FAIL, "buildDummyActualCoreEnvelope", why);
        return failAndDump(B, dumpDir);
    }
    B.actualBeforeGraft = env.core;

    int keep = -1;
    if (!ops.chooseKeepMiniNode(mini, keep, why) || keep < 0) {
        setFailure(B, HarnessStage::KEEP_SELECT_FAIL, "chooseKeepMiniNode", why.empty() ? "invalid keep" : why);
        return failAndDump(B, dumpDir);
    }

    std::queue<NodeId> q;
    GraftTrace trace;
    if (!ops.graftMiniCoreIntoPlace(env.core, env.oldR, env.H, mini, keep, q, &trace, why)) {
        B.actualAfterGraft = env.core;
        setFailure(B, HarnessStage::GRAFT_FAIL, "graftMiniCoreIntoPlace", why.empty() ? "returned false" : why);
        return failAndDump(B, dumpDir);
    }
    B.actualAfterGraft = env.core;
    B.trace = trace;

    if (!ops.rebuildActualMetadata(env.core, why)) {
        B.actualAfterGraft = env.core;
        setFailure(B, HarnessStage::ACTUAL_METADATA_FAIL, "rebuildActualMetadata", why);
        return failAndDump(B, dumpDir);
    }
    B.actualAfterGraft = env.core;

    if (!ops.checkActualReducedInvariant(env.core, &env.stubNodes, why)) {
        setFailure(B, HarnessStage::ACTUAL_INVARIANT_FAIL, "checkActualReducedInvariant", why);
        return failAndDump(B, dumpDir);
    }

    B.explicitExpected = ops.materializeCompactRealProjection(env.H);
    B.explicitGot = ops.materializeWholeCoreExplicit(env.core);
    if (!ops.checkEquivalentExplicitGraphs(*B.explicitGot, *B.explicitExpected, why)) {
        setFailure(B, HarnessStage::DUMMY_REAL_SET_FAIL, "checkEquivalentExplicitGraphs", why);
        return failAndDump(B, dumpDir);
    }

    if (!ops.checkDummyProxyRewire(env, mini, trace, why)) {
        setFailure(B, HarnessStage::DUMMY_PROXY_REWIRE_FAIL, "checkDummyProxyRewire", why);
        return failAndDump(B, dumpDir);
    }

    return {};
}

HarnessResult runRewriteRFallbackCaseDumpAware(const ExplicitBlockGraph &G,
                                               IRawSpqrBackend &backend,
                                               IHarnessOps &ops,
                                               uint64_t seed,
                                               int tc,
                                               const std::string &dumpDir) {
    HarnessBundle B = makeBaseBundle(seed, tc, backend.name(), G);

    ReducedSPQRCore before;
    std::string why;
    if (!buildWholeCoreForTesting(G, before, why)) {
        setFailure(B, HarnessStage::LOCAL_BUILD_CORE_FAIL, "buildWholeCoreForTesting", why);
        return failAndDump(B, dumpDir);
    }

    B.actualBeforeRewrite = before;
    B.explicitBefore = ops.materializeWholeCoreExplicit(before);

    NodeId chosenR = -1;
    VertexId chosenX = -1;
    if (!chooseDeterministicRewriteTarget(before, chosenR, chosenX, why)) {
        B.actualAfterRewrite = before;
        setFailure(B, HarnessStage::LOCAL_CHOOSE_RX_FAIL, "chooseDeterministicRewriteTarget", why);
        return failAndDump(B, dumpDir);
    }
    B.chosenR = chosenR;
    B.chosenX = chosenX;

    ReducedSPQRCore after = before;
    setRewriteRCaseContext(seed, tc);
    setRewriteRSequenceMode(false);
    if (!ops.rewriteRFallback(after, chosenR, chosenX, why)) {
        captureRewriteAfter(B, ops, after);
        setFailure(B, HarnessStage::LOCAL_REWRITE_R_FAIL, "rewriteR_fallback", why);
        return failAndDump(B, dumpDir);
    }

    if (!ops.normalizeTouchedRegion(after, why)) {
        captureRewriteAfter(B, ops, after);
        setFailure(B, HarnessStage::LOCAL_NORMALIZE_FAIL, "normalizeTouchedRegion", why);
        return failAndDump(B, dumpDir);
    }
    captureRewriteAfter(B, ops, after);

    if (!ops.checkActualReducedInvariant(after, nullptr, why)) {
        setFailure(B, HarnessStage::LOCAL_ACTUAL_INVARIANT_FAIL, "checkActualReducedInvariant", why);
        return failAndDump(B, dumpDir);
    }

    ReducedSPQRCore oracle;
    if (!buildWholeCoreForTesting(*B.explicitAfter, oracle, why)) {
        B.explicitGot = B.explicitAfter;
        setFailure(B, HarnessStage::LOCAL_ORACLE_FAIL, "buildWholeCoreForTesting(oracle)", why);
        return failAndDump(B, dumpDir);
    }
    B.explicitExpected = ops.materializeWholeCoreExplicit(oracle);
    B.explicitGot = B.explicitAfter;
    if (!ops.checkEquivalentExplicitGraphs(*B.explicitGot, *B.explicitExpected, why)) {
        setFailure(B, HarnessStage::LOCAL_ORACLE_FAIL, "checkEquivalentExplicitGraphs", why);
        return failAndDump(B, dumpDir);
    }

    return {};
}

HarnessResult runRewriteRSequenceCaseDumpAware(const ExplicitBlockGraph &G,
                                               IRawSpqrBackend &backend,
                                               IHarnessOps &ops,
                                               uint64_t seed,
                                               int tc,
                                               const std::string &dumpDir) {
    (void)ops;
    noteRewriteSeqCaseStart();
    HarnessBundle B = makeBaseBundle(seed, tc, backend.name(), G);

    ReducedSPQRCore current;
    std::string why;
    if (!buildWholeCoreForTesting(G, current, why)) {
        noteRewriteSeqCaseFinish(false, 0, false, false);
        setFailure(B, HarnessStage::SEQ_BUILD_CORE_FAIL, "buildWholeCoreForTesting", why);
        return failAndDump(B, dumpDir);
    }
    setRewriteRCaseContext(seed, tc);
    RewriteSeqStats seqStats;
    const bool ok = runRewriteSequenceToFixpoint(current, seqStats, why);
    noteRewriteSeqCaseFinish(ok,
                             seqStats.completedSteps,
                             seqStats.hadSequenceFallback,
                             seqStats.maxStepReached);
    if (ok) {
        return {};
    }

    B.stepIndex = seqStats.failureStepIndex;
    B.sequenceLengthSoFar = seqStats.sequenceLengthSoFar;
    B.chosenR = seqStats.chosenR;
    B.chosenX = seqStats.chosenX;
    B.actualBeforeRewrite = seqStats.actualBeforeFailure;
    B.actualAfterRewrite = seqStats.actualAfterFailure;
    B.explicitBefore = seqStats.explicitBeforeFailure;
    B.explicitAfter = seqStats.explicitAfterFailure;
    B.explicitExpected = seqStats.explicitExpected;
    B.explicitGot = seqStats.explicitGot;
    setFailure(B,
               seqStats.failureStage,
               seqStats.failureWhere.empty() ? "runRewriteSequenceToFixpoint"
                                             : seqStats.failureWhere,
               seqStats.failureWhy.empty() ? why : seqStats.failureWhy);
    return failAndDump(B, dumpDir);
}

HarnessResult runRewriteRSequenceReplayDumpAware(const ExplicitBlockGraph &G,
                                                 IRawSpqrBackend &backend,
                                                 IHarnessOps &ops,
                                                 uint64_t seed,
                                                 int tc,
                                                 int targetStep,
                                                 const std::string &dumpDir,
                                                 HarnessBundle *capturedBundle) {
    constexpr int kMaxSequenceSteps = 64;

    noteRewriteSeqCaseStart();
    HarnessBundle B = makeBaseBundle(seed, tc, backend.name(), G);
    bool hadSequenceFallback = false;
    int completedSteps = 0;
    B.targetTcIndex = tc;
    B.targetStep = targetStep;

    auto returnFail = [&](HarnessStage stage,
                          const std::string &where,
                          const std::string &failWhy) {
        noteRewriteSeqCaseFinish(false, completedSteps, hadSequenceFallback, stage == HarnessStage::SEQ_MAX_STEPS_REACHED);
        setFailure(B, stage, where, failWhy);
        HarnessResult R = failAndDump(B, dumpDir);
        R.bundle = B;
        if (capturedBundle) *capturedBundle = B;
        return R;
    };

    auto returnSuccessCapture = [&]() {
        noteRewriteSeqCaseFinish(true, completedSteps, hadSequenceFallback, false);
        B.stage = HarnessStage::SEQ_REPLAY_CAPTURE;
        B.where = "runRewriteRSequenceReplayDumpAware";
        B.why = "captured target sequence step";
        HarnessResult R = succeedAndDump(B, dumpDir);
        R.bundle = B;
        if (capturedBundle) *capturedBundle = B;
        return R;
    };

    auto captureReplayTraceIntoBundle = [&](const GraftTrace &trace) {
        B.trace = trace;
        if (trace.preCleanupPostcheckSubtype != GraftPostcheckSubtype::GPS_OTHER ||
            trace.postCleanupPostcheckSubtype != GraftPostcheckSubtype::GPS_OTHER) {
            B.postcheckSubtype = trace.postCleanupPostcheckSubtype;
        } else {
            B.postcheckSubtype = trace.postcheckSubtype;
        }
        B.oldNodeSnapshotsByPhase = trace.oldNodeSnapshotsByPhase;
        B.affectedNodeSnapshotsByPhase = trace.affectedNodeSnapshotsByPhase;
    };

    ReducedSPQRCore current;
    std::string why;
    if (!buildWholeCoreForTesting(G, current, why)) {
        return returnFail(HarnessStage::SEQ_BUILD_CORE_FAIL,
                          "buildWholeCoreForTesting",
                          why);
    }

    for (int step = 0; step < kMaxSequenceSteps; ++step) {
        B.stepIndex = step;
        B.sequenceLengthSoFar = step;
        B.actualBeforeRewrite = current;
        B.explicitBefore = ops.materializeWholeCoreExplicit(current);
        B.actualAfterRewrite.reset();
        B.actualAfterNormalize.reset();
        B.explicitAfter.reset();
        B.explicitAfterNormalize.reset();
        B.explicitExpected.reset();
        B.explicitGot.reset();
        B.trace.reset();
        B.postcheckSubtype.reset();
        B.normalizeOk.reset();
        B.actualInvariantOk.reset();
        B.oracleBuildOk.reset();
        B.oracleEquivalentOk.reset();
        B.normalizeWhy.clear();
        B.actualInvariantWhy.clear();
        B.oracleWhy.clear();
        B.oldNodeSnapshotsByPhase.clear();
        B.affectedNodeSnapshotsByPhase.clear();
        B.chosenR.reset();
        B.chosenX.reset();

        NodeId chosenR = -1;
        VertexId chosenX = -1;
        const SequenceChooseStatus chooseStatus =
            chooseDeterministicSequenceRewriteTarget(current, chosenR, chosenX, why);
        if (chooseStatus == SequenceChooseStatus::ERROR) {
            return returnFail(HarnessStage::SEQ_CHOOSE_RX_FAIL,
                              "chooseDeterministicSequenceRewriteTarget",
                              why);
        }
        if (chooseStatus == SequenceChooseStatus::NONE) {
            return returnFail(HarnessStage::SEQ_PROGRESS_STUCK,
                              "rewrite-r-seq-replay",
                              "replay target step not reached before sequence exhausted");
        }
        B.chosenR = chosenR;
        B.chosenX = chosenX;

        ReducedSPQRCore after = current;
        B.sequenceLengthSoFar = step + 1;

        const bool captureThisStep = (step == targetStep);
        auto disableReplayCapture = [&]() {
            if (captureThisStep) {
                setRewriteRSequenceReplayCaptureEnabled(false);
            }
        };
        if (captureThisStep) {
            clearRewriteRSequenceReplayCapture();
            setRewriteRSequenceReplayCaptureEnabled(true);
        }

        setRewriteRCaseContext(seed, tc);
        setRewriteRSequenceStepContext(step, step + 1);
        const uint64_t seqFallbacksBefore = getRewriteRStats().seqRewriteWholeCoreFallbackCount;
        setRewriteRSequenceMode(true);
        const bool rewriteOk = ops.rewriteRFallback(after, chosenR, chosenX, why);
        setRewriteRSequenceMode(false);
        const uint64_t seqFallbacksAfter = getRewriteRStats().seqRewriteWholeCoreFallbackCount;
        hadSequenceFallback = hadSequenceFallback || (seqFallbacksAfter > seqFallbacksBefore);

        if (captureThisStep) {
            GraftTrace replayTrace;
            if (takeRewriteRSequenceReplayTrace(replayTrace)) {
                captureReplayTraceIntoBundle(replayTrace);
            }
        }

        if (!rewriteOk) {
            disableReplayCapture();
            captureRewriteAfter(B, ops, after);
            return returnFail(HarnessStage::SEQ_REWRITE_R_FAIL, "rewriteR_fallback", why);
        }

        if (captureThisStep) {
            B.actualAfterRewrite = after;
            B.explicitAfter = ops.materializeWholeCoreExplicit(after);
        }

        if (!ops.normalizeTouchedRegion(after, why)) {
            noteRewriteRWeakRepairCommitOutcome(WeakRepairCommitOutcome::WCO_NORMALIZE_FAIL,
                                                &after,
                                                why);
            if (captureThisStep) {
                B.normalizeOk = false;
                B.normalizeWhy = why;
            }
            disableReplayCapture();
            return returnFail(HarnessStage::SEQ_NORMALIZE_FAIL, "normalizeTouchedRegion", why);
        }

        if (captureThisStep) {
            B.normalizeOk = true;
            B.normalizeWhy.clear();
            B.actualAfterNormalize = after;
            B.explicitAfterNormalize = ops.materializeWholeCoreExplicit(after);
            if (B.trace) {
                captureReplaySnapshotsForPhase(after,
                                               chosenR,
                                               chosenR,
                                               B.trace->actualNodes,
                                               B.trace->resolvedProxyEndpoints,
                                               B.trace->preservedProxyArcs,
                                               ReplaySnapshotPhase::AFTER_NORMALIZE,
                                               *B.trace);
                B.oldNodeSnapshotsByPhase = B.trace->oldNodeSnapshotsByPhase;
                B.affectedNodeSnapshotsByPhase = B.trace->affectedNodeSnapshotsByPhase;
                if (hasPendingSequenceDeferredSameTypeSP()) {
                    setSequenceDeferredSameTypeSPTrace(*B.trace);
                }
            }
        }

        if (hasPendingSequenceDeferredSameTypeSP()) {
            GraftTrace cleanupTrace;
            bool haveCleanupTrace = false;
            if (captureThisStep && B.trace) {
                cleanupTrace = *B.trace;
                haveCleanupTrace = true;
            } else if (peekSequenceDeferredSameTypeSPTrace(cleanupTrace)) {
                haveCleanupTrace = true;
            }

            if (haveCleanupTrace) {
                std::string preCleanupWhyDetailed;
                cleanupTrace.preCleanupPostcheckSubtype =
                    classifyPostcheckFailureDetailed(after, preCleanupWhyDetailed);
                cleanupTrace.postCleanupPostcheckSubtype =
                    cleanupTrace.preCleanupPostcheckSubtype;
                if (!preCleanupWhyDetailed.empty()) {
                    cleanupTrace.postcheckWhyDetailed = preCleanupWhyDetailed;
                }

                if (cleanupTrace.preCleanupPostcheckSubtype ==
                    GraftPostcheckSubtype::GPS_SAME_TYPE_SP_ONLY) {
                    const auto seeds = collectSequenceSPCleanupSeeds(
                        after, chosenR, &cleanupTrace, &cleanupTrace.preservedProxyArcs);
                    cleanupTrace.sameTypeSPCleanupSeedNodes = seeds;

                    if (captureThisStep) {
                        captureReplaySnapshotsForPhase(after,
                                                       chosenR,
                                                       chosenR,
                                                       cleanupTrace.actualNodes,
                                                       cleanupTrace.resolvedProxyEndpoints,
                                                       cleanupTrace.preservedProxyArcs,
                                                       ReplaySnapshotPhase::BEFORE_SP_CLEANUP,
                                                       cleanupTrace);
                    }

                    setSequenceDeferredSameTypeSPTrace(cleanupTrace);

                    std::string cleanupWhy;
                    if (!cleanupSequenceSameTypeSPAdjacency(after, seeds, cleanupWhy)) {
                        peekSequenceDeferredSameTypeSPTrace(cleanupTrace);
                        cleanupTrace.graftOtherWhy = cleanupWhy;
                        if (captureThisStep) {
                            captureReplaySnapshotsForPhase(after,
                                                           chosenR,
                                                           chosenR,
                                                           cleanupTrace.actualNodes,
                                                           cleanupTrace.resolvedProxyEndpoints,
                                                           cleanupTrace.preservedProxyArcs,
                                                           ReplaySnapshotPhase::AFTER_SP_CLEANUP,
                                                           cleanupTrace);
                            captureReplayTraceIntoBundle(cleanupTrace);
                            B.actualInvariantOk = false;
                            B.actualInvariantWhy = cleanupWhy;
                        }
                        setSequenceDeferredSameTypeSPTrace(cleanupTrace);
                        flushSequenceDeferredSameTypeSPDump(after);
                        disableReplayCapture();
                        return returnFail(HarnessStage::SEQ_ACTUAL_INVARIANT_FAIL,
                                          "cleanupSequenceSameTypeSPAdjacency",
                                          cleanupWhy);
                    }

                    peekSequenceDeferredSameTypeSPTrace(cleanupTrace);
                    std::string postCleanupWhyDetailed;
                    cleanupTrace.postCleanupPostcheckSubtype =
                        classifyPostcheckFailureDetailed(after, postCleanupWhyDetailed);
                    if (!postCleanupWhyDetailed.empty()) {
                        cleanupTrace.postcheckWhyDetailed = postCleanupWhyDetailed;
                    }

                    if (captureThisStep) {
                        captureReplaySnapshotsForPhase(after,
                                                       chosenR,
                                                       chosenR,
                                                       cleanupTrace.actualNodes,
                                                       cleanupTrace.resolvedProxyEndpoints,
                                                       cleanupTrace.preservedProxyArcs,
                                                       ReplaySnapshotPhase::AFTER_SP_CLEANUP,
                                                       cleanupTrace);
                        captureReplayTraceIntoBundle(cleanupTrace);
                    }
                    setSequenceDeferredSameTypeSPTrace(cleanupTrace);
                } else {
                    if (captureThisStep) {
                        captureReplayTraceIntoBundle(cleanupTrace);
                    }
                    setSequenceDeferredSameTypeSPTrace(cleanupTrace);
                }
            }
        }

        flushSequenceDeferredSameTypeSPDump(after);

        if (!ops.checkActualReducedInvariant(after, nullptr, why)) {
            noteRewriteRWeakRepairCommitOutcome(
                WeakRepairCommitOutcome::WCO_ACTUAL_INVARIANT_FAIL,
                &after,
                why);
            if (captureThisStep) {
                B.actualInvariantOk = false;
                B.actualInvariantWhy = why;
            }
            disableReplayCapture();
            return returnFail(HarnessStage::SEQ_ACTUAL_INVARIANT_FAIL,
                              "checkActualReducedInvariant",
                              why);
        }
        if (captureThisStep) {
            B.actualInvariantOk = true;
            B.actualInvariantWhy.clear();
        }

        const ExplicitBlockGraph explicitAfterGraph =
            (captureThisStep && B.explicitAfter)
                ? *B.explicitAfter
                : ops.materializeWholeCoreExplicit(after);

        NodeId nextR = -1;
        VertexId nextX = -1;
        const SequenceChooseStatus nextChooseStatus =
            chooseDeterministicSequenceRewriteTarget(after, nextR, nextX, why);
        if (nextChooseStatus == SequenceChooseStatus::ERROR) {
            disableReplayCapture();
            return returnFail(HarnessStage::SEQ_CHOOSE_RX_FAIL,
                              "chooseDeterministicSequenceRewriteTarget(after)",
                              why);
        }
        if (B.explicitBefore &&
            explicitAfterGraph.edges.size() >= B.explicitBefore->edges.size() &&
            nextChooseStatus == SequenceChooseStatus::FOUND) {
            disableReplayCapture();
            return returnFail(
                HarnessStage::SEQ_PROGRESS_STUCK,
                "sequence progress check",
                "rewrite sequence made no edge-count progress while another rewrite target remains");
        }

        ReducedSPQRCore oracle;
        if (!buildWholeCoreForTesting(explicitAfterGraph, oracle, why)) {
            noteRewriteRWeakRepairCommitOutcome(WeakRepairCommitOutcome::WCO_ORACLE_FAIL,
                                                &after,
                                                why);
            if (captureThisStep) {
                B.oracleBuildOk = false;
                B.oracleWhy = why;
                B.explicitGot = explicitAfterGraph;
            }
            disableReplayCapture();
            return returnFail(HarnessStage::SEQ_ORACLE_FAIL,
                              "buildWholeCoreForTesting(oracle)",
                              why);
        }

        const ExplicitBlockGraph explicitExpectedGraph =
            ops.materializeWholeCoreExplicit(oracle);
        if (captureThisStep) {
            B.oracleBuildOk = true;
            B.explicitExpected = explicitExpectedGraph;
            B.explicitGot = explicitAfterGraph;
        }
        if (!ops.checkEquivalentExplicitGraphs(explicitAfterGraph, explicitExpectedGraph, why)) {
            noteRewriteRWeakRepairCommitOutcome(WeakRepairCommitOutcome::WCO_ORACLE_FAIL,
                                                &after,
                                                why);
            if (captureThisStep) {
                B.oracleEquivalentOk = false;
                B.oracleWhy = why;
            }
            disableReplayCapture();
            return returnFail(HarnessStage::SEQ_ORACLE_FAIL,
                              "checkEquivalentExplicitGraphs",
                              why);
        }
        if (captureThisStep) {
            B.oracleEquivalentOk = true;
            B.oracleWhy.clear();
        }

        noteRewriteRWeakRepairCommitOutcome(WeakRepairCommitOutcome::WCO_COMMITTED,
                                            &after,
                                            {});
        current = std::move(after);
        completedSteps = step + 1;

        if (captureThisStep) {
            disableReplayCapture();
            return returnSuccessCapture();
        }
    }

    B.stepIndex = kMaxSequenceSteps;
    B.sequenceLengthSoFar = kMaxSequenceSteps;
    B.actualBeforeRewrite = current;
    B.explicitBefore = ops.materializeWholeCoreExplicit(current);
    return returnFail(HarnessStage::SEQ_MAX_STEPS_REACHED,
                      "rewrite-r-seq-replay",
                      "replay target step not reached before max steps");
}

bool loadRewriteSeqRegressionCases(const std::string &manifestPath,
                                   std::vector<RewriteSeqRegressionCase> &cases,
                                   std::string &why) {
    cases.clear();
    std::ifstream ifs(manifestPath);
    if (!ifs) {
        why = "failed to open manifest: " + manifestPath;
        return false;
    }
    std::ostringstream buffer;
    buffer << ifs.rdbuf();

    JsonValue root;
    JsonParser parser(buffer.str());
    if (!parser.parse(root, why)) return false;

    const JsonArray *caseArray = nullptr;
    if (const auto *obj = jsonObject(root)) {
        const auto it = obj->find("cases");
        if (it == obj->end()) {
            why = "manifest: top-level object must contain 'cases'";
            return false;
        }
        caseArray = jsonArray(it->second);
    } else {
        caseArray = jsonArray(root);
    }
    if (caseArray == nullptr) {
        why = "manifest: top-level value must be an array or object with 'cases'";
        return false;
    }

    for (const auto &item : *caseArray) {
        const auto *obj = jsonObject(item);
        if (obj == nullptr) {
            why = "manifest: each case must be an object";
            return false;
        }
        RewriteSeqRegressionCase c;
        if (!jsonStringField(*obj, "name", c.name, why, true)) return false;
        if (!jsonUint64Field(*obj, "seed", c.seed, why)) return false;
        if (!jsonIntField(*obj, "tcIndex", c.tcIndex, why)) return false;
        if (!jsonIntField(*obj, "targetStep", c.targetStep, why)) return false;
        if (!jsonBoolOrDefault(*obj, "expectedTopLevelOk", true, c.expectedTopLevelOk, why)) return false;
        if (!jsonBoolOrDefault(*obj, "expectedActualInvariantOk", true, c.expectedActualInvariantOk, why)) return false;
        if (!jsonBoolOrDefault(*obj, "expectedOracleEquivalentOk", true, c.expectedOracleEquivalentOk, why)) return false;
        if (!jsonStringField(*obj, "expectedPostcheckSubtype", c.expectedPostcheckSubtype, why, false)) return false;
        if (!jsonStringField(*obj, "expectedSpecialPathTag", c.expectedSpecialPathTag, why, false)) return false;
        cases.push_back(std::move(c));
    }

    if (cases.empty()) {
        why = "manifest: no cases";
        return false;
    }
    why.clear();
    return true;
}

bool runRewriteRSequenceRegressionManifestDumpAware(
    const std::vector<RewriteSeqRegressionCase> &cases,
    IRawSpqrBackend &backend,
    IHarnessOps &ops,
    const std::string &dumpDir,
    RewriteSeqRegressionSummary &summary,
    std::string &why) {
    summary = {};
    summary.totalCases = static_cast<int>(cases.size());
    summary.dumpDir = dumpDir;
    std::filesystem::create_directories(dumpDir);

    for (const auto &c : cases) {
        const auto startedAt = std::chrono::steady_clock::now();
        const RewriteRStats statsBefore = getRewriteRStats();
        const std::string caseDumpDir = dumpDir + "/" + sanitizePathComponent(c.name);
        HarnessBundle capturedBundle;
        HarnessResult result = runRewriteRSequenceReplayDumpAware(
            makeRandomRewriteCase(c.seed, c.tcIndex),
            backend,
            ops,
            c.seed,
            c.tcIndex,
            c.targetStep,
            caseDumpDir,
            &capturedBundle);
        const RewriteRStats statsAfter = getRewriteRStats();
        const auto finishedAt = std::chrono::steady_clock::now();

        RewriteSeqRegressionCaseResult caseResult;
        caseResult.name = c.name;
        caseResult.elapsedMs =
            std::chrono::duration<double, std::milli>(finishedAt - startedAt).count();
        caseResult.dumpPath = result.dumpPath;

        auto failCase = [&](const std::string &msg) {
            caseResult.ok = false;
            caseResult.why = msg;
            summary.failedCaseNames.push_back(c.name);
        };

        const HarnessBundle *bundle = result.bundle ? &*result.bundle : &capturedBundle;
        if (result.ok != c.expectedTopLevelOk) {
            failCase("expectedTopLevelOk mismatch");
        } else if (bundle == nullptr) {
            failCase("missing captured bundle");
        } else {
            const bool actualInvariantOk =
                bundle->actualInvariantOk.has_value() && *bundle->actualInvariantOk;
            const bool oracleEquivalentOk =
                bundle->oracleEquivalentOk.has_value() && *bundle->oracleEquivalentOk;
            if (actualInvariantOk != c.expectedActualInvariantOk) {
                failCase("expectedActualInvariantOk mismatch");
            } else if (oracleEquivalentOk != c.expectedOracleEquivalentOk) {
                failCase("expectedOracleEquivalentOk mismatch");
            } else if (!c.expectedPostcheckSubtype.empty()) {
                const std::string actualSubtype =
                    bundle->postcheckSubtype.has_value()
                        ? graftPostcheckSubtypeName(*bundle->postcheckSubtype)
                        : "";
                if (actualSubtype != c.expectedPostcheckSubtype) {
                    failCase("expectedPostcheckSubtype mismatch");
                }
            }

            if (caseResult.why.empty() &&
                !hasExpectedSpecialPathTagDelta(statsBefore, statsAfter, c.expectedSpecialPathTag)) {
                failCase("expectedSpecialPathTag mismatch");
            }
        }

        if (caseResult.why.empty()) {
            caseResult.ok = true;
            ++summary.passedCases;
        } else {
            ++summary.failedCases;
        }
        summary.caseResults.push_back(std::move(caseResult));
    }

    writeRewriteSeqRegressionSummaryJson(summary, dumpDir + "/regression_summary.json");
    if (summary.failedCases != 0) {
        why = "rewrite-r-seq-regression failed";
        return false;
    }
    why.clear();
    return true;
}

} // namespace harness
