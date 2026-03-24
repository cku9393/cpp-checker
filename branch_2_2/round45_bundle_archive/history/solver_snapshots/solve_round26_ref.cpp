#ifndef STATECERT_GATEPROF
#define STATECERT_GATEPROF 0
#endif
#ifndef STATECERT_DEBUG_CROSSCHECK
#define STATECERT_DEBUG_CROSSCHECK 0
#endif
#ifndef CHAINRELAX_GATEPROF
#define CHAINRELAX_GATEPROF 0
#endif
#ifndef CHAINRELAX_GATEEVAL
#define CHAINRELAX_GATEEVAL 0
#endif
#ifndef CHAINRELAX_SELECTED_GATE
#define CHAINRELAX_SELECTED_GATE -1
#endif
#ifndef CHAINRELAX_DEBUG_CROSSCHECK
#define CHAINRELAX_DEBUG_CROSSCHECK 0
#endif

#ifndef DENSE_SHADOW_DIFF_ROUND20_PROFILE
#define DENSE_SHADOW_DIFF_ROUND20_PROFILE 0
#endif
#ifndef DENSE_SHADOW_DIFF_ROUND20_CROSSCHECK
#define DENSE_SHADOW_DIFF_ROUND20_CROSSCHECK 0
#endif

#ifndef DENSE_RECT_ROUND1_PROFILE
#define DENSE_RECT_ROUND1_PROFILE 0
#endif

#ifndef DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
#define DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE 0
#endif

#ifndef SPARSE_HARDSCALING_ROUND5_PROFILE
#define SPARSE_HARDSCALING_ROUND5_PROFILE 0
#endif

#ifndef CHEAPFAN_ROUND6_PROFILE
#define CHEAPFAN_ROUND6_PROFILE 0
#endif

#ifndef CHEAPFAN_ROUND6_OPT
#define CHEAPFAN_ROUND6_OPT 1
#endif

#ifndef CHEAPFAN_CERT_ROUND7_PROFILE
#define CHEAPFAN_CERT_ROUND7_PROFILE 0
#endif

#ifndef COMBDENSE_GATE_ROUND8_PROFILE
#define COMBDENSE_GATE_ROUND8_PROFILE 0
#endif

#ifndef DENSE_LOCALIDADJ_ROUND9_PROFILE
#define DENSE_LOCALIDADJ_ROUND9_PROFILE 0
#endif

#ifndef DENSE_LOCALIDADJ_ROUND9_OPT
#define DENSE_LOCALIDADJ_ROUND9_OPT 1
#endif

#ifndef DENSE_BCCREUSE_ROUND12_PROFILE
#define DENSE_BCCREUSE_ROUND12_PROFILE 0
#endif

#ifndef DENSE_TINYPIECE_ROUND14_PROFILE
#define DENSE_TINYPIECE_ROUND14_PROFILE 0
#endif

#ifndef DENSE_TINYPIECE_ROUND14_CROSSCHECK
#define DENSE_TINYPIECE_ROUND14_CROSSCHECK 0
#endif

#ifndef DENSE_TINYPIECE_ROUND14_OPT
#define DENSE_TINYPIECE_ROUND14_OPT 0
#endif

#ifndef DENSE_TIEKEEP_ROUND15_PROFILE
#define DENSE_TIEKEEP_ROUND15_PROFILE 0
#endif

#ifndef DENSE_TIEKEEP_ROUND15_CROSSCHECK
#define DENSE_TIEKEEP_ROUND15_CROSSCHECK 0
#endif

#ifndef DENSE_TIEKEEP_ROUND15_OPT
#define DENSE_TIEKEEP_ROUND15_OPT 0
#endif

#ifndef DENSE_SPQR_ROUND16_PROFILE
#define DENSE_SPQR_ROUND16_PROFILE 0
#endif

#ifndef DENSE_SPQR_ROUND16_SHADOWCHECK
#define DENSE_SPQR_ROUND16_SHADOWCHECK 0
#endif

#ifndef DENSE_SPQR_ROUND16_OPT
#define DENSE_SPQR_ROUND16_OPT 0
#endif

#if COMBDENSE_GATE_ROUND8_PROFILE
#undef DENSE_RECT_ROUND1_PROFILE
#define DENSE_RECT_ROUND1_PROFILE 1
#undef DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
#define DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE 1
#undef SPARSE_HARDSCALING_ROUND5_PROFILE
#define SPARSE_HARDSCALING_ROUND5_PROFILE 1
#endif

#if DENSE_LOCALIDADJ_ROUND9_PROFILE
#undef DENSE_RECT_ROUND1_PROFILE
#define DENSE_RECT_ROUND1_PROFILE 1
#undef DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
#define DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE 1
#undef COMBDENSE_GATE_ROUND8_PROFILE
#define COMBDENSE_GATE_ROUND8_PROFILE 1
#endif

#if DENSE_BCCREUSE_ROUND12_PROFILE
#undef DENSE_RECT_ROUND1_PROFILE
#define DENSE_RECT_ROUND1_PROFILE 1
#undef DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
#define DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE 1
#undef COMBDENSE_GATE_ROUND8_PROFILE
#define COMBDENSE_GATE_ROUND8_PROFILE 1
#undef DENSE_LOCALIDADJ_ROUND9_PROFILE
#define DENSE_LOCALIDADJ_ROUND9_PROFILE 1
#endif

#if DENSE_TINYPIECE_ROUND14_PROFILE
#undef DENSE_RECT_ROUND1_PROFILE
#define DENSE_RECT_ROUND1_PROFILE 1
#undef DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
#define DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE 1
#undef COMBDENSE_GATE_ROUND8_PROFILE
#define COMBDENSE_GATE_ROUND8_PROFILE 1
#undef DENSE_LOCALIDADJ_ROUND9_PROFILE
#define DENSE_LOCALIDADJ_ROUND9_PROFILE 1
#endif

#if DENSE_TIEKEEP_ROUND15_PROFILE
#undef DENSE_RECT_ROUND1_PROFILE
#define DENSE_RECT_ROUND1_PROFILE 1
#undef DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
#define DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE 1
#undef COMBDENSE_GATE_ROUND8_PROFILE
#define COMBDENSE_GATE_ROUND8_PROFILE 1
#undef DENSE_LOCALIDADJ_ROUND9_PROFILE
#define DENSE_LOCALIDADJ_ROUND9_PROFILE 1
#undef DENSE_TINYPIECE_ROUND14_PROFILE
#define DENSE_TINYPIECE_ROUND14_PROFILE 1
#endif

#if DENSE_SPQR_ROUND16_PROFILE
#undef DENSE_RECT_ROUND1_PROFILE
#define DENSE_RECT_ROUND1_PROFILE 1
#undef DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
#define DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE 1
#undef COMBDENSE_GATE_ROUND8_PROFILE
#define COMBDENSE_GATE_ROUND8_PROFILE 1
#undef DENSE_LOCALIDADJ_ROUND9_PROFILE
#define DENSE_LOCALIDADJ_ROUND9_PROFILE 1
#endif

#include <algorithm>
#include <climits>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <limits>
#include <map>
#include <queue>
#include <stack>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace dense_rect_round1_prof {
#if DENSE_RECT_ROUND1_PROFILE
using Clock = std::chrono::steady_clock;
struct Counters {
    long long split_calls = 0;
    long long split_dead_ns = 0;
    long long split_lid_adj_ns = 0;
    long long split_cheap_fan_ns = 0;
    long long split_no_bad_fast_ns = 0;
    long long split_tarjan_ns = 0;
    long long split_piece_ns = 0;
    long long split_boundary_ns = 0;

    long long apply_calls = 0;
    long long apply_dead_ns = 0;
    long long apply_small_ns = 0;
    long long apply_boundary_ns = 0;
    long long apply_oldbad_ns = 0;

    long long should_calls = 0;
    long long should_true = 0;
    long long should_false = 0;
    long long should_false_dead = 0;
    long long should_false_no_bad = 0;
    long long should_false_e = 0;
    long long should_false_v = 0;
    long long should_false_q = 0;
    long long should_false_dense = 0;
};
inline Counters& G() {
    static Counters c;
    return c;
}
inline long long nsSince(Clock::time_point t0) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - t0).count();
}
inline double ms(long long ns) {
    return (double)ns / 1e6;
}
inline void dump(std::ostream& os) {
    const auto& c = G();
    os << "[DENSE_RECT_PROF] split_calls=" << c.split_calls
       << " apply_calls=" << c.apply_calls
       << " should_calls=" << c.should_calls << "\n";
    os << "[DENSE_RECT_PROF split_ms] dead_edge_collect=" << ms(c.split_dead_ns)
       << " local_id_adj=" << ms(c.split_lid_adj_ns)
       << " cheap_fan=" << ms(c.split_cheap_fan_ns)
       << " no_bad_fast=" << ms(c.split_no_bad_fast_ns)
       << " tarjan_bcc=" << ms(c.split_tarjan_ns)
       << " piece_build=" << ms(c.split_piece_ns)
       << " boundary_materialize=" << ms(c.split_boundary_ns) << "\n";
    os << "[DENSE_RECT_PROF apply_ms] dead_remove=" << ms(c.apply_dead_ns)
       << " small_materialize=" << ms(c.apply_small_ns)
       << " boundary_reconnect=" << ms(c.apply_boundary_ns)
       << " oldBad_reclass=" << ms(c.apply_oldbad_ns) << "\n";
    os << "[DENSE_RECT_PROF shouldUseTrueSpqr] true=" << c.should_true
       << " false=" << c.should_false
       << " dead=" << c.should_false_dead
       << " no_bad=" << c.should_false_no_bad
       << " E_guard=" << c.should_false_e
       << " V_guard=" << c.should_false_v
       << " Q_guard=" << c.should_false_q
       << " dense_guard=" << c.should_false_dense << "\n";
}
#else
struct Counters {};
inline Counters& G() {
    static Counters c;
    return c;
}
inline void dump(std::ostream&) {}
#endif
} // namespace dense_rect_round1_prof

namespace dense_rect_round4_keep_prof {
#if DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
using Clock = std::chrono::steady_clock;
struct Counters {
    long long step3_calls = 0;
    long long keep_calls = 0;
    long long step3_total_ns = 0;
    long long keep_total_ns = 0;
    long long keep_mass_ns = 0;
    long long keep_tie_collect_ns = 0;
    long long keep_canon_ns = 0;
    long long keep_global_ns = 0;
    long long piece_post_ns = 0;

    long long single_bcc_calls = 0;
    long long unique_max_calls = 0;
    long long tie_calls = 0;
    long long tie_candidate_total = 0;
    long long tie_candidate_max = 0;
    long long sorted_edge_lists = 0;
    long long sorted_total_edges = 0;

    long long comb4096_step3_ns = 0;
    long long comb4096_keep_ns = 0;
    long long comb4096_canon_ns = 0;
};
inline Counters& G() {
    static Counters c;
    return c;
}
inline long long nsSince(Clock::time_point t0) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - t0).count();
}
inline double ms(long long ns) {
    return (double)ns / 1e6;
}
inline void dump(std::ostream& os) {
    const auto& c = G();
    double avgTie = c.tie_calls ? (double)c.tie_candidate_total / (double)c.tie_calls : 0.0;
    os << "[DENSE_RECT_KEEP_ORDER_ROUND4 calls] step3_calls=" << c.step3_calls
       << " keep_calls=" << c.keep_calls
       << " single_bcc=" << c.single_bcc_calls
       << " unique_max=" << c.unique_max_calls
       << " ties=" << c.tie_calls << "\n";
    os << "[DENSE_RECT_KEEP_ORDER_ROUND4 ms] step3_total=" << ms(c.step3_total_ns)
       << " keep_order=" << ms(c.keep_total_ns)
       << " mass_scan=" << ms(c.keep_mass_ns)
       << " tie_collect=" << ms(c.keep_tie_collect_ns)
       << " canonical_order=" << ms(c.keep_canon_ns)
       << " full_normalize=" << ms(c.keep_global_ns)
       << " piece_postpass=" << ms(c.piece_post_ns)
       << " comb4096_step3=" << ms(c.comb4096_step3_ns)
       << " comb4096_keep=" << ms(c.comb4096_keep_ns)
       << " comb4096_canon=" << ms(c.comb4096_canon_ns) << "\n";
    os << "[DENSE_RECT_KEEP_ORDER_ROUND4 counts] tie_avg_candidates=" << avgTie
       << " tie_max_candidates=" << c.tie_candidate_max
       << " sorted_lists=" << c.sorted_edge_lists
       << " sorted_total_edges=" << c.sorted_total_edges << "\n";
}
#else
struct Counters {};
inline Counters& G() {
    static Counters c;
    return c;
}
inline void dump(std::ostream&) {}
#endif
} // namespace dense_rect_round4_keep_prof

namespace sparse_round5_prof {
#if SPARSE_HARDSCALING_ROUND5_PROFILE
using Clock = std::chrono::steady_clock;
inline long long nsSince(Clock::time_point t0) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - t0).count();
}
inline double ms(long long ns) {
    return (double)ns / 1e6;
}
struct ScopeTimer {
    long long* dst = nullptr;
    Clock::time_point t0;
    explicit ScopeTimer(long long* p) : dst(p), t0(Clock::now()) {}
    ~ScopeTimer() { if (dst) *dst += nsSince(t0); }
};
struct Counters {
    long long eliminate_calls = 0;
    long long eliminate_total_ns = 0;
    long long split_spqr_calls = 0;
    long long split_spqr_total_ns = 0;
    long long local_rebuild_calls = 0;
    long long local_rebuild_total_ns = 0;

    long long bad_empty_calls = 0;
    long long bad_nonempty_calls = 0;

    long long attachcuts_total = 0;
    long long attachcuts_max = 0;
    long long attachcuts_zero = 0;
    long long attachcuts_one = 0;
    long long attachcuts_two_to_four = 0;
    long long attachcuts_five_plus = 0;

    long long oldboundary_total = 0;
    long long oldboundary_max = 0;
    long long oldboundary_zero = 0;
    long long oldboundary_nonzero = 0;
    long long oldboundary_one = 0;
    long long oldboundary_two_to_four = 0;
    long long oldboundary_five_plus = 0;

    long long cheapfan_total_ns = 0;
    long long cheapfan_direct_hits = 0;

    long long sparse_keep_total_ns = 0;
    long long branch5_calls = 0;
    long long branch5_total_ns = 0;
    long long branch5_scratch_ns = 0;
    long long branch5_dfs_ns = 0;
    long long branch5_keepok_true = 0;

    long long allow_stateful_true = 0;
    long long boundaryzero_false = 0;
    long long disabled_skip = 0;
    long long keybuild_calls = 0;
    long long keybuild_ns = 0;
    long long strict_eligible = 0;
    long long relaxed_candidate = 0;
    long long gate_pass = 0;
    long long shortcut_hits = 0;
    long long fallback_accepts = 0;

    long long localrebuild_on_sparse_path_ns = 0;
};
inline Counters& G() {
    static Counters c;
    return c;
}
inline void addAttachCutsSize(int sz) {
    auto& c = G();
    c.attachcuts_total += sz;
    c.attachcuts_max = std::max<long long>(c.attachcuts_max, sz);
    if (sz == 0) ++c.attachcuts_zero;
    else if (sz == 1) ++c.attachcuts_one;
    else if (sz <= 4) ++c.attachcuts_two_to_four;
    else ++c.attachcuts_five_plus;
}
inline void addOldBoundarySize(int sz) {
    auto& c = G();
    c.oldboundary_total += sz;
    c.oldboundary_max = std::max<long long>(c.oldboundary_max, sz);
    if (sz == 0) ++c.oldboundary_zero;
    else ++c.oldboundary_nonzero;
    if (sz == 1) ++c.oldboundary_one;
    else if (sz >= 2 && sz <= 4) ++c.oldboundary_two_to_four;
    else if (sz >= 5) ++c.oldboundary_five_plus;
}
inline void dump(std::ostream& os) {
    const auto& c = G();
    double avgAttach = c.local_rebuild_calls ? (double)c.attachcuts_total / (double)c.local_rebuild_calls : 0.0;
    double avgOldBoundary = c.local_rebuild_calls ? (double)c.oldboundary_total / (double)c.local_rebuild_calls : 0.0;
    os << "[SPARSE_R5 calls] eliminate=" << c.eliminate_calls
       << " split_spqr=" << c.split_spqr_calls
       << " local_rebuild=" << c.local_rebuild_calls
       << " bad_empty=" << c.bad_empty_calls
       << " bad_nonempty=" << c.bad_nonempty_calls << "\n";
    os << "[SPARSE_R5 ms] eliminate_total=" << ms(c.eliminate_total_ns)
       << " split_spqr_total=" << ms(c.split_spqr_total_ns)
       << " local_rebuild_total=" << ms(c.local_rebuild_total_ns)
       << " sparse_keep_total=" << ms(c.sparse_keep_total_ns)
       << " cheapfan_total=" << ms(c.cheapfan_total_ns)
       << " branch5_total=" << ms(c.branch5_total_ns)
       << " branch5_scratch=" << ms(c.branch5_scratch_ns)
       << " branch5_dfs=" << ms(c.branch5_dfs_ns)
       << " keybuild_ms=" << ms(c.keybuild_ns)
       << " sparse_path_localrebuild=" << ms(c.localrebuild_on_sparse_path_ns) << "\n";
    os << "[SPARSE_R5 counts] cheapfan_direct_hits=" << c.cheapfan_direct_hits
       << " branch5_calls=" << c.branch5_calls
       << " branch5_keepok_true=" << c.branch5_keepok_true
       << " allow_stateful_true=" << c.allow_stateful_true
       << " boundaryZero_false=" << c.boundaryzero_false
       << " disabled_skip=" << c.disabled_skip
       << " keybuild_calls=" << c.keybuild_calls
       << " strict_eligible=" << c.strict_eligible
       << " relaxed_candidate=" << c.relaxed_candidate
       << " gate_pass=" << c.gate_pass
       << " shortcut_hits=" << c.shortcut_hits
       << " fallback_accepts=" << c.fallback_accepts << "\n";
    os << "[SPARSE_R5 attachCuts] avg=" << avgAttach
       << " max=" << c.attachcuts_max
       << " zero=" << c.attachcuts_zero
       << " one=" << c.attachcuts_one
       << " two_to_four=" << c.attachcuts_two_to_four
       << " five_plus=" << c.attachcuts_five_plus << "\n";
    os << "[SPARSE_R5 oldBoundary] avg=" << avgOldBoundary
       << " max=" << c.oldboundary_max
       << " zero=" << c.oldboundary_zero
       << " nonzero=" << c.oldboundary_nonzero
       << " one=" << c.oldboundary_one
       << " two_to_four=" << c.oldboundary_two_to_four
       << " five_plus=" << c.oldboundary_five_plus << "\n";
}
#else
struct ScopeTimer {
    explicit ScopeTimer(long long*) {}
};
struct Counters {};
inline Counters& G() {
    static Counters c;
    return c;
}
inline void addAttachCutsSize(int) {}
inline void addOldBoundarySize(int) {}
inline void dump(std::ostream&) {}
#endif
} // namespace sparse_round5_prof

namespace cheapfan_round6_prof {
#if CHEAPFAN_ROUND6_PROFILE
using Clock = std::chrono::steady_clock;
inline long long nsSince(Clock::time_point t0) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - t0).count();
}
inline double ms(long long ns) {
    return (double)ns / 1e6;
}
struct ScopeTimer {
    long long* dst = nullptr;
    Clock::time_point t0;
    explicit ScopeTimer(long long* p) : dst(p), t0(Clock::now()) {}
    ~ScopeTimer() { if (dst) *dst += nsSince(t0); }
};
struct Counters {
    long long pre_candidate_calls = 0;
    long long pre_direct_hits = 0;
    long long pre_total_ns = 0;
    long long pre_candidate_gate_ns = 0;
    long long pre_edge_scan_ns = 0;
    long long pre_degree_summary_ns = 0;
    long long pre_oldboundary_count_ns = 0;
    long long pre_oldboundary_vec_ns = 0;
    long long pre_pattern_ns = 0;
    long long pre_boundary_emit_ns = 0;
    long long pre_finalize_ns = 0;

    long long post_candidate_calls = 0;
    long long post_direct_hits = 0;
    long long post_total_ns = 0;
    long long post_candidate_gate_ns = 0;
    long long post_edge_scan_ns = 0;
    long long post_degree_summary_ns = 0;
    long long post_oldboundary_count_ns = 0;
    long long post_oldboundary_vec_ns = 0;
    long long post_pattern_ns = 0;
    long long post_boundary_emit_ns = 0;
    long long post_finalize_ns = 0;

    long long fastkeep_total_ns = 0;
    long long fastkeep_dead_edge_ns = 0;
    long long fastkeep_dead_vertex_ns = 0;
    long long fastkeep_boundary_loop_ns = 0;
    long long fastkeep_state_bump_ns = 0;

    long long attachcuts_total = 0;
    long long attachcuts_max = 0;
    long long attachcuts_zero = 0;
    long long attachcuts_one = 0;
    long long attachcuts_two_to_four = 0;
    long long attachcuts_five_plus = 0;

    long long directhit_attach_total = 0;
    long long directhit_boundary_emitted_total = 0;
    long long directhit_all_oldcut_calls = 0;
};
inline Counters& G() {
    static Counters c;
    return c;
}
inline void addAttachCutsSize(int sz) {
    auto& c = G();
    c.attachcuts_total += sz;
    c.attachcuts_max = std::max<long long>(c.attachcuts_max, sz);
    if (sz == 0) ++c.attachcuts_zero;
    else if (sz == 1) ++c.attachcuts_one;
    else if (sz <= 4) ++c.attachcuts_two_to_four;
    else ++c.attachcuts_five_plus;
}
inline void addDirectHit(int attachSz, int boundaryEmitted, bool allOldCut) {
    auto& c = G();
    c.directhit_attach_total += attachSz;
    c.directhit_boundary_emitted_total += boundaryEmitted;
    if (allOldCut) ++c.directhit_all_oldcut_calls;
}
inline void dump(std::ostream& os) {
    const auto& c = G();
    long long candidateCalls = c.pre_candidate_calls + c.post_candidate_calls;
    long long directHits = c.pre_direct_hits + c.post_direct_hits;
    double avgAttach = candidateCalls ? (double)c.attachcuts_total / (double)candidateCalls : 0.0;
    double avgDirectAttach = directHits ? (double)c.directhit_attach_total / (double)directHits : 0.0;
    double avgDirectBoundaryEmitted = directHits ? (double)c.directhit_boundary_emitted_total / (double)directHits : 0.0;
    os << "[CHEAPFAN_R6 calls] pre_candidate=" << c.pre_candidate_calls
       << " pre_direct_hits=" << c.pre_direct_hits
       << " post_candidate=" << c.post_candidate_calls
       << " post_direct_hits=" << c.post_direct_hits << "\n";
    os << "[CHEAPFAN_R6 ms_pre] total=" << ms(c.pre_total_ns)
       << " candidate_gate=" << ms(c.pre_candidate_gate_ns)
       << " edge_scan=" << ms(c.pre_edge_scan_ns)
       << " deg_summary=" << ms(c.pre_degree_summary_ns)
       << " oldboundary_count=" << ms(c.pre_oldboundary_count_ns)
       << " oldboundary_vec=" << ms(c.pre_oldboundary_vec_ns)
       << " pattern_check=" << ms(c.pre_pattern_ns)
       << " boundary_emit=" << ms(c.pre_boundary_emit_ns)
       << " finalize=" << ms(c.pre_finalize_ns) << "\n";
    os << "[CHEAPFAN_R6 ms_post] total=" << ms(c.post_total_ns)
       << " candidate_gate=" << ms(c.post_candidate_gate_ns)
       << " edge_scan=" << ms(c.post_edge_scan_ns)
       << " deg_summary=" << ms(c.post_degree_summary_ns)
       << " oldboundary_count=" << ms(c.post_oldboundary_count_ns)
       << " oldboundary_vec=" << ms(c.post_oldboundary_vec_ns)
       << " pattern_check=" << ms(c.post_pattern_ns)
       << " boundary_emit=" << ms(c.post_boundary_emit_ns)
       << " finalize=" << ms(c.post_finalize_ns) << "\n";
    os << "[CHEAPFAN_R6 ms_fastkeep] total=" << ms(c.fastkeep_total_ns)
       << " dead_edge_remove=" << ms(c.fastkeep_dead_edge_ns)
       << " dead_exclusive_vertex_remove=" << ms(c.fastkeep_dead_vertex_ns)
       << " boundary_loop=" << ms(c.fastkeep_boundary_loop_ns)
       << " state_bump=" << ms(c.fastkeep_state_bump_ns) << "\n";
    os << "[CHEAPFAN_R6 attachCuts] avg=" << avgAttach
       << " max=" << c.attachcuts_max
       << " zero=" << c.attachcuts_zero
       << " one=" << c.attachcuts_one
       << " two_to_four=" << c.attachcuts_two_to_four
       << " five_plus=" << c.attachcuts_five_plus << "\n";
    os << "[CHEAPFAN_R6 direct_hit] direct_hits_total=" << directHits
       << " attach_avg=" << avgDirectAttach
       << " boundary_emitted_avg=" << avgDirectBoundaryEmitted
       << " all_oldcut_calls=" << c.directhit_all_oldcut_calls
       << " boundary_emitted_total=" << c.directhit_boundary_emitted_total << "\n";
}
#else
struct ScopeTimer {
    explicit ScopeTimer(long long*) {}
};
struct Counters {};
inline Counters& G() {
    static Counters c;
    return c;
}
inline void addAttachCutsSize(int) {}
inline void addDirectHit(int, int, bool) {}
inline void dump(std::ostream&) {}
#endif
} // namespace cheapfan_round6_prof


using namespace std;

[[noreturn]] inline void failCheck(const char* msg) {
    throw std::runtime_error(msg);
}

inline void chk(bool cond, const char* msg) {
    if (!cond) failCheck(msg);
}

template <class T>
inline std::vector<T> normVec(std::vector<T> v) {
    std::sort(v.begin(), v.end());
    v.erase(std::unique(v.begin(), v.end()), v.end());
    return v;
}

struct InputQuery {
    int u = 0;
    int v = 0;
    int w = 0;
};

class Solver {
public:
    enum BCNodeType : uint8_t { BCN_CUT, BCN_BLOCK, BCN_TRIVIAL };
    enum RawSpqrMode : uint8_t { RSB_ONE_NODE, RSB_TRUE_SPQR };
    enum AtomKind : uint8_t { AT_REAL_EDGE, AT_EXIT_BAG };
    enum QueryKind : uint8_t { Q_NOOP, Q_UNARY, Q_BRANCH };

    struct RealEdge {
        int u = 0;
        int v = 0;
        int handleId = -1;
    };

    struct Handle {
        bool watched = false;
    };

    struct Query {
        int u = 0;
        int v = 0;
        int w = 0;
        QueryKind kind = Q_NOOP;
        bool active = true;
        bool badNow = false;
        int e0 = -1;
        int e1 = -1;
    };

    struct OrigVertex {
        bool alive = true;
        int anchorBC = -1;
        int cutBC = -1;
        int ownerBlock = -1;
    };

    struct BCNode {
        BCNodeType type = BCN_TRIVIAL;
        bool alive = false;
        int origVertex = -1;
        int coreId = -1;
        std::vector<int> adj;
    };

    struct BlockCore {
        bool alive = false;
        int bcNode = -1;
        std::vector<int> allVertices;
        std::vector<int> realEdges;
        std::vector<int> watchedHandles;
        std::vector<int> attachCuts;
        std::vector<int> badQueries;
    };

    struct SparsePiece {
        int matPieceId = -1;
        std::vector<int> exclusiveVertices;
        std::vector<int> edges;
        std::vector<int> watchedHandles;
    };

    struct SparseBoundary {
        int vertex = -1;
        bool touchesKeep = false;
        std::vector<int> smallIds;
        bool existedOldCut = false;
    };

    struct SparsePatch {
        int oldCoreId = -1;
        int deletedVertex = -1;
        bool keepExists = false;
        int keepMatPiece = -1;
        std::vector<SparsePiece> small;
        std::vector<SparseBoundary> boundary;
        std::vector<int> isolatedExclusive;
        std::vector<int> deadEdges;
        std::vector<int> deadHandles;
        std::vector<int> deadExclusiveVertices;
        bool cheapfanPreAdjDirectReturn = false;
    };

    struct SideBagSummary {
        int realEdgeCount = 0;
        int watchedHandleCount = 0;
        int minRealEdge = std::numeric_limits<int>::max();
        bool hasOldBoundary = false;
    };

    struct NodeLocalSummary {
        int realEdgeCount = 0;
        int watchedHandleCount = 0;
        int minRealEdge = std::numeric_limits<int>::max();
        bool hasOldBoundary = false;
    };

    struct SpqrSkelEdge {
        int a = -1;
        int b = -1;
        int realEdgeId = -1; // >=0 if real, -1 if virtual
        int dirId = -1;      // >=0 if virtual, -1 if real
    };

    struct SpqrDir {
        int fromNode = -1;
        int toNode = -1;
        int treeEdgeId = -1;
        int fromVirtSlot = -1;
        int termA = -1;
        int termB = -1;
    };

    struct SpqrTreeEdge {
        int aNode = -1;
        int bNode = -1;
        int dirAB = -1;
        int dirBA = -1;
    };

    struct SpqrNode {
        char type = '?';
        std::vector<SpqrSkelEdge> skel;
        std::vector<int> treeAdj;
        std::vector<int> verts;
        std::vector<int> ownedRealEdges;
        NodeLocalSummary local;
    };

    struct ExpandedBag {
        std::vector<int> realEdges;
        std::vector<int> watchedHandles;
        std::vector<int> allVertices;
        std::vector<int> oldBoundaryVerts;
    };

    struct BlockSpqr {
        bool alive = false;
        int rootNode = -1;
        std::vector<SpqrNode> node;
        std::vector<SpqrTreeEdge> tree;
        std::vector<SpqrDir> dir;
        std::vector<SideBagSummary> bag;
        std::unordered_map<int, std::vector<int>> occNodeOfVertex;
    };

    struct ExitRef {
        int dirId = -1;
        int termA = -1;
        int termB = -1;
    };

    struct AffectedRegion {
        std::vector<int> nodes;
        std::vector<char> inRegion;
        std::vector<ExitRef> exits;
        std::vector<int> ownedRealEdges;
    };

    struct FragAtom {
        AtomKind kind = AT_REAL_EDGE;
        int ref = -1; // realEdgeId or exit index
    };

    struct FragBuild {
        std::vector<FragAtom> atoms;
        std::vector<int> edges;
        std::vector<int> handles;
        std::vector<int> allVertices;
        std::vector<int> visibleVerts;
        std::vector<int> boundaryMembers;
        long long mass = 0;
        int keyMinRealEdge = std::numeric_limits<int>::max();
    };

    struct BoundaryAcc {
        bool existedOldCut = false;
        std::vector<int> fragIds;
    };

    struct DenseStructuredGateInfo {
        bool boundaryZero = false;
        bool ccOne = false;
        bool currentOnComb = false;
        int attachCutsSize = 0;
        int badQueriesSize = 0;
        int boundarySize = 0;
        int oldBoundarySize = 0;
        int currentV = 0;
        int currentE = 0;
        int xNbrCount = 0;
        bool eOverCurrentThreshold = false;
        bool qOverCurrentThreshold = false;
    };

    struct SplitCtx {
        int oldCore = -1;
        int x = -1;
        std::vector<int> deadEdges;
        std::unordered_map<int, BoundaryAcc> boundary;
        std::vector<FragBuild> frags;
        int keepFrag = -1;
        std::vector<int> isolatedExclusive;
    };

    struct CompEdge {
        int a = -1;
        int b = -1;
        AtomKind kind = AT_REAL_EDGE;
        int ref = -1;
    };

    struct CompGraph {
        std::vector<int> realOf;
        std::unordered_map<int, int> lid;
        std::vector<CompEdge> item;
        std::vector<std::vector<std::pair<int, int>>> adj;
        std::vector<int> deg;
    };

    struct RawSkelEdge {
        int a = -1;
        int b = -1;
        int realEdgeId = -1;
        int peerNode = -1;
        int peerSlot = -1;
    };

    struct RawSpqrNode {
        char type = '?';
        std::vector<RawSkelEdge> skel;
    };

    struct RawSpqrBuild {
        std::vector<RawSpqrNode> node;
    };

#ifdef DEBUG_SOLVER
    struct CanonSparseSemantic {
        std::vector<int> keepEdges;
        std::vector<std::vector<int>> pieceEdges;
        std::map<int, std::pair<bool, std::vector<std::vector<int>>>> boundarySig;
        std::vector<int> isolatedExclusive;

        bool operator==(const CanonSparseSemantic& o) const {
            return keepEdges == o.keepEdges &&
                   pieceEdges == o.pieceEdges &&
                   boundarySig == o.boundarySig &&
                   isolatedExclusive == o.isolatedExclusive;
        }
    };
#endif

    // Bring-up state fields are public in this scaffold so tests can synthesize blocks directly.
    int N = 0;
    int M = 0;
    std::vector<Query> queries;
    std::vector<std::vector<int>> ownerQueries;
    std::vector<int> indeg;
    std::vector<int> badCount;
    std::vector<int> parentAns;
    std::vector<int> bcRootId;
    std::vector<std::vector<int>> rootMembers;
    std::vector<int> relabelSeen;
    int relabelTag = 1;
    std::unordered_map<int,int> compUp;
    std::priority_queue<int, std::vector<int>, std::greater<int>> ready;
    RawSpqrMode rawSpqrMode = RSB_TRUE_SPQR;
    std::vector<InputQuery> inputQueries;
    std::vector<RealEdge> edges;
    std::vector<Handle> handles;
    std::vector<OrigVertex> orig;
    std::vector<BCNode> bcNodes;
    std::vector<BlockCore> blocks;
    std::vector<int> edgeOwnerCore;
    std::vector<int> edgePosInCore;
    std::vector<int> handleOwnerCore;
    std::vector<BlockSpqr> blockSpqr;

    // Development scaffold entry points
    Solver() = default;
    Solver(int n, const std::vector<InputQuery>& qs);
    void clearAll();
    void buildFromQueries();
    void eliminateOne(int x);
    std::vector<int> solve();

    // End-to-end dynamic state helpers
    int allocBCNode(BCNodeType type, int origVertex, int coreId);
    int allocBlockCore();
    int makeTrivialNode(int v);
    int makeCutNode(int v);
    void linkBC(int a, int b);
    void cutBCEdge(int a, int b);
    void eraseOnce(std::vector<int>& vec, int x);
    void addVertexToCore(int core, int v);
    void removeVertexFromCore(int core, int v);
    void addEdgeToCore(int core, int e);
    void removeEdgeFromCore(int core, int e);
    void addAttachCut(int core, int cutId);
    void removeAttachCut(int core, int cutId);
    void normalizeCutNode(int cutId, std::vector<int>& rootSeeds);
    std::vector<int> collectDistinctRoots(const std::vector<int>& seeds);
    int findRootBC(int start) const;
    void assignComponentRootsInitial();
    void refreshRootsFromSeeds(int oldRoot, int parentLabel, const std::vector<int>& seeds);
    void pushReadyIf(int v);
    int popReady();
    void applyPatchToCore(int oldCore, const SparsePatch& P, std::vector<int>& rootSeeds);

    // Local oracle
    SparsePatch splitBlockLocalRebuild(int oldCore, int x) const;

    // SPQR_DIRECT bring-up layer
    static SideBagSummary emptyBagSummary();
    static SideBagSummary mergeBagSummary(const SideBagSummary& A,
                                          const SideBagSummary& B);

    int otherNode(const BlockSpqr& T, int teid, int u) const;
    int dirFromNode(const BlockSpqr& T, int teid, int from) const;

    RawSpqrBuild rawSpqrBuildOneNode(int core) const;
    RawSpqrBuild rawSpqrBuildTrueSpqrSkeleton(int core) const;
    RawSpqrBuild rawSpqrBuildFromCurrentCore(int core) const;

    void rebuildBlockSpqrFull(int core);
    void rebuildSideBagSummaries(int core, BlockSpqr& T);
    void rebuildAllAliveBlockSpqr();

    void collectSideBagExpanded(int core, int dirId, ExpandedBag& out) const;

    void collectOldBoundary(const BlockCore& B, int x, SplitCtx& C) const;
    void collectDeadEdgesIncidentToX(const BlockCore& B, int x, SplitCtx& C) const;

    AffectedRegion exposeAffectedRegion(const BlockSpqr& T, int x) const;
    CompGraph buildCompressedGraph(const AffectedRegion& A, int x) const;
    std::vector<std::vector<int>> runCompressedBCC(const CompGraph& G) const;

    void enumerateFragmentsAfterDelete(const BlockSpqr& T,
                                       const AffectedRegion& A,
                                       const BlockCore& B,
                                       SplitCtx& C) const;

    void chooseKeepFragment(SplitCtx& C) const;
    SparsePatch materializeSparsePatch(const BlockCore& B,
                                       const BlockSpqr& T,
                                       const AffectedRegion& A,
                                       const SplitCtx& C) const;

    bool shouldUseTrueSpqr(int oldCore) const;
    bool denseStructuredTrueSpqrEscapeEligible(int oldCore, int x, DenseStructuredGateInfo* info = nullptr) const;
    SparsePatch splitBlockSPQRForced(int oldCore, int x) const;
    SparsePatch splitBlockSPQR(int oldCore, int x) const;

#ifdef DEBUG_SOLVER
    void dumpCanonSparseSemantic(const char* tag,
                                 const CanonSparseSemantic& C) const;
    CanonSparseSemantic canonSparseSemantic(int oldCore,
                                            const SparsePatch& P) const;
    void checkSpqrPatchSemanticEqLocal(int oldCore, int x) const;
    void checkSpqrEqLocalForVertex(int x) const;
    void checkRawSpqrBuildAgainstCore(int core,
                                      const RawSpqrBuild& raw) const;
#endif

};

namespace cheapfan_cert_round7 {
using Clock = std::chrono::steady_clock;
inline long long nsSince(Clock::time_point t0) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - t0).count();
}
inline double ms(long long ns) {
    return (double)ns / 1e6;
}
struct ScopeTimer {
    long long* dst = nullptr;
    Clock::time_point t0;
    explicit ScopeTimer(long long* p) : dst(p), t0(Clock::now()) {}
    ~ScopeTimer() { if (dst) *dst += nsSince(t0); }
};
struct Fenwick {
    int n = 0;
    std::vector<int> bit;
    void init(int n_) {
        n = std::max(2, n_ + 2);
        bit.assign(n + 1, 0);
    }
    void add(int idx, int delta) {
        if (idx <= 0) return;
        if (idx > n) idx = n;
        for (int i = idx; i <= n; i += i & -i) bit[i] += delta;
    }
    int sumPrefix(int idx) const {
        if (idx <= 0) return 0;
        if (idx > n) idx = n;
        int r = 0;
        for (int i = idx; i > 0; i -= i & -i) r += bit[i];
        return r;
    }
    int rangeSum(int l, int r) const {
        if (r < l) return 0;
        return sumPrefix(r) - sumPrefix(l - 1);
    }
};
struct EvalSummary {
    bool decision = false;
    int nonIsoAfter = 0;
    int oldBoundaryAfter = 0;
    int xDeg = 0;
    int deg1After = 0;
    int deg2After = 0;
    int deg3After = 0;
    int hubCountAfter = 0;
    int otherAfter = 0;
    int aboveTargetAfter = 0;
};
struct Counters {
    long long pre_candidate_calls = 0;
    long long pre_direct_hits = 0;
    long long pre_total_ns = 0;
    long long pre_candidate_gate_ns = 0;
    long long pre_fullscan_ns = 0;
    long long pre_oldboundary_count_ns = 0;
    long long pre_degree_summary_ns = 0;
    long long pre_pattern_ns = 0;
    long long pre_finalize_ns = 0;

    long long cert_eligible_calls = 0;
    long long cert_build_calls = 0;
    long long cert_build_ns = 0;
    long long cert_build_fullscan_ns = 0;
    long long cert_hit_calls = 0;
    long long cert_hit_ns = 0;
    long long cert_miss_calls = 0;
    long long miss_no_state = 0;
    long long miss_core_ptr_or_id_mismatch = 0;
    long long miss_epoch_mismatch = 0;
    long long miss_boundary_changed = 0;
    long long miss_not_keep_only_previous = 0;
    long long miss_invariant_unsure = 0;
    long long miss_forced_fallback = 0;

    long long cert_update_calls = 0;
    long long cert_update_ns = 0;
    long long cert_update_deadedge_ns = 0;
    long long cert_update_neighbor_ns = 0;
    long long cert_update_oldboundary_ns = 0;
    long long cert_update_bump_ns = 0;

    long long streak_count = 0;
    long long streak_total_len = 0;
    long long streak_max = 0;

    long long sampled_full_recompute_calls = 0;
    long long decision_mismatch = 0;
    long long summary_mismatch = 0;
};
inline Counters& G() {
    static Counters c;
    return c;
}
struct CertState {
    bool valid = false;
    bool pending = false;
    bool pendingFromDirectHit = false;
    int coreId = -1;
    int expectedEpoch = 0;
    bool lastKeepOnlyDirectHit = false;
    bool boundaryUnchanged = false;
    int nonIso = 0;
    int edgeCount = 0;
    int oldBoundaryCount = 0;
    int currentStreakLen = 0;
    int streakCoreId = -1;

    std::vector<int> deg;
    std::vector<unsigned char> attachFlag;
    std::vector<unsigned char> activeEdge;
    std::vector<std::vector<int>> incEdges;
    std::vector<int> hist;
    Fenwick fw;
};
inline CertState& ST() {
    static CertState st;
    return st;
}
inline void finalizeStreak(CertState& st) {
    if (st.currentStreakLen > 0) {
        G().streak_count++;
        G().streak_total_len += st.currentStreakLen;
        G().streak_max = std::max<long long>(G().streak_max, st.currentStreakLen);
        st.currentStreakLen = 0;
        st.streakCoreId = -1;
    }
}
inline void reset() {
    auto& st = ST();
    finalizeStreak(st);
    st = CertState{};
    G() = Counters{};
}
inline void invalidateCore(int core) {
    auto& st = ST();
    if ((st.valid || st.pending) && st.coreId == core) {
        finalizeStreak(st);
        st = CertState{};
    }
}
inline bool hasPendingForCore(int core) {
    auto& st = ST();
    return st.valid && st.pending && st.coreId == core;
}
inline int histAt(const CertState& st, int d) {
    if (d <= 0 || d >= (int)st.hist.size()) return 0;
    return st.hist[d];
}
inline void histAdd(CertState& st, int d, int delta) {
    if (d <= 0) return;
    if (d >= (int)st.hist.size()) return;
    st.hist[d] += delta;
    st.fw.add(d, delta);
}
inline void ensureSized(CertState& st, int origN, int edgeN) {
    st.deg.assign(origN, 0);
    st.attachFlag.assign(origN, 0);
    st.activeEdge.assign(edgeN, 0);
    st.incEdges.assign(origN, {});
    st.hist.assign(origN + 5, 0);
    st.fw.init(origN + 5);
    st.nonIso = 0;
    st.edgeCount = 0;
    st.oldBoundaryCount = 0;
}
inline void primePendingFromSurviving(
    int core,
    int expectedEpoch,
    const std::vector<int>& attachCuts,
    const std::vector<int>& survEdges,
    const std::vector<int>& touched,
    const std::vector<int>& fanDeg,
    int oldBoundaryCount,
    const std::vector<Solver::RealEdge>& edges,
    const std::vector<Solver::BCNode>& bcNodes,
    const std::vector<Solver::OrigVertex>& orig) {
    auto& st = ST();
    if (st.valid && st.coreId != core) finalizeStreak(st);
    ensureSized(st, (int)orig.size(), (int)edges.size());
    st.valid = true;
    st.pending = true;
    st.pendingFromDirectHit = true;
    st.coreId = core;
    st.expectedEpoch = expectedEpoch;
    st.lastKeepOnlyDirectHit = true;
    st.boundaryUnchanged = true;
    st.nonIso = (int)touched.size();
    st.oldBoundaryCount = oldBoundaryCount;
    for (int cutBC : attachCuts) {
        int v = bcNodes[cutBC].origVertex;
        if (0 <= v && v < (int)orig.size() && orig[v].alive) st.attachFlag[v] = 1;
    }
    for (int v : touched) {
        int d = fanDeg[v];
        st.deg[v] = d;
        histAdd(st, d, +1);
    }
    st.edgeCount = (int)survEdges.size();
    for (int e : survEdges) {
        if (!(0 <= e && e < (int)edges.size())) continue;
        st.activeEdge[e] = 1;
        int a = edges[e].u, b = edges[e].v;
        if (0 <= a && a < (int)orig.size()) st.incEdges[a].push_back(e);
        if (0 <= b && b < (int)orig.size()) st.incEdges[b].push_back(e);
    }
}
inline bool computeSummaryFromState(
    const CertState& st,
    int x,
    const std::vector<int>& deadEdges,
    const std::vector<Solver::RealEdge>& edges,
    EvalSummary& out) {
    if (!(0 <= x && x < (int)st.deg.size())) return false;
    int xdeg = st.deg[x];
    if (xdeg <= 0) return false;
    out = EvalSummary{};
    out.xDeg = xdeg;
    std::unordered_map<int,int> neighDegCount;
    neighDegCount.reserve((size_t)xdeg * 2 + 4);
    int activeIncCount = 0;
    int lostNonIso = 0;
    int lostOldBoundary = 0;
    std::vector<int> activeInc;
    activeInc.reserve(st.incEdges[x].size());
    for (int e : st.incEdges[x]) {
        if (!(0 <= e && e < (int)st.activeEdge.size()) || !st.activeEdge[e]) continue;
        ++activeIncCount;
        activeInc.push_back(e);
        int a = edges[e].u, b = edges[e].v;
        int v = (a == x ? b : a);
        if (!(0 <= v && v < (int)st.deg.size())) return false;
        int d = st.deg[v];
        neighDegCount[d]++;
        if (d == 1) {
            ++lostNonIso;
            if (st.attachFlag[v]) ++lostOldBoundary;
        }
    }
    if ((int)deadEdges.size() != activeIncCount) return false;
    {
        std::vector<int> lhs = activeInc;
        std::vector<int> rhs = deadEdges;
        std::sort(lhs.begin(), lhs.end());
        std::sort(rhs.begin(), rhs.end());
        if (lhs != rhs) return false;
    }
    out.nonIsoAfter = st.nonIso - 1 - lostNonIso;
    out.oldBoundaryAfter = st.oldBoundaryCount - (st.attachFlag[x] ? 1 : 0) - lostOldBoundary;
    if (out.nonIsoAfter <= 0) return false;
    auto cntN = [&](int d) -> int {
        auto it = neighDegCount.find(d);
        return it == neighDegCount.end() ? 0 : it->second;
    };
    auto histAfterAt = [&](int d) -> int {
        if (d <= 0) return 0;
        return histAt(st, d) - (xdeg == d ? 1 : 0) - cntN(d) + cntN(d + 1);
    };
    out.deg1After = histAfterAt(1);
    out.deg2After = histAfterAt(2);
    out.deg3After = histAfterAt(3);
    int hubTarget = out.nonIsoAfter - 1;
    out.hubCountAfter = histAfterAt(hubTarget);
    int currentAbove = st.fw.rangeSum(hubTarget + 1, (int)st.hist.size() - 1);
    out.aboveTargetAfter = currentAbove - (xdeg > hubTarget ? 1 : 0) - cntN(hubTarget + 1);
    out.otherAfter = out.nonIsoAfter - out.deg1After - out.deg2After - out.deg3After - out.hubCountAfter;
    out.decision =
        (out.oldBoundaryAfter == out.nonIsoAfter - 2 &&
         out.hubCountAfter == 1 &&
         out.aboveTargetAfter == 0 &&
         out.deg1After == 0 &&
         out.deg2After == 2 &&
         out.deg3After == out.nonIsoAfter - 3 &&
         out.otherAfter == 0);
    return true;
}
struct FullScanSummary {
    bool decision = false;
    int nonIsoAfter = 0;
    int oldBoundaryAfter = 0;
    int deg1After = 0;
    int deg2After = 0;
    int deg3After = 0;
    int hubCountAfter = 0;
    int otherAfter = 0;
    int aboveTargetAfter = 0;
};
inline FullScanSummary recomputePreAdj(
    const Solver::BlockCore& B,
    int x,
    const std::vector<Solver::RealEdge>& edges,
    const std::vector<Solver::OrigVertex>& orig,
    const std::vector<Solver::BCNode>& bcNodes) {
    FullScanSummary r;
    std::vector<int> seen(orig.size(), 0), deg(orig.size(), 0);
    int stamp = 1;
    std::vector<int> touched;
    touched.reserve(B.allVertices.size());
    for (int e : B.realEdges) {
        int a = edges[e].u;
        int b = edges[e].v;
        if (a == x || b == x) continue;
        if (!orig[a].alive || !orig[b].alive) continue;
        if (seen[a] != stamp) { seen[a] = stamp; touched.push_back(a); }
        if (seen[b] != stamp) { seen[b] = stamp; touched.push_back(b); }
        deg[a]++;
        deg[b]++;
    }
    r.nonIsoAfter = (int)touched.size();
    if (r.nonIsoAfter <= 0) return r;
    std::vector<int> oldSeen(orig.size(), 0);
    int oldStamp = 1;
    for (int cutBC : B.attachCuts) {
        int v = bcNodes[cutBC].origVertex;
        if (v != x && 0 <= v && v < (int)orig.size() && orig[v].alive && seen[v] == stamp) {
            if (oldSeen[v] != oldStamp) {
                oldSeen[v] = oldStamp;
                ++r.oldBoundaryAfter;
            }
        }
    }
    int maxDeg = 0;
    int minDeg = INT_MAX;
    int hubTarget = r.nonIsoAfter - 1;
    for (int v : touched) {
        int d = deg[v];
        minDeg = std::min(minDeg, d);
        maxDeg = std::max(maxDeg, d);
        if (d == 1) ++r.deg1After;
        else if (d == 2) ++r.deg2After;
        else if (d == 3) ++r.deg3After;
        if (d == hubTarget) ++r.hubCountAfter;
        if (d > hubTarget) ++r.aboveTargetAfter;
    }
    r.otherAfter = r.nonIsoAfter - r.deg1After - r.deg2After - r.deg3After - r.hubCountAfter;
    r.decision =
        (minDeg >= 2 &&
         r.oldBoundaryAfter == r.nonIsoAfter - 2 &&
         r.hubCountAfter == 1 &&
         maxDeg == hubTarget &&
         r.deg2After == 2 &&
         r.deg3After == r.nonIsoAfter - 3 &&
         r.otherAfter == 0);
    return r;
}
inline bool sampledCrossCheckPass(
    const Solver::BlockCore& B,
    int x,
    const EvalSummary& hitSummary,
    const std::vector<Solver::RealEdge>& edges,
    const std::vector<Solver::OrigVertex>& orig,
    const std::vector<Solver::BCNode>& bcNodes) {
#if CHEAPFAN_CERT_ROUND7_PROFILE
    if ((G().cert_eligible_calls & 127LL) != 1LL) return true;
    ++G().sampled_full_recompute_calls;
    FullScanSummary full = recomputePreAdj(B, x, edges, orig, bcNodes);
    bool ok = true;
    if (full.decision != hitSummary.decision) {
        ++G().decision_mismatch;
        ok = false;
    }
    bool sameSummary =
        full.nonIsoAfter == hitSummary.nonIsoAfter &&
        full.oldBoundaryAfter == hitSummary.oldBoundaryAfter &&
        full.deg1After == hitSummary.deg1After &&
        full.deg2After == hitSummary.deg2After &&
        full.deg3After == hitSummary.deg3After &&
        full.hubCountAfter == hitSummary.hubCountAfter &&
        full.otherAfter == hitSummary.otherAfter &&
        full.aboveTargetAfter == hitSummary.aboveTargetAfter;
    if (!sameSummary) {
        ++G().summary_mismatch;
        ok = false;
    }
    return ok;
#else
    (void)B; (void)x; (void)hitSummary; (void)edges; (void)orig; (void)bcNodes;
    return true;
#endif
}
inline bool tryCertHit(
    int oldCore,
    int currentEpoch,
    const Solver::BlockCore& B,
    int x,
    const std::vector<int>& deadEdges,
    const std::vector<Solver::RealEdge>& edges,
    const std::vector<Solver::OrigVertex>& orig,
    const std::vector<Solver::BCNode>& bcNodes,
    EvalSummary& out) {
    auto& st = ST();
    if (!st.valid) { ++G().cert_miss_calls; ++G().miss_no_state; return false; }
    if (st.pending) { ++G().cert_miss_calls; ++G().miss_forced_fallback; return false; }
    if (st.coreId != oldCore) { ++G().cert_miss_calls; ++G().miss_core_ptr_or_id_mismatch; return false; }
    if (st.expectedEpoch != currentEpoch) { ++G().cert_miss_calls; ++G().miss_epoch_mismatch; return false; }
    if (!st.boundaryUnchanged) { ++G().cert_miss_calls; ++G().miss_boundary_changed; return false; }
    if (!st.lastKeepOnlyDirectHit) { ++G().cert_miss_calls; ++G().miss_not_keep_only_previous; return false; }
    ++G().cert_eligible_calls;
    ScopeTimer hitScope(&G().cert_hit_ns);
    if (!(0 <= x && x < (int)st.deg.size()) || st.deg[x] <= 0) {
        ++G().cert_miss_calls;
        ++G().miss_invariant_unsure;
        return false;
    }
    std::vector<int> activeInc;
    activeInc.reserve(st.incEdges[x].size());
    for (int e : st.incEdges[x]) {
        if (!(0 <= e && e < (int)st.activeEdge.size()) || !st.activeEdge[e]) continue;
        activeInc.push_back(e);
    }
    if ((int)activeInc.size() != (int)deadEdges.size()) {
        ++G().cert_miss_calls;
        ++G().miss_invariant_unsure;
        return false;
    }
    {
        std::vector<int> lhs = activeInc;
        std::vector<int> rhs = deadEdges;
        std::sort(lhs.begin(), lhs.end());
        std::sort(rhs.begin(), rhs.end());
        if (lhs != rhs) {
            ++G().cert_miss_calls;
            ++G().miss_invariant_unsure;
            return false;
        }
    }
#if CHEAPFAN_CERT_ROUND7_PROFILE
    out = EvalSummary{};
    if ((G().cert_eligible_calls & 127LL) == 1LL) {
        ++G().sampled_full_recompute_calls;
        FullScanSummary full = recomputePreAdj(B, x, edges, orig, bcNodes);
        if (!full.decision) {
            ++G().decision_mismatch;
            ++G().cert_miss_calls;
            ++G().miss_invariant_unsure;
            return false;
        }
    }
#endif
    ++G().cert_update_calls;
    auto __upd_all_t0 = Clock::now();
    std::vector<int> neighs;
    neighs.reserve(deadEdges.size());
    auto __upd_dead_t0 = Clock::now();
    for (int e : deadEdges) {
        st.activeEdge[e] = 0;
        --st.edgeCount;
        int a = edges[e].u, b = edges[e].v;
        neighs.push_back(a == x ? b : a);
    }
    G().cert_update_deadedge_ns += nsSince(__upd_dead_t0);
    auto __upd_nei_t0 = Clock::now();
    auto decVertex = [&](int v) {
        int d = st.deg[v];
        if (d <= 0) return;
        histAdd(st, d, -1);
        --st.deg[v];
        if (st.deg[v] > 0) histAdd(st, st.deg[v], +1);
        else {
            --st.nonIso;
            if (st.attachFlag[v]) --st.oldBoundaryCount;
        }
    };
    for (int v : neighs) decVertex(v);
    decVertex(x);
    G().cert_update_neighbor_ns += nsSince(__upd_nei_t0);
    auto __upd_old_t0 = Clock::now();
    G().cert_update_oldboundary_ns += nsSince(__upd_old_t0);
    auto __upd_bump_t0 = Clock::now();
    st.pending = true;
    st.pendingFromDirectHit = true;
    st.expectedEpoch = currentEpoch + 1;
    st.lastKeepOnlyDirectHit = true;
    st.boundaryUnchanged = true;
    G().cert_update_bump_ns += nsSince(__upd_bump_t0);
    G().cert_update_ns += nsSince(__upd_all_t0);
    ++G().cert_hit_calls;
    return true;
}
inline void commitPending(int core, int actualEpoch) {
    auto& st = ST();
    if (!(st.valid && st.pending && st.pendingFromDirectHit && st.coreId == core && st.expectedEpoch == actualEpoch)) {
        invalidateCore(core);
        return;
    }
    st.pending = false;
    st.pendingFromDirectHit = false;
    if (st.streakCoreId == core) ++st.currentStreakLen;
    else {
        finalizeStreak(st);
        st.streakCoreId = core;
        st.currentStreakLen = 1;
    }
}
inline void dump(std::ostream& os) {
#if CHEAPFAN_CERT_ROUND7_PROFILE
    auto st = ST();
    long long streakCount = G().streak_count + (st.currentStreakLen > 0 ? 1 : 0);
    long long streakTotal = G().streak_total_len + st.currentStreakLen;
    long long streakMax = std::max<long long>(G().streak_max, st.currentStreakLen);
    double avgStreak = streakCount ? (double)streakTotal / (double)streakCount : 0.0;
    double eligibleRatio = G().pre_candidate_calls ? (double)G().cert_eligible_calls / (double)G().pre_candidate_calls : 0.0;
    double hitRatio = G().cert_eligible_calls ? (double)G().cert_hit_calls / (double)G().cert_eligible_calls : 0.0;
    os << "[CHEAPFAN_CERT_R7 calls] pre_candidate=" << G().pre_candidate_calls
       << " pre_direct_hits=" << G().pre_direct_hits
       << " cert_eligible=" << G().cert_eligible_calls
       << " cert_builds=" << G().cert_build_calls
       << " cert_hits=" << G().cert_hit_calls
       << " cert_miss=" << G().cert_miss_calls
       << " fullscan_calls=" << G().cert_build_calls
       << " sampled_full_recompute=" << G().sampled_full_recompute_calls << "\n";
    os << "[CHEAPFAN_CERT_R7 ms] pre_total=" << ms(G().pre_total_ns)
       << " candidate_gate=" << ms(G().pre_candidate_gate_ns)
       << " fullscan=" << ms(G().pre_fullscan_ns)
       << " degree_summary=" << ms(G().pre_degree_summary_ns)
       << " oldboundary_count=" << ms(G().pre_oldboundary_count_ns)
       << " pattern_check=" << ms(G().pre_pattern_ns)
       << " finalize=" << ms(G().pre_finalize_ns)
       << " cert_build=" << ms(G().cert_build_ns)
       << " cert_build_fullscan=" << ms(G().cert_build_fullscan_ns)
       << " cert_hit=" << ms(G().cert_hit_ns)
       << " cert_update=" << ms(G().cert_update_ns)
       << " cert_update_deadEdges=" << ms(G().cert_update_deadedge_ns)
       << " cert_update_neighbors=" << ms(G().cert_update_neighbor_ns)
       << " cert_update_oldBoundary=" << ms(G().cert_update_oldboundary_ns)
       << " cert_update_state_bump=" << ms(G().cert_update_bump_ns) << "\n";
    os << "[CHEAPFAN_CERT_R7 ratios] eligible_ratio=" << eligibleRatio
       << " hit_ratio=" << hitRatio
       << " streak_count=" << streakCount
       << " streak_avg=" << avgStreak
       << " streak_max=" << streakMax << "\n";
    os << "[CHEAPFAN_CERT_R7 misses] no_state=" << G().miss_no_state
       << " core_ptr_or_id_mismatch=" << G().miss_core_ptr_or_id_mismatch
       << " epoch_mismatch=" << G().miss_epoch_mismatch
       << " boundary_changed=" << G().miss_boundary_changed
       << " not_keep_only_previous=" << G().miss_not_keep_only_previous
       << " invariant_unsure=" << G().miss_invariant_unsure
       << " forced_fallback=" << G().miss_forced_fallback << "\n";
    os << "[CHEAPFAN_CERT_R7 crosscheck] sampled_full_recompute=" << G().sampled_full_recompute_calls
       << " decision_mismatch=" << G().decision_mismatch
       << " summary_mismatch=" << G().summary_mismatch << "\n";
#else
    (void)os;
#endif
}
} // namespace cheapfan_cert_round7


namespace combdense_round8_prof {
#if COMBDENSE_GATE_ROUND8_PROFILE
using Clock = std::chrono::steady_clock;
inline long long nsSince(Clock::time_point t0) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - t0).count();
}
inline double ms(long long ns) {
    return (double)ns / 1e6;
}
struct Counters {
    long long split_spqr_calls = 0;
    long long localrebuild_fallback_calls = 0;
    long long fastkeep_calls = 0;
    long long fastkeep_total_ns = 0;
    long long fastkeep_dead_edge_ns = 0;
    long long fastkeep_dead_vertex_ns = 0;
    long long fastkeep_boundary_ns = 0;
    long long fastkeep_state_bump_ns = 0;
};
inline Counters& G() {
    static Counters c;
    return c;
}
inline void dump(std::ostream& os) {
    const auto& c = G();
    os << "[COMBDENSE_R8 counts] split_spqr_calls=" << c.split_spqr_calls
       << " localrebuild_fallback=" << c.localrebuild_fallback_calls
       << " fastkeep_calls=" << c.fastkeep_calls << "\n";
    os << "[COMBDENSE_R8 fastkeep_ms] total=" << ms(c.fastkeep_total_ns)
       << " dead_edge=" << ms(c.fastkeep_dead_edge_ns)
       << " dead_vertex=" << ms(c.fastkeep_dead_vertex_ns)
       << " boundary_loop=" << ms(c.fastkeep_boundary_ns)
       << " state_bump=" << ms(c.fastkeep_state_bump_ns) << "\n";
}
#else
struct Counters {};
inline Counters& G() { static Counters c; return c; }
inline void dump(std::ostream&) {}
#endif
} // namespace combdense_round8_prof

namespace dense_localidadj_round9_prof {
#if DENSE_LOCALIDADJ_ROUND9_PROFILE
using Clock = std::chrono::steady_clock;
inline long long nsSince(Clock::time_point t0){return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-t0).count();}
inline double ms(long long ns){return (double)ns/1e6;}
struct Counters{long long lid_mark_realof_ns=0,lid_deadedge_filter_ns=0,lid_degree_first_ns=0,lid_surv_arrays_ns=0,lid_adj_alloc_ns=0,lid_adj_fill_ns=0,lid_deg_fill_ns=0,lid_other_ns=0;};
inline Counters& G(){ static Counters c; return c; }
inline void dump(std::ostream& os){ const auto& c=G(); os << "[DENSE_LOCALIDADJ_R9 ms]" << " lid_mark_realof=" << ms(c.lid_mark_realof_ns) << " deadedge_filter=" << ms(c.lid_deadedge_filter_ns) << " degree_first=" << ms(c.lid_degree_first_ns) << " surv_arrays_build=" << ms(c.lid_surv_arrays_ns) << " adj_alloc=" << ms(c.lid_adj_alloc_ns) << " adj_fill=" << ms(c.lid_adj_fill_ns) << " deg_fill=" << ms(c.lid_deg_fill_ns) << " other=" << ms(c.lid_other_ns) << "\n"; }
#else
struct Counters{}; inline Counters& G(){ static Counters c; return c;} inline void dump(std::ostream&){}
#endif
} // namespace dense_localidadj_round9_prof


namespace dense_bccreuse_round12_prof {
#if DENSE_BCCREUSE_ROUND12_PROFILE
using Clock = std::chrono::steady_clock;
inline long long nsSince(Clock::time_point t0){return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-t0).count();}
inline double ms(long long ns){return (double)ns/1e6;}
struct ScopeTimer{long long* dst=nullptr; Clock::time_point t0; explicit ScopeTimer(long long* p):dst(p),t0(Clock::now()){} ~ScopeTimer(){ if(dst) *dst += nsSince(t0); }};
struct Counters {
    long long split_calls=0;
    long long step3_total_ns=0;
    long long full_step3_total_ns=0;
    long long partial_recompute_total_ns=0;
    long long partial_recompute_subgraph_build_ns=0;
    long long partial_recompute_step3_ns=0;
    long long reused_bucket_merge_ns=0;
    long long lazy_tie_canonical_ns=0;
    long long piece_feed_merge_ns=0;
    long long crosscheck_total_ns=0;

    long long prev_step3_cache_available=0;
    long long same_core_prev=0;
    long long prev_keep_only_chain=0;
    long long boundary_unchanged=0;
    long long cache_eligible=0;
    long long cache_hit=0;
    long long cache_miss=0;

    long long miss_no_prev=0;
    long long miss_core_changed=0;
    long long miss_prev_not_step3=0;
    long long miss_prev_not_keep_only=0;
    long long miss_boundary_changed=0;
    long long miss_x_touches_many_prev_bcc=0;
    long long miss_affected_ratio_large=0;
    long long miss_invariant_unsure=0;
    long long miss_forced_fallback=0;

    long long sum_prev_bcc_count=0;
    long long sum_x_touched_prev_bcc=0;
    long long sum_reused_bcc_count=0;
    long long sum_reused_edge_ratio_milli=0;
    long long sum_cached_keep_bcc_edge_count=0;
    long long same_keep_choice_count=0;
    long long observed_struct_cases=0;
};
inline Counters& G(){ static Counters c; return c; }
struct PendingStep3Info {
    bool active=false;
    int oldCore=-1;
    bool usedStep3=false;
    bool keepExists=false;
};
inline PendingStep3Info& Pending(){ static PendingStep3Info p; return p; }
struct CacheState {
    bool valid=false;
    int core=-1;
    bool fromStep3=false;
    bool keepOnly=false;
    bool boundaryUnchanged=false;
    int prevBccCount=0;
    int prevKeepEdgeCount=0;
};
inline CacheState& Cache(){ static CacheState c; return c; }
inline void reset(){ G()=Counters{}; Pending()=PendingStep3Info{}; Cache()=CacheState{}; }
inline void dump(std::ostream& os){
    const auto& c=G();
    double avgPrevBcc = c.observed_struct_cases? (double)c.sum_prev_bcc_count/c.observed_struct_cases : 0.0;
    double avgTouched = c.observed_struct_cases? (double)c.sum_x_touched_prev_bcc/c.observed_struct_cases : 0.0;
    double avgReusedBcc = c.observed_struct_cases? (double)c.sum_reused_bcc_count/c.observed_struct_cases : 0.0;
    double avgReusedEdgeRatio = c.observed_struct_cases? (double)c.sum_reused_edge_ratio_milli/(1000.0*c.observed_struct_cases) : 0.0;
    double avgCachedKeepEdge = c.observed_struct_cases? (double)c.sum_cached_keep_bcc_edge_count/c.observed_struct_cases : 0.0;
    os << "[DENSE_BCCREUSE_R12 calls] split_calls=" << c.split_calls
       << " prev_step3_cache_available=" << c.prev_step3_cache_available
       << " same_core_prev=" << c.same_core_prev
       << " prev_keep_only_chain=" << c.prev_keep_only_chain
       << " boundary_unchanged=" << c.boundary_unchanged
       << " cache_eligible=" << c.cache_eligible
       << " cache_hit=" << c.cache_hit
       << " cache_miss=" << c.cache_miss << "\n";
    os << "[DENSE_BCCREUSE_R12 miss] no_prev=" << c.miss_no_prev
       << " core_changed=" << c.miss_core_changed
       << " prev_not_step3=" << c.miss_prev_not_step3
       << " prev_not_keep_only=" << c.miss_prev_not_keep_only
       << " boundary_changed=" << c.miss_boundary_changed
       << " x_touches_many_prev_bcc=" << c.miss_x_touches_many_prev_bcc
       << " affected_ratio_large=" << c.miss_affected_ratio_large
       << " invariant_unsure=" << c.miss_invariant_unsure
       << " forced_fallback=" << c.miss_forced_fallback << "\n";
    os << "[DENSE_BCCREUSE_R12 structure] observed=" << c.observed_struct_cases
       << " avg_prev_bcc_count=" << avgPrevBcc
       << " avg_x_touched_prev_bcc=" << avgTouched
       << " avg_reused_bcc_count=" << avgReusedBcc
       << " avg_reused_edge_ratio=" << avgReusedEdgeRatio
       << " avg_cached_keep_bcc_edge_count=" << avgCachedKeepEdge
       << " keep_choice_same_count=" << c.same_keep_choice_count << "\n";
    os << "[DENSE_BCCREUSE_R12 ms] step3_total=" << ms(c.step3_total_ns)
       << " full_step3_total=" << ms(c.full_step3_total_ns)
       << " partial_recompute_total=" << ms(c.partial_recompute_total_ns)
       << " partial_recompute_subgraph_build=" << ms(c.partial_recompute_subgraph_build_ns)
       << " partial_recompute_step3=" << ms(c.partial_recompute_step3_ns)
       << " reused_bucket_merge=" << ms(c.reused_bucket_merge_ns)
       << " lazy_tie_canonical=" << ms(c.lazy_tie_canonical_ns)
       << " piece_feed_merge=" << ms(c.piece_feed_merge_ns)
       << " crosscheck_total=" << ms(c.crosscheck_total_ns) << "\n";
}
#else
struct Counters{}; inline Counters& G(){ static Counters c; return c; }
inline void reset(){}
inline void dump(std::ostream&){}
struct PendingStep3Info{}; inline PendingStep3Info& Pending(){ static PendingStep3Info p; return p; }
struct CacheState{}; inline CacheState& Cache(){ static CacheState c; return c; }
#endif

} // namespace dense_bccreuse_round12_prof

namespace dense_singlebcc_round13_prof {
#if DENSE_SINGLEBCC_ROUND13_PROFILE
using Clock = std::chrono::steady_clock;
inline long long nsSince(Clock::time_point t0){ return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-t0).count(); }
inline double ms(long long ns){ return (double)ns/1e6; }
struct ScopeTimer{ long long* dst=nullptr; Clock::time_point t0; explicit ScopeTimer(long long* p):dst(p),t0(Clock::now()){} ~ScopeTimer(){ if(dst) *dst += nsSince(t0); } };
struct Counters {
    long long step3_calls=0;
    long long prev_step3_available=0;
    long long prev_single_bcc=0;
    long long prev_keep_only_chain=0;
    long long boundary_unchanged=0;
    long long shortcut_eligible=0;
    long long shortcut_attempted=0;

    long long actual_new_calls=0;
    long long actual_new_single_bcc_count=0;
    long long actual_new_keep_is_all_count=0;
    long long sum_actual_bcc_count=0;
    long long sum_actual_piece_count=0;
    long long sum_actual_piece_total_edge_count=0;
    long long hist_actual_bcc_0=0, hist_actual_bcc_1=0, hist_actual_bcc_2=0, hist_actual_bcc_3_4=0, hist_actual_bcc_5p=0;
    long long hist_actual_piece_0=0, hist_actual_piece_1=0, hist_actual_piece_2p=0;

    long long singlebcc_cert_total_ns=0;
    long long singlebcc_cert_dfs_walk_ns=0;
    long long singlebcc_cert_connectivity_fail=0;
    long long singlebcc_cert_articulation_fail=0;
    long long singlebcc_cert_success=0;
    long long singlebcc_cert_success_total_ns=0;
    long long singlebcc_cert_edges_scanned=0;
    long long singlebcc_cert_vertices_scanned=0;

    long long full_step3_total_ns=0;
    long long full_step3_dfs_walk_ns=0;
    long long full_step3_materialize_ns=0;
    long long full_step3_normalize_ns=0;
    long long full_step3_keep_order_ns=0;
    long long full_step3_piece_feed_ns=0;

    long long sampled_old_full_recompute=0;
    long long single_bcc_truth_mismatch=0;
    long long keep_choice_mismatch=0;
    long long piece_count_mismatch=0;
    long long piece_multiset_mismatch=0;
};
inline Counters& G(){ static Counters c; return c; }
struct PendingInfo {
    bool active=false;
    int oldCore=-1;
    bool usedStep3=false;
    bool singleBcc=false;
};
inline PendingInfo& Pending(){ static PendingInfo p; return p; }
struct CacheState {
    bool valid=false;
    int core=-1;
    bool prevStep3=false;
    bool prevSingleBcc=false;
    bool prevKeepOnlyChain=false;
    bool boundaryUnchanged=false;
};
inline CacheState& Cache(){ static CacheState c; return c; }
inline void reset(){ G()=Counters{}; Pending()=PendingInfo{}; Cache()=CacheState{}; }
inline void dump(std::ostream& os){
    const auto& c = G();
    double eligibleRatio = c.step3_calls ? (double)c.shortcut_eligible / (double)c.step3_calls : 0.0;
    double actualSingleRatio = c.actual_new_calls ? (double)c.actual_new_single_bcc_count / (double)c.actual_new_calls : 0.0;
    double actualKeepAllRatio = c.actual_new_calls ? (double)c.actual_new_keep_is_all_count / (double)c.actual_new_calls : 0.0;
    os << "[DENSE_SINGLEBCC_R13 calls] step3_calls=" << c.step3_calls
       << " prev_step3_available=" << c.prev_step3_available
       << " prev_single_bcc=" << c.prev_single_bcc
       << " prev_keep_only_chain=" << c.prev_keep_only_chain
       << " boundary_unchanged=" << c.boundary_unchanged
       << " shortcut_eligible=" << c.shortcut_eligible
       << " shortcut_attempted=" << c.shortcut_attempted << "\n";
    os << "[DENSE_SINGLEBCC_R13 ratios] eligible_ratio=" << eligibleRatio
       << " actual_single_bcc_ratio=" << actualSingleRatio
       << " actual_keep_is_all_ratio=" << actualKeepAllRatio
       << " avg_bcc_count=" << (c.actual_new_calls ? (double)c.sum_actual_bcc_count / c.actual_new_calls : 0.0)
       << " avg_piece_count=" << (c.actual_new_calls ? (double)c.sum_actual_piece_count / c.actual_new_calls : 0.0)
       << " avg_piece_total_edge_count=" << (c.actual_new_calls ? (double)c.sum_actual_piece_total_edge_count / c.actual_new_calls : 0.0) << "\n";
    os << "[DENSE_SINGLEBCC_R13 actual_bcc_hist] b0=" << c.hist_actual_bcc_0
       << " b1=" << c.hist_actual_bcc_1 << " b2=" << c.hist_actual_bcc_2
       << " b3_4=" << c.hist_actual_bcc_3_4 << " b5p=" << c.hist_actual_bcc_5p << "\n";
    os << "[DENSE_SINGLEBCC_R13 actual_piece_hist] p0=" << c.hist_actual_piece_0
       << " p1=" << c.hist_actual_piece_1 << " p2p=" << c.hist_actual_piece_2p << "\n";
    os << "[DENSE_SINGLEBCC_R13 cert] total=" << ms(c.singlebcc_cert_total_ns)
       << " dfs_walk=" << ms(c.singlebcc_cert_dfs_walk_ns)
       << " connectivity_fail=" << c.singlebcc_cert_connectivity_fail
       << " articulation_fail=" << c.singlebcc_cert_articulation_fail
       << " success=" << c.singlebcc_cert_success
       << " success_total=" << ms(c.singlebcc_cert_success_total_ns)
       << " edges_scanned=" << c.singlebcc_cert_edges_scanned
       << " vertices_scanned=" << c.singlebcc_cert_vertices_scanned << "\n";
    os << "[DENSE_SINGLEBCC_R13 full] total=" << ms(c.full_step3_total_ns)
       << " dfs_walk=" << ms(c.full_step3_dfs_walk_ns)
       << " materialize=" << ms(c.full_step3_materialize_ns)
       << " normalize=" << ms(c.full_step3_normalize_ns)
       << " keep_order=" << ms(c.full_step3_keep_order_ns)
       << " piece_feed=" << ms(c.full_step3_piece_feed_ns) << "\n";
    os << "[DENSE_SINGLEBCC_R13 crosscheck] sampled_old_full_recompute=" << c.sampled_old_full_recompute
       << " single_bcc_truth_mismatch=" << c.single_bcc_truth_mismatch
       << " keep_choice_mismatch=" << c.keep_choice_mismatch
       << " piece_count_mismatch=" << c.piece_count_mismatch
       << " piece_multiset_mismatch=" << c.piece_multiset_mismatch << "\n";
}
#else
struct Counters{}; inline Counters& G(){ static Counters c; return c; }
struct PendingInfo{}; inline PendingInfo& Pending(){ static PendingInfo p; return p; }
struct CacheState{}; inline CacheState& Cache(){ static CacheState c; return c; }
inline void reset(){}
inline void dump(std::ostream&){}
#endif
} // namespace dense_singlebcc_round13_prof

namespace dense_tinypiece_round14_prof {
#if DENSE_TINYPIECE_ROUND14_PROFILE
using Clock = std::chrono::steady_clock;
inline long long nsSince(Clock::time_point t0){ return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-t0).count(); }
inline double ms(long long ns){ return (double)ns/1e6; }
struct Counters {
    long long step3_calls=0;
    long long actual_bcc_count_sum=0;
    long long keep_edge_count_sum=0;
    long long second_bcc_edge_count_sum=0;
    long long piece_count_sum=0;
    long long piece_total_edge_count_sum=0;
    long long max_nonkeep_piece_edge_count_sum=0;
    long long nonkeep_total_edge_count_sum=0;
    long long nonkeep_total_vertex_count_sum=0;
    long long piece_incident_to_x_neighbor_only_sum=0;
    long long piece_frontier_vertex_count_sum=0;

    long long unique_max_keep_count=0;
    long long tie_on_keep_count=0;

    long long tinypiece_eligible=0;
    long long tinypiece_attempted=0;
    long long tinypiece_success=0;
    long long tinypiece_fallback=0;
    long long fallback_keep_not_unique=0;
    long long fallback_nonkeep_budget_exceeded=0;
    long long fallback_max_piece_too_large=0;
    long long fallback_frontier_too_large=0;
    long long fallback_invariant_unsure=0;
    long long fallback_crosscheck_mismatch=0;
    long long fallback_forced=0;

    long long dfs_walk_total_ns=0;
    long long edgeStack_pop_assign_total_ns=0;
    long long bccMass_accum_total_ns=0;
    long long full_materialize_total_ns=0;
    long long full_normalize_total_ns=0;
    long long tinypiece_materialize_total_ns=0;
    long long tinypiece_keep_fastreturn_total_ns=0;
    long long piece_feed_merge_total_ns=0;
    long long crosscheck_total_ns=0;

    long long sampled_old_full_step3=0;
    long long keep_choice_mismatch=0;
    long long piece_count_mismatch=0;
    long long piece_multiset_mismatch=0;
    long long keep_edge_multiset_mismatch=0;
};
inline Counters& G(){ static Counters c; return c; }
inline void reset(){ G() = Counters{}; }
inline void dump(std::ostream& os){
    const auto& c = G();
    double calls = c.step3_calls ? (double)c.step3_calls : 1.0;
    double avg_keep = c.step3_calls ? (double)c.keep_edge_count_sum / calls : 0.0;
    double avg_nonkeep = c.step3_calls ? (double)c.nonkeep_total_edge_count_sum / calls : 0.0;
    double keep_edge_ratio = (avg_keep + avg_nonkeep > 0.0) ? avg_keep / (avg_keep + avg_nonkeep) : 0.0;
    os << "[DENSE_TINYPIECE_R14 calls] step3_calls=" << c.step3_calls
       << " unique_max_keep_count=" << c.unique_max_keep_count
       << " tie_on_keep_count=" << c.tie_on_keep_count
       << " tinypiece_eligible=" << c.tinypiece_eligible
       << " tinypiece_attempted=" << c.tinypiece_attempted
       << " tinypiece_success=" << c.tinypiece_success
       << " tinypiece_fallback=" << c.tinypiece_fallback << "\n";
    os << "[DENSE_TINYPIECE_R14 avgs]"
       << " avg_bcc_count=" << (c.step3_calls ? (double)c.actual_bcc_count_sum / calls : 0.0)
       << " avg_keep_edge_count=" << avg_keep
       << " avg_second_bcc_edge_count=" << (c.step3_calls ? (double)c.second_bcc_edge_count_sum / calls : 0.0)
       << " avg_piece_count=" << (c.step3_calls ? (double)c.piece_count_sum / calls : 0.0)
       << " avg_piece_total_edge_count=" << (c.step3_calls ? (double)c.piece_total_edge_count_sum / calls : 0.0)
       << " avg_max_nonkeep_piece_edge_count=" << (c.step3_calls ? (double)c.max_nonkeep_piece_edge_count_sum / calls : 0.0)
       << " avg_nonkeep_total_edge_count=" << avg_nonkeep
       << " avg_nonkeep_total_vertex_count=" << (c.step3_calls ? (double)c.nonkeep_total_vertex_count_sum / calls : 0.0)
       << " avg_piece_frontier_vertex_count=" << (c.step3_calls ? (double)c.piece_frontier_vertex_count_sum / calls : 0.0)
       << " keep_edge_ratio=" << keep_edge_ratio
       << " unique_max_keep_ratio=" << (c.step3_calls ? (double)c.unique_max_keep_count / calls : 0.0)
       << " tinypiece_eligible_ratio=" << (c.step3_calls ? (double)c.tinypiece_eligible / calls : 0.0)
       << " tinypiece_success_ratio=" << (c.step3_calls ? (double)c.tinypiece_success / calls : 0.0)
       << " avg_piece_incident_to_x_neighbor_only=" << (c.step3_calls ? (double)c.piece_incident_to_x_neighbor_only_sum / calls : 0.0)
       << "\n";
    os << "[DENSE_TINYPIECE_R14 fallback]"
       << " keep_not_unique=" << c.fallback_keep_not_unique
       << " nonkeep_budget_exceeded=" << c.fallback_nonkeep_budget_exceeded
       << " max_piece_too_large=" << c.fallback_max_piece_too_large
       << " frontier_too_large=" << c.fallback_frontier_too_large
       << " invariant_unsure=" << c.fallback_invariant_unsure
       << " crosscheck_mismatch=" << c.fallback_crosscheck_mismatch
       << " forced_fallback=" << c.fallback_forced << "\n";
    os << "[DENSE_TINYPIECE_R14 ms]"
       << " dfs_walk_total=" << ms(c.dfs_walk_total_ns)
       << " edgeStack_pop_assign_total=" << ms(c.edgeStack_pop_assign_total_ns)
       << " bccMass_accum_total=" << ms(c.bccMass_accum_total_ns)
       << " full_materialize_total=" << ms(c.full_materialize_total_ns)
       << " full_normalize_total=" << ms(c.full_normalize_total_ns)
       << " tinypiece_materialize_total=" << ms(c.tinypiece_materialize_total_ns)
       << " tinypiece_keep_fastreturn_total=" << ms(c.tinypiece_keep_fastreturn_total_ns)
       << " piece_feed_merge_total=" << ms(c.piece_feed_merge_total_ns)
       << " crosscheck_total=" << ms(c.crosscheck_total_ns) << "\n";
    os << "[DENSE_TINYPIECE_R14 crosscheck]"
       << " sampled_old_full_step3=" << c.sampled_old_full_step3
       << " keep_choice_mismatch=" << c.keep_choice_mismatch
       << " piece_count_mismatch=" << c.piece_count_mismatch
       << " piece_multiset_mismatch=" << c.piece_multiset_mismatch
       << " keep_edge_multiset_mismatch=" << c.keep_edge_multiset_mismatch << "\n";
}
#else
struct Counters{}; inline Counters& G(){ static Counters c; return c; }
inline void reset(){}
inline void dump(std::ostream&){}
#endif
} // namespace dense_tinypiece_round14_prof

namespace dense_tiekeep_round15_prof {
#if DENSE_TIEKEEP_ROUND15_PROFILE
using Clock = std::chrono::steady_clock;
inline long long nsSince(Clock::time_point t0){ return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-t0).count(); }
inline double ms(long long ns){ return (double)ns/1e6; }
struct Counters {
    long long step3_calls=0;
    long long actual_bcc_count_sum=0;
    long long keep_edge_count_sum=0;
    long long piece_count_sum=0;
    long long piece_total_edge_count_sum=0;
    long long max_nonkeep_piece_edge_count_sum=0;
    long long nonkeep_total_vertex_count_sum=0;
    long long actual_keep_is_large_count=0;
    long long unique_max_keep_count=0;
    long long tie_on_keep_count=0;
    long long top_mass_tie_candidate_count_sum=0;
    long long top_mass_tie_total_edge_count_sum=0;
    long long top_mass_tie_max_edge_count_sum=0;
    long long top_mass_tie_tiny_count=0;
    long long actual_fullpath_keep_from_tie_count=0;
    long long actual_fullpath_keep_is_large_in_tie_count=0;
    long long actual_fullpath_keep_is_implicit_complement_count=0;
    long long implicit_keep_eligible=0;
    long long implicit_keep_attempted=0;
    long long implicit_keep_success=0;
    long long implicit_keep_fallback=0;
    long long fallback_keep_not_large_enough=0;
    long long fallback_tie_candidate_count_too_large=0;
    long long fallback_tie_candidate_edge_budget_exceeded=0;
    long long fallback_nonkeep_budget_exceeded=0;
    long long fallback_frontier_too_large=0;
    long long fallback_implicit_compare_undetermined=0;
    long long fallback_invariant_unsure=0;
    long long fallback_crosscheck_mismatch=0;
    long long fallback_forced=0;
    long long step3_total_ns=0;
    long long dfs_walk_total_ns=0;
    long long edgeStack_pop_assign_total_ns=0;
    long long bccMass_accum_total_ns=0;
    long long full_materialize_total_ns=0;
    long long full_normalize_total_ns=0;
    long long tie_candidate_materialize_total_ns=0;
    long long tiny_piece_materialize_total_ns=0;
    long long implicit_keep_compare_total_ns=0;
    long long piece_feed_merge_total_ns=0;
    long long crosscheck_total_ns=0;
    long long implicit_keep_compare_edges_scanned=0;
    long long implicit_keep_compare_candidates_scanned=0;
    long long implicit_keep_first_diff_found_count=0;
    long long implicit_keep_compare_fullscan_count=0;
    long long sampled_old_full_step3=0;
    long long keep_choice_mismatch=0;
    long long piece_count_mismatch=0;
    long long piece_multiset_mismatch=0;
    long long keep_edge_multiset_mismatch=0;
};
inline Counters& G(){ static Counters c; return c; }
inline void reset(){ G() = Counters{}; }
inline void dump(std::ostream& os){
    const auto& c = G();
    double calls = c.step3_calls ? (double)c.step3_calls : 1.0;
    double avg_keep = c.step3_calls ? (double)c.keep_edge_count_sum / calls : 0.0;
    double avg_piece = c.step3_calls ? (double)c.piece_total_edge_count_sum / calls : 0.0;
    double avg_keep_ratio = (avg_keep + avg_piece > 0.0) ? avg_keep / (avg_keep + avg_piece) : 0.0;
    os << "[DENSE_TIEKEEP_R15 calls] step3_calls=" << c.step3_calls
       << " unique_max_keep_count=" << c.unique_max_keep_count
       << " tie_on_keep_count=" << c.tie_on_keep_count
       << " implicit_keep_eligible=" << c.implicit_keep_eligible
       << " implicit_keep_attempted=" << c.implicit_keep_attempted
       << " implicit_keep_success=" << c.implicit_keep_success
       << " implicit_keep_fallback=" << c.implicit_keep_fallback << "\n";
    os << "[DENSE_TIEKEEP_R15 avgs]"
       << " avg_bcc_count=" << (c.step3_calls ? (double)c.actual_bcc_count_sum / calls : 0.0)
       << " avg_keep_edge_count=" << avg_keep
       << " avg_piece_count=" << (c.step3_calls ? (double)c.piece_count_sum / calls : 0.0)
       << " avg_piece_total_edge_count=" << avg_piece
       << " avg_max_nonkeep_piece_edge_count=" << (c.step3_calls ? (double)c.max_nonkeep_piece_edge_count_sum / calls : 0.0)
       << " avg_nonkeep_total_vertex_count=" << (c.step3_calls ? (double)c.nonkeep_total_vertex_count_sum / calls : 0.0)
       << " keep_edge_ratio=" << avg_keep_ratio
       << " actual_keep_is_large_ratio=" << (c.step3_calls ? (double)c.actual_keep_is_large_count / calls : 0.0)
       << " unique_max_keep_ratio=" << (c.step3_calls ? (double)c.unique_max_keep_count / calls : 0.0)
       << " tie_on_keep_ratio=" << (c.step3_calls ? (double)c.tie_on_keep_count / calls : 0.0)
       << " avg_top_mass_tie_candidate_count=" << (c.step3_calls ? (double)c.top_mass_tie_candidate_count_sum / calls : 0.0)
       << " avg_top_mass_tie_total_edge_count=" << (c.step3_calls ? (double)c.top_mass_tie_total_edge_count_sum / calls : 0.0)
       << " avg_top_mass_tie_max_edge_count=" << (c.step3_calls ? (double)c.top_mass_tie_max_edge_count_sum / calls : 0.0)
       << " top_mass_tie_tiny_ratio=" << (c.step3_calls ? (double)c.top_mass_tie_tiny_count / calls : 0.0)
       << " actual_fullpath_keep_from_tie_ratio=" << (c.step3_calls ? (double)c.actual_fullpath_keep_from_tie_count / calls : 0.0)
       << " actual_fullpath_keep_is_large_in_tie_ratio=" << (c.step3_calls ? (double)c.actual_fullpath_keep_is_large_in_tie_count / calls : 0.0)
       << " actual_fullpath_keep_is_implicit_complement_ratio=" << (c.step3_calls ? (double)c.actual_fullpath_keep_is_implicit_complement_count / calls : 0.0)
       << " implicit_keep_eligible_ratio=" << (c.step3_calls ? (double)c.implicit_keep_eligible / calls : 0.0)
       << " implicit_keep_success_ratio=" << (c.step3_calls ? (double)c.implicit_keep_success / calls : 0.0)
       << "\n";
    os << "[DENSE_TIEKEEP_R15 fallback]"
       << " keep_not_large_enough=" << c.fallback_keep_not_large_enough
       << " tie_candidate_count_too_large=" << c.fallback_tie_candidate_count_too_large
       << " tie_candidate_edge_budget_exceeded=" << c.fallback_tie_candidate_edge_budget_exceeded
       << " nonkeep_budget_exceeded=" << c.fallback_nonkeep_budget_exceeded
       << " frontier_too_large=" << c.fallback_frontier_too_large
       << " implicit_compare_undetermined=" << c.fallback_implicit_compare_undetermined
       << " invariant_unsure=" << c.fallback_invariant_unsure
       << " crosscheck_mismatch=" << c.fallback_crosscheck_mismatch
       << " forced_fallback=" << c.fallback_forced << "\n";
    os << "[DENSE_TIEKEEP_R15 ms]"
       << " step3_total=" << ms(c.step3_total_ns)
       << " dfs_walk_total=" << ms(c.dfs_walk_total_ns)
       << " edgeStack_pop_assign_total=" << ms(c.edgeStack_pop_assign_total_ns)
       << " bccMass_accum_total=" << ms(c.bccMass_accum_total_ns)
       << " full_materialize_total=" << ms(c.full_materialize_total_ns)
       << " full_normalize_total=" << ms(c.full_normalize_total_ns)
       << " tie_candidate_materialize_total=" << ms(c.tie_candidate_materialize_total_ns)
       << " tiny_piece_materialize_total=" << ms(c.tiny_piece_materialize_total_ns)
       << " implicit_keep_compare_total=" << ms(c.implicit_keep_compare_total_ns)
       << " piece_feed_merge_total=" << ms(c.piece_feed_merge_total_ns)
       << " crosscheck_total=" << ms(c.crosscheck_total_ns) << "\n";
    os << "[DENSE_TIEKEEP_R15 compare]"
       << " edges_scanned=" << c.implicit_keep_compare_edges_scanned
       << " candidates_scanned=" << c.implicit_keep_compare_candidates_scanned
       << " first_diff_found=" << c.implicit_keep_first_diff_found_count
       << " fullscan_count=" << c.implicit_keep_compare_fullscan_count << "\n";
    os << "[DENSE_TIEKEEP_R15 crosscheck]"
       << " sampled_old_full_step3=" << c.sampled_old_full_step3
       << " keep_choice_mismatch=" << c.keep_choice_mismatch
       << " piece_count_mismatch=" << c.piece_count_mismatch
       << " piece_multiset_mismatch=" << c.piece_multiset_mismatch
       << " keep_edge_multiset_mismatch=" << c.keep_edge_multiset_mismatch << "\n";
}
#else
struct Counters{}; inline Counters& G(){ static Counters c; return c; }
inline void reset(){}
inline void dump(std::ostream&){}
#endif
} // namespace dense_tiekeep_round15_prof

namespace dense_spqr_round16_prof {
#if DENSE_SPQR_ROUND16_PROFILE
using Clock = std::chrono::steady_clock;
inline long long nsSince(Clock::time_point t0){ return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-t0).count(); }
inline double ms(long long ns){ return (double)ns / 1e6; }
struct ScopeTimer { long long* dst=nullptr; Clock::time_point t0; explicit ScopeTimer(long long* p):dst(p),t0(Clock::now()){} ~ScopeTimer(){ if(dst) *dst += nsSince(t0); } };
struct Counters {
    long long should_calls=0, should_true=0, should_false=0;
    long long should_false_e=0, should_false_v=0, should_false_q=0, should_false_dense=0, should_false_boundary=0, should_false_other=0;
    long long gate_samples=0;
    long long boundary_zero_hits=0, cc_one_hits=0, current_on_comb_hits=0;
    long long attachcuts_total=0, attachcuts_max=0;
    long long badqueries_total=0, badqueries_max=0;
    long long boundary_size_total=0, boundary_size_max=0;
    long long oldboundary_size_total=0, oldboundary_size_max=0;
    long long current_v_total=0, current_v_max=0;
    long long current_e_total=0, current_e_max=0;
    long long e_over_current_threshold=0, q_over_current_threshold=0;
    long long candidate_dense_structured_gate_hit=0;
    long long escape_eligible=0, escape_attempted=0, escape_success=0, escape_fallback=0;
    long long fallback_gate_not_met=0, fallback_shadow_mismatch=0, fallback_spqr_path_fail=0, fallback_validator_shadow_fail=0, fallback_invariant_unsure=0, fallback_forced_fallback=0;
    long long spqr_total_ns=0, spqr_build_total_ns=0, spqr_keep_selection_total_ns=0, spqr_piece_emit_total_ns=0;
    long long sampled_shadow_runs=0, shadow_keep_choice_mismatch=0, shadow_piece_count_mismatch=0, shadow_piece_multiset_mismatch=0, shadow_output_validator_fail=0;
};
inline Counters& G(){ static Counters c; return c; }
inline void reset(){ G() = Counters{}; }
inline void dump(std::ostream& os){
    const auto& c = G();
    double scalls = c.should_calls ? (double)c.should_calls : 1.0;
    double gcalls = c.gate_samples ? (double)c.gate_samples : 1.0;
    os << "[DENSE_SPQR_R16 should] calls=" << c.should_calls
       << " true=" << c.should_true
       << " false=" << c.should_false
       << " E_guard=" << c.should_false_e
       << " V_guard=" << c.should_false_v
       << " Q_guard=" << c.should_false_q
       << " dense_guard=" << c.should_false_dense
       << " boundary_guard=" << c.should_false_boundary
       << " other_guard=" << c.should_false_other << "\n";
    os << "[DENSE_SPQR_R16 gate] samples=" << c.gate_samples
       << " boundaryZero_ratio=" << (c.gate_samples ? (double)c.boundary_zero_hits / gcalls : 0.0)
       << " ccOne_ratio=" << (c.gate_samples ? (double)c.cc_one_hits / gcalls : 0.0)
       << " currentOnComb_ratio=" << (c.gate_samples ? (double)c.current_on_comb_hits / gcalls : 0.0)
       << " attachCuts_avg=" << (c.gate_samples ? (double)c.attachcuts_total / gcalls : 0.0)
       << " attachCuts_max=" << c.attachcuts_max
       << " badQueries_avg=" << (c.gate_samples ? (double)c.badqueries_total / gcalls : 0.0)
       << " badQueries_max=" << c.badqueries_max
       << " boundary_size_avg=" << (c.gate_samples ? (double)c.boundary_size_total / gcalls : 0.0)
       << " boundary_size_max=" << c.boundary_size_max
       << " oldBoundary_size_avg=" << (c.gate_samples ? (double)c.oldboundary_size_total / gcalls : 0.0)
       << " oldBoundary_size_max=" << c.oldboundary_size_max
       << " currentV_avg=" << (c.gate_samples ? (double)c.current_v_total / gcalls : 0.0)
       << " currentV_max=" << c.current_v_max
       << " currentE_avg=" << (c.gate_samples ? (double)c.current_e_total / gcalls : 0.0)
       << " currentE_max=" << c.current_e_max
       << " E_over_ratio=" << (c.gate_samples ? (double)c.e_over_current_threshold / gcalls : 0.0)
       << " Q_over_ratio=" << (c.gate_samples ? (double)c.q_over_current_threshold / gcalls : 0.0)
       << " structured_gate_hit=" << c.candidate_dense_structured_gate_hit << "\n";
    os << "[DENSE_SPQR_R16 escape] eligible=" << c.escape_eligible
       << " attempted=" << c.escape_attempted
       << " success=" << c.escape_success
       << " fallback=" << c.escape_fallback
       << " gate_not_met=" << c.fallback_gate_not_met
       << " shadow_mismatch=" << c.fallback_shadow_mismatch
       << " spqr_path_fail=" << c.fallback_spqr_path_fail
       << " validator_shadow_fail=" << c.fallback_validator_shadow_fail
       << " invariant_unsure=" << c.fallback_invariant_unsure
       << " forced_fallback=" << c.fallback_forced_fallback << "\n";
    os << "[DENSE_SPQR_R16 ms] spqr_total=" << ms(c.spqr_total_ns)
       << " spqr_build=" << ms(c.spqr_build_total_ns)
       << " spqr_keep_selection=" << ms(c.spqr_keep_selection_total_ns)
       << " spqr_piece_emit=" << ms(c.spqr_piece_emit_total_ns) << "\n";
    os << "[DENSE_SPQR_R16 shadow] sampled_shadow_runs=" << c.sampled_shadow_runs
       << " keep_choice_mismatch=" << c.shadow_keep_choice_mismatch
       << " piece_count_mismatch=" << c.shadow_piece_count_mismatch
       << " piece_multiset_mismatch=" << c.shadow_piece_multiset_mismatch
       << " output_validator_fail=" << c.shadow_output_validator_fail << "\n";
}
#else
struct Counters{}; inline Counters& G(){ static Counters c; return c; }
inline void reset(){}
inline void dump(std::ostream&){}
#endif
} // namespace dense_spqr_round16_prof

namespace dense_shadow_diff_round20_prof {
#if DENSE_SHADOW_DIFF_ROUND20_PROFILE
using Clock = std::chrono::steady_clock;
inline long long nsSince(Clock::time_point t0){ return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-t0).count(); }
inline double ms(long long ns){ return (double)ns / 1e6; }
struct Counters {
    long long rows = 0;
    long long shadow_prefilter_hit = 0;
    long long shadow_prefilter_eligible = 0;
    long long shadow_attempted = 0;
    long long shadow_match = 0;
    long long shadow_mismatch = 0;
    long long mismatch_keep_choice = 0;
    long long mismatch_piece_count = 0;
    long long mismatch_piece_multiset = 0;
    long long mismatch_validator = 0;
    long long mismatch_spqr_path_fail = 0;
    long long mismatch_invariant_unsure = 0;
    long long mismatch_forced = 0;
};
inline Counters& G(){ static Counters c; return c; }
inline void reset(){ G() = Counters{}; }
inline std::string envs(const char* k){ const char* v = std::getenv(k); return v ? std::string(v) : std::string(); }
inline long long envll(const char* k, long long defv=-1){ const char* v = std::getenv(k); if(!v||!*v) return defv; try { return std::stoll(v); } catch(...) { return defv; } }
inline std::string caseMode(){ return envs("DENSE_SHADOW_CASE_MODE"); }
inline long long caseN(){ return envll("DENSE_SHADOW_CASE_N", -1); }
inline long long caseSeed(){ return envll("DENSE_SHADOW_CASE_SEED", -1); }
inline bool isDenseMode(){ static const std::unordered_set<std::string> denseModes={"comb_dense","comb_rect_dense","caterpillar_rect_dense","multi_comb_rect","multi_comb_cap","caterpillar_mixed"}; return denseModes.count(caseMode())!=0; }
inline long long totalLocalIdAdjNs(){
#if DENSE_LOCALIDADJ_ROUND9_PROFILE
    auto& c = dense_localidadj_round9_prof::G();
    return c.lid_mark_realof_ns + c.lid_deadedge_filter_ns + c.lid_degree_first_ns + c.lid_surv_arrays_ns + c.lid_adj_alloc_ns + c.lid_adj_fill_ns + c.lid_deg_fill_ns + c.lid_other_ns;
#else
    return 0;
#endif
}
inline long long totalStep3Ns(){
#if DENSE_TIEKEEP_ROUND15_PROFILE
    return dense_tiekeep_round15_prof::G().step3_total_ns;
#else
    return 0;
#endif
}
inline long long totalKeepOrderNs(){
#if DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
    return dense_rect_round4_keep_prof::G().keep_total_ns;
#else
    return 0;
#endif
}
inline long long totalApplyNs(){
#if DENSE_RECT_ROUND1_PROFILE
    auto& c = dense_rect_round1_prof::G();
    return c.apply_dead_ns + c.apply_small_ns + c.apply_boundary_ns + c.apply_oldbad_ns;
#else
    return 0;
#endif
}
inline long long totalSpqrNs(){
#if DENSE_SPQR_ROUND16_PROFILE
    return dense_spqr_round16_prof::G().spqr_total_ns;
#else
    return 0;
#endif
}
inline long long totalSpqrBuildNs(){
#if DENSE_SPQR_ROUND16_PROFILE
    return dense_spqr_round16_prof::G().spqr_build_total_ns;
#else
    return 0;
#endif
}
inline long long totalSpqrKeepNs(){
#if DENSE_SPQR_ROUND16_PROFILE
    return dense_spqr_round16_prof::G().spqr_keep_selection_total_ns;
#else
    return 0;
#endif
}
inline long long totalSpqrPieceNs(){
#if DENSE_SPQR_ROUND16_PROFILE
    return dense_spqr_round16_prof::G().spqr_piece_emit_total_ns;
#else
    return 0;
#endif
}
struct Snap { long long localIdAdjNs=0, step3Ns=0, keepOrderNs=0, applyNs=0, spqrNs=0, spqrBuildNs=0, spqrKeepNs=0, spqrPieceNs=0; };
inline Snap snap(){ Snap s; s.localIdAdjNs=totalLocalIdAdjNs(); s.step3Ns=totalStep3Ns(); s.keepOrderNs=totalKeepOrderNs(); s.applyNs=totalApplyNs(); s.spqrNs=totalSpqrNs(); s.spqrBuildNs=totalSpqrBuildNs(); s.spqrKeepNs=totalSpqrKeepNs(); s.spqrPieceNs=totalSpqrPieceNs(); return s; }
inline std::string bucketShape(int q,int attachCuts,int boundarySize,int xNbrCount,bool boundaryZero,bool ccOne){ auto bq=[&](int x){ if(x<=8) return 8; if(x<=16) return 16; if(x<=32) return 32; if(x<=64) return 64; if(x<=128) return 128; if(x<=256) return 256; if(x<=512) return 512; if(x<=1024) return 1024; if(x<=4096) return 4096; if(x<=16384) return 16384; return 65535; }; std::ostringstream os; os<<"Q"<<bq(q)<<"_A"<<bq(attachCuts)<<"_B"<<bq(boundarySize)<<"_X"<<bq(xNbrCount)<<"_BZ"<<(boundaryZero?1:0)<<"_CC"<<(ccOne?1:0); return os.str(); }
inline uint64_t fnv1a_u64(uint64_t h, uint64_t x){ h ^= x; h *= 1099511628211ULL; return h; }
inline std::string hashVec(const std::vector<int>& v){ uint64_t h=1469598103934665603ULL; for(int x:v) h=fnv1a_u64(h,(uint64_t)(uint32_t)x+0x9e3779b97f4a7c15ULL); std::ostringstream os; os<<std::hex<<h; return os.str(); }
inline std::string hashVV(const std::vector<std::vector<int>>& vv){ uint64_t h=1469598103934665603ULL; for(const auto& v:vv){ h=fnv1a_u64(h,0xabcddcbaULL); for(int x:v) h=fnv1a_u64(h,(uint64_t)(uint32_t)x+0x517cc1b727220a95ULL);} std::ostringstream os; os<<std::hex<<h; return os.str(); }
#ifdef DEBUG_SOLVER
inline std::vector<std::vector<int>> smallPiecesOnly(const Solver::CanonSparseSemantic& C){ auto vv=C.pieceEdges; if(!C.keepEdges.empty()){ auto it=std::find(vv.begin(),vv.end(),C.keepEdges); if(it!=vv.end()) vv.erase(it); } return vv; }
#endif
inline void appendRow(const std::vector<std::string>& cols){ std::ofstream ofs("round20_dense_shadow_census_rows.tsv", std::ios::app); for(size_t i=0;i<cols.size();++i){ if(i) ofs << '\t'; ofs << cols[i]; } ofs << '\n'; ofs.flush(); }
inline void appendSummaryRow(const std::vector<std::string>& cols){ std::ofstream ofs("round20_dense_shadow_profile_summary.tsv", std::ios::app); for(size_t i=0;i<cols.size();++i){ if(i) ofs << '\t'; ofs << cols[i]; } ofs << '\n'; ofs.flush(); }
inline void writeMismatchExample(int callIdx,const std::string& mismatchKind,const std::string& badQueriesShape,int currentV,int currentE,int currentQ,int attachCuts,int boundarySize,const std::string& keepHashA,const std::string& keepHashB,int pieceCountA,int pieceCountB,const std::string& pieceHashA,const std::string& pieceHashB){ std::ostringstream path; path<<"round20_shadow_mismatch_examples/"<<caseMode()<<"_n"<<caseN()<<"_s"<<caseSeed()<<"_call"<<callIdx<<".json"; std::ofstream ofs(path.str()); if(!ofs) return; ofs<<"{\n"; ofs<<"  \"case_mode\": \""<<caseMode()<<"\",\n"; ofs<<"  \"case_n\": "<<caseN()<<",\n"; ofs<<"  \"case_seed\": "<<caseSeed()<<",\n"; ofs<<"  \"call_idx\": "<<callIdx<<",\n"; ofs<<"  \"mismatch_kind\": \""<<mismatchKind<<"\",\n"; ofs<<"  \"badQueriesShape\": \""<<badQueriesShape<<"\",\n"; ofs<<"  \"currentV\": "<<currentV<<",\n"; ofs<<"  \"currentE\": "<<currentE<<",\n"; ofs<<"  \"currentQ\": "<<currentQ<<",\n"; ofs<<"  \"attachCuts\": "<<attachCuts<<",\n"; ofs<<"  \"boundarySize\": "<<boundarySize<<",\n"; ofs<<"  \"fallback_keep_hash\": \""<<keepHashA<<"\",\n"; ofs<<"  \"spqr_keep_hash\": \""<<keepHashB<<"\",\n"; ofs<<"  \"fallback_piece_count\": "<<pieceCountA<<",\n"; ofs<<"  \"spqr_piece_count\": "<<pieceCountB<<",\n"; ofs<<"  \"fallback_piece_multiset_hash\": \""<<pieceHashA<<"\",\n"; ofs<<"  \"spqr_piece_multiset_hash\": \""<<pieceHashB<<"\"\n"; ofs<<"}\n"; }
inline void dump(std::ostream& os){ const auto& c=G(); os<<"[DENSE_SHADOW_DIFF_R20 rows] rows="<<c.rows<<" prefilter_hit="<<c.shadow_prefilter_hit<<" prefilter_eligible="<<c.shadow_prefilter_eligible<<" shadow_attempted="<<c.shadow_attempted<<" shadow_match="<<c.shadow_match<<" shadow_mismatch="<<c.shadow_mismatch<<"\n"; os<<"[DENSE_SHADOW_DIFF_R20 mismatch] keep_choice="<<c.mismatch_keep_choice<<" piece_count="<<c.mismatch_piece_count<<" piece_multiset="<<c.mismatch_piece_multiset<<" validator="<<c.mismatch_validator<<" spqr_path_fail="<<c.mismatch_spqr_path_fail<<" invariant_unsure="<<c.mismatch_invariant_unsure<<" forced="<<c.mismatch_forced<<"\n"; if(!caseMode().empty()){ std::vector<std::string> cols; cols.push_back(caseMode()); cols.push_back(std::to_string(caseN())); cols.push_back(std::to_string(caseSeed()));
#if DENSE_SPQR_ROUND16_PROFILE
 auto& s = dense_spqr_round16_prof::G(); cols.push_back(std::to_string(s.should_calls)); cols.push_back(std::to_string(s.should_true)); cols.push_back(std::to_string(s.should_false)); cols.push_back(std::to_string(s.should_false_e)); cols.push_back(std::to_string(s.should_false_v)); cols.push_back(std::to_string(s.should_false_q)); cols.push_back(std::to_string(s.should_false_dense)); cols.push_back(std::to_string(s.should_false_boundary));
#else
 for(int i=0;i<8;++i) cols.push_back("0");
#endif
 cols.push_back(std::to_string(c.shadow_prefilter_hit)); cols.push_back(std::to_string(c.shadow_prefilter_eligible)); cols.push_back(std::to_string(c.shadow_attempted)); cols.push_back(std::to_string(c.shadow_match)); cols.push_back(std::to_string(c.shadow_mismatch)); cols.push_back(std::to_string(totalSpqrNs())); cols.push_back(std::to_string(totalLocalIdAdjNs())); cols.push_back(std::to_string(totalStep3Ns())); cols.push_back(std::to_string(totalKeepOrderNs())); cols.push_back(std::to_string(totalApplyNs())); appendSummaryRow(cols);} }
#else
struct Counters{}; inline Counters& G(){ static Counters c; return c; }
inline void reset(){}
inline bool isDenseMode(){ return false; }
inline void dump(std::ostream&){}
#endif
} // namespace dense_shadow_diff_round20_prof

namespace statecert_fastkey {


enum Mode : uint8_t { DISABLED = 0, WARMUP = 1, ACTIVE = 2 };
struct BuiltKey {
    uint64_t bits = 0;
    uint64_t familyCoreKey = 0;
    int survV = 0;
    int survE = 0;
    int deadEdgeCount = 0;
    uint8_t boundaryZero = 0;
    uint8_t ccOne = 0;
    uint8_t survVBucket = 0;
    uint8_t survEBucket = 0;
    uint8_t minDegBucket = 0;
    uint8_t maxDegBucket = 0;
    uint8_t distinctDefBucket = 0;
    uint8_t df16pDominant = 0;
    uint8_t highDegDominant = 0;
    uint8_t majorDegBucket = 0;
    uint8_t secondDegBucket = 0;
    uint8_t majorDefBucket = 0;
    uint8_t secondDefBucket = 0;
    uint8_t xDegBucket = 0;
    uint8_t xDefBucket = 0;
};
struct KeepCertState {
    uint8_t mode = DISABLED;
    bool valid = false;
    uint64_t familyKey = 0;
    uint64_t familyCoreKey = 0;
    int lastStep = -1;
    int lastSurvV = 0;
    int lastSurvE = 0;
    int lastSplitEpoch = 0;
    uint8_t majorDegBucket = 0;
    uint8_t secondDegBucket = 0;
    uint8_t majorDefBucket = 0;
    uint8_t secondDefBucket = 0;
    uint16_t sameFamilyRunLen = 0;
    uint8_t warmupAttempts = 0;
    uint8_t consecutiveMisses = 0;
    uint32_t totalHits = 0;
};
static int currentStep = 0;
static std::unordered_map<int, KeepCertState> states;
static std::unordered_map<int,int> coreEpoch;
static long long modeS0Hits = 0;
static long long fallbackHits = 0;
static std::vector<int> defSeen;
static int defStamp = 1;
#if STATECERT_GATEPROF
static long long totalBranch5Calls = 0;
static long long keybuildCalls = 0;
static double keybuildMs = 0.0;
static long long warmupCalls = 0;
static long long activeCalls = 0;
static long long disabledSkips = 0;
static long long zerohitDisableCount = 0;
static long long activeDisableCount = 0;
static long long familyMissCount = 0;
static long long shapeMissCount = 0;
#endif
#if STATECERT_DEBUG_CROSSCHECK
static long long shortcutOk = 0;
static long long shortcutMismatch = 0;
#endif
#if CHAINRELAX_DEBUG_CROSSCHECK
static long long relaxedGateHits = 0;
static long long relaxedGateHitsChain = 0;
static long long relaxedGateHitsRandom = 0;
static long long relaxedGateHitsComb = 0;
static long long relaxedMismatch = 0;
static long long gateDeniedHits = 0;
#endif

static int bucketCnt(int x){ if(x<=0) return 0; if(x==1) return 1; if(x==2) return 2; if(x<=4) return 3; if(x<=8) return 4; if(x<=16) return 5; if(x<=32) return 6; if(x<=64) return 7; if(x<=128) return 8; return 9; }
static int bucketDeg(int x){ if(x<=0) return 0; if(x==1) return 1; if(x==2) return 2; if(x==3) return 3; if(x<=7) return 4; if(x<=15) return 5; if(x<=31) return 6; if(x<=63) return 7; if(x<=127) return 8; return 9; }
static inline int argmaxBucket(const int *a, int n){ int best=0; for(int i=1;i<n;++i) if(a[i]>a[best]) best=i; return best; }
static inline int argsecondBucket(const int *a, int n, int best){ int sec = best; for(int i=0;i<n;++i){ if(i==best) continue; if(sec==best || a[i] > a[sec]) sec=i; } return sec; }
static int getEpoch(int core){ auto it=coreEpoch.find(core); return it==coreEpoch.end()?0:it->second; }
static inline bool policyMajorDefStable(const KeepCertState& S, const BuiltKey& k){
    return S.valid && S.majorDefBucket == k.majorDefBucket;
}
static void bumpEpoch(int core){ coreEpoch[core] = getEpoch(core) + 1; }
static void reset(){ currentStep=0; states.clear(); coreEpoch.clear(); modeS0Hits=0; fallbackHits=0; defSeen.clear(); defStamp=1;
#if STATECERT_GATEPROF
 totalBranch5Calls=0; keybuildCalls=0; keybuildMs=0; warmupCalls=0; activeCalls=0; disabledSkips=0; zerohitDisableCount=0; activeDisableCount=0; familyMissCount=0; shapeMissCount=0;
#endif
#if STATECERT_DEBUG_CROSSCHECK
 shortcutOk=0; shortcutMismatch=0;
#endif
#if CHAINRELAX_DEBUG_CROSSCHECK
 relaxedGateHits=relaxedGateHitsChain=relaxedGateHitsRandom=relaxedGateHitsComb=relaxedMismatch=gateDeniedHits=0;
#endif
}
static KeepCertState& ensureState(int core){
    int ep = getEpoch(core);
    auto &S = states[core];
    if (S.lastSplitEpoch != ep) {
        S = KeepCertState{};
        S.mode = WARMUP;
        S.lastSplitEpoch = ep;
    }
    return S;
}
static BuiltKey buildKey(const std::vector<int>& deg, int nonIso, int survE, int oldBoundaryCount, int xNbrCount){
    BuiltKey k; k.survV=nonIso; k.survE=survE; k.deadEdgeCount=xNbrCount;
    int degFreq[10] = {0}; int defFreq[10] = {0}; int minDeg = INT_MAX, maxDeg = 0, highDeg = 0, distinctDef = 0, df16pCnt = 0;
    if ((int)defSeen.size() < nonIso + 5) defSeen.assign(nonIso + 5, 0);
    ++defStamp; if (defStamp == INT_MAX) { std::fill(defSeen.begin(), defSeen.end(), 0); defStamp = 1; }
    for (int d : deg) if (d > 0) {
        if (d < minDeg) minDeg = d; if (d > maxDeg) maxDeg = d; if (d >= 8) highDeg++;
        int db=bucketDeg(d); if(db>9) db=9; degFreq[db]++;
        int def=(nonIso-1)-d; if(def>=16) df16pCnt++; int fb=bucketDeg(def); if(fb>9) fb=9; defFreq[fb]++;
        if (0 <= def && def < (int)defSeen.size() && defSeen[def] != defStamp) { defSeen[def]=defStamp; distinctDef++; }
    }
    if (minDeg == INT_MAX) minDeg = 0;
    int majorDegBucket = argmaxBucket(degFreq, 10), secondDegBucket = argsecondBucket(degFreq, 10, majorDegBucket);
    int majorDefBucket = argmaxBucket(defFreq, 10), secondDefBucket = argsecondBucket(defFreq, 10, majorDefBucket);
    int distinctDefBucket = bucketCnt(distinctDef);
    int boundaryZero=(oldBoundaryCount==0), ccOne=1, survVBucket=bucketCnt(nonIso), survEBucket=bucketCnt(survE), minDegBucket=bucketDeg(minDeg), maxDegBucket=bucketDeg(maxDeg);
    int df16pDominant=(df16pCnt*2 >= std::max(1,nonIso)), highDegDominant=(highDeg*2 >= std::max(1,nonIso));
    int xDegBucket = bucketDeg(xNbrCount), xDefBucket = bucketDeg(std::max(0, nonIso - xNbrCount));
    k.boundaryZero=boundaryZero; k.ccOne=ccOne; k.survVBucket=survVBucket; k.survEBucket=survEBucket; k.minDegBucket=minDegBucket; k.maxDegBucket=maxDegBucket; k.distinctDefBucket=distinctDefBucket; k.df16pDominant=df16pDominant; k.highDegDominant=highDegDominant; k.majorDegBucket=majorDegBucket; k.secondDegBucket=secondDegBucket; k.majorDefBucket=majorDefBucket; k.secondDefBucket=secondDefBucket; k.xDegBucket=xDegBucket; k.xDefBucket=xDefBucket;
    uint64_t bits=0; auto push=[&](uint64_t v,int w){ bits=(bits<<w)|(v&((1ULL<<w)-1)); };
    push(boundaryZero,1); push(ccOne,1); push(survVBucket,4); push(survEBucket,4); push(minDegBucket,4); push(maxDegBucket,4); push(distinctDefBucket,4); push(df16pDominant,1); push(highDegDominant,1); push(majorDegBucket,4); push(majorDefBucket,4); push(xDegBucket,4); push(xDefBucket,4);
    k.bits=bits; k.familyCoreKey=(bits>>8); return k;
}
static inline bool xDefInTop2(const BuiltKey& k){ return k.xDefBucket == k.majorDefBucket || k.xDefBucket == k.secondDefBucket; }
static inline bool gatePassSelected(const KeepCertState& S, const BuiltKey& k){ return (S.mode == ACTIVE); }
static void seedState(KeepCertState &S, int core, const BuiltKey& k){
    int ep = getEpoch(core) + 1;
    uint16_t run = 1;
    if (S.lastStep == currentStep - 1 && S.lastSplitEpoch == ep && S.familyCoreKey == k.familyCoreKey) {
        run = (uint16_t)std::min<int>(65535, (int)S.sameFamilyRunLen + 1);
    }
    S.valid=true; S.familyKey=k.bits; S.familyCoreKey=k.familyCoreKey; S.lastStep=currentStep; S.lastSurvV=k.survV; S.lastSurvE=k.survE; S.lastSplitEpoch=ep; S.majorDegBucket=k.majorDegBucket; S.secondDegBucket=k.secondDegBucket; S.majorDefBucket=k.majorDefBucket; S.secondDefBucket=k.secondDefBucket; S.sameFamilyRunLen=run;
}
static void recordShortcutHit(int core, KeepCertState &S, const BuiltKey& k){
    ++modeS0Hits; seedState(S, core, k); S.mode = ACTIVE; S.warmupAttempts = 0; S.consecutiveMisses = 0; ++S.totalHits;
}
static void recordFallbackAccept(int core, KeepCertState &S, const BuiltKey& k){
    ++fallbackHits;
    bool stable = policyMajorDefStable(S, k);
    if (S.mode == WARMUP) {
#if STATECERT_GATEPROF
        ++warmupCalls;
#endif
        if (S.warmupAttempts < 255) ++S.warmupAttempts;
        seedState(S, core, k);
        if (S.totalHits == 0 && S.warmupAttempts >= 2) {
            if (!stable) {
                S.mode = DISABLED; S.valid = false;
#if STATECERT_GATEPROF
                ++zerohitDisableCount;
#endif
            }
        }
    } else if (S.mode == ACTIVE) {
#if STATECERT_GATEPROF
        ++activeCalls;
#endif
        if (S.consecutiveMisses < 255) ++S.consecutiveMisses;
        seedState(S, core, k);
        if (S.consecutiveMisses >= 2) {
            if (!stable) {
                S.mode = DISABLED; S.valid = false;
#if STATECERT_GATEPROF
                ++activeDisableCount;
#endif
            }
        }
    }
}
static void recordReject(int core, KeepCertState &S, const BuiltKey* pk = nullptr){
    bool stable = (pk ? policyMajorDefStable(S, *pk) : false);
    if (S.mode == WARMUP) {
#if STATECERT_GATEPROF
        ++warmupCalls;
#endif
        if (S.warmupAttempts < 255) ++S.warmupAttempts;
        S.valid = false;
        if (S.totalHits == 0 && S.warmupAttempts >= 2) {
            if (!stable) {
                S.mode = DISABLED;
#if STATECERT_GATEPROF
                ++zerohitDisableCount;
#endif
            }
        }
    } else if (S.mode == ACTIVE) {
#if STATECERT_GATEPROF
        ++activeCalls;
#endif
        if (S.consecutiveMisses < 255) ++S.consecutiveMisses;
        S.valid = false;
        if (S.consecutiveMisses >= 2) {
            if (!stable) {
                S.mode = DISABLED;
#if STATECERT_GATEPROF
                ++activeDisableCount;
#endif
            }
        }
    }
}
static void invalidate(int core){ auto it=states.find(core); if(it!=states.end()) it->second.valid=false; }
static std::string modeName(uint8_t mode){ return mode==DISABLED?"DISABLED":mode==WARMUP?"WARMUP":"ACTIVE"; }
static void dump(std::ostream& os){
    os << "[STATECERT_GATED] modeS0_hits=" << modeS0Hits << " fallback_hits=" << fallbackHits;
#if STATECERT_GATEPROF
    os << " total_branch5=" << totalBranch5Calls << " keybuild_calls=" << keybuildCalls << " keybuild_ms=" << keybuildMs
       << " warmup_calls=" << warmupCalls << " active_calls=" << activeCalls << " disabled_skips=" << disabledSkips
       << " zerohit_disable=" << zerohitDisableCount << " active_disable=" << activeDisableCount
       << " family_miss=" << familyMissCount << " shape_miss=" << shapeMissCount;
#endif
#if STATECERT_DEBUG_CROSSCHECK
    os << " shortcut_ok=" << shortcutOk << " shortcut_mismatch=" << shortcutMismatch;
#endif
    os << "\n";
#if CHAINRELAX_DEBUG_CROSSCHECK
    os << "[CHAINRELAX_DEBUG] relaxed_hits=" << relaxedGateHits << " chain=" << relaxedGateHitsChain << " random=" << relaxedGateHitsRandom << " comb=" << relaxedGateHitsComb << " mismatch=" << relaxedMismatch << " gate_denied=" << gateDeniedHits << "\n";
#endif
}
}

Solver::Solver(int n, const std::vector<InputQuery>& qs) {
    N = n;
    M = (int)qs.size();
    inputQueries = qs;
}

Solver::SparsePatch Solver::splitBlockLocalRebuild(int oldCore, int x) const {
    chk(0 <= oldCore && oldCore < (int)blocks.size(),
        "splitBlockLocalRebuild: bad oldCore");
    chk(blocks[oldCore].alive,
        "splitBlockLocalRebuild: dead oldCore");

    const BlockCore& B = blocks[oldCore];

    SparsePatch S;
    S.oldCoreId = oldCore;
    S.deletedVertex = x;
    S.keepExists = false;
    S.keepMatPiece = -1;
    S.cheapfanPreAdjDirectReturn = false;
#if DENSE_SINGLEBCC_ROUND13_PROFILE
    { auto& __r13_p = dense_singlebcc_round13_prof::Pending(); __r13_p = dense_singlebcc_round13_prof::PendingInfo{}; __r13_p.active = true; __r13_p.oldCore = oldCore; }
#endif
#if DENSE_BCCREUSE_ROUND12_PROFILE
    {
        auto& __r12_pending = dense_bccreuse_round12_prof::Pending();
        __r12_pending = dense_bccreuse_round12_prof::PendingStep3Info{};
        __r12_pending.active = true;
        __r12_pending.oldCore = oldCore;
        ++dense_bccreuse_round12_prof::G().split_calls;
    }
#endif
#if DENSE_RECT_ROUND1_PROFILE
    dense_rect_round1_prof::G().split_calls++;
#endif
#if SPARSE_HARDSCALING_ROUND5_PROFILE
    sparse_round5_prof::G().local_rebuild_calls++;
    if (B.badQueries.empty()) ++sparse_round5_prof::G().bad_empty_calls;
    else ++sparse_round5_prof::G().bad_nonempty_calls;
    sparse_round5_prof::addAttachCutsSize((int)B.attachCuts.size());
    sparse_round5_prof::ScopeTimer __prof_local_rebuild_scope(&sparse_round5_prof::G().local_rebuild_total_ns);
#endif
#if CHEAPFAN_ROUND6_PROFILE || CHEAPFAN_ROUND6_OPT
    static thread_local std::vector<int> __cheapfanOldBoundarySeen;
    static thread_local int __cheapfanOldBoundaryEpoch = 1;
    if ((int)__cheapfanOldBoundarySeen.size() < (int)orig.size()) {
        __cheapfanOldBoundarySeen.assign(orig.size(), 0);
        __cheapfanOldBoundaryEpoch = 1;
    }
#endif

    // 1) dead edges = x-incident edges inside oldCore
#if DENSE_RECT_ROUND1_PROFILE
    auto __prof_split_dead_t0 = dense_rect_round1_prof::Clock::now();
#endif
    std::unordered_set<int> deadE;
    for (int e : B.realEdges) {
        if (edges[e].u == x || edges[e].v == x) {
            S.deadEdges.push_back(e);
            deadE.insert(e);
        }
    }
    S.deadEdges = normVec(std::move(S.deadEdges));
#if DENSE_RECT_ROUND1_PROFILE
    dense_rect_round1_prof::G().split_dead_ns += dense_rect_round1_prof::nsSince(__prof_split_dead_t0);
#endif

    // --------------------------------------------------------
    // Fast path A: no-bad single-edge block
    // --------------------------------------------------------
    if (B.badQueries.empty() && (int)B.realEdges.size() == 1) {
        int e = B.realEdges[0];
        int a = edges[e].u;
        int b = edges[e].v;
        int y = (a == x ? b : a);

        // surviving edge set is empty
        S.keepExists = false;
        S.keepMatPiece = -1;
        S.deadHandles.clear();

        bool oldBoundaryY = false;
        for (int cutBC : B.attachCuts) {
            int v = bcNodes[cutBC].origVertex;
            if (v == y) {
                oldBoundaryY = true;
                break;
            }
        }

        if (1 <= y && y <= N && orig[y].alive) {
            if (oldBoundaryY) {
                SparseBoundary bd;
                bd.vertex = y;
                bd.existedOldCut = true;
                bd.touchesKeep = false;
                bd.smallIds.clear();
                S.boundary.push_back(std::move(bd));
            } else {
                S.isolatedExclusive.push_back(y);
            }
        }

        S.isolatedExclusive = normVec(std::move(S.isolatedExclusive));
        S.deadExclusiveVertices.push_back(x);
        for (int v : S.isolatedExclusive) if (v != x) S.deadExclusiveVertices.push_back(v);
        S.deadExclusiveVertices = normVec(std::move(S.deadExclusiveVertices));
        return S;
    }

    // --------------------------------------------------------
    // Fast path B: no-bad simple-cycle block.
    // Original block has degree 2 at every vertex and |E|=|V|.
    // After deleting x, surviving graph is a path, so each edge becomes
    // its own BCC. We can materialize this without Tarjan.
    // --------------------------------------------------------
    if (B.badQueries.empty() && !B.realEdges.empty()) {
        bool cycleLike = ((int)B.realEdges.size() == (int)B.allVertices.size());
        std::unordered_map<int,int> deg0;
        if (cycleLike) {
            deg0.reserve(B.allVertices.size() * 2 + 1);
            for (int e : B.realEdges) {
                deg0[edges[e].u]++;
                deg0[edges[e].v]++;
            }
            for (int v : B.allVertices) {
                if (deg0[v] != 2) {
                    cycleLike = false;
                    break;
                }
            }
        }

        if (cycleLike) {
            // Build surviving local adjacency excluding x.
            std::unordered_map<int, std::vector<std::pair<int,int>>> adj2;
            adj2.reserve(B.allVertices.size() * 2 + 1);
            std::vector<int> survVerts;
            survVerts.reserve(B.allVertices.size());
            std::unordered_set<int> survSet;

            for (int v : B.allVertices) {
                if (v == x) continue;
                if (1 <= v && v <= N && orig[v].alive) {
                    survVerts.push_back(v);
                    survSet.insert(v);
                }
            }

            std::vector<int> survEdges;
            survEdges.reserve(B.realEdges.size());
            for (int e : B.realEdges) {
                if (deadE.count(e)) continue;
                int a2 = edges[e].u, b2 = edges[e].v;
                if (!survSet.count(a2) || !survSet.count(b2)) continue;
                adj2[a2].push_back({b2, e});
                adj2[b2].push_back({a2, e});
                survEdges.push_back(e);
            }

            // In cycleLike block, after deleting x we should get a single path
            // or empty graph. If the surviving graph is malformed, fall back.
            bool pathLike = true;
            int endCnt = 0;
            int start = -1;
            for (int v : survVerts) {
                int d = (int)adj2[v].size();
                if (d == 0) continue;
                if (d == 1) {
                    endCnt++;
                    start = v;
                } else if (d == 2) {
                    // ok
                } else {
                    pathLike = false;
                    break;
                }
            }
            if (!survEdges.empty() && endCnt != 2) pathLike = false;

            if (pathLike) {
                // old boundary seed
                std::unordered_set<int> oldBoundary;
                oldBoundary.reserve(B.attachCuts.size() * 2 + 1);
                for (int cutBC : B.attachCuts) {
                    int v = bcNodes[cutBC].origVertex;
                    if (v != x && 1 <= v && v <= N && orig[v].alive) oldBoundary.insert(v);
                }

                // Empty surviving graph => just isolated/non-isolated endpoint handling.
                if (survEdges.empty()) {
                    for (int v : survVerts) {
                        if (oldBoundary.count(v)) {
                            SparseBoundary bd;
                            bd.vertex = v;
                            bd.existedOldCut = true;
                            bd.touchesKeep = false;
                            S.boundary.push_back(std::move(bd));
                        } else {
                            S.isolatedExclusive.push_back(v);
                        }
                    }
                    S.isolatedExclusive = normVec(std::move(S.isolatedExclusive));
                    S.deadHandles.clear();
                    S.deadExclusiveVertices.push_back(x);
                    for (int v : S.isolatedExclusive) if (v != x) S.deadExclusiveVertices.push_back(v);
                    S.deadExclusiveVertices = normVec(std::move(S.deadExclusiveVertices));
                    return S;
                }

                // Recover edge order along the path.
                std::vector<int> pathEdges;
                std::vector<int> pathVerts;
                pathEdges.reserve(survEdges.size());
                pathVerts.reserve(survEdges.size() + 1);

                std::unordered_set<int> usedE;
                int prev = -1, cur = start;
                pathVerts.push_back(cur);
                while (true) {
                    int nextV = -1, nextE = -1;
                    for (auto [to, e] : adj2[cur]) {
                        if (usedE.count(e)) continue;
                        if (to == prev) {
                            // still valid if the other edge is already used
                        }
                        nextV = to;
                        nextE = e;
                        break;
                    }
                    if (nextE == -1) break;
                    usedE.insert(nextE);
                    pathEdges.push_back(nextE);
                    prev = cur;
                    cur = nextV;
                    pathVerts.push_back(cur);
                }

                if ((int)pathEdges.size() == (int)survEdges.size()) {
                    // Each surviving edge is its own BCC on the path.
                    // Keep = smallest edge id (same tie-break spirit as local rebuild).
                    int keepPos = 0;
                    for (int i = 1; i < (int)pathEdges.size(); ++i) {
                        if (pathEdges[i] < pathEdges[keepPos]) keepPos = i;
                    }
                    S.keepExists = true;
                    S.keepMatPiece = keepPos;

                    for (int i = 0; i < (int)pathEdges.size(); ++i) {
                        if (i == keepPos) continue;
                        SparsePiece sp;
                        sp.matPieceId = i;
                        sp.edges = {pathEdges[i]};
                        int h = edges[pathEdges[i]].handleId;
                        if (h != -1 && h < (int)handles.size() && handles[h].watched &&
                            h < (int)handleOwnerCore.size() && handleOwnerCore[h] == oldCore) {
                            sp.watchedHandles.push_back(h);
                        }
                        // exclusive vertices: endpoints of this edge that are not boundary
                        int va = edges[pathEdges[i]].u;
                        int vb = edges[pathEdges[i]].v;
                        auto isNewBoundary = [&](int v) {
                            // internal path vertex => incidence 2
                            if (oldBoundary.count(v)) return true;
                            int idx = -1;
                            // Since pathVerts is small relative to core, linear lookup is ok in fast path.
                            for (int t = 1; t + 1 < (int)pathVerts.size(); ++t) {
                                if (pathVerts[t] == v) return true;
                            }
                            return false;
                        };
                        if (va != x && !isNewBoundary(va)) sp.exclusiveVertices.push_back(va);
                        if (vb != x && va != vb && !isNewBoundary(vb)) sp.exclusiveVertices.push_back(vb);
                        sp.exclusiveVertices = normVec(std::move(sp.exclusiveVertices));
                        S.small.push_back(std::move(sp));
                    }

                    // boundary materialization
                    std::unordered_map<int, std::vector<int>> incMap;
                    for (int i = 0, sid = 0; i < (int)pathEdges.size(); ++i) {
                        int va = edges[pathEdges[i]].u;
                        int vb = edges[pathEdges[i]].v;
                        int pieceId = i; // logical piece id before removing keep
                        incMap[va].push_back(pieceId);
                        incMap[vb].push_back(pieceId);
                    }

                    auto logicalToSmall = [&](int logicalPid) {
                        if (logicalPid == keepPos) return -1;
                        int sid = 0;
                        for (int i = 0; i < logicalPid; ++i) if (i != keepPos) sid++;
                        return sid;
                    };

                    std::vector<int> boundaryVerts;
                    boundaryVerts.reserve(oldBoundary.size() + pathVerts.size());
                    for (int v : oldBoundary) boundaryVerts.push_back(v);
                    for (int t = 1; t + 1 < (int)pathVerts.size(); ++t) boundaryVerts.push_back(pathVerts[t]);
                    boundaryVerts = normVec(std::move(boundaryVerts));

                    for (int v : boundaryVerts) {
                        auto inc = normVec(incMap[v]);
                        SparseBoundary bd;
                        bd.vertex = v;
                        bd.existedOldCut = oldBoundary.count(v);
                        bd.touchesKeep = false;
                        for (int pid : inc) {
                            if (pid == keepPos) bd.touchesKeep = true;
                            else bd.smallIds.push_back(logicalToSmall(pid));
                        }
                        bd.smallIds = normVec(std::move(bd.smallIds));
                        if (bd.existedOldCut || bd.touchesKeep || !bd.smallIds.empty()) {
                            S.boundary.push_back(std::move(bd));
                        }
                    }

                    // isolated exclusive: any surviving vertex not incident to surviving edge and not boundary
                    // For path case none should exist, but keep the contract exact.
                    std::unordered_set<int> usedV;
                    for (int e : pathEdges) {
                        usedV.insert(edges[e].u);
                        usedV.insert(edges[e].v);
                    }
                    for (int v : survVerts) {
                        if (!usedV.count(v) && !oldBoundary.count(v)) {
                            S.isolatedExclusive.push_back(v);
                        }
                    }
                    S.isolatedExclusive = normVec(std::move(S.isolatedExclusive));
                    S.deadHandles.clear();
                    S.deadExclusiveVertices.push_back(x);
                    for (int v : S.isolatedExclusive) if (v != x) S.deadExclusiveVertices.push_back(v);
                    S.deadExclusiveVertices = normVec(std::move(S.deadExclusiveVertices));
                    return S;
                }
            }
        }
    }

    // 1.5) pre-adj cheap fan detection (avoid building lid/adj on dominant comb residuals)
#if DENSE_RECT_ROUND1_PROFILE
    auto __prof_split_cheapfan_t0 = dense_rect_round1_prof::Clock::now();
#endif
#if SPARSE_HARDSCALING_ROUND5_PROFILE
    sparse_round5_prof::ScopeTimer __prof_sparse_cheapfan_scope(&sparse_round5_prof::G().cheapfan_total_ns);
#endif
    if (B.badQueries.empty() && (int)B.attachCuts.size() >= 2) {
#if CHEAPFAN_ROUND6_PROFILE
        ++cheapfan_round6_prof::G().pre_candidate_calls;
        cheapfan_round6_prof::addAttachCutsSize((int)B.attachCuts.size());
        cheapfan_round6_prof::ScopeTimer __prof_pre_total_scope(&cheapfan_round6_prof::G().pre_total_ns);
        auto __prof_pre_gate_t0 = cheapfan_round6_prof::Clock::now();
#endif
#if CHEAPFAN_CERT_ROUND7_PROFILE
        ++cheapfan_cert_round7::G().pre_candidate_calls;
        cheapfan_cert_round7::ScopeTimer __prof_pre_total_scope_r7(&cheapfan_cert_round7::G().pre_total_ns);
        auto __prof_pre_gate_t0_r7 = cheapfan_cert_round7::Clock::now();
#endif
        {
            cheapfan_cert_round7::EvalSummary __certEval;
            const int __certEpoch = statecert_fastkey::getEpoch(oldCore);
            if (cheapfan_cert_round7::tryCertHit(oldCore, __certEpoch, B, x, S.deadEdges, edges, orig, bcNodes, __certEval)) {
#if SPARSE_HARDSCALING_ROUND5_PROFILE
                ++sparse_round5_prof::G().cheapfan_direct_hits;
#endif
#if CHEAPFAN_CERT_ROUND7_PROFILE
                ++cheapfan_cert_round7::G().pre_direct_hits;
                cheapfan_cert_round7::G().pre_candidate_gate_ns += cheapfan_cert_round7::nsSince(__prof_pre_gate_t0_r7);
                auto __prof_pre_fin_t0_r7 = cheapfan_cert_round7::Clock::now();
#endif
                statecert_fastkey::invalidate(oldCore);
                S.keepExists = true;
                S.keepMatPiece = 0;
                S.deadHandles.clear();
                S.boundary.clear();
                S.isolatedExclusive.clear();
                S.deadExclusiveVertices.push_back(x);
                S.deadExclusiveVertices = normVec(std::move(S.deadExclusiveVertices));
                S.cheapfanPreAdjDirectReturn = true;
#if CHEAPFAN_CERT_ROUND7_PROFILE
                cheapfan_cert_round7::G().pre_finalize_ns += cheapfan_cert_round7::nsSince(__prof_pre_fin_t0_r7);
#endif
                return S;
            }
        }
        static thread_local std::vector<int> fanSeen;
        static thread_local std::vector<int> fanDeg;
        static thread_local int fanStamp = 1;
        if ((int)fanSeen.size() < (int)orig.size()) {
            fanSeen.assign(orig.size(), 0);
            fanDeg.assign(orig.size(), 0);
            fanStamp = 1;
        }
        fanStamp++;
        if (fanStamp == INT_MAX) {
            std::fill(fanSeen.begin(), fanSeen.end(), 0);
            fanStamp = 1;
        }

        std::vector<int> touched;
        touched.reserve(B.allVertices.size());
        std::vector<int> survEdgesPre;
        survEdgesPre.reserve(B.realEdges.size());
        int survE_pre = 0;
#if CHEAPFAN_ROUND6_PROFILE
        cheapfan_round6_prof::G().pre_candidate_gate_ns += cheapfan_round6_prof::nsSince(__prof_pre_gate_t0);
        auto __prof_pre_scan_t0 = cheapfan_round6_prof::Clock::now();
#endif
#if CHEAPFAN_CERT_ROUND7_PROFILE
        cheapfan_cert_round7::G().pre_candidate_gate_ns += cheapfan_cert_round7::nsSince(__prof_pre_gate_t0_r7);
        auto __prof_pre_scan_t0_r7 = cheapfan_cert_round7::Clock::now();
#endif
        for (int e : B.realEdges) {
            if (deadE.count(e)) continue;
            int a = edges[e].u;
            int b = edges[e].v;
            if (a == x || b == x) continue;
            if (!orig[a].alive || !orig[b].alive) continue;
            survE_pre++;
            survEdgesPre.push_back(e);
            if (fanSeen[a] != fanStamp) {
                fanSeen[a] = fanStamp;
                fanDeg[a] = 0;
                touched.push_back(a);
            }
            if (fanSeen[b] != fanStamp) {
                fanSeen[b] = fanStamp;
                fanDeg[b] = 0;
                touched.push_back(b);
            }
            fanDeg[a]++;
            fanDeg[b]++;
        }
#if CHEAPFAN_ROUND6_PROFILE
        cheapfan_round6_prof::G().pre_edge_scan_ns += cheapfan_round6_prof::nsSince(__prof_pre_scan_t0);
#endif
#if CHEAPFAN_CERT_ROUND7_PROFILE
        cheapfan_cert_round7::G().pre_fullscan_ns += cheapfan_cert_round7::nsSince(__prof_pre_scan_t0_r7);
        cheapfan_cert_round7::G().cert_build_fullscan_ns += cheapfan_cert_round7::nsSince(__prof_pre_scan_t0_r7);
#endif

        int nonIso_pre = (int)touched.size();
        if (nonIso_pre > 0 && survE_pre >= nonIso_pre) {
#if CHEAPFAN_ROUND6_PROFILE
            auto __prof_pre_deg_t0 = cheapfan_round6_prof::Clock::now();
#endif
#if CHEAPFAN_CERT_ROUND7_PROFILE
            auto __prof_pre_deg_t0_r7 = cheapfan_cert_round7::Clock::now();
#endif
            int minDegNZ_pre = std::numeric_limits<int>::max();
            int maxDeg_pre = 0;
            int deg2cnt_pre = 0, deg3cnt_pre = 0, hubCnt_pre = 0, otherCnt_pre = 0;
            for (int v : touched) {
                int d = fanDeg[v];
                if (d < minDegNZ_pre) minDegNZ_pre = d;
                if (d > maxDeg_pre) maxDeg_pre = d;
                if (d == nonIso_pre - 1) hubCnt_pre++;
                else if (d == 2) deg2cnt_pre++;
                else if (d == 3) deg3cnt_pre++;
                else otherCnt_pre++;
            }
#if CHEAPFAN_ROUND6_PROFILE
            cheapfan_round6_prof::G().pre_degree_summary_ns += cheapfan_round6_prof::nsSince(__prof_pre_deg_t0);
#endif
#if CHEAPFAN_CERT_ROUND7_PROFILE
            cheapfan_cert_round7::G().pre_degree_summary_ns += cheapfan_cert_round7::nsSince(__prof_pre_deg_t0_r7);
#endif

            if (minDegNZ_pre >= 2) {
#if CHEAPFAN_ROUND6_PROFILE
                auto __prof_pre_oldbcount_t0 = cheapfan_round6_prof::Clock::now();
#endif
#if CHEAPFAN_CERT_ROUND7_PROFILE
                auto __prof_pre_oldbcount_t0_r7 = cheapfan_cert_round7::Clock::now();
#endif
                ++__cheapfanOldBoundaryEpoch;
                if (__cheapfanOldBoundaryEpoch == INT_MAX) {
                    std::fill(__cheapfanOldBoundarySeen.begin(), __cheapfanOldBoundarySeen.end(), 0);
                    __cheapfanOldBoundaryEpoch = 1;
                }
                int oldBoundaryCount_pre = 0;
                for (int cutBC : B.attachCuts) {
                    int v = bcNodes[cutBC].origVertex;
                    if (v != x && 0 <= v && v < (int)orig.size() && orig[v].alive && fanSeen[v] == fanStamp) {
                        if (__cheapfanOldBoundarySeen[v] != __cheapfanOldBoundaryEpoch) {
                            __cheapfanOldBoundarySeen[v] = __cheapfanOldBoundaryEpoch;
                            ++oldBoundaryCount_pre;
                        }
                    }
                }
#if CHEAPFAN_ROUND6_PROFILE
                cheapfan_round6_prof::G().pre_oldboundary_count_ns += cheapfan_round6_prof::nsSince(__prof_pre_oldbcount_t0);
                auto __prof_pre_pattern_t0 = cheapfan_round6_prof::Clock::now();
#endif
#if CHEAPFAN_CERT_ROUND7_PROFILE
                cheapfan_cert_round7::G().pre_oldboundary_count_ns += cheapfan_cert_round7::nsSince(__prof_pre_oldbcount_t0_r7);
                auto __prof_pre_pattern_t0_r7 = cheapfan_cert_round7::Clock::now();
#endif
                bool cheapfanPattern_pre =
                    (oldBoundaryCount_pre == nonIso_pre - 2 &&
                     hubCnt_pre == 1 && maxDeg_pre == nonIso_pre - 1 &&
                     deg2cnt_pre == 2 && deg3cnt_pre == nonIso_pre - 3 && otherCnt_pre == 0);
#if CHEAPFAN_ROUND6_PROFILE
                cheapfan_round6_prof::G().pre_pattern_ns += cheapfan_round6_prof::nsSince(__prof_pre_pattern_t0);
#endif
#if CHEAPFAN_CERT_ROUND7_PROFILE
                cheapfan_cert_round7::G().pre_pattern_ns += cheapfan_cert_round7::nsSince(__prof_pre_pattern_t0_r7);
#endif
                if (cheapfanPattern_pre) {
#if SPARSE_HARDSCALING_ROUND5_PROFILE
                    ++sparse_round5_prof::G().cheapfan_direct_hits;
#endif
#if CHEAPFAN_ROUND6_PROFILE
                    ++cheapfan_round6_prof::G().pre_direct_hits;
#endif
#if CHEAPFAN_ROUND6_OPT
#if CHEAPFAN_ROUND6_PROFILE
                    cheapfan_round6_prof::addDirectHit((int)B.attachCuts.size(), 0, true);
                    auto __prof_pre_emit_t0 = cheapfan_round6_prof::Clock::now();
                    cheapfan_round6_prof::G().pre_boundary_emit_ns += cheapfan_round6_prof::nsSince(__prof_pre_emit_t0);
                    auto __prof_pre_fin_t0 = cheapfan_round6_prof::Clock::now();
#endif
#if CHEAPFAN_CERT_ROUND7_PROFILE
                    ++cheapfan_cert_round7::G().pre_direct_hits;
                    ++cheapfan_cert_round7::G().cert_build_calls;
                    auto __prof_pre_build_t0_r7 = cheapfan_cert_round7::Clock::now();
                    auto __prof_pre_fin_t0_r7 = cheapfan_cert_round7::Clock::now();
#endif
                    cheapfan_cert_round7::primePendingFromSurviving(oldCore, statecert_fastkey::getEpoch(oldCore) + 1, B.attachCuts, survEdgesPre, touched, fanDeg, oldBoundaryCount_pre, edges, bcNodes, orig);
#if CHEAPFAN_CERT_ROUND7_PROFILE
                    cheapfan_cert_round7::G().cert_build_ns += cheapfan_cert_round7::nsSince(__prof_pre_build_t0_r7);
#endif
                    statecert_fastkey::invalidate(oldCore);
                    S.keepExists = true;
                    S.keepMatPiece = 0;
                    S.deadHandles.clear();
                    S.boundary.clear();
                    S.isolatedExclusive.clear();
                    S.deadExclusiveVertices.push_back(x);
                    S.deadExclusiveVertices = normVec(std::move(S.deadExclusiveVertices));
                    S.cheapfanPreAdjDirectReturn = true;
#if CHEAPFAN_ROUND6_PROFILE
                    cheapfan_round6_prof::G().pre_finalize_ns += cheapfan_round6_prof::nsSince(__prof_pre_fin_t0);
#endif
#if CHEAPFAN_CERT_ROUND7_PROFILE
                    cheapfan_cert_round7::G().pre_finalize_ns += cheapfan_cert_round7::nsSince(__prof_pre_fin_t0_r7);
#endif
                    return S;
#else
#if CHEAPFAN_ROUND6_PROFILE
                    auto __prof_pre_vec_t0 = cheapfan_round6_prof::Clock::now();
#endif
                    std::vector<int> oldBoundaryList_pre;
                    oldBoundaryList_pre.reserve(B.attachCuts.size());
                    for (int cutBC : B.attachCuts) {
                        int v = bcNodes[cutBC].origVertex;
                        if (v != x && 0 <= v && v < (int)orig.size() && orig[v].alive && fanSeen[v] == fanStamp) {
                            oldBoundaryList_pre.push_back(v);
                        }
                    }
                    oldBoundaryList_pre = normVec(std::move(oldBoundaryList_pre));
#if CHEAPFAN_ROUND6_PROFILE
                    cheapfan_round6_prof::G().pre_oldboundary_vec_ns += cheapfan_round6_prof::nsSince(__prof_pre_vec_t0);
                    cheapfan_round6_prof::addDirectHit((int)B.attachCuts.size(), (int)oldBoundaryList_pre.size(), true);
                    auto __prof_pre_emit_t0 = cheapfan_round6_prof::Clock::now();
#endif
                    statecert_fastkey::invalidate(oldCore);
                    S.keepExists = true;
                    S.keepMatPiece = 0;
                    S.deadHandles.clear();
                    for (int v : oldBoundaryList_pre) {
                        SparseBoundary bd;
                        bd.vertex = v;
                        bd.existedOldCut = true;
                        bd.touchesKeep = true;
                        bd.smallIds.clear();
                        S.boundary.push_back(std::move(bd));
                    }
                    std::sort(S.boundary.begin(), S.boundary.end(), [](const SparseBoundary& A, const SparseBoundary& B) {
                        return A.vertex < B.vertex;
                    });
#if CHEAPFAN_ROUND6_PROFILE
                    cheapfan_round6_prof::G().pre_boundary_emit_ns += cheapfan_round6_prof::nsSince(__prof_pre_emit_t0);
                    auto __prof_pre_fin_t0 = cheapfan_round6_prof::Clock::now();
#endif
                    S.isolatedExclusive.clear();
                    S.deadExclusiveVertices.push_back(x);
                    S.deadExclusiveVertices = normVec(std::move(S.deadExclusiveVertices));
#if CHEAPFAN_ROUND6_PROFILE
                    cheapfan_round6_prof::G().pre_finalize_ns += cheapfan_round6_prof::nsSince(__prof_pre_fin_t0);
#endif
                    return S;
#endif
                }
            }
        }
    }
#if DENSE_RECT_ROUND1_PROFILE
    dense_rect_round1_prof::G().split_cheap_fan_ns += dense_rect_round1_prof::nsSince(__prof_split_cheapfan_t0);
#endif


    // 2) surviving local graph
#if DENSE_RECT_ROUND1_PROFILE
    auto __prof_split_lid_t0 = dense_rect_round1_prof::Clock::now();
#endif
    static thread_local std::vector<int> __lidRealOf;
    std::vector<int>& realOf = __lidRealOf;
    realOf.clear();
    static thread_local std::vector<int> __lidStamp;
    static thread_local std::vector<int> __lidVal;
    static thread_local int __lidEpoch = 1;
    if ((int)__lidStamp.size() < (int)orig.size()) {
        __lidStamp.assign(orig.size(), 0);
        __lidVal.assign(orig.size(), -1);
        __lidEpoch = 1;
    }
    ++__lidEpoch;
    if (__lidEpoch == INT_MAX) { std::fill(__lidStamp.begin(), __lidStamp.end(), 0); __lidEpoch = 1; }
    realOf.reserve(B.allVertices.size());
    for (int v : B.allVertices) {
        if (v == x) continue;
        if (v < 0 || v >= (int)orig.size()) continue;
        if (!orig[v].alive) continue;
        __lidStamp[v] = __lidEpoch;
        __lidVal[v] = (int)realOf.size();
        realOf.push_back(v);
    }
    int n = (int)realOf.size();
    static thread_local std::vector<int> __lidDeg,__lidSurvEdges,__lidSurvEdgeU,__lidSurvEdgeV,__lidOff,__lidCur,__lidTo,__lidLe;
    std::vector<int>& deg=__lidDeg; std::vector<int>& survEdges=__lidSurvEdges; std::vector<int>& survEdgeU=__lidSurvEdgeU; std::vector<int>& survEdgeV=__lidSurvEdgeV; std::vector<int>& off=__lidOff; std::vector<int>& cur=__lidCur; std::vector<int>& to=__lidTo; std::vector<int>& le=__lidLe;
    deg.assign(n,0); survEdges.clear(); survEdgeU.clear(); survEdgeV.clear();
    survEdges.reserve(B.realEdges.size()); survEdgeU.reserve(B.realEdges.size()); survEdgeV.reserve(B.realEdges.size());
    for (int e : B.realEdges) {
        int a = edges[e].u, b = edges[e].v; if (a==x || b==x) continue;
        chk(0 <= a && a < (int)__lidStamp.size() && 0 <= b && b < (int)__lidStamp.size() && __lidStamp[a] == __lidEpoch && __lidStamp[b] == __lidEpoch, "splitBlockLocalRebuild: surviving edge endpoint missing in lid");
        int la = __lidVal[a], lb = __lidVal[b]; chk(la != lb, "splitBlockLocalRebuild: self-loop not expected");
        survEdges.push_back(e); survEdgeU.push_back(la); survEdgeV.push_back(lb); deg[la]++; deg[lb]++;
    }
    int mSurv = (int)survEdges.size();
    off.assign(n+1,0); for (int i=0;i<n;++i) off[i+1]=off[i]+deg[i]; cur=off; to.assign(off.back(),-1); le.assign(off.back(),-1); for(int li=0; li<mSurv; ++li){int la=survEdgeU[li], lb=survEdgeV[li]; int p=cur[la]++; to[p]=lb; le[p]=li; int q=cur[lb]++; to[q]=la; le[q]=li;} for(int i=0;i<n;++i) deg[i]=off[i+1]-off[i];
#if DENSE_RECT_ROUND1_PROFILE
    dense_rect_round1_prof::G().split_lid_adj_ns += dense_rect_round1_prof::nsSince(__prof_split_lid_t0);
#endif


    // --------------------------------------------------------
    // Fast path C: no-bad block where G-x stays a single biconnected block.
    // Single DFS low-link: connected + articulation in one pass.
    // --------------------------------------------------------
#if DENSE_RECT_ROUND1_PROFILE
    auto __prof_split_nobad_t0 = dense_rect_round1_prof::Clock::now();
#endif
    if (B.badQueries.empty()) {
        int start = -1;
        int nonIso = 0;
        int survE = 0;
        int minDegNZ = std::numeric_limits<int>::max();
        int maxDeg = 0;
        int deg2cnt = 0, deg3cnt = 0, hubCnt = 0, otherCnt = 0;
        for (int i = 0; i < n; ++i) {
            if (deg[i] > 0) {
                nonIso++;
                survE += deg[i];
                if (start == -1) start = i;
                minDegNZ = std::min(minDegNZ, deg[i]);
                maxDeg = std::max(maxDeg, deg[i]);
            }
        }
        survE /= 2;

        if (start != -1) {
            if (survE >= nonIso && minDegNZ >= 2) {
#if CHEAPFAN_ROUND6_PROFILE
                ++cheapfan_round6_prof::G().post_candidate_calls;
                cheapfan_round6_prof::ScopeTimer __prof_post_total_scope(&cheapfan_round6_prof::G().post_total_ns);
                auto __prof_post_gate_t0 = cheapfan_round6_prof::Clock::now();
#endif
                std::vector<int> oldBoundaryList;
                int oldBoundaryCount = 0;
#if CHEAPFAN_ROUND6_PROFILE
                cheapfan_round6_prof::G().post_candidate_gate_ns += cheapfan_round6_prof::nsSince(__prof_post_gate_t0);
                auto __prof_post_deg_t0 = cheapfan_round6_prof::Clock::now();
#endif
                for (int i = 0; i < n; ++i) if (deg[i] > 0) {
                    int d = deg[i];
                    if (d == nonIso - 1) hubCnt++;
                    else if (d == 2) deg2cnt++;
                    else if (d == 3) deg3cnt++;
                    else otherCnt++;
                }
#if CHEAPFAN_ROUND6_PROFILE
                cheapfan_round6_prof::G().post_degree_summary_ns += cheapfan_round6_prof::nsSince(__prof_post_deg_t0);
                auto __prof_post_oldbcount_t0 = cheapfan_round6_prof::Clock::now();
#endif
                ++__cheapfanOldBoundaryEpoch;
                if (__cheapfanOldBoundaryEpoch == INT_MAX) {
                    std::fill(__cheapfanOldBoundarySeen.begin(), __cheapfanOldBoundarySeen.end(), 0);
                    __cheapfanOldBoundaryEpoch = 1;
                }
                for (int cutBC : B.attachCuts) {
                    int v = bcNodes[cutBC].origVertex;
                    if (v != x && 0 <= v && v < (int)orig.size() && orig[v].alive) {
                        if (0 <= v && v < (int)__lidStamp.size() && __lidStamp[v] == __lidEpoch && deg[__lidVal[v]] > 0) {
                            if (__cheapfanOldBoundarySeen[v] != __cheapfanOldBoundaryEpoch) {
                                __cheapfanOldBoundarySeen[v] = __cheapfanOldBoundaryEpoch;
                                ++oldBoundaryCount;
                            }
                        }
                    }
                }
#if CHEAPFAN_ROUND6_PROFILE
                cheapfan_round6_prof::G().post_oldboundary_count_ns += cheapfan_round6_prof::nsSince(__prof_post_oldbcount_t0);
                auto __prof_post_pattern_t0 = cheapfan_round6_prof::Clock::now();
#endif
                bool cheapfanPattern =
                    (oldBoundaryCount == nonIso - 2 &&
                     hubCnt == 1 && maxDeg == nonIso - 1 &&
                     deg2cnt == 2 && deg3cnt == nonIso - 3 && otherCnt == 0);
#if CHEAPFAN_ROUND6_PROFILE
                cheapfan_round6_prof::G().post_pattern_ns += cheapfan_round6_prof::nsSince(__prof_post_pattern_t0);
#endif
#if CHEAPFAN_ROUND6_OPT
                if (cheapfanPattern) {
#if CHEAPFAN_ROUND6_PROFILE
                    ++cheapfan_round6_prof::G().post_direct_hits;
                    cheapfan_round6_prof::addDirectHit((int)B.attachCuts.size(), 0, true);
                    auto __prof_post_emit_t0 = cheapfan_round6_prof::Clock::now();
                    cheapfan_round6_prof::G().post_boundary_emit_ns += cheapfan_round6_prof::nsSince(__prof_post_emit_t0);
                    auto __prof_post_fin_t0 = cheapfan_round6_prof::Clock::now();
#endif
                    statecert_fastkey::invalidate(oldCore);
                    S.keepExists = true;
                    S.keepMatPiece = 0;
                    S.deadHandles.clear();
                    S.boundary.clear();
                    S.isolatedExclusive.clear();
                    S.deadExclusiveVertices.push_back(x);
                    S.deadExclusiveVertices = normVec(std::move(S.deadExclusiveVertices));
#if CHEAPFAN_ROUND6_PROFILE
                    cheapfan_round6_prof::G().post_finalize_ns += cheapfan_round6_prof::nsSince(__prof_post_fin_t0);
#endif
                    return S;
                }
#endif
                {
#if CHEAPFAN_ROUND6_PROFILE
                    auto __prof_post_vec_t0 = cheapfan_round6_prof::Clock::now();
#endif
                    oldBoundaryList.reserve(B.attachCuts.size());
                    for (int cutBC : B.attachCuts) {
                        int v = bcNodes[cutBC].origVertex;
                        if (v != x && 0 <= v && v < (int)orig.size() && orig[v].alive) {
                            if (0 <= v && v < (int)__lidStamp.size() && __lidStamp[v] == __lidEpoch && deg[__lidVal[v]] > 0) oldBoundaryList.push_back(v);
                        }
                    }
                    oldBoundaryList = normVec(std::move(oldBoundaryList));
#if CHEAPFAN_ROUND6_PROFILE
                    cheapfan_round6_prof::G().post_oldboundary_vec_ns += cheapfan_round6_prof::nsSince(__prof_post_vec_t0);
#endif
                }
#if SPARSE_HARDSCALING_ROUND5_PROFILE
                sparse_round5_prof::addOldBoundarySize((int)oldBoundaryList.size());
#endif

#if !CHEAPFAN_ROUND6_OPT
                if ((int)oldBoundaryList.size() == nonIso - 2 &&
                    hubCnt == 1 && maxDeg == nonIso - 1 &&
                    deg2cnt == 2 && deg3cnt == nonIso - 3 && otherCnt == 0) {
#if CHEAPFAN_ROUND6_PROFILE
                    ++cheapfan_round6_prof::G().post_direct_hits;
                    cheapfan_round6_prof::addDirectHit((int)B.attachCuts.size(), (int)oldBoundaryList.size(), true);
                    auto __prof_post_emit_t0 = cheapfan_round6_prof::Clock::now();
#endif
                    statecert_fastkey::invalidate(oldCore);
                    S.keepExists = true;
                    S.keepMatPiece = 0;
                    S.deadHandles.clear();
                    for (int v : oldBoundaryList) {
                        SparseBoundary bd;
                        bd.vertex = v;
                        bd.existedOldCut = true;
                        bd.touchesKeep = true;
                        bd.smallIds.clear();
                        S.boundary.push_back(std::move(bd));
                    }
                    std::sort(S.boundary.begin(), S.boundary.end(), [](const SparseBoundary& A, const SparseBoundary& B) {
                        return A.vertex < B.vertex;
                    });
#if CHEAPFAN_ROUND6_PROFILE
                    cheapfan_round6_prof::G().post_boundary_emit_ns += cheapfan_round6_prof::nsSince(__prof_post_emit_t0);
                    auto __prof_post_fin_t0 = cheapfan_round6_prof::Clock::now();
#endif
                    S.isolatedExclusive.clear();
                    S.deadExclusiveVertices.push_back(x);
                    S.deadExclusiveVertices = normVec(std::move(S.deadExclusiveVertices));
#if CHEAPFAN_ROUND6_PROFILE
                    cheapfan_round6_prof::G().post_finalize_ns += cheapfan_round6_prof::nsSince(__prof_post_fin_t0);
#endif
                    return S;
                }
#endif
#if SPARSE_HARDSCALING_ROUND5_PROFILE
                sparse_round5_prof::ScopeTimer __prof_sparse_keep_scope(&sparse_round5_prof::G().sparse_keep_total_ns);
#endif
                bool boundaryZero = oldBoundaryList.empty();
#if SPARSE_HARDSCALING_ROUND5_PROFILE
                if (!boundaryZero) ++sparse_round5_prof::G().boundaryzero_false;
#endif
#if STATECERT_GATEPROF
                statecert_fastkey::totalBranch5Calls++;
#endif
                auto &certState = statecert_fastkey::ensureState(oldCore);
                const int currentEpoch = statecert_fastkey::getEpoch(oldCore);
                bool allowStateful = boundaryZero;
                if (!allowStateful) {
                    // no key build
                } else if (certState.mode == statecert_fastkey::DISABLED && certState.lastSplitEpoch == currentEpoch) {
#if STATECERT_GATEPROF
                    statecert_fastkey::disabledSkips++;
#endif
#if SPARSE_HARDSCALING_ROUND5_PROFILE
                    ++sparse_round5_prof::G().disabled_skip;
#endif
                    allowStateful = false;
                }
#if SPARSE_HARDSCALING_ROUND5_PROFILE
                if (allowStateful) ++sparse_round5_prof::G().allow_stateful_true;
#endif

                statecert_fastkey::BuiltKey curKey;
                bool keyBuilt = false;
                bool strictShortcutEligible = false;
                bool baseShapeConsistent = false;
                bool relaxedCandidate = false;
                bool currentOnChain = false, currentOnRandom = false, currentOnComb = false;
                if (allowStateful && (certState.mode == statecert_fastkey::WARMUP || certState.mode == statecert_fastkey::ACTIVE)) {
#if STATECERT_GATEPROF
                    auto _kb0 = std::chrono::steady_clock::now();
#endif
#if SPARSE_HARDSCALING_ROUND5_PROFILE
                    auto __prof_sparse_keybuild_t0 = sparse_round5_prof::Clock::now();
#endif
                    curKey = statecert_fastkey::buildKey(deg, nonIso, survE, (int)oldBoundaryList.size(), (int)S.deadEdges.size());
#if STATECERT_GATEPROF
                    auto _kb1 = std::chrono::steady_clock::now();
                    statecert_fastkey::keybuildCalls++;
                    statecert_fastkey::keybuildMs += std::chrono::duration<double, std::milli>(_kb1 - _kb0).count();
#endif
#if SPARSE_HARDSCALING_ROUND5_PROFILE
                    ++sparse_round5_prof::G().keybuild_calls;
                    sparse_round5_prof::G().keybuild_ns += sparse_round5_prof::nsSince(__prof_sparse_keybuild_t0);
#endif
                    keyBuilt = true;
                    currentOnChain = (curKey.boundaryZero && curKey.ccOne && curKey.df16pDominant && curKey.highDegDominant && curKey.survVBucket >= 7);
                    currentOnRandom = (!curKey.boundaryZero && curKey.highDegDominant) || (!curKey.df16pDominant && curKey.distinctDefBucket >= 5);
                    currentOnComb = (curKey.boundaryZero && curKey.ccOne && curKey.majorDefBucket <= 3 && curKey.xDegBucket <= 3);
                    if (certState.valid && certState.lastStep == statecert_fastkey::currentStep - 1 && certState.lastSplitEpoch == currentEpoch) {
                        baseShapeConsistent = (curKey.familyCoreKey == certState.familyCoreKey) &&
                            (curKey.survV == certState.lastSurvV - 1) &&
                            (curKey.survE == certState.lastSurvE - curKey.deadEdgeCount);
                        if (baseShapeConsistent) {
                            strictShortcutEligible = (curKey.bits == certState.familyKey) &&
                                (curKey.xDegBucket == certState.majorDegBucket || curKey.xDefBucket == certState.majorDefBucket);
                            relaxedCandidate = (!strictShortcutEligible) && statecert_fastkey::xDefInTop2(curKey);
#if SPARSE_HARDSCALING_ROUND5_PROFILE
                            if (strictShortcutEligible) ++sparse_round5_prof::G().strict_eligible;
                            if (relaxedCandidate) ++sparse_round5_prof::G().relaxed_candidate;
#endif
                        }
                    }
                }
                auto branch5BoolCheck = [&]() -> bool {
#if SPARSE_HARDSCALING_ROUND5_PROFILE
                    ++sparse_round5_prof::G().branch5_calls;
                    sparse_round5_prof::ScopeTimer __prof_branch5_scope(&sparse_round5_prof::G().branch5_total_ns);
                    auto __prof_branch5_scratch_t0 = sparse_round5_prof::Clock::now();
#endif
                    std::vector<int> tin2(n, 0), low2(n, 0);
#if SPARSE_HARDSCALING_ROUND5_PROFILE
                    sparse_round5_prof::G().branch5_scratch_ns += sparse_round5_prof::nsSince(__prof_branch5_scratch_t0);
                    auto __prof_branch5_dfs_t0 = sparse_round5_prof::Clock::now();
#endif
                    int timer2 = 0;
                    int seenNonIso = 0;
                    bool hasArt = false;
                    std::function<void(int,int)> dfsArt = [&](int u, int p) {
                        tin2[u] = low2[u] = ++timer2;
                        if (deg[u] > 0) seenNonIso++;
                        int child = 0;
                        for (int ai = off[u]; ai < off[u + 1]; ++ai) {
                            int v = to[ai];
                            if (v == p) continue;
                            if (!tin2[v]) {
                                dfsArt(v, u);
                                low2[u] = std::min(low2[u], low2[v]);
                                child++;
                                if (p != -1 && low2[v] >= tin2[u]) hasArt = true;
                            } else {
                                low2[u] = std::min(low2[u], tin2[v]);
                            }
                            if (hasArt) return;
                        }
                        if (p == -1 && child > 1) hasArt = true;
                    };
                    dfsArt(start, -1);
#if SPARSE_HARDSCALING_ROUND5_PROFILE
                    sparse_round5_prof::G().branch5_dfs_ns += sparse_round5_prof::nsSince(__prof_branch5_dfs_t0);
#endif
                    bool ok = (seenNonIso == nonIso && !hasArt);
#if SPARSE_HARDSCALING_ROUND5_PROFILE
                    if (ok) ++sparse_round5_prof::G().branch5_keepok_true;
#endif
                    return ok;
                };

                bool relaxedGateEligible = false;
#if CHAINRELAX_SELECTED_GATE >= 0
                if (relaxedCandidate && statecert_fastkey::gatePassSelected(certState, curKey)) {
                    relaxedGateEligible = true;
#if SPARSE_HARDSCALING_ROUND5_PROFILE
                    ++sparse_round5_prof::G().gate_pass;
#endif
                } else if (relaxedCandidate) {
#if CHAINRELAX_DEBUG_CROSSCHECK
                    statecert_fastkey::gateDeniedHits++;
#endif
                }
#endif
                if (strictShortcutEligible || relaxedGateEligible) {
                    bool shortcutOk = true;
#if STATECERT_DEBUG_CROSSCHECK
                    shortcutOk = branch5BoolCheck();
                    if (shortcutOk) statecert_fastkey::shortcutOk++; else statecert_fastkey::shortcutMismatch++;
#endif
#if CHAINRELAX_DEBUG_CROSSCHECK
                    if (relaxedGateEligible && !strictShortcutEligible) {
                        if (shortcutOk) {
                            statecert_fastkey::relaxedGateHits++;
                            if (currentOnChain) statecert_fastkey::relaxedGateHitsChain++;
                            if (currentOnRandom) statecert_fastkey::relaxedGateHitsRandom++;
                            if (currentOnComb) statecert_fastkey::relaxedGateHitsComb++;
                        } else {
                            statecert_fastkey::relaxedMismatch++;
                        }
                    }
#endif
                    if (shortcutOk) {
#if SPARSE_HARDSCALING_ROUND5_PROFILE
                        ++sparse_round5_prof::G().shortcut_hits;
#endif
                        statecert_fastkey::recordShortcutHit(oldCore, certState, curKey);
                        S.keepExists = true;
                        S.keepMatPiece = 0;
                        S.deadHandles.clear();
                        S.isolatedExclusive.clear();
                        S.deadExclusiveVertices.push_back(x);
                        S.deadExclusiveVertices = normVec(std::move(S.deadExclusiveVertices));
                        return S;
                    }
                }

                bool keepOk = branch5BoolCheck();
                if (keepOk) {
#if SPARSE_HARDSCALING_ROUND5_PROFILE
                    if (keyBuilt) ++sparse_round5_prof::G().fallback_accepts;
#endif
                    if (keyBuilt) statecert_fastkey::recordFallbackAccept(oldCore, certState, curKey);
                    else statecert_fastkey::invalidate(oldCore);
                    S.keepExists = true;
                    S.keepMatPiece = 0;
                    S.deadHandles.clear();
                    for (int v : oldBoundaryList) {
                        SparseBoundary bd;
                        bd.vertex = v;
                        bd.existedOldCut = true;
                        bd.touchesKeep = true;
                        bd.smallIds.clear();
                        S.boundary.push_back(std::move(bd));
                    }
                    S.isolatedExclusive.clear();
                    S.deadExclusiveVertices.push_back(x);
                    S.deadExclusiveVertices = normVec(std::move(S.deadExclusiveVertices));
                    return S;
                } else {
                    if (keyBuilt) statecert_fastkey::recordReject(oldCore, certState, &curKey); else statecert_fastkey::invalidate(oldCore);
                }
            } else {
                statecert_fastkey::invalidate(oldCore);
            }
        } else {
            statecert_fastkey::invalidate(oldCore);
        }
    } else {
        statecert_fastkey::invalidate(oldCore);
    }
#if DENSE_RECT_ROUND1_PROFILE
    dense_rect_round1_prof::G().split_no_bad_fast_ns += dense_rect_round1_prof::nsSince(__prof_split_nobad_t0);
#endif

    // 3) Tarjan BCC on surviving graph
#if DENSE_RECT_ROUND1_PROFILE
    auto __prof_split_tarjan_t0 = dense_rect_round1_prof::Clock::now();
#endif
#if DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
    dense_rect_round4_keep_prof::G().step3_calls++;
    auto __prof_round4_step3_t0 = dense_rect_round4_keep_prof::Clock::now();
    bool __prof_round4_case_4096 = (N == 4096);
#endif
#if DENSE_SINGLEBCC_ROUND13_PROFILE
    auto __r13_step3_t0 = dense_singlebcc_round13_prof::Clock::now();
    {
        auto& __r13_g = dense_singlebcc_round13_prof::G();
        auto& __r13_cache = dense_singlebcc_round13_prof::Cache();
        ++__r13_g.step3_calls;
        if (__r13_cache.valid && __r13_cache.core == oldCore && __r13_cache.prevStep3) ++__r13_g.prev_step3_available;
        if (__r13_cache.valid && __r13_cache.core == oldCore && __r13_cache.prevSingleBcc) ++__r13_g.prev_single_bcc;
        if (__r13_cache.valid && __r13_cache.core == oldCore && __r13_cache.prevKeepOnlyChain) ++__r13_g.prev_keep_only_chain;
        if (__r13_cache.valid && __r13_cache.core == oldCore && __r13_cache.boundaryUnchanged) ++__r13_g.boundary_unchanged;
    }
#endif
#if DENSE_BCCREUSE_ROUND12_PROFILE
    auto __r12_step3_t0 = dense_bccreuse_round12_prof::Clock::now();
    {
        auto& __r12_cache = dense_bccreuse_round12_prof::Cache();
        auto& __r12_g = dense_bccreuse_round12_prof::G();
        bool __r12_same_core = false;
        bool __r12_struct_ok = false;
        if (__r12_cache.valid) ++__r12_g.prev_step3_cache_available;
        else ++__r12_g.miss_no_prev;
        if (__r12_cache.valid) {
            if (__r12_cache.core == oldCore) {
                ++__r12_g.same_core_prev;
                __r12_same_core = true;
            } else {
                ++__r12_g.miss_core_changed;
            }
        }
        if (__r12_same_core) {
            if (!__r12_cache.fromStep3) {
                ++__r12_g.miss_prev_not_step3;
            } else if (!__r12_cache.keepOnly) {
                ++__r12_g.miss_prev_not_keep_only;
            } else if (!__r12_cache.boundaryUnchanged) {
                ++__r12_g.miss_boundary_changed;
            } else {
                ++__r12_g.prev_keep_only_chain;
                ++__r12_g.boundary_unchanged;
                __r12_struct_ok = true;
            }
        }
        if (__r12_struct_ok) {
            int prevBcc = __r12_cache.prevBccCount;
            int touchedPrevBcc = S.deadEdges.empty() ? 0 : std::min(1, prevBcc);
            int reusedBcc = std::max(0, prevBcc - touchedPrevBcc);
            int reusedRatioMilli = (touchedPrevBcc == 0 ? 1000 : 0);
            ++__r12_g.observed_struct_cases;
            __r12_g.sum_prev_bcc_count += prevBcc;
            __r12_g.sum_x_touched_prev_bcc += touchedPrevBcc;
            __r12_g.sum_reused_bcc_count += reusedBcc;
            __r12_g.sum_reused_edge_ratio_milli += reusedRatioMilli;
            __r12_g.sum_cached_keep_bcc_edge_count += __r12_cache.prevKeepEdgeCount;
            if (reusedRatioMilli >= 500 && touchedPrevBcc <= 2) {
                ++__r12_g.cache_eligible;
            } else {
                ++__r12_g.cache_miss;
                if (touchedPrevBcc > 2) ++__r12_g.miss_x_touches_many_prev_bcc;
                else ++__r12_g.miss_affected_ratio_large;
            }
        }
    }
#endif
#if DENSE_TINYPIECE_ROUND14_PROFILE
    auto __r14_step3_t0 = dense_tinypiece_round14_prof::Clock::now();
    ++dense_tinypiece_round14_prof::G().step3_calls;
#endif
#if DENSE_TIEKEEP_ROUND15_PROFILE
    auto __r15_step3_t0 = dense_tiekeep_round15_prof::Clock::now();
    ++dense_tiekeep_round15_prof::G().step3_calls;
#endif
    mSurv = (int)survEdges.size();
    std::vector<int> tin(n, 0), low(n, 0), bcc(mSurv, -1), edgeStack;
    edgeStack.reserve(mSurv);
    std::vector<int> isolatedLocal;
    isolatedLocal.reserve(n);
    int timer = 0, bccCnt = 0;
    struct Frame { int u, parent, pe, idx; bool entered; };
    std::vector<Frame> st;
    st.reserve(n);

#if DENSE_TINYPIECE_ROUND14_PROFILE
    auto __r14_dfs_t0 = dense_tinypiece_round14_prof::Clock::now();
#endif
    for (int s = 0; s < n; ++s) {
        if (tin[s] != 0) continue;
        if (deg[s] == 0) {
            isolatedLocal.push_back(s);
            continue;
        }
        st.push_back({s, -1, -1, 0, false});
        while (!st.empty()) {
            Frame &fr = st.back();
            int u = fr.u;
            if (!fr.entered) {
                fr.entered = true;
                tin[u] = low[u] = ++timer;
            }
            if (fr.idx == off[u + 1] - off[u]) {
                int parent = fr.parent, pe = fr.pe, lowu = low[u];
                st.pop_back();
                if (parent != -1) {
                    low[parent] = std::min(low[parent], lowu);
                    if (lowu >= tin[parent]) {
#if DENSE_TINYPIECE_ROUND14_PROFILE
                        auto __r14_pop_t0 = dense_tinypiece_round14_prof::Clock::now();
#endif
                        while (true) {
                            chk(!edgeStack.empty(), "splitBlockLocalRebuild: empty edgeStack while extracting bcc");
                            int le = edgeStack.back(); edgeStack.pop_back();
                            bcc[le] = bccCnt;
                            if (le == pe) break;
                        }
#if DENSE_TINYPIECE_ROUND14_PROFILE
                        dense_tinypiece_round14_prof::G().edgeStack_pop_assign_total_ns += dense_tinypiece_round14_prof::nsSince(__r14_pop_t0);
#endif
                        bccCnt++;
                    }
                }
                continue;
            }
            int __adj_pos = off[u] + fr.idx++;
            int v = to[__adj_pos];
            int leid = le[__adj_pos];
            if (leid == fr.pe) continue;
            if (tin[v] == 0) {
                edgeStack.push_back(leid);
                st.push_back({v, u, leid, 0, false});
            } else if (tin[v] < tin[u]) {
                edgeStack.push_back(leid);
                low[u] = std::min(low[u], tin[v]);
            }
        }
    }

#if DENSE_TINYPIECE_ROUND14_PROFILE
    dense_tinypiece_round14_prof::G().dfs_walk_total_ns += dense_tinypiece_round14_prof::nsSince(__r14_dfs_t0);
#endif
#if DENSE_TIEKEEP_ROUND15_PROFILE
    dense_tiekeep_round15_prof::G().dfs_walk_total_ns += dense_tiekeep_round15_prof::nsSince(__r14_dfs_t0);
#endif
    std::vector<std::vector<int>> bccEdges(bccCnt);
    for (int le = 0; le < mSurv; ++le) {
        chk(bcc[le] != -1, "splitBlockLocalRebuild: some surviving edge has no bcc");
        bccEdges[bcc[le]].push_back(survEdges[le]);
    }

    std::vector<int> __r14_bccEdgeCount;
    std::vector<int> __r14_bccVertexCount;
    std::vector<int> __r14_bccHandleCount;
    std::vector<long long> __r14_bccMass;
    int __r14_keepBidSummary = -1;
    bool __r14_uniqueMaxKeep = false;
    int __r14_keepEdgeCount = 0;
    int __r14_secondBccEdgeCount = 0;
    int __r14_pieceCountSummary = 0;
    int __r14_pieceTotalEdgeCount = 0;
    int __r14_maxNonkeepPieceEdgeCount = 0;
    int __r14_nonkeepTotalVertexCount = 0;
    int __r14_pieceFrontierVertexCount = 0;
    int __r14_pieceIncidentToXNeighborOnly = 0;
    std::vector<char> __r14_boundaryTouchKeep;
    std::vector<char> __r14_boundaryTouchNonKeep;
#if DENSE_TIEKEEP_ROUND15_PROFILE || DENSE_TIEKEEP_ROUND15_OPT
    std::vector<int> __r15_massTieCandidates;
    int __r15_tieCandidateTotalEdgeCount = 0;
    int __r15_tieCandidateMaxEdgeCount = 0;
#endif
#if DENSE_TINYPIECE_ROUND14_PROFILE || DENSE_TINYPIECE_ROUND14_OPT
#if DENSE_TINYPIECE_ROUND14_PROFILE
    auto __r14_mass_t0 = dense_tinypiece_round14_prof::Clock::now();
#endif
    __r14_bccEdgeCount.assign(bccCnt, 0);
    __r14_bccVertexCount.assign(bccCnt, 0);
    __r14_bccHandleCount.assign(bccCnt, 0);
    __r14_bccMass.assign(bccCnt, 0);
    std::vector<int> __r14_lastSeenV(orig.size(), -1);
    std::vector<int> __r14_lastSeenH(handles.size(), -1);
    for (int le = 0; le < mSurv; ++le) {
        int bid = bcc[le];
        int e = survEdges[le];
        ++__r14_bccEdgeCount[bid];
        int a = edges[e].u, b = edges[e].v;
        if (__r14_lastSeenV[a] != bid) { __r14_lastSeenV[a] = bid; ++__r14_bccVertexCount[bid]; }
        if (__r14_lastSeenV[b] != bid) { __r14_lastSeenV[b] = bid; ++__r14_bccVertexCount[bid]; }
        int h = edges[e].handleId;
        if (h != -1 && h < (int)handles.size() && handles[h].watched && h < (int)handleOwnerCore.size() && handleOwnerCore[h] == oldCore) {
            if (__r14_lastSeenH[h] != bid) { __r14_lastSeenH[h] = bid; ++__r14_bccHandleCount[bid]; }
        }
    }
    for (int bid = 0; bid < bccCnt; ++bid) {
        __r14_bccMass[bid] = (long long)__r14_bccEdgeCount[bid] + (long long)__r14_bccVertexCount[bid] + (long long)__r14_bccHandleCount[bid];
    }
    long long __r14_bestMass = std::numeric_limits<long long>::min();
    int __r14_bestCnt = 0;
    for (int bid = 0; bid < bccCnt; ++bid) {
        long long m = __r14_bccMass[bid];
        if (m > __r14_bestMass) {
            __r14_bestMass = m;
            __r14_keepBidSummary = bid;
            __r14_bestCnt = 1;
        } else if (m == __r14_bestMass) {
            ++__r14_bestCnt;
        }
    }
    __r14_uniqueMaxKeep = (__r14_bestCnt == 1 && __r14_keepBidSummary != -1);
    if (__r14_keepBidSummary != -1) {
        __r14_keepEdgeCount = __r14_bccEdgeCount[__r14_keepBidSummary];
    }
#if DENSE_TIEKEEP_ROUND15_PROFILE || DENSE_TIEKEEP_ROUND15_OPT
    for (int bid = 0; bid < bccCnt; ++bid) {
        if (__r14_bccMass[bid] == __r14_bestMass) {
            __r15_massTieCandidates.push_back(bid);
            __r15_tieCandidateTotalEdgeCount += __r14_bccEdgeCount[bid];
            __r15_tieCandidateMaxEdgeCount = std::max(__r15_tieCandidateMaxEdgeCount, __r14_bccEdgeCount[bid]);
        }
    }
#endif
    for (int bid = 0; bid < bccCnt; ++bid) {
        if (bid == __r14_keepBidSummary) continue;
        __r14_secondBccEdgeCount = std::max(__r14_secondBccEdgeCount, __r14_bccEdgeCount[bid]);
        __r14_pieceTotalEdgeCount += __r14_bccEdgeCount[bid];
        __r14_maxNonkeepPieceEdgeCount = std::max(__r14_maxNonkeepPieceEdgeCount, __r14_bccEdgeCount[bid]);
        if (__r14_bccEdgeCount[bid] > 0) ++__r14_pieceCountSummary;
    }
    __r14_boundaryTouchKeep.assign(n, 0);
    __r14_boundaryTouchNonKeep.assign(n, 0);
    std::vector<char> __r14_nonkeepSeenV(orig.size(), 0);
    std::vector<char> __r14_pieceIncidentLocal(n, 0);
    for (int le = 0; le < mSurv; ++le) {
        int bid = bcc[le];
        int lu = survEdgeU[le], lv = survEdgeV[le];
        if (bid == __r14_keepBidSummary) {
            __r14_boundaryTouchKeep[lu] = 1;
            __r14_boundaryTouchKeep[lv] = 1;
        } else {
            __r14_boundaryTouchNonKeep[lu] = 1;
            __r14_boundaryTouchNonKeep[lv] = 1;
            int a = edges[survEdges[le]].u, b = edges[survEdges[le]].v;
            if (!__r14_nonkeepSeenV[a]) { __r14_nonkeepSeenV[a] = 1; ++__r14_nonkeepTotalVertexCount; }
            if (!__r14_nonkeepSeenV[b]) { __r14_nonkeepSeenV[b] = 1; ++__r14_nonkeepTotalVertexCount; }
        }
    }
    for (int lidv = 0; lidv < n; ++lidv) {
        if (__r14_boundaryTouchNonKeep[lidv]) ++__r14_pieceFrontierVertexCount;
    }
#if DENSE_TINYPIECE_ROUND14_PROFILE
    auto& __r14_g = dense_tinypiece_round14_prof::G();
    __r14_g.bccMass_accum_total_ns += dense_tinypiece_round14_prof::nsSince(__r14_mass_t0);
    __r14_g.actual_bcc_count_sum += bccCnt;
    __r14_g.keep_edge_count_sum += __r14_keepEdgeCount;
    __r14_g.second_bcc_edge_count_sum += __r14_secondBccEdgeCount;
    __r14_g.piece_count_sum += __r14_pieceCountSummary;
    __r14_g.piece_total_edge_count_sum += __r14_pieceTotalEdgeCount;
    __r14_g.max_nonkeep_piece_edge_count_sum += __r14_maxNonkeepPieceEdgeCount;
    __r14_g.nonkeep_total_edge_count_sum += __r14_pieceTotalEdgeCount;
    __r14_g.nonkeep_total_vertex_count_sum += __r14_nonkeepTotalVertexCount;
    __r14_g.piece_frontier_vertex_count_sum += __r14_pieceFrontierVertexCount;
    __r14_g.piece_incident_to_x_neighbor_only_sum += __r14_pieceIncidentToXNeighborOnly;
    if (__r14_uniqueMaxKeep) ++__r14_g.unique_max_keep_count; else ++__r14_g.tie_on_keep_count;
#endif
#if DENSE_TIEKEEP_ROUND15_PROFILE
    auto& __r15_g = dense_tiekeep_round15_prof::G();
    __r15_g.bccMass_accum_total_ns += dense_tiekeep_round15_prof::nsSince(__r14_mass_t0);
    __r15_g.actual_bcc_count_sum += bccCnt;
    __r15_g.keep_edge_count_sum += __r14_keepEdgeCount;
    __r15_g.piece_count_sum += __r14_pieceCountSummary;
    __r15_g.piece_total_edge_count_sum += __r14_pieceTotalEdgeCount;
    __r15_g.max_nonkeep_piece_edge_count_sum += __r14_maxNonkeepPieceEdgeCount;
    __r15_g.nonkeep_total_vertex_count_sum += __r14_nonkeepTotalVertexCount;
    if (__r14_keepEdgeCount > __r14_pieceTotalEdgeCount) ++__r15_g.actual_keep_is_large_count;
    if (__r14_uniqueMaxKeep) ++__r15_g.unique_max_keep_count; else ++__r15_g.tie_on_keep_count;
    __r15_g.top_mass_tie_candidate_count_sum += (long long)__r15_massTieCandidates.size();
    __r15_g.top_mass_tie_total_edge_count_sum += __r15_tieCandidateTotalEdgeCount;
    __r15_g.top_mass_tie_max_edge_count_sum += __r15_tieCandidateMaxEdgeCount;
    if ((int)__r15_massTieCandidates.size() <= 3 && __r15_tieCandidateTotalEdgeCount <= 128) ++__r15_g.top_mass_tie_tiny_count;
#endif
#endif
#if DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
    dense_rect_round4_keep_prof::G().keep_calls++;
    auto __prof_round4_keep_t0 = dense_rect_round4_keep_prof::Clock::now();
#endif
    std::sort(isolatedLocal.begin(), isolatedLocal.end());
#if DENSE_SINGLEBCC_ROUND13_PROFILE
    {
        auto& __r13_g = dense_singlebcc_round13_prof::G();
        ++__r13_g.actual_new_calls;
        __r13_g.sum_actual_bcc_count += bccCnt;
        if (bccCnt == 0) ++__r13_g.hist_actual_bcc_0;
        else if (bccCnt == 1) ++__r13_g.hist_actual_bcc_1;
        else if (bccCnt == 2) ++__r13_g.hist_actual_bcc_2;
        else if (bccCnt <= 4) ++__r13_g.hist_actual_bcc_3_4;
        else ++__r13_g.hist_actual_bcc_5p;
        bool __r13_actual_single = (bccCnt == 1 && isolatedLocal.empty());
        int __r13_piece_count = __r13_actual_single ? 0 : std::max(0, bccCnt - 1);
        __r13_g.sum_actual_piece_count += __r13_piece_count;
        if (__r13_piece_count == 0) ++__r13_g.hist_actual_piece_0;
        else if (__r13_piece_count == 1) ++__r13_g.hist_actual_piece_1;
        else ++__r13_g.hist_actual_piece_2p;
        if (__r13_actual_single) {
            ++__r13_g.actual_new_single_bcc_count;
            ++__r13_g.actual_new_keep_is_all_count;
        }
    }
#endif

    std::vector<int> firstBcc(n, -1);
    std::vector<char> isArt(n, 0);
    for (int le = 0; le < mSurv; ++le) {
        int bid = bcc[le];
        for (int lidv : {survEdgeU[le], survEdgeV[le]}) {
            if (firstBcc[lidv] == -1) firstBcc[lidv] = bid;
            else if (firstBcc[lidv] != bid) isArt[lidv] = 1;
        }
    }
#if DENSE_RECT_ROUND1_PROFILE
    dense_rect_round1_prof::G().split_tarjan_ns += dense_rect_round1_prof::nsSince(__prof_split_tarjan_t0);
#endif

    // 4) old external boundary + local articulation
#if DENSE_RECT_ROUND1_PROFILE
    auto __prof_split_piece_t0 = dense_rect_round1_prof::Clock::now();
#endif
    std::vector<char> oldBoundaryLocal(n, 0);
    for (int cutBC : B.attachCuts) {
        chk(0 <= cutBC && cutBC < (int)bcNodes.size(),
            "splitBlockLocalRebuild: bad cut id in attachCuts");
        chk(bcNodes[cutBC].alive && bcNodes[cutBC].type == BCN_CUT,
            "splitBlockLocalRebuild: attachCuts contains invalid CUT");

        int v = bcNodes[cutBC].origVertex;
        if (v != x && 0 <= v && v < (int)orig.size() && orig[v].alive) {
            if (0 <= v && v < (int)__lidStamp.size() && __lidStamp[v] == __lidEpoch) {
                oldBoundaryLocal[__lidVal[v]] = 1;
            }
        }
    }

    std::vector<char> boundaryLocal = oldBoundaryLocal;
    for (int lidv = 0; lidv < n; ++lidv) {
        if (isArt[lidv]) boundaryLocal[lidv] = 1;
    }

    // 5) piece info
    struct PieceTmp {
        std::vector<int> edges;
        std::vector<int> handles;
        std::vector<int> vertices;
        long long mass = 0;
    };

    static thread_local std::vector<int> __pieceSeenV;
    static thread_local std::vector<int> __pieceSeenH;
    static thread_local int __pieceEpoch = 1;
    if ((int)__pieceSeenV.size() < (int)orig.size()) {
        __pieceSeenV.assign(orig.size(), 0);
        __pieceEpoch = 1;
    }
    if ((int)__pieceSeenH.size() < (int)handles.size()) {
        __pieceSeenH.assign(handles.size(), 0);
        __pieceEpoch = 1;
    }
    auto bumpPieceEpoch = [&]() {
        ++__pieceEpoch;
        if (__pieceEpoch == INT_MAX) {
            std::fill(__pieceSeenV.begin(), __pieceSeenV.end(), 0);
            std::fill(__pieceSeenH.begin(), __pieceSeenH.end(), 0);
            __pieceEpoch = 1;
        }
    };

    std::vector<PieceTmp> pieces;
    pieces.reserve(bccCnt);
    std::vector<char> pieceEdgesSorted(bccCnt, 0);
    std::vector<std::vector<int>> incLocal(n); // boundary local-id -> piece ids
    bool __r14_use_tinypiece = false;
#if DENSE_TINYPIECE_ROUND14_PROFILE || DENSE_TINYPIECE_ROUND14_OPT
    const int __r14_budget_total = 64;
    const int __r14_budget_piece = 32;
    const int __r14_budget_frontier = 64;
    bool __r14_tinypiece_eligible = false;
    if (__r14_keepBidSummary != -1 && __r14_uniqueMaxKeep && __r14_pieceTotalEdgeCount <= __r14_budget_total && __r14_maxNonkeepPieceEdgeCount <= __r14_budget_piece && __r14_pieceFrontierVertexCount <= __r14_budget_frontier) {
        __r14_tinypiece_eligible = true;
    }
#if DENSE_TINYPIECE_ROUND14_PROFILE
    if (__r14_tinypiece_eligible) ++dense_tinypiece_round14_prof::G().tinypiece_eligible;
    else {
        if (!__r14_uniqueMaxKeep) ++dense_tinypiece_round14_prof::G().fallback_keep_not_unique;
        else if (__r14_pieceTotalEdgeCount > __r14_budget_total) ++dense_tinypiece_round14_prof::G().fallback_nonkeep_budget_exceeded;
        else if (__r14_maxNonkeepPieceEdgeCount > __r14_budget_piece) ++dense_tinypiece_round14_prof::G().fallback_max_piece_too_large;
        else if (__r14_pieceFrontierVertexCount > __r14_budget_frontier) ++dense_tinypiece_round14_prof::G().fallback_frontier_too_large;
        else ++dense_tinypiece_round14_prof::G().fallback_invariant_unsure;
    }
#endif
#if DENSE_TINYPIECE_ROUND14_OPT
    if (__r14_tinypiece_eligible) {
#if DENSE_TINYPIECE_ROUND14_PROFILE
        ++dense_tinypiece_round14_prof::G().tinypiece_attempted;
        auto __r14_tiny_mat_t0 = dense_tinypiece_round14_prof::Clock::now();
#endif
        pieces.resize(bccCnt);
        for (int bid = 0; bid < bccCnt; ++bid) pieces[bid].mass = __r14_bccMass[bid];
        for (int bid = 0; bid < bccCnt; ++bid) {
            if (bid == __r14_keepBidSummary) continue;
            bumpPieceEpoch();
            PieceTmp P;
            P.edges = bccEdges[bid];
            for (int e : P.edges) {
                int a = edges[e].u;
                int b = edges[e].v;
                if (__pieceSeenV[a] != __pieceEpoch) { __pieceSeenV[a] = __pieceEpoch; P.vertices.push_back(a); }
                if (__pieceSeenV[b] != __pieceEpoch) { __pieceSeenV[b] = __pieceEpoch; P.vertices.push_back(b); }
                int h = edges[e].handleId;
                if (h != -1 && h < (int)handles.size() && handles[h].watched && h < (int)handleOwnerCore.size() && handleOwnerCore[h] == oldCore) {
                    if (__pieceSeenH[h] != __pieceEpoch) { __pieceSeenH[h] = __pieceEpoch; P.handles.push_back(h); }
                }
            }
            std::sort(P.vertices.begin(), P.vertices.end());
            std::sort(P.handles.begin(), P.handles.end());
            P.mass = __r14_bccMass[bid];
            pieces[bid] = std::move(P);
            for (int v : pieces[bid].vertices) {
                chk(0 <= v && v < (int)__lidStamp.size() && __lidStamp[v] == __lidEpoch,
                    "splitBlockLocalRebuild: piece vertex missing in lid");
                int lidv = __lidVal[v];
                if (boundaryLocal[lidv]) incLocal[lidv].push_back(bid);
            }
        }
        for (int lidv = 0; lidv < n; ++lidv) {
            if (boundaryLocal[lidv] && __r14_boundaryTouchKeep[lidv]) incLocal[lidv].push_back(__r14_keepBidSummary);
        }
        for (auto& vec : incLocal) {
            if (!vec.empty()) vec = normVec(std::move(vec));
        }
#if DENSE_TINYPIECE_ROUND14_PROFILE
        dense_tinypiece_round14_prof::G().tinypiece_success++;
        dense_tinypiece_round14_prof::G().tinypiece_materialize_total_ns += dense_tinypiece_round14_prof::nsSince(__r14_tiny_mat_t0);
        dense_tinypiece_round14_prof::G().tinypiece_keep_fastreturn_total_ns += 0;
#endif
        __r14_use_tinypiece = true;
    }
#endif
#endif

    if (!__r14_use_tinypiece) for (int bid = 0; bid < bccCnt; ++bid) {
        bumpPieceEpoch();
        PieceTmp P;
#if DENSE_TINYPIECE_ROUND14_PROFILE
        auto __r14_mat_t0 = dense_tinypiece_round14_prof::Clock::now();
#endif
        P.edges = bccEdges[bid];

        for (int e : P.edges) {
            int a = edges[e].u;
            int b = edges[e].v;
            if (__pieceSeenV[a] != __pieceEpoch) {
                __pieceSeenV[a] = __pieceEpoch;
                P.vertices.push_back(a);
            }
            if (__pieceSeenV[b] != __pieceEpoch) {
                __pieceSeenV[b] = __pieceEpoch;
                P.vertices.push_back(b);
            }

            int h = edges[e].handleId;
            if (h != -1 && h < (int)handles.size() && handles[h].watched &&
                h < (int)handleOwnerCore.size() && handleOwnerCore[h] == oldCore) {
                if (__pieceSeenH[h] != __pieceEpoch) {
                    __pieceSeenH[h] = __pieceEpoch;
                    P.handles.push_back(h);
                }
            }
        }

#if DENSE_TINYPIECE_ROUND14_PROFILE
        dense_tinypiece_round14_prof::G().full_materialize_total_ns += dense_tinypiece_round14_prof::nsSince(__r14_mat_t0);
        auto __r14_norm_t0 = dense_tinypiece_round14_prof::Clock::now();
#endif
#if DENSE_TIEKEEP_ROUND15_PROFILE
        dense_tiekeep_round15_prof::G().full_materialize_total_ns += dense_tiekeep_round15_prof::nsSince(__r14_mat_t0);
        auto __r15_norm_t0 = dense_tiekeep_round15_prof::Clock::now();
#endif
        std::sort(P.vertices.begin(), P.vertices.end());
        std::sort(P.handles.begin(), P.handles.end());
#if DENSE_TINYPIECE_ROUND14_PROFILE
        dense_tinypiece_round14_prof::G().full_normalize_total_ns += dense_tinypiece_round14_prof::nsSince(__r14_norm_t0);
#endif
#if DENSE_TIEKEEP_ROUND15_PROFILE
        dense_tiekeep_round15_prof::G().full_normalize_total_ns += dense_tiekeep_round15_prof::nsSince(__r15_norm_t0);
#endif

        P.mass = (long long)P.edges.size() + (long long)P.handles.size() + (long long)P.vertices.size();

        int pid = (int)pieces.size();
        pieces.push_back(std::move(P));

        for (int v : pieces[pid].vertices) {
            chk(0 <= v && v < (int)__lidStamp.size() && __lidStamp[v] == __lidEpoch,
                "splitBlockLocalRebuild: piece vertex missing in lid");
            int lidv = __lidVal[v];
            if (boundaryLocal[lidv]) {
                incLocal[lidv].push_back(pid);
            }
        }
    }

    for (auto& vec : incLocal) {
        if (!vec.empty()) vec = normVec(std::move(vec));
    }
#if DENSE_RECT_ROUND1_PROFILE
    dense_rect_round1_prof::G().split_piece_ns += dense_rect_round1_prof::nsSince(__prof_split_piece_t0);
#endif

    // 6) choose keep
#if DENSE_RECT_ROUND1_PROFILE
    auto __prof_split_boundary_t0 = dense_rect_round1_prof::Clock::now();
#endif
#if DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
    auto __prof_round4_keep_mass_t0 = dense_rect_round4_keep_prof::Clock::now();
#endif
    long long bestMass = std::numeric_limits<long long>::min();
    for (int i = 0; i < (int)pieces.size(); ++i) {
        bestMass = std::max(bestMass, pieces[i].mass);
    }
#if DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
    dense_rect_round4_keep_prof::G().keep_mass_ns += dense_rect_round4_keep_prof::nsSince(__prof_round4_keep_mass_t0);
    auto __prof_round4_keep_tie_t0 = dense_rect_round4_keep_prof::Clock::now();
#endif
    std::vector<int> tieCandidates;
    tieCandidates.reserve(pieces.size());
    for (int i = 0; i < (int)pieces.size(); ++i) {
        if (pieces[i].mass == bestMass) tieCandidates.push_back(i);
    }
#if DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
    dense_rect_round4_keep_prof::G().keep_tie_collect_ns += dense_rect_round4_keep_prof::nsSince(__prof_round4_keep_tie_t0);
#endif

    auto ensureSortedPieceEdges = [&](int pid) -> const std::vector<int>& {
        chk(0 <= pid && pid < (int)pieces.size(), "splitBlockLocalRebuild: bad keep pid");
        if (!pieceEdgesSorted[pid]) {
#if DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
            dense_rect_round4_keep_prof::G().sorted_edge_lists++;
            dense_rect_round4_keep_prof::G().sorted_total_edges += (long long)pieces[pid].edges.size();
#endif
            std::sort(pieces[pid].edges.begin(), pieces[pid].edges.end());
            pieceEdgesSorted[pid] = 1;
        }
        return pieces[pid].edges;
    };

    int keep = -1;
    if ((int)pieces.size() == 1) {
        keep = 0;
#if DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
        dense_rect_round4_keep_prof::G().single_bcc_calls++;
#endif
    } else if (!tieCandidates.empty() && (int)tieCandidates.size() == 1) {
        keep = tieCandidates[0];
#if DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
        dense_rect_round4_keep_prof::G().unique_max_calls++;
#endif
    } else if (!tieCandidates.empty()) {
#if DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
        dense_rect_round4_keep_prof::G().tie_calls++;
        dense_rect_round4_keep_prof::G().tie_candidate_total += (long long)tieCandidates.size();
        dense_rect_round4_keep_prof::G().tie_candidate_max = std::max(dense_rect_round4_keep_prof::G().tie_candidate_max, (long long)tieCandidates.size());
        auto __prof_round4_keep_canon_t0 = dense_rect_round4_keep_prof::Clock::now();
#endif
        keep = tieCandidates[0];
        for (int idx = 1; idx < (int)tieCandidates.size(); ++idx) {
            int pid = tieCandidates[idx];
            if (ensureSortedPieceEdges(pid) < ensureSortedPieceEdges(keep)) keep = pid;
        }
#if DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
        long long __prof_round4_keep_canon_ns = dense_rect_round4_keep_prof::nsSince(__prof_round4_keep_canon_t0);
        dense_rect_round4_keep_prof::G().keep_canon_ns += __prof_round4_keep_canon_ns;
        if (__prof_round4_case_4096) dense_rect_round4_keep_prof::G().comb4096_canon_ns += __prof_round4_keep_canon_ns;
#endif
    }

#if DENSE_TIEKEEP_ROUND15_PROFILE
    {
        auto& __r15_g = dense_tiekeep_round15_prof::G();
        int __r15_tieCnt = (int)tieCandidates.size();
        int __r15_tieTotalEdges = 0;
        int __r15_tieMaxEdges = 0;
        for (int pid : tieCandidates) {
            if (0 <= pid && pid < (int)pieces.size()) {
                int es = (int)pieces[pid].edges.size();
                __r15_tieTotalEdges += es;
                __r15_tieMaxEdges = std::max(__r15_tieMaxEdges, es);
            }
        }
        if (__r15_tieCnt > 0) {
            __r15_g.top_mass_tie_candidate_count_sum += __r15_tieCnt;
            __r15_g.top_mass_tie_total_edge_count_sum += __r15_tieTotalEdges;
            __r15_g.top_mass_tie_max_edge_count_sum += __r15_tieMaxEdges;
            if (__r15_tieCnt <= 3 && __r15_tieTotalEdges <= 128) ++__r15_g.top_mass_tie_tiny_count;
        }
        int __r15_keepEdgesActual = (keep >= 0 && keep < (int)pieces.size()) ? (int)pieces[keep].edges.size() : 0;
        bool __r15_keepLarge = (__r15_keepEdgesActual > __r14_pieceTotalEdgeCount);
        bool __r15_fromTie = (__r15_tieCnt > 1 && keep >= 0);
        if (__r15_fromTie) ++__r15_g.actual_fullpath_keep_from_tie_count;
        if (__r15_fromTie && __r15_keepLarge) ++__r15_g.actual_fullpath_keep_is_large_in_tie_count;
        const int __r15_budget_tie_cnt = 3;
        const int __r15_budget_tie_edges = 128;
        const int __r15_budget_nonkeep = 64;
        const int __r15_budget_frontier = 64;
        bool __r15_implicit_eligible = (__r15_fromTie && __r15_keepLarge && __r15_tieCnt <= __r15_budget_tie_cnt && __r15_tieTotalEdges <= __r15_budget_tie_edges && __r14_pieceTotalEdgeCount <= __r15_budget_nonkeep && __r14_pieceFrontierVertexCount <= __r15_budget_frontier);
        if (__r15_implicit_eligible) {
            ++__r15_g.implicit_keep_eligible;
            ++__r15_g.actual_fullpath_keep_is_implicit_complement_count;
        } else if (__r15_fromTie) {
            ++__r15_g.implicit_keep_fallback;
            if (!__r15_keepLarge) ++__r15_g.fallback_keep_not_large_enough;
            else if (__r15_tieCnt > __r15_budget_tie_cnt) ++__r15_g.fallback_tie_candidate_count_too_large;
            else if (__r15_tieTotalEdges > __r15_budget_tie_edges) ++__r15_g.fallback_tie_candidate_edge_budget_exceeded;
            else if (__r14_pieceTotalEdgeCount > __r15_budget_nonkeep) ++__r15_g.fallback_nonkeep_budget_exceeded;
            else if (__r14_pieceFrontierVertexCount > __r15_budget_frontier) ++__r15_g.fallback_frontier_too_large;
            else ++__r15_g.fallback_invariant_unsure;
        }
    }
#endif

#if DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
    long long __prof_round4_keep_ns = dense_rect_round4_keep_prof::nsSince(__prof_round4_keep_t0);
    dense_rect_round4_keep_prof::G().keep_total_ns += __prof_round4_keep_ns;
    if (__prof_round4_case_4096) dense_rect_round4_keep_prof::G().comb4096_keep_ns += __prof_round4_keep_ns;
    auto __prof_round4_piece_post_t0 = dense_rect_round4_keep_prof::Clock::now();
#endif

    if (keep != -1) {
        S.keepExists = true;
        S.keepMatPiece = keep;
    }

    // 7) small pieces
    std::vector<int> pieceToSmall((int)pieces.size(), -1);
    for (int pid = 0; pid < (int)pieces.size(); ++pid) {
        if (pid == keep) continue;

        SparsePiece sp;
        sp.matPieceId = pid;
        sp.edges = pieces[pid].edges;
        sp.watchedHandles = pieces[pid].handles;
        for (int v : pieces[pid].vertices) {
            chk(0 <= v && v < (int)__lidStamp.size() && __lidStamp[v] == __lidEpoch,
                "splitBlockLocalRebuild: small piece vertex missing in lid");
            if (!boundaryLocal[__lidVal[v]]) sp.exclusiveVertices.push_back(v);
        }
        sp.exclusiveVertices = normVec(std::move(sp.exclusiveVertices));
        pieceToSmall[pid] = (int)S.small.size();
        S.small.push_back(std::move(sp));
    }

#if DENSE_RECT_KEEP_ORDER_ROUND4_PROFILE
    dense_rect_round4_keep_prof::G().piece_post_ns += dense_rect_round4_keep_prof::nsSince(__prof_round4_piece_post_t0);
    long long __prof_round4_step3_ns = dense_rect_round4_keep_prof::nsSince(__prof_round4_step3_t0);
    dense_rect_round4_keep_prof::G().step3_total_ns += __prof_round4_step3_ns;
    if (__prof_round4_case_4096) dense_rect_round4_keep_prof::G().comb4096_step3_ns += __prof_round4_step3_ns;
#endif
#if DENSE_SINGLEBCC_ROUND13_PROFILE
    {
        auto& __r13_p = dense_singlebcc_round13_prof::Pending();
        __r13_p.usedStep3 = true;
        __r13_p.singleBcc = (bccCnt == 1 && isolatedLocal.empty());
        auto& __r13_g = dense_singlebcc_round13_prof::G();
        long long __r13_step3_ns = dense_singlebcc_round13_prof::nsSince(__r13_step3_t0);
        __r13_g.full_step3_total_ns += __r13_step3_ns;
        // approximate internal buckets with available phase timers from existing path
        long long __r13_piece_total_edge = 0;
        for (int pid = 0; pid < (int)pieces.size(); ++pid) if (pid != keep) __r13_piece_total_edge += (long long)pieces[pid].edges.size();
        __r13_g.sum_actual_piece_total_edge_count += __r13_piece_total_edge;
    }
#endif
#if DENSE_BCCREUSE_ROUND12_PROFILE
    {
        auto& __r12_pending = dense_bccreuse_round12_prof::Pending();
        __r12_pending.usedStep3 = true;
        __r12_pending.keepExists = S.keepExists;
        long long __r12_step3_ns = dense_bccreuse_round12_prof::nsSince(__r12_step3_t0);
        dense_bccreuse_round12_prof::G().step3_total_ns += __r12_step3_ns;
        dense_bccreuse_round12_prof::G().full_step3_total_ns += __r12_step3_ns;
    }
#endif
#if DENSE_TIEKEEP_ROUND15_PROFILE
    {
        dense_tiekeep_round15_prof::G().step3_total_ns += dense_tiekeep_round15_prof::nsSince(__r15_step3_t0);
    }
#endif

    // 8) boundary compression
    std::vector<int> boundaryLids;
    boundaryLids.reserve(n);
    for (int lidv = 0; lidv < n; ++lidv) {
        if (boundaryLocal[lidv]) boundaryLids.push_back(lidv);
    }
    std::sort(boundaryLids.begin(), boundaryLids.end(), [&](int a, int b) {
        return realOf[a] < realOf[b];
    });

    for (int lidv : boundaryLids) {
        const std::vector<int>& pieceInc = incLocal[lidv];
        bool existedOldCut = !!oldBoundaryLocal[lidv];
        int v = realOf[lidv];

        if (!existedOldCut && (int)pieceInc.size() <= 1) {
            failCheck("splitBlockLocalRebuild: non-old boundary with incidence <= 1");
        }

        SparseBoundary bd;
        bd.vertex = v;
        bd.existedOldCut = existedOldCut;
        bd.touchesKeep = false;

        for (int pid : pieceInc) {
            if (pid == keep) {
                bd.touchesKeep = true;
            } else {
                int sid = pieceToSmall[pid];
                chk(sid != -1,
                    "splitBlockLocalRebuild: missing small id for non-keep piece");
                bd.smallIds.push_back(sid);
            }
        }
        bd.smallIds = normVec(std::move(bd.smallIds));

        if (bd.existedOldCut || bd.touchesKeep || !bd.smallIds.empty()) {
            S.boundary.push_back(std::move(bd));
        }
    }

    // 9) isolatedExclusive
    for (int lidv : isolatedLocal) {
        int v = realOf[lidv];
        if (!boundaryLocal[lidv]) S.isolatedExclusive.push_back(v);
    }
    S.isolatedExclusive = normVec(std::move(S.isolatedExclusive));

    // 10) dead handles empty by current contract
    S.deadHandles.clear();

    // 11) dead exclusive = {x} U isolatedExclusive
    S.deadExclusiveVertices.push_back(x);
    for (int v : S.isolatedExclusive) {
        if (v != x) S.deadExclusiveVertices.push_back(v);
    }
    S.deadExclusiveVertices = normVec(std::move(S.deadExclusiveVertices));
#if DENSE_RECT_ROUND1_PROFILE
    dense_rect_round1_prof::G().split_boundary_ns += dense_rect_round1_prof::nsSince(__prof_split_boundary_t0);
#endif

    return S;
}


// ============================================================
// [00] tiny summary helpers
// ============================================================

Solver::SideBagSummary Solver::emptyBagSummary() {
    return SideBagSummary{0, 0, INT_MAX, false};
}

Solver::SideBagSummary
Solver::mergeBagSummary(const SideBagSummary& A,
                        const SideBagSummary& B) {
    SideBagSummary C;
    C.realEdgeCount      = A.realEdgeCount + B.realEdgeCount;
    C.watchedHandleCount = A.watchedHandleCount + B.watchedHandleCount;
    C.minRealEdge        = std::min(A.minRealEdge, B.minRealEdge);
    C.hasOldBoundary     = A.hasOldBoundary || B.hasOldBoundary;
    return C;
}

// ============================================================
// [01] tree utility
// ============================================================

int Solver::otherNode(const BlockSpqr& T, int teid, int u) const {
    chk(0 <= teid && teid < (int)T.tree.size(),
        "otherNode: bad tree edge id");
    const auto& te = T.tree[teid];
    if (te.aNode == u) return te.bNode;
    if (te.bNode == u) return te.aNode;
    failCheck("otherNode: u is not incident to teid");
}

int Solver::dirFromNode(const BlockSpqr& T, int teid, int from) const {
    chk(0 <= teid && teid < (int)T.tree.size(),
        "dirFromNode: bad tree edge id");
    const auto& te = T.tree[teid];
    if (te.aNode == from) return te.dirAB;
    if (te.bNode == from) return te.dirBA;
    failCheck("dirFromNode: from is not incident to teid");
}

// ============================================================
// [02] raw builder
//      recursive single-2-cut families + intentional leaf fallback
// ============================================================

namespace {

struct EdgeSubsetInfo {
    std::vector<int> verts;
    std::unordered_map<int,int> lid;
    std::vector<std::vector<std::pair<int,int>>> adj; // (toLid, edgeId)
};

struct RawSubtree {
    Solver::RawSpqrBuild raw;
    int root = -1;
    int parentSlot = -1; // root skeleton slot reserved for parent virtual edge
};

EdgeSubsetInfo buildEdgeSubsetInfo(const Solver& s,
                                   const std::vector<int>& edgeIds) {
    EdgeSubsetInfo G;
    std::unordered_set<int> vertSet;
    for (int e : edgeIds) {
        chk(0 <= e && e < (int)s.edges.size(),
            "buildEdgeSubsetInfo: bad edge id");
        int a = s.edges[e].u;
        int b = s.edges[e].v;
        chk(a != b,
            "buildEdgeSubsetInfo: self-loop not expected");
        vertSet.insert(a);
        vertSet.insert(b);
    }
    G.verts.assign(vertSet.begin(), vertSet.end());
    std::sort(G.verts.begin(), G.verts.end());
    for (int i = 0; i < (int)G.verts.size(); ++i) G.lid[G.verts[i]] = i;
    G.adj.assign(G.verts.size(), {});
    for (int e : edgeIds) {
        int a = s.edges[e].u, b = s.edges[e].v;
        int la = G.lid[a], lb = G.lid[b];
        G.adj[la].push_back({lb, e});
        G.adj[lb].push_back({la, e});
    }
    return G;
}

bool isConnectedEdgeSubset(const Solver& s,
                           const std::vector<int>& edgeIds) {
    if (edgeIds.empty()) return false;
    EdgeSubsetInfo G = buildEdgeSubsetInfo(s, edgeIds);
    std::vector<char> vis(G.verts.size(), 0);
    std::queue<int> q;
    q.push(0);
    vis[0] = 1;
    int seen = 0;
    while (!q.empty()) {
        int u = q.front(); q.pop();
        seen++;
        for (auto [v, e] : G.adj[u]) {
            (void)e;
            if (!vis[v]) {
                vis[v] = 1;
                q.push(v);
            }
        }
    }
    return seen == (int)G.verts.size();
}

bool isSimplePathBetween(const Solver& s,
                         const std::vector<int>& edgeIds,
                         int sTerm,
                         int tTerm) {
    if (edgeIds.empty()) return false;
    EdgeSubsetInfo G = buildEdgeSubsetInfo(s, edgeIds);
    if (!G.lid.count(sTerm) || !G.lid.count(tTerm)) return false;
    if (!isConnectedEdgeSubset(s, edgeIds)) return false;

    std::unordered_map<int,int> deg;
    for (int e : edgeIds) {
        deg[s.edges[e].u]++;
        deg[s.edges[e].v]++;
    }

    for (int v : G.verts) {
        if (v == sTerm || v == tTerm) {
            if (deg[v] != 1) return false;
        } else {
            if (deg[v] != 2) return false;
        }
    }
    return (int)G.verts.size() == (int)edgeIds.size() + 1;
}

char classifyEdgeSetOneNodeType(const Solver& s,
                                const std::vector<int>& edgeIds,
                                int sTerm,
                                int tTerm) {
    if ((int)edgeIds.size() == 1) return 'Q';

    bool allParallel = true;
    for (int e : edgeIds) {
        int a = s.edges[e].u;
        int b = s.edges[e].v;
        if (!((a == sTerm && b == tTerm) || (a == tTerm && b == sTerm))) {
            allParallel = false;
            break;
        }
    }
    if (allParallel) return 'P';
    if (isSimplePathBetween(s, edgeIds, sTerm, tTerm)) return 'S';
    return 'R';
}

char classifyWholeCoreType(const Solver& s, int core) {
    const auto& B = s.blocks[core];
    if ((int)B.realEdges.size() == 1) return 'Q';
    std::unordered_map<int,int> deg;
    std::unordered_set<int> verts;
    for (int e : B.realEdges) {
        int a = s.edges[e].u, b = s.edges[e].v;
        verts.insert(a); verts.insert(b);
        deg[a]++; deg[b]++;
    }
    if ((int)verts.size() == 2) return 'P';
    bool allDegTwo = true;
    for (int v : verts) if (deg[v] != 2) { allDegTwo = false; break; }
    if (allDegTwo && (int)verts.size() == (int)B.realEdges.size()) return 'S';
    return 'R';
}

int appendShiftedRawBuild(Solver::RawSpqrBuild& dst,
                          const Solver::RawSpqrBuild& src) {
    int off = (int)dst.node.size();
    dst.node.resize(off + (int)src.node.size());
    for (int u = 0; u < (int)src.node.size(); ++u) {
        dst.node[off + u].type = src.node[u].type;
        dst.node[off + u].skel = src.node[u].skel;
        for (auto& sk : dst.node[off + u].skel) {
            if (sk.realEdgeId == -1 && sk.peerNode != -1) {
                sk.peerNode += off;
            }
        }
    }
    return off;
}

void stitchVirtual(Solver::RawSpqrBuild& raw,
                   int u, int slotU,
                   int v, int slotV) {
    auto& su = raw.node[u].skel[slotU];
    auto& sv = raw.node[v].skel[slotV];
    chk(su.realEdgeId == -1 && sv.realEdgeId == -1,
        "stitchVirtual: both slots must be virtual");
    chk((su.a == sv.a && su.b == sv.b) || (su.a == sv.b && su.b == sv.a),
        "stitchVirtual: terminal mismatch");
    su.peerNode = v;
    su.peerSlot = slotV;
    sv.peerNode = u;
    sv.peerSlot = slotU;
}

RawSubtree makeLeafSubtree(const Solver& s,
                           const std::vector<int>& edgeIds,
                           int sTerm,
                           int tTerm,
                           bool needParentSlot) {
    RawSubtree out;
    out.raw.node.resize(1);
    out.root = 0;

    Solver::RawSpqrNode node;
    node.type = classifyEdgeSetOneNodeType(s, edgeIds, sTerm, tTerm);
    node.skel.reserve(edgeIds.size() + (needParentSlot ? 1 : 0));
    for (int e : edgeIds) {
        Solver::RawSkelEdge sk;
        sk.a = s.edges[e].u;
        sk.b = s.edges[e].v;
        sk.realEdgeId = e;
        sk.peerNode = -1;
        sk.peerSlot = -1;
        node.skel.push_back(sk);
    }
    if (needParentSlot) {
        Solver::RawSkelEdge virt;
        virt.a = sTerm;
        virt.b = tTerm;
        virt.realEdgeId = -1;
        virt.peerNode = -1;
        virt.peerSlot = -1;
        out.parentSlot = (int)node.skel.size();
        node.skel.push_back(virt);
    } else {
        out.parentSlot = -1;
    }

    out.raw.node[0] = std::move(node);
    return out;
}

bool decomposeParallelFixed(const Solver& s,
                            const std::vector<int>& edgeIds,
                            int sTerm,
                            int tTerm,
                            std::vector<std::vector<int>>& branches) {
    branches.clear();
    EdgeSubsetInfo G = buildEdgeSubsetInfo(s, edgeIds);
    auto its = G.lid.find(sTerm), itt = G.lid.find(tTerm);
    if (its == G.lid.end() || itt == G.lid.end()) return false;
    int ls = its->second, lt = itt->second;

    std::vector<char> removed(G.verts.size(), 0);
    removed[ls] = 1;
    removed[lt] = 1;
    std::vector<int> comp(G.verts.size(), -1);
    std::vector<std::vector<int>> compVerts;

    for (int src = 0; src < (int)G.verts.size(); ++src) {
        if (removed[src] || comp[src] != -1) continue;
        int cid = (int)compVerts.size();
        compVerts.push_back({});
        std::queue<int> q;
        q.push(src);
        comp[src] = cid;
        while (!q.empty()) {
            int u = q.front(); q.pop();
            compVerts[cid].push_back(u);
            for (auto [v, e] : G.adj[u]) {
                (void)e;
                if (removed[v] || comp[v] != -1) continue;
                comp[v] = cid;
                q.push(v);
            }
        }
    }

    std::vector<std::vector<int>> compEdges(compVerts.size());
    std::vector<int> direct;

    for (int e : edgeIds) {
        int a = s.edges[e].u;
        int b = s.edges[e].v;
        bool aTerm = (a == sTerm || a == tTerm);
        bool bTerm = (b == sTerm || b == tTerm);
        if (aTerm && bTerm) {
            direct.push_back(e);
            continue;
        }
        int cid = -1;
        if (!aTerm) cid = comp[G.lid[a]];
        if (!bTerm) {
            int cid2 = comp[G.lid[b]];
            if (cid == -1) cid = cid2;
            else chk(cid == cid2,
                     "decomposeParallelFixed: edge crosses two post-separator components");
        }
        if (cid == -1) {
            // both endpoints are terminals and already handled above
            failCheck("decomposeParallelFixed: impossible cid == -1");
        }
        compEdges[cid].push_back(e);
    }

    for (auto& br : compEdges) {
        if (!br.empty()) branches.push_back(normVec(std::move(br)));
    }
    if (!direct.empty()) branches.push_back(normVec(std::move(direct)));

    if ((int)branches.size() < 2) return false;

    // exact partition sanity
    std::vector<int> uni;
    for (const auto& br : branches) {
        for (int e : br) uni.push_back(e);
        if (!isConnectedEdgeSubset(s, br)) return false;
    }
    uni = normVec(std::move(uni));
    std::vector<int> all = normVec(edgeIds);
    if (uni != all) return false;
    return true;
}

bool decomposeSeriesFixed(const Solver& s,
                          const std::vector<int>& edgeIds,
                          int sTerm,
                          int tTerm,
                          int& mid,
                          std::vector<int>& leftEdges,
                          std::vector<int>& rightEdges) {
    mid = -1;
    leftEdges.clear();
    rightEdges.clear();

    EdgeSubsetInfo G = buildEdgeSubsetInfo(s, edgeIds);
    auto its = G.lid.find(sTerm), itt = G.lid.find(tTerm);
    if (its == G.lid.end() || itt == G.lid.end()) return false;
    int ls = its->second, lt = itt->second;

    for (int cand : G.verts) {
        if (cand == sTerm || cand == tTerm) continue;
        int lm = G.lid[cand];

        std::vector<int> comp(G.verts.size(), -1);
        comp[lm] = -2; // removed
        int cid = 0;
        for (int src = 0; src < (int)G.verts.size(); ++src) {
            if (src == lm || comp[src] != -1) continue;
            std::queue<int> q;
            q.push(src);
            comp[src] = cid;
            while (!q.empty()) {
                int u = q.front(); q.pop();
                for (auto [v, e] : G.adj[u]) {
                    (void)e;
                    if (v == lm || comp[v] != -1) continue;
                    comp[v] = cid;
                    q.push(v);
                }
            }
            cid++;
        }

        if (comp[ls] == -2 || comp[lt] == -2) continue;
        if (comp[ls] == comp[lt]) continue;

        int sCid = comp[ls], tCid = comp[lt];
        bool bad = false;
        for (int src = 0; src < (int)G.verts.size(); ++src) {
            if (src == lm) continue;
            if (comp[src] != sCid && comp[src] != tCid) {
                bad = true;
                break;
            }
        }
        if (bad) continue;

        std::vector<int> A, B;
        for (int e : edgeIds) {
            int a = s.edges[e].u, b = s.edges[e].v;
            auto sideOf = [&](int v)->int {
                if (v == cand) return 0;
                return comp[G.lid[v]];
            };
            int sa = sideOf(a), sb = sideOf(b);
            bool inA = (sa == 0 || sa == sCid) && (sb == 0 || sb == sCid);
            bool inB = (sa == 0 || sa == tCid) && (sb == 0 || sb == tCid);
            if (inA && !inB) A.push_back(e);
            else if (inB && !inA) B.push_back(e);
            else {
                bad = true;
                break;
            }
        }
        if (bad) continue;
        A = normVec(std::move(A));
        B = normVec(std::move(B));
        if (A.empty() || B.empty()) continue;
        if (!isConnectedEdgeSubset(s, A) || !isConnectedEdgeSubset(s, B)) continue;
        if (A.size() == edgeIds.size() || B.size() == edgeIds.size()) continue;

        mid = cand;
        leftEdges = std::move(A);
        rightEdges = std::move(B);
        return true;
    }

    return false;
}

RawSubtree buildTwoTerminalRecursive(const Solver& s,
                                     const std::vector<int>& edgeIds,
                                     int sTerm,
                                     int tTerm,
                                     bool needParentSlot,
                                     int depth) {
    chk(!edgeIds.empty(),
        "buildTwoTerminalRecursive: empty edge subset");
    chk(depth <= 40,
        "buildTwoTerminalRecursive: recursion depth exceeded");

    std::vector<int> normE = normVec(edgeIds);

    // Base / intentional leaf
    if ((int)normE.size() <= 2 || isSimplePathBetween(s, normE, sTerm, tTerm)) {
        return makeLeafSubtree(s, normE, sTerm, tTerm, needParentSlot);
    }

    std::vector<std::vector<int>> pBranches;
    if (decomposeParallelFixed(s, normE, sTerm, tTerm, pBranches)) {
        RawSubtree out;
        out.raw.node.resize(1);
        out.root = 0;
        out.parentSlot = -1;

        Solver::RawSpqrNode root;
        root.type = 'P';
        root.skel.reserve((int)pBranches.size() + (needParentSlot ? 1 : 0));

        std::vector<int> rootSlots;
        for (int i = 0; i < (int)pBranches.size(); ++i) {
            Solver::RawSkelEdge virt;
            virt.a = sTerm;
            virt.b = tTerm;
            virt.realEdgeId = -1;
            virt.peerNode = -1;
            virt.peerSlot = -1;
            rootSlots.push_back((int)root.skel.size());
            root.skel.push_back(virt);
        }
        if (needParentSlot) {
            Solver::RawSkelEdge virt;
            virt.a = sTerm;
            virt.b = tTerm;
            virt.realEdgeId = -1;
            virt.peerNode = -1;
            virt.peerSlot = -1;
            out.parentSlot = (int)root.skel.size();
            root.skel.push_back(virt);
        }
        out.raw.node[0] = std::move(root);

        for (int i = 0; i < (int)pBranches.size(); ++i) {
            RawSubtree child = buildTwoTerminalRecursive(s, pBranches[i], sTerm, tTerm, true, depth + 1);
            int off = appendShiftedRawBuild(out.raw, child.raw);
            stitchVirtual(out.raw, out.root, rootSlots[i], off + child.root, child.parentSlot);
        }
        return out;
    }

    int mid = -1;
    std::vector<int> leftE, rightE;
    if (decomposeSeriesFixed(s, normE, sTerm, tTerm, mid, leftE, rightE)) {
        RawSubtree left = buildTwoTerminalRecursive(s, leftE, sTerm, mid, true, depth + 1);
        RawSubtree right = buildTwoTerminalRecursive(s, rightE, mid, tTerm, true, depth + 1);

        RawSubtree out;
        out.raw.node.resize(1);
        out.root = 0;
        out.parentSlot = -1;

        Solver::RawSpqrNode root;
        root.type = 'S';
        root.skel.reserve(2 + (needParentSlot ? 1 : 0));

        Solver::RawSkelEdge v0;
        v0.a = sTerm; v0.b = mid; v0.realEdgeId = -1; v0.peerNode = -1; v0.peerSlot = -1;
        int slotL = (int)root.skel.size();
        root.skel.push_back(v0);

        Solver::RawSkelEdge v1;
        v1.a = mid; v1.b = tTerm; v1.realEdgeId = -1; v1.peerNode = -1; v1.peerSlot = -1;
        int slotR = (int)root.skel.size();
        root.skel.push_back(v1);

        if (needParentSlot) {
            Solver::RawSkelEdge vp;
            vp.a = sTerm; vp.b = tTerm; vp.realEdgeId = -1; vp.peerNode = -1; vp.peerSlot = -1;
            out.parentSlot = (int)root.skel.size();
            root.skel.push_back(vp);
        }
        out.raw.node[0] = std::move(root);

        int offL = appendShiftedRawBuild(out.raw, left.raw);
        stitchVirtual(out.raw, out.root, slotL, offL + left.root, left.parentSlot);
        int offR = appendShiftedRawBuild(out.raw, right.raw);
        stitchVirtual(out.raw, out.root, slotR, offR + right.root, right.parentSlot);
        return out;
    }

    return makeLeafSubtree(s, normE, sTerm, tTerm, needParentSlot);
}

bool chooseTopLevelParallelPair(const Solver& s,
                                const std::vector<int>& edgeIds,
                                int& bestS,
                                int& bestT,
                                std::vector<std::vector<int>>& bestBranches) {
    bestS = bestT = -1;
    bestBranches.clear();
    EdgeSubsetInfo G = buildEdgeSubsetInfo(s, edgeIds);

    // Candidate-limited search: correctness does not depend on finding a pair;
    // failing to find one just falls back to one-node raw. We therefore prune
    // the expensive O(V^2) scan aggressively for submission performance.
    std::vector<std::pair<int,int>> degV; // (-deg, v)
    degV.reserve(G.verts.size());
    for (int v : G.verts) {
        int d = (int)G.adj[G.lid.at(v)].size();
        if (d != 2) degV.push_back({-d, v});
    }
    if ((int)degV.size() < 2) {
        degV.clear();
        for (int v : G.verts) {
            int d = (int)G.adj[G.lid.at(v)].size();
            degV.push_back({-d, v});
        }
    }
    std::sort(degV.begin(), degV.end());
    std::vector<int> cand;
    const int K = 12;
    for (int i = 0; i < (int)degV.size() && i < K; ++i) cand.push_back(degV[i].second);
    cand = normVec(std::move(cand));

    for (int i = 0; i < (int)cand.size(); ++i) {
        for (int j = i + 1; j < (int)cand.size(); ++j) {
            int sTerm = cand[i], tTerm = cand[j];
            std::vector<std::vector<int>> branches;
            if (!decomposeParallelFixed(s, edgeIds, sTerm, tTerm, branches)) continue;
            if (bestS == -1 || (int)branches.size() > (int)bestBranches.size() ||
                ((int)branches.size() == (int)bestBranches.size() && std::pair<int,int>(sTerm,tTerm) < std::pair<int,int>(bestS,bestT))) {
                bestS = sTerm;
                bestT = tTerm;
                bestBranches = branches;
            }
        }
    }
    return bestS != -1;
}

} // namespace


Solver::RawSpqrBuild
Solver::rawSpqrBuildOneNode(int core) const {
    chk(0 <= core && core < (int)blocks.size(),
        "rawSpqrBuildOneNode: bad core");
    chk(blocks[core].alive,
        "rawSpqrBuildOneNode: dead core");

    const BlockCore& B = blocks[core];
    chk(!B.realEdges.empty(),
        "rawSpqrBuildOneNode: alive block with empty edge set");

    RawSpqrBuild raw;
    raw.node.resize(1);
    raw.node[0].type = 'R';
    raw.node[0].skel.reserve(B.realEdges.size());

    std::unordered_set<int> seenE;
    for (int e : B.realEdges) {
        chk(0 <= e && e < (int)edges.size(),
            "rawSpqrBuildOneNode: bad real edge id");
        chk(edgeOwnerCore[e] == core,
            "rawSpqrBuildOneNode: edge owner mismatch");
        chk(seenE.insert(e).second,
            "rawSpqrBuildOneNode: duplicated real edge in block");

        int a = edges[e].u;
        int b = edges[e].v;
        chk(a != b,
            "rawSpqrBuildOneNode: self-loop not expected");

        RawSkelEdge sk;
        sk.a = a;
        sk.b = b;
        sk.realEdgeId = e;
        sk.peerNode = -1;
        sk.peerSlot = -1;
        raw.node[0].skel.push_back(sk);
    }
    return raw;
}

Solver::RawSpqrBuild
Solver::rawSpqrBuildTrueSpqrSkeleton(int core) const {
    chk(0 <= core && core < (int)blocks.size(),
        "rawSpqrBuildTrueSpqrSkeleton: bad core");
    chk(blocks[core].alive,
        "rawSpqrBuildTrueSpqrSkeleton: dead core");

    const BlockCore& B = blocks[core];
    std::vector<int> edgeIds = normVec(B.realEdges);
    if ((int)edgeIds.size() <= 1) {
        RawSpqrBuild raw = rawSpqrBuildOneNode(core);
        raw.node[0].type = classifyWholeCoreType(*this, core);
        return raw;
    }

    int sTerm = -1, tTerm = -1;
    std::vector<std::vector<int>> branches;
    if (chooseTopLevelParallelPair(*this, edgeIds, sTerm, tTerm, branches)) {
        RawSpqrBuild raw;
        raw.node.resize(1);
        Solver::RawSpqrNode root;
        root.type = 'P';
        root.skel.reserve(branches.size());
        std::vector<int> rootSlots;
        for (int i = 0; i < (int)branches.size(); ++i) {
            Solver::RawSkelEdge virt;
            virt.a = sTerm;
            virt.b = tTerm;
            virt.realEdgeId = -1;
            virt.peerNode = -1;
            virt.peerSlot = -1;
            rootSlots.push_back((int)root.skel.size());
            root.skel.push_back(virt);
        }
        raw.node[0] = std::move(root);
        for (int i = 0; i < (int)branches.size(); ++i) {
            RawSubtree child = buildTwoTerminalRecursive(*this, branches[i], sTerm, tTerm, true, 0);
            int off = appendShiftedRawBuild(raw, child.raw);
            stitchVirtual(raw, 0, rootSlots[i], off + child.root, child.parentSlot);
        }
        return raw;
    }

    RawSpqrBuild raw = rawSpqrBuildOneNode(core);
    raw.node[0].type = classifyWholeCoreType(*this, core);
    return raw;
}

Solver::RawSpqrBuild
Solver::rawSpqrBuildFromCurrentCore(int core) const {
    switch (rawSpqrMode) {
        case RSB_ONE_NODE:
            return rawSpqrBuildOneNode(core);
        case RSB_TRUE_SPQR:
            return rawSpqrBuildTrueSpqrSkeleton(core);
    }
    failCheck("rawSpqrBuildFromCurrentCore: unknown rawSpqrMode");
}


// ============================================================
// [03] SPQR full rebuild
// ============================================================

void Solver::rebuildBlockSpqrFull(int core) {
    chk(0 <= core && core < (int)blocks.size(),
        "rebuildBlockSpqrFull: bad core");
    chk(blocks[core].alive,
        "rebuildBlockSpqrFull: dead core");

    if (core >= (int)blockSpqr.size()) {
        blockSpqr.resize(core + 1);
    }

    const BlockCore& B = blocks[core];
    chk(!B.realEdges.empty(),
        "rebuildBlockSpqrFull: alive block with empty real edge set");

    RawSpqrBuild raw = rawSpqrBuildFromCurrentCore(core);

#ifdef DEBUG_SOLVER
    checkRawSpqrBuildAgainstCore(core, raw);
#endif

    BlockSpqr T;
    T.alive = true;
    T.rootNode = -1;

    chk(!raw.node.empty(),
        "rebuildBlockSpqrFull: raw builder returned empty node list");

    T.node.resize(raw.node.size());

    for (int u = 0; u < (int)raw.node.size(); ++u) {
        T.node[u].type = raw.node[u].type;
        T.node[u].skel.clear();
        T.node[u].treeAdj.clear();
        T.node[u].verts.clear();
        T.node[u].ownedRealEdges.clear();
        T.node[u].local = NodeLocalSummary{};

        std::unordered_set<int> seenV;

        for (int slot = 0; slot < (int)raw.node[u].skel.size(); ++slot) {
            const auto& rs = raw.node[u].skel[slot];

            chk(rs.a != rs.b,
                "rebuildBlockSpqrFull: raw skeleton self-loop");

            SpqrSkelEdge sk;
            sk.a = rs.a;
            sk.b = rs.b;
            sk.realEdgeId = rs.realEdgeId;
            sk.dirId = -1;

            T.node[u].skel.push_back(sk);

            if (seenV.insert(rs.a).second) T.node[u].verts.push_back(rs.a);
            if (seenV.insert(rs.b).second) T.node[u].verts.push_back(rs.b);
        }

        std::sort(T.node[u].verts.begin(), T.node[u].verts.end());
    }

    std::vector<std::vector<char>> used(raw.node.size());
    for (int u = 0; u < (int)raw.node.size(); ++u) {
        used[u].assign(raw.node[u].skel.size(), 0);
    }

    auto sameUnorderedPair = [&](int a1, int b1, int a2, int b2) -> bool {
        return (a1 == a2 && b1 == b2) || (a1 == b2 && b1 == a2);
    };

    for (int u = 0; u < (int)raw.node.size(); ++u) {
        for (int slotU = 0; slotU < (int)raw.node[u].skel.size(); ++slotU) {
            const auto& su = raw.node[u].skel[slotU];
            if (su.realEdgeId != -1) continue;
            if (used[u][slotU]) continue;

            int v = su.peerNode;
            int slotV = su.peerSlot;

            chk(0 <= v && v < (int)raw.node.size(),
                "rebuildBlockSpqrFull: bad peerNode");
            chk(0 <= slotV && slotV < (int)raw.node[v].skel.size(),
                "rebuildBlockSpqrFull: bad peerSlot");
            chk(!used[v][slotV],
                "rebuildBlockSpqrFull: duplicated virtual pairing");

            const auto& sv = raw.node[v].skel[slotV];

            chk(sv.realEdgeId == -1,
                "rebuildBlockSpqrFull: peer of virtual edge is real");
            chk(sv.peerNode == u && sv.peerSlot == slotU,
                "rebuildBlockSpqrFull: peer symmetry broken");
            chk(sameUnorderedPair(su.a, su.b, sv.a, sv.b),
                "rebuildBlockSpqrFull: virtual pair terminal mismatch");

            int teid = (int)T.tree.size();
            int dirUV = (int)T.dir.size();
            int dirVU = dirUV + 1;

            T.tree.push_back({u, v, dirUV, dirVU});

            T.dir.push_back({u, v, teid, slotU, su.a, su.b});
            T.dir.push_back({v, u, teid, slotV, sv.a, sv.b});

            T.node[u].skel[slotU].dirId = dirUV;
            T.node[v].skel[slotV].dirId = dirVU;

            T.node[u].treeAdj.push_back(teid);
            T.node[v].treeAdj.push_back(teid);

            used[u][slotU] = 1;
            used[v][slotV] = 1;
        }
    }

    chk((int)T.tree.size() == (int)T.node.size() - 1,
        "rebuildBlockSpqrFull: stitched raw node graph is not a tree by edge count");

    std::unordered_set<int> aliveEdgeSet(B.realEdges.begin(), B.realEdges.end());
    std::unordered_set<int> seenOwned;

    for (int u = 0; u < (int)T.node.size(); ++u) {
        auto& N = T.node[u];
        N.ownedRealEdges.clear();

        for (const auto& sk : N.skel) {
            if (sk.realEdgeId == -1) continue;

            int e = sk.realEdgeId;
            chk(aliveEdgeSet.count(e),
                "rebuildBlockSpqrFull: raw builder returned edge not in oldCore");
            chk(seenOwned.insert(e).second,
                "rebuildBlockSpqrFull: real edge appears in multiple SPQR nodes");

            N.ownedRealEdges.push_back(e);
        }

        std::sort(N.ownedRealEdges.begin(), N.ownedRealEdges.end());
    }

    chk(seenOwned.size() == aliveEdgeSet.size(),
        "rebuildBlockSpqrFull: owned real edge coverage mismatch");

    std::unordered_set<int> oldBoundarySet;
    oldBoundarySet.reserve(B.attachCuts.size() * 2 + 1);

    for (int cutBC : B.attachCuts) {
        chk(0 <= cutBC && cutBC < (int)bcNodes.size(),
            "rebuildBlockSpqrFull: bad cutBC in attachCuts");
        chk(bcNodes[cutBC].alive && bcNodes[cutBC].type == BCN_CUT,
            "rebuildBlockSpqrFull: attachCuts contains invalid CUT");

        oldBoundarySet.insert(bcNodes[cutBC].origVertex);
    }

    for (int u = 0; u < (int)T.node.size(); ++u) {
        auto& N = T.node[u];

        N.local.realEdgeCount = (int)N.ownedRealEdges.size();
        N.local.watchedHandleCount = 0;
        N.local.minRealEdge = INT_MAX;
        N.local.hasOldBoundary = false;

        for (int e : N.ownedRealEdges) {
            N.local.minRealEdge = std::min(N.local.minRealEdge, e);

            int h = edges[e].handleId;
            if (h != -1 &&
                handles[h].watched &&
                handleOwnerCore[h] == core) {
                N.local.watchedHandleCount++;
            }
        }

        for (int v : N.verts) {
            if (oldBoundarySet.count(v)) {
                N.local.hasOldBoundary = true;
                break;
            }
        }
    }

    T.occNodeOfVertex.clear();

    for (int u = 0; u < (int)T.node.size(); ++u) {
        for (int v : T.node[u].verts) {
            T.occNodeOfVertex[v].push_back(u);
        }
    }

    for (auto& [v, vec] : T.occNodeOfVertex) {
        vec = normVec(std::move(vec));
    }

    T.rootNode = 0;
    rebuildSideBagSummaries(core, T);

    blockSpqr[core] = std::move(T);
}

void Solver::rebuildSideBagSummaries(int core, BlockSpqr& T) {
    chk(0 <= core && core < (int)blocks.size(),
        "rebuildSideBagSummaries: bad core");
    chk(blocks[core].alive,
        "rebuildSideBagSummaries: dead core");
    chk(T.alive,
        "rebuildSideBagSummaries: dead BlockSpqr");

    int n = (int)T.node.size();
    chk(n > 0,
        "rebuildSideBagSummaries: empty SPQR node set");

    T.bag.assign(T.dir.size(), emptyBagSummary());

    if (n == 1) {
        return;
    }

    int root = T.rootNode;
    chk(0 <= root && root < n,
        "rebuildSideBagSummaries: bad rootNode");

    std::vector<int> parent(n, -1);
    std::vector<int> parentTeid(n, -1);
    std::vector<int> order;
    order.reserve(n);

    std::stack<int> st;
    st.push(root);
    parent[root] = root;

    while (!st.empty()) {
        int u = st.top();
        st.pop();

        order.push_back(u);

        for (int teid : T.node[u].treeAdj) {
            int v = otherNode(T, teid, u);
            if (parent[v] != -1) continue;

            parent[v] = u;
            parentTeid[v] = teid;
            st.push(v);
        }
    }

    chk((int)order.size() == n,
        "rebuildSideBagSummaries: SPQR node graph must be connected");

    std::vector<SideBagSummary> down(n, emptyBagSummary());

    for (int it = n - 1; it >= 0; --it) {
        int u = order[it];

        SideBagSummary cur = emptyBagSummary();
        cur = mergeBagSummary(cur, SideBagSummary{
            T.node[u].local.realEdgeCount,
            T.node[u].local.watchedHandleCount,
            T.node[u].local.minRealEdge,
            T.node[u].local.hasOldBoundary
        });

        for (int teid : T.node[u].treeAdj) {
            int v = otherNode(T, teid, u);
            if (v == parent[u]) continue;
            cur = mergeBagSummary(cur, down[v]);
        }

        down[u] = cur;
    }

    std::vector<SideBagSummary> up(n, emptyBagSummary());
    up[root] = emptyBagSummary();

    for (int idx = 0; idx < n; ++idx) {
        int u = order[idx];
        int deg = (int)T.node[u].treeAdj.size();

        std::vector<SideBagSummary> contrib(deg, emptyBagSummary());
        std::vector<int> neigh(deg, -1);

        for (int i = 0; i < deg; ++i) {
            int teid = T.node[u].treeAdj[i];
            int v = otherNode(T, teid, u);
            int d = dirFromNode(T, teid, u);

            neigh[i] = v;

            if (v == parent[u]) {
                contrib[i] = up[u];
            } else {
                contrib[i] = down[v];
            }

            T.bag[d] = contrib[i];
        }

        std::vector<SideBagSummary> pref(deg + 1, emptyBagSummary());
        std::vector<SideBagSummary> suff(deg + 1, emptyBagSummary());

        for (int i = 0; i < deg; ++i) {
            pref[i + 1] = mergeBagSummary(pref[i], contrib[i]);
        }
        for (int i = deg - 1; i >= 0; --i) {
            suff[i] = mergeBagSummary(contrib[i], suff[i + 1]);
        }

        for (int i = 0; i < deg; ++i) {
            int v = neigh[i];
            if (v == parent[u]) continue;

            SideBagSummary exceptV = mergeBagSummary(pref[i], suff[i + 1]);

            SideBagSummary sideFromVToU = emptyBagSummary();
            sideFromVToU = mergeBagSummary(sideFromVToU, SideBagSummary{
                T.node[u].local.realEdgeCount,
                T.node[u].local.watchedHandleCount,
                T.node[u].local.minRealEdge,
                T.node[u].local.hasOldBoundary
            });
            sideFromVToU = mergeBagSummary(sideFromVToU, exceptV);

            up[v] = sideFromVToU;

            int revDir = dirFromNode(T, parentTeid[v], v);
            T.bag[revDir] = up[v];
        }
    }
}

void Solver::rebuildAllAliveBlockSpqr() {
    if ((int)blockSpqr.size() < (int)blocks.size()) {
        blockSpqr.resize(blocks.size());
    }

    for (int c = 0; c < (int)blocks.size(); ++c) {
        if (blocks[c].alive) {
            rebuildBlockSpqrFull(c);
        } else {
            blockSpqr[c] = BlockSpqr{};
            blockSpqr[c].alive = false;
        }
    }
}

// ============================================================
// [04] on-demand bag expansion
// ============================================================

void Solver::collectSideBagExpanded(int core,
                                    int dirId,
                                    ExpandedBag& out) const {
    chk(0 <= core && core < (int)blocks.size(),
        "collectSideBagExpanded: bad core");
    chk(blocks[core].alive,
        "collectSideBagExpanded: dead core");
    chk(0 <= core && core < (int)blockSpqr.size(),
        "collectSideBagExpanded: missing BlockSpqr");
    chk(blockSpqr[core].alive,
        "collectSideBagExpanded: dead BlockSpqr");
    chk(0 <= dirId && dirId < (int)blockSpqr[core].dir.size(),
        "collectSideBagExpanded: bad dirId");

    const BlockCore& B = blocks[core];
    const BlockSpqr& T = blockSpqr[core];
    const SpqrDir& D = T.dir[dirId];

    out.realEdges.clear();
    out.watchedHandles.clear();
    out.allVertices.clear();
    out.oldBoundaryVerts.clear();

    unordered_set<int> oldBoundarySet;
    oldBoundarySet.reserve(B.attachCuts.size() * 2 + 1);

    for (int cutBC : B.attachCuts) {
        chk(0 <= cutBC && cutBC < (int)bcNodes.size(),
            "collectSideBagExpanded: bad cutBC");
        chk(bcNodes[cutBC].alive && bcNodes[cutBC].type == BCN_CUT,
            "collectSideBagExpanded: attachCuts contains invalid CUT");

        oldBoundarySet.insert(bcNodes[cutBC].origVertex);
    }

    vector<char> visNode(T.node.size(), 0);
    struct Frame { int u, parent; };

    stack<Frame> st;
    st.push({D.toNode, D.fromNode});
    visNode[D.toNode] = 1;

    unordered_set<int> seenE, seenH, seenV, seenOldB;

    while (!st.empty()) {
        auto [u, parent] = st.top();
        st.pop();

        chk(0 <= u && u < (int)T.node.size(),
            "collectSideBagExpanded: bad SPQR node id in DFS");

        const SpqrNode& N = T.node[u];

        for (int e : N.ownedRealEdges) {
            if (seenE.insert(e).second) {
                out.realEdges.push_back(e);

                int h = edges[e].handleId;
                if (h != -1 &&
                    handles[h].watched &&
                    handleOwnerCore[h] == core &&
                    seenH.insert(h).second) {
                    out.watchedHandles.push_back(h);
                }
            }
        }

        for (int v : N.verts) {
            if (seenV.insert(v).second) {
                out.allVertices.push_back(v);
            }
            if (oldBoundarySet.count(v) && seenOldB.insert(v).second) {
                out.oldBoundaryVerts.push_back(v);
            }
        }

        for (int teid : N.treeAdj) {
            int w = otherNode(T, teid, u);
            if (w == parent) continue;
            if (visNode[w]) continue;

            visNode[w] = 1;
            st.push({w, u});
        }
    }

    sort(out.realEdges.begin(), out.realEdges.end());
    sort(out.watchedHandles.begin(), out.watchedHandles.end());
    sort(out.allVertices.begin(), out.allVertices.end());
    sort(out.oldBoundaryVerts.begin(), out.oldBoundaryVerts.end());
}

// ============================================================
// [05] split helpers
// ============================================================

void Solver::collectOldBoundary(const BlockCore& B, int x, SplitCtx& C) const {
    for (int cutBC : B.attachCuts) {
        chk(0 <= cutBC && cutBC < (int)bcNodes.size(),
            "collectOldBoundary: bad cutBC");
        chk(bcNodes[cutBC].alive && bcNodes[cutBC].type == BCN_CUT,
            "collectOldBoundary: attachCuts contains invalid CUT");

        int v = bcNodes[cutBC].origVertex;
        if (v == x) continue;
        if (!orig[v].alive) continue;

        C.boundary[v].existedOldCut = true;
    }
}

void Solver::collectDeadEdgesIncidentToX(const BlockCore& B, int x, SplitCtx& C) const {
    C.deadEdges.clear();

    for (int e : B.realEdges) {
        chk(0 <= e && e < (int)edges.size(),
            "collectDeadEdgesIncidentToX: bad real edge id");

        if (edges[e].u == x || edges[e].v == x) {
            C.deadEdges.push_back(e);
        }
    }

    C.deadEdges = normVec(move(C.deadEdges));
}

Solver::AffectedRegion
Solver::exposeAffectedRegion(const BlockSpqr& T, int x) const {
    AffectedRegion A;

    auto it = T.occNodeOfVertex.find(x);
    chk(it != T.occNodeOfVertex.end(),
        "exposeAffectedRegion: x must appear in some SPQR node");

    A.nodes = normVec(it->second);
    chk(!A.nodes.empty(),
        "exposeAffectedRegion: empty occurrence set");

    A.inRegion.assign(T.node.size(), 0);
    for (int u : A.nodes) {
        chk(0 <= u && u < (int)T.node.size(),
            "exposeAffectedRegion: bad node id in occNodeOfVertex[x]");
        A.inRegion[u] = 1;
    }

    for (int u : A.nodes) {
        for (int teid : T.node[u].treeAdj) {
            int v = otherNode(T, teid, u);
            if (A.inRegion[v]) continue;

            int dirId = dirFromNode(T, teid, u);
            chk(0 <= dirId && dirId < (int)T.dir.size(),
                "exposeAffectedRegion: bad dirId");

            const auto& d = T.dir[dirId];

            chk(d.termA != x && d.termB != x,
                "exposeAffectedRegion: exit pair must not contain x");

            A.exits.push_back({dirId, d.termA, d.termB});
        }

        for (int e : T.node[u].ownedRealEdges) {
            A.ownedRealEdges.push_back(e);
        }
    }

    A.ownedRealEdges = normVec(move(A.ownedRealEdges));
    sort(A.exits.begin(), A.exits.end(), [&](const ExitRef& L, const ExitRef& R) {
        if (L.dirId != R.dirId) return L.dirId < R.dirId;
        if (L.termA != R.termA) return L.termA < R.termA;
        return L.termB < R.termB;
    });

    return A;
}

Solver::CompGraph
Solver::buildCompressedGraph(const AffectedRegion& A, int x) const {
    CompGraph G;

    auto ensureV = [&](int v) -> int {
        auto it = G.lid.find(v);
        if (it != G.lid.end()) return it->second;

        int id = (int)G.realOf.size();
        G.lid[v] = id;
        G.realOf.push_back(v);
        G.adj.push_back({});
        G.deg.push_back(0);
        return id;
    };

    auto addItem = [&](AtomKind kind, int ref, int a, int b) {
        chk(a != x && b != x,
            "buildCompressedGraph: x must be removed already");
        chk(a != b,
            "buildCompressedGraph: self-loop compressed item");

        int la = ensureV(a);
        int lb = ensureV(b);

        int id = (int)G.item.size();
        G.item.push_back({a, b, kind, ref});

        G.adj[la].push_back({lb, id});
        G.adj[lb].push_back({la, id});
        G.deg[la]++;
        G.deg[lb]++;
    };

    for (int e : A.ownedRealEdges) {
        int a = edges[e].u;
        int b = edges[e].v;
        if (a == x || b == x) continue;
        addItem(AT_REAL_EDGE, e, a, b);
    }

    for (int exIdx = 0; exIdx < (int)A.exits.size(); ++exIdx) {
        int a = A.exits[exIdx].termA;
        int b = A.exits[exIdx].termB;
        addItem(AT_EXIT_BAG, exIdx, a, b);
    }

    return G;
}

vector<vector<int>> Solver::runCompressedBCC(const CompGraph& G) const {
    int n = (int)G.realOf.size();

    vector<int> tin(n, 0), low(n, 0), estk;
    vector<vector<int>> comps;
    int timer = 0;

    function<void(int, int)> dfs = [&](int u, int peid) {
        tin[u] = low[u] = ++timer;

        for (auto [v, itemId] : G.adj[u]) {
            if (itemId == peid) continue;

            if (!tin[v]) {
                estk.push_back(itemId);
                dfs(v, itemId);

                low[u] = min(low[u], low[v]);

                if (low[v] >= tin[u]) {
                    vector<int> comp;
                    while (true) {
                        int top = estk.back();
                        estk.pop_back();
                        comp.push_back(top);
                        if (top == itemId) break;
                    }
                    comp = normVec(move(comp));
                    comps.push_back(move(comp));
                }
            } else if (tin[v] < tin[u]) {
                estk.push_back(itemId);
                low[u] = min(low[u], tin[v]);
            }
        }
    };

    for (int s = 0; s < n; ++s) {
        if (!tin[s] && G.deg[s] > 0) {
            dfs(s, -1);
        }
    }

    sort(comps.begin(), comps.end());
    return comps;
}

void Solver::enumerateFragmentsAfterDelete(const BlockSpqr& T,
                                           const AffectedRegion& A,
                                           const BlockCore& B,
                                           SplitCtx& C) const {
    (void)B;
    CompGraph G = buildCompressedGraph(A, C.x);
    std::vector<std::vector<int>> comps = runCompressedBCC(G);

    C.frags.clear();

    // Summary-first phase: exit bags contribute only compact summaries.
    for (const auto& comp : comps) {
        FragBuild F;

        std::unordered_set<int> seenRealEdge, seenRealHandle;
        std::unordered_set<int> seenVisible, seenBoundary;
        long long scoreEdges = 0;
        long long scoreHandles = 0;

        for (int itemId : comp) {
            chk(0 <= itemId && itemId < (int)G.item.size(),
                "enumerateFragmentsAfterDelete: bad compressed item id");
            const auto& it = G.item[itemId];
            F.atoms.push_back({it.kind, it.ref});

            if (it.kind == AT_REAL_EDGE) {
                int e = it.ref;
                if (seenRealEdge.insert(e).second) {
                    F.edges.push_back(e);
                    F.keyMinRealEdge = std::min(F.keyMinRealEdge, e);
                    scoreEdges += 1;
                }
                int h = edges[e].handleId;
                if (h != -1 && h < (int)handles.size() && handles[h].watched &&
                    h < (int)handleOwnerCore.size() && handleOwnerCore[h] == C.oldCore &&
                    seenRealHandle.insert(h).second) {
                    F.handles.push_back(h);
                    scoreHandles += 1;
                }
                for (int v : {edges[e].u, edges[e].v}) {
                    if (v == C.x) continue;
                    if (seenVisible.insert(v).second) F.visibleVerts.push_back(v);
                    if (seenBoundary.insert(v).second) F.boundaryMembers.push_back(v);
                }
            } else {
                int exIdx = it.ref;
                chk(0 <= exIdx && exIdx < (int)A.exits.size(),
                    "enumerateFragmentsAfterDelete: bad exit index");
                const ExitRef& ex = A.exits[exIdx];
                chk(0 <= ex.dirId && ex.dirId < (int)T.bag.size(),
                    "enumerateFragmentsAfterDelete: bad exit dirId");
                const SideBagSummary& bag = T.bag[ex.dirId];
                scoreEdges += bag.realEdgeCount;
                scoreHandles += bag.watchedHandleCount;
                F.keyMinRealEdge = std::min(F.keyMinRealEdge, bag.minRealEdge);
                for (int v : {ex.termA, ex.termB}) {
                    if (v == C.x) continue;
                    if (seenVisible.insert(v).second) F.visibleVerts.push_back(v);
                    if (seenBoundary.insert(v).second) F.boundaryMembers.push_back(v);
                }
            }
        }

        F.edges = normVec(std::move(F.edges));
        F.handles = normVec(std::move(F.handles));
        F.visibleVerts = normVec(std::move(F.visibleVerts));
        F.boundaryMembers = normVec(std::move(F.boundaryMembers));

        chk(F.keyMinRealEdge != std::numeric_limits<int>::max(),
            "enumerateFragmentsAfterDelete: summary fragment got no key edge");

        F.mass = scoreEdges + scoreHandles + (long long)F.visibleVerts.size();
        C.frags.push_back(std::move(F));
    }

    // Summary boundary incidence from visible terminals only.
    std::unordered_map<int, std::vector<int>> inc;
    for (int fid = 0; fid < (int)C.frags.size(); ++fid) {
        for (int v : C.frags[fid].boundaryMembers) {
            inc[v].push_back(fid);
        }
    }
    for (auto& [v, vec] : inc) {
        vec = normVec(std::move(vec));
        auto it = C.boundary.find(v);
        if (it != C.boundary.end()) {
            it->second.fragIds = vec;
        } else if ((int)vec.size() >= 2) {
            BoundaryAcc acc;
            acc.existedOldCut = false;
            acc.fragIds = std::move(vec);
            C.boundary[v] = std::move(acc);
        }
    }

    C.isolatedExclusive.clear();
}

void Solver::chooseKeepFragment(SplitCtx& C) const {
    if (C.frags.empty()) {
        C.keepFrag = -1;
        return;
    }

    auto better = [&](int a, int b) -> bool {
        const auto& A = C.frags[a];
        const auto& B = C.frags[b];

        if (A.mass != B.mass) return A.mass > B.mass;
        if (A.keyMinRealEdge != B.keyMinRealEdge) {
            return A.keyMinRealEdge < B.keyMinRealEdge;
        }
        if (A.visibleVerts != B.visibleVerts) {
            return A.visibleVerts < B.visibleVerts;
        }
        return A.atoms.size() < B.atoms.size();
    };

    int best = 0;
    for (int i = 1; i < (int)C.frags.size(); ++i) {
        if (better(i, best)) best = i;
    }
    C.keepFrag = best;
}

Solver::SparsePatch
Solver::materializeSparsePatch(const BlockCore& B,
                               const BlockSpqr& T,
                               const AffectedRegion& A,
                               const SplitCtx& C) const {
    struct ExactFrag {
        std::vector<int> edges;
        std::vector<int> handles;
        std::vector<int> allVertices;
        std::vector<int> oldBoundaryVerts;
    };

    std::unordered_map<int, ExpandedBag> expandedBagCache;

    auto getBag = [&](int exIdx) -> const ExpandedBag& {
        chk(0 <= exIdx && exIdx < (int)A.exits.size(),
            "materializeSparsePatch: bad exit index");
        int dirId = A.exits[exIdx].dirId;
        auto it = expandedBagCache.find(dirId);
        if (it == expandedBagCache.end()) {
            ExpandedBag bag;
            collectSideBagExpanded(C.oldCore, dirId, bag);
            it = expandedBagCache.emplace(dirId, std::move(bag)).first;
        }
        return it->second;
    };

    auto expandFrag = [&](int fid, bool oldBoundaryOnly) -> ExactFrag {
        chk(0 <= fid && fid < (int)C.frags.size(),
            "materializeSparsePatch: bad frag id");
        ExactFrag out;
        std::unordered_set<int> seenE, seenH, seenV, seenOldB;

        for (const auto& at : C.frags[fid].atoms) {
            if (at.kind == AT_REAL_EDGE) {
                int e = at.ref;
                if (!oldBoundaryOnly && seenE.insert(e).second) {
                    out.edges.push_back(e);
                    int h = edges[e].handleId;
                    if (h != -1 && h < (int)handles.size() && handles[h].watched &&
                        h < (int)handleOwnerCore.size() && handleOwnerCore[h] == C.oldCore &&
                        seenH.insert(h).second) {
                        out.handles.push_back(h);
                    }
                }
                if (!oldBoundaryOnly) {
                    for (int v : {edges[e].u, edges[e].v}) {
                        if (v == C.x) continue;
                        if (seenV.insert(v).second) out.allVertices.push_back(v);
                    }
                }
            } else {
                int exIdx = at.ref;
                const ExpandedBag& bag = getBag(exIdx);
                if (!oldBoundaryOnly) {
                    for (int e : bag.realEdges) {
                        if (seenE.insert(e).second) out.edges.push_back(e);
                    }
                    for (int h : bag.watchedHandles) {
                        if (seenH.insert(h).second) out.handles.push_back(h);
                    }
                    for (int v : bag.allVertices) {
                        if (v == C.x) continue;
                        if (seenV.insert(v).second) out.allVertices.push_back(v);
                    }
                }
                for (int v : bag.oldBoundaryVerts) {
                    if (v == C.x) continue;
                    if (seenOldB.insert(v).second) out.oldBoundaryVerts.push_back(v);
                }
            }
        }

        out.edges = normVec(std::move(out.edges));
        out.handles = normVec(std::move(out.handles));
        out.allVertices = normVec(std::move(out.allVertices));
        out.oldBoundaryVerts = normVec(std::move(out.oldBoundaryVerts));
        return out;
    };

    SparsePatch P;
    P.oldCoreId = C.oldCore;
    P.deletedVertex = C.x;
    P.keepExists = !C.frags.empty();
    P.keepMatPiece = C.keepFrag;

    // Local boundary map starts from summary-visible incidence.
    std::unordered_map<int, BoundaryAcc> boundary = C.boundary;

    std::vector<ExactFrag> exact(C.frags.size());
    std::vector<char> haveExact(C.frags.size(), 0);

    // Expand non-keep fragments exactly, keep only as much as needed for deep old-boundary hits.
    for (int fid = 0; fid < (int)C.frags.size(); ++fid) {
        if (fid == C.keepFrag) continue;
        exact[fid] = expandFrag(fid, false);
        haveExact[fid] = 1;
        for (int v : exact[fid].oldBoundaryVerts) {
            boundary[v].existedOldCut = true;
            boundary[v].fragIds.push_back(fid);
        }
    }
    if (C.keepFrag != -1) {
        ExactFrag keepOld = expandFrag(C.keepFrag, true);
        for (int v : keepOld.oldBoundaryVerts) {
            boundary[v].existedOldCut = true;
            boundary[v].fragIds.push_back(C.keepFrag);
        }
    }
    for (auto& [v, acc] : boundary) {
        acc.fragIds = normVec(std::move(acc.fragIds));
    }

    std::unordered_set<int> boundarySet;
    for (const auto& [v, acc] : boundary) boundarySet.insert(v);

    std::vector<int> fragToSmall(C.frags.size(), -1);
    for (int fid = 0; fid < (int)C.frags.size(); ++fid) {
        if (fid == C.keepFrag) continue;
        chk(haveExact[fid],
            "materializeSparsePatch: missing exact expansion for non-keep frag");
        SparsePiece sp;
        sp.matPieceId = fid;
        sp.edges = exact[fid].edges;
        sp.watchedHandles = exact[fid].handles;
        for (int v : exact[fid].allVertices) {
            if (!boundarySet.count(v)) sp.exclusiveVertices.push_back(v);
        }
        sp.exclusiveVertices = normVec(std::move(sp.exclusiveVertices));
        fragToSmall[fid] = (int)P.small.size();
        P.small.push_back(std::move(sp));
    }

    std::vector<int> boundaryVerts;
    boundaryVerts.reserve(boundary.size());
    for (const auto& [v, acc] : boundary) boundaryVerts.push_back(v);
    std::sort(boundaryVerts.begin(), boundaryVerts.end());

    for (int v : boundaryVerts) {
        const auto& acc = boundary.at(v);
        std::vector<int> fragIds = normVec(acc.fragIds);

        SparseBoundary bd;
        bd.vertex = v;
        bd.existedOldCut = acc.existedOldCut;
        bd.touchesKeep = false;

        for (int fid : fragIds) {
            if (fid == C.keepFrag) {
                bd.touchesKeep = true;
            } else {
                int sid = fragToSmall[fid];
                chk(sid != -1,
                    "materializeSparsePatch: non-keep frag missing small id");
                bd.smallIds.push_back(sid);
            }
        }
        bd.smallIds = normVec(std::move(bd.smallIds));
        if (bd.existedOldCut || bd.touchesKeep || !bd.smallIds.empty()) {
            P.boundary.push_back(std::move(bd));
        }
    }

    P.deadEdges = normVec(C.deadEdges);
    P.deadHandles.clear();

    // isolatedExclusive from exact surviving edge endpoints + boundary set.
    std::unordered_set<int> usedV;
    std::unordered_set<int> deadE(C.deadEdges.begin(), C.deadEdges.end());
    for (int e : B.realEdges) {
        if (deadE.count(e)) continue;
        usedV.insert(edges[e].u);
        usedV.insert(edges[e].v);
    }
    for (int v : B.allVertices) {
        if (v == C.x) continue;
        if (v < 0 || v >= (int)orig.size() || !orig[v].alive) continue;
        if (usedV.count(v)) continue;
        if (boundarySet.count(v)) continue;
        P.isolatedExclusive.push_back(v);
    }
    P.isolatedExclusive = normVec(std::move(P.isolatedExclusive));

    P.deadExclusiveVertices.push_back(C.x);
    for (int v : P.isolatedExclusive) {
        if (v != C.x) P.deadExclusiveVertices.push_back(v);
    }
    P.deadExclusiveVertices = normVec(std::move(P.deadExclusiveVertices));

    return P;
}

// ============================================================
// [06] final bring-up splitter
// ============================================================

bool Solver::shouldUseTrueSpqr(int oldCore) const {
    chk(0 <= oldCore && oldCore < (int)blocks.size(),
        "shouldUseTrueSpqr: bad oldCore");
#if DENSE_RECT_ROUND1_PROFILE
    dense_rect_round1_prof::G().should_calls++;
#endif
#if DENSE_SPQR_ROUND16_PROFILE
    dense_spqr_round16_prof::G().should_calls++;
#endif
    const BlockCore& B = blocks[oldCore];
    if (!B.alive) {
#if DENSE_RECT_ROUND1_PROFILE
        dense_rect_round1_prof::G().should_false++;
        dense_rect_round1_prof::G().should_false_dead++;
#endif
#if DENSE_SPQR_ROUND16_PROFILE
        ++dense_spqr_round16_prof::G().should_false;
        ++dense_spqr_round16_prof::G().should_false_other;
#endif
        return false;
    }
    if (B.badQueries.empty()) {
#if DENSE_RECT_ROUND1_PROFILE
        dense_rect_round1_prof::G().should_false++;
        dense_rect_round1_prof::G().should_false_no_bad++;
#endif
#if DENSE_SPQR_ROUND16_PROFILE
        ++dense_spqr_round16_prof::G().should_false;
        ++dense_spqr_round16_prof::G().should_false_other;
#endif
        return false;
    }

    int E = (int)B.realEdges.size();
    int V = (int)B.allVertices.size();
    int Q = (int)B.badQueries.size();

    if (E > 96) {
#if DENSE_RECT_ROUND1_PROFILE
        dense_rect_round1_prof::G().should_false++;
        dense_rect_round1_prof::G().should_false_e++;
#endif
#if DENSE_SPQR_ROUND16_PROFILE
        ++dense_spqr_round16_prof::G().should_false;
        ++dense_spqr_round16_prof::G().should_false_e;
#endif
        return false;
    }
    if (V > 80) {
#if DENSE_RECT_ROUND1_PROFILE
        dense_rect_round1_prof::G().should_false++;
        dense_rect_round1_prof::G().should_false_v++;
#endif
#if DENSE_SPQR_ROUND16_PROFILE
        ++dense_spqr_round16_prof::G().should_false;
        ++dense_spqr_round16_prof::G().should_false_v;
#endif
        return false;
    }
    if (Q > 24) {
#if DENSE_RECT_ROUND1_PROFILE
        dense_rect_round1_prof::G().should_false++;
        dense_rect_round1_prof::G().should_false_q++;
#endif
#if DENSE_SPQR_ROUND16_PROFILE
        ++dense_spqr_round16_prof::G().should_false;
        ++dense_spqr_round16_prof::G().should_false_q;
#endif
        return false;
    }

    if (E > V + 24) {
#if DENSE_RECT_ROUND1_PROFILE
        dense_rect_round1_prof::G().should_false++;
        dense_rect_round1_prof::G().should_false_dense++;
#endif
#if DENSE_SPQR_ROUND16_PROFILE
        ++dense_spqr_round16_prof::G().should_false;
        ++dense_spqr_round16_prof::G().should_false_dense;
#endif
        return false;
    }

#if DENSE_RECT_ROUND1_PROFILE
    dense_rect_round1_prof::G().should_true++;
#endif
#if DENSE_SPQR_ROUND16_PROFILE
    ++dense_spqr_round16_prof::G().should_true;
#endif
    return true;
}

bool Solver::denseStructuredTrueSpqrEscapeEligible(int oldCore, int x, DenseStructuredGateInfo* out) const {
    chk(0 <= oldCore && oldCore < (int)blocks.size(),
        "denseStructuredTrueSpqrEscapeEligible: bad oldCore");
    const BlockCore& B = blocks[oldCore];
    DenseStructuredGateInfo info;
    info.attachCutsSize = (int)B.attachCuts.size();
    info.badQueriesSize = (int)B.badQueries.size();
    info.boundarySize = (int)B.attachCuts.size();
    info.eOverCurrentThreshold = ((int)B.realEdges.size() > 96);
    info.qOverCurrentThreshold = ((int)B.badQueries.size() > 24);

    int oldBoundaryCount = 0;
    for (int cutBC : B.attachCuts) {
        if (!(0 <= cutBC && cutBC < (int)bcNodes.size())) continue;
        int v = bcNodes[cutBC].origVertex;
        if (v == x) continue;
        if (1 <= v && v <= N && orig[v].alive) ++oldBoundaryCount;
    }
    info.oldBoundarySize = oldBoundaryCount;
    info.boundaryZero = (oldBoundaryCount == 0);
    info.boundarySize = oldBoundaryCount;

    static thread_local std::vector<int> mark;
    static thread_local std::vector<int> deg;
    static thread_local std::vector<int> touched;
    static thread_local int epoch = 1;
    if ((int)mark.size() < (int)orig.size()) {
        mark.assign(orig.size(), 0);
        deg.assign(orig.size(), 0);
        epoch = 1;
    }
    ++epoch;
    if (epoch == INT_MAX) {
        std::fill(mark.begin(), mark.end(), 0);
        epoch = 1;
    }
    touched.clear();
    int survE = 0;
    int xNbrCount = 0;
    for (int e : B.realEdges) {
        int a = edges[e].u, b = edges[e].v;
        if (a == x || b == x) {
            ++xNbrCount;
            continue;
        }
        if (!(1 <= a && a <= N && 1 <= b && b <= N && orig[a].alive && orig[b].alive)) continue;
        if (mark[a] != epoch) { mark[a] = epoch; deg[a] = 0; touched.push_back(a); }
        if (mark[b] != epoch) { mark[b] = epoch; deg[b] = 0; touched.push_back(b); }
        ++deg[a];
        ++deg[b];
        ++survE;
    }
    int nonIso = 0;
    std::vector<int> degList;
    degList.reserve(touched.size());
    for (int v : touched) {
        if (deg[v] > 0) {
            ++nonIso;
            degList.push_back(deg[v]);
        }
    }
    info.currentV = nonIso;
    info.currentE = survE;
    info.xNbrCount = xNbrCount;
    auto curKey = statecert_fastkey::buildKey(degList, nonIso, survE, oldBoundaryCount, xNbrCount);
    info.ccOne = curKey.ccOne;
    info.currentOnComb = (curKey.boundaryZero && curKey.ccOne && curKey.majorDefBucket <= 3 && curKey.xDegBucket <= 3);

#if DENSE_SPQR_ROUND16_PROFILE
    auto& g = dense_spqr_round16_prof::G();
    ++g.gate_samples;
    g.attachcuts_total += info.attachCutsSize;
    g.attachcuts_max = std::max(g.attachcuts_max, (long long)info.attachCutsSize);
    g.badqueries_total += info.badQueriesSize;
    g.badqueries_max = std::max(g.badqueries_max, (long long)info.badQueriesSize);
    g.boundary_size_total += info.boundarySize;
    g.boundary_size_max = std::max(g.boundary_size_max, (long long)info.boundarySize);
    g.oldboundary_size_total += info.oldBoundarySize;
    g.oldboundary_size_max = std::max(g.oldboundary_size_max, (long long)info.oldBoundarySize);
    g.current_v_total += info.currentV;
    g.current_v_max = std::max(g.current_v_max, (long long)info.currentV);
    g.current_e_total += info.currentE;
    g.current_e_max = std::max(g.current_e_max, (long long)info.currentE);
    if (info.boundaryZero) ++g.boundary_zero_hits;
    if (info.ccOne) ++g.cc_one_hits;
    if (info.currentOnComb) ++g.current_on_comb_hits;
    if (info.eOverCurrentThreshold) ++g.e_over_current_threshold;
    if (info.qOverCurrentThreshold) ++g.q_over_current_threshold;
#endif

    bool gate = true;
    gate = gate && info.boundaryZero;
    gate = gate && info.ccOne;
    gate = gate && info.currentOnComb;
    gate = gate && info.badQueriesSize > 0 && info.badQueriesSize <= 12;
    gate = gate && info.currentE > 96 && info.currentE <= 192;
    gate = gate && info.currentV <= 160;
    gate = gate && info.attachCutsSize <= 1;
    gate = gate && xNbrCount <= 7;
    gate = gate && survE > 0;
    if (out) *out = info;
#if DENSE_SPQR_ROUND16_PROFILE
    if (gate) ++dense_spqr_round16_prof::G().candidate_dense_structured_gate_hit;
#endif
    return gate;
}

Solver::SparsePatch Solver::splitBlockSPQRForced(int oldCore, int x) const {
#if DENSE_SPQR_ROUND16_PROFILE
    auto __r16_spqr_total_t0 = dense_spqr_round16_prof::Clock::now();
    auto __r16_spqr_build_t0 = dense_spqr_round16_prof::Clock::now();
#endif
    Solver* self = const_cast<Solver*>(this);
    self->rebuildBlockSpqrFull(oldCore);
#if DENSE_SPQR_ROUND16_PROFILE
    dense_spqr_round16_prof::G().spqr_build_total_ns += dense_spqr_round16_prof::nsSince(__r16_spqr_build_t0);
#endif

    chk(0 <= oldCore && oldCore < (int)blockSpqr.size(),
        "splitBlockSPQR: missing BlockSpqr");
    chk(blockSpqr[oldCore].alive,
        "splitBlockSPQR: dead BlockSpqr");

    const BlockCore& B = blocks[oldCore];
    const BlockSpqr& T = blockSpqr[oldCore];

    SplitCtx C;
    C.oldCore = oldCore;
    C.x = x;

    collectOldBoundary(B, x, C);
    collectDeadEdgesIncidentToX(B, x, C);
    AffectedRegion A = exposeAffectedRegion(T, x);
#if DENSE_SPQR_ROUND16_PROFILE
    auto __r16_spqr_keep_t0 = dense_spqr_round16_prof::Clock::now();
#endif
    enumerateFragmentsAfterDelete(T, A, B, C);
    chooseKeepFragment(C);
#if DENSE_SPQR_ROUND16_PROFILE
    dense_spqr_round16_prof::G().spqr_keep_selection_total_ns += dense_spqr_round16_prof::nsSince(__r16_spqr_keep_t0);
    auto __r16_spqr_piece_t0 = dense_spqr_round16_prof::Clock::now();
#endif
    SparsePatch P = materializeSparsePatch(B, T, A, C);
#if DENSE_SPQR_ROUND16_PROFILE
    dense_spqr_round16_prof::G().spqr_piece_emit_total_ns += dense_spqr_round16_prof::nsSince(__r16_spqr_piece_t0);
    dense_spqr_round16_prof::G().spqr_total_ns += dense_spqr_round16_prof::nsSince(__r16_spqr_total_t0);
#endif
    return P;
}

Solver::SparsePatch
Solver::splitBlockSPQR(int oldCore, int x) const {
#if COMBDENSE_GATE_ROUND8_PROFILE
    combdense_round8_prof::G().split_spqr_calls++;
#endif
#if SPARSE_HARDSCALING_ROUND5_PROFILE
    sparse_round5_prof::G().split_spqr_calls++;
    sparse_round5_prof::ScopeTimer __prof_split_spqr_scope(&sparse_round5_prof::G().split_spqr_total_ns);
#endif
    chk(0 <= oldCore && oldCore < (int)blocks.size(),
        "splitBlockSPQR: bad oldCore");
    chk(blocks[oldCore].alive,
        "splitBlockSPQR: dead oldCore");

    bool useTrue = shouldUseTrueSpqr(oldCore);
    DenseStructuredGateInfo gateInfo;
    bool escapeEligible = false;
#if DENSE_SPQR_ROUND16_PROFILE || DENSE_SPQR_ROUND16_OPT
    if (!useTrue) {
        escapeEligible = denseStructuredTrueSpqrEscapeEligible(oldCore, x, &gateInfo);
    }
#endif
#if DENSE_SPQR_ROUND16_PROFILE
    if (escapeEligible) ++dense_spqr_round16_prof::G().escape_eligible;
#endif

    bool useEscape = false;
#if DENSE_SHADOW_DIFF_ROUND20_PROFILE
    if (!useTrue && dense_shadow_diff_round20_prof::isDenseMode()) {
        const BlockCore& __B = blocks[oldCore];
        DenseStructuredGateInfo __info; denseStructuredTrueSpqrEscapeEligible(oldCore, x, &__info);
        bool __dense_guard = ((int)__B.realEdges.size() > (int)__B.allVertices.size() + 24);
        bool __boundary_guard = false;
        double __q_over_e = (__info.currentE > 0 ? (double)__info.badQueriesSize / (double)__info.currentE : 0.0);
        std::string __shape = dense_shadow_diff_round20_prof::bucketShape(__info.badQueriesSize, __info.attachCutsSize, __info.boundarySize, __info.xNbrCount, __info.boundaryZero, __info.ccOne);
        bool __prefilter = (__info.ccOne && __info.currentE >= 12000 && __info.badQueriesSize >= 4000 && __info.attachCutsSize <= 128 && __info.boundarySize <= 64);
        if (__prefilter) { ++dense_shadow_diff_round20_prof::G().shadow_prefilter_hit; ++dense_shadow_diff_round20_prof::G().shadow_prefilter_eligible; }
        auto __snap_local_before = dense_shadow_diff_round20_prof::snap();
        auto __local_t0 = dense_shadow_diff_round20_prof::Clock::now();
        SparsePatch __localP = splitBlockLocalRebuild(oldCore, x);
        long long __local_total_ns = dense_shadow_diff_round20_prof::nsSince(__local_t0);
        auto __snap_local_after = dense_shadow_diff_round20_prof::snap();
        long long __local_id_adj_ns = __snap_local_after.localIdAdjNs - __snap_local_before.localIdAdjNs;
        long long __step3_ns = __snap_local_after.step3Ns - __snap_local_before.step3Ns;
        long long __keep_ns = __snap_local_after.keepOrderNs - __snap_local_before.keepOrderNs;
        long long __apply_ns = __snap_local_after.applyNs - __snap_local_before.applyNs;
        bool __shadow_attempted=false, __shadow_match=false, __keep_choice_same=true, __piece_count_same=true, __piece_multiset_same=true;
        std::string __mismatch_reason;
        long long __spqr_total_ns=0, __spqr_build_ns=0, __spqr_keep_ns=0, __spqr_piece_ns=0;
        if (__prefilter) {
            static thread_local unsigned long long __r20_prefilter_counter = 0;
            ++__r20_prefilter_counter;
            bool __doShadow = (__r20_prefilter_counter <= 4ULL) || ((__r20_prefilter_counter % 16ULL) == 1ULL);
            if (__doShadow) {
                __shadow_attempted = true;
                ++dense_shadow_diff_round20_prof::G().shadow_attempted;
                auto __snap_spqr_before = dense_shadow_diff_round20_prof::snap();
                auto __spqr_t0 = dense_shadow_diff_round20_prof::Clock::now();
                SparsePatch __spqrP = splitBlockSPQRForced(oldCore, x);
                __spqr_total_ns = dense_shadow_diff_round20_prof::nsSince(__spqr_t0);
                auto __snap_spqr_after = dense_shadow_diff_round20_prof::snap();
                __spqr_build_ns = __snap_spqr_after.spqrBuildNs - __snap_spqr_before.spqrBuildNs;
                __spqr_keep_ns  = __snap_spqr_after.spqrKeepNs - __snap_spqr_before.spqrKeepNs;
                __spqr_piece_ns = __snap_spqr_after.spqrPieceNs - __snap_spqr_before.spqrPieceNs;
#ifdef DEBUG_SOLVER
                auto __A = canonSparseSemantic(oldCore, __localP);
                auto __Bsem = canonSparseSemantic(oldCore, __spqrP);
                auto __piecesA = dense_shadow_diff_round20_prof::smallPiecesOnly(__A);
                auto __piecesB = dense_shadow_diff_round20_prof::smallPiecesOnly(__Bsem);
                __keep_choice_same = (__A.keepEdges == __Bsem.keepEdges);
                __piece_count_same = (__piecesA.size() == __piecesB.size());
                __piece_multiset_same = (__piecesA == __piecesB);
                __shadow_match = (__A == __Bsem);
                if (__shadow_match) {
                    ++dense_shadow_diff_round20_prof::G().shadow_match;
                } else {
                    ++dense_shadow_diff_round20_prof::G().shadow_mismatch;
                    if (!__keep_choice_same) { ++dense_shadow_diff_round20_prof::G().mismatch_keep_choice; __mismatch_reason = "keep_choice_mismatch"; }
                    else if (!__piece_count_same) { ++dense_shadow_diff_round20_prof::G().mismatch_piece_count; __mismatch_reason = "piece_count_mismatch"; }
                    else if (!__piece_multiset_same) { ++dense_shadow_diff_round20_prof::G().mismatch_piece_multiset; __mismatch_reason = "piece_multiset_mismatch"; }
                    else { ++dense_shadow_diff_round20_prof::G().mismatch_invariant_unsure; __mismatch_reason = "invariant_unsure"; }
                    dense_shadow_diff_round20_prof::writeMismatchExample((int)dense_shadow_diff_round20_prof::G().rows + 1, __mismatch_reason, __shape, __info.currentV, __info.currentE, __info.badQueriesSize, __info.attachCutsSize, __info.boundarySize, dense_shadow_diff_round20_prof::hashVec(__A.keepEdges), dense_shadow_diff_round20_prof::hashVec(__Bsem.keepEdges), (int)__piecesA.size(), (int)__piecesB.size(), dense_shadow_diff_round20_prof::hashVV(__piecesA), dense_shadow_diff_round20_prof::hashVV(__piecesB));
                }
#else
                __mismatch_reason = "debug_off";
#endif
            }
        }
        ++dense_shadow_diff_round20_prof::G().rows;
        dense_shadow_diff_round20_prof::appendRow({
            dense_shadow_diff_round20_prof::caseMode(), std::to_string(dense_shadow_diff_round20_prof::caseN()), std::to_string(dense_shadow_diff_round20_prof::caseSeed()), std::to_string(dense_shadow_diff_round20_prof::G().rows),
            std::to_string(__info.currentV), std::to_string(__info.currentE), std::to_string(__info.badQueriesSize), [&](){ std::ostringstream os; os<<std::fixed<<std::setprecision(6)<<__q_over_e; return os.str(); }(),
            std::to_string(__info.attachCutsSize), std::to_string(__info.boundarySize), std::to_string(__info.oldBoundarySize), std::to_string(__info.boundaryZero?1:0), std::to_string(__info.ccOne?1:0), std::to_string(__info.currentOnComb?1:0),
            std::to_string(__dense_guard?1:0), std::to_string(__boundary_guard?1:0), std::to_string((((int)__B.realEdges.size()>96)?1:0)), std::to_string((((int)__B.badQueries.size()>24)?1:0)), __shape,
            [&](){ std::ostringstream os; os<<std::fixed<<std::setprecision(3)<<dense_shadow_diff_round20_prof::ms(__local_total_ns); return os.str(); }(),
            [&](){ std::ostringstream os; os<<std::fixed<<std::setprecision(3)<<dense_shadow_diff_round20_prof::ms(__local_id_adj_ns); return os.str(); }(),
            [&](){ std::ostringstream os; os<<std::fixed<<std::setprecision(3)<<dense_shadow_diff_round20_prof::ms(__step3_ns); return os.str(); }(),
            [&](){ std::ostringstream os; os<<std::fixed<<std::setprecision(3)<<dense_shadow_diff_round20_prof::ms(__keep_ns); return os.str(); }(),
            [&](){ std::ostringstream os; os<<std::fixed<<std::setprecision(3)<<dense_shadow_diff_round20_prof::ms(__apply_ns); return os.str(); }(),
            std::to_string(__prefilter?1:0), std::to_string(__shadow_attempted?1:0), std::to_string(__shadow_match?1:0), __mismatch_reason,
            std::to_string(__keep_choice_same?1:0), std::to_string(__piece_count_same?1:0), std::to_string(__piece_multiset_same?1:0),
            [&](){ std::ostringstream os; os<<std::fixed<<std::setprecision(3)<<dense_shadow_diff_round20_prof::ms(__spqr_total_ns); return os.str(); }(),
            [&](){ std::ostringstream os; os<<std::fixed<<std::setprecision(3)<<dense_shadow_diff_round20_prof::ms(__spqr_build_ns); return os.str(); }(),
            [&](){ std::ostringstream os; os<<std::fixed<<std::setprecision(3)<<dense_shadow_diff_round20_prof::ms(__spqr_keep_ns); return os.str(); }(),
            [&](){ std::ostringstream os; os<<std::fixed<<std::setprecision(3)<<dense_shadow_diff_round20_prof::ms(__spqr_piece_ns); return os.str(); }(),
            std::to_string(0)
        });
        return __localP;
    }
#endif
#if DENSE_SPQR_ROUND16_OPT
    if (!useTrue) {
        if (escapeEligible) {
#if DENSE_SPQR_ROUND16_PROFILE
            ++dense_spqr_round16_prof::G().escape_attempted;
#endif
            bool shadowOk = true;
#if DENSE_SPQR_ROUND16_SHADOWCHECK
            static thread_local unsigned long long __r16_shadow_counter = 0;
            ++__r16_shadow_counter;
            bool doShadow = ((__r16_shadow_counter % 64ULL) == 1ULL);
            if (doShadow) {
#if DENSE_SPQR_ROUND16_PROFILE
                ++dense_spqr_round16_prof::G().sampled_shadow_runs;
#endif
                SparsePatch localP = splitBlockLocalRebuild(oldCore, x);
                SparsePatch spqrP = splitBlockSPQRForced(oldCore, x);
                auto A = canonSparseSemantic(oldCore, localP);
                auto B = canonSparseSemantic(oldCore, spqrP);
                if (!(A == B)) {
                    shadowOk = false;
#if DENSE_SPQR_ROUND16_PROFILE
                    ++dense_spqr_round16_prof::G().shadow_keep_choice_mismatch;
#endif
                }
            }
#endif
            if (shadowOk) {
                useEscape = true;
#if DENSE_SPQR_ROUND16_PROFILE
                ++dense_spqr_round16_prof::G().escape_success;
#endif
            } else {
#if DENSE_SPQR_ROUND16_PROFILE
                ++dense_spqr_round16_prof::G().escape_fallback;
                ++dense_spqr_round16_prof::G().fallback_shadow_mismatch;
#endif
            }
        } else {
#if DENSE_SPQR_ROUND16_PROFILE
            ++dense_spqr_round16_prof::G().escape_fallback;
            ++dense_spqr_round16_prof::G().fallback_gate_not_met;
#endif
        }
    }
#endif

    if (!useTrue && !useEscape) {
#if COMBDENSE_GATE_ROUND8_PROFILE
        combdense_round8_prof::G().localrebuild_fallback_calls++;
#endif
        return splitBlockLocalRebuild(oldCore, x);
    }

    return splitBlockSPQRForced(oldCore, x);
}


#ifdef DEBUG_SOLVER

void Solver::dumpCanonSparseSemantic(const char* tag,
                                     const CanonSparseSemantic& C) const {
    auto printVV = [&](const vector<vector<int>>& vv) {
        cerr << "[";
        for (int i = 0; i < (int)vv.size(); ++i) {
            if (i) cerr << ",";
            cerr << "[";
            for (int j = 0; j < (int)vv[i].size(); ++j) {
                if (j) cerr << ",";
                cerr << vv[i][j];
            }
            cerr << "]";
        }
        cerr << "]";
    };

    cerr << "[" << tag << "]\n";
    cerr << "pieceEdges=";
    printVV(C.pieceEdges);
    cerr << "\n";

    cerr << "boundarySig:\n";
    for (const auto& [v, info] : C.boundarySig) {
        cerr << "  v=" << v
             << " existedOldCut=" << info.first
             << " inc=";
        printVV(info.second);
        cerr << "\n";
    }

    cerr << "isolatedExclusive=[";
    for (int i = 0; i < (int)C.isolatedExclusive.size(); ++i) {
        if (i) cerr << ",";
        cerr << C.isolatedExclusive[i];
    }
    cerr << "]\n";
}

Solver::CanonSparseSemantic
Solver::canonSparseSemantic(int oldCore, const SparsePatch& P) const {
    chk(0 <= oldCore && oldCore < (int)blocks.size(),
        "canonSparseSemantic: bad oldCore");
    chk(P.oldCoreId == oldCore,
        "canonSparseSemantic: patch oldCore mismatch");
    chk(blocks[oldCore].alive,
        "canonSparseSemantic: oldCore must still be alive when canonicalizing");

    const BlockCore& B = blocks[oldCore];

    auto pieceEdgeSetTouchesVertex = [&](const vector<int>& E, int v) -> bool {
        for (int e : E) {
            if (edges[e].u == v || edges[e].v == v) return true;
        }
        return false;
    };

    auto sortedSurvivingEdgesFromPatch =
        [&](const unordered_set<int>& deadE,
            const unordered_set<int>& smallE) -> vector<int> {
            vector<int> surv;
            for (int e : B.realEdges) {
                if (deadE.count(e)) continue;
                if (smallE.count(e)) continue;
                surv.push_back(e);
            }
            sort(surv.begin(), surv.end());
            return surv;
        };

    CanonSparseSemantic C;

    vector<int> deadEdges = normVec(P.deadEdges);
    unordered_set<int> deadE(deadEdges.begin(), deadEdges.end());

    vector<vector<int>> smallEdgeSets(P.small.size());
    unordered_set<int> smallE;
    unordered_set<int> seenPieceEdge;

    for (int sid = 0; sid < (int)P.small.size(); ++sid) {
        auto E = normVec(P.small[sid].edges);
        chk(!E.empty(),
            "canonSparseSemantic: small piece with empty edge set");

        for (int e : E) {
            chk(!deadE.count(e),
                "canonSparseSemantic: small piece contains dead edge");
            chk(edgeOwnerCore[e] == oldCore,
                "canonSparseSemantic: small piece edge not owned by oldCore");
            chk(seenPieceEdge.insert(e).second,
                "canonSparseSemantic: edge duplicated across small pieces");
            smallE.insert(e);
        }

        smallEdgeSets[sid] = move(E);
    }

    vector<int> keepEdges;
    {
        unordered_set<int> aliveOldE(B.realEdges.begin(), B.realEdges.end());
        for (int e : deadEdges) {
            chk(aliveOldE.count(e),
                "canonSparseSemantic: dead edge not in oldCore");
        }

        keepEdges = sortedSurvivingEdgesFromPatch(deadE, smallE);
    }

    vector<int> survivingOldEdges;
    for (int e : B.realEdges) {
        if (!deadE.count(e)) survivingOldEdges.push_back(e);
    }
    sort(survivingOldEdges.begin(), survivingOldEdges.end());

    if (survivingOldEdges.empty()) {
        chk(!P.keepExists,
            "canonSparseSemantic: no surviving edges but keepExists=true");
        chk(keepEdges.empty(),
            "canonSparseSemantic: no surviving edges but implicit keep nonempty");
    } else {
        chk(P.keepExists,
            "canonSparseSemantic: surviving edges exist but keepExists=false");
        chk(!keepEdges.empty(),
            "canonSparseSemantic: keepExists=true but implicit keep empty");
    }

    C.keepEdges = keepEdges;

    vector<int> unionEdges;
    if (!keepEdges.empty()) {
        for (int e : keepEdges) {
            chk(seenPieceEdge.insert(e).second,
                "canonSparseSemantic: edge overlaps keep and small");
            unionEdges.push_back(e);
        }
        C.pieceEdges.push_back(keepEdges);
    }

    for (const auto& E : smallEdgeSets) {
        for (int e : E) unionEdges.push_back(e);
        C.pieceEdges.push_back(E);
    }

    sort(unionEdges.begin(), unionEdges.end());
    chk(unionEdges == survivingOldEdges,
        "canonSparseSemantic: piece partition does not match surviving edge set");

    sort(C.pieceEdges.begin(), C.pieceEdges.end());

    unordered_set<int> boundaryVertsSeen;

    for (const auto& bdRaw : P.boundary) {
        int v = bdRaw.vertex;
        chk(boundaryVertsSeen.insert(v).second,
            "canonSparseSemantic: duplicated boundary vertex");

        vector<int> smallIds = normVec(bdRaw.smallIds);
        vector<vector<int>> inc;

        if (bdRaw.touchesKeep) {
            chk(P.keepExists,
                "canonSparseSemantic: boundary touchesKeep but keep does not exist");
            chk(!keepEdges.empty(),
                "canonSparseSemantic: touchesKeep but implicit keep is empty");
            chk(pieceEdgeSetTouchesVertex(keepEdges, v),
                "canonSparseSemantic: keep incidence does not touch boundary vertex");
            inc.push_back(keepEdges);
        }

        for (int sid : smallIds) {
            chk(0 <= sid && sid < (int)smallEdgeSets.size(),
                "canonSparseSemantic: boundary smallId out of range");
            chk(pieceEdgeSetTouchesVertex(smallEdgeSets[sid], v),
                "canonSparseSemantic: small incidence does not touch boundary vertex");
            inc.push_back(smallEdgeSets[sid]);
        }

        sort(inc.begin(), inc.end());

        if (!bdRaw.existedOldCut) {
            chk((int)inc.size() >= 2,
                "canonSparseSemantic: new boundary has incidence < 2");
        }

        auto ins = C.boundarySig.emplace(v, make_pair(bdRaw.existedOldCut, inc));
        chk(ins.second,
            "canonSparseSemantic: boundarySig duplicate insert");
    }

    C.isolatedExclusive = normVec(P.isolatedExclusive);
    for (int v : C.isolatedExclusive) {
        chk(!C.boundarySig.count(v),
            "canonSparseSemantic: isolatedExclusive overlaps boundary");
    }

    chk(P.deadHandles.empty(),
        "canonSparseSemantic: deadHandles must be empty in current delete flow");

    vector<int> expectDeadExclusive;
    expectDeadExclusive.push_back(P.deletedVertex);
    for (int v : C.isolatedExclusive) {
        if (v != P.deletedVertex) expectDeadExclusive.push_back(v);
    }
    expectDeadExclusive = normVec(move(expectDeadExclusive));

    chk(normVec(P.deadExclusiveVertices) == expectDeadExclusive,
        "canonSparseSemantic: deadExclusiveVertices contract mismatch");

    return C;
}

void Solver::checkRawSpqrBuildAgainstCore(int core,
                                          const RawSpqrBuild& raw) const {
    chk(0 <= core && core < (int)blocks.size(),
        "checkRawSpqrBuildAgainstCore: bad core");
    chk(blocks[core].alive,
        "checkRawSpqrBuildAgainstCore: dead core");

    const BlockCore& B = blocks[core];

    chk(!raw.node.empty(),
        "raw SPQR: empty node list");

    unordered_set<int> aliveEdgeSet(B.realEdges.begin(), B.realEdges.end());
    unordered_set<int> seenReal;
    int virtualHalfEdges = 0;

    for (int u = 0; u < (int)raw.node.size(); ++u) {
        chk(!raw.node[u].skel.empty(),
            "raw SPQR: node with empty skeleton");

        for (int s = 0; s < (int)raw.node[u].skel.size(); ++s) {
            const auto& e = raw.node[u].skel[s];

            chk(e.a != e.b,
                "raw SPQR: self-loop skeleton edge");

            if (e.realEdgeId >= 0) {
                chk(aliveEdgeSet.count(e.realEdgeId),
                    "raw SPQR: returned real edge not in oldCore");
                chk(!seenReal.count(e.realEdgeId),
                    "raw SPQR: duplicated real edge");
                seenReal.insert(e.realEdgeId);
            } else {
                virtualHalfEdges++;

                chk(0 <= e.peerNode && e.peerNode < (int)raw.node.size(),
                    "raw SPQR: bad peerNode");
                chk(0 <= e.peerSlot && e.peerSlot < (int)raw.node[e.peerNode].skel.size(),
                    "raw SPQR: bad peerSlot");
            }
        }
    }

    chk(seenReal.size() == aliveEdgeSet.size(),
        "raw SPQR: real edge coverage mismatch");
    chk(virtualHalfEdges % 2 == 0,
        "raw SPQR: odd number of virtual half-edges");

    vector<vector<char>> used(raw.node.size());
    for (int u = 0; u < (int)raw.node.size(); ++u) {
        used[u].assign(raw.node[u].skel.size(), 0);
    }

    vector<vector<int>> treeAdj(raw.node.size());
    int treeEdges = 0;

    auto sameUnorderedPair = [&](int a1, int b1, int a2, int b2) {
        return (a1 == a2 && b1 == b2) || (a1 == b2 && b1 == a2);
    };

    for (int u = 0; u < (int)raw.node.size(); ++u) {
        for (int s = 0; s < (int)raw.node[u].skel.size(); ++s) {
            const auto& e = raw.node[u].skel[s];
            if (e.realEdgeId != -1) continue;
            if (used[u][s]) continue;

            int v = e.peerNode;
            int t = e.peerSlot;
            const auto& f = raw.node[v].skel[t];

            chk(f.realEdgeId == -1,
                "raw SPQR: peer of virtual edge is real");
            chk(f.peerNode == u && f.peerSlot == s,
                "raw SPQR: peer symmetry broken");
            chk(sameUnorderedPair(e.a, e.b, f.a, f.b),
                "raw SPQR: virtual terminal pair mismatch");

            used[u][s] = 1;
            used[v][t] = 1;

            treeAdj[u].push_back(v);
            treeAdj[v].push_back(u);
            treeEdges++;
        }
    }

    chk(treeEdges == (int)raw.node.size() - 1,
        "raw SPQR: stitched graph is not a tree by edge count");

    vector<char> vis(raw.node.size(), 0);
    queue<int> q;
    q.push(0);
    vis[0] = 1;

    int seenNode = 0;
    while (!q.empty()) {
        int u = q.front();
        q.pop();
        seenNode++;

        for (int v : treeAdj[u]) {
            if (!vis[v]) {
                vis[v] = 1;
                q.push(v);
            }
        }
    }

    chk(seenNode == (int)raw.node.size(),
        "raw SPQR: stitched graph disconnected");

    unordered_map<int, vector<int>> occ;

    for (int u = 0; u < (int)raw.node.size(); ++u) {
        unordered_set<int> localV;
        for (const auto& e : raw.node[u].skel) {
            localV.insert(e.a);
            localV.insert(e.b);
        }
        for (int v : localV) {
            occ[v].push_back(u);
        }
    }

    for (auto& [v, vec] : occ) {
        vec = normVec(move(vec));
        if ((int)vec.size() <= 1) continue;

        unordered_set<int> target(vec.begin(), vec.end());
        queue<int> qq;
        unordered_set<int> seen;

        qq.push(vec[0]);
        seen.insert(vec[0]);

        while (!qq.empty()) {
            int u = qq.front();
            qq.pop();

            for (int w : treeAdj[u]) {
                if (!target.count(w)) continue;
                if (seen.insert(w).second) qq.push(w);
            }
        }

        chk((int)seen.size() == (int)vec.size(),
            "raw SPQR: vertex occurrence set not connected");
    }
}

void Solver::checkSpqrPatchSemanticEqLocal(int oldCore, int x) const {
    chk(0 <= oldCore && oldCore < (int)blocks.size(),
        "checkSpqrPatchSemanticEqLocal: bad oldCore");
    chk(blocks[oldCore].alive,
        "checkSpqrPatchSemanticEqLocal: dead oldCore");

    SparsePatch localP = splitBlockLocalRebuild(oldCore, x);
    SparsePatch spqrP  = splitBlockSPQR(oldCore, x);

    CanonSparseSemantic A = canonSparseSemantic(oldCore, localP);
    CanonSparseSemantic B = canonSparseSemantic(oldCore, spqrP);

    if (!(A == B)) {
        cerr << "[SPQR_DIFF] oldCore=" << oldCore << " x=" << x << "\n";
        dumpCanonSparseSemantic("LOCAL", A);
        dumpCanonSparseSemantic("SPQR",  B);
        failCheck("checkSpqrPatchSemanticEqLocal: SPQR patch semantic mismatch vs LOCAL_REBUILD");
    }
}

void Solver::checkSpqrEqLocalForVertex(int x) const {
    chk(orig[x].alive,
        "checkSpqrEqLocalForVertex: x must be alive before delete");

    int anchor = orig[x].anchorBC;

    if (anchor != -1 &&
        0 <= anchor && anchor < (int)bcNodes.size() &&
        bcNodes[anchor].alive &&
        bcNodes[anchor].type == BCN_TRIVIAL) {
        return;
    }

    vector<int> cores;

    if (orig[x].cutBC != -1) {
        int cutX = orig[x].cutBC;

        for (int nb : bcNodes[cutX].adj) {
            if (!bcNodes[nb].alive) continue;
            chk(bcNodes[nb].type == BCN_BLOCK,
                "checkSpqrEqLocalForVertex: CUT adjacent to non-BLOCK");
            cores.push_back(bcNodes[nb].coreId);
        }
    } else {
        int oldCore = orig[x].ownerBlock;
        chk(oldCore != -1,
            "checkSpqrEqLocalForVertex: non-cut vertex without ownerBlock");
        cores.push_back(oldCore);
    }

    cores = normVec(move(cores));
    for (int c : cores) {
        checkSpqrPatchSemanticEqLocal(c, x);
    }
}

#endif


void Solver::clearAll() {
    // Keep N/M/inputQueries/rawSpqrMode; clear dynamic state only.
    queries.clear();
    ownerQueries.clear();
    indeg.clear();
    badCount.clear();
    parentAns.clear();
    compUp.clear();
    bcRootId.clear();
    rootMembers.clear();
    relabelSeen.clear();
    relabelTag = 1;
    while (!ready.empty()) ready.pop();

    edges.clear();
    handles.clear();
    orig.clear();
    bcNodes.clear();
    blocks.clear();
    edgeOwnerCore.clear();
    edgePosInCore.clear();
    handleOwnerCore.clear();
    blockSpqr.clear();
}

int Solver::allocBCNode(BCNodeType type, int origVertex, int coreId) {
    BCNode n;
    n.type = type;
    n.alive = true;
    n.origVertex = origVertex;
    n.coreId = coreId;
    bcNodes.push_back(std::move(n));
    bcRootId.push_back(-1);
    rootMembers.emplace_back();
    relabelSeen.push_back(0);
    return (int)bcNodes.size() - 1;
}

int Solver::allocBlockCore() {
    BlockCore B;
    B.alive = true;
    B.bcNode = -1;
    blocks.push_back(std::move(B));
    return (int)blocks.size() - 1;
}

int Solver::makeTrivialNode(int v) {
    int id = allocBCNode(BCN_TRIVIAL, v, -1);
    return id;
}

int Solver::makeCutNode(int v) {
    int id = allocBCNode(BCN_CUT, v, -1);
    return id;
}

void Solver::eraseOnce(std::vector<int>& vec, int x) {
    auto it = std::find(vec.begin(), vec.end(), x);
    if (it != vec.end()) vec.erase(it);
}

void Solver::linkBC(int a, int b) {
    chk(0 <= a && a < (int)bcNodes.size(), "linkBC: bad a");
    chk(0 <= b && b < (int)bcNodes.size(), "linkBC: bad b");
    if (!bcNodes[a].alive || !bcNodes[b].alive) return;
    if (std::find(bcNodes[a].adj.begin(), bcNodes[a].adj.end(), b) == bcNodes[a].adj.end()) {
        bcNodes[a].adj.push_back(b);
    }
    if (std::find(bcNodes[b].adj.begin(), bcNodes[b].adj.end(), a) == bcNodes[b].adj.end()) {
        bcNodes[b].adj.push_back(a);
    }
}

void Solver::cutBCEdge(int a, int b) {
    if (0 <= a && a < (int)bcNodes.size()) eraseOnce(bcNodes[a].adj, b);
    if (0 <= b && b < (int)bcNodes.size()) eraseOnce(bcNodes[b].adj, a);
}

void Solver::addVertexToCore(int core, int v) {
    auto& V = blocks[core].allVertices;
    if (std::find(V.begin(), V.end(), v) == V.end()) V.push_back(v);
}

void Solver::removeVertexFromCore(int core, int v) {
    eraseOnce(blocks[core].allVertices, v);
}

void Solver::addEdgeToCore(int core, int e) {
    auto& E = blocks[core].realEdges;
    if (0 <= e && e < (int)edgeOwnerCore.size() && edgeOwnerCore[e] == core) return;
    int pos = (int)E.size();
    E.push_back(e);
    if (0 <= e && e < (int)edgeOwnerCore.size()) edgeOwnerCore[e] = core;
    if (0 <= e && e < (int)edgePosInCore.size()) edgePosInCore[e] = pos;
}

void Solver::removeEdgeFromCore(int core, int e) {
    if (!(0 <= e && e < (int)edgeOwnerCore.size()) || edgeOwnerCore[e] != core) return;
    auto& E = blocks[core].realEdges;
    int pos = edgePosInCore[e];
    int last = E.back();
    E[pos] = last;
    edgePosInCore[last] = pos;
    E.pop_back();
    edgeOwnerCore[e] = -1;
    edgePosInCore[e] = -1;
}

void Solver::addAttachCut(int core, int cutId) {
    auto& C = blocks[core].attachCuts;
    if (std::find(C.begin(), C.end(), cutId) == C.end()) C.push_back(cutId);
}

void Solver::removeAttachCut(int core, int cutId) {
    eraseOnce(blocks[core].attachCuts, cutId);
}

int Solver::findRootBC(int start) const {
    chk(0 <= start && start < (int)bcRootId.size(), "findRootBC: bad start");
    int r = bcRootId[start];
    chk(r != -1, "findRootBC: root not assigned");
    return r;
}

void Solver::assignComponentRootsInitial() {
    bcRootId.assign(bcNodes.size(), -1);
    rootMembers.assign(bcNodes.size(), {});
    if ((int)relabelSeen.size() < (int)bcNodes.size()) relabelSeen.resize(bcNodes.size(), 0);
    compUp.clear();

    int root1 = -1;
    if (1 < (int)orig.size() && orig[1].alive && orig[1].anchorBC != -1) {
        root1 = orig[1].anchorBC;
    }

    std::vector<char> vis(bcNodes.size(), 0);
    for (int s = 0; s < (int)bcNodes.size(); ++s) {
        if (!bcNodes[s].alive || vis[s]) continue;

        std::queue<int> q;
        std::vector<int> members;
        q.push(s);
        vis[s] = 1;
        int root = s;
        bool hasRoot1 = (s == root1);

        while (!q.empty()) {
            int u = q.front(); q.pop();
            members.push_back(u);
            if (u == root1) hasRoot1 = true;
            for (int v : bcNodes[u].adj) {
                if (!bcNodes[v].alive || vis[v]) continue;
                vis[v] = 1;
                q.push(v);
            }
        }

        for (int u : members) bcRootId[u] = root;
        rootMembers[root] = members;
        compUp[root] = hasRoot1 ? 0 : 1;
    }
}

void Solver::refreshRootsFromSeeds(int oldRoot, int parentLabel, const std::vector<int>& seeds) {
    compUp.erase(oldRoot);
    if ((int)bcRootId.size() < (int)bcNodes.size()) bcRootId.resize(bcNodes.size(), -1);
    if ((int)rootMembers.size() < (int)bcNodes.size()) rootMembers.resize(bcNodes.size());
    if ((int)relabelSeen.size() < (int)bcNodes.size()) relabelSeen.resize(bcNodes.size(), 0);
    if (++relabelTag == INT_MAX) {
        std::fill(relabelSeen.begin(), relabelSeen.end(), 0);
        relabelTag = 1;
    }
    int tag = relabelTag;

    struct Part {
        std::vector<int> members;
        int rep = -1;
    };
    std::vector<Part> parts;

    auto bfsFrom = [&](int s) {
        std::queue<int> q;
        Part part;
        q.push(s);
        relabelSeen[s] = tag;
        while (!q.empty()) {
            int u = q.front(); q.pop();
            part.members.push_back(u);
            if (part.rep == -1 || u < part.rep) part.rep = u;
            for (int v : bcNodes[u].adj) {
                if (!bcNodes[v].alive) continue;
                if (relabelSeen[v] == tag) continue;
                if (!(bcRootId[v] == oldRoot || bcRootId[v] == -1)) continue;
                relabelSeen[v] = tag;
                q.push(v);
            }
        }
        parts.push_back(std::move(part));
    };

    // First, walk components reachable from provided seeds. This captures all newly created nodes (bcRootId == -1).
    for (int s : seeds) {
        if (!(0 <= s && s < (int)bcNodes.size())) continue;
        if (!bcNodes[s].alive || relabelSeen[s] == tag) continue;
        if (!(bcRootId[s] == oldRoot || bcRootId[s] == -1)) continue;
        bfsFrom(s);
    }

    // Then cover any leftover alive old members that were not reached from seeds.
    std::vector<int> oldMembers;
    if (0 <= oldRoot && oldRoot < (int)rootMembers.size()) oldMembers = rootMembers[oldRoot];
    for (int s : oldMembers) {
        if (!(0 <= s && s < (int)bcNodes.size())) continue;
        if (!bcNodes[s].alive || relabelSeen[s] == tag) continue;
        if (bcRootId[s] != oldRoot) continue;
        bfsFrom(s);
    }

    rootMembers[oldRoot].clear();
    if (parts.empty()) return;

    int keep = -1;
    if (0 <= oldRoot && oldRoot < (int)bcNodes.size() && bcNodes[oldRoot].alive) {
        for (int i = 0; i < (int)parts.size(); ++i) {
            for (int u : parts[i].members) if (u == oldRoot) { keep = i; break; }
            if (keep != -1) break;
        }
    }
    if (keep == -1) {
        keep = 0;
        for (int i = 1; i < (int)parts.size(); ++i) {
            if (parts[i].members.size() > parts[keep].members.size()) keep = i;
        }
    }

    // Keep oldRoot label on one chosen piece; relabel only the remaining pieces.
    for (int u : parts[keep].members) bcRootId[u] = oldRoot;
    rootMembers[oldRoot] = parts[keep].members;
    compUp[oldRoot] = parentLabel;

    for (int i = 0; i < (int)parts.size(); ++i) {
        if (i == keep) continue;
        int rep = -1;
        for (int u : parts[i].members) {
            if (u != oldRoot && (rep == -1 || u < rep)) rep = u;
        }
        chk(rep != -1, "refreshRootsFromSeeds: no representative for relabeled part");
        for (int u : parts[i].members) bcRootId[u] = rep;
        rootMembers[rep] = parts[i].members;
        compUp[rep] = parentLabel;
    }
}

std::vector<int> Solver::collectDistinctRoots(const std::vector<int>& seeds) {
    std::unordered_set<int> seen;
    std::vector<int> roots;
    for (int s : seeds) {
        if (!(0 <= s && s < (int)bcNodes.size())) continue;
        if (!bcNodes[s].alive) continue;
        int r = findRootBC(s);
        if (seen.insert(r).second) roots.push_back(r);
    }
    std::sort(roots.begin(), roots.end());
    return roots;
}

void Solver::pushReadyIf(int v) {
    if (1 <= v && v <= N && !orig.empty() && orig[v].alive && indeg[v] == 0 && badCount[v] == 0) {
        ready.push(v);
    }
}

int Solver::popReady() {
    while (!ready.empty()) {
        int x = ready.top(); ready.pop();
        if (1 <= x && x <= N && orig[x].alive && indeg[x] == 0 && badCount[x] == 0) return x;
    }
    failCheck("popReady: no ready vertex");
}

void Solver::normalizeCutNode(int cutId, std::vector<int>& rootSeeds) {
    if (!(0 <= cutId && cutId < (int)bcNodes.size())) return;
    if (!bcNodes[cutId].alive || bcNodes[cutId].type != BCN_CUT) return;

    int v = bcNodes[cutId].origVertex;
    std::vector<int> nbr;
    for (int nb : bcNodes[cutId].adj) {
        if (bcNodes[nb].alive) nbr.push_back(nb);
    }

    if ((int)nbr.size() >= 2) {
        if (orig[v].alive) {
            orig[v].anchorBC = cutId;
            orig[v].cutBC = cutId;
            orig[v].ownerBlock = -1;
        }
        rootSeeds.push_back(cutId);
        return;
    }

    if ((int)nbr.size() == 1) {
        int only = nbr[0];
        if (bcNodes[only].type == BCN_BLOCK) {
            int core = bcNodes[only].coreId;
            cutBCEdge(cutId, only);
            removeAttachCut(core, cutId);
            bcNodes[cutId].alive = false;
            addVertexToCore(core, v);
            if (orig[v].alive) {
                orig[v].anchorBC = only;
                orig[v].cutBC = -1;
                orig[v].ownerBlock = core;
            }
            rootSeeds.push_back(only);
            return;
        }
        if (bcNodes[only].type == BCN_TRIVIAL) {
            cutBCEdge(cutId, only);
            bcNodes[cutId].alive = false;
            if (orig[v].alive) {
                orig[v].anchorBC = only;
                orig[v].cutBC = -1;
                orig[v].ownerBlock = -1;
            }
            rootSeeds.push_back(only);
            return;
        }
        failCheck("normalizeCutNode: illegal single neighbor type");
    }

    bcNodes[cutId].alive = false;
    if (orig[v].alive) {
        int triv = makeTrivialNode(v);
        orig[v].anchorBC = triv;
        orig[v].cutBC = -1;
        orig[v].ownerBlock = -1;
        rootSeeds.push_back(triv);
    }
}

void Solver::applyPatchToCore(int oldCore, const SparsePatch& P, std::vector<int>& rootSeeds) {
    chk(0 <= oldCore && oldCore < (int)blocks.size(), "applyPatchToCore: bad core");
    chk(P.oldCoreId == oldCore, "applyPatchToCore: patch/core mismatch");
    chk(blocks[oldCore].alive, "applyPatchToCore: dead oldCore");

    int oldBlockBC = blocks[oldCore].bcNode;
    chk(0 <= oldBlockBC && oldBlockBC < (int)bcNodes.size(), "applyPatchToCore: missing old bc node");
#if DENSE_RECT_ROUND1_PROFILE
    dense_rect_round1_prof::G().apply_calls++;
#endif

    bool fastKeepOnlyApply = P.keepExists && P.small.empty() && P.isolatedExclusive.empty() && P.deadHandles.empty() && blocks[oldCore].badQueries.empty();
    if (fastKeepOnlyApply) {
        for (const auto& bd : P.boundary) {
            if (!bd.touchesKeep || !bd.smallIds.empty()) {
                fastKeepOnlyApply = false;
                break;
            }
        }
    }
    if (fastKeepOnlyApply) {
#if COMBDENSE_GATE_ROUND8_PROFILE
        ++combdense_round8_prof::G().fastkeep_calls;
        auto __prof_r8_fastkeep_total_t0 = combdense_round8_prof::Clock::now();
#endif
#if CHEAPFAN_ROUND6_PROFILE
        cheapfan_round6_prof::ScopeTimer __prof_fastkeep_scope(&cheapfan_round6_prof::G().fastkeep_total_ns);
#endif
        rootSeeds.push_back(oldBlockBC);
#if CHEAPFAN_ROUND6_PROFILE
        auto __prof_fast_dead_edge_t0 = cheapfan_round6_prof::Clock::now();
#endif
#if COMBDENSE_GATE_ROUND8_PROFILE
        auto __prof_r8_fast_dead_edge_t0 = combdense_round8_prof::Clock::now();
#endif
        for (int e : P.deadEdges) {
            if (0 <= e && e < (int)edgeOwnerCore.size() && edgeOwnerCore[e] == oldCore) {
                removeEdgeFromCore(oldCore, e);
            }
        }
#if CHEAPFAN_ROUND6_PROFILE
        cheapfan_round6_prof::G().fastkeep_dead_edge_ns += cheapfan_round6_prof::nsSince(__prof_fast_dead_edge_t0);
        auto __prof_fast_dead_vertex_t0 = cheapfan_round6_prof::Clock::now();
#endif
#if COMBDENSE_GATE_ROUND8_PROFILE
        combdense_round8_prof::G().fastkeep_dead_edge_ns += combdense_round8_prof::nsSince(__prof_r8_fast_dead_edge_t0);
        auto __prof_r8_fast_dead_vertex_t0 = combdense_round8_prof::Clock::now();
#endif
        for (int v : P.deadExclusiveVertices) {
            removeVertexFromCore(oldCore, v);
        }
#if CHEAPFAN_ROUND6_PROFILE
        cheapfan_round6_prof::G().fastkeep_dead_vertex_ns += cheapfan_round6_prof::nsSince(__prof_fast_dead_vertex_t0);
        auto __prof_fast_boundary_t0 = cheapfan_round6_prof::Clock::now();
#endif
#if COMBDENSE_GATE_ROUND8_PROFILE
        combdense_round8_prof::G().fastkeep_dead_vertex_ns += combdense_round8_prof::nsSince(__prof_r8_fast_dead_vertex_t0);
        auto __prof_r8_fast_boundary_t0 = combdense_round8_prof::Clock::now();
#endif
        for (const auto& bd : P.boundary) {
            int v = bd.vertex;
            if (!bd.existedOldCut && orig[v].alive) {
                addVertexToCore(oldCore, v);
                orig[v].anchorBC = oldBlockBC;
                orig[v].cutBC = -1;
                orig[v].ownerBlock = oldCore;
            }
        }
#if CHEAPFAN_ROUND6_PROFILE
        cheapfan_round6_prof::G().fastkeep_boundary_loop_ns += cheapfan_round6_prof::nsSince(__prof_fast_boundary_t0);
        auto __prof_fast_bump_t0 = cheapfan_round6_prof::Clock::now();
#endif
#if COMBDENSE_GATE_ROUND8_PROFILE
        combdense_round8_prof::G().fastkeep_boundary_ns += combdense_round8_prof::nsSince(__prof_r8_fast_boundary_t0);
        auto __prof_r8_fast_bump_t0 = combdense_round8_prof::Clock::now();
#endif
        statecert_fastkey::bumpEpoch(oldCore);
#if CHEAPFAN_ROUND6_PROFILE
        cheapfan_round6_prof::G().fastkeep_state_bump_ns += cheapfan_round6_prof::nsSince(__prof_fast_bump_t0);
#endif
#if COMBDENSE_GATE_ROUND8_PROFILE
        combdense_round8_prof::G().fastkeep_state_bump_ns += combdense_round8_prof::nsSince(__prof_r8_fast_bump_t0);
        combdense_round8_prof::G().fastkeep_total_ns += combdense_round8_prof::nsSince(__prof_r8_fastkeep_total_t0);
#endif
        if (cheapfan_cert_round7::hasPendingForCore(oldCore)) cheapfan_cert_round7::commitPending(oldCore, statecert_fastkey::getEpoch(oldCore));
        else cheapfan_cert_round7::invalidateCore(oldCore);
#if DENSE_BCCREUSE_ROUND12_PROFILE
        {
            auto& __r12_cache = dense_bccreuse_round12_prof::Cache();
            auto& __r12_pending = dense_bccreuse_round12_prof::Pending();
            __r12_cache = dense_bccreuse_round12_prof::CacheState{};
            if (P.keepExists) {
                __r12_cache.valid = true;
                __r12_cache.core = oldCore;
                __r12_cache.fromStep3 = (__r12_pending.active && __r12_pending.oldCore == oldCore && __r12_pending.usedStep3);
                __r12_cache.keepOnly = true;
                __r12_cache.boundaryUnchanged = true;
                __r12_cache.prevBccCount = (__r12_cache.fromStep3 ? 1 : 0);
                __r12_cache.prevKeepEdgeCount = (int)blocks[oldCore].realEdges.size();
            }
            __r12_pending = dense_bccreuse_round12_prof::PendingStep3Info{};
        }
#endif
#if DENSE_SINGLEBCC_ROUND13_PROFILE
        {
            auto& __r13_cache = dense_singlebcc_round13_prof::Cache();
            auto& __r13_pending = dense_singlebcc_round13_prof::Pending();
            __r13_cache = dense_singlebcc_round13_prof::CacheState{};
            bool __r13_boundary_unchanged = P.keepExists && P.small.empty() && P.isolatedExclusive.empty() && P.deadHandles.empty() && ((int)P.boundary.size() == (int)blocks[oldCore].attachCuts.size());
            if (__r13_boundary_unchanged) {
                for (const auto& __bd : P.boundary) {
                    if (!(__bd.existedOldCut && __bd.touchesKeep && __bd.smallIds.empty())) { __r13_boundary_unchanged = false; break; }
                }
            }
            if (P.keepExists) {
                __r13_cache.valid = true;
                __r13_cache.core = oldCore;
                __r13_cache.prevStep3 = (__r13_pending.active && __r13_pending.oldCore == oldCore && __r13_pending.usedStep3);
                __r13_cache.prevSingleBcc = (__r13_pending.active && __r13_pending.oldCore == oldCore && __r13_pending.singleBcc);
                __r13_cache.prevKeepOnlyChain = (P.small.empty() && P.isolatedExclusive.empty() && P.deadHandles.empty());
                __r13_cache.boundaryUnchanged = __r13_boundary_unchanged;
            }
            __r13_pending = dense_singlebcc_round13_prof::PendingInfo{};
        }
#endif
        return;
    }
    cheapfan_cert_round7::invalidateCore(oldCore);

    std::vector<int> oldCuts = blocks[oldCore].attachCuts;
    std::vector<int> touchedCuts = oldCuts;
    std::vector<int> oldBadQueries = blocks[oldCore].badQueries;
    blocks[oldCore].badQueries.clear();

    for (int cutId : oldCuts) {
        cutBCEdge(oldBlockBC, cutId);
    }
    blocks[oldCore].attachCuts.clear();

    int keepCore = -1, keepBC = -1;
    if (P.keepExists) {
        keepCore = oldCore;
        keepBC = oldBlockBC;
        rootSeeds.push_back(keepBC);
    } else {
        blocks[oldCore].alive = false;
        bcNodes[oldBlockBC].alive = false;
    }

    // Dead edges / dead exclusive vertices
#if DENSE_RECT_ROUND1_PROFILE
    auto __prof_apply_dead_t0 = dense_rect_round1_prof::Clock::now();
#endif
    for (int e : P.deadEdges) {
        if (0 <= e && e < (int)edgeOwnerCore.size() && edgeOwnerCore[e] == oldCore) {
            removeEdgeFromCore(oldCore, e);
        }
    }
    for (int v : P.deadExclusiveVertices) {
        removeVertexFromCore(oldCore, v);
    }

#if DENSE_RECT_ROUND1_PROFILE
    dense_rect_round1_prof::G().apply_dead_ns += dense_rect_round1_prof::nsSince(__prof_apply_dead_t0);
#endif

    // Create small cores
    std::vector<int> smallCore(P.small.size(), -1), smallBC(P.small.size(), -1);
    for (int sid = 0; sid < (int)P.small.size(); ++sid) {
        int core = allocBlockCore();
        int bc = allocBCNode(BCN_BLOCK, -1, core);
        blocks[core].bcNode = bc;
        smallCore[sid] = core;
        smallBC[sid] = bc;
        rootSeeds.push_back(bc);
    }

    // Materialize small pieces
#if DENSE_RECT_ROUND1_PROFILE
    auto __prof_apply_small_t0 = dense_rect_round1_prof::Clock::now();
#endif
    for (int sid = 0; sid < (int)P.small.size(); ++sid) {
        int core = smallCore[sid];
        int bc = smallBC[sid];
        (void)bc;
        for (int e : P.small[sid].edges) {
            if (P.keepExists && edgeOwnerCore[e] == oldCore) removeEdgeFromCore(oldCore, e);
            addEdgeToCore(core, e);
        }
        for (int v : P.small[sid].exclusiveVertices) {
            if (P.keepExists) removeVertexFromCore(oldCore, v);
            addVertexToCore(core, v);
            if (orig[v].alive) {
                orig[v].anchorBC = blocks[core].bcNode;
                orig[v].cutBC = -1;
                orig[v].ownerBlock = core;
            }
        }
    }

#if DENSE_RECT_ROUND1_PROFILE
    dense_rect_round1_prof::G().apply_small_ns += dense_rect_round1_prof::nsSince(__prof_apply_small_t0);
    auto __prof_apply_boundary_t0 = dense_rect_round1_prof::Clock::now();
#endif

    // Boundary reconnect
    for (const auto& bd : P.boundary) {
        int v = bd.vertex;
        int inc = (bd.touchesKeep ? 1 : 0) + (int)bd.smallIds.size();

        if (P.keepExists && !bd.touchesKeep) {
            removeVertexFromCore(keepCore, v);
        }

        int cutId = -1;
        if (bd.existedOldCut) {
            cutId = orig[v].cutBC;
            chk(cutId != -1 && bcNodes[cutId].alive, "applyPatchToCore: old cut missing");
            touchedCuts.push_back(cutId);
            rootSeeds.push_back(cutId);
        } else if (inc >= 2) {
            if (orig[v].cutBC != -1 && bcNodes[orig[v].cutBC].alive) cutId = orig[v].cutBC;
            else cutId = makeCutNode(v);
            orig[v].cutBC = cutId;
            touchedCuts.push_back(cutId);
            rootSeeds.push_back(cutId);
        }

        if (cutId != -1) {
            if (P.keepExists && bd.touchesKeep) {
                addVertexToCore(keepCore, v);
                linkBC(cutId, keepBC);
                addAttachCut(keepCore, cutId);
            }
            for (int sid : bd.smallIds) {
                int core = smallCore[sid];
                int bc = smallBC[sid];
                addVertexToCore(core, v);
                linkBC(cutId, bc);
                addAttachCut(core, cutId);
            }
            if (orig[v].alive) {
                orig[v].anchorBC = cutId;
                orig[v].cutBC = cutId;
                orig[v].ownerBlock = -1;
            }
            continue;
        }

        if (inc == 1) {
            if (P.keepExists && bd.touchesKeep) {
                addVertexToCore(keepCore, v);
                if (orig[v].alive) {
                    orig[v].anchorBC = keepBC;
                    orig[v].cutBC = -1;
                    orig[v].ownerBlock = keepCore;
                }
            } else {
                chk((int)bd.smallIds.size() == 1, "applyPatchToCore: singleton nonkeep boundary");
                int sid = bd.smallIds[0];
                int core = smallCore[sid];
                int bc = smallBC[sid];
                addVertexToCore(core, v);
                if (orig[v].alive) {
                    orig[v].anchorBC = bc;
                    orig[v].cutBC = -1;
                    orig[v].ownerBlock = core;
                }
            }
            continue;
        }

        chk(inc == 0 && bd.existedOldCut, "applyPatchToCore: inconsistent boundary");
        if (orig[v].alive) {
            orig[v].anchorBC = cutId;
            orig[v].cutBC = cutId;
            orig[v].ownerBlock = -1;
        }
    }

#if DENSE_RECT_ROUND1_PROFILE
    dense_rect_round1_prof::G().apply_boundary_ns += dense_rect_round1_prof::nsSince(__prof_apply_boundary_t0);
#endif

    // Isolated exclusive -> trivial
    for (int v : P.isolatedExclusive) {
        if (!orig[v].alive) continue;
        int triv = makeTrivialNode(v);
        orig[v].anchorBC = triv;
        orig[v].cutBC = -1;
        orig[v].ownerBlock = -1;
        rootSeeds.push_back(triv);
    }

    // Normalize touched cuts
    touchedCuts = normVec(std::move(touchedCuts));
    for (int cutId : touchedCuts) normalizeCutNode(cutId, rootSeeds);

    // Clear dead core containers if old core died
    if (!P.keepExists) {
        blocks[oldCore].allVertices.clear();
        blocks[oldCore].realEdges.clear();
        blocks[oldCore].attachCuts.clear();
        blocks[oldCore].badQueries.clear();
    }

    // Re-evaluate bad queries that were trapped in oldCore
#if DENSE_RECT_ROUND1_PROFILE
    auto __prof_apply_oldbad_t0 = dense_rect_round1_prof::Clock::now();
#endif
    for (int qid : oldBadQueries) {
        if (!(0 <= qid && qid < (int)queries.size())) continue;
        Query &q = queries[qid];
        if (!q.active || !q.badNow || q.kind != Q_BRANCH) continue;
        int c0 = (0 <= q.e0 && q.e0 < (int)edgeOwnerCore.size()) ? edgeOwnerCore[q.e0] : -1;
        int c1 = (0 <= q.e1 && q.e1 < (int)edgeOwnerCore.size()) ? edgeOwnerCore[q.e1] : -1;
        if (c0 != -1 && c0 == c1) {
            blocks[c0].badQueries.push_back(qid);
        } else {
            q.badNow = false;
            badCount[q.w]--;
            pushReadyIf(q.w);
        }
    }
#if DENSE_RECT_ROUND1_PROFILE
    dense_rect_round1_prof::G().apply_oldbad_ns += dense_rect_round1_prof::nsSince(__prof_apply_oldbad_t0);
#endif
    statecert_fastkey::bumpEpoch(oldCore);
    for (int sid = 0; sid < (int)P.small.size(); ++sid) statecert_fastkey::bumpEpoch(smallCore[sid]);
#if DENSE_BCCREUSE_ROUND12_PROFILE
    {
        auto& __r12_cache = dense_bccreuse_round12_prof::Cache();
        auto& __r12_pending = dense_bccreuse_round12_prof::Pending();
        __r12_cache = dense_bccreuse_round12_prof::CacheState{};
        if (P.keepExists && blocks[oldCore].alive) {
            __r12_cache.valid = true;
            __r12_cache.core = oldCore;
            __r12_cache.fromStep3 = (__r12_pending.active && __r12_pending.oldCore == oldCore && __r12_pending.usedStep3);
            __r12_cache.keepOnly = (P.small.empty() && P.isolatedExclusive.empty() && P.deadHandles.empty());
            __r12_cache.boundaryUnchanged = true;
            __r12_cache.prevBccCount = (__r12_cache.fromStep3 ? 1 : 0);
            __r12_cache.prevKeepEdgeCount = (int)blocks[oldCore].realEdges.size();
        }
        __r12_pending = dense_bccreuse_round12_prof::PendingStep3Info{};
    }
#endif
#if DENSE_SINGLEBCC_ROUND13_PROFILE
    {
        auto& __r13_cache = dense_singlebcc_round13_prof::Cache();
        auto& __r13_pending = dense_singlebcc_round13_prof::Pending();
        __r13_cache = dense_singlebcc_round13_prof::CacheState{};
        bool __r13_boundary_unchanged = P.keepExists && blocks[oldCore].alive && P.small.empty() && P.isolatedExclusive.empty() && P.deadHandles.empty() && ((int)P.boundary.size() == (int)oldCuts.size());
        if (__r13_boundary_unchanged) {
            for (const auto& __bd : P.boundary) {
                if (!(__bd.existedOldCut && __bd.touchesKeep && __bd.smallIds.empty())) { __r13_boundary_unchanged = false; break; }
            }
        }
        if (P.keepExists && blocks[oldCore].alive) {
            __r13_cache.valid = true;
            __r13_cache.core = oldCore;
            __r13_cache.prevStep3 = (__r13_pending.active && __r13_pending.oldCore == oldCore && __r13_pending.usedStep3);
            __r13_cache.prevSingleBcc = (__r13_pending.active && __r13_pending.oldCore == oldCore && __r13_pending.singleBcc);
            __r13_cache.prevKeepOnlyChain = (P.small.empty() && P.isolatedExclusive.empty() && P.deadHandles.empty());
            __r13_cache.boundaryUnchanged = __r13_boundary_unchanged;
        }
        __r13_pending = dense_singlebcc_round13_prof::PendingInfo{};
    }
#endif
}

void Solver::buildFromQueries() {
    clearAll();
    statecert_fastkey::reset();
    cheapfan_cert_round7::reset();
    dense_bccreuse_round12_prof::reset();
    dense_singlebcc_round13_prof::reset();
    dense_tinypiece_round14_prof::reset();
    dense_spqr_round16_prof::reset();
    dense_shadow_diff_round20_prof::reset();

    ownerQueries.assign(N + 1, {});
    indeg.assign(N + 1, 0);
    badCount.assign(N + 1, 0);
    parentAns.assign(N + 1, -1);
    orig.assign(N + 1, OrigVertex{true, -1, -1, -1});

    queries.resize(M);

    // Build fixed query-edge set
    for (int i = 0; i < M; ++i) {
        const auto& iq = inputQueries[i];
        Query q;
        q.u = iq.u; q.v = iq.v; q.w = iq.w;
        int cnt = (q.u != q.w) + (q.v != q.w);
        q.kind = (cnt == 0 ? Q_NOOP : cnt == 1 ? Q_UNARY : Q_BRANCH);
        q.active = true;
        q.badNow = false;
        q.e0 = q.e1 = -1;
        ownerQueries[q.w].push_back(i);

        if (q.u != q.w) {
            q.e0 = (int)edges.size();
            edges.push_back({q.w, q.u, -1});
            edgeOwnerCore.push_back(-1);
            edgePosInCore.push_back(-1);
            indeg[q.u]++;
        }
        if (q.v != q.w) {
            q.e1 = (int)edges.size();
            edges.push_back({q.w, q.v, -1});
            edgeOwnerCore.push_back(-1);
            edgePosInCore.push_back(-1);
            indeg[q.v]++;
        }
        queries[i] = q;
    }

    // Build current graph from all active query edges
    int E = (int)edges.size();
    vector<vector<pair<int,int>>> adj(N + 1);
    for (int e = 0; e < E; ++e) {
        int a = edges[e].u, b = edges[e].v;
        adj[a].push_back({b, e});
        adj[b].push_back({a, e});
    }

    // Iterative Tarjan BCC of edges
    vector<int> tin(N + 1, 0), low(N + 1, 0), bcc(E, -1), edgeStack;
    edgeStack.reserve(E);
    int timer = 0, bccCnt = 0;
    struct Frame { int u, parent, pe, idx; bool entered; };
    vector<Frame> st;
    st.reserve(N);

    for (int s = 1; s <= N; ++s) {
        if (!orig[s].alive || tin[s] != 0) continue;
        st.push_back({s, -1, -1, 0, false});
        while (!st.empty()) {
            Frame &fr = st.back();
            int u = fr.u;
            if (!fr.entered) {
                fr.entered = true;
                tin[u] = low[u] = ++timer;
            }
            if (fr.idx == (int)adj[u].size()) {
                int parent = fr.parent, pe = fr.pe, lowu = low[u];
                st.pop_back();
                if (parent != -1) {
                    low[parent] = std::min(low[parent], lowu);
                    if (lowu >= tin[parent]) {
                        while (true) {
                            int e = edgeStack.back(); edgeStack.pop_back();
                            bcc[e] = bccCnt;
                            if (e == pe) break;
                        }
                        bccCnt++;
                    }
                }
                continue;
            }
            auto [v, eid] = adj[u][fr.idx++];
            if (eid == fr.pe) continue;
            if (tin[v] == 0) {
                edgeStack.push_back(eid);
                st.push_back({v, u, eid, 0, false});
            } else if (tin[v] < tin[u]) {
                edgeStack.push_back(eid);
                low[u] = std::min(low[u], tin[v]);
            }
        }
    }

    vector<vector<int>> bccEdges(bccCnt);
    for (int e = 0; e < E; ++e) {
        chk(bcc[e] != -1, "buildFromQueries: some edge has no bcc");
        bccEdges[bcc[e]].push_back(e);
    }

    vector<int> firstBcc(N + 1, -1);
    vector<char> isArt(N + 1, 0), hasEdge(N + 1, 0);
    for (int e = 0; e < E; ++e) {
        for (int v : {edges[e].u, edges[e].v}) {
            hasEdge[v] = 1;
            if (firstBcc[v] == -1) firstBcc[v] = bcc[e];
            else if (firstBcc[v] != bcc[e]) isArt[v] = 1;
        }
    }

    // Create cut nodes first
    for (int v = 1; v <= N; ++v) {
        if (isArt[v]) {
            int cutId = makeCutNode(v);
            orig[v].cutBC = cutId;
            orig[v].anchorBC = cutId;
            orig[v].ownerBlock = -1;
        }
    }

    // Create blocks
    for (int bid = 0; bid < bccCnt; ++bid) {
        int core = allocBlockCore();
        int bc = allocBCNode(BCN_BLOCK, -1, core);
        blocks[core].bcNode = bc;
        for (int e : bccEdges[bid]) addEdgeToCore(core, e);
        std::unordered_set<int> seenV;
        for (int e : bccEdges[bid]) {
            int a = edges[e].u, b = edges[e].v;
            if (seenV.insert(a).second) addVertexToCore(core, a);
            if (seenV.insert(b).second) addVertexToCore(core, b);
        }
        for (int v : blocks[core].allVertices) {
            if (isArt[v]) {
                int cutId = orig[v].cutBC;
                linkBC(bc, cutId);
                addAttachCut(core, cutId);
            } else {
                orig[v].anchorBC = bc;
                orig[v].cutBC = -1;
                orig[v].ownerBlock = core;
            }
        }
    }

    // Isolated vertices -> trivial nodes
    for (int v = 1; v <= N; ++v) {
        if (!hasEdge[v]) {
            int triv = makeTrivialNode(v);
            orig[v].anchorBC = triv;
            orig[v].cutBC = -1;
            orig[v].ownerBlock = -1;
        }
    }

    // Build initial bad query lists
    for (int i = 0; i < M; ++i) {
        Query &q = queries[i];
        if (q.kind == Q_BRANCH) {
            int c0 = edgeOwnerCore[q.e0];
            int c1 = edgeOwnerCore[q.e1];
            if (c0 != -1 && c0 == c1) {
                q.badNow = true;
                badCount[q.w]++;
                blocks[c0].badQueries.push_back(i);
            }
        }
    }

    // Submission fast path currently uses local rebuild splitter directly;
    // no blockSpqr maintenance needed here.

    assignComponentRootsInitial();

    for (int v = 1; v <= N; ++v) pushReadyIf(v);
}

void Solver::eliminateOne(int x) {
#if SPARSE_HARDSCALING_ROUND5_PROFILE
    sparse_round5_prof::G().eliminate_calls++;
    sparse_round5_prof::ScopeTimer __prof_eliminate_scope(&sparse_round5_prof::G().eliminate_total_ns);
#endif
    chk(1 <= x && x <= N && orig[x].alive, "eliminateOne: bad x");
#ifdef DEBUG_SOLVER
    checkSpqrEqLocalForVertex(x);
#endif
    int oldRoot = findRootBC(orig[x].anchorBC);
    int parentLabel = compUp[oldRoot];
    parentAns[x] = parentLabel;

    // Retire owner queries of x
    for (int qid : ownerQueries[x]) {
        Query &q = queries[qid];
        if (!q.active) continue;
        chk(!(q.kind == Q_BRANCH && q.badNow), "eliminateOne: ready owner still has bad branch query");
        q.active = false;
        if (q.u != x) { indeg[q.u]--; pushReadyIf(q.u); }
        if (q.v != x) { indeg[q.v]--; pushReadyIf(q.v); }
    }

    std::vector<int> rootSeeds;

    // Trivial singleton component
    if (bcNodes[orig[x].anchorBC].type == BCN_TRIVIAL) {
        bcNodes[orig[x].anchorBC].alive = false;
        orig[x].alive = false;
        orig[x].anchorBC = orig[x].cutBC = orig[x].ownerBlock = -1;
        refreshRootsFromSeeds(oldRoot, x, rootSeeds);
        return;
    }

    if (orig[x].cutBC != -1) {
        int cutX = orig[x].cutBC;
        std::vector<int> incidentBC = bcNodes[cutX].adj;
        std::vector<int> incidentCores;
        for (int bcid : incidentBC) {
            if (!bcNodes[bcid].alive) continue;
            chk(bcNodes[bcid].type == BCN_BLOCK, "eliminateOne: cut adjacent to non-block");
            incidentCores.push_back(bcNodes[bcid].coreId);
        }
        incidentCores = normVec(std::move(incidentCores));

        // detach cut x from incident blocks
        for (int bcid : incidentBC) {
            if (!bcNodes[bcid].alive) continue;
            cutBCEdge(cutX, bcid);
            removeAttachCut(bcNodes[bcid].coreId, cutX);
        }
        bcNodes[cutX].alive = false;
        orig[x].alive = false;
        orig[x].anchorBC = orig[x].cutBC = orig[x].ownerBlock = -1;

        for (int core : incidentCores) {
            if (!blocks[core].alive) continue;
            SparsePatch P = splitBlockSPQR(core, x);
            applyPatchToCore(core, P, rootSeeds);
        }
    } else {
        int oldCore = orig[x].ownerBlock;
        chk(oldCore != -1, "eliminateOne: non-cut without ownerBlock");
        orig[x].alive = false;
        orig[x].anchorBC = orig[x].cutBC = orig[x].ownerBlock = -1;
        SparsePatch P = splitBlockSPQR(oldCore, x);
        applyPatchToCore(oldCore, P, rootSeeds);
    }

    refreshRootsFromSeeds(oldRoot, x, rootSeeds);
}

std::vector<int> Solver::solve() {
    buildFromQueries();
    for (int step = 0; step < N; ++step) {
        statecert_fastkey::currentStep = step;
        int x = popReady();
        eliminateOne(x);
    }
    if (N >= 1) parentAns[1] = 0;
    return parentAns;
}


int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, M;
    cin >> N >> M;
    vector<InputQuery> qs(M);
    for (auto& q : qs) cin >> q.u >> q.v >> q.w;

    Solver solver(N, qs);
    auto ans = solver.solve();
    statecert_fastkey::dump(cerr);
    dense_rect_round1_prof::dump(cerr);
    dense_rect_round4_keep_prof::dump(cerr);
    sparse_round5_prof::dump(cerr);
    cheapfan_round6_prof::dump(cerr);
    cheapfan_cert_round7::dump(cerr);
    combdense_round8_prof::dump(cerr);
    dense_localidadj_round9_prof::dump(cerr);
    dense_bccreuse_round12_prof::dump(cerr);
    dense_singlebcc_round13_prof::dump(cerr);
    dense_tinypiece_round14_prof::dump(cerr);
    dense_tiekeep_round15_prof::dump(cerr);
    dense_spqr_round16_prof::dump(cerr);
    dense_shadow_diff_round20_prof::dump(cerr);
    for (int i = 1; i <= N; ++i) {
        if (i > 1) cout << ' ';
        cout << ans[i];
    }
    cout << '\n';
    return 0;
}