#!/usr/bin/env bash
set -euo pipefail

if ! command -v cmake >/dev/null 2>&1; then
  echo "cmake not found" >&2
  exit 1
fi

# shellcheck disable=SC1091
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/preset_env.sh"
choose_default_presets
validate_cache_or_clean

cmake --preset "${configure_preset}"
write_active_presets

echo "Configured ${build_dir} using preset '${configure_preset}'."
echo "Next commands:"
echo "  ./scripts/build.sh"
echo "  ./scripts/test.sh"
