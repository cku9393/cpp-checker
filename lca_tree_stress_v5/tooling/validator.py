#!/usr/bin/env python3
from __future__ import annotations

import argparse
import sys
from collections import deque


def read_ints(path: str):
    with open(path, "rb") as f:
        return list(map(int, f.read().split()))


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate a solution output for BOJ 28350.")
    parser.add_argument("input_path")
    parser.add_argument("output_path")
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()

    data = read_ints(args.input_path)
    if len(data) < 2:
        print("[validator] bad input: missing N M", file=sys.stderr)
        return 1

    n, m = data[0], data[1]
    expect = 2 + 3 * m
    if len(data) != expect:
        print(f"[validator] bad input size: expected {expect} ints, got {len(data)}", file=sys.stderr)
        return 1

    q = []
    ptr = 2
    for _ in range(m):
        u, v, w = data[ptr], data[ptr + 1], data[ptr + 2]
        ptr += 3
        q.append((u, v, w))

    out = read_ints(args.output_path)
    if len(out) != n:
        print(f"[validator] output must contain exactly {n} ints, got {len(out)}", file=sys.stderr)
        return 1

    par = [0] + out
    if par[1] != 0:
        print(f"[validator] parent[1] must be 0, got {par[1]}", file=sys.stderr)
        return 1

    children = [[] for _ in range(n + 1)]
    for v in range(2, n + 1):
        p = par[v]
        if not (1 <= p <= n):
            print(f"[validator] node {v} has invalid parent {p}", file=sys.stderr)
            return 1
        if p == v:
            print(f"[validator] node {v} cannot be its own parent", file=sys.stderr)
            return 1
        children[p].append(v)

    depth = [0] * (n + 1)
    tin = [0] * (n + 1)
    tout = [0] * (n + 1)
    order = [0] * (n + 1)
    visited = 0
    timer = 0
    st = [1]
    idx = [0] * (n + 1)

    while st:
        u = st[-1]
        if idx[u] == 0:
            visited += 1
            timer += 1
            tin[u] = timer
            order[timer] = u
        if idx[u] < len(children[u]):
            v = children[u][idx[u]]
            idx[u] += 1
            depth[v] = depth[u] + 1
            st.append(v)
        else:
            tout[u] = timer
            st.pop()

    if visited != n:
        print(f"[validator] output is not a rooted tree at 1, visited {visited}/{n} nodes", file=sys.stderr)
        return 1

    LOG = max(1, n.bit_length())
    up = [[0] * (n + 1) for _ in range(LOG)]
    for v in range(1, n + 1):
        up[0][v] = par[v]
    for k in range(1, LOG):
        row = up[k]
        prev = up[k - 1]
        for v in range(1, n + 1):
            row[v] = prev[prev[v]]

    def is_ancestor(a: int, b: int) -> bool:
        return tin[a] <= tin[b] <= tout[a]

    def lca(a: int, b: int) -> int:
        if is_ancestor(a, b):
            return a
        if is_ancestor(b, a):
            return b
        x = a
        for k in range(LOG - 1, -1, -1):
            nx = up[k][x]
            if nx and not is_ancestor(nx, b):
                x = nx
        return up[0][x]

    for i, (u, v, w) in enumerate(q, 1):
        got = lca(u, v)
        if got != w:
            print(
                f"[validator] query #{i} failed: ({u}, {v}) expected LCA {w}, got {got}",
                file=sys.stderr,
            )
            return 1

    if not args.quiet:
        print(f"[validator] OK: N={n}, M={m}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
