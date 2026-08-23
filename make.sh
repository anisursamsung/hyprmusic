#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_PREFIX="${1:-${PREFIX:-${HOME}/.local}}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_DIR="${SCRIPT_DIR}/build"

echo "==> Configuring hlmusic (${BUILD_TYPE}) with prefix: ${INSTALL_PREFIX}..."
cmake -B "${BUILD_DIR}" -S "${SCRIPT_DIR}" \
      -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
      -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}"

echo "==> Building hlmusic..."
cmake --build "${BUILD_DIR}" --parallel

echo "==> Installing hlmusic to ${INSTALL_PREFIX}..."
cmake --install "${BUILD_DIR}"

if command -v update-desktop-database &>/dev/null; then
    echo "==> Updating desktop MIME database..."
    update-desktop-database "${INSTALL_PREFIX}/share/applications" 2>/dev/null || true
fi

echo "==> Successfully installed hlmusic to ${INSTALL_PREFIX}"
