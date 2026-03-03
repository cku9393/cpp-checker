#include <bits/stdc++.h>
using namespace std;

struct FastScanner {
    static const int BUFSIZE = 1 << 20;
    int idx = 0, size = 0;
    char buf[BUFSIZE];

    inline char read() {
        if (idx >= size) {
            size = (int)fread(buf, 1, BUFSIZE, stdin);
            idx = 0;
            if (size == 0) return 0;
        }
        return buf[idx++];
    }

    template <class T>
    bool nextInt(T& out) {
        char c = read();
        if (!c) return false;
        while (c <= ' ') {
            c = read();
            if (!c) return false;
        }

        T sign = 1;
        if (c == '-') {
            sign = -1;
            c = read();
        }

        T val = 0;
        while (c > ' ') {
            val = val * 10 + (c - '0');
            c = read();
        }
        out = val * sign;
        return true;
    }
} fs;

static const int MAXN = 100000 + 5;
static const int MAXM = 100000 + 5;
static const int MAXG = 400000 + 5;   // 4 * M
static const int MAXU = 200000 + 5;   // 2 * M

struct Query {
    int u, v, w;
};

struct Task {
    int nl, nr;
    int ql, qr;
    int parent;
};

int N, M;
Query Q[MAXM];

int Ans[MAXN];
int LocStamp[MAXN], LocIdx[MAXN], Stamp = 0;

// 입력 그래프용으로 쓰다가, 초기 분할 후에는 task local graph scratch로 재사용
int headE[MAXN], toE[MAXG], nxtE[MAXG], ecnt = 0;

// unary topo scratch
int headU[MAXN], toU[MAXU], nxtU[MAXU], ucnt = 0;

// task pools
int nodePool[MAXN], tempNode[MAXN];
int qidPool[MAXM], tempQ[MAXM];
Task taskSt[MAXN];
int taskTop = 0;

// scratch arrays
int indegA[MAXN], branchCntA[MAXN], headQA[MAXN], nextQA[MAXM];
int tinA[MAXN], lowA[MAXN], parA[MAXN], peA[MAXN], itA[MAXN], subA[MAXN], toutA[MAXN], revA[MAXN];
int sepCntA[MAXN], sepOffA[MAXN], curOffA[MAXN], sepListA[MAXN], sumSepA[MAXN], largestA[MAXN], compOfA[MAXN];
int cntNodeA[MAXN], cntQA[MAXN];
int stkA[MAXN], queA[MAXN];
int compA[MAXN];

inline void add_edge_global(int a, int b) {
    toE[ecnt] = b;
    nxtE[ecnt] = headE[a];
    headE[a] = ecnt++;

    toE[ecnt] = a;
    nxtE[ecnt] = headE[b];
    headE[b] = ecnt++;
}

inline void add_edge_local(int a, int b) {
    toE[ecnt] = b;
    nxtE[ecnt] = headE[a];
    headE[a] = ecnt++;

    toE[ecnt] = a;
    nxtE[ecnt] = headE[b];
    headE[b] = ecnt++;
}

inline void add_arc_u(int a, int b) {
    toU[ucnt] = b;
    nxtU[ucnt] = headU[a];
    headU[a] = ucnt++;
    ++indegA[b];
}

void solve_unary(int nl, int nr, int ql, int qr, int parent_of_root, int fixed_root = 0) {
    int n = nr - nl;

    ++Stamp;
    for (int i = 0; i < n; ++i) {
        int g = nodePool[nl + i];
        LocStamp[g] = Stamp;
        LocIdx[g] = i;
        indegA[i] = 0;
        headU[i] = -1;
    }

    ucnt = 0;
    for (int p = ql; p < qr; ++p) {
        int id = qidPool[p];
        const Query& qu = Q[id];
        int w = LocIdx[qu.w];

        if (qu.u != qu.w) add_arc_u(w, LocIdx[qu.u]);
        if (qu.v != qu.w) add_arc_u(w, LocIdx[qu.v]);
    }

    int root = -1;
    if (fixed_root != 0) {
        root = LocIdx[fixed_root];
    } else {
        for (int i = 0; i < n; ++i) {
            if (indegA[i] == 0) {
                root = i;
                break;
            }
        }
    }

    int qh = 0, qt = 0;
    int rootg = nodePool[nl + root];
    Ans[rootg] = parent_of_root;
    indegA[root] = -1;

    for (int e = headU[root]; e != -1; e = nxtU[e]) {
        int v = toU[e];
        if (--indegA[v] == 0) {
            queA[qt++] = v;
            indegA[v] = -1;
        }
    }

    for (int i = 0; i < n; ++i) {
        if (indegA[i] == 0) {
            queA[qt++] = i;
            indegA[i] = -1;
        }
    }

    int last = rootg;
    while (qh < qt) {
        int u = queA[qh++];
        int ug = nodePool[nl + u];
        Ans[ug] = last;
        last = ug;

        for (int e = headU[u]; e != -1; e = nxtU[e]) {
            int v = toU[e];
            if (--indegA[v] == 0) {
                queA[qt++] = v;
                indegA[v] = -1;
            }
        }
    }
}

