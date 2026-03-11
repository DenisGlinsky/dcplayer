#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="${project_root}/build"
active_preset_file="${build_root}/.active_preset"

choose_default_presets() {
  configure_preset="dev-make"
  build_preset="build-dev-make"
  test_preset="test-dev-make"
  build_dir="${build_root}/dev-make"
  expected_generator="Unix Makefiles"

  if command -v ninja >/dev/null 2>&1; then
    configure_preset="dev-ninja"
    build_preset="build-dev-ninja"
    test_preset="test-dev-ninja"
    build_dir="${build_root}/dev-ninja"
    expected_generator="Ninja"
  fi
}

load_active_presets() {
  if [[ -f "${active_preset_file}" ]]; then
    # shellcheck disable=SC1090
    source "${active_preset_file}"
  else
    choose_default_presets
  fi
}

validate_cache_or_clean() {
  local cache_file="${build_dir}/CMakeCache.txt"
  if [[ ! -f "${cache_file}" ]]; then
    return
  fi

  local current_generator current_home expected_home
  current_generator="$(grep '^CMAKE_GENERATOR:INTERNAL=' "${cache_file}" | cut -d'=' -f2- || true)"
  current_home="$(grep '^CMAKE_HOME_DIRECTORY:INTERNAL=' "${cache_file}" | cut -d'=' -f2- || true)"
  expected_home="${project_root}"

  if [[ -n "${current_generator}" && "${current_generator}" != "${expected_generator}" ]]; then
    echo "Existing ${build_dir} uses '${current_generator}', expected '${expected_generator}'. Cleaning build dir."
    rm -rf "${build_dir}"
    return
  fi

  if [[ -n "${current_home}" && "${current_home}" != "${expected_home}" ]]; then
    echo "Existing ${build_dir} is bound to '${current_home}', expected '${expected_home}'. Cleaning build dir."
    rm -rf "${build_dir}"
  fi
}

write_active_presets() {
  mkdir -p "${build_root}"
  cat > "${active_preset_file}" <<EOF
configure_preset="${configure_preset}"
build_preset="${build_preset}"
test_preset="${test_preset}"
build_dir="${build_dir}"
expected_generator="${expected_generator}"
EOF
}
