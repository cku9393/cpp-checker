#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import random
from typing import Dict, List, Tuple

from generators.query_patterns import (
    QueryBuilder,
    fill_random_mixed,
    pattern_ancestors_to_target,
    pattern_broom_head_pairs,
    pattern_caterpillar_spine_vs_side,
    pattern_chain_windows,
    pattern_comb_core,
    pattern_comb_side_vs_deepest,
    pattern_internal_distinct_child,
    pattern_star_pairs,
    verify_queries,
)
from generators.tree_ctx import TreeContext
from generators.tree_shapes import (
    build_balanced_binary_tree,
    build_broom_tree,
    build_caterpillar_tree,
    build_chain_tree,
    build_comb_tree,
    build_random_recursive_tree,
    build_star_tree,
)

MODE_HELP: Dict[str, str] = {
    "comb_core": "Slow-shrink comb family. Minimal but sharp decomposition worst case.",
    "comb_plus_unary": "comb_core + ancestor-to-deepest unary constraints.",
    "comb_dense": "comb_plus_unary + side-vs-deepest + random valid filler up to M cap.",
    "chain_unary": "Path tree with many unary ancestor constraints.",
    "star_pairs": "Star tree with many leaf-pair queries, all LCAs are root.",
    "balanced_sibling": "Balanced binary tree with representative queries across child subtrees.",
    "balanced_dense": "balanced_sibling + randomized child-subtree pairs + filler.",
    "broom_mixed": "Long handle + dense broom head queries + unary handle constraints.",
    "caterpillar_mixed": "Long spine with side leaves; mixes local and deep subtree queries.",
    "random_recursive_mixed": "Random recursive tree with mixed valid query types.",
}


def permute_preserving_root(parent: List[int], queries: List[Tuple[int, int, int]], seed: int):
    n = len(parent) - 1
    rng = random.Random(seed)
    labels = list(range(2, n + 1))
    rng.shuffle(labels)

    perm = list(range(n + 1))
    for old, new in zip(range(2, n + 1), labels):
        perm[old] = new

    new_parent = [0] * (n + 1)
    new_parent[1] = 0
    for old in range(2, n + 1):
        new = perm[old]
        p = parent[old]
        new_parent[new] = 0 if p == 0 else perm[p]

    new_queries = []
    for u, v, w in queries:
        nu, nv, nw = perm[u], perm[v], perm[w]
        if nu > nv:
            nu, nv = nv, nu
        new_queries.append((nu, nv, nw))

    return new_parent, new_queries


