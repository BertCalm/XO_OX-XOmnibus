# release.yml Signing Repair Plan

**Issue:** #1444 — "release.yml workflow fails on tag push: signing-secrets missing"
**Triage date:** 2026-05-03
**Status:** Blocked on user action (Apple cert provisioning)

---

## Root Cause

The `sign` job in `.github/workflows/release.yml` fails at the **"Import Developer ID Application
certificate"** step with:

```
security: SecKeychainItemImport: One or more parameters passed to a function were not valid.
```

The CI log shows both required secrets resolving to empty strings:

```
MACOS_CERTIFICATE:
MACOS_CERTIFICATE_PWD:
```

These two secrets have **never been added** to the repository's Actions secret store.
`base64 --decode` receives an empty string → produces a zero-byte file → `security import`
rejects it with the above error code.

**The workflow YAML itself is correct.** `notarytool` is already used (not the deprecated
`altool`). The keychain provisioning steps are correct for macOS 12+. No workflow edits are
needed once the secrets are populated.

---

## Secrets Required

Five secrets must be present in **Settings → Secrets and variables → Actions** before
the workflow can complete end-to-end:

| Secret name | Description |
|---|---|
| `MACOS_CERTIFICATE` | Base64-encoded Developer ID Application `.p12` export |
| `MACOS_CERTIFICATE_PWD` | The export password you used when creating the `.p12` |
| `APPLE_TEAM_ID` | Your 10-character Team ID from [developer.apple.com](https://developer.apple.com/account) |
| `APPLE_ID` | The Apple ID email associated with your Developer account |
| `APPLE_APP_PASSWORD` | An app-specific password for that Apple ID (generated at [appleid.apple.com](https://appleid.apple.com) → Sign-In and Security → App-Specific Passwords) |

---

## Step-by-Step Repair

### Step 1 — Obtain (or verify) your Developer ID Application certificate

1. Log in to [developer.apple.com/account](https://developer.apple.com/account).
2. Go to **Certificates, Identifiers & Profiles → Certificates**.
3. Confirm you have a **Developer ID Application** certificate issued to `XO_OX Designs`.
   - If it is expired or missing, click **+** and create a new one (requires a CSR generated
     in Keychain Access → Certificate Assistant).
4. Download the `.cer` file and double-click to install it in your local keychain.

### Step 2 — Export the certificate as a .p12

1. Open **Keychain Access** on your Mac.
2. Under **My Certificates**, find **Developer ID Application: XO_OX Designs (XXXXXXXXXX)**.
3. Right-click → **Export** → choose **Personal Information Exchange (.p12)**.
4. Set a strong export password — you will need this for `MACOS_CERTIFICATE_PWD`.
5. Save the file (e.g., `xoceanus-dev-id.p12`).

### Step 3 — Base64-encode the .p12

```bash
openssl base64 -in xoceanus-dev-id.p12 | pbcopy
# The base64 string is now in your clipboard
```

### Step 4 — Obtain an app-specific password

1. Go to [appleid.apple.com](https://appleid.apple.com) → **Sign-In and Security → App-Specific Passwords**.
2. Click **+**, label it something like `XOceanus CI Notarization`.
3. Copy the generated password (it looks like `xxxx-xxxx-xxxx-xxxx`).

### Step 5 — Add all five secrets to GitHub

1. Navigate to **[repo] → Settings → Secrets and variables → Actions → New repository secret**.
2. Add each secret:

   | Name | Value |
   |------|-------|
   | `MACOS_CERTIFICATE` | Paste the base64 string from Step 3 |
   | `MACOS_CERTIFICATE_PWD` | The export password from Step 2 |
   | `APPLE_TEAM_ID` | Your 10-char Team ID (e.g., `A1B2C3D4E5`) |
   | `APPLE_ID` | Your Apple ID email |
   | `APPLE_APP_PASSWORD` | The app-specific password from Step 4 |

### Step 6 — Re-trigger the release workflow

After secrets are in place, re-run against the existing tags (no new commits needed):

```bash
# Re-run the latest failed run:
gh run rerun 25158180082 --failed

# OR push a new tag to trigger fresh:
git tag v0.1.1 && git push origin v0.1.1
```

---

## Verification

A successful run will:
1. Build job completes → uploads `raw-au-component` and `raw-standalone-app` artifacts.
2. Sign job completes → uploads `signed-au-component` and `signed-standalone-app` artifacts.
3. Notarize job completes → Apple returns `Accepted` for both bundles.
4. Package job builds, signs, and notarizes the DMG.
5. Release job creates a GitHub Release at the tag with the DMG attached.

---

## If the Apple Developer Program is not yet enrolled

The Developer ID Application cert requires **Apple Developer Program** membership (~$99/year).
If not enrolled:
- Go to [developer.apple.com/programs](https://developer.apple.com/programs/enroll/).
- Enrollment takes 24-48 hours to activate.
- Once approved, follow Steps 1-6 above.

Without a Developer ID cert, macOS Gatekeeper will block users from opening the plugin.
An unsigned distribution is only viable for direct beta testers who can bypass Gatekeeper via
`xattr -dr com.apple.quarantine XOceanus.component`.

---

## No Workflow Changes Needed

The `.github/workflows/release.yml` file is architecturally correct:
- Uses `xcrun notarytool` (not deprecated `altool`)
- Correct keychain provisioning for macOS 12+
- Proper `set-key-partition-list` to avoid GUI prompts
- Cleanup step runs `if: always()` to prevent credential leaks

The only change committed on this branch is this repair plan document.
