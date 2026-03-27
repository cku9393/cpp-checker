#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import random
import sys
from pathlib import Path
from typing import Dict, Sequence

os.environ["PYTHONDONTWRITEBYTECODE"] = "1"
sys.dont_write_bytecode = True

from artifact_paths import configure_branch_process_env, resolve_output_path
from branch_generators.query_patterns import (
    QueryBuilder,
    fill_random_mixed,
    pattern_ancestors_to_target,
    pattern_broom_head_pairs,
    pattern_caterpillar_rectangular,
    pattern_caterpillar_spine_vs_side,
    pattern_chain_windows,
    pattern_comb_core,
    pattern_comb_rectangular,
    pattern_comb_side_vs_deepest,
    pattern_internal_distinct_child,
    pattern_multi_comb_core,
    pattern_multi_comb_rectangular,
    pattern_star_pairs,
    verify_queries,
)
from branch_generators.tree_ctx import TreeContext
from branch_generators.tree_shapes import (
    build_balanced_binary_tree,
    build_broom_tree,
    build_caterpillar_tree,
    build_chain_tree,
    build_comb_tree,
    build_multi_comb_tree,
    build_random_recursive_tree,
    build_star_tree,
)


configure_branch_process_env()

BRANCH_ROOT = Path(__file__).resolve().parent

# Keep smoke and gate generation branch-local so helper startup does not depend
# on parent-repo imports that may stall or write outside the branch sandbox.
MODE_HELP: Dict[str, str] = {
    "comb_core": "Minimal comb family. Sharp decomposition worst case with low M.",
    "comb_plus_unary": "comb_core + unary ancestor constraints toward deepest node.",
    "comb_dense": "comb_plus_unary + side-vs-deepest + random filler.",
    "comb_rect_dense": "comb tree + rectangular long-lived constraints across many depths.",
    "multi_comb_core": "Heavy spine with multiple side leaves per level.",
    "multi_comb_rect": "multi_comb + rectangular cross-depth constraints; usually harsher than plain comb.",
    "multi_comb_cap": "multi_comb_rect tuned to fill near M cap aggressively.",
    "chain_unary": "Path tree with many unary ancestor constraints.",
    "star_pairs": "Star tree with dense leaf pairs, all LCAs at root.",
    "balanced_sibling": "Balanced binary tree with representative cross-child queries.",
    "balanced_dense": "balanced_sibling + more child-pair density + filler.",
    "broom_mixed": "Long handle + dense broom head + unary handle constraints.",
    "caterpillar_mixed": "Long spine with side leaves; local+deep subtree queries.",
    "caterpillar_rect_dense": "caterpillar_mixed + rectangular cross-depth side-vs-deep queries.",
    "random_recursive_mixed": "Random recursive tree with mixed valid query types.",
}


def permute_preserving_root(parent: list[int], queries: list[tuple[int, int, int]], seed: int):
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


def _desc_targets_lazy(ctx: TreeContext, spine: Sequence[int], start_idx: int, stride: int = 1):
    for j in range(start_idx, len(spine), stride):
        yield ctx.deepest_desc[spine[j]]


def _pattern_multi_comb_rectangular_fast(
    ctx: TreeContext,
    qb: QueryBuilder,
    meta: Dict,
    *,
    stride: int = 1,
    per_leaf_limit: int | None = None,
) -> None:
    spine = meta.get("spine", [])
    side_groups = meta.get("side_groups", [])
    upto = min(len(side_groups), len(spine))
    for i in range(upto):
        w = spine[i]
        for leaf in side_groups[i]:
            used = 0
            for target in _desc_targets_lazy(ctx, spine, i + 1, stride):
                if leaf == target or target == w:
                    continue
                qb.add(leaf, target, w)
                used += 1
                if qb.remaining() == 0:
                    return
                if per_leaf_limit is not None and used >= per_leaf_limit:
                    break


def _pattern_caterpillar_rectangular_fast(
    ctx: TreeContext,
    qb: QueryBuilder,
    meta: Dict,
    *,
    stride: int = 1,
    per_leaf_limit: int | None = None,
) -> None:
    spine = meta.get("spine", [])
    side_map = meta.get("side_map", {})
    for i, w in enumerate(spine[:-1]):
        for leaf in side_map.get(w, []):
            used = 0
            for target in _desc_targets_lazy(ctx, spine, i + 1, stride):
                if leaf == target or target == w:
                    continue
                qb.add(leaf, target, w)
                used += 1
                if qb.remaining() == 0:
                    return
                if per_leaf_limit is not None and used >= per_leaf_limit:
                    break


