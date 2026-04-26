#!/usr/bin/env bash
# Wires .githooks/ as the hooks directory for this repo.
# Run once after clone: ./Tools/enable-git-hooks.sh
git config core.hooksPath .githooks
echo "✓ Git hooks enabled (.githooks/)"
echo "  Pre-commit: compile check, dead-param guard, audio thread lint, clang-format"
echo "  Pre-push: incremental build sanity check"
echo "  Bypass once: git push --no-verify"
