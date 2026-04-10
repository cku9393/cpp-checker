#!/usr/bin/env python3
from __future__ import annotations

import argparse
from collections import deque
from pathlib import Path


def _read_tokens(path: Path) -> list[int]:
    try:
        data = path.read_text(encoding="utf-8")
    except OSError as exc:
        raise ValueError(f"failed to read {path}: {exc}") from exc
    tokens = data.split()
    try:
        return [int(token) for token in tokens]
    except ValueError as exc:
        raise ValueError(f"non-integer token in {path}") from exc


def _parse_input(in_path: Path) -> tuple[int, list[tuple[int, int, int]]]:
    tokens = _read_tokens(in_path)
    if len(tokens) < 2:
        raise ValueError("input is missing N and M")
    n, m = tokens[0], tokens[1]
    if n <= 0 or m < 0:
        raise ValueError(f"invalid header n={n} m={m}")
    expected = 2 + 3 * m
    if len(tokens) != expected:
        raise ValueError(f"input token count mismatch: expected {expected}, got {len(tokens)}")
    queries: list[tuple[int, int, int]] = []
    idx = 2
    for _ in range(m):
        u, v, w = tokens[idx], tokens[idx + 1], tokens[idx + 2]
        idx += 3
        if not (1 <= u <= n and 1 <= v <= n and 1 <= w <= n):
            raise ValueError(f"query vertex out of range: {(u, v, w)}")
        queries.append((u, v, w))
    return n, queries


def _parse_output(out_path: Path, n: int) -> list[int]:
    tokens = _read_tokens(out_path)
    if len(tokens) != n:
        raise ValueError(f"output token count mismatch: expected {n}, got {len(tokens)}")
    parent = [0] * (n + 1)
    for vertex, value in enumerate(tokens, start=1):
        parent[vertex] = value
    return parent


def _build_tree(parent: list[int]) -> tuple[list[list[int]], list[int], list[list[int]]]:
    n = len(parent) - 1
    if parent[1] != 0:
        raise ValueError(f"parent[1] must be 0, got {parent[1]}")

    children = [[] for _ in range(n + 1)]
    for vertex in range(2, n + 1):
        par = parent[vertex]
        if not (1 <= par <= n):
            raise ValueError(f"parent[{vertex}] out of range: {par}")
        if par == vertex:
            raise ValueError(f"parent[{vertex}] cannot equal the vertex itself")
        children[par].append(vertex)

    depth = [-1] * (n + 1)
    depth[1] = 0
    bfs = deque([1])
    order = [1]
    while bfs:
        node = bfs.popleft()
        for child in children[node]:
            if depth[child] != -1:
                raise ValueError(f"cycle or duplicate parent detected at vertex {child}")
            depth[child] = depth[node] + 1
            bfs.append(child)
            order.append(child)

    if len(order) != n:
        raise ValueError("output does not form a single rooted tree with root 1")

    log = max(1, n.bit_length())
    up = [[0] * (n + 1) for _ in range(log)]
    for vertex in range(1, n + 1):
        up[0][vertex] = parent[vertex]
    for level in range(1, log):
        prev = up[level - 1]
        curr = up[level]
        for vertex in range(1, n + 1):
            curr[vertex] = prev[prev[vertex]]
    return children, depth, up


def _lca(u: int, v: int, depth: list[int], up: list[list[int]]) -> int:
    if depth[u] < depth[v]:
        u, v = v, u
    diff = depth[u] - depth[v]
    bit = 0
    while diff:
        if diff & 1:
            u = up[bit][u]
        diff >>= 1
        bit += 1
    if u == v:
        return u
    for bit in range(len(up) - 1, -1, -1):
        if up[bit][u] != up[bit][v]:
            u = up[bit][u]
            v = up[bit][v]
    return up[0][u]


def validate_case(in_path: str | Path, out_path: str | Path) -> tuple[bool, str]:
    in_file = Path(in_path)
    out_file = Path(out_path)
    try:
        n, queries = _parse_input(in_file)
        parent = _parse_output(out_file, n)
        _, depth, up = _build_tree(parent)
        for idx, (u, v, want) in enumerate(queries, start=1):
            got = _lca(u, v, depth, up)
            if got != want:
                return False, f"query #{idx} mismatch: lca({u}, {v})={got}, expected {want}"
    except ValueError as exc:
        return False, str(exc)
    return True, "OK"


def main() -> int:
    ap = argparse.ArgumentParser(description="Branch-local validator for BOJ 28350 solver outputs.")
    ap.add_argument("input_path")
    ap.add_argument("output_path")
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args()

    ok, message = validate_case(args.input_path, args.output_path)
    if ok:
        if not args.quiet:
            print("OK")
        return 0

    if not args.quiet:
        print(message)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
