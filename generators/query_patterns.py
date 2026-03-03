from __future__ import annotations

import math
from typing import Dict, List

from .tree_ctx import TreeContext


class QueryBuilder:
    def __init__(self, cap: int, dedup: bool = True):
        self.cap = cap
        self.queries: List[tuple[int, int, int]] = []
        self.seen = set() if dedup else None

    def __len__(self) -> int:
        return len(self.queries)

    def remaining(self) -> int:
        return self.cap - len(self.queries)

    def add(self, u: int, v: int, w: int) -> bool:
        if len(self.queries) >= self.cap:
            return False
        if u == v and w != u:
            return False
        if u > v:
            u, v = v, u
        item = (u, v, w)
        if self.seen is not None:
            if item in self.seen:
                return False
            self.seen.add(item)
        self.queries.append(item)
        return True


def pattern_comb_core(ctx: TreeContext, qb: QueryBuilder, meta: Dict) -> None:
    spine = meta.get("spine", [])
    side = meta.get("side", [])
    for i, leaf in enumerate(side):
        if i + 1 >= len(spine):
            break
        qb.add(spine[i + 1], leaf, spine[i])
        if qb.remaining() == 0:
            return


def pattern_ancestors_to_target(ctx: TreeContext, qb: QueryBuilder, target: int, stride: int = 1) -> None:
    path = ctx.ancestors_topdown(target, include_self=False)
    for idx, w in enumerate(path):
        if idx % stride == 0:
            qb.add(w, target, w)
            if qb.remaining() == 0:
                return


def pattern_comb_side_vs_deepest(ctx: TreeContext, qb: QueryBuilder, meta: Dict) -> None:
    deepest = ctx.deepest_desc[1]
    spine = meta.get("spine", [])
    side = meta.get("side", [])
    for i, leaf in enumerate(side):
        if i >= len(spine):
            break
        w = spine[i]
        if leaf != deepest and w != deepest:
            qb.add(leaf, deepest, w)
            if qb.remaining() == 0:
                return


def pattern_chain_windows(ctx: TreeContext, qb: QueryBuilder) -> None:
    n = ctx.n
    gaps = []
    g = 1
    while g < n:
        gaps.append(g)
        g <<= 1
    gaps.extend([3, 5, 7, 11, 17, 31, 63, 127])

    for gap in gaps:
        for u in range(1, n):
            v = u + gap
            if v > n:
                break
            qb.add(u, v, u)
            if qb.remaining() == 0:
                return

    step = max(1, n // 997)
    for u in range(1, n, step):
        for v in range(n, u, -step):
            qb.add(u, v, u)
            if qb.remaining() == 0:
                return


def pattern_star_pairs(ctx: TreeContext, qb: QueryBuilder) -> None:
    leaves = list(range(2, ctx.n + 1))
    for i in range(len(leaves)):
        for j in range(i + 1, len(leaves)):
            qb.add(leaves[i], leaves[j], 1)
            if qb.remaining() == 0:
                return


def pattern_internal_distinct_child(ctx: TreeContext, qb: QueryBuilder, rng, per_node: int = 1, deep: bool = True) -> None:
    for w in ctx.multi_child:
        children = ctx.children[w]
        if len(children) < 2:
            continue

        used = 0
        if deep:
            for i in range(len(children)):
                for j in range(i + 1, len(children)):
                    u = ctx.deepest_desc[children[i]]
                    v = ctx.deepest_desc[children[j]]
                    qb.add(u, v, w)
                    used += 1
                    if qb.remaining() == 0 or used >= per_node:
                        break
                if qb.remaining() == 0 or used >= per_node:
                    break
        else:
            limit = min(per_node, len(children) * (len(children) - 1) // 2)
            for _ in range(limit):
                a, b = rng.sample(children, 2)
                u = ctx.random_descendant(a, rng)
                v = ctx.random_descendant(b, rng)
                qb.add(u, v, w)
                if qb.remaining() == 0:
                    return
        if qb.remaining() == 0:
            return


def pattern_broom_head_pairs(ctx: TreeContext, qb: QueryBuilder, meta: Dict) -> None:
    head = meta.get("broom_head", 1)
    leaves = meta.get("head_leaves", [])
    for i in range(len(leaves)):
        for j in range(i + 1, len(leaves)):
            qb.add(leaves[i], leaves[j], head)
            if qb.remaining() == 0:
                return


def pattern_caterpillar_spine_vs_side(ctx: TreeContext, qb: QueryBuilder, meta: Dict) -> None:
    spine = meta.get("spine", [])
    side_map = meta.get("side_map", {})
    for i, w in enumerate(spine[:-1]):
        nxt = spine[i + 1]
        target = ctx.deepest_desc[nxt]
        for leaf in side_map.get(w, []):
            if leaf != target:
                qb.add(leaf, target, w)
                if qb.remaining() == 0:
                    return


def fill_random_mixed(ctx: TreeContext, qb: QueryBuilder, rng, branch_ratio: float = 0.45, ancestor_ratio: float = 0.35) -> None:
    if ctx.n == 1:
        return

    attempts = 0
    hard_limit = max(5000, qb.cap * 40)
    while qb.remaining() > 0 and attempts < hard_limit:
        attempts += 1
        t = rng.random()
        if t < branch_ratio and ctx.multi_child:
            w = rng.choice(ctx.multi_child)
            ch = ctx.children[w]
            if len(ch) >= 2:
                a, b = rng.sample(ch, 2)
                u = ctx.random_descendant(a, rng)
                v = ctx.random_descendant(b, rng)
                qb.add(u, v, w)
                continue
        if t < branch_ratio + ancestor_ratio:
            v = rng.randint(2, ctx.n)
            k = rng.randint(1, ctx.depth[v])
            w = ctx.kth_ancestor(v, k)
            qb.add(w, v, w)
            continue

        u = rng.randint(1, ctx.n)
        v = rng.randint(1, ctx.n)
        if u == v:
            continue
        qb.add(u, v, ctx.lca(u, v))


def verify_queries(ctx: TreeContext, queries: List[tuple[int, int, int]]) -> None:
    for idx, (u, v, w) in enumerate(queries, 1):
        real = ctx.lca(u, v)
        if real != w:
            raise AssertionError(
                f"invalid generated query #{idx}: ({u}, {v}, {w}), real lca={real}"
            )