def _finalize_mode(mode: str, seed: int, parent: list[int], meta: Dict, qb: QueryBuilder):
    ctx = TreeContext(parent)
    verify_queries(ctx, qb.queries)
    summary = {
        "mode": mode,
        "shape": meta["shape"],
        "n": len(parent) - 1,
        "m": len(qb.queries),
        "seed": seed,
        "max_depth": ctx.max_depth,
        "leaves": len(ctx.leaves),
        "internal": len(ctx.internal),
        "multi_child": len(ctx.multi_child),
        "deepest_node": ctx.deepest_node,
        "subtree_root_deepest": ctx.deepest_desc[1],
    }
    summary.update({k: v for k, v in meta.items() if k != "shape"})
    return parent, qb.queries, summary


def _build_default_mode(mode: str, n: int, cap: int, seed: int):
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
    elif mode == "comb_rect_dense":
        parent, meta = build_comb_tree(n)
        ctx = TreeContext(parent)
        qb = QueryBuilder(cap)
        pattern_comb_core(ctx, qb, meta)
        pattern_comb_rectangular(ctx, qb, meta, stride=1, per_side_limit=8)
        deepest = ctx.deepest_desc[1]
        pattern_ancestors_to_target(ctx, qb, deepest)
        pattern_comb_side_vs_deepest(ctx, qb, meta)
        fill_random_mixed(ctx, qb, rng, branch_ratio=0.55, ancestor_ratio=0.20)
    elif mode == "multi_comb_core":
        fanout = 3 if n >= 30 else 2
        parent, meta = build_multi_comb_tree(n, fanout=fanout)
        ctx = TreeContext(parent)
        qb = QueryBuilder(cap)
        pattern_multi_comb_core(ctx, qb, meta)
        deepest = ctx.deepest_desc[1]
        pattern_ancestors_to_target(ctx, qb, deepest, stride=2)
    elif mode == "multi_comb_rect":
        fanout = 3 if n >= 30 else 2
        parent, meta = build_multi_comb_tree(n, fanout=fanout)
        ctx = TreeContext(parent)
        qb = QueryBuilder(cap)
        pattern_multi_comb_core(ctx, qb, meta)
        pattern_multi_comb_rectangular(ctx, qb, meta, stride=1, per_leaf_limit=8)
        deepest = ctx.deepest_desc[1]
        pattern_ancestors_to_target(ctx, qb, deepest, stride=2)
        fill_random_mixed(ctx, qb, rng, branch_ratio=0.50, ancestor_ratio=0.25)
    elif mode == "multi_comb_cap":
        fanout = 5 if n >= 60 else 3
        parent, meta = build_multi_comb_tree(n, fanout=fanout)
        ctx = TreeContext(parent)
        qb = QueryBuilder(cap)
        pattern_multi_comb_core(ctx, qb, meta)
        pattern_multi_comb_rectangular(ctx, qb, meta, stride=1, per_leaf_limit=16)
        deepest = ctx.deepest_desc[1]
        pattern_ancestors_to_target(ctx, qb, deepest, stride=1)
        fill_random_mixed(ctx, qb, rng, branch_ratio=0.60, ancestor_ratio=0.20)
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
    elif mode == "caterpillar_rect_dense":
        parent, meta = build_caterpillar_tree(n)
        ctx = TreeContext(parent)
        qb = QueryBuilder(cap)
        pattern_caterpillar_spine_vs_side(ctx, qb, meta)
        pattern_caterpillar_rectangular(ctx, qb, meta, stride=1, per_leaf_limit=8)
        target = ctx.deepest_desc[1]
        pattern_ancestors_to_target(ctx, qb, target, stride=2)
        fill_random_mixed(ctx, qb, rng, branch_ratio=0.55, ancestor_ratio=0.20)
    elif mode == "random_recursive_mixed":
        parent, meta = build_random_recursive_tree(n, seed)
        ctx = TreeContext(parent)
        qb = QueryBuilder(cap)
        pattern_internal_distinct_child(ctx, qb, rng, per_node=2, deep=False)
        fill_random_mixed(ctx, qb, rng, branch_ratio=0.50, ancestor_ratio=0.25)
    else:
        raise ValueError(f"unknown mode: {mode}")

    return _finalize_mode(mode, seed, parent, meta, qb)


