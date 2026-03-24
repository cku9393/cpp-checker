#include <bits/stdc++.h>
// progress11 preserved piece split compaction reconstruction based on progress10 final
// added preserved split exclusive timers, counters, env guards, and compact release diagnostics
using namespace std;

/*
 * Reconstructed progress9 delivery artifact.
 *
 * Durable base: progress8 profiling split and connector/watch attribution.
 * Reconstructed subset of the transient progress9 delta that was lost when the
 * container reset: per-publish tree-pos-map caching for handle annotation and
 * removal of one redundant connectorWatchEntryIds rebuild in the connector
 * skeleton publish path. The accompanying report and merged JSON preserve the
 * measured before/after numbers recovered from the prior session logs.
 */

// ===== Embedded EulerTourForest (from EulerTourForest.h/.cpp) =====

#include <string>
#include <vector>

namespace dgraph {

class Entry;

class Iterator {
public:
    explicit Iterator(Entry* entry = nullptr);
    Iterator& operator++();
    unsigned operator*();
    bool hasNext();
private:
    Entry* entry;
    friend class Entry;
    friend class EulerTourForest;
};

class TreeEdge {
public:
    TreeEdge(Entry* e = nullptr, Entry* t = nullptr);
    TreeEdge(TreeEdge&& edge) noexcept;
    TreeEdge(const TreeEdge&) = delete;
    TreeEdge& operator=(const TreeEdge&) = delete;
    TreeEdge& operator=(TreeEdge&&) = default;
    Entry* edge;
    Entry* twin;
};

class Entry {
public:
    Entry(unsigned v, Entry* l = nullptr, Entry* r = nullptr, Entry* p = nullptr);
    void splay();
    void remove();
    void rotate(bool left_rotate);
    void recalc();
    Entry* succ();
    Iterator iterator();
    unsigned vertex();
    std::string str();
    Entry* leftmost();
    Entry* rightmost();
    bool is_singleton();

    Entry* left = nullptr;
    Entry* right = nullptr;
    Entry* parent = nullptr;
    unsigned v = 0;
    unsigned size = 1;
    unsigned edges = 0;
    bool good = false;
};

Entry* merge(Entry* l, Entry* r);
Entry* find_root(Entry* e);
std::pair<Entry*, Entry*> split(Entry* e, bool keep_in_left);

class EulerTourForest {
public:
    explicit EulerTourForest(unsigned n = 0);
    EulerTourForest(EulerTourForest&& forest) noexcept;
    EulerTourForest(const EulerTourForest&) = delete;
    EulerTourForest& operator=(const EulerTourForest&) = delete;
    EulerTourForest& operator=(EulerTourForest&&) = default;
    ~EulerTourForest();

    Entry* make_root(unsigned v);
    Entry* expand(unsigned v);
    TreeEdge link(unsigned v, unsigned u);
    void cut(Entry* first, Entry* last);
    void cutoff(Entry* e, Entry* replacement = nullptr);
    void change_any(Entry* e);
    bool is_connected();
    bool is_connected(unsigned v, unsigned u);
    void increment_edges(unsigned v);
    void decrement_edges(unsigned v);
    void change_edges(unsigned v, unsigned n);
    void repair_edges_number(Entry* curr);
    unsigned size(unsigned v);
    Iterator iterator(unsigned v);
    std::string str();
    void cut(TreeEdge&& edge);
    unsigned degree(unsigned v);
    unsigned component_size(unsigned v);

private:
    unsigned n = 0;
    std::vector<Entry*> any;
    Entry* any_root = nullptr;
};

} // namespace dgraph

#include <utility>
#include <list>

namespace dgraph {

    void Entry::splay() {
        while (parent != nullptr) {
            Entry* grandpa = parent->parent;
            bool is_left = parent->left == this;
            if (grandpa != nullptr) {
                bool p_is_left = grandpa->left == parent;
                if (is_left == p_is_left) {
                    grandpa->rotate(p_is_left);
                    parent->rotate(is_left);
                } else {
                    parent->rotate(is_left);
                    grandpa->rotate(p_is_left);
                }
            } else {
                parent->rotate(is_left);
            }
        }
    }

    void Entry::remove() {
        splay();
        if (left != nullptr) {
            left->parent = nullptr;
        }
        if (right != nullptr) {
            right->parent = nullptr;
        }
        if (left == nullptr || right == nullptr){
            return;
        }
        merge(left, right);
    }

    void Entry::rotate(bool left_rotate){
        Entry* child = nullptr;
        if(left_rotate) {
            child = left;
            left = child->right;
            if (left != nullptr) {
                left->parent = this;
            }
            child->right = this;
        } else {
            child = right;
            right = child->left;
            if (right != nullptr) {
                right->parent = this;
            }
            child->left = this;
        }
        if (parent != nullptr) {
            if (this == parent->left){
                parent->left = child;
            } else {
                parent->right = child;
            }
        }
        child->parent = parent;
        parent = child;
        recalc();
        child->recalc();
        if (parent != nullptr){
            parent->recalc();
        }
    }

    Entry* merge(Entry* l, Entry* r) {
        if (l == nullptr) {
            return r;
        }
        if (r == nullptr) {
            return l;
        }
        r = find_root(r);
        l = find_root(l)->rightmost();

        l->splay();
        l->right = r;
        r->parent = l;
        l->recalc();
        return l;
    }

    Entry* find_root(Entry* e) {
        while (e->parent != nullptr) e = e->parent;
        return e;
    }

    Entry::Entry(unsigned v, Entry* l, Entry* r, Entry* p) : left(l), right(r), parent(p), v(v),
                                                             size(1), edges(0), good(false) {}

    Entry* Entry::succ() {
        Entry* curr = this;
        if(right == nullptr){
            while (curr->parent != nullptr && curr == curr->parent->right) curr = curr->parent;
            if (curr->parent == nullptr){
                return nullptr;
            }
            return curr->parent;
        }
        curr = right;
        curr = curr->leftmost();
        return curr;
    }

    std::pair<Entry*, Entry*> split(Entry* e, bool keep_in_left) {
        e->splay();
        Entry* left;
        Entry* right;
        if (keep_in_left) {
            left = e;
            right = e->right;
            e->right = nullptr;
            left->recalc();
            if (right != nullptr) {
                right->recalc();
                right->parent = nullptr;
            }
        } else {
            left = e->left;
            right = e;
            e->left = nullptr;
            right->recalc();
            if (left != nullptr) {
                left->recalc();
                left->parent = nullptr;
            }
        }
        return std::make_pair(left, right);
    }

    void Entry::recalc() {
        size = 1;
        good = edges > 0;
        if(right != nullptr){
            size += right->size;
            good |= right->good;
        }
        if(left != nullptr){
            size += left->size;
            good |= left->good;
        }
    }

    EulerTourForest::EulerTourForest(unsigned n) : n(n), any_root(nullptr) {
        for (unsigned i = 0; i < n; i++) {
            auto* vertex = new Entry(i);
            any.push_back(vertex);
        }
    }

    EulerTourForest::EulerTourForest(EulerTourForest&& forest) noexcept :n(forest.n), any(std::move(forest.any)),
                                                                         any_root(forest.any_root) {
        forest.n = 0;
    }

    EulerTourForest::~EulerTourForest() {
        std::vector<bool> vis(n, false);
        std::list<Entry*> entries;
        for (unsigned i = 0; i < n; i++) {
            if (vis[i]) {
                continue;
            }
            vis[i] = true;
            Entry* e = find_root(any[i])->leftmost();
            while (e != nullptr){
                vis[e->v] = true;
                entries.push_back(e);
                e = e->succ();
            }
        }
        for (Entry* e : entries) {
            delete e;
        }
    }

    Entry* EulerTourForest::make_root(unsigned v) {
        Entry* e = any[v];
        auto cut = split(e, false);
        return merge(cut.second, cut.first);
    }

    Entry* EulerTourForest::expand(unsigned v) {
        Entry* e = make_root(v);
        if (e->size == 1){
            return e;
        }
        auto new_node = new Entry(v);
        merge(e, new_node);
        return new_node;
    }

    TreeEdge EulerTourForest::link(unsigned v, unsigned u) {
        Entry* l = expand(v);
        Entry* r = expand(u);
        any_root = merge(l, r);
        return {l, r};
    }

    void EulerTourForest::cut(Entry* first, Entry* last) {
        any_root = nullptr;
        auto first_cut = split(first, true);
        bool right_ordered = first_cut.second != nullptr && find_root(first_cut.second) == find_root(last);
        auto second_cut = split(last, true);
        if (!right_ordered) {
            std::swap(first_cut, second_cut);
        }
        Entry* to_remove = first_cut.first->rightmost();
        if (to_remove->is_singleton()) {
            if (second_cut.second != nullptr) {
                change_any(second_cut.second->leftmost());
                delete to_remove;
            }
        } else {
            merge(to_remove, second_cut.second);
            Entry* next = to_remove->succ();
            if (next == nullptr) {
                cutoff(to_remove);
            } else {
                cutoff(to_remove, next);
            }
        }
        cutoff(second_cut.first->rightmost());
    }

    void EulerTourForest::cutoff(Entry* e, Entry* replacement) {
        if (e->is_singleton()) {
            return;
        }
        if (any[e->v] == e){
            if (replacement == nullptr) {
                change_any(find_root(e)->leftmost());
            } else {
                change_any(replacement);
            }
        }
        e->remove();
        delete e;
    }

    void EulerTourForest::change_any(Entry* e) {
        unsigned edges = any[e->v]->edges;
        unsigned v = e->v;
        change_edges(v, 0);
        any[v] = e;
        change_edges(v, edges);
    }

    bool EulerTourForest::is_connected() {
        return any_root != nullptr && any_root->size == 2 * (n - 1);
    }

    bool EulerTourForest::is_connected(unsigned v, unsigned u) {
        if (is_connected()) {
            return true;
        }
        return find_root(any[v]) == find_root(any[u]);
    }

    void EulerTourForest::increment_edges(unsigned v) {
        Entry* curr = any[v];
        ++curr->edges;
        if (curr->edges == 1) {
            curr->good = true;
            repair_edges_number(curr->parent);
        }
    }

    void EulerTourForest::decrement_edges(unsigned v) {
        Entry* curr = any[v];
        --curr->edges;
        if (curr->edges == 0) {
            repair_edges_number(curr);
        }
    }

    void EulerTourForest::change_edges(unsigned v, unsigned n) {
        Entry* curr = any[v];
        curr->edges = n;
        repair_edges_number(curr);
    }

    void EulerTourForest::repair_edges_number(Entry* curr){
        while (curr != nullptr) {
            bool good = curr->edges > 0;
            if (curr->left != nullptr) {
                good |= curr->left->good;
            }
            if (curr->right != nullptr) {
                good |= curr->right->good;
            }
            if (good != curr->good) {
                curr->good = good;
                curr = curr->parent;
            } else {
                return;
            }
        }
    }

    unsigned EulerTourForest::size(unsigned v) {
        return find_root(any[v])->size;
    }

    Iterator EulerTourForest::iterator(unsigned v){
        return any[v]->iterator();
    }

    std::string EulerTourForest::str() {
        std::string str;
        std::vector<bool> vis(n, false);
        for (unsigned i = 0; i < n; i++) {
            Entry* curr = find_root(any[i]);
            if(!vis[curr->vertex()]){
                vis[curr->vertex()] = true;
                str += curr->str() + "\n";
            }
        }
        str += "edges: \n";
        for (unsigned i = 0; i < n; i++) {
            str += std::to_string(any[i]->edges) + " ";
        }
        str += "\n";
        return str;
    }

    void EulerTourForest::cut(TreeEdge&& edge) {
        if (edge.edge != nullptr) {
            cut(edge.edge, edge.twin);
        }
    }

    unsigned EulerTourForest::degree(unsigned v) {
        return any[v]->edges;
    }

    unsigned EulerTourForest::component_size(unsigned v) {
        unsigned nodes = size(v);
        if (nodes == 1) {
            return 1;
        }
        return nodes / 2 + 1;
    }

    Iterator::Iterator(Entry* entry) :entry(entry){}

    Iterator& Iterator::operator++() {
        if (entry->right != nullptr && entry->right->good){
            entry = entry->right;
            while (true){
                if (entry->left != nullptr && entry->left->good){
                    entry = entry->left;
                    continue;
                }
                if(entry->edges > 0){
                    return *this;
                }
                entry = entry->right;
            }
        }
        while (true) {
            if (entry->parent == nullptr) {
                entry = nullptr;
                return *this;
            }
            if (entry->parent->right != nullptr && entry->parent->right == entry){
                entry = entry->parent;
                continue;
            } else {
                entry = entry->parent;
                break;
            }
        }
        if (entry->edges > 0){
            return *this;
        }
        return ++(*this);
    }

    unsigned Iterator::operator*() {
        return entry->v;
    }

    bool Iterator::hasNext() {
        return entry != nullptr;
    }

    Iterator Entry::iterator() {
        Entry* curr = find_root(this)->leftmost();
        Iterator iterator(curr);
        if(!curr->good) {
            ++iterator;
        }
        return iterator;
    }

    unsigned Entry::vertex() {
        return v;
    }

    std::string Entry::str() {
        std::string str;
        Entry* e = leftmost();
        while(e != nullptr){
            str += std::to_string(e->v);
            e = e->succ();
        }
        return str;
    }

    Entry* Entry::leftmost() {
        Entry* curr = this;
        while (curr->left != nullptr) curr = curr->left;
        return curr;
    }

    Entry* Entry::rightmost() {
        Entry* curr = this;
        while (curr->right != nullptr) curr = curr->right;
        return curr;
    }

    bool Entry::is_singleton() {
        return parent == nullptr && left == nullptr && right == nullptr;
    }

    TreeEdge::TreeEdge(Entry* e, Entry* t) :edge(e), twin(t) {}

    TreeEdge::TreeEdge(TreeEdge&& edge) noexcept :edge(edge.edge), twin(edge.twin){
        edge.twin = nullptr;
        edge.edge = nullptr;
    }
}

#include <optional>
#include <string>
#include <vector>

namespace dgraph {

class Edge;
class List;

class ListIterator {
public:
    explicit ListIterator(List* list = nullptr);
    ListIterator operator++(int);
    List* operator*();
    bool hasNext();
private:
    List* list;
};

class List {
public:
    List(unsigned u, Edge* edge, List* prev, List* next);
    List();
    ~List();
    List* add(unsigned v, Edge* edge);
    ListIterator iterator();
    unsigned vertex();
    Edge* e();

    unsigned u;
    Edge* edge;
    List* prev;
    List* next;
};

class Edge {
public:
    Edge(unsigned lvl, unsigned v, unsigned u);
    ~Edge();
    void subscribe(List* first, List* second);
    unsigned level();
    void removeLinks();
    unsigned from();
    unsigned to();
    void add_tree_edge(TreeEdge&& edge);
    bool is_tree_edge();

    unsigned lvl;
    unsigned v;
    unsigned u;
    List* first_link = nullptr;
    List* second_link = nullptr;
    std::vector<TreeEdge> tree_edges;
};

class EdgeToken {
public:
    explicit EdgeToken(Edge* edge);
    EdgeToken(EdgeToken&& e) noexcept;
    EdgeToken(const EdgeToken&) = delete;
    EdgeToken& operator=(const EdgeToken&) = delete;
    EdgeToken& operator=(EdgeToken&& other) noexcept;
    EdgeToken();
    bool moved();
private:
    Edge* edge;
    friend class DynamicGraph;
};

class DynamicGraph {
public:
    explicit DynamicGraph(unsigned n);
    ~DynamicGraph();
    EdgeToken add(unsigned v, unsigned u);
    void remove(EdgeToken&& edge_token);
    void downgrade(Edge* e);
    bool is_connected(unsigned v, unsigned u);
    bool is_connected();
    std::string str();
    unsigned degree(unsigned v);
    unsigned component_size(unsigned v);

private:
    unsigned n = 0;
    unsigned size = 0;
    std::vector<EulerTourForest> forests;
    std::vector<std::vector<List*>> adjLists;
};

} // namespace dgraph


#include <cmath>
#include <utility>
#include <limits>
#include <iostream>

namespace dgraph {

    DynamicGraph::DynamicGraph(unsigned n) : n(n) {
        size = std::lround(std::ceil(std::log2(n)) + 1);
        for (unsigned i = 0; i < size; i++) {
            forests.emplace_back(n);
            adjLists.emplace_back();
            for (unsigned j = 0; j < n; j++){
                adjLists[i].push_back(new List());
            }
        }
    }

    DynamicGraph::~DynamicGraph() {
        for (unsigned i = 0; i < size; i++) {
            for (unsigned j = 0; j < n; j++) {
                ListIterator it = adjLists[i][j]->iterator();
                while (it.hasNext()) {
                    List* list = *it;
                    it++;
                    delete list->e();
                }
                delete *it;
            }
        }
    }

    EdgeToken DynamicGraph::add(unsigned v, unsigned u) {
        if (v == u) {
            return EdgeToken(nullptr);
        }
        unsigned n = size - 1;
        auto* edge = new Edge(n, v, u);
        if (!is_connected(v, u)) {
            edge->add_tree_edge(forests[n].link(v, u));
        }
        forests[n].increment_edges(v);
        forests[n].increment_edges(u);
        edge->subscribe(adjLists[n][v]->add(u, edge), adjLists[n][u]->add(v, edge));
        return EdgeToken(edge);
    }

    void DynamicGraph::remove(EdgeToken&& edge_token) {
        Edge* link = edge_token.edge;
        edge_token.edge = nullptr;
        if (link == nullptr) {
            return;
        }

        unsigned v = link->from();
        unsigned u = link->to();
        bool complex_deletion = link->is_tree_edge();
        unsigned level = link->level();

        if (complex_deletion) {
            for (unsigned i = 0; i <= size - level - 1; i++){
                forests[size - i - 1].cut(std::move(link->tree_edges[i]));
            }
        }

        forests[level].decrement_edges(v);
        forests[level].decrement_edges(u);

        delete link;

        if (complex_deletion) {
            for (unsigned i = level; i < size; i++){
                // find new connection
                // to do that choose the lesser component
                if(forests[i].size(v) > forests[i].size(u)){
                    std::swap(v, u);
                }
                // and iterate over good vertices until success
                // propagating all tree edges of smallest component
                Edge* replacement = nullptr;
                Iterator it = forests[i].iterator(v);
                while(it.hasNext()){
                    unsigned w = *it;
                    ListIterator lit = adjLists[i][w]->iterator();
                    while(lit.hasNext()){
                        List* l = *(lit++);
                        Edge* e = l->e();
                        unsigned up = l->vertex();
                        if (e->is_tree_edge()) {
                            downgrade(e);
                        } else {
                            if (replacement != nullptr) {
                                break;
                            }
                            if (is_connected(up, u)) {
                                replacement = e;
                            } else {
                                downgrade(e);
                            }
                        }
                    }
                    ++it;
                }

                if (replacement != nullptr) {
                    for (unsigned j = size - 1; j >= i; j--){
                        replacement->add_tree_edge(forests[j].link(replacement->v, replacement->u));
                    }
                    break;
                }
            }
        }
    }

    void DynamicGraph::downgrade(Edge* e){
        unsigned v = e->from();
        unsigned w = e->to();
        unsigned lvl = e->lvl--;
        e->removeLinks();
        e->subscribe(adjLists[lvl - 1][w]->add(v, e), adjLists[lvl - 1][v]->add(w, e));
        forests[lvl].decrement_edges(w);
        forests[lvl].decrement_edges(v);
        forests[lvl - 1].increment_edges(w);
        forests[lvl - 1].increment_edges(v);
        if (e->is_tree_edge()) {
            e->add_tree_edge(forests[lvl - 1].link(v, w));
        }
    }

    bool DynamicGraph::is_connected(unsigned v, unsigned u) {
        return forests[forests.size() - 1].is_connected(v, u);
    }

    bool DynamicGraph::is_connected() {
        return forests[forests.size() - 1].is_connected();
    }

    std::string DynamicGraph::str() {
        std::string str;
        for(unsigned i = 0; i < size; i++){
            str += "level " + std::to_string(i) + ": \n";
            str += forests[i].str() + "\n";
        }
        return str;
    }

    unsigned DynamicGraph::degree(unsigned v) {
        unsigned sum = 0;
        for (unsigned i = 0; i < size; i++) {
            sum += forests[i].degree(v);
        }
        return sum;
    }

    unsigned DynamicGraph::component_size(unsigned v) {
        return forests[forests.size() - 1].component_size(v);
    }

    List* List::add(unsigned v, Edge* edge) {
        List* new_list = new List(v, edge, prev, this);
        prev->next = new_list;
        prev = new_list;
        return new_list;
    }

    List::~List() {
        next->prev = prev;
        prev->next = next;
    }

    List::List(unsigned u, Edge* edge, List* prev, List* next) :u(u), edge(edge), prev(prev), next(next){}

    List::List() :edge(nullptr) {
        next = this;
        prev = this;
        u = std::numeric_limits<unsigned>::max();
    }

    ListIterator List::iterator() {
        return ListIterator(next);
    }

    unsigned List::vertex() {
        return u;
    }

    Edge* List::e() {
        return edge;
    }

    Edge::Edge(unsigned lvl, unsigned v, unsigned u) : lvl(lvl), v(v), u(u) {}

    void Edge::subscribe(List* first, List* second) {
        first_link = first;
        second_link = second;
    }

    unsigned Edge::level() {
        return lvl;
    }

    void Edge::removeLinks() {
        delete first_link;
        delete second_link;
        first_link = nullptr;
        second_link = nullptr;
    }

    unsigned Edge::from() {
        return v;
    }

    unsigned Edge::to() {
        return u;
    }

    void Edge::add_tree_edge(TreeEdge&& edge) {
        tree_edges.push_back(std::move(edge));
    }

    bool Edge::is_tree_edge() {
        return !tree_edges.empty();
    }

    Edge::~Edge() {
        removeLinks();
    }

    ListIterator::ListIterator(List* list) :list(list) {}

    ListIterator ListIterator::operator++(int) {
        ListIterator state(list);
        list = list->next;
        return state;
    }

    List* ListIterator::operator*() {
        return list;
    }

    bool ListIterator::hasNext() {
        return list->edge != nullptr;
    }

    EdgeToken::EdgeToken(Edge* edge) :edge(edge){}

    EdgeToken::EdgeToken(EdgeToken&& e) noexcept :edge(e.edge){
        e.edge = nullptr;
    }

    EdgeToken& EdgeToken::operator=(EdgeToken&& other) noexcept {
        edge = other.edge;
        other.edge = nullptr;
        return *this;
    }

    EdgeToken::EdgeToken() :edge(nullptr){}

    bool EdgeToken::moved() {
        return edge == nullptr;
    }
}

struct RawQuery { int u, v, w; };
struct BranchQuery { int owner, a, b, multiplicity; };
struct WitnessChange { int qid, newHandle; bool resolved; };
struct OwnerSplitArtifact {
    int removedX = -1;
    vector<int> visitedVerts;
    vector<int> parentVals;
    vector<int> rootVals;
    vector<int> depthVals;
    vector<int> tinVals;
    vector<int> toutVals;
    vector<int> lowVals;
    vector<int> compVals;
    bool valid = false;
};

static bool runtime_env_enabled(const char* name, bool defv) {
    const char* v = getenv(name);
    if (!v || !*v) return defv;
    if (strcmp(v, "0") == 0 || strcasecmp(v, "false") == 0 || strcasecmp(v, "off") == 0 || strcasecmp(v, "no") == 0) return false;
    if (strcmp(v, "1") == 0 || strcasecmp(v, "true") == 0 || strcasecmp(v, "on") == 0 || strcasecmp(v, "yes") == 0) return true;
    return defv;
}
static bool reuse_apply_opt_enabled() {
    static bool enabled = runtime_env_enabled("ENABLE_REUSE_APPLY_OPT", false);
    return enabled;
}
static bool preserved_split_opt_enabled() {
    static bool enabled = runtime_env_enabled("ENABLE_PRESERVED_SPLIT_OPT", false);
    return enabled;
}
static bool watch_scan_opt_enabled() {
    static bool enabled = runtime_env_enabled("ENABLE_WATCH_SCAN_OPT", false);
    return enabled;
}
static bool retain_compaction_opt_enabled() {
    static bool enabled = runtime_env_enabled("ENABLE_RETAIN_COMPACTION_OPT", false);
    return enabled;
}
static bool kept_vector_opt_enabled() {
    static bool enabled = runtime_env_enabled("ENABLE_KEPT_VECTOR_OPT", false);
    return enabled;
}
static bool stable_compaction_opt_enabled() {
    static bool enabled = runtime_env_enabled("ENABLE_STABLE_COMPACTION_OPT", false);
    return enabled;
}
static bool block_copy_compaction_opt_enabled() {
    static bool enabled = runtime_env_enabled("ENABLE_BLOCK_COPY_COMPACTION_OPT", false);
    return enabled;
}
static bool copy_plan_build_opt_enabled() {
    static bool enabled = runtime_env_enabled("ENABLE_COPY_PLAN_BUILD_OPT", false);
    return enabled;
}
static bool run_discovery_fusion_opt_enabled() {
    static bool enabled = runtime_env_enabled("ENABLE_RUN_DISCOVERY_FUSION_OPT", false);
    return enabled;
}
static bool fused_discovery_classify_opt_enabled() {
    static bool enabled = runtime_env_enabled("ENABLE_FUSED_DISCOVERY_CLASSIFY_OPT", false);
    return enabled;
}
static bool tscan_core_opt_enabled() {
    static bool enabled = runtime_env_enabled("ENABLE_TSCAN_CORE_OPT", false);
    return enabled;
}
static bool compact_release_diag_enabled() {
    static bool enabled = runtime_env_enabled("ENABLE_COMPACT_RELEASE_DIAG", false);
    return enabled;
}
static long long runtime_now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
static long long g_release_diag_case_start_ns = 0;
static int g_release_diag_total_deletions = 0;
static int g_release_diag_last_deletion = 0;
static long long g_release_diag_preserved_piece_split_calls = 0;
static long long g_release_diag_preserved_piece_split_vertices = 0;
static long long g_release_diag_preserved_piece_split_ns = 0;
static void emit_release_diag(const char* phase, int deletion, int x) {
    if (!compact_release_diag_enabled()) return;
    const char* run_tag = getenv("RUN_TAG");
    long long elapsed_ms = 0;
    if (g_release_diag_case_start_ns != 0) elapsed_ms = (runtime_now_ns() - g_release_diag_case_start_ns) / 1000000LL;
    fprintf(stderr,
            "[release_diag] phase=%s run_tag=%s deletion=%d/%d x=%d elapsed_ms=%lld psplit_calls=%lld psplit_vertices=%lld psplit_ns=%lld\n",
            phase, run_tag ? run_tag : "", deletion, g_release_diag_total_deletions, x, elapsed_ms,
            g_release_diag_preserved_piece_split_calls, g_release_diag_preserved_piece_split_vertices,
            g_release_diag_preserved_piece_split_ns);
    fflush(stderr);
}
static void compact_release_diag_case_start(int total_deletions) {
    g_release_diag_case_start_ns = runtime_now_ns();
    g_release_diag_total_deletions = total_deletions;
    g_release_diag_last_deletion = 0;
    g_release_diag_preserved_piece_split_calls = 0;
    g_release_diag_preserved_piece_split_vertices = 0;
    g_release_diag_preserved_piece_split_ns = 0;
    emit_release_diag("case_start", 0, -1);
}
static void compact_release_diag_init_done() {
    emit_release_diag("init_done", g_release_diag_last_deletion, -1);
}
static void compact_release_diag_maybe_checkpoint(int deletion, int x) {
    g_release_diag_last_deletion = deletion;
    if (deletion == 1 || (deletion % 64) == 0) emit_release_diag("deletion_checkpoint", deletion, x);
}
static void compact_release_diag_summary() {
    emit_release_diag("summary", g_release_diag_last_deletion, -1);
}

#ifdef LOCAL
struct StrictChildDebugStats {
    long long strict_child_exists_but_missed = 0;
    long long strict_child_found = 0;
    long long strict_child_structural_miss = 0;
    long long semantic_escape_count = 0;
    long long strict_child_rebuild_used = 0;
    long long strict_child_global_fallback_used = 0;
    long long strict_child_depth_sum = 0;
    long long strict_child_rebuild_vertices = 0;
    long long strict_child_rebuild_edges = 0;
    long long build_exact_restricted_calls = 0;
    long long build_exact_restricted_vertices = 0;
    long long build_exact_restricted_edges = 0;
    long long fast_restricted_search_calls = 0;
    long long fast_restricted_search_vertices = 0;
    long long fast_restricted_search_edges = 0;
    long long try_build_child_calls = 0;
    long long try_build_child_success = 0;
    long long same_base_relocation_count = 0;
    long long proper_child_relocation_count = 0;
    long long region_size_before_sum = 0;
    long long region_size_after_sum = 0;
    long long cert_size_before_sum = 0;
    long long cert_size_after_sum = 0;
    long long region_stats_count = 0;
    long long cert_untouched_fast_keep = 0;
};
static StrictChildDebugStats g_strict_child_dbg;
struct TopologyDebugStats {
    long long dbg_owner_rebuild_calls = 0;
    long long dbg_owner_rebuild_vertices = 0;
    long long dbg_owner_rebuild_edges = 0;
    vector<long long> dbg_owner_rebuild_by_owner;
    long long dbg_owner_local_updates = 0;
    long long dbg_owner_local_updates_fallback = 0;
    long long dbg_endpoint_partition_mismatch = 0;
    long long dbg_fallback_deleted_owner = 0;
    long long dbg_fallback_multi_old_class_touch = 0;
    long long dbg_fallback_relabel_collision = 0;
    long long dbg_fallback_endpoint_outside_zone = 0;
    long long dbg_fallback_component_merge_ambiguous = 0;
    long long topology_zone_bfs_vertices = 0;
    long long topology_zone_bfs_edges = 0;
    long long global_delete_dfs_calls = 0;
    long long global_delete_dfs_vertices = 0;
    long long global_delete_dfs_edges = 0;
    long long global_delete_component_count = 0;
    long long owner_bucket_assignments = 0;
    long long owner_bucket_binary_search_steps = 0;
    long long owner_relabel_calls = 0;
    long long owner_relabel_active_endpoints = 0;
    long long owner_relabel_moved_endpoints = 0;
    long long owner_relabel_candidate_classes = 0;
    long long class_local_refine_calls = 0;
    long long class_local_refine_endpoints = 0;
    long long class_local_refine_moved_endpoints = 0;
    long long class_local_new_class_count = 0;
    long long class_local_kept_old_cid_count = 0;
    long long untouched_class_skips = 0;
    long long owner_wide_relabel_calls = 0;
    long long owner_wide_relabel_endpoints = 0;
    long long topo_active_endpoint_total = 0;
    long long topo_active_endpoint_peak = 0;
    long long topo_deactivated_endpoint_count = 0;
};
static TopologyDebugStats g_topo_dbg;
struct BatchPivotDebugStats {
    long long owner_support_build_calls = 0;
    long long owner_support_build_vertices = 0;
    long long owner_support_build_edges = 0;
    long long owner_support_relevant_endpoints_sum = 0;
    long long owner_support_watch_vertices_sum = 0;
    long long owner_touched_by_watch = 0;
    long long owner_touched_unique = 0;
    long long class_split_events = 0;
    long long moved_endpoint_count = 0;
    long long query_incident_scans = 0;
    long long query_resolved_by_split = 0;
    long long query_resolved_owner_dead_or_endpoint_dead = 0;
    long long active_query_peak = 0;
    long long support_watch_peak = 0;
    long long local_active_mismatch = 0;
    long long local_active_partition_mismatch = 0;
    long long debug_first_divergence_dumped = 0;
    long long debug_reference_compare_calls = 0;
    long long debug_reference_compare_preserved_only_calls = 0;
    long long debug_reference_compare_connector_only_calls = 0;
    long long debug_reference_compare_both_calls = 0;
    long long debug_divergence_support_vertex_coverage = 0;
    long long debug_divergence_support_disconnected = 0;
    long long debug_divergence_attachment_dead = 0;
    long long debug_divergence_attachment_wrong_piece = 0;
    long long debug_divergence_terminal_set = 0;
    long long debug_divergence_touched_missing = 0;
    long long debug_divergence_touched_extra = 0;
    long long debug_divergence_mode_mismatch = 0;
    long long debug_divergence_other = 0;
    long long debug_touched_check_calls = 0;
    long long debug_touched_missing_classes = 0;
    long long debug_touched_extra_classes = 0;
    long long support_build_failures = 0;
    long long support_rebuild_artifact_calls = 0;
    long long support_rebuild_artifact_vertices = 0;
    long long support_rebuild_artifact_chain_steps = 0;
    long long support_rebuild_fallback_calls = 0;
    long long support_rebuild_fallback_vertices = 0;
    long long support_rebuild_fallback_edges = 0;
    long long watch_register_vertices = 0;
    long long watch_unregister_vertices = 0;
    long long watch_live_entries_peak = 0;
    long long watch_stale_drops = 0;
    long long touched_class_total = 0;
    long long support_positive_component_total = 0;
    long long skip_by_single_positive_component = 0;
    long long skip_by_rep_bucket_unanimous = 0;
    long long split_required_class_count = 0;
    long long rep_bucket_checks = 0;
    long long moved_endpoint_enumerations = 0;
    long long moved_endpoint_total = 0;
    long long largest_bucket_kept_count = 0;
    long long class_local_fullscan_calls = 0;
    long long class_local_fullscan_endpoints = 0;
    long long fullscan_bad_meta = 0;
    long long fullscan_bad_xpos = 0;
    long long fullscan_bad_rep = 0;
    long long fullscan_bad_ctx = 0;
    long long support_meta_build_ok = 0;
    long long support_meta_fail_artifact_stamp = 0;
    long long support_meta_fail_fallback_stamp = 0;
    long long support_meta_fail_root = 0;
    long long support_meta_build_calls = 0;
    long long support_meta_build_watch_vertices = 0;
    long long support_meta_build_relevant_endpoints = 0;
    long long support_meta_hash_pos_build_items = 0;
    long long support_meta_graph_recover_calls = 0;
    long long support_meta_graph_recover_edges = 0;
    long long support_meta_endpoint_sort_calls = 0;
    long long support_meta_endpoint_sort_items = 0;
    long long support_meta_from_collector_calls = 0;
    long long support_meta_from_collector_watch_vertices = 0;
    long long support_meta_from_collector_relevant_endpoints = 0;
    long long support_full_rebuild_calls = 0;
    long long support_full_rebuild_watch_vertices = 0;
    long long support_reuse_single_calls = 0;
    long long support_reuse_single_watch_vertices_kept = 0;
    long long support_reuse_single_watch_vertices_removed = 0;
    long long support_reuse_unanimous_calls = 0;
    long long support_reuse_unanimous_components = 0;
    long long support_reuse_unanimous_reps = 0;
    long long support_reuse_unanimous_connector_calls = 0;
    long long support_reuse_unanimous_connector_vertices = 0;
    long long support_reuse_unanimous_watch_vertices_kept = 0;
    long long support_reuse_unanimous_watch_vertices_added = 0;
    long long support_reuse_unanimous_watch_vertices_removed = 0;
    long long support_merged_metadata_calls = 0;
    long long support_merged_metadata_vertices = 0;
    long long piece_shadow_skip_classes_total = 0;
    long long piece_shadow_single_positive_classes = 0;
    long long piece_shadow_unanimous_classes = 0;
    long long piece_shadow_split_classes = 0;
    long long piece_shadow_current_materialize_vertices = 0;
    long long piece_shadow_current_watch_unregister_vertices = 0;
    long long piece_shadow_current_watch_register_vertices = 0;
    long long piece_shadow_candidate_piece_reused_vertices = 0;
    long long piece_shadow_candidate_piece_removed_vertices = 0;
    long long piece_shadow_candidate_connector_vertices = 0;
    long long piece_shadow_candidate_boundary_ops = 0;
    long long piece_shadow_candidate_piece_count = 0;
    long long piece_shadow_candidate_parent_side_positive_cases = 0;
    long long piece_shadow_candidate_multi_piece_classes = 0;
    long long piece_shadow_estimated_saved_vertices = 0;
    long long piece_live_count = 0;
    long long piece_live_vertices = 0;
    long long piece_reuse_single_calls = 0;
    long long piece_reuse_single_reused_vertices = 0;
    long long piece_reuse_single_removed_vertices = 0;
    long long piece_reuse_unanimous_calls = 0;
    long long piece_reuse_unanimous_reused_vertices = 0;
    long long piece_reuse_unanimous_removed_vertices = 0;
    long long piece_reuse_unanimous_added_connector_vertices = 0;
    long long piece_materialize_fallback_calls = 0;
    long long piece_materialize_fallback_vertices = 0;
    long long piece_fallback_reason_need_support_meta = 0;
    long long piece_fallback_reason_bad_x_handle = 0;
    long long piece_fallback_reason_preserved_piece_hit = 0;
    long long piece_fallback_reason_connector_hit = 0;
    long long piece_fallback_reason_split_required = 0;
    long long piece_fallback_reason_other = 0;
    long long piece_native_candidate_classes = 0;
    long long piece_native_candidate_preserved_hits = 0;
    long long piece_native_candidate_connector_hits = 0;
    long long piece_native_single_calls = 0;
    long long piece_native_single_preserved_hits = 0;
    long long piece_native_single_connector_hits = 0;
    long long piece_native_single_reused_vertices = 0;
    long long piece_native_single_removed_vertices = 0;
    long long piece_native_single_boundary_ops = 0;
    long long piece_native_unanimous_calls = 0;
    long long piece_native_unanimous_preserved_hits = 0;
    long long piece_native_unanimous_connector_hits = 0;
    long long piece_native_unanimous_reused_vertices = 0;
    long long piece_native_unanimous_removed_vertices = 0;
    long long piece_native_unanimous_added_connector_vertices = 0;
    long long piece_native_unanimous_boundary_ops = 0;
    long long connector_shadow_unanimous_classes = 0;
    long long connector_shadow_current_removed_vertices = 0;
    long long connector_shadow_current_added_vertices = 0;
    long long connector_shadow_candidate_reused_connector_vertices = 0;
    long long connector_shadow_candidate_removed_connector_vertices = 0;
    long long connector_shadow_candidate_patch_vertices = 0;
    long long connector_shadow_candidate_attachment_retargets = 0;
    long long connector_shadow_candidate_terminal_fragment_groups = 0;
    long long connector_shadow_candidate_no_patch_needed = 0;
    long long connector_shadow_estimated_saved_vertices = 0;
    long long connector_delta_preserved_hit_calls = 0;
    long long connector_delta_preserved_hit_reused_connector_vertices = 0;
    long long connector_delta_preserved_hit_removed_connector_vertices = 0;
    long long connector_delta_preserved_hit_added_patch_vertices = 0;
    long long connector_delta_preserved_hit_attachment_retargets = 0;
    long long connector_delta_connector_hit_calls = 0;
    long long connector_delta_connector_hit_reused_vertices = 0;
    long long connector_delta_connector_hit_removed_vertices = 0;
    long long connector_delta_connector_hit_added_patch_vertices = 0;
    long long connector_delta_connector_hit_terminal_fragment_groups = 0;
    long long connector_delta_connector_hit_no_patch_needed = 0;
    long long connector_skeleton_shadow_classes = 0;
    long long connector_skeleton_shadow_current_removed_vertices = 0;
    long long connector_skeleton_shadow_current_added_vertices = 0;
    long long connector_skeleton_shadow_candidate_terminals = 0;
    long long connector_skeleton_shadow_candidate_vertices = 0;
    long long connector_skeleton_shadow_candidate_watch_unregister = 0;
    long long connector_skeleton_shadow_candidate_watch_register = 0;
    long long connector_skeleton_shadow_candidate_no_rebuild_needed = 0;
    long long connector_skeleton_shadow_estimated_saved_vertices = 0;
    long long connector_skeleton_old_vertices = 0;
    long long connector_skeleton_new_vertices = 0;
    long long connector_skeleton_common_vertices = 0;
    long long connector_skeleton_added_vertices = 0;
    long long connector_skeleton_removed_vertices = 0;
    long long connector_skeleton_intersection_ratio_permille = 0;
    long long connector_watch_full_unregister = 0;
    long long connector_watch_full_register = 0;
    long long connector_watch_diff_unregister = 0;
    long long connector_watch_diff_register = 0;
    long long connector_watch_diff_reused = 0;
    long long connector_watch_diff_actual_calls = 0;
    long long connector_watch_diff_actual_reused = 0;
    long long connector_watch_diff_actual_removed = 0;
    long long connector_watch_diff_actual_added = 0;
    long long connector_skeleton_actual_calls = 0;
    long long connector_skeleton_actual_terminals = 0;
    long long connector_skeleton_actual_vertices = 0;
    long long connector_skeleton_actual_removed_old_connector_vertices = 0;
    long long connector_skeleton_actual_retargets = 0;
    long long connector_skeleton_build_calls = 0;
    long long connector_skeleton_terminals = 0;
    long long connector_skeleton_vertices = 0;
    long long connector_skeleton_watch_unregister = 0;
    long long connector_skeleton_watch_register = 0;
    long long preserved_piece_split_calls = 0;
    long long preserved_piece_split_vertices = 0;
    long long preserved_piece_split_boundary_ops = 0;
    long long time_route_dispatch_ns = 0;
    long long time_route_dispatch_calls = 0;
    long long time_global_delete_dfs_ns = 0;
    long long time_global_delete_dfs_calls = 0;
    long long time_connector_skeleton_build_ns = 0;
    long long time_connector_skeleton_build_calls = 0;
    long long time_connector_skeleton_watch_unregister_ns = 0;
    long long time_connector_skeleton_watch_unregister_calls = 0;
    long long time_connector_skeleton_watch_register_ns = 0;
    long long time_connector_skeleton_watch_register_calls = 0;
    long long time_preserved_piece_split_ns = 0;
    long long time_preserved_piece_split_calls = 0;
    long long time_query_incident_scan_ns = 0;
    long long time_query_incident_scan_calls = 0;
    long long time_unanimous_mode_dispatch_ns = 0;
    long long time_unanimous_mode_dispatch_calls = 0;
    long long time_terminal_collection_ns = 0;
    long long time_terminal_collection_calls = 0;
    long long time_vertex_lookup_ns = 0;
    long long time_vertex_lookup_calls = 0;
    long long time_watch_diff_build_ns = 0;
    long long time_watch_diff_build_calls = 0;
    long long time_state_publish_ns = 0;
    long long time_state_publish_calls = 0;
    long long connector_skeleton_terminal_collection_calls = 0;
    long long time_connector_skeleton_terminal_collection_ns = 0;
    long long connector_skeleton_terminal_dedupe_calls = 0;
    long long time_connector_skeleton_terminal_dedupe_ns = 0;
    long long connector_skeleton_vertexset_build_calls = 0;
    long long time_connector_skeleton_vertexset_build_ns = 0;
    long long connector_skeleton_vertex_lookup_build_calls = 0;
    long long connector_skeleton_vertex_lookup_build_vertices = 0;
    long long time_connector_skeleton_vertex_lookup_build_ns = 0;
    long long time_connector_skeleton_core_build_ns = 0;
    long long time_connector_skeleton_core_build_calls = 0;
    long long connector_skeleton_candidate_classes = 0;
    long long connector_skeleton_selected_classes = 0;
    long long connector_skeleton_selected_connector_only = 0;
    long long connector_skeleton_selected_both_on = 0;
    long long connector_skeleton_forced_classes = 0;
    long long connector_skeleton_reject_state_not_unanimous = 0;
    long long connector_skeleton_reject_no_preserved_pieces = 0;
    long long connector_skeleton_reject_no_attachment_vertices = 0;
    long long connector_skeleton_reject_support_meta_valid = 0;
    long long connector_skeleton_reject_origin_kind = 0;
    long long connector_skeleton_reject_missing_tree = 0;
    long long connector_skeleton_reject_fallback_guard = 0;
    long long connector_skeleton_reject_other = 0;
    long long unanimous_baseline_path_calls = 0;
    long long unanimous_baseline_path_vertices = 0;
    long long debug_force_skeleton_calls = 0;
    long long debug_force_skeleton_reference_compare_calls = 0;
    long long debug_force_skeleton_divergence = 0;
    long long debug_unanimous_state_old_field_read = 0;
    long long debug_unanimous_state_new_field_read = 0;
    long long debug_watch_leak_on_deleted_vertex = 0;
    long long debug_watch_leak_old_only = 0;
    long long debug_watch_leak_new_only = 0;
    long long debug_watch_leak_foreign = 0;
    long long debug_watch_leak_both_snapshot = 0;
    long long debug_watch_leak_origin_materialized = 0;
    long long debug_watch_leak_origin_preserved_piece = 0;
    long long debug_watch_leak_origin_connector_skeleton = 0;
    long long debug_watch_leak_origin_patch_tree = 0;
    long long debug_both_snapshot_piece_contains_x = 0;
    long long debug_both_snapshot_piece_excludes_x_but_watch_contains_x = 0;
    long long debug_both_snapshot_attachment_is_x = 0;
    long long debug_both_snapshot_attachment_outside_piece = 0;
    long long debug_both_snapshot_reused_same_piece_handle = 0;
    long long debug_both_snapshot_other = 0;
    long long debug_postcondition_piece_contains_x = 0;
    long long debug_postcondition_attachment_is_x = 0;
    long long debug_postcondition_watch_points_to_x = 0;
    long long debug_forced_preserved_split_due_x_in_piece = 0;
    long long debug_reused_piece_forbidden = 0;
    long long debug_targeted_piece_watch_refresh_calls = 0;
    long long debug_targeted_piece_watch_refresh_removed = 0;
    long long debug_targeted_piece_watch_refresh_added = 0;
    long long debug_attachment_retarget_due_x = 0;
    long long debug_attachment_retarget_due_outside_piece = 0;
    long long debug_replay_case_saved = 0;
    long long debug_replay_target_hit = 0;
    long long debug_skeleton_builder_attempted_deleted_vertex = 0;
    long long debug_skeleton_builder_skipped_deleted_vertex = 0;
    long long debug_watch_double_owned = 0;
    long long debug_watch_owner_mismatch = 0;
    long long debug_watch_stale_handle_in_state = 0;
    long long debug_progress_checkpoint_calls = 0;
    long long debug_progress_last_deletion = 0;
    long long debug_profile_total_deletions = 0;
    long long debug_profile_sampled_deletions = 0;
    long long time_dispatch_reuse_apply_piece_native_ns = 0;
    long long time_dispatch_reuse_apply_piece_native_calls = 0;
    long long time_dispatch_reuse_apply_rep_unanimous_ns = 0;
    long long time_dispatch_reuse_apply_rep_unanimous_calls = 0;
    long long time_dispatch_publish_preserved_annotate_ns = 0;
    long long time_dispatch_publish_preserved_annotate_calls = 0;
    long long time_dispatch_publish_connector_annotate_ns = 0;
    long long time_dispatch_publish_connector_annotate_calls = 0;
    long long time_dispatch_publish_watch_id_rebuild_ns = 0;
    long long time_dispatch_publish_watch_id_rebuild_calls = 0;
    long long time_dispatch_publish_canonical_rebuild_ns = 0;
    long long time_dispatch_publish_canonical_rebuild_calls = 0;
    long long time_dispatch_publish_posmap_build_ns = 0;
    long long time_dispatch_publish_posmap_build_calls = 0;
    long long dispatch_candidate_cids = 0;
    long long dispatch_publish_preserved_handles = 0;
    long long dispatch_publish_connector_handles = 0;
    long long dispatch_publish_preserved_pieces_visited = 0;
    long long dispatch_publish_connector_pieces_visited = 0;
    long long dispatch_publish_watch_id_rebuild_calls = 0;
    long long dispatch_publish_watch_id_rebuild_handles = 0;
    long long dispatch_publish_canonical_rebuild_calls = 0;
    long long dispatch_publish_canonical_vertices = 0;
    long long dispatch_publish_posmap_build_calls = 0;
    long long dispatch_publish_posmap_build_vertices = 0;
    long long dispatch_publish_noop_calls = 0;
    long long dispatch_publish_full_rescan_calls = 0;
    long long time_reuse_route_baseline_ns = 0;
    long long time_reuse_route_delta_preserved_then_skeleton_ns = 0;
    long long time_reuse_route_connector_skeleton_ns = 0;
    long long time_reuse_route_general_delta_ns = 0;
    long long reuse_route_baseline_calls = 0;
    long long reuse_route_delta_preserved_then_skeleton_calls = 0;
    long long reuse_route_connector_skeleton_calls = 0;
    long long reuse_route_general_delta_calls = 0;
    long long time_reuse_old_attachment_map_build_ns = 0;
    long long time_reuse_old_attachment_map_build_calls = 0;
    long long time_reuse_piece_split_apply_ns = 0;
    long long time_reuse_piece_split_apply_calls = 0;
    long long time_reuse_connector_split_apply_ns = 0;
    long long time_reuse_connector_split_apply_calls = 0;
    long long time_reuse_keepmask_scan_ns = 0;
    long long time_reuse_keepmask_scan_calls = 0;
    long long time_reuse_watch_retain_ns = 0;
    long long time_reuse_watch_retain_calls = 0;
    long long time_reuse_preserved_direct_retag_ns = 0;
    long long time_reuse_preserved_direct_retag_calls = 0;
    long long time_reuse_connector_direct_retag_ns = 0;
    long long time_reuse_connector_direct_retag_calls = 0;
    long long time_reuse_attachment_fixup_ns = 0;
    long long time_reuse_attachment_fixup_calls = 0;
    long long time_reuse_patch_vertex_collect_ns = 0;
    long long time_reuse_patch_vertex_collect_calls = 0;
    long long time_reuse_patch_tree_build_ns = 0;
    long long time_reuse_patch_tree_build_calls = 0;
    long long time_reuse_prepublish_preserved_annotate_ns = 0;
    long long time_reuse_prepublish_preserved_annotate_calls = 0;
    long long time_reuse_prepublish_connector_annotate_ns = 0;
    long long time_reuse_prepublish_connector_annotate_calls = 0;
    long long time_reuse_final_publish_commit_ns = 0;
    long long time_reuse_final_publish_commit_calls = 0;
    long long reuse_old_piece_hits = 0;
    long long reuse_old_connector_hits = 0;
    long long reuse_replacement_pieces = 0;
    long long reuse_keepmask_removed_handles = 0;
    long long reuse_keepmask_removed_preserved_handles = 0;
    long long reuse_keepmask_removed_connector_handles = 0;
    long long reuse_preserved_direct_retag_handles = 0;
    long long reuse_connector_direct_retag_handles = 0;
    long long reuse_attachment_retargets = 0;
    long long reuse_patch_vertices = 0;
    long long reuse_patch_tree_build_calls = 0;
    long long reuse_patch_handles_added = 0;
    long long reuse_prepublish_preserved_annotate_calls = 0;
    long long reuse_prepublish_preserved_handles = 0;
    long long reuse_prepublish_connector_annotate_calls = 0;
    long long reuse_prepublish_connector_handles = 0;
    long long reuse_full_connector_watch_id_rebuild_calls = 0;
    long long reuse_incremental_connector_watch_id_update_calls = 0;
    long long reuse_final_publish_calls = 0;
    long long reuse_final_publish_noop_calls = 0;
    long long reuse_final_publish_skipped_calls = 0;
    long long reuse_watch_handle_full_scan_calls = 0;
    long long reuse_watch_handle_full_scan_handles = 0;
    long long reuse_duplicate_preserved_annotate_passes = 0;
    long long reuse_duplicate_connector_watch_id_rebuild_passes = 0;
    long long reuse_state_commit_identical_calls = 0;
    long long time_psplit_old_attachment_index_build_ns = 0;
    long long time_psplit_old_attachment_index_build_calls = 0;
    long long time_psplit_old_piece_scan_ns = 0;
    long long time_psplit_old_piece_scan_calls = 0;
    long long time_psplit_contains_x_check_ns = 0;
    long long time_psplit_contains_x_check_calls = 0;
    long long time_psplit_x_local_pos_lookup_ns = 0;
    long long time_psplit_x_local_pos_lookup_calls = 0;
    long long time_psplit_tree_posmap_build_ns = 0;
    long long time_psplit_tree_posmap_build_calls = 0;
    long long time_psplit_split_piece_core_ns = 0;
    long long time_psplit_split_piece_core_calls = 0;
    long long time_psplit_replacement_attachment_validate_ns = 0;
    long long time_psplit_replacement_attachment_validate_calls = 0;
    long long time_psplit_replacement_attachment_retarget_ns = 0;
    long long time_psplit_replacement_attachment_retarget_calls = 0;
    long long time_psplit_new_piece_emit_ns = 0;
    long long time_psplit_new_piece_emit_calls = 0;
    long long time_psplit_attachment_fixup_validate_ns = 0;
    long long time_psplit_attachment_fixup_validate_calls = 0;
    long long time_psplit_attachment_fixup_retarget_ns = 0;
    long long time_psplit_attachment_fixup_retarget_calls = 0;
    long long time_psplit_connector_path_attachment_normalize_ns = 0;
    long long time_psplit_connector_path_attachment_normalize_calls = 0;
    long long psplit_old_attachment_map_entries = 0;
    long long psplit_preserved_pieces_scanned = 0;
    long long psplit_contains_x_checks = 0;
    long long psplit_contains_x_hits = 0;
    long long psplit_x_local_pos_lookup_calls = 0;
    long long psplit_x_local_pos_lookup_direct_hits = 0;
    long long psplit_x_local_pos_lookup_posmap_lookups = 0;
    long long psplit_tree_posmap_build_calls = 0;
    long long psplit_tree_posmap_cache_hits = 0;
    long long psplit_split_piece_calls = 0;
    long long psplit_replacement_piece_count = 0;
    long long psplit_attachment_validate_calls = 0;
    long long psplit_attachment_validate_hits = 0;
    long long psplit_attachment_retarget_calls = 0;
    long long psplit_attachment_retarget_changes = 0;
    long long psplit_new_piece_emit_count = 0;
    long long psplit_attachment_fixup_calls = 0;
    long long psplit_attachment_fixup_changes = 0;
    long long psplit_connector_path_attachment_normalize_calls = 0;
    long long psplit_old_attachment_fastpath_reuse_calls = 0;
    long long psplit_validate_then_fixup_duplicate_checks = 0;
    long long psplit_same_tree_posmap_rebuilds = 0;
    long long time_wscan_preserved_keepstamp_build_ns = 0;
    long long time_wscan_preserved_keepstamp_build_calls = 0;
    long long time_wscan_preserved_keepmask_decision_ns = 0;
    long long time_wscan_preserved_keepmask_decision_calls = 0;
    long long time_wscan_preserved_stamp_mark_ns = 0;
    long long time_wscan_preserved_stamp_mark_calls = 0;
    long long time_wscan_connector_desired_set_build_ns = 0;
    long long time_wscan_connector_desired_set_build_calls = 0;
    long long time_wscan_connector_keepmask_decision_ns = 0;
    long long time_wscan_connector_keepmask_decision_calls = 0;
    long long time_wscan_connector_existing_set_build_ns = 0;
    long long time_wscan_connector_existing_set_build_calls = 0;
    long long time_wscan_connector_addverts_diff_ns = 0;
    long long time_wscan_connector_addverts_diff_calls = 0;
    long long time_wscan_retain_remove_entries_ns = 0;
    long long time_wscan_retain_remove_entries_calls = 0;
    long long time_wscan_retain_compact_handles_ns = 0;
    long long time_wscan_retain_compact_handles_calls = 0;
    long long time_wscan_retain_slotpos_fixup_ns = 0;
    long long time_wscan_retain_slotpos_fixup_calls = 0;
    long long time_wscan_retain_handleidx_fixup_ns = 0;
    long long time_wscan_retain_handleidx_fixup_calls = 0;
    long long time_wscan_retain_owner_lookup_ns = 0;
    long long time_wscan_retain_owner_lookup_calls = 0;
    long long time_wscan_route_baseline_ns = 0;
    long long time_wscan_route_delta_preserved_then_skeleton_ns = 0;
    long long time_wscan_route_connector_skeleton_ns = 0;
    long long time_wscan_route_general_delta_ns = 0;
    long long wscan_preserved_keepmask_scans = 0;
    long long wscan_connector_keepmask_scans = 0;
    long long wscan_handles_scanned_preserved_keepmask = 0;
    long long wscan_handles_scanned_connector_keepmask = 0;
    long long wscan_handles_scanned_preserved_stamp_mark = 0;
    long long wscan_handles_scanned_existing_connector_set = 0;
    long long wscan_preserved_keepstamp_vertices_marked = 0;
    long long wscan_preserved_stamp_vertices_marked = 0;
    long long wscan_desired_connector_vertices = 0;
    long long wscan_existing_connector_vertices = 0;
    long long wscan_addverts_candidates = 0;
    long long wscan_addverts_selected = 0;
    long long wscan_retain_removed_handles = 0;
    long long wscan_retain_noop_calls = 0;
    long long wscan_retain_slotpos_fixups = 0;
    long long wscan_retain_handleidx_fixups = 0;
    long long wscan_retain_owner_state_lookups = 0;
    long long wscan_route_baseline_calls = 0;
    long long wscan_route_delta_preserved_then_skeleton_calls = 0;
    long long wscan_route_connector_skeleton_calls = 0;
    long long wscan_route_general_delta_calls = 0;
    long long wscan_duplicate_full_scan_passes = 0;
    long long wscan_duplicate_full_scan_handles = 0;
    long long wscan_used_connectorWatchEntryIds_fastpath_calls = 0;
    long long wscan_used_preservedHandleIdxs_fastpath_calls = 0;
    long long time_retain_remove_bitmap_build_ns = 0;
    long long time_retain_remove_bitmap_build_calls = 0;
    long long time_retain_sparse_remove_list_build_ns = 0;
    long long time_retain_sparse_remove_list_build_calls = 0;
    long long time_retain_watchByVertex_pop_ns = 0;
    long long time_retain_watchByVertex_pop_calls = 0;
    long long time_retain_moved_entry_owner_lookup_ns = 0;
    long long time_retain_moved_entry_owner_lookup_calls = 0;
    long long time_retain_moved_entry_same_owner_fastpath_ns = 0;
    long long time_retain_moved_entry_same_owner_fastpath_calls = 0;
    long long time_retain_moved_entry_slotpos_patch_ns = 0;
    long long time_retain_moved_entry_slotpos_patch_calls = 0;
    long long time_retain_kept_vector_build_ns = 0;
    long long time_retain_kept_vector_build_calls = 0;
    long long time_retain_kept_handle_copy_ns = 0;
    long long time_retain_kept_handle_copy_calls = 0;
    long long time_retain_kept_handleidx_patch_ns = 0;
    long long time_retain_kept_handleidx_patch_calls = 0;
    long long time_retain_final_swap_state_update_ns = 0;
    long long time_retain_final_swap_state_update_calls = 0;
    long long retain_calls = 0;
    long long retain_watch_handles_before = 0;
    long long retain_watch_handles_after = 0;
    long long retain_removed_handles = 0;
    long long retain_removed_connector_handles = 0;
    long long retain_removed_preserved_handles = 0;
    long long retain_removed_sparse_calls = 0;
    long long retain_removed_sparse_entries = 0;
    long long retain_removed_dense_calls = 0;
    long long retain_remove_bitmap_entries = 0;
    long long retain_watchByVertex_pop_calls = 0;
    long long retain_moved_entry_count = 0;
    long long retain_moved_entry_same_owner_fastpath_hits = 0;
    long long retain_owner_lookup_calls = 0;
    long long retain_owner_lookup_hits = 0;
    long long retain_owner_lookup_misses = 0;
    long long retain_slotpos_fixups = 0;
    long long retain_kept_handles_copied = 0;
    long long retain_handleidx_fixups = 0;
    long long retain_final_swap_calls = 0;
    long long retain_noop_calls = 0;
    long long retain_remove_ratio_ppm_sum = 0;
    long long retain_sparse_remove_fastpath_calls = 0;
    long long retain_sparse_remove_fastpath_removed_entries = 0;
    long long retain_skip_handleidx_patch_calls = 0;
    long long retain_skip_slotpos_patch_calls = 0;
    long long time_kvec_prefix_fastpath_check_ns = 0;
    long long time_kvec_prefix_fastpath_check_calls = 0;
    long long time_kvec_suffix_fastpath_check_ns = 0;
    long long time_kvec_suffix_fastpath_check_calls = 0;
    long long time_kvec_kept_count_scan_ns = 0;
    long long time_kvec_kept_count_scan_calls = 0;
    long long time_kvec_scratch_prepare_ns = 0;
    long long time_kvec_scratch_prepare_calls = 0;
    long long time_kvec_stable_emit_unchanged_prefix_ns = 0;
    long long time_kvec_stable_emit_unchanged_prefix_calls = 0;
    long long time_kvec_stable_emit_moved_suffix_ns = 0;
    long long time_kvec_stable_emit_moved_suffix_calls = 0;
    long long time_kvec_patchlist_build_ns = 0;
    long long time_kvec_patchlist_build_calls = 0;
    long long time_kvec_handleidx_patch_changed_only_ns = 0;
    long long time_kvec_handleidx_patch_changed_only_calls = 0;
    long long time_kvec_handleidx_patch_skip_same_index_ns = 0;
    long long time_kvec_handleidx_patch_skip_same_index_calls = 0;
    long long time_kvec_final_resize_or_swap_ns = 0;
    long long time_kvec_final_resize_or_swap_calls = 0;
    long long kvec_calls = 0;
    long long kvec_watch_handles_before = 0;
    long long kvec_watch_handles_after = 0;
    long long kvec_removed_handles = 0;
    long long kvec_first_removed_index_sum = 0;
    long long kvec_last_removed_suffix_len_sum = 0;
    long long kvec_unchanged_prefix_handles = 0;
    long long kvec_unchanged_suffix_handles = 0;
    long long kvec_moved_suffix_handles = 0;
    long long kvec_changed_patchlist_entries = 0;
    long long kvec_handle_copy_entries = 0;
    long long kvec_handleidx_patch_changed_entries = 0;
    long long kvec_handleidx_patch_skipped_same_index_entries = 0;
    long long kvec_inplace_compact_calls = 0;
    long long kvec_scratch_vector_build_calls = 0;
    long long kvec_capacity_reuse_calls = 0;
    long long kvec_suffix_resize_fastpath_calls = 0;
    long long kvec_noop_calls = 0;
    long long kvec_removed_ratio_ppm_sum = 0;
    long long kvec_prefix_fastpath_hits = 0;
    long long kvec_suffix_fastpath_hits = 0;
    long long kvec_inplace_write_same_slot_hits = 0;
    long long kvec_swap_skipped_calls = 0;
    long long time_scomp_first_removed_seek_ns = 0;
    long long time_scomp_first_removed_seek_calls = 0;
    long long time_scomp_suffix_only_check_ns = 0;
    long long time_scomp_suffix_only_check_calls = 0;
    long long time_scomp_kept_count_scan_ns = 0;
    long long time_scomp_kept_count_scan_calls = 0;
    long long time_scomp_kept_run_partition_build_ns = 0;
    long long time_scomp_kept_run_partition_build_calls = 0;
    long long time_scomp_prefix_skip_ns = 0;
    long long time_scomp_prefix_skip_calls = 0;
    long long time_scomp_contiguous_run_block_copy_ns = 0;
    long long time_scomp_contiguous_run_block_copy_calls = 0;
    long long time_scomp_elementwise_emit_ns = 0;
    long long time_scomp_elementwise_emit_calls = 0;
    long long time_scomp_scratch_prepare_ns = 0;
    long long time_scomp_scratch_prepare_calls = 0;
    long long time_scomp_tail_cleanup_ns = 0;
    long long time_scomp_tail_cleanup_calls = 0;
    long long time_scomp_final_resize_swap_ns = 0;
    long long time_scomp_final_resize_swap_calls = 0;
    long long scomp_calls = 0;
    long long scomp_watch_handles_before = 0;
    long long scomp_watch_handles_after = 0;
    long long scomp_removed_handles = 0;
    long long scomp_first_removed_index_sum = 0;
    long long scomp_suffix_only_calls = 0;
    long long scomp_single_middle_run_calls = 0;
    long long scomp_removed_run_count_sum = 0;
    long long scomp_kept_run_count_sum = 0;
    long long scomp_prefix_skipped_handles = 0;
    long long scomp_suffix_skipped_handles = 0;
    long long scomp_block_copy_runs = 0;
    long long scomp_block_copied_handles = 0;
    long long scomp_elementwise_emitted_handles = 0;
    long long scomp_scratch_prepare_calls = 0;
    long long scomp_scratch_capacity_reuse_calls = 0;
    long long scomp_tail_cleared_handles = 0;
    long long scomp_final_resize_calls = 0;
    long long scomp_noop_calls = 0;
    long long scomp_removed_ratio_ppm_sum = 0;
    long long scomp_contiguous_middle_memmove_calls = 0;
    long long scomp_prefix_skip_hits = 0;
    long long scomp_suffix_resize_hits = 0;
    long long scomp_inplace_suffix_shift_calls = 0;
    long long scomp_swap_skipped_calls = 0;
    long long time_bcopy_single_middle_run_detect_ns = 0;
    long long time_bcopy_single_middle_run_detect_calls = 0;
    long long time_bcopy_run_coalesce_build_ns = 0;
    long long time_bcopy_run_coalesce_build_calls = 0;
    long long time_bcopy_direct_suffix_memmove_ns = 0;
    long long time_bcopy_direct_suffix_memmove_calls = 0;
    long long time_bcopy_multi_run_block_copy_ns = 0;
    long long time_bcopy_multi_run_block_copy_calls = 0;
    long long time_bcopy_short_fragment_elementwise_fallback_ns = 0;
    long long time_bcopy_short_fragment_elementwise_fallback_calls = 0;
    long long time_bcopy_overlap_safe_staging_ns = 0;
    long long time_bcopy_overlap_safe_staging_calls = 0;
    long long time_bcopy_route_baseline_ns = 0;
    long long time_bcopy_route_delta_preserved_then_skeleton_ns = 0;
    long long time_bcopy_route_connector_skeleton_ns = 0;
    long long time_bcopy_route_general_delta_ns = 0;
    long long bcopy_calls = 0;
    long long bcopy_watch_handles_before = 0;
    long long bcopy_watch_handles_after = 0;
    long long bcopy_removed_handles = 0;
    long long bcopy_single_middle_run_calls = 0;
    long long bcopy_suffix_only_calls = 0;
    long long bcopy_removed_run_count_sum = 0;
    long long bcopy_kept_run_count_sum = 0;
    long long bcopy_copy_plan_entries = 0;
    long long bcopy_coalesced_run_merges = 0;
    long long bcopy_direct_memmove_calls = 0;
    long long bcopy_direct_memmoved_handles = 0;
    long long bcopy_runwise_block_copy_calls = 0;
    long long bcopy_runwise_block_copied_handles = 0;
    long long bcopy_elementwise_fallback_calls = 0;
    long long bcopy_elementwise_fallback_handles = 0;
    long long bcopy_overlap_staging_calls = 0;
    long long bcopy_overlap_staged_handles = 0;
    long long bcopy_same_slot_skip_handles = 0;
    long long bcopy_prefix_skipped_handles = 0;
    long long bcopy_suffix_skipped_handles = 0;
    long long bcopy_route_baseline_calls = 0;
    long long bcopy_route_delta_preserved_then_skeleton_calls = 0;
    long long bcopy_route_connector_skeleton_calls = 0;
    long long bcopy_route_general_delta_calls = 0;
    long long bcopy_block_copy_threshold_hits = 0;
    long long bcopy_contiguous_middle_memmove_calls = 0;
    long long bcopy_contiguous_middle_memmove_handles = 0;
    long long bcopy_adjacent_run_coalesce_hits = 0;
    long long bcopy_scratchless_overlap_safe_calls = 0;
    long long time_plan_first_removed_seek_ns = 0;
    long long time_plan_first_removed_seek_calls = 0;
    long long time_plan_removed_run_discovery_ns = 0;
    long long time_plan_removed_run_discovery_calls = 0;
    long long time_plan_kept_run_discovery_ns = 0;
    long long time_plan_kept_run_discovery_calls = 0;
    long long time_plan_adjacent_run_coalesce_ns = 0;
    long long time_plan_adjacent_run_coalesce_calls = 0;
    long long time_plan_single_middle_shortcircuit_eligibility_ns = 0;
    long long time_plan_single_middle_shortcircuit_eligibility_calls = 0;
    long long time_plan_dst_index_accumulate_ns = 0;
    long long time_plan_dst_index_accumulate_calls = 0;
    long long time_plan_descriptor_emit_ns = 0;
    long long time_plan_descriptor_emit_calls = 0;
    long long time_plan_small_inline_buffer_prepare_ns = 0;
    long long time_plan_small_inline_buffer_prepare_calls = 0;
    long long time_plan_route_baseline_ns = 0;
    long long time_plan_route_delta_preserved_then_skeleton_ns = 0;
    long long time_plan_route_connector_skeleton_ns = 0;
    long long time_plan_route_general_delta_ns = 0;
    long long plan_calls = 0;
    long long plan_watch_handles_before = 0;
    long long plan_watch_handles_after = 0;
    long long plan_removed_handles = 0;
    long long plan_first_removed_index_sum = 0;
    long long plan_removed_run_count_sum = 0;
    long long plan_kept_run_count_sum = 0;
    long long plan_adjacent_merge_hits = 0;
    long long plan_descriptor_count = 0;
    long long plan_dst_index_updates = 0;
    long long plan_single_middle_shortcircuit_hits = 0;
    long long plan_suffix_only_shortcircuit_hits = 0;
    long long plan_small_inline_hits = 0;
    long long plan_small_inline_capacity_reuse_hits = 0;
    long long plan_heap_plan_build_calls = 0;
    long long plan_route_baseline_calls = 0;
    long long plan_route_delta_preserved_then_skeleton_calls = 0;
    long long plan_route_connector_skeleton_calls = 0;
    long long plan_route_general_delta_calls = 0;
    long long plan_removed_ratio_ppm_sum = 0;
    long long plan_copy_plan_rebuild_calls = 0;
    long long plan_copy_plan_skipped_calls = 0;
    long long plan_prefix_suffix_boundary_reuse_hits = 0;
    long long plan_descriptor_emit_skipped_for_direct_shift_calls = 0;
    long long time_rdisc_first_removed_seek_ns = 0;
    long long time_rdisc_first_removed_seek_calls = 0;
    long long time_rdisc_boundary_reuse_check_ns = 0;
    long long time_rdisc_boundary_reuse_check_calls = 0;
    long long time_rdisc_removed_run_scan_ns = 0;
    long long time_rdisc_removed_run_scan_calls = 0;
    long long time_rdisc_kept_run_scan_ns = 0;
    long long time_rdisc_kept_run_scan_calls = 0;
    long long time_rdisc_suffix_only_shortcircuit_ns = 0;
    long long time_rdisc_suffix_only_shortcircuit_calls = 0;
    long long time_rdisc_single_middle_shortcircuit_ns = 0;
    long long time_rdisc_single_middle_shortcircuit_calls = 0;
    long long time_rdisc_fused_onepass_scan_ns = 0;
    long long time_rdisc_fused_onepass_scan_calls = 0;
    long long time_rdisc_small_runlist_inline_materialize_ns = 0;
    long long time_rdisc_small_runlist_inline_materialize_calls = 0;
    long long time_rdisc_route_baseline_ns = 0;
    long long time_rdisc_route_delta_preserved_then_skeleton_ns = 0;
    long long time_rdisc_route_connector_skeleton_ns = 0;
    long long time_rdisc_route_general_delta_ns = 0;
    long long rdisc_calls = 0;
    long long rdisc_watch_handles_before = 0;
    long long rdisc_watch_handles_after = 0;
    long long rdisc_removed_handles = 0;
    long long rdisc_first_removed_index_sum = 0;
    long long rdisc_removed_run_count_sum = 0;
    long long rdisc_kept_run_count_sum = 0;
    long long rdisc_boundary_reuse_hits = 0;
    long long rdisc_suffix_only_hits = 0;
    long long rdisc_single_middle_hits = 0;
    long long rdisc_two_pass_removed_scan_calls = 0;
    long long rdisc_two_pass_kept_scan_calls = 0;
    long long rdisc_fused_onepass_calls = 0;
    long long rdisc_removed_scan_steps = 0;
    long long rdisc_kept_scan_steps = 0;
    long long rdisc_fused_scan_steps = 0;
    long long rdisc_small_runlist_inline_hits = 0;
    long long rdisc_heap_runlist_build_calls = 0;
    long long rdisc_route_baseline_calls = 0;
    long long rdisc_route_delta_preserved_then_skeleton_calls = 0;
    long long rdisc_route_connector_skeleton_calls = 0;
    long long rdisc_route_general_delta_calls = 0;
    long long rdisc_removed_ratio_ppm_sum = 0;
    long long rdisc_prefix_suffix_boundary_reuse_hits = 0;
    long long rdisc_shortcircuit_skipped_kept_scan_calls = 0;
    long long rdisc_shortcircuit_skipped_removed_scan_calls = 0;
    long long rdisc_runlist_materialize_skipped_calls = 0;
    long long time_fclass_suffix_only_gate_ns = 0;
    long long time_fclass_suffix_only_gate_calls = 0;
    long long time_fclass_single_middle_gate_ns = 0;
    long long time_fclass_single_middle_gate_calls = 0;
    long long time_fclass_onepass_transition_scan_ns = 0;
    long long time_fclass_onepass_transition_scan_calls = 0;
    long long time_fclass_transition_emit_runs_ns = 0;
    long long time_fclass_transition_emit_runs_calls = 0;
    long long time_fclass_run_count_finalize_ns = 0;
    long long time_fclass_run_count_finalize_calls = 0;
    long long time_fclass_small_runlist_inline_ns = 0;
    long long time_fclass_small_runlist_inline_calls = 0;
    long long time_fclass_route_baseline_ns = 0;
    long long time_fclass_route_delta_preserved_then_skeleton_ns = 0;
    long long time_fclass_route_connector_skeleton_ns = 0;
    long long time_fclass_route_general_delta_ns = 0;
    long long fclass_calls = 0;
    long long fclass_watch_handles_before = 0;
    long long fclass_watch_handles_after = 0;
    long long fclass_removed_handles = 0;
    long long fclass_suffix_only_hits = 0;
    long long fclass_single_middle_hits = 0;
    long long fclass_fused_onepass_calls = 0;
    long long fclass_transition_steps = 0;
    long long fclass_removed_to_kept_transitions = 0;
    long long fclass_kept_to_removed_transitions = 0;
    long long fclass_run_count_finalize_calls = 0;
    long long fclass_small_inline_hits = 0;
    long long fclass_small_inline_capacity_reuse_hits = 0;
    long long fclass_heap_runlist_build_calls = 0;
    long long fclass_route_baseline_calls = 0;
    long long fclass_route_delta_preserved_then_skeleton_calls = 0;
    long long fclass_route_connector_skeleton_calls = 0;
    long long fclass_route_general_delta_calls = 0;
    long long fclass_shortcircuit_skipped_emit_calls = 0;
    long long fclass_shortcircuit_skipped_scan_steps = 0;
    long long fclass_prefix_suffix_boundary_reuse_hits = 0;
    long long fclass_run_finalize_skipped_calls = 0;
    long long time_tscan_window_seed_ns = 0;
    long long time_tscan_window_seed_calls = 0;
    long long time_tscan_boundary_clip_ns = 0;
    long long time_tscan_boundary_clip_calls = 0;
    long long time_tscan_state_load_compare_ns = 0;
    long long time_tscan_state_load_compare_calls = 0;
    long long time_tscan_removed_to_kept_detect_ns = 0;
    long long time_tscan_removed_to_kept_detect_calls = 0;
    long long time_tscan_kept_to_removed_detect_ns = 0;
    long long time_tscan_kept_to_removed_detect_calls = 0;
    long long time_tscan_run_boundary_commit_ns = 0;
    long long time_tscan_run_boundary_commit_calls = 0;
    long long time_tscan_tail_stop_check_ns = 0;
    long long time_tscan_tail_stop_check_calls = 0;
    long long time_tscan_early_exit_finalize_ns = 0;
    long long time_tscan_early_exit_finalize_calls = 0;
    long long time_tscan_route_baseline_ns = 0;
    long long time_tscan_route_delta_preserved_then_skeleton_ns = 0;
    long long time_tscan_route_connector_skeleton_ns = 0;
    long long time_tscan_route_general_delta_ns = 0;
    long long tscan_calls = 0;
    long long tscan_watch_handles_before = 0;
    long long tscan_watch_handles_after = 0;
    long long tscan_removed_handles = 0;
    long long tscan_first_removed_index_sum = 0;
    long long tscan_scan_window_handles = 0;
    long long tscan_boundary_seed_hits = 0;
    long long tscan_boundary_clip_hits = 0;
    long long tscan_state_steps = 0;
    long long tscan_removed_to_kept_transitions = 0;
    long long tscan_kept_to_removed_transitions = 0;
    long long tscan_run_boundary_commit_calls = 0;
    long long tscan_tail_stop_hits = 0;
    long long tscan_early_exit_calls = 0;
    long long tscan_suffix_only_after_scan_hits = 0;
    long long tscan_single_middle_after_scan_hits = 0;
    long long tscan_route_baseline_calls = 0;
    long long tscan_route_delta_preserved_then_skeleton_calls = 0;
    long long tscan_route_connector_skeleton_calls = 0;
    long long tscan_route_general_delta_calls = 0;
    long long tscan_removed_ratio_ppm_sum = 0;
    long long tscan_scan_window_saved_handles = 0;
    long long tscan_transition_branch_taken_count = 0;
    long long tscan_tail_finalize_skipped_calls = 0;
    long long tscan_prefix_boundary_reuse_hits = 0;
};
static BatchPivotDebugStats g_batch_dbg;
static inline long long dbg_now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
struct ScopedNsAcc {
    long long* ns = nullptr;
    long long* calls = nullptr;
    long long start = 0;
    ScopedNsAcc(long long* ns_, long long* calls_) : ns(ns_), calls(calls_), start(dbg_now_ns()) {}
    ~ScopedNsAcc() {
        long long dt = dbg_now_ns() - start;
        if (ns) *ns += dt;
        if (calls) (*calls)++;
    }
};
struct ScopedIntInc {
    int* ref = nullptr;
    explicit ScopedIntInc(int* r) : ref(r) { if (ref) ++(*ref); }
    ~ScopedIntInc() { if (ref) --(*ref); }
};
struct SlowDeletionProfile {
    int deletionIndex = -1;
    int deletedVertex = -1;
    int touchedClassCount = 0;
    long long connectorSkeletonTerminals = 0;
    long long connectorSkeletonVertices = 0;
    long long connectorSkeletonWatchUnregister = 0;
    long long connectorSkeletonWatchRegister = 0;
    long long preservedPieceSplitVertices = 0;
    long long globalDeleteDfsEdges = 0;
    long long queryIncidentScans = 0;
    long long totalDeletionTimeNs = 0;
    long long timeGlobalDeleteDfsNs = 0;
    long long timeConnectorSkeletonBuildNs = 0;
    long long timeConnectorSkeletonWatchUnregisterNs = 0;
    long long timeConnectorSkeletonWatchRegisterNs = 0;
    long long timePreservedPieceSplitNs = 0;
    long long timeQueryIncidentScanNs = 0;
    long long dispatchCandidateCids = 0;
    long long publishPreservedHandles = 0;
    long long publishConnectorHandles = 0;
    long long publishPosmapBuilds = 0;
    long long publishFullRescanCalls = 0;
    long long publishNoopCalls = 0;
    int reuseRouteTag = 0;
    long long reuseKeepmaskRemovedHandles = 0;
    long long reusePreservedDirectRetagHandles = 0;
    long long reuseConnectorDirectRetagHandles = 0;
    long long reuseAttachmentRetargets = 0;
    long long reusePatchVertices = 0;
    long long reusePatchHandlesAdded = 0;
    long long reusePrepublishPreservedAnnotateCalls = 0;
    long long reusePrepublishConnectorAnnotateCalls = 0;
    long long reuseFinalPublishNoopCalls = 0;
    long long reuseFinalPublishSkippedCalls = 0;
    long long psplitOldPiecesScanned = 0;
    long long psplitContainsXHits = 0;
    long long psplitXLocalPosLookups = 0;
    long long psplitTreePosmapBuilds = 0;
    long long psplitReplacementPieceCount = 0;
    long long psplitAttachmentValidateCalls = 0;
    long long psplitAttachmentRetargetCalls = 0;
    long long psplitAttachmentFixupCalls = 0;
    long long timePsplitTotalNs = 0;
    long long timeReuseTotalNs = 0;
    int wscanRouteTag = 0;
    long long wscanPreservedHandlesScanned = 0;
    long long wscanConnectorHandlesScanned = 0;
    long long wscanExistingConnectorSetHandlesScanned = 0;
    long long wscanRetainRemovedHandles = 0;
    long long wscanRetainSlotposFixups = 0;
    long long wscanDuplicateFullScanPasses = 0;
    long long retainRemovedHandles = 0;
    long long retainSparseRemovedEntries = 0;
    long long retainMovedEntryCount = 0;
    long long retainOwnerLookupCalls = 0;
    long long retainOwnerLookupMisses = 0;
    long long retainSlotposFixups = 0;
    long long retainKeptHandlesCopied = 0;
    long long retainHandleidxFixups = 0;
    long long kvecUnchangedPrefixHandles = 0;
    long long kvecUnchangedSuffixHandles = 0;
    long long kvecMovedSuffixHandles = 0;
    long long kvecChangedPatchlistEntries = 0;
    long long kvecHandleCopyEntries = 0;
    long long kvecHandleidxPatchChangedEntries = 0;
    long long kvecHandleidxPatchSkippedSameIndexEntries = 0;
    long long kvecInplaceCompactCalls = 0;
    long long kvecSuffixResizeFastpathCalls = 0;
    long long scompFirstRemovedIndex = 0;
    long long scompRemovedRunCount = 0;
    long long scompKeptRunCount = 0;
    long long scompPrefixSkippedHandles = 0;
    long long scompBlockCopiedHandles = 0;
    long long scompElementwiseEmittedHandles = 0;
    long long scompSuffixOnlyCalls = 0;
    long long scompSingleMiddleRunCalls = 0;
    long long scompScratchCapacityReuseCalls = 0;
    int bcopyRouteTag = 0;
    long long bcopySingleMiddleRunCalls = 0;
    long long bcopyRemovedRunCount = 0;
    long long bcopyKeptRunCount = 0;
    long long bcopyCopyPlanEntries = 0;
    long long bcopyDirectMemmoveCalls = 0;
    long long bcopyDirectMemmovedHandles = 0;
    long long bcopyBlockCopiedHandles = 0;
    long long bcopyElementwiseFallbackHandles = 0;
    long long bcopyOverlapStagingCalls = 0;
    int planRouteTag = 0;
    long long planFirstRemovedIndex = 0;
    long long planRemovedRunCount = 0;
    long long planKeptRunCount = 0;
    long long planAdjacentMergeHits = 0;
    long long planDescriptorCount = 0;
    long long planDstIndexUpdates = 0;
    long long planSingleMiddleShortcircuitHits = 0;
    long long planSmallInlineHits = 0;
    int rdiscRouteTag = 0;
    long long rdiscFirstRemovedIndex = 0;
    long long rdiscRemovedRunCount = 0;
    long long rdiscKeptRunCount = 0;
    long long rdiscBoundaryReuseHits = 0;
    long long rdiscSuffixOnlyHits = 0;
    long long rdiscSingleMiddleHits = 0;
    long long rdiscFusedOnepassCalls = 0;
    long long rdiscSmallRunlistInlineHits = 0;
    int fclassRouteTag = 0;
    long long fclassSuffixOnlyHits = 0;
    long long fclassSingleMiddleHits = 0;
    long long fclassFusedOnepassCalls = 0;
    long long fclassTransitionSteps = 0;
    long long fclassRemovedToKeptTransitions = 0;
    long long fclassKeptToRemovedTransitions = 0;
    long long fclassSmallInlineHits = 0;
    long long timeFclassTotalNs = 0;
    int tscanRouteTag = 0;
    long long tscanFirstRemovedIndex = 0;
    long long tscanScanWindowHandles = 0;
    long long tscanRemovedToKeptTransitions = 0;
    long long tscanKeptToRemovedTransitions = 0;
    long long tscanBoundaryReuseHits = 0;
    long long tscanTailStopHits = 0;
    long long tscanEarlyExitCalls = 0;
    long long timeTscanTotalNs = 0;
    long long timeRdiscTotalNs = 0;
    long long timePlanTotalNs = 0;
    long long timeBcopyTotalNs = 0;
    long long timeScompTotalNs = 0;
    long long timeKvecTotalNs = 0;
    long long timeRetainTotalNs = 0;
    long long timeWscanTotalNs = 0;
};
static std::vector<SlowDeletionProfile> g_slow_deletion_profiles;
static constexpr int kSlowDeletionKeep = 10;
enum ReuseRouteTag {
    REUSE_ROUTE_NONE = 0,
    REUSE_ROUTE_BASELINE = 1,
    REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON = 2,
    REUSE_ROUTE_CONNECTOR_SKELETON = 3,
    REUSE_ROUTE_GENERAL_DELTA = 4,
};
static const char* reuse_route_name(int tag) {
    switch (tag) {
        case REUSE_ROUTE_BASELINE: return "baseline";
        case REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON: return "delta_preserved_then_skeleton";
        case REUSE_ROUTE_CONNECTOR_SKELETON: return "connector_skeleton";
        case REUSE_ROUTE_GENERAL_DELTA: return "general_delta";
        default: return "none";
    }
}
static int g_wscan_active_route_tag = REUSE_ROUTE_NONE;
static int g_wscan_route_full_scan_passes = 0;
static long long g_wscan_route_full_scan_handles = 0;
static int g_wscan_retain_ctx = 0;
static void add_wscan_route_time(long long dt) {
    switch (g_wscan_active_route_tag) {
        case REUSE_ROUTE_BASELINE: g_batch_dbg.time_wscan_route_baseline_ns += dt; break;
        case REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON: g_batch_dbg.time_wscan_route_delta_preserved_then_skeleton_ns += dt; break;
        case REUSE_ROUTE_CONNECTOR_SKELETON: g_batch_dbg.time_wscan_route_connector_skeleton_ns += dt; break;
        case REUSE_ROUTE_GENERAL_DELTA: g_batch_dbg.time_wscan_route_general_delta_ns += dt; break;
        default: break;
    }
}
static void acc_wscan_keepmask_ns(long long dt, long long* ns, long long* calls) {
    if (!ns || !calls) return;
    *ns += dt;
    (*calls)++;
    g_batch_dbg.time_reuse_keepmask_scan_ns += dt;
    g_batch_dbg.time_reuse_keepmask_scan_calls++;
    add_wscan_route_time(dt);
}
static void acc_wscan_retain_ns(long long dt, long long* ns, long long* calls) {
    if (!ns || !calls) return;
    *ns += dt;
    (*calls)++;
    g_batch_dbg.time_reuse_watch_retain_ns += dt;
    g_batch_dbg.time_reuse_watch_retain_calls++;
    add_wscan_route_time(dt);
}
static void add_bcopy_route_time(long long dt) {
    switch (g_wscan_active_route_tag) {
        case REUSE_ROUTE_BASELINE: g_batch_dbg.time_bcopy_route_baseline_ns += dt; break;
        case REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON: g_batch_dbg.time_bcopy_route_delta_preserved_then_skeleton_ns += dt; break;
        case REUSE_ROUTE_CONNECTOR_SKELETON: g_batch_dbg.time_bcopy_route_connector_skeleton_ns += dt; break;
        case REUSE_ROUTE_GENERAL_DELTA: g_batch_dbg.time_bcopy_route_general_delta_ns += dt; break;
        default: break;
    }
}
static void note_bcopy_route_call() {
    switch (g_wscan_active_route_tag) {
        case REUSE_ROUTE_BASELINE: g_batch_dbg.bcopy_route_baseline_calls++; break;
        case REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON: g_batch_dbg.bcopy_route_delta_preserved_then_skeleton_calls++; break;
        case REUSE_ROUTE_CONNECTOR_SKELETON: g_batch_dbg.bcopy_route_connector_skeleton_calls++; break;
        case REUSE_ROUTE_GENERAL_DELTA: g_batch_dbg.bcopy_route_general_delta_calls++; break;
        default: break;
    }
}
static void add_plan_route_time(long long dt) {
    switch (g_wscan_active_route_tag) {
        case REUSE_ROUTE_BASELINE: g_batch_dbg.time_plan_route_baseline_ns += dt; break;
        case REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON: g_batch_dbg.time_plan_route_delta_preserved_then_skeleton_ns += dt; break;
        case REUSE_ROUTE_CONNECTOR_SKELETON: g_batch_dbg.time_plan_route_connector_skeleton_ns += dt; break;
        case REUSE_ROUTE_GENERAL_DELTA: g_batch_dbg.time_plan_route_general_delta_ns += dt; break;
        default: break;
    }
}
static void note_plan_route_call() {
    switch (g_wscan_active_route_tag) {
        case REUSE_ROUTE_BASELINE: g_batch_dbg.plan_route_baseline_calls++; break;
        case REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON: g_batch_dbg.plan_route_delta_preserved_then_skeleton_calls++; break;
        case REUSE_ROUTE_CONNECTOR_SKELETON: g_batch_dbg.plan_route_connector_skeleton_calls++; break;
        case REUSE_ROUTE_GENERAL_DELTA: g_batch_dbg.plan_route_general_delta_calls++; break;
        default: break;
    }
}
static void add_rdisc_route_time(long long dt) {
    switch (g_wscan_active_route_tag) {
        case REUSE_ROUTE_BASELINE: g_batch_dbg.time_rdisc_route_baseline_ns += dt; break;
        case REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON: g_batch_dbg.time_rdisc_route_delta_preserved_then_skeleton_ns += dt; break;
        case REUSE_ROUTE_CONNECTOR_SKELETON: g_batch_dbg.time_rdisc_route_connector_skeleton_ns += dt; break;
        case REUSE_ROUTE_GENERAL_DELTA: g_batch_dbg.time_rdisc_route_general_delta_ns += dt; break;
        default: break;
    }
}
static void note_rdisc_route_call() {
    switch (g_wscan_active_route_tag) {
        case REUSE_ROUTE_BASELINE: g_batch_dbg.rdisc_route_baseline_calls++; break;
        case REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON: g_batch_dbg.rdisc_route_delta_preserved_then_skeleton_calls++; break;
        case REUSE_ROUTE_CONNECTOR_SKELETON: g_batch_dbg.rdisc_route_connector_skeleton_calls++; break;
        case REUSE_ROUTE_GENERAL_DELTA: g_batch_dbg.rdisc_route_general_delta_calls++; break;
        default: break;
    }
}
static void add_fclass_route_time(long long dt) {
    switch (g_wscan_active_route_tag) {
        case REUSE_ROUTE_BASELINE: g_batch_dbg.time_fclass_route_baseline_ns += dt; break;
        case REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON: g_batch_dbg.time_fclass_route_delta_preserved_then_skeleton_ns += dt; break;
        case REUSE_ROUTE_CONNECTOR_SKELETON: g_batch_dbg.time_fclass_route_connector_skeleton_ns += dt; break;
        case REUSE_ROUTE_GENERAL_DELTA: g_batch_dbg.time_fclass_route_general_delta_ns += dt; break;
        default: break;
    }
}
static void note_fclass_route_call() {
    switch (g_wscan_active_route_tag) {
        case REUSE_ROUTE_BASELINE: g_batch_dbg.fclass_route_baseline_calls++; break;
        case REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON: g_batch_dbg.fclass_route_delta_preserved_then_skeleton_calls++; break;
        case REUSE_ROUTE_CONNECTOR_SKELETON: g_batch_dbg.fclass_route_connector_skeleton_calls++; break;
        case REUSE_ROUTE_GENERAL_DELTA: g_batch_dbg.fclass_route_general_delta_calls++; break;
        default: break;
    }
}
static void add_tscan_route_time(long long dt) {
    switch (g_wscan_active_route_tag) {
        case REUSE_ROUTE_BASELINE: g_batch_dbg.time_tscan_route_baseline_ns += dt; break;
        case REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON: g_batch_dbg.time_tscan_route_delta_preserved_then_skeleton_ns += dt; break;
        case REUSE_ROUTE_CONNECTOR_SKELETON: g_batch_dbg.time_tscan_route_connector_skeleton_ns += dt; break;
        case REUSE_ROUTE_GENERAL_DELTA: g_batch_dbg.time_tscan_route_general_delta_ns += dt; break;
        default: break;
    }
}
static void note_tscan_route_call() {
    switch (g_wscan_active_route_tag) {
        case REUSE_ROUTE_BASELINE: g_batch_dbg.tscan_route_baseline_calls++; break;
        case REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON: g_batch_dbg.tscan_route_delta_preserved_then_skeleton_calls++; break;
        case REUSE_ROUTE_CONNECTOR_SKELETON: g_batch_dbg.tscan_route_connector_skeleton_calls++; break;
        case REUSE_ROUTE_GENERAL_DELTA: g_batch_dbg.tscan_route_general_delta_calls++; break;
        default: break;
    }
}
struct ScopedWScanRouteContext {
    int prevTag = REUSE_ROUTE_NONE;
    int prevFullPasses = 0;
    long long prevFullHandles = 0;
    explicit ScopedWScanRouteContext(int tag) {
        prevTag = g_wscan_active_route_tag;
        prevFullPasses = g_wscan_route_full_scan_passes;
        prevFullHandles = g_wscan_route_full_scan_handles;
        g_wscan_active_route_tag = tag;
        g_wscan_route_full_scan_passes = 0;
        g_wscan_route_full_scan_handles = 0;
        switch (tag) {
            case REUSE_ROUTE_BASELINE: g_batch_dbg.wscan_route_baseline_calls++; break;
            case REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON: g_batch_dbg.wscan_route_delta_preserved_then_skeleton_calls++; break;
            case REUSE_ROUTE_CONNECTOR_SKELETON: g_batch_dbg.wscan_route_connector_skeleton_calls++; break;
            case REUSE_ROUTE_GENERAL_DELTA: g_batch_dbg.wscan_route_general_delta_calls++; break;
            default: break;
        }
    }
    ~ScopedWScanRouteContext() {
        g_wscan_active_route_tag = prevTag;
        g_wscan_route_full_scan_passes = prevFullPasses;
        g_wscan_route_full_scan_handles = prevFullHandles;
    }
};
static void reset_slow_deletion_profiles() { g_slow_deletion_profiles.clear(); }
static void record_slow_deletion(const SlowDeletionProfile& rec) {
    g_slow_deletion_profiles.push_back(rec);
}
static int g_connector_skeleton_unregister_ctx = 0;
static int g_connector_skeleton_register_ctx = 0;
static int g_connector_skeleton_build_detail_ctx = 0;
static bool g_local_profile_current_delete_sampled = false;
static long long g_local_progress_case_start_ns = 0;
static int g_local_progress_total_deletions = 0;
static int g_local_progress_stride = 1;
static bool local_env_enabled(const char* name, bool defv) {
    return runtime_env_enabled(name, defv);
}
static int local_env_int(const char* name, int defv) {
    const char* v = getenv(name);
    if (!v || !*v) return defv;
    char* end = nullptr;
    long x = strtol(v, &end, 10);
    if (end == v) return defv;
    if (x < 0) x = 0;
    if (x > INT_MAX) x = INT_MAX;
    return (int)x;
}
enum class LocalProfileMode {
    PROFILE_NONE,
    PROFILE_BASE,
    PROFILE_SAMPLED,
};
static LocalProfileMode local_profile_mode() {
    static LocalProfileMode mode = []() {
        const char* v = getenv("PROFILE_MODE");
        string modeText = (v && *v) ? string(v) : string("PROFILE_BASE");
        for (char& ch : modeText) ch = (char)toupper((unsigned char)ch);
        if (modeText == "0" || modeText == "NONE" || modeText == "PROFILE_NONE") return LocalProfileMode::PROFILE_NONE;
        if (modeText == "1" || modeText == "BASE" || modeText == "PROFILE_BASE") return LocalProfileMode::PROFILE_BASE;
        if (modeText == "2" || modeText == "SAMPLED" || modeText == "PROFILE_SAMPLED") return LocalProfileMode::PROFILE_SAMPLED;
        return LocalProfileMode::PROFILE_BASE;
    }();
    return mode;
}
static const char* local_profile_mode_name() {
    switch (local_profile_mode()) {
        case LocalProfileMode::PROFILE_NONE: return "PROFILE_NONE";
        case LocalProfileMode::PROFILE_BASE: return "PROFILE_BASE";
        case LocalProfileMode::PROFILE_SAMPLED: return "PROFILE_SAMPLED";
    }
    return "PROFILE_BASE";
}
static bool local_profile_coarse_enabled() {
    return local_profile_mode() != LocalProfileMode::PROFILE_NONE;
}
static bool local_profile_topk_enabled() {
    return local_profile_mode() == LocalProfileMode::PROFILE_SAMPLED;
}
static bool local_profile_detailed_enabled() {
    return local_profile_mode() == LocalProfileMode::PROFILE_SAMPLED && g_local_profile_current_delete_sampled;
}
static int profile_sample_stride() {
    static int stride = local_env_int("PROFILE_SAMPLE_STRIDE", 8);
    if (stride <= 1) return 1;
    return stride;
}
static int profile_sample_warmup() {
    static int warmup = local_env_int("PROFILE_SAMPLE_WARMUP", 64);
    if (warmup < 0) return 0;
    return warmup;
}
static bool should_profile_deletion_sample(int deletionIndex) {
    if (!local_profile_topk_enabled()) return false;
    const int stride = profile_sample_stride();
    const int warmup = profile_sample_warmup();
    if (stride <= 1) return true;
    if (deletionIndex <= warmup) return true;
    return (deletionIndex % stride) == 0;
}
static const char* local_delta_toggle_mode_name() {
    bool p = local_env_enabled("ENABLE_DELTA_PRESERVED_HIT", true);
    bool c = local_env_enabled("ENABLE_DELTA_CONNECTOR_HIT", true);
    if (!p && !c) return "both_off";
    if (p && !c) return "preserved_only";
    if (!p && c) return "connector_only";
    return "both_on";
}
static int profile_progress_stride() {
    int stride = local_env_int("PROFILE_PROGRESS_STRIDE", 0);
    if (stride > 0) return stride;
    if (g_local_progress_total_deletions <= 0) return 1;
    return max(1, g_local_progress_total_deletions / 16);
}
template <typename T>
static T* ptr_if(bool enabled, T* p) {
    return enabled ? p : nullptr;
}
static void emit_progress_checkpoint(const char* phase, int deletionIndex, int deletedVertex, int touchedClassCount) {
    g_batch_dbg.debug_progress_checkpoint_calls++;
    if (deletionIndex > g_batch_dbg.debug_progress_last_deletion) g_batch_dbg.debug_progress_last_deletion = deletionIndex;
    long long elapsedMs = 0;
    if (g_local_progress_case_start_ns > 0) elapsedMs = (dbg_now_ns() - g_local_progress_case_start_ns) / 1000000LL;
    cerr << "[progress] phase=" << phase
         << " run_tag=" << ([]() -> const char* { const char* v = getenv("RUN_TAG"); return (v && *v) ? v : ""; })()
         << " delta_mode=" << local_delta_toggle_mode_name()
         << " profile_mode=" << local_profile_mode_name()
         << " sampled=" << (local_profile_topk_enabled() ? "yes" : "no")
         << " sample_stride=" << (local_profile_topk_enabled() ? profile_sample_stride() : 0)
         << " sample_warmup=" << (local_profile_topk_enabled() ? profile_sample_warmup() : 0)
         << " progress_stride=" << g_local_progress_stride
         << " deletion=" << deletionIndex << "/" << g_local_progress_total_deletions
         << " x=" << deletedVertex
         << " touched=" << touchedClassCount
         << " detailed_sampled=" << (g_local_profile_current_delete_sampled ? 1 : 0)
         << " elapsed_ms=" << elapsedMs
         << "\n";
    cerr.flush();
}
static void progress_case_start(int n, int m) {
    g_local_progress_case_start_ns = dbg_now_ns();
    g_local_progress_total_deletions = max(0, n);
    g_local_progress_stride = profile_progress_stride();
    (void)m;
    emit_progress_checkpoint("case_start", 0, -1, -1);
}
static void progress_init_done() {
    emit_progress_checkpoint("init_done", 0, -1, -1);
}
static bool should_emit_progress_checkpoint(int deletionIndex) {
    if (deletionIndex <= 1) return true;
    if (g_local_progress_total_deletions > 0 && deletionIndex >= g_local_progress_total_deletions) return true;
    int stride = max(1, g_local_progress_stride);
    return (deletionIndex % stride) == 0;
}
#endif

class NBOracle {
public:
    virtual ~NBOracle() = default;
    virtual void init(int n, const vector<pair<int,int>>& undirectedEdges,
                      const vector<BranchQuery>& branchQueries) = 0;
    virtual int comp(int v) const = 0;
    virtual vector<int> listComponents() const = 0;
    virtual bool isFailing(int qid) const = 0;
    virtual void eraseVertex(int x, vector<int>& newComponents, vector<WitnessChange>& changes) = 0;
};

class DynamicForestCoreHDT {
public:
    struct EdgeRec {
        int id=-1,u=-1,v=-1; bool alive=false;
        optional<dgraph::EdgeToken> tok;
    };
    void init(int n) {
        n_ = n;
        g_ = make_unique<dgraph::DynamicGraph>(n_ + 1);
        aliveV_.assign(n_ + 1, true);
        edges_.clear();
        adj_.assign(n_ + 1, {});
        edgeId_.clear();
    }
    int addEdge(int u, int v) {
        if (u > v) swap(u, v);
        long long key = (1LL*u<<32) ^ (unsigned)v;
        auto it = edgeId_.find(key);
        if (it != edgeId_.end()) return it->second;
        int id = (int)edges_.size();
        EdgeRec e; e.id=id; e.u=u; e.v=v; e.alive=true; e.tok.emplace(g_->add((unsigned)u,(unsigned)v));
        edges_.push_back(std::move(e));
        adj_[u].push_back(id); adj_[v].push_back(id);
        edgeId_[key]=id;
        return id;
    }
    bool vertexAlive(int v) const { return 1<=v && v<=n_ && aliveV_[v]; }
    bool edgeAlive(int eid) const { return 0<=eid && eid<(int)edges_.size() && edges_[eid].alive; }
    int other(int eid, int x) const { const auto& e = edges_[eid]; return e.u ^ e.v ^ x; }
    pair<int,int> edgeEndpoints(int eid) const { return {edges_[eid].u, edges_[eid].v}; }
    const vector<int>& incidentEdges(int v) const { return adj_[v]; }
    int maxVertexId() const { return n_; }
    int edgeIdOf(int u, int v) const {
        if (u > v) swap(u, v);
        long long key = (1LL*u<<32) ^ (unsigned)v;
        auto it = edgeId_.find(key);
        return it == edgeId_.end() ? -1 : it->second;
    }
    vector<int> deleteVertexBatch(int x) {
        vector<int> deleted;
        if (!vertexAlive(x)) return deleted;
        aliveV_[x] = false;
        for (int eid : adj_[x]) {
            if (!edgeAlive(eid)) continue;
            edges_[eid].alive = false;
            if (edges_[eid].tok.has_value()) {
                g_->remove(std::move(*edges_[eid].tok));
                edges_[eid].tok.reset();
            }
            deleted.push_back(eid);
        }
        return deleted;
    }
    bool sameComponent(int u, int v) const {
        if (!vertexAlive(u) || !vertexAlive(v)) return false;
        return const_cast<dgraph::DynamicGraph*>(g_.get())->is_connected((unsigned)u, (unsigned)v);
    }
    vector<int> enumerateComponent(int start) const {
        vector<int> comp;
        if (!vertexAlive(start)) return comp;
        vector<char> vis(n_ + 1, 0);
        queue<int> q; q.push(start); vis[start]=1;
        while(!q.empty()){
            int u=q.front(); q.pop();
            comp.push_back(u);
            for(int eid : adj_[u]) if(edgeAlive(eid)) {
                int v = other(eid,u);
                if(!vertexAlive(v) || vis[v]) continue;
                vis[v]=1; q.push(v);
            }
        }
        return comp;
    }
    vector<vector<int>> enumerateAllComponents() const {
        vector<vector<int>> comps; vector<char> seen(n_+1,0);
        for(int v=1; v<=n_; ++v) if(vertexAlive(v) && !seen[v]) {
            auto cc = enumerateComponent(v);
            for(int x : cc) seen[x]=1;
            if(!cc.empty()) comps.push_back(move(cc));
        }
        return comps;
    }
    vector<vector<int>> enumerateTouchedComponents(const vector<int>& touched) const {
        vector<vector<int>> comps; vector<char> seen(n_+1,0);
        for(int s : touched) if(vertexAlive(s) && !seen[s]) {
            auto cc = enumerateComponent(s);
            for(int x : cc) seen[x]=1;
            if(!cc.empty()) comps.push_back(move(cc));
        }
        return comps;
    }
private:
    int n_ = 0;
    unique_ptr<dgraph::DynamicGraph> g_;
    vector<char> aliveV_;
    vector<EdgeRec> edges_;
    vector<vector<int>> adj_;
    unordered_map<long long,int> edgeId_;
};

class PotentialHandleManager {
public:
    struct Handle {
        int nodeId=-1; // explicit decomposition-lattice node id
        int owner=-1, a=-1, b=-1;
        vector<int> regionVerts; // decomposition region / potential support
        vector<int> certVerts;   // exact witness support vertices
        vector<int> certEdges;   // exact witness support edges
        int budgetExp=0;         // synthetic potential: every strict child decreases by 1
    };
    struct Result {
        bool has=false;
        bool strict=false;
        Handle h;
    };

    static long long mass(const Handle& h) {
        return (long long)h.budgetExp;
    }
    static int scoreSize(const Handle& h) {
        return (int)h.regionVerts.size() + (int)h.certEdges.size();
    }
    static int childBudget(const Handle& h) {
        return h.budgetExp > 0 ? h.budgetExp - 1 : 0;
    }
    static constexpr int INITIAL_BUDGET_EXP = 1000000;
    static bool containsVertex(const Handle& h, int v) {
        return binary_search(h.regionVerts.begin(), h.regionVerts.end(), v);
    }
    static bool containsCertVertex(const Handle& h, int v) {
        return binary_search(h.certVerts.begin(), h.certVerts.end(), v);
    }
    static bool valid(const Handle& h) {
        return h.owner!=-1 && !h.certVerts.empty() && !h.regionVerts.empty();
    }

    static Handle buildExactRestricted(const DynamicForestCoreHDT& core, int owner, int a, int b,
                                       const unordered_set<int>& allowed,
                                       long long* outVisitedV = nullptr,
                                       long long* outVisitedE = nullptr) {
        Handle out; out.owner=owner; out.a=a; out.b=b;
#ifdef LOCAL
        g_strict_child_dbg.build_exact_restricted_calls++;
#endif
        if (!allowed.count(owner) || !allowed.count(a) || !allowed.count(b)) return out;
        unordered_map<int,int> par, parEdge;
        queue<int> q; q.push(a); par[a]=a;
        long long visV = 0, visE = 0;
        while(!q.empty()){
            int u=q.front(); q.pop();
            ++visV;
            if(u==b) break;
            for(int eid: core.incidentEdges(u)){
                if(!core.edgeAlive(eid)) continue;
                ++visE;
                auto [x,y]=core.edgeEndpoints(eid);
                if(!allowed.count(x) || !allowed.count(y)) continue;
                int v=core.other(eid,u);
                if(v==owner || !core.vertexAlive(v)) continue;
                if(par.count(v)) continue;
                par[v]=u; parEdge[v]=eid; q.push(v);
            }
        }
        if(!par.count(b)) return out;
        int cur=b;
        while(true){
            out.certVerts.push_back(cur);
            if(cur==a) break;
            out.certEdges.push_back(parEdge[cur]);
            cur=par[cur];
        }
        vector<int> region;
        region.reserve(allowed.size());
        for(int v: allowed) region.push_back(v);
        sort(region.begin(), region.end());
        out.regionVerts = region;
        out.certVerts.push_back(owner);
        sort(out.certVerts.begin(), out.certVerts.end());
        out.certVerts.erase(unique(out.certVerts.begin(), out.certVerts.end()), out.certVerts.end());
        sort(out.certEdges.begin(), out.certEdges.end());
        out.certEdges.erase(unique(out.certEdges.begin(), out.certEdges.end()), out.certEdges.end());
        if (outVisitedV) *outVisitedV = visV;
        if (outVisitedE) *outVisitedE = visE;
#ifdef LOCAL
        g_strict_child_dbg.build_exact_restricted_vertices += visV;
        g_strict_child_dbg.build_exact_restricted_edges += visE;
#endif
        return out;
    }

    static unordered_set<int> collectNeighborhood(const DynamicForestCoreHDT& core,
                                                  const vector<int>& seeds,
                                                  int owner, int removedV, int radius) {
        unordered_set<int> allowed;
        queue<pair<int,int>> q;
        for(int v: seeds){
            if(!core.vertexAlive(v) || v==owner || v==removedV) continue;
            if(allowed.insert(v).second) q.push({v,0});
        }
        while(!q.empty()){
            auto [u,d]=q.front(); q.pop();
            if(d>=radius) continue;
            for(int eid: core.incidentEdges(u)) if(core.edgeAlive(eid)) {
                int v=core.other(eid,u);
                if(!core.vertexAlive(v) || v==owner || v==removedV) continue;
                if(allowed.insert(v).second) q.push({v,d+1});
            }
        }
        allowed.insert(owner);
        return allowed;
    }

    static Handle buildSeedHandle(const DynamicForestCoreHDT& core, int owner, int a, int b, int radius, int budgetExp) {
        unordered_set<int> trivial = {owner,a,b};
        auto ex = buildExactRestricted(core, owner, a, b, trivial);
        if (!valid(ex)) {
            // exact path in full graph
            unordered_set<int> full;
            full.insert(owner);
            queue<int> q; unordered_map<int,int> par, parEdge;
            q.push(a); par[a]=a;
            while(!q.empty()){
                int u=q.front(); q.pop();
                if(u==b) break;
                for(int eid: core.incidentEdges(u)) if(core.edgeAlive(eid)) {
                    int v=core.other(eid,u);
                    if(!core.vertexAlive(v) || v==owner) continue;
                    if(par.count(v)) continue;
                    par[v]=u; parEdge[v]=eid; q.push(v);
                }
            }
            Handle out; out.owner=owner; out.a=a; out.b=b;
            if(!par.count(b)) return out;
            int cur=b; vector<int> path;
            while(true){ path.push_back(cur); if(cur==a) break; out.certEdges.push_back(parEdge[cur]); cur=par[cur]; }
            for(int v:path) full.insert(v);
            auto allowed = collectNeighborhood(core, path, owner, -1, radius);
            auto h = buildExactRestricted(core, owner, a, b, allowed);
            h.budgetExp = budgetExp;
            return h;
        }
        auto allowed = collectNeighborhood(core, ex.certVerts, owner, -1, radius);
        auto h = buildExactRestricted(core, owner, a, b, allowed);
        h.budgetExp = budgetExp;
        return h;
    }

    static Result wrap(const Handle& old, Handle cand) {
        Result r;
        if(!valid(cand)) return r;
        cand.budgetExp = childBudget(old);
        r.has = true;
        r.strict = (mass(cand) < mass(old));
        r.h = std::move(cand);
        return r;
    }

    static Result relocateByRegionCentroid(const DynamicForestCoreHDT& core,
                                           const Handle& old, int removedV) {
        Result out;
        if(removedV==old.owner || removedV==old.a || removedV==old.b) return out;
        unordered_set<int> allowed(old.regionVerts.begin(), old.regionVerts.end());
        allowed.erase(removedV);
        if(!allowed.count(old.owner) || !allowed.count(old.a) || !allowed.count(old.b)) return out;
        // reachable graph on region-owner-removed
        unordered_map<int, vector<int>> g;
        unordered_set<int> alive;
        for(int v: old.regionVerts) if(v!=old.owner && v!=removedV && core.vertexAlive(v)) alive.insert(v);
        if(!alive.count(old.a) || !alive.count(old.b)) return out;
        queue<int> qq; unordered_map<int,int> par; qq.push(old.a); par[old.a]=old.a;
        vector<int> reach;
        while(!qq.empty()){
            int u=qq.front(); qq.pop();
            reach.push_back(u);
            for(int eid: core.incidentEdges(u)) if(core.edgeAlive(eid)) {
                int v=core.other(eid,u);
                if(!alive.count(v)) continue;
                if(par.count(v)) continue;
                par[v]=u; qq.push(v);
            }
        }
        if(!par.count(old.b)) return out;
        int m=(int)reach.size();
        unordered_map<int,int> idx; for(int i=0;i<m;++i) idx[reach[i]]=i;
        vector<vector<int>> tree(m);
        for(int i=0;i<m;++i){ int u=reach[i]; if(u!=old.a){ int p=par[u]; tree[i].push_back(idx[p]); tree[idx[p]].push_back(i);} }
        vector<int> sub(m,0);
        function<void(int,int)> dfs=[&](int u,int p){ sub[u]=1; for(int v:tree[u]) if(v!=p){ dfs(v,u); sub[u]+=sub[v]; } };
        dfs(idx[old.a], -1);
        int centroid=idx[old.a], best=m;
        function<void(int,int)> fcent=[&](int u,int p){ int mx=m-sub[u]; for(int v:tree[u]) if(v!=p){ mx=max(mx,sub[v]); fcent(v,u);} if(mx<best){best=mx; centroid=u;} };
        fcent(idx[old.a], -1);
        vector<int> compId(m,-1);
        int cid=0;
        for(int v: tree[centroid]){
            queue<int> q; q.push(v); compId[v]=cid;
            while(!q.empty()){
                int u=q.front(); q.pop();
                for(int w: tree[u]) if(w!=centroid && compId[w]==-1){ compId[w]=cid; q.push(w);}    
            }
            ++cid;
        }
        int ca = (idx.count(old.a)? idx[old.a] : -1);
        int cb = (idx.count(old.b)? idx[old.b] : -1);
        vector<Handle> cands;
        // same child component without centroid
        if(ca!=-1 && cb!=-1 && compId[ca]!=-1 && compId[ca]==compId[cb]){
            unordered_set<int> child = {old.owner};
            for(int i=0;i<m;++i) if(compId[i]==compId[ca]) child.insert(reach[i]);
            auto cand = buildExactRestricted(core, old.owner, old.a, old.b, child);
            cands.push_back(std::move(cand));
        }
        // try each centroid+component region
        for(int c=0;c<cid;++c){
            unordered_set<int> child = {old.owner, reach[centroid]};
            for(int i=0;i<m;++i) if(compId[i]==c) child.insert(reach[i]);
            auto cand = buildExactRestricted(core, old.owner, old.a, old.b, child);
            if(valid(cand)) cands.push_back(std::move(cand));
        }
        // try centroid + path-side around one endpoint if centroid on tree path
        Result bestR;
        for(auto &cand: cands){
            auto rr = wrap(old, std::move(cand));
            if(!rr.has) continue;
            if(!bestR.has || (rr.strict > bestR.strict) || (rr.strict==bestR.strict && mass(rr.h)<mass(bestR.h))) bestR=rr;
        }
        return bestR;
    }

    static Result relocateByRegionBlockCut(const DynamicForestCoreHDT& core,
                                           const Handle& old, int removedV) {
        Result out;
        if(removedV==old.owner || removedV==old.a || removedV==old.b) return out;
        unordered_set<int> keep(old.regionVerts.begin(), old.regionVerts.end());
        keep.erase(old.owner); keep.erase(removedV);
        if(!keep.count(old.a) || !keep.count(old.b)) return out;
        vector<int> verts(keep.begin(), keep.end()); sort(verts.begin(), verts.end());
        int n=(int)verts.size(); if(n==0) return out;
        unordered_map<int,int> vid; vid.reserve(n*2+1); for(int i=0;i<n;++i) vid[verts[i]]=i;
        struct LE{int u,v;}; vector<LE> ledges; vector<vector<pair<int,int>>> adj(n);
        for(int rv: verts){
            for(int eid: core.incidentEdges(rv)) if(core.edgeAlive(eid)) {
                auto [u,v]=core.edgeEndpoints(eid);
                if(!keep.count(u) || !keep.count(v)) continue;
                int iu=vid[u], iv=vid[v];
                if(iu>iv) swap(iu,iv);
            }
        }
        unordered_set<long long> seenE;
        for(int i=0;i<n;++i){
            int u=verts[i];
            for(int eid: core.incidentEdges(u)) if(core.edgeAlive(eid)) {
                int v=core.other(eid,u); if(!keep.count(v)) continue;
                int iu=vid[u], iv=vid[v]; if(iu>iv) swap(iu,iv);
                long long key=(1LL*iu<<32)^iv; if(!seenE.insert(key).second) continue;
                int id=(int)ledges.size(); ledges.push_back({iu,iv}); adj[iu].push_back({iv,id}); adj[iv].push_back({iu,id});
            }
        }
        int sa=vid[old.a], sb=vid[old.b];
        vector<int> disc(n,-1), low(n,0), st; int timer=0; vector<vector<int>> blocksE;
        function<void(int,int)> dfs=[&](int u,int pe){
            disc[u]=low[u]=++timer;
            for(auto [v,eid]: adj[u]){
                if(eid==pe) continue;
                if(disc[v]==-1){ st.push_back(eid); dfs(v,eid); low[u]=min(low[u],low[v]); if(low[v]>=disc[u]){ vector<int> comp; while(true){ int x=st.back(); st.pop_back(); comp.push_back(x); if(x==eid) break; } blocksE.push_back(move(comp)); } }
                else if(disc[v]<disc[u]){ st.push_back(eid); low[u]=min(low[u],disc[v]); }
            }
        };
        dfs(sa,-1);
        if(disc[sb]==-1) return out;
        int B=(int)blocksE.size(); if(B==0) return out;
        vector<vector<int>> vBlocks(n), blockVerts(B);
        for(int bi=0; bi<B; ++bi){ unordered_set<int> seen; for(int id: blocksE[bi]){ int u=ledges[id].u, v=ledges[id].v; if(seen.insert(u).second){blockVerts[bi].push_back(u); vBlocks[u].push_back(bi);} if(seen.insert(v).second){blockVerts[bi].push_back(v); vBlocks[v].push_back(bi);} } }
        Result bestR;
        for(int bi: vBlocks[sa]){
            bool hasb=false; for(int bj: vBlocks[sb]) if(bj==bi){hasb=true; break;}
            if(hasb){ unordered_set<int> allowed={old.owner}; for(int i:blockVerts[bi]) allowed.insert(verts[i]); auto cand=buildExactRestricted(core, old.owner, old.a, old.b, allowed); auto rr=wrap(old, move(cand)); if(rr.has && (!bestR.has || (rr.strict>bestR.strict)||(rr.strict==bestR.strict && mass(rr.h)<mass(bestR.h)))) bestR=rr; }
        }
        return bestR;
    }


    static Result relocateByRegionBCTreePath(const DynamicForestCoreHDT& core,
                                             const Handle& old, int removedV) {
        Result out;
        if (removedV == old.owner || removedV == old.a || removedV == old.b) return out;
        unordered_set<int> keep(old.regionVerts.begin(), old.regionVerts.end());
        keep.erase(old.owner);
        if (removedV != -1) keep.erase(removedV);
        if (!keep.count(old.a) || !keep.count(old.b)) return out;
        vector<int> verts(keep.begin(), keep.end());
        sort(verts.begin(), verts.end());
        int n = (int)verts.size();
        if (n == 0) return out;
        unordered_map<int,int> vid; vid.reserve(n * 2 + 1);
        for (int i = 0; i < n; ++i) vid[verts[i]] = i;
        struct LE { int u, v; };
        vector<LE> ledges;
        vector<vector<pair<int,int>>> adj(n);
        unordered_set<long long> seenE;
        for (int i = 0; i < n; ++i) {
            int u = verts[i];
            for (int eid : core.incidentEdges(u)) if (core.edgeAlive(eid)) {
                int v = core.other(eid, u);
                if (!keep.count(v)) continue;
                int iu = vid[u], iv = vid[v];
                if (iu > iv) swap(iu, iv);
                long long key = (1LL * iu << 32) ^ iv;
                if (!seenE.insert(key).second) continue;
                int id = (int)ledges.size();
                ledges.push_back({iu, iv});
                adj[iu].push_back({iv, id});
                adj[iv].push_back({iu, id});
            }
        }
        int sa = vid[old.a], sb = vid[old.b];
        vector<int> vis(n, 0); queue<int> q0; q0.push(sa); vis[sa] = 1;
        while (!q0.empty()) {
            int u = q0.front(); q0.pop();
            for (auto [v, _] : adj[u]) if (!vis[v]) { vis[v] = 1; q0.push(v); }
        }
        if (!vis[sb]) return out;

        vector<int> disc(n, -1), low(n, 0), st; int timer = 0;
        vector<vector<int>> blocksE;
        function<void(int,int)> dfs = [&](int u, int pe) {
            disc[u] = low[u] = ++timer;
            for (auto [v, eid] : adj[u]) {
                if (eid == pe) continue;
                if (disc[v] == -1) {
                    st.push_back(eid);
                    dfs(v, eid);
                    low[u] = min(low[u], low[v]);
                    if (low[v] >= disc[u]) {
                        vector<int> comp;
                        while (true) {
                            int x = st.back(); st.pop_back();
                            comp.push_back(x);
                            if (x == eid) break;
                        }
                        blocksE.push_back(move(comp));
                    }
                } else if (disc[v] < disc[u]) {
                    st.push_back(eid);
                    low[u] = min(low[u], disc[v]);
                }
            }
        };
        dfs(sa, -1);
        int B = (int)blocksE.size();
        if (B == 0) return out;

        vector<vector<int>> vBlocks(n), blockVerts(B);
        for (int bi = 0; bi < B; ++bi) {
            unordered_set<int> seen;
            for (int id : blocksE[bi]) {
                int u = ledges[id].u, v = ledges[id].v;
                if (seen.insert(u).second) { blockVerts[bi].push_back(u); vBlocks[u].push_back(bi); }
                if (seen.insert(v).second) { blockVerts[bi].push_back(v); vBlocks[v].push_back(bi); }
            }
        }

        vector<int> artIdx;
        vector<int> artNodeOf(n, -1);
        for (int i = 0; i < n; ++i) if ((int)vBlocks[i].size() >= 2) {
            artNodeOf[i] = B + (int)artIdx.size();
            artIdx.push_back(i);
        }
        int A = (int)artIdx.size();
        int T = B + A;
        vector<vector<int>> bc(T);
        for (int ai = 0; ai < A; ++ai) {
            int vi = artIdx[ai];
            int an = B + ai;
            for (int bi : vBlocks[vi]) {
                bc[an].push_back(bi);
                bc[bi].push_back(an);
            }
        }

        vector<int> starts, targets;
        for (int bi : vBlocks[sa]) starts.push_back(bi);
        for (int bi : vBlocks[sb]) targets.push_back(bi);
        if (artNodeOf[sa] != -1) starts.push_back(artNodeOf[sa]);
        if (artNodeOf[sb] != -1) targets.push_back(artNodeOf[sb]);
        sort(starts.begin(), starts.end()); starts.erase(unique(starts.begin(), starts.end()), starts.end());
        sort(targets.begin(), targets.end()); targets.erase(unique(targets.begin(), targets.end()), targets.end());
        vector<char> isTarget(T, 0); for (int t : targets) if (0 <= t && t < T) isTarget[t] = 1;
        vector<int> par(T, -1); queue<int> q;
        for (int s : starts) if (0 <= s && s < T && par[s] == -1) { par[s] = s; q.push(s); }
        int meet = -1;
        while (!q.empty() && meet == -1) {
            int u = q.front(); q.pop();
            if (isTarget[u]) { meet = u; break; }
            for (int v : bc[u]) if (par[v] == -1) { par[v] = u; q.push(v); }
        }
        if (meet == -1) return out;
        unordered_set<int> allowed = {old.owner};
        int cur = meet;
        while (true) {
            if (cur < B) {
                for (int i : blockVerts[cur]) allowed.insert(verts[i]);
            } else {
                int artPos = cur - B;
                if (0 <= artPos && artPos < A) allowed.insert(verts[artIdx[artPos]]);
            }
            if (par[cur] == cur) break;
            cur = par[cur];
        }
        auto cand = buildExactRestricted(core, old.owner, old.a, old.b, allowed);
        return wrap(old, std::move(cand));
    }

    static Result relocateByRegionSeparatorChild(const DynamicForestCoreHDT& core,
                                                 const Handle& old, int removedV) {
        Result bestR;
        if (removedV == old.owner || removedV == old.a || removedV == old.b) return bestR;
        unordered_set<int> keep(old.regionVerts.begin(), old.regionVerts.end());
        keep.erase(old.owner);
        if (removedV != -1) keep.erase(removedV);
        if (!keep.count(old.a) || !keep.count(old.b)) return bestR;
        vector<int> verts(keep.begin(), keep.end());
        sort(verts.begin(), verts.end());
        int n = (int)verts.size();
        if (n == 0) return bestR;
        unordered_map<int,int> vid; vid.reserve(n * 2 + 1);
        for (int i = 0; i < n; ++i) vid[verts[i]] = i;
        vector<vector<int>> adj(n);
        unordered_set<long long> seenE;
        for (int i = 0; i < n; ++i) {
            int u = verts[i];
            for (int eid : core.incidentEdges(u)) if (core.edgeAlive(eid)) {
                int v = core.other(eid, u);
                if (!keep.count(v)) continue;
                int iu = vid[u], iv = vid[v];
                if (iu > iv) swap(iu, iv);
                long long key = (1LL * iu << 32) ^ iv;
                if (!seenE.insert(key).second) continue;
                adj[iu].push_back(iv);
                adj[iv].push_back(iu);
            }
        }
        int sa = vid[old.a], sb = vid[old.b];
        vector<int> disc(n, -1), low(n, 0), par(n, -1); int timer = 0;
        function<void(int)> dfs = [&](int u) {
            disc[u] = low[u] = ++timer;
            for (int v : adj[u]) {
                if (disc[v] == -1) {
                    par[v] = u;
                    dfs(v);
                    low[u] = min(low[u], low[v]);
                } else if (v != par[u]) {
                    low[u] = min(low[u], disc[v]);
                }
            }
        };
        dfs(sa);
        if (disc[sb] == -1) return bestR;
        vector<char> isArt(n, 0);
        int rootChildren = 0;
        for (int v : adj[sa]) if (par[v] == sa) ++rootChildren;
        if (rootChildren > 1) isArt[sa] = 1;
        for (int v = 0; v < n; ++v) if (v != sa && par[v] != -1) {
            int p = par[v];
            if (low[v] >= disc[p]) isArt[p] = 1;
        }
        for (int cut = 0; cut < n; ++cut) if (isArt[cut] && cut != sa && cut != sb) {
            vector<int> comp(n, -1); int cc = 0;
            for (int s = 0; s < n; ++s) if (s != cut && comp[s] == -1) {
                queue<int> q; q.push(s); comp[s] = cc;
                while (!q.empty()) {
                    int u = q.front(); q.pop();
                    for (int v : adj[u]) if (v != cut && comp[v] == -1) { comp[v] = cc; q.push(v); }
                }
                ++cc;
            }
            if (comp[sa] == -1 || comp[sb] == -1 || comp[sa] != comp[sb]) continue;
            unordered_set<int> allowed = {old.owner, verts[cut]};
            int keepComp = comp[sa];
            for (int i = 0; i < n; ++i) if (i == cut || comp[i] == keepComp) allowed.insert(verts[i]);
            auto cand = buildExactRestricted(core, old.owner, old.a, old.b, allowed);
            auto rr = wrap(old, std::move(cand));
            if (rr.has && (!bestR.has || (rr.strict > bestR.strict) || (rr.strict == bestR.strict && mass(rr.h) < mass(bestR.h)))) bestR = rr;
        }
        return bestR;
    }

    static Result chooseBest(const Handle& old, const vector<Result>& cands){
        (void)old;
        Result best; long long bestMass=(1LL<<60); int bestScore=(1<<30); int bestRank=3;
        for(const auto& r: cands){
            if(!r.has) continue;
            int rank=r.strict?0:1; long long m=mass(r.h); int sc=scoreSize(r.h);
            if(rank<bestRank || (rank==bestRank && (m<bestMass || (m==bestMass && sc<bestScore)))){ best=r; bestRank=rank; bestMass=m; bestScore=sc; }
        }
        return best;
    }

    static vector<Result> enumerateOneStep(const DynamicForestCoreHDT& core, const Handle& old, int removedV){
        vector<Result> cands;
        cands.push_back(relocateByRegionCentroid(core, old, removedV));
        cands.push_back(relocateByRegionSeparatorChild(core, old, removedV));
        cands.push_back(relocateByRegionBCTreePath(core, old, removedV));
        cands.push_back(relocateByRegionBlockCut(core, old, removedV));
        {
            unordered_set<int> allowed(old.regionVerts.begin(), old.regionVerts.end());
            if(removedV!=-1) allowed.erase(removedV);
            auto cand = buildExactRestricted(core, old.owner, old.a, old.b, allowed);
            cands.push_back(wrap(old, std::move(cand)));
        }
        return cands;
    }

    static uint64_t hashRegion(const vector<int>& region){
        uint64_t h = 1469598103934665603ULL;
        for(int v: region){ h ^= (uint64_t)(unsigned)v + 0x9e3779b97f4a7c15ULL; h *= 1099511628211ULL; }
        return h;
    }

    static Result relocateByRecursiveSeparatorSearch(const DynamicForestCoreHDT& core, const Handle& old, int removedV, int budget=24){
        struct Node { Handle h; int first; int depth; };
        deque<Node> dq; dq.push_back({old, removedV, 0});
        unordered_set<uint64_t> seen; seen.insert(hashRegion(old.regionVerts));
        Result best; int expanded = 0;
        while(!dq.empty() && expanded < budget){
            Node cur = std::move(dq.front()); dq.pop_front();
            auto cands = enumerateOneStep(core, cur.h, cur.first);
            ++expanded;
            for(auto &r: cands){
                if(!r.has) continue;
                uint64_t key = hashRegion(r.h.regionVerts);
                if(!seen.insert(key).second) continue;
                if(!best.has || (r.strict > best.strict) || (r.strict==best.strict && (mass(r.h)<mass(best.h) || (mass(r.h)==mass(best.h) && scoreSize(r.h)<scoreSize(best.h))))) best = r;
                if(cur.depth + 1 < 4 && mass(r.h) < mass(cur.h)) dq.push_back({r.h, -1, cur.depth + 1});
            }
        }
        return best;
    }

    static Handle postprocess(const DynamicForestCoreHDT& core, Handle cur){
        if(!valid(cur)) return cur;
        for(int it=0; it<6; ++it){
            auto best = relocateByRecursiveSeparatorSearch(core, cur, -1, 18);
            if(!best.has) break;
            if(mass(best.h) >= mass(cur)) break;
            cur = best.h;
        }
        return cur;
    }
};



class DecrementalNBTopology {
    int n_ = 0;
    DynamicForestCoreHDT core_;
    vector<char> alive_;
    vector<int> compId_;
    vector<char> compAlive_;
    vector<vector<int>> compMembers_;
    int nextComp_ = 0;

    vector<vector<int>> ownerEndpoints_;
    mutable vector<char> ownerDirty_;
    mutable vector<unordered_map<int,int>> endpointClass_;
    mutable vector<unordered_map<int, vector<int>>> classEndpoints_;
    mutable vector<unordered_map<int,int>> classRep_;
    mutable vector<vector<int>> classTouchedByRemoved_;
    mutable vector<unordered_map<int,int>> endpointWitnessZone_;
    mutable vector<int> nextClassId_;
    mutable vector<int> ownerLastRemoved_;
    mutable vector<int> oldCompStamp_;
    mutable int oldCompCurStamp_ = 1;
    mutable vector<int> dfsSeenStamp_;
    mutable int dfsCurStamp_ = 1;
    mutable vector<int> dfsTin_;
    mutable vector<int> dfsTout_;
    mutable vector<int> dfsLow_;
    mutable vector<int> dfsParent_;
    mutable vector<int> dfsRoot_;
    mutable vector<int> dfsDepth_;
    mutable vector<int> dfsCompLocal_;
    mutable vector<int> dfsChildCount_;
    mutable vector<int> dfsLowWitnessDesc_;
    mutable vector<int> dfsLowWitnessAnc_;
    mutable vector<int> supportCollectStamp_;
    mutable vector<int> supportJumpStamp_;
    mutable vector<int> supportJump_;
    mutable vector<int> supportTreeParentStamp_;
    mutable vector<int> supportTreeParent_;
    mutable vector<int> supportPathBuf_;
    mutable int supportCollectCur_ = 1;
    mutable int supportTreeParentCur_ = 1;
    mutable bool lastDeleteArtifactReady_ = false;
    mutable int topoActiveEndpointTotal_ = 0;

    void applyOwnerPartition(int owner,
                             unordered_map<int,int> mp,
                             const unordered_map<int,int>* witnessZone = nullptr,
                             const vector<int>* touchedClasses = nullptr) const {
        int oldSz = (1 <= owner && owner <= n_) ? (int)endpointClass_[owner].size() : 0;
        endpointClass_[owner] = std::move(mp);
        topoActiveEndpointTotal_ += (int)endpointClass_[owner].size() - oldSz;
#ifdef LOCAL
        g_topo_dbg.topo_active_endpoint_total = topoActiveEndpointTotal_;
        g_topo_dbg.topo_active_endpoint_peak = max<long long>(g_topo_dbg.topo_active_endpoint_peak, topoActiveEndpointTotal_);
#endif
        ownerDirty_[owner] = false;
        classEndpoints_[owner].clear();
        classRep_[owner].clear();
        endpointWitnessZone_[owner].clear();
        classTouchedByRemoved_[owner].clear();
        int mx = -1;
        for (const auto& kv : endpointClass_[owner]) {
            int ep = kv.first, cid = kv.second;
            classEndpoints_[owner][cid].push_back(ep);
            if (!classRep_[owner].count(cid)) classRep_[owner][cid] = ep;
            mx = max(mx, cid);
        }
        if (witnessZone) endpointWitnessZone_[owner] = *witnessZone;
        if (touchedClasses) classTouchedByRemoved_[owner] = *touchedClasses;
        nextClassId_[owner] = max(nextClassId_[owner], mx + 1);
    }

    void initializeOwnerExact(int owner) const {
        auto exact = computeOwnerExactMap(owner);
        applyOwnerPartition(owner, std::move(exact));
    }

    void rebuildAllComponents() {
        compId_.assign(n_ + 1, -1);
        compAlive_.clear(); compMembers_.clear(); nextComp_ = 0;
        auto comps = core_.enumerateAllComponents();
        for (auto &cc : comps) {
            int h = nextComp_++;
            compAlive_.push_back(true); compMembers_.push_back(cc);
            for (int v : cc) compId_[v] = h;
        }
    }
    void ensureCompCapacity(int h) {
        if (h >= (int)compAlive_.size()) {
            compAlive_.resize(h + 1, false);
            compMembers_.resize(h + 1);
        }
    }

    unordered_map<int,int> computeOwnerExactMap(int owner, long long* outVerts=nullptr, long long* outEdges=nullptr) const {
        unordered_map<int,int> res;
        if (owner < 1 || owner > n_ || !alive_[owner]) return res;
        vector<int> starts;
        starts.reserve(ownerEndpoints_[owner].size());
        for (int v : ownerEndpoints_[owner]) {
            int eid = core_.edgeIdOf(owner, v);
            if (eid == -1) continue;
            if (!core_.vertexAlive(v) || !core_.edgeAlive(eid)) continue;
            starts.push_back(v);
        }
        unordered_map<int,int> seen;
        queue<int> qu;
        int clsCnt = 0;
        long long visV = 0, visE = 0;
        for (int s : starts) if (!seen.count(s)) {
            seen[s] = clsCnt;
            qu.push(s);
            while (!qu.empty()) {
                int u = qu.front(); qu.pop();
                ++visV;
                for (int eid : core_.incidentEdges(u)) if (core_.edgeAlive(eid)) {
                    ++visE;
                    int v = core_.other(eid, u);
                    if (!core_.vertexAlive(v) || v == owner) continue;
                    if (!seen.count(v)) {
                        seen[v] = clsCnt;
                        qu.push(v);
                    }
                }
            }
            ++clsCnt;
        }
        for (int v : starts) res[v] = seen[v];
        if (outVerts) *outVerts = visV;
        if (outEdges) *outEdges = visE;
        return res;
    }

    void rebuildOwnerExact(int owner) const {
        long long vv = 0, ee = 0;
        auto exact = computeOwnerExactMap(owner, &vv, &ee);
        applyOwnerPartition(owner, std::move(exact));
#ifdef LOCAL
        g_topo_dbg.dbg_owner_rebuild_calls++;
        g_topo_dbg.dbg_owner_rebuild_vertices += vv;
        g_topo_dbg.dbg_owner_rebuild_edges += ee;
        if ((int)g_topo_dbg.dbg_owner_rebuild_by_owner.size() <= owner) g_topo_dbg.dbg_owner_rebuild_by_owner.resize(owner + 1, 0);
        g_topo_dbg.dbg_owner_rebuild_by_owner[owner]++;
#endif
    }


    void markOldComponent(const vector<int>& oldVerts, int removedX, int oldStamp) const {
        for (int v : oldVerts) if (1 <= v && v <= n_ && v != removedX && alive_[v]) oldCompStamp_[v] = oldStamp;
    }

    void buildGlobalDeleteArtifact(int removedX,
                                   const vector<int>& oldVerts,
                                   vector<int>& newComponents,
                                   OwnerSplitArtifact* artifact,
                                   int& oldStamp,
                                   int& dfsStamp) {
        if (++oldCompCurStamp_ == INT_MAX) {
            fill(oldCompStamp_.begin(), oldCompStamp_.end(), 0);
            oldCompCurStamp_ = 1;
        }
        if (++dfsCurStamp_ == INT_MAX) {
            fill(dfsSeenStamp_.begin(), dfsSeenStamp_.end(), 0);
            dfsCurStamp_ = 1;
        }
        oldStamp = oldCompCurStamp_;
        dfsStamp = dfsCurStamp_;
        markOldComponent(oldVerts, removedX, oldStamp);

        if (artifact) *artifact = OwnerSplitArtifact();
        vector<int> visitedOrder;
        visitedOrder.reserve(oldVerts.size());
        vector<vector<int>> localComponents;
        localComponents.reserve(max<int>(1, (int)oldVerts.size() / 8));
        struct Frame { int u, parent, it; };
        vector<Frame> st;
        st.reserve(oldVerts.size());
        int timer = 0;
        long long edgeScan = 0;
        long long visCount = 0;

        const auto& core = core_;
        for (int s : oldVerts) {
            if (!(1 <= s && s <= n_)) continue;
            if (!alive_[s] || s == removedX || oldCompStamp_[s] != oldStamp) continue;
            if (dfsSeenStamp_[s] == dfsStamp) continue;

            localComponents.push_back({});
            int localComp = (int)localComponents.size() - 1;

            dfsSeenStamp_[s] = dfsStamp;
            dfsParent_[s] = s;
            dfsRoot_[s] = s;
            dfsDepth_[s] = 0;
            dfsCompLocal_[s] = localComp;
            dfsChildCount_[s] = 0;
            dfsLowWitnessDesc_[s] = s;
            dfsLowWitnessAnc_[s] = -1;
            dfsTin_[s] = dfsLow_[s] = ++timer;
            visitedOrder.push_back(s);
            localComponents.back().push_back(s);
            st.push_back({s, s, 0});

            while (!st.empty()) {
                Frame &fr = st.back();
                int u = fr.u;
                const auto& inc = core.incidentEdges(u);
                if (fr.it < (int)inc.size()) {
                    int eid = inc[fr.it++];
                    if (!core.edgeAlive(eid)) continue;
                    int v = core.other(eid, u);
                    if (!(1 <= v && v <= n_) || !alive_[v] || v == removedX) continue;
                    if (oldCompStamp_[v] != oldStamp) continue;
                    ++edgeScan;
                    if (dfsSeenStamp_[v] != dfsStamp) {
                        dfsSeenStamp_[v] = dfsStamp;
                        dfsParent_[v] = u;
                        dfsRoot_[v] = dfsRoot_[u];
                        dfsDepth_[v] = dfsDepth_[u] + 1;
                        dfsCompLocal_[v] = localComp;
                        dfsChildCount_[v] = 0;
                        dfsLowWitnessDesc_[v] = v;
                        dfsLowWitnessAnc_[v] = -1;
                        dfsChildCount_[u]++;
                        dfsTin_[v] = dfsLow_[v] = ++timer;
                        visitedOrder.push_back(v);
                        localComponents.back().push_back(v);
                        st.push_back({v, u, 0});
                    } else if (v != fr.parent) {
                        if (dfsTin_[v] < dfsLow_[u]) {
                            dfsLow_[u] = dfsTin_[v];
                            dfsLowWitnessDesc_[u] = u;
                            dfsLowWitnessAnc_[u] = v;
                        }
                    }
                } else {
                    dfsTout_[u] = timer;
                    ++visCount;
                    st.pop_back();
                    if (u != fr.parent && dfsLow_[u] < dfsLow_[fr.parent]) {
                        dfsLow_[fr.parent] = dfsLow_[u];
                        dfsLowWitnessDesc_[fr.parent] = dfsLowWitnessDesc_[u];
                        dfsLowWitnessAnc_[fr.parent] = dfsLowWitnessAnc_[u];
                    }
                }
            }
        }

#ifdef LOCAL
        g_topo_dbg.global_delete_dfs_calls++;
        g_topo_dbg.global_delete_dfs_vertices += visCount;
        g_topo_dbg.global_delete_dfs_edges += edgeScan;
        g_topo_dbg.global_delete_component_count += (long long)localComponents.size();
#endif
        lastDeleteArtifactReady_ = true;

        for (auto& restricted : localComponents) {
            if (restricted.empty()) continue;
            int h = nextComp_++;
            ensureCompCapacity(h);
            compAlive_[h] = true;
            compMembers_[h] = restricted;
            for (int v : restricted) compId_[v] = h;
            newComponents.push_back(h);
        }

        if (artifact) {
            artifact->removedX = removedX;
            artifact->visitedVerts = visitedOrder;
            artifact->parentVals.reserve(visitedOrder.size());
            artifact->rootVals.reserve(visitedOrder.size());
            artifact->depthVals.reserve(visitedOrder.size());
            artifact->tinVals.reserve(visitedOrder.size());
            artifact->toutVals.reserve(visitedOrder.size());
            artifact->lowVals.reserve(visitedOrder.size());
            artifact->compVals.reserve(visitedOrder.size());
            for (int v : visitedOrder) {
                artifact->parentVals.push_back(dfsParent_[v]);
                artifact->rootVals.push_back(dfsRoot_[v]);
                artifact->depthVals.push_back(dfsDepth_[v]);
                artifact->tinVals.push_back(dfsTin_[v]);
                artifact->toutVals.push_back(dfsTout_[v]);
                artifact->lowVals.push_back(dfsLow_[v]);
                artifact->compVals.push_back(compId_[v]);
            }
            artifact->valid = !visitedOrder.empty();
        }
    }

    static bool byIntervalTin(const pair<int,int>& a, const pair<int,int>& b) {
        if (a.first != b.first) return a.first < b.first;
        return a.second < b.second;
    }

    int classifyEndpointBucket(int owner, int endpoint,
                               const vector<pair<int,int>>& childIntervals,
                               bool ownerIsRoot) const {
        if (endpoint == owner) return -1;
        if (!(1 <= endpoint && endpoint <= n_) || !alive_[endpoint]) return -1;
        if (dfsSeenStamp_[owner] != dfsCurStamp_ || dfsSeenStamp_[endpoint] != dfsCurStamp_) return -1;
        if (dfsRoot_[owner] != dfsRoot_[endpoint]) return -1;

        int keyTin = dfsTin_[endpoint];
        int lo = 0, hi = (int)childIntervals.size();
        long long steps = 0;
        while (lo < hi) {
            ++steps;
            int mid = (lo + hi) >> 1;
            if (childIntervals[mid].first <= keyTin) lo = mid + 1;
            else hi = mid;
        }
#ifdef LOCAL
        g_topo_dbg.owner_bucket_binary_search_steps += steps;
#endif
        int idx = lo - 1;
        if (idx >= 0) {
            int child = childIntervals[idx].second;
            if (dfsTin_[child] <= keyTin && keyTin <= dfsTout_[child]) {
                if (ownerIsRoot) return child;
                if (dfsLow_[child] >= dfsTin_[owner]) return child;
            }
        }
        return 0;
    }


    pair<int,int> classifyEndpointDetail(int owner, int endpoint,
                                         const vector<pair<int,int>>& childIntervals,
                                         bool ownerIsRoot) const {
        if (endpoint == owner) return {-1, -1};
        if (!(1 <= endpoint && endpoint <= n_) || !alive_[endpoint]) return {-1, -1};
        if (dfsSeenStamp_[owner] != dfsCurStamp_ || dfsSeenStamp_[endpoint] != dfsCurStamp_) return {-1, -1};
        if (dfsRoot_[owner] != dfsRoot_[endpoint]) return {-1, -1};
        int keyTin = dfsTin_[endpoint];
        int lo = 0, hi = (int)childIntervals.size();
        while (lo < hi) {
            int mid = (lo + hi) >> 1;
            if (childIntervals[mid].first <= keyTin) lo = mid + 1;
            else hi = mid;
        }
        int idx = lo - 1;
        if (idx >= 0) {
            int child = childIntervals[idx].second;
            if (dfsTin_[child] <= keyTin && keyTin <= dfsTout_[child]) {
                if (ownerIsRoot) return {child, child};
                if (dfsLow_[child] >= dfsTin_[owner]) return {child, child};
                return {0, child};
            }
        }
        return {0, -1};
    }

    int nextSupportCollectStamp() const {
        if (++supportCollectCur_ == INT_MAX) {
            fill(supportCollectStamp_.begin(), supportCollectStamp_.end(), 0);
            supportCollectCur_ = 1;
        }
        return supportCollectCur_;
    }

    int nextSupportTreeParentStamp() const {
        if (++supportTreeParentCur_ == INT_MAX) {
            fill(supportTreeParentStamp_.begin(), supportTreeParentStamp_.end(), 0);
            supportTreeParentCur_ = 1;
        }
        return supportTreeParentCur_;
    }

    void collectSupportRootVertex(int v, vector<int>& out, int collectStamp,
                                  int treeParentStamp, long long& chainSteps) const {
        if (!(1 <= v && v <= n_)) return;
        if (supportCollectStamp_[v] != collectStamp) {
            supportCollectStamp_[v] = collectStamp;
            out.push_back(v);
            ++chainSteps;
        }
        supportTreeParentStamp_[v] = treeParentStamp;
        supportTreeParent_[v] = v;
    }

    void ensureSupportTreeParent(int child, int parent, vector<int>& out, int collectStamp,
                                 int treeParentStamp, long long& chainSteps) const {
        if (!(1 <= child && child <= n_) || !(1 <= parent && parent <= n_)) return;
        if (supportCollectStamp_[child] != collectStamp) {
            supportCollectStamp_[child] = collectStamp;
            out.push_back(child);
            ++chainSteps;
            supportTreeParentStamp_[child] = treeParentStamp;
            supportTreeParent_[child] = parent;
        }
    }

    bool buildSimplePathOnLastDeleteTree(int a, int b, vector<int>& path) const {
        path.clear();
        int l = lcaOnLastDeleteTree(a, b);
        if (!(1 <= l && l <= n_)) return false;
        int u = a;
        while (u != l) {
            path.push_back(u);
            int p = dfsParent_[u];
            if (!(1 <= p && p <= n_) || p == u) return false;
            u = p;
        }
        path.push_back(l);
        vector<int> tail;
        u = b;
        while (u != l) {
            tail.push_back(u);
            int p = dfsParent_[u];
            if (!(1 <= p && p <= n_) || p == u) return false;
            u = p;
        }
        reverse(tail.begin(), tail.end());
        path.insert(path.end(), tail.begin(), tail.end());
        return true;
    }

    bool attachPathTowardTargetOnLastDeleteTree(int src, int target, vector<int>& out,
                                                int collectStamp, int treeParentStamp,
                                                long long& chainSteps) const {
        vector<int> path;
        if (!buildSimplePathOnLastDeleteTree(src, target, path)) return false;
        int attachParent = -1;
        for (int i = (int)path.size() - 1; i >= 0; --i) {
            int v = path[i];
            if (supportCollectStamp_[v] == collectStamp) {
                attachParent = v;
                continue;
            }
            if (!(1 <= attachParent && attachParent <= n_)) return false;
            supportCollectStamp_[v] = collectStamp;
            out.push_back(v);
            ++chainSteps;
            supportTreeParentStamp_[v] = treeParentStamp;
            supportTreeParent_[v] = attachParent;
            attachParent = v;
        }
        return true;
    }

    int lcaOnLastDeleteTree(int a, int b) const {
        if (!(1 <= a && a <= n_ && 1 <= b && b <= n_)) return -1;
        if (dfsSeenStamp_[a] != dfsCurStamp_ || dfsSeenStamp_[b] != dfsCurStamp_) return -1;
        if (dfsRoot_[a] != dfsRoot_[b]) return -1;
        while (dfsDepth_[a] > dfsDepth_[b]) {
            int p = dfsParent_[a];
            if (!(1 <= p && p <= n_) || p == a) return -1;
            a = p;
        }
        while (dfsDepth_[b] > dfsDepth_[a]) {
            int p = dfsParent_[b];
            if (!(1 <= p && p <= n_) || p == b) return -1;
            b = p;
        }
        while (a != b) {
            int pa = dfsParent_[a];
            int pb = dfsParent_[b];
            if (!(1 <= pa && pa <= n_) || !(1 <= pb && pb <= n_) || pa == a || pb == b) return -1;
            a = pa;
            b = pb;
        }
        return a;
    }

    int nextUncollectedOnLastDeleteTree(int u, int collectStamp) const {
        supportPathBuf_.clear();
        while (1 <= u && u <= n_) {
            if (supportCollectStamp_[u] != collectStamp) break;
            supportPathBuf_.push_back(u);
            int nxt;
            if (supportJumpStamp_[u] == collectStamp) nxt = supportJump_[u];
            else {
                int p = dfsParent_[u];
                nxt = (1 <= p && p <= n_ && p != u) ? p : 0;
            }
            u = nxt;
        }
        int rep = (1 <= u && u <= n_) ? u : 0;
        for (int v : supportPathBuf_) {
            supportJumpStamp_[v] = collectStamp;
            supportJump_[v] = rep;
        }
        return rep;
    }

    void collectVertexOnLastDeleteTree(int u, vector<int>& out, int collectStamp) const {
        if (!(1 <= u && u <= n_)) return;
        if (supportCollectStamp_[u] == collectStamp) return;
        supportCollectStamp_[u] = collectStamp;
        out.push_back(u);
        int p = dfsParent_[u];
        int nxt = (1 <= p && p <= n_ && p != u) ? nextUncollectedOnLastDeleteTree(p, collectStamp) : 0;
        supportJumpStamp_[u] = collectStamp;
        supportJump_[u] = nxt;
    }

    void collectPathToAncestorOnLastDeleteTree(int u, int anc, vector<int>& out,
                                               int collectStamp, long long& chainSteps) const {
        if (!(1 <= u && u <= n_ && 1 <= anc && anc <= n_)) return;
        if (dfsSeenStamp_[u] != dfsCurStamp_ || dfsSeenStamp_[anc] != dfsCurStamp_) return;
        if (dfsRoot_[u] != dfsRoot_[anc]) return;
        if (dfsDepth_[u] < dfsDepth_[anc]) return;
        while (true) {
            int v = nextUncollectedOnLastDeleteTree(u, collectStamp);
            if (!(1 <= v && v <= n_)) break;
            if (dfsRoot_[v] != dfsRoot_[anc]) break;
            if (dfsDepth_[v] < dfsDepth_[anc]) break;
            if (!(dfsTin_[anc] <= dfsTin_[v] && dfsTin_[v] <= dfsTout_[anc])) break;
            collectVertexOnLastDeleteTree(v, out, collectStamp);
            ++chainSteps;
            if (v == anc) break;
            int p = dfsParent_[v];
            if (!(1 <= p && p <= n_) || p == v) break;
            u = p;
        }
    }

    void collectPathBetweenOnLastDeleteTree(int a, int b, vector<int>& out,
                                            int collectStamp, long long& chainSteps) const {
        int l = lcaOnLastDeleteTree(a, b);
        if (!(1 <= l && l <= n_)) return;
        collectPathToAncestorOnLastDeleteTree(a, l, out, collectStamp, chainSteps);
        collectPathToAncestorOnLastDeleteTree(b, l, out, collectStamp, chainSteps);
    }

    vector<int> buildSupportVerticesFromLastDeleteArtifactImpl(int owner, const vector<int>& relevantVerts,
                                                           long long* outWatchV = nullptr,
                                                           long long* outChainSteps = nullptr) const {
        vector<int> watchVerts;
        if (!lastDeleteArtifactReady_ || relevantVerts.empty()) return watchVerts;
        if (!(1 <= owner && owner <= n_) || !alive_[owner]) return watchVerts;
        if (dfsSeenStamp_[owner] != dfsCurStamp_) return watchVerts;
        if ((int)relevantVerts.size() == 1) {
            watchVerts.push_back(relevantVerts[0]);
            if (outWatchV) *outWatchV = 1;
            if (outChainSteps) *outChainSteps = 1;
            return watchVerts;
        }

        vector<pair<int,int>> childIntervals;
        childIntervals.reserve(core_.incidentEdges(owner).size());
        for (int eid : core_.incidentEdges(owner)) {
            if (!core_.edgeAlive(eid)) continue;
            int v = core_.other(eid, owner);
            if (!(1 <= v && v <= n_) || !alive_[v]) continue;
            if (dfsSeenStamp_[v] != dfsCurStamp_) continue;
            if (dfsParent_[v] == owner) childIntervals.push_back({dfsTin_[v], v});
        }
        sort(childIntervals.begin(), childIntervals.end(), byIntervalTin);
        bool ownerIsRoot = (dfsParent_[owner] == owner);
        auto firstInfo = classifyEndpointDetail(owner, relevantVerts[0], childIntervals, ownerIsRoot);
        int bucket = firstInfo.first;
        if (bucket < 0) return watchVerts;

        const int collectStamp = nextSupportCollectStamp();
        long long chainSteps = 0;

        for (size_t i = 1; i < relevantVerts.size(); ++i) {
            auto info = classifyEndpointDetail(owner, relevantVerts[i], childIntervals, ownerIsRoot);
            if (info.first != bucket) return {};
        }

        if (bucket != 0) {
            int common = relevantVerts[0];
            for (size_t i = 1; i < relevantVerts.size(); ++i) {
                common = lcaOnLastDeleteTree(common, relevantVerts[i]);
                if (!(1 <= common && common <= n_) || common == owner) return {};
            }
            for (int ep : relevantVerts) collectPathToAncestorOnLastDeleteTree(ep, common, watchVerts, collectStamp, chainSteps);
        } else {
            if (ownerIsRoot) return {};
            int anchor = -1;
            vector<int> ancestorSide;
            unordered_map<int, vector<int>> childEps;
            childEps.reserve(relevantVerts.size() * 2 + 1);
            for (int ep : relevantVerts) {
                auto info = classifyEndpointDetail(owner, ep, childIntervals, ownerIsRoot);
                if (info.first != 0) return {};
                int child = info.second;
                if (child == -1) {
                    ancestorSide.push_back(ep);
                    if (anchor == -1) anchor = ep;
                } else {
                    if (dfsLow_[child] >= dfsTin_[owner]) return {};
                    childEps[child].push_back(ep);
                    int anc = dfsLowWitnessAnc_[child];
                    if (anchor == -1 && 1 <= anc && anc <= n_ && anc != owner) anchor = anc;
                }
            }
            if (!(1 <= anchor && anchor <= n_) || anchor == owner) return {};
            collectPathToAncestorOnLastDeleteTree(anchor, anchor, watchVerts, collectStamp, chainSteps);
            for (int ep : ancestorSide) collectPathBetweenOnLastDeleteTree(ep, anchor, watchVerts, collectStamp, chainSteps);
            for (const auto& kv : childEps) {
                int child = kv.first;
                int desc = dfsLowWitnessDesc_[child];
                int anc = dfsLowWitnessAnc_[child];
                if (!(1 <= desc && desc <= n_ && 1 <= anc && anc <= n_)) return {};
                if (anc == owner) return {};
                collectPathBetweenOnLastDeleteTree(anc, anchor, watchVerts, collectStamp, chainSteps);
                for (int ep : kv.second) {
                    int l = lcaOnLastDeleteTree(ep, desc);
                    if (!(1 <= l && l <= n_)) return {};
                    if (!(dfsTin_[child] <= dfsTin_[l] && dfsTin_[l] <= dfsTout_[child])) return {};
                    collectPathToAncestorOnLastDeleteTree(ep, l, watchVerts, collectStamp, chainSteps);
                    collectPathToAncestorOnLastDeleteTree(desc, l, watchVerts, collectStamp, chainSteps);
                }
            }
        }

        if (outWatchV) *outWatchV = (long long)watchVerts.size();
        if (outChainSteps) *outChainSteps = chainSteps;
        return watchVerts;
    }

    void updateOwnerFromGlobalArtifact(int owner, int removedX) const {
#ifdef LOCAL
        g_topo_dbg.dbg_owner_local_updates++;
        g_topo_dbg.owner_relabel_calls++;
        g_topo_dbg.owner_wide_relabel_calls++;
#endif
        if (owner < 1 || owner > n_ || !alive_[owner]) {
            if (1 <= owner && owner <= n_) applyOwnerPartition(owner, {});
            ownerLastRemoved_[owner] = removedX;
            return;
        }
        const auto oldMap = endpointClass_[owner];
        unordered_map<int, vector<int>> oldBuckets;
        int maxOldId = -1;
        for (const auto& kv : oldMap) {
            oldBuckets[kv.second].push_back(kv.first);
            maxOldId = max(maxOldId, kv.second);
        }
        nextClassId_[owner] = max(nextClassId_[owner], maxOldId + 1);

        vector<pair<int,int>> childIntervals;
        childIntervals.reserve(core_.incidentEdges(owner).size());
        for (int eid : core_.incidentEdges(owner)) {
            if (!core_.edgeAlive(eid)) continue;
            int v = core_.other(eid, owner);
            if (!(1 <= v && v <= n_) || !alive_[v] || v == removedX) continue;
            if (dfsSeenStamp_[v] != dfsCurStamp_) continue;
            if (dfsParent_[v] == owner) childIntervals.push_back({dfsTin_[v], v});
        }
        sort(childIntervals.begin(), childIntervals.end(), byIntervalTin);
        bool ownerIsRoot = (dfsSeenStamp_[owner] == dfsCurStamp_ && dfsParent_[owner] == owner);

        unordered_map<int, unordered_map<int, vector<int>>> partsByOld;
        long long relabelAlive = 0;
        long long relabelMoved = 0;
        int candidateClasses = 0;

        for (const auto& [oldId, vec] : oldBuckets) {
            for (int ep : vec) {
                int eid = core_.edgeIdOf(owner, ep);
                if (ep == removedX || eid == -1 || !alive_[ep] || !core_.edgeAlive(eid)) continue;
                int bucket = classifyEndpointBucket(owner, ep, childIntervals, ownerIsRoot);
                partsByOld[oldId][bucket].push_back(ep);
                ++relabelAlive;
#ifdef LOCAL
                g_topo_dbg.owner_bucket_assignments++;
#endif
            }
        }

        unordered_map<int,int> newMap;
        newMap.reserve(oldMap.size() + 4);
        for (const auto& [oldId, zoneMap] : partsByOld) {
            vector<pair<int, vector<int>>> parts;
            parts.reserve(zoneMap.size());
            for (const auto& kv : zoneMap) parts.push_back({kv.first, kv.second});
            sort(parts.begin(), parts.end(), [&](const auto& A, const auto& B){
                if (A.second.size() != B.second.size()) return A.second.size() > B.second.size();
                return A.first < B.first;
            });
            bool reused = false;
            for (auto& [bucket, vec] : parts) {
                int useId = reused ? nextClassId_[owner]++ : oldId;
                reused = true;
                ++candidateClasses;
                for (int ep : vec) {
                    newMap[ep] = useId;
                    auto itOld = oldMap.find(ep);
                    int oldCid = (itOld == oldMap.end()) ? -1 : itOld->second;
                    if (oldCid != useId) ++relabelMoved;
                }
            }
        }

        applyOwnerPartition(owner, std::move(newMap));
        ownerLastRemoved_[owner] = removedX;
#ifdef LOCAL
        g_topo_dbg.owner_relabel_active_endpoints += relabelAlive;
        g_topo_dbg.owner_relabel_moved_endpoints += relabelMoved;
        g_topo_dbg.owner_relabel_candidate_classes += candidateClasses;
        g_topo_dbg.owner_wide_relabel_endpoints += relabelAlive;
        auto exact = computeOwnerExactMap(owner);
        auto canon = [](const unordered_map<int,int>& mp){
            unordered_map<int,int> rem; int nxt = 0; vector<pair<int,int>> out; out.reserve(mp.size());
            vector<pair<int,int>> kv(mp.begin(), mp.end()); sort(kv.begin(), kv.end());
            for (auto [k,v] : kv) {
                auto it = rem.find(v);
                if (it == rem.end()) it = rem.emplace(v, nxt++).first;
                out.push_back({k, it->second});
            }
            return out;
        };
        if (canon(endpointClass_[owner]) != canon(exact)) {
            g_topo_dbg.dbg_endpoint_partition_mismatch++;
            applyOwnerPartition(owner, std::move(exact));
        }
#endif
    }

public:
    struct OwnerBucketContext {
        bool valid = false;
        bool ownerIsRoot = false;
        vector<pair<int,int>> childIntervals;
    };

    struct SupportTreeBuild {
        vector<int> watchVerts;
        vector<int> parentVertex;
        int rootVertex = -1;
    };

    OwnerBucketContext buildOwnerBucketContext(int owner) const {
        OwnerBucketContext ctx;
        if (!(1 <= owner && owner <= n_) || !alive_[owner] || !lastDeleteArtifactReady_) return ctx;
        if (dfsSeenStamp_[owner] != dfsCurStamp_) return ctx;
        ctx.ownerIsRoot = (dfsParent_[owner] == owner);
        ctx.childIntervals.reserve(core_.incidentEdges(owner).size());
        for (int eid : core_.incidentEdges(owner)) {
            if (!core_.edgeAlive(eid)) continue;
            int v = core_.other(eid, owner);
            if (!(1 <= v && v <= n_) || !alive_[v]) continue;
            if (dfsSeenStamp_[v] != dfsCurStamp_) continue;
            if (dfsParent_[v] == owner) ctx.childIntervals.push_back({dfsTin_[v], v});
        }
        sort(ctx.childIntervals.begin(), ctx.childIntervals.end(), byIntervalTin);
        ctx.valid = true;
        return ctx;
    }

    int classifyEndpointBucketWithContext(int owner, int endpoint, const OwnerBucketContext& ctx) const {
        if (!ctx.valid) return -1;
        return classifyEndpointBucket(owner, endpoint, ctx.childIntervals, ctx.ownerIsRoot);
    }

    int classifyEndpointBucketFromLastDeleteArtifact(int owner, int endpoint) const {
        auto ctx = buildOwnerBucketContext(owner);
        return classifyEndpointBucketWithContext(owner, endpoint, ctx);
    }

    int allocateFreshClassId(int owner) const {
        if (!(1 <= owner && owner <= n_)) return -1;
        return nextClassId_[owner]++;
    }

    void assignEndpointClass(int owner, int endpoint, int cid) const {
        if (!(1 <= owner && owner <= n_) || cid < 0) return;
        endpointClass_[owner][endpoint] = cid;
    }

    vector<int> buildSupportVerticesFromLastDeleteArtifact(int owner, const vector<int>& relevantVerts,
                                                           long long* outWatchV = nullptr,
                                                           long long* outChainSteps = nullptr) const {
        return buildSupportVerticesFromLastDeleteArtifactImpl(owner, relevantVerts, outWatchV, outChainSteps);
    }
    SupportTreeBuild buildSupportTreeFromLastDeleteArtifact(int owner, const vector<int>& relevantVerts,
                                                            long long* outWatchV = nullptr,
                                                            long long* outChainSteps = nullptr) const {
        SupportTreeBuild res;
        if (!lastDeleteArtifactReady_ || relevantVerts.empty()) return res;
        if (!(1 <= owner && owner <= n_) || !alive_[owner]) return res;
        if (dfsSeenStamp_[owner] != dfsCurStamp_) return res;

        if ((int)relevantVerts.size() == 1) {
            res.watchVerts.push_back(relevantVerts[0]);
            res.parentVertex.push_back(relevantVerts[0]);
            res.rootVertex = relevantVerts[0];
            if (outWatchV) *outWatchV = 1;
            if (outChainSteps) *outChainSteps = 1;
            return res;
        }

        vector<pair<int,int>> childIntervals;
        childIntervals.reserve(core_.incidentEdges(owner).size());
        for (int eid : core_.incidentEdges(owner)) {
            if (!core_.edgeAlive(eid)) continue;
            int v = core_.other(eid, owner);
            if (!(1 <= v && v <= n_) || !alive_[v]) continue;
            if (dfsSeenStamp_[v] != dfsCurStamp_) continue;
            if (dfsParent_[v] == owner) childIntervals.push_back({dfsTin_[v], v});
        }
        sort(childIntervals.begin(), childIntervals.end(), byIntervalTin);
        bool ownerIsRoot = (dfsParent_[owner] == owner);
        auto firstInfo = classifyEndpointDetail(owner, relevantVerts[0], childIntervals, ownerIsRoot);
        int bucket = firstInfo.first;
        if (bucket < 0) return res;
        for (size_t i = 1; i < relevantVerts.size(); ++i) {
            auto info = classifyEndpointDetail(owner, relevantVerts[i], childIntervals, ownerIsRoot);
            if (info.first != bucket) return res;
        }

        const int collectStamp = nextSupportCollectStamp();
        const int treeParentStamp = nextSupportTreeParentStamp();
        long long chainSteps = 0;

        if (bucket != 0) {
            int common = relevantVerts[0];
            for (size_t i = 1; i < relevantVerts.size(); ++i) {
                common = lcaOnLastDeleteTree(common, relevantVerts[i]);
                if (!(1 <= common && common <= n_) || common == owner) return SupportTreeBuild();
            }
            collectSupportRootVertex(common, res.watchVerts, collectStamp, treeParentStamp, chainSteps);
            for (int ep : relevantVerts) {
                if (!attachPathTowardTargetOnLastDeleteTree(ep, common, res.watchVerts, collectStamp, treeParentStamp, chainSteps)) {
                    return SupportTreeBuild();
                }
            }
            res.rootVertex = common;
        } else {
            if (ownerIsRoot) return res;
            int anchor = -1;
            vector<int> ancestorSide;
            unordered_map<int, vector<int>> childEps;
            childEps.reserve(relevantVerts.size() * 2 + 1);
            for (int ep : relevantVerts) {
                auto info = classifyEndpointDetail(owner, ep, childIntervals, ownerIsRoot);
                if (info.first != 0) return SupportTreeBuild();
                int child = info.second;
                if (child == -1) {
                    ancestorSide.push_back(ep);
                    if (anchor == -1) anchor = ep;
                } else {
                    if (dfsLow_[child] >= dfsTin_[owner]) return SupportTreeBuild();
                    childEps[child].push_back(ep);
                    int anc = dfsLowWitnessAnc_[child];
                    if (anchor == -1 && 1 <= anc && anc <= n_ && anc != owner) anchor = anc;
                }
            }
            if (!(1 <= anchor && anchor <= n_) || anchor == owner) return SupportTreeBuild();
            collectSupportRootVertex(anchor, res.watchVerts, collectStamp, treeParentStamp, chainSteps);
            for (int ep : ancestorSide) {
                if (!attachPathTowardTargetOnLastDeleteTree(ep, anchor, res.watchVerts, collectStamp, treeParentStamp, chainSteps)) {
                    return SupportTreeBuild();
                }
            }
            for (const auto& kv : childEps) {
                int child = kv.first;
                int desc = dfsLowWitnessDesc_[child];
                int anc = dfsLowWitnessAnc_[child];
                if (!(1 <= desc && desc <= n_ && 1 <= anc && anc <= n_) || anc == owner) return SupportTreeBuild();
                if (!attachPathTowardTargetOnLastDeleteTree(anc, anchor, res.watchVerts, collectStamp, treeParentStamp, chainSteps)) {
                    return SupportTreeBuild();
                }
                ensureSupportTreeParent(desc, anc, res.watchVerts, collectStamp, treeParentStamp, chainSteps);
                for (int ep : kv.second) {
                    if (!attachPathTowardTargetOnLastDeleteTree(ep, desc, res.watchVerts, collectStamp, treeParentStamp, chainSteps)) {
                        return SupportTreeBuild();
                    }
                }
            }
            res.rootVertex = anchor;
        }

        res.parentVertex.reserve(res.watchVerts.size());
        for (int v : res.watchVerts) {
            if (supportTreeParentStamp_[v] != treeParentStamp) return SupportTreeBuild();
            res.parentVertex.push_back(supportTreeParent_[v]);
        }
        if (outWatchV) *outWatchV = (long long)res.watchVerts.size();
        if (outChainSteps) *outChainSteps = chainSteps;
        return res;
    }
    bool hasLastDeleteArtifact() const { return lastDeleteArtifactReady_; }
    int lastDeleteParentVertex(int v) const {
        if (!lastDeleteArtifactReady_ || !(1 <= v && v <= n_)) return -1;
        if (dfsSeenStamp_[v] != dfsCurStamp_) return -1;
        return dfsParent_[v];
    }

    struct ClassLocalRefineResult {
        int keptCid = -1;
        vector<pair<int,int>> endpointToNewCid;
        vector<int> movedEndpoints;
        vector<int> candidateCids;
    };

    void restrictOwnerToActiveEndpoints(int owner, const vector<int>& activeEndpoints) const {
        if (!(1 <= owner && owner <= n_)) return;
        if (!alive_[owner]) {
            applyOwnerPartition(owner, {});
            return;
        }
        unordered_map<int,int> filtered;
        filtered.reserve(activeEndpoints.size() * 2 + 1);
        for (int ep : activeEndpoints) {
            auto it = endpointClass_[owner].find(ep);
            if (it != endpointClass_[owner].end()) filtered.emplace(ep, it->second);
        }
        applyOwnerPartition(owner, std::move(filtered));
    }

    void deactivateEndpoint(int owner, int endpoint) const {
        if (!(1 <= owner && owner <= n_)) return;
        auto it = endpointClass_[owner].find(endpoint);
        if (it == endpointClass_[owner].end()) return;
        endpointClass_[owner].erase(it);
        if (topoActiveEndpointTotal_ > 0) --topoActiveEndpointTotal_;
#ifdef LOCAL
        g_topo_dbg.topo_deactivated_endpoint_count++;
        g_topo_dbg.topo_active_endpoint_total = topoActiveEndpointTotal_;
#endif
    }

    ClassLocalRefineResult refineTouchedClassFromLastDeleteArtifact(int owner, int oldCid,
                                                                    const vector<int>& relevantVerts) const {
        ClassLocalRefineResult res;
        res.candidateCids.push_back(oldCid);
#ifdef LOCAL
        g_topo_dbg.class_local_refine_calls++;
#endif
        if (!(1 <= owner && owner <= n_) || !alive_[owner] || oldCid < 0) return res;
        if (!lastDeleteArtifactReady_) return res;
        vector<pair<int,int>> childIntervals;
        childIntervals.reserve(core_.incidentEdges(owner).size());
        for (int eid : core_.incidentEdges(owner)) {
            if (!core_.edgeAlive(eid)) continue;
            int v = core_.other(eid, owner);
            if (!(1 <= v && v <= n_) || !alive_[v]) continue;
            if (dfsSeenStamp_[v] != dfsCurStamp_) continue;
            if (dfsParent_[v] == owner) childIntervals.push_back({dfsTin_[v], v});
        }
        sort(childIntervals.begin(), childIntervals.end(), byIntervalTin);
        bool ownerIsRoot = (dfsSeenStamp_[owner] == dfsCurStamp_ && dfsParent_[owner] == owner);

        unordered_map<int, vector<int>> parts;
        parts.reserve(relevantVerts.size() * 2 + 1);
        vector<int> gone;
        gone.reserve(relevantVerts.size());
        long long refineEndpoints = 0;
        long long movedEndpoints = 0;
        int newClassCount = 0;
        int keptOld = 0;

        for (int ep : relevantVerts) {
            auto itOld = endpointClass_[owner].find(ep);
            if (itOld == endpointClass_[owner].end() || itOld->second != oldCid) continue;
            ++refineEndpoints;
#ifdef LOCAL
            g_topo_dbg.owner_bucket_assignments++;
#endif
            int eid = core_.edgeIdOf(owner, ep);
            if (ep == owner || eid == -1 || !alive_[ep] || !core_.edgeAlive(eid)) {
                gone.push_back(ep);
                continue;
            }
            int bucket = classifyEndpointBucket(owner, ep, childIntervals, ownerIsRoot);
            if (bucket < 0) {
                gone.push_back(ep);
                continue;
            }
            parts[bucket].push_back(ep);
        }

        for (int ep : gone) {
            endpointClass_[owner].erase(ep);
            if (topoActiveEndpointTotal_ > 0) --topoActiveEndpointTotal_;
#ifdef LOCAL
            g_topo_dbg.topo_deactivated_endpoint_count++;
#endif
            res.endpointToNewCid.push_back({ep, -1});
            res.movedEndpoints.push_back(ep);
            ++movedEndpoints;
        }

        vector<pair<int, vector<int>>> buckets;
        buckets.reserve(parts.size());
        for (auto& kv : parts) buckets.push_back({kv.first, std::move(kv.second)});
        sort(buckets.begin(), buckets.end(), [&](const auto& A, const auto& B){
            if (A.second.size() != B.second.size()) return A.second.size() > B.second.size();
            return A.first < B.first;
        });
        bool reusedOld = false;
        for (auto& kv : buckets) {
            int useCid = reusedOld ? nextClassId_[owner]++ : oldCid;
            if (!reusedOld) {
                res.keptCid = useCid;
                keptOld = 1;
                reusedOld = true;
            } else {
                ++newClassCount;
            }
            res.candidateCids.push_back(useCid);
            for (int ep : kv.second) {
                auto itOld = endpointClass_[owner].find(ep);
                int prev = (itOld == endpointClass_[owner].end()) ? -1 : itOld->second;
                endpointClass_[owner][ep] = useCid;
                res.endpointToNewCid.push_back({ep, useCid});
                if (prev != useCid) {
                    res.movedEndpoints.push_back(ep);
                    ++movedEndpoints;
                }
            }
        }
        sort(res.candidateCids.begin(), res.candidateCids.end());
        res.candidateCids.erase(unique(res.candidateCids.begin(), res.candidateCids.end()), res.candidateCids.end());
#ifdef LOCAL
        g_topo_dbg.class_local_refine_endpoints += refineEndpoints;
        g_topo_dbg.class_local_refine_moved_endpoints += movedEndpoints;
        g_topo_dbg.class_local_new_class_count += newClassCount;
        g_topo_dbg.class_local_kept_old_cid_count += keptOld;
        g_topo_dbg.topo_active_endpoint_total = topoActiveEndpointTotal_;
        g_topo_dbg.topo_active_endpoint_peak = max<long long>(g_topo_dbg.topo_active_endpoint_peak, topoActiveEndpointTotal_);
#endif
        return res;
    }

    void init(int n, const vector<pair<int,int>>& edges, const vector<BranchQuery>& branchQueries) {
        n_ = n;
        core_.init(n_);
        for (auto [u, v] : edges) core_.addEdge(u, v);
        alive_.assign(n_ + 1, true);
        rebuildAllComponents();
        ownerEndpoints_.assign(n_ + 1, {});
        for (const auto& q : branchQueries) {
            ownerEndpoints_[q.owner].push_back(q.a);
            ownerEndpoints_[q.owner].push_back(q.b);
        }
        for (int v = 1; v <= n_; ++v) {
            sort(ownerEndpoints_[v].begin(), ownerEndpoints_[v].end());
            ownerEndpoints_[v].erase(unique(ownerEndpoints_[v].begin(), ownerEndpoints_[v].end()), ownerEndpoints_[v].end());
        }
        ownerDirty_.assign(n_ + 1, false);
        endpointClass_.assign(n_ + 1, {});
        classEndpoints_.assign(n_ + 1, {});
        classRep_.assign(n_ + 1, {});
        classTouchedByRemoved_.assign(n_ + 1, {});
        endpointWitnessZone_.assign(n_ + 1, {});
        nextClassId_.assign(n_ + 1, 0);
        ownerLastRemoved_.assign(n_ + 1, -1);
        oldCompStamp_.assign(n_ + 1, 0);
        oldCompCurStamp_ = 1;
        dfsSeenStamp_.assign(n_ + 1, 0);
        dfsCurStamp_ = 1;
        dfsTin_.assign(n_ + 1, 0);
        dfsTout_.assign(n_ + 1, 0);
        dfsLow_.assign(n_ + 1, 0);
        dfsParent_.assign(n_ + 1, -1);
        dfsRoot_.assign(n_ + 1, -1);
        dfsDepth_.assign(n_ + 1, 0);
        dfsCompLocal_.assign(n_ + 1, -1);
        dfsChildCount_.assign(n_ + 1, 0);
        dfsLowWitnessDesc_.assign(n_ + 1, -1);
        dfsLowWitnessAnc_.assign(n_ + 1, -1);
        supportCollectStamp_.assign(n_ + 1, 0);
        supportJumpStamp_.assign(n_ + 1, 0);
        supportJump_.assign(n_ + 1, 0);
        supportTreeParentStamp_.assign(n_ + 1, 0);
        supportTreeParent_.assign(n_ + 1, -1);
        supportPathBuf_.clear();
        supportCollectCur_ = 1;
        supportTreeParentCur_ = 1;
        lastDeleteArtifactReady_ = false;
        topoActiveEndpointTotal_ = 0;
#ifdef LOCAL
        g_topo_dbg.topo_active_endpoint_total = 0;
        g_topo_dbg.topo_active_endpoint_peak = 0;
        g_topo_dbg.topo_deactivated_endpoint_count = 0;
#endif
#ifdef LOCAL
        g_topo_dbg = TopologyDebugStats();
        g_topo_dbg.dbg_owner_rebuild_by_owner.assign(n_ + 1, 0);
#endif
        for (int owner = 1; owner <= n_; ++owner) initializeOwnerExact(owner);
    }
    const DynamicForestCoreHDT& core() const { return core_; }
    bool aliveVertex(int v) const { return 1 <= v && v <= n_ && alive_[v]; }
    int componentOf(int v) const { return (1 <= v && v <= n_ && alive_[v]) ? compId_[v] : -1; }
    vector<int> listComponents() const {
        vector<int> out;
        for (int h = 0; h < (int)compAlive_.size(); ++h) if (compAlive_[h]) out.push_back(h);
        return out;
    }
    void markOwnersDirty(const vector<int>& owners) {
        for (int owner : owners) if (1 <= owner && owner <= n_) ownerDirty_[owner] = false;
    }
    int incidentClass(int owner, int endpoint) const {
        if (owner < 1 || owner > n_ || !alive_[owner]) return -1;
        if (endpoint < 1 || endpoint > n_ || !alive_[endpoint]) return -1;
#ifdef LOCAL
        if (ownerDirty_[owner]) abort();
#endif
        auto it = endpointClass_[owner].find(endpoint);
        return it == endpointClass_[owner].end() ? -1 : it->second;
    }
    bool ownerPairConnected(int owner, int a, int b) const {
        int ca = incidentClass(owner, a);
        int cb = incidentClass(owner, b);
        return ca >= 0 && ca == cb;
    }
    void deleteVertexAndSplit(int x, const vector<int>& touchedOwners, vector<int>& newComponents,
                              OwnerSplitArtifact* artifact = nullptr) {
        (void)touchedOwners;
        newComponents.clear();
        if (artifact) *artifact = OwnerSplitArtifact();
        if (!aliveVertex(x)) return;
        int oldComp = compId_[x];
        vector<int> oldVerts;
        if (oldComp >= 0 && oldComp < (int)compMembers_.size()) oldVerts = compMembers_[oldComp];
        core_.deleteVertexBatch(x);
        alive_[x] = false;
        if (oldComp >= 0 && oldComp < (int)compAlive_.size()) {
            compAlive_[oldComp] = false;
            for (int v : oldVerts) if (1 <= v && v <= n_) compId_[v] = -1;
            compMembers_[oldComp].clear();
        }

        int oldStamp = 0, dfsStamp = 0;
#ifdef LOCAL
        {
            ScopedNsAcc __timer(ptr_if(local_profile_coarse_enabled(), &g_batch_dbg.time_global_delete_dfs_ns),
                               ptr_if(local_profile_coarse_enabled(), &g_batch_dbg.time_global_delete_dfs_calls));
            buildGlobalDeleteArtifact(x, oldVerts, newComponents, artifact, oldStamp, dfsStamp);
        }
#else
        buildGlobalDeleteArtifact(x, oldVerts, newComponents, artifact, oldStamp, dfsStamp);
#endif
    }
};




class PotentialHandleKernel {
    DecrementalNBTopology* topo_;

    // ===== Explicit decomposition tree API (BC-tree based; SPQR seam left as future work) =====
    enum DecompKind { BC_BLOCKCUT_LATTICE = 0, SPQR_SEAM = 1 };
    struct DecompNode {
        int id = -1;
        int kind = BC_BLOCKCUT_LATTICE;
        vector<int> verts;        // region vertices (closed under BC-tree path)
        vector<int> edges;        // optional: internal edge ids (may be empty)
        vector<int> children;     // explicit lattice children (stored)
        int parent = -1;
        vector<int> boundaryVerts; // articulation / boundary vertices (optional)
        int budgetExp = 0;
    };
    struct DecompTree {
        vector<DecompNode> nodes;
        int root = -1;
    };

    // Base regions are the closed decomposition regions; lattice nodes are (baseId,budget) pairs.
    struct BaseRegion {
        vector<int> regionVerts;
        vector<int> childBaseIds; // explicit BC-tree pruned child regions (strict subset when possible)
        vector<int> boundaryVerts;
        bool built = false;
        uint64_t key = 0;
    };
    struct LatticeNode {
        int baseId = -1;
        int budgetExp = 0;
        vector<int> children; // lattice node ids
    };

    mutable vector<BaseRegion> base_;
    mutable unordered_map<uint64_t,int> baseKeyToId_;
    mutable vector<LatticeNode> lattice_;
    mutable unordered_map<long long,int> latticeKeyToId_;
    mutable DecompTree decomp_; // mirrors lattice_ for explicit decomposition node API

    struct RestrictedSearchScratch {
        mutable vector<int> allowStamp;
        mutable vector<int> seenStamp;
        mutable vector<int> parent;
        mutable vector<int> parentEdge;
        mutable vector<int> queueBuf;
        mutable int allowCur = 1;
        mutable int seenCur = 1;
        void ensure(int n) const {
            int need = n + 1;
            if ((int)allowStamp.size() < need) {
                allowStamp.assign(need, 0);
                seenStamp.assign(need, 0);
                parent.assign(need, -1);
                parentEdge.assign(need, -1);
            }
        }
        void nextAllow() const {
            if (++allowCur == INT_MAX) {
                fill(allowStamp.begin(), allowStamp.end(), 0);
                allowCur = 1;
            }
        }
        void nextSeen() const {
            if (++seenCur == INT_MAX) {
                fill(seenStamp.begin(), seenStamp.end(), 0);
                seenCur = 1;
            }
        }
    };

    mutable RestrictedSearchScratch restrictedScratch_;

    PotentialHandleManager::Handle buildRestrictedFromSortedRegion(int owner, int a, int b,
                                                                   const vector<int>& sortedRegion,
                                                                   int removedV,
                                                                   long long* outVisitedV = nullptr,
                                                                   long long* outVisitedE = nullptr) const {
        PotentialHandleManager::Handle out;
        out.owner = owner;
        out.a = a;
        out.b = b;
#ifdef LOCAL
        g_strict_child_dbg.fast_restricted_search_calls++;
#endif
        if (owner < 1 || !topo_->aliveVertex(owner)) return out;
        const auto& core = topo_->core();
        restrictedScratch_.ensure(core.maxVertexId());
        restrictedScratch_.nextAllow();
        restrictedScratch_.nextSeen();
        const int allowStamp = restrictedScratch_.allowCur;
        const int seenStamp = restrictedScratch_.seenCur;

        bool hasOwner = false;
        bool hasA = false;
        bool hasB = false;
        out.regionVerts.clear();
        out.regionVerts.reserve(sortedRegion.size());
        for (int v : sortedRegion) {
            if (v == removedV) continue;
            if (v == owner) {
                hasOwner = true;
                out.regionVerts.push_back(v);
                restrictedScratch_.allowStamp[v] = allowStamp;
                continue;
            }
            if (!topo_->aliveVertex(v)) continue;
            out.regionVerts.push_back(v);
            restrictedScratch_.allowStamp[v] = allowStamp;
            if (v == a) hasA = true;
            if (v == b) hasB = true;
        }
        if (!hasOwner) {
            auto it = lower_bound(out.regionVerts.begin(), out.regionVerts.end(), owner);
            out.regionVerts.insert(it, owner);
            restrictedScratch_.allowStamp[owner] = allowStamp;
            hasOwner = true;
        }
        if (!hasA || !hasB) return out;

        auto& q = restrictedScratch_.queueBuf;
        q.clear();
        q.push_back(a);
        restrictedScratch_.seenStamp[a] = seenStamp;
        restrictedScratch_.parent[a] = a;
        restrictedScratch_.parentEdge[a] = -1;
        long long visV = 0;
        long long visE = 0;
        for (size_t qi = 0; qi < q.size(); ++qi) {
            int u = q[qi];
            ++visV;
            if (u == b) break;
            for (int eid : core.incidentEdges(u)) {
                if (!core.edgeAlive(eid)) continue;
                ++visE;
                int v = core.other(eid, u);
                if (v == owner || v == removedV || !topo_->aliveVertex(v)) continue;
                if (restrictedScratch_.allowStamp[v] != allowStamp) continue;
                if (restrictedScratch_.seenStamp[v] == seenStamp) continue;
                restrictedScratch_.seenStamp[v] = seenStamp;
                restrictedScratch_.parent[v] = u;
                restrictedScratch_.parentEdge[v] = eid;
                q.push_back(v);
            }
        }
        if (outVisitedV) *outVisitedV = visV;
        if (outVisitedE) *outVisitedE = visE;
#ifdef LOCAL
        g_strict_child_dbg.fast_restricted_search_vertices += visV;
        g_strict_child_dbg.fast_restricted_search_edges += visE;
#endif
        if (restrictedScratch_.seenStamp[b] != seenStamp) return {};

        out.certVerts.clear();
        out.certEdges.clear();
        int cur = b;
        while (true) {
            out.certVerts.push_back(cur);
            if (cur == a) break;
            out.certEdges.push_back(restrictedScratch_.parentEdge[cur]);
            cur = restrictedScratch_.parent[cur];
        }
        out.certVerts.push_back(owner);
        sort(out.certVerts.begin(), out.certVerts.end());
        out.certVerts.erase(unique(out.certVerts.begin(), out.certVerts.end()), out.certVerts.end());
        sort(out.certEdges.begin(), out.certEdges.end());
        out.certEdges.erase(unique(out.certEdges.begin(), out.certEdges.end()), out.certEdges.end());
        return out;
    }

    vector<int> compactRegionFromCertificate(const PotentialHandleManager::Handle& h, int removedV = -1) const {
        auto compact = buildClosedHandleFromWitness(h.owner, h.a, h.b, h.certVerts, h.certEdges);
        return normalizeRegion(std::move(compact), h.owner, removedV);
    }

    PotentialHandleManager::Handle maybeCompactHandle(PotentialHandleManager::Handle h, int removedV = -1) const {
        if (!PotentialHandleManager::valid(h)) return h;
        auto compact = compactRegionFromCertificate(h, removedV);
        if (!compact.empty() && compact.size() < h.regionVerts.size()) h.regionVerts = std::move(compact);
        return h;
    }

    void recordRelocationStats(const PotentialHandleManager::Handle& before,
                               const PotentialHandleManager::Handle& after) const {
#ifdef LOCAL
        g_strict_child_dbg.region_stats_count++;
        g_strict_child_dbg.region_size_before_sum += (long long)before.regionVerts.size();
        g_strict_child_dbg.region_size_after_sum += (long long)after.regionVerts.size();
        g_strict_child_dbg.cert_size_before_sum += (long long)before.certVerts.size();
        g_strict_child_dbg.cert_size_after_sum += (long long)after.certVerts.size();
        if (after.regionVerts.size() < before.regionVerts.size()) g_strict_child_dbg.proper_child_relocation_count++;
        else g_strict_child_dbg.same_base_relocation_count++;
#endif
    }

    static uint64_t splitmix64(uint64_t x){
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    }

    static vector<int> normalizeRegion(vector<int> region, int owner, int removedV) {
        sort(region.begin(), region.end());
        region.erase(unique(region.begin(), region.end()), region.end());
        if (removedV != -1) region.erase(remove(region.begin(), region.end(), removedV), region.end());
        if (!binary_search(region.begin(), region.end(), owner)) region.push_back(owner);
        sort(region.begin(), region.end());
        return region;
    }

    static uint64_t hashRegion(const vector<int>& region){
        uint64_t h = 1469598103934665603ULL;
        for(int v: region){ h ^= (uint64_t)(unsigned)v + 0x9e3779b97f4a7c15ULL; h *= 1099511628211ULL; }
        return h;
    }

    int getOrCreateBaseRegion(vector<int> region, int owner) const {
        region = normalizeRegion(std::move(region), owner, -1);
        uint64_t key = hashRegion(region);
        for (int probe = 0; probe < 4; ++probe) {
            auto it = baseKeyToId_.find(key);
            if (it == baseKeyToId_.end()) {
                int id = (int)base_.size();
                BaseRegion bn; bn.regionVerts = std::move(region); bn.key = key;
                base_.push_back(std::move(bn));
                baseKeyToId_[key] = id;
                return id;
            }
            int id = it->second;
            if (base_[id].regionVerts == region) return id;
            key = splitmix64(key + 0x123456789abcdefULL);
        }
        key = splitmix64(key ^ (uint64_t)base_.size());
        int id = (int)base_.size();
        BaseRegion bn; bn.regionVerts = std::move(region); bn.key = key;
        base_.push_back(std::move(bn));
        baseKeyToId_[key] = id;
        return id;
    }

    int getOrCreateLatticeNode(int baseId, int budgetExp) const {
        long long key = ((long long)baseId << 32) ^ (unsigned)budgetExp;
        auto it = latticeKeyToId_.find(key);
        if (it != latticeKeyToId_.end()) return it->second;
        int id = (int)lattice_.size();
        LatticeNode ln; ln.baseId = baseId; ln.budgetExp = budgetExp;
        lattice_.push_back(std::move(ln));
        latticeKeyToId_[key] = id;
        // mirror as explicit decomposition node
        DecompNode dn; dn.id = id; dn.kind = BC_BLOCKCUT_LATTICE; dn.budgetExp = budgetExp; dn.parent = -1;
        dn.verts = base_[baseId].regionVerts;
        dn.boundaryVerts = base_[baseId].boundaryVerts;
        decomp_.nodes.push_back(std::move(dn));
        if (decomp_.root == -1) decomp_.root = id;
        return id;
    }

    struct StackE { int u, v, eid; };

    // Build BC-tree of induced subgraph on 'region' (excluding owner). Return closed BC-path region.
    vector<int> closeByBCPath(const vector<int>& region, int owner, int a, int b,
                              vector<int>* outBoundary = nullptr) const {
        const auto& core = topo_->core();
        unordered_set<int> in;
        in.reserve(region.size() * 2 + 1);
        for (int v : region) {
            if (v == owner) continue;
            if (!topo_->aliveVertex(v)) continue;
            in.insert(v);
        }
        if (!in.count(a) || !in.count(b)) return normalizeRegion(region, owner, -1);

        vector<int> verts(in.begin(), in.end());
        sort(verts.begin(), verts.end());
        int nV = (int)verts.size();
        unordered_map<int,int> idx;
        idx.reserve(nV*2);
        for(int i=0;i<nV;++i) idx[verts[i]]=i;

        vector<vector<pair<int,int>>> adj(nV);
        long long edgeScan = 0;
        for (int u : verts) {
            int iu = idx[u];
            for (int eid : core.incidentEdges(u)) {
                if (!core.edgeAlive(eid)) continue;
                ++edgeScan;
                auto [x,y] = core.edgeEndpoints(eid);
                int v = (x==u? y : x);
                if (v==owner) continue;
                auto it = idx.find(v);
                if (it==idx.end()) continue;
                int iv = it->second;
                adj[iu].push_back({iv,eid});
            }
        }
        // de-dup adjacency (optional)

        vector<int> disc(nV,0), low(nV,0), parent(nV,-1);
        vector<char> isArt(nV,false);
        vector<vector<int>> bccVerts;
        vector<vector<int>> bccEdges;
        vector<StackE> st;
        int timer=0;

        function<void(int,int)> dfs = [&](int u, int root){
            disc[u]=low[u]=++timer;
            int child=0;
            for(auto [v,eid]: adj[u]){
                if(!disc[v]){
                    parent[v]=u;
                    ++child;
                    st.push_back({u,v,eid});
                    dfs(v, root);
                    low[u]=min(low[u], low[v]);
                    if(low[v] >= disc[u]){
                        if (u!=root || child>1) isArt[u]=true;
                        unordered_set<int> vs;
                        vector<int> es;
                        while(!st.empty()){
                            auto e = st.back(); st.pop_back();
                            vs.insert(verts[e.u]);
                            vs.insert(verts[e.v]);
                            es.push_back(e.eid);
                            if(e.u==u && e.v==v) break;
                        }
                        if(vs.size()>=2){
                            vector<int> vv(vs.begin(), vs.end());
                            sort(vv.begin(), vv.end());
                            sort(es.begin(), es.end());
                            es.erase(unique(es.begin(), es.end()), es.end());
                            bccVerts.push_back(std::move(vv));
                            bccEdges.push_back(std::move(es));
                        }
                    }
                }else if(v!=parent[u] && disc[v] < disc[u]){
                    low[u]=min(low[u], disc[v]);
                    st.push_back({u,v,eid});
                }
            }
        };

        // run DFS from a's vertex to cover its connected component
        int ia = idx[a];
        dfs(ia, ia);
        // build membership: vertex -> blocks
        int B = (int)bccVerts.size();
        if (B==0) return normalizeRegion(region, owner, -1);
        unordered_map<int, vector<int>> v2blocks;
        v2blocks.reserve(B*4);
        for(int bi=0;bi<B;++bi){
            for(int v: bccVerts[bi]) v2blocks[v].push_back(bi);
        }
        // articulation set
        vector<int> artVerts;
        unordered_map<int,int> artToId;
        for(int i=0;i<nV;++i) if(isArt[i]){
            int v=verts[i];
            artToId[v]=(int)artVerts.size();
            artVerts.push_back(v);
        }
        int A = (int)artVerts.size();
        int N = B + A;
        vector<vector<int>> bcAdj(N);
        vector<vector<int>> bcNodeVerts(N);
        // blocks
        for(int bi=0;bi<B;++bi) bcNodeVerts[bi] = bccVerts[bi];
        // cuts
        for(int ai=0;ai<A;++ai){
            bcNodeVerts[B+ai] = {artVerts[ai]};
        }
        // connect
        for(int bi=0;bi<B;++bi){
            for(int v: bccVerts[bi]){
                auto it = artToId.find(v);
                if(it==artToId.end()) continue;
                int ci = B + it->second;
                bcAdj[bi].push_back(ci);
                bcAdj[ci].push_back(bi);
            }
        }
        for(int i=0;i<N;++i){
            sort(bcAdj[i].begin(), bcAdj[i].end());
            bcAdj[i].erase(unique(bcAdj[i].begin(), bcAdj[i].end()), bcAdj[i].end());
        }

        auto pickNodeForVertex = [&](int v)->int{
            auto itA = artToId.find(v);
            if(itA!=artToId.end()) return B + itA->second;
            auto itB = v2blocks.find(v);
            if(itB==v2blocks.end() || itB->second.empty()) return -1;
            return itB->second[0];
        };
        int s = pickNodeForVertex(a);
        int t = pickNodeForVertex(b);
        if(s==-1 || t==-1) return normalizeRegion(region, owner, -1);

        vector<int> par(N,-1);
        queue<int> q; q.push(s); par[s]=s;
        while(!q.empty()){
            int u=q.front(); q.pop();
            if(u==t) break;
            for(int v: bcAdj[u]) if(par[v]==-1){ par[v]=u; q.push(v);}    
        }
        if(par[t]==-1) return normalizeRegion(region, owner, -1);
        vector<int> pathNodes;
        int cur=t;
        while(true){ pathNodes.push_back(cur); if(cur==s) break; cur=par[cur]; }
        reverse(pathNodes.begin(), pathNodes.end());

        unordered_set<int> closed;
        for(int nid: pathNodes){
            for(int v: bcNodeVerts[nid]) closed.insert(v);
        }
        closed.insert(owner);
        vector<int> out(closed.begin(), closed.end());
        sort(out.begin(), out.end());

        if(outBoundary){
            vector<int> bv;
            for(int nid: pathNodes){
                if(nid>=B){ bv.push_back(bcNodeVerts[nid][0]); }
            }
            sort(bv.begin(), bv.end());
            bv.erase(unique(bv.begin(), bv.end()), bv.end());
            *outBoundary = std::move(bv);
        }
#ifdef LOCAL
        g_topo_dbg.topology_zone_bfs_edges += edgeScan; // reuse counter channel for induced scans
        g_topo_dbg.topology_zone_bfs_vertices += nV;
#endif
        return out;
    }

    void ensureBaseBuilt(int baseId, const PotentialHandleManager::Handle& context) const {
        if (baseId < 0 || baseId >= (int)base_.size()) return;
        if (base_[baseId].built) return;
        base_[baseId].built = true;

        // Explicit BC-tree pruning: shrink base region to BC-path closure between a and b.
        vector<int> boundary;
        auto closed = closeByBCPath(base_[baseId].regionVerts, context.owner, context.a, context.b, &boundary);
        base_[baseId].boundaryVerts = boundary;
        if ((int)closed.size() < (int)base_[baseId].regionVerts.size()) {
            int cid = getOrCreateBaseRegion(closed, context.owner);
            if (cid != baseId) base_[baseId].childBaseIds.push_back(cid);
        }
        // New strict-subset rule: build a compact child directly from the current certificate.
        auto compact = compactRegionFromCertificate(context);
        if (!compact.empty() && (int)compact.size() < (int)base_[baseId].regionVerts.size()) {
            int cid = getOrCreateBaseRegion(compact, context.owner);
            if (cid != baseId) base_[baseId].childBaseIds.push_back(cid);
        }
        sort(base_[baseId].childBaseIds.begin(), base_[baseId].childBaseIds.end());
        base_[baseId].childBaseIds.erase(unique(base_[baseId].childBaseIds.begin(), base_[baseId].childBaseIds.end()), base_[baseId].childBaseIds.end());
    }

    void ensureLatticeChildren(int nodeId, const PotentialHandleManager::Handle& context) const {
        if (nodeId < 0 || nodeId >= (int)lattice_.size()) return;
        int bud = lattice_[nodeId].budgetExp;
        if (!lattice_[nodeId].children.empty() || bud == 0) return;
        int b = lattice_[nodeId].baseId;
        ensureBaseBuilt(b, context);
        int nextBudget = max(0, bud - 1);
        vector<int> ch;
        // Always include staying in the same base region (explicit lattice descent in potential).
        ch.push_back(getOrCreateLatticeNode(b, nextBudget));
        for (int cb : base_[b].childBaseIds) ch.push_back(getOrCreateLatticeNode(cb, nextBudget));
        sort(ch.begin(), ch.end());
        ch.erase(unique(ch.begin(), ch.end()), ch.end());
        lattice_[nodeId].children = ch;
        // mirror in decomposition nodes
        decomp_.nodes[nodeId].children = ch;
    }

    PotentialHandleManager::Handle assignNode(PotentialHandleManager::Handle h, int budgetExp) const {
        if (!PotentialHandleManager::valid(h)) return h;
        int baseId = getOrCreateBaseRegion(h.regionVerts, h.owner);
        int nodeId = getOrCreateLatticeNode(baseId, budgetExp);
        h.nodeId = nodeId;
        h.budgetExp = budgetExp;
        h.regionVerts = base_[baseId].regionVerts;
        ensureLatticeChildren(nodeId, h);
        return h;
    }

    // Build a closed decomposition handle from a witness path by lifting to BC-path closure.
    vector<int> buildClosedHandleFromWitness(int owner, int a, int b,
                                             const vector<int>& pathVerts,
                                             const vector<int>& /*pathEdges*/) const {
        // Candidate region: expand from witness path (radius 4). If huge, keep as-is.
        unordered_set<int> allowed = PotentialHandleManager::collectNeighborhood(topo_->core(), pathVerts, owner, -1, 4);
        vector<int> cand;
        cand.reserve(allowed.size());
        for(int v: allowed) cand.push_back(v);
        sort(cand.begin(), cand.end());
        vector<int> boundary;
        auto closed = closeByBCPath(cand, owner, a, b, &boundary);
        (void)boundary;
        return closed;
    }

    PotentialHandleManager::Handle tryBuildChild(const PotentialHandleManager::Handle& oldH,
                                                 int childNodeId,
                                                 int removedV) const {
        if (removedV == oldH.owner || removedV == oldH.a || removedV == oldH.b) return {};
        if (childNodeId < 0 || childNodeId >= (int)lattice_.size()) return {};
#ifdef LOCAL
        g_strict_child_dbg.try_build_child_calls++;
#endif
        int baseId = lattice_[childNodeId].baseId;
        int bud = lattice_[childNodeId].budgetExp;
        if (baseId < 0 || baseId >= (int)base_.size()) return {};
        long long vv = 0, ee = 0;
        auto h = buildRestrictedFromSortedRegion(oldH.owner, oldH.a, oldH.b, base_[baseId].regionVerts, removedV, &vv, &ee);
        if (!PotentialHandleManager::valid(h)) return {};
#ifdef LOCAL
        g_strict_child_dbg.try_build_child_success++;
        g_strict_child_dbg.strict_child_rebuild_used++;
        g_strict_child_dbg.strict_child_rebuild_vertices += vv;
        g_strict_child_dbg.strict_child_rebuild_edges += ee;
#endif
        h = maybeCompactHandle(std::move(h), removedV);
        h = assignNode(std::move(h), bud);
#ifdef LOCAL
        if (h.budgetExp != PotentialHandleManager::childBudget(oldH)) abort();
#endif
        return h;
    }

public:
    explicit PotentialHandleKernel(DecrementalNBTopology* topo) : topo_(topo) {
        decomp_.nodes.clear();
        decomp_.root = -1;
    }

    PotentialHandleManager::Handle buildSeedHandle(int owner, int a, int b, int budgetExp) const {
        // Start from an exact witness in a neighborhood, then lift to a closed BC-path region.
        auto h0 = PotentialHandleManager::buildSeedHandle(topo_->core(), owner, a, b, 3, budgetExp);
        if (!PotentialHandleManager::valid(h0)) return {};
        auto compact = compactRegionFromCertificate(h0);
        if (!compact.empty()) h0.regionVerts = std::move(compact);
        h0.budgetExp = budgetExp;
        return assignNode(std::move(h0), budgetExp);
    }

    PotentialHandleManager::Handle relocateToStrictChild(const PotentialHandleManager::Handle& oldH,
                                                         int owner, int a, int b,
                                                         int removedV) const {
        (void)owner; (void)a; (void)b;
        ensureLatticeChildren(oldH.nodeId, oldH);
        vector<int> childNodes;
        if (oldH.nodeId >= 0 && oldH.nodeId < (int)lattice_.size()) childNodes = lattice_[oldH.nodeId].children;
        vector<PotentialHandleManager::Result> cands;
        for (int cid : childNodes) {
            auto h = tryBuildChild(oldH, cid, removedV);
            if (!PotentialHandleManager::valid(h)) continue;
            PotentialHandleManager::Result r;
            r.has = true;
            r.strict = (h.regionVerts.size() < oldH.regionVerts.size()) || (h.budgetExp < oldH.budgetExp);
            r.h = std::move(h);
            cands.push_back(std::move(r));
        }
        auto best = PotentialHandleManager::chooseBest(oldH, cands);
#ifdef LOCAL
        if (!cands.empty()) {
            if (!best.has) {
                g_strict_child_dbg.strict_child_exists_but_missed++;
                g_strict_child_dbg.strict_child_structural_miss++;
            } else {
                g_strict_child_dbg.strict_child_found++;
                g_strict_child_dbg.strict_child_depth_sum++;
            }
        }
#endif
        if (!best.has) return {};
        recordRelocationStats(oldH, best.h);
        return best.h;
    }

    // For LOCAL semantic differential: check if there exists a global witness but not inside handle.
    bool hasRestrictedWitness(const PotentialHandleManager::Handle& h) const {
        auto ex = buildRestrictedFromSortedRegion(h.owner, h.a, h.b, h.regionVerts, -1);
        return PotentialHandleManager::valid(ex);
    }

    bool hasGlobalWitness(int owner, int a, int b) const {
        const auto& core = topo_->core();
        unordered_map<int,int> par;
        queue<int> q;
        q.push(a); par[a]=a;
        while(!q.empty()){
            int u=q.front(); q.pop();
            if(u==b) break;
            for(int eid: core.incidentEdges(u)){
                if(!core.edgeAlive(eid)) continue;
                int v=core.other(eid,u);
                if(!topo_->aliveVertex(v) || v==owner) continue;
                if(par.count(v)) continue;
                par[v]=u; q.push(v);
            }
        }
        return par.count(b);
    }
};
class LiteraturePotentialOracle final : public NBOracle {
    enum class SupportOriginKind {
        MaterializedSupport,
        PreservedPiece,
        ConnectorTree,
    };
    struct WatchHandle {
        int vertex = -1;
        int slotPos = -1;
        SupportOriginKind originKind = SupportOriginKind::MaterializedSupport;
        int treeId = -1;
        int pieceId = -1;
        int localPos = -1;
    };
    struct WatchEntry {
        int owner = -1;
        int cid = -1;
        int handleIdx = -1;
    };
    struct QueryState {
        int owner = -1;
        int aIdx = -1;
        int bIdx = -1;
        int multiplicity = 0;
        int cid = -1;
        bool active = false;
    };
    struct SupportTreeObject {
        int treeId = -1;
        int rootPos = -1;
        vector<int> parentPos;
        vector<int> depth;
        vector<int> tin;
        vector<int> tout;
        vector<int> preorder;
        vector<int> vertexByPos;
        vector<int> watchEntryIdsByPos;
        vector<int> endpointIdxByPos;
        vector<int> endpointPosSorted;
    };
    struct SupportPieceRef {
        int pieceId = -1;
        int treeId = -1;
        int entryVertexPos = -1;
        int blockedParentPos = -1;
        int attachmentVertexPos = -1;
        int pieceRepresentativeEndpoint = -1;
        int pieceEndpointCount = 0;
        bool pieceAlive = false;
        bool complementOfBlockedSubtree = false;
    };
    struct ClassState {
        int epoch = 0;
        bool watchActive = false;
        int watchVertexCount = 0;
        int activeQueryCount = 0;
        vector<int> endpointPool;
        vector<WatchHandle> watchHandles;

        bool supportMetaValid = false;
        int supportRootPos = -1;
        vector<int> supportVerts;
        vector<int> supportParentPos;
        vector<int> supportDepth;
        vector<int> supportTin;
        vector<int> supportTout;
        vector<int> supportChildFirst;
        vector<int> supportNextSibling;
        vector<int> supportSubtreeEndpointCount;
        vector<int> supportSubtreeRepresentativeEndpoint;
        vector<int> supportNodeEndpointIdx;
        vector<int> supportPreorder;
        vector<int> activeEndpointIdxsSortedBySupportTin;
        vector<int> activeEndpointTinsSorted;

        int materializedTreeId = -1;
        bool pieceModeActive = false;
        vector<SupportPieceRef> preservedPieces;
        vector<SupportPieceRef> connectorPieces;
        vector<int> patchTreeIds;
        vector<int> attachmentVerticesByPiece;
        int connectorTreeId = -1;
        vector<int> connectorWatchEntryIds;
        vector<int> connectorSkeletonVertices;
        unordered_map<int,int> connectorVertexToPos;
        unordered_map<int,int> connectorSkeletonWatchHandleByVertex;
    };
    struct OwnerData {
        vector<int> qids;
        vector<int> endpoints;
        unordered_map<int,int> endpointIndex;
        vector<vector<int>> incidentQids;
        vector<int> endpointActiveCount;
        vector<int> endpointMark;
        int endpointMarkCur = 1;
        unordered_map<int, ClassState> classStates;
        int activeQueryCount = 0;

        int ensureEndpoint(int v) {
            auto it = endpointIndex.find(v);
            if (it != endpointIndex.end()) return it->second;
            int id = (int)endpoints.size();
            endpointIndex.emplace(v, id);
            endpoints.push_back(v);
            incidentQids.emplace_back();
            endpointActiveCount.push_back(0);
            endpointMark.push_back(0);
            return id;
        }

        int nextMark() {
            if (++endpointMarkCur == INT_MAX) {
                fill(endpointMark.begin(), endpointMark.end(), 0);
                endpointMarkCur = 1;
            }
            return endpointMarkCur;
        }
    };
    struct SupportScratch {
        vector<int> seenStamp;
        vector<int> parent;
        vector<int> queueBuf;
        vector<int> targetStamp;
        vector<int> collectStamp;
        vector<int> artifactStamp;
        vector<int> artifactParent;
        vector<int> artifactRoot;
        vector<int> artifactDepth;
        vector<int> supportPosStamp;
        vector<int> supportPosVal;
        int seenCur = 1;
        int targetCur = 1;
        int collectCur = 1;
        int artifactCur = 1;
        int supportPosCur = 1;

        void ensure(int n) {
            int need = n + 1;
            if ((int)seenStamp.size() < need) {
                seenStamp.assign(need, 0);
                parent.assign(need, -1);
                targetStamp.assign(need, 0);
                collectStamp.assign(need, 0);
                artifactStamp.assign(need, 0);
                artifactParent.assign(need, -1);
                artifactRoot.assign(need, -1);
                artifactDepth.assign(need, -1);
                supportPosStamp.assign(need, 0);
                supportPosVal.assign(need, -1);
            }
        }
        int nextSeen() {
            if (++seenCur == INT_MAX) {
                fill(seenStamp.begin(), seenStamp.end(), 0);
                seenCur = 1;
            }
            return seenCur;
        }
        int nextTarget() {
            if (++targetCur == INT_MAX) {
                fill(targetStamp.begin(), targetStamp.end(), 0);
                targetCur = 1;
            }
            return targetCur;
        }
        int nextCollect() {
            if (++collectCur == INT_MAX) {
                fill(collectStamp.begin(), collectStamp.end(), 0);
                collectCur = 1;
            }
            return collectCur;
        }
        int nextArtifact() {
            if (++artifactCur == INT_MAX) {
                fill(artifactStamp.begin(), artifactStamp.end(), 0);
                artifactCur = 1;
            }
            return artifactCur;
        }
        int nextSupportPos() {
            if (++supportPosCur == INT_MAX) {
                fill(supportPosStamp.begin(), supportPosStamp.end(), 0);
                supportPosCur = 1;
            }
            return supportPosCur;
        }
    };

    struct SupportBuildProduct {
        vector<int> watchVerts;
        int rootPos = -1;
        int posStamp = 0;
        vector<int> parentPos;
        vector<int> depth;
        vector<int> tin;
        vector<int> tout;
        vector<int> childFirst;
        vector<int> nextSibling;
        vector<int> nodeEndpointIdx;
        vector<int> preorder;
    };

    struct PieceHitInfo {
        int pieceId = -1;
        int treeId = -1;
        int localPos = -1;
    };
    struct TouchedClassInfo {
        int owner = -1;
        int oldCid = -1;
        int xHandleIdx = -1;
        vector<PieceHitInfo> pieceHits;
        vector<PieceHitInfo> connectorHits;
        bool connectorHit = false;
        int connectorTreeId = -1;
        int connectorPieceId = -1;
        int connectorLocalPos = -1;
    };

    struct StatePublishContext {
        unordered_map<int, int> treePosStampCache;
    };

    int n_ = 0;
    vector<BranchQuery> bq_;
    vector<char> alive_;
    vector<int> compId_;
    vector<char> failing_;
    vector<OwnerData> ownerData_;
    vector<QueryState> qstate_;
    vector<vector<WatchEntry>> watchByVertex_;
    DecrementalNBTopology topo_;
    SupportScratch supportScratch_;
    vector<SupportTreeObject> supportTrees_;
    int nextSupportTreeId_ = 1;
    int nextPieceId_ = 1;
    vector<int> querySeenStamp_;
    int querySeenCur_ = 1;
    long long currentSupportWatch_ = 0;
    int activeQueryTotal_ = 0;
    int currentDeleteX_ = -1;
    int currentDeleteStep_ = 0;
    StatePublishContext* activePublishCtx_ = nullptr;
    struct DeleteWatchSnapshot {
        unordered_set<string> oldKeys;
        unordered_set<string> newKeys;
    };
    unordered_map<long long, DeleteWatchSnapshot> currentDeleteWatchSnapshots_;

    static string watchHandleKey(const WatchHandle& h) {
        return to_string(h.vertex)+"|"+to_string((int)h.originKind)+"|"+to_string(h.treeId)+"|"+to_string(h.pieceId)+"|"+to_string(h.localPos);
    }
    void saveDeleteWatchSnapshotOld(int owner, int cid, const ClassState& st) {
        if (!(1 <= currentDeleteX_ && currentDeleteX_ <= n_)) return;
        auto &snap = currentDeleteWatchSnapshots_[watchKey(owner,cid)];
        for (const auto& h : st.watchHandles) if (h.vertex == currentDeleteX_) snap.oldKeys.insert(watchHandleKey(h));
    }
    void saveDeleteWatchSnapshotNew(int owner, int cid, const ClassState& st) {
        if (!(1 <= currentDeleteX_ && currentDeleteX_ <= n_)) return;
        auto &snap = currentDeleteWatchSnapshots_[watchKey(owner,cid)];
        for (const auto& h : st.watchHandles) if (h.vertex == currentDeleteX_) snap.newKeys.insert(watchHandleKey(h));
    }

    static long long watchKey(int owner, int cid) {
        return (static_cast<long long>(owner) << 32) ^ static_cast<unsigned>(cid);
    }

    static const char* supportOriginKindName(SupportOriginKind k) {
        switch (k) {
            case SupportOriginKind::MaterializedSupport: return "MaterializedSupport";
            case SupportOriginKind::PreservedPiece: return "PreservedPiece";
            case SupportOriginKind::ConnectorTree: return "ConnectorTree";
        }
        return "Unknown";
    }

    static optional<int> envIntOpt(const char* name) {
        const char* v = getenv(name);
        if (!v || !*v) return nullopt;
        char* endp = nullptr;
        long long val = strtoll(v, &endp, 10);
        if (!endp || *endp != '\0') return nullopt;
        if (val < INT_MIN || val > INT_MAX) return nullopt;
        return (int)val;
    }


    struct BothSnapshotLeakDetail {
        bool pieceContainsXInNewState = false;
        bool watchHandlePointsToX = false;
        bool attachmentIsX = false;
        bool attachmentInsidePiece = false;
        bool samePieceHandleReused = false;
        int attachmentVertex = -1;
        const char* classified = "both_snapshot_other";
    };

    bool handleWatchesDeletedVertex(const WatchHandle& h) const {
        return h.vertex == currentDeleteX_;
    }

    BothSnapshotLeakDetail classifyBothSnapshotLeakAgainstNewState(int owner, int cid, const WatchHandle& h) {
        BothSnapshotLeakDetail d;
        d.watchHandlePointsToX = handleWatchesDeletedVertex(h);
        if (!(1 <= owner && owner <= n_)) {
#ifdef LOCAL
            g_batch_dbg.debug_both_snapshot_other++;
#endif
            return d;
        }
        auto itOwner = ownerData_[owner].classStates.find(cid);
        if (itOwner == ownerData_[owner].classStates.end()) {
#ifdef LOCAL
            g_batch_dbg.debug_both_snapshot_other++;
#endif
            return d;
        }
        const auto& st = itOwner->second;
        for (size_t i = 0; i < st.preservedPieces.size(); ++i) {
            const auto& piece = st.preservedPieces[i];
            if (!piece.pieceAlive) continue;
            if (piece.pieceId == h.pieceId && piece.treeId == h.treeId) {
                d.samePieceHandleReused = true;
                d.pieceContainsXInNewState = pieceContainsVertex(piece, currentDeleteX_);
                if (i < st.attachmentVerticesByPiece.size()) d.attachmentVertex = st.attachmentVerticesByPiece[i];
                d.attachmentIsX = (d.attachmentVertex == currentDeleteX_);
                if (1 <= d.attachmentVertex && d.attachmentVertex <= n_) {
                    d.attachmentInsidePiece = pieceContainsVertex(piece, d.attachmentVertex);
                }
                break;
            }
        }
#ifdef LOCAL
        if (d.samePieceHandleReused) g_batch_dbg.debug_both_snapshot_reused_same_piece_handle++;
        if (d.attachmentIsX) g_batch_dbg.debug_both_snapshot_attachment_is_x++;
        if (d.samePieceHandleReused && d.attachmentVertex != -1 && !d.attachmentInsidePiece) g_batch_dbg.debug_both_snapshot_attachment_outside_piece++;
        if (d.pieceContainsXInNewState) {
            d.classified = "both_snapshot_piece_contains_x";
            g_batch_dbg.debug_both_snapshot_piece_contains_x++;
        } else if (d.watchHandlePointsToX) {
            d.classified = "both_snapshot_piece_excludes_x_but_watch_contains_x";
            g_batch_dbg.debug_both_snapshot_piece_excludes_x_but_watch_contains_x++;
        } else {
            d.classified = "both_snapshot_other";
            g_batch_dbg.debug_both_snapshot_other++;
        }
#endif
        return d;
    }

    void debugCheckNoDeletedVertexInCanonicalState(int owner, int cid, const ClassState& st) {
#ifdef LOCAL
        if (!(1 <= currentDeleteX_ && currentDeleteX_ <= n_)) return;
        for (const auto& p : st.preservedPieces) {
            if (!p.pieceAlive) continue;
            if (pieceContainsVertex(p, currentDeleteX_)) {
                g_batch_dbg.debug_postcondition_piece_contains_x++;
                break;
            }
        }
        for (int att : st.attachmentVerticesByPiece) {
            if (att == currentDeleteX_) {
                g_batch_dbg.debug_postcondition_attachment_is_x++;
                break;
            }
        }
        for (const auto& h : st.watchHandles) {
            if (h.vertex == currentDeleteX_) {
                g_batch_dbg.debug_postcondition_watch_points_to_x++;
                break;
            }
        }
#endif
    }

    bool replayTargetMatches(int owner, int cid, const WatchHandle& h) const {
        auto delIdx = envIntOpt("REPLAY_DELETION_INDEX");
        if (!delIdx || *delIdx != currentDeleteStep_) return false;
        auto delX = envIntOpt("REPLAY_DELETED_VERTEX");
        if (delX && *delX != currentDeleteX_) return false;
        auto wantOwner = envIntOpt("REPLAY_OWNER");
        if (wantOwner && *wantOwner != owner) return false;
        auto wantCid = envIntOpt("REPLAY_CID");
        if (wantCid && *wantCid != cid) return false;
        auto wantTreeId = envIntOpt("REPLAY_TREE_ID");
        if (wantTreeId && *wantTreeId != h.treeId) return false;
        auto wantPieceId = envIntOpt("REPLAY_PIECE_ID");
        if (wantPieceId && *wantPieceId != h.pieceId) return false;
        auto wantLocalPos = envIntOpt("REPLAY_LOCAL_POS");
        if (wantLocalPos && *wantLocalPos != h.localPos) return false;
        const char* ok = getenv("REPLAY_ORIGIN_KIND");
        if (ok && *ok) {
            string s = ok;
            if (s != supportOriginKindName(h.originKind)) return false;
        }
        return true;
    }

    void dumpDeletedVertexWatchLeak(int x, int owner, int cid, const WatchHandle& h) {
#ifdef LOCAL
        g_batch_dbg.debug_watch_leak_on_deleted_vertex++;
        const string key = watchHandleKey(h);
        auto itSnap = currentDeleteWatchSnapshots_.find(watchKey(owner, cid));
        bool inOld = false, inNew = false;
        size_t oldCount = 0, newCount = 0, oldOnlyCount = 0, newOnlyCount = 0;
        if (itSnap != currentDeleteWatchSnapshots_.end()) {
            inOld = itSnap->second.oldKeys.find(key) != itSnap->second.oldKeys.end();
            inNew = itSnap->second.newKeys.find(key) != itSnap->second.newKeys.end();
            oldCount = itSnap->second.oldKeys.size();
            newCount = itSnap->second.newKeys.size();
            for (const auto& k2 : itSnap->second.oldKeys) if (!itSnap->second.newKeys.count(k2)) ++oldOnlyCount;
            for (const auto& k2 : itSnap->second.newKeys) if (!itSnap->second.oldKeys.count(k2)) ++newOnlyCount;
        }
        const char* classified = "foreign_leak";
        BothSnapshotLeakDetail second;
        if (inOld && !inNew) {
            classified = "old_only_leak";
            g_batch_dbg.debug_watch_leak_old_only++;
        } else if (!inOld && inNew) {
            classified = "new_only_leak";
            g_batch_dbg.debug_watch_leak_new_only++;
        } else if (inOld && inNew) {
            classified = "both_snapshot_leak";
            g_batch_dbg.debug_watch_leak_both_snapshot++;
        } else {
            classified = "foreign_leak";
            g_batch_dbg.debug_watch_leak_foreign++;
        }
        if (inOld && inNew) second = classifyBothSnapshotLeakAgainstNewState(owner, cid, h);
        switch (h.originKind) {
            case SupportOriginKind::MaterializedSupport: g_batch_dbg.debug_watch_leak_origin_materialized++; break;
            case SupportOriginKind::PreservedPiece: g_batch_dbg.debug_watch_leak_origin_preserved_piece++; break;
            case SupportOriginKind::ConnectorTree: g_batch_dbg.debug_watch_leak_origin_connector_skeleton++; break;
        }
        int stateEpoch = -1;
        if (1 <= owner && owner <= n_) {
            auto itOwner = ownerData_[owner].classStates.find(cid);
            if (itOwner != ownerData_[owner].classStates.end()) {
                stateEpoch = itOwner->second.epoch;
                unordered_map<long long,int> seen;
                for (const auto& wh : itOwner->second.watchHandles) {
                    long long hk = (static_cast<long long>(wh.vertex) << 32) ^ static_cast<unsigned>(wh.slotPos);
                    if (++seen[hk] > 1) g_batch_dbg.debug_watch_double_owned++;
                    if (!(1 <= wh.vertex && wh.vertex <= n_)) continue;
                    const auto& vec = watchByVertex_[wh.vertex];
                    if (!(0 <= wh.slotPos && wh.slotPos < (int)vec.size())) { g_batch_dbg.debug_watch_stale_handle_in_state++; continue; }
                    const auto& ref = vec[wh.slotPos];
                    if (ref.owner != owner || ref.cid != cid) g_batch_dbg.debug_watch_owner_mismatch++;
                }
            }
        }
        if (replayTargetMatches(owner, cid, h)) {
            g_batch_dbg.debug_replay_target_hit++;
            cerr << "REPLAY_TARGET_HIT deletion_index=" << currentDeleteStep_
                 << " deleted_vertex=" << x
                 << " owner=" << owner
                 << " cid=" << cid
                 << " originKind=" << supportOriginKindName(h.originKind)
                 << " treeId=" << h.treeId
                 << " pieceId=" << h.pieceId
                 << " localPos=" << h.localPos << "\n";
        }
        g_batch_dbg.debug_replay_case_saved++;
        cerr << "FIRST_LEAK deletion_index=" << currentDeleteStep_
             << " deleted_vertex=" << x
             << " owner=" << owner
             << " cid=" << cid
             << " originKind=" << supportOriginKindName(h.originKind)
             << " treeId=" << h.treeId
             << " pieceId=" << h.pieceId
             << " localPos=" << h.localPos
             << " stateEpoch=" << stateEpoch
             << " is_in_old_snapshot=" << (inOld?1:0)
             << " is_in_new_snapshot=" << (inNew?1:0)
             << " classified_as=" << classified
             << " piece_contains_x_in_new_state=" << (second.pieceContainsXInNewState?1:0)
             << " attachment_vertex=" << second.attachmentVertex
             << " attachment_is_x=" << (second.attachmentIsX?1:0)
             << " attachment_inside_piece=" << (second.attachmentInsidePiece?1:0)
             << " watch_handle_points_to_x=" << (second.watchHandlePointsToX?1:0)
             << " same_piece_handle_reused=" << (second.samePieceHandleReused?1:0)
             << " second_level_classified_as=" << second.classified
             << " old_snapshot_handle_count=" << oldCount
             << " new_snapshot_handle_count=" << newCount
             << " old_only_unregister_count=" << oldOnlyCount
             << " new_only_register_count=" << newOnlyCount
             << " replay_seed=" << (getenv("REPLAY_SEED")?getenv("REPLAY_SEED"):"")
             << " replay_case_name=" << (getenv("REPLAY_CASE_NAME")?getenv("REPLAY_CASE_NAME"):"")
             << " replay_deletion_index=" << currentDeleteStep_
             << " replay_deleted_vertex=" << x
             << " replay_owner=" << owner
             << " replay_cid=" << cid
             << " replay_origin_kind=" << supportOriginKindName(h.originKind)
             << " replay_tree_id=" << h.treeId
             << " replay_piece_id=" << h.pieceId
             << " replay_local_pos=" << h.localPos
             << "\n";
#endif
    }

    void syncComponents() {
        compId_.assign(n_ + 1, -1);
        for (int v = 1; v <= n_; ++v) compId_[v] = topo_.componentOf(v);
    }

    void bumpActiveQueryPeak() {
#ifdef LOCAL
        g_batch_dbg.active_query_peak = max<long long>(g_batch_dbg.active_query_peak, activeQueryTotal_);
#endif
    }

    void bumpSupportWatchPeak() {
#ifdef LOCAL
        g_batch_dbg.support_watch_peak = max<long long>(g_batch_dbg.support_watch_peak, currentSupportWatch_);
#endif
    }

    void nextQuerySeenStamp() {
        if (++querySeenCur_ == INT_MAX) {
            fill(querySeenStamp_.begin(), querySeenStamp_.end(), 0);
            querySeenCur_ = 1;
        }
    }

    ClassState& classState(int owner, int cid) {
        return ownerData_[owner].classStates[cid];
    }

    SupportTreeObject* getSupportTreeObject(int treeId) {
        if (treeId <= 0) return nullptr;
        for (auto& t : supportTrees_) if (t.treeId == treeId) return &t;
        return nullptr;
    }
    const SupportTreeObject* getSupportTreeObject(int treeId) const {
        if (treeId <= 0) return nullptr;
        for (const auto& t : supportTrees_) if (t.treeId == treeId) return &t;
        return nullptr;
    }

    int buildTreeVertexPosMapCached(const SupportTreeObject& tree) {
        if (!activePublishCtx_) {
            int stamp = 0;
            buildTreeVertexPosMap(tree, stamp);
            return stamp;
        }
        auto it = activePublishCtx_->treePosStampCache.find(tree.treeId);
        if (it != activePublishCtx_->treePosStampCache.end()) return it->second;
        int stamp = 0;
#ifdef LOCAL
        g_batch_dbg.dispatch_publish_posmap_build_calls++;
        g_batch_dbg.dispatch_publish_posmap_build_vertices += (long long)tree.vertexByPos.size();
        ScopedNsAcc __timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_dispatch_publish_posmap_build_ns),
                           ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_dispatch_publish_posmap_build_calls));
        buildTreeVertexPosMap(tree, stamp);
#else
        buildTreeVertexPosMap(tree, stamp);
#endif
        activePublishCtx_->treePosStampCache.emplace(tree.treeId, stamp);
        return stamp;
    }

    struct ScopedStatePublishContext {
        LiteraturePotentialOracle* self = nullptr;
        StatePublishContext* prev = nullptr;
        ScopedStatePublishContext(LiteraturePotentialOracle* s, StatePublishContext* ctx) : self(s) {
            if (self) {
                prev = self->activePublishCtx_;
                self->activePublishCtx_ = ctx;
            }
        }
        ~ScopedStatePublishContext() {
            if (self) self->activePublishCtx_ = prev;
        }
    };

    void syncWatchEntryFromHandle(const WatchHandle& h) {
        if (!(1 <= h.vertex && h.vertex <= n_)) return;
        auto& vec = watchByVertex_[h.vertex];
        if (!(0 <= h.slotPos && h.slotPos < (int)vec.size())) return;
        vec[h.slotPos].handleIdx = vec[h.slotPos].handleIdx;
    }

    void annotateHandleMetadata(WatchHandle& h, SupportOriginKind kind, int treeId, int pieceId, int localPos) {
        h.originKind = kind;
        h.treeId = treeId;
        h.pieceId = pieceId;
        h.localPos = localPos;
    }
    vector<int> remapRetainedHandleIndices(const vector<int>& oldIdxs, const vector<char>& keepMask) {
        vector<int> prefixRemoved(keepMask.size() + 1, 0);
        for (int i = 0; i < (int)keepMask.size(); ++i) prefixRemoved[i + 1] = prefixRemoved[i] + (keepMask[i] ? 0 : 1);
        vector<int> out;
        out.reserve(oldIdxs.size());
        for (int oldIdx : oldIdxs) {
            if (!(0 <= oldIdx && oldIdx < (int)keepMask.size())) continue;
            if (!keepMask[oldIdx]) continue;
            out.push_back(oldIdx - prefixRemoved[oldIdx]);
        }
        return out;
    }
#ifdef LOCAL
    void noteReuseWatchFullScan(const ClassState& st) {
        if (!local_profile_coarse_enabled()) return;
        long long cnt = (long long)st.watchHandles.size();
        g_batch_dbg.reuse_watch_handle_full_scan_calls++;
        g_batch_dbg.reuse_watch_handle_full_scan_handles += cnt;
        if (g_wscan_active_route_tag != REUSE_ROUTE_NONE) {
            if (g_wscan_route_full_scan_passes > 0) {
                g_batch_dbg.wscan_duplicate_full_scan_passes++;
                g_batch_dbg.wscan_duplicate_full_scan_handles += cnt;
            }
            g_wscan_route_full_scan_passes++;
            g_wscan_route_full_scan_handles += cnt;
        }
    }
#endif
    void dispatchPublishAnnotatePreserved(ClassState& st, const vector<SupportPieceRef>& pieces) {
#ifdef LOCAL
        if (pieces.empty()) return;
        g_batch_dbg.dispatch_publish_preserved_handles += (long long)st.watchHandles.size();
        g_batch_dbg.dispatch_publish_preserved_pieces_visited += (long long)pieces.size();
        g_batch_dbg.dispatch_publish_full_rescan_calls++;
        ScopedNsAcc __timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_dispatch_publish_preserved_annotate_ns),
                           ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_dispatch_publish_preserved_annotate_calls));
        annotatePreservedHandlesByPieces(st, pieces);
#else
        annotatePreservedHandlesByPieces(st, pieces);
#endif
    }
    void dispatchPublishAnnotateConnectorPieces(ClassState& st, const vector<SupportPieceRef>& pieces) {
#ifdef LOCAL
        if (pieces.empty()) return;
        g_batch_dbg.dispatch_publish_connector_handles += (long long)st.watchHandles.size();
        g_batch_dbg.dispatch_publish_connector_pieces_visited += (long long)pieces.size();
        g_batch_dbg.dispatch_publish_full_rescan_calls++;
        ScopedNsAcc __timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_dispatch_publish_connector_annotate_ns),
                           ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_dispatch_publish_connector_annotate_calls));
        annotateConnectorHandlesByPieces(st, pieces);
#else
        annotateConnectorHandlesByPieces(st, pieces);
#endif
    }
    void dispatchPublishAnnotateConnectorHandles(ClassState& st, const vector<int>& handleIdxs, int connectorTreeId) {
#ifdef LOCAL
        if (connectorTreeId <= 0) return;
        g_batch_dbg.dispatch_publish_connector_handles += (long long)st.watchHandles.size();
        g_batch_dbg.dispatch_publish_connector_pieces_visited += 1;
        g_batch_dbg.dispatch_publish_full_rescan_calls++;
        ScopedNsAcc __timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_dispatch_publish_connector_annotate_ns),
                           ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_dispatch_publish_connector_annotate_calls));
        annotateConnectorHandles(st, handleIdxs, connectorTreeId);
#else
        annotateConnectorHandles(st, handleIdxs, connectorTreeId);
#endif
    }
    void dispatchPublishRebuildConnectorWatchEntryIds(ClassState& st) {
#ifdef LOCAL
        g_batch_dbg.dispatch_publish_watch_id_rebuild_calls++;
        g_batch_dbg.dispatch_publish_watch_id_rebuild_handles += (long long)st.watchHandles.size();
        g_batch_dbg.dispatch_publish_full_rescan_calls++;
        ScopedNsAcc __timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_dispatch_publish_watch_id_rebuild_ns),
                           ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_dispatch_publish_watch_id_rebuild_calls));
        rebuildConnectorWatchEntryIds(st);
#else
        rebuildConnectorWatchEntryIds(st);
#endif
    }
    void dispatchPublishRebuildCanonicalState(ClassState& st) {
#ifdef LOCAL
        g_batch_dbg.dispatch_publish_canonical_rebuild_calls++;
        g_batch_dbg.dispatch_publish_canonical_vertices += (long long)st.connectorWatchEntryIds.size();
        ScopedNsAcc __timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_dispatch_publish_canonical_rebuild_ns),
                           ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_dispatch_publish_canonical_rebuild_calls));
        rebuildConnectorSkeletonCanonicalState(st);
#else
        rebuildConnectorSkeletonCanonicalState(st);
#endif
    }
    void reusePrepublishAnnotatePreserved(ClassState& st, const vector<SupportPieceRef>& pieces) {
#ifdef LOCAL
        if (pieces.empty()) return;
        g_batch_dbg.reuse_prepublish_preserved_annotate_calls++;
        g_batch_dbg.reuse_prepublish_preserved_handles += (long long)st.watchHandles.size();
        ScopedNsAcc __timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_prepublish_preserved_annotate_ns),
                           ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_prepublish_preserved_annotate_calls));
        annotatePreservedHandlesByPieces(st, pieces);
#else
        annotatePreservedHandlesByPieces(st, pieces);
#endif
    }
    void reusePrepublishConnectorMetadataRefresh(ClassState& st) {
#ifdef LOCAL
        g_batch_dbg.reuse_prepublish_connector_annotate_calls++;
        g_batch_dbg.reuse_prepublish_connector_handles += (long long)st.watchHandles.size();
        g_batch_dbg.reuse_full_connector_watch_id_rebuild_calls++;
        ScopedNsAcc __timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_prepublish_connector_annotate_ns),
                           ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_prepublish_connector_annotate_calls));
        rebuildConnectorWatchEntryIds(st);
#else
        rebuildConnectorWatchEntryIds(st);
#endif
    }

    void annotateMaterializedHandles(ClassState& st) {
        if (!st.supportMetaValid || st.materializedTreeId <= 0) return;
        int m = min((int)st.watchHandles.size(), (int)st.supportVerts.size());
        for (int i = 0; i < m; ++i) {
            annotateHandleMetadata(st.watchHandles[i], SupportOriginKind::MaterializedSupport, st.materializedTreeId, -1, i);
        }
    }

    void buildTreeVertexPosMap(const SupportTreeObject& tree, int& stamp) {
        supportScratch_.ensure(n_);
        stamp = supportScratch_.nextSupportPos();
        for (int pos = 0; pos < (int)tree.vertexByPos.size(); ++pos) {
            int v = tree.vertexByPos[pos];
            if (!(1 <= v && v <= n_)) continue;
            supportScratch_.supportPosStamp[v] = stamp;
            supportScratch_.supportPosVal[v] = pos;
        }
    }

    void annotateConnectorHandlesByPieces(ClassState& st, const vector<SupportPieceRef>& pieces) {
        if (pieces.empty()) return;
        unordered_map<int, const SupportTreeObject*> treeCache;
        for (int hi : st.connectorWatchEntryIds) {
            if (!(0 <= hi && hi < (int)st.watchHandles.size())) continue;
            auto& h = st.watchHandles[hi];
            bool assigned = false;
            for (const auto& piece : pieces) {
                const SupportTreeObject* tree = nullptr;
                auto it = treeCache.find(piece.treeId);
                if (it == treeCache.end()) {
                    tree = getSupportTreeObject(piece.treeId);
                    treeCache.emplace(piece.treeId, tree);
                } else tree = it->second;
                if (!tree) continue;
                int pos = h.localPos;
                if (h.treeId != piece.treeId || !(0 <= pos && pos < (int)tree->vertexByPos.size()) || tree->vertexByPos[pos] != h.vertex) {
                    int stamp = buildTreeVertexPosMapCached(*tree);
                    if (1 <= h.vertex && h.vertex <= n_ && supportScratch_.supportPosStamp[h.vertex] == stamp) pos = supportScratch_.supportPosVal[h.vertex];
                    else pos = -1;
                }
                if (0 <= pos && pieceContainsPos(*tree, piece, pos)) {
                    annotateHandleMetadata(h, SupportOriginKind::ConnectorTree, piece.treeId, piece.pieceId, pos);
                    assigned = true;
                    break;
                }
            }
            if (!assigned) {
                annotateHandleMetadata(h, SupportOriginKind::ConnectorTree, h.treeId, h.pieceId, h.localPos);
            }
        }
    }

    void annotateConnectorHandles(ClassState& st, const vector<int>& handleIdxs, int connectorTreeId) {
        vector<SupportPieceRef> pieces;
        const auto* tree = getSupportTreeObject(connectorTreeId);
        if (tree && connectorTreeId > 0) pieces.push_back(makeWholeTreePieceRef(connectorTreeId, tree->rootPos, -1, 0));
        st.connectorWatchEntryIds = handleIdxs;
        annotateConnectorHandlesByPieces(st, pieces);
    }

    void annotatePreservedHandlesByPieces(ClassState& st, const vector<SupportPieceRef>& pieces) {
        if (pieces.empty()) return;
        unordered_map<int, const SupportTreeObject*> treeCache;
        for (auto& h : st.watchHandles) {
            bool assigned = false;
            for (const auto& piece : pieces) {
                const SupportTreeObject* tree = nullptr;
                auto it = treeCache.find(piece.treeId);
                if (it == treeCache.end()) {
                    tree = getSupportTreeObject(piece.treeId);
                    treeCache.emplace(piece.treeId, tree);
                } else tree = it->second;
                if (!tree) continue;
                int pos = h.localPos;
                if (h.treeId != piece.treeId || !(0 <= pos && pos < (int)tree->vertexByPos.size()) || tree->vertexByPos[pos] != h.vertex) {
                    int stamp = buildTreeVertexPosMapCached(*tree);
                    if (1 <= h.vertex && h.vertex <= n_ && supportScratch_.supportPosStamp[h.vertex] == stamp) pos = supportScratch_.supportPosVal[h.vertex];
                    else pos = -1;
                }
                if (0 <= pos && pieceContainsPos(*tree, piece, pos)) {
                    annotateHandleMetadata(h, SupportOriginKind::PreservedPiece, piece.treeId, piece.pieceId, pos);
                    assigned = true;
                    break;
                }
            }
            if (!assigned) {
                if (h.originKind != SupportOriginKind::ConnectorTree)
                    annotateHandleMetadata(h, SupportOriginKind::MaterializedSupport, h.treeId, -1, h.localPos);
            }
        }
    }

    void rebuildConnectorWatchEntryIds(ClassState& st) {
        st.connectorWatchEntryIds.clear();
        for (int i = 0; i < (int)st.watchHandles.size(); ++i) {
            if (st.watchHandles[i].originKind == SupportOriginKind::ConnectorTree) st.connectorWatchEntryIds.push_back(i);
        }
    }

    void rebuildConnectorSkeletonCanonicalState(ClassState& st) {
        st.connectorSkeletonVertices.clear();
        st.connectorVertexToPos.clear();
        st.connectorSkeletonWatchHandleByVertex.clear();
        if (st.connectorTreeId > 0) {
            const auto* tree = getSupportTreeObject(st.connectorTreeId);
            if (tree) {
                st.connectorSkeletonVertices = tree->vertexByPos;
                for (int pos = 0; pos < (int)tree->vertexByPos.size(); ++pos) {
                    int v = tree->vertexByPos[pos];
                    if (1 <= v && v <= n_) st.connectorVertexToPos[v] = pos;
                }
            }
        }
        for (int hi : st.connectorWatchEntryIds) {
            if (!(0 <= hi && hi < (int)st.watchHandles.size())) continue;
            const auto& h = st.watchHandles[hi];
            if (h.originKind != SupportOriginKind::ConnectorTree) continue;
            if (!(1 <= h.vertex && h.vertex <= n_)) continue;
            st.connectorSkeletonWatchHandleByVertex[h.vertex] = hi;
        }
    }

    void clearMaterializedMetadataOnly(ClassState& st) {
        st.supportMetaValid = false;
        st.supportRootPos = -1;
        st.supportVerts.clear();
        st.supportParentPos.clear();
        st.supportDepth.clear();
        st.supportTin.clear();
        st.supportTout.clear();
        st.supportChildFirst.clear();
        st.supportNextSibling.clear();
        st.supportSubtreeEndpointCount.clear();
        st.supportSubtreeRepresentativeEndpoint.clear();
        st.supportNodeEndpointIdx.clear();
        st.supportPreorder.clear();
        st.activeEndpointIdxsSortedBySupportTin.clear();
        st.activeEndpointTinsSorted.clear();
        st.materializedTreeId = -1;
    }

    void clearPieceStateOnly(ClassState& st) {
        st.pieceModeActive = false;
        st.preservedPieces.clear();
        st.connectorPieces.clear();
        st.patchTreeIds.clear();
        st.attachmentVerticesByPiece.clear();
        st.connectorTreeId = -1;
        st.connectorWatchEntryIds.clear();
        st.connectorSkeletonVertices.clear();
        st.connectorVertexToPos.clear();
        st.connectorSkeletonWatchHandleByVertex.clear();
    }

    SupportPieceRef makeWholeTreePieceRef(int treeId, int attachmentVertexPos = -1, int repEndpoint = -1, int endpointCount = 0) {
        const auto* tree = getSupportTreeObject(treeId);
        if (!tree || tree->rootPos < 0) return SupportPieceRef();
        SupportPieceRef ref;
        ref.pieceId = nextPieceId_++;
        ref.treeId = treeId;
        ref.entryVertexPos = tree->rootPos;
        ref.blockedParentPos = -1;
        ref.attachmentVertexPos = attachmentVertexPos;
        ref.pieceRepresentativeEndpoint = repEndpoint;
        ref.pieceEndpointCount = endpointCount;
        ref.pieceAlive = true;
        ref.complementOfBlockedSubtree = false;
        return ref;
    }

    void syncAttachmentVerticesByPiece(ClassState& st) {
        st.attachmentVerticesByPiece.clear();
        st.attachmentVerticesByPiece.reserve(st.preservedPieces.size());
        for (const auto& piece : st.preservedPieces) {
            int v = -1;
            const auto* tree = getSupportTreeObject(piece.treeId);
            if (tree && 0 <= piece.attachmentVertexPos && piece.attachmentVertexPos < (int)tree->vertexByPos.size()) v = tree->vertexByPos[piece.attachmentVertexPos];
            st.attachmentVerticesByPiece.push_back(v);
        }
    }

    void ensureConnectorPiecesMigrated(ClassState& st) {
        if (!st.connectorPieces.empty()) return;
        if (st.connectorTreeId <= 0) return;
        const auto* tree = getSupportTreeObject(st.connectorTreeId);
        if (!tree || tree->rootPos < 0) return;
        st.connectorPieces.push_back(makeWholeTreePieceRef(st.connectorTreeId, tree->rootPos, -1, 0));
        if (find(st.patchTreeIds.begin(), st.patchTreeIds.end(), st.connectorTreeId) == st.patchTreeIds.end()) st.patchTreeIds.push_back(st.connectorTreeId);
    }

    int storeSupportTreeObjectFromClassState(const ClassState& st) {
        if (!st.supportMetaValid || st.supportVerts.empty() || st.supportRootPos < 0) return -1;
        SupportTreeObject obj;
        obj.treeId = nextSupportTreeId_++;
        obj.rootPos = st.supportRootPos;
        obj.parentPos = st.supportParentPos;
        obj.depth = st.supportDepth;
        obj.tin = st.supportTin;
        obj.tout = st.supportTout;
        obj.preorder = st.supportPreorder;
        obj.vertexByPos = st.supportVerts;
        obj.endpointIdxByPos = st.supportNodeEndpointIdx;
        for (int pos : st.supportPreorder) if (0 <= pos && pos < (int)st.supportNodeEndpointIdx.size() && st.supportNodeEndpointIdx[pos] >= 0) obj.endpointPosSorted.push_back(pos);
        obj.watchEntryIdsByPos.assign(st.supportVerts.size(), -1);
        for (int i = 0; i < (int)st.watchHandles.size() && i < (int)obj.watchEntryIdsByPos.size(); ++i) obj.watchEntryIdsByPos[i] = st.watchHandles[i].slotPos;
        supportTrees_.push_back(std::move(obj));
        return supportTrees_.back().treeId;
    }

    int storeSupportTreeObjectFromProduct(const SupportBuildProduct& prod, const vector<int>* nodeEndpointIdx = nullptr) {
        if (prod.watchVerts.empty() || prod.rootPos < 0) return -1;
        SupportTreeObject obj;
        obj.treeId = nextSupportTreeId_++;
        obj.rootPos = prod.rootPos;
        obj.parentPos = prod.parentPos;
        obj.depth = prod.depth;
        obj.tin = prod.tin;
        obj.tout = prod.tout;
        obj.preorder = prod.preorder;
        obj.vertexByPos = prod.watchVerts;
        if (nodeEndpointIdx) obj.endpointIdxByPos = *nodeEndpointIdx; else obj.endpointIdxByPos.assign(prod.watchVerts.size(), -1);
        for (int pos : prod.preorder) if (0 <= pos && pos < (int)obj.endpointIdxByPos.size() && obj.endpointIdxByPos[pos] >= 0) obj.endpointPosSorted.push_back(pos);
        obj.watchEntryIdsByPos.assign(prod.watchVerts.size(), -1);
        supportTrees_.push_back(std::move(obj));
        return supportTrees_.back().treeId;
    }

    bool ensureMaterializedTreeId(ClassState& st) {
        if (st.materializedTreeId > 0) return true;
        if (!st.supportMetaValid) return false;
        int tid = storeSupportTreeObjectFromClassState(st);
        if (tid <= 0) return false;
        st.materializedTreeId = tid;
        return true;
    }

    bool pieceContainsPos(const SupportTreeObject& tree, const SupportPieceRef& piece, int pos) const {
        if (!(0 <= pos && pos < (int)tree.vertexByPos.size())) return false;
        if (piece.complementOfBlockedSubtree) {
            if (!(0 <= piece.blockedParentPos && piece.blockedParentPos < (int)tree.vertexByPos.size())) return false;
            int lo = tree.tin[piece.blockedParentPos];
            int hi = tree.tout[piece.blockedParentPos];
            int tp = tree.tin[pos];
            return !(lo <= tp && tp <= hi);
        }
        if (!(0 <= piece.entryVertexPos && piece.entryVertexPos < (int)tree.vertexByPos.size())) return false;
        int lo = tree.tin[piece.entryVertexPos];
        int hi = tree.tout[piece.entryVertexPos];
        int tp = tree.tin[pos];
        return lo <= tp && tp <= hi;
    }

    int pieceVertexCount(const SupportTreeObject& tree, const SupportPieceRef& piece) const {
        if (piece.complementOfBlockedSubtree) {
            if (!(0 <= piece.blockedParentPos && piece.blockedParentPos < (int)tree.vertexByPos.size())) return 0;
            int sz = tree.tout[piece.blockedParentPos] - tree.tin[piece.blockedParentPos] + 1;
            return max(0, (int)tree.vertexByPos.size() - sz);
        }
        if (!(0 <= piece.entryVertexPos && piece.entryVertexPos < (int)tree.vertexByPos.size())) return 0;
        return max(0, tree.tout[piece.entryVertexPos] - tree.tin[piece.entryVertexPos] + 1);
    }

    bool pieceContainsVertex(const SupportPieceRef& piece, int vertex) {
        const auto* tree = getSupportTreeObject(piece.treeId);
        if (!tree || !(1 <= vertex && vertex <= n_)) return false;
        int stamp = 0;
        buildTreeVertexPosMap(*tree, stamp);
        if (supportScratch_.supportPosStamp[vertex] != stamp) return false;
        int pos = supportScratch_.supportPosVal[vertex];
        return (0 <= pos && pieceContainsPos(*tree, piece, pos));
    }

    template <class Fn>
    void forEachPiecePos(const SupportTreeObject& tree, const SupportPieceRef& piece, Fn&& fn) const {
        if (piece.complementOfBlockedSubtree) {
            if (!(0 <= piece.blockedParentPos && piece.blockedParentPos < (int)tree.vertexByPos.size())) return;
            int lo = tree.tin[piece.blockedParentPos];
            int hi = tree.tout[piece.blockedParentPos];
            for (int t = 0; t < (int)tree.preorder.size(); ++t) {
                if (lo <= t && t <= hi) continue;
                fn(tree.preorder[t]);
            }
            return;
        }
        if (!(0 <= piece.entryVertexPos && piece.entryVertexPos < (int)tree.vertexByPos.size())) return;
        int lo = tree.tin[piece.entryVertexPos];
        int hi = tree.tout[piece.entryVertexPos];
        for (int t = lo; t <= hi; ++t) fn(tree.preorder[t]);
    }

    SupportPieceRef makeSubtreePieceRef(int treeId, int entryPos, int blockedParentPos,
                                        int attachmentVertexPos, int repEndpoint, int endpointCount) {
        SupportPieceRef ref;
        ref.pieceId = nextPieceId_++;
        ref.treeId = treeId;
        ref.entryVertexPos = entryPos;
        ref.blockedParentPos = blockedParentPos;
        ref.attachmentVertexPos = attachmentVertexPos;
        ref.pieceRepresentativeEndpoint = repEndpoint;
        ref.pieceEndpointCount = endpointCount;
        ref.pieceAlive = true;
        ref.complementOfBlockedSubtree = false;
        return ref;
    }

    SupportPieceRef makeComplementPieceRef(int treeId, int rootPos, int blockedSubtreePos,
                                           int attachmentVertexPos, int repEndpoint, int endpointCount) {
        SupportPieceRef ref;
        ref.pieceId = nextPieceId_++;
        ref.treeId = treeId;
        ref.entryVertexPos = rootPos;
        ref.blockedParentPos = blockedSubtreePos;
        ref.attachmentVertexPos = attachmentVertexPos;
        ref.pieceRepresentativeEndpoint = repEndpoint;
        ref.pieceEndpointCount = endpointCount;
        ref.pieceAlive = true;
        ref.complementOfBlockedSubtree = true;
        return ref;
    }

    void bumpWatchLiveEntriesPeak() {
#ifdef LOCAL
        g_batch_dbg.watch_live_entries_peak = max<long long>(g_batch_dbg.watch_live_entries_peak, currentSupportWatch_);
#endif
    }

    void unregisterClassWatch(int owner, int cid, ClassState& st) {
        if (!st.watchHandles.empty()) {
#ifdef LOCAL
            g_batch_dbg.watch_unregister_vertices += (long long)st.watchHandles.size();
#endif
            for (const auto& h : st.watchHandles) {
                if (!(1 <= h.vertex && h.vertex <= n_)) continue;
                auto& vec = watchByVertex_[h.vertex];
                if (!(0 <= h.slotPos && h.slotPos < (int)vec.size())) continue;
                WatchEntry moved = vec.back();
                if (h.slotPos != (int)vec.size() - 1) {
                    vec[h.slotPos] = moved;
                    if (1 <= moved.owner && moved.owner <= n_) {
                        auto itOwner = ownerData_[moved.owner].classStates.find(moved.cid);
                        if (itOwner != ownerData_[moved.owner].classStates.end() &&
                            0 <= moved.handleIdx && moved.handleIdx < (int)itOwner->second.watchHandles.size()) {
                            itOwner->second.watchHandles[moved.handleIdx].slotPos = h.slotPos;
                        }
                    }
                }
                vec.pop_back();
            }
            st.watchHandles.clear();
        }
        if (st.watchActive) {
            currentSupportWatch_ -= st.watchVertexCount;
            st.watchActive = false;
            st.watchVertexCount = 0;
        } else {
            st.watchVertexCount = 0;
        }
        clearMaterializedMetadataOnly(st);
        clearPieceStateOnly(st);
    }

    void registerClassWatch(int owner, int cid, ClassState& st, const vector<int>& watchVerts) {
#ifdef LOCAL
        long long __watch_start_ns = (g_connector_skeleton_register_ctx > 0) ? dbg_now_ns() : 0;
#endif
        st.watchHandles.clear();
        st.watchHandles.reserve(watchVerts.size());
        for (int v : watchVerts) {
            if (!(1 <= v && v <= n_)) continue;
            auto& vec = watchByVertex_[v];
            int handleIdx = (int)st.watchHandles.size();
            st.watchHandles.push_back({v, (int)vec.size(), SupportOriginKind::MaterializedSupport, -1, -1, -1});
            vec.push_back({owner, cid, handleIdx});
        }
        st.watchActive = !st.watchHandles.empty();
        st.watchVertexCount = (int)st.watchHandles.size();
#ifdef LOCAL
        g_batch_dbg.watch_register_vertices += (long long)st.watchVertexCount;
        if (g_connector_skeleton_register_ctx > 0) {
            g_batch_dbg.connector_skeleton_watch_register += (long long)st.watchVertexCount;
            g_batch_dbg.time_connector_skeleton_watch_register_ns += dbg_now_ns() - __watch_start_ns;
            g_batch_dbg.time_connector_skeleton_watch_register_calls++;
        }
#endif
        currentSupportWatch_ += st.watchVertexCount;
        bumpSupportWatchPeak();
        bumpWatchLiveEntriesPeak();
    }

    void appendClassWatchEntries(int owner, int cid, ClassState& st, const vector<int>& watchVerts, vector<int>* outHandleIdxs = nullptr) {
#ifdef LOCAL
        long long __watch_start_ns = (g_connector_skeleton_register_ctx > 0) ? dbg_now_ns() : 0;
        long long __watch_added = 0;
#endif
        if (outHandleIdxs) outHandleIdxs->clear();
        for (int v : watchVerts) {
            if (!(1 <= v && v <= n_)) continue;
            auto& vec = watchByVertex_[v];
            int handleIdx = (int)st.watchHandles.size();
            st.watchHandles.push_back({v, (int)vec.size(), SupportOriginKind::MaterializedSupport, -1, -1, -1});
            vec.push_back({owner, cid, handleIdx});
            if (outHandleIdxs) outHandleIdxs->push_back(handleIdx);
            ++st.watchVertexCount;
            ++currentSupportWatch_;
#ifdef LOCAL
            g_batch_dbg.watch_register_vertices++;
            __watch_added++;
#endif
        }
        st.watchActive = !st.watchHandles.empty();
#ifdef LOCAL
        if (g_connector_skeleton_register_ctx > 0) {
            g_batch_dbg.connector_skeleton_watch_register += __watch_added;
            if (local_profile_coarse_enabled()) {
                g_batch_dbg.time_connector_skeleton_watch_register_ns += dbg_now_ns() - __watch_start_ns;
                g_batch_dbg.time_connector_skeleton_watch_register_calls++;
            }
        }
#endif
        bumpSupportWatchPeak();
        bumpWatchLiveEntriesPeak();
    }

    void retainClassWatchByKeepMask(int owner, int cid, ClassState& st, const vector<char>& keepMask, const vector<int>* sparseRemoveIdxs = nullptr) {
#ifdef LOCAL
        long long __watch_start_ns = (g_connector_skeleton_unregister_ctx > 0) ? dbg_now_ns() : 0;
        long long __watch_removed = 0;
        const bool __wscan_volume = local_profile_coarse_enabled() && g_wscan_retain_ctx > 0;
        const bool __wscan_detail = local_profile_detailed_enabled() && g_wscan_retain_ctx > 0;
        long long __dt_remove_bitmap_build = 0;
        long long __dt_sparse_remove_list_build = 0;
        long long __dt_watchByVertex_pop = 0;
        long long __dt_moved_entry_owner_lookup = 0;
        long long __dt_moved_entry_same_owner_fastpath = 0;
        long long __dt_moved_entry_slotpos_patch = 0;
        long long __dt_kept_vector_build = 0;
        long long __dt_kept_handle_copy = 0;
        long long __dt_kept_handleidx_patch = 0;
        long long __dt_final_swap_state_update = 0;
        long long __dt_kvec_prefix_fastpath_check = 0;
        long long __dt_kvec_suffix_fastpath_check = 0;
        long long __dt_kvec_kept_count_scan = 0;
        long long __dt_kvec_scratch_prepare = 0;
        long long __dt_kvec_stable_emit_unchanged_prefix = 0;
        long long __dt_kvec_stable_emit_moved_suffix = 0;
        long long __dt_kvec_patchlist_build = 0;
        long long __dt_kvec_handleidx_patch_changed_only = 0;
        long long __dt_kvec_handleidx_patch_skip_same_index = 0;
        long long __dt_kvec_final_resize_or_swap = 0;
        long long __dt_scomp_first_removed_seek = 0;
        long long __dt_scomp_suffix_only_check = 0;
        long long __dt_scomp_kept_count_scan = 0;
        long long __dt_scomp_kept_run_partition_build = 0;
        long long __dt_scomp_prefix_skip = 0;
        long long __dt_scomp_contiguous_run_block_copy = 0;
        long long __dt_scomp_elementwise_emit = 0;
        long long __dt_scomp_scratch_prepare = 0;
        long long __dt_scomp_tail_cleanup = 0;
        long long __dt_scomp_final_resize_swap = 0;
        long long __dt_bcopy_single_middle_run_detect = 0;
        long long __dt_bcopy_run_coalesce_build = 0;
        long long __dt_bcopy_direct_suffix_memmove = 0;
        long long __dt_bcopy_multi_run_block_copy = 0;
        long long __dt_bcopy_short_fragment_elementwise_fallback = 0;
        long long __dt_bcopy_overlap_safe_staging = 0;
        long long __dt_plan_first_removed_seek = 0;
        long long __dt_plan_removed_run_discovery = 0;
        long long __dt_plan_kept_run_discovery = 0;
        long long __dt_plan_adjacent_run_coalesce = 0;
        long long __dt_plan_single_middle_shortcircuit_eligibility = 0;
        long long __dt_plan_dst_index_accumulate = 0;
        long long __dt_plan_descriptor_emit = 0;
        long long __dt_plan_small_inline_buffer_prepare = 0;
        long long __dt_rdisc_first_removed_seek = 0;
        long long __dt_rdisc_boundary_reuse_check = 0;
        long long __dt_rdisc_removed_run_scan = 0;
        long long __dt_rdisc_kept_run_scan = 0;
        long long __dt_rdisc_suffix_only_shortcircuit = 0;
        long long __dt_rdisc_single_middle_shortcircuit = 0;
        long long __dt_rdisc_fused_onepass_scan = 0;
        long long __dt_rdisc_small_runlist_inline_materialize = 0;
        long long __dt_fclass_suffix_only_gate = 0;
        long long __dt_fclass_single_middle_gate = 0;
        long long __dt_fclass_onepass_transition_scan = 0;
        long long __dt_fclass_transition_emit_runs = 0;
        long long __dt_fclass_run_count_finalize = 0;
        long long __dt_fclass_small_runlist_inline = 0;
        long long __dt_tscan_window_seed = 0;
        long long __dt_tscan_boundary_clip = 0;
        long long __dt_tscan_state_load_compare = 0;
        long long __dt_tscan_removed_to_kept_detect = 0;
        long long __dt_tscan_kept_to_removed_detect = 0;
        long long __dt_tscan_run_boundary_commit = 0;
        long long __dt_tscan_tail_stop_check = 0;
        long long __dt_tscan_early_exit_finalize = 0;
        long long __bcopy_single_middle_run_calls = 0;
        long long __bcopy_removed_run_count = 0;
        long long __bcopy_kept_run_count = 0;
        long long __bcopy_copy_plan_entries = 0;
        long long __bcopy_direct_memmove_calls = 0;
        long long __bcopy_direct_memmoved_handles = 0;
        long long __bcopy_block_copied_handles = 0;
        long long __bcopy_elementwise_fallback_handles = 0;
        long long __bcopy_overlap_staging_calls = 0;
        long long __plan_first_removed_index = 0;
        long long __plan_removed_run_count = 0;
        long long __plan_kept_run_count = 0;
        long long __plan_adjacent_merge_hits = 0;
        long long __plan_descriptor_count = 0;
        long long __plan_dst_index_updates = 0;
        long long __plan_single_middle_shortcircuit_hits = 0;
        long long __plan_small_inline_hits = 0;
        long long __rdisc_first_removed_index = 0;
        long long __rdisc_removed_run_count = 0;
        long long __rdisc_kept_run_count = 0;
        long long __rdisc_boundary_reuse_hits = 0;
        long long __rdisc_suffix_only_hits = 0;
        long long __rdisc_single_middle_hits = 0;
        long long __rdisc_fused_onepass_calls = 0;
        long long __rdisc_small_runlist_inline_hits = 0;
        long long __fclass_suffix_only_hits = 0;
        long long __fclass_single_middle_hits = 0;
        long long __fclass_fused_onepass_calls = 0;
        long long __fclass_transition_steps = 0;
        long long __fclass_removed_to_kept_transitions = 0;
        long long __fclass_kept_to_removed_transitions = 0;
        long long __fclass_small_inline_hits = 0;
        long long __tscan_first_removed_index = 0;
        long long __tscan_scan_window_handles = 0;
        long long __tscan_removed_to_kept_transitions = 0;
        long long __tscan_kept_to_removed_transitions = 0;
        long long __tscan_boundary_reuse_hits = 0;
        long long __tscan_tail_stop_hits = 0;
        long long __tscan_early_exit_calls = 0;
        long long __kvec_handle_copy_entries = 0;
        long long __kvec_handleidx_patch_changed_entries = 0;
        long long __kvec_handleidx_patch_skipped_same_index_entries = 0;
        long long __kvec_changed_patchlist_entries = 0;
        long long __kvec_unchanged_prefix_handles = 0;
        long long __kvec_unchanged_suffix_handles = 0;
        long long __kvec_moved_suffix_handles = 0;
        long long __kvec_inplace_compact_calls = 0;
        long long __kvec_suffix_resize_fastpath_calls = 0;
        long long __scomp_first_removed_index = 0;
        long long __scomp_removed_run_count = 0;
        long long __scomp_kept_run_count = 0;
        long long __scomp_prefix_skipped_handles = 0;
        long long __scomp_block_copied_handles = 0;
        long long __scomp_elementwise_emitted_handles = 0;
        long long __scomp_suffix_only_calls = 0;
        long long __scomp_single_middle_run_calls = 0;
        long long __scomp_scratch_capacity_reuse_calls = 0;
        auto __acc_retain = [&](long long dt, long long* ns, long long* calls, long long* umbrellaNs, long long* umbrellaCalls) {
            if (!__wscan_detail || dt <= 0) return;
            if (ns) *ns += dt;
            if (calls) (*calls)++;
            if (umbrellaNs) *umbrellaNs += dt;
            if (umbrellaCalls) (*umbrellaCalls)++;
            g_batch_dbg.time_reuse_watch_retain_ns += dt;
            g_batch_dbg.time_reuse_watch_retain_calls++;
            add_wscan_route_time(dt);
        };
        auto __acc_simple = [&](long long dt, long long* ns, long long* calls) {
            if (!__wscan_detail || dt <= 0) return;
            if (ns) *ns += dt;
            if (calls) (*calls)++;
        };
        auto __acc_bcopy = [&](long long dt, long long* ns, long long* calls) {
            if (!__wscan_detail || dt <= 0) return;
            if (ns) *ns += dt;
            if (calls) (*calls)++;
            add_bcopy_route_time(dt);
        };
        auto __acc_plan = [&](long long dt, long long* ns, long long* calls) {
            if (!__wscan_detail || dt <= 0) return;
            if (ns) *ns += dt;
            if (calls) (*calls)++;
            __dt_bcopy_run_coalesce_build += dt;
            add_plan_route_time(dt);
        };
        auto __acc_rdisc = [&](long long dt, long long* ns, long long* calls) {
            if (!__wscan_detail || dt <= 0) return;
            if (ns) *ns += dt;
            if (calls) (*calls)++;
            add_rdisc_route_time(dt);
        };
        auto __acc_fclass = [&](long long dt, long long* ns, long long* calls) {
            if (!__wscan_detail || dt <= 0) return;
            if (ns) *ns += dt;
            if (calls) (*calls)++;
            add_fclass_route_time(dt);
        };
        auto __acc_tscan = [&](long long dt, long long* ns, long long* calls) {
            if (!__wscan_detail || dt <= 0) return;
            if (ns) *ns += dt;
            if (calls) (*calls)++;
            add_tscan_route_time(dt);
        };
#endif
        const bool __kvec_opt = kept_vector_opt_enabled();
        const bool __scomp_opt = stable_compaction_opt_enabled();
        const bool __bcopy_opt = block_copy_compaction_opt_enabled();
        const bool __plan_opt = copy_plan_build_opt_enabled();
        const bool __rdisc_opt = run_discovery_fusion_opt_enabled();
        const bool __fclass_opt = fused_discovery_classify_opt_enabled();
        const bool __tscan_opt = tscan_core_opt_enabled();
        int m = (int)st.watchHandles.size();
        if ((int)keepMask.size() != m) return;
#ifdef LOCAL
        if (__wscan_volume) {
            g_batch_dbg.retain_calls++;
            g_batch_dbg.retain_watch_handles_before += (long long)m;
            g_batch_dbg.kvec_calls++;
            g_batch_dbg.kvec_watch_handles_before += (long long)m;
            g_batch_dbg.scomp_calls++;
            g_batch_dbg.scomp_watch_handles_before += (long long)m;
            if (__tscan_opt) {
                g_batch_dbg.tscan_calls++;
                g_batch_dbg.tscan_watch_handles_before += (long long)m;
            }
        }
#endif
        vector<char> remove;
        vector<int> removedIdxs;
        bool useSparseFastPath = false;
        if (retain_compaction_opt_enabled() && sparseRemoveIdxs && !sparseRemoveIdxs->empty()) {
#ifdef LOCAL
            long long __sparse_build_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
            removedIdxs = *sparseRemoveIdxs;
            sort(removedIdxs.begin(), removedIdxs.end());
            vector<int> cleaned;
            cleaned.reserve(removedIdxs.size());
            int prev = -1;
            for (int idx : removedIdxs) {
                if (!(0 <= idx && idx < m)) continue;
                if (keepMask[idx]) continue;
                if (idx == prev) continue;
                cleaned.push_back(idx);
                prev = idx;
            }
            removedIdxs.swap(cleaned);
#ifdef LOCAL
            if (__wscan_detail) __dt_sparse_remove_list_build += dbg_now_ns() - __sparse_build_start_ns;
#endif
            useSparseFastPath = !removedIdxs.empty();
#ifdef LOCAL
            if (__wscan_volume && useSparseFastPath) {
                g_batch_dbg.retain_removed_sparse_calls++;
                g_batch_dbg.retain_removed_sparse_entries += (long long)removedIdxs.size();
                g_batch_dbg.retain_sparse_remove_fastpath_calls++;
                g_batch_dbg.retain_sparse_remove_fastpath_removed_entries += (long long)removedIdxs.size();
            }
#endif
        } else {
#ifdef LOCAL
            long long __bitmap_build_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
            remove.assign(m, 0);
            removedIdxs.reserve(m);
            for (int i = 0; i < m; ++i) if (!keepMask[i]) {
                remove[i] = 1;
                removedIdxs.push_back(i);
            }
#ifdef LOCAL
            if (__wscan_detail) __dt_remove_bitmap_build += dbg_now_ns() - __bitmap_build_start_ns;
            if (__wscan_volume) {
                g_batch_dbg.retain_removed_dense_calls++;
                g_batch_dbg.retain_remove_bitmap_entries += (long long)m;
            }
#endif
        }
        int removedCnt = (int)removedIdxs.size();
#ifdef LOCAL
        if (__wscan_volume) {
            if (m > 0) {
                g_batch_dbg.retain_remove_ratio_ppm_sum += (1000000LL * removedCnt) / m;
                g_batch_dbg.kvec_removed_ratio_ppm_sum += (1000000LL * removedCnt) / m;
                g_batch_dbg.scomp_removed_ratio_ppm_sum += (1000000LL * removedCnt) / m;
            }
            if (removedCnt == 0) {
                g_batch_dbg.retain_noop_calls++;
                g_batch_dbg.wscan_retain_noop_calls++;
                g_batch_dbg.retain_watch_handles_after += (long long)m;
                g_batch_dbg.kvec_noop_calls++;
                g_batch_dbg.kvec_watch_handles_after += (long long)m;
                g_batch_dbg.scomp_noop_calls++;
                g_batch_dbg.scomp_watch_handles_after += (long long)m;
            }
        }
#endif
        if (removedCnt == 0) return;
#ifdef LOCAL
        if (__wscan_volume) {
            g_batch_dbg.wscan_retain_removed_handles += removedCnt;
            g_batch_dbg.retain_removed_handles += removedCnt;
            g_batch_dbg.kvec_removed_handles += removedCnt;
            g_batch_dbg.scomp_removed_handles += removedCnt;
        }
#endif
#ifdef LOCAL
        if (__wscan_volume && __kvec_opt && __scomp_opt) {
            g_batch_dbg.bcopy_calls++;
            g_batch_dbg.bcopy_watch_handles_before += (long long)m;
            g_batch_dbg.bcopy_removed_handles += (long long)removedCnt;
            note_bcopy_route_call();
            g_batch_dbg.plan_calls++;
            g_batch_dbg.plan_watch_handles_before += (long long)m;
            g_batch_dbg.plan_removed_handles += (long long)removedCnt;
            if (m > 0) g_batch_dbg.plan_removed_ratio_ppm_sum += (1000000LL * removedCnt) / m;
            note_plan_route_call();
            if (__plan_opt) {
                g_batch_dbg.rdisc_calls++;
                g_batch_dbg.rdisc_watch_handles_before += (long long)m;
                g_batch_dbg.rdisc_removed_handles += (long long)removedCnt;
                if (m > 0) g_batch_dbg.rdisc_removed_ratio_ppm_sum += (1000000LL * removedCnt) / m;
                note_rdisc_route_call();
                if (__rdisc_opt) {
                    g_batch_dbg.fclass_calls++;
                    g_batch_dbg.fclass_watch_handles_before += (long long)m;
                    g_batch_dbg.fclass_removed_handles += (long long)removedCnt;
                    note_fclass_route_call();
                }
            }
        }
#endif
        std::unordered_map<unsigned long long, ClassState*> movedStateCache;
        if (retain_compaction_opt_enabled() && removedCnt > 0) movedStateCache.reserve((size_t)removedCnt * 2 + 1);
        auto __lookup_cache_key = [](int own, int cls) -> unsigned long long {
            return (unsigned long long)(uint32_t)own << 32 | (uint32_t)cls;
        };
        auto __process_removed_index = [&](int idx) {
            if (!(0 <= idx && idx < m)) return;
            const auto h = st.watchHandles[idx];
#ifdef LOCAL
            if (__wscan_volume) {
                if (h.originKind == SupportOriginKind::ConnectorTree) g_batch_dbg.retain_removed_connector_handles++;
                else if (h.originKind == SupportOriginKind::PreservedPiece) g_batch_dbg.retain_removed_preserved_handles++;
            }
#endif
            if (!(1 <= h.vertex && h.vertex <= n_)) return;
            auto& vec = watchByVertex_[h.vertex];
            if (!(0 <= h.slotPos && h.slotPos < (int)vec.size())) return;
            bool movedNeeded = (h.slotPos != (int)vec.size() - 1);
            WatchEntry moved = vec.back();
#ifdef LOCAL
            long long __pop_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
            if (movedNeeded) vec[h.slotPos] = moved;
            vec.pop_back();
#ifdef LOCAL
            if (__wscan_detail) __dt_watchByVertex_pop += dbg_now_ns() - __pop_start_ns;
            g_batch_dbg.watch_unregister_vertices++;
            __watch_removed++;
            if (__wscan_volume) g_batch_dbg.retain_watchByVertex_pop_calls++;
#endif
            if (movedNeeded) {
#ifdef LOCAL
                if (__wscan_volume) g_batch_dbg.retain_moved_entry_count++;
#endif
                ClassState* movedState = nullptr;
                if (moved.owner == owner && moved.cid == cid && 0 <= moved.handleIdx && moved.handleIdx < (int)st.watchHandles.size()) {
#ifdef LOCAL
                    long long __same_owner_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                    movedState = &st;
#ifdef LOCAL
                    if (__wscan_detail) __dt_moved_entry_same_owner_fastpath += dbg_now_ns() - __same_owner_start_ns;
                    if (__wscan_volume) g_batch_dbg.retain_moved_entry_same_owner_fastpath_hits++;
#endif
                } else if (1 <= moved.owner && moved.owner <= n_) {
#ifdef LOCAL
                    long long __owner_lookup_start_ns = __wscan_detail ? dbg_now_ns() : 0;
                    if (__wscan_volume) {
                        g_batch_dbg.retain_owner_lookup_calls++;
                        g_batch_dbg.wscan_retain_owner_state_lookups++;
                    }
#endif
                    unsigned long long key = __lookup_cache_key(moved.owner, moved.cid);
                    auto itCache = movedStateCache.find(key);
                    if (itCache != movedStateCache.end()) {
                        movedState = itCache->second;
#ifdef LOCAL
                        if (__wscan_volume && movedState) g_batch_dbg.retain_owner_lookup_hits++;
#endif
                    } else {
                        auto itOwner = ownerData_[moved.owner].classStates.find(moved.cid);
                        if (itOwner != ownerData_[moved.owner].classStates.end() &&
                            0 <= moved.handleIdx && moved.handleIdx < (int)itOwner->second.watchHandles.size()) {
                            movedState = &itOwner->second;
                            movedStateCache.emplace(key, movedState);
#ifdef LOCAL
                            if (__wscan_volume) g_batch_dbg.retain_owner_lookup_hits++;
#endif
                        } else {
#ifdef LOCAL
                            if (__wscan_volume) g_batch_dbg.retain_owner_lookup_misses++;
#endif
                        }
                    }
#ifdef LOCAL
                    if (__wscan_detail) __dt_moved_entry_owner_lookup += dbg_now_ns() - __owner_lookup_start_ns;
#endif
                }
                if (movedState) {
#ifdef LOCAL
                    long long __slot_patch_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                    if (movedState->watchHandles[moved.handleIdx].slotPos != h.slotPos) {
                        movedState->watchHandles[moved.handleIdx].slotPos = h.slotPos;
#ifdef LOCAL
                        if (__wscan_volume) {
                            g_batch_dbg.retain_slotpos_fixups++;
                            g_batch_dbg.wscan_retain_slotpos_fixups++;
                        }
#endif
                    } else {
#ifdef LOCAL
                        if (__wscan_volume) g_batch_dbg.retain_skip_slotpos_patch_calls++;
#endif
                    }
#ifdef LOCAL
                    if (__wscan_detail) __dt_moved_entry_slotpos_patch += dbg_now_ns() - __slot_patch_start_ns;
#endif
                }
            }
            --currentSupportWatch_;
        };
        if (useSparseFastPath) {
            for (int idx : removedIdxs) __process_removed_index(idx);
        } else {
            for (int i = 0; i < m; ++i) if (remove[i]) __process_removed_index(i);
        }

#ifdef LOCAL
        long long __first_seek_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
        int newSize = m - removedCnt;
        int firstRemoved = removedIdxs.empty() ? m : removedIdxs.front();
        int lastRemoved = removedIdxs.empty() ? -1 : removedIdxs.back();
#ifdef LOCAL
        if (__wscan_detail) {
            long long __dt = dbg_now_ns() - __first_seek_start_ns;
            __dt_scomp_first_removed_seek += __dt;
            __dt_plan_first_removed_seek += __dt;
            __dt_rdisc_first_removed_seek += __dt;
            __acc_plan(__dt, &g_batch_dbg.time_plan_first_removed_seek_ns, &g_batch_dbg.time_plan_first_removed_seek_calls);
            __acc_rdisc(__dt, &g_batch_dbg.time_rdisc_first_removed_seek_ns, &g_batch_dbg.time_rdisc_first_removed_seek_calls);
        }
        __scomp_first_removed_index = firstRemoved;
        __plan_first_removed_index = firstRemoved;
        if (__wscan_volume) {
            g_batch_dbg.kvec_first_removed_index_sum += firstRemoved;
            g_batch_dbg.kvec_last_removed_suffix_len_sum += (lastRemoved >= 0 ? (long long)(m - 1 - lastRemoved) : 0LL);
            g_batch_dbg.scomp_first_removed_index_sum += firstRemoved;
            if (__kvec_opt && __scomp_opt && __bcopy_opt) {
                g_batch_dbg.plan_first_removed_index_sum += firstRemoved;
                if (firstRemoved > 0 || lastRemoved >= 0) g_batch_dbg.plan_prefix_suffix_boundary_reuse_hits++;
            }
        }
#endif
        int removedRunCount = 0;
        int keptRunCount = 0;
        if (__plan_opt && __rdisc_opt) {
#ifdef LOCAL
            long long __fused_discovery_start_ns = __wscan_detail ? dbg_now_ns() : 0;
            long long __fclass_onepass_start_ns = (__wscan_detail && __fclass_opt) ? dbg_now_ns() : 0;
#endif
            if (removedCnt > 0) {
                removedRunCount = 1;
                for (int i = 1; i < removedCnt; ++i) {
                    if (removedIdxs[i] != removedIdxs[i - 1] + 1) ++removedRunCount;
                }
            }
            if (newSize > 0) {
                if (removedCnt == 0) keptRunCount = 1;
                else keptRunCount = removedRunCount + 1 - (firstRemoved == 0 ? 1 : 0) - (lastRemoved == m - 1 ? 1 : 0);
            }
#ifdef LOCAL
            if (__wscan_detail) {
                long long __dt = dbg_now_ns() - __fused_discovery_start_ns;
                __dt_rdisc_fused_onepass_scan += __dt;
                __acc_rdisc(__dt, &g_batch_dbg.time_rdisc_fused_onepass_scan_ns, &g_batch_dbg.time_rdisc_fused_onepass_scan_calls);
            }
            if (__wscan_detail && __fclass_opt) {
                long long __dt = dbg_now_ns() - __fclass_onepass_start_ns;
                __dt_fclass_onepass_transition_scan += __dt;
                __acc_fclass(__dt, &g_batch_dbg.time_fclass_onepass_transition_scan_ns, &g_batch_dbg.time_fclass_onepass_transition_scan_calls);
            }
            if (__wscan_volume) {
                g_batch_dbg.rdisc_fused_onepass_calls++;
                g_batch_dbg.rdisc_fused_scan_steps += removedCnt;
                if (removedCnt == 0) g_batch_dbg.rdisc_shortcircuit_skipped_removed_scan_calls++;
                if (removedCnt <= 1) g_batch_dbg.rdisc_shortcircuit_skipped_kept_scan_calls++;
                if (__fclass_opt) {
                    g_batch_dbg.fclass_fused_onepass_calls++;
                    g_batch_dbg.fclass_transition_steps += removedCnt;
                    long long __r2k = removedCnt > 0 ? (removedRunCount - (lastRemoved == m - 1 ? 1 : 0)) : 0;
                    long long __k2r = removedCnt > 0 ? (removedRunCount - (firstRemoved == 0 ? 1 : 0)) : 0;
                    if (__r2k < 0) __r2k = 0;
                    if (__k2r < 0) __k2r = 0;
                    g_batch_dbg.fclass_removed_to_kept_transitions += __r2k;
                    g_batch_dbg.fclass_kept_to_removed_transitions += __k2r;
                    __fclass_fused_onepass_calls++;
                    __fclass_transition_steps += removedCnt;
                    __fclass_removed_to_kept_transitions += __r2k;
                    __fclass_kept_to_removed_transitions += __k2r;
                }
            }
            __rdisc_fused_onepass_calls++;
#endif
        } else {
#ifdef LOCAL
            long long __removed_run_discovery_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
            if (removedCnt > 0) {
                removedRunCount = 1;
                for (int i = 1; i < removedCnt; ++i) if (removedIdxs[i] != removedIdxs[i - 1] + 1) ++removedRunCount;
            }
#ifdef LOCAL
            if (__wscan_detail) {
                long long __dt = dbg_now_ns() - __removed_run_discovery_start_ns;
                __dt_plan_removed_run_discovery += __dt;
                __acc_plan(__dt, &g_batch_dbg.time_plan_removed_run_discovery_ns, &g_batch_dbg.time_plan_removed_run_discovery_calls);
                __dt_rdisc_removed_run_scan += __dt;
                __acc_rdisc(__dt, &g_batch_dbg.time_rdisc_removed_run_scan_ns, &g_batch_dbg.time_rdisc_removed_run_scan_calls);
            }
            if (__wscan_volume && __plan_opt) {
                g_batch_dbg.rdisc_two_pass_removed_scan_calls++;
                g_batch_dbg.rdisc_removed_scan_steps += removedCnt;
            }
#endif
#ifdef LOCAL
            long long __kept_run_discovery_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
            if (newSize > 0) {
                if (firstRemoved > 0) ++keptRunCount;
                if (removedCnt > 0) {
                    for (int i = 1; i < removedCnt; ++i) if (removedIdxs[i - 1] + 1 <= removedIdxs[i] - 1) ++keptRunCount;
                    if (lastRemoved + 1 <= m - 1) ++keptRunCount;
                } else {
                    keptRunCount = 1;
                }
            }
#ifdef LOCAL
            if (__wscan_detail) {
                long long __dt = dbg_now_ns() - __kept_run_discovery_start_ns;
                __dt_plan_kept_run_discovery += __dt;
                __acc_plan(__dt, &g_batch_dbg.time_plan_kept_run_discovery_ns, &g_batch_dbg.time_plan_kept_run_discovery_calls);
                __dt_rdisc_kept_run_scan += __dt;
                __acc_rdisc(__dt, &g_batch_dbg.time_rdisc_kept_run_scan_ns, &g_batch_dbg.time_rdisc_kept_run_scan_calls);
            }
            if (__wscan_volume && __plan_opt) {
                g_batch_dbg.rdisc_two_pass_kept_scan_calls++;
                g_batch_dbg.rdisc_kept_scan_steps += removedCnt;
            }
#endif
        }
#ifdef LOCAL
        __scomp_removed_run_count = removedRunCount;
        __scomp_kept_run_count = keptRunCount;
        __bcopy_removed_run_count = removedRunCount;
        __bcopy_kept_run_count = keptRunCount;
        __plan_removed_run_count = removedRunCount;
        __plan_kept_run_count = keptRunCount;
        __rdisc_removed_run_count = removedRunCount;
        __rdisc_kept_run_count = keptRunCount;
        __rdisc_first_removed_index = firstRemoved;
        if (__wscan_volume) {
            g_batch_dbg.scomp_removed_run_count_sum += removedRunCount;
            g_batch_dbg.scomp_kept_run_count_sum += keptRunCount;
            if (__kvec_opt && __scomp_opt) {
                g_batch_dbg.bcopy_removed_run_count_sum += removedRunCount;
                g_batch_dbg.bcopy_kept_run_count_sum += keptRunCount;
                g_batch_dbg.plan_removed_run_count_sum += removedRunCount;
                g_batch_dbg.plan_kept_run_count_sum += keptRunCount;
                if (__plan_opt) {
                    g_batch_dbg.rdisc_first_removed_index_sum += firstRemoved;
                    g_batch_dbg.rdisc_removed_run_count_sum += removedRunCount;
                    g_batch_dbg.rdisc_kept_run_count_sum += keptRunCount;
                    if (__rdisc_opt && __fclass_opt) {
                        g_batch_dbg.fclass_run_count_finalize_calls++;
                    }
                }
            }
        }
        if (__wscan_detail && __fclass_opt) {
            long long __fclass_finalize_start_ns = dbg_now_ns();
            long long __dt = dbg_now_ns() - __fclass_finalize_start_ns;
            __dt_fclass_run_count_finalize += __dt;
            __acc_fclass(__dt, &g_batch_dbg.time_fclass_run_count_finalize_ns, &g_batch_dbg.time_fclass_run_count_finalize_calls);
        }
#endif

        bool removedIsSuffix = false;
#ifdef LOCAL
        long long __prefix_check_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
        if (firstRemoved > 0) {
#ifdef LOCAL
            __scomp_prefix_skipped_handles += firstRemoved;
            __kvec_unchanged_prefix_handles += firstRemoved;
            if (__wscan_volume) {
                g_batch_dbg.kvec_prefix_fastpath_hits++;
                g_batch_dbg.kvec_unchanged_prefix_handles += (long long)firstRemoved;
                g_batch_dbg.scomp_prefix_skip_hits++;
                g_batch_dbg.scomp_prefix_skipped_handles += (long long)firstRemoved;
            }
#endif
        }
#ifdef LOCAL
        if (__wscan_detail) {
            long long __dt = dbg_now_ns() - __prefix_check_start_ns;
            __dt_kvec_prefix_fastpath_check += __dt;
            __dt_kvec_stable_emit_unchanged_prefix += __dt;
            __dt_scomp_prefix_skip += __dt;
        }
        long long __boundary_check_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
        removedIsSuffix = (removedCnt > 0 && firstRemoved == newSize);
#ifdef LOCAL
        if (__wscan_detail) {
            long long __dt = dbg_now_ns() - __boundary_check_start_ns;
            __dt_kvec_suffix_fastpath_check += __dt;
            __dt_scomp_suffix_only_check += __dt;
            __dt_rdisc_boundary_reuse_check += __dt;
            __dt_plan_first_removed_seek += __dt;
            __acc_plan(__dt, &g_batch_dbg.time_plan_first_removed_seek_ns, &g_batch_dbg.time_plan_first_removed_seek_calls);
            __acc_rdisc(__dt, &g_batch_dbg.time_rdisc_boundary_reuse_check_ns, &g_batch_dbg.time_rdisc_boundary_reuse_check_calls);
            if (__fclass_opt) {
                __dt_fclass_suffix_only_gate += __dt;
                __acc_fclass(__dt, &g_batch_dbg.time_fclass_suffix_only_gate_ns, &g_batch_dbg.time_fclass_suffix_only_gate_calls);
            }
        }
#endif
#ifdef LOCAL
        long long __single_detect_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
        bool singleMiddleRun = (removedRunCount == 1 && firstRemoved > 0 && lastRemoved >= 0 && lastRemoved < m - 1);
#ifdef LOCAL
        if (__wscan_detail) {
            long long __dt = dbg_now_ns() - __single_detect_start_ns;
            __dt_bcopy_single_middle_run_detect += __dt;
            __dt_plan_single_middle_shortcircuit_eligibility += __dt;
            __acc_plan(__dt, &g_batch_dbg.time_plan_single_middle_shortcircuit_eligibility_ns, &g_batch_dbg.time_plan_single_middle_shortcircuit_eligibility_calls);
            if (removedIsSuffix) {
                __dt_rdisc_suffix_only_shortcircuit += __dt;
                __acc_rdisc(__dt, &g_batch_dbg.time_rdisc_suffix_only_shortcircuit_ns, &g_batch_dbg.time_rdisc_suffix_only_shortcircuit_calls);
            }
            if (singleMiddleRun) {
                __dt_rdisc_single_middle_shortcircuit += __dt;
                __acc_rdisc(__dt, &g_batch_dbg.time_rdisc_single_middle_shortcircuit_ns, &g_batch_dbg.time_rdisc_single_middle_shortcircuit_calls);
            }
            if (__fclass_opt) {
                __dt_fclass_single_middle_gate += __dt;
                __acc_fclass(__dt, &g_batch_dbg.time_fclass_single_middle_gate_ns, &g_batch_dbg.time_fclass_single_middle_gate_calls);
            }
        }
        if (__wscan_volume && __plan_opt) {
            if (removedIsSuffix) {
                g_batch_dbg.rdisc_boundary_reuse_hits++;
                g_batch_dbg.rdisc_prefix_suffix_boundary_reuse_hits++;
                g_batch_dbg.rdisc_suffix_only_hits++;
                g_batch_dbg.rdisc_shortcircuit_skipped_kept_scan_calls++;
                g_batch_dbg.rdisc_shortcircuit_skipped_removed_scan_calls++;
                __rdisc_boundary_reuse_hits++;
                __rdisc_suffix_only_hits++;
                if (__fclass_opt) {
                    g_batch_dbg.fclass_suffix_only_hits++;
                    g_batch_dbg.fclass_prefix_suffix_boundary_reuse_hits++;
                    g_batch_dbg.fclass_shortcircuit_skipped_scan_steps += removedCnt;
                    g_batch_dbg.fclass_shortcircuit_skipped_emit_calls++;
                    g_batch_dbg.fclass_run_finalize_skipped_calls++;
                    __fclass_suffix_only_hits++;
                }
            }
            if (singleMiddleRun) {
                g_batch_dbg.rdisc_single_middle_hits++;
                __rdisc_single_middle_hits++;
                if (__fclass_opt) {
                    g_batch_dbg.fclass_single_middle_hits++;
                    g_batch_dbg.fclass_shortcircuit_skipped_scan_steps += removedCnt;
                    g_batch_dbg.fclass_shortcircuit_skipped_emit_calls++;
                    __fclass_single_middle_hits++;
                }
            }
        }
        if (removedIsSuffix && __wscan_volume && __kvec_opt && __scomp_opt && __bcopy_opt) {
            g_batch_dbg.plan_suffix_only_shortcircuit_hits++;
            g_batch_dbg.plan_copy_plan_skipped_calls++;
        }
        if (singleMiddleRun) {
            if (__wscan_volume && __kvec_opt && __scomp_opt && __bcopy_opt) {
                g_batch_dbg.plan_single_middle_shortcircuit_hits++;
                g_batch_dbg.plan_copy_plan_skipped_calls++;
                g_batch_dbg.plan_descriptor_emit_skipped_for_direct_shift_calls++;
            }
            __plan_single_middle_shortcircuit_hits++;
            __scomp_single_middle_run_calls++;
            __bcopy_single_middle_run_calls++;
            if (__wscan_volume) {
                g_batch_dbg.scomp_single_middle_run_calls++;
                if (__kvec_opt && __scomp_opt) g_batch_dbg.bcopy_single_middle_run_calls++;
            }
        }
#endif

        auto __patch_handleidx = [&](int newIdx) {
            if (!(0 <= newIdx && newIdx < (int)st.watchHandles.size())) return;
            auto& h = st.watchHandles[newIdx];
            if (!(1 <= h.vertex && h.vertex <= n_)) return;
            auto& vec = watchByVertex_[h.vertex];
            if (!(0 <= h.slotPos && h.slotPos < (int)vec.size())) return;
            bool needPatch = (vec[h.slotPos].handleIdx != newIdx || vec[h.slotPos].owner != owner || vec[h.slotPos].cid != cid);
            if (needPatch) {
#ifdef LOCAL
                long long __patch_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                vec[h.slotPos].handleIdx = newIdx;
                vec[h.slotPos].owner = owner;
                vec[h.slotPos].cid = cid;
#ifdef LOCAL
                if (__wscan_detail) __dt_kvec_handleidx_patch_changed_only += dbg_now_ns() - __patch_start_ns;
                if (__wscan_volume) {
                    g_batch_dbg.retain_handleidx_fixups++;
                    g_batch_dbg.wscan_retain_handleidx_fixups++;
                    g_batch_dbg.kvec_handleidx_patch_changed_entries++;
                    __kvec_handleidx_patch_changed_entries++;
                }
#endif
            } else {
#ifdef LOCAL
                if (__wscan_volume) {
                    g_batch_dbg.retain_skip_handleidx_patch_calls++;
                    g_batch_dbg.kvec_handleidx_patch_skipped_same_index_entries++;
                    __kvec_handleidx_patch_skipped_same_index_entries++;
                }
#endif
            }
        };

        if (__kvec_opt) {
            if (removedIsSuffix) {
#ifdef LOCAL
                __scomp_suffix_only_calls++;
                if (__wscan_volume) {
                    g_batch_dbg.kvec_suffix_fastpath_hits++;
                    g_batch_dbg.kvec_suffix_resize_fastpath_calls++;
                    __kvec_suffix_resize_fastpath_calls++;
                    g_batch_dbg.scomp_suffix_only_calls++;
                    g_batch_dbg.scomp_suffix_resize_hits++;
                    g_batch_dbg.scomp_tail_cleared_handles += removedCnt;
                    if (__kvec_opt && __scomp_opt) {
                        g_batch_dbg.bcopy_suffix_only_calls++;
                        g_batch_dbg.bcopy_suffix_skipped_handles += (long long)(m - newSize);
                    }
                }
#endif
#ifdef LOCAL
                long long __tail_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                // tail cleanup is implicit through resize; account only the conceptual tail-cleared region
#ifdef LOCAL
                if (__wscan_detail) __dt_scomp_tail_cleanup += dbg_now_ns() - __tail_start_ns;
                long long __final_resize_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                st.watchHandles.resize(newSize);
                st.watchVertexCount = (int)st.watchHandles.size();
                st.watchActive = !st.watchHandles.empty();
#ifdef LOCAL
                if (__wscan_detail) {
                    long long __dt = dbg_now_ns() - __final_resize_start_ns;
                    __dt_kvec_final_resize_or_swap += __dt;
                    __dt_scomp_final_resize_swap += __dt;
                }
                if (__wscan_volume) {
                    g_batch_dbg.kvec_watch_handles_after += (long long)st.watchHandles.size();
                    g_batch_dbg.scomp_watch_handles_after += (long long)st.watchHandles.size();
                    if (__kvec_opt && __scomp_opt && __bcopy_opt) { g_batch_dbg.plan_watch_handles_after += (long long)st.watchHandles.size(); if (__plan_opt) { g_batch_dbg.rdisc_watch_handles_after += (long long)st.watchHandles.size(); if (__rdisc_opt && __fclass_opt) g_batch_dbg.fclass_watch_handles_after += (long long)st.watchHandles.size(); } }
                    g_batch_dbg.scomp_final_resize_calls++;
                }
#endif
            } else if (__scomp_opt) {
                static thread_local vector<pair<int,int>> __scratch_kept_runs;
                static thread_local vector<pair<int,int>> __scratch_patch_ranges;
#ifdef LOCAL
                if (__wscan_volume) g_batch_dbg.kvec_inplace_compact_calls++;
                __kvec_inplace_compact_calls++;
                long long __count_scan_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                int keptAfterPrefix = newSize - firstRemoved;
                (void)keptAfterPrefix;
#ifdef LOCAL
                if (__wscan_detail) {
                    long long __dt = dbg_now_ns() - __count_scan_start_ns;
                    __dt_kvec_kept_count_scan += __dt;
                    __dt_scomp_kept_count_scan += __dt;
                }
                long long __scratch_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                bool patchCapReuse = __scratch_patch_ranges.capacity() >= (size_t)max(1, keptRunCount);
                bool runCapReuse = __scratch_kept_runs.capacity() >= (size_t)max(1, keptRunCount);
                array<pair<int,int>, 16> __inline_kept_runs{};
                size_t __inline_kept_run_count = 0;
                const bool __use_inline_kept_runs = __plan_opt && ((size_t)keptRunCount <= __inline_kept_runs.size());
                __scratch_patch_ranges.clear();
                __scratch_kept_runs.clear();
                __scratch_patch_ranges.reserve((size_t)max(1, keptRunCount));
                if (!__use_inline_kept_runs) __scratch_kept_runs.reserve((size_t)max(1, keptRunCount));
#ifdef LOCAL
                if (__wscan_detail) {
                    long long __dt = dbg_now_ns() - __scratch_start_ns;
                    __dt_kvec_scratch_prepare += __dt;
                    __dt_scomp_scratch_prepare += __dt;
                    __dt_plan_small_inline_buffer_prepare += __dt;
                    __acc_plan(__dt, &g_batch_dbg.time_plan_small_inline_buffer_prepare_ns, &g_batch_dbg.time_plan_small_inline_buffer_prepare_calls);
                    if (__fclass_opt) {
                        __dt_fclass_small_runlist_inline += __dt;
                        __acc_fclass(__dt, &g_batch_dbg.time_fclass_small_runlist_inline_ns, &g_batch_dbg.time_fclass_small_runlist_inline_calls);
                    }
                }
                if (__wscan_volume) {
                    g_batch_dbg.kvec_scratch_vector_build_calls++;
                    g_batch_dbg.scomp_scratch_prepare_calls++;
                    if (patchCapReuse) { g_batch_dbg.kvec_capacity_reuse_calls++; g_batch_dbg.scomp_scratch_capacity_reuse_calls++; __scomp_scratch_capacity_reuse_calls++; }
                    if (runCapReuse) { g_batch_dbg.kvec_capacity_reuse_calls++; g_batch_dbg.scomp_scratch_capacity_reuse_calls++; __scomp_scratch_capacity_reuse_calls++; }
                    if (__kvec_opt && __scomp_opt && __bcopy_opt) {
                        if (__use_inline_kept_runs) {
                            g_batch_dbg.plan_small_inline_hits++;
                            __plan_small_inline_hits++;
                            g_batch_dbg.plan_small_inline_capacity_reuse_hits++;
                            if (__fclass_opt) {
                                g_batch_dbg.fclass_small_inline_hits++;
                                g_batch_dbg.fclass_small_inline_capacity_reuse_hits++;
                                __fclass_small_inline_hits++;
                            }
                        } else {
                            g_batch_dbg.plan_heap_plan_build_calls++;
                            if (runCapReuse) g_batch_dbg.plan_small_inline_capacity_reuse_hits++;
                            if (__fclass_opt) {
                                g_batch_dbg.fclass_heap_runlist_build_calls++;
                                if (runCapReuse) g_batch_dbg.fclass_small_inline_capacity_reuse_hits++;
                            }
                        }
                    }
                }
#endif
                auto __push_patch_range = [&](int startIdx, int len) {
                    if (len <= 0) return;
#ifdef LOCAL
                    long long __build_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                    __scratch_patch_ranges.emplace_back(startIdx, len);
#ifdef LOCAL
                    if (__wscan_detail) __dt_kvec_patchlist_build += dbg_now_ns() - __build_start_ns;
                    __kvec_changed_patchlist_entries += len;
                    if (__wscan_volume) g_batch_dbg.kvec_changed_patchlist_entries += len;
#endif
                };
                auto __emit_kept_run = [&](int srcL, int srcR) {
                    if (srcL > srcR) return;
#ifdef LOCAL
                    long long __emit_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                    if (__plan_opt) {
                        if (__use_inline_kept_runs) {
                            if (__inline_kept_run_count > 0) {
                                auto &bk = __inline_kept_runs[__inline_kept_run_count - 1];
                                if (bk.second + 1 >= srcL) {
                                    if (srcR > bk.second) bk.second = srcR;
#ifdef LOCAL
                                    if (__wscan_detail) {
                                        long long __dt = dbg_now_ns() - __emit_start_ns;
                                        __dt_plan_adjacent_run_coalesce += __dt;
                                        __acc_plan(__dt, &g_batch_dbg.time_plan_adjacent_run_coalesce_ns, &g_batch_dbg.time_plan_adjacent_run_coalesce_calls);
                                    }
                                    __plan_adjacent_merge_hits++;
                                    if (__wscan_volume && __kvec_opt && __scomp_opt && __bcopy_opt) {
                                        g_batch_dbg.plan_adjacent_merge_hits++;
                                        g_batch_dbg.bcopy_coalesced_run_merges++;
                                        g_batch_dbg.bcopy_adjacent_run_coalesce_hits++;
                                    }
#endif
                                    return;
                                }
                            }
                            __inline_kept_runs[__inline_kept_run_count++] = {srcL, srcR};
                        } else {
                            if (!__scratch_kept_runs.empty()) {
                                auto &bk = __scratch_kept_runs.back();
                                if (bk.second + 1 >= srcL) {
                                    if (srcR > bk.second) bk.second = srcR;
#ifdef LOCAL
                                    if (__wscan_detail) {
                                        long long __dt = dbg_now_ns() - __emit_start_ns;
                                        __dt_plan_adjacent_run_coalesce += __dt;
                                        __acc_plan(__dt, &g_batch_dbg.time_plan_adjacent_run_coalesce_ns, &g_batch_dbg.time_plan_adjacent_run_coalesce_calls);
                                    }
                                    __plan_adjacent_merge_hits++;
                                    if (__wscan_volume && __kvec_opt && __scomp_opt && __bcopy_opt) {
                                        g_batch_dbg.plan_adjacent_merge_hits++;
                                        g_batch_dbg.bcopy_coalesced_run_merges++;
                                        g_batch_dbg.bcopy_adjacent_run_coalesce_hits++;
                                    }
#endif
                                    return;
                                }
                            }
                            __scratch_kept_runs.emplace_back(srcL, srcR);
                        }
#ifdef LOCAL
                        if (__wscan_detail) {
                            long long __dt = dbg_now_ns() - __emit_start_ns;
                            __dt_plan_descriptor_emit += __dt;
                            __acc_plan(__dt, &g_batch_dbg.time_plan_descriptor_emit_ns, &g_batch_dbg.time_plan_descriptor_emit_calls);
                            if (__fclass_opt) {
                                __dt_fclass_transition_emit_runs += __dt;
                                __acc_fclass(__dt, &g_batch_dbg.time_fclass_transition_emit_runs_ns, &g_batch_dbg.time_fclass_transition_emit_runs_calls);
                            }
                        }
#endif
                    } else {
                        __scratch_kept_runs.emplace_back(srcL, srcR);
#ifdef LOCAL
                        if (__wscan_detail) {
                            long long __dt = dbg_now_ns() - __emit_start_ns;
                            __dt_plan_descriptor_emit += __dt;
                            __acc_plan(__dt, &g_batch_dbg.time_plan_descriptor_emit_ns, &g_batch_dbg.time_plan_descriptor_emit_calls);
                            if (__fclass_opt) {
                                __dt_fclass_transition_emit_runs += __dt;
                                __acc_fclass(__dt, &g_batch_dbg.time_fclass_transition_emit_runs_ns, &g_batch_dbg.time_fclass_transition_emit_runs_calls);
                            }
                        }
#endif
                    }
                };
#ifdef LOCAL
                long long __run_build_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                if (!singleMiddleRun) {
                    int prevRemoved = firstRemoved;
                    for (int i = 1; i < removedCnt; ++i) {
#ifdef LOCAL
                        long long __disc_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                        int srcL = prevRemoved + 1;
                        int srcR = removedIdxs[i] - 1;
#ifdef LOCAL
                        if (!__rdisc_opt && __wscan_detail) {
                            long long __dt = dbg_now_ns() - __disc_start_ns;
                            __dt_plan_kept_run_discovery += __dt;
                            __acc_plan(__dt, &g_batch_dbg.time_plan_kept_run_discovery_ns, &g_batch_dbg.time_plan_kept_run_discovery_calls);
                        }
#endif
                        if (srcL <= srcR) __emit_kept_run(srcL, srcR);
                        prevRemoved = removedIdxs[i];
                    }
#ifdef LOCAL
                    long long __disc_tail_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                    int __tailL = lastRemoved + 1;
                    int __tailR = m - 1;
#ifdef LOCAL
                    if (!__rdisc_opt && __wscan_detail) {
                        long long __dt = dbg_now_ns() - __disc_tail_start_ns;
                        __dt_plan_kept_run_discovery += __dt;
                        __acc_plan(__dt, &g_batch_dbg.time_plan_kept_run_discovery_ns, &g_batch_dbg.time_plan_kept_run_discovery_calls);
                    }
#endif
                    if (__tailL <= __tailR) __emit_kept_run(__tailL, __tailR);
                    if (!__plan_opt && __bcopy_opt && __scratch_kept_runs.size() >= 2) {
#ifdef LOCAL
                        long long __coalesce_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                        size_t wr = 0;
                        for (size_t i = 0; i < __scratch_kept_runs.size(); ++i) {
                            if (wr == 0) { __scratch_kept_runs[wr++] = __scratch_kept_runs[i]; continue; }
                            auto &bk = __scratch_kept_runs[wr - 1];
                            auto cur = __scratch_kept_runs[i];
                            if (bk.second + 1 >= cur.first) {
                                if (cur.second > bk.second) bk.second = cur.second;
#ifdef LOCAL
                                if (__wscan_volume) { g_batch_dbg.bcopy_coalesced_run_merges++; g_batch_dbg.bcopy_adjacent_run_coalesce_hits++; }
#endif
                            } else {
                                __scratch_kept_runs[wr++] = cur;
                            }
                        }
                        __scratch_kept_runs.resize(wr);
#ifdef LOCAL
                        if (__wscan_detail) {
                            long long __dt = dbg_now_ns() - __coalesce_start_ns;
                            __dt_plan_adjacent_run_coalesce += __dt;
                            __acc_plan(__dt, &g_batch_dbg.time_plan_adjacent_run_coalesce_ns, &g_batch_dbg.time_plan_adjacent_run_coalesce_calls);
                        }
#endif
                    }
                }
#ifdef LOCAL
                if (__wscan_detail) {
                    long long __dt = dbg_now_ns() - __run_build_start_ns;
                    __dt_scomp_kept_run_partition_build += __dt;
                }
                size_t __plan_desc_count = __use_inline_kept_runs ? __inline_kept_run_count : __scratch_kept_runs.size();
                __bcopy_copy_plan_entries += (long long)__plan_desc_count;
                __plan_descriptor_count += (long long)__plan_desc_count;
                if (__wscan_volume && __kvec_opt && __scomp_opt) {
                    g_batch_dbg.bcopy_copy_plan_entries += (long long)__plan_desc_count;
                    g_batch_dbg.plan_descriptor_count += (long long)__plan_desc_count;
                    g_batch_dbg.plan_copy_plan_rebuild_calls++;
                }
#endif
                int write = firstRemoved;
                if (singleMiddleRun) {
                    int srcL = lastRemoved + 1;
                    int srcR = m - 1;
                    int len = srcR - srcL + 1;
                    if (len > 0) {
#ifdef LOCAL
                        long long __copy_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                        std::memmove(&st.watchHandles[write], &st.watchHandles[srcL], sizeof(WatchHandle) * (size_t)len);
#ifdef LOCAL
                        if (__wscan_detail) {
                            long long __dt = dbg_now_ns() - __copy_start_ns;
                            __dt_scomp_contiguous_run_block_copy += __dt;
                            __dt_kvec_stable_emit_moved_suffix += __dt;
                            __dt_kept_handle_copy += __dt;
                            __dt_bcopy_direct_suffix_memmove += __dt;
                        }
                        __scomp_block_copied_handles += len;
                        __bcopy_direct_memmove_calls++;
                        __bcopy_direct_memmoved_handles += len;
                        __bcopy_block_copied_handles += len;
                        if (__wscan_volume) {
                            g_batch_dbg.scomp_block_copy_runs++;
                            g_batch_dbg.scomp_block_copied_handles += len;
                            g_batch_dbg.scomp_contiguous_middle_memmove_calls++;
                            g_batch_dbg.kvec_handle_copy_entries += len;
                            g_batch_dbg.kvec_moved_suffix_handles += len;
                            g_batch_dbg.bcopy_direct_memmove_calls++;
                            g_batch_dbg.bcopy_direct_memmoved_handles += len;
                            g_batch_dbg.bcopy_runwise_block_copy_calls++;
                            g_batch_dbg.bcopy_runwise_block_copied_handles += len;
                            g_batch_dbg.bcopy_contiguous_middle_memmove_calls++;
                            g_batch_dbg.bcopy_contiguous_middle_memmove_handles += len;
                            g_batch_dbg.bcopy_block_copy_threshold_hits++;
                            g_batch_dbg.bcopy_scratchless_overlap_safe_calls++;
                        }
                        __kvec_handle_copy_entries += len;
                        __kvec_moved_suffix_handles += len;
                        if (__wscan_volume) g_batch_dbg.scomp_inplace_suffix_shift_calls++;
#endif
                        __push_patch_range(write, len);
                        write += len;
                    }
                } else {
                    auto __visit_kept_runs = [&](auto&& __fn) {
                        if (__use_inline_kept_runs) {
                            for (size_t __ri = 0; __ri < __inline_kept_run_count; ++__ri) __fn(__inline_kept_runs[__ri].first, __inline_kept_runs[__ri].second);
                        } else {
                            for (auto [srcL, srcR] : __scratch_kept_runs) __fn(srcL, srcR);
                        }
                    };
                    __visit_kept_runs([&](int srcL, int srcR) {
                        int len = srcR - srcL + 1;
                        if (len <= 0) return;
#ifdef LOCAL
                        long long __dst_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                        int dst0 = write;
                        write += len;
#ifdef LOCAL
                        if (__wscan_detail) {
                            long long __dt = dbg_now_ns() - __dst_start_ns;
                            __dt_plan_dst_index_accumulate += __dt;
                            __acc_plan(__dt, &g_batch_dbg.time_plan_dst_index_accumulate_ns, &g_batch_dbg.time_plan_dst_index_accumulate_calls);
                        }
                        __plan_dst_index_updates++;
                        if (__wscan_volume && __kvec_opt && __scomp_opt && __bcopy_opt) g_batch_dbg.plan_dst_index_updates++;
#endif
                        bool sameSlot = (dst0 == srcL);
                        const bool useBlockCopy = __bcopy_opt ? (len >= 2) : (len >= 4);
                        if (sameSlot) {
#ifdef LOCAL
                            if (__wscan_volume && __kvec_opt && __scomp_opt) g_batch_dbg.bcopy_same_slot_skip_handles += len;
#endif
                            __push_patch_range(dst0, len);
                        } else if (useBlockCopy) {
#ifdef LOCAL
                            long long __copy_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                            std::memmove(&st.watchHandles[dst0], &st.watchHandles[srcL], sizeof(WatchHandle) * (size_t)len);
#ifdef LOCAL
                            if (__wscan_detail) {
                                long long __dt = dbg_now_ns() - __copy_start_ns;
                                __dt_scomp_contiguous_run_block_copy += __dt;
                                __dt_kvec_stable_emit_moved_suffix += __dt;
                                __dt_kept_handle_copy += __dt;
                                __dt_bcopy_multi_run_block_copy += __dt;
                            }
                            __scomp_block_copied_handles += len;
                            __bcopy_block_copied_handles += len;
                            if (__wscan_volume) {
                                g_batch_dbg.scomp_block_copy_runs++;
                                g_batch_dbg.scomp_block_copied_handles += len;
                                g_batch_dbg.kvec_handle_copy_entries += len;
                                g_batch_dbg.kvec_moved_suffix_handles += len;
                                g_batch_dbg.bcopy_runwise_block_copy_calls++;
                                g_batch_dbg.bcopy_runwise_block_copied_handles += len;
                                if (__bcopy_opt) g_batch_dbg.bcopy_block_copy_threshold_hits++;
                            }
                            __kvec_handle_copy_entries += len;
                            __kvec_moved_suffix_handles += len;
#endif
                            __push_patch_range(dst0, len);
                        } else {
#ifdef LOCAL
                            long long __emit_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                            for (int i = srcL, w = dst0; i <= srcR; ++i, ++w) st.watchHandles[w] = st.watchHandles[i];
#ifdef LOCAL
                            if (__wscan_detail) {
                                long long __dt = dbg_now_ns() - __emit_start_ns;
                                __dt_scomp_elementwise_emit += __dt;
                                __dt_kvec_stable_emit_moved_suffix += __dt;
                                __dt_kept_handle_copy += __dt;
                                __dt_bcopy_short_fragment_elementwise_fallback += __dt;
                            }
                            __scomp_elementwise_emitted_handles += len;
                            __bcopy_elementwise_fallback_handles += len;
                            if (__wscan_volume) {
                                g_batch_dbg.scomp_elementwise_emitted_handles += len;
                                g_batch_dbg.kvec_handle_copy_entries += len;
                                g_batch_dbg.kvec_moved_suffix_handles += len;
                                g_batch_dbg.bcopy_elementwise_fallback_calls++;
                                g_batch_dbg.bcopy_elementwise_fallback_handles += len;
                            }
                            __kvec_handle_copy_entries += len;
                            __kvec_moved_suffix_handles += len;
#endif
                            __push_patch_range(dst0, len);
                        }
                    });
                }
                for (auto [startIdx, len] : __scratch_patch_ranges) {
                    for (int newIdx = startIdx; newIdx < startIdx + len; ++newIdx) __patch_handleidx(newIdx);
                }
#ifdef LOCAL
                long long __tail_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                // stale tail becomes unreachable after resize; just account the logical tail cleared region
#ifdef LOCAL
                if (__wscan_detail) __dt_scomp_tail_cleanup += dbg_now_ns() - __tail_start_ns;
                if (__wscan_volume) g_batch_dbg.scomp_tail_cleared_handles += removedCnt;
                long long __final_resize_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                st.watchHandles.resize(newSize);
                st.watchVertexCount = (int)st.watchHandles.size();
                st.watchActive = !st.watchHandles.empty();
#ifdef LOCAL
                if (__wscan_detail) {
                    long long __dt = dbg_now_ns() - __final_resize_start_ns;
                    __dt_kvec_final_resize_or_swap += __dt;
                    __dt_scomp_final_resize_swap += __dt;
                }
                if (__wscan_volume) {
                    g_batch_dbg.kvec_watch_handles_after += (long long)st.watchHandles.size();
                    g_batch_dbg.scomp_watch_handles_after += (long long)st.watchHandles.size();
                    g_batch_dbg.scomp_final_resize_calls++;
                    g_batch_dbg.kvec_swap_skipped_calls++;
                    g_batch_dbg.scomp_swap_skipped_calls++;
                }
#endif
            } else {
#ifdef LOCAL
                if (__wscan_volume) g_batch_dbg.kvec_inplace_compact_calls++;
                __kvec_inplace_compact_calls++;
                long long __count_scan_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                int keptAfterPrefix = newSize - firstRemoved;
                (void)keptAfterPrefix;
#ifdef LOCAL
                if (__wscan_detail) {
                    long long __dt = dbg_now_ns() - __count_scan_start_ns;
                    __dt_kvec_kept_count_scan += __dt;
                    __dt_scomp_kept_count_scan += __dt;
                }
                long long __scratch_start_ns = __wscan_detail ? dbg_now_ns() : 0;
                if (__wscan_volume) g_batch_dbg.kvec_scratch_vector_build_calls++;
#endif
                vector<WatchHandle> kept;
                kept.reserve(newSize);
#ifdef LOCAL
                if (__wscan_detail) {
                    long long __dt = dbg_now_ns() - __scratch_start_ns;
                    __dt_kvec_scratch_prepare += __dt;
                    __dt_scomp_scratch_prepare += __dt;
                }
#endif
                int removedPtr = 0;
                for (int i = 0; i < m; ++i) {
                    bool isRemoved = false;
                    if (useSparseFastPath) {
                        while (removedPtr < removedCnt && removedIdxs[removedPtr] < i) ++removedPtr;
                        isRemoved = (removedPtr < removedCnt && removedIdxs[removedPtr] == i);
                    } else {
                        isRemoved = remove[i];
                    }
                    if (isRemoved) continue;
#ifdef LOCAL
                    long long __iter_start_ns = __wscan_detail ? dbg_now_ns() : 0;
                    long long __copy_dt = 0;
                    long long __patch_dt = 0;
#endif
                    int newIdx = (int)kept.size();
#ifdef LOCAL
                    long long __copy_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                    kept.push_back(st.watchHandles[i]);
#ifdef LOCAL
                    if (__wscan_detail) __copy_dt = dbg_now_ns() - __copy_start_ns;
                    if (__wscan_volume) {
                        g_batch_dbg.retain_kept_handles_copied++;
                        g_batch_dbg.kvec_handle_copy_entries++;
                        __kvec_handle_copy_entries++;
                        if (i < firstRemoved) {
                            g_batch_dbg.kvec_unchanged_prefix_handles++;
                            __kvec_unchanged_prefix_handles++;
                            g_batch_dbg.scomp_prefix_skipped_handles++;
                            __scomp_prefix_skipped_handles++;
                        } else if (newIdx == i) {
                            g_batch_dbg.kvec_unchanged_suffix_handles++;
                            __kvec_unchanged_suffix_handles++;
                        } else {
                            g_batch_dbg.kvec_moved_suffix_handles++;
                            __kvec_moved_suffix_handles++;
                            g_batch_dbg.scomp_elementwise_emitted_handles++;
                            __scomp_elementwise_emitted_handles++;
                        }
                    }
#endif
                    auto& h = kept.back();
                    if (1 <= h.vertex && h.vertex <= n_) {
                        auto& vec = watchByVertex_[h.vertex];
                        if (0 <= h.slotPos && h.slotPos < (int)vec.size()) {
                            bool needPatch = (vec[h.slotPos].handleIdx != newIdx || vec[h.slotPos].owner != owner || vec[h.slotPos].cid != cid);
                            if (needPatch) {
#ifdef LOCAL
                                long long __patch_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                                vec[h.slotPos].handleIdx = newIdx;
                                vec[h.slotPos].owner = owner;
                                vec[h.slotPos].cid = cid;
#ifdef LOCAL
                                if (__wscan_detail) __patch_dt = dbg_now_ns() - __patch_start_ns;
                                if (__wscan_volume) {
                                    g_batch_dbg.retain_handleidx_fixups++;
                                    g_batch_dbg.wscan_retain_handleidx_fixups++;
                                    g_batch_dbg.kvec_handleidx_patch_changed_entries++;
                                    __kvec_handleidx_patch_changed_entries++;
                                    if (newIdx != i) {
                                        g_batch_dbg.kvec_changed_patchlist_entries++;
                                        __kvec_changed_patchlist_entries++;
                                    }
                                }
#endif
                            } else {
#ifdef LOCAL
                                if (__wscan_volume) {
                                    g_batch_dbg.retain_skip_handleidx_patch_calls++;
                                    g_batch_dbg.kvec_handleidx_patch_skipped_same_index_entries++;
                                    __kvec_handleidx_patch_skipped_same_index_entries++;
                                }
#endif
                            }
                        }
                    }
#ifdef LOCAL
                    if (__wscan_detail) {
                        long long __iter_dt = dbg_now_ns() - __iter_start_ns;
                        long long __overhead_dt = __iter_dt - __copy_dt - __patch_dt;
                        if (__overhead_dt < 0) __overhead_dt = 0;
                        __dt_scomp_elementwise_emit += __overhead_dt + __copy_dt;
                        __dt_kvec_stable_emit_moved_suffix += __overhead_dt + __copy_dt;
                        __dt_kept_handle_copy += __copy_dt;
                        __dt_kvec_handleidx_patch_changed_only += __patch_dt;
                    }
#endif
                }
#ifdef LOCAL
                long long __tail_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                // swap-based path does not have separate explicit tail cleanup beyond the swap itself
#ifdef LOCAL
                if (__wscan_detail) __dt_scomp_tail_cleanup += dbg_now_ns() - __tail_start_ns;
                if (__wscan_volume) g_batch_dbg.scomp_tail_cleared_handles += removedCnt;
                long long __final_swap_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                st.watchHandles.swap(kept);
                st.watchVertexCount = (int)st.watchHandles.size();
                st.watchActive = !st.watchHandles.empty();
#ifdef LOCAL
                if (__wscan_detail) {
                    long long __dt = dbg_now_ns() - __final_swap_start_ns;
                    __dt_kvec_final_resize_or_swap += __dt;
                    __dt_scomp_final_resize_swap += __dt;
                }
                if (__wscan_volume) {
                    g_batch_dbg.kvec_watch_handles_after += (long long)st.watchHandles.size();
                    g_batch_dbg.scomp_watch_handles_after += (long long)st.watchHandles.size();
                    if (__kvec_opt && __scomp_opt && __bcopy_opt) { g_batch_dbg.plan_watch_handles_after += (long long)st.watchHandles.size(); if (__plan_opt) { g_batch_dbg.rdisc_watch_handles_after += (long long)st.watchHandles.size(); if (__rdisc_opt && __fclass_opt) g_batch_dbg.fclass_watch_handles_after += (long long)st.watchHandles.size(); } }
                    g_batch_dbg.scomp_final_resize_calls++;
                }
#endif
            }
        } else {
#ifdef LOCAL
            long long __count_scan_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
            int keptAfterPrefix = newSize - firstRemoved;
            (void)keptAfterPrefix;
#ifdef LOCAL
            if (__wscan_detail) {
                long long __dt = dbg_now_ns() - __count_scan_start_ns;
                __dt_kvec_kept_count_scan += __dt;
                __dt_scomp_kept_count_scan += __dt;
            }
            long long __scratch_start_ns = __wscan_detail ? dbg_now_ns() : 0;
            if (__wscan_volume) g_batch_dbg.kvec_scratch_vector_build_calls++;
#endif
            vector<WatchHandle> kept;
            kept.reserve(newSize);
#ifdef LOCAL
            if (__wscan_detail) {
                long long __dt = dbg_now_ns() - __scratch_start_ns;
                __dt_kvec_scratch_prepare += __dt;
                __dt_scomp_scratch_prepare += __dt;
            }
#endif
            int removedPtr = 0;
            for (int i = 0; i < m; ++i) {
                bool isRemoved = false;
                if (useSparseFastPath) {
                    while (removedPtr < removedCnt && removedIdxs[removedPtr] < i) ++removedPtr;
                    isRemoved = (removedPtr < removedCnt && removedIdxs[removedPtr] == i);
                } else {
                    isRemoved = remove[i];
                }
                if (isRemoved) continue;
#ifdef LOCAL
                long long __iter_start_ns = __wscan_detail ? dbg_now_ns() : 0;
                long long __copy_dt = 0;
                long long __patch_dt = 0;
#endif
                int newIdx = (int)kept.size();
#ifdef LOCAL
                long long __copy_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                kept.push_back(st.watchHandles[i]);
#ifdef LOCAL
                if (__wscan_detail) __copy_dt = dbg_now_ns() - __copy_start_ns;
#endif
                auto& h = kept.back();
                if (1 <= h.vertex && h.vertex <= n_) {
                    auto& vec = watchByVertex_[h.vertex];
                    if (0 <= h.slotPos && h.slotPos < (int)vec.size()) {
                        bool needPatch = (vec[h.slotPos].handleIdx != newIdx || vec[h.slotPos].owner != owner || vec[h.slotPos].cid != cid);
                        if (needPatch) {
#ifdef LOCAL
                            long long __patch_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
                            vec[h.slotPos].handleIdx = newIdx;
                            vec[h.slotPos].owner = owner;
                            vec[h.slotPos].cid = cid;
#ifdef LOCAL
                            if (__wscan_detail) __patch_dt = dbg_now_ns() - __patch_start_ns;
#endif
                        }
                    }
                }
#ifdef LOCAL
                if (__wscan_detail) {
                    long long __iter_dt = dbg_now_ns() - __iter_start_ns;
                    long long __overhead_dt = __iter_dt - __copy_dt - __patch_dt;
                    if (__overhead_dt < 0) __overhead_dt = 0;
                    __dt_scomp_elementwise_emit += __overhead_dt + __copy_dt;
                    __dt_kvec_stable_emit_moved_suffix += __overhead_dt + __copy_dt;
                    __dt_kept_handle_copy += __copy_dt;
                    __dt_kvec_handleidx_patch_changed_only += __patch_dt;
                }
#endif
            }
#ifdef LOCAL
            long long __final_swap_start_ns = __wscan_detail ? dbg_now_ns() : 0;
#endif
            st.watchHandles.swap(kept);
            st.watchVertexCount = (int)st.watchHandles.size();
            st.watchActive = !st.watchHandles.empty();
#ifdef LOCAL
            if (__wscan_detail) {
                long long __dt = dbg_now_ns() - __final_swap_start_ns;
                __dt_kvec_final_resize_or_swap += __dt;
                __dt_scomp_final_resize_swap += __dt;
            }
#endif
        }
#ifdef LOCAL
        if (__wscan_volume) {
            g_batch_dbg.retain_watch_handles_after += (long long)st.watchHandles.size();
            g_batch_dbg.scomp_watch_handles_after += (long long)st.watchHandles.size();
            g_batch_dbg.scomp_calls += 0;
            if (__kvec_opt && __scomp_opt) {
                g_batch_dbg.bcopy_watch_handles_after += (long long)st.watchHandles.size();
                g_batch_dbg.bcopy_prefix_skipped_handles += __scomp_prefix_skipped_handles;
                g_batch_dbg.bcopy_single_middle_run_calls += __bcopy_single_middle_run_calls;
            }
        }
        __dt_kept_vector_build += __dt_kvec_prefix_fastpath_check + __dt_kvec_suffix_fastpath_check + __dt_kvec_kept_count_scan + __dt_kvec_scratch_prepare + __dt_kvec_stable_emit_unchanged_prefix + __dt_kvec_stable_emit_moved_suffix;
        __dt_kept_handleidx_patch += __dt_kvec_patchlist_build + __dt_kvec_handleidx_patch_changed_only + __dt_kvec_handleidx_patch_skip_same_index;
        __dt_final_swap_state_update += __dt_kvec_final_resize_or_swap;
        __acc_retain(__dt_remove_bitmap_build,
                     &g_batch_dbg.time_retain_remove_bitmap_build_ns,
                     &g_batch_dbg.time_retain_remove_bitmap_build_calls,
                     &g_batch_dbg.time_wscan_retain_remove_entries_ns,
                     &g_batch_dbg.time_wscan_retain_remove_entries_calls);
        __acc_retain(__dt_sparse_remove_list_build,
                     &g_batch_dbg.time_retain_sparse_remove_list_build_ns,
                     &g_batch_dbg.time_retain_sparse_remove_list_build_calls,
                     &g_batch_dbg.time_wscan_retain_remove_entries_ns,
                     &g_batch_dbg.time_wscan_retain_remove_entries_calls);
        __acc_retain(__dt_watchByVertex_pop,
                     &g_batch_dbg.time_retain_watchByVertex_pop_ns,
                     &g_batch_dbg.time_retain_watchByVertex_pop_calls,
                     &g_batch_dbg.time_wscan_retain_remove_entries_ns,
                     &g_batch_dbg.time_wscan_retain_remove_entries_calls);
        __acc_retain(__dt_moved_entry_owner_lookup,
                     &g_batch_dbg.time_retain_moved_entry_owner_lookup_ns,
                     &g_batch_dbg.time_retain_moved_entry_owner_lookup_calls,
                     &g_batch_dbg.time_wscan_retain_owner_lookup_ns,
                     &g_batch_dbg.time_wscan_retain_owner_lookup_calls);
        __acc_retain(__dt_moved_entry_same_owner_fastpath,
                     &g_batch_dbg.time_retain_moved_entry_same_owner_fastpath_ns,
                     &g_batch_dbg.time_retain_moved_entry_same_owner_fastpath_calls,
                     &g_batch_dbg.time_wscan_retain_owner_lookup_ns,
                     &g_batch_dbg.time_wscan_retain_owner_lookup_calls);
        __acc_retain(__dt_moved_entry_slotpos_patch,
                     &g_batch_dbg.time_retain_moved_entry_slotpos_patch_ns,
                     &g_batch_dbg.time_retain_moved_entry_slotpos_patch_calls,
                     &g_batch_dbg.time_wscan_retain_slotpos_fixup_ns,
                     &g_batch_dbg.time_wscan_retain_slotpos_fixup_calls);
        __acc_retain(__dt_kept_vector_build,
                     &g_batch_dbg.time_retain_kept_vector_build_ns,
                     &g_batch_dbg.time_retain_kept_vector_build_calls,
                     &g_batch_dbg.time_wscan_retain_compact_handles_ns,
                     &g_batch_dbg.time_wscan_retain_compact_handles_calls);
        __acc_retain(__dt_kept_handle_copy,
                     &g_batch_dbg.time_retain_kept_handle_copy_ns,
                     &g_batch_dbg.time_retain_kept_handle_copy_calls,
                     &g_batch_dbg.time_wscan_retain_compact_handles_ns,
                     &g_batch_dbg.time_wscan_retain_compact_handles_calls);
        __acc_retain(__dt_kept_handleidx_patch,
                     &g_batch_dbg.time_retain_kept_handleidx_patch_ns,
                     &g_batch_dbg.time_retain_kept_handleidx_patch_calls,
                     &g_batch_dbg.time_wscan_retain_handleidx_fixup_ns,
                     &g_batch_dbg.time_wscan_retain_handleidx_fixup_calls);
        __acc_retain(__dt_final_swap_state_update,
                     &g_batch_dbg.time_retain_final_swap_state_update_ns,
                     &g_batch_dbg.time_retain_final_swap_state_update_calls,
                     &g_batch_dbg.time_wscan_retain_compact_handles_ns,
                     &g_batch_dbg.time_wscan_retain_compact_handles_calls);
        __acc_retain(__dt_kvec_prefix_fastpath_check,
                     &g_batch_dbg.time_kvec_prefix_fastpath_check_ns,
                     &g_batch_dbg.time_kvec_prefix_fastpath_check_calls,
                     &g_batch_dbg.time_retain_kept_vector_build_ns,
                     &g_batch_dbg.time_retain_kept_vector_build_calls);
        __acc_retain(__dt_kvec_suffix_fastpath_check,
                     &g_batch_dbg.time_kvec_suffix_fastpath_check_ns,
                     &g_batch_dbg.time_kvec_suffix_fastpath_check_calls,
                     &g_batch_dbg.time_retain_kept_vector_build_ns,
                     &g_batch_dbg.time_retain_kept_vector_build_calls);
        __acc_retain(__dt_kvec_kept_count_scan,
                     &g_batch_dbg.time_kvec_kept_count_scan_ns,
                     &g_batch_dbg.time_kvec_kept_count_scan_calls,
                     &g_batch_dbg.time_retain_kept_vector_build_ns,
                     &g_batch_dbg.time_retain_kept_vector_build_calls);
        __acc_retain(__dt_kvec_scratch_prepare,
                     &g_batch_dbg.time_kvec_scratch_prepare_ns,
                     &g_batch_dbg.time_kvec_scratch_prepare_calls,
                     &g_batch_dbg.time_retain_kept_vector_build_ns,
                     &g_batch_dbg.time_retain_kept_vector_build_calls);
        __acc_retain(__dt_kvec_stable_emit_unchanged_prefix,
                     &g_batch_dbg.time_kvec_stable_emit_unchanged_prefix_ns,
                     &g_batch_dbg.time_kvec_stable_emit_unchanged_prefix_calls,
                     &g_batch_dbg.time_retain_kept_vector_build_ns,
                     &g_batch_dbg.time_retain_kept_vector_build_calls);
        __acc_retain(__dt_kvec_stable_emit_moved_suffix,
                     &g_batch_dbg.time_kvec_stable_emit_moved_suffix_ns,
                     &g_batch_dbg.time_kvec_stable_emit_moved_suffix_calls,
                     &g_batch_dbg.time_retain_kept_vector_build_ns,
                     &g_batch_dbg.time_retain_kept_vector_build_calls);
        __acc_retain(__dt_kvec_patchlist_build,
                     &g_batch_dbg.time_kvec_patchlist_build_ns,
                     &g_batch_dbg.time_kvec_patchlist_build_calls,
                     &g_batch_dbg.time_retain_kept_handleidx_patch_ns,
                     &g_batch_dbg.time_retain_kept_handleidx_patch_calls);
        __acc_retain(__dt_kvec_handleidx_patch_changed_only,
                     &g_batch_dbg.time_kvec_handleidx_patch_changed_only_ns,
                     &g_batch_dbg.time_kvec_handleidx_patch_changed_only_calls,
                     &g_batch_dbg.time_retain_kept_handleidx_patch_ns,
                     &g_batch_dbg.time_retain_kept_handleidx_patch_calls);
        __acc_retain(__dt_kvec_handleidx_patch_skip_same_index,
                     &g_batch_dbg.time_kvec_handleidx_patch_skip_same_index_ns,
                     &g_batch_dbg.time_kvec_handleidx_patch_skip_same_index_calls,
                     &g_batch_dbg.time_retain_kept_handleidx_patch_ns,
                     &g_batch_dbg.time_retain_kept_handleidx_patch_calls);
        __acc_retain(__dt_kvec_final_resize_or_swap,
                     &g_batch_dbg.time_kvec_final_resize_or_swap_ns,
                     &g_batch_dbg.time_kvec_final_resize_or_swap_calls,
                     &g_batch_dbg.time_retain_final_swap_state_update_ns,
                     &g_batch_dbg.time_retain_final_swap_state_update_calls);
        __acc_simple(__dt_scomp_first_removed_seek, &g_batch_dbg.time_scomp_first_removed_seek_ns, &g_batch_dbg.time_scomp_first_removed_seek_calls);
        __acc_simple(__dt_scomp_suffix_only_check, &g_batch_dbg.time_scomp_suffix_only_check_ns, &g_batch_dbg.time_scomp_suffix_only_check_calls);
        __acc_simple(__dt_scomp_kept_count_scan, &g_batch_dbg.time_scomp_kept_count_scan_ns, &g_batch_dbg.time_scomp_kept_count_scan_calls);
        __acc_simple(__dt_scomp_kept_run_partition_build, &g_batch_dbg.time_scomp_kept_run_partition_build_ns, &g_batch_dbg.time_scomp_kept_run_partition_build_calls);
        __acc_simple(__dt_scomp_prefix_skip, &g_batch_dbg.time_scomp_prefix_skip_ns, &g_batch_dbg.time_scomp_prefix_skip_calls);
        __acc_simple(__dt_scomp_contiguous_run_block_copy, &g_batch_dbg.time_scomp_contiguous_run_block_copy_ns, &g_batch_dbg.time_scomp_contiguous_run_block_copy_calls);
        __acc_simple(__dt_scomp_elementwise_emit, &g_batch_dbg.time_scomp_elementwise_emit_ns, &g_batch_dbg.time_scomp_elementwise_emit_calls);
        __acc_simple(__dt_scomp_scratch_prepare, &g_batch_dbg.time_scomp_scratch_prepare_ns, &g_batch_dbg.time_scomp_scratch_prepare_calls);
        __acc_simple(__dt_scomp_tail_cleanup, &g_batch_dbg.time_scomp_tail_cleanup_ns, &g_batch_dbg.time_scomp_tail_cleanup_calls);
        __acc_simple(__dt_scomp_final_resize_swap, &g_batch_dbg.time_scomp_final_resize_swap_ns, &g_batch_dbg.time_scomp_final_resize_swap_calls);
        __acc_bcopy(__dt_bcopy_single_middle_run_detect, &g_batch_dbg.time_bcopy_single_middle_run_detect_ns, &g_batch_dbg.time_bcopy_single_middle_run_detect_calls);
        __acc_bcopy(__dt_bcopy_run_coalesce_build, &g_batch_dbg.time_bcopy_run_coalesce_build_ns, &g_batch_dbg.time_bcopy_run_coalesce_build_calls);
        __acc_bcopy(__dt_bcopy_direct_suffix_memmove, &g_batch_dbg.time_bcopy_direct_suffix_memmove_ns, &g_batch_dbg.time_bcopy_direct_suffix_memmove_calls);
        __acc_bcopy(__dt_bcopy_multi_run_block_copy, &g_batch_dbg.time_bcopy_multi_run_block_copy_ns, &g_batch_dbg.time_bcopy_multi_run_block_copy_calls);
        __acc_bcopy(__dt_bcopy_short_fragment_elementwise_fallback, &g_batch_dbg.time_bcopy_short_fragment_elementwise_fallback_ns, &g_batch_dbg.time_bcopy_short_fragment_elementwise_fallback_calls);
        __acc_bcopy(__dt_bcopy_overlap_safe_staging, &g_batch_dbg.time_bcopy_overlap_safe_staging_ns, &g_batch_dbg.time_bcopy_overlap_safe_staging_calls);
        if (g_connector_skeleton_unregister_ctx > 0) {
            g_batch_dbg.connector_skeleton_watch_unregister += __watch_removed;
            if (local_profile_coarse_enabled()) {
                g_batch_dbg.time_connector_skeleton_watch_unregister_ns += dbg_now_ns() - __watch_start_ns;
                g_batch_dbg.time_connector_skeleton_watch_unregister_calls++;
            }
        }
#endif
        bumpWatchLiveEntriesPeak();
    }



    void appendEndpointToClassPool(int owner, int cid, int idx) {
        if (cid < 0) return;
        classState(owner, cid).endpointPool.push_back(idx);
    }

#ifdef LOCAL
    bool deltaPreservedHitEnabled() const {
        static bool v = local_env_enabled("ENABLE_DELTA_PRESERVED_HIT", true);
        return v;
    }
    bool deltaConnectorHitEnabled() const {
        static bool v = local_env_enabled("ENABLE_DELTA_CONNECTOR_HIT", true);
        return v;
    }
    bool connectorSkeletonForceEnabled() const {
        static bool v = local_env_enabled("DEBUG_FORCE_CONNECTOR_SKELETON_ON_UNANIMOUS", false);
        return v;
    }
    enum class DeltaToggleMode {
        BothOff,
        PreservedOnly,
        ConnectorOnly,
        BothOn,
    };
    DeltaToggleMode currentDeltaToggleMode() const {
        bool p = deltaPreservedHitEnabled();
        bool c = deltaConnectorHitEnabled();
        if (!p && !c) return DeltaToggleMode::BothOff;
        if (p && !c) return DeltaToggleMode::PreservedOnly;
        if (!p && c) return DeltaToggleMode::ConnectorOnly;
        return DeltaToggleMode::BothOn;
    }
    const char* deltaToggleModeName() const {
        switch (currentDeltaToggleMode()) {
            case DeltaToggleMode::BothOff: return "both_off";
            case DeltaToggleMode::PreservedOnly: return "preserved_only";
            case DeltaToggleMode::ConnectorOnly: return "connector_only";
            case DeltaToggleMode::BothOn: return "both_on";
        }
        return "unknown";
    }
#endif

    bool classCurrentSupportContainsVertex(int owner, const ClassState& st, int v) {
        if (!(1 <= v && v <= n_)) return false;
        if (st.supportMetaValid) {
            for (int u : st.supportVerts) if (u == v) return true;
            return false;
        }
        if (st.pieceModeActive) {
            for (const auto& p : st.preservedPieces) if (p.pieceAlive && pieceContainsVertex(p, v)) return true;
            for (const auto& p : st.connectorPieces) if (p.pieceAlive && pieceContainsVertex(p, v)) return true;
            if (st.connectorTreeId > 0) {
                const auto* t = getSupportTreeObject(st.connectorTreeId);
                if (t) {
                    int stamp = 0; buildTreeVertexPosMap(*t, stamp);
                    if (supportScratch_.supportPosStamp[v] == stamp) return true;
                }
            }
            for (int tid : st.patchTreeIds) {
                if (tid <= 0) continue;
                const auto* t = getSupportTreeObject(tid);
                if (!t) continue;
                int stamp = 0; buildTreeVertexPosMap(*t, stamp);
                if (supportScratch_.supportPosStamp[v] == stamp) return true;
            }
        }
        return false;
    }
#ifdef LOCAL
    void verifyTouchedClassesExactForDeletion(int x, const vector<TouchedClassInfo>& infos) {
        g_batch_dbg.debug_touched_check_calls++;
        unordered_set<long long> actual;
        actual.reserve(infos.size()*2+1);
        for (const auto& info : infos) actual.insert(watchKey(info.owner, info.oldCid));
        unordered_set<long long> exact;
        for (int owner = 1; owner <= n_; ++owner) {
            auto& od = ownerData_[owner];
            for (const auto& kv : od.classStates) {
                const auto& st = kv.second;
                if (!st.watchActive || st.activeQueryCount <= 0) continue;
                if (classCurrentSupportContainsVertex(owner, st, x)) exact.insert(watchKey(owner, kv.first));
            }
        }
        for (long long k : exact) if (!actual.count(k)) {
            g_batch_dbg.debug_touched_missing_classes++;
            if (!g_batch_dbg.debug_first_divergence_dumped) {
                g_batch_dbg.debug_first_divergence_dumped = 1;
                cerr << "FIRST_TOUCHED_MISSING x=" << x << " key=" << k << " mode=" << deltaToggleModeName() << "\n";
            }
        }
        for (long long k : actual) if (!exact.count(k)) {
            g_batch_dbg.debug_touched_extra_classes++;
            if (!g_batch_dbg.debug_first_divergence_dumped) {
                g_batch_dbg.debug_first_divergence_dumped = 1;
                cerr << "FIRST_TOUCHED_EXTRA x=" << x << " key=" << k << " mode=" << deltaToggleModeName() << "\n";
            }
        }
    }
#endif

    vector<int> collectRelevantEndpointIdxs(int owner, int cid, bool rewritePool) {
        vector<int> filtered;
        if (!(1 <= owner && owner <= n_) || !topo_.aliveVertex(owner)) return filtered;
        auto& od = ownerData_[owner];
        auto it = od.classStates.find(cid);
        if (it == od.classStates.end()) return filtered;
        auto& st = it->second;
        int mark = od.nextMark();
        filtered.reserve(st.endpointPool.size());
        for (int idx : st.endpointPool) {
            if (idx < 0 || idx >= (int)od.endpoints.size()) continue;
            if (od.endpointMark[idx] == mark) continue;
            od.endpointMark[idx] = mark;
            if (od.endpointActiveCount[idx] <= 0) continue;
            int ep = od.endpoints[idx];
            if (!topo_.aliveVertex(ep)) continue;
            if (topo_.incidentClass(owner, ep) != cid) continue;
            filtered.push_back(idx);
        }
        if (rewritePool) st.endpointPool = filtered;
        return filtered;
    }

    vector<int> fallbackRelevantEndpointIdxsFromQueries(int owner, int cid) {
        vector<int> out;
        if (!(1 <= owner && owner <= n_) || !topo_.aliveVertex(owner)) return out;
        auto& od = ownerData_[owner];
        int mark = od.nextMark();
        for (int qid : od.qids) {
            const auto& qs = qstate_[qid];
            if (!qs.active || qs.cid != cid) continue;
            if (qs.aIdx >= 0 && qs.aIdx < (int)od.endpoints.size() && od.endpointMark[qs.aIdx] != mark) {
                od.endpointMark[qs.aIdx] = mark;
                out.push_back(qs.aIdx);
            }
            if (qs.bIdx != qs.aIdx && qs.bIdx >= 0 && qs.bIdx < (int)od.endpoints.size() && od.endpointMark[qs.bIdx] != mark) {
                od.endpointMark[qs.bIdx] = mark;
                out.push_back(qs.bIdx);
            }
        }
        classState(owner, cid).endpointPool = out;
        return out;
    }

    int loadArtifactIntoScratch(const OwnerSplitArtifact& artifact) {
        supportScratch_.ensure(n_);
        const int stamp = supportScratch_.nextArtifact();
        for (size_t i = 0; i < artifact.visitedVerts.size(); ++i) {
            int v = artifact.visitedVerts[i];
            if (!(1 <= v && v <= n_)) continue;
            supportScratch_.artifactStamp[v] = stamp;
            supportScratch_.artifactParent[v] = artifact.parentVals[i];
            supportScratch_.artifactRoot[v] = artifact.rootVals[i];
            supportScratch_.artifactDepth[v] = artifact.depthVals[i];
        }
        return stamp;
    }

    int artifactLca(int a, int b, int artifactStamp) {
        if (!(1 <= a && a <= n_ && 1 <= b && b <= n_)) return -1;
        if (supportScratch_.artifactStamp[a] != artifactStamp || supportScratch_.artifactStamp[b] != artifactStamp) return -1;
        while (supportScratch_.artifactDepth[a] > supportScratch_.artifactDepth[b]) {
            int p = supportScratch_.artifactParent[a];
            if (!(1 <= p && p <= n_) || p == a) return -1;
            a = p;
        }
        while (supportScratch_.artifactDepth[b] > supportScratch_.artifactDepth[a]) {
            int p = supportScratch_.artifactParent[b];
            if (!(1 <= p && p <= n_) || p == b) return -1;
            b = p;
        }
        while (a != b) {
            int pa = supportScratch_.artifactParent[a];
            int pb = supportScratch_.artifactParent[b];
            if (!(1 <= pa && pa <= n_) || !(1 <= pb && pb <= n_) || pa == a || pb == b) return -1;
            a = pa;
            b = pb;
        }
        return a;
    }

    vector<int> buildSupportVerticesFromArtifact(int owner, const vector<int>& relevantVerts,
                                                 const OwnerSplitArtifact* artifact, int artifactStamp,
                                                 long long* outWatchV = nullptr,
                                                 long long* outChainSteps = nullptr) {
        vector<int> watchVerts;
        if (!artifact || !artifact->valid || relevantVerts.empty() || artifactStamp == 0) return watchVerts;
        supportScratch_.ensure(n_);
        if ((int)relevantVerts.size() == 1) {
            watchVerts.push_back(relevantVerts[0]);
            if (outWatchV) *outWatchV = 1;
            if (outChainSteps) *outChainSteps = 1;
            return watchVerts;
        }
        int zoneRoot = -1;
        for (int ep : relevantVerts) {
            if (!(1 <= ep && ep <= n_)) return {};
            if (supportScratch_.artifactStamp[ep] != artifactStamp) return {};
            int r = supportScratch_.artifactRoot[ep];
            if (!(1 <= r && r <= n_)) return {};
            if (zoneRoot == -1) zoneRoot = r;
            else if (zoneRoot != r) return {};
        }
        int commonRoot = relevantVerts[0];
        for (size_t i = 1; i < relevantVerts.size(); ++i) {
            commonRoot = artifactLca(commonRoot, relevantVerts[i], artifactStamp);
            if (!(1 <= commonRoot && commonRoot <= n_)) return {};
        }
        const int collectStamp = supportScratch_.nextCollect();
        long long chainSteps = 0;
        for (int ep : relevantVerts) {
            int cur = ep;
            while (true) {
                if (!(1 <= cur && cur <= n_)) return {};
                if (supportScratch_.artifactStamp[cur] != artifactStamp) return {};
                if (supportScratch_.collectStamp[cur] == collectStamp) break;
                supportScratch_.collectStamp[cur] = collectStamp;
                watchVerts.push_back(cur);
                ++chainSteps;
                if (cur == commonRoot) break;
                int p = supportScratch_.artifactParent[cur];
                if (!(1 <= p && p <= n_) || p == cur) return {};
                cur = p;
            }
        }
        if (outWatchV) *outWatchV = (long long)watchVerts.size();
        if (outChainSteps) *outChainSteps = chainSteps;
        return watchVerts;
    }

    vector<int> buildSupportVerticesFallback(int owner, const vector<int>& relevantVerts,
                                             long long* outVisitedV = nullptr,
                                             long long* outVisitedE = nullptr,
                                             int* outSeenStamp = nullptr,
                                             int* outRoot = nullptr) {
        vector<int> watchVerts;
        if (relevantVerts.empty()) return watchVerts;
        if ((int)relevantVerts.size() == 1) {
            watchVerts.push_back(relevantVerts[0]);
            if (outVisitedV) *outVisitedV = 1;
            if (outVisitedE) *outVisitedE = 0;
            if (outSeenStamp) *outSeenStamp = 0;
            if (outRoot) *outRoot = relevantVerts[0];
            return watchVerts;
        }
        supportScratch_.ensure(n_);
        const int seenStamp = supportScratch_.nextSeen();
        const int targetStamp = supportScratch_.nextTarget();
        const int collectStamp = supportScratch_.nextCollect();
        for (int ep : relevantVerts) supportScratch_.targetStamp[ep] = targetStamp;

        const auto& core = topo_.core();
        auto& q = supportScratch_.queueBuf;
        q.clear();
        int root = relevantVerts[0];
        q.push_back(root);
        supportScratch_.seenStamp[root] = seenStamp;
        supportScratch_.parent[root] = root;
        int found = 1;
        long long visV = 0, visE = 0;
        for (size_t qi = 0; qi < q.size(); ++qi) {
            int u = q[qi];
            ++visV;
            if (found == (int)relevantVerts.size()) break;
            for (int eid : core.incidentEdges(u)) {
                if (!core.edgeAlive(eid)) continue;
                ++visE;
                int v = core.other(eid, u);
                if (v == owner || !topo_.aliveVertex(v)) continue;
                if (supportScratch_.seenStamp[v] == seenStamp) continue;
                supportScratch_.seenStamp[v] = seenStamp;
                supportScratch_.parent[v] = u;
                q.push_back(v);
                if (supportScratch_.targetStamp[v] == targetStamp) ++found;
            }
        }
        if (outVisitedV) *outVisitedV = visV;
        if (outVisitedE) *outVisitedE = visE;
        if (outSeenStamp) *outSeenStamp = seenStamp;
        if (outRoot) *outRoot = root;
        if (found != (int)relevantVerts.size()) {
#ifdef LOCAL
            g_batch_dbg.support_build_failures++;
            cerr << "SUPPORT BUILD FAILURE owner=" << owner << " relevant=" << relevantVerts.size() << " found=" << found << "\n";
            auto exactDbg = computeExactActivePartition(owner);
            cerr << "relevantVerts:";
            for (int ep : relevantVerts) cerr << ' ' << ep << "(cur=" << topo_.incidentClass(owner, ep) << ",ex=" << (exactDbg.count(ep)?exactDbg[ep]:-1) << ")";
            cerr << "\n";
            abort();
#endif
            const int dedupeStamp = supportScratch_.nextCollect();
            for (int ep : relevantVerts) {
                if (!(1 <= ep && ep <= n_)) continue;
                if (supportScratch_.collectStamp[ep] == dedupeStamp) continue;
                supportScratch_.collectStamp[ep] = dedupeStamp;
                watchVerts.push_back(ep);
            }
            return watchVerts;
        }

        watchVerts.reserve(q.size());
        for (int ep : relevantVerts) {
            int cur = ep;
            while (supportScratch_.collectStamp[cur] != collectStamp) {
                supportScratch_.collectStamp[cur] = collectStamp;
                watchVerts.push_back(cur);
                if (cur == root) break;
                int p = supportScratch_.parent[cur];
                if (p == cur || p < 0) break;
                cur = p;
            }
        }
        return watchVerts;
    }

    template <class ParentAccessor>
    bool buildSupportProductFromWatchVerts(const vector<int>& watchVerts,
                                           ParentAccessor parentAccessor,
                                           SupportBuildProduct& prod) {
        prod = SupportBuildProduct();
        prod.watchVerts = watchVerts;
        int m = (int)watchVerts.size();
        if (m == 0) return false;
        supportScratch_.ensure(n_);
        const int posStamp = supportScratch_.nextSupportPos();
        prod.posStamp = posStamp;
        for (int i = 0; i < m; ++i) {
            int v = watchVerts[i];
            if (!(1 <= v && v <= n_)) return false;
            supportScratch_.supportPosStamp[v] = posStamp;
            supportScratch_.supportPosVal[v] = i;
        }

        prod.rootPos = -1;
        prod.parentPos.assign(m, -1);
        int rootCount = 0;
        for (int i = 0; i < m; ++i) {
            int v = watchVerts[i];
            int p = parentAccessor(v);
            if (!(1 <= p && p <= n_) || p == v || supportScratch_.supportPosStamp[p] != posStamp) {
                ++rootCount;
                prod.rootPos = i;
            } else {
                prod.parentPos[i] = supportScratch_.supportPosVal[p];
            }
        }
        if (rootCount != 1 || prod.rootPos < 0) {
#ifdef LOCAL
            g_batch_dbg.support_meta_fail_root++;
#endif
            return false;
        }

        prod.depth.assign(m, 0);
        prod.tin.assign(m, -1);
        prod.tout.assign(m, -1);
        prod.childFirst.assign(m, -1);
        prod.nextSibling.assign(m, -1);
        prod.nodeEndpointIdx.assign(m, -1);
        prod.preorder.clear();
        prod.preorder.reserve(m);

        for (int i = 0; i < m; ++i) if (prod.parentPos[i] != -1) {
            int par = prod.parentPos[i];
            prod.nextSibling[i] = prod.childFirst[par];
            prod.childFirst[par] = i;
        }

        vector<int> nextChild = prod.childFirst;
        vector<int> stack;
        stack.reserve(m);
        stack.push_back(prod.rootPos);
        prod.depth[prod.rootPos] = 0;
        int timer = 0;
        while (!stack.empty()) {
            int u = stack.back();
            if (prod.tin[u] == -1) {
                prod.tin[u] = timer++;
                prod.preorder.push_back(u);
            }
            int c = nextChild[u];
            if (c == -1) {
                prod.tout[u] = timer - 1;
                stack.pop_back();
                continue;
            }
            nextChild[u] = prod.nextSibling[c];
            prod.depth[c] = prod.depth[u] + 1;
            stack.push_back(c);
        }
        return true;
    }

    SupportBuildProduct buildSupportProductFromLastDeleteArtifact(int owner,
                                                                  const vector<int>& relevantVerts,
                                                                  long long* outWatchV = nullptr,
                                                                  long long* outChainSteps = nullptr) {
        SupportBuildProduct prod;
#ifdef LOCAL
        const bool __connector_detail = (g_connector_skeleton_build_detail_ctx > 0) && local_profile_detailed_enabled();
        long long __vertexset_start_ns = __connector_detail ? dbg_now_ns() : 0;
#endif
        auto tree = topo_.buildSupportTreeFromLastDeleteArtifact(owner, relevantVerts, outWatchV, outChainSteps);
#ifdef LOCAL
        if (__connector_detail) {
            g_batch_dbg.time_connector_skeleton_vertexset_build_ns += dbg_now_ns() - __vertexset_start_ns;
        }
#endif
        if (tree.watchVerts.empty()) return prod;
        const auto& watchVerts = tree.watchVerts;
        if ((int)watchVerts.size() != (int)tree.parentVertex.size()) {
#ifdef LOCAL
            g_batch_dbg.support_meta_fail_artifact_stamp++;
#endif
            return SupportBuildProduct();
        }
#ifdef LOCAL
        long long __vertex_lookup_start_ns = __connector_detail ? dbg_now_ns() : 0;
#endif
        prod.watchVerts = watchVerts;
        supportScratch_.ensure(n_);
        const int posStamp = supportScratch_.nextSupportPos();
        prod.posStamp = posStamp;
        for (int i = 0; i < (int)watchVerts.size(); ++i) {
            int v = watchVerts[i];
            supportScratch_.supportPosStamp[v] = posStamp;
            supportScratch_.supportPosVal[v] = i;
        }
#ifdef LOCAL
        if (__connector_detail) {
            g_batch_dbg.time_connector_skeleton_vertex_lookup_build_ns += dbg_now_ns() - __vertex_lookup_start_ns;
        }
        ScopedNsAcc __core_timer(ptr_if(__connector_detail, &g_batch_dbg.time_connector_skeleton_core_build_ns), nullptr);
#endif
        prod.parentPos.assign(watchVerts.size(), -1);
        prod.depth.assign(watchVerts.size(), 0);
        prod.tin.assign(watchVerts.size(), -1);
        prod.tout.assign(watchVerts.size(), -1);
        prod.childFirst.assign(watchVerts.size(), -1);
        prod.nextSibling.assign(watchVerts.size(), -1);
        prod.nodeEndpointIdx.assign(watchVerts.size(), -1);
        prod.preorder.clear();
        prod.preorder.reserve(watchVerts.size());
        prod.rootPos = -1;
        int rootCount = 0;
        for (int i = 0; i < (int)watchVerts.size(); ++i) {
            int p = tree.parentVertex[i];
            if (!(1 <= p && p <= n_) || p == watchVerts[i]) {
                ++rootCount;
                prod.rootPos = i;
                prod.parentPos[i] = -1;
            } else {
                if (supportScratch_.supportPosStamp[p] != prod.posStamp) {
#ifdef LOCAL
                    g_batch_dbg.support_meta_fail_artifact_stamp++;
#endif
                    return SupportBuildProduct();
                }
                prod.parentPos[i] = supportScratch_.supportPosVal[p];
            }
        }
        if (rootCount != 1 || prod.rootPos < 0) {
#ifdef LOCAL
            g_batch_dbg.support_meta_fail_root++;
#endif
            return SupportBuildProduct();
        }
        fill(prod.childFirst.begin(), prod.childFirst.end(), -1);
        fill(prod.nextSibling.begin(), prod.nextSibling.end(), -1);
        fill(prod.tin.begin(), prod.tin.end(), -1);
        fill(prod.tout.begin(), prod.tout.end(), -1);
        fill(prod.depth.begin(), prod.depth.end(), 0);
        prod.preorder.clear();
        for (int i = 0; i < (int)watchVerts.size(); ++i) if (prod.parentPos[i] != -1) {
            int par = prod.parentPos[i];
            prod.nextSibling[i] = prod.childFirst[par];
            prod.childFirst[par] = i;
        }
        vector<int> nextChild = prod.childFirst;
        vector<int> stack;
        stack.reserve(watchVerts.size());
        stack.push_back(prod.rootPos);
        int timer = 0;
        while (!stack.empty()) {
            int u = stack.back();
            if (prod.tin[u] == -1) {
                prod.tin[u] = timer++;
                prod.preorder.push_back(u);
            }
            int c = nextChild[u];
            if (c == -1) {
                prod.tout[u] = timer - 1;
                stack.pop_back();
                continue;
            }
            nextChild[u] = prod.nextSibling[c];
            prod.depth[c] = prod.depth[u] + 1;
            stack.push_back(c);
        }
        return prod;
    }

    SupportBuildProduct buildSupportProductFromFallback(int owner, const vector<int>& relevantVerts,
                                                        long long* outVisitedV = nullptr,
                                                        long long* outVisitedE = nullptr,
                                                        int* outSeenStamp = nullptr,
                                                        int* outRoot = nullptr) {
        SupportBuildProduct prod;
        int seenStamp = 0;
        int root = -1;
        auto watchVerts = buildSupportVerticesFallback(owner, relevantVerts, outVisitedV, outVisitedE, &seenStamp, &root);
        if (outSeenStamp) *outSeenStamp = seenStamp;
        if (outRoot) *outRoot = root;
        if (watchVerts.empty()) return prod;
        if ((int)watchVerts.size() > 1) {
            for (int v : watchVerts) {
                if (seenStamp == 0 || supportScratch_.seenStamp[v] != seenStamp) {
#ifdef LOCAL
                    g_batch_dbg.support_meta_fail_fallback_stamp++;
#endif
                    return SupportBuildProduct();
                }
            }
        }
        if (!buildSupportProductFromWatchVerts(watchVerts, [&](int v) {
                if (seenStamp != 0 && supportScratch_.seenStamp[v] == seenStamp) return supportScratch_.parent[v];
                return v;
            }, prod)) {
            return SupportBuildProduct();
        }
        return prod;
    }

    bool materializeSupportMetadataFromCollector(int owner, ClassState& st,
                                                 const vector<int>& relevantIdxs,
                                                 const SupportBuildProduct& prod) {
        st.supportMetaValid = false;
        st.supportRootPos = -1;
        if (prod.watchVerts.empty() || prod.rootPos < 0) return false;
        auto& od = ownerData_[owner];
        st.supportVerts = prod.watchVerts;
        st.supportRootPos = prod.rootPos;
        st.supportParentPos = prod.parentPos;
        st.supportDepth = prod.depth;
        st.supportTin = prod.tin;
        st.supportTout = prod.tout;
        st.supportChildFirst = prod.childFirst;
        st.supportNextSibling = prod.nextSibling;
        st.supportSubtreeEndpointCount.assign(prod.watchVerts.size(), 0);
        st.supportSubtreeRepresentativeEndpoint.assign(prod.watchVerts.size(), -1);
        st.supportNodeEndpointIdx.assign(prod.watchVerts.size(), -1);
        st.supportPreorder = prod.preorder;
        st.activeEndpointIdxsSortedBySupportTin.clear();
        st.activeEndpointTinsSorted.clear();
        st.activeEndpointIdxsSortedBySupportTin.reserve(relevantIdxs.size());
        st.activeEndpointTinsSorted.reserve(relevantIdxs.size());

#ifdef LOCAL
        g_batch_dbg.support_meta_build_calls++;
        g_batch_dbg.support_meta_build_watch_vertices += (long long)prod.watchVerts.size();
        g_batch_dbg.support_meta_build_relevant_endpoints += (long long)relevantIdxs.size();
        g_batch_dbg.support_meta_from_collector_calls++;
        g_batch_dbg.support_meta_from_collector_watch_vertices += (long long)prod.watchVerts.size();
        g_batch_dbg.support_meta_from_collector_relevant_endpoints += (long long)relevantIdxs.size();
#endif

        for (int idx : relevantIdxs) {
            if (idx < 0 || idx >= (int)od.endpoints.size()) continue;
            if (od.endpointActiveCount[idx] <= 0) continue;
            int ep = od.endpoints[idx];
            if (!(1 <= ep && ep <= n_)) continue;
            if (supportScratch_.supportPosStamp[ep] != prod.posStamp) continue;
            int pos = supportScratch_.supportPosVal[ep];
            if (!(0 <= pos && pos < (int)prod.watchVerts.size())) continue;
            st.supportNodeEndpointIdx[pos] = idx;
        }

        for (int pos : prod.preorder) {
            int idx = st.supportNodeEndpointIdx[pos];
            if (idx >= 0) {
                st.activeEndpointTinsSorted.push_back(st.supportTin[pos]);
                st.activeEndpointIdxsSortedBySupportTin.push_back(idx);
            }
        }

        for (int i = (int)prod.preorder.size() - 1; i >= 0; --i) {
            int u = prod.preorder[i];
            int cnt = 0;
            int rep = -1;
            int selfIdx = st.supportNodeEndpointIdx[u];
            if (selfIdx >= 0 && selfIdx < (int)od.endpoints.size()) {
                cnt = 1;
                rep = od.endpoints[selfIdx];
            }
            for (int c = st.supportChildFirst[u]; c != -1; c = st.supportNextSibling[c]) {
                cnt += st.supportSubtreeEndpointCount[c];
                if (rep == -1 && st.supportSubtreeRepresentativeEndpoint[c] != -1) rep = st.supportSubtreeRepresentativeEndpoint[c];
            }
            st.supportSubtreeEndpointCount[u] = cnt;
            st.supportSubtreeRepresentativeEndpoint[u] = rep;
        }

        st.supportMetaValid = true;
        clearPieceStateOnly(st);
        st.materializedTreeId = storeSupportTreeObjectFromClassState(st);
        annotateMaterializedHandles(st);
#ifdef LOCAL
        g_batch_dbg.support_meta_build_ok++;
#endif
        return true;
    }

    bool materializeSupportMetadataFromPieceState(int owner, int cid) {
        if (!(1 <= owner && owner <= n_)) return false;
        auto& od = ownerData_[owner];
        auto it = od.classStates.find(cid);
        if (it == od.classStates.end()) return false;
        auto& st = it->second;
        if (!st.pieceModeActive) return false;
        auto relevantIdxs = collectRelevantEndpointIdxs(owner, cid, false);
        if (relevantIdxs.empty() && st.activeQueryCount > 0) relevantIdxs = fallbackRelevantEndpointIdxsFromQueries(owner, cid);
        supportScratch_.ensure(n_);
        SupportBuildProduct prod;
        const int posStamp = supportScratch_.nextSupportPos();
        prod.posStamp = posStamp;
        unordered_map<int,int> mapPos;
        mapPos.reserve(256);
        auto addVertex = [&](int v) -> int {
            auto itp = mapPos.find(v);
            if (itp != mapPos.end()) return itp->second;
            int np = (int)prod.watchVerts.size();
            mapPos.emplace(v, np);
            prod.watchVerts.push_back(v);
            prod.parentPos.push_back(-1);
            prod.nodeEndpointIdx.push_back(-1);
            supportScratch_.supportPosStamp[v] = posStamp;
            supportScratch_.supportPosVal[v] = np;
            return np;
        };
        auto trySetParent = [&](int childPos, int parentV) {
            if (!(0 <= childPos && childPos < (int)prod.parentPos.size())) return;
            if (!(1 <= parentV && parentV <= n_)) return;
            if (supportScratch_.supportPosStamp[parentV] != posStamp) return;
            int pp = supportScratch_.supportPosVal[parentV];
            if (pp == childPos) return;
            if (prod.parentPos[childPos] == -1) prod.parentPos[childPos] = pp;
        };
        auto addPiece = [&](const SupportPieceRef& piece) -> bool {
            const auto* tree = getSupportTreeObject(piece.treeId);
            if (!tree) return false;
            forEachPiecePos(*tree, piece, [&](int oldPos){
                int v = tree->vertexByPos[oldPos];
                int np = addVertex(v);
                if (tree->endpointIdxByPos.size() == tree->vertexByPos.size() && tree->endpointIdxByPos[oldPos] >= 0 && prod.nodeEndpointIdx[np] < 0) {
                    prod.nodeEndpointIdx[np] = tree->endpointIdxByPos[oldPos];
                }
            });
            forEachPiecePos(*tree, piece, [&](int oldPos){
                int v = tree->vertexByPos[oldPos];
                int np = supportScratch_.supportPosVal[v];
                int parOld = tree->parentPos[oldPos];
                if (parOld != -1 && pieceContainsPos(*tree, piece, parOld)) {
                    trySetParent(np, tree->vertexByPos[parOld]);
                }
            });
            return true;
        };
        for (const auto& piece : st.preservedPieces) {
            if (piece.pieceAlive && !addPiece(piece)) return false;
        }
        if (!st.connectorPieces.empty()) {
            for (const auto& piece : st.connectorPieces) {
                if (piece.pieceAlive && !addPiece(piece)) return false;
            }
        } else if (st.connectorTreeId > 0) {
            const auto* tree = getSupportTreeObject(st.connectorTreeId);
            if (!tree) return false;
            for (int oldPos : tree->preorder) {
                int v = tree->vertexByPos[oldPos];
                int np = addVertex(v);
                if (tree->endpointIdxByPos.size() == tree->vertexByPos.size() && tree->endpointIdxByPos[oldPos] >= 0 && prod.nodeEndpointIdx[np] < 0) {
                    prod.nodeEndpointIdx[np] = tree->endpointIdxByPos[oldPos];
                }
            }
            for (int oldPos : tree->preorder) {
                int v = tree->vertexByPos[oldPos];
                int np = supportScratch_.supportPosVal[v];
                int parOld = tree->parentPos[oldPos];
                if (parOld != -1) trySetParent(np, tree->vertexByPos[parOld]);
            }
        }
        if (prod.watchVerts.empty()) return false;
        if (!finalizeSupportBuildProduct(prod)) return false;
#ifdef LOCAL
        g_batch_dbg.piece_materialize_fallback_calls++;
        g_batch_dbg.piece_materialize_fallback_vertices += (long long)prod.watchVerts.size();
#endif
        return materializeSupportMetadataFromCollector(owner, st, relevantIdxs, prod);
    }

    void rebuildSupport(int owner, int cid, const OwnerSplitArtifact* artifact = nullptr, int artifactStamp = 0) {
        if (!(1 <= owner && owner <= n_)) return;
        auto& od = ownerData_[owner];
        auto& st = classState(owner, cid);
        unregisterClassWatch(owner, cid, st);
        if (++st.epoch == INT_MAX) st.epoch = 1;
        if (!topo_.aliveVertex(owner) || st.activeQueryCount <= 0) {
            st.endpointPool.clear();
            return;
        }

        vector<int> relevantIdxs = collectRelevantEndpointIdxs(owner, cid, true);
        if (relevantIdxs.empty() && st.activeQueryCount > 0) relevantIdxs = fallbackRelevantEndpointIdxsFromQueries(owner, cid);
        if (relevantIdxs.empty()) {
            st.activeQueryCount = 0;
            st.endpointPool.clear();
            return;
        }

        vector<int> relevantVerts;
        relevantVerts.reserve(relevantIdxs.size());
        for (int idx : relevantIdxs) relevantVerts.push_back(od.endpoints[idx]);

        long long visV = 0, visE = 0, chainSteps = 0;
        int fallbackSeenStamp = 0;
        int fallbackRoot = -1;
        bool canUseArtifact = (artifact && artifact->valid && artifactStamp != 0);
        SupportBuildProduct prod = canUseArtifact ? buildSupportProductFromLastDeleteArtifact(owner, relevantVerts, &visV, &chainSteps)
                                                  : SupportBuildProduct();
        bool usedArtifact = !prod.watchVerts.empty();
        if (usedArtifact) {
#ifdef LOCAL
            g_batch_dbg.support_rebuild_artifact_calls++;
            g_batch_dbg.support_rebuild_artifact_vertices += visV;
            g_batch_dbg.support_rebuild_artifact_chain_steps += chainSteps;
#endif
        } else {
            prod = buildSupportProductFromFallback(owner, relevantVerts, &visV, &visE, &fallbackSeenStamp, &fallbackRoot);
#ifdef LOCAL
            if (canUseArtifact) {
                g_batch_dbg.support_rebuild_fallback_calls++;
                g_batch_dbg.support_rebuild_fallback_vertices += visV;
                g_batch_dbg.support_rebuild_fallback_edges += visE;
            }
            g_batch_dbg.owner_support_build_calls++;
            g_batch_dbg.owner_support_build_vertices += visV;
            g_batch_dbg.owner_support_build_edges += visE;
#endif
        }
        if (prod.watchVerts.empty()) {
            st.activeQueryCount = 0;
            st.endpointPool.clear();
            return;
        }

#ifdef LOCAL
        g_batch_dbg.owner_support_relevant_endpoints_sum += (long long)relevantVerts.size();
        g_batch_dbg.owner_support_watch_vertices_sum += (long long)prod.watchVerts.size();
        g_batch_dbg.support_full_rebuild_calls++;
        g_batch_dbg.support_full_rebuild_watch_vertices += (long long)prod.watchVerts.size();
#endif

        registerClassWatch(owner, cid, st, prod.watchVerts);
        materializeSupportMetadataFromCollector(owner, st, relevantIdxs, prod);
    }

    void resolveQuery(int qid, bool ownerDeadOrEndpointDead, vector<WitnessChange>& changes) {
        auto& qs = qstate_[qid];
        if (!qs.active) return;
        auto& od = ownerData_[qs.owner];
        qs.active = false;
        failing_[qid] = false;
        if (qs.cid >= 0) {
            auto it = od.classStates.find(qs.cid);
            if (it != od.classStates.end() && it->second.activeQueryCount > 0) it->second.activeQueryCount--;
        }
        if (qs.aIdx >= 0 && qs.aIdx < (int)od.endpointActiveCount.size() && od.endpointActiveCount[qs.aIdx] > 0) {
            od.endpointActiveCount[qs.aIdx]--;
            if (od.endpointActiveCount[qs.aIdx] == 0) topo_.deactivateEndpoint(qs.owner, od.endpoints[qs.aIdx]);
        }
        if (qs.bIdx != qs.aIdx && qs.bIdx >= 0 && qs.bIdx < (int)od.endpointActiveCount.size() && od.endpointActiveCount[qs.bIdx] > 0) {
            od.endpointActiveCount[qs.bIdx]--;
            if (od.endpointActiveCount[qs.bIdx] == 0) topo_.deactivateEndpoint(qs.owner, od.endpoints[qs.bIdx]);
        }
        if (od.activeQueryCount > 0) od.activeQueryCount--;
        qs.cid = -1;
        activeQueryTotal_--;
        changes.push_back({qid, -1, true});
#ifdef LOCAL
        if (ownerDeadOrEndpointDead) g_batch_dbg.query_resolved_owner_dead_or_endpoint_dead++;
        else g_batch_dbg.query_resolved_by_split++;
#endif
    }

    void deactivateAllOwnerWatches(int owner) {
        if (!(1 <= owner && owner <= n_)) return;
        auto& od = ownerData_[owner];
        for (auto& kv : od.classStates) {
            auto& st = kv.second;
            unregisterClassWatch(owner, kv.first, st);
            if (++st.epoch == INT_MAX) st.epoch = 1;
            st.activeQueryCount = 0;
            st.endpointPool.clear();
        }
        od.activeQueryCount = 0;
    }

    enum class SupportReuseKind {
        None,
        SinglePositive,
        RepUnanimous,
    };

    struct PositiveComponentDesc {
        bool parentSide = false;
        int childPos = -1;
        int count = 0;
        int repVertex = -1;
        int bucket = -1;
        int rootPos = -1;
    };

    struct CoverageCollapseResult {
        bool usedFullScan = false;
        int keptCid = -1;
        vector<int> movedIdxs;
        vector<int> candidateCids;
        SupportReuseKind reuseKind = SupportReuseKind::None;
        int xPos = -1;
        vector<PositiveComponentDesc> positiveComps;
        bool pieceNativePlanned = false;
        int hitOldPieceId = -1;
        bool connectorWasHit = false;
        vector<SupportPieceRef> replacementPieces;
        vector<int> connectorRepVertices;
    };

    vector<TouchedClassInfo> gatherTouchedClassInfos(int x) {
        vector<TouchedClassInfo> infos;
        if (!(1 <= x && x <= n_)) return infos;
        unordered_map<long long,int> where;
#ifdef LOCAL
        auto noteReason = [&](SupportOriginKind kind) {
            if (kind == SupportOriginKind::PreservedPiece) g_batch_dbg.piece_native_candidate_preserved_hits++;
            else if (kind == SupportOriginKind::ConnectorTree) g_batch_dbg.piece_native_candidate_connector_hits++;
        };
#endif
        for (const auto& ref : watchByVertex_[x]) {
            bool stale = false;
            if (!(1 <= ref.owner && ref.owner <= n_) || !topo_.aliveVertex(ref.owner)) stale = true;
            else {
                auto it = ownerData_[ref.owner].classStates.find(ref.cid);
                if (it == ownerData_[ref.owner].classStates.end()) stale = true;
                else {
                    const auto& st = it->second;
                    if (!st.watchActive) stale = true;
                    else if (!(0 <= ref.handleIdx && ref.handleIdx < (int)st.watchHandles.size())) stale = true;
                    else if (st.watchHandles[ref.handleIdx].vertex != x) stale = true;
                }
            }
#ifdef LOCAL
            if (stale) {
                g_batch_dbg.watch_stale_drops++;
                continue;
            }
#else
            if (stale) continue;
#endif
            const auto& st = ownerData_[ref.owner].classStates.find(ref.cid)->second;
            const auto& h = st.watchHandles[ref.handleIdx];
            long long key = watchKey(ref.owner, ref.cid);
            int idx;
            auto itw = where.find(key);
            if (itw == where.end()) {
                idx = (int)infos.size();
                where.emplace(key, idx);
                infos.push_back({ref.owner, ref.cid, -1, {}, {}, false, -1, -1, -1});
            } else idx = itw->second;
            auto& info = infos[idx];
            if (h.originKind == SupportOriginKind::MaterializedSupport) {
                info.xHandleIdx = h.localPos;
            } else if (h.originKind == SupportOriginKind::PreservedPiece) {
                bool dup = false;
                for (const auto& ph : info.pieceHits) if (ph.pieceId == h.pieceId) { dup = true; break; }
                if (!dup) info.pieceHits.push_back({h.pieceId, h.treeId, h.localPos});
#ifdef LOCAL
                noteReason(h.originKind);
#endif
            } else if (h.originKind == SupportOriginKind::ConnectorTree) {
                bool dup = false;
                for (const auto& ch : info.connectorHits) if (ch.pieceId == h.pieceId) { dup = true; break; }
                if (!dup) info.connectorHits.push_back({h.pieceId, h.treeId, h.localPos});
                info.connectorHit = !info.connectorHits.empty();
                if (!info.connectorHits.empty()) {
                    info.connectorTreeId = info.connectorHits[0].treeId;
                    info.connectorPieceId = info.connectorHits[0].pieceId;
                    info.connectorLocalPos = info.connectorHits[0].localPos;
                }
#ifdef LOCAL
                noteReason(h.originKind);
#endif
            }
        }
        sort(infos.begin(), infos.end(), [](const TouchedClassInfo& a, const TouchedClassInfo& b){
            if (a.owner != b.owner) return a.owner < b.owner;
            return a.oldCid < b.oldCid;
        });
#ifdef LOCAL
        verifyTouchedClassesExactForDeletion(x, infos);
        g_batch_dbg.owner_touched_by_watch += (long long)infos.size();
        vector<int> owners;
        owners.reserve(infos.size());
        for (const auto& info : infos) owners.push_back(info.owner);
        sort(owners.begin(), owners.end());
        owners.erase(unique(owners.begin(), owners.end()), owners.end());
        g_batch_dbg.owner_touched_unique += (long long)owners.size();
#endif
        return infos;
    }

    void appendEndpointIdxRangeByTin(const ClassState& st, int loTin, int hiTin, vector<int>& out) const {
        if (st.activeEndpointIdxsSortedBySupportTin.empty()) return;
        auto itL = lower_bound(st.activeEndpointTinsSorted.begin(), st.activeEndpointTinsSorted.end(), loTin);
        auto itR = upper_bound(st.activeEndpointTinsSorted.begin(), st.activeEndpointTinsSorted.end(), hiTin);
        int L = (int)(itL - st.activeEndpointTinsSorted.begin());
        int R = (int)(itR - st.activeEndpointTinsSorted.begin());
        for (int i = L; i < R; ++i) out.push_back(st.activeEndpointIdxsSortedBySupportTin[i]);
    }

    void appendEndpointIdxOutsideSubtree(const ClassState& st, int subTin, int subTout, vector<int>& out) const {
        if (st.activeEndpointIdxsSortedBySupportTin.empty()) return;
        auto itL = lower_bound(st.activeEndpointTinsSorted.begin(), st.activeEndpointTinsSorted.end(), subTin);
        auto itR = upper_bound(st.activeEndpointTinsSorted.begin(), st.activeEndpointTinsSorted.end(), subTout);
        int L = (int)(itL - st.activeEndpointTinsSorted.begin());
        int R = (int)(itR - st.activeEndpointTinsSorted.begin());
        for (int i = 0; i < L; ++i) out.push_back(st.activeEndpointIdxsSortedBySupportTin[i]);
        for (int i = R; i < (int)st.activeEndpointIdxsSortedBySupportTin.size(); ++i) out.push_back(st.activeEndpointIdxsSortedBySupportTin[i]);
    }

    int pickRepresentativeOutsideSubtreeVertex(int owner, const ClassState& st, int subTin, int subTout) const {
        const auto& od = ownerData_[owner];
        auto itL = lower_bound(st.activeEndpointTinsSorted.begin(), st.activeEndpointTinsSorted.end(), subTin);
        int L = (int)(itL - st.activeEndpointTinsSorted.begin());
        if (L > 0) {
            int idx = st.activeEndpointIdxsSortedBySupportTin[0];
            return (0 <= idx && idx < (int)od.endpoints.size()) ? od.endpoints[idx] : -1;
        }
        auto itR = upper_bound(st.activeEndpointTinsSorted.begin(), st.activeEndpointTinsSorted.end(), subTout);
        int R = (int)(itR - st.activeEndpointTinsSorted.begin());
        if (R < (int)st.activeEndpointIdxsSortedBySupportTin.size()) {
            int idx = st.activeEndpointIdxsSortedBySupportTin[R];
            return (0 <= idx && idx < (int)od.endpoints.size()) ? od.endpoints[idx] : -1;
        }
        return -1;
    }

    bool finalizeSupportBuildProduct(SupportBuildProduct& prod) {
        int m = (int)prod.watchVerts.size();
        if (m == 0 || (int)prod.parentPos.size() != m) return false;
        prod.rootPos = -1;
        int rootCount = 0;
        for (int i = 0; i < m; ++i) {
            int p = prod.parentPos[i];
            if (p == -1) {
                prod.rootPos = i;
                ++rootCount;
            } else if (!(0 <= p && p < m) || p == i) {
                return false;
            }
        }
        if (rootCount != 1 || prod.rootPos < 0) return false;
        prod.depth.assign(m, 0);
        prod.tin.assign(m, -1);
        prod.tout.assign(m, -1);
        prod.childFirst.assign(m, -1);
        prod.nextSibling.assign(m, -1);
        if ((int)prod.nodeEndpointIdx.size() != m) prod.nodeEndpointIdx.assign(m, -1);
        prod.preorder.clear();
        prod.preorder.reserve(m);
        for (int i = 0; i < m; ++i) if (prod.parentPos[i] != -1) {
            int par = prod.parentPos[i];
            prod.nextSibling[i] = prod.childFirst[par];
            prod.childFirst[par] = i;
        }
        vector<int> nextChild = prod.childFirst;
        vector<int> stack;
        stack.reserve(m);
        stack.push_back(prod.rootPos);
        int timer = 0;
        while (!stack.empty()) {
            int u = stack.back();
            if (prod.tin[u] == -1) {
                prod.tin[u] = timer++;
                prod.preorder.push_back(u);
            }
            int c = nextChild[u];
            if (c == -1) {
                prod.tout[u] = timer - 1;
                stack.pop_back();
                continue;
            }
            nextChild[u] = prod.nextSibling[c];
            prod.depth[c] = prod.depth[u] + 1;
            stack.push_back(c);
        }
        return (int)prod.preorder.size() == m;
    }

    void markKeptSubtreeByPos(const ClassState& st, int rootPos, vector<char>& keep) const {
        if (!(0 <= rootPos && rootPos < (int)st.supportVerts.size())) return;
        int lo = st.supportTin[rootPos];
        int hi = st.supportTout[rootPos];
        if (lo < 0 || hi < lo || hi >= (int)st.supportPreorder.size()) return;
        for (int t = lo; t <= hi; ++t) keep[st.supportPreorder[t]] = 1;
    }

    void markKeptOutsideSubtreeByPos(const ClassState& st, int rootPos, vector<char>& keep) const {
        if (!(0 <= rootPos && rootPos < (int)st.supportVerts.size())) return;
        int lo = st.supportTin[rootPos];
        int hi = st.supportTout[rootPos];
        int m = (int)st.supportPreorder.size();
        if (lo < 0 || hi < lo || hi >= m) return;
        for (int t = 0; t < lo; ++t) keep[st.supportPreorder[t]] = 1;
        for (int t = hi + 1; t < m; ++t) keep[st.supportPreorder[t]] = 1;
    }

    bool buildSupportProductFromTreeSubset(const SupportTreeObject& tree,
                                          const vector<char>& keepOld,
                                          SupportBuildProduct& prod,
                                          vector<int>* outOldToNew = nullptr) {
        prod = SupportBuildProduct();
        int mOld = (int)tree.vertexByPos.size();
        if (mOld == 0 || (int)keepOld.size() != mOld || tree.preorder.empty()) return false;
        supportScratch_.ensure(n_);
        const int posStamp = supportScratch_.nextSupportPos();
        prod.posStamp = posStamp;
        vector<int> oldToNew(mOld, -1);
        prod.watchVerts.reserve(mOld);
        prod.nodeEndpointIdx.reserve(mOld);
        for (int pos : tree.preorder) if (keepOld[pos]) {
            int v = tree.vertexByPos[pos];
            int np = (int)prod.watchVerts.size();
            oldToNew[pos] = np;
            prod.watchVerts.push_back(v);
            prod.nodeEndpointIdx.push_back((0 <= pos && pos < (int)tree.endpointIdxByPos.size()) ? tree.endpointIdxByPos[pos] : -1);
            supportScratch_.supportPosStamp[v] = posStamp;
            supportScratch_.supportPosVal[v] = np;
        }
        if (prod.watchVerts.empty()) return false;
        prod.parentPos.assign(prod.watchVerts.size(), -1);
        for (int pos : tree.preorder) if (keepOld[pos]) {
            int np = oldToNew[pos];
            int par = tree.parentPos[pos];
            if (par != -1 && keepOld[par]) prod.parentPos[np] = oldToNew[par];
        }
        if (outOldToNew) *outOldToNew = std::move(oldToNew);
        return finalizeSupportBuildProduct(prod);
    }

    bool buildSupportProductFromSinglePiece(const SupportPieceRef& piece,
                                            SupportBuildProduct& prod,
                                            vector<int>* outOldToNew = nullptr) {
        const auto* tree = getSupportTreeObject(piece.treeId);
        if (!tree) return false;
        vector<char> keepOld(tree->vertexByPos.size(), 0);
        forEachPiecePos(*tree, piece, [&](int pos){ keepOld[pos] = 1; });
        return buildSupportProductFromTreeSubset(*tree, keepOld, prod, outOldToNew);
    }

    int endpointIndexAtTreePosIfActive(int owner, const SupportTreeObject& tree, int pos) const {
        if (!(0 <= pos && pos < (int)tree.endpointIdxByPos.size())) return -1;
        int idx = tree.endpointIdxByPos[pos];
        if (!(0 <= idx && idx < (int)ownerData_[owner].endpointActiveCount.size())) return -1;
        if (ownerData_[owner].endpointActiveCount[idx] <= 0) return -1;
        int ep = ownerData_[owner].endpoints[idx];
        if (!topo_.aliveVertex(ep)) return -1;
        return idx;
    }

    int pickRepresentativeOutsideSubtreePosOnTree(int owner, const SupportTreeObject& tree,
                                                  const vector<int>& activeEndpointPosSorted,
                                                  int subTin, int subTout) const {
        auto itL = lower_bound(activeEndpointPosSorted.begin(), activeEndpointPosSorted.end(), subTin,
            [&](int pos, int val){ return tree.tin[pos] < val; });
        int L = (int)(itL - activeEndpointPosSorted.begin());
        if (L > 0) return activeEndpointPosSorted[0];
        auto itR = upper_bound(activeEndpointPosSorted.begin(), activeEndpointPosSorted.end(), subTout,
            [&](int val, int pos){ return val < tree.tin[pos]; });
        int R = (int)(itR - activeEndpointPosSorted.begin());
        if (R < (int)activeEndpointPosSorted.size()) return activeEndpointPosSorted[R];
        return -1;
    }

    struct PieceTreeComponentDesc {
        bool parentSide = false;
        int childPos = -1;
        int count = 0;
        int repVertex = -1;
        int repPos = -1;
    };

    bool computeWholeTreePositiveComponents(int owner, const SupportTreeObject& tree, int xPos,
                                            vector<PieceTreeComponentDesc>& comps,
                                            int* outDeadIdx = nullptr) const {
        comps.clear();
        if (!(0 <= xPos && xPos < (int)tree.vertexByPos.size())) return false;
        int m = (int)tree.vertexByPos.size();
        vector<int> subtreeCount(m, 0), subtreeRepPos(m, -1);
        vector<int> activeEndpointPosSorted;
        activeEndpointPosSorted.reserve(tree.endpointPosSorted.size());
        for (int pos : tree.preorder) {
            int idx = endpointIndexAtTreePosIfActive(owner, tree, pos);
            if (idx >= 0) activeEndpointPosSorted.push_back(pos);
        }
        for (int i = (int)tree.preorder.size() - 1; i >= 0; --i) {
            int u = tree.preorder[i];
            int cnt = 0;
            int repPos = -1;
            int idx = endpointIndexAtTreePosIfActive(owner, tree, u);
            if (idx >= 0) {
                cnt = 1;
                repPos = u;
            }
            for (int c = 0; c < m; ++c) {}
            for (int c = 0; c < m; ++c) {}
        }
        vector<int> childFirst(m, -1), nextSibling(m, -1);
        for (int pos = 0; pos < m; ++pos) {
            int par = tree.parentPos[pos];
            if (par != -1) {
                nextSibling[pos] = childFirst[par];
                childFirst[par] = pos;
            }
        }
        for (int i = (int)tree.preorder.size() - 1; i >= 0; --i) {
            int u = tree.preorder[i];
            int cnt = 0;
            int repPos = -1;
            int idx = endpointIndexAtTreePosIfActive(owner, tree, u);
            if (idx >= 0) { cnt = 1; repPos = u; }
            for (int c = childFirst[u]; c != -1; c = nextSibling[c]) {
                cnt += subtreeCount[c];
                if (repPos == -1 && subtreeRepPos[c] != -1) repPos = subtreeRepPos[c];
            }
            subtreeCount[u] = cnt;
            subtreeRepPos[u] = repPos;
        }
        if (outDeadIdx) *outDeadIdx = endpointIndexAtTreePosIfActive(owner, tree, xPos);
        for (int c = childFirst[xPos]; c != -1; c = nextSibling[c]) {
            if (subtreeCount[c] <= 0) continue;
            PieceTreeComponentDesc d;
            d.parentSide = false;
            d.childPos = c;
            d.count = subtreeCount[c];
            d.repPos = subtreeRepPos[c];
            d.repVertex = (d.repPos >= 0 ? tree.vertexByPos[d.repPos] : -1);
            comps.push_back(d);
        }
        int totalEndpoints = (int)activeEndpointPosSorted.size();
        int parentCount = totalEndpoints - subtreeCount[xPos];
        if (parentCount > 0) {
            int repPos = pickRepresentativeOutsideSubtreePosOnTree(owner, tree, activeEndpointPosSorted, tree.tin[xPos], tree.tout[xPos]);
            if (repPos >= 0) {
                PieceTreeComponentDesc d;
                d.parentSide = true;
                d.childPos = -1;
                d.count = parentCount;
                d.repPos = repPos;
                d.repVertex = tree.vertexByPos[repPos];
                comps.push_back(d);
            }
        }
        return true;
    }

    int findPosOfEndpointVertexInTree(const SupportTreeObject& tree, int vertex) {
        int stamp = 0;
        buildTreeVertexPosMap(tree, stamp);
        if (1 <= vertex && vertex <= n_ && supportScratch_.supportPosStamp[vertex] == stamp) return supportScratch_.supportPosVal[vertex];
        return -1;
    }

    bool splitPieceAtLocalPos(int owner, const SupportPieceRef& piece, int oldLocalPos,
                              vector<SupportPieceRef>& survivingPieces,
                              int& removedPieceVertices,
                              int& boundaryOps,
                              int* outDeadIdx = nullptr) {
#ifdef LOCAL
        ScopedNsAcc __timer(ptr_if(local_profile_coarse_enabled(), &g_batch_dbg.time_preserved_piece_split_ns),
                           ptr_if(local_profile_coarse_enabled(), &g_batch_dbg.time_preserved_piece_split_calls));
#endif
        survivingPieces.clear();
        removedPieceVertices = 0;
        boundaryOps = 0;
        SupportBuildProduct prod;
        vector<int> oldToNew;
        if (!buildSupportProductFromSinglePiece(piece, prod, &oldToNew)) return false;
        if (!(0 <= oldLocalPos && oldLocalPos < (int)oldToNew.size())) return false;
        int xPos = oldToNew[oldLocalPos];
        if (xPos < 0) return false;
        int treeId = storeSupportTreeObjectFromProduct(prod, &prod.nodeEndpointIdx);
        if (treeId <= 0) return false;
        const auto* tree = getSupportTreeObject(treeId);
        if (!tree) return false;
        vector<PieceTreeComponentDesc> comps;
        if (!computeWholeTreePositiveComponents(owner, *tree, xPos, comps, outDeadIdx)) return false;
        removedPieceVertices = max(0, (int)tree->vertexByPos.size() - 1);
        boundaryOps = (int)comps.size();
        for (const auto& comp : comps) {
            int attachmentPos = comp.repPos;
            if (comp.parentSide) {
                survivingPieces.push_back(makeComplementPieceRef(treeId, tree->rootPos, xPos, attachmentPos, comp.repVertex, comp.count));
            } else {
                survivingPieces.push_back(makeSubtreePieceRef(treeId, comp.childPos, xPos, attachmentPos, comp.repVertex, comp.count));
            }
        }
#ifdef LOCAL
        g_batch_dbg.preserved_piece_split_calls++;
        g_batch_dbg.preserved_piece_split_vertices += removedPieceVertices;
        g_batch_dbg.preserved_piece_split_boundary_ops += boundaryOps;
#endif
        return true;
    }

    bool buildConnectorForRepresentatives(int owner, const vector<int>& repVertices,
                                          int& connectorTreeId,
                                          vector<int>& connectorOnlyVerts) {
        connectorTreeId = -1;
        connectorOnlyVerts.clear();
        vector<int> reps = repVertices;
#ifdef LOCAL
        const bool __detail = local_profile_detailed_enabled();
        long long __t0 = __detail ? dbg_now_ns() : 0;
        g_batch_dbg.connector_skeleton_terminal_collection_calls++;
#endif
        sort(reps.begin(), reps.end());
#ifdef LOCAL
        if (__detail) {
            long long __dt0 = dbg_now_ns() - __t0;
            g_batch_dbg.time_connector_skeleton_terminal_collection_ns += __dt0;
            g_batch_dbg.time_terminal_collection_ns += __dt0;
            g_batch_dbg.time_terminal_collection_calls++;
        }
        long long __t1 = __detail ? dbg_now_ns() : 0;
        g_batch_dbg.connector_skeleton_terminal_dedupe_calls++;
#endif
        reps.erase(unique(reps.begin(), reps.end()), reps.end());
#ifdef LOCAL
        if (__detail) {
            long long __dt1 = dbg_now_ns() - __t1;
            g_batch_dbg.time_connector_skeleton_terminal_dedupe_ns += __dt1;
            g_batch_dbg.time_terminal_collection_ns += __dt1;
            g_batch_dbg.time_terminal_collection_calls++;
        }
#endif
        if (reps.empty()) return true;
        long long visV = 0, chainSteps = 0;
#ifdef LOCAL
        g_batch_dbg.connector_skeleton_vertexset_build_calls++;
#endif
        SupportBuildProduct connector;
#ifdef LOCAL
        {
            ScopedIntInc __ctx(ptr_if(__detail, &g_connector_skeleton_build_detail_ctx));
            connector = buildSupportProductFromLastDeleteArtifact(owner, reps, &visV, &chainSteps);
        }
#else
        connector = buildSupportProductFromLastDeleteArtifact(owner, reps, &visV, &chainSteps);
#endif
        if (connector.watchVerts.empty()) return false;
#ifdef LOCAL
        g_batch_dbg.connector_skeleton_vertex_lookup_build_calls++;
        g_batch_dbg.connector_skeleton_vertex_lookup_build_vertices += (long long)connector.watchVerts.size();
        if (__detail) g_batch_dbg.time_connector_skeleton_core_build_calls++;
        long long __store_start_ns = __detail ? dbg_now_ns() : 0;
#endif
        connectorTreeId = storeSupportTreeObjectFromProduct(connector, &connector.nodeEndpointIdx);
#ifdef LOCAL
        if (__detail) {
            g_batch_dbg.time_connector_skeleton_core_build_ns += dbg_now_ns() - __store_start_ns;
        }
#endif
        if (connectorTreeId <= 0) return false;
        connectorOnlyVerts = connector.watchVerts;
        return true;
    }


    int countConnectorWatchHandlesInPiece(const ClassState& st, const SupportPieceRef& piece) const {
        const auto* tree = getSupportTreeObject(piece.treeId);
        if (!tree) return 0;
        int cnt = 0;
        for (int hi : st.connectorWatchEntryIds) {
            if (!(0 <= hi && hi < (int)st.watchHandles.size())) continue;
            const auto& h = st.watchHandles[hi];
            if (h.originKind != SupportOriginKind::ConnectorTree) continue;
            if (h.treeId != piece.treeId) continue;
            if (!pieceContainsPos(*tree, piece, h.localPos)) continue;
            ++cnt;
        }
        return cnt;
    }

    int countTerminalsInPiece(const SupportTreeObject& tree, const SupportPieceRef& piece, const vector<int>& terminalVertices) {
        if (terminalVertices.empty()) return 0;
        int stamp = 0;
        buildTreeVertexPosMap(tree, stamp);
        int cnt = 0;
        for (int v : terminalVertices) {
            if (!(1 <= v && v <= n_)) continue;
            if (supportScratch_.supportPosStamp[v] != stamp) continue;
            int pos = supportScratch_.supportPosVal[v];
            if (pieceContainsPos(tree, piece, pos)) ++cnt;
        }
        return cnt;
    }

    int pickRepresentativeTerminalInPiece(const SupportTreeObject& tree, const SupportPieceRef& piece,
                                          const vector<int>& terminalVertices) {
        if (terminalVertices.empty()) return -1;
        int stamp = 0;
        buildTreeVertexPosMap(tree, stamp);
        int bestPos = INT_MAX;
        int bestV = -1;
        for (int v : terminalVertices) {
            if (!(1 <= v && v <= n_)) continue;
            if (supportScratch_.supportPosStamp[v] != stamp) continue;
            int pos = supportScratch_.supportPosVal[v];
            if (!pieceContainsPos(tree, piece, pos)) continue;
            int t = tree.tin[pos];
            if (t < bestPos) { bestPos = t; bestV = v; }
        }
        return bestV;
    }

    vector<int> normalizedAliveAttachmentVertices(const vector<int>& attachmentVertices) const {
        vector<int> out;
        out.reserve(attachmentVertices.size());
        for (int v : attachmentVertices) {
#ifdef LOCAL
            if (v == currentDeleteX_) g_batch_dbg.debug_skeleton_builder_attempted_deleted_vertex++;
#endif
            if (1 <= v && v <= n_ && topo_.aliveVertex(v)) {
                out.push_back(v);
            } else {
#ifdef LOCAL
                if (v == currentDeleteX_) g_batch_dbg.debug_skeleton_builder_skipped_deleted_vertex++;
#endif
            }
        }
        sort(out.begin(), out.end());
        out.erase(unique(out.begin(), out.end()), out.end());
        return out;
    }

    vector<int> aliveAttachmentVertices(const ClassState& st) const {
        return normalizedAliveAttachmentVertices(st.attachmentVerticesByPiece);
    }

    int attachmentVertexOfPiece(const ClassState& st, int pieceId) const {
        for (size_t i = 0; i < st.preservedPieces.size() && i < st.attachmentVerticesByPiece.size(); ++i) {
            if (st.preservedPieces[i].pieceId == pieceId) return st.attachmentVerticesByPiece[i];
        }
        return -1;
    }

    int terminalCountInConnectorPiece(const ClassState& st, const SupportPieceRef& piece) {
        const auto* tree = getSupportTreeObject(piece.treeId);
        if (!tree) return 0;
        auto terminals = aliveAttachmentVertices(st);
        return countTerminalsInPiece(*tree, piece, terminals);
    }

    int terminalCountInConnectorPieceFromAttachments(const SupportPieceRef& piece, const vector<int>& attachmentVertices) {
        const auto* tree = getSupportTreeObject(piece.treeId);
        if (!tree) return 0;
        auto terminals = normalizedAliveAttachmentVertices(attachmentVertices);
        return countTerminalsInPiece(*tree, piece, terminals);
    }

    int representativeTerminalInConnectorPiece(const ClassState& st, const SupportPieceRef& piece) {
        const auto* tree = getSupportTreeObject(piece.treeId);
        if (!tree) return -1;
        auto terminals = aliveAttachmentVertices(st);
        return pickRepresentativeTerminalInPiece(*tree, piece, terminals);
    }

    int representativeTerminalInConnectorPieceFromAttachments(const SupportPieceRef& piece, const vector<int>& attachmentVertices) {
        const auto* tree = getSupportTreeObject(piece.treeId);
        if (!tree) return -1;
        auto terminals = normalizedAliveAttachmentVertices(attachmentVertices);
        return pickRepresentativeTerminalInPiece(*tree, piece, terminals);
    }

    int retargetAttachmentIfDead(const SupportTreeObject& tree, const SupportPieceRef& piece) {
        int rep = piece.pieceRepresentativeEndpoint;
        int repPos = (1 <= rep && rep <= n_) ? findPosOfEndpointVertexInTree(tree, rep) : -1;
        if (0 <= repPos && pieceContainsPos(tree, piece, repPos)) return rep;
        int bestPos = INT_MAX;
        int bestV = -1;
        for (int pos : tree.endpointPosSorted) {
            if (!pieceContainsPos(tree, piece, pos)) continue;
            int t = tree.tin[pos];
            if (t < bestPos) { bestPos = t; bestV = tree.vertexByPos[pos]; }
        }
        if (bestV != -1) return bestV;
        forEachPiecePos(tree, piece, [&](int pos){
            int t = tree.tin[pos];
            if (t < bestPos) { bestPos = t; bestV = tree.vertexByPos[pos]; }
        });
        return bestV;
    }

    bool buildPatchTreeForVertices(int owner, int cid, const vector<int>& patchVertices, ClassState& st,
                                   int& patchTreeId, vector<int>& connectorOnlyVerts, vector<int>* outNewHandleIdxs = nullptr) {
        patchTreeId = -1;
        connectorOnlyVerts.clear();
        if (outNewHandleIdxs) outNewHandleIdxs->clear();
        vector<int> reps = patchVertices;
        sort(reps.begin(), reps.end());
        reps.erase(unique(reps.begin(), reps.end()), reps.end());
        if (reps.size() <= 1) return true;
        long long visV = 0, chainSteps = 0;
        SupportBuildProduct connector = buildSupportProductFromLastDeleteArtifact(owner, reps, &visV, &chainSteps);
        if (connector.watchVerts.empty()) return false;
        patchTreeId = storeSupportTreeObjectFromProduct(connector, &connector.nodeEndpointIdx);
        if (patchTreeId <= 0) return false;
        supportScratch_.ensure(n_);
        int stamp = supportScratch_.nextCollect();
        for (const auto& h : st.watchHandles) if (1 <= h.vertex && h.vertex <= n_) supportScratch_.collectStamp[h.vertex] = stamp;
        for (int v : connector.watchVerts) {
            if (1 <= v && v <= n_ && supportScratch_.collectStamp[v] == stamp) continue;
            connectorOnlyVerts.push_back(v);
            if (1 <= v && v <= n_) supportScratch_.collectStamp[v] = stamp;
        }
        if (outNewHandleIdxs) appendClassWatchEntries(owner, cid, st, connectorOnlyVerts, outNewHandleIdxs);
        return true;
    }

    struct ConnectorPieceCompDesc {
        bool parentSide = false;
        int childPos = -1;
        int terminalCount = 0;
        int repTerminal = -1;
        int repPos = -1;
    };

    bool computeWholeTreeTerminalComponents(const SupportTreeObject& tree, const vector<int>& terminalVertices, int xPos,
                                            vector<ConnectorPieceCompDesc>& comps) {
        comps.clear();
        if (!(0 <= xPos && xPos < (int)tree.vertexByPos.size())) return false;
        int m = (int)tree.vertexByPos.size();
        vector<int> childFirst(m, -1), nextSibling(m, -1);
        for (int pos = 0; pos < m; ++pos) {
            int par = tree.parentPos[pos];
            if (par != -1) {
                nextSibling[pos] = childFirst[par];
                childFirst[par] = pos;
            }
        }
        int stamp = 0;
        buildTreeVertexPosMap(tree, stamp);
        vector<char> isTerminal(m, 0);
        vector<int> terminalPosByTin;
        terminalPosByTin.reserve(terminalVertices.size());
        for (int v : terminalVertices) {
            if (!(1 <= v && v <= n_)) continue;
            if (supportScratch_.supportPosStamp[v] != stamp) continue;
            int pos = supportScratch_.supportPosVal[v];
            if (!isTerminal[pos]) {
                isTerminal[pos] = 1;
                terminalPosByTin.push_back(pos);
            }
        }
        sort(terminalPosByTin.begin(), terminalPosByTin.end(), [&](int a, int b){ return tree.tin[a] < tree.tin[b]; });
        vector<int> subtreeCount(m, 0), subtreeRepPos(m, -1);
        for (int i = (int)tree.preorder.size() - 1; i >= 0; --i) {
            int u = tree.preorder[i];
            int cnt = isTerminal[u] ? 1 : 0;
            int repPos = isTerminal[u] ? u : -1;
            for (int c = childFirst[u]; c != -1; c = nextSibling[c]) {
                cnt += subtreeCount[c];
                if (repPos == -1 && subtreeRepPos[c] != -1) repPos = subtreeRepPos[c];
            }
            subtreeCount[u] = cnt;
            subtreeRepPos[u] = repPos;
        }
        for (int c = childFirst[xPos]; c != -1; c = nextSibling[c]) {
            if (subtreeCount[c] <= 0) continue;
            ConnectorPieceCompDesc d;
            d.parentSide = false;
            d.childPos = c;
            d.terminalCount = subtreeCount[c];
            d.repPos = subtreeRepPos[c];
            d.repTerminal = (d.repPos >= 0 ? tree.vertexByPos[d.repPos] : -1);
            comps.push_back(d);
        }
        int totalTerminals = (int)terminalPosByTin.size();
        int parentCount = totalTerminals - subtreeCount[xPos];
        if (parentCount > 0) {
            int repPos = -1;
            for (int pos : terminalPosByTin) {
                int t = tree.tin[pos];
                if (!(tree.tin[xPos] <= t && t <= tree.tout[xPos])) { repPos = pos; break; }
            }
            if (repPos >= 0) {
                ConnectorPieceCompDesc d;
                d.parentSide = true;
                d.childPos = -1;
                d.terminalCount = parentCount;
                d.repPos = repPos;
                d.repTerminal = tree.vertexByPos[repPos];
                comps.push_back(d);
            }
        }
        return true;
    }


    bool splitConnectorPieceAtLocalPos(const SupportPieceRef& piece, int oldLocalPos, const vector<int>& terminalVertices,
                                       vector<SupportPieceRef>& survivingPieces, int& removedVertices, int& terminalGroups) {
        survivingPieces.clear();
        removedVertices = 0;
        terminalGroups = 0;
        SupportBuildProduct prod;
        vector<int> oldToNew;
        if (!buildSupportProductFromSinglePiece(piece, prod, &oldToNew)) return false;
        if (!(0 <= oldLocalPos && oldLocalPos < (int)oldToNew.size())) return false;
        int xPos = oldToNew[oldLocalPos];
        if (xPos < 0) return false;
        int treeId = storeSupportTreeObjectFromProduct(prod, &prod.nodeEndpointIdx);
        if (treeId <= 0) return false;
        const auto* tree = getSupportTreeObject(treeId);
        if (!tree) return false;
        vector<ConnectorPieceCompDesc> comps;
        if (!computeWholeTreeTerminalComponents(*tree, terminalVertices, xPos, comps)) return false;
        removedVertices = max(0, (int)tree->vertexByPos.size() - 1);
        terminalGroups = 0;
        for (const auto& comp : comps) {
            if (comp.terminalCount <= 0) continue;
            ++terminalGroups;
            if (comp.parentSide) survivingPieces.push_back(makeComplementPieceRef(treeId, tree->rootPos, xPos, comp.repPos, -1, 0));
            else survivingPieces.push_back(makeSubtreePieceRef(treeId, comp.childPos, xPos, comp.repPos, -1, 0));
        }
        return true;
    }

    struct ConnectorShadowEstimate {
        long long candidateReused = 0;
        long long candidateRemoved = 0;
        long long candidatePatch = 0;
        long long candidateRetargets = 0;
        long long terminalGroups = 0;
        bool noPatchNeeded = false;
    };

    ConnectorShadowEstimate estimateConnectorDeltaForUnanimous(int owner, const ClassState& st,
                                                               const TouchedClassInfo& info,
                                                               const CoverageCollapseResult& refineRes) {
        ConnectorShadowEstimate est;
#ifdef LOCAL
        if (refineRes.reuseKind != SupportReuseKind::RepUnanimous) return est;
        g_batch_dbg.connector_shadow_unanimous_classes++;

        vector<int> terminalVertices = refineRes.connectorRepVertices;
        sort(terminalVertices.begin(), terminalVertices.end());
        terminalVertices.erase(unique(terminalVertices.begin(), terminalVertices.end()), terminalVertices.end());

        if (st.connectorTreeId > 0) {
            const auto* tree = getSupportTreeObject(st.connectorTreeId);
            if (tree) {
                vector<SupportPieceRef> frags;
                if (!info.connectorHits.empty() && info.connectorTreeId == st.connectorTreeId && 0 <= info.connectorLocalPos && info.connectorLocalPos < (int)tree->vertexByPos.size()) {
                    int xPos = info.connectorLocalPos;
                    for (int c = 0; c < (int)tree->vertexByPos.size(); ++c) {
                        if (tree->parentPos[c] == xPos) {
                            frags.push_back(makeSubtreePieceRef(st.connectorTreeId, c, xPos, tree->vertexByPos[c], -1, 0));
                        }
                    }
                    if (xPos != tree->rootPos || (int)tree->vertexByPos.size() > 1) {
                        frags.push_back(makeComplementPieceRef(st.connectorTreeId, tree->rootPos, xPos, tree->vertexByPos[tree->rootPos], -1, 0));
                    }
                } else {
                    frags.push_back(makeSubtreePieceRef(st.connectorTreeId, tree->rootPos, -1, tree->vertexByPos[tree->rootPos], -1, 0));
                }
                vector<int> patchReps;
                unordered_set<int> patchRepSet;
                for (const auto& frag : frags) {
                    int tcnt = countTerminalsInPiece(*tree, frag, terminalVertices);
                    if (tcnt <= 0) {
                        est.candidateRemoved += countConnectorWatchHandlesInPiece(st, frag);
                        continue;
                    }
                    est.terminalGroups++;
                    est.candidateReused += countConnectorWatchHandlesInPiece(st, frag);
                    int rep = pickRepresentativeTerminalInPiece(*tree, frag, terminalVertices);
                    if (1 <= rep && rep <= n_ && patchRepSet.insert(rep).second) patchReps.push_back(rep);
                }
                if ((int)patchReps.size() <= 1) est.noPatchNeeded = true;
                else est.candidatePatch += (long long)patchReps.size();
            }
        }

        if (!info.pieceHits.empty()) {
            const auto& hit = info.pieceHits[0];
            auto itPiece = find_if(st.preservedPieces.begin(), st.preservedPieces.end(), [&](const SupportPieceRef& p){
                return p.pieceAlive && p.pieceId == hit.pieceId;
            });
            if (itPiece != st.preservedPieces.end()) {
                int oldAttachment = itPiece->pieceRepresentativeEndpoint;
                bool attachmentSurvives = false;
                for (const auto& p : refineRes.replacementPieces) {
                    const auto* ptree = getSupportTreeObject(p.treeId);
                    if (!ptree) continue;
                    int pstamp = 0;
                    buildTreeVertexPosMap(*ptree, pstamp);
                    if (1 <= oldAttachment && oldAttachment <= n_ && supportScratch_.supportPosStamp[oldAttachment] == pstamp) {
                        int pos = supportScratch_.supportPosVal[oldAttachment];
                        if (pieceContainsPos(*ptree, p, pos)) {
                            attachmentSurvives = true;
                            break;
                        }
                    }
                }
                if (!attachmentSurvives && !refineRes.replacementPieces.empty()) {
                    est.candidateRetargets += (long long)refineRes.replacementPieces.size();
                }
            }
        }

        g_batch_dbg.connector_shadow_candidate_reused_connector_vertices += est.candidateReused;
        g_batch_dbg.connector_shadow_candidate_removed_connector_vertices += est.candidateRemoved;
        g_batch_dbg.connector_shadow_candidate_patch_vertices += est.candidatePatch;
        g_batch_dbg.connector_shadow_candidate_attachment_retargets += est.candidateRetargets;
        g_batch_dbg.connector_shadow_candidate_terminal_fragment_groups += est.terminalGroups;
        if (est.noPatchNeeded) g_batch_dbg.connector_shadow_candidate_no_patch_needed++;
#endif
        return est;
    }

    bool buildSupportProductFromKeptOldSupport(const ClassState& st,
                                               const vector<char>& keepOld,
                                               SupportBuildProduct& prod) {
        prod = SupportBuildProduct();
        int mOld = (int)st.supportVerts.size();
        if (mOld == 0 || (int)keepOld.size() != mOld || st.supportPreorder.empty()) return false;
        supportScratch_.ensure(n_);
        const int posStamp = supportScratch_.nextSupportPos();
        prod.posStamp = posStamp;
        vector<int> oldToNew(mOld, -1);
        prod.watchVerts.reserve(mOld);
        for (int pos : st.supportPreorder) if (keepOld[pos]) {
            int v = st.supportVerts[pos];
            int np = (int)prod.watchVerts.size();
            oldToNew[pos] = np;
            prod.watchVerts.push_back(v);
            supportScratch_.supportPosStamp[v] = posStamp;
            supportScratch_.supportPosVal[v] = np;
        }
        if (prod.watchVerts.empty()) return false;
        prod.parentPos.assign(prod.watchVerts.size(), -1);
        for (int pos : st.supportPreorder) if (keepOld[pos]) {
            int np = oldToNew[pos];
            int par = st.supportParentPos[pos];
            if (par != -1 && keepOld[par]) prod.parentPos[np] = oldToNew[par];
        }
        return finalizeSupportBuildProduct(prod);
    }

    bool buildSupportProductFromMergedReuse(const ClassState& st,
                                            const vector<char>& keepOld,
                                            const SupportBuildProduct& connector,
                                            SupportBuildProduct& prod,
                                            long long* outKeptOld = nullptr,
                                            long long* outAdded = nullptr,
                                            long long* outRemoved = nullptr) {
        prod = SupportBuildProduct();
        int mOld = (int)st.supportVerts.size();
        if (mOld == 0 || (int)keepOld.size() != mOld || st.supportPreorder.empty()) return false;
        if (connector.watchVerts.empty()) return false;
        supportScratch_.ensure(n_);
        const int posStamp = supportScratch_.nextSupportPos();
        prod.posStamp = posStamp;
        vector<int> oldToNew(mOld, -1);
        vector<int> connToNew(connector.watchVerts.size(), -1);
        vector<int> newToOld;
        vector<int> newToConn;
        auto addVertex = [&](int v) {
            int np;
            if (supportScratch_.supportPosStamp[v] == posStamp) {
                np = supportScratch_.supportPosVal[v];
            } else {
                np = (int)prod.watchVerts.size();
                prod.watchVerts.push_back(v);
                supportScratch_.supportPosStamp[v] = posStamp;
                supportScratch_.supportPosVal[v] = np;
                newToOld.push_back(-1);
                newToConn.push_back(-1);
            }
            return np;
        };
        long long keptOld = 0;
        for (int pos : st.supportPreorder) if (keepOld[pos]) {
            int np = addVertex(st.supportVerts[pos]);
            oldToNew[pos] = np;
            if (newToOld[np] == -1) ++keptOld;
            newToOld[np] = pos;
        }
        for (int cpos : connector.preorder) {
            int np = addVertex(connector.watchVerts[cpos]);
            connToNew[cpos] = np;
            newToConn[np] = cpos;
        }
        prod.parentPos.assign(prod.watchVerts.size(), -2);
        for (int cpos : connector.preorder) {
            int np = connToNew[cpos];
            int cp = connector.parentPos[cpos];
            prod.parentPos[np] = (cp == -1 ? -1 : connToNew[cp]);
        }
        for (int pos : st.supportPreorder) if (keepOld[pos]) {
            int np = oldToNew[pos];
            if (prod.parentPos[np] != -2) continue;
            int par = st.supportParentPos[pos];
            if (par != -1 && keepOld[par]) prod.parentPos[np] = oldToNew[par];
            else prod.parentPos[np] = -1;
        }
        for (int np = 0; np < (int)prod.parentPos.size(); ++np) if (prod.parentPos[np] == -2) return false;
        if (!finalizeSupportBuildProduct(prod)) return false;
        if (outKeptOld) *outKeptOld = keptOld;
        if (outAdded) *outAdded = (long long)prod.watchVerts.size() - keptOld;
        if (outRemoved) *outRemoved = (long long)st.watchVertexCount - keptOld;
        return true;
    }

    void accountPieceShadowCurrentVsCandidate(const CoverageCollapseResult& refineRes,
                                              long long currentMaterialize,
                                              long long currentUnregister,
                                              long long currentRegister,
                                              long long candidateReused,
                                              long long candidateRemoved,
                                              long long candidateConnector,
                                              long long candidateBoundaryOps) {
#ifdef LOCAL
        g_batch_dbg.piece_shadow_current_materialize_vertices += currentMaterialize;
        g_batch_dbg.piece_shadow_current_watch_unregister_vertices += currentUnregister;
        g_batch_dbg.piece_shadow_current_watch_register_vertices += currentRegister;
        g_batch_dbg.piece_shadow_candidate_piece_reused_vertices += candidateReused;
        g_batch_dbg.piece_shadow_candidate_piece_removed_vertices += candidateRemoved;
        g_batch_dbg.piece_shadow_candidate_connector_vertices += candidateConnector;
        g_batch_dbg.piece_shadow_candidate_boundary_ops += candidateBoundaryOps;
        g_batch_dbg.piece_shadow_candidate_piece_count += (long long)refineRes.positiveComps.size();
        for (const auto& comp : refineRes.positiveComps) if (comp.parentSide) {
            g_batch_dbg.piece_shadow_candidate_parent_side_positive_cases++;
        }
        if ((int)refineRes.positiveComps.size() > 1) g_batch_dbg.piece_shadow_candidate_multi_piece_classes++;
        long long currentVol = currentMaterialize + currentUnregister + currentRegister;
        long long candidateVol = candidateRemoved + candidateConnector + candidateBoundaryOps;
        g_batch_dbg.piece_shadow_estimated_saved_vertices += max<long long>(0, currentVol - candidateVol);
#endif
    }

    CoverageCollapseResult runFullScanFallbackForClass(int owner, int oldCid, ClassState& st, int why) {
        CoverageCollapseResult fb;
        fb.usedFullScan = true;
        fb.keptCid = oldCid;
        fb.candidateCids.push_back(oldCid);
        auto& od = ownerData_[owner];
        vector<int> relevantVerts;
        if (!st.activeEndpointIdxsSortedBySupportTin.empty()) {
            relevantVerts.reserve(st.activeEndpointIdxsSortedBySupportTin.size());
            for (int idx : st.activeEndpointIdxsSortedBySupportTin) {
                if (idx < 0 || idx >= (int)od.endpoints.size()) continue;
                if (od.endpointActiveCount[idx] <= 0) continue;
                relevantVerts.push_back(od.endpoints[idx]);
            }
        } else {
            auto relevantIdxs = collectRelevantEndpointIdxs(owner, oldCid, false);
            if (relevantIdxs.empty() && st.activeQueryCount > 0) relevantIdxs = fallbackRelevantEndpointIdxsFromQueries(owner, oldCid);
            relevantVerts.reserve(relevantIdxs.size());
            for (int idx : relevantIdxs) if (0 <= idx && idx < (int)od.endpoints.size()) relevantVerts.push_back(od.endpoints[idx]);
        }
#ifdef LOCAL
        g_batch_dbg.class_local_fullscan_calls++;
        g_batch_dbg.class_local_fullscan_endpoints += (long long)relevantVerts.size();
        if (why != 7) {
            g_batch_dbg.piece_materialize_fallback_calls++;
            g_batch_dbg.piece_materialize_fallback_vertices += (long long)relevantVerts.size();
        }
        if (why == 1) { g_batch_dbg.fullscan_bad_meta++; g_batch_dbg.piece_fallback_reason_need_support_meta++; }
        else if (why == 2) { g_batch_dbg.fullscan_bad_xpos++; g_batch_dbg.piece_fallback_reason_bad_x_handle++; }
        else if (why == 3) { g_batch_dbg.fullscan_bad_ctx++; g_batch_dbg.piece_fallback_reason_other++; }
        else if (why == 4) { g_batch_dbg.fullscan_bad_rep++; g_batch_dbg.piece_fallback_reason_other++; }
        else if (why == 5) { g_batch_dbg.piece_fallback_reason_preserved_piece_hit++; }
        else if (why == 6) { g_batch_dbg.piece_fallback_reason_connector_hit++; }
        else if (why == 7) { g_batch_dbg.piece_fallback_reason_split_required++; }
        else { g_batch_dbg.piece_fallback_reason_other++; }
        g_topo_dbg.class_local_refine_endpoints += (long long)relevantVerts.size();
#endif
        auto full = topo_.refineTouchedClassFromLastDeleteArtifact(owner, oldCid, relevantVerts);
        fb.keptCid = full.keptCid < 0 ? oldCid : full.keptCid;
        fb.candidateCids = full.candidateCids;
        for (int ep : full.movedEndpoints) {
            auto it = od.endpointIndex.find(ep);
            if (it != od.endpointIndex.end()) fb.movedIdxs.push_back(it->second);
        }
        sort(fb.movedIdxs.begin(), fb.movedIdxs.end());
        fb.movedIdxs.erase(unique(fb.movedIdxs.begin(), fb.movedIdxs.end()), fb.movedIdxs.end());
#ifdef LOCAL
        g_batch_dbg.moved_endpoint_total += (long long)fb.movedIdxs.size();
        g_topo_dbg.class_local_refine_moved_endpoints += (long long)fb.movedIdxs.size();
#endif
        sort(fb.candidateCids.begin(), fb.candidateCids.end());
        fb.candidateCids.erase(unique(fb.candidateCids.begin(), fb.candidateCids.end()), fb.candidateCids.end());
        return fb;
    }

    bool relabelHitPieceHandlesByReplacement(ClassState& st, int oldPieceId,
                                             const vector<SupportPieceRef>& replacementPieces,
                                             vector<char>& keepMask,
                                             long long* outRemoved) {
        if (outRemoved) *outRemoved = 0;
        if (oldPieceId < 0) return true;
        unordered_map<int, pair<int,int>> vertexToPiecePos;
        for (const auto& piece : replacementPieces) {
            const auto* tree = getSupportTreeObject(piece.treeId);
            if (!tree) return false;
            forEachPiecePos(*tree, piece, [&](int pos){
                int v = tree->vertexByPos[pos];
                vertexToPiecePos[v] = {piece.pieceId, pos};
            });
        }
        if ((int)keepMask.size() != (int)st.watchHandles.size()) keepMask.assign(st.watchHandles.size(), 1);
        for (int i = 0; i < (int)st.watchHandles.size(); ++i) {
            auto& h = st.watchHandles[i];
            if (h.originKind != SupportOriginKind::PreservedPiece || h.pieceId != oldPieceId) continue;
            auto it = vertexToPiecePos.find(h.vertex);
            if (it == vertexToPiecePos.end()) {
                keepMask[i] = 0;
                if (outRemoved) (*outRemoved)++;
            }
        }
        retainClassWatchByKeepMask(0, 0, st, keepMask); // owner/cid fixed later by caller via entry metadata already updated in vec
        for (auto& h : st.watchHandles) {
            if (h.originKind != SupportOriginKind::PreservedPiece || h.pieceId != oldPieceId) continue;
            auto it = vertexToPiecePos.find(h.vertex);
            if (it == vertexToPiecePos.end()) continue;
            int newPieceId = it->second.first;
            int newPos = it->second.second;
            const SupportPieceRef* target = nullptr;
            for (const auto& p : replacementPieces) if (p.pieceId == newPieceId) { target = &p; break; }
            if (!target) continue;
            annotateHandleMetadata(h, SupportOriginKind::PreservedPiece, target->treeId, target->pieceId, newPos);
        }
        return true;
    }

    CoverageCollapseResult refineTouchedClassCoverageCollapsePieceNative(
        int x,
        const TouchedClassInfo& info,
        unordered_map<int, DecrementalNBTopology::OwnerBucketContext>& ownerCtxCache) {
        CoverageCollapseResult res;
        res.keptCid = info.oldCid;
        res.candidateCids.push_back(info.oldCid);
        int owner = info.owner;
        int oldCid = info.oldCid;
        if (!(1 <= owner && owner <= n_) || !topo_.aliveVertex(owner) || oldCid < 0) return res;
        auto& od = ownerData_[owner];
        auto itState = od.classStates.find(oldCid);
        if (itState == od.classStates.end()) return res;
        auto& st = itState->second;
#ifdef LOCAL
        g_batch_dbg.piece_native_candidate_classes++;
        g_batch_dbg.piece_native_candidate_preserved_hits += (long long)info.pieceHits.size();
        g_batch_dbg.piece_native_candidate_connector_hits += (long long)info.connectorHits.size();
#endif
        if (!st.pieceModeActive) return runFullScanFallbackForClass(owner, oldCid, st, 1);
        if (info.pieceHits.size() > 1) return runFullScanFallbackForClass(owner, oldCid, st, 5);

        int deadIdx = -1;
        vector<SupportPieceRef> workingPieces;
        workingPieces.reserve(st.preservedPieces.size() + 4);
        if (!info.pieceHits.empty()) {
            const auto& hit = info.pieceHits[0];
            auto itPiece = find_if(st.preservedPieces.begin(), st.preservedPieces.end(), [&](const SupportPieceRef& p){
                return p.pieceAlive && p.pieceId == hit.pieceId;
            });
            if (itPiece == st.preservedPieces.end()) return runFullScanFallbackForClass(owner, oldCid, st, 5);
            vector<SupportPieceRef> replacement;
            int removedVertices = 0, boundaryOps = 0;
            if (!splitPieceAtLocalPos(owner, *itPiece, hit.localPos, replacement, removedVertices, boundaryOps, &deadIdx)) {
                return runFullScanFallbackForClass(owner, oldCid, st, 5);
            }
            res.hitOldPieceId = hit.pieceId;
            res.replacementPieces = replacement;
            for (const auto& p : st.preservedPieces) if (p.pieceAlive && p.pieceId != hit.pieceId && p.pieceEndpointCount > 0) {
                workingPieces.push_back(p);
            }
            for (const auto& p : replacement) if (p.pieceAlive && p.pieceEndpointCount > 0) workingPieces.push_back(p);
        } else {
            for (const auto& p : st.preservedPieces) if (p.pieceAlive && p.pieceEndpointCount > 0) workingPieces.push_back(p);
        }
        res.connectorWasHit = !info.connectorHits.empty();
        if (deadIdx >= 0 && deadIdx < (int)od.endpoints.size()) {
            topo_.deactivateEndpoint(owner, od.endpoints[deadIdx]);
            res.movedIdxs.push_back(deadIdx);
#ifdef LOCAL
            g_batch_dbg.moved_endpoint_total++;
            g_topo_dbg.class_local_refine_moved_endpoints++;
#endif
        }
        if (workingPieces.empty()) {
            sort(res.candidateCids.begin(), res.candidateCids.end());
            res.candidateCids.erase(unique(res.candidateCids.begin(), res.candidateCids.end()), res.candidateCids.end());
            return res;
        }
        if ((int)workingPieces.size() <= 1) {
            res.pieceNativePlanned = true;
            res.reuseKind = SupportReuseKind::SinglePositive;
            sort(res.candidateCids.begin(), res.candidateCids.end());
            res.candidateCids.erase(unique(res.candidateCids.begin(), res.candidateCids.end()), res.candidateCids.end());
            return res;
        }
        auto itCtx = ownerCtxCache.find(owner);
        if (itCtx == ownerCtxCache.end()) {
            itCtx = ownerCtxCache.emplace(owner, topo_.buildOwnerBucketContext(owner)).first;
        }
        const auto& ctx = itCtx->second;
        if (!ctx.valid) return runFullScanFallbackForClass(owner, oldCid, st, 3);
        bool sameBucket = true;
        int firstBucket = -1;
        for (const auto& p : workingPieces) {
            int rep = p.pieceRepresentativeEndpoint;
            int b = topo_.classifyEndpointBucketWithContext(owner, rep, ctx);
#ifdef LOCAL
            g_batch_dbg.rep_bucket_checks++;
#endif
            if (b < 0) return runFullScanFallbackForClass(owner, oldCid, st, 4);
            if (firstBucket == -1) firstBucket = b;
            else if (firstBucket != b) sameBucket = false;
        }
        if (!sameBucket) return runFullScanFallbackForClass(owner, oldCid, st, 7);
        res.pieceNativePlanned = true;
        res.reuseKind = SupportReuseKind::RepUnanimous;
        for (const auto& p : workingPieces) res.connectorRepVertices.push_back(p.pieceRepresentativeEndpoint);
        sort(res.candidateCids.begin(), res.candidateCids.end());
        res.candidateCids.erase(unique(res.candidateCids.begin(), res.candidateCids.end()), res.candidateCids.end());
        return res;
    }

    CoverageCollapseResult refineTouchedClassCoverageCollapse(
        int x,
        const TouchedClassInfo& info,
        unordered_map<int, DecrementalNBTopology::OwnerBucketContext>& ownerCtxCache) {
        CoverageCollapseResult res;
        res.keptCid = info.oldCid;
        res.candidateCids.push_back(info.oldCid);
        int owner = info.owner;
        int oldCid = info.oldCid;
        if (!(1 <= owner && owner <= n_) || !topo_.aliveVertex(owner) || oldCid < 0) return res;
        auto& od = ownerData_[owner];
        auto itState = od.classStates.find(oldCid);
        if (itState == od.classStates.end()) return res;
        auto& st = itState->second;
#ifdef LOCAL
        g_batch_dbg.touched_class_total++;
        g_batch_dbg.piece_shadow_skip_classes_total++;
        g_topo_dbg.class_local_refine_calls++;
#endif

        if (!st.supportMetaValid && st.pieceModeActive) {
            return refineTouchedClassCoverageCollapsePieceNative(x, info, ownerCtxCache);
        }

        auto runFullScanFallback = [&](int why = 0) -> CoverageCollapseResult {
            return runFullScanFallbackForClass(owner, oldCid, st, why);
        };

        if (!st.supportMetaValid || !(0 <= info.xHandleIdx && info.xHandleIdx < (int)st.supportVerts.size())) {
            return runFullScanFallback(1);
        }
        int xPos = info.xHandleIdx;
        if (xPos >= (int)st.watchHandles.size() || st.watchHandles[xPos].vertex != x || st.supportVerts[xPos] != x) {
            return runFullScanFallback(2);
        }

        res.xPos = xPos;
        int totalEndpoints = (int)st.activeEndpointIdxsSortedBySupportTin.size();
        int deadIdx = -1;
        if (0 <= xPos && xPos < (int)st.supportNodeEndpointIdx.size()) deadIdx = st.supportNodeEndpointIdx[xPos];

        vector<PositiveComponentDesc> comps;
        for (int c = st.supportChildFirst[xPos]; c != -1; c = st.supportNextSibling[c]) {
            int cnt = st.supportSubtreeEndpointCount[c];
            if (cnt <= 0) continue;
            int rep = st.supportSubtreeRepresentativeEndpoint[c];
            if (!(1 <= rep && rep <= n_)) return runFullScanFallback(4);
            PositiveComponentDesc comp;
            comp.parentSide = false;
            comp.childPos = c;
            comp.count = cnt;
            comp.repVertex = rep;
            comp.bucket = -1;
            comp.rootPos = c;
            comps.push_back(comp);
        }
        int parentCount = totalEndpoints - st.supportSubtreeEndpointCount[xPos];
        if (parentCount > 0) {
            int rep = pickRepresentativeOutsideSubtreeVertex(owner, st, st.supportTin[xPos], st.supportTout[xPos]);
            if (!(1 <= rep && rep <= n_)) return runFullScanFallback(4);
            PositiveComponentDesc comp;
            comp.parentSide = true;
            comp.childPos = -1;
            comp.count = parentCount;
            comp.repVertex = rep;
            comp.bucket = -1;
            comp.rootPos = st.supportRootPos;
            comps.push_back(comp);
        }
        res.positiveComps = comps;
#ifdef LOCAL
        g_batch_dbg.support_positive_component_total += (long long)comps.size();
#endif

        auto handleDeadEndpoint = [&]() {
            if (deadIdx >= 0 && deadIdx < (int)od.endpoints.size()) {
                topo_.deactivateEndpoint(owner, od.endpoints[deadIdx]);
                res.movedIdxs.push_back(deadIdx);
#ifdef LOCAL
                g_batch_dbg.moved_endpoint_total++;
                g_topo_dbg.class_local_refine_moved_endpoints++;
#endif
            }
            sort(res.candidateCids.begin(), res.candidateCids.end());
            res.candidateCids.erase(unique(res.candidateCids.begin(), res.candidateCids.end()), res.candidateCids.end());
        };

        if ((int)comps.size() <= 1) {
#ifdef LOCAL
            g_batch_dbg.skip_by_single_positive_component++;
            g_batch_dbg.piece_shadow_single_positive_classes++;
#endif
            if ((int)comps.size() == 1) res.reuseKind = SupportReuseKind::SinglePositive;
            handleDeadEndpoint();
            return res;
        }

        auto itCtx = ownerCtxCache.find(owner);
        if (itCtx == ownerCtxCache.end()) {
            itCtx = ownerCtxCache.emplace(owner, topo_.buildOwnerBucketContext(owner)).first;
        }
        const auto& ctx = itCtx->second;
        if (!ctx.valid) return runFullScanFallback(3);

        bool sameBucket = true;
        int firstBucket = -1;
        for (auto& comp : comps) {
            comp.bucket = topo_.classifyEndpointBucketWithContext(owner, comp.repVertex, ctx);
#ifdef LOCAL
            g_batch_dbg.rep_bucket_checks++;
#endif
            if (comp.bucket < 0) return runFullScanFallback(4);
            if (firstBucket == -1) firstBucket = comp.bucket;
            else if (firstBucket != comp.bucket) sameBucket = false;
        }

        if (sameBucket) {
#ifdef LOCAL
            g_batch_dbg.skip_by_rep_bucket_unanimous++;
            g_batch_dbg.piece_shadow_unanimous_classes++;
#endif
            res.reuseKind = SupportReuseKind::RepUnanimous;
            res.positiveComps = comps;
            handleDeadEndpoint();
            return res;
        }

#ifdef LOCAL
        g_batch_dbg.split_required_class_count++;
        g_batch_dbg.piece_shadow_split_classes++;
#endif

        unordered_map<int,int> bucketCount;
        for (const auto& comp : comps) bucketCount[comp.bucket] += comp.count;
        int keptBucket = -1;
        int keptCount = -1;
        for (const auto& kv : bucketCount) {
            if (kv.second > keptCount || (kv.second == keptCount && kv.first < keptBucket)) {
                keptBucket = kv.first;
                keptCount = kv.second;
            }
        }
#ifdef LOCAL
        g_batch_dbg.largest_bucket_kept_count++;
#endif

        unordered_map<int,int> bucketToCid;
        bucketToCid[keptBucket] = oldCid;
        for (const auto& kv : bucketCount) if (kv.first != keptBucket) {
            bucketToCid[kv.first] = topo_.allocateFreshClassId(owner);
            res.candidateCids.push_back(bucketToCid[kv.first]);
        }

        auto appendMovedFromComponent = [&](const PositiveComponentDesc& comp, vector<int>& acc) {
            int before = (int)acc.size();
            if (comp.parentSide) {
                appendEndpointIdxOutsideSubtree(st, st.supportTin[xPos], st.supportTout[xPos], acc);
            } else {
                int c = comp.childPos;
                appendEndpointIdxRangeByTin(st, st.supportTin[c], st.supportTout[c], acc);
            }
#ifdef LOCAL
            g_batch_dbg.moved_endpoint_enumerations += (long long)acc.size() - before;
#endif
        };

        vector<int> movedIdxs;
        for (const auto& comp : comps) {
            if (comp.bucket == keptBucket) continue;
            vector<int> compIdxs;
            appendMovedFromComponent(comp, compIdxs);
            int newCid = bucketToCid[comp.bucket];
            for (int idx : compIdxs) {
                if (idx < 0 || idx >= (int)od.endpoints.size()) continue;
                topo_.assignEndpointClass(owner, od.endpoints[idx], newCid);
                movedIdxs.push_back(idx);
            }
        }

        if (deadIdx >= 0 && deadIdx < (int)od.endpoints.size()) {
            topo_.deactivateEndpoint(owner, od.endpoints[deadIdx]);
            movedIdxs.push_back(deadIdx);
#ifdef LOCAL
            g_batch_dbg.moved_endpoint_enumerations++;
#endif
        }

        sort(movedIdxs.begin(), movedIdxs.end());
        movedIdxs.erase(unique(movedIdxs.begin(), movedIdxs.end()), movedIdxs.end());
        res.movedIdxs = std::move(movedIdxs);
#ifdef LOCAL
        g_batch_dbg.moved_endpoint_total += (long long)res.movedIdxs.size();
        g_topo_dbg.class_local_refine_moved_endpoints += (long long)res.movedIdxs.size();
        g_topo_dbg.class_local_refine_endpoints += (long long)res.movedIdxs.size();
#endif
        sort(res.candidateCids.begin(), res.candidateCids.end());
        res.candidateCids.erase(unique(res.candidateCids.begin(), res.candidateCids.end()), res.candidateCids.end());
        return res;
    }

    bool applyPieceNativeReuseForClassBaseline(int owner, int cid,
                                      const TouchedClassInfo& info,
                                      const CoverageCollapseResult& refineRes) {
        if (!(1 <= owner && owner <= n_) || cid < 0) return false;
        if (!refineRes.pieceNativePlanned || refineRes.usedFullScan || refineRes.reuseKind == SupportReuseKind::None) return false;
        auto& od = ownerData_[owner];
        auto itState = od.classStates.find(cid);
        if (itState == od.classStates.end()) return false;
        auto& st = itState->second;
        if (!st.pieceModeActive) return false;
        ConnectorShadowEstimate connectorShadowEst;
#ifdef LOCAL
        if (refineRes.reuseKind == SupportReuseKind::RepUnanimous) {
            connectorShadowEst = estimateConnectorDeltaForUnanimous(owner, st, info, refineRes);
        }
#endif
        if (!topo_.aliveVertex(owner) || st.activeQueryCount <= 0) {
            unregisterClassWatch(owner, cid, st);
            if (++st.epoch == INT_MAX) st.epoch = 1;
            st.endpointPool.clear();
            return true;
        }
        vector<int> relevantIdxs = collectRelevantEndpointIdxs(owner, cid, true);
        if (relevantIdxs.empty() && st.activeQueryCount > 0) relevantIdxs = fallbackRelevantEndpointIdxsFromQueries(owner, cid);
        if (relevantIdxs.empty()) {
            unregisterClassWatch(owner, cid, st);
            if (++st.epoch == INT_MAX) st.epoch = 1;
            st.activeQueryCount = 0;
            st.endpointPool.clear();
            return true;
        }
        const bool wscanOpt = watch_scan_opt_enabled();
        const bool retainOpt = retain_compaction_opt_enabled();
        vector<char> keepMask(st.watchHandles.size(), 1);
        vector<int> sparseRemovedIdxs;
        if (retainOpt) sparseRemovedIdxs.reserve(st.connectorWatchEntryIds.size() + 8);
        long long removedVertices = 0;
        long long currentConnectorRemoved = (long long)st.connectorWatchEntryIds.size();
        long long currentConnectorAdded = 0;
        unordered_map<int, tuple<int,int,int>> vertexToMeta;
        if (refineRes.hitOldPieceId >= 0) {
            for (const auto& piece : refineRes.replacementPieces) {
                const auto* tree = getSupportTreeObject(piece.treeId);
                if (!tree) return false;
                forEachPiecePos(*tree, piece, [&](int pos){
                    int v = tree->vertexByPos[pos];
                    vertexToMeta[v] = make_tuple(piece.treeId, piece.pieceId, pos);
                });
            }
        }
        if (wscanOpt) {
#ifdef LOCAL
            if (!st.connectorWatchEntryIds.empty()) g_batch_dbg.wscan_used_connectorWatchEntryIds_fastpath_calls++;
            g_batch_dbg.wscan_connector_keepmask_scans++;
            g_batch_dbg.wscan_handles_scanned_connector_keepmask += (long long)st.connectorWatchEntryIds.size();
            long long __connector_keepmask_start_ns = local_profile_detailed_enabled() ? dbg_now_ns() : 0;
#endif
            for (int hi : st.connectorWatchEntryIds) {
                if (!(0 <= hi && hi < (int)st.watchHandles.size())) continue;
                if (keepMask[hi]) {
                    keepMask[hi] = 0;
                    if (retainOpt) sparseRemovedIdxs.push_back(hi);
                    ++removedVertices;
#ifdef LOCAL
                    g_batch_dbg.reuse_keepmask_removed_handles++;
                    g_batch_dbg.reuse_keepmask_removed_connector_handles++;
#endif
                }
            }
#ifdef LOCAL
            if (local_profile_detailed_enabled()) {
                acc_wscan_keepmask_ns(dbg_now_ns() - __connector_keepmask_start_ns,
                                      &g_batch_dbg.time_wscan_connector_keepmask_decision_ns,
                                      &g_batch_dbg.time_wscan_connector_keepmask_decision_calls);
            }
#endif
            if (refineRes.hitOldPieceId >= 0) {
#ifdef LOCAL
                noteReuseWatchFullScan(st);
                g_batch_dbg.wscan_used_preservedHandleIdxs_fastpath_calls++;
                g_batch_dbg.wscan_preserved_keepmask_scans++;
                g_batch_dbg.wscan_handles_scanned_preserved_keepmask += (long long)st.watchHandles.size();
                long long __preserved_keepmask_start_ns = local_profile_detailed_enabled() ? dbg_now_ns() : 0;
#endif
                for (int i = 0; i < (int)st.watchHandles.size(); ++i) {
                    const auto& h = st.watchHandles[i];
                    if (h.originKind != SupportOriginKind::PreservedPiece || h.pieceId != refineRes.hitOldPieceId) continue;
                    if (!vertexToMeta.count(h.vertex) && keepMask[i]) {
                        keepMask[i] = 0;
                        if (retainOpt) sparseRemovedIdxs.push_back(i);
                        ++removedVertices;
#ifdef LOCAL
                        g_batch_dbg.reuse_keepmask_removed_handles++;
                        g_batch_dbg.reuse_keepmask_removed_preserved_handles++;
#endif
                    }
                }
#ifdef LOCAL
                if (local_profile_detailed_enabled()) {
                    acc_wscan_keepmask_ns(dbg_now_ns() - __preserved_keepmask_start_ns,
                                          &g_batch_dbg.time_wscan_preserved_keepmask_decision_ns,
                                          &g_batch_dbg.time_wscan_preserved_keepmask_decision_calls);
                }
#endif
            }
        } else {
#ifdef LOCAL
            noteReuseWatchFullScan(st);
            g_batch_dbg.wscan_preserved_keepmask_scans++;
            g_batch_dbg.wscan_handles_scanned_preserved_keepmask += (long long)st.watchHandles.size();
            long long __keepmask_start_ns = local_profile_detailed_enabled() ? dbg_now_ns() : 0;
#endif
            for (int i = 0; i < (int)st.watchHandles.size(); ++i) {
                const auto& h = st.watchHandles[i];
                if (h.originKind == SupportOriginKind::ConnectorTree) {
                    keepMask[i] = 0;
                    if (retainOpt) sparseRemovedIdxs.push_back(i);
                    ++removedVertices;
#ifdef LOCAL
                    g_batch_dbg.reuse_keepmask_removed_handles++;
                    g_batch_dbg.reuse_keepmask_removed_connector_handles++;
#endif
                } else if (refineRes.hitOldPieceId >= 0 && h.originKind == SupportOriginKind::PreservedPiece && h.pieceId == refineRes.hitOldPieceId) {
                    if (!vertexToMeta.count(h.vertex)) {
                        keepMask[i] = 0;
                        if (retainOpt) sparseRemovedIdxs.push_back(i);
                        ++removedVertices;
#ifdef LOCAL
                        g_batch_dbg.reuse_keepmask_removed_handles++;
                        g_batch_dbg.reuse_keepmask_removed_preserved_handles++;
#endif
                    }
                }
            }
#ifdef LOCAL
            if (local_profile_detailed_enabled()) {
                acc_wscan_keepmask_ns(dbg_now_ns() - __keepmask_start_ns,
                                      &g_batch_dbg.time_wscan_preserved_keepmask_decision_ns,
                                      &g_batch_dbg.time_wscan_preserved_keepmask_decision_calls);
            }
#endif
        }
#ifdef LOCAL
        {
            ScopedIntInc __wscan_ctx(&g_wscan_retain_ctx);
            retainClassWatchByKeepMask(owner, cid, st, keepMask, retainOpt ? &sparseRemovedIdxs : nullptr);
        }
#else
        retainClassWatchByKeepMask(owner, cid, st, keepMask, retainOpt ? &sparseRemovedIdxs : nullptr);
#endif
        if (refineRes.hitOldPieceId >= 0) {
#ifdef LOCAL
            ScopedNsAcc __retag_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_preserved_direct_retag_ns),
                                      ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_preserved_direct_retag_calls));
#endif
            for (auto& h : st.watchHandles) {
                if (h.originKind != SupportOriginKind::PreservedPiece || h.pieceId != refineRes.hitOldPieceId) continue;
                auto it = vertexToMeta.find(h.vertex);
                if (it == vertexToMeta.end()) continue;
                annotateHandleMetadata(h, SupportOriginKind::PreservedPiece, get<0>(it->second), get<1>(it->second), get<2>(it->second));
#ifdef LOCAL
                g_batch_dbg.reuse_preserved_direct_retag_handles++;
#endif
            }
        }
        vector<SupportPieceRef> newPreserved;
        newPreserved.reserve(st.preservedPieces.size() + refineRes.replacementPieces.size());
        for (const auto& p : st.preservedPieces) {
            if (!p.pieceAlive) continue;
            if (p.pieceId == refineRes.hitOldPieceId) continue;
            newPreserved.push_back(p);
        }
        for (const auto& p : refineRes.replacementPieces) if (p.pieceAlive && p.pieceEndpointCount > 0) newPreserved.push_back(p);
        if (refineRes.hitOldPieceId < 0) {
            newPreserved = st.preservedPieces;
        }
        int newConnectorTreeId = -1;
        vector<int> newConnectorHandleIdxs;
        if (refineRes.reuseKind == SupportReuseKind::RepUnanimous) {
            vector<int> reps = refineRes.connectorRepVertices;
            sort(reps.begin(), reps.end());
            reps.erase(unique(reps.begin(), reps.end()), reps.end());
            if (reps.size() > 1) {
                long long visV = 0, chainSteps = 0;
#ifdef LOCAL
                g_batch_dbg.reuse_patch_tree_build_calls++;
                ScopedNsAcc __patch_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_patch_tree_build_ns),
                                          ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_patch_tree_build_calls));
#endif
                SupportBuildProduct connector = buildSupportProductFromLastDeleteArtifact(owner, reps, &visV, &chainSteps);
                if (connector.watchVerts.empty()) return false;
                newConnectorTreeId = storeSupportTreeObjectFromProduct(connector, &connector.nodeEndpointIdx);
                if (newConnectorTreeId <= 0) return false;
#ifdef LOCAL
                g_batch_dbg.reuse_patch_vertices += (long long)connector.watchVerts.size();
#endif
                supportScratch_.ensure(n_);
                int stamp = supportScratch_.nextCollect();
                for (const auto& h : st.watchHandles) if (1 <= h.vertex && h.vertex <= n_) supportScratch_.collectStamp[h.vertex] = stamp;
                vector<int> connectorOnlyVerts;
                connectorOnlyVerts.reserve(connector.watchVerts.size());
                for (int v : connector.watchVerts) {
                    if (1 <= v && v <= n_ && supportScratch_.collectStamp[v] == stamp) continue;
                    connectorOnlyVerts.push_back(v);
                    if (1 <= v && v <= n_) supportScratch_.collectStamp[v] = stamp;
                }
                appendClassWatchEntries(owner, cid, st, connectorOnlyVerts, &newConnectorHandleIdxs);
                annotateConnectorHandles(st, newConnectorHandleIdxs, newConnectorTreeId);
#ifdef LOCAL
                g_batch_dbg.piece_native_unanimous_added_connector_vertices += (long long)connectorOnlyVerts.size();
#endif
                currentConnectorAdded = (long long)connectorOnlyVerts.size();
            }
        }
        clearMaterializedMetadataOnly(st);
        clearPieceStateOnly(st);
        st.pieceModeActive = true;
        st.preservedPieces = std::move(newPreserved);
        StatePublishContext __publish_ctx;
        ScopedStatePublishContext __publish_scope(this, &__publish_ctx);
#ifdef LOCAL
        ScopedNsAcc __state_publish_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_state_publish_ns),
                                          ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_state_publish_calls));
        ScopedNsAcc __reuse_publish_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_final_publish_commit_ns),
                                          ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_final_publish_commit_calls));
        g_batch_dbg.reuse_final_publish_calls++;
#endif
        st.connectorTreeId = newConnectorTreeId;
        st.connectorWatchEntryIds = std::move(newConnectorHandleIdxs);
        dispatchPublishAnnotatePreserved(st, st.preservedPieces);
        if (st.connectorTreeId > 0) dispatchPublishAnnotateConnectorHandles(st, st.connectorWatchEntryIds, st.connectorTreeId);
#ifdef LOCAL
        long long pieceWatchVertices = 0;
        for (const auto& h : st.watchHandles) if (h.originKind == SupportOriginKind::PreservedPiece) ++pieceWatchVertices;
        if (refineRes.reuseKind == SupportReuseKind::SinglePositive) {
            g_batch_dbg.piece_native_single_calls++;
            if (!info.pieceHits.empty()) g_batch_dbg.piece_native_single_preserved_hits++;
            if (!info.connectorHits.empty()) g_batch_dbg.piece_native_single_connector_hits++;
            g_batch_dbg.piece_native_single_reused_vertices += pieceWatchVertices;
            g_batch_dbg.piece_native_single_removed_vertices += removedVertices;
            g_batch_dbg.piece_native_single_boundary_ops += max<long long>(1, (long long)refineRes.replacementPieces.size());
        } else if (refineRes.reuseKind == SupportReuseKind::RepUnanimous) {
            g_batch_dbg.piece_native_unanimous_calls++;
            if (!info.pieceHits.empty()) g_batch_dbg.piece_native_unanimous_preserved_hits++;
            if (!info.connectorHits.empty()) g_batch_dbg.piece_native_unanimous_connector_hits++;
            g_batch_dbg.piece_native_unanimous_reused_vertices += pieceWatchVertices;
            g_batch_dbg.piece_native_unanimous_removed_vertices += removedVertices;
            g_batch_dbg.piece_native_unanimous_boundary_ops += max<long long>(1, (long long)refineRes.replacementPieces.size());
            g_batch_dbg.connector_shadow_current_removed_vertices += currentConnectorRemoved;
            g_batch_dbg.connector_shadow_current_added_vertices += currentConnectorAdded;
            long long currentTotal = currentConnectorRemoved + currentConnectorAdded;
            long long candidateTotal = connectorShadowEst.candidateRemoved + connectorShadowEst.candidatePatch + connectorShadowEst.candidateRetargets;
            if (currentTotal > candidateTotal) g_batch_dbg.connector_shadow_estimated_saved_vertices += (currentTotal - candidateTotal);
        }
#endif
        return true;
    }



    vector<int> computeLiveAttachmentVerticesForPieces(const vector<SupportPieceRef>& pieces,
                                                       const vector<int>& oldAttachmentVertices,
                                                       long long* outRetargets = nullptr) {
        vector<int> attachments;
        attachments.reserve(pieces.size());
        if (outRetargets) *outRetargets = 0;
        for (size_t i = 0; i < pieces.size(); ++i) {
            const auto& piece = pieces[i];
            const auto* tree = getSupportTreeObject(piece.treeId);
            int att = (i < oldAttachmentVertices.size() ? oldAttachmentVertices[i] : -1);
            bool ok = false;
            if (tree && 1 <= att && att <= n_ && topo_.aliveVertex(att)) {
                int pstamp = 0;
                buildTreeVertexPosMap(*tree, pstamp);
                if (supportScratch_.supportPosStamp[att] == pstamp) {
                    int pos = supportScratch_.supportPosVal[att];
                    ok = pieceContainsPos(*tree, piece, pos);
                }
            }
            if (!ok && tree) {
                int newAtt = retargetAttachmentIfDead(*tree, piece);
                if (1 <= newAtt && newAtt <= n_) {
                    if (outRetargets && newAtt != att) (*outRetargets)++;
                    att = newAtt;
                }
            }
            attachments.push_back(att);
        }
        return attachments;
    }

#ifdef LOCAL
    void accountConnectorSkeletonShadow(const ClassState& st,
                                        const vector<SupportPieceRef>& preservedPieces,
                                        const vector<int>& attachmentVertices,
                                        int owner) {
        g_batch_dbg.connector_skeleton_shadow_classes++;
        long long currentRemoved = (long long)st.connectorWatchEntryIds.size();
        long long currentAdded = currentRemoved;
        g_batch_dbg.connector_skeleton_shadow_current_removed_vertices += currentRemoved;
        g_batch_dbg.connector_skeleton_shadow_current_added_vertices += currentAdded;
        vector<int> terminals = normalizedAliveAttachmentVertices(attachmentVertices);
        g_batch_dbg.connector_skeleton_shadow_candidate_terminals += (long long)terminals.size();
        long long candidateVertices = 0;
        long long candidateUnreg = currentRemoved;
        long long candidateReg = 0;
        if (terminals.size() <= 1) {
            g_batch_dbg.connector_skeleton_shadow_candidate_no_rebuild_needed++;
        } else {
            long long visV = 0, chainSteps = 0;
            SupportBuildProduct connector = buildSupportProductFromLastDeleteArtifact(owner, terminals, &visV, &chainSteps);
            supportScratch_.ensure(n_);
            int stamp = supportScratch_.nextCollect();
            for (const auto& piece : preservedPieces) {
                const auto* tree = getSupportTreeObject(piece.treeId);
                if (!tree) continue;
                forEachPiecePos(*tree, piece, [&](int pos){
                    int v = tree->vertexByPos[pos];
                    if (1 <= v && v <= n_) supportScratch_.collectStamp[v] = stamp;
                });
            }
            for (int v : connector.watchVerts) {
                if (1 <= v && v <= n_ && supportScratch_.collectStamp[v] == stamp) continue;
                ++candidateVertices;
            }
            candidateReg = candidateVertices;
        }
        g_batch_dbg.connector_skeleton_shadow_candidate_vertices += candidateVertices;
        g_batch_dbg.connector_skeleton_shadow_candidate_watch_unregister += candidateUnreg;
        g_batch_dbg.connector_skeleton_shadow_candidate_watch_register += candidateReg;
        long long currentTot = currentRemoved + currentAdded;
        long long candTot = candidateUnreg + candidateReg;
        if (currentTot > candTot) g_batch_dbg.connector_skeleton_shadow_estimated_saved_vertices += (currentTot - candTot);
    }
#endif

#ifdef LOCAL
    void accountConnectorSkeletonOverlapShadow(const vector<int>& oldSkeletonVerts,
                                               const vector<int>& newSkeletonVerts,
                                               const vector<int>& oldWatchVerts,
                                               const vector<int>& newWatchVerts) {
        supportScratch_.ensure(n_);
        int stamp = supportScratch_.nextCollect();
        long long commonFull = 0;
        for (int v : oldSkeletonVerts) if (1 <= v && v <= n_) supportScratch_.collectStamp[v] = stamp;
        for (int v : newSkeletonVerts) if (1 <= v && v <= n_ && supportScratch_.collectStamp[v] == stamp) ++commonFull;
        long long oldFull = (long long)oldSkeletonVerts.size();
        long long newFull = (long long)newSkeletonVerts.size();
        long long addedFull = max<long long>(0, newFull - commonFull);
        long long removedFull = max<long long>(0, oldFull - commonFull);
        g_batch_dbg.connector_skeleton_old_vertices += oldFull;
        g_batch_dbg.connector_skeleton_new_vertices += newFull;
        g_batch_dbg.connector_skeleton_common_vertices += commonFull;
        g_batch_dbg.connector_skeleton_added_vertices += addedFull;
        g_batch_dbg.connector_skeleton_removed_vertices += removedFull;
        if (newFull > 0) g_batch_dbg.connector_skeleton_intersection_ratio_permille += (commonFull * 1000LL) / newFull;
        int stamp2 = supportScratch_.nextCollect();
        long long commonWatch = 0;
        for (int v : oldWatchVerts) if (1 <= v && v <= n_) supportScratch_.collectStamp[v] = stamp2;
        for (int v : newWatchVerts) if (1 <= v && v <= n_ && supportScratch_.collectStamp[v] == stamp2) ++commonWatch;
        long long oldWatch = (long long)oldWatchVerts.size();
        long long newWatch = (long long)newWatchVerts.size();
        long long addedWatch = max<long long>(0, newWatch - commonWatch);
        long long removedWatch = max<long long>(0, oldWatch - commonWatch);
        g_batch_dbg.connector_watch_full_unregister += oldWatch;
        g_batch_dbg.connector_watch_full_register += newWatch;
        g_batch_dbg.connector_watch_diff_unregister += removedWatch;
        g_batch_dbg.connector_watch_diff_register += addedWatch;
        g_batch_dbg.connector_watch_diff_reused += commonWatch;
    }
#endif

    bool applyConnectorSkeletonRebuildForClass(int owner, int cid,
                                               const TouchedClassInfo& info,
                                               const CoverageCollapseResult& refineRes,
                                               bool forced = false) {
        if (!(1 <= owner && owner <= n_) || cid < 0) return false;
        auto& od = ownerData_[owner];
        auto itState = od.classStates.find(cid);
        if (itState == od.classStates.end()) return false;
        auto& st = itState->second;
        saveDeleteWatchSnapshotOld(owner, cid, st);
#ifdef LOCAL
        bool candidate = (refineRes.reuseKind == SupportReuseKind::RepUnanimous && st.pieceModeActive && !st.supportMetaValid && refineRes.pieceNativePlanned);
        if (candidate) g_batch_dbg.connector_skeleton_candidate_classes++;
#endif
        if (!st.pieceModeActive || refineRes.reuseKind != SupportReuseKind::RepUnanimous || st.supportMetaValid || !refineRes.pieceNativePlanned) {
#ifdef LOCAL
            g_batch_dbg.connector_skeleton_reject_state_not_unanimous++;
#endif
            return false;
        }
        if (st.preservedPieces.empty()) {
#ifdef LOCAL
            g_batch_dbg.connector_skeleton_reject_no_preserved_pieces++;
#endif
            return false;
        }
        if (st.attachmentVerticesByPiece.empty()) syncAttachmentVerticesByPiece(st);
        if (st.attachmentVerticesByPiece.empty()) {
#ifdef LOCAL
            g_batch_dbg.connector_skeleton_reject_no_attachment_vertices++;
#endif
            return false;
        }
        if (!forced && info.pieceHits.empty() && info.connectorHits.empty()) {
#ifdef LOCAL
            g_batch_dbg.connector_skeleton_reject_origin_kind++;
#endif
            return false;
        }
#ifdef LOCAL
        g_batch_dbg.debug_unanimous_state_new_field_read++;
#endif
        if (!topo_.aliveVertex(owner) || st.activeQueryCount <= 0) {
            unregisterClassWatch(owner, cid, st);
            if (++st.epoch == INT_MAX) st.epoch = 1;
            st.endpointPool.clear();
            return true;
        }
        vector<int> relevantIdxs = collectRelevantEndpointIdxs(owner, cid, true);
        if (relevantIdxs.empty() && st.activeQueryCount > 0) relevantIdxs = fallbackRelevantEndpointIdxsFromQueries(owner, cid);
        if (relevantIdxs.empty()) {
            unregisterClassWatch(owner, cid, st);
            if (++st.epoch == INT_MAX) st.epoch = 1;
            st.activeQueryCount = 0;
            st.endpointPool.clear();
            return true;
        }
        ensureConnectorPiecesMigrated(st);
#ifdef LOCAL
        if (refineRes.hitOldPieceId >= 0) g_batch_dbg.reuse_old_piece_hits++;
        if (!info.connectorHits.empty()) g_batch_dbg.reuse_old_connector_hits++;
        g_batch_dbg.reuse_replacement_pieces += (long long)refineRes.replacementPieces.size();
        g_batch_dbg.dispatch_candidate_cids += (long long)refineRes.candidateCids.size();
#endif
        const bool psplitOpt = preserved_split_opt_enabled();
        const bool wscanOpt = watch_scan_opt_enabled();
        const bool retainOpt = retain_compaction_opt_enabled();
        unordered_map<int,int> oldAttachmentByPiece;
#ifdef LOCAL
        {
            ScopedNsAcc __reuse_old_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_old_attachment_map_build_ns),
                                         ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_old_attachment_map_build_calls));
            ScopedNsAcc __psplit_old_index_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_old_attachment_index_build_ns),
                                                 ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_old_attachment_index_build_calls));
            oldAttachmentByPiece.reserve(st.preservedPieces.size() * 2 + 1);
            for (size_t i = 0; i < st.preservedPieces.size() && i < st.attachmentVerticesByPiece.size(); ++i) {
                oldAttachmentByPiece[st.preservedPieces[i].pieceId] = st.attachmentVerticesByPiece[i];
            }
            g_batch_dbg.psplit_old_attachment_map_entries += (long long)oldAttachmentByPiece.size();
        }
#else
        oldAttachmentByPiece.reserve(st.preservedPieces.size() * 2 + 1);
        for (size_t i = 0; i < st.preservedPieces.size() && i < st.attachmentVerticesByPiece.size(); ++i) {
            oldAttachmentByPiece[st.preservedPieces[i].pieceId] = st.attachmentVerticesByPiece[i];
        }
#endif
#ifdef LOCAL
        long long __piece_split_start_ns = local_profile_detailed_enabled() ? dbg_now_ns() : 0;
#endif
        vector<SupportPieceRef> newPreserved;
        vector<int> newAttachmentVertices;
        vector<unsigned char> newAttachmentNeedsFixup;
        unordered_set<int> splitOldPieceIds;
        unordered_map<int,int> cachedTreePosStamp;
        unordered_set<int> seenTreePosStampBuilds;
        auto ensureTreePosStamp = [&](int treeId, const SupportTreeObject*& tree) -> int {
            tree = getSupportTreeObject(treeId);
            if (!tree) return 0;
            if (psplitOpt) {
                auto it = cachedTreePosStamp.find(treeId);
                if (it != cachedTreePosStamp.end()) {
#ifdef LOCAL
                    g_batch_dbg.psplit_tree_posmap_cache_hits++;
#endif
                    return it->second;
                }
            } else {
#ifdef LOCAL
                if (seenTreePosStampBuilds.count(treeId)) g_batch_dbg.psplit_same_tree_posmap_rebuilds++;
                seenTreePosStampBuilds.insert(treeId);
#endif
            }
            int stamp = 0;
#ifdef LOCAL
            {
                ScopedNsAcc __tree_posmap_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_tree_posmap_build_ns),
                                                ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_tree_posmap_build_calls));
                buildTreeVertexPosMap(*tree, stamp);
            }
            g_batch_dbg.psplit_tree_posmap_build_calls++;
#else
            buildTreeVertexPosMap(*tree, stamp);
#endif
            if (psplitOpt) cachedTreePosStamp[treeId] = stamp;
            return stamp;
        };
        auto findXLocalPosForPiece = [&](const SupportPieceRef& piece) -> int {
#ifdef LOCAL
            ScopedNsAcc __x_lookup_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_x_local_pos_lookup_ns),
                                         ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_x_local_pos_lookup_calls));
            g_batch_dbg.psplit_x_local_pos_lookup_calls++;
#endif
            for (const auto& ph : info.pieceHits) {
                if (ph.pieceId == piece.pieceId && ph.treeId == piece.treeId) {
#ifdef LOCAL
                    g_batch_dbg.psplit_x_local_pos_lookup_direct_hits++;
#endif
                    return ph.localPos;
                }
            }
            if (!(1 <= currentDeleteX_ && currentDeleteX_ <= n_)) return -1;
            const SupportTreeObject* tree = nullptr;
            int stamp = ensureTreePosStamp(piece.treeId, tree);
#ifdef LOCAL
            g_batch_dbg.psplit_x_local_pos_lookup_posmap_lookups++;
#endif
            if (!tree || stamp == 0 || supportScratch_.supportPosStamp[currentDeleteX_] != stamp) return -1;
            return supportScratch_.supportPosVal[currentDeleteX_];
        };
        newPreserved.reserve(st.preservedPieces.size() + refineRes.replacementPieces.size() + 4);
        newAttachmentVertices.reserve(st.preservedPieces.size() + refineRes.replacementPieces.size() + 4);
        newAttachmentNeedsFixup.reserve(st.preservedPieces.size() + refineRes.replacementPieces.size() + 4);
        for (const auto& p : st.preservedPieces) {
#ifdef LOCAL
            ScopedNsAcc __old_piece_scan_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_old_piece_scan_ns),
                                               ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_old_piece_scan_calls));
            g_batch_dbg.psplit_preserved_pieces_scanned++;
#endif
            if (!p.pieceAlive || p.pieceEndpointCount <= 0) continue;
            auto itOldAtt = oldAttachmentByPiece.find(p.pieceId);
            int oldAtt = (itOldAtt != oldAttachmentByPiece.end()) ? itOldAtt->second : -1;
            bool containsX = false;
#ifdef LOCAL
            {
                ScopedNsAcc __contains_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_contains_x_check_ns),
                                             ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_contains_x_check_calls));
                g_batch_dbg.psplit_contains_x_checks++;
                containsX = (1 <= currentDeleteX_ && currentDeleteX_ <= n_ && pieceContainsVertex(p, currentDeleteX_));
            }
#else
            containsX = (1 <= currentDeleteX_ && currentDeleteX_ <= n_ && pieceContainsVertex(p, currentDeleteX_));
#endif
            if (!containsX) {
                newPreserved.push_back(p);
                newAttachmentVertices.push_back(oldAtt);
                newAttachmentNeedsFixup.push_back(psplitOpt ? 0 : 1);
#ifdef LOCAL
                if (psplitOpt) g_batch_dbg.psplit_old_attachment_fastpath_reuse_calls++;
#endif
                continue;
            }
#ifdef LOCAL
            g_batch_dbg.psplit_contains_x_hits++;
            g_batch_dbg.debug_forced_preserved_split_due_x_in_piece++;
#endif
            splitOldPieceIds.insert(p.pieceId);
            int xLocalPos = findXLocalPosForPiece(p);
            vector<SupportPieceRef> replacements;
            int removedPieceVertices = 0, boundaryOps = 0, deadIdx = -1;
            bool splitOk = false;
#ifdef LOCAL
            {
                ScopedNsAcc __split_core_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_split_piece_core_ns),
                                               ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_split_piece_core_calls));
                splitOk = (xLocalPos >= 0 && splitPieceAtLocalPos(owner, p, xLocalPos, replacements, removedPieceVertices, boundaryOps, &deadIdx));
            }
#else
            splitOk = (xLocalPos >= 0 && splitPieceAtLocalPos(owner, p, xLocalPos, replacements, removedPieceVertices, boundaryOps, &deadIdx));
#endif
            if (!splitOk) {
#ifdef LOCAL
                g_batch_dbg.piece_materialize_fallback_calls++;
                g_batch_dbg.piece_materialize_fallback_vertices += max<long long>(1, p.pieceEndpointCount);
#endif
                return false;
            }
#ifdef LOCAL
            g_batch_dbg.psplit_split_piece_calls++;
            g_batch_dbg.psplit_replacement_piece_count += (long long)replacements.size();
            g_release_diag_preserved_piece_split_calls++;
            g_release_diag_preserved_piece_split_vertices += max<long long>(1, removedPieceVertices > 0 ? removedPieceVertices : p.pieceEndpointCount);
#endif
            for (auto& rp : replacements) {
                if (!rp.pieceAlive || rp.pieceEndpointCount <= 0) continue;
                int att = oldAtt;
                const SupportTreeObject* tree = getSupportTreeObject(rp.treeId);
                bool ok = false;
                if (tree && 1 <= att && att <= n_ && topo_.aliveVertex(att)) {
#ifdef LOCAL
                    ScopedNsAcc __replacement_validate_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_replacement_attachment_validate_ns),
                                                             ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_replacement_attachment_validate_calls));
                    g_batch_dbg.psplit_attachment_validate_calls++;
#endif
                    int pstamp = ensureTreePosStamp(rp.treeId, tree);
                    if (tree && pstamp != 0 && supportScratch_.supportPosStamp[att] == pstamp) {
                        int pos = supportScratch_.supportPosVal[att];
                        ok = pieceContainsPos(*tree, rp, pos);
#ifdef LOCAL
                        if (ok) g_batch_dbg.psplit_attachment_validate_hits++;
#endif
                    }
                }
                if (!ok && tree) {
#ifdef LOCAL
                    ScopedNsAcc __replacement_retarget_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_replacement_attachment_retarget_ns),
                                                             ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_replacement_attachment_retarget_calls));
                    g_batch_dbg.psplit_attachment_retarget_calls++;
                    if (att == currentDeleteX_) g_batch_dbg.debug_attachment_retarget_due_x++;
                    else if (att != -1) g_batch_dbg.debug_attachment_retarget_due_outside_piece++;
#endif
                    int newAtt = retargetAttachmentIfDead(*tree, rp);
#ifdef LOCAL
                    if (1 <= newAtt && newAtt <= n_ && newAtt != att) g_batch_dbg.psplit_attachment_retarget_changes++;
#endif
                    att = newAtt;
                }
                if (att == currentDeleteX_) att = -1;
#ifdef LOCAL
                {
                    ScopedNsAcc __emit_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_new_piece_emit_ns),
                                             ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_new_piece_emit_calls));
                    newPreserved.push_back(rp);
                    newAttachmentVertices.push_back(att);
                    newAttachmentNeedsFixup.push_back(psplitOpt ? 0 : 1);
                    g_batch_dbg.psplit_new_piece_emit_count++;
                }
#else
                newPreserved.push_back(rp);
                newAttachmentVertices.push_back(att);
                newAttachmentNeedsFixup.push_back(psplitOpt ? 0 : 1);
#endif
            }
        }
#ifdef LOCAL
        if (local_profile_detailed_enabled()) {
            g_batch_dbg.time_reuse_piece_split_apply_ns += dbg_now_ns() - __piece_split_start_ns;
            g_batch_dbg.time_reuse_piece_split_apply_calls++;
            g_release_diag_preserved_piece_split_ns = g_batch_dbg.time_reuse_piece_split_apply_ns;
        }
#endif
        long long retargets = 0;
#ifdef LOCAL
        long long __attachment_fixup_start_ns = local_profile_detailed_enabled() ? dbg_now_ns() : 0;
#endif
        for (size_t i = 0; i < newPreserved.size(); ++i) {
            if (psplitOpt && i < newAttachmentNeedsFixup.size() && !newAttachmentNeedsFixup[i]) continue;
            int att = (i < newAttachmentVertices.size() ? newAttachmentVertices[i] : -1);
            const SupportTreeObject* tree = getSupportTreeObject(newPreserved[i].treeId);
            bool ok = false;
            if (tree && 1 <= att && att <= n_ && topo_.aliveVertex(att)) {
#ifdef LOCAL
                ScopedNsAcc __fixup_validate_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_attachment_fixup_validate_ns),
                                                   ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_attachment_fixup_validate_calls));
                g_batch_dbg.psplit_attachment_fixup_calls++;
                if (!psplitOpt) g_batch_dbg.psplit_validate_then_fixup_duplicate_checks++;
#endif
                int pstamp = ensureTreePosStamp(newPreserved[i].treeId, tree);
                if (tree && pstamp != 0 && supportScratch_.supportPosStamp[att] == pstamp) {
                    int pos = supportScratch_.supportPosVal[att];
                    ok = pieceContainsPos(*tree, newPreserved[i], pos);
#ifdef LOCAL
                    if (ok) g_batch_dbg.psplit_attachment_validate_hits++;
#endif
                }
            }
            if (!ok && tree) {
#ifdef LOCAL
                ScopedNsAcc __fixup_retarget_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_attachment_fixup_retarget_ns),
                                                   ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_attachment_fixup_retarget_calls));
#endif
                int newAtt = retargetAttachmentIfDead(*tree, newPreserved[i]);
                if (1 <= newAtt && newAtt <= n_ && newAtt != att) {
                    ++retargets;
#ifdef LOCAL
                    g_batch_dbg.psplit_attachment_fixup_changes++;
#endif
                }
                newAttachmentVertices[i] = newAtt;
            }
            if (i < newAttachmentVertices.size() && newAttachmentVertices[i] == currentDeleteX_) {
#ifdef LOCAL
                g_batch_dbg.debug_attachment_retarget_due_x++;
#endif
                newAttachmentVertices[i] = -1;
            }
        }
#ifdef LOCAL
        if (local_profile_detailed_enabled()) {
            g_batch_dbg.time_reuse_attachment_fixup_ns += dbg_now_ns() - __attachment_fixup_start_ns;
            g_batch_dbg.time_reuse_attachment_fixup_calls++;
        }
        g_batch_dbg.reuse_attachment_retargets += retargets;
        accountConnectorSkeletonShadow(st, newPreserved, newAttachmentVertices, owner);
#endif
        vector<int> oldConnectorFullVerts;
        if (st.connectorTreeId > 0) {
            if (const auto* oldTree = getSupportTreeObject(st.connectorTreeId)) oldConnectorFullVerts = oldTree->vertexByPos;
        }
        vector<int> oldConnectorWatchVerts;
        oldConnectorWatchVerts.reserve(st.connectorWatchEntryIds.size());
        for (int hi : st.connectorWatchEntryIds) {
            if (!(0 <= hi && hi < (int)st.watchHandles.size())) continue;
            const auto& h = st.watchHandles[hi];
            if (h.originKind != SupportOriginKind::ConnectorTree) continue;
            if (1 <= h.vertex && h.vertex <= n_) oldConnectorWatchVerts.push_back(h.vertex);
        }

        vector<int> validOldConnectorHandleIdxs;
        validOldConnectorHandleIdxs.reserve(st.connectorWatchEntryIds.size());
        for (int hi : st.connectorWatchEntryIds) {
            if (!(0 <= hi && hi < (int)st.watchHandles.size())) continue;
            if (st.watchHandles[hi].originKind != SupportOriginKind::ConnectorTree) continue;
            validOldConnectorHandleIdxs.push_back(hi);
        }

        vector<char> keepMask(st.watchHandles.size(), 1);
        vector<int> preservedSparseRemovedIdxs;
        if (retainOpt) preservedSparseRemovedIdxs.reserve(st.watchHandles.size() / 4 + 8);
        vector<int> keptPreservedVertices;
        if (!(1 <= currentDeleteX_ && currentDeleteX_ <= n_)) {
            if (wscanOpt) {
#ifdef LOCAL
                noteReuseWatchFullScan(st);
                g_batch_dbg.wscan_used_preservedHandleIdxs_fastpath_calls++;
                g_batch_dbg.wscan_preserved_keepmask_scans++;
                g_batch_dbg.wscan_handles_scanned_preserved_keepmask += (long long)st.watchHandles.size();
                long long __preserved_keepmask_start_ns = local_profile_detailed_enabled() ? dbg_now_ns() : 0;
#endif
                keptPreservedVertices.reserve(st.watchHandles.size());
                for (int i = 0; i < (int)st.watchHandles.size(); ++i) {
                    const auto& h = st.watchHandles[i];
                    if (h.originKind != SupportOriginKind::PreservedPiece) continue;
                    if (1 <= h.vertex && h.vertex <= n_) keptPreservedVertices.push_back(h.vertex);
                }
#ifdef LOCAL
                if (local_profile_detailed_enabled()) {
                    acc_wscan_keepmask_ns(dbg_now_ns() - __preserved_keepmask_start_ns,
                                          &g_batch_dbg.time_wscan_preserved_keepmask_decision_ns,
                                          &g_batch_dbg.time_wscan_preserved_keepmask_decision_calls);
                }
#endif
            }
        } else {
            supportScratch_.ensure(n_);
            int keepStamp = supportScratch_.nextCollect();
#ifdef LOCAL
            long long __keepstamp_start_ns = local_profile_detailed_enabled() ? dbg_now_ns() : 0;
#endif
            for (const auto& p : newPreserved) {
                const auto* tree = getSupportTreeObject(p.treeId);
                if (!tree) continue;
                forEachPiecePos(*tree, p, [&](int pos){
                    int v = tree->vertexByPos[pos];
                    if (1 <= v && v <= n_) {
                        supportScratch_.collectStamp[v] = keepStamp;
#ifdef LOCAL
                        g_batch_dbg.wscan_preserved_keepstamp_vertices_marked++;
#endif
                    }
                });
            }
#ifdef LOCAL
            if (local_profile_detailed_enabled()) {
                acc_wscan_keepmask_ns(dbg_now_ns() - __keepstamp_start_ns,
                                      &g_batch_dbg.time_wscan_preserved_keepstamp_build_ns,
                                      &g_batch_dbg.time_wscan_preserved_keepstamp_build_calls);
            }
#endif
            bool anyRefresh = false;
            long long removedByRefresh = 0;
            if (wscanOpt) {
#ifdef LOCAL
                noteReuseWatchFullScan(st);
                g_batch_dbg.wscan_used_preservedHandleIdxs_fastpath_calls++;
                g_batch_dbg.wscan_preserved_keepmask_scans++;
                g_batch_dbg.wscan_handles_scanned_preserved_keepmask += (long long)st.watchHandles.size();
                long long __preserved_keepmask_start_ns = local_profile_detailed_enabled() ? dbg_now_ns() : 0;
#endif
                keptPreservedVertices.reserve(st.watchHandles.size());
                for (int i = 0; i < (int)st.watchHandles.size(); ++i) {
                    const auto& h = st.watchHandles[i];
                    if (h.originKind != SupportOriginKind::PreservedPiece) continue;
                    bool shouldRemove = false;
                    if (splitOldPieceIds.count(h.pieceId)) {
                        shouldRemove = true;
                        if (1 <= h.vertex && h.vertex <= n_ && topo_.aliveVertex(h.vertex) && h.vertex != currentDeleteX_ && supportScratch_.collectStamp[h.vertex] == keepStamp) {
                            shouldRemove = false;
                        }
                    }
                    if (h.vertex == currentDeleteX_) shouldRemove = true;
                    if (shouldRemove) {
                        keepMask[i] = 0;
                        if (retainOpt) preservedSparseRemovedIdxs.push_back(i);
                        anyRefresh = true;
                        ++removedByRefresh;
#ifdef LOCAL
                        g_batch_dbg.reuse_keepmask_removed_handles++;
                        g_batch_dbg.reuse_keepmask_removed_preserved_handles++;
#endif
                    } else if (1 <= h.vertex && h.vertex <= n_) {
                        keptPreservedVertices.push_back(h.vertex);
                    }
                }
#ifdef LOCAL
                if (local_profile_detailed_enabled()) {
                    acc_wscan_keepmask_ns(dbg_now_ns() - __preserved_keepmask_start_ns,
                                          &g_batch_dbg.time_wscan_preserved_keepmask_decision_ns,
                                          &g_batch_dbg.time_wscan_preserved_keepmask_decision_calls);
                }
#endif
            } else {
#ifdef LOCAL
                noteReuseWatchFullScan(st);
                g_batch_dbg.wscan_preserved_keepmask_scans++;
                g_batch_dbg.wscan_handles_scanned_preserved_keepmask += (long long)st.watchHandles.size();
                long long __preserved_keepmask_start_ns = local_profile_detailed_enabled() ? dbg_now_ns() : 0;
#endif
                for (int i = 0; i < (int)st.watchHandles.size(); ++i) {
                    const auto& h = st.watchHandles[i];
                    if (h.originKind != SupportOriginKind::PreservedPiece) continue;
                    bool shouldRemove = false;
                    if (splitOldPieceIds.count(h.pieceId)) {
                        shouldRemove = true;
                        if (1 <= h.vertex && h.vertex <= n_ && topo_.aliveVertex(h.vertex) && h.vertex != currentDeleteX_ && supportScratch_.collectStamp[h.vertex] == keepStamp) {
                            shouldRemove = false;
                        }
                    }
                    if (h.vertex == currentDeleteX_) shouldRemove = true;
                    if (shouldRemove) {
                        keepMask[i] = 0;
                        if (retainOpt) preservedSparseRemovedIdxs.push_back(i);
                        anyRefresh = true;
                        ++removedByRefresh;
#ifdef LOCAL
                        g_batch_dbg.reuse_keepmask_removed_handles++;
                        g_batch_dbg.reuse_keepmask_removed_preserved_handles++;
#endif
                    }
                }
#ifdef LOCAL
                if (local_profile_detailed_enabled()) {
                    acc_wscan_keepmask_ns(dbg_now_ns() - __preserved_keepmask_start_ns,
                                          &g_batch_dbg.time_wscan_preserved_keepmask_decision_ns,
                                          &g_batch_dbg.time_wscan_preserved_keepmask_decision_calls);
                }
#endif
            }
#ifdef LOCAL
            if (anyRefresh) {
                g_batch_dbg.debug_targeted_piece_watch_refresh_calls++;
                g_batch_dbg.debug_targeted_piece_watch_refresh_removed += removedByRefresh;
            }
#endif
        }
        vector<int> remappedOldConnectorHandleIdxs;
        if (wscanOpt) remappedOldConnectorHandleIdxs = remapRetainedHandleIndices(validOldConnectorHandleIdxs, keepMask);
#ifdef LOCAL
        {
            ScopedIntInc __wscan_ctx(&g_wscan_retain_ctx);
            retainClassWatchByKeepMask(owner, cid, st, keepMask, retainOpt ? &preservedSparseRemovedIdxs : nullptr);
        }
#else
        retainClassWatchByKeepMask(owner, cid, st, keepMask, retainOpt ? &preservedSparseRemovedIdxs : nullptr);
#endif
        clearMaterializedMetadataOnly(st);
        st.pieceModeActive = true;
        st.preservedPieces = newPreserved;
        st.attachmentVerticesByPiece = newAttachmentVertices;
        st.connectorPieces.clear();
        st.patchTreeIds.clear();
        st.connectorTreeId = -1;
        st.connectorWatchEntryIds.clear();
        st.connectorSkeletonVertices.clear();
        st.connectorVertexToPos.clear();
        st.connectorSkeletonWatchHandleByVertex.clear();
        if (!reuse_apply_opt_enabled()) {
#ifdef LOCAL
            g_batch_dbg.reuse_duplicate_preserved_annotate_passes++;
#endif
            reusePrepublishAnnotatePreserved(st, st.preservedPieces);
        }
        vector<int> terminals;
#ifdef LOCAL
        {
            ScopedNsAcc __normalize_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_connector_path_attachment_normalize_ns),
                                          ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_psplit_connector_path_attachment_normalize_calls));
            terminals = normalizedAliveAttachmentVertices(st.attachmentVerticesByPiece);
            g_batch_dbg.psplit_connector_path_attachment_normalize_calls++;
        }
#else
        terminals = normalizedAliveAttachmentVertices(st.attachmentVerticesByPiece);
#endif
        int newConnectorTreeId = -1;
        vector<int> newConnectorFullVerts;
        const SupportTreeObject* newConnectorTree = nullptr;
        if (terminals.size() > 1) {
#ifdef LOCAL
            long long __build_start_ns = local_profile_coarse_enabled() ? dbg_now_ns() : 0;
            g_batch_dbg.reuse_patch_tree_build_calls++;
            ScopedNsAcc __reuse_patch_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_patch_tree_build_ns),
                                            ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_patch_tree_build_calls));
            if (!buildConnectorForRepresentatives(owner, terminals, newConnectorTreeId, newConnectorFullVerts)) {
                g_batch_dbg.piece_materialize_fallback_calls++;
                g_batch_dbg.piece_materialize_fallback_vertices += (long long)terminals.size();
                return false;
            }
            if (local_profile_coarse_enabled()) {
                g_batch_dbg.time_connector_skeleton_build_ns += dbg_now_ns() - __build_start_ns;
                g_batch_dbg.time_connector_skeleton_build_calls++;
            }
#else
            if (!buildConnectorForRepresentatives(owner, terminals, newConnectorTreeId, newConnectorFullVerts)) {
                return false;
            }
#endif
#ifdef LOCAL
            g_batch_dbg.reuse_patch_vertices += (long long)newConnectorFullVerts.size();
#endif
            newConnectorTree = getSupportTreeObject(newConnectorTreeId);
            if (!newConnectorTree) return false;
        }

        supportScratch_.ensure(n_);
        int preservedStamp = supportScratch_.nextCollect();
        if (wscanOpt) {
#ifdef LOCAL
            g_batch_dbg.wscan_handles_scanned_preserved_stamp_mark += (long long)keptPreservedVertices.size();
            long long __preserved_stamp_start_ns = local_profile_detailed_enabled() ? dbg_now_ns() : 0;
#endif
            for (int v : keptPreservedVertices) {
                if (1 <= v && v <= n_) {
                    supportScratch_.collectStamp[v] = preservedStamp;
#ifdef LOCAL
                    g_batch_dbg.wscan_preserved_stamp_vertices_marked++;
#endif
                }
            }
#ifdef LOCAL
            if (local_profile_detailed_enabled()) {
                acc_wscan_keepmask_ns(dbg_now_ns() - __preserved_stamp_start_ns,
                                      &g_batch_dbg.time_wscan_preserved_stamp_mark_ns,
                                      &g_batch_dbg.time_wscan_preserved_stamp_mark_calls);
            }
#endif
        } else {
#ifdef LOCAL
            noteReuseWatchFullScan(st);
            g_batch_dbg.wscan_handles_scanned_preserved_stamp_mark += (long long)st.watchHandles.size();
            long long __preserved_stamp_start_ns = local_profile_detailed_enabled() ? dbg_now_ns() : 0;
#endif
            for (const auto& h : st.watchHandles) {
                if (h.originKind != SupportOriginKind::PreservedPiece) continue;
                if (!(1 <= h.vertex && h.vertex <= n_)) continue;
                supportScratch_.collectStamp[h.vertex] = preservedStamp;
#ifdef LOCAL
                g_batch_dbg.wscan_preserved_stamp_vertices_marked++;
#endif
            }
#ifdef LOCAL
            if (local_profile_detailed_enabled()) {
                acc_wscan_keepmask_ns(dbg_now_ns() - __preserved_stamp_start_ns,
                                      &g_batch_dbg.time_wscan_preserved_stamp_mark_ns,
                                      &g_batch_dbg.time_wscan_preserved_stamp_mark_calls);
            }
#endif
        }
        vector<int> desiredConnectorWatchVerts;
        desiredConnectorWatchVerts.reserve(newConnectorFullVerts.size());
#ifdef LOCAL
        long long __desired_set_start_ns = local_profile_detailed_enabled() ? dbg_now_ns() : 0;
#endif
        for (int v : newConnectorFullVerts) {
            if (!(1 <= v && v <= n_)) continue;
            if (supportScratch_.collectStamp[v] == preservedStamp) continue;
            desiredConnectorWatchVerts.push_back(v);
        }
        unordered_set<int> desiredConnectorSet;
        desiredConnectorSet.reserve(desiredConnectorWatchVerts.size() * 2 + 1);
        for (int v : desiredConnectorWatchVerts) desiredConnectorSet.insert(v);
#ifdef LOCAL
        g_batch_dbg.wscan_desired_connector_vertices += (long long)desiredConnectorWatchVerts.size();
        if (local_profile_detailed_enabled()) {
            acc_wscan_keepmask_ns(dbg_now_ns() - __desired_set_start_ns,
                                  &g_batch_dbg.time_wscan_connector_desired_set_build_ns,
                                  &g_batch_dbg.time_wscan_connector_desired_set_build_calls);
        }
        accountConnectorSkeletonOverlapShadow(oldConnectorFullVerts, newConnectorFullVerts, oldConnectorWatchVerts, desiredConnectorWatchVerts);
#endif

        int newTreePosStamp = 0;
        if (newConnectorTree) {
#ifdef LOCAL
            ScopedNsAcc __vx_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_vertex_lookup_ns),
                                   ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_vertex_lookup_calls));
#endif
            buildTreeVertexPosMap(*newConnectorTree, newTreePosStamp);
        }
#ifdef LOCAL
        ScopedNsAcc __watchdiff_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_watch_diff_build_ns),
                                      ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_watch_diff_build_calls));
#endif

        vector<char> keepConnectorMask(st.watchHandles.size(), 1);
        vector<int> connectorSparseRemovedIdxs;
        if (retainOpt) connectorSparseRemovedIdxs.reserve(remappedOldConnectorHandleIdxs.size() + 8);
        long long removedOldConnectorVertices = 0;
        long long reusedConnectorVertices = 0;
        vector<int> keptConnectorVertices;
#ifdef LOCAL
        if (wscanOpt && !remappedOldConnectorHandleIdxs.empty()) g_batch_dbg.wscan_used_connectorWatchEntryIds_fastpath_calls++;
        g_batch_dbg.wscan_connector_keepmask_scans++;
        long long __connector_keepmask_start_ns = local_profile_detailed_enabled() ? dbg_now_ns() : 0;
        ScopedNsAcc __connector_retag_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_connector_direct_retag_ns),
                                            ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_connector_direct_retag_calls));
#endif
        if (wscanOpt) {
#ifdef LOCAL
            g_batch_dbg.wscan_handles_scanned_connector_keepmask += (long long)remappedOldConnectorHandleIdxs.size();
#endif
            keptConnectorVertices.reserve(remappedOldConnectorHandleIdxs.size());
            for (int hi : remappedOldConnectorHandleIdxs) {
                if (!(0 <= hi && hi < (int)st.watchHandles.size())) continue;
                auto& h = st.watchHandles[hi];
                if (h.originKind != SupportOriginKind::ConnectorTree) continue;
                bool keep = (1 <= h.vertex && h.vertex <= n_ && desiredConnectorSet.count(h.vertex));
                if (keep && newConnectorTree && supportScratch_.supportPosStamp[h.vertex] == newTreePosStamp) {
                    int pos = supportScratch_.supportPosVal[h.vertex];
                    annotateHandleMetadata(h, SupportOriginKind::ConnectorTree, newConnectorTreeId, -1, pos);
                    ++reusedConnectorVertices;
                    keptConnectorVertices.push_back(h.vertex);
#ifdef LOCAL
                    g_batch_dbg.reuse_connector_direct_retag_handles++;
#endif
                } else {
                    keepConnectorMask[hi] = 0;
                    if (retainOpt) connectorSparseRemovedIdxs.push_back(hi);
                    ++removedOldConnectorVertices;
#ifdef LOCAL
                    g_batch_dbg.reuse_keepmask_removed_handles++;
                    g_batch_dbg.reuse_keepmask_removed_connector_handles++;
#endif
                }
            }
        } else {
#ifdef LOCAL
            noteReuseWatchFullScan(st);
            g_batch_dbg.wscan_handles_scanned_connector_keepmask += (long long)st.watchHandles.size();
#endif
            for (int i = 0; i < (int)st.watchHandles.size(); ++i) {
                auto& h = st.watchHandles[i];
                if (h.originKind != SupportOriginKind::ConnectorTree) continue;
                bool keep = (1 <= h.vertex && h.vertex <= n_ && desiredConnectorSet.count(h.vertex));
                if (keep && newConnectorTree && supportScratch_.supportPosStamp[h.vertex] == newTreePosStamp) {
                    int pos = supportScratch_.supportPosVal[h.vertex];
                    annotateHandleMetadata(h, SupportOriginKind::ConnectorTree, newConnectorTreeId, -1, pos);
                    ++reusedConnectorVertices;
                    keptConnectorVertices.push_back(h.vertex);
#ifdef LOCAL
                    g_batch_dbg.reuse_connector_direct_retag_handles++;
#endif
                } else {
                    keepConnectorMask[i] = 0;
                    if (retainOpt) connectorSparseRemovedIdxs.push_back(i);
                    ++removedOldConnectorVertices;
#ifdef LOCAL
                    g_batch_dbg.reuse_keepmask_removed_handles++;
                    g_batch_dbg.reuse_keepmask_removed_connector_handles++;
#endif
                }
            }
        }
#ifdef LOCAL
        if (local_profile_detailed_enabled()) {
            acc_wscan_keepmask_ns(dbg_now_ns() - __connector_keepmask_start_ns,
                                  &g_batch_dbg.time_wscan_connector_keepmask_decision_ns,
                                  &g_batch_dbg.time_wscan_connector_keepmask_decision_calls);
        }
        {
            ScopedIntInc __ctxA(&g_connector_skeleton_unregister_ctx);
            ScopedIntInc __ctxB(&g_wscan_retain_ctx);
            retainClassWatchByKeepMask(owner, cid, st, keepConnectorMask, retainOpt ? &connectorSparseRemovedIdxs : nullptr);
        }
#else
        retainClassWatchByKeepMask(owner, cid, st, keepConnectorMask, retainOpt ? &connectorSparseRemovedIdxs : nullptr);
#endif

        unordered_set<int> existingConnectorVertexSet;
#ifdef LOCAL
        long long __existing_set_start_ns = local_profile_detailed_enabled() ? dbg_now_ns() : 0;
#endif
        if (wscanOpt) {
            existingConnectorVertexSet.reserve(keptConnectorVertices.size() * 2 + 1);
#ifdef LOCAL
            g_batch_dbg.wscan_handles_scanned_existing_connector_set += (long long)keptConnectorVertices.size();
#endif
            for (int v : keptConnectorVertices) {
                if (1 <= v && v <= n_) existingConnectorVertexSet.insert(v);
            }
        } else {
            existingConnectorVertexSet.reserve(st.watchHandles.size() * 2 + 1);
#ifdef LOCAL
            noteReuseWatchFullScan(st);
            g_batch_dbg.wscan_handles_scanned_existing_connector_set += (long long)st.watchHandles.size();
#endif
            for (int i = 0; i < (int)st.watchHandles.size(); ++i) {
                const auto& h = st.watchHandles[i];
                if (h.originKind != SupportOriginKind::ConnectorTree) continue;
                if (1 <= h.vertex && h.vertex <= n_) existingConnectorVertexSet.insert(h.vertex);
            }
        }
#ifdef LOCAL
        g_batch_dbg.wscan_existing_connector_vertices += (long long)existingConnectorVertexSet.size();
        if (local_profile_detailed_enabled()) {
            acc_wscan_keepmask_ns(dbg_now_ns() - __existing_set_start_ns,
                                  &g_batch_dbg.time_wscan_connector_existing_set_build_ns,
                                  &g_batch_dbg.time_wscan_connector_existing_set_build_calls);
        }
#endif
        vector<int> addVerts;
        addVerts.reserve(desiredConnectorWatchVerts.size());
#ifdef LOCAL
        long long __addverts_diff_start_ns = local_profile_detailed_enabled() ? dbg_now_ns() : 0;
        g_batch_dbg.wscan_addverts_candidates += (long long)desiredConnectorWatchVerts.size();
#endif
        for (int v : desiredConnectorWatchVerts) {
            if (!existingConnectorVertexSet.count(v)) addVerts.push_back(v);
        }
#ifdef LOCAL
        g_batch_dbg.wscan_addverts_selected += (long long)addVerts.size();
        if (local_profile_detailed_enabled()) {
            acc_wscan_keepmask_ns(dbg_now_ns() - __addverts_diff_start_ns,
                                  &g_batch_dbg.time_wscan_connector_addverts_diff_ns,
                                  &g_batch_dbg.time_wscan_connector_addverts_diff_calls);
        }
#endif
        vector<int> connectorHandleIdxs;
        if (!addVerts.empty()) {
#ifdef LOCAL
            {
                ScopedIntInc __ctx(&g_connector_skeleton_register_ctx);
                appendClassWatchEntries(owner, cid, st, addVerts, &connectorHandleIdxs);
            }
            g_batch_dbg.reuse_patch_handles_added += (long long)connectorHandleIdxs.size();
#else
            appendClassWatchEntries(owner, cid, st, addVerts, &connectorHandleIdxs);
#endif
            if (newConnectorTree) {
                if (newTreePosStamp == 0) buildTreeVertexPosMap(*newConnectorTree, newTreePosStamp);
#ifdef LOCAL
                ScopedNsAcc __connector_add_retag_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_connector_direct_retag_ns),
                                                        nullptr);
#endif
                for (int hi : connectorHandleIdxs) {
                    if (!(0 <= hi && hi < (int)st.watchHandles.size())) continue;
                    auto& h = st.watchHandles[hi];
                    int pos = -1;
                    if (1 <= h.vertex && h.vertex <= n_ && supportScratch_.supportPosStamp[h.vertex] == newTreePosStamp) pos = supportScratch_.supportPosVal[h.vertex];
                    annotateHandleMetadata(h, SupportOriginKind::ConnectorTree, newConnectorTreeId, -1, pos);
#ifdef LOCAL
                    g_batch_dbg.reuse_connector_direct_retag_handles++;
#endif
                }
            }
        }

#ifdef LOCAL
        ScopedNsAcc __state_publish_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_state_publish_ns),
                                          ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_state_publish_calls));
        ScopedNsAcc __reuse_publish_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_final_publish_commit_ns),
                                          ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_final_publish_commit_calls));
        g_batch_dbg.reuse_final_publish_calls++;
#endif
        StatePublishContext __publish_ctx;
        ScopedStatePublishContext __publish_scope(this, &__publish_ctx);
        st.connectorTreeId = newConnectorTreeId;
        if (newConnectorTreeId > 0) {
            st.connectorPieces.push_back(makeWholeTreePieceRef(newConnectorTreeId, -1, -1, 0));
            st.patchTreeIds.push_back(newConnectorTreeId);
        }
        if (reuse_apply_opt_enabled()) dispatchPublishAnnotatePreserved(st, st.preservedPieces);
        dispatchPublishRebuildConnectorWatchEntryIds(st);
        dispatchPublishAnnotateConnectorPieces(st, st.connectorPieces);
        dispatchPublishRebuildCanonicalState(st);
        debugCheckNoDeletedVertexInCanonicalState(owner, cid, st);
#ifdef LOCAL
        g_batch_dbg.connector_skeleton_build_calls++;
        g_batch_dbg.connector_skeleton_terminals += (long long)terminals.size();
        g_batch_dbg.connector_skeleton_vertices += (long long)desiredConnectorWatchVerts.size();
        g_batch_dbg.connector_skeleton_selected_classes++;
        switch (currentDeltaToggleMode()) {
            case DeltaToggleMode::ConnectorOnly: g_batch_dbg.connector_skeleton_selected_connector_only++; break;
            case DeltaToggleMode::BothOn: g_batch_dbg.connector_skeleton_selected_both_on++; break;
            default: break;
        }
        if (forced) {
            g_batch_dbg.connector_skeleton_forced_classes++;
            g_batch_dbg.debug_force_skeleton_calls++;
        }
        g_batch_dbg.connector_skeleton_actual_calls++;
        g_batch_dbg.connector_skeleton_actual_terminals += (long long)terminals.size();
        g_batch_dbg.connector_skeleton_actual_vertices += (long long)desiredConnectorWatchVerts.size();
        g_batch_dbg.connector_skeleton_actual_removed_old_connector_vertices += removedOldConnectorVertices;
        g_batch_dbg.connector_skeleton_actual_retargets += retargets;
        g_batch_dbg.connector_watch_diff_actual_calls++;
        g_batch_dbg.connector_watch_diff_actual_reused += reusedConnectorVertices;
        g_batch_dbg.connector_watch_diff_actual_removed += removedOldConnectorVertices;
        g_batch_dbg.connector_watch_diff_actual_added += (long long)addVerts.size();
#endif
        saveDeleteWatchSnapshotNew(owner, cid, st);
        return true;
    }

    bool applyPieceNativeReuseForClass(int owner, int cid,
                                      const TouchedClassInfo& info,
                                      const CoverageCollapseResult& refineRes) {
        if (!(1 <= owner && owner <= n_) || cid < 0) return false;
        if (!refineRes.pieceNativePlanned || refineRes.usedFullScan || refineRes.reuseKind == SupportReuseKind::None) return false;
        auto& od = ownerData_[owner];
        auto itState = od.classStates.find(cid);
        if (itState == od.classStates.end()) return false;
        auto& st = itState->second;
        if (!st.pieceModeActive) return false;
        ensureConnectorPiecesMigrated(st);
#ifdef LOCAL
        if (refineRes.hitOldPieceId >= 0) g_batch_dbg.reuse_old_piece_hits++;
        if (!info.connectorHits.empty()) g_batch_dbg.reuse_old_connector_hits++;
        g_batch_dbg.reuse_replacement_pieces += (long long)refineRes.replacementPieces.size();
        g_batch_dbg.dispatch_candidate_cids += (long long)refineRes.candidateCids.size();
#endif
#ifdef LOCAL
        g_batch_dbg.debug_reference_compare_calls++;
        switch (currentDeltaToggleMode()) {
            case DeltaToggleMode::BothOff: g_batch_dbg.debug_reference_compare_both_calls++; break;
            case DeltaToggleMode::PreservedOnly: g_batch_dbg.debug_reference_compare_preserved_only_calls++; break;
            case DeltaToggleMode::ConnectorOnly: g_batch_dbg.debug_reference_compare_connector_only_calls++; break;
            case DeltaToggleMode::BothOn: g_batch_dbg.debug_reference_compare_both_calls++; break;
        }
#endif
        bool repUnanimousCandidate = (refineRes.reuseKind == SupportReuseKind::RepUnanimous && st.pieceModeActive && !st.supportMetaValid && refineRes.pieceNativePlanned);
        bool canDeltaPreserved = (refineRes.reuseKind == SupportReuseKind::RepUnanimous && info.connectorHits.empty() && info.pieceHits.size() == 1);
        bool canConnectorSkeleton = (repUnanimousCandidate && (!info.connectorHits.empty() || !info.pieceHits.empty()));
#ifdef LOCAL
        bool forceSkeleton = connectorSkeletonForceEnabled();
        if (canDeltaPreserved && !deltaPreservedHitEnabled()) canDeltaPreserved = false;
        if (canConnectorSkeleton && !deltaConnectorHitEnabled() && !forceSkeleton) canConnectorSkeleton = false;
#else
        const bool forceSkeleton = false;
#endif
#ifdef LOCAL
        if (refineRes.reuseKind == SupportReuseKind::RepUnanimous) {
            g_batch_dbg.connector_skeleton_candidate_classes++;
            bool attachmentsReady = !st.attachmentVerticesByPiece.empty();
            if (!attachmentsReady) syncAttachmentVerticesByPiece(st);
            attachmentsReady = !st.attachmentVerticesByPiece.empty();
            bool hasHits = forceSkeleton || !info.connectorHits.empty() || !info.pieceHits.empty();
            if (!st.pieceModeActive) g_batch_dbg.connector_skeleton_reject_state_not_unanimous++;
            else if (st.preservedPieces.empty()) g_batch_dbg.connector_skeleton_reject_no_preserved_pieces++;
            else if (!attachmentsReady) g_batch_dbg.connector_skeleton_reject_no_attachment_vertices++;
            else if (st.supportMetaValid) g_batch_dbg.connector_skeleton_reject_support_meta_valid++;
            else if (!hasHits) g_batch_dbg.connector_skeleton_reject_origin_kind++;
        }
#endif
        if (!(canDeltaPreserved || canConnectorSkeleton)) {
            if (refineRes.reuseKind == SupportReuseKind::RepUnanimous) {
#ifdef LOCAL
                g_batch_dbg.unanimous_baseline_path_calls++;
                g_batch_dbg.unanimous_baseline_path_vertices += st.watchVertexCount;
                g_batch_dbg.debug_unanimous_state_old_field_read++;
#endif
            }
#ifdef LOCAL
            g_batch_dbg.reuse_route_baseline_calls++;
            ScopedNsAcc __route_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_route_baseline_ns), nullptr);
#endif
#ifdef LOCAL
            ScopedWScanRouteContext __wscan_route(REUSE_ROUTE_BASELINE);
#endif
            return applyPieceNativeReuseForClassBaseline(owner, cid, info, refineRes);
        }
        // Preserved hit remains logically correct via the baseline preserved update.
        // After that, immediately normalize unanimous state through connector skeleton rebuild.
        if (canDeltaPreserved) {
#ifdef LOCAL
            g_batch_dbg.debug_unanimous_state_old_field_read++;
#endif
#ifdef LOCAL
            g_batch_dbg.reuse_route_delta_preserved_then_skeleton_calls++;
            ScopedNsAcc __route_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_route_delta_preserved_then_skeleton_ns), nullptr);
#endif
#ifdef LOCAL
            ScopedWScanRouteContext __wscan_route(REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON);
#endif
            bool ok = applyPieceNativeReuseForClassBaseline(owner, cid, info, refineRes);
            if (!ok) return false;
            auto itState2 = ownerData_[owner].classStates.find(cid);
            if (itState2 == ownerData_[owner].classStates.end()) return false;
            if (!itState2->second.pieceModeActive) return true;
            return applyConnectorSkeletonRebuildForClass(owner, cid, info, refineRes, true);
        }
        if (canConnectorSkeleton) {
#ifdef LOCAL
            g_batch_dbg.reuse_route_connector_skeleton_calls++;
            ScopedNsAcc __route_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_route_connector_skeleton_ns), nullptr);
#endif
#ifdef LOCAL
            ScopedWScanRouteContext __wscan_route(REUSE_ROUTE_CONNECTOR_SKELETON);
#endif
            return applyConnectorSkeletonRebuildForClass(owner, cid, info, refineRes, forceSkeleton);
        }
#ifdef LOCAL
        g_batch_dbg.reuse_route_general_delta_calls++;
        ScopedNsAcc __route_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_route_general_delta_ns), nullptr);
#endif
#ifdef LOCAL
        ScopedWScanRouteContext __wscan_route(REUSE_ROUTE_GENERAL_DELTA);
#endif
        if (!topo_.aliveVertex(owner) || st.activeQueryCount <= 0) {
            unregisterClassWatch(owner, cid, st);
            if (++st.epoch == INT_MAX) st.epoch = 1;
            st.endpointPool.clear();
            return true;
        }
        vector<int> relevantIdxs = collectRelevantEndpointIdxs(owner, cid, true);
        if (relevantIdxs.empty() && st.activeQueryCount > 0) relevantIdxs = fallbackRelevantEndpointIdxsFromQueries(owner, cid);
        if (relevantIdxs.empty()) {
            unregisterClassWatch(owner, cid, st);
            if (++st.epoch == INT_MAX) st.epoch = 1;
            st.activeQueryCount = 0;
            st.endpointPool.clear();
            return true;
        }

        vector<char> keepMask(st.watchHandles.size(), 1);
        long long removedVertices = 0;

        unordered_map<int,int> oldAttachmentByPiece;
        for (size_t i = 0; i < st.preservedPieces.size() && i < st.attachmentVerticesByPiece.size(); ++i) {
            oldAttachmentByPiece[st.preservedPieces[i].pieceId] = st.attachmentVerticesByPiece[i];
        }

        auto removePreservedHandlesOfPieceByReplacement = [&](int oldPieceId,
                                                              const vector<SupportPieceRef>& replacementPieces,
                                                              unordered_map<int, tuple<int,int,int>>& vertexToMeta) {
            vertexToMeta.clear();
            for (const auto& piece : replacementPieces) {
                const auto* tree = getSupportTreeObject(piece.treeId);
                if (!tree) return false;
                forEachPiecePos(*tree, piece, [&](int pos){
                    int v = tree->vertexByPos[pos];
                    vertexToMeta[v] = make_tuple(piece.treeId, piece.pieceId, pos);
                });
            }
            for (int i = 0; i < (int)st.watchHandles.size(); ++i) {
                const auto& h = st.watchHandles[i];
                if (h.originKind != SupportOriginKind::PreservedPiece || h.pieceId != oldPieceId) continue;
                if (!vertexToMeta.count(h.vertex)) {
                    keepMask[i] = 0;
                    ++removedVertices;
                }
            }
            return true;
        };

        auto removeConnectorHandlesOfPieceByReplacement = [&](int oldPieceId,
                                                              const vector<SupportPieceRef>& replacementPieces,
                                                              unordered_map<int, tuple<int,int,int>>& vertexToMeta) {
            vertexToMeta.clear();
            for (const auto& piece : replacementPieces) {
                const auto* tree = getSupportTreeObject(piece.treeId);
                if (!tree) return false;
                forEachPiecePos(*tree, piece, [&](int pos){
                    int v = tree->vertexByPos[pos];
                    vertexToMeta[v] = make_tuple(piece.treeId, piece.pieceId, pos);
                });
            }
            for (int i = 0; i < (int)st.watchHandles.size(); ++i) {
                const auto& h = st.watchHandles[i];
                if (h.originKind != SupportOriginKind::ConnectorTree || h.pieceId != oldPieceId) continue;
                if (!vertexToMeta.count(h.vertex)) {
                    keepMask[i] = 0;
                    ++removedVertices;
                }
            }
            return true;
        };

        vector<SupportPieceRef> newPreserved;
        vector<int> newAttachmentVertices;
        newPreserved.reserve(st.preservedPieces.size() + refineRes.replacementPieces.size());
        newAttachmentVertices.reserve(st.preservedPieces.size() + refineRes.replacementPieces.size());
        if (refineRes.hitOldPieceId >= 0) {
            int oldAttachment = attachmentVertexOfPiece(st, refineRes.hitOldPieceId);
            for (const auto& p : st.preservedPieces) {
                if (!p.pieceAlive) continue;
                if (p.pieceId == refineRes.hitOldPieceId) continue;
                newPreserved.push_back(p);
                newAttachmentVertices.push_back(attachmentVertexOfPiece(st, p.pieceId));
            }
            int attachmentAssigned = 0;
            const auto* tree0 = refineRes.replacementPieces.empty() ? nullptr : getSupportTreeObject(refineRes.replacementPieces[0].treeId);
            for (const auto& p : refineRes.replacementPieces) {
                if (!p.pieceAlive || p.pieceEndpointCount <= 0) continue;
                int att = p.pieceRepresentativeEndpoint;
                if (!attachmentAssigned && 1 <= oldAttachment && oldAttachment <= n_) {
                    if (pieceContainsVertex(p, oldAttachment)) {
                        att = oldAttachment;
                        attachmentAssigned = 1;
                    }
                }
                newPreserved.push_back(p);
                newAttachmentVertices.push_back(att);
            }
        } else {
            for (const auto& p : st.preservedPieces) if (p.pieceAlive && p.pieceEndpointCount > 0) newPreserved.push_back(p);
            newAttachmentVertices = st.attachmentVerticesByPiece;
            if ((int)newAttachmentVertices.size() > (int)newPreserved.size()) newAttachmentVertices.resize(newPreserved.size());
            while ((int)newAttachmentVertices.size() < (int)newPreserved.size()) newAttachmentVertices.push_back(-1);
        }

        if (refineRes.hitOldPieceId >= 0) {
            for (int i = 0; i < (int)newPreserved.size(); ++i) {
                int att = (i < (int)newAttachmentVertices.size() ? newAttachmentVertices[i] : -1);
                const auto* ptree = getSupportTreeObject(newPreserved[i].treeId);
                bool ok = false;
                if (ptree && 1 <= att && att <= n_ && topo_.aliveVertex(att)) {
                    int pstamp = 0;
                    buildTreeVertexPosMap(*ptree, pstamp);
                    if (supportScratch_.supportPosStamp[att] == pstamp) {
                        int pos = supportScratch_.supportPosVal[att];
                        ok = pieceContainsPos(*ptree, newPreserved[i], pos);
                    }
                }
                if (!ok) {
                    int newAtt = ptree ? retargetAttachmentIfDead(*ptree, newPreserved[i]) : -1;
                    if (1 <= newAtt && newAtt <= n_) newAttachmentVertices[i] = newAtt;
                }
            }
        }

        vector<SupportPieceRef> newConnectorPieces;
        vector<int> newPatchTreeIds;
        vector<int> newConnectorHandleIdxs;
        long long addedPatchVertices = 0;
        long long removedConnectorVertices = 0;
        long long reusedConnectorVertices = 0;
        long long attachmentRetargets = 0;
        long long terminalFragmentGroups = 0;
        bool noPatchNeeded = false;

        if (refineRes.reuseKind == SupportReuseKind::RepUnanimous) {
            unordered_map<int, tuple<int,int,int>> replMeta;
            if (!info.pieceHits.empty()) {
                if (!removePreservedHandlesOfPieceByReplacement(refineRes.hitOldPieceId, refineRes.replacementPieces, replMeta)) return false;
            }

            if (!info.connectorHits.empty() && info.connectorPieceId >= 0) {
                vector<int> terminals = newAttachmentVertices;
                sort(terminals.begin(), terminals.end());
                terminals.erase(remove_if(terminals.begin(), terminals.end(), [&](int v){ return !(1 <= v && v <= n_ && topo_.aliveVertex(v)); }), terminals.end());
                terminals.erase(unique(terminals.begin(), terminals.end()), terminals.end());
                vector<SupportPieceRef> splitFrags;
                int localRemoved = 0, localGroups = 0;
                SupportPieceRef hitPiece;
                bool foundHit = false;
                for (const auto& p : st.connectorPieces) if (p.pieceAlive && p.pieceId == info.connectorPieceId) { hitPiece = p; foundHit = true; break; }
                if (!foundHit) {
#ifdef LOCAL
                    g_batch_dbg.piece_materialize_fallback_calls++;
                    g_batch_dbg.piece_fallback_reason_bad_x_handle++;
#endif
                    return false;
                }
                if (!splitConnectorPieceAtLocalPos(hitPiece, info.connectorLocalPos, terminals, splitFrags, localRemoved, localGroups)) {
#ifdef LOCAL
                    g_batch_dbg.piece_materialize_fallback_calls++;
                    g_batch_dbg.piece_fallback_reason_connector_hit++;
#endif
                    return false;
                }
                if (!removeConnectorHandlesOfPieceByReplacement(info.connectorPieceId, splitFrags, replMeta)) return false;
                removedConnectorVertices += localRemoved;
                terminalFragmentGroups += localGroups;
                for (const auto& p : st.connectorPieces) if (p.pieceAlive && p.pieceId != info.connectorPieceId) newConnectorPieces.push_back(p);
                for (const auto& p : splitFrags) if (p.pieceAlive) newConnectorPieces.push_back(p);
            } else {
                newConnectorPieces = st.connectorPieces;
            }

#ifdef LOCAL
            {
                ScopedIntInc __wscan_ctx(&g_wscan_retain_ctx);
                retainClassWatchByKeepMask(owner, cid, st, keepMask);
            }
#else
            retainClassWatchByKeepMask(owner, cid, st, keepMask);
#endif

            if (refineRes.hitOldPieceId >= 0) {
                unordered_map<int, tuple<int,int,int>> replMeta2;
                for (const auto& p : refineRes.replacementPieces) {
                    const auto* tree = getSupportTreeObject(p.treeId);
                    if (!tree) return false;
                    forEachPiecePos(*tree, p, [&](int pos){ replMeta2[tree->vertexByPos[pos]] = make_tuple(p.treeId, p.pieceId, pos); });
                }
                for (auto& h : st.watchHandles) {
                    if (h.originKind != SupportOriginKind::PreservedPiece || h.pieceId != refineRes.hitOldPieceId) continue;
                    auto it = replMeta2.find(h.vertex);
                    if (it == replMeta2.end()) continue;
                    annotateHandleMetadata(h, SupportOriginKind::PreservedPiece, get<0>(it->second), get<1>(it->second), get<2>(it->second));
                }
            }
            if (!info.connectorHits.empty() && info.connectorPieceId >= 0) {
                unordered_map<int, tuple<int,int,int>> replMeta3;
                for (const auto& p : newConnectorPieces) {
                    if (p.pieceId == info.connectorPieceId) continue;
                }
                for (const auto& p : newConnectorPieces) {
                    if (!(p.pieceAlive && p.treeId > 0)) continue;
                    const auto* tree = getSupportTreeObject(p.treeId);
                    if (!tree) return false;
                    forEachPiecePos(*tree, p, [&](int pos){
                        int v = tree->vertexByPos[pos];
                        replMeta3[v] = make_tuple(p.treeId, p.pieceId, pos);
                    });
                }
                for (auto& h : st.watchHandles) {
                    if (h.originKind != SupportOriginKind::ConnectorTree) continue;
                    auto it = replMeta3.find(h.vertex);
                    if (it == replMeta3.end()) continue;
                    annotateHandleMetadata(h, SupportOriginKind::ConnectorTree, get<0>(it->second), get<1>(it->second), get<2>(it->second));
                }
            }

            vector<int> liveTerminals = normalizedAliveAttachmentVertices(newAttachmentVertices);
            vector<int> connectorReps;
            vector<int> connectorPieceRepByIdx;
            connectorPieceRepByIdx.reserve(newConnectorPieces.size());
            for (const auto& cp : newConnectorPieces) {
                int rep = representativeTerminalInConnectorPieceFromAttachments(cp, liveTerminals);
                if (rep != -1) {
                    connectorReps.push_back(rep);
                    connectorPieceRepByIdx.push_back(rep);
                    reusedConnectorVertices += countConnectorWatchHandlesInPiece(st, cp);
                }
            }
            sort(connectorReps.begin(), connectorReps.end());
            connectorReps.erase(unique(connectorReps.begin(), connectorReps.end()), connectorReps.end());

            vector<int> patchVertices = connectorReps;
            if (info.connectorHits.empty()) {
                int anchor = connectorReps.empty() ? -1 : connectorReps[0];
                for (size_t i = 0; i < newPreserved.size(); ++i) {
                    int att = (i < newAttachmentVertices.size() ? newAttachmentVertices[i] : -1);
                    if (!(1 <= att && att <= n_ && topo_.aliveVertex(att))) continue;
                    bool covered = false;
                    for (const auto& cp : newConnectorPieces) if (pieceContainsVertex(cp, att)) { covered = true; break; }
                    if (!covered) {
                        if (anchor != -1) patchVertices.push_back(anchor);
                        patchVertices.push_back(att);
                    }
                }
            } else {
                for (size_t i = 0; i < newPreserved.size(); ++i) {
                    int att = (i < newAttachmentVertices.size() ? newAttachmentVertices[i] : -1);
                    if (!(1 <= att && att <= n_ && topo_.aliveVertex(att))) continue;
                    bool covered = false;
                    for (const auto& cp : newConnectorPieces) if (pieceContainsVertex(cp, att)) { covered = true; break; }
                    if (!covered) patchVertices.push_back(att);
                }
            }
            sort(patchVertices.begin(), patchVertices.end());
            patchVertices.erase(unique(patchVertices.begin(), patchVertices.end()), patchVertices.end());
            if (patchVertices.size() <= 1) noPatchNeeded = true;
            int patchTreeId = -1;
            vector<int> patchOnlyVerts, patchHandleIdxs;
            if (!buildPatchTreeForVertices(owner, cid, patchVertices, st, patchTreeId, patchOnlyVerts, &patchHandleIdxs)) return false;
            addedPatchVertices += (long long)patchOnlyVerts.size();
            if (patchTreeId > 0) {
                newConnectorPieces.push_back(makeWholeTreePieceRef(patchTreeId, -1, -1, 0));
                newPatchTreeIds.push_back(patchTreeId);
            }
            for (int i = 0; i < (int)newPreserved.size(); ++i) {
                int att = (i < (int)newAttachmentVertices.size() ? newAttachmentVertices[i] : -1);
                bool covered = false;
                if (1 <= att && att <= n_ && topo_.aliveVertex(att)) {
                    for (const auto& cp : newConnectorPieces) if (pieceContainsVertex(cp, att)) { covered = true; break; }
                }
                if (!covered) {
                    int newAtt = newPreserved[i].pieceRepresentativeEndpoint;
                    if (1 <= newAtt && newAtt <= n_ && newAtt != att) {
                        newAttachmentVertices[i] = newAtt;
                        ++attachmentRetargets;
                    }
                }
            }
            if (!reuse_apply_opt_enabled()) {
#ifdef LOCAL
                g_batch_dbg.reuse_duplicate_preserved_annotate_passes++;
#endif
                reusePrepublishAnnotatePreserved(st, newPreserved);
            }
            reusePrepublishConnectorMetadataRefresh(st);
            st.connectorWatchEntryIds.insert(st.connectorWatchEntryIds.end(), patchHandleIdxs.begin(), patchHandleIdxs.end());
#ifdef LOCAL
            g_batch_dbg.reuse_incremental_connector_watch_id_update_calls++;
#endif
        } else {
#ifdef LOCAL
            {
                ScopedIntInc __wscan_ctx(&g_wscan_retain_ctx);
                retainClassWatchByKeepMask(owner, cid, st, keepMask);
            }
#else
            retainClassWatchByKeepMask(owner, cid, st, keepMask);
#endif
            if (!reuse_apply_opt_enabled()) {
#ifdef LOCAL
                g_batch_dbg.reuse_duplicate_preserved_annotate_passes++;
#endif
                reusePrepublishAnnotatePreserved(st, newPreserved);
            }
        }

        clearMaterializedMetadataOnly(st);
        st.pieceModeActive = true;
        st.preservedPieces = newPreserved;
        st.attachmentVerticesByPiece = newAttachmentVertices;
        st.connectorPieces = newConnectorPieces;
        st.patchTreeIds = newPatchTreeIds;
#ifdef LOCAL
        ScopedNsAcc __state_publish_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_state_publish_ns),
                                          ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_state_publish_calls));
        ScopedNsAcc __reuse_publish_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_final_publish_commit_ns),
                                          ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_reuse_final_publish_commit_calls));
        g_batch_dbg.reuse_final_publish_calls++;
#endif
        StatePublishContext __publish_ctx;
        ScopedStatePublishContext __publish_scope(this, &__publish_ctx);
        st.connectorTreeId = -1;
        dispatchPublishAnnotatePreserved(st, st.preservedPieces);
        dispatchPublishAnnotateConnectorPieces(st, st.connectorPieces);
        dispatchPublishRebuildConnectorWatchEntryIds(st);
        st.connectorTreeId = (st.connectorPieces.size() == 1 ? st.connectorPieces[0].treeId : -1);

#ifdef LOCAL
        long long pieceWatchVertices = 0;
        for (const auto& h : st.watchHandles) if (h.originKind == SupportOriginKind::PreservedPiece) ++pieceWatchVertices;
        if (refineRes.reuseKind == SupportReuseKind::SinglePositive) {
            g_batch_dbg.piece_native_single_calls++;
            if (!info.pieceHits.empty()) g_batch_dbg.piece_native_single_preserved_hits++;
            if (!info.connectorHits.empty()) g_batch_dbg.piece_native_single_connector_hits++;
            g_batch_dbg.piece_native_single_reused_vertices += pieceWatchVertices;
            g_batch_dbg.piece_native_single_removed_vertices += removedVertices;
            g_batch_dbg.piece_native_single_boundary_ops += max<long long>(1, (long long)refineRes.replacementPieces.size());
        } else if (refineRes.reuseKind == SupportReuseKind::RepUnanimous) {
            g_batch_dbg.piece_native_unanimous_calls++;
            if (!info.pieceHits.empty()) g_batch_dbg.piece_native_unanimous_preserved_hits++;
            if (!info.connectorHits.empty()) g_batch_dbg.piece_native_unanimous_connector_hits++;
            g_batch_dbg.piece_native_unanimous_reused_vertices += pieceWatchVertices;
            g_batch_dbg.piece_native_unanimous_removed_vertices += removedVertices;
            g_batch_dbg.piece_native_unanimous_added_connector_vertices += addedPatchVertices;
            g_batch_dbg.piece_native_unanimous_boundary_ops += max<long long>(1, (long long)refineRes.replacementPieces.size());
            if (info.connectorHits.empty()) {
                g_batch_dbg.connector_delta_preserved_hit_calls++;
                g_batch_dbg.connector_delta_preserved_hit_reused_connector_vertices += reusedConnectorVertices;
                g_batch_dbg.connector_delta_preserved_hit_removed_connector_vertices += removedConnectorVertices;
                g_batch_dbg.connector_delta_preserved_hit_added_patch_vertices += addedPatchVertices;
                g_batch_dbg.connector_delta_preserved_hit_attachment_retargets += attachmentRetargets;
            } else {
                g_batch_dbg.connector_delta_connector_hit_calls++;
                g_batch_dbg.connector_delta_connector_hit_reused_vertices += reusedConnectorVertices;
                g_batch_dbg.connector_delta_connector_hit_removed_vertices += removedConnectorVertices;
                g_batch_dbg.connector_delta_connector_hit_added_patch_vertices += addedPatchVertices;
                g_batch_dbg.connector_delta_connector_hit_terminal_fragment_groups += terminalFragmentGroups;
                if (noPatchNeeded) g_batch_dbg.connector_delta_connector_hit_no_patch_needed++;
            }
        }
#endif
        return true;
    }

    bool tryReuseSupportForClass(int owner, int cid, const TouchedClassInfo& info, const CoverageCollapseResult& refineRes) {
        if (!(1 <= owner && owner <= n_) || cid < 0) return false;
        if (refineRes.usedFullScan || refineRes.reuseKind == SupportReuseKind::None) return false;
        auto& od = ownerData_[owner];
        auto itState = od.classStates.find(cid);
        if (itState == od.classStates.end()) return false;
        auto& st = itState->second;
        if (st.pieceModeActive && !st.supportMetaValid && refineRes.pieceNativePlanned) {
#ifdef LOCAL
            ScopedNsAcc __reuse_apply_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_dispatch_reuse_apply_piece_native_ns),
                                           ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_dispatch_reuse_apply_piece_native_calls));
#endif
            return applyPieceNativeReuseForClass(owner, cid, info, refineRes);
        }
        if (!st.supportMetaValid || st.supportVerts.empty() || st.supportPreorder.empty()) return false;
        if (!(0 <= refineRes.xPos && refineRes.xPos < (int)st.supportVerts.size())) return false;

        if (!topo_.aliveVertex(owner) || st.activeQueryCount <= 0) {
            unregisterClassWatch(owner, cid, st);
            if (++st.epoch == INT_MAX) st.epoch = 1;
            st.endpointPool.clear();
            return true;
        }

        vector<int> relevantIdxs = collectRelevantEndpointIdxs(owner, cid, true);
        if (relevantIdxs.empty() && st.activeQueryCount > 0) relevantIdxs = fallbackRelevantEndpointIdxsFromQueries(owner, cid);
        if (relevantIdxs.empty()) {
            unregisterClassWatch(owner, cid, st);
            if (++st.epoch == INT_MAX) st.epoch = 1;
            st.activeQueryCount = 0;
            st.endpointPool.clear();
            return true;
        }

        if (!ensureMaterializedTreeId(st)) return false;
        const int baseTreeId = st.materializedTreeId;

        if (refineRes.reuseKind == SupportReuseKind::SinglePositive) {
            if (refineRes.positiveComps.size() != 1) return false;
            vector<char> keepOld(st.supportVerts.size(), 0);
            const auto& comp = refineRes.positiveComps[0];
            if (comp.parentSide) markKeptOutsideSubtreeByPos(st, refineRes.xPos, keepOld);
            else markKeptSubtreeByPos(st, comp.childPos, keepOld);
            long long kept = 0;
            for (char b : keepOld) kept += (b != 0);
            long long removed = (long long)st.watchVertexCount - kept;
#ifdef LOCAL
            g_batch_dbg.support_reuse_single_calls++;
            g_batch_dbg.support_reuse_single_watch_vertices_kept += kept;
            g_batch_dbg.support_reuse_single_watch_vertices_removed += max<long long>(0, removed);
            accountPieceShadowCurrentVsCandidate(refineRes,
                                               kept,
                                               (long long)st.watchVertexCount,
                                               kept,
                                               kept,
                                               max<long long>(0, removed),
                                               0LL,
                                               1LL);
            g_batch_dbg.piece_reuse_single_calls++;
            g_batch_dbg.piece_reuse_single_reused_vertices += kept;
            g_batch_dbg.piece_reuse_single_removed_vertices += max<long long>(0, removed);
            g_batch_dbg.piece_live_count += 1;
            g_batch_dbg.piece_live_vertices += kept;
#endif
            SupportPieceRef piece;
            if (comp.parentSide) {
                piece = makeComplementPieceRef(baseTreeId, st.supportRootPos, refineRes.xPos,
                                               st.supportRootPos, comp.repVertex, comp.count);
            } else {
                piece = makeSubtreePieceRef(baseTreeId, comp.childPos, refineRes.xPos,
                                            comp.rootPos, comp.repVertex, comp.count);
            }
            retainClassWatchByKeepMask(owner, cid, st, keepOld);
            clearMaterializedMetadataOnly(st);
            clearPieceStateOnly(st);
            st.pieceModeActive = true;
            st.preservedPieces.push_back(piece);
            st.attachmentVerticesByPiece.clear();
            st.attachmentVerticesByPiece.push_back(piece.pieceRepresentativeEndpoint);
            st.connectorPieces.clear();
            st.patchTreeIds.clear();
            st.connectorTreeId = -1;
            st.connectorWatchEntryIds.clear();
            annotatePreservedHandlesByPieces(st, st.preservedPieces);
            return true;
        }

        if (refineRes.reuseKind == SupportReuseKind::RepUnanimous) {
#ifdef LOCAL
            ScopedNsAcc __rep_unanimous_timer(ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_dispatch_reuse_apply_rep_unanimous_ns),
                                              ptr_if(local_profile_detailed_enabled(), &g_batch_dbg.time_dispatch_reuse_apply_rep_unanimous_calls));
#endif
            if (refineRes.positiveComps.empty()) return false;
            vector<char> keepOld(st.supportVerts.size(), 0);
            vector<int> repRoots;
            repRoots.reserve(refineRes.positiveComps.size());
            vector<SupportPieceRef> newPieces;
            newPieces.reserve(refineRes.positiveComps.size());
            for (const auto& comp : refineRes.positiveComps) {
                if (comp.parentSide) {
                    markKeptOutsideSubtreeByPos(st, refineRes.xPos, keepOld);
                    newPieces.push_back(makeComplementPieceRef(baseTreeId, st.supportRootPos, refineRes.xPos,
                                                               st.supportRootPos, comp.repVertex, comp.count));
                } else {
                    markKeptSubtreeByPos(st, comp.childPos, keepOld);
                    newPieces.push_back(makeSubtreePieceRef(baseTreeId, comp.childPos, refineRes.xPos,
                                                            comp.rootPos, comp.repVertex, comp.count));
                }
                if (!(0 <= comp.rootPos && comp.rootPos < (int)st.supportVerts.size())) return false;
                repRoots.push_back(st.supportVerts[comp.rootPos]);
            }
            sort(repRoots.begin(), repRoots.end());
            repRoots.erase(unique(repRoots.begin(), repRoots.end()), repRoots.end());
            long long connWatchV = 0, connChainSteps = 0;
            SupportBuildProduct connector = buildSupportProductFromLastDeleteArtifact(owner, repRoots, &connWatchV, &connChainSteps);
            if (connector.watchVerts.empty()) return false;

            supportScratch_.ensure(n_);
            const int keepStamp = supportScratch_.nextCollect();
            long long kept = 0;
            for (int pos : st.supportPreorder) if (keepOld[pos]) {
                int v = st.supportVerts[pos];
                supportScratch_.collectStamp[v] = keepStamp;
                ++kept;
            }
            vector<int> connectorOnlyVerts;
            connectorOnlyVerts.reserve(connector.watchVerts.size());
            for (int v : connector.watchVerts) {
                if (supportScratch_.collectStamp[v] == keepStamp) continue;
                connectorOnlyVerts.push_back(v);
                supportScratch_.collectStamp[v] = keepStamp;
            }
            long long added = (long long)connectorOnlyVerts.size();
            long long removed = (long long)st.watchVertexCount - kept;
#ifdef LOCAL
            g_batch_dbg.support_reuse_unanimous_calls++;
            g_batch_dbg.support_reuse_unanimous_components += (long long)refineRes.positiveComps.size();
            g_batch_dbg.support_reuse_unanimous_reps += (long long)repRoots.size();
            g_batch_dbg.support_reuse_unanimous_connector_calls++;
            g_batch_dbg.support_reuse_unanimous_connector_vertices += (long long)connector.watchVerts.size();
            g_batch_dbg.support_reuse_unanimous_watch_vertices_kept += kept;
            g_batch_dbg.support_reuse_unanimous_watch_vertices_added += added;
            g_batch_dbg.support_reuse_unanimous_watch_vertices_removed += max<long long>(0, removed);
            accountPieceShadowCurrentVsCandidate(refineRes,
                                               kept + added,
                                               (long long)st.watchVertexCount,
                                               kept + added,
                                               kept,
                                               max<long long>(0, removed),
                                               added,
                                               (long long)refineRes.positiveComps.size() + (long long)repRoots.size());
            g_batch_dbg.piece_reuse_unanimous_calls++;
            g_batch_dbg.piece_reuse_unanimous_reused_vertices += kept;
            g_batch_dbg.piece_reuse_unanimous_removed_vertices += max<long long>(0, removed);
            g_batch_dbg.piece_reuse_unanimous_added_connector_vertices += added;
            g_batch_dbg.piece_live_count += (long long)newPieces.size();
            g_batch_dbg.piece_live_vertices += kept;
#endif
            retainClassWatchByKeepMask(owner, cid, st, keepOld);
            int connectorTreeId = storeSupportTreeObjectFromProduct(connector, &connector.nodeEndpointIdx);
            if (connectorTreeId <= 0) return false;
            vector<int> connectorHandleIdxs;
            appendClassWatchEntries(owner, cid, st, connectorOnlyVerts, &connectorHandleIdxs);
            clearMaterializedMetadataOnly(st);
            clearPieceStateOnly(st);
            st.pieceModeActive = true;
            st.preservedPieces = std::move(newPieces);
            st.attachmentVerticesByPiece.clear();
            for (const auto& p : st.preservedPieces) st.attachmentVerticesByPiece.push_back(p.pieceRepresentativeEndpoint);
            st.connectorTreeId = connectorTreeId;
            st.connectorWatchEntryIds = std::move(connectorHandleIdxs);
            st.connectorPieces.clear();
            if (connectorTreeId > 0) {
                st.connectorPieces.push_back(makeWholeTreePieceRef(connectorTreeId, -1, -1, 0));
                st.patchTreeIds.push_back(connectorTreeId);
            }
            annotatePreservedHandlesByPieces(st, st.preservedPieces);
            annotateConnectorHandlesByPieces(st, st.connectorPieces);
            return true;
        }
        return false;
    }

#ifdef LOCAL
    unordered_map<int,int> computeExactActivePartition(int owner) const {
        unordered_map<int,int> res;
        if (!(1 <= owner && owner <= n_) || !topo_.aliveVertex(owner)) return res;
        const auto& od = ownerData_[owner];
        vector<int> starts;
        starts.reserve(od.endpoints.size());
        const auto& core = topo_.core();
        for (int idx = 0; idx < (int)od.endpoints.size(); ++idx) {
            if (od.endpointActiveCount[idx] <= 0) continue;
            int ep = od.endpoints[idx];
            int eid = core.edgeIdOf(owner, ep);
            if (eid == -1 || !topo_.aliveVertex(ep) || !core.edgeAlive(eid)) continue;
            starts.push_back(ep);
        }
        unordered_map<int,int> seen;
        queue<int> qu;
        int clsCnt = 0;
        for (int s : starts) if (!seen.count(s)) {
            seen[s] = clsCnt;
            qu.push(s);
            while (!qu.empty()) {
                int u = qu.front(); qu.pop();
                for (int eid : core.incidentEdges(u)) if (core.edgeAlive(eid)) {
                    int v = core.other(eid, u);
                    if (!topo_.aliveVertex(v) || v == owner) continue;
                    if (!seen.count(v)) {
                        seen[v] = clsCnt;
                        qu.push(v);
                    }
                }
            }
            ++clsCnt;
        }
        for (int s : starts) res[s] = seen[s];
        return res;
    }

    void verifyTouchedOwnersExact(const vector<int>& touchedOwners) {
        auto canon = [](const vector<pair<int,int>>& kv) {
            unordered_map<int,int> rem;
            int nxt = 0;
            vector<pair<int,int>> out;
            out.reserve(kv.size());
            for (auto [ep, cid] : kv) {
                auto it = rem.find(cid);
                if (it == rem.end()) it = rem.emplace(cid, nxt++).first;
                out.push_back({ep, it->second});
            }
            return out;
        };
        for (int owner : touchedOwners) {
            if (!(1 <= owner && owner <= n_)) continue;
            const auto& od = ownerData_[owner];
            auto exactMap = computeExactActivePartition(owner);
            for (int qid : od.qids) {
                const auto& qs = qstate_[qid];
                bool exact = false;
                if (topo_.aliveVertex(owner)) {
                    int a = od.endpoints[qs.aIdx];
                    int b = od.endpoints[qs.bIdx];
                    auto itA = exactMap.find(a);
                    auto itB = exactMap.find(b);
                    exact = (itA != exactMap.end() && itB != exactMap.end() && itA->second == itB->second);
                }
                if ((bool)qs.active != exact) {
                    g_batch_dbg.local_active_mismatch++;
                    cerr << "LOCAL batch active mismatch owner=" << owner
                         << " qid=" << qid
                         << " stored=" << qs.active
                         << " exact=" << exact << "\n";
                    abort();
                }
            }
            vector<pair<int,int>> currentPairs, exactPairs;
            for (int idx = 0; idx < (int)od.endpoints.size(); ++idx) {
                if (od.endpointActiveCount[idx] <= 0) continue;
                int ep = od.endpoints[idx];
                auto itEx = exactMap.find(ep);
                if (itEx == exactMap.end()) continue;
                currentPairs.push_back({ep, topo_.incidentClass(owner, ep)});
                exactPairs.push_back({ep, itEx->second});
            }
            sort(currentPairs.begin(), currentPairs.end());
            sort(exactPairs.begin(), exactPairs.end());
            if (canon(currentPairs) != canon(exactPairs)) {
                g_batch_dbg.local_active_partition_mismatch++;
                cerr << "LOCAL active partition mismatch owner=" << owner << "\n";
                abort();
            }
        }
    }
#endif
public:
    void init(int n, const vector<pair<int,int>>& undirectedEdges,
              const vector<BranchQuery>& branchQueries) override {
        n_ = n;
        bq_ = branchQueries;
        topo_.init(n_, undirectedEdges, branchQueries);
        alive_.assign(n_ + 1, true);
        syncComponents();
        failing_.assign((int)bq_.size(), false);
        ownerData_.assign(n_ + 1, {});
        qstate_.assign((int)bq_.size(), {});
        watchByVertex_.assign(n_ + 1, {});
        querySeenStamp_.assign((int)bq_.size(), 0);
        querySeenCur_ = 1;
        currentSupportWatch_ = 0;
        activeQueryTotal_ = 0;
#ifdef LOCAL
        g_batch_dbg = BatchPivotDebugStats();
        reset_slow_deletion_profiles();
#endif

        for (int qid = 0; qid < (int)bq_.size(); ++qid) {
            const auto& q = bq_[qid];
            auto& od = ownerData_[q.owner];
            int aIdx = od.ensureEndpoint(q.a);
            int bIdx = od.ensureEndpoint(q.b);
            od.qids.push_back(qid);
            od.incidentQids[aIdx].push_back(qid);
            if (bIdx != aIdx) od.incidentQids[bIdx].push_back(qid);
            qstate_[qid].owner = q.owner;
            qstate_[qid].aIdx = aIdx;
            qstate_[qid].bIdx = bIdx;
            qstate_[qid].multiplicity = q.multiplicity;
        }

        for (int qid = 0; qid < (int)bq_.size(); ++qid) {
            const auto& q = bq_[qid];
            auto& od = ownerData_[q.owner];
            int cid = topo_.incidentClass(q.owner, q.a);
            bool active = (cid >= 0 && cid == topo_.incidentClass(q.owner, q.b));
            if (!active) continue;
            qstate_[qid].active = true;
            qstate_[qid].cid = cid;
            failing_[qid] = true;
            od.activeQueryCount++;
            activeQueryTotal_++;
            if (qstate_[qid].aIdx >= 0) od.endpointActiveCount[qstate_[qid].aIdx]++;
            if (qstate_[qid].bIdx != qstate_[qid].aIdx) od.endpointActiveCount[qstate_[qid].bIdx]++;
            classState(q.owner, cid).activeQueryCount++;
        }
        bumpActiveQueryPeak();

        for (int owner = 1; owner <= n_; ++owner) {
            auto& od = ownerData_[owner];
            vector<int> activeEndpoints;
            activeEndpoints.reserve(od.endpoints.size());
            for (int idx = 0; idx < (int)od.endpoints.size(); ++idx) if (od.endpointActiveCount[idx] > 0) {
                activeEndpoints.push_back(od.endpoints[idx]);
            }
            topo_.restrictOwnerToActiveEndpoints(owner, activeEndpoints);
            for (int idx = 0; idx < (int)od.endpoints.size(); ++idx) if (od.endpointActiveCount[idx] > 0) {
                int ep = od.endpoints[idx];
                int cid = topo_.incidentClass(owner, ep);
                if (cid >= 0) classState(owner, cid).endpointPool.push_back(idx);
            }
            vector<int> cids;
            cids.reserve(od.classStates.size());
            for (const auto& kv : od.classStates) if (kv.second.activeQueryCount > 0) cids.push_back(kv.first);
            sort(cids.begin(), cids.end());
            cids.erase(unique(cids.begin(), cids.end()), cids.end());
            for (int cid : cids) rebuildSupport(owner, cid);
        }
    }

    int comp(int v) const override { return (1 <= v && v <= n_ && alive_[v]) ? compId_[v] : -1; }
    vector<int> listComponents() const override { return topo_.listComponents(); }
    bool isFailing(int qid) const override { return failing_[qid]; }

    void eraseVertex(int x, vector<int>& newComponents, vector<WitnessChange>& changes) override {
        newComponents.clear();
        changes.clear();
        if (!(1 <= x && x <= n_) || !alive_[x]) return;
        currentDeleteX_ = x;
        ++currentDeleteStep_;
#ifdef LOCAL
        g_batch_dbg.debug_profile_total_deletions++;
        g_local_profile_current_delete_sampled = should_profile_deletion_sample(currentDeleteStep_);
        if (g_local_profile_current_delete_sampled) g_batch_dbg.debug_profile_sampled_deletions++;
        const bool __need_deletion_profile = local_profile_topk_enabled();
        const long long __delete_start_ns = __need_deletion_profile ? dbg_now_ns() : 0;
        const long long __pre_global_delete_dfs_edges = __need_deletion_profile ? g_topo_dbg.global_delete_dfs_edges : 0;
        const long long __pre_query_incident_scans = __need_deletion_profile ? g_batch_dbg.query_incident_scans : 0;
        const long long __pre_connector_skeleton_terminals = __need_deletion_profile ? g_batch_dbg.connector_skeleton_terminals : 0;
        const long long __pre_connector_skeleton_vertices = __need_deletion_profile ? g_batch_dbg.connector_skeleton_vertices : 0;
        const long long __pre_connector_skeleton_watch_unregister = __need_deletion_profile ? g_batch_dbg.connector_skeleton_watch_unregister : 0;
        const long long __pre_connector_skeleton_watch_register = __need_deletion_profile ? g_batch_dbg.connector_skeleton_watch_register : 0;
        const long long __pre_preserved_piece_split_vertices = __need_deletion_profile ? g_batch_dbg.preserved_piece_split_vertices : 0;
        const long long __pre_time_global_delete_dfs_ns = __need_deletion_profile ? g_batch_dbg.time_global_delete_dfs_ns : 0;
        const long long __pre_time_connector_skeleton_build_ns = __need_deletion_profile ? g_batch_dbg.time_connector_skeleton_build_ns : 0;
        const long long __pre_time_connector_skeleton_watch_unregister_ns = __need_deletion_profile ? g_batch_dbg.time_connector_skeleton_watch_unregister_ns : 0;
        const long long __pre_time_connector_skeleton_watch_register_ns = __need_deletion_profile ? g_batch_dbg.time_connector_skeleton_watch_register_ns : 0;
        const long long __pre_time_preserved_piece_split_ns = __need_deletion_profile ? g_batch_dbg.time_preserved_piece_split_ns : 0;
        const long long __pre_time_query_incident_scan_ns = __need_deletion_profile ? g_batch_dbg.time_query_incident_scan_ns : 0;
        const long long __pre_dispatch_candidate_cids = __need_deletion_profile ? g_batch_dbg.dispatch_candidate_cids : 0;
        const long long __pre_dispatch_publish_preserved_handles = __need_deletion_profile ? g_batch_dbg.dispatch_publish_preserved_handles : 0;
        const long long __pre_dispatch_publish_connector_handles = __need_deletion_profile ? g_batch_dbg.dispatch_publish_connector_handles : 0;
        const long long __pre_dispatch_publish_posmap_build_calls = __need_deletion_profile ? g_batch_dbg.dispatch_publish_posmap_build_calls : 0;
        const long long __pre_dispatch_publish_full_rescan_calls = __need_deletion_profile ? g_batch_dbg.dispatch_publish_full_rescan_calls : 0;
        const long long __pre_dispatch_publish_noop_calls = __need_deletion_profile ? g_batch_dbg.dispatch_publish_noop_calls : 0;
        const long long __pre_time_reuse_route_baseline_ns = __need_deletion_profile ? g_batch_dbg.time_reuse_route_baseline_ns : 0;
        const long long __pre_time_reuse_route_delta_preserved_then_skeleton_ns = __need_deletion_profile ? g_batch_dbg.time_reuse_route_delta_preserved_then_skeleton_ns : 0;
        const long long __pre_time_reuse_route_connector_skeleton_ns = __need_deletion_profile ? g_batch_dbg.time_reuse_route_connector_skeleton_ns : 0;
        const long long __pre_time_reuse_route_general_delta_ns = __need_deletion_profile ? g_batch_dbg.time_reuse_route_general_delta_ns : 0;
        const long long __pre_reuse_keepmask_removed_handles = __need_deletion_profile ? g_batch_dbg.reuse_keepmask_removed_handles : 0;
        const long long __pre_reuse_preserved_direct_retag_handles = __need_deletion_profile ? g_batch_dbg.reuse_preserved_direct_retag_handles : 0;
        const long long __pre_reuse_connector_direct_retag_handles = __need_deletion_profile ? g_batch_dbg.reuse_connector_direct_retag_handles : 0;
        const long long __pre_reuse_attachment_retargets = __need_deletion_profile ? g_batch_dbg.reuse_attachment_retargets : 0;
        const long long __pre_reuse_patch_vertices = __need_deletion_profile ? g_batch_dbg.reuse_patch_vertices : 0;
        const long long __pre_reuse_patch_handles_added = __need_deletion_profile ? g_batch_dbg.reuse_patch_handles_added : 0;
        const long long __pre_reuse_prepublish_preserved_annotate_calls = __need_deletion_profile ? g_batch_dbg.reuse_prepublish_preserved_annotate_calls : 0;
        const long long __pre_reuse_prepublish_connector_annotate_calls = __need_deletion_profile ? g_batch_dbg.reuse_prepublish_connector_annotate_calls : 0;
        const long long __pre_reuse_final_publish_noop_calls = __need_deletion_profile ? g_batch_dbg.reuse_final_publish_noop_calls : 0;
        const long long __pre_reuse_final_publish_skipped_calls = __need_deletion_profile ? g_batch_dbg.reuse_final_publish_skipped_calls : 0;
        const long long __pre_time_reuse_keepmask_scan_ns = __need_deletion_profile ? g_batch_dbg.time_reuse_keepmask_scan_ns : 0;
        const long long __pre_time_reuse_watch_retain_ns = __need_deletion_profile ? g_batch_dbg.time_reuse_watch_retain_ns : 0;
        const long long __pre_time_retain_kept_vector_build_ns = __need_deletion_profile ? g_batch_dbg.time_retain_kept_vector_build_ns : 0;
        const long long __pre_time_retain_kept_handle_copy_ns = __need_deletion_profile ? g_batch_dbg.time_retain_kept_handle_copy_ns : 0;
        const long long __pre_time_retain_kept_handleidx_patch_ns = __need_deletion_profile ? g_batch_dbg.time_retain_kept_handleidx_patch_ns : 0;
        const long long __pre_time_retain_final_swap_state_update_ns = __need_deletion_profile ? g_batch_dbg.time_retain_final_swap_state_update_ns : 0;
        const long long __pre_time_wscan_route_baseline_ns = __need_deletion_profile ? g_batch_dbg.time_wscan_route_baseline_ns : 0;
        const long long __pre_time_wscan_route_delta_preserved_then_skeleton_ns = __need_deletion_profile ? g_batch_dbg.time_wscan_route_delta_preserved_then_skeleton_ns : 0;
        const long long __pre_time_wscan_route_connector_skeleton_ns = __need_deletion_profile ? g_batch_dbg.time_wscan_route_connector_skeleton_ns : 0;
        const long long __pre_time_wscan_route_general_delta_ns = __need_deletion_profile ? g_batch_dbg.time_wscan_route_general_delta_ns : 0;
        const long long __pre_wscan_handles_scanned_preserved_keepmask = __need_deletion_profile ? g_batch_dbg.wscan_handles_scanned_preserved_keepmask : 0;
        const long long __pre_wscan_handles_scanned_connector_keepmask = __need_deletion_profile ? g_batch_dbg.wscan_handles_scanned_connector_keepmask : 0;
        const long long __pre_wscan_handles_scanned_existing_connector_set = __need_deletion_profile ? g_batch_dbg.wscan_handles_scanned_existing_connector_set : 0;
        const long long __pre_wscan_retain_removed_handles = __need_deletion_profile ? g_batch_dbg.wscan_retain_removed_handles : 0;
        const long long __pre_wscan_retain_slotpos_fixups = __need_deletion_profile ? g_batch_dbg.wscan_retain_slotpos_fixups : 0;
        const long long __pre_wscan_duplicate_full_scan_passes = __need_deletion_profile ? g_batch_dbg.wscan_duplicate_full_scan_passes : 0;
        const long long __pre_retain_removed_handles = __need_deletion_profile ? g_batch_dbg.retain_removed_handles : 0;
        const long long __pre_retain_removed_sparse_entries = __need_deletion_profile ? g_batch_dbg.retain_removed_sparse_entries : 0;
        const long long __pre_retain_moved_entry_count = __need_deletion_profile ? g_batch_dbg.retain_moved_entry_count : 0;
        const long long __pre_retain_owner_lookup_calls = __need_deletion_profile ? g_batch_dbg.retain_owner_lookup_calls : 0;
        const long long __pre_retain_owner_lookup_misses = __need_deletion_profile ? g_batch_dbg.retain_owner_lookup_misses : 0;
        const long long __pre_retain_slotpos_fixups = __need_deletion_profile ? g_batch_dbg.retain_slotpos_fixups : 0;
        const long long __pre_retain_kept_handles_copied = __need_deletion_profile ? g_batch_dbg.retain_kept_handles_copied : 0;
        const long long __pre_retain_handleidx_fixups = __need_deletion_profile ? g_batch_dbg.retain_handleidx_fixups : 0;
        const long long __pre_kvec_unchanged_prefix_handles = __need_deletion_profile ? g_batch_dbg.kvec_unchanged_prefix_handles : 0;
        const long long __pre_kvec_unchanged_suffix_handles = __need_deletion_profile ? g_batch_dbg.kvec_unchanged_suffix_handles : 0;
        const long long __pre_kvec_moved_suffix_handles = __need_deletion_profile ? g_batch_dbg.kvec_moved_suffix_handles : 0;
        const long long __pre_kvec_changed_patchlist_entries = __need_deletion_profile ? g_batch_dbg.kvec_changed_patchlist_entries : 0;
        const long long __pre_kvec_handle_copy_entries = __need_deletion_profile ? g_batch_dbg.kvec_handle_copy_entries : 0;
        const long long __pre_kvec_handleidx_patch_changed_entries = __need_deletion_profile ? g_batch_dbg.kvec_handleidx_patch_changed_entries : 0;
        const long long __pre_kvec_handleidx_patch_skipped_same_index_entries = __need_deletion_profile ? g_batch_dbg.kvec_handleidx_patch_skipped_same_index_entries : 0;
        const long long __pre_kvec_inplace_compact_calls = __need_deletion_profile ? g_batch_dbg.kvec_inplace_compact_calls : 0;
        const long long __pre_kvec_suffix_resize_fastpath_calls = __need_deletion_profile ? g_batch_dbg.kvec_suffix_resize_fastpath_calls : 0;
        const long long __pre_scomp_first_removed_index_sum = __need_deletion_profile ? g_batch_dbg.scomp_first_removed_index_sum : 0;
        const long long __pre_scomp_removed_run_count_sum = __need_deletion_profile ? g_batch_dbg.scomp_removed_run_count_sum : 0;
        const long long __pre_scomp_kept_run_count_sum = __need_deletion_profile ? g_batch_dbg.scomp_kept_run_count_sum : 0;
        const long long __pre_scomp_prefix_skipped_handles = __need_deletion_profile ? g_batch_dbg.scomp_prefix_skipped_handles : 0;
        const long long __pre_scomp_block_copied_handles = __need_deletion_profile ? g_batch_dbg.scomp_block_copied_handles : 0;
        const long long __pre_scomp_elementwise_emitted_handles = __need_deletion_profile ? g_batch_dbg.scomp_elementwise_emitted_handles : 0;
        const long long __pre_scomp_suffix_only_calls = __need_deletion_profile ? g_batch_dbg.scomp_suffix_only_calls : 0;
        const long long __pre_scomp_single_middle_run_calls = __need_deletion_profile ? g_batch_dbg.scomp_single_middle_run_calls : 0;
        const long long __pre_scomp_scratch_capacity_reuse_calls = __need_deletion_profile ? g_batch_dbg.scomp_scratch_capacity_reuse_calls : 0;
        const long long __pre_time_scomp_first_removed_seek_ns = __need_deletion_profile ? g_batch_dbg.time_scomp_first_removed_seek_ns : 0;
        const long long __pre_time_scomp_suffix_only_check_ns = __need_deletion_profile ? g_batch_dbg.time_scomp_suffix_only_check_ns : 0;
        const long long __pre_time_scomp_kept_count_scan_ns = __need_deletion_profile ? g_batch_dbg.time_scomp_kept_count_scan_ns : 0;
        const long long __pre_time_scomp_kept_run_partition_build_ns = __need_deletion_profile ? g_batch_dbg.time_scomp_kept_run_partition_build_ns : 0;
        const long long __pre_time_scomp_prefix_skip_ns = __need_deletion_profile ? g_batch_dbg.time_scomp_prefix_skip_ns : 0;
        const long long __pre_time_scomp_contiguous_run_block_copy_ns = __need_deletion_profile ? g_batch_dbg.time_scomp_contiguous_run_block_copy_ns : 0;
        const long long __pre_time_scomp_elementwise_emit_ns = __need_deletion_profile ? g_batch_dbg.time_scomp_elementwise_emit_ns : 0;
        const long long __pre_time_scomp_scratch_prepare_ns = __need_deletion_profile ? g_batch_dbg.time_scomp_scratch_prepare_ns : 0;
        const long long __pre_time_scomp_tail_cleanup_ns = __need_deletion_profile ? g_batch_dbg.time_scomp_tail_cleanup_ns : 0;
        const long long __pre_time_scomp_final_resize_swap_ns = __need_deletion_profile ? g_batch_dbg.time_scomp_final_resize_swap_ns : 0;
        const long long __pre_time_bcopy_single_middle_run_detect_ns = __need_deletion_profile ? g_batch_dbg.time_bcopy_single_middle_run_detect_ns : 0;
        const long long __pre_time_bcopy_run_coalesce_build_ns = __need_deletion_profile ? g_batch_dbg.time_bcopy_run_coalesce_build_ns : 0;
        const long long __pre_time_bcopy_direct_suffix_memmove_ns = __need_deletion_profile ? g_batch_dbg.time_bcopy_direct_suffix_memmove_ns : 0;
        const long long __pre_time_bcopy_multi_run_block_copy_ns = __need_deletion_profile ? g_batch_dbg.time_bcopy_multi_run_block_copy_ns : 0;
        const long long __pre_time_bcopy_short_fragment_elementwise_fallback_ns = __need_deletion_profile ? g_batch_dbg.time_bcopy_short_fragment_elementwise_fallback_ns : 0;
        const long long __pre_time_bcopy_overlap_safe_staging_ns = __need_deletion_profile ? g_batch_dbg.time_bcopy_overlap_safe_staging_ns : 0;
        const long long __pre_time_bcopy_route_baseline_ns = __need_deletion_profile ? g_batch_dbg.time_bcopy_route_baseline_ns : 0;
        const long long __pre_time_bcopy_route_delta_preserved_then_skeleton_ns = __need_deletion_profile ? g_batch_dbg.time_bcopy_route_delta_preserved_then_skeleton_ns : 0;
        const long long __pre_time_bcopy_route_connector_skeleton_ns = __need_deletion_profile ? g_batch_dbg.time_bcopy_route_connector_skeleton_ns : 0;
        const long long __pre_time_bcopy_route_general_delta_ns = __need_deletion_profile ? g_batch_dbg.time_bcopy_route_general_delta_ns : 0;
        const long long __pre_bcopy_single_middle_run_calls = __need_deletion_profile ? g_batch_dbg.bcopy_single_middle_run_calls : 0;
        const long long __pre_bcopy_removed_run_count_sum = __need_deletion_profile ? g_batch_dbg.bcopy_removed_run_count_sum : 0;
        const long long __pre_bcopy_kept_run_count_sum = __need_deletion_profile ? g_batch_dbg.bcopy_kept_run_count_sum : 0;
        const long long __pre_bcopy_copy_plan_entries = __need_deletion_profile ? g_batch_dbg.bcopy_copy_plan_entries : 0;
        const long long __pre_bcopy_direct_memmove_calls = __need_deletion_profile ? g_batch_dbg.bcopy_direct_memmove_calls : 0;
        const long long __pre_bcopy_direct_memmoved_handles = __need_deletion_profile ? g_batch_dbg.bcopy_direct_memmoved_handles : 0;
        const long long __pre_bcopy_runwise_block_copied_handles = __need_deletion_profile ? g_batch_dbg.bcopy_runwise_block_copied_handles : 0;
        const long long __pre_bcopy_elementwise_fallback_handles = __need_deletion_profile ? g_batch_dbg.bcopy_elementwise_fallback_handles : 0;
        const long long __pre_bcopy_overlap_staging_calls = __need_deletion_profile ? g_batch_dbg.bcopy_overlap_staging_calls : 0;
        const long long __pre_time_plan_first_removed_seek_ns = __need_deletion_profile ? g_batch_dbg.time_plan_first_removed_seek_ns : 0;
        const long long __pre_time_plan_removed_run_discovery_ns = __need_deletion_profile ? g_batch_dbg.time_plan_removed_run_discovery_ns : 0;
        const long long __pre_time_plan_kept_run_discovery_ns = __need_deletion_profile ? g_batch_dbg.time_plan_kept_run_discovery_ns : 0;
        const long long __pre_time_plan_adjacent_run_coalesce_ns = __need_deletion_profile ? g_batch_dbg.time_plan_adjacent_run_coalesce_ns : 0;
        const long long __pre_time_plan_single_middle_shortcircuit_eligibility_ns = __need_deletion_profile ? g_batch_dbg.time_plan_single_middle_shortcircuit_eligibility_ns : 0;
        const long long __pre_time_plan_dst_index_accumulate_ns = __need_deletion_profile ? g_batch_dbg.time_plan_dst_index_accumulate_ns : 0;
        const long long __pre_time_plan_descriptor_emit_ns = __need_deletion_profile ? g_batch_dbg.time_plan_descriptor_emit_ns : 0;
        const long long __pre_time_plan_small_inline_buffer_prepare_ns = __need_deletion_profile ? g_batch_dbg.time_plan_small_inline_buffer_prepare_ns : 0;
        const long long __pre_time_plan_route_baseline_ns = __need_deletion_profile ? g_batch_dbg.time_plan_route_baseline_ns : 0;
        const long long __pre_time_plan_route_delta_preserved_then_skeleton_ns = __need_deletion_profile ? g_batch_dbg.time_plan_route_delta_preserved_then_skeleton_ns : 0;
        const long long __pre_time_plan_route_connector_skeleton_ns = __need_deletion_profile ? g_batch_dbg.time_plan_route_connector_skeleton_ns : 0;
        const long long __pre_time_plan_route_general_delta_ns = __need_deletion_profile ? g_batch_dbg.time_plan_route_general_delta_ns : 0;
        const long long __pre_plan_first_removed_index_sum = __need_deletion_profile ? g_batch_dbg.plan_first_removed_index_sum : 0;
        const long long __pre_plan_removed_run_count_sum = __need_deletion_profile ? g_batch_dbg.plan_removed_run_count_sum : 0;
        const long long __pre_plan_kept_run_count_sum = __need_deletion_profile ? g_batch_dbg.plan_kept_run_count_sum : 0;
        const long long __pre_plan_adjacent_merge_hits = __need_deletion_profile ? g_batch_dbg.plan_adjacent_merge_hits : 0;
        const long long __pre_plan_descriptor_count = __need_deletion_profile ? g_batch_dbg.plan_descriptor_count : 0;
        const long long __pre_plan_dst_index_updates = __need_deletion_profile ? g_batch_dbg.plan_dst_index_updates : 0;
        const long long __pre_plan_single_middle_shortcircuit_hits = __need_deletion_profile ? g_batch_dbg.plan_single_middle_shortcircuit_hits : 0;
        const long long __pre_plan_small_inline_hits = __need_deletion_profile ? g_batch_dbg.plan_small_inline_hits : 0;
        const long long __pre_time_rdisc_first_removed_seek_ns = __need_deletion_profile ? g_batch_dbg.time_rdisc_first_removed_seek_ns : 0;
        const long long __pre_time_rdisc_boundary_reuse_check_ns = __need_deletion_profile ? g_batch_dbg.time_rdisc_boundary_reuse_check_ns : 0;
        const long long __pre_time_rdisc_removed_run_scan_ns = __need_deletion_profile ? g_batch_dbg.time_rdisc_removed_run_scan_ns : 0;
        const long long __pre_time_rdisc_kept_run_scan_ns = __need_deletion_profile ? g_batch_dbg.time_rdisc_kept_run_scan_ns : 0;
        const long long __pre_time_rdisc_suffix_only_shortcircuit_ns = __need_deletion_profile ? g_batch_dbg.time_rdisc_suffix_only_shortcircuit_ns : 0;
        const long long __pre_time_rdisc_single_middle_shortcircuit_ns = __need_deletion_profile ? g_batch_dbg.time_rdisc_single_middle_shortcircuit_ns : 0;
        const long long __pre_time_rdisc_fused_onepass_scan_ns = __need_deletion_profile ? g_batch_dbg.time_rdisc_fused_onepass_scan_ns : 0;
        const long long __pre_time_rdisc_small_runlist_inline_materialize_ns = __need_deletion_profile ? g_batch_dbg.time_rdisc_small_runlist_inline_materialize_ns : 0;
        const long long __pre_time_rdisc_route_baseline_ns = __need_deletion_profile ? g_batch_dbg.time_rdisc_route_baseline_ns : 0;
        const long long __pre_time_rdisc_route_delta_preserved_then_skeleton_ns = __need_deletion_profile ? g_batch_dbg.time_rdisc_route_delta_preserved_then_skeleton_ns : 0;
        const long long __pre_time_rdisc_route_connector_skeleton_ns = __need_deletion_profile ? g_batch_dbg.time_rdisc_route_connector_skeleton_ns : 0;
        const long long __pre_time_rdisc_route_general_delta_ns = __need_deletion_profile ? g_batch_dbg.time_rdisc_route_general_delta_ns : 0;
        const long long __pre_rdisc_first_removed_index_sum = __need_deletion_profile ? g_batch_dbg.rdisc_first_removed_index_sum : 0;
        const long long __pre_rdisc_removed_run_count_sum = __need_deletion_profile ? g_batch_dbg.rdisc_removed_run_count_sum : 0;
        const long long __pre_rdisc_kept_run_count_sum = __need_deletion_profile ? g_batch_dbg.rdisc_kept_run_count_sum : 0;
        const long long __pre_rdisc_boundary_reuse_hits = __need_deletion_profile ? g_batch_dbg.rdisc_boundary_reuse_hits : 0;
        const long long __pre_rdisc_suffix_only_hits = __need_deletion_profile ? g_batch_dbg.rdisc_suffix_only_hits : 0;
        const long long __pre_rdisc_single_middle_hits = __need_deletion_profile ? g_batch_dbg.rdisc_single_middle_hits : 0;
        const long long __pre_rdisc_fused_onepass_calls = __need_deletion_profile ? g_batch_dbg.rdisc_fused_onepass_calls : 0;
        const long long __pre_rdisc_small_runlist_inline_hits = __need_deletion_profile ? g_batch_dbg.rdisc_small_runlist_inline_hits : 0;
        const long long __pre_time_fclass_suffix_only_gate_ns = __need_deletion_profile ? g_batch_dbg.time_fclass_suffix_only_gate_ns : 0;
        const long long __pre_time_fclass_single_middle_gate_ns = __need_deletion_profile ? g_batch_dbg.time_fclass_single_middle_gate_ns : 0;
        const long long __pre_time_fclass_onepass_transition_scan_ns = __need_deletion_profile ? g_batch_dbg.time_fclass_onepass_transition_scan_ns : 0;
        const long long __pre_time_fclass_transition_emit_runs_ns = __need_deletion_profile ? g_batch_dbg.time_fclass_transition_emit_runs_ns : 0;
        const long long __pre_time_fclass_run_count_finalize_ns = __need_deletion_profile ? g_batch_dbg.time_fclass_run_count_finalize_ns : 0;
        const long long __pre_time_fclass_small_runlist_inline_ns = __need_deletion_profile ? g_batch_dbg.time_fclass_small_runlist_inline_ns : 0;
        const long long __pre_time_fclass_route_baseline_ns = __need_deletion_profile ? g_batch_dbg.time_fclass_route_baseline_ns : 0;
        const long long __pre_time_fclass_route_delta_preserved_then_skeleton_ns = __need_deletion_profile ? g_batch_dbg.time_fclass_route_delta_preserved_then_skeleton_ns : 0;
        const long long __pre_time_fclass_route_connector_skeleton_ns = __need_deletion_profile ? g_batch_dbg.time_fclass_route_connector_skeleton_ns : 0;
        const long long __pre_time_fclass_route_general_delta_ns = __need_deletion_profile ? g_batch_dbg.time_fclass_route_general_delta_ns : 0;
        const long long __pre_fclass_suffix_only_hits = __need_deletion_profile ? g_batch_dbg.fclass_suffix_only_hits : 0;
        const long long __pre_fclass_single_middle_hits = __need_deletion_profile ? g_batch_dbg.fclass_single_middle_hits : 0;
        const long long __pre_fclass_fused_onepass_calls = __need_deletion_profile ? g_batch_dbg.fclass_fused_onepass_calls : 0;
        const long long __pre_fclass_transition_steps = __need_deletion_profile ? g_batch_dbg.fclass_transition_steps : 0;
        const long long __pre_fclass_removed_to_kept_transitions = __need_deletion_profile ? g_batch_dbg.fclass_removed_to_kept_transitions : 0;
        const long long __pre_fclass_kept_to_removed_transitions = __need_deletion_profile ? g_batch_dbg.fclass_kept_to_removed_transitions : 0;
        const long long __pre_fclass_small_inline_hits = __need_deletion_profile ? g_batch_dbg.fclass_small_inline_hits : 0;
        int __touched_class_count = 0;
#endif
        currentDeleteWatchSnapshots_.clear();
        vector<TouchedClassInfo> touchedInfos = gatherTouchedClassInfos(x);
#ifdef LOCAL
        __touched_class_count = (int)touchedInfos.size();
#endif
        vector<int> touchedOwners;
        touchedOwners.reserve(touchedInfos.size() + 1);
        for (const auto& info : touchedInfos) {
            if (!(1 <= info.owner && info.owner <= n_)) continue;
            auto it = ownerData_[info.owner].classStates.find(info.oldCid);
            if (it == ownerData_[info.owner].classStates.end() || it->second.activeQueryCount <= 0) {
                auto& st = classState(info.owner, info.oldCid);
                unregisterClassWatch(info.owner, info.oldCid, st);
                if (++st.epoch == INT_MAX) st.epoch = 1;
                st.endpointPool.clear();
#ifdef LOCAL
                g_topo_dbg.untouched_class_skips++;
#endif
                continue;
            }
            touchedOwners.push_back(info.owner);
        }
        if (1 <= x && x <= n_ && ownerData_[x].activeQueryCount > 0) touchedOwners.push_back(x);
        sort(touchedOwners.begin(), touchedOwners.end());
        touchedOwners.erase(unique(touchedOwners.begin(), touchedOwners.end()), touchedOwners.end());

        OwnerSplitArtifact deleteArtifact;
        topo_.deleteVertexAndSplit(x, touchedOwners, newComponents, &deleteArtifact);
        int artifactStamp = deleteArtifact.valid ? loadArtifactIntoScratch(deleteArtifact) : 0;
        alive_[x] = false;
        syncComponents();

        if (1 <= x && x <= n_) {
            auto qids = ownerData_[x].qids;
            for (int qid : qids) if (qstate_[qid].active) resolveQuery(qid, true, changes);
            deactivateAllOwnerWatches(x);
        }

        nextQuerySeenStamp();
        unordered_map<int, DecrementalNBTopology::OwnerBucketContext> ownerCtxCache;
        vector<pair<int,int>> rebuildKeys;
        rebuildKeys.reserve(touchedInfos.size() * 3 + 4);
        for (const auto& info : touchedInfos) {
            int owner = info.owner;
            if (!topo_.aliveVertex(owner)) {
#ifdef LOCAL
                g_topo_dbg.untouched_class_skips++;
#endif
                continue;
            }
#ifdef LOCAL
            ScopedNsAcc __route_timer(ptr_if(local_profile_coarse_enabled(), &g_batch_dbg.time_route_dispatch_ns),
                                      ptr_if(local_profile_coarse_enabled(), &g_batch_dbg.time_route_dispatch_calls));
            g_batch_dbg.class_split_events++;
#endif
            auto& od = ownerData_[owner];
#ifdef LOCAL
            const bool __detail_dispatch = local_profile_detailed_enabled();
            long long __unanimous_dispatch_start_ns = __detail_dispatch ? dbg_now_ns() : 0;
#endif
            auto refineRes = refineTouchedClassCoverageCollapse(x, info, ownerCtxCache);
            vector<int> candidateCids = refineRes.candidateCids;
            for (int idx : refineRes.movedIdxs) {
                if (!(0 <= idx && idx < (int)od.endpoints.size())) continue;
                int ep = od.endpoints[idx];
                int newCid = topo_.aliveVertex(ep) ? topo_.incidentClass(owner, ep) : -1;
                if (newCid != info.oldCid) {
#ifdef LOCAL
                    g_batch_dbg.moved_endpoint_count++;
#endif
                    if (newCid >= 0 && od.endpointActiveCount[idx] > 0) appendEndpointToClassPool(owner, newCid, idx);
                    if (newCid >= 0) candidateCids.push_back(newCid);
                }
            }
            for (int idx : refineRes.movedIdxs) {
                if (!(0 <= idx && idx < (int)od.incidentQids.size())) continue;
#ifdef LOCAL
                ScopedNsAcc __query_timer(ptr_if(local_profile_coarse_enabled(), &g_batch_dbg.time_query_incident_scan_ns),
                                          ptr_if(local_profile_coarse_enabled(), &g_batch_dbg.time_query_incident_scan_calls));
#endif
                for (int qid : od.incidentQids[idx]) {
#ifdef LOCAL
                    g_batch_dbg.query_incident_scans++;
#endif
                    if (querySeenStamp_[qid] == querySeenCur_) continue;
                    querySeenStamp_[qid] = querySeenCur_;
                    auto& qs = qstate_[qid];
                    if (!qs.active) continue;
                    int a = od.endpoints[qs.aIdx];
                    int b = od.endpoints[qs.bIdx];
                    if (!topo_.aliveVertex(owner) || !topo_.aliveVertex(a) || !topo_.aliveVertex(b)) {
                        resolveQuery(qid, true, changes);
                        continue;
                    }
                    int newCa = topo_.incidentClass(owner, a);
                    int newCb = topo_.incidentClass(owner, b);
                    if (newCa < 0 || newCb < 0) {
                        resolveQuery(qid, true, changes);
                        continue;
                    }
                    if (newCa != newCb) {
                        resolveQuery(qid, false, changes);
                        continue;
                    }
                    int newCid = newCa;
                    if (newCid != qs.cid) {
                        auto itOld = od.classStates.find(qs.cid);
                        if (itOld != od.classStates.end() && itOld->second.activeQueryCount > 0) itOld->second.activeQueryCount--;
                        qs.cid = newCid;
                        classState(owner, newCid).activeQueryCount++;
                        appendEndpointToClassPool(owner, newCid, qs.aIdx);
                        if (qs.bIdx != qs.aIdx) appendEndpointToClassPool(owner, newCid, qs.bIdx);
                        candidateCids.push_back(newCid);
                    }
                }
            }

            bool oldHandledByReuse = false;
            if (!refineRes.usedFullScan && refineRes.reuseKind != SupportReuseKind::None) {
                auto itOldSt = ownerData_[owner].classStates.find(info.oldCid);
                if (itOldSt != ownerData_[owner].classStates.end()) {
                    if (!topo_.aliveVertex(owner) || itOldSt->second.activeQueryCount <= 0) {
                        unregisterClassWatch(owner, info.oldCid, itOldSt->second);
                        if (++itOldSt->second.epoch == INT_MAX) itOldSt->second.epoch = 1;
                        itOldSt->second.endpointPool.clear();
                        oldHandledByReuse = true;
                    } else {
                        oldHandledByReuse = tryReuseSupportForClass(owner, info.oldCid, info, refineRes);
                    }
                }
            }

#ifdef LOCAL
            if (__detail_dispatch && refineRes.reuseKind != SupportReuseKind::None && !refineRes.usedFullScan) {
                g_batch_dbg.time_unanimous_mode_dispatch_ns += dbg_now_ns() - __unanimous_dispatch_start_ns;
                g_batch_dbg.time_unanimous_mode_dispatch_calls++;
            }
#endif
            sort(candidateCids.begin(), candidateCids.end());
            candidateCids.erase(unique(candidateCids.begin(), candidateCids.end()), candidateCids.end());
            for (int cid : candidateCids) if (cid >= 0) {
                if (oldHandledByReuse && cid == info.oldCid) continue;
                rebuildKeys.push_back({owner, cid});
            }
        }

        sort(rebuildKeys.begin(), rebuildKeys.end());
        rebuildKeys.erase(unique(rebuildKeys.begin(), rebuildKeys.end()), rebuildKeys.end());
        for (auto [owner, cid] : rebuildKeys) {
            rebuildSupport(owner, cid, &deleteArtifact, artifactStamp);
        }

#ifdef LOCAL
        if (!watchByVertex_[x].empty()) {
            for (const auto& ref : watchByVertex_[x]) {
                if (!(1 <= ref.owner && ref.owner <= n_)) continue;
                auto it = ownerData_[ref.owner].classStates.find(ref.cid);
                if (it != ownerData_[ref.owner].classStates.end() &&
                    0 <= ref.handleIdx && ref.handleIdx < (int)it->second.watchHandles.size() &&
                    it->second.watchHandles[ref.handleIdx].vertex == x) {
                    dumpDeletedVertexWatchLeak(x, ref.owner, ref.cid, it->second.watchHandles[ref.handleIdx]);
                    cerr << "LIVE WATCH LEFT ON DELETED VERTEX x=" << x << " owner=" << ref.owner << " cid=" << ref.cid << "\n";
                    abort();
                } else {
                    WatchHandle fake;
                    fake.vertex = x;
                    fake.originKind = SupportOriginKind::MaterializedSupport;
                    fake.treeId = -1;
                    fake.pieceId = -1;
                    fake.localPos = -1;
                    dumpDeletedVertexWatchLeak(x, ref.owner, ref.cid, fake);
                    cerr << "LIVE WATCH LEFT ON DELETED VERTEX x=" << x << " owner=" << ref.owner << " cid=" << ref.cid << " (stale/foreign)\n";
                    abort();
                }
            }
        }
#endif
        vector<WatchEntry>().swap(watchByVertex_[x]);
        currentDeleteWatchSnapshots_.clear();
        currentDeleteX_ = -1;
        bumpActiveQueryPeak();

#ifdef LOCAL
        verifyTouchedOwnersExact(touchedOwners);
        g_batch_dbg.debug_progress_last_deletion = currentDeleteStep_;
        if (__need_deletion_profile) {
            SlowDeletionProfile __rec;
            __rec.deletionIndex = currentDeleteStep_;
            __rec.deletedVertex = x;
            __rec.touchedClassCount = __touched_class_count;
            __rec.connectorSkeletonTerminals = g_batch_dbg.connector_skeleton_terminals - __pre_connector_skeleton_terminals;
            __rec.connectorSkeletonVertices = g_batch_dbg.connector_skeleton_vertices - __pre_connector_skeleton_vertices;
            __rec.connectorSkeletonWatchUnregister = g_batch_dbg.connector_skeleton_watch_unregister - __pre_connector_skeleton_watch_unregister;
            __rec.connectorSkeletonWatchRegister = g_batch_dbg.connector_skeleton_watch_register - __pre_connector_skeleton_watch_register;
            __rec.preservedPieceSplitVertices = g_batch_dbg.preserved_piece_split_vertices - __pre_preserved_piece_split_vertices;
            __rec.globalDeleteDfsEdges = g_topo_dbg.global_delete_dfs_edges - __pre_global_delete_dfs_edges;
            __rec.queryIncidentScans = g_batch_dbg.query_incident_scans - __pre_query_incident_scans;
            __rec.totalDeletionTimeNs = dbg_now_ns() - __delete_start_ns;
            __rec.timeGlobalDeleteDfsNs = g_batch_dbg.time_global_delete_dfs_ns - __pre_time_global_delete_dfs_ns;
            __rec.timeConnectorSkeletonBuildNs = g_batch_dbg.time_connector_skeleton_build_ns - __pre_time_connector_skeleton_build_ns;
            __rec.timeConnectorSkeletonWatchUnregisterNs = g_batch_dbg.time_connector_skeleton_watch_unregister_ns - __pre_time_connector_skeleton_watch_unregister_ns;
            __rec.timeConnectorSkeletonWatchRegisterNs = g_batch_dbg.time_connector_skeleton_watch_register_ns - __pre_time_connector_skeleton_watch_register_ns;
            __rec.timePreservedPieceSplitNs = g_batch_dbg.time_preserved_piece_split_ns - __pre_time_preserved_piece_split_ns;
            __rec.timeQueryIncidentScanNs = g_batch_dbg.time_query_incident_scan_ns - __pre_time_query_incident_scan_ns;
            __rec.dispatchCandidateCids = g_batch_dbg.dispatch_candidate_cids - __pre_dispatch_candidate_cids;
            __rec.publishPreservedHandles = g_batch_dbg.dispatch_publish_preserved_handles - __pre_dispatch_publish_preserved_handles;
            __rec.publishConnectorHandles = g_batch_dbg.dispatch_publish_connector_handles - __pre_dispatch_publish_connector_handles;
            __rec.publishPosmapBuilds = g_batch_dbg.dispatch_publish_posmap_build_calls - __pre_dispatch_publish_posmap_build_calls;
            __rec.publishFullRescanCalls = g_batch_dbg.dispatch_publish_full_rescan_calls - __pre_dispatch_publish_full_rescan_calls;
            __rec.publishNoopCalls = g_batch_dbg.dispatch_publish_noop_calls - __pre_dispatch_publish_noop_calls;
            long long __route_baseline = g_batch_dbg.time_reuse_route_baseline_ns - __pre_time_reuse_route_baseline_ns;
            long long __route_delta = g_batch_dbg.time_reuse_route_delta_preserved_then_skeleton_ns - __pre_time_reuse_route_delta_preserved_then_skeleton_ns;
            long long __route_conn = g_batch_dbg.time_reuse_route_connector_skeleton_ns - __pre_time_reuse_route_connector_skeleton_ns;
            long long __route_general = g_batch_dbg.time_reuse_route_general_delta_ns - __pre_time_reuse_route_general_delta_ns;
            __rec.timeReuseTotalNs = __route_baseline + __route_delta + __route_conn + __route_general;
            __rec.reuseRouteTag = REUSE_ROUTE_NONE;
            long long __best_route_ns = 0;
            if (__route_baseline > __best_route_ns) { __best_route_ns = __route_baseline; __rec.reuseRouteTag = REUSE_ROUTE_BASELINE; }
            if (__route_delta > __best_route_ns) { __best_route_ns = __route_delta; __rec.reuseRouteTag = REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON; }
            if (__route_conn > __best_route_ns) { __best_route_ns = __route_conn; __rec.reuseRouteTag = REUSE_ROUTE_CONNECTOR_SKELETON; }
            if (__route_general > __best_route_ns) { __best_route_ns = __route_general; __rec.reuseRouteTag = REUSE_ROUTE_GENERAL_DELTA; }
            __rec.reuseKeepmaskRemovedHandles = g_batch_dbg.reuse_keepmask_removed_handles - __pre_reuse_keepmask_removed_handles;
            __rec.reusePreservedDirectRetagHandles = g_batch_dbg.reuse_preserved_direct_retag_handles - __pre_reuse_preserved_direct_retag_handles;
            __rec.reuseConnectorDirectRetagHandles = g_batch_dbg.reuse_connector_direct_retag_handles - __pre_reuse_connector_direct_retag_handles;
            __rec.reuseAttachmentRetargets = g_batch_dbg.reuse_attachment_retargets - __pre_reuse_attachment_retargets;
            __rec.reusePatchVertices = g_batch_dbg.reuse_patch_vertices - __pre_reuse_patch_vertices;
            __rec.reusePatchHandlesAdded = g_batch_dbg.reuse_patch_handles_added - __pre_reuse_patch_handles_added;
            __rec.reusePrepublishPreservedAnnotateCalls = g_batch_dbg.reuse_prepublish_preserved_annotate_calls - __pre_reuse_prepublish_preserved_annotate_calls;
            __rec.reusePrepublishConnectorAnnotateCalls = g_batch_dbg.reuse_prepublish_connector_annotate_calls - __pre_reuse_prepublish_connector_annotate_calls;
            __rec.reuseFinalPublishNoopCalls = g_batch_dbg.reuse_final_publish_noop_calls - __pre_reuse_final_publish_noop_calls;
            __rec.reuseFinalPublishSkippedCalls = g_batch_dbg.reuse_final_publish_skipped_calls - __pre_reuse_final_publish_skipped_calls;
            long long __wscan_route_baseline = g_batch_dbg.time_wscan_route_baseline_ns - __pre_time_wscan_route_baseline_ns;
            long long __wscan_route_delta = g_batch_dbg.time_wscan_route_delta_preserved_then_skeleton_ns - __pre_time_wscan_route_delta_preserved_then_skeleton_ns;
            long long __wscan_route_conn = g_batch_dbg.time_wscan_route_connector_skeleton_ns - __pre_time_wscan_route_connector_skeleton_ns;
            long long __wscan_route_general = g_batch_dbg.time_wscan_route_general_delta_ns - __pre_time_wscan_route_general_delta_ns;
            __rec.wscanRouteTag = REUSE_ROUTE_NONE;
            long long __best_wscan_route_ns = 0;
            if (__wscan_route_baseline > __best_wscan_route_ns) { __best_wscan_route_ns = __wscan_route_baseline; __rec.wscanRouteTag = REUSE_ROUTE_BASELINE; }
            if (__wscan_route_delta > __best_wscan_route_ns) { __best_wscan_route_ns = __wscan_route_delta; __rec.wscanRouteTag = REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON; }
            if (__wscan_route_conn > __best_wscan_route_ns) { __best_wscan_route_ns = __wscan_route_conn; __rec.wscanRouteTag = REUSE_ROUTE_CONNECTOR_SKELETON; }
            if (__wscan_route_general > __best_wscan_route_ns) { __best_wscan_route_ns = __wscan_route_general; __rec.wscanRouteTag = REUSE_ROUTE_GENERAL_DELTA; }
            __rec.wscanPreservedHandlesScanned = g_batch_dbg.wscan_handles_scanned_preserved_keepmask - __pre_wscan_handles_scanned_preserved_keepmask;
            __rec.wscanConnectorHandlesScanned = g_batch_dbg.wscan_handles_scanned_connector_keepmask - __pre_wscan_handles_scanned_connector_keepmask;
            __rec.wscanExistingConnectorSetHandlesScanned = g_batch_dbg.wscan_handles_scanned_existing_connector_set - __pre_wscan_handles_scanned_existing_connector_set;
            __rec.wscanRetainRemovedHandles = g_batch_dbg.wscan_retain_removed_handles - __pre_wscan_retain_removed_handles;
            __rec.wscanRetainSlotposFixups = g_batch_dbg.wscan_retain_slotpos_fixups - __pre_wscan_retain_slotpos_fixups;
            __rec.wscanDuplicateFullScanPasses = g_batch_dbg.wscan_duplicate_full_scan_passes - __pre_wscan_duplicate_full_scan_passes;
            __rec.retainRemovedHandles = g_batch_dbg.retain_removed_handles - __pre_retain_removed_handles;
            __rec.retainSparseRemovedEntries = g_batch_dbg.retain_removed_sparse_entries - __pre_retain_removed_sparse_entries;
            __rec.retainMovedEntryCount = g_batch_dbg.retain_moved_entry_count - __pre_retain_moved_entry_count;
            __rec.retainOwnerLookupCalls = g_batch_dbg.retain_owner_lookup_calls - __pre_retain_owner_lookup_calls;
            __rec.retainOwnerLookupMisses = g_batch_dbg.retain_owner_lookup_misses - __pre_retain_owner_lookup_misses;
            __rec.retainSlotposFixups = g_batch_dbg.retain_slotpos_fixups - __pre_retain_slotpos_fixups;
            __rec.retainKeptHandlesCopied = g_batch_dbg.retain_kept_handles_copied - __pre_retain_kept_handles_copied;
            __rec.retainHandleidxFixups = g_batch_dbg.retain_handleidx_fixups - __pre_retain_handleidx_fixups;
            __rec.kvecUnchangedPrefixHandles = g_batch_dbg.kvec_unchanged_prefix_handles - __pre_kvec_unchanged_prefix_handles;
            __rec.kvecUnchangedSuffixHandles = g_batch_dbg.kvec_unchanged_suffix_handles - __pre_kvec_unchanged_suffix_handles;
            __rec.kvecMovedSuffixHandles = g_batch_dbg.kvec_moved_suffix_handles - __pre_kvec_moved_suffix_handles;
            __rec.kvecChangedPatchlistEntries = g_batch_dbg.kvec_changed_patchlist_entries - __pre_kvec_changed_patchlist_entries;
            __rec.kvecHandleCopyEntries = g_batch_dbg.kvec_handle_copy_entries - __pre_kvec_handle_copy_entries;
            __rec.kvecHandleidxPatchChangedEntries = g_batch_dbg.kvec_handleidx_patch_changed_entries - __pre_kvec_handleidx_patch_changed_entries;
            __rec.kvecHandleidxPatchSkippedSameIndexEntries = g_batch_dbg.kvec_handleidx_patch_skipped_same_index_entries - __pre_kvec_handleidx_patch_skipped_same_index_entries;
            __rec.kvecInplaceCompactCalls = g_batch_dbg.kvec_inplace_compact_calls - __pre_kvec_inplace_compact_calls;
            __rec.kvecSuffixResizeFastpathCalls = g_batch_dbg.kvec_suffix_resize_fastpath_calls - __pre_kvec_suffix_resize_fastpath_calls;
            __rec.scompFirstRemovedIndex = g_batch_dbg.scomp_first_removed_index_sum - __pre_scomp_first_removed_index_sum;
            __rec.scompRemovedRunCount = g_batch_dbg.scomp_removed_run_count_sum - __pre_scomp_removed_run_count_sum;
            __rec.scompKeptRunCount = g_batch_dbg.scomp_kept_run_count_sum - __pre_scomp_kept_run_count_sum;
            __rec.scompPrefixSkippedHandles = g_batch_dbg.scomp_prefix_skipped_handles - __pre_scomp_prefix_skipped_handles;
            __rec.scompBlockCopiedHandles = g_batch_dbg.scomp_block_copied_handles - __pre_scomp_block_copied_handles;
            __rec.scompElementwiseEmittedHandles = g_batch_dbg.scomp_elementwise_emitted_handles - __pre_scomp_elementwise_emitted_handles;
            __rec.scompSuffixOnlyCalls = g_batch_dbg.scomp_suffix_only_calls - __pre_scomp_suffix_only_calls;
            __rec.scompSingleMiddleRunCalls = g_batch_dbg.scomp_single_middle_run_calls - __pre_scomp_single_middle_run_calls;
            __rec.scompScratchCapacityReuseCalls = g_batch_dbg.scomp_scratch_capacity_reuse_calls - __pre_scomp_scratch_capacity_reuse_calls;
            long long __bcopy_route_baseline = g_batch_dbg.time_bcopy_route_baseline_ns - __pre_time_bcopy_route_baseline_ns;
            long long __bcopy_route_delta = g_batch_dbg.time_bcopy_route_delta_preserved_then_skeleton_ns - __pre_time_bcopy_route_delta_preserved_then_skeleton_ns;
            long long __bcopy_route_conn = g_batch_dbg.time_bcopy_route_connector_skeleton_ns - __pre_time_bcopy_route_connector_skeleton_ns;
            long long __bcopy_route_general = g_batch_dbg.time_bcopy_route_general_delta_ns - __pre_time_bcopy_route_general_delta_ns;
            __rec.bcopyRouteTag = REUSE_ROUTE_NONE;
            long long __best_bcopy_route_ns = 0;
            if (__bcopy_route_baseline > __best_bcopy_route_ns) { __best_bcopy_route_ns = __bcopy_route_baseline; __rec.bcopyRouteTag = REUSE_ROUTE_BASELINE; }
            if (__bcopy_route_delta > __best_bcopy_route_ns) { __best_bcopy_route_ns = __bcopy_route_delta; __rec.bcopyRouteTag = REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON; }
            if (__bcopy_route_conn > __best_bcopy_route_ns) { __best_bcopy_route_ns = __bcopy_route_conn; __rec.bcopyRouteTag = REUSE_ROUTE_CONNECTOR_SKELETON; }
            if (__bcopy_route_general > __best_bcopy_route_ns) { __best_bcopy_route_ns = __bcopy_route_general; __rec.bcopyRouteTag = REUSE_ROUTE_GENERAL_DELTA; }
            __rec.bcopySingleMiddleRunCalls = g_batch_dbg.bcopy_single_middle_run_calls - __pre_bcopy_single_middle_run_calls;
            __rec.bcopyRemovedRunCount = g_batch_dbg.bcopy_removed_run_count_sum - __pre_bcopy_removed_run_count_sum;
            __rec.bcopyKeptRunCount = g_batch_dbg.bcopy_kept_run_count_sum - __pre_bcopy_kept_run_count_sum;
            __rec.bcopyCopyPlanEntries = g_batch_dbg.bcopy_copy_plan_entries - __pre_bcopy_copy_plan_entries;
            __rec.bcopyDirectMemmoveCalls = g_batch_dbg.bcopy_direct_memmove_calls - __pre_bcopy_direct_memmove_calls;
            __rec.bcopyDirectMemmovedHandles = g_batch_dbg.bcopy_direct_memmoved_handles - __pre_bcopy_direct_memmoved_handles;
            __rec.bcopyBlockCopiedHandles = g_batch_dbg.bcopy_runwise_block_copied_handles - __pre_bcopy_runwise_block_copied_handles;
            __rec.bcopyElementwiseFallbackHandles = g_batch_dbg.bcopy_elementwise_fallback_handles - __pre_bcopy_elementwise_fallback_handles;
            __rec.bcopyOverlapStagingCalls = g_batch_dbg.bcopy_overlap_staging_calls - __pre_bcopy_overlap_staging_calls;
            long long __plan_route_baseline = g_batch_dbg.time_plan_route_baseline_ns - __pre_time_plan_route_baseline_ns;
            long long __plan_route_delta = g_batch_dbg.time_plan_route_delta_preserved_then_skeleton_ns - __pre_time_plan_route_delta_preserved_then_skeleton_ns;
            long long __plan_route_conn = g_batch_dbg.time_plan_route_connector_skeleton_ns - __pre_time_plan_route_connector_skeleton_ns;
            long long __plan_route_general = g_batch_dbg.time_plan_route_general_delta_ns - __pre_time_plan_route_general_delta_ns;
            __rec.planRouteTag = REUSE_ROUTE_NONE;
            long long __best_plan_route_ns = 0;
            if (__plan_route_baseline > __best_plan_route_ns) { __best_plan_route_ns = __plan_route_baseline; __rec.planRouteTag = REUSE_ROUTE_BASELINE; }
            if (__plan_route_delta > __best_plan_route_ns) { __best_plan_route_ns = __plan_route_delta; __rec.planRouteTag = REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON; }
            if (__plan_route_conn > __best_plan_route_ns) { __best_plan_route_ns = __plan_route_conn; __rec.planRouteTag = REUSE_ROUTE_CONNECTOR_SKELETON; }
            if (__plan_route_general > __best_plan_route_ns) { __best_plan_route_ns = __plan_route_general; __rec.planRouteTag = REUSE_ROUTE_GENERAL_DELTA; }
            __rec.planFirstRemovedIndex = g_batch_dbg.plan_first_removed_index_sum - __pre_plan_first_removed_index_sum;
            __rec.planRemovedRunCount = g_batch_dbg.plan_removed_run_count_sum - __pre_plan_removed_run_count_sum;
            __rec.planKeptRunCount = g_batch_dbg.plan_kept_run_count_sum - __pre_plan_kept_run_count_sum;
            __rec.planAdjacentMergeHits = g_batch_dbg.plan_adjacent_merge_hits - __pre_plan_adjacent_merge_hits;
            __rec.planDescriptorCount = g_batch_dbg.plan_descriptor_count - __pre_plan_descriptor_count;
            __rec.planDstIndexUpdates = g_batch_dbg.plan_dst_index_updates - __pre_plan_dst_index_updates;
            __rec.planSingleMiddleShortcircuitHits = g_batch_dbg.plan_single_middle_shortcircuit_hits - __pre_plan_single_middle_shortcircuit_hits;
            __rec.planSmallInlineHits = g_batch_dbg.plan_small_inline_hits - __pre_plan_small_inline_hits;
            long long __rdisc_route_baseline = g_batch_dbg.time_rdisc_route_baseline_ns - __pre_time_rdisc_route_baseline_ns;
            long long __rdisc_route_delta = g_batch_dbg.time_rdisc_route_delta_preserved_then_skeleton_ns - __pre_time_rdisc_route_delta_preserved_then_skeleton_ns;
            long long __rdisc_route_conn = g_batch_dbg.time_rdisc_route_connector_skeleton_ns - __pre_time_rdisc_route_connector_skeleton_ns;
            long long __rdisc_route_general = g_batch_dbg.time_rdisc_route_general_delta_ns - __pre_time_rdisc_route_general_delta_ns;
            __rec.rdiscRouteTag = REUSE_ROUTE_NONE;
            long long __best_rdisc_route_ns = 0;
            if (__rdisc_route_baseline > __best_rdisc_route_ns) { __best_rdisc_route_ns = __rdisc_route_baseline; __rec.rdiscRouteTag = REUSE_ROUTE_BASELINE; }
            if (__rdisc_route_delta > __best_rdisc_route_ns) { __best_rdisc_route_ns = __rdisc_route_delta; __rec.rdiscRouteTag = REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON; }
            if (__rdisc_route_conn > __best_rdisc_route_ns) { __best_rdisc_route_ns = __rdisc_route_conn; __rec.rdiscRouteTag = REUSE_ROUTE_CONNECTOR_SKELETON; }
            if (__rdisc_route_general > __best_rdisc_route_ns) { __best_rdisc_route_ns = __rdisc_route_general; __rec.rdiscRouteTag = REUSE_ROUTE_GENERAL_DELTA; }
            __rec.rdiscFirstRemovedIndex = g_batch_dbg.rdisc_first_removed_index_sum - __pre_rdisc_first_removed_index_sum;
            __rec.rdiscRemovedRunCount = g_batch_dbg.rdisc_removed_run_count_sum - __pre_rdisc_removed_run_count_sum;
            __rec.rdiscKeptRunCount = g_batch_dbg.rdisc_kept_run_count_sum - __pre_rdisc_kept_run_count_sum;
            __rec.rdiscBoundaryReuseHits = g_batch_dbg.rdisc_boundary_reuse_hits - __pre_rdisc_boundary_reuse_hits;
            __rec.rdiscSuffixOnlyHits = g_batch_dbg.rdisc_suffix_only_hits - __pre_rdisc_suffix_only_hits;
            __rec.rdiscSingleMiddleHits = g_batch_dbg.rdisc_single_middle_hits - __pre_rdisc_single_middle_hits;
            __rec.rdiscFusedOnepassCalls = g_batch_dbg.rdisc_fused_onepass_calls - __pre_rdisc_fused_onepass_calls;
            __rec.rdiscSmallRunlistInlineHits = g_batch_dbg.rdisc_small_runlist_inline_hits - __pre_rdisc_small_runlist_inline_hits;
            __rec.timeRdiscTotalNs = (g_batch_dbg.time_rdisc_first_removed_seek_ns - __pre_time_rdisc_first_removed_seek_ns) +
                                     (g_batch_dbg.time_rdisc_boundary_reuse_check_ns - __pre_time_rdisc_boundary_reuse_check_ns) +
                                     (g_batch_dbg.time_rdisc_removed_run_scan_ns - __pre_time_rdisc_removed_run_scan_ns) +
                                     (g_batch_dbg.time_rdisc_kept_run_scan_ns - __pre_time_rdisc_kept_run_scan_ns) +
                                     (g_batch_dbg.time_rdisc_suffix_only_shortcircuit_ns - __pre_time_rdisc_suffix_only_shortcircuit_ns) +
                                     (g_batch_dbg.time_rdisc_single_middle_shortcircuit_ns - __pre_time_rdisc_single_middle_shortcircuit_ns) +
                                     (g_batch_dbg.time_rdisc_fused_onepass_scan_ns - __pre_time_rdisc_fused_onepass_scan_ns) +
                                     (g_batch_dbg.time_rdisc_small_runlist_inline_materialize_ns - __pre_time_rdisc_small_runlist_inline_materialize_ns);
            long long __fclass_route_baseline = g_batch_dbg.time_fclass_route_baseline_ns - __pre_time_fclass_route_baseline_ns;
            long long __fclass_route_delta = g_batch_dbg.time_fclass_route_delta_preserved_then_skeleton_ns - __pre_time_fclass_route_delta_preserved_then_skeleton_ns;
            long long __fclass_route_conn = g_batch_dbg.time_fclass_route_connector_skeleton_ns - __pre_time_fclass_route_connector_skeleton_ns;
            long long __fclass_route_general = g_batch_dbg.time_fclass_route_general_delta_ns - __pre_time_fclass_route_general_delta_ns;
            __rec.fclassRouteTag = REUSE_ROUTE_NONE;
            long long __best_fclass_route_ns = 0;
            if (__fclass_route_baseline > __best_fclass_route_ns) { __best_fclass_route_ns = __fclass_route_baseline; __rec.fclassRouteTag = REUSE_ROUTE_BASELINE; }
            if (__fclass_route_delta > __best_fclass_route_ns) { __best_fclass_route_ns = __fclass_route_delta; __rec.fclassRouteTag = REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON; }
            if (__fclass_route_conn > __best_fclass_route_ns) { __best_fclass_route_ns = __fclass_route_conn; __rec.fclassRouteTag = REUSE_ROUTE_CONNECTOR_SKELETON; }
            if (__fclass_route_general > __best_fclass_route_ns) { __best_fclass_route_ns = __fclass_route_general; __rec.fclassRouteTag = REUSE_ROUTE_GENERAL_DELTA; }
            __rec.fclassSuffixOnlyHits = g_batch_dbg.fclass_suffix_only_hits - __pre_fclass_suffix_only_hits;
            __rec.fclassSingleMiddleHits = g_batch_dbg.fclass_single_middle_hits - __pre_fclass_single_middle_hits;
            __rec.fclassFusedOnepassCalls = g_batch_dbg.fclass_fused_onepass_calls - __pre_fclass_fused_onepass_calls;
            __rec.fclassTransitionSteps = g_batch_dbg.fclass_transition_steps - __pre_fclass_transition_steps;
            __rec.fclassRemovedToKeptTransitions = g_batch_dbg.fclass_removed_to_kept_transitions - __pre_fclass_removed_to_kept_transitions;
            __rec.fclassKeptToRemovedTransitions = g_batch_dbg.fclass_kept_to_removed_transitions - __pre_fclass_kept_to_removed_transitions;
            __rec.fclassSmallInlineHits = g_batch_dbg.fclass_small_inline_hits - __pre_fclass_small_inline_hits;
            __rec.timeFclassTotalNs = (g_batch_dbg.time_fclass_suffix_only_gate_ns - __pre_time_fclass_suffix_only_gate_ns) +
                                      (g_batch_dbg.time_fclass_single_middle_gate_ns - __pre_time_fclass_single_middle_gate_ns) +
                                      (g_batch_dbg.time_fclass_onepass_transition_scan_ns - __pre_time_fclass_onepass_transition_scan_ns) +
                                      (g_batch_dbg.time_fclass_transition_emit_runs_ns - __pre_time_fclass_transition_emit_runs_ns) +
                                      (g_batch_dbg.time_fclass_run_count_finalize_ns - __pre_time_fclass_run_count_finalize_ns) +
                                      (g_batch_dbg.time_fclass_small_runlist_inline_ns - __pre_time_fclass_small_runlist_inline_ns);
            __rec.timePlanTotalNs = (g_batch_dbg.time_plan_first_removed_seek_ns - __pre_time_plan_first_removed_seek_ns) +
                                    (g_batch_dbg.time_plan_removed_run_discovery_ns - __pre_time_plan_removed_run_discovery_ns) +
                                    (g_batch_dbg.time_plan_kept_run_discovery_ns - __pre_time_plan_kept_run_discovery_ns) +
                                    (g_batch_dbg.time_plan_adjacent_run_coalesce_ns - __pre_time_plan_adjacent_run_coalesce_ns) +
                                    (g_batch_dbg.time_plan_single_middle_shortcircuit_eligibility_ns - __pre_time_plan_single_middle_shortcircuit_eligibility_ns) +
                                    (g_batch_dbg.time_plan_dst_index_accumulate_ns - __pre_time_plan_dst_index_accumulate_ns) +
                                    (g_batch_dbg.time_plan_descriptor_emit_ns - __pre_time_plan_descriptor_emit_ns) +
                                    (g_batch_dbg.time_plan_small_inline_buffer_prepare_ns - __pre_time_plan_small_inline_buffer_prepare_ns);
            __rec.timeBcopyTotalNs = (g_batch_dbg.time_bcopy_single_middle_run_detect_ns - __pre_time_bcopy_single_middle_run_detect_ns) +
                                     (g_batch_dbg.time_bcopy_run_coalesce_build_ns - __pre_time_bcopy_run_coalesce_build_ns) +
                                     (g_batch_dbg.time_bcopy_direct_suffix_memmove_ns - __pre_time_bcopy_direct_suffix_memmove_ns) +
                                     (g_batch_dbg.time_bcopy_multi_run_block_copy_ns - __pre_time_bcopy_multi_run_block_copy_ns) +
                                     (g_batch_dbg.time_bcopy_short_fragment_elementwise_fallback_ns - __pre_time_bcopy_short_fragment_elementwise_fallback_ns) +
                                     (g_batch_dbg.time_bcopy_overlap_safe_staging_ns - __pre_time_bcopy_overlap_safe_staging_ns);
            __rec.timeScompTotalNs = (g_batch_dbg.time_scomp_first_removed_seek_ns - __pre_time_scomp_first_removed_seek_ns) +
                                     (g_batch_dbg.time_scomp_suffix_only_check_ns - __pre_time_scomp_suffix_only_check_ns) +
                                     (g_batch_dbg.time_scomp_kept_count_scan_ns - __pre_time_scomp_kept_count_scan_ns) +
                                     (g_batch_dbg.time_scomp_kept_run_partition_build_ns - __pre_time_scomp_kept_run_partition_build_ns) +
                                     (g_batch_dbg.time_scomp_prefix_skip_ns - __pre_time_scomp_prefix_skip_ns) +
                                     (g_batch_dbg.time_scomp_contiguous_run_block_copy_ns - __pre_time_scomp_contiguous_run_block_copy_ns) +
                                     (g_batch_dbg.time_scomp_elementwise_emit_ns - __pre_time_scomp_elementwise_emit_ns) +
                                     (g_batch_dbg.time_scomp_scratch_prepare_ns - __pre_time_scomp_scratch_prepare_ns) +
                                     (g_batch_dbg.time_scomp_tail_cleanup_ns - __pre_time_scomp_tail_cleanup_ns) +
                                     (g_batch_dbg.time_scomp_final_resize_swap_ns - __pre_time_scomp_final_resize_swap_ns);
            __rec.timeKvecTotalNs = (g_batch_dbg.time_retain_kept_vector_build_ns - __pre_time_retain_kept_vector_build_ns) +
                                    (g_batch_dbg.time_retain_kept_handle_copy_ns - __pre_time_retain_kept_handle_copy_ns) +
                                    (g_batch_dbg.time_retain_kept_handleidx_patch_ns - __pre_time_retain_kept_handleidx_patch_ns) +
                                    (g_batch_dbg.time_retain_final_swap_state_update_ns - __pre_time_retain_final_swap_state_update_ns);
            __rec.timeRetainTotalNs = g_batch_dbg.time_reuse_watch_retain_ns - __pre_time_reuse_watch_retain_ns;
            __rec.timeWscanTotalNs = (g_batch_dbg.time_reuse_keepmask_scan_ns - __pre_time_reuse_keepmask_scan_ns) +
                                     (g_batch_dbg.time_reuse_watch_retain_ns - __pre_time_reuse_watch_retain_ns);
            record_slow_deletion(__rec);
        }
        if (should_emit_progress_checkpoint(currentDeleteStep_)) {
            emit_progress_checkpoint("deletion_checkpoint", currentDeleteStep_, x, __touched_class_count);
        }
        g_local_profile_current_delete_sampled = false;
#endif
    }
};

class OuterSolver {
public:
    int n = 0;
    vector<RawQuery> raw;
    vector<pair<int,int>> directPairs;
    vector<pair<int,int>> undirectedEdges;
    vector<BranchQuery> branchQueries;
    vector<vector<int>> ownedDirect;
    vector<int> indeg, bad, parent;
    vector<char> alive;
    vector<int> compParent;

    void preprocess(int N, const vector<RawQuery>& queries) {
        n = N; raw = queries;
        vector<pair<int,int>> rawDirect, rawUndirected;
        vector<tuple<int,int,int>> rawBranch;
        rawDirect.reserve(2*raw.size()); rawUndirected.reserve(2*raw.size()); rawBranch.reserve(raw.size());
        for (const auto& q : raw) {
            if (q.u != q.w) { rawDirect.push_back({q.w,q.u}); rawUndirected.push_back({min(q.w,q.u),max(q.w,q.u)}); }
            if (q.v != q.w) { rawDirect.push_back({q.w,q.v}); rawUndirected.push_back({min(q.w,q.v),max(q.w,q.v)}); }
            if (q.u != q.w && q.v != q.w) { int a=min(q.u,q.v), b=max(q.u,q.v); rawBranch.push_back({q.w,a,b}); }
        }
        sort(rawDirect.begin(), rawDirect.end()); rawDirect.erase(unique(rawDirect.begin(), rawDirect.end()), rawDirect.end());
        sort(rawUndirected.begin(), rawUndirected.end()); rawUndirected.erase(unique(rawUndirected.begin(), rawUndirected.end()), rawUndirected.end());
        sort(rawBranch.begin(), rawBranch.end());
        directPairs = rawDirect; undirectedEdges = rawUndirected; branchQueries.clear();
        for (int i=0;i<(int)rawBranch.size();) {
            int j=i+1; while (j<(int)rawBranch.size() && rawBranch[j]==rawBranch[i]) ++j;
            auto [owner,a,b]=rawBranch[i];
            branchQueries.push_back({owner,a,b,j-i});
            i=j;
        }
        ownedDirect.assign(n+1,{}); indeg.assign(n+1,0); bad.assign(n+1,0); parent.assign(n+1,-1); alive.assign(n+1,1); compParent.clear();
        for (auto [owner,to] : directPairs) { ownedDirect[owner].push_back(to); indeg[to]++; }
    }
    void ensureCompParent(int h) { if (h >= (int)compParent.size()) compParent.resize(h+1,-1); }
    vector<int> solveWithOracle(NBOracle& oracle) {
        oracle.init(n, undirectedEdges, branchQueries);
#ifdef LOCAL
        progress_init_done();
#endif
        for (int qid=0; qid<(int)branchQueries.size(); ++qid)
            if (oracle.isFailing(qid)) bad[branchQueries[qid].owner] += branchQueries[qid].multiplicity;
        auto tryPush = [&](int v, queue<int>& qu){ if (1<=v && v<=n && alive[v] && indeg[v]==0 && bad[v]==0) qu.push(v); };
        queue<int> qu;
        parent[1]=0; alive[1]=0;
        for (int to : ownedDirect[1]) indeg[to]--;
        vector<int> newComponents; vector<WitnessChange> changes;
        oracle.eraseVertex(1, newComponents, changes);
        for (const auto& ch : changes) if (ch.resolved) bad[branchQueries[ch.qid].owner] -= branchQueries[ch.qid].multiplicity;
        for (int h : oracle.listComponents()) { ensureCompParent(h); compParent[h]=1; }
        for (int v=2; v<=n; ++v) tryPush(v, qu);
        while (!qu.empty()) {
            int v = qu.front(); qu.pop();
            if (!alive[v] || indeg[v]!=0 || bad[v]!=0) continue;
            int c = oracle.comp(v); if (c<0) continue;
            ensureCompParent(c); parent[v] = compParent[c]; alive[v]=0;
            for (int to : ownedDirect[v]) { indeg[to]--; tryPush(to, qu); }
            oracle.eraseVertex(v, newComponents, changes);
            for (int h : newComponents) { ensureCompParent(h); compParent[h]=v; }
            for (const auto& ch : changes) if (ch.resolved) {
                bad[branchQueries[ch.qid].owner] -= branchQueries[ch.qid].multiplicity;
                tryPush(branchQueries[ch.qid].owner, qu);
            }
        }
        return parent;
    }
};

#ifdef LOCAL
static int lca_naive(int u, int v, const vector<int>& parent) {
    int n = (int)parent.size() - 1;
    vector<int> depth(n + 1, -1);
    depth[1] = 0;
    for (int i = 2; i <= n; ++i) {
        int x = i; vector<int> st;
        while (depth[x] == -1) { st.push_back(x); x = parent[x]; if (x < 0 || x > n) return -1; }
        int d = depth[x];
        while (!st.empty()) { int y = st.back(); st.pop_back(); depth[y] = ++d; }
    }
    int a=u,b=v;
    while (depth[a] > depth[b]) a = parent[a];
    while (depth[b] > depth[a]) b = parent[b];
    while (a != b) { a = parent[a]; b = parent[b]; }
    return a;
}
static bool is_valid_tree(const vector<int>& parent) {
    int n=(int)parent.size()-1;
    if(parent[1]!=0) return false;
    for(int i=2;i<=n;++i) if(parent[i]<1||parent[i]>n) return false;
    vector<int> vis(n+1,0);
    for(int i=1;i<=n;++i){ int x=i; while(x!=0&&!vis[x]){ vis[x]=i; x=parent[x]; } if(x!=0&&vis[x]==i) return false; }
    return true;
}
static bool verify_solution(int n,const vector<int>& parent,const vector<RawQuery>& queries){
    if((int)parent.size()!=n+1) return false;
    if(!is_valid_tree(parent)) return false;
    for(const auto& q:queries) if(lca_naive(q.u,q.v,parent)!=q.w) return false;
    return true;
}
static vector<RawQuery> random_queries_from_tree(mt19937& rng,int n,const vector<int>& parent){
    vector<int> depth(n+1,0); for(int i=2;i<=n;++i) depth[i]=depth[parent[i]]+1;
    auto lca=[&](int u,int v){ int a=u,b=v; while(depth[a]>depth[b]) a=parent[a]; while(depth[b]>depth[a]) b=parent[b]; while(a!=b){a=parent[a]; b=parent[b];} return a; };
    vector<RawQuery> qs; uniform_int_distribution<int> bit(0,99);
    for(int u=1;u<=n;++u) for(int v=u;v<=n;++v) if(bit(rng)<40) qs.push_back({u,v,lca(u,v)});
    if(qs.empty()) qs.push_back({1,1,1}); return qs;
}
static void self_test(){
    mt19937 rng(712367);
    for(int n=1;n<=10;++n){
        for(int it=0;it<200;++it){
            vector<int> parent(n+1,0);
            for(int v=2;v<=n;++v){ uniform_int_distribution<int> pick(1,v-1); parent[v]=pick(rng); }
            auto queries=random_queries_from_tree(rng,n,parent);
            OuterSolver solver; solver.preprocess(n,queries);
            LiteraturePotentialOracle oracle;
            auto out=solver.solveWithOracle(oracle);
            if(!verify_solution(n,out,queries)){
                cerr<<"LITERATURE PROGRESS SELF TEST FAILED\n";
                cerr<<"n="<<n<<"\n";
                for(auto &q:queries) cerr<<q.u<<' '<<q.v<<' '<<q.w<<'\n';
                cerr<<"parent:\n"; for(int i=1;i<=n;++i) cerr<<out[i]<<(i==n?'\n':' ');
                exit(1);
            }
        }
    }
    cerr<<"LITERATURE PROGRESS SELF TEST OK\n";
#ifdef LOCAL
    cerr << "owner_rebuild_calls=" << g_topo_dbg.dbg_owner_rebuild_calls
         << " local_updates=" << g_topo_dbg.dbg_owner_local_updates
         << " local_fallback=" << g_topo_dbg.dbg_owner_local_updates_fallback
         << " partition_mismatch=" << g_topo_dbg.dbg_endpoint_partition_mismatch << "\n";
    cerr << "fallback_breakdown deleted_owner=" << g_topo_dbg.dbg_fallback_deleted_owner
         << " multi_old_class_touch=" << g_topo_dbg.dbg_fallback_multi_old_class_touch
         << " relabel_collision=" << g_topo_dbg.dbg_fallback_relabel_collision
         << " endpoint_outside_zone=" << g_topo_dbg.dbg_fallback_endpoint_outside_zone
         << " merge_ambiguous=" << g_topo_dbg.dbg_fallback_component_merge_ambiguous << "\n";
    cerr << "topology_zone_bfs V=" << g_topo_dbg.topology_zone_bfs_vertices
         << " E=" << g_topo_dbg.topology_zone_bfs_edges << "\n";
    cerr << "global_delete_dfs calls=" << g_topo_dbg.global_delete_dfs_calls
         << " V=" << g_topo_dbg.global_delete_dfs_vertices
         << " E=" << g_topo_dbg.global_delete_dfs_edges
         << " comps=" << g_topo_dbg.global_delete_component_count << "\n";
    cerr << "owner_bucket_assignments=" << g_topo_dbg.owner_bucket_assignments
         << " owner_bucket_binary_search_steps=" << g_topo_dbg.owner_bucket_binary_search_steps
         << " owner_relabel_calls=" << g_topo_dbg.owner_relabel_calls
         << " owner_relabel_active_endpoints=" << g_topo_dbg.owner_relabel_active_endpoints
         << " owner_relabel_moved_endpoints=" << g_topo_dbg.owner_relabel_moved_endpoints
         << " owner_relabel_candidate_classes=" << g_topo_dbg.owner_relabel_candidate_classes
         << " class_local_refine_calls=" << g_topo_dbg.class_local_refine_calls
         << " class_local_refine_endpoints=" << g_topo_dbg.class_local_refine_endpoints
         << " class_local_refine_moved_endpoints=" << g_topo_dbg.class_local_refine_moved_endpoints
         << " class_local_new_class_count=" << g_topo_dbg.class_local_new_class_count
         << " class_local_kept_old_cid_count=" << g_topo_dbg.class_local_kept_old_cid_count
         << " untouched_class_skips=" << g_topo_dbg.untouched_class_skips
         << " owner_wide_relabel_calls=" << g_topo_dbg.owner_wide_relabel_calls
         << " owner_wide_relabel_endpoints=" << g_topo_dbg.owner_wide_relabel_endpoints
         << " topo_active_endpoint_total=" << g_topo_dbg.topo_active_endpoint_total
         << " topo_active_endpoint_peak=" << g_topo_dbg.topo_active_endpoint_peak
         << " topo_deactivated_endpoint_count=" << g_topo_dbg.topo_deactivated_endpoint_count << "\n";
    cerr << "strict_child_found=" << g_strict_child_dbg.strict_child_found
         << " strict_child_exists_but_missed=" << g_strict_child_dbg.strict_child_exists_but_missed
         << " strict_child_structural_miss=" << g_strict_child_dbg.strict_child_structural_miss
         << " semantic_escape_count=" << g_strict_child_dbg.semantic_escape_count
         << " strict_child_rebuild_used=" << g_strict_child_dbg.strict_child_rebuild_used
         << " strict_child_global_fallback_used=" << g_strict_child_dbg.strict_child_global_fallback_used
         << "\n";
    cerr << "strict_child_depth_sum=" << g_strict_child_dbg.strict_child_depth_sum
         << " strict_child_rebuild V=" << g_strict_child_dbg.strict_child_rebuild_vertices
         << " E=" << g_strict_child_dbg.strict_child_rebuild_edges << "\n";
    cerr << "owner_support_build_calls=" << g_batch_dbg.owner_support_build_calls
         << " owner_support_build_V=" << g_batch_dbg.owner_support_build_vertices
         << " owner_support_build_E=" << g_batch_dbg.owner_support_build_edges
         << " owner_support_relevant_ep_sum=" << g_batch_dbg.owner_support_relevant_endpoints_sum
         << " owner_support_watch_vertices_sum=" << g_batch_dbg.owner_support_watch_vertices_sum
         << "\n";
    cerr << "owner_touched_by_watch=" << g_batch_dbg.owner_touched_by_watch
         << " owner_touched_unique=" << g_batch_dbg.owner_touched_unique
         << " class_split_events=" << g_batch_dbg.class_split_events
         << " moved_endpoint_count=" << g_batch_dbg.moved_endpoint_count
         << " query_incident_scans=" << g_batch_dbg.query_incident_scans
         << " query_resolved_by_split=" << g_batch_dbg.query_resolved_by_split
         << " query_resolved_owner_dead_or_endpoint_dead=" << g_batch_dbg.query_resolved_owner_dead_or_endpoint_dead
         << " active_query_peak=" << g_batch_dbg.active_query_peak
         << " support_watch_peak=" << g_batch_dbg.support_watch_peak
         << " local_active_mismatch=" << g_batch_dbg.local_active_mismatch
         << " local_active_partition_mismatch=" << g_batch_dbg.local_active_partition_mismatch
         << " debug_touched_missing_classes=" << g_batch_dbg.debug_touched_missing_classes
         << " debug_touched_extra_classes=" << g_batch_dbg.debug_touched_extra_classes
         << " debug_touched_missing_classes=" << g_batch_dbg.debug_touched_missing_classes
         << " debug_touched_extra_classes=" << g_batch_dbg.debug_touched_extra_classes
         << " support_build_failures=" << g_batch_dbg.support_build_failures
         << "\n";
    cerr << "support_rebuild_artifact_calls=" << g_batch_dbg.support_rebuild_artifact_calls
         << " support_rebuild_artifact_vertices=" << g_batch_dbg.support_rebuild_artifact_vertices
         << " support_rebuild_artifact_chain_steps=" << g_batch_dbg.support_rebuild_artifact_chain_steps
         << " support_rebuild_fallback_calls=" << g_batch_dbg.support_rebuild_fallback_calls
         << " support_rebuild_fallback_vertices=" << g_batch_dbg.support_rebuild_fallback_vertices
         << " support_rebuild_fallback_edges=" << g_batch_dbg.support_rebuild_fallback_edges
         << " watch_register_vertices=" << g_batch_dbg.watch_register_vertices
         << " watch_unregister_vertices=" << g_batch_dbg.watch_unregister_vertices
         << " watch_live_entries_peak=" << g_batch_dbg.watch_live_entries_peak
         << " watch_stale_drops=" << g_batch_dbg.watch_stale_drops
         << "\n";
    cerr << "touched_class_total=" << g_batch_dbg.touched_class_total
         << " support_positive_component_total=" << g_batch_dbg.support_positive_component_total
         << " skip_by_single_positive_component=" << g_batch_dbg.skip_by_single_positive_component
         << " skip_by_rep_bucket_unanimous=" << g_batch_dbg.skip_by_rep_bucket_unanimous
         << " split_required_class_count=" << g_batch_dbg.split_required_class_count
         << " rep_bucket_checks=" << g_batch_dbg.rep_bucket_checks
         << " moved_endpoint_enumerations=" << g_batch_dbg.moved_endpoint_enumerations
         << " moved_endpoint_total=" << g_batch_dbg.moved_endpoint_total
         << " largest_bucket_kept_count=" << g_batch_dbg.largest_bucket_kept_count
         << " class_local_fullscan_calls=" << g_batch_dbg.class_local_fullscan_calls
         << " class_local_fullscan_endpoints=" << g_batch_dbg.class_local_fullscan_endpoints
         << " fullscan_bad_meta=" << g_batch_dbg.fullscan_bad_meta
         << " fullscan_bad_xpos=" << g_batch_dbg.fullscan_bad_xpos
         << " fullscan_bad_ctx=" << g_batch_dbg.fullscan_bad_ctx
         << " fullscan_bad_rep=" << g_batch_dbg.fullscan_bad_rep
         << " support_meta_build_ok=" << g_batch_dbg.support_meta_build_ok
         << " support_meta_fail_artifact_stamp=" << g_batch_dbg.support_meta_fail_artifact_stamp
         << " support_meta_fail_fallback_stamp=" << g_batch_dbg.support_meta_fail_fallback_stamp
         << " support_meta_fail_root=" << g_batch_dbg.support_meta_fail_root
         << "\n";
    cerr << "support_meta_build_calls=" << g_batch_dbg.support_meta_build_calls
         << " support_meta_build_watch_vertices=" << g_batch_dbg.support_meta_build_watch_vertices
         << " support_meta_build_relevant_endpoints=" << g_batch_dbg.support_meta_build_relevant_endpoints
         << " support_meta_hash_pos_build_items=" << g_batch_dbg.support_meta_hash_pos_build_items
         << " support_meta_graph_recover_calls=" << g_batch_dbg.support_meta_graph_recover_calls
         << " support_meta_graph_recover_edges=" << g_batch_dbg.support_meta_graph_recover_edges
         << " support_meta_endpoint_sort_calls=" << g_batch_dbg.support_meta_endpoint_sort_calls
         << " support_meta_endpoint_sort_items=" << g_batch_dbg.support_meta_endpoint_sort_items
         << " support_meta_from_collector_calls=" << g_batch_dbg.support_meta_from_collector_calls
         << " support_meta_from_collector_watch_vertices=" << g_batch_dbg.support_meta_from_collector_watch_vertices
         << " support_meta_from_collector_relevant_endpoints=" << g_batch_dbg.support_meta_from_collector_relevant_endpoints
         << "\n";
    cerr << "support_full_rebuild_calls=" << g_batch_dbg.support_full_rebuild_calls
         << " support_full_rebuild_watch_vertices=" << g_batch_dbg.support_full_rebuild_watch_vertices
         << " support_reuse_single_calls=" << g_batch_dbg.support_reuse_single_calls
         << " support_reuse_single_watch_vertices_kept=" << g_batch_dbg.support_reuse_single_watch_vertices_kept
         << " support_reuse_single_watch_vertices_removed=" << g_batch_dbg.support_reuse_single_watch_vertices_removed
         << " support_reuse_unanimous_calls=" << g_batch_dbg.support_reuse_unanimous_calls
         << " support_reuse_unanimous_components=" << g_batch_dbg.support_reuse_unanimous_components
         << " support_reuse_unanimous_reps=" << g_batch_dbg.support_reuse_unanimous_reps
         << " support_reuse_unanimous_connector_calls=" << g_batch_dbg.support_reuse_unanimous_connector_calls
         << " support_reuse_unanimous_connector_vertices=" << g_batch_dbg.support_reuse_unanimous_connector_vertices
         << " support_reuse_unanimous_watch_vertices_kept=" << g_batch_dbg.support_reuse_unanimous_watch_vertices_kept
         << " support_reuse_unanimous_watch_vertices_added=" << g_batch_dbg.support_reuse_unanimous_watch_vertices_added
         << " support_reuse_unanimous_watch_vertices_removed=" << g_batch_dbg.support_reuse_unanimous_watch_vertices_removed
         << " support_merged_metadata_calls=" << g_batch_dbg.support_merged_metadata_calls
         << " support_merged_metadata_vertices=" << g_batch_dbg.support_merged_metadata_vertices
         << "\n";
    cerr << "piece_shadow_skip_classes_total=" << g_batch_dbg.piece_shadow_skip_classes_total
         << " piece_shadow_single_positive_classes=" << g_batch_dbg.piece_shadow_single_positive_classes
         << " piece_shadow_unanimous_classes=" << g_batch_dbg.piece_shadow_unanimous_classes
         << " piece_shadow_split_classes=" << g_batch_dbg.piece_shadow_split_classes
         << " piece_shadow_current_materialize_vertices=" << g_batch_dbg.piece_shadow_current_materialize_vertices
         << " piece_shadow_current_watch_unregister_vertices=" << g_batch_dbg.piece_shadow_current_watch_unregister_vertices
         << " piece_shadow_current_watch_register_vertices=" << g_batch_dbg.piece_shadow_current_watch_register_vertices
         << " piece_shadow_candidate_piece_reused_vertices=" << g_batch_dbg.piece_shadow_candidate_piece_reused_vertices
         << " piece_shadow_candidate_piece_removed_vertices=" << g_batch_dbg.piece_shadow_candidate_piece_removed_vertices
         << " piece_shadow_candidate_connector_vertices=" << g_batch_dbg.piece_shadow_candidate_connector_vertices
         << " piece_shadow_candidate_boundary_ops=" << g_batch_dbg.piece_shadow_candidate_boundary_ops
         << " piece_shadow_candidate_piece_count=" << g_batch_dbg.piece_shadow_candidate_piece_count
         << " piece_shadow_candidate_parent_side_positive_cases=" << g_batch_dbg.piece_shadow_candidate_parent_side_positive_cases
         << " piece_shadow_candidate_multi_piece_classes=" << g_batch_dbg.piece_shadow_candidate_multi_piece_classes
         << " piece_shadow_estimated_saved_vertices=" << g_batch_dbg.piece_shadow_estimated_saved_vertices
         << " piece_live_count=" << g_batch_dbg.piece_live_count
         << " piece_live_vertices=" << g_batch_dbg.piece_live_vertices
         << " piece_reuse_single_calls=" << g_batch_dbg.piece_reuse_single_calls
         << " piece_reuse_single_reused_vertices=" << g_batch_dbg.piece_reuse_single_reused_vertices
         << " piece_reuse_single_removed_vertices=" << g_batch_dbg.piece_reuse_single_removed_vertices
         << " piece_reuse_unanimous_calls=" << g_batch_dbg.piece_reuse_unanimous_calls
         << " piece_reuse_unanimous_reused_vertices=" << g_batch_dbg.piece_reuse_unanimous_reused_vertices
         << " piece_reuse_unanimous_removed_vertices=" << g_batch_dbg.piece_reuse_unanimous_removed_vertices
         << " piece_reuse_unanimous_added_connector_vertices=" << g_batch_dbg.piece_reuse_unanimous_added_connector_vertices
         << " piece_materialize_fallback_calls=" << g_batch_dbg.piece_materialize_fallback_calls
         << " piece_materialize_fallback_vertices=" << g_batch_dbg.piece_materialize_fallback_vertices
         << " piece_fallback_reason_need_support_meta=" << g_batch_dbg.piece_fallback_reason_need_support_meta
         << " piece_fallback_reason_bad_x_handle=" << g_batch_dbg.piece_fallback_reason_bad_x_handle
         << " piece_fallback_reason_preserved_piece_hit=" << g_batch_dbg.piece_fallback_reason_preserved_piece_hit
         << " piece_fallback_reason_connector_hit=" << g_batch_dbg.piece_fallback_reason_connector_hit
         << " piece_fallback_reason_split_required=" << g_batch_dbg.piece_fallback_reason_split_required
         << " piece_fallback_reason_other=" << g_batch_dbg.piece_fallback_reason_other
         << " piece_native_candidate_classes=" << g_batch_dbg.piece_native_candidate_classes
         << " piece_native_candidate_preserved_hits=" << g_batch_dbg.piece_native_candidate_preserved_hits
         << " piece_native_candidate_connector_hits=" << g_batch_dbg.piece_native_candidate_connector_hits
         << " piece_native_single_calls=" << g_batch_dbg.piece_native_single_calls
         << " piece_native_single_preserved_hits=" << g_batch_dbg.piece_native_single_preserved_hits
         << " piece_native_single_connector_hits=" << g_batch_dbg.piece_native_single_connector_hits
         << " piece_native_single_reused_vertices=" << g_batch_dbg.piece_native_single_reused_vertices
         << " piece_native_single_removed_vertices=" << g_batch_dbg.piece_native_single_removed_vertices
         << " piece_native_single_boundary_ops=" << g_batch_dbg.piece_native_single_boundary_ops
         << " piece_native_unanimous_calls=" << g_batch_dbg.piece_native_unanimous_calls
         << " piece_native_unanimous_preserved_hits=" << g_batch_dbg.piece_native_unanimous_preserved_hits
         << " piece_native_unanimous_connector_hits=" << g_batch_dbg.piece_native_unanimous_connector_hits
         << " piece_native_unanimous_reused_vertices=" << g_batch_dbg.piece_native_unanimous_reused_vertices
         << " piece_native_unanimous_removed_vertices=" << g_batch_dbg.piece_native_unanimous_removed_vertices
         << " piece_native_unanimous_added_connector_vertices=" << g_batch_dbg.piece_native_unanimous_added_connector_vertices
         << " piece_native_unanimous_boundary_ops=" << g_batch_dbg.piece_native_unanimous_boundary_ops
         << " connector_shadow_unanimous_classes=" << g_batch_dbg.connector_shadow_unanimous_classes
         << " connector_shadow_current_removed_vertices=" << g_batch_dbg.connector_shadow_current_removed_vertices
         << " connector_shadow_current_added_vertices=" << g_batch_dbg.connector_shadow_current_added_vertices
         << " connector_shadow_candidate_reused_connector_vertices=" << g_batch_dbg.connector_shadow_candidate_reused_connector_vertices
         << " connector_shadow_candidate_removed_connector_vertices=" << g_batch_dbg.connector_shadow_candidate_removed_connector_vertices
         << " connector_shadow_candidate_patch_vertices=" << g_batch_dbg.connector_shadow_candidate_patch_vertices
         << " connector_shadow_candidate_attachment_retargets=" << g_batch_dbg.connector_shadow_candidate_attachment_retargets
         << " connector_shadow_candidate_terminal_fragment_groups=" << g_batch_dbg.connector_shadow_candidate_terminal_fragment_groups
         << " connector_shadow_candidate_no_patch_needed=" << g_batch_dbg.connector_shadow_candidate_no_patch_needed
         << " connector_shadow_estimated_saved_vertices=" << g_batch_dbg.connector_shadow_estimated_saved_vertices
         << " connector_skeleton_shadow_classes=" << g_batch_dbg.connector_skeleton_shadow_classes
         << " connector_skeleton_shadow_current_removed_vertices=" << g_batch_dbg.connector_skeleton_shadow_current_removed_vertices
         << " connector_skeleton_shadow_current_added_vertices=" << g_batch_dbg.connector_skeleton_shadow_current_added_vertices
         << " connector_skeleton_shadow_candidate_terminals=" << g_batch_dbg.connector_skeleton_shadow_candidate_terminals
         << " connector_skeleton_shadow_candidate_vertices=" << g_batch_dbg.connector_skeleton_shadow_candidate_vertices
         << " connector_skeleton_shadow_candidate_watch_unregister=" << g_batch_dbg.connector_skeleton_shadow_candidate_watch_unregister
         << " connector_skeleton_shadow_candidate_watch_register=" << g_batch_dbg.connector_skeleton_shadow_candidate_watch_register
         << " connector_skeleton_shadow_candidate_no_rebuild_needed=" << g_batch_dbg.connector_skeleton_shadow_candidate_no_rebuild_needed
         << " connector_skeleton_shadow_estimated_saved_vertices=" << g_batch_dbg.connector_skeleton_shadow_estimated_saved_vertices
         << " connector_skeleton_old_vertices=" << g_batch_dbg.connector_skeleton_old_vertices
         << " connector_skeleton_new_vertices=" << g_batch_dbg.connector_skeleton_new_vertices
         << " connector_skeleton_common_vertices=" << g_batch_dbg.connector_skeleton_common_vertices
         << " connector_skeleton_added_vertices=" << g_batch_dbg.connector_skeleton_added_vertices
         << " connector_skeleton_removed_vertices=" << g_batch_dbg.connector_skeleton_removed_vertices
         << " connector_skeleton_intersection_ratio_permille=" << g_batch_dbg.connector_skeleton_intersection_ratio_permille
         << " connector_watch_full_unregister=" << g_batch_dbg.connector_watch_full_unregister
         << " connector_watch_full_register=" << g_batch_dbg.connector_watch_full_register
         << " connector_watch_diff_unregister=" << g_batch_dbg.connector_watch_diff_unregister
         << " connector_watch_diff_register=" << g_batch_dbg.connector_watch_diff_register
         << " connector_watch_diff_reused=" << g_batch_dbg.connector_watch_diff_reused
         << " connector_watch_diff_actual_calls=" << g_batch_dbg.connector_watch_diff_actual_calls
         << " connector_watch_diff_actual_reused=" << g_batch_dbg.connector_watch_diff_actual_reused
         << " connector_watch_diff_actual_removed=" << g_batch_dbg.connector_watch_diff_actual_removed
         << " connector_watch_diff_actual_added=" << g_batch_dbg.connector_watch_diff_actual_added
         << " connector_skeleton_actual_calls=" << g_batch_dbg.connector_skeleton_actual_calls
         << " connector_skeleton_actual_terminals=" << g_batch_dbg.connector_skeleton_actual_terminals
         << " connector_skeleton_actual_vertices=" << g_batch_dbg.connector_skeleton_actual_vertices
         << " connector_skeleton_actual_removed_old_connector_vertices=" << g_batch_dbg.connector_skeleton_actual_removed_old_connector_vertices
         << " connector_skeleton_actual_retargets=" << g_batch_dbg.connector_skeleton_actual_retargets
         << " connector_skeleton_candidate_classes=" << g_batch_dbg.connector_skeleton_candidate_classes
         << " connector_skeleton_selected_classes=" << g_batch_dbg.connector_skeleton_selected_classes
         << " connector_skeleton_selected_connector_only=" << g_batch_dbg.connector_skeleton_selected_connector_only
         << " connector_skeleton_selected_both_on=" << g_batch_dbg.connector_skeleton_selected_both_on
         << " connector_skeleton_forced_classes=" << g_batch_dbg.connector_skeleton_forced_classes
         << " connector_skeleton_reject_state_not_unanimous=" << g_batch_dbg.connector_skeleton_reject_state_not_unanimous
         << " connector_skeleton_reject_no_preserved_pieces=" << g_batch_dbg.connector_skeleton_reject_no_preserved_pieces
         << " connector_skeleton_reject_no_attachment_vertices=" << g_batch_dbg.connector_skeleton_reject_no_attachment_vertices
         << " connector_skeleton_reject_support_meta_valid=" << g_batch_dbg.connector_skeleton_reject_support_meta_valid
         << " connector_skeleton_reject_origin_kind=" << g_batch_dbg.connector_skeleton_reject_origin_kind
         << " connector_skeleton_reject_missing_tree=" << g_batch_dbg.connector_skeleton_reject_missing_tree
         << " connector_skeleton_reject_fallback_guard=" << g_batch_dbg.connector_skeleton_reject_fallback_guard
         << " connector_skeleton_reject_other=" << g_batch_dbg.connector_skeleton_reject_other
         << " unanimous_baseline_path_calls=" << g_batch_dbg.unanimous_baseline_path_calls
         << " unanimous_baseline_path_vertices=" << g_batch_dbg.unanimous_baseline_path_vertices
         << " debug_force_skeleton_calls=" << g_batch_dbg.debug_force_skeleton_calls
         << " debug_force_skeleton_reference_compare_calls=" << g_batch_dbg.debug_force_skeleton_reference_compare_calls
         << " debug_force_skeleton_divergence=" << g_batch_dbg.debug_force_skeleton_divergence
         << " debug_unanimous_state_old_field_read=" << g_batch_dbg.debug_unanimous_state_old_field_read
         << " debug_unanimous_state_new_field_read=" << g_batch_dbg.debug_unanimous_state_new_field_read
         << " connector_shadow_unanimous_classes=" << g_batch_dbg.connector_shadow_unanimous_classes
         << " connector_shadow_current_removed_vertices=" << g_batch_dbg.connector_shadow_current_removed_vertices
         << " connector_shadow_current_added_vertices=" << g_batch_dbg.connector_shadow_current_added_vertices
         << " connector_shadow_candidate_reused_connector_vertices=" << g_batch_dbg.connector_shadow_candidate_reused_connector_vertices
         << " connector_shadow_candidate_removed_connector_vertices=" << g_batch_dbg.connector_shadow_candidate_removed_connector_vertices
         << " connector_shadow_candidate_patch_vertices=" << g_batch_dbg.connector_shadow_candidate_patch_vertices
         << " connector_shadow_candidate_attachment_retargets=" << g_batch_dbg.connector_shadow_candidate_attachment_retargets
         << " connector_shadow_candidate_terminal_fragment_groups=" << g_batch_dbg.connector_shadow_candidate_terminal_fragment_groups
         << " connector_shadow_candidate_no_patch_needed=" << g_batch_dbg.connector_shadow_candidate_no_patch_needed
         << " connector_shadow_estimated_saved_vertices=" << g_batch_dbg.connector_shadow_estimated_saved_vertices
         << " both_snapshot_piece_contains_x=" << g_batch_dbg.debug_both_snapshot_piece_contains_x
         << " both_snapshot_piece_excludes_x_but_watch_contains_x=" << g_batch_dbg.debug_both_snapshot_piece_excludes_x_but_watch_contains_x
         << " both_snapshot_attachment_is_x=" << g_batch_dbg.debug_both_snapshot_attachment_is_x
         << " both_snapshot_attachment_outside_piece=" << g_batch_dbg.debug_both_snapshot_attachment_outside_piece
         << " both_snapshot_reused_same_piece_handle=" << g_batch_dbg.debug_both_snapshot_reused_same_piece_handle
         << " both_snapshot_other=" << g_batch_dbg.debug_both_snapshot_other
         << " post_piece_contains_x=" << g_batch_dbg.debug_postcondition_piece_contains_x
         << " post_attachment_is_x=" << g_batch_dbg.debug_postcondition_attachment_is_x
         << " post_watch_points_to_x=" << g_batch_dbg.debug_postcondition_watch_points_to_x
         << " forced_preserved_split_due_x_in_piece=" << g_batch_dbg.debug_forced_preserved_split_due_x_in_piece
         << " targeted_piece_watch_refresh_calls=" << g_batch_dbg.debug_targeted_piece_watch_refresh_calls
         << " targeted_piece_watch_refresh_removed=" << g_batch_dbg.debug_targeted_piece_watch_refresh_removed
         << " attachment_retarget_due_x=" << g_batch_dbg.debug_attachment_retarget_due_x
         << " attachment_retarget_due_outside_piece=" << g_batch_dbg.debug_attachment_retarget_due_outside_piece
         << "\n";
#endif
}
#endif

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
#ifdef LOCAL
    cerr << std::unitbuf;
    if (!local_env_enabled("LOCAL_SKIP_SELF_TEST", false)) self_test();
    g_strict_child_dbg = StrictChildDebugStats();
    g_topo_dbg = TopologyDebugStats();
    g_batch_dbg = BatchPivotDebugStats();
#endif
    int N,M; if(!(cin>>N>>M)) return 0;
    vector<RawQuery> queries(M);
    for(int i=0;i<M;++i) cin>>queries[i].u>>queries[i].v>>queries[i].w;
#ifdef LOCAL
    progress_case_start(N, M);
#endif
    OuterSolver solver; solver.preprocess(N,queries);
    LiteraturePotentialOracle oracle;
    auto parent = solver.solveWithOracle(oracle);
#ifdef LOCAL
    emit_progress_checkpoint("summary", g_batch_dbg.debug_profile_total_deletions, -1, -1);
#endif
    for(int i=1;i<=N;++i){ if(i>1) cout << ' '; cout << parent[i]; }
    cout << '\n';
#ifdef LOCAL
    cerr << "owner_rebuild_calls=" << g_topo_dbg.dbg_owner_rebuild_calls
         << " local_updates=" << g_topo_dbg.dbg_owner_local_updates
         << " local_fallback=" << g_topo_dbg.dbg_owner_local_updates_fallback
         << " partition_mismatch=" << g_topo_dbg.dbg_endpoint_partition_mismatch << "\n";
    cerr << "topology_zone_bfs V=" << g_topo_dbg.topology_zone_bfs_vertices
         << " E=" << g_topo_dbg.topology_zone_bfs_edges << "\n";
    cerr << "global_delete_dfs calls=" << g_topo_dbg.global_delete_dfs_calls
         << " V=" << g_topo_dbg.global_delete_dfs_vertices
         << " E=" << g_topo_dbg.global_delete_dfs_edges
         << " comps=" << g_topo_dbg.global_delete_component_count << "\n";
    cerr << "owner_bucket_assignments=" << g_topo_dbg.owner_bucket_assignments
         << " owner_bucket_binary_search_steps=" << g_topo_dbg.owner_bucket_binary_search_steps
         << " owner_relabel_calls=" << g_topo_dbg.owner_relabel_calls
         << " owner_relabel_active_endpoints=" << g_topo_dbg.owner_relabel_active_endpoints
         << " owner_relabel_moved_endpoints=" << g_topo_dbg.owner_relabel_moved_endpoints
         << " owner_relabel_candidate_classes=" << g_topo_dbg.owner_relabel_candidate_classes
         << " class_local_refine_calls=" << g_topo_dbg.class_local_refine_calls
         << " class_local_refine_endpoints=" << g_topo_dbg.class_local_refine_endpoints
         << " class_local_refine_moved_endpoints=" << g_topo_dbg.class_local_refine_moved_endpoints
         << " class_local_new_class_count=" << g_topo_dbg.class_local_new_class_count
         << " class_local_kept_old_cid_count=" << g_topo_dbg.class_local_kept_old_cid_count
         << " untouched_class_skips=" << g_topo_dbg.untouched_class_skips
         << " owner_wide_relabel_calls=" << g_topo_dbg.owner_wide_relabel_calls
         << " owner_wide_relabel_endpoints=" << g_topo_dbg.owner_wide_relabel_endpoints
         << " topo_active_endpoint_total=" << g_topo_dbg.topo_active_endpoint_total
         << " topo_active_endpoint_peak=" << g_topo_dbg.topo_active_endpoint_peak
         << " topo_deactivated_endpoint_count=" << g_topo_dbg.topo_deactivated_endpoint_count << "\n";
    cerr << "strict_child_found=" << g_strict_child_dbg.strict_child_found
         << " strict_child_exists_but_missed=" << g_strict_child_dbg.strict_child_exists_but_missed
         << " strict_child_structural_miss=" << g_strict_child_dbg.strict_child_structural_miss
         << " semantic_escape_count=" << g_strict_child_dbg.semantic_escape_count
         << " strict_child_rebuild_used=" << g_strict_child_dbg.strict_child_rebuild_used
         << " strict_child_global_fallback_used=" << g_strict_child_dbg.strict_child_global_fallback_used
         << " cert_untouched_fast_keep=" << g_strict_child_dbg.cert_untouched_fast_keep
         << "\n";
    cerr << "buildExactRestricted_calls=" << g_strict_child_dbg.build_exact_restricted_calls
         << " buildExactRestricted_V=" << g_strict_child_dbg.build_exact_restricted_vertices
         << " buildExactRestricted_E=" << g_strict_child_dbg.build_exact_restricted_edges
         << " fastRestricted_calls=" << g_strict_child_dbg.fast_restricted_search_calls
         << " fastRestricted_V=" << g_strict_child_dbg.fast_restricted_search_vertices
         << " fastRestricted_E=" << g_strict_child_dbg.fast_restricted_search_edges
         << "\n";
    cerr << "try_build_child_calls=" << g_strict_child_dbg.try_build_child_calls
         << " try_build_child_success=" << g_strict_child_dbg.try_build_child_success
         << " same_base_relocation_count=" << g_strict_child_dbg.same_base_relocation_count
         << " proper_child_relocation_count=" << g_strict_child_dbg.proper_child_relocation_count
         << "\n";
    double avgRegionBefore = g_strict_child_dbg.region_stats_count ? (double)g_strict_child_dbg.region_size_before_sum / g_strict_child_dbg.region_stats_count : 0.0;
    double avgRegionAfter = g_strict_child_dbg.region_stats_count ? (double)g_strict_child_dbg.region_size_after_sum / g_strict_child_dbg.region_stats_count : 0.0;
    double avgCertBefore = g_strict_child_dbg.region_stats_count ? (double)g_strict_child_dbg.cert_size_before_sum / g_strict_child_dbg.region_stats_count : 0.0;
    double avgCertAfter = g_strict_child_dbg.region_stats_count ? (double)g_strict_child_dbg.cert_size_after_sum / g_strict_child_dbg.region_stats_count : 0.0;
    cerr << fixed << setprecision(3)
         << "avg_region_size_before=" << avgRegionBefore
         << " avg_region_size_after=" << avgRegionAfter
         << " avg_cert_size_before=" << avgCertBefore
         << " avg_cert_size_after=" << avgCertAfter
         << "\n";
    cerr << "owner_support_build_calls=" << g_batch_dbg.owner_support_build_calls
         << " owner_support_build_V=" << g_batch_dbg.owner_support_build_vertices
         << " owner_support_build_E=" << g_batch_dbg.owner_support_build_edges
         << " owner_support_relevant_ep_sum=" << g_batch_dbg.owner_support_relevant_endpoints_sum
         << " owner_support_watch_vertices_sum=" << g_batch_dbg.owner_support_watch_vertices_sum
         << "\n";
    cerr << "owner_touched_by_watch=" << g_batch_dbg.owner_touched_by_watch
         << " owner_touched_unique=" << g_batch_dbg.owner_touched_unique
         << " class_split_events=" << g_batch_dbg.class_split_events
         << " moved_endpoint_count=" << g_batch_dbg.moved_endpoint_count
         << " query_incident_scans=" << g_batch_dbg.query_incident_scans
         << " query_resolved_by_split=" << g_batch_dbg.query_resolved_by_split
         << " query_resolved_owner_dead_or_endpoint_dead=" << g_batch_dbg.query_resolved_owner_dead_or_endpoint_dead
         << " active_query_peak=" << g_batch_dbg.active_query_peak
         << " support_watch_peak=" << g_batch_dbg.support_watch_peak
         << " local_active_mismatch=" << g_batch_dbg.local_active_mismatch
         << " local_active_partition_mismatch=" << g_batch_dbg.local_active_partition_mismatch
         << " debug_touched_missing_classes=" << g_batch_dbg.debug_touched_missing_classes
         << " debug_touched_extra_classes=" << g_batch_dbg.debug_touched_extra_classes
         << " debug_touched_missing_classes=" << g_batch_dbg.debug_touched_missing_classes
         << " debug_touched_extra_classes=" << g_batch_dbg.debug_touched_extra_classes
         << " support_build_failures=" << g_batch_dbg.support_build_failures
         << "\n";
    cerr << "support_rebuild_artifact_calls=" << g_batch_dbg.support_rebuild_artifact_calls
         << " support_rebuild_artifact_vertices=" << g_batch_dbg.support_rebuild_artifact_vertices
         << " support_rebuild_artifact_chain_steps=" << g_batch_dbg.support_rebuild_artifact_chain_steps
         << " support_rebuild_fallback_calls=" << g_batch_dbg.support_rebuild_fallback_calls
         << " support_rebuild_fallback_vertices=" << g_batch_dbg.support_rebuild_fallback_vertices
         << " support_rebuild_fallback_edges=" << g_batch_dbg.support_rebuild_fallback_edges
         << " watch_register_vertices=" << g_batch_dbg.watch_register_vertices
         << " watch_unregister_vertices=" << g_batch_dbg.watch_unregister_vertices
         << " watch_live_entries_peak=" << g_batch_dbg.watch_live_entries_peak
         << " watch_stale_drops=" << g_batch_dbg.watch_stale_drops
         << "\n";
    cerr << "touched_class_total=" << g_batch_dbg.touched_class_total
         << " support_positive_component_total=" << g_batch_dbg.support_positive_component_total
         << " skip_by_single_positive_component=" << g_batch_dbg.skip_by_single_positive_component
         << " skip_by_rep_bucket_unanimous=" << g_batch_dbg.skip_by_rep_bucket_unanimous
         << " split_required_class_count=" << g_batch_dbg.split_required_class_count
         << " rep_bucket_checks=" << g_batch_dbg.rep_bucket_checks
         << " moved_endpoint_enumerations=" << g_batch_dbg.moved_endpoint_enumerations
         << " moved_endpoint_total=" << g_batch_dbg.moved_endpoint_total
         << " largest_bucket_kept_count=" << g_batch_dbg.largest_bucket_kept_count
         << " class_local_fullscan_calls=" << g_batch_dbg.class_local_fullscan_calls
         << " class_local_fullscan_endpoints=" << g_batch_dbg.class_local_fullscan_endpoints
         << " fullscan_bad_meta=" << g_batch_dbg.fullscan_bad_meta
         << " fullscan_bad_xpos=" << g_batch_dbg.fullscan_bad_xpos
         << " fullscan_bad_ctx=" << g_batch_dbg.fullscan_bad_ctx
         << " fullscan_bad_rep=" << g_batch_dbg.fullscan_bad_rep
         << " support_meta_build_ok=" << g_batch_dbg.support_meta_build_ok
         << " support_meta_fail_artifact_stamp=" << g_batch_dbg.support_meta_fail_artifact_stamp
         << " support_meta_fail_fallback_stamp=" << g_batch_dbg.support_meta_fail_fallback_stamp
         << " support_meta_fail_root=" << g_batch_dbg.support_meta_fail_root
         << "\n";
    cerr << "support_meta_build_calls=" << g_batch_dbg.support_meta_build_calls
         << " support_meta_build_watch_vertices=" << g_batch_dbg.support_meta_build_watch_vertices
         << " support_meta_build_relevant_endpoints=" << g_batch_dbg.support_meta_build_relevant_endpoints
         << " support_meta_hash_pos_build_items=" << g_batch_dbg.support_meta_hash_pos_build_items
         << " support_meta_graph_recover_calls=" << g_batch_dbg.support_meta_graph_recover_calls
         << " support_meta_graph_recover_edges=" << g_batch_dbg.support_meta_graph_recover_edges
         << " support_meta_endpoint_sort_calls=" << g_batch_dbg.support_meta_endpoint_sort_calls
         << " support_meta_endpoint_sort_items=" << g_batch_dbg.support_meta_endpoint_sort_items
         << " support_meta_from_collector_calls=" << g_batch_dbg.support_meta_from_collector_calls
         << " support_meta_from_collector_watch_vertices=" << g_batch_dbg.support_meta_from_collector_watch_vertices
         << " support_meta_from_collector_relevant_endpoints=" << g_batch_dbg.support_meta_from_collector_relevant_endpoints
         << "\n";
    cerr << "support_full_rebuild_calls=" << g_batch_dbg.support_full_rebuild_calls
         << " support_full_rebuild_watch_vertices=" << g_batch_dbg.support_full_rebuild_watch_vertices
         << " support_reuse_single_calls=" << g_batch_dbg.support_reuse_single_calls
         << " support_reuse_single_watch_vertices_kept=" << g_batch_dbg.support_reuse_single_watch_vertices_kept
         << " support_reuse_single_watch_vertices_removed=" << g_batch_dbg.support_reuse_single_watch_vertices_removed
         << " support_reuse_unanimous_calls=" << g_batch_dbg.support_reuse_unanimous_calls
         << " support_reuse_unanimous_components=" << g_batch_dbg.support_reuse_unanimous_components
         << " support_reuse_unanimous_reps=" << g_batch_dbg.support_reuse_unanimous_reps
         << " support_reuse_unanimous_connector_calls=" << g_batch_dbg.support_reuse_unanimous_connector_calls
         << " support_reuse_unanimous_connector_vertices=" << g_batch_dbg.support_reuse_unanimous_connector_vertices
         << " support_reuse_unanimous_watch_vertices_kept=" << g_batch_dbg.support_reuse_unanimous_watch_vertices_kept
         << " support_reuse_unanimous_watch_vertices_added=" << g_batch_dbg.support_reuse_unanimous_watch_vertices_added
         << " support_reuse_unanimous_watch_vertices_removed=" << g_batch_dbg.support_reuse_unanimous_watch_vertices_removed
         << " support_merged_metadata_calls=" << g_batch_dbg.support_merged_metadata_calls
         << " support_merged_metadata_vertices=" << g_batch_dbg.support_merged_metadata_vertices
         << "\n";
    cerr << "piece_shadow_skip_classes_total=" << g_batch_dbg.piece_shadow_skip_classes_total
         << " piece_shadow_single_positive_classes=" << g_batch_dbg.piece_shadow_single_positive_classes
         << " piece_shadow_unanimous_classes=" << g_batch_dbg.piece_shadow_unanimous_classes
         << " piece_shadow_split_classes=" << g_batch_dbg.piece_shadow_split_classes
         << " piece_shadow_current_materialize_vertices=" << g_batch_dbg.piece_shadow_current_materialize_vertices
         << " piece_shadow_current_watch_unregister_vertices=" << g_batch_dbg.piece_shadow_current_watch_unregister_vertices
         << " piece_shadow_current_watch_register_vertices=" << g_batch_dbg.piece_shadow_current_watch_register_vertices
         << " piece_shadow_candidate_piece_reused_vertices=" << g_batch_dbg.piece_shadow_candidate_piece_reused_vertices
         << " piece_shadow_candidate_piece_removed_vertices=" << g_batch_dbg.piece_shadow_candidate_piece_removed_vertices
         << " piece_shadow_candidate_connector_vertices=" << g_batch_dbg.piece_shadow_candidate_connector_vertices
         << " piece_shadow_candidate_boundary_ops=" << g_batch_dbg.piece_shadow_candidate_boundary_ops
         << " piece_shadow_candidate_piece_count=" << g_batch_dbg.piece_shadow_candidate_piece_count
         << " piece_shadow_candidate_parent_side_positive_cases=" << g_batch_dbg.piece_shadow_candidate_parent_side_positive_cases
         << " piece_shadow_candidate_multi_piece_classes=" << g_batch_dbg.piece_shadow_candidate_multi_piece_classes
         << " piece_shadow_estimated_saved_vertices=" << g_batch_dbg.piece_shadow_estimated_saved_vertices
         << " piece_live_count=" << g_batch_dbg.piece_live_count
         << " piece_live_vertices=" << g_batch_dbg.piece_live_vertices
         << " piece_reuse_single_calls=" << g_batch_dbg.piece_reuse_single_calls
         << " piece_reuse_single_reused_vertices=" << g_batch_dbg.piece_reuse_single_reused_vertices
         << " piece_reuse_single_removed_vertices=" << g_batch_dbg.piece_reuse_single_removed_vertices
         << " piece_reuse_unanimous_calls=" << g_batch_dbg.piece_reuse_unanimous_calls
         << " piece_reuse_unanimous_reused_vertices=" << g_batch_dbg.piece_reuse_unanimous_reused_vertices
         << " piece_reuse_unanimous_removed_vertices=" << g_batch_dbg.piece_reuse_unanimous_removed_vertices
         << " piece_reuse_unanimous_added_connector_vertices=" << g_batch_dbg.piece_reuse_unanimous_added_connector_vertices
         << " piece_materialize_fallback_calls=" << g_batch_dbg.piece_materialize_fallback_calls
         << " piece_materialize_fallback_vertices=" << g_batch_dbg.piece_materialize_fallback_vertices
         << " piece_fallback_reason_need_support_meta=" << g_batch_dbg.piece_fallback_reason_need_support_meta
         << " piece_fallback_reason_bad_x_handle=" << g_batch_dbg.piece_fallback_reason_bad_x_handle
         << " piece_fallback_reason_preserved_piece_hit=" << g_batch_dbg.piece_fallback_reason_preserved_piece_hit
         << " piece_fallback_reason_connector_hit=" << g_batch_dbg.piece_fallback_reason_connector_hit
         << " piece_fallback_reason_split_required=" << g_batch_dbg.piece_fallback_reason_split_required
         << " piece_fallback_reason_other=" << g_batch_dbg.piece_fallback_reason_other
         << " piece_native_candidate_classes=" << g_batch_dbg.piece_native_candidate_classes
         << " piece_native_candidate_preserved_hits=" << g_batch_dbg.piece_native_candidate_preserved_hits
         << " piece_native_candidate_connector_hits=" << g_batch_dbg.piece_native_candidate_connector_hits
         << " piece_native_single_calls=" << g_batch_dbg.piece_native_single_calls
         << " piece_native_single_preserved_hits=" << g_batch_dbg.piece_native_single_preserved_hits
         << " piece_native_single_connector_hits=" << g_batch_dbg.piece_native_single_connector_hits
         << " piece_native_single_reused_vertices=" << g_batch_dbg.piece_native_single_reused_vertices
         << " piece_native_single_removed_vertices=" << g_batch_dbg.piece_native_single_removed_vertices
         << " piece_native_single_boundary_ops=" << g_batch_dbg.piece_native_single_boundary_ops
         << " piece_native_unanimous_calls=" << g_batch_dbg.piece_native_unanimous_calls
         << " piece_native_unanimous_preserved_hits=" << g_batch_dbg.piece_native_unanimous_preserved_hits
         << " piece_native_unanimous_connector_hits=" << g_batch_dbg.piece_native_unanimous_connector_hits
         << " piece_native_unanimous_reused_vertices=" << g_batch_dbg.piece_native_unanimous_reused_vertices
         << " piece_native_unanimous_removed_vertices=" << g_batch_dbg.piece_native_unanimous_removed_vertices
         << " piece_native_unanimous_added_connector_vertices=" << g_batch_dbg.piece_native_unanimous_added_connector_vertices
         << " piece_native_unanimous_boundary_ops=" << g_batch_dbg.piece_native_unanimous_boundary_ops
         << " connector_shadow_unanimous_classes=" << g_batch_dbg.connector_shadow_unanimous_classes
         << " connector_shadow_current_removed_vertices=" << g_batch_dbg.connector_shadow_current_removed_vertices
         << " connector_shadow_current_added_vertices=" << g_batch_dbg.connector_shadow_current_added_vertices
         << " connector_shadow_candidate_reused_connector_vertices=" << g_batch_dbg.connector_shadow_candidate_reused_connector_vertices
         << " connector_shadow_candidate_removed_connector_vertices=" << g_batch_dbg.connector_shadow_candidate_removed_connector_vertices
         << " connector_shadow_candidate_patch_vertices=" << g_batch_dbg.connector_shadow_candidate_patch_vertices
         << " connector_shadow_candidate_attachment_retargets=" << g_batch_dbg.connector_shadow_candidate_attachment_retargets
         << " connector_shadow_candidate_terminal_fragment_groups=" << g_batch_dbg.connector_shadow_candidate_terminal_fragment_groups
         << " connector_shadow_candidate_no_patch_needed=" << g_batch_dbg.connector_shadow_candidate_no_patch_needed
         << " connector_shadow_estimated_saved_vertices=" << g_batch_dbg.connector_shadow_estimated_saved_vertices
         << "\n";
    cerr << "connector_skeleton_actual_calls=" << g_batch_dbg.connector_skeleton_actual_calls
         << " connector_skeleton_actual_terminals=" << g_batch_dbg.connector_skeleton_actual_terminals
         << " connector_skeleton_actual_vertices=" << g_batch_dbg.connector_skeleton_actual_vertices
         << " connector_skeleton_actual_removed_old_connector_vertices=" << g_batch_dbg.connector_skeleton_actual_removed_old_connector_vertices
         << " connector_skeleton_actual_retargets=" << g_batch_dbg.connector_skeleton_actual_retargets
         << " connector_skeleton_old_vertices=" << g_batch_dbg.connector_skeleton_old_vertices
         << " connector_skeleton_new_vertices=" << g_batch_dbg.connector_skeleton_new_vertices
         << " connector_skeleton_common_vertices=" << g_batch_dbg.connector_skeleton_common_vertices
         << " connector_skeleton_added_vertices=" << g_batch_dbg.connector_skeleton_added_vertices
         << " connector_skeleton_removed_vertices=" << g_batch_dbg.connector_skeleton_removed_vertices
         << " connector_skeleton_intersection_ratio_permille=" << g_batch_dbg.connector_skeleton_intersection_ratio_permille
         << " connector_watch_full_unregister=" << g_batch_dbg.connector_watch_full_unregister
         << " connector_watch_full_register=" << g_batch_dbg.connector_watch_full_register
         << " connector_watch_diff_unregister=" << g_batch_dbg.connector_watch_diff_unregister
         << " connector_watch_diff_register=" << g_batch_dbg.connector_watch_diff_register
         << " connector_watch_diff_reused=" << g_batch_dbg.connector_watch_diff_reused
         << " connector_watch_diff_actual_calls=" << g_batch_dbg.connector_watch_diff_actual_calls
         << " connector_watch_diff_actual_reused=" << g_batch_dbg.connector_watch_diff_actual_reused
         << " connector_watch_diff_actual_removed=" << g_batch_dbg.connector_watch_diff_actual_removed
         << " connector_watch_diff_actual_added=" << g_batch_dbg.connector_watch_diff_actual_added
         << " connector_skeleton_candidate_classes=" << g_batch_dbg.connector_skeleton_candidate_classes
         << " connector_skeleton_selected_classes=" << g_batch_dbg.connector_skeleton_selected_classes
         << " connector_skeleton_selected_connector_only=" << g_batch_dbg.connector_skeleton_selected_connector_only
         << " connector_skeleton_selected_both_on=" << g_batch_dbg.connector_skeleton_selected_both_on
         << " connector_skeleton_forced_classes=" << g_batch_dbg.connector_skeleton_forced_classes
         << " connector_skeleton_reject_state_not_unanimous=" << g_batch_dbg.connector_skeleton_reject_state_not_unanimous
         << " connector_skeleton_reject_no_preserved_pieces=" << g_batch_dbg.connector_skeleton_reject_no_preserved_pieces
         << " connector_skeleton_reject_no_attachment_vertices=" << g_batch_dbg.connector_skeleton_reject_no_attachment_vertices
         << " connector_skeleton_reject_support_meta_valid=" << g_batch_dbg.connector_skeleton_reject_support_meta_valid
         << " connector_skeleton_reject_origin_kind=" << g_batch_dbg.connector_skeleton_reject_origin_kind
         << " connector_skeleton_reject_missing_tree=" << g_batch_dbg.connector_skeleton_reject_missing_tree
         << " connector_skeleton_reject_fallback_guard=" << g_batch_dbg.connector_skeleton_reject_fallback_guard
         << " connector_skeleton_reject_other=" << g_batch_dbg.connector_skeleton_reject_other
         << " unanimous_baseline_path_calls=" << g_batch_dbg.unanimous_baseline_path_calls
         << " unanimous_baseline_path_vertices=" << g_batch_dbg.unanimous_baseline_path_vertices
         << " debug_force_skeleton_calls=" << g_batch_dbg.debug_force_skeleton_calls
         << " debug_force_skeleton_reference_compare_calls=" << g_batch_dbg.debug_force_skeleton_reference_compare_calls
         << " debug_force_skeleton_divergence=" << g_batch_dbg.debug_force_skeleton_divergence
         << " debug_unanimous_state_old_field_read=" << g_batch_dbg.debug_unanimous_state_old_field_read
         << " debug_unanimous_state_new_field_read=" << g_batch_dbg.debug_unanimous_state_new_field_read
         << "\n";
    cerr << "profile_mode=" << local_profile_mode_name()
         << " sampled=" << (local_profile_topk_enabled() ? "yes" : "no")
         << " profile_sample_stride=" << (local_profile_topk_enabled() ? profile_sample_stride() : 0)
         << " profile_sample_warmup=" << (local_profile_topk_enabled() ? profile_sample_warmup() : 0)
         << " profile_progress_stride=" << g_local_progress_stride
         << " profile_total_deletions=" << g_batch_dbg.debug_profile_total_deletions
         << " profile_sampled_deletions=" << g_batch_dbg.debug_profile_sampled_deletions
         << " debug_progress_checkpoint_calls=" << g_batch_dbg.debug_progress_checkpoint_calls
         << " debug_progress_last_deletion=" << g_batch_dbg.debug_progress_last_deletion
         << "\n";
    cerr << "time_route_dispatch_ns=" << g_batch_dbg.time_route_dispatch_ns
         << " time_route_dispatch_calls=" << g_batch_dbg.time_route_dispatch_calls
         << " time_global_delete_dfs_ns=" << g_batch_dbg.time_global_delete_dfs_ns
         << " time_global_delete_dfs_calls=" << g_batch_dbg.time_global_delete_dfs_calls
         << " time_connector_skeleton_build_ns=" << g_batch_dbg.time_connector_skeleton_build_ns
         << " time_connector_skeleton_build_calls=" << g_batch_dbg.time_connector_skeleton_build_calls
         << " time_connector_skeleton_watch_unregister_ns=" << g_batch_dbg.time_connector_skeleton_watch_unregister_ns
         << " time_connector_skeleton_watch_unregister_calls=" << g_batch_dbg.time_connector_skeleton_watch_unregister_calls
         << " time_connector_skeleton_watch_register_ns=" << g_batch_dbg.time_connector_skeleton_watch_register_ns
         << " time_connector_skeleton_watch_register_calls=" << g_batch_dbg.time_connector_skeleton_watch_register_calls
         << " time_preserved_piece_split_ns=" << g_batch_dbg.time_preserved_piece_split_ns
         << " time_preserved_piece_split_calls=" << g_batch_dbg.time_preserved_piece_split_calls
         << " time_query_incident_scan_ns=" << g_batch_dbg.time_query_incident_scan_ns
         << " time_query_incident_scan_calls=" << g_batch_dbg.time_query_incident_scan_calls
         << "\n";
    cerr << "time_unanimous_mode_dispatch_ns=" << g_batch_dbg.time_unanimous_mode_dispatch_ns
         << " time_unanimous_mode_dispatch_calls=" << g_batch_dbg.time_unanimous_mode_dispatch_calls
         << " time_terminal_collection_ns=" << g_batch_dbg.time_terminal_collection_ns
         << " time_terminal_collection_calls=" << g_batch_dbg.time_terminal_collection_calls
         << " time_vertex_lookup_ns=" << g_batch_dbg.time_vertex_lookup_ns
         << " time_vertex_lookup_calls=" << g_batch_dbg.time_vertex_lookup_calls
         << " time_watch_diff_build_ns=" << g_batch_dbg.time_watch_diff_build_ns
         << " time_watch_diff_build_calls=" << g_batch_dbg.time_watch_diff_build_calls
         << " time_state_publish_ns=" << g_batch_dbg.time_state_publish_ns
         << " time_state_publish_calls=" << g_batch_dbg.time_state_publish_calls
         << " time_dispatch_reuse_apply_piece_native_ns=" << g_batch_dbg.time_dispatch_reuse_apply_piece_native_ns
         << " time_dispatch_reuse_apply_piece_native_calls=" << g_batch_dbg.time_dispatch_reuse_apply_piece_native_calls
         << " time_dispatch_reuse_apply_rep_unanimous_ns=" << g_batch_dbg.time_dispatch_reuse_apply_rep_unanimous_ns
         << " time_dispatch_reuse_apply_rep_unanimous_calls=" << g_batch_dbg.time_dispatch_reuse_apply_rep_unanimous_calls
         << "\n";
    cerr << "time_dispatch_publish_preserved_annotate_ns=" << g_batch_dbg.time_dispatch_publish_preserved_annotate_ns
         << " time_dispatch_publish_preserved_annotate_calls=" << g_batch_dbg.time_dispatch_publish_preserved_annotate_calls
         << " time_dispatch_publish_connector_annotate_ns=" << g_batch_dbg.time_dispatch_publish_connector_annotate_ns
         << " time_dispatch_publish_connector_annotate_calls=" << g_batch_dbg.time_dispatch_publish_connector_annotate_calls
         << " time_dispatch_publish_watch_id_rebuild_ns=" << g_batch_dbg.time_dispatch_publish_watch_id_rebuild_ns
         << " time_dispatch_publish_watch_id_rebuild_calls=" << g_batch_dbg.time_dispatch_publish_watch_id_rebuild_calls
         << " time_dispatch_publish_canonical_rebuild_ns=" << g_batch_dbg.time_dispatch_publish_canonical_rebuild_ns
         << " time_dispatch_publish_canonical_rebuild_calls=" << g_batch_dbg.time_dispatch_publish_canonical_rebuild_calls
         << " time_dispatch_publish_posmap_build_ns=" << g_batch_dbg.time_dispatch_publish_posmap_build_ns
         << " time_dispatch_publish_posmap_build_calls=" << g_batch_dbg.time_dispatch_publish_posmap_build_calls
         << "\n";
    cerr << "connector_skeleton_terminal_collection_calls=" << g_batch_dbg.connector_skeleton_terminal_collection_calls
         << " connector_skeleton_terminals=" << g_batch_dbg.connector_skeleton_terminals
         << " time_connector_skeleton_terminal_collection_ns=" << g_batch_dbg.time_connector_skeleton_terminal_collection_ns
         << " connector_skeleton_terminal_dedupe_calls=" << g_batch_dbg.connector_skeleton_terminal_dedupe_calls
         << " time_connector_skeleton_terminal_dedupe_ns=" << g_batch_dbg.time_connector_skeleton_terminal_dedupe_ns
         << " connector_skeleton_vertexset_build_calls=" << g_batch_dbg.connector_skeleton_vertexset_build_calls
         << " connector_skeleton_vertices=" << g_batch_dbg.connector_skeleton_vertices
         << " time_connector_skeleton_vertexset_build_ns=" << g_batch_dbg.time_connector_skeleton_vertexset_build_ns
         << " connector_skeleton_vertex_lookup_build_calls=" << g_batch_dbg.connector_skeleton_vertex_lookup_build_calls
         << " connector_skeleton_vertex_lookup_build_vertices=" << g_batch_dbg.connector_skeleton_vertex_lookup_build_vertices
         << " time_connector_skeleton_vertex_lookup_build_ns=" << g_batch_dbg.time_connector_skeleton_vertex_lookup_build_ns
         << " time_connector_skeleton_core_build_ns=" << g_batch_dbg.time_connector_skeleton_core_build_ns
         << " time_connector_skeleton_core_build_calls=" << g_batch_dbg.time_connector_skeleton_core_build_calls
         << "\n";
    cerr << "connector_skeleton_build_calls=" << g_batch_dbg.connector_skeleton_build_calls
         << " connector_skeleton_terminals=" << g_batch_dbg.connector_skeleton_terminals
         << " connector_skeleton_vertices=" << g_batch_dbg.connector_skeleton_vertices
         << " connector_skeleton_watch_unregister=" << g_batch_dbg.connector_skeleton_watch_unregister
         << " connector_skeleton_watch_register=" << g_batch_dbg.connector_skeleton_watch_register
         << " preserved_piece_split_calls=" << g_batch_dbg.preserved_piece_split_calls
         << " preserved_piece_split_vertices=" << g_batch_dbg.preserved_piece_split_vertices
         << " preserved_piece_split_boundary_ops=" << g_batch_dbg.preserved_piece_split_boundary_ops
         << "\n";
    cerr << "time_reuse_route_baseline_ns=" << g_batch_dbg.time_reuse_route_baseline_ns
         << " reuse_route_baseline_calls=" << g_batch_dbg.reuse_route_baseline_calls
         << " time_reuse_route_delta_preserved_then_skeleton_ns=" << g_batch_dbg.time_reuse_route_delta_preserved_then_skeleton_ns
         << " reuse_route_delta_preserved_then_skeleton_calls=" << g_batch_dbg.reuse_route_delta_preserved_then_skeleton_calls
         << " time_reuse_route_connector_skeleton_ns=" << g_batch_dbg.time_reuse_route_connector_skeleton_ns
         << " reuse_route_connector_skeleton_calls=" << g_batch_dbg.reuse_route_connector_skeleton_calls
         << " time_reuse_route_general_delta_ns=" << g_batch_dbg.time_reuse_route_general_delta_ns
         << " reuse_route_general_delta_calls=" << g_batch_dbg.reuse_route_general_delta_calls
         << "\n";
    cerr << "time_reuse_old_attachment_map_build_ns=" << g_batch_dbg.time_reuse_old_attachment_map_build_ns
         << " time_reuse_old_attachment_map_build_calls=" << g_batch_dbg.time_reuse_old_attachment_map_build_calls
         << " time_reuse_piece_split_apply_ns=" << g_batch_dbg.time_reuse_piece_split_apply_ns
         << " time_reuse_piece_split_apply_calls=" << g_batch_dbg.time_reuse_piece_split_apply_calls
         << " time_reuse_connector_split_apply_ns=" << g_batch_dbg.time_reuse_connector_split_apply_ns
         << " time_reuse_connector_split_apply_calls=" << g_batch_dbg.time_reuse_connector_split_apply_calls
         << " time_reuse_keepmask_scan_ns=" << g_batch_dbg.time_reuse_keepmask_scan_ns
         << " time_reuse_keepmask_scan_calls=" << g_batch_dbg.time_reuse_keepmask_scan_calls
         << " time_reuse_watch_retain_ns=" << g_batch_dbg.time_reuse_watch_retain_ns
         << " time_reuse_watch_retain_calls=" << g_batch_dbg.time_reuse_watch_retain_calls
         << " time_reuse_preserved_direct_retag_ns=" << g_batch_dbg.time_reuse_preserved_direct_retag_ns
         << " time_reuse_preserved_direct_retag_calls=" << g_batch_dbg.time_reuse_preserved_direct_retag_calls
         << " time_reuse_connector_direct_retag_ns=" << g_batch_dbg.time_reuse_connector_direct_retag_ns
         << " time_reuse_connector_direct_retag_calls=" << g_batch_dbg.time_reuse_connector_direct_retag_calls
         << " time_reuse_attachment_fixup_ns=" << g_batch_dbg.time_reuse_attachment_fixup_ns
         << " time_reuse_attachment_fixup_calls=" << g_batch_dbg.time_reuse_attachment_fixup_calls
         << " time_reuse_patch_vertex_collect_ns=" << g_batch_dbg.time_reuse_patch_vertex_collect_ns
         << " time_reuse_patch_vertex_collect_calls=" << g_batch_dbg.time_reuse_patch_vertex_collect_calls
         << " time_reuse_patch_tree_build_ns=" << g_batch_dbg.time_reuse_patch_tree_build_ns
         << " time_reuse_patch_tree_build_calls=" << g_batch_dbg.time_reuse_patch_tree_build_calls
         << " time_reuse_prepublish_preserved_annotate_ns=" << g_batch_dbg.time_reuse_prepublish_preserved_annotate_ns
         << " time_reuse_prepublish_preserved_annotate_calls=" << g_batch_dbg.time_reuse_prepublish_preserved_annotate_calls
         << " time_reuse_prepublish_connector_annotate_ns=" << g_batch_dbg.time_reuse_prepublish_connector_annotate_ns
         << " time_reuse_prepublish_connector_annotate_calls=" << g_batch_dbg.time_reuse_prepublish_connector_annotate_calls
         << " time_reuse_final_publish_commit_ns=" << g_batch_dbg.time_reuse_final_publish_commit_ns
         << " time_reuse_final_publish_commit_calls=" << g_batch_dbg.time_reuse_final_publish_commit_calls
         << "\n";
    cerr << "dispatch_candidate_cids=" << g_batch_dbg.dispatch_candidate_cids
         << " dispatch_publish_preserved_handles=" << g_batch_dbg.dispatch_publish_preserved_handles
         << " dispatch_publish_connector_handles=" << g_batch_dbg.dispatch_publish_connector_handles
         << " dispatch_publish_preserved_pieces_visited=" << g_batch_dbg.dispatch_publish_preserved_pieces_visited
         << " dispatch_publish_connector_pieces_visited=" << g_batch_dbg.dispatch_publish_connector_pieces_visited
         << " dispatch_publish_watch_id_rebuild_calls=" << g_batch_dbg.dispatch_publish_watch_id_rebuild_calls
         << " dispatch_publish_watch_id_rebuild_handles=" << g_batch_dbg.dispatch_publish_watch_id_rebuild_handles
         << " dispatch_publish_canonical_rebuild_calls=" << g_batch_dbg.dispatch_publish_canonical_rebuild_calls
         << " dispatch_publish_canonical_vertices=" << g_batch_dbg.dispatch_publish_canonical_vertices
         << " dispatch_publish_posmap_build_calls=" << g_batch_dbg.dispatch_publish_posmap_build_calls
         << " dispatch_publish_posmap_build_vertices=" << g_batch_dbg.dispatch_publish_posmap_build_vertices
         << " dispatch_publish_noop_calls=" << g_batch_dbg.dispatch_publish_noop_calls
         << " dispatch_publish_full_rescan_calls=" << g_batch_dbg.dispatch_publish_full_rescan_calls
         << " reuse_old_piece_hits=" << g_batch_dbg.reuse_old_piece_hits
         << " reuse_old_connector_hits=" << g_batch_dbg.reuse_old_connector_hits
         << " reuse_replacement_pieces=" << g_batch_dbg.reuse_replacement_pieces
         << " reuse_keepmask_removed_handles=" << g_batch_dbg.reuse_keepmask_removed_handles
         << " reuse_keepmask_removed_preserved_handles=" << g_batch_dbg.reuse_keepmask_removed_preserved_handles
         << " reuse_keepmask_removed_connector_handles=" << g_batch_dbg.reuse_keepmask_removed_connector_handles
         << " reuse_preserved_direct_retag_handles=" << g_batch_dbg.reuse_preserved_direct_retag_handles
         << " reuse_connector_direct_retag_handles=" << g_batch_dbg.reuse_connector_direct_retag_handles
         << " reuse_attachment_retargets=" << g_batch_dbg.reuse_attachment_retargets
         << " reuse_patch_vertices=" << g_batch_dbg.reuse_patch_vertices
         << " reuse_patch_tree_build_calls=" << g_batch_dbg.reuse_patch_tree_build_calls
         << " reuse_patch_handles_added=" << g_batch_dbg.reuse_patch_handles_added
         << " reuse_prepublish_preserved_annotate_calls=" << g_batch_dbg.reuse_prepublish_preserved_annotate_calls
         << " reuse_prepublish_preserved_handles=" << g_batch_dbg.reuse_prepublish_preserved_handles
         << " reuse_prepublish_connector_annotate_calls=" << g_batch_dbg.reuse_prepublish_connector_annotate_calls
         << " reuse_prepublish_connector_handles=" << g_batch_dbg.reuse_prepublish_connector_handles
         << " reuse_full_connector_watch_id_rebuild_calls=" << g_batch_dbg.reuse_full_connector_watch_id_rebuild_calls
         << " reuse_incremental_connector_watch_id_update_calls=" << g_batch_dbg.reuse_incremental_connector_watch_id_update_calls
         << " reuse_final_publish_calls=" << g_batch_dbg.reuse_final_publish_calls
         << " reuse_final_publish_noop_calls=" << g_batch_dbg.reuse_final_publish_noop_calls
         << " reuse_final_publish_skipped_calls=" << g_batch_dbg.reuse_final_publish_skipped_calls
         << " reuse_watch_handle_full_scan_calls=" << g_batch_dbg.reuse_watch_handle_full_scan_calls
         << " reuse_watch_handle_full_scan_handles=" << g_batch_dbg.reuse_watch_handle_full_scan_handles
         << " reuse_duplicate_preserved_annotate_passes=" << g_batch_dbg.reuse_duplicate_preserved_annotate_passes
         << " reuse_duplicate_connector_watch_id_rebuild_passes=" << g_batch_dbg.reuse_duplicate_connector_watch_id_rebuild_passes
         << " reuse_state_commit_identical_calls=" << g_batch_dbg.reuse_state_commit_identical_calls
         << "\n";
    cerr << "time_wscan_preserved_keepstamp_build_ns=" << g_batch_dbg.time_wscan_preserved_keepstamp_build_ns
         << " time_wscan_preserved_keepstamp_build_calls=" << g_batch_dbg.time_wscan_preserved_keepstamp_build_calls
         << " time_wscan_preserved_keepmask_decision_ns=" << g_batch_dbg.time_wscan_preserved_keepmask_decision_ns
         << " time_wscan_preserved_keepmask_decision_calls=" << g_batch_dbg.time_wscan_preserved_keepmask_decision_calls
         << " time_wscan_preserved_stamp_mark_ns=" << g_batch_dbg.time_wscan_preserved_stamp_mark_ns
         << " time_wscan_preserved_stamp_mark_calls=" << g_batch_dbg.time_wscan_preserved_stamp_mark_calls
         << " time_wscan_connector_desired_set_build_ns=" << g_batch_dbg.time_wscan_connector_desired_set_build_ns
         << " time_wscan_connector_desired_set_build_calls=" << g_batch_dbg.time_wscan_connector_desired_set_build_calls
         << " time_wscan_connector_keepmask_decision_ns=" << g_batch_dbg.time_wscan_connector_keepmask_decision_ns
         << " time_wscan_connector_keepmask_decision_calls=" << g_batch_dbg.time_wscan_connector_keepmask_decision_calls
         << " time_wscan_connector_existing_set_build_ns=" << g_batch_dbg.time_wscan_connector_existing_set_build_ns
         << " time_wscan_connector_existing_set_build_calls=" << g_batch_dbg.time_wscan_connector_existing_set_build_calls
         << " time_wscan_connector_addverts_diff_ns=" << g_batch_dbg.time_wscan_connector_addverts_diff_ns
         << " time_wscan_connector_addverts_diff_calls=" << g_batch_dbg.time_wscan_connector_addverts_diff_calls
         << "\n";
    cerr << "time_wscan_retain_remove_entries_ns=" << g_batch_dbg.time_wscan_retain_remove_entries_ns
         << " time_wscan_retain_remove_entries_calls=" << g_batch_dbg.time_wscan_retain_remove_entries_calls
         << " time_wscan_retain_compact_handles_ns=" << g_batch_dbg.time_wscan_retain_compact_handles_ns
         << " time_wscan_retain_compact_handles_calls=" << g_batch_dbg.time_wscan_retain_compact_handles_calls
         << " time_wscan_retain_slotpos_fixup_ns=" << g_batch_dbg.time_wscan_retain_slotpos_fixup_ns
         << " time_wscan_retain_slotpos_fixup_calls=" << g_batch_dbg.time_wscan_retain_slotpos_fixup_calls
         << " time_wscan_retain_handleidx_fixup_ns=" << g_batch_dbg.time_wscan_retain_handleidx_fixup_ns
         << " time_wscan_retain_handleidx_fixup_calls=" << g_batch_dbg.time_wscan_retain_handleidx_fixup_calls
         << " time_wscan_retain_owner_lookup_ns=" << g_batch_dbg.time_wscan_retain_owner_lookup_ns
         << " time_wscan_retain_owner_lookup_calls=" << g_batch_dbg.time_wscan_retain_owner_lookup_calls
         << " time_wscan_route_baseline_ns=" << g_batch_dbg.time_wscan_route_baseline_ns
         << " time_wscan_route_delta_preserved_then_skeleton_ns=" << g_batch_dbg.time_wscan_route_delta_preserved_then_skeleton_ns
         << " time_wscan_route_connector_skeleton_ns=" << g_batch_dbg.time_wscan_route_connector_skeleton_ns
         << " time_wscan_route_general_delta_ns=" << g_batch_dbg.time_wscan_route_general_delta_ns
         << "\n";
    cerr << "wscan_preserved_keepmask_scans=" << g_batch_dbg.wscan_preserved_keepmask_scans
         << " wscan_connector_keepmask_scans=" << g_batch_dbg.wscan_connector_keepmask_scans
         << " wscan_handles_scanned_preserved_keepmask=" << g_batch_dbg.wscan_handles_scanned_preserved_keepmask
         << " wscan_handles_scanned_connector_keepmask=" << g_batch_dbg.wscan_handles_scanned_connector_keepmask
         << " wscan_handles_scanned_preserved_stamp_mark=" << g_batch_dbg.wscan_handles_scanned_preserved_stamp_mark
         << " wscan_handles_scanned_existing_connector_set=" << g_batch_dbg.wscan_handles_scanned_existing_connector_set
         << " wscan_preserved_keepstamp_vertices_marked=" << g_batch_dbg.wscan_preserved_keepstamp_vertices_marked
         << " wscan_preserved_stamp_vertices_marked=" << g_batch_dbg.wscan_preserved_stamp_vertices_marked
         << " wscan_desired_connector_vertices=" << g_batch_dbg.wscan_desired_connector_vertices
         << " wscan_existing_connector_vertices=" << g_batch_dbg.wscan_existing_connector_vertices
         << " wscan_addverts_candidates=" << g_batch_dbg.wscan_addverts_candidates
         << " wscan_addverts_selected=" << g_batch_dbg.wscan_addverts_selected
         << " wscan_retain_removed_handles=" << g_batch_dbg.wscan_retain_removed_handles
         << " wscan_retain_noop_calls=" << g_batch_dbg.wscan_retain_noop_calls
         << " wscan_retain_slotpos_fixups=" << g_batch_dbg.wscan_retain_slotpos_fixups
         << " wscan_retain_handleidx_fixups=" << g_batch_dbg.wscan_retain_handleidx_fixups
         << " wscan_retain_owner_state_lookups=" << g_batch_dbg.wscan_retain_owner_state_lookups
         << " wscan_route_baseline_calls=" << g_batch_dbg.wscan_route_baseline_calls
         << " wscan_route_delta_preserved_then_skeleton_calls=" << g_batch_dbg.wscan_route_delta_preserved_then_skeleton_calls
         << " wscan_route_connector_skeleton_calls=" << g_batch_dbg.wscan_route_connector_skeleton_calls
         << " wscan_route_general_delta_calls=" << g_batch_dbg.wscan_route_general_delta_calls
         << " wscan_duplicate_full_scan_passes=" << g_batch_dbg.wscan_duplicate_full_scan_passes
         << " wscan_duplicate_full_scan_handles=" << g_batch_dbg.wscan_duplicate_full_scan_handles
         << " wscan_used_connectorWatchEntryIds_fastpath_calls=" << g_batch_dbg.wscan_used_connectorWatchEntryIds_fastpath_calls
         << " wscan_used_preservedHandleIdxs_fastpath_calls=" << g_batch_dbg.wscan_used_preservedHandleIdxs_fastpath_calls
         << "\n";
    cerr << "time_retain_remove_bitmap_build_ns=" << g_batch_dbg.time_retain_remove_bitmap_build_ns
         << " time_retain_remove_bitmap_build_calls=" << g_batch_dbg.time_retain_remove_bitmap_build_calls
         << " time_retain_sparse_remove_list_build_ns=" << g_batch_dbg.time_retain_sparse_remove_list_build_ns
         << " time_retain_sparse_remove_list_build_calls=" << g_batch_dbg.time_retain_sparse_remove_list_build_calls
         << " time_retain_watchByVertex_pop_ns=" << g_batch_dbg.time_retain_watchByVertex_pop_ns
         << " time_retain_watchByVertex_pop_calls=" << g_batch_dbg.time_retain_watchByVertex_pop_calls
         << " time_retain_moved_entry_owner_lookup_ns=" << g_batch_dbg.time_retain_moved_entry_owner_lookup_ns
         << " time_retain_moved_entry_owner_lookup_calls=" << g_batch_dbg.time_retain_moved_entry_owner_lookup_calls
         << " time_retain_moved_entry_same_owner_fastpath_ns=" << g_batch_dbg.time_retain_moved_entry_same_owner_fastpath_ns
         << " time_retain_moved_entry_same_owner_fastpath_calls=" << g_batch_dbg.time_retain_moved_entry_same_owner_fastpath_calls
         << " time_retain_moved_entry_slotpos_patch_ns=" << g_batch_dbg.time_retain_moved_entry_slotpos_patch_ns
         << " time_retain_moved_entry_slotpos_patch_calls=" << g_batch_dbg.time_retain_moved_entry_slotpos_patch_calls
         << " time_retain_kept_vector_build_ns=" << g_batch_dbg.time_retain_kept_vector_build_ns
         << " time_retain_kept_vector_build_calls=" << g_batch_dbg.time_retain_kept_vector_build_calls
         << " time_retain_kept_handle_copy_ns=" << g_batch_dbg.time_retain_kept_handle_copy_ns
         << " time_retain_kept_handle_copy_calls=" << g_batch_dbg.time_retain_kept_handle_copy_calls
         << " time_retain_kept_handleidx_patch_ns=" << g_batch_dbg.time_retain_kept_handleidx_patch_ns
         << " time_retain_kept_handleidx_patch_calls=" << g_batch_dbg.time_retain_kept_handleidx_patch_calls
         << " time_retain_final_swap_state_update_ns=" << g_batch_dbg.time_retain_final_swap_state_update_ns
         << " time_retain_final_swap_state_update_calls=" << g_batch_dbg.time_retain_final_swap_state_update_calls
         << "\n";
    cerr << "retain_calls=" << g_batch_dbg.retain_calls
         << " retain_watch_handles_before=" << g_batch_dbg.retain_watch_handles_before
         << " retain_watch_handles_after=" << g_batch_dbg.retain_watch_handles_after
         << " retain_removed_handles=" << g_batch_dbg.retain_removed_handles
         << " retain_removed_connector_handles=" << g_batch_dbg.retain_removed_connector_handles
         << " retain_removed_preserved_handles=" << g_batch_dbg.retain_removed_preserved_handles
         << " retain_removed_sparse_calls=" << g_batch_dbg.retain_removed_sparse_calls
         << " retain_removed_sparse_entries=" << g_batch_dbg.retain_removed_sparse_entries
         << " retain_removed_dense_calls=" << g_batch_dbg.retain_removed_dense_calls
         << " retain_remove_bitmap_entries=" << g_batch_dbg.retain_remove_bitmap_entries
         << " retain_watchByVertex_pop_calls=" << g_batch_dbg.retain_watchByVertex_pop_calls
         << " retain_moved_entry_count=" << g_batch_dbg.retain_moved_entry_count
         << " retain_moved_entry_same_owner_fastpath_hits=" << g_batch_dbg.retain_moved_entry_same_owner_fastpath_hits
         << " retain_owner_lookup_calls=" << g_batch_dbg.retain_owner_lookup_calls
         << " retain_owner_lookup_hits=" << g_batch_dbg.retain_owner_lookup_hits
         << " retain_owner_lookup_misses=" << g_batch_dbg.retain_owner_lookup_misses
         << " retain_slotpos_fixups=" << g_batch_dbg.retain_slotpos_fixups
         << " retain_kept_handles_copied=" << g_batch_dbg.retain_kept_handles_copied
         << " retain_handleidx_fixups=" << g_batch_dbg.retain_handleidx_fixups
         << " retain_final_swap_calls=" << g_batch_dbg.retain_final_swap_calls
         << " retain_noop_calls=" << g_batch_dbg.retain_noop_calls
         << " retain_remove_ratio_ppm_sum=" << g_batch_dbg.retain_remove_ratio_ppm_sum
         << " retain_sparse_remove_fastpath_calls=" << g_batch_dbg.retain_sparse_remove_fastpath_calls
         << " retain_sparse_remove_fastpath_removed_entries=" << g_batch_dbg.retain_sparse_remove_fastpath_removed_entries
         << " retain_skip_handleidx_patch_calls=" << g_batch_dbg.retain_skip_handleidx_patch_calls
         << " retain_skip_slotpos_patch_calls=" << g_batch_dbg.retain_skip_slotpos_patch_calls
         << "\n";
    cerr << "time_kvec_prefix_fastpath_check_ns=" << g_batch_dbg.time_kvec_prefix_fastpath_check_ns
         << " time_kvec_prefix_fastpath_check_calls=" << g_batch_dbg.time_kvec_prefix_fastpath_check_calls
         << " time_kvec_suffix_fastpath_check_ns=" << g_batch_dbg.time_kvec_suffix_fastpath_check_ns
         << " time_kvec_suffix_fastpath_check_calls=" << g_batch_dbg.time_kvec_suffix_fastpath_check_calls
         << " time_kvec_kept_count_scan_ns=" << g_batch_dbg.time_kvec_kept_count_scan_ns
         << " time_kvec_kept_count_scan_calls=" << g_batch_dbg.time_kvec_kept_count_scan_calls
         << " time_kvec_scratch_prepare_ns=" << g_batch_dbg.time_kvec_scratch_prepare_ns
         << " time_kvec_scratch_prepare_calls=" << g_batch_dbg.time_kvec_scratch_prepare_calls
         << " time_kvec_stable_emit_unchanged_prefix_ns=" << g_batch_dbg.time_kvec_stable_emit_unchanged_prefix_ns
         << " time_kvec_stable_emit_unchanged_prefix_calls=" << g_batch_dbg.time_kvec_stable_emit_unchanged_prefix_calls
         << " time_kvec_stable_emit_moved_suffix_ns=" << g_batch_dbg.time_kvec_stable_emit_moved_suffix_ns
         << " time_kvec_stable_emit_moved_suffix_calls=" << g_batch_dbg.time_kvec_stable_emit_moved_suffix_calls
         << " time_kvec_patchlist_build_ns=" << g_batch_dbg.time_kvec_patchlist_build_ns
         << " time_kvec_patchlist_build_calls=" << g_batch_dbg.time_kvec_patchlist_build_calls
         << " time_kvec_handleidx_patch_changed_only_ns=" << g_batch_dbg.time_kvec_handleidx_patch_changed_only_ns
         << " time_kvec_handleidx_patch_changed_only_calls=" << g_batch_dbg.time_kvec_handleidx_patch_changed_only_calls
         << " time_kvec_handleidx_patch_skip_same_index_ns=" << g_batch_dbg.time_kvec_handleidx_patch_skip_same_index_ns
         << " time_kvec_handleidx_patch_skip_same_index_calls=" << g_batch_dbg.time_kvec_handleidx_patch_skip_same_index_calls
         << " time_kvec_final_resize_or_swap_ns=" << g_batch_dbg.time_kvec_final_resize_or_swap_ns
         << " time_kvec_final_resize_or_swap_calls=" << g_batch_dbg.time_kvec_final_resize_or_swap_calls
         << "\n";
    cerr << "kvec_calls=" << g_batch_dbg.kvec_calls
         << " kvec_watch_handles_before=" << g_batch_dbg.kvec_watch_handles_before
         << " kvec_watch_handles_after=" << g_batch_dbg.kvec_watch_handles_after
         << " kvec_removed_handles=" << g_batch_dbg.kvec_removed_handles
         << " kvec_first_removed_index_sum=" << g_batch_dbg.kvec_first_removed_index_sum
         << " kvec_last_removed_suffix_len_sum=" << g_batch_dbg.kvec_last_removed_suffix_len_sum
         << " kvec_unchanged_prefix_handles=" << g_batch_dbg.kvec_unchanged_prefix_handles
         << " kvec_unchanged_suffix_handles=" << g_batch_dbg.kvec_unchanged_suffix_handles
         << " kvec_moved_suffix_handles=" << g_batch_dbg.kvec_moved_suffix_handles
         << " kvec_changed_patchlist_entries=" << g_batch_dbg.kvec_changed_patchlist_entries
         << " kvec_handle_copy_entries=" << g_batch_dbg.kvec_handle_copy_entries
         << " kvec_handleidx_patch_changed_entries=" << g_batch_dbg.kvec_handleidx_patch_changed_entries
         << " kvec_handleidx_patch_skipped_same_index_entries=" << g_batch_dbg.kvec_handleidx_patch_skipped_same_index_entries
         << " kvec_inplace_compact_calls=" << g_batch_dbg.kvec_inplace_compact_calls
         << " kvec_scratch_vector_build_calls=" << g_batch_dbg.kvec_scratch_vector_build_calls
         << " kvec_capacity_reuse_calls=" << g_batch_dbg.kvec_capacity_reuse_calls
         << " kvec_suffix_resize_fastpath_calls=" << g_batch_dbg.kvec_suffix_resize_fastpath_calls
         << " kvec_noop_calls=" << g_batch_dbg.kvec_noop_calls
         << " kvec_removed_ratio_ppm_sum=" << g_batch_dbg.kvec_removed_ratio_ppm_sum
         << " kvec_prefix_fastpath_hits=" << g_batch_dbg.kvec_prefix_fastpath_hits
         << " kvec_suffix_fastpath_hits=" << g_batch_dbg.kvec_suffix_fastpath_hits
         << " kvec_inplace_write_same_slot_hits=" << g_batch_dbg.kvec_inplace_write_same_slot_hits
         << " kvec_swap_skipped_calls=" << g_batch_dbg.kvec_swap_skipped_calls
         << "\n";
    cerr << "time_scomp_first_removed_seek_ns=" << g_batch_dbg.time_scomp_first_removed_seek_ns
         << " time_scomp_first_removed_seek_calls=" << g_batch_dbg.time_scomp_first_removed_seek_calls
         << " time_scomp_suffix_only_check_ns=" << g_batch_dbg.time_scomp_suffix_only_check_ns
         << " time_scomp_suffix_only_check_calls=" << g_batch_dbg.time_scomp_suffix_only_check_calls
         << " time_scomp_kept_count_scan_ns=" << g_batch_dbg.time_scomp_kept_count_scan_ns
         << " time_scomp_kept_count_scan_calls=" << g_batch_dbg.time_scomp_kept_count_scan_calls
         << " time_scomp_kept_run_partition_build_ns=" << g_batch_dbg.time_scomp_kept_run_partition_build_ns
         << " time_scomp_kept_run_partition_build_calls=" << g_batch_dbg.time_scomp_kept_run_partition_build_calls
         << " time_scomp_prefix_skip_ns=" << g_batch_dbg.time_scomp_prefix_skip_ns
         << " time_scomp_prefix_skip_calls=" << g_batch_dbg.time_scomp_prefix_skip_calls
         << " time_scomp_contiguous_run_block_copy_ns=" << g_batch_dbg.time_scomp_contiguous_run_block_copy_ns
         << " time_scomp_contiguous_run_block_copy_calls=" << g_batch_dbg.time_scomp_contiguous_run_block_copy_calls
         << " time_scomp_elementwise_emit_ns=" << g_batch_dbg.time_scomp_elementwise_emit_ns
         << " time_scomp_elementwise_emit_calls=" << g_batch_dbg.time_scomp_elementwise_emit_calls
         << " time_scomp_scratch_prepare_ns=" << g_batch_dbg.time_scomp_scratch_prepare_ns
         << " time_scomp_scratch_prepare_calls=" << g_batch_dbg.time_scomp_scratch_prepare_calls
         << " time_scomp_tail_cleanup_ns=" << g_batch_dbg.time_scomp_tail_cleanup_ns
         << " time_scomp_tail_cleanup_calls=" << g_batch_dbg.time_scomp_tail_cleanup_calls
         << " time_scomp_final_resize_swap_ns=" << g_batch_dbg.time_scomp_final_resize_swap_ns
         << " time_scomp_final_resize_swap_calls=" << g_batch_dbg.time_scomp_final_resize_swap_calls
         << "\n";
    cerr << "scomp_calls=" << g_batch_dbg.scomp_calls
         << " scomp_watch_handles_before=" << g_batch_dbg.scomp_watch_handles_before
         << " scomp_watch_handles_after=" << g_batch_dbg.scomp_watch_handles_after
         << " scomp_removed_handles=" << g_batch_dbg.scomp_removed_handles
         << " scomp_first_removed_index_sum=" << g_batch_dbg.scomp_first_removed_index_sum
         << " scomp_suffix_only_calls=" << g_batch_dbg.scomp_suffix_only_calls
         << " scomp_single_middle_run_calls=" << g_batch_dbg.scomp_single_middle_run_calls
         << " scomp_removed_run_count_sum=" << g_batch_dbg.scomp_removed_run_count_sum
         << " scomp_kept_run_count_sum=" << g_batch_dbg.scomp_kept_run_count_sum
         << " scomp_prefix_skipped_handles=" << g_batch_dbg.scomp_prefix_skipped_handles
         << " scomp_suffix_skipped_handles=" << g_batch_dbg.scomp_suffix_skipped_handles
         << " scomp_block_copy_runs=" << g_batch_dbg.scomp_block_copy_runs
         << " scomp_block_copied_handles=" << g_batch_dbg.scomp_block_copied_handles
         << " scomp_elementwise_emitted_handles=" << g_batch_dbg.scomp_elementwise_emitted_handles
         << " scomp_scratch_prepare_calls=" << g_batch_dbg.scomp_scratch_prepare_calls
         << " scomp_scratch_capacity_reuse_calls=" << g_batch_dbg.scomp_scratch_capacity_reuse_calls
         << " scomp_tail_cleared_handles=" << g_batch_dbg.scomp_tail_cleared_handles
         << " scomp_final_resize_calls=" << g_batch_dbg.scomp_final_resize_calls
         << " scomp_noop_calls=" << g_batch_dbg.scomp_noop_calls
         << " scomp_removed_ratio_ppm_sum=" << g_batch_dbg.scomp_removed_ratio_ppm_sum
         << " scomp_contiguous_middle_memmove_calls=" << g_batch_dbg.scomp_contiguous_middle_memmove_calls
         << " scomp_prefix_skip_hits=" << g_batch_dbg.scomp_prefix_skip_hits
         << " scomp_suffix_resize_hits=" << g_batch_dbg.scomp_suffix_resize_hits
         << " scomp_inplace_suffix_shift_calls=" << g_batch_dbg.scomp_inplace_suffix_shift_calls
         << " scomp_swap_skipped_calls=" << g_batch_dbg.scomp_swap_skipped_calls
         << "\n";
    cerr << "time_bcopy_single_middle_run_detect_ns=" << g_batch_dbg.time_bcopy_single_middle_run_detect_ns
         << " time_bcopy_single_middle_run_detect_calls=" << g_batch_dbg.time_bcopy_single_middle_run_detect_calls
         << " time_bcopy_run_coalesce_build_ns=" << g_batch_dbg.time_bcopy_run_coalesce_build_ns
         << " time_bcopy_run_coalesce_build_calls=" << g_batch_dbg.time_bcopy_run_coalesce_build_calls
         << " time_bcopy_direct_suffix_memmove_ns=" << g_batch_dbg.time_bcopy_direct_suffix_memmove_ns
         << " time_bcopy_direct_suffix_memmove_calls=" << g_batch_dbg.time_bcopy_direct_suffix_memmove_calls
         << " time_bcopy_multi_run_block_copy_ns=" << g_batch_dbg.time_bcopy_multi_run_block_copy_ns
         << " time_bcopy_multi_run_block_copy_calls=" << g_batch_dbg.time_bcopy_multi_run_block_copy_calls
         << " time_bcopy_short_fragment_elementwise_fallback_ns=" << g_batch_dbg.time_bcopy_short_fragment_elementwise_fallback_ns
         << " time_bcopy_short_fragment_elementwise_fallback_calls=" << g_batch_dbg.time_bcopy_short_fragment_elementwise_fallback_calls
         << " time_bcopy_overlap_safe_staging_ns=" << g_batch_dbg.time_bcopy_overlap_safe_staging_ns
         << " time_bcopy_overlap_safe_staging_calls=" << g_batch_dbg.time_bcopy_overlap_safe_staging_calls
         << " time_bcopy_route_baseline_ns=" << g_batch_dbg.time_bcopy_route_baseline_ns
         << " time_bcopy_route_delta_preserved_then_skeleton_ns=" << g_batch_dbg.time_bcopy_route_delta_preserved_then_skeleton_ns
         << " time_bcopy_route_connector_skeleton_ns=" << g_batch_dbg.time_bcopy_route_connector_skeleton_ns
         << " time_bcopy_route_general_delta_ns=" << g_batch_dbg.time_bcopy_route_general_delta_ns
         << "\n";
    cerr << "bcopy_calls=" << g_batch_dbg.bcopy_calls
         << " bcopy_watch_handles_before=" << g_batch_dbg.bcopy_watch_handles_before
         << " bcopy_watch_handles_after=" << g_batch_dbg.bcopy_watch_handles_after
         << " bcopy_removed_handles=" << g_batch_dbg.bcopy_removed_handles
         << " bcopy_single_middle_run_calls=" << g_batch_dbg.bcopy_single_middle_run_calls
         << " bcopy_suffix_only_calls=" << g_batch_dbg.bcopy_suffix_only_calls
         << " bcopy_removed_run_count_sum=" << g_batch_dbg.bcopy_removed_run_count_sum
         << " bcopy_kept_run_count_sum=" << g_batch_dbg.bcopy_kept_run_count_sum
         << " bcopy_copy_plan_entries=" << g_batch_dbg.bcopy_copy_plan_entries
         << " bcopy_coalesced_run_merges=" << g_batch_dbg.bcopy_coalesced_run_merges
         << " bcopy_direct_memmove_calls=" << g_batch_dbg.bcopy_direct_memmove_calls
         << " bcopy_direct_memmoved_handles=" << g_batch_dbg.bcopy_direct_memmoved_handles
         << " bcopy_runwise_block_copy_calls=" << g_batch_dbg.bcopy_runwise_block_copy_calls
         << " bcopy_runwise_block_copied_handles=" << g_batch_dbg.bcopy_runwise_block_copied_handles
         << " bcopy_elementwise_fallback_calls=" << g_batch_dbg.bcopy_elementwise_fallback_calls
         << " bcopy_elementwise_fallback_handles=" << g_batch_dbg.bcopy_elementwise_fallback_handles
         << " bcopy_overlap_staging_calls=" << g_batch_dbg.bcopy_overlap_staging_calls
         << " bcopy_overlap_staged_handles=" << g_batch_dbg.bcopy_overlap_staged_handles
         << " bcopy_same_slot_skip_handles=" << g_batch_dbg.bcopy_same_slot_skip_handles
         << " bcopy_prefix_skipped_handles=" << g_batch_dbg.bcopy_prefix_skipped_handles
         << " bcopy_suffix_skipped_handles=" << g_batch_dbg.bcopy_suffix_skipped_handles
         << " bcopy_route_baseline_calls=" << g_batch_dbg.bcopy_route_baseline_calls
         << " bcopy_route_delta_preserved_then_skeleton_calls=" << g_batch_dbg.bcopy_route_delta_preserved_then_skeleton_calls
         << " bcopy_route_connector_skeleton_calls=" << g_batch_dbg.bcopy_route_connector_skeleton_calls
         << " bcopy_route_general_delta_calls=" << g_batch_dbg.bcopy_route_general_delta_calls
         << " bcopy_block_copy_threshold_hits=" << g_batch_dbg.bcopy_block_copy_threshold_hits
         << " bcopy_contiguous_middle_memmove_calls=" << g_batch_dbg.bcopy_contiguous_middle_memmove_calls
         << " bcopy_contiguous_middle_memmove_handles=" << g_batch_dbg.bcopy_contiguous_middle_memmove_handles
         << " bcopy_adjacent_run_coalesce_hits=" << g_batch_dbg.bcopy_adjacent_run_coalesce_hits
         << " bcopy_scratchless_overlap_safe_calls=" << g_batch_dbg.bcopy_scratchless_overlap_safe_calls
         << "\n";
    cerr << "time_plan_first_removed_seek_ns=" << g_batch_dbg.time_plan_first_removed_seek_ns
         << " time_plan_first_removed_seek_calls=" << g_batch_dbg.time_plan_first_removed_seek_calls
         << " time_plan_removed_run_discovery_ns=" << g_batch_dbg.time_plan_removed_run_discovery_ns
         << " time_plan_removed_run_discovery_calls=" << g_batch_dbg.time_plan_removed_run_discovery_calls
         << " time_plan_kept_run_discovery_ns=" << g_batch_dbg.time_plan_kept_run_discovery_ns
         << " time_plan_kept_run_discovery_calls=" << g_batch_dbg.time_plan_kept_run_discovery_calls
         << " time_plan_adjacent_run_coalesce_ns=" << g_batch_dbg.time_plan_adjacent_run_coalesce_ns
         << " time_plan_adjacent_run_coalesce_calls=" << g_batch_dbg.time_plan_adjacent_run_coalesce_calls
         << " time_plan_single_middle_shortcircuit_eligibility_ns=" << g_batch_dbg.time_plan_single_middle_shortcircuit_eligibility_ns
         << " time_plan_single_middle_shortcircuit_eligibility_calls=" << g_batch_dbg.time_plan_single_middle_shortcircuit_eligibility_calls
         << " time_plan_dst_index_accumulate_ns=" << g_batch_dbg.time_plan_dst_index_accumulate_ns
         << " time_plan_dst_index_accumulate_calls=" << g_batch_dbg.time_plan_dst_index_accumulate_calls
         << " time_plan_descriptor_emit_ns=" << g_batch_dbg.time_plan_descriptor_emit_ns
         << " time_plan_descriptor_emit_calls=" << g_batch_dbg.time_plan_descriptor_emit_calls
         << " time_plan_small_inline_buffer_prepare_ns=" << g_batch_dbg.time_plan_small_inline_buffer_prepare_ns
         << " time_plan_small_inline_buffer_prepare_calls=" << g_batch_dbg.time_plan_small_inline_buffer_prepare_calls
         << " time_plan_route_baseline_ns=" << g_batch_dbg.time_plan_route_baseline_ns
         << " time_plan_route_delta_preserved_then_skeleton_ns=" << g_batch_dbg.time_plan_route_delta_preserved_then_skeleton_ns
         << " time_plan_route_connector_skeleton_ns=" << g_batch_dbg.time_plan_route_connector_skeleton_ns
         << " time_plan_route_general_delta_ns=" << g_batch_dbg.time_plan_route_general_delta_ns
         << "\n";
    cerr << "plan_calls=" << g_batch_dbg.plan_calls
         << " plan_watch_handles_before=" << g_batch_dbg.plan_watch_handles_before
         << " plan_watch_handles_after=" << g_batch_dbg.plan_watch_handles_after
         << " plan_removed_handles=" << g_batch_dbg.plan_removed_handles
         << " plan_first_removed_index_sum=" << g_batch_dbg.plan_first_removed_index_sum
         << " plan_removed_run_count_sum=" << g_batch_dbg.plan_removed_run_count_sum
         << " plan_kept_run_count_sum=" << g_batch_dbg.plan_kept_run_count_sum
         << " plan_adjacent_merge_hits=" << g_batch_dbg.plan_adjacent_merge_hits
         << " plan_descriptor_count=" << g_batch_dbg.plan_descriptor_count
         << " plan_dst_index_updates=" << g_batch_dbg.plan_dst_index_updates
         << " plan_single_middle_shortcircuit_hits=" << g_batch_dbg.plan_single_middle_shortcircuit_hits
         << " plan_suffix_only_shortcircuit_hits=" << g_batch_dbg.plan_suffix_only_shortcircuit_hits
         << " plan_small_inline_hits=" << g_batch_dbg.plan_small_inline_hits
         << " plan_small_inline_capacity_reuse_hits=" << g_batch_dbg.plan_small_inline_capacity_reuse_hits
         << " plan_heap_plan_build_calls=" << g_batch_dbg.plan_heap_plan_build_calls
         << " plan_route_baseline_calls=" << g_batch_dbg.plan_route_baseline_calls
         << " plan_route_delta_preserved_then_skeleton_calls=" << g_batch_dbg.plan_route_delta_preserved_then_skeleton_calls
         << " plan_route_connector_skeleton_calls=" << g_batch_dbg.plan_route_connector_skeleton_calls
         << " plan_route_general_delta_calls=" << g_batch_dbg.plan_route_general_delta_calls
         << " plan_removed_ratio_ppm_sum=" << g_batch_dbg.plan_removed_ratio_ppm_sum
         << " plan_copy_plan_rebuild_calls=" << g_batch_dbg.plan_copy_plan_rebuild_calls
         << " plan_copy_plan_skipped_calls=" << g_batch_dbg.plan_copy_plan_skipped_calls
         << " plan_prefix_suffix_boundary_reuse_hits=" << g_batch_dbg.plan_prefix_suffix_boundary_reuse_hits
         << " plan_descriptor_emit_skipped_for_direct_shift_calls=" << g_batch_dbg.plan_descriptor_emit_skipped_for_direct_shift_calls
         << "\n";
    cerr << "time_rdisc_first_removed_seek_ns=" << g_batch_dbg.time_rdisc_first_removed_seek_ns
         << " time_rdisc_first_removed_seek_calls=" << g_batch_dbg.time_rdisc_first_removed_seek_calls
         << " time_rdisc_boundary_reuse_check_ns=" << g_batch_dbg.time_rdisc_boundary_reuse_check_ns
         << " time_rdisc_boundary_reuse_check_calls=" << g_batch_dbg.time_rdisc_boundary_reuse_check_calls
         << " time_rdisc_removed_run_scan_ns=" << g_batch_dbg.time_rdisc_removed_run_scan_ns
         << " time_rdisc_removed_run_scan_calls=" << g_batch_dbg.time_rdisc_removed_run_scan_calls
         << " time_rdisc_kept_run_scan_ns=" << g_batch_dbg.time_rdisc_kept_run_scan_ns
         << " time_rdisc_kept_run_scan_calls=" << g_batch_dbg.time_rdisc_kept_run_scan_calls
         << " time_rdisc_suffix_only_shortcircuit_ns=" << g_batch_dbg.time_rdisc_suffix_only_shortcircuit_ns
         << " time_rdisc_suffix_only_shortcircuit_calls=" << g_batch_dbg.time_rdisc_suffix_only_shortcircuit_calls
         << " time_rdisc_single_middle_shortcircuit_ns=" << g_batch_dbg.time_rdisc_single_middle_shortcircuit_ns
         << " time_rdisc_single_middle_shortcircuit_calls=" << g_batch_dbg.time_rdisc_single_middle_shortcircuit_calls
         << " time_rdisc_fused_onepass_scan_ns=" << g_batch_dbg.time_rdisc_fused_onepass_scan_ns
         << " time_rdisc_fused_onepass_scan_calls=" << g_batch_dbg.time_rdisc_fused_onepass_scan_calls
         << " time_rdisc_small_runlist_inline_materialize_ns=" << g_batch_dbg.time_rdisc_small_runlist_inline_materialize_ns
         << " time_rdisc_small_runlist_inline_materialize_calls=" << g_batch_dbg.time_rdisc_small_runlist_inline_materialize_calls
         << " time_rdisc_route_baseline_ns=" << g_batch_dbg.time_rdisc_route_baseline_ns
         << " time_rdisc_route_delta_preserved_then_skeleton_ns=" << g_batch_dbg.time_rdisc_route_delta_preserved_then_skeleton_ns
         << " time_rdisc_route_connector_skeleton_ns=" << g_batch_dbg.time_rdisc_route_connector_skeleton_ns
         << " time_rdisc_route_general_delta_ns=" << g_batch_dbg.time_rdisc_route_general_delta_ns
         << "\n";
    cerr << "rdisc_calls=" << g_batch_dbg.rdisc_calls
         << " rdisc_watch_handles_before=" << g_batch_dbg.rdisc_watch_handles_before
         << " rdisc_watch_handles_after=" << g_batch_dbg.rdisc_watch_handles_after
         << " rdisc_removed_handles=" << g_batch_dbg.rdisc_removed_handles
         << " rdisc_first_removed_index_sum=" << g_batch_dbg.rdisc_first_removed_index_sum
         << " rdisc_removed_run_count_sum=" << g_batch_dbg.rdisc_removed_run_count_sum
         << " rdisc_kept_run_count_sum=" << g_batch_dbg.rdisc_kept_run_count_sum
         << " rdisc_boundary_reuse_hits=" << g_batch_dbg.rdisc_boundary_reuse_hits
         << " rdisc_suffix_only_hits=" << g_batch_dbg.rdisc_suffix_only_hits
         << " rdisc_single_middle_hits=" << g_batch_dbg.rdisc_single_middle_hits
         << " rdisc_two_pass_removed_scan_calls=" << g_batch_dbg.rdisc_two_pass_removed_scan_calls
         << " rdisc_two_pass_kept_scan_calls=" << g_batch_dbg.rdisc_two_pass_kept_scan_calls
         << " rdisc_fused_onepass_calls=" << g_batch_dbg.rdisc_fused_onepass_calls
         << " rdisc_removed_scan_steps=" << g_batch_dbg.rdisc_removed_scan_steps
         << " rdisc_kept_scan_steps=" << g_batch_dbg.rdisc_kept_scan_steps
         << " rdisc_fused_scan_steps=" << g_batch_dbg.rdisc_fused_scan_steps
         << " rdisc_small_runlist_inline_hits=" << g_batch_dbg.rdisc_small_runlist_inline_hits
         << " rdisc_heap_runlist_build_calls=" << g_batch_dbg.rdisc_heap_runlist_build_calls
         << " rdisc_route_baseline_calls=" << g_batch_dbg.rdisc_route_baseline_calls
         << " rdisc_route_delta_preserved_then_skeleton_calls=" << g_batch_dbg.rdisc_route_delta_preserved_then_skeleton_calls
         << " rdisc_route_connector_skeleton_calls=" << g_batch_dbg.rdisc_route_connector_skeleton_calls
         << " rdisc_route_general_delta_calls=" << g_batch_dbg.rdisc_route_general_delta_calls
         << " rdisc_removed_ratio_ppm_sum=" << g_batch_dbg.rdisc_removed_ratio_ppm_sum
         << " rdisc_prefix_suffix_boundary_reuse_hits=" << g_batch_dbg.rdisc_prefix_suffix_boundary_reuse_hits
         << " rdisc_shortcircuit_skipped_kept_scan_calls=" << g_batch_dbg.rdisc_shortcircuit_skipped_kept_scan_calls
         << " rdisc_shortcircuit_skipped_removed_scan_calls=" << g_batch_dbg.rdisc_shortcircuit_skipped_removed_scan_calls
         << " rdisc_runlist_materialize_skipped_calls=" << g_batch_dbg.rdisc_runlist_materialize_skipped_calls
         << "\n";
    cerr << "time_fclass_suffix_only_gate_ns=" << g_batch_dbg.time_fclass_suffix_only_gate_ns
         << " time_fclass_suffix_only_gate_calls=" << g_batch_dbg.time_fclass_suffix_only_gate_calls
         << " time_fclass_single_middle_gate_ns=" << g_batch_dbg.time_fclass_single_middle_gate_ns
         << " time_fclass_single_middle_gate_calls=" << g_batch_dbg.time_fclass_single_middle_gate_calls
         << " time_fclass_onepass_transition_scan_ns=" << g_batch_dbg.time_fclass_onepass_transition_scan_ns
         << " time_fclass_onepass_transition_scan_calls=" << g_batch_dbg.time_fclass_onepass_transition_scan_calls
         << " time_fclass_transition_emit_runs_ns=" << g_batch_dbg.time_fclass_transition_emit_runs_ns
         << " time_fclass_transition_emit_runs_calls=" << g_batch_dbg.time_fclass_transition_emit_runs_calls
         << " time_fclass_run_count_finalize_ns=" << g_batch_dbg.time_fclass_run_count_finalize_ns
         << " time_fclass_run_count_finalize_calls=" << g_batch_dbg.time_fclass_run_count_finalize_calls
         << " time_fclass_small_runlist_inline_ns=" << g_batch_dbg.time_fclass_small_runlist_inline_ns
         << " time_fclass_small_runlist_inline_calls=" << g_batch_dbg.time_fclass_small_runlist_inline_calls
         << " time_fclass_route_baseline_ns=" << g_batch_dbg.time_fclass_route_baseline_ns
         << " time_fclass_route_delta_preserved_then_skeleton_ns=" << g_batch_dbg.time_fclass_route_delta_preserved_then_skeleton_ns
         << " time_fclass_route_connector_skeleton_ns=" << g_batch_dbg.time_fclass_route_connector_skeleton_ns
         << " time_fclass_route_general_delta_ns=" << g_batch_dbg.time_fclass_route_general_delta_ns
         << "\n";
    cerr << "fclass_calls=" << g_batch_dbg.fclass_calls
         << " fclass_watch_handles_before=" << g_batch_dbg.fclass_watch_handles_before
         << " fclass_watch_handles_after=" << g_batch_dbg.fclass_watch_handles_after
         << " fclass_removed_handles=" << g_batch_dbg.fclass_removed_handles
         << " fclass_suffix_only_hits=" << g_batch_dbg.fclass_suffix_only_hits
         << " fclass_single_middle_hits=" << g_batch_dbg.fclass_single_middle_hits
         << " fclass_fused_onepass_calls=" << g_batch_dbg.fclass_fused_onepass_calls
         << " fclass_transition_steps=" << g_batch_dbg.fclass_transition_steps
         << " fclass_removed_to_kept_transitions=" << g_batch_dbg.fclass_removed_to_kept_transitions
         << " fclass_kept_to_removed_transitions=" << g_batch_dbg.fclass_kept_to_removed_transitions
         << " fclass_run_count_finalize_calls=" << g_batch_dbg.fclass_run_count_finalize_calls
         << " fclass_small_inline_hits=" << g_batch_dbg.fclass_small_inline_hits
         << " fclass_small_inline_capacity_reuse_hits=" << g_batch_dbg.fclass_small_inline_capacity_reuse_hits
         << " fclass_heap_runlist_build_calls=" << g_batch_dbg.fclass_heap_runlist_build_calls
         << " fclass_route_baseline_calls=" << g_batch_dbg.fclass_route_baseline_calls
         << " fclass_route_delta_preserved_then_skeleton_calls=" << g_batch_dbg.fclass_route_delta_preserved_then_skeleton_calls
         << " fclass_route_connector_skeleton_calls=" << g_batch_dbg.fclass_route_connector_skeleton_calls
         << " fclass_route_general_delta_calls=" << g_batch_dbg.fclass_route_general_delta_calls
         << " fclass_shortcircuit_skipped_emit_calls=" << g_batch_dbg.fclass_shortcircuit_skipped_emit_calls
         << " fclass_shortcircuit_skipped_scan_steps=" << g_batch_dbg.fclass_shortcircuit_skipped_scan_steps
         << " fclass_prefix_suffix_boundary_reuse_hits=" << g_batch_dbg.fclass_prefix_suffix_boundary_reuse_hits
         << " fclass_run_finalize_skipped_calls=" << g_batch_dbg.fclass_run_finalize_skipped_calls
         << "\n";
    if (!g_slow_deletion_profiles.empty()) {
        auto prof = g_slow_deletion_profiles;
        sort(prof.begin(), prof.end(), [](const SlowDeletionProfile& a, const SlowDeletionProfile& b){ return a.totalDeletionTimeNs > b.totalDeletionTimeNs; });
        if ((int)prof.size() > kSlowDeletionKeep) prof.resize(kSlowDeletionKeep);
        for (size_t i = 0; i < prof.size(); ++i) {
            const auto& r = prof[i];
            cerr << "slow_del[" << i << "] idx=" << r.deletionIndex
                 << " x=" << r.deletedVertex
                 << " touched=" << r.touchedClassCount
                 << " terms=" << r.connectorSkeletonTerminals
                 << " skelV=" << r.connectorSkeletonVertices
                 << " unreg=" << r.connectorSkeletonWatchUnregister
                 << " reg=" << r.connectorSkeletonWatchRegister
                 << " splitV=" << r.preservedPieceSplitVertices
                 << " gdfsE=" << r.globalDeleteDfsEdges
                 << " qscan=" << r.queryIncidentScans
                 << " total_ns=" << r.totalDeletionTimeNs
                 << " t_gdfs_ns=" << r.timeGlobalDeleteDfsNs
                 << " t_skel_ns=" << r.timeConnectorSkeletonBuildNs
                 << " t_unreg_ns=" << r.timeConnectorSkeletonWatchUnregisterNs
                 << " t_reg_ns=" << r.timeConnectorSkeletonWatchRegisterNs
                 << " t_split_ns=" << r.timePreservedPieceSplitNs
                 << " t_qscan_ns=" << r.timeQueryIncidentScanNs
                 << " dispatch_candidate_cids=" << r.dispatchCandidateCids
                 << " publish_preserved_handles=" << r.publishPreservedHandles
                 << " publish_connector_handles=" << r.publishConnectorHandles
                 << " publish_posmap_builds=" << r.publishPosmapBuilds
                 << " publish_full_rescan_calls=" << r.publishFullRescanCalls
                 << " publish_noop_calls=" << r.publishNoopCalls
                 << " reuse_route=" << reuse_route_name(r.reuseRouteTag)
                 << " reuse_keepmask_removed_handles=" << r.reuseKeepmaskRemovedHandles
                 << " reuse_preserved_direct_retag_handles=" << r.reusePreservedDirectRetagHandles
                 << " reuse_connector_direct_retag_handles=" << r.reuseConnectorDirectRetagHandles
                 << " reuse_attachment_retargets=" << r.reuseAttachmentRetargets
                 << " reuse_patch_vertices=" << r.reusePatchVertices
                 << " reuse_patch_handles_added=" << r.reusePatchHandlesAdded
                 << " reuse_prepublish_preserved_annotate_calls=" << r.reusePrepublishPreservedAnnotateCalls
                 << " reuse_prepublish_connector_annotate_calls=" << r.reusePrepublishConnectorAnnotateCalls
                 << " reuse_final_publish_noop_calls=" << r.reuseFinalPublishNoopCalls
                 << " reuse_final_publish_skipped_calls=" << r.reuseFinalPublishSkippedCalls
                 << " reuse_total_ns=" << r.timeReuseTotalNs
                 << " wscan_route=" << reuse_route_name(r.wscanRouteTag)
                 << " wscan_preserved_handles_scanned=" << r.wscanPreservedHandlesScanned
                 << " wscan_connector_handles_scanned=" << r.wscanConnectorHandlesScanned
                 << " wscan_existing_connector_set_handles_scanned=" << r.wscanExistingConnectorSetHandlesScanned
                 << " wscan_retain_removed_handles=" << r.wscanRetainRemovedHandles
                 << " wscan_retain_slotpos_fixups=" << r.wscanRetainSlotposFixups
                 << " wscan_duplicate_full_scan_passes=" << r.wscanDuplicateFullScanPasses
                 << " retain_removed_handles=" << r.retainRemovedHandles
                 << " retain_sparse_removed_entries=" << r.retainSparseRemovedEntries
                 << " retain_moved_entry_count=" << r.retainMovedEntryCount
                 << " retain_owner_lookup_calls=" << r.retainOwnerLookupCalls
                 << " retain_owner_lookup_misses=" << r.retainOwnerLookupMisses
                 << " retain_slotpos_fixups=" << r.retainSlotposFixups
                 << " retain_kept_handles_copied=" << r.retainKeptHandlesCopied
                 << " retain_handleidx_fixups=" << r.retainHandleidxFixups
                 << " scomp_first_removed_index=" << r.scompFirstRemovedIndex
                 << " scomp_removed_run_count=" << r.scompRemovedRunCount
                 << " scomp_kept_run_count=" << r.scompKeptRunCount
                 << " scomp_prefix_skipped_handles=" << r.scompPrefixSkippedHandles
                 << " scomp_block_copied_handles=" << r.scompBlockCopiedHandles
                 << " scomp_elementwise_emitted_handles=" << r.scompElementwiseEmittedHandles
                 << " scomp_suffix_only_calls=" << r.scompSuffixOnlyCalls
                 << " scomp_single_middle_run_calls=" << r.scompSingleMiddleRunCalls
                 << " scomp_scratch_capacity_reuse_calls=" << r.scompScratchCapacityReuseCalls
                 << " plan_route=" << reuse_route_name(r.planRouteTag)
                 << " plan_first_removed_index=" << r.planFirstRemovedIndex
                 << " plan_removed_run_count=" << r.planRemovedRunCount
                 << " plan_kept_run_count=" << r.planKeptRunCount
                 << " plan_adjacent_merge_hits=" << r.planAdjacentMergeHits
                 << " plan_descriptor_count=" << r.planDescriptorCount
                 << " plan_dst_index_updates=" << r.planDstIndexUpdates
                 << " plan_single_middle_shortcircuit_hits=" << r.planSingleMiddleShortcircuitHits
                 << " plan_small_inline_hits=" << r.planSmallInlineHits
                 << " plan_total_ns=" << r.timePlanTotalNs
                 << " rdisc_route=" << reuse_route_name(r.rdiscRouteTag)
                 << " rdisc_first_removed_index=" << r.rdiscFirstRemovedIndex
                 << " rdisc_removed_run_count=" << r.rdiscRemovedRunCount
                 << " rdisc_kept_run_count=" << r.rdiscKeptRunCount
                 << " rdisc_boundary_reuse_hits=" << r.rdiscBoundaryReuseHits
                 << " rdisc_suffix_only_hits=" << r.rdiscSuffixOnlyHits
                 << " rdisc_single_middle_hits=" << r.rdiscSingleMiddleHits
                 << " rdisc_fused_onepass_calls=" << r.rdiscFusedOnepassCalls
                 << " rdisc_small_runlist_inline_hits=" << r.rdiscSmallRunlistInlineHits
                 << " rdisc_total_ns=" << r.timeRdiscTotalNs
                 << " fclass_route=" << reuse_route_name(r.fclassRouteTag)
                 << " fclass_suffix_only_hits=" << r.fclassSuffixOnlyHits
                 << " fclass_single_middle_hits=" << r.fclassSingleMiddleHits
                 << " fclass_fused_onepass_calls=" << r.fclassFusedOnepassCalls
                 << " fclass_transition_steps=" << r.fclassTransitionSteps
                 << " fclass_removed_to_kept_transitions=" << r.fclassRemovedToKeptTransitions
                 << " fclass_kept_to_removed_transitions=" << r.fclassKeptToRemovedTransitions
                 << " fclass_small_inline_hits=" << r.fclassSmallInlineHits
                 << " fclass_total_ns=" << r.timeFclassTotalNs
                 << " bcopy_route=" << reuse_route_name(r.bcopyRouteTag)
                 << " bcopy_single_middle_run_calls=" << r.bcopySingleMiddleRunCalls
                 << " bcopy_removed_run_count=" << r.bcopyRemovedRunCount
                 << " bcopy_kept_run_count=" << r.bcopyKeptRunCount
                 << " bcopy_copy_plan_entries=" << r.bcopyCopyPlanEntries
                 << " bcopy_direct_memmove_calls=" << r.bcopyDirectMemmoveCalls
                 << " bcopy_direct_memmoved_handles=" << r.bcopyDirectMemmovedHandles
                 << " bcopy_block_copied_handles=" << r.bcopyBlockCopiedHandles
                 << " bcopy_elementwise_fallback_handles=" << r.bcopyElementwiseFallbackHandles
                 << " bcopy_overlap_staging_calls=" << r.bcopyOverlapStagingCalls
                 << " bcopy_total_ns=" << r.timeBcopyTotalNs
                 << " scomp_total_ns=" << r.timeScompTotalNs
                 << " retain_total_ns=" << r.timeRetainTotalNs
                 << " wscan_total_ns=" << r.timeWscanTotalNs
                 << "\n";
        }
    }
#endif
    return 0;
}
