#!/usr/bin/env bash
set -euo pipefail

./scripts/bootstrap.sh
./scripts/build.sh
./scripts/test.sh

# shellcheck disable=SC1091
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/preset_env.sh"
load_active_presets

"${build_dir}/bin/dcp_probe" --help >/dev/null
"${build_dir}/bin/playout_service" --help >/dev/null
"${build_dir}/bin/sms_ui" --help >/dev/null
"${build_dir}/bin/tms_web" --help >/dev/null

echo "Smoke checks passed"
