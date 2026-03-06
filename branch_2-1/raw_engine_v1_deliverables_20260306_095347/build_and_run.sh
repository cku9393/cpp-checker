#!/usr/bin/env bash
set -euo pipefail
CXX=${CXX:-g++}
CXXFLAGS=${CXXFLAGS:-"-std=c++20 -O2 -pipe -Wall -Wextra -Wshadow"}
$CXX $CXXFLAGS raw_engine_v1.cpp -o raw_engine_v1
./raw_engine_v1
