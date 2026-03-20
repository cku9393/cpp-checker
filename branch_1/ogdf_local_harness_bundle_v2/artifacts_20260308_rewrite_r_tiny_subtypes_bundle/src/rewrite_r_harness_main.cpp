#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
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
        else if (a == "--manual-only") cfg.manualOnly = true;
        else if (a == "--mode") cfg.mode = next("--mode");
        else if (a == "--dump-dir") cfg.dumpDir = next("--dump-dir");
        else if (a == "--backend") { (void)next("--backend"); }
        else if (a == "--help") {
            std::cout << "--backend ogdf --mode {static|dummy|rewrite-r} --seed N --rounds N --manual-only --dump-dir DIR\n";
            std::exit(0);
        } else {
            throw std::runtime_error("unknown argument: " + a);
        }
    }
    return cfg;
}
}

int main(int argc, char **argv) {
    try {
        RunConfig cfg = parseArgs(argc, argv);
        std::filesystem::create_directories(cfg.dumpDir);

        OgdfRawSpqrBackend backend;
        ProjectHarnessOps ops;
        if (cfg.mode == "rewrite-r") {
            resetRewriteRStats();
        }

        int tc = 0;
        auto printRewriteStats = [&]() {
            if (cfg.mode != "rewrite-r") return;
            const RewriteRStats stats = getRewriteRStats();
            std::cout << "[REWRITE_R_STATS] "
                      << "rewriteCalls=" << stats.rewriteCalls
                      << " compactReadyCount=" << stats.compactReadyCount
                      << " compactRejectedFallbackCount=" << stats.compactRejectedFallbackCount
                      << " backendBuildRawDirectCount=" << stats.backendBuildRawDirectCount
                      << " backendBuildRawFallbackCount=" << stats.backendBuildRawFallbackCount
                      << " compactSingleCutTwoBlocksHandled=" << stats.compactSingleCutTwoBlocksHandled
                      << " compactPathOfBlocksHandled=" << stats.compactPathOfBlocksHandled
                      << " compactTooSmallHandledCount=" << stats.compactTooSmallHandledCount
                      << " rewriteFallbackWholeCoreCount=" << stats.rewriteFallbackWholeCoreCount
                      << " rewriteFallbackSpecialCaseCount=" << stats.rewriteFallbackSpecialCaseCount
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
            HarnessResult r = runRewriteRFallbackCaseDumpAware(G, backend, ops, cfg.seed, tc, cfg.dumpDir);
            if (!r.ok) {
                std::cerr << "[FAIL] tc=" << tc << " where=" << r.where << " why=" << r.why << "\n";
                std::cerr << "bundle=" << r.dumpPath << "\n";
                return false;
            }
            recordRewriteRPass(cfg.manualOnly);
            ++tc;
            return true;
        };

        if (cfg.mode == "rewrite-r") {
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
