from __future__ import annotations

import random
from typing import Dict, List, Tuple


def _base_parent(n: int) -> List[int]:
    if n < 1:
        raise ValueError("n must be >= 1")
    parent = [0] * (n + 1)
    parent[1] = 0
    return parent


def build_chain_tree(n: int) -> Tuple[List[int], Dict]:
    parent = _base_parent(n)
    for v in range(2, n + 1):
        parent[v] = v - 1
    return parent, {"shape": "chain"}


def build_star_tree(n: int) -> Tuple[List[int], Dict]:
    parent = _base_parent(n)
    for v in range(2, n + 1):
        parent[v] = 1
    return parent, {"shape": "star"}


def build_balanced_binary_tree(n: int) -> Tuple[List[int], Dict]:
    parent = _base_parent(n)
    for v in range(2, n + 1):
        parent[v] = v // 2
    return parent, {"shape": "balanced_binary"}


def build_comb_tree(n: int) -> Tuple[List[int], Dict]:
    parent = _base_parent(n)
    spine = [1]
    side = []

    cur = 1
    nxt = 2
    while nxt + 1 <= n:
        parent[nxt] = cur
        parent[nxt + 1] = cur
        spine.append(nxt)
        side.append(nxt + 1)
        cur = nxt
        nxt += 2

    extra = None
    if nxt <= n:
        parent[nxt] = cur
        spine.append(nxt)
        extra = nxt

    return parent, {
        "shape": "comb",
        "spine": spine,
        "side": side,
        "extra": extra,
        "fanout": 1,
    }


def build_multi_comb_tree(n: int, fanout: int = 3) -> Tuple[List[int], Dict]:
    """
    Heavy path (spine) where each spine node gets up to `fanout` side leaves.
    This is a stronger variant of comb for decomposition-style solvers because
    one large component survives while many side leaves can keep generating
    long-lived constraints.
    """
    fanout = max(1, fanout)
    parent = _base_parent(n)
    spine = [1]
    side_groups: List[List[int]] = [[]]

    cur = 1
    nxt = 2
    while nxt <= n:
        # heavy child first
        parent[nxt] = cur
        spine.append(nxt)
        side_groups.append([])
        heavy = nxt
        nxt += 1

        # side leaves for current spine node
        for _ in range(fanout):
            if nxt > n:
                break
            parent[nxt] = cur
            side_groups[-2].append(nxt)
            nxt += 1

        cur = heavy

    total_side = sum(len(g) for g in side_groups)
    return parent, {
        "shape": "multi_comb",
        "spine": spine,
        "side_groups": side_groups,
        "fanout": fanout,
        "total_side": total_side,
    }


def build_broom_tree(n: int, handle_len: int | None = None) -> Tuple[List[int], Dict]:
    parent = _base_parent(n)
    if handle_len is None:
        handle_len = max(2, n * 2 // 3)
    handle_len = min(max(1, handle_len), n)

    for v in range(2, handle_len + 1):
        parent[v] = v - 1
    for v in range(handle_len + 1, n + 1):
        parent[v] = handle_len

    return parent, {
        "shape": "broom",
        "handle": list(range(1, handle_len + 1)),
        "broom_head": handle_len,
        "head_leaves": list(range(handle_len + 1, n + 1)),
    }


def build_caterpillar_tree(n: int) -> Tuple[List[int], Dict]:
    parent = _base_parent(n)
    if n == 1:
        return parent, {"shape": "caterpillar", "spine": [1], "side_map": {}}

    spine_len = max(2, (n + 1) // 2)
    spine_len = min(spine_len, n)
    for v in range(2, spine_len + 1):
        parent[v] = v - 1

    side_map: Dict[int, List[int]] = {v: [] for v in range(1, spine_len + 1)}
    cur = spine_len + 1
    target = 1
    while cur <= n:
        parent[cur] = target
        side_map[target].append(cur)
        cur += 1
        target += 1
        if target > spine_len:
            target = 1

    return parent, {
        "shape": "caterpillar",
        "spine": list(range(1, spine_len + 1)),
        "side_map": side_map,
    }


def build_random_recursive_tree(n: int, seed: int) -> Tuple[List[int], Dict]:
    rng = random.Random(seed)
    parent = _base_parent(n)
    for v in range(2, n + 1):
        parent[v] = rng.randint(1, v - 1)
    return parent, {"shape": "random_recursive", "seed": seed}
