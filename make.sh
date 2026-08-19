#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BIN_DIR="${HOME}/.local/bin"
APP_DIR="${HOME}/.local/share/applications"

# Ensure user directories exist (~/.local/bin and ~/.local/share/applications)
mkdir -p "${BIN_DIR}" "${APP_DIR}" "${BUILD_DIR}"

echo "==> Building hyprmusic..."
cd "${BUILD_DIR}"
cmake "${SCRIPT_DIR}" -DCMAKE_INSTALL_PREFIX="${HOME}/.local"
make -j$(nproc 2>/dev/null || echo 1)

echo "==> Installing binary and desktop file to ~/.local..."
cp -f "${BUILD_DIR}/hyprmusic" "${BIN_DIR}/hyprmusic"

if [ -f "${SCRIPT_DIR}/hyprmusic.desktop" ]; then
    cp -f "${SCRIPT_DIR}/hyprmusic.desktop" "${APP_DIR}/hyprmusic.desktop"
    echo "==> Updating desktop MIME database..."
    update-desktop-database "${APP_DIR}" 2>/dev/null || true
fi

echo "==> Successfully installed:"
echo "    - Executable: ${BIN_DIR}/hyprmusic"
echo "    - Desktop File: ${APP_DIR}/hyprmusic.desktop"

