#!/usr/bin/env bash
# build_pkg.sh — Build the XOceanus macOS .pkg installer
#
# Usage:
#   ./Installer/build_pkg.sh [Debug|Release]
#   (defaults to Release)
#
# Prerequisites:
#   - CMake build already completed (AU + Standalone artefacts present)
#   - Xcode Command Line Tools (provides pkgbuild / productbuild)
#
# Output:
#   Installer/XOceanus-Installer.pkg

set -euo pipefail

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
BUILD_CONFIG="${1:-Release}"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build"
ARTEFACTS_DIR="${BUILD_DIR}/XOceanus_artefacts/${BUILD_CONFIG}"
INSTALLER_DIR="${REPO_ROOT}/Installer"
STAGING_DIR="${INSTALLER_DIR}/_staging"
PKG_OUTPUT="${INSTALLER_DIR}/XOceanus-Installer.pkg"

AU_SRC="${ARTEFACTS_DIR}/AU/XOceanus.component"
APP_SRC="${ARTEFACTS_DIR}/Standalone/XOceanus.app"
PRESETS_SRC="${REPO_ROOT}/Presets/XOceanus"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
info()  { echo "[build_pkg] $*"; }
error() { echo "[build_pkg] ERROR: $*" >&2; exit 1; }

require_cmd() { command -v "$1" &>/dev/null || error "Required command not found: $1"; }

# ---------------------------------------------------------------------------
# Pre-flight checks
# ---------------------------------------------------------------------------
require_cmd pkgbuild
require_cmd productbuild

[[ -d "${AU_SRC}" ]]      || error "AU artefact not found: ${AU_SRC}"
[[ -d "${APP_SRC}" ]]     || error "Standalone artefact not found: ${APP_SRC}"
[[ -d "${PRESETS_SRC}" ]] || error "Presets directory not found: ${PRESETS_SRC}"

info "Build config : ${BUILD_CONFIG}"
info "Repository   : ${REPO_ROOT}"
info "AU source    : ${AU_SRC}"
info "App source   : ${APP_SRC}"
info "Presets src  : ${PRESETS_SRC}"

# ---------------------------------------------------------------------------
# Clean previous staging area
# ---------------------------------------------------------------------------
rm -rf "${STAGING_DIR}"
mkdir -p \
    "${STAGING_DIR}/au/Library/Audio/Plug-Ins/Components" \
    "${STAGING_DIR}/standalone/Applications" \
    "${STAGING_DIR}/presets/Library/Audio/Presets/XO_OX/XOceanus"

# ---------------------------------------------------------------------------
# Stage artefacts
# ---------------------------------------------------------------------------
info "Staging AU component..."
cp -R "${AU_SRC}" "${STAGING_DIR}/au/Library/Audio/Plug-Ins/Components/"

info "Staging Standalone app..."
cp -R "${APP_SRC}" "${STAGING_DIR}/standalone/Applications/"

info "Staging Presets..."
cp -R "${PRESETS_SRC}/." "${STAGING_DIR}/presets/Library/Audio/Presets/XO_OX/XOceanus/"

# ---------------------------------------------------------------------------
# Build component packages
# ---------------------------------------------------------------------------
mkdir -p "${STAGING_DIR}/pkgs"

info "Building AU component package..."
pkgbuild \
    --root "${STAGING_DIR}/au" \
    --identifier "org.xo-ox.xoceanus.au" \
    --version "1.0.0" \
    --install-location "/" \
    "${STAGING_DIR}/pkgs/XOceanus-AU.pkg"

info "Building Standalone component package..."
pkgbuild \
    --root "${STAGING_DIR}/standalone" \
    --identifier "org.xo-ox.xoceanus.standalone" \
    --version "1.0.0" \
    --install-location "/" \
    "${STAGING_DIR}/pkgs/XOceanus-Standalone.pkg"

info "Building Presets component package..."
pkgbuild \
    --root "${STAGING_DIR}/presets" \
    --identifier "org.xo-ox.xoceanus.presets" \
    --version "1.0.0" \
    --install-location "/" \
    "${STAGING_DIR}/pkgs/XOceanus-Presets.pkg"

# ---------------------------------------------------------------------------
# Build product installer
# ---------------------------------------------------------------------------
info "Assembling product installer..."
productbuild \
    --distribution "${INSTALLER_DIR}/distribution.xml" \
    --resources "${INSTALLER_DIR}/resources" \
    --package-path "${STAGING_DIR}/pkgs" \
    "${PKG_OUTPUT}"

# ---------------------------------------------------------------------------
# Done
# ---------------------------------------------------------------------------
info "Installer written to: ${PKG_OUTPUT}"
info "Done."
