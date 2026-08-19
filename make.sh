#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
INSTALL_PREFIX="${HOME}/.local"

# Ensure user installation directories exist
mkdir -p "${INSTALL_PREFIX}/bin" "${INSTALL_PREFIX}/share/applications"

echo "==> Preparing build directory..."
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo "==> Running CMake configuration..."
cmake "${SCRIPT_DIR}" -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}"

echo "==> Building hyprmusic..."
make -j$(nproc 2>/dev/null || echo 1)

echo "==> Installing to ${INSTALL_PREFIX}..."
make install

echo "==> Done! Installed hyprmusic to ${INSTALL_PREFIX}/bin/hyprmusic and desktop file to ${INSTALL_PREFIX}/share/applications/hyprmusic.desktop"
