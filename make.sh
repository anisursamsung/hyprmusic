#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_PREFIX="${1:-${HOME}/.local}"

echo "==> Configuring hlmusic..."
cmake -B "${SCRIPT_DIR}/build" -S "${SCRIPT_DIR}" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}"

echo "==> Building hlmusic..."
cmake --build "${SCRIPT_DIR}/build" -j$(nproc 2>/dev/null || echo 1)

echo "==> Installing hlmusic to ${INSTALL_PREFIX}..."
cmake --install "${SCRIPT_DIR}/build"

if command -v update-desktop-database &>/dev/null; then
    echo "==> Updating desktop MIME database..."
    update-desktop-database "${INSTALL_PREFIX}/share/applications" 2>/dev/null || true
fi

echo "==> Successfully installed hlmusic to ${INSTALL_PREFIX}"
