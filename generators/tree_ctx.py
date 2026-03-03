from __future__ import annotations

from typing import Iterator, List


class TreeContext:
    def __init__(self, parent: List[int]):
        self.parent = parent
        self.n = len(parent) - 1
        self.children = [[] for _ in range(self.n + 1)]
        for v in range(2, self.n + 1):
            self.children[parent[v]].append(v)

        self.LOG = max(1, self.n.bit_length())
        self.up = [[0] * (self.n + 1) for _ in range(self.LOG)]
        self.depth = [0] * (self.n + 1)
        self.tin = [0] * (self.n + 1)
        self.tout = [0] * (self.n + 1)
        self.euler = [0] * (self.n + 1)
        self.subtree_size = [1] * (self.n + 1)
        self.deepest_desc = list(range(self.n + 1))
        self.order: List[int] = []

        self._build()
        self.leaves = [u for u in range(1, self.n + 1) if not self.children[u]]
        self.internal = [u for u in range(1, self.n + 1) if self.children[u]]
        self.multi_child = [u for u in range(1, self.n + 1) if len(self.children[u]) >= 2]
        self.max_depth = max(self.depth)
        self.deepest_node = max(range(1, self.n + 1), key=lambda x: self.depth[x])

    def _build(self) -> None:
        timer = 0
        child_idx = [0] * (self.n + 1)
        st = [1]
        self.up[0][1] = 0
        self.depth[1] = 0

        while st:
            u = st[-1]
            if child_idx[u] == 0:
                timer += 1
                self.tin[u] = timer
                self.euler[timer] = u
                self.order.append(u)
                for k in range(1, self.LOG):
                    self.up[k][u] = self.up[k - 1][self.up[k - 1][u]]

            if child_idx[u] < len(self.children[u]):
                v = self.children[u][child_idx[u]]
                child_idx[u] += 1
                self.up[0][v] = u
                self.depth[v] = self.depth[u] + 1
                st.append(v)
            else:
                self.tout[u] = timer
                st.pop()

        for u in reversed(self.order):
            best = u
            best_depth = self.depth[u]
            for v in self.children[u]:
                self.subtree_size[u] += self.subtree_size[v]
                cand = self.deepest_desc[v]
                cand_depth = self.depth[cand]
                if cand_depth > best_depth:
                    best = cand
                    best_depth = cand_depth
            self.deepest_desc[u] = best

    def is_ancestor(self, u: int, v: int) -> bool:
        return self.tin[u] <= self.tin[v] <= self.tout[u]

    def lca(self, u: int, v: int) -> int:
        if self.is_ancestor(u, v):
            return u
        if self.is_ancestor(v, u):
            return v
        x = u
        for k in range(self.LOG - 1, -1, -1):
            nx = self.up[k][x]
            if nx and not self.is_ancestor(nx, v):
                x = nx
        return self.up[0][x]

    def kth_ancestor(self, v: int, k: int) -> int:
        x = v
        bit = 0
        while k and x:
            if k & 1:
                x = self.up[bit][x]
            k >>= 1
            bit += 1
        return x

    def random_descendant(self, u: int, rng, exclude_self: bool = False) -> int:
        left = self.tin[u] + (1 if exclude_self else 0)
        right = self.tout[u]
        if left > right:
            return u
        return self.euler[rng.randint(left, right)]

    def ancestors_topdown(self, v: int, include_self: bool = True) -> List[int]:
        path = []
        x = v
        while x:
            path.append(x)
            x = self.parent[x]
        path.reverse()
        if not include_self and path:
            path.pop()
        return path
