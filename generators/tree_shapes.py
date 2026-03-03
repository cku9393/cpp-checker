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
