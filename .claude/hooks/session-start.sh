#!/bin/bash
set -euo pipefail

# Only run in remote/web environments
if [ "${CLAUDE_CODE_REMOTE:-}" != "true" ]; then
  exit 0
fi

# Install CMake + Ninja for C++ build
if ! command -v cmake &>/dev/null; then
  apt-get update -qq && apt-get install -y -qq cmake ninja-build >/dev/null 2>&1
fi

# Install Python 3 for tools
if ! command -v python3 &>/dev/null; then
  apt-get update -qq && apt-get install -y -qq python3 python3-pip >/dev/null 2>&1
fi

# Set PYTHONPATH for Tools/
echo "export PYTHONPATH=\"${CLAUDE_PROJECT_DIR:-.}\"" >> "${CLAUDE_ENV_FILE:-/dev/null}"
