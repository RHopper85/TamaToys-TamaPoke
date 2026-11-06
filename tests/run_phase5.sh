#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CXX="${CXX:-g++}"
OUT="${TMPDIR:-/tmp}/tamapoke_exp_rules_$$"
trap 'rm -f "$OUT"' EXIT
"$CXX" -std=c++17 -Wall -Wextra -Werror "$ROOT/tests/expedition_rules_test.cpp" -o "$OUT"
"$OUT"
