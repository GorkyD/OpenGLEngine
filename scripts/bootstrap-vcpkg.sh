#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"

VCPKG_ROOT="${VCPKG_ROOT:-${REPO_ROOT}/vcpkg}"
VCPKG_REPOSITORY="${VCPKG_REPOSITORY:-https://github.com/microsoft/vcpkg.git}"
VCPKG_REF="${VCPKG_REF:-}"

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Required command is not available: $1" >&2
        exit 1
    fi
}

require_command git

if [ ! -d "${VCPKG_ROOT}" ]; then
    echo "Cloning vcpkg into: ${VCPKG_ROOT}"
    git clone "${VCPKG_REPOSITORY}" "${VCPKG_ROOT}"
elif [ ! -d "${VCPKG_ROOT}/.git" ]; then
    echo "VCPKG_ROOT exists but is not a git checkout: ${VCPKG_ROOT}" >&2
    exit 1
fi

if [ -n "${VCPKG_REF}" ]; then
    echo "Checking out vcpkg ref: ${VCPKG_REF}"
    git -C "${VCPKG_ROOT}" fetch --tags origin
    git -C "${VCPKG_ROOT}" checkout "${VCPKG_REF}"
fi

if [ "${VCPKG_BOOTSTRAP_FORCE:-0}" = "1" ] || [ ! -x "${VCPKG_ROOT}/vcpkg" ]; then
    "${VCPKG_ROOT}/bootstrap-vcpkg.sh" -disableMetrics
else
    echo "vcpkg executable is already available: ${VCPKG_ROOT}/vcpkg"
fi

echo "vcpkg is ready at: ${VCPKG_ROOT}"