void process_task(const Task& task) {
    int nl = task.nl, nr = task.nr;
    int ql = task.ql, qr = task.qr;
    int parent = task.parent;

    int n0 = nr - nl;
    int qsz = qr - ql;

    if (n0 == 1) {
        Ans[nodePool[nl]] = parent;
        return;
    }

    ++Stamp;
    for (int i = 0; i < n0; ++i) {
        int g = nodePool[nl + i];
        LocStamp[g] = Stamp;
        LocIdx[g] = i;
        indegA[i] = 0;
        branchCntA[i] = 0;
        headQA[i] = -1;
        headE[i] = -1;
    }

    bool has_branch = false;
    ecnt = 0;

    for (int p = 0; p < qsz; ++p) {
        int id = qidPool[ql + p];
        const Query& qu = Q[id];
        int w = LocIdx[qu.w];

        nextQA[p] = headQA[w];
        headQA[w] = p;

        if (qu.u != qu.w) {
            int u = LocIdx[qu.u];
            ++indegA[u];
            add_edge_local(w, u);
        }
        if (qu.v != qu.w) {
            int v = LocIdx[qu.v];
            ++indegA[v];
            add_edge_local(w, v);
        }
        if (qu.u != qu.w && qu.v != qu.w) {
            ++branchCntA[w];
            has_branch = true;
        }
    }

    if (!has_branch) {
        solve_unary(nl, nr, ql, qr, parent, 0);
        return;
    }

    for (int i = 0; i < n0; ++i) {
        tinA[i] = 0;
        subA[i] = 1;
        sepCntA[i] = 0;
        sumSepA[i] = 0;
        largestA[i] = 0;
    }

    int timer = 1;
    tinA[0] = lowA[0] = 1;
    revA[1] = 0;
    parA[0] = -1;
    peA[0] = -1;
    itA[0] = headE[0];

    int top = 0;
    stkA[top++] = 0;

    while (top) {
        int u = stkA[top - 1];
        int& e = itA[u];
        bool advanced = false;

        while (e != -1) {
            int ee = e;
            e = nxtE[e];
            int v = toE[ee];

            if (peA[u] != -1 && ee == (peA[u] ^ 1)) continue;

            if (!tinA[v]) {
                parA[v] = u;
                peA[v] = ee;
                tinA[v] = lowA[v] = ++timer;
                revA[timer] = v;
                subA[v] = 1;
                itA[v] = headE[v];
                stkA[top++] = v;
                advanced = true;
                break;
            }

            if (tinA[v] < tinA[u] && tinA[v] < lowA[u]) {
                lowA[u] = tinA[v];
            }
        }

        if (advanced) continue;

        --top;
        toutA[u] = timer;

        int p = parA[u];
        if (p != -1) {
            if (lowA[u] < lowA[p]) lowA[p] = lowA[u];
            subA[p] += subA[u];
        }
    }

    int totalSep = 0;
    for (int t = 2; t <= n0; ++t) {
        int v = revA[t];
        int p = parA[v];
        if (lowA[v] >= tinA[p]) {
            ++sepCntA[p];
            ++totalSep;
            int s = subA[v];
            sumSepA[p] += s;
            if (s > largestA[p]) largestA[p] = s;
        }
    }

    for (int i = 0; i < n0; ++i) {
        int rest = n0 - 1 - sumSepA[i];
        if (rest > largestA[i]) largestA[i] = rest;
    }

    sepOffA[0] = 0;
    for (int i = 0; i < n0; ++i) sepOffA[i + 1] = sepOffA[i] + sepCntA[i];
    for (int i = 0; i < n0; ++i) curOffA[i] = sepOffA[i];

    for (int t = 2; t <= n0; ++t) {
        int v = revA[t];
        int p = parA[v];
        if (lowA[v] >= tinA[p]) {
            sepListA[curOffA[p]++] = v;
        }
    }

    auto comp_token = [&](int r, int x) -> int {
        int tx = tinA[x];
        if (tx < tinA[r] || tx > toutA[r]) return 0;

        int l = sepOffA[r];
        int rr = sepOffA[r + 1];
        while (l < rr) {
            int m = (l + rr) >> 1;
            if (tinA[sepListA[m]] <= tx) l = m + 1;
            else rr = m;
        }

        int pos = l - 1;
        if (pos >= sepOffA[r]) {
            int c = sepListA[pos];
            if (tx <= toutA[c]) return c + 1;
        }
        return 0;
    };

    int best = -1;
    int best_largest = INT_MAX;
    int best_pref = -1;
    int best_label = INT_MAX;

    for (int r = 0; r < n0; ++r) {
        if (indegA[r] != 0) continue;

        int pref = (branchCntA[r] > 0 ? 1 : 0);
        int lg = largestA[r];
        int label = nodePool[nl + r];

        if (best != -1) {
            if (lg > best_largest) continue;
            if (lg == best_largest && pref < best_pref) continue;
            if (lg == best_largest && pref == best_pref && label >= best_label) continue;
        }

        bool ok = true;
        if (pref) {
            int rest = n0 - 1 - sumSepA[r];
            if (sepCntA[r] + (rest > 0) < 2) ok = false;

            for (int p = headQA[r]; ok && p != -1; p = nextQA[p]) {
                const Query& qu = Q[qidPool[ql + p]];
                if (qu.u == qu.w || qu.v == qu.w) continue;

                int cu = comp_token(r, LocIdx[qu.u]);
                int cv = comp_token(r, LocIdx[qu.v]);
                if (cu == cv) ok = false;
            }
        }

        if (!ok) continue;

        best = r;
        best_largest = lg;
        best_pref = pref;
        best_label = label;
    }

    int root = best;
    int rootg = nodePool[nl + root];
    Ans[rootg] = parent;

    int restSize = n0 - 1 - sumSepA[root];
    int sepNum = sepCntA[root];
    int groups = sepNum + (restSize > 0 ? 1 : 0);

    if (restSize > 0) {
        for (int i = 0; i < n0; ++i) compOfA[i] = 0;
        compOfA[root] = -1;
    } else {
        for (int i = 0; i < n0; ++i) compOfA[i] = -1;
    }

    int base = (restSize > 0 ? 1 : 0);
    for (int i = 0; i < sepNum; ++i) {
        int c = sepListA[sepOffA[root] + i];
        int gid = base + i;
        for (int t = tinA[c]; t <= toutA[c]; ++t) {
            compOfA[revA[t]] = gid;
        }
    }

    for (int g = 0; g < groups; ++g) {
        cntNodeA[g] = 0;
        cntQA[g] = 0;
    }

    for (int v = 0; v < n0; ++v) {
        if (v != root) ++cntNodeA[compOfA[v]];
    }

    for (int p = ql; p < qr; ++p) {
        int id = qidPool[p];
        if (Q[id].w == rootg) continue;
        ++cntQA[compOfA[LocIdx[Q[id].w]]];
    }

    sepOffA[0] = 0;
    sumSepA[0] = 0;
    for (int g = 0; g < groups; ++g) {
        sepOffA[g + 1] = sepOffA[g] + cntNodeA[g];
        sumSepA[g + 1] = sumSepA[g] + cntQA[g];
        curOffA[g] = sepOffA[g];
        largestA[g] = sumSepA[g];
    }

    for (int v = 0; v < n0; ++v) {
        if (v == root) continue;
        int g = compOfA[v];
        tempNode[curOffA[g]++] = nodePool[nl + v];
    }

    for (int p = ql; p < qr; ++p) {
        int id = qidPool[p];
        if (Q[id].w == rootg) continue;
        int g = compOfA[LocIdx[Q[id].w]];
        tempQ[largestA[g]++] = id;
    }

    memcpy(nodePool + nl, tempNode, sizeof(int) * (n0 - 1));
    int childQTotal = sumSepA[groups];
    if (childQTotal > 0) {
        memcpy(qidPool + ql, tempQ, sizeof(int) * childQTotal);
    }

    for (int g = groups - 1; g >= 0; --g) {
        int nn = cntNodeA[g];
        if (nn == 0) continue;

        int nodeL = nl + sepOffA[g];
        int nodeR = nl + sepOffA[g + 1];
        int qL = ql + sumSepA[g];
        int qR = ql + sumSepA[g + 1];

        if (nn == 1) {
            Ans[nodePool[nodeL]] = rootg;
        } else {
            taskSt[taskTop++] = {nodeL, nodeR, qL, qR, rootg};
        }
    }
}