def build_mode(mode: str, n: int, cap: int, seed: int):
    rng = random.Random(seed)

    if mode == "comb_core":
        parent, meta = build_comb_tree(n)
        ctx = TreeContext(parent)
        qb = QueryBuilder(cap)
        pattern_comb_core(ctx, qb, meta)

    elif mode == "comb_plus_unary":
        parent, meta = build_comb_tree(n)
        ctx = TreeContext(parent)
        qb = QueryBuilder(cap)
        pattern_comb_core(ctx, qb, meta)
        deepest = ctx.deepest_desc[1]
        pattern_ancestors_to_target(ctx, qb, deepest)

    elif mode == "comb_dense":
        parent, meta = build_comb_tree(n)
        ctx = TreeContext(parent)
        qb = QueryBuilder(cap)
        pattern_comb_core(ctx, qb, meta)
        deepest = ctx.deepest_desc[1]
        pattern_ancestors_to_target(ctx, qb, deepest)
        pattern_comb_side_vs_deepest(ctx, qb, meta)
        fill_random_mixed(ctx, qb, rng, branch_ratio=0.50, ancestor_ratio=0.35)

    elif mode == "chain_unary":
        parent, meta = build_chain_tree(n)
        ctx = TreeContext(parent)
        qb = QueryBuilder(cap)
        pattern_chain_windows(ctx, qb)
        fill_random_mixed(ctx, qb, rng, branch_ratio=0.05, ancestor_ratio=0.90)

    elif mode == "star_pairs":
        parent, meta = build_star_tree(n)
        ctx = TreeContext(parent)
        qb = QueryBuilder(cap)
        pattern_star_pairs(ctx, qb)
        fill_random_mixed(ctx, qb, rng, branch_ratio=0.05, ancestor_ratio=0.20)

    elif mode == "balanced_sibling":
        parent, meta = build_balanced_binary_tree(n)
        ctx = TreeContext(parent)
        qb = QueryBuilder(cap)
        pattern_internal_distinct_child(ctx, qb, rng, per_node=1, deep=True)
        fill_random_mixed(ctx, qb, rng, branch_ratio=0.55, ancestor_ratio=0.20)

    elif mode == "balanced_dense":
        parent, meta = build_balanced_binary_tree(n)
        ctx = TreeContext(parent)
        qb = QueryBuilder(cap)
        pattern_internal_distinct_child(ctx, qb, rng, per_node=4, deep=True)
        pattern_internal_distinct_child(ctx, qb, rng, per_node=2, deep=False)
        fill_random_mixed(ctx, qb, rng, branch_ratio=0.60, ancestor_ratio=0.15)

    elif mode == "broom_mixed":
        handle_len = max(2, min(n, n * 2 // 3))
        parent, meta = build_broom_tree(n, handle_len=handle_len)
        ctx = TreeContext(parent)
        qb = QueryBuilder(cap)
        pattern_broom_head_pairs(ctx, qb, meta)
        target = meta["head_leaves"][0] if meta["head_leaves"] else meta["broom_head"]
        pattern_ancestors_to_target(ctx, qb, target)
        fill_random_mixed(ctx, qb, rng, branch_ratio=0.45, ancestor_ratio=0.35)

    elif mode == "caterpillar_mixed":
        parent, meta = build_caterpillar_tree(n)
        ctx = TreeContext(parent)
        qb = QueryBuilder(cap)
        pattern_caterpillar_spine_vs_side(ctx, qb, meta)
        target = ctx.deepest_desc[1]
        pattern_ancestors_to_target(ctx, qb, target, stride=2)
        fill_random_mixed(ctx, qb, rng, branch_ratio=0.50, ancestor_ratio=0.25)

    elif mode == "random_recursive_mixed":
        parent, meta = build_random_recursive_tree(n, seed)
        ctx = TreeContext(parent)
        qb = QueryBuilder(cap)
        pattern_internal_distinct_child(ctx, qb, rng, per_node=2, deep=False)
        fill_random_mixed(ctx, qb, rng, branch_ratio=0.50, ancestor_ratio=0.25)

    else:
        raise ValueError(f"unknown mode: {mode}")

    verify_queries(ctx, qb.queries)
    summary = {
        "mode": mode,
        "shape": meta["shape"],
        "n": n,
        "m": len(qb.queries),
        "seed": seed,
        "max_depth": ctx.max_depth,
        "leaves": len(ctx.leaves),
        "internal": len(ctx.internal),
        "multi_child": len(ctx.multi_child),
        "deepest_node": ctx.deepest_node,
    }
    summary.update({k: v for k, v in meta.items() if k != "shape"})
    return parent, qb.queries, summary


def write_parent_file(path: str, parent: List[int]) -> None:
    with open(path, "w", encoding="utf-8") as f:
        f.write(" ".join(str(parent[i]) for i in range(1, len(parent))))
        f.write("\n")


def main() -> int:
    parser = argparse.ArgumentParser(description="Detailed valid stress generator for BOJ 28350.")
    parser.add_argument("--mode", type=str, help="generation mode")
    parser.add_argument("--n", type=int, default=99999)
    parser.add_argument("--m-cap", type=int, default=100000)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--shuffle-labels", action="store_true")
    parser.add_argument("--shuffle-queries", action="store_true")
    parser.add_argument("--meta", type=str, default=None)
    parser.add_argument("--parent-out", type=str, default=None)
    parser.add_argument("--list-modes", action="store_true")
    args = parser.parse_args()

    if args.list_modes:
        for name, desc in MODE_HELP.items():
            print(f"{name:24s} {desc}")
        return 0

    if not args.mode:
        parser.error("--mode is required unless --list-modes is used")
    if args.n < 1:
        parser.error("--n must be >= 1")
    if args.m_cap < 1 or args.m_cap > 100000:
        parser.error("--m-cap must be in [1, 100000]")

    parent, queries, summary = build_mode(args.mode, args.n, args.m_cap, args.seed)

    if args.shuffle_labels:
        parent, queries = permute_preserving_root(parent, queries, args.seed)
        summary["shuffle_labels"] = True
    else:
        summary["shuffle_labels"] = False

    if args.shuffle_queries:
        rng = random.Random(args.seed + 7919)
        rng.shuffle(queries)
        summary["shuffle_queries"] = True
    else:
        summary["shuffle_queries"] = False

    if args.meta:
        with open(args.meta, "w", encoding="utf-8") as f:
            json.dump(summary, f, ensure_ascii=False, indent=2)

    if args.parent_out:
        write_parent_file(args.parent_out, parent)

    print(args.n, len(queries))
    for u, v, w in queries:
        print(u, v, w)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
