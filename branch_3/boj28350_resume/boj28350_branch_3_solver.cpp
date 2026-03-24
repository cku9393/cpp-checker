#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace std;

namespace {

// The previous branch-local solver spent its time in a dynamic oracle and did
// not emit output before the hard_scaling timeout wall. This branch-local
// rewrite uses a static separator decomposition that directly constructs a tree
// satisfying the LCA constraints.

struct FastScanner {
    static constexpr int kBufferSize = 1 << 20;

    int index = 0;
    int size = 0;
    char buffer[kBufferSize];

    inline char read() {
        if (index >= size) {
            size = static_cast<int>(fread(buffer, 1, kBufferSize, stdin));
            index = 0;
            if (size == 0) {
                return 0;
            }
        }
        return buffer[index++];
    }

    template <class T>
    bool next_int(T& out) {
        char c = read();
        if (!c) {
            return false;
        }
        while (c <= ' ') {
            c = read();
            if (!c) {
                return false;
            }
        }
        T sign = 1;
        if (c == '-') {
            sign = -1;
            c = read();
        }
        T value = 0;
        while (c > ' ') {
            value = value * 10 + (c - '0');
            c = read();
        }
        out = value * sign;
        return true;
    }
} scanner;

constexpr int kMaxN = 100000 + 5;
constexpr int kMaxM = 100000 + 5;
constexpr int kMaxG = 400000 + 5;
constexpr int kMaxU = 200000 + 5;
constexpr int kMaxLog = 18;

struct Query {
    int u;
    int v;
    int w;
};

struct Task {
    int node_left;
    int node_right;
    int query_left;
    int query_right;
    int parent;
};

int n_vertices;
int n_queries;
Query queries[kMaxM];

int answer[kMaxN];
int loc_stamp[kMaxN];
int loc_index[kMaxN];
int current_stamp = 0;

int edge_head[kMaxN];
int edge_to[kMaxG];
int edge_next[kMaxG];
int edge_count = 0;

int unary_head[kMaxN];
int unary_to[kMaxU];
int unary_next[kMaxU];
int unary_count = 0;

int node_pool[kMaxN];
int temp_nodes[kMaxN];
int query_pool[kMaxM];
int temp_queries[kMaxM];
Task task_stack[kMaxN];
int task_count = 0;

int indegree[kMaxN];
int branch_count[kMaxN];
int query_head[kMaxN];
int next_query[kMaxM];
int tin[kMaxN];
int low[kMaxN];
int parent_idx[kMaxN];
int parent_edge[kMaxN];
int iter_edge[kMaxN];
int subtree_size[kMaxN];
int tout[kMaxN];
int rev_tin[kMaxN];
int sep_count[kMaxN];
int sep_offset[kMaxN];
int sep_cursor[kMaxN];
int sep_list[kMaxN];
int prefix_query_count[kMaxN];
int largest_component[kMaxN];
int component_of[kMaxN];
int node_count_by_group[kMaxN];
int query_count_by_group[kMaxN];
int stack_vertices[kMaxN];
int queue_vertices[kMaxN];
int root_component[kMaxN];
int dsu_parent[kMaxN];
int dag_seen_stamp[kMaxN];
int dag_out_head[kMaxN];
int dag_pred_head[kMaxN];
int dag_out_to[kMaxU];
int dag_out_next[kMaxU];
int dag_pred_to[kMaxU];
int dag_pred_next[kMaxU];
int dag_indegree[kMaxN];
int dag_depth[kMaxN];
int dag_seen_token = 0;
int fast_depth[kMaxN];
int fast_up[kMaxLog][kMaxN];

int dsu_find(int x) {
    while (dsu_parent[x] != x) {
        dsu_parent[x] = dsu_parent[dsu_parent[x]];
        x = dsu_parent[x];
    }
    return x;
}

void dsu_union(int a, int b) {
    a = dsu_find(a);
    b = dsu_find(b);
    if (a != b) {
        dsu_parent[b] = a;
    }
}

inline void add_undirected_edge(int a, int b) {
    edge_to[edge_count] = b;
    edge_next[edge_count] = edge_head[a];
    edge_head[a] = edge_count++;

    edge_to[edge_count] = a;
    edge_next[edge_count] = edge_head[b];
    edge_head[b] = edge_count++;
}

inline void add_unary_arc(int a, int b) {
    unary_to[unary_count] = b;
    unary_next[unary_count] = unary_head[a];
    unary_head[a] = unary_count++;
    ++indegree[b];
}

inline void add_dag_edge(int from, int to, int& dag_edge_count) {
    dag_out_to[dag_edge_count] = to;
    dag_out_next[dag_edge_count] = dag_out_head[from];
    dag_out_head[from] = dag_edge_count;

    dag_pred_to[dag_edge_count] = from;
    dag_pred_next[dag_edge_count] = dag_pred_head[to];
    dag_pred_head[to] = dag_edge_count;
    ++dag_indegree[to];
    ++dag_edge_count;
}

int fast_lca(int a, int b) {
    if (fast_depth[a] < fast_depth[b]) {
        swap(a, b);
    }

    int diff = fast_depth[a] - fast_depth[b];
    int bit = 0;
    while (diff) {
        if (diff & 1) {
            a = fast_up[bit][a];
        }
        diff >>= 1;
        ++bit;
    }

    if (a == b) {
        return a;
    }

    for (int k = kMaxLog - 1; k >= 0; --k) {
        if (fast_up[k][a] != fast_up[k][b]) {
            a = fast_up[k][a];
            b = fast_up[k][b];
        }
    }
    return fast_up[0][a];
}

bool try_fast_parent_dag_solution() {
    for (int i = 1; i <= n_vertices; ++i) {
        query_head[i] = -1;
        dag_out_head[i] = -1;
        dag_pred_head[i] = -1;
        dag_indegree[i] = 0;
        unary_head[i] = -1;
    }

    for (int i = 0; i < n_queries; ++i) {
        next_query[i] = query_head[queries[i].w];
        query_head[queries[i].w] = i;
    }

    int dag_edge_count = 0;
    for (int w = 1; w <= n_vertices; ++w) {
        ++dag_seen_token;
        for (int id = query_head[w]; id != -1; id = next_query[id]) {
            const Query& q = queries[id];
            if (q.u != w && dag_seen_stamp[q.u] != dag_seen_token) {
                dag_seen_stamp[q.u] = dag_seen_token;
                add_dag_edge(w, q.u, dag_edge_count);
            }
            if (q.v != w && dag_seen_stamp[q.v] != dag_seen_token) {
                dag_seen_stamp[q.v] = dag_seen_token;
                add_dag_edge(w, q.v, dag_edge_count);
            }
        }
    }

    int queue_head = 0;
    int queue_tail = 0;
    for (int v = 1; v <= n_vertices; ++v) {
        if (dag_indegree[v] == 0) {
            queue_vertices[queue_tail++] = v;
        }
    }

    int topo_count = 0;
    while (queue_head < queue_tail) {
        const int u = queue_vertices[queue_head++];
        temp_nodes[topo_count++] = u;
        for (int e = dag_out_head[u]; e != -1; e = dag_out_next[e]) {
            const int v = dag_out_to[e];
            if (--dag_indegree[v] == 0) {
                queue_vertices[queue_tail++] = v;
            }
        }
    }

    if (topo_count != n_vertices) {
        return false;
    }

    auto validate_current_tree = [&]() -> bool {
        for (int i = 0; i < n_queries; ++i) {
            const Query& q = queries[i];
            if (fast_lca(q.u, q.v) != q.w) {
                return false;
            }
        }
        return true;
    };

    answer[1] = 0;
    fast_depth[1] = 0;
    fast_up[0][1] = 0;
    for (int k = 1; k < kMaxLog; ++k) {
        fast_up[k][1] = 0;
    }

    bool ok = true;
    for (int i = 0; i < topo_count; ++i) {
        const int u = temp_nodes[i];
        if (u == 1) {
            continue;
        }

        int parent = 0;
        for (int e = dag_pred_head[u]; e != -1; e = dag_pred_next[e]) {
            const int p = dag_pred_to[e];
            parent = (parent == 0 ? p : fast_lca(parent, p));
        }
        if (parent == 0) {
            parent = 1;
        }
        if (parent <= 0 || parent > n_vertices || parent == u) {
            ok = false;
            break;
        }

        answer[u] = parent;
        fast_up[0][u] = parent;
        fast_depth[u] = fast_depth[parent] + 1;
        for (int k = 1; k < kMaxLog; ++k) {
            fast_up[k][u] = fast_up[k - 1][fast_up[k - 1][u]];
        }
    }

    if (ok && validate_current_tree()) {
        return true;
    }

    answer[1] = 0;
    dag_depth[1] = 0;
    for (int i = 0; i < topo_count; ++i) {
        const int u = temp_nodes[i];
        if (u == 1) {
            continue;
        }

        int best_parent = 1;
        int best_depth = -1;
        for (int e = dag_pred_head[u]; e != -1; e = dag_pred_next[e]) {
            const int p = dag_pred_to[e];
            const int depth = dag_depth[p];
            if (depth > best_depth || (depth == best_depth && p < best_parent)) {
                best_depth = depth;
                best_parent = p;
            }
        }

        answer[u] = best_parent;
        dag_depth[u] = best_depth + 1;
    }

    unary_count = 0;
    for (int v = 1; v <= n_vertices; ++v) {
        unary_head[v] = -1;
    }
    for (int v = 2; v <= n_vertices; ++v) {
        const int p = answer[v];
        if (p <= 0 || p > n_vertices || p == v) {
            return false;
        }
        unary_to[unary_count] = v;
        unary_next[unary_count] = unary_head[p];
        unary_head[p] = unary_count++;
    }

    queue_head = 0;
    queue_tail = 0;
    queue_vertices[queue_tail++] = 1;
    fast_depth[1] = 0;
    fast_up[0][1] = 0;

    int visited = 0;
    while (queue_head < queue_tail) {
        const int u = queue_vertices[queue_head++];
        ++visited;
        for (int e = unary_head[u]; e != -1; e = unary_next[e]) {
            const int v = unary_to[e];
            fast_up[0][v] = u;
            fast_depth[v] = fast_depth[u] + 1;
            queue_vertices[queue_tail++] = v;
        }
    }

    if (visited != n_vertices) {
        return false;
    }

    for (int k = 1; k < kMaxLog; ++k) {
        for (int v = 1; v <= n_vertices; ++v) {
            fast_up[k][v] = fast_up[k - 1][fast_up[k - 1][v]];
        }
    }

    return validate_current_tree();
}

void write_answer() {
    static char output_buffer[1 << 22];
    int ptr = 0;
    for (int i = 1; i <= n_vertices; ++i) {
        int value = answer[i];
        if (value == 0) {
            output_buffer[ptr++] = '0';
        } else {
            char digits[16];
            int len = 0;
            while (value) {
                digits[len++] = static_cast<char>('0' + (value % 10));
                value /= 10;
            }
            while (len--) {
                output_buffer[ptr++] = digits[len];
            }
        }
        output_buffer[ptr++] = (i == n_vertices ? '\n' : ' ');
    }

    fwrite(output_buffer, 1, ptr, stdout);
}

void solve_unary_block(int node_left, int node_right, int query_left, int query_right,
                       int parent_of_root, int fixed_root_global = 0) {
    const int block_size = node_right - node_left;

    ++current_stamp;
    for (int i = 0; i < block_size; ++i) {
        const int global_vertex = node_pool[node_left + i];
        loc_stamp[global_vertex] = current_stamp;
        loc_index[global_vertex] = i;
        indegree[i] = 0;
        unary_head[i] = -1;
    }

    unary_count = 0;
    for (int p = query_left; p < query_right; ++p) {
        const Query& q = queries[query_pool[p]];
        const int w = loc_index[q.w];
        if (q.u != q.w) {
            add_unary_arc(w, loc_index[q.u]);
        }
        if (q.v != q.w) {
            add_unary_arc(w, loc_index[q.v]);
        }
    }

    int root = -1;
    if (fixed_root_global != 0) {
        root = loc_index[fixed_root_global];
    } else {
        for (int i = 0; i < block_size; ++i) {
            if (indegree[i] == 0) {
                root = i;
                break;
            }
        }
    }

    int queue_head = 0;
    int queue_tail = 0;
    const int root_global = node_pool[node_left + root];
    answer[root_global] = parent_of_root;
    indegree[root] = -1;

    for (int e = unary_head[root]; e != -1; e = unary_next[e]) {
        const int v = unary_to[e];
        if (--indegree[v] == 0) {
            queue_vertices[queue_tail++] = v;
            indegree[v] = -1;
        }
    }

    for (int i = 0; i < block_size; ++i) {
        if (indegree[i] == 0) {
            queue_vertices[queue_tail++] = i;
            indegree[i] = -1;
        }
    }

    int last_parent = root_global;
    while (queue_head < queue_tail) {
        const int u = queue_vertices[queue_head++];
        const int global_vertex = node_pool[node_left + u];
        answer[global_vertex] = last_parent;
        last_parent = global_vertex;

        for (int e = unary_head[u]; e != -1; e = unary_next[e]) {
            const int v = unary_to[e];
            if (--indegree[v] == 0) {
                queue_vertices[queue_tail++] = v;
                indegree[v] = -1;
            }
        }
    }
}

void process_task(const Task& task) {
    const int node_left = task.node_left;
    const int node_right = task.node_right;
    const int query_left = task.query_left;
    const int query_right = task.query_right;
    const int parent_of_block = task.parent;

    const int block_size = node_right - node_left;
    const int block_queries = query_right - query_left;

    if (block_size == 1) {
        answer[node_pool[node_left]] = parent_of_block;
        return;
    }

    ++current_stamp;
    for (int i = 0; i < block_size; ++i) {
        const int global_vertex = node_pool[node_left + i];
        loc_stamp[global_vertex] = current_stamp;
        loc_index[global_vertex] = i;
        indegree[i] = 0;
        branch_count[i] = 0;
        query_head[i] = -1;
        edge_head[i] = -1;
    }

    bool has_branch = false;
    edge_count = 0;

    for (int p = 0; p < block_queries; ++p) {
        const int query_id = query_pool[query_left + p];
        const Query& q = queries[query_id];
        const int w = loc_index[q.w];

        next_query[p] = query_head[w];
        query_head[w] = p;

        if (q.u != q.w) {
            const int u = loc_index[q.u];
            ++indegree[u];
            add_undirected_edge(w, u);
        }
        if (q.v != q.w) {
            const int v = loc_index[q.v];
            ++indegree[v];
            add_undirected_edge(w, v);
        }
        if (q.u != q.w && q.v != q.w) {
            ++branch_count[w];
            has_branch = true;
        }
    }

    int forced_root = -1;
    for (int i = 0; i < block_size; ++i) {
        if (indegree[i] != 0) {
            continue;
        }
        if (forced_root != -1) {
            forced_root = -2;
            break;
        }
        forced_root = i;
    }

    if (!has_branch) {
        solve_unary_block(node_left, node_right, query_left, query_right, parent_of_block, 0);
        return;
    }

    if (forced_root >= 0) {
        const int root_global = node_pool[node_left + forced_root];
        for (int i = 0; i < block_size; ++i) {
            dsu_parent[i] = i;
            component_of[i] = -1;
        }

        for (int p = query_left; p < query_right; ++p) {
            const int query_id = query_pool[p];
            const Query& q = queries[query_id];
            if (q.w == root_global) {
                continue;
            }
            const int w = loc_index[q.w];
            if (q.u != q.w) {
                dsu_union(w, loc_index[q.u]);
            }
            if (q.v != q.w) {
                dsu_union(w, loc_index[q.v]);
            }
        }

        bool ok = true;
        for (int p = query_head[forced_root]; ok && p != -1; p = next_query[p]) {
            const Query& q = queries[query_pool[query_left + p]];
            if (q.u == q.w || q.v == q.w) {
                continue;
            }
            if (dsu_find(loc_index[q.u]) == dsu_find(loc_index[q.v])) {
                ok = false;
            }
        }

        if (ok) {
            int groups = 0;
            for (int v = 0; v < block_size; ++v) {
                if (v == forced_root) {
                    continue;
                }
                const int leader = dsu_find(v);
                if (component_of[leader] == -1) {
                    component_of[leader] = groups++;
                    node_count_by_group[groups - 1] = 0;
                    query_count_by_group[groups - 1] = 0;
                }
                component_of[v] = component_of[leader];
                ++node_count_by_group[component_of[v]];
            }

            for (int p = query_left; p < query_right; ++p) {
                const int query_id = query_pool[p];
                if (queries[query_id].w == root_global) {
                    continue;
                }
                ++query_count_by_group[component_of[loc_index[queries[query_id].w]]];
            }

            int non_trivial_group = -1;
            int non_trivial_count = 0;
            for (int g = 0; g < groups; ++g) {
                if (node_count_by_group[g] <= 1) {
                    continue;
                }
                non_trivial_group = g;
                ++non_trivial_count;
                if (non_trivial_count > 1) {
                    break;
                }
            }

            if (non_trivial_count == 0) {
                answer[root_global] = parent_of_block;
                for (int v = 0; v < block_size; ++v) {
                    if (v != forced_root) {
                        answer[node_pool[node_left + v]] = root_global;
                    }
                }
                return;
            }

            if (non_trivial_count == 1) {
                const int dominant_group = non_trivial_group;
                const int dominant_nodes = node_count_by_group[dominant_group];
                const int dominant_queries = query_count_by_group[dominant_group];
                int dominant_node_pos = 0;
                int dominant_query_pos = 0;

                answer[root_global] = parent_of_block;
                for (int v = 0; v < block_size; ++v) {
                    if (v == forced_root) {
                        continue;
                    }
                    const int group = component_of[v];
                    const int global_vertex = node_pool[node_left + v];
                    if (group == dominant_group) {
                        temp_nodes[dominant_node_pos++] = global_vertex;
                    } else {
                        answer[global_vertex] = root_global;
                    }
                }

                for (int p = query_left; p < query_right; ++p) {
                    const int query_id = query_pool[p];
                    if (queries[query_id].w == root_global) {
                        continue;
                    }
                    if (component_of[loc_index[queries[query_id].w]] == dominant_group) {
                        temp_queries[dominant_query_pos++] = query_id;
                    }
                }

                memcpy(node_pool + node_left, temp_nodes, sizeof(int) * dominant_nodes);
                if (dominant_queries > 0) {
                    memcpy(query_pool + query_left, temp_queries, sizeof(int) * dominant_queries);
                }

                if (dominant_nodes == 1) {
                    answer[node_pool[node_left]] = root_global;
                } else {
                    task_stack[task_count++] = {
                        node_left,
                        node_left + dominant_nodes,
                        query_left,
                        query_left + dominant_queries,
                        root_global,
                    };
                }
                return;
            }

            sep_offset[0] = 0;
            prefix_query_count[0] = 0;
            for (int g = 0; g < groups; ++g) {
                sep_offset[g + 1] = sep_offset[g] + node_count_by_group[g];
                prefix_query_count[g + 1] = prefix_query_count[g] + query_count_by_group[g];
                sep_cursor[g] = sep_offset[g];
                largest_component[g] = prefix_query_count[g];
            }

            answer[root_global] = parent_of_block;
            for (int v = 0; v < block_size; ++v) {
                if (v == forced_root) {
                    continue;
                }
                const int group = component_of[v];
                temp_nodes[sep_cursor[group]++] = node_pool[node_left + v];
            }

            for (int p = query_left; p < query_right; ++p) {
                const int query_id = query_pool[p];
                if (queries[query_id].w == root_global) {
                    continue;
                }
                const int group = component_of[loc_index[queries[query_id].w]];
                temp_queries[largest_component[group]++] = query_id;
            }

            memcpy(node_pool + node_left, temp_nodes, sizeof(int) * (block_size - 1));
            const int child_query_total = prefix_query_count[groups];
            if (child_query_total > 0) {
                memcpy(query_pool + query_left, temp_queries, sizeof(int) * child_query_total);
            }

            for (int g = groups - 1; g >= 0; --g) {
                const int group_nodes = node_count_by_group[g];
                if (group_nodes == 0) {
                    continue;
                }

                const int next_node_left = node_left + sep_offset[g];
                const int next_node_right = node_left + sep_offset[g + 1];
                const int next_query_left = query_left + prefix_query_count[g];
                const int next_query_right = query_left + prefix_query_count[g + 1];

                if (group_nodes == 1) {
                    answer[node_pool[next_node_left]] = root_global;
                } else {
                    task_stack[task_count++] = {
                        next_node_left,
                        next_node_right,
                        next_query_left,
                        next_query_right,
                        root_global,
                    };
                }
            }
            return;
        }
    }

    for (int i = 0; i < block_size; ++i) {
        tin[i] = 0;
        subtree_size[i] = 1;
        sep_count[i] = 0;
        prefix_query_count[i] = 0;
        largest_component[i] = 0;
    }

    int timer = 1;
    tin[0] = low[0] = 1;
    rev_tin[1] = 0;
    parent_idx[0] = -1;
    parent_edge[0] = -1;
    iter_edge[0] = edge_head[0];

    int stack_size = 0;
    stack_vertices[stack_size++] = 0;

    while (stack_size) {
        const int u = stack_vertices[stack_size - 1];
        int& e = iter_edge[u];
        bool advanced = false;

        while (e != -1) {
            const int edge_id = e;
            e = edge_next[e];
            const int v = edge_to[edge_id];

            if (parent_edge[u] != -1 && edge_id == (parent_edge[u] ^ 1)) {
                continue;
            }

            if (!tin[v]) {
                parent_idx[v] = u;
                parent_edge[v] = edge_id;
                tin[v] = low[v] = ++timer;
                rev_tin[timer] = v;
                subtree_size[v] = 1;
                iter_edge[v] = edge_head[v];
                stack_vertices[stack_size++] = v;
                advanced = true;
                break;
            }

            if (tin[v] < tin[u] && tin[v] < low[u]) {
                low[u] = tin[v];
            }
        }

        if (advanced) {
            continue;
        }

        --stack_size;
        tout[u] = timer;

        const int p = parent_idx[u];
        if (p != -1) {
            if (low[u] < low[p]) {
                low[p] = low[u];
            }
            subtree_size[p] += subtree_size[u];
        }
    }

    int total_sep = 0;
    for (int order = 2; order <= block_size; ++order) {
        const int v = rev_tin[order];
        const int p = parent_idx[v];
        if (low[v] >= tin[p]) {
            ++sep_count[p];
            ++total_sep;
            const int part_size = subtree_size[v];
            prefix_query_count[p] += part_size;
            if (part_size > largest_component[p]) {
                largest_component[p] = part_size;
            }
        }
    }

    for (int i = 0; i < block_size; ++i) {
        const int rest = block_size - 1 - prefix_query_count[i];
        if (rest > largest_component[i]) {
            largest_component[i] = rest;
        }
    }

    sep_offset[0] = 0;
    for (int i = 0; i < block_size; ++i) {
        sep_offset[i + 1] = sep_offset[i] + sep_count[i];
        sep_cursor[i] = sep_offset[i];
    }

    for (int order = 2; order <= block_size; ++order) {
        const int v = rev_tin[order];
        const int p = parent_idx[v];
        if (low[v] >= tin[p]) {
            sep_list[sep_cursor[p]++] = v;
        }
    }

    auto component_token = [&](int root, int node) -> int {
        const int node_tin = tin[node];
        if (node_tin < tin[root] || node_tin > tout[root]) {
            return 0;
        }

        int left = sep_offset[root];
        int right = sep_offset[root + 1];
        while (left < right) {
            const int mid = (left + right) >> 1;
            if (tin[sep_list[mid]] <= node_tin) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }

        const int pos = left - 1;
        if (pos >= sep_offset[root]) {
            const int child = sep_list[pos];
            if (node_tin <= tout[child]) {
                return child + 1;
            }
        }
        return 0;
    };

    int best_root = -1;
    int best_largest = INT_MAX;
    int best_pref = -1;
    int best_label = INT_MAX;

    auto try_root = [&](int root) {
        const int pref = branch_count[root] > 0 ? 1 : 0;
        const int largest = largest_component[root];
        const int label = node_pool[node_left + root];

        if (best_root != -1) {
            if (largest > best_largest) {
                return;
            }
            if (largest == best_largest && pref < best_pref) {
                return;
            }
            if (largest == best_largest && pref == best_pref && label >= best_label) {
                return;
            }
        }

        bool ok = true;
        if (pref) {
            const int rest = block_size - 1 - prefix_query_count[root];
            if (sep_count[root] + (rest > 0 ? 1 : 0) < 2) {
                ok = false;
            }

            for (int p = query_head[root]; ok && p != -1; p = next_query[p]) {
                const Query& q = queries[query_pool[query_left + p]];
                if (q.u == q.w || q.v == q.w) {
                    continue;
                }
                const int cu = component_token(root, loc_index[q.u]);
                const int cv = component_token(root, loc_index[q.v]);
                if (cu == cv) {
                    ok = false;
                }
            }
        }

        if (!ok) {
            return;
        }

        best_root = root;
        best_largest = largest;
        best_pref = pref;
        best_label = label;
    };

    if (forced_root >= 0) {
        try_root(forced_root);
    }

    if (best_root == -1) {
        for (int root = 0; root < block_size; ++root) {
            if (indegree[root] != 0) {
                continue;
            }
            try_root(root);
        }
    }

    const int root = best_root;
    const int root_global = node_pool[node_left + root];
    answer[root_global] = parent_of_block;

    const int sep_num = sep_count[root];
    const int rest_size = block_size - 1 - prefix_query_count[root];
    const int groups = sep_num + (rest_size > 0 ? 1 : 0);

    int base_group = 0;
    if (rest_size > 0) {
        for (int i = 0; i < block_size; ++i) {
            component_of[i] = 0;
        }
        component_of[root] = -1;
        base_group = 1;
    } else {
        for (int i = 0; i < block_size; ++i) {
            component_of[i] = -1;
        }
    }

    for (int i = 0; i < sep_num; ++i) {
        const int child = sep_list[sep_offset[root] + i];
        const int group = base_group + i;
        for (int t = tin[child]; t <= tout[child]; ++t) {
            component_of[rev_tin[t]] = group;
        }
    }

    for (int g = 0; g < groups; ++g) {
        node_count_by_group[g] = 0;
        query_count_by_group[g] = 0;
    }

    for (int v = 0; v < block_size; ++v) {
        if (v != root) {
            ++node_count_by_group[component_of[v]];
        }
    }

    for (int p = query_left; p < query_right; ++p) {
        const int query_id = query_pool[p];
        if (queries[query_id].w != root_global) {
            ++query_count_by_group[component_of[loc_index[queries[query_id].w]]];
        }
    }

    int non_trivial_group = -1;
    int non_trivial_count = 0;
    for (int g = 0; g < groups; ++g) {
        if (node_count_by_group[g] <= 1) {
            continue;
        }
        non_trivial_group = g;
        ++non_trivial_count;
        if (non_trivial_count > 1) {
            break;
        }
    }

    if (non_trivial_count == 0) {
        for (int v = 0; v < block_size; ++v) {
            if (v != root) {
                answer[node_pool[node_left + v]] = root_global;
            }
        }
        return;
    }

    if (non_trivial_count == 1) {
        const int dominant_group = non_trivial_group;
        const int dominant_nodes = node_count_by_group[dominant_group];
        const int dominant_queries = query_count_by_group[dominant_group];
        int dominant_node_pos = 0;
        int dominant_query_pos = 0;

        for (int v = 0; v < block_size; ++v) {
            if (v == root) {
                continue;
            }
            const int group = component_of[v];
            const int global_vertex = node_pool[node_left + v];
            if (group == dominant_group) {
                temp_nodes[dominant_node_pos++] = global_vertex;
            } else {
                answer[global_vertex] = root_global;
            }
        }

        for (int p = query_left; p < query_right; ++p) {
            const int query_id = query_pool[p];
            if (queries[query_id].w == root_global) {
                continue;
            }
            if (component_of[loc_index[queries[query_id].w]] == dominant_group) {
                temp_queries[dominant_query_pos++] = query_id;
            }
        }

        memcpy(node_pool + node_left, temp_nodes, sizeof(int) * dominant_nodes);
        if (dominant_queries > 0) {
            memcpy(query_pool + query_left, temp_queries, sizeof(int) * dominant_queries);
        }

        if (dominant_nodes == 1) {
            answer[node_pool[node_left]] = root_global;
        } else {
            task_stack[task_count++] = {
                node_left,
                node_left + dominant_nodes,
                query_left,
                query_left + dominant_queries,
                root_global,
            };
        }
        return;
    }

    sep_offset[0] = 0;
    prefix_query_count[0] = 0;
    for (int g = 0; g < groups; ++g) {
        sep_offset[g + 1] = sep_offset[g] + node_count_by_group[g];
        prefix_query_count[g + 1] = prefix_query_count[g] + query_count_by_group[g];
        sep_cursor[g] = sep_offset[g];
        largest_component[g] = prefix_query_count[g];
    }

    for (int v = 0; v < block_size; ++v) {
        if (v == root) {
            continue;
        }
        const int group = component_of[v];
        temp_nodes[sep_cursor[group]++] = node_pool[node_left + v];
    }

    for (int p = query_left; p < query_right; ++p) {
        const int query_id = query_pool[p];
        if (queries[query_id].w == root_global) {
            continue;
        }
        const int group = component_of[loc_index[queries[query_id].w]];
        temp_queries[largest_component[group]++] = query_id;
    }

    memcpy(node_pool + node_left, temp_nodes, sizeof(int) * (block_size - 1));
    const int child_query_total = prefix_query_count[groups];
    if (child_query_total > 0) {
        memcpy(query_pool + query_left, temp_queries, sizeof(int) * child_query_total);
    }

    for (int g = groups - 1; g >= 0; --g) {
        const int group_nodes = node_count_by_group[g];
        if (group_nodes == 0) {
            continue;
        }

        const int next_node_left = node_left + sep_offset[g];
        const int next_node_right = node_left + sep_offset[g + 1];
        const int next_query_left = query_left + prefix_query_count[g];
        const int next_query_right = query_left + prefix_query_count[g + 1];

        if (group_nodes == 1) {
            answer[node_pool[next_node_left]] = root_global;
        } else {
            task_stack[task_count++] = {
                next_node_left,
                next_node_right,
                next_query_left,
                next_query_right,
                root_global,
            };
        }
    }
}

}  // namespace

