#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
clang++ -O2 -std=c++17 -c solve.cpp -o solve.o
clang++ -static solve.o -o solve
rm -f solve.o
