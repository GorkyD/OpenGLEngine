#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"

detect_preset() {
    case "$(uname -s)" in
        Darwin)
            echo "clang-debug-macos"
            ;;
        Linux)
            echo "clang-debug-linux"
            ;;
        *)
            echo "Cannot auto-detect CMake preset for this OS. Pass preset name explicitly." >&2
            exit 1
            ;;
    esac
}

if [ "$#" -gt 0 ]; then
    PRESET="$1"
    shift
else
    PRESET="$(detect_preset)"
fi

VCPKG_ROOT="${VCPKG_ROOT:-${REPO_ROOT}/vcpkg}"

VCPKG_ROOT="${VCPKG_ROOT}" "${SCRIPT_DIR}/bootstrap-vcpkg.sh"

cd "${REPO_ROOT}"
VCPKG_ROOT="${VCPKG_ROOT}" cmake --preset "${PRESET}" \
    -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" \
    "$@"