def build_mode(mode: str, n: int, cap: int, seed: int):
    if mode not in {"multi_comb_rect", "multi_comb_cap", "caterpillar_rect_dense"}:
        return _build_default_mode(mode, n, cap, seed)

    rng = random.Random(seed)

    if mode == "multi_comb_rect":
        fanout = 3 if n >= 30 else 2
        parent, meta = build_multi_comb_tree(n, fanout=fanout)
        ctx = TreeContext(parent)
        qb = QueryBuilder(cap)
        pattern_multi_comb_core(ctx, qb, meta)
        _pattern_multi_comb_rectangular_fast(ctx, qb, meta, stride=1, per_leaf_limit=8)
        deepest = ctx.deepest_desc[1]
        pattern_ancestors_to_target(ctx, qb, deepest, stride=2)
        fill_random_mixed(ctx, qb, rng, branch_ratio=0.50, ancestor_ratio=0.25)
    elif mode == "multi_comb_cap":
        fanout = 5 if n >= 60 else 3
        parent, meta = build_multi_comb_tree(n, fanout=fanout)
        ctx = TreeContext(parent)
        qb = QueryBuilder(cap)
        pattern_multi_comb_core(ctx, qb, meta)
        _pattern_multi_comb_rectangular_fast(ctx, qb, meta, stride=1, per_leaf_limit=16)
        deepest = ctx.deepest_desc[1]
        pattern_ancestors_to_target(ctx, qb, deepest, stride=1)
        fill_random_mixed(ctx, qb, rng, branch_ratio=0.60, ancestor_ratio=0.20)
    else:
        parent, meta = build_caterpillar_tree(n)
        ctx = TreeContext(parent)
        qb = QueryBuilder(cap)
        pattern_caterpillar_spine_vs_side(ctx, qb, meta)
        _pattern_caterpillar_rectangular_fast(ctx, qb, meta, stride=1, per_leaf_limit=8)
        target = ctx.deepest_desc[1]
        pattern_ancestors_to_target(ctx, qb, target, stride=2)
        fill_random_mixed(ctx, qb, rng, branch_ratio=0.55, ancestor_ratio=0.20)

    return _finalize_mode(mode, seed, parent, meta, qb)


def write_parent_file(path: str, parent: list[int]) -> None:
    with open(path, "w", encoding="utf-8") as f:
        f.write(" ".join(str(parent[i]) for i in range(1, len(parent))))
        f.write("\n")


def resolve_aux_output_path(path_like: str) -> Path | None:
    if not path_like:
        return None
    return resolve_output_path(path_like, default_key="branch_gen_case_aux")


def main() -> int:
    parser = argparse.ArgumentParser(description="Branch-local optimized stress generator for BOJ 28350.")
    parser.add_argument("--mode", type=str, help="generation mode")
    parser.add_argument("--n", type=int, default=99999)
    parser.add_argument("--m-cap", type=int, default=100000)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--shuffle-labels", action="store_true")
    parser.add_argument("--shuffle-queries", action="store_true")
    parser.add_argument("--meta", type=str, default="")
    parser.add_argument("--parent-out", type=str, default="")
    parser.add_argument("--list-modes", action="store_true")
    parser.add_argument("--describe-modes", action="store_true")
    args = parser.parse_args()

    if args.list_modes or args.describe_modes:
        for mode, desc in MODE_HELP.items():
            if args.describe_modes:
                print(f"{mode}\t{desc}")
            else:
                print(mode)
        return 0

    if not args.mode:
        raise SystemExit("--mode is required unless --list-modes is used")

    parent, queries, summary = build_mode(args.mode, args.n, args.m_cap, args.seed)

    if args.shuffle_labels:
        parent, queries = permute_preserving_root(parent, queries, args.seed ^ 0x9E3779B1)

    if args.shuffle_queries:
        rng = random.Random(args.seed ^ 0x85EBCA77)
        rng.shuffle(queries)

    try:
        meta_path = resolve_aux_output_path(args.meta)
        parent_out_path = resolve_aux_output_path(args.parent_out)
    except ValueError as exc:
        print(f"[branch_gen_case] {exc}", file=sys.stderr)
        return 2

    if meta_path is not None:
        meta_path.parent.mkdir(parents=True, exist_ok=True)
        with meta_path.open("w", encoding="utf-8") as f:
            json.dump(summary, f, ensure_ascii=False, indent=2)

    if parent_out_path is not None:
        parent_out_path.parent.mkdir(parents=True, exist_ok=True)
        write_parent_file(str(parent_out_path), parent)

    print(args.n, len(queries))
    for u, v, w in queries:
        print(u, v, w)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
