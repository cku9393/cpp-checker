#include <bits/stdc++.h>
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
queue<int> q;

static inline void ensure_graph(int id) {
    if (id >= (int)graph.size()) graph.resize(id + 1);
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
    if (lvl < 0 || lvl >= (int)graph.size()) return;

    node_list.clear();
    int last_comp = comp;

    const int layer_size = (int)graph[lvl].size();

    for (int pos = 0; pos < layer_size; ++pos) {
        add(graph[lvl][pos]);
    }

    for (int u : node_list) {
        if (!node_done[u] && !dfn[u]) {
            tp = -1;
            tarjan(u);
        }
    }

    if (last_comp == comp) {
        reset();
        return;
    }

    for (int pos = 0; pos < layer_size; ++pos) {
        int i = graph[lvl][pos];
        auto [lca, u, k] = edge[i];
        if (node_done[lca] || node_done[u]) continue;

        int g = bcc[u];
        if (dfn[lca] > dfn[u]) g = bcc[lca];
        ensure_graph(g);
        graph[g].pb(i);
        e2g[i] = g;
    }

    for (int pos = 0; pos < layer_size; ++pos) {
        int i = graph[lvl][pos];
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
        set<int> todo;
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
                todo.insert(e2g[i]);
            }
        }

        for (int i : todo) rem_layer(i);
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cin >> N >> Q;
    graph.assign(1, vi());
    graph.reserve(4 * (N + Q) + 16);

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