int main() {
    fs.nextInt(N);
    fs.nextInt(M);

    for (int i = 1; i <= N; ++i) {
        headE[i] = -1;
        Ans[i] = -1;
    }

    bool has_branch = false;
    for (int i = 0; i < M; ++i) {
        int u, v, w;
        fs.nextInt(u);
        fs.nextInt(v);
        fs.nextInt(w);

        Q[i] = {u, v, w};

        if (u != w) add_edge_global(u, w);
        if (v != w) add_edge_global(v, w);
        if (u != w && v != w) has_branch = true;
    }

    if (!has_branch) {
        for (int i = 0; i < N; ++i) nodePool[i] = i + 1;
        for (int i = 0; i < M; ++i) qidPool[i] = i;
        solve_unary(0, N, 0, M, 0, 1);
    } else {
        Ans[1] = 0;
        for (int i = 1; i <= N; ++i) compA[i] = -1;

        int cc = 0;
        for (int s = 2; s <= N; ++s) {
            if (compA[s] != -1) continue;

            int qh = 0, qt = 0;
            queA[qt++] = s;
            compA[s] = cc;

            while (qh < qt) {
                int u = queA[qh++];
                for (int e = headE[u]; e != -1; e = nxtE[e]) {
                    int v = toE[e];
                    if (v == 1 || compA[v] != -1) continue;
                    compA[v] = cc;
                    queA[qt++] = v;
                }
            }
            ++cc;
        }

        for (int c = 0; c < cc; ++c) {
            cntNodeA[c] = 0;
            cntQA[c] = 0;
        }

        for (int v = 2; v <= N; ++v) ++cntNodeA[compA[v]];
        for (int i = 0; i < M; ++i) {
            if (Q[i].w != 1) ++cntQA[compA[Q[i].w]];
        }

        sepOffA[0] = 0;
        sumSepA[0] = 0;
        for (int c = 0; c < cc; ++c) {
            sepOffA[c + 1] = sepOffA[c] + cntNodeA[c];
            sumSepA[c + 1] = sumSepA[c] + cntQA[c];
            curOffA[c] = sepOffA[c];
            largestA[c] = sumSepA[c];
        }

        for (int v = 2; v <= N; ++v) {
            int c = compA[v];
            nodePool[curOffA[c]++] = v;
        }
        for (int i = 0; i < M; ++i) {
            if (Q[i].w == 1) continue;
            int c = compA[Q[i].w];
            qidPool[largestA[c]++] = i;
        }

        taskTop = 0;
        for (int c = cc - 1; c >= 0; --c) {
            int nn = cntNodeA[c];
            if (nn == 0) continue;

            if (nn == 1) {
                Ans[nodePool[sepOffA[c]]] = 1;
            } else {
                taskSt[taskTop++] = {sepOffA[c], sepOffA[c + 1], sumSepA[c], sumSepA[c + 1], 1};
            }
        }

        while (taskTop) {
            Task cur = taskSt[--taskTop];
            process_task(cur);
        }
    }

    static char outbuf[1 << 22];
    int ptr = 0;

    for (int i = 1; i <= N; ++i) {
        int x = Ans[i];
        if (x == 0) {
            outbuf[ptr++] = '0';
        } else {
            char s[16];
            int len = 0;
            while (x) {
                s[len++] = char('0' + (x % 10));
                x /= 10;
            }
            while (len--) outbuf[ptr++] = s[len];
        }
        outbuf[ptr++] = (i == N ? '\n' : ' ');
    }

    fwrite(outbuf, 1, ptr, stdout);
    return 0;
}
