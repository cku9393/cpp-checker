#include <algorithm>
#include <array>
#include <climits>
#include <iostream>
#include <numeric>
#include <queue>
#include <tuple>
#include <vector>
using namespace std;

#define pb push_back
#define all(x) (x).begin(), (x).end()
using vi = vector<int>;

const int MM = 100000 + 10;

int N, Q;
int dfn[MM], low[MM], bcc[MM], dsu[MM], cnt[MM], idx, comp;
int in[MM], stk[MM], tp = -1, e2g[2 * MM], ans[MM];
vector<array<int, 3>> edge;
vi adj[MM], ord, node_list, lst[MM], eid[MM];
bool node_done[MM], edge_done[2 * MM], vis[MM];
vector<vi> graph;
vector<int> tmp_bucket_id, pending_seen;
queue<int> q, pending_layers;

static inline void ensure_graph(int id) {
    if (id >= (int)graph.size()) graph.resize(id + 1);
    if (id >= (int)pending_seen.size()) pending_seen.resize(id + 1, 0);
}

static inline void add_pending(int g) {
    if (g < 0) return;
    ensure_graph(g);
    if (graph[g].empty()) return;
    if (pending_seen[g]) return;
    pending_seen[g] = 1;
    pending_layers.push(g);
}

static inline int pop_pending() {
    while (!pending_layers.empty()) {
        int g = pending_layers.front();
        pending_layers.pop();
        if (g < 0 || g >= (int)pending_seen.size()) continue;
        if (!pending_seen[g]) continue;
        pending_seen[g] = 0;
        if (g < (int)graph.size() && !graph[g].empty()) return g;
    }
    return -1;
}

int fd(int x) {
    int r = x;
    while (dsu[r] != r) r = dsu[r];
    while (dsu[x] != x) {
        int p = dsu[x];
        dsu[x] = r;
        x = p;
    }
    return r;
}

void add(int i) {
    auto [u, v, k] = edge[i];
    if (!vis[u]) { node_list.pb(u); vis[u] = true; }
    if (!vis[v]) { node_list.pb(v); vis[v] = true; }
    adj[u].pb(v);
    adj[v].pb(u);
}

void tarjan(int start) {
    struct Frame { int u, par, it; };
    vector<Frame> dfs;
    dfs.reserve(node_list.size());

    dfn[start] = low[start] = ++idx;
    stk[++tp] = start;
    dfs.push_back({start, -1, 0});

    while (!dfs.empty()) {
        Frame &fr = dfs.back();
        int u = fr.u;

        if (fr.it < (int)adj[u].size()) {
            int v = adj[u][fr.it++];
            if (node_done[v] || v == fr.par) continue;

            if (!dfn[v]) {
                dfn[v] = low[v] = ++idx;
                stk[++tp] = v;
                dfs.push_back({v, u, 0});
            } else {
                low[u] = min(low[u], dfn[v]);
            }
            continue;
        }

        int par = fr.par;
        dfs.pop_back();
        if (par == -1) continue;

        low[par] = min(low[par], low[u]);
        if (low[u] >= dfn[par]) {
            ++comp;
            ensure_graph(comp);
            while (true) {
                int x = stk[tp--];
                bcc[x] = comp;
                if (x == u) break;
            }
        }
    }
}

void reset() {
    for (int u : node_list) {
        dfn[u] = low[u] = bcc[u] = 0;
        vis[u] = false;
        adj[u].clear();
    }
}

