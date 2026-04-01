# Git History Secret Scan — Pre-Launch Checklist

**Status:** PENDING — must be completed before V1 public launch.

## Required Scans

Run the following before making the repository public:

```bash
# Check for API keys (OpenAI, Anthropic, etc.)
git log --all -S "sk-" --source --oneline

# Check for RSA/PEM private keys
git log --all -S "BEGIN RSA" --source --oneline
git log --all -S "BEGIN PRIVATE KEY" --source --oneline

# Check for JWT tokens
git log --all -S "eyJ" --source --oneline

# Check for AWS credentials
git log --all -S "AKIA" --source --oneline

# Check for personal paths (privacy, not security)
git log --all -S "/Users/joshuacramblet" --source --oneline
```

If any of the above return results:
1. Verify the match is a real secret (not a test fixture or comment)
2. If real: use `git filter-repo` to scrub from history before making the repo public
3. If found in source code (not just docs): rotate the credential immediately

## .gitignore Coverage

The `.gitignore` covers the following secret file patterns:
- `*.pem`, `*.key`, `*.cert`, `*.crt` — TLS/SSL certificates and keys
- `*.pfx`, `*.p12` — PKCS containers
- `credentials.json`, `secrets/` — service account credentials
- `.env`, `.env.local` — environment variable files

## Status Log

| Date | Scanner | Result | Notes |
|------|---------|--------|-------|
| — | — | PENDING | Pre-launch scan not yet performed |
