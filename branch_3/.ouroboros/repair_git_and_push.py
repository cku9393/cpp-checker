#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import subprocess
import sys
import zlib
from pathlib import Path


ROOT = Path("/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker")
GIT_DIR = ROOT / ".git"
RAW_BASE = "https://raw.githubusercontent.com/cku9393/cpp-checker/main/"
COMMIT_MESSAGE = "chore: update ouroboros loop prompts and pause snapshot\n"


def run_git(*args: str, check: bool = True) -> subprocess.CompletedProcess[str]:
    cp = subprocess.run(
        ["git", "-C", str(ROOT), *args],
        capture_output=True,
        text=True,
    )
    if check and cp.returncode != 0:
        raise RuntimeError(f"git {' '.join(args)} failed: {cp.stderr.strip()}")
    return cp


def parse_missing(stderr: str) -> tuple[str | None, str | None]:
    marker = "invalid object 100644 "
    for line in stderr.splitlines():
        idx = line.find(marker)
        if idx == -1:
            continue
        tail = line[idx + len(marker) :]
        obj, _, rest = tail.partition(" for ")
        rel = rest.strip()
        if rel.startswith("'") and rel.endswith("'"):
            rel = rel[1:-1]
        return obj.strip(), rel
    return None, None


def blob_sha1(data: bytes) -> str:
    payload = f"blob {len(data)}\0".encode() + data
    return hashlib.sha1(payload).hexdigest(), payload


def ensure_loose_object(obj: str, rel: str) -> None:
    src = ROOT / rel
    data = b""
    if src.exists():
        data = src.read_bytes()
    if not data:
        tmp = Path("/tmp") / (Path(rel).name + ".restore")
        curl = subprocess.run(
            ["curl", "-L", "--fail", RAW_BASE + rel, "-o", str(tmp)],
            capture_output=True,
            text=True,
        )
        if curl.returncode != 0:
            raise RuntimeError(
                f"failed to download raw file for {rel}: {curl.stderr.strip()}"
            )
        data = tmp.read_bytes()
    actual, payload = blob_sha1(data)
    if actual != obj:
        raise RuntimeError(f"hash mismatch for {rel}: wanted {obj}, got {actual}")
    obj_path = GIT_DIR / "objects" / obj[:2] / obj[2:]
    obj_path.parent.mkdir(parents=True, exist_ok=True)
    obj_path.write_bytes(zlib.compress(payload))


def main() -> int:
    restored: list[str] = []
    index_lock = GIT_DIR / "index.lock"
    for _ in range(500):
        if index_lock.exists():
            index_lock.unlink()
        cp = run_git("write-tree", check=False)
        if cp.returncode == 0:
            tree = cp.stdout.strip()
            parent = run_git("rev-parse", "HEAD").stdout.strip()
            commit = subprocess.run(
                ["git", "-C", str(ROOT), "commit-tree", tree, "-p", parent],
                input=COMMIT_MESSAGE,
                capture_output=True,
                text=True,
            )
            if commit.returncode != 0:
                raise RuntimeError(commit.stderr.strip())
            commit_id = commit.stdout.strip()
            update = subprocess.run(
                ["git", "-C", str(ROOT), "update-ref", "refs/heads/main", commit_id, parent],
                input="",
                capture_output=True,
                text=True,
            )
            if update.returncode != 0:
                raise RuntimeError(update.stderr.strip())
            push = run_git("push", "origin", "HEAD:main", check=False)
            sys.stdout.write(push.stdout)
            sys.stderr.write(push.stderr)
            if push.returncode != 0:
                return push.returncode
            print(f"COMMIT {commit_id}")
            print(f"RESTORED_COUNT {len(restored)}")
            for item in restored:
                print(f"RESTORED {item}")
            return 0

        obj, rel = parse_missing(cp.stderr)
        if not obj or not rel:
            raise RuntimeError(cp.stderr.strip())
        ensure_loose_object(obj, rel)
        restored.append(f"{obj} {rel}")

    raise RuntimeError("repair loop exceeded limit")


if __name__ == "__main__":
    raise SystemExit(main())
