#!/usr/bin/env bash
set -euo pipefail

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format not found; skipping"
  exit 0
fi

mapfile -t files < <(find apps src spb1 tests -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' \) | sort)
if [ "${#files[@]}" -eq 0 ]; then
  echo "No C/C++ source files found; nothing to format"
  exit 0
fi

clang-format -i "${files[@]}"
echo "Formatted ${#files[@]} files"
