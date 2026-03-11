#!/usr/bin/env bash
set -euo pipefail

# shellcheck disable=SC1091
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/preset_env.sh"
if [[ ! -f "${active_preset_file}" ]]; then
  "${project_root}/scripts/bootstrap.sh"
fi
load_active_presets
ctest --preset "${test_preset}" "$@"
