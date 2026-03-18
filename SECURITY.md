# Security Policy

## Reporting a Vulnerability

If you discover a security vulnerability in XOmnibus, please report it responsibly:

**Do NOT file a public GitHub issue for security vulnerabilities.**

Instead, email: **security@xo-ox.org**

Include:
- Description of the vulnerability
- Steps to reproduce
- Potential impact
- Suggested fix (if you have one)

You will receive an acknowledgment within 48 hours. Fixes for confirmed vulnerabilities will be prioritized and credited in release notes (unless you prefer to remain anonymous).

## Scope

XOmnibus is an audio plugin that processes audio signals locally. The primary security concerns are:
- Malicious preset files (`.xometa` JSON parsing)
- Malicious XPN expansion packs (ZIP extraction)
- Buffer overflows in DSP processing
- Path traversal in file loading

## Supported Versions

| Version | Supported |
|---------|-----------|
| Latest release | ✅ |
| Older releases | Best effort |
