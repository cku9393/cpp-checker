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
            std::cout << "--backend ogdf --mode {static|dummy|rewrite-r|rewrite-r-seq|rewrite-seq} --seed N --rounds N --manual-only --dump-dir DIR\n";
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
        if (cfg.mode == "rewrite-r" || cfg.mode == "rewrite-r-seq" || cfg.mode == "rewrite-seq") {
            resetRewriteRStats();
        }

        int tc = 0;
        auto printRewriteStats = [&]() {
            if (cfg.mode != "rewrite-r" &&
                cfg.mode != "rewrite-r-seq" &&
                cfg.mode != "rewrite-seq") {
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
                      << " seqTooSmallOtherHandledCount=" << stats.seqTooSmallOtherHandledCount
                      << " seqLoopPlusEdgeSharedHandledCount=" << stats.seqLoopPlusEdgeSharedHandledCount
                      << " seqSelfLoopRemainderTwoPathHandledCount="
                      << stats.seqSelfLoopRemainderTwoPathHandledCount
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
            } else if (cfg.mode == "rewrite-r-seq" || cfg.mode == "rewrite-seq") {
                r = runRewriteRSequenceCaseDumpAware(G, backend, ops, cfg.seed, tc, cfg.dumpDir);
            } else {
                throw std::runtime_error("unknown rewrite mode: " + cfg.mode);
            }
            if (!r.ok) {
                std::cerr << "[FAIL] tc=" << tc << " where=" << r.where << " why=" << r.why << "\n";
                std::cerr << "bundle=" << r.dumpPath << "\n";
                return false;
            }
            recordRewriteRPass(cfg.manualOnly);
            ++tc;
            return true;
        };

        if (cfg.mode == "rewrite-r" || cfg.mode == "rewrite-r-seq" || cfg.mode == "rewrite-seq") {
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