void rem_layer(int lvl) {
    if (lvl < 0 || lvl >= (int)graph.size() || graph[lvl].empty()) return;

    // Own and compact this layer.  All surviving edges are moved to live layers;
    // edges whose endpoint/lca is already deleted become tombstones (e2g = -1).
    // This removes the old stale-layer rescans that the original code could do
    // after queued vertices were popped.
    vi cur;
    cur.swap(graph[lvl]);

    node_list.clear();
    int last_comp = comp;

    for (int i : cur) add(i);

    for (int u : node_list) {
        if (!node_done[u] && !dfn[u]) {
            tp = -1;
            tarjan(u);
        }
    }

    if (last_comp == comp) {
        // No active edge survived in this layer.
        for (int i : cur) e2g[i] = -1;
        reset();
        return;
    }

    vector<int> touched_groups;
    vector<vi> buckets;

    auto bucket_for = [&](int g) -> vi& {
        ensure_graph(g);
        if (g >= (int)tmp_bucket_id.size()) tmp_bucket_id.resize(g + 1, -1);
        int &bid = tmp_bucket_id[g];
        if (bid == -1) {
            bid = (int)buckets.size();
            buckets.emplace_back();
            touched_groups.pb(g);
        }
        return buckets[bid];
    };

    for (int i : cur) {
        auto [lca, u, k] = edge[i];
        if (node_done[lca] || node_done[u]) {
            e2g[i] = -1;
            continue;
        }

        int g = bcc[u];
        if (dfn[lca] > dfn[u]) g = bcc[lca];
        bucket_for(g).pb(i);
        e2g[i] = g;
    }

    int largest_g = -1, largest_size = -1;
    for (int g : touched_groups) {
        int sz = (int)buckets[tmp_bucket_id[g]].size();
        if (sz > largest_size) {
            largest_size = sz;
            largest_g = g;
        }
    }

    // Reuse the old layer id for the largest surviving component.  This reduces
    // live layer id growth and, more importantly, keeps the live vectors compact:
    // graph[lvl] now contains only still-active edges.
    if (largest_g != -1) {
        int bid = tmp_bucket_id[largest_g];
        graph[lvl].swap(buckets[bid]);
        for (int i : graph[lvl]) e2g[i] = lvl;
    }

    for (int g : touched_groups) {
        if (g == largest_g) continue;
        int bid = tmp_bucket_id[g];
        graph[g].swap(buckets[bid]);
    }

    for (int i : cur) {
        auto [lca, u, k] = edge[i];
        if (edge_done[k]) continue;
        if (eid[k].size() != 2) continue;  // valid branching queries have exactly two lifted edges.

        int e1 = eid[k][0], e2 = eid[k][1];
        if (e2g[e1] != e2g[e2]) {
            edge_done[k] = true;
            cnt[lca]--;
            if (!cnt[lca] && !in[lca] && !node_done[lca]) {
                q.push(lca);
                node_done[lca] = true;
            }
        }
    }

    for (int g : touched_groups) tmp_bucket_id[g] = -1;
    reset();
}

void solve() {
    for (int i = 1; i <= N; i++) {
        if (!in[i] && !cnt[i]) {
            q.push(i);
            node_done[i] = true;
        }
    }

    rem_layer(0);

    while (ord.size() < (size_t)N) {
        while (!q.empty()) {
            int u = q.front();
            q.pop();
            ord.pb(u);

            for (int i : lst[u]) {
                int v = edge[i][1];
                in[v]--;
                if (!in[v] && !cnt[v] && !node_done[v]) {
                    q.push(v);
                    node_done[v] = true;
                }
                add_pending(e2g[i]);
            }
        }

        if (ord.size() == (size_t)N) break;

        bool progressed = false;
        while (q.empty()) {
            int g = pop_pending();
            if (g < 0) break;
            rem_layer(g);
            progressed = true;
        }

        // This should not happen for valid inputs if the invariants are correct.
        // Avoid an infinite loop on accidental invariant damage.
        if (q.empty() && !progressed) break;
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cin >> N >> Q;
    graph.assign(1, vi());
    graph.reserve(4 * (N + Q) + 16);
    pending_seen.assign(1, 0);

    for (int i = 0, k = 0, u, v, w; i < Q; i++) {
        cin >> u >> v >> w;
        if (u == v && v == w) continue;

        if (u != w) {
            int id = (int)edge.size();
            lst[w].pb(id);
            graph[0].pb(id);
            eid[k].pb(id);
            edge.pb({w, u, k});
            in[u]++;
        }
        if (v != w) {
            int id = (int)edge.size();
            lst[w].pb(id);
            graph[0].pb(id);
            eid[k].pb(id);
            edge.pb({w, v, k});
            in[v]++;
        }

        if (u == w || v == w) edge_done[k] = true;
        else cnt[w]++;
        k++;
    }

    solve();

    int p = (int)(find(all(ord), 1) - ord.begin());
    swap(ord[p], ord[0]);
    reverse(all(ord));

    iota(dsu, dsu + N + 1, 0);
    for (int w : ord) {
        for (int i : lst[w]) {
            int u = edge[i][1], fu = fd(u);
            if (fu != w) {
                dsu[fu] = w;
                ans[fu] = w;
            }
        }
    }

    cout << "0 ";
    for (int i = 2; i <= N; i++) {
        cout << (ans[i] ? ans[i] : 1) << " \n"[i == N];
    }
    return 0;
}