int main() {
    scanner.next_int(n_vertices);
    scanner.next_int(n_queries);

    for (int i = 1; i <= n_vertices; ++i) {
        edge_head[i] = -1;
        answer[i] = -1;
    }

    bool has_branch = false;
    for (int i = 0; i < n_queries; ++i) {
        int u;
        int v;
        int w;
        scanner.next_int(u);
        scanner.next_int(v);
        scanner.next_int(w);

        queries[i] = {u, v, w};
        if (u != w) {
            add_undirected_edge(u, w);
        }
        if (v != w) {
            add_undirected_edge(v, w);
        }
        if (u != w && v != w) {
            has_branch = true;
        }
    }

    if (!has_branch) {
        for (int i = 0; i < n_vertices; ++i) {
            node_pool[i] = i + 1;
        }
        for (int i = 0; i < n_queries; ++i) {
            query_pool[i] = i;
        }
        solve_unary_block(0, n_vertices, 0, n_queries, 0, 1);
    } else {
        if (try_fast_parent_dag_solution()) {
            write_answer();
            return 0;
        }

        answer[1] = 0;
        for (int i = 1; i <= n_vertices; ++i) {
            root_component[i] = -1;
        }

        int components = 0;
        for (int start = 2; start <= n_vertices; ++start) {
            if (root_component[start] != -1) {
                continue;
            }

            int queue_head = 0;
            int queue_tail = 0;
            queue_vertices[queue_tail++] = start;
            root_component[start] = components;

            while (queue_head < queue_tail) {
                const int x = queue_vertices[queue_head++];
                for (int e = edge_head[x]; e != -1; e = edge_next[e]) {
                    const int y = edge_to[e];
                    if (y == 1 || root_component[y] != -1) {
                        continue;
                    }
                    root_component[y] = components;
                    queue_vertices[queue_tail++] = y;
                }
            }

            ++components;
        }

        for (int c = 0; c < components; ++c) {
            node_count_by_group[c] = 0;
            query_count_by_group[c] = 0;
        }

        for (int v = 2; v <= n_vertices; ++v) {
            ++node_count_by_group[root_component[v]];
        }
        for (int i = 0; i < n_queries; ++i) {
            if (queries[i].w != 1) {
                ++query_count_by_group[root_component[queries[i].w]];
            }
        }

        sep_offset[0] = 0;
        prefix_query_count[0] = 0;
        for (int c = 0; c < components; ++c) {
            sep_offset[c + 1] = sep_offset[c] + node_count_by_group[c];
            prefix_query_count[c + 1] = prefix_query_count[c] + query_count_by_group[c];
            sep_cursor[c] = sep_offset[c];
            largest_component[c] = prefix_query_count[c];
        }

        for (int v = 2; v <= n_vertices; ++v) {
            const int c = root_component[v];
            node_pool[sep_cursor[c]++] = v;
        }
        for (int i = 0; i < n_queries; ++i) {
            if (queries[i].w == 1) {
                continue;
            }
            const int c = root_component[queries[i].w];
            query_pool[largest_component[c]++] = i;
        }

        task_count = 0;
        for (int c = components - 1; c >= 0; --c) {
            const int component_nodes = node_count_by_group[c];
            if (component_nodes == 0) {
                continue;
            }
            if (component_nodes == 1) {
                answer[node_pool[sep_offset[c]]] = 1;
            } else {
                task_stack[task_count++] = {
                    sep_offset[c],
                    sep_offset[c + 1],
                    prefix_query_count[c],
                    prefix_query_count[c + 1],
                    1,
                };
            }
        }

        while (task_count) {
            process_task(task_stack[--task_count]);
        }
    }

    write_answer();
    return 0;
}
