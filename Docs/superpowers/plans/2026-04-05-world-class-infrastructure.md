# World-Class Infrastructure Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the 7 verified infrastructure gaps to bring XOceanus from "production-grade" to "world-class" — code signing, crash reporting, dependency automation, audio regression baselines, release safety, sanitizer coverage, and test coverage reporting.

**Architecture:** Each task is independent and can be implemented in any order. Tasks 1-4 are high-impact; Tasks 5-7 are quick wins that should be done first to build momentum.

**Tech Stack:** GitHub Actions, CMake, Apple codesign/notarytool, Sentry Native SDK, Dependabot, LCOV/gcov, Catch2 v3

---

### Task 1: Dependabot Configuration (Quick Win)

**Files:**
- Create: `.github/dependabot.yml`

- [ ] **Step 1: Create Dependabot config**

```yaml
# .github/dependabot.yml
# Automated dependency update monitoring for XOceanus
version: 2
updates:
  # GitHub Actions — keep action versions current
  - package-ecosystem: "github-actions"
    directory: "/"
    schedule:
      interval: "weekly"
      day: "monday"
    labels:
      - "dependencies"
      - "ci"
    commit-message:
      prefix: "ci"

  # Python tools (oxport, preset utilities)
  - package-ecosystem: "pip"
    directory: "/Tools"
    schedule:
      interval: "weekly"
      day: "monday"
    labels:
      - "dependencies"
      - "python"
    commit-message:
      prefix: "chore"
```

- [ ] **Step 2: Verify syntax**

Run: `python3 -c "import yaml; yaml.safe_load(open('.github/dependabot.yml'))"`
Expected: No output (valid YAML)

- [ ] **Step 3: Commit**

```bash
git add .github/dependabot.yml
git commit -m "ci: add Dependabot for GitHub Actions and Python dependency updates"
```

---

### Task 2: Add UBSan to Sanitizer Matrix (Quick Win)

**Files:**
- Modify: `.github/workflows/sanitizers.yml`

- [ ] **Step 1: Add ubsan to the matrix**

In `.github/workflows/sanitizers.yml`, add a third entry to the `matrix.include` array:

```yaml
    strategy:
      fail-fast: false
      matrix:
        sanitizer: [asan, tsan, ubsan]
        include:
          - sanitizer: asan
            cxx_flags: "-fsanitize=address -fno-omit-frame-pointer"
            c_flags: "-fsanitize=address -fno-omit-frame-pointer"
            linker_flags: "-fsanitize=address"
          - sanitizer: tsan
            cxx_flags: "-fsanitize=thread"
            c_flags: "-fsanitize=thread"
            linker_flags: "-fsanitize=thread"
          - sanitizer: ubsan
            cxx_flags: "-fsanitize=undefined -fno-omit-frame-pointer -fno-sanitize=vptr"
            c_flags: "-fsanitize=undefined -fno-omit-frame-pointer"
            linker_flags: "-fsanitize=undefined"
```

Note: `-fno-sanitize=vptr` is needed because JUCE uses dynamic_cast across shared library boundaries which triggers false positives with vptr checks.

- [ ] **Step 2: Commit**

```bash
git add .github/workflows/sanitizers.yml
git commit -m "ci: add UndefinedBehaviorSanitizer (UBSan) to sanitizer matrix"
```

---

### Task 3: Fix Release Workflow — Build Verification Gate (Quick Win)

**Files:**
- Modify: `.github/workflows/release.yml`

The current `release.yml` creates a GitHub release without verifying the build passes. This means a broken build could be tagged and released.

- [ ] **Step 1: Add build + test steps before version bump**

Add these steps after "Validate version format" and before "Update VERSION in CMakeLists.txt":

```yaml
      - name: Cache JUCE
        id: cache-juce
        uses: actions/cache@v4
        with:
          path: Libs/JUCE
          key: juce-8.0.4

      - name: Clone JUCE 8.0.4
        if: steps.cache-juce.outputs.cache-hit != 'true'
        run: |
          mkdir -p Libs
          git clone https://github.com/juce-framework/JUCE.git Libs/JUCE
          cd Libs/JUCE && git checkout 51d11a2be6d5c97ccf12b4e5e827006e19f0555a

      - name: Install Ninja
        run: brew install ninja

      - name: Build and verify before release
        run: |
          cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
          cmake --build build

      - name: Run tests
        run: |
          cd build
          ctest --output-on-failure --timeout 120

      - name: Verify AU component exists
        run: |
          if [ ! -d "build/XOceanus_artefacts/Release/AU/XOceanus.component" ]; then
            echo "ERROR: AU component not found — aborting release"
            exit 1
          fi

      - name: Run auval validation
        run: |
          cp -R build/XOceanus_artefacts/Release/AU/XOceanus.component ~/Library/Audio/Plug-Ins/Components/
          auval -v aumu Xocn XoOx
```

- [ ] **Step 2: Add build artifact upload to the release**

Modify the "Create GitHub release" step to attach the AU and Standalone artifacts:

```yaml
      - name: Create GitHub release
        env:
          GH_TOKEN: ${{ github.token }}
        run: |
          VERSION="${{ github.event.inputs.version }}"

          # Create a zip of the AU component for attachment
          cd build/XOceanus_artefacts/Release
          zip -r "${GITHUB_WORKSPACE}/XOceanus-AU-v${VERSION}.zip" AU/XOceanus.component
          zip -r "${GITHUB_WORKSPACE}/XOceanus-Standalone-v${VERSION}.zip" Standalone/XOceanus.app
          cd "${GITHUB_WORKSPACE}"

          gh release create "v${VERSION}" \
            --title "XOceanus v${VERSION}" \
            --generate-notes \
            "XOceanus-AU-v${VERSION}.zip#AU Plugin (macOS)" \
            "XOceanus-Standalone-v${VERSION}.zip#Standalone App (macOS)"
```

- [ ] **Step 3: Commit**

```bash
git add .github/workflows/release.yml
git commit -m "ci: add build verification gate + artifact upload to release workflow"
```

---

### Task 4: Test Coverage Reporting with LCOV

**Files:**
- Modify: `CMakeLists.txt` (add coverage option)
- Create: `.github/workflows/coverage.yml`

- [ ] **Step 1: Add coverage build option to CMakeLists.txt**

After the existing `option(XOCEANUS_BUILD_VST3 ...)` line (around line 37), add:

```cmake
option(XOCEANUS_BUILD_COVERAGE "Build with code coverage instrumentation (gcov/llvm-cov)" OFF)

if(XOCEANUS_BUILD_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
        add_link_options(--coverage)
    else()
        message(WARNING "Coverage instrumentation not supported for this compiler")
    endif()
endif()
```

- [ ] **Step 2: Create the coverage workflow**

```yaml
# .github/workflows/coverage.yml
name: Test Coverage

on:
  push:
    branches: [main]
  workflow_dispatch:

env:
  MACOSX_DEPLOYMENT_TARGET: '12.0'

jobs:
  coverage:
    runs-on: macos-latest

    steps:
      - name: Checkout repo
        uses: actions/checkout@v4

      - name: Cache JUCE
        id: cache-juce
        uses: actions/cache@v4
        with:
          path: Libs/JUCE
          key: juce-8.0.4

      - name: Clone JUCE 8.0.4
        if: steps.cache-juce.outputs.cache-hit != 'true'
        run: |
          mkdir -p Libs
          git clone https://github.com/juce-framework/JUCE.git Libs/JUCE
          cd Libs/JUCE && git checkout 51d11a2be6d5c97ccf12b4e5e827006e19f0555a

      - name: Install tools
        run: brew install ninja lcov

      - name: Configure CMake with coverage
        run: |
          cmake -B build -G Ninja \
            -DCMAKE_BUILD_TYPE=Debug \
            -DXOCEANUS_BUILD_COVERAGE=ON

      - name: Build
        run: cmake --build build

      - name: Run tests
        run: |
          cd build
          ctest --output-on-failure --timeout 120

      - name: Generate coverage report
        run: |
          # Capture coverage data
          lcov --capture --directory build \
            --output-file build/coverage.info \
            --ignore-errors mismatch

          # Remove external code (JUCE, Catch2, system headers)
          lcov --remove build/coverage.info \
            '*/Libs/*' '*/Tests/*' '*/catch2/*' '/usr/*' '/Applications/*' \
            --output-file build/coverage-filtered.info \
            --ignore-errors unused

          # Generate HTML report
          genhtml build/coverage-filtered.info \
            --output-directory build/coverage-report \
            --title "XOceanus Test Coverage"

          # Summary for GitHub
          echo "## Test Coverage Summary" >> $GITHUB_STEP_SUMMARY
          echo '```' >> $GITHUB_STEP_SUMMARY
          lcov --summary build/coverage-filtered.info 2>&1 | tee -a $GITHUB_STEP_SUMMARY
          echo '```' >> $GITHUB_STEP_SUMMARY

      - name: Upload coverage report
        uses: actions/upload-artifact@v4
        with:
          name: coverage-report
          path: build/coverage-report/
```

- [ ] **Step 3: Commit**

```bash
git add CMakeLists.txt .github/workflows/coverage.yml
git commit -m "ci: add LCOV test coverage reporting workflow"
```

---

### Task 5: Code Signing & Notarization Pipeline

**Files:**
- Create: `.github/workflows/sign-and-notarize.yml`
- Modify: `Installer/build_pkg.sh` (add signing steps)

**Prerequisites:** The following GitHub Secrets must be configured by the repo owner:
- `APPLE_DEVELOPER_ID_P12` — Base64-encoded Developer ID Application certificate
- `APPLE_DEVELOPER_ID_PASSWORD` — Certificate password
- `APPLE_INSTALLER_ID_P12` — Base64-encoded Developer ID Installer certificate
- `APPLE_INSTALLER_ID_PASSWORD` — Installer certificate password
- `APPLE_ID` — Apple ID email for notarization
- `APPLE_APP_PASSWORD` — App-specific password for notarization
- `APPLE_TEAM_ID` — Apple Developer Team ID

- [ ] **Step 1: Create the signing workflow**

```yaml
# .github/workflows/sign-and-notarize.yml
name: Sign & Notarize

on:
  workflow_dispatch:
    inputs:
      version:
        description: 'Version to sign (must match an existing release tag)'
        required: true
        type: string

env:
  MACOSX_DEPLOYMENT_TARGET: '12.0'

jobs:
  sign:
    runs-on: macos-latest

    steps:
      - name: Checkout repo
        uses: actions/checkout@v4
        with:
          ref: "v${{ github.event.inputs.version }}"

      - name: Cache JUCE
        id: cache-juce
        uses: actions/cache@v4
        with:
          path: Libs/JUCE
          key: juce-8.0.4

      - name: Clone JUCE 8.0.4
        if: steps.cache-juce.outputs.cache-hit != 'true'
        run: |
          mkdir -p Libs
          git clone https://github.com/juce-framework/JUCE.git Libs/JUCE
          cd Libs/JUCE && git checkout 51d11a2be6d5c97ccf12b4e5e827006e19f0555a

      - name: Install Ninja
        run: brew install ninja

      - name: Build Release
        run: |
          cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
          cmake --build build

      - name: Import signing certificates
        env:
          APPLE_DEVELOPER_ID_P12: ${{ secrets.APPLE_DEVELOPER_ID_P12 }}
          APPLE_DEVELOPER_ID_PASSWORD: ${{ secrets.APPLE_DEVELOPER_ID_PASSWORD }}
          APPLE_INSTALLER_ID_P12: ${{ secrets.APPLE_INSTALLER_ID_P12 }}
          APPLE_INSTALLER_ID_PASSWORD: ${{ secrets.APPLE_INSTALLER_ID_PASSWORD }}
        run: |
          # Create temporary keychain
          KEYCHAIN_PATH=$RUNNER_TEMP/signing.keychain-db
          KEYCHAIN_PASSWORD=$(openssl rand -hex 16)

          security create-keychain -p "$KEYCHAIN_PASSWORD" "$KEYCHAIN_PATH"
          security set-keychain-settings -lut 21600 "$KEYCHAIN_PATH"
          security unlock-keychain -p "$KEYCHAIN_PASSWORD" "$KEYCHAIN_PATH"

          # Import Developer ID Application certificate
          echo "$APPLE_DEVELOPER_ID_P12" | base64 --decode > $RUNNER_TEMP/dev_cert.p12
          security import $RUNNER_TEMP/dev_cert.p12 \
            -P "$APPLE_DEVELOPER_ID_PASSWORD" \
            -A -t cert -f pkcs12 \
            -k "$KEYCHAIN_PATH"

          # Import Developer ID Installer certificate
          echo "$APPLE_INSTALLER_ID_P12" | base64 --decode > $RUNNER_TEMP/inst_cert.p12
          security import $RUNNER_TEMP/inst_cert.p12 \
            -P "$APPLE_INSTALLER_ID_PASSWORD" \
            -A -t cert -f pkcs12 \
            -k "$KEYCHAIN_PATH"

          # Set keychain search list
          security list-keychains -d user -s "$KEYCHAIN_PATH" $(security list-keychains -d user | tr -d '"')
          security set-key-partition-list -S apple-tool:,apple: -s -k "$KEYCHAIN_PASSWORD" "$KEYCHAIN_PATH"

      - name: Sign AU component
        run: |
          codesign --deep --force --options runtime \
            --sign "Developer ID Application: XO_OX Designs (${{ secrets.APPLE_TEAM_ID }})" \
            --timestamp \
            build/XOceanus_artefacts/Release/AU/XOceanus.component

      - name: Sign Standalone app
        run: |
          codesign --deep --force --options runtime \
            --sign "Developer ID Application: XO_OX Designs (${{ secrets.APPLE_TEAM_ID }})" \
            --timestamp \
            build/XOceanus_artefacts/Release/Standalone/XOceanus.app

      - name: Build signed PKG installer
        run: |
          ./Installer/build_pkg.sh Release

          # Sign the PKG with Developer ID Installer certificate
          productsign --sign "Developer ID Installer: XO_OX Designs (${{ secrets.APPLE_TEAM_ID }})" \
            Installer/XOceanus-Installer.pkg \
            Installer/XOceanus-Installer-signed.pkg

          mv Installer/XOceanus-Installer-signed.pkg Installer/XOceanus-Installer.pkg

      - name: Create DMG
        run: |
          cd build && cpack -G DragNDrop

      - name: Sign DMG
        run: |
          DMG_FILE=$(ls build/XOceanus-*.dmg)
          codesign --force \
            --sign "Developer ID Application: XO_OX Designs (${{ secrets.APPLE_TEAM_ID }})" \
            --timestamp \
            "$DMG_FILE"

      - name: Notarize PKG
        env:
          APPLE_ID: ${{ secrets.APPLE_ID }}
          APPLE_APP_PASSWORD: ${{ secrets.APPLE_APP_PASSWORD }}
          APPLE_TEAM_ID: ${{ secrets.APPLE_TEAM_ID }}
        run: |
          xcrun notarytool submit Installer/XOceanus-Installer.pkg \
            --apple-id "$APPLE_ID" \
            --password "$APPLE_APP_PASSWORD" \
            --team-id "$APPLE_TEAM_ID" \
            --wait --timeout 30m

          xcrun stapler staple Installer/XOceanus-Installer.pkg

      - name: Notarize DMG
        env:
          APPLE_ID: ${{ secrets.APPLE_ID }}
          APPLE_APP_PASSWORD: ${{ secrets.APPLE_APP_PASSWORD }}
          APPLE_TEAM_ID: ${{ secrets.APPLE_TEAM_ID }}
        run: |
          DMG_FILE=$(ls build/XOceanus-*.dmg)

          xcrun notarytool submit "$DMG_FILE" \
            --apple-id "$APPLE_ID" \
            --password "$APPLE_APP_PASSWORD" \
            --team-id "$APPLE_TEAM_ID" \
            --wait --timeout 30m

          xcrun stapler staple "$DMG_FILE"

      - name: Upload signed artifacts
        uses: actions/upload-artifact@v4
        with:
          name: XOceanus-signed-v${{ github.event.inputs.version }}
          path: |
            Installer/XOceanus-Installer.pkg
            build/XOceanus-*.dmg

      - name: Attach to GitHub release
        env:
          GH_TOKEN: ${{ github.token }}
        run: |
          VERSION="${{ github.event.inputs.version }}"
          DMG_FILE=$(ls build/XOceanus-*.dmg)

          gh release upload "v${VERSION}" \
            "Installer/XOceanus-Installer.pkg#Signed Installer (macOS)" \
            "${DMG_FILE}#Signed DMG (macOS)" \
            --clobber

      - name: Cleanup keychain
        if: always()
        run: |
          security delete-keychain $RUNNER_TEMP/signing.keychain-db 2>/dev/null || true
          rm -f $RUNNER_TEMP/dev_cert.p12 $RUNNER_TEMP/inst_cert.p12
```

- [ ] **Step 2: Commit**

```bash
git add .github/workflows/sign-and-notarize.yml
git commit -m "ci: add code signing and notarization workflow for macOS distribution"
```

---

### Task 6: Crash Reporting with Sentry Native SDK

**Files:**
- Modify: `CMakeLists.txt` (add Sentry option + FetchContent)
- Create: `Source/Core/CrashReporter.h`
- Modify: `Source/XOceanusProcessor.cpp` (initialize on startup)

**Prerequisites:** The repo owner must:
1. Create a Sentry project at sentry.io (free tier available)
2. Add `SENTRY_DSN` as a GitHub Secret
3. Optionally add `SENTRY_DSN` as a CMake define for local builds

- [ ] **Step 1: Add Sentry build option to CMakeLists.txt**

After the `XOCEANUS_BUILD_COVERAGE` option, add:

```cmake
option(XOCEANUS_BUILD_CRASH_REPORTING "Build with Sentry crash reporting (opt-in at runtime)" OFF)

if(XOCEANUS_BUILD_CRASH_REPORTING)
    include(FetchContent)
    FetchContent_Declare(
        sentry
        GIT_REPOSITORY https://github.com/getsentry/sentry-native.git
        GIT_TAG        0.7.17
        GIT_SHALLOW    TRUE
    )
    set(SENTRY_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SENTRY_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(sentry)

    target_link_libraries(XOceanus PRIVATE sentry::sentry)
    target_compile_definitions(XOceanus PRIVATE XOCEANUS_CRASH_REPORTING=1)

    if(DEFINED SENTRY_DSN)
        target_compile_definitions(XOceanus PRIVATE SENTRY_DSN="${SENTRY_DSN}")
    endif()
endif()
```

- [ ] **Step 2: Create CrashReporter.h**

```cpp
// Source/Core/CrashReporter.h
// Opt-in crash reporting via Sentry Native SDK.
// Gated behind XOCEANUS_CRASH_REPORTING build flag (default OFF).
// User must explicitly enable via preferences — no data sent without consent.

#pragma once

#if XOCEANUS_CRASH_REPORTING
#include <sentry.h>
#endif

namespace xoceanus
{

class CrashReporter
{
public:
    static void initialize()
    {
#if XOCEANUS_CRASH_REPORTING
#ifdef SENTRY_DSN
        sentry_options_t* options = sentry_options_new();
        sentry_options_set_dsn(options, SENTRY_DSN);
        sentry_options_set_release(options, "xoceanus@" XOCEANUS_VERSION_STRING);
        sentry_options_set_environment(options, "production");

        // Privacy: only send crash reports, not breadcrumbs or user data
        sentry_options_set_max_breadcrumbs(options, 0);
        sentry_options_set_require_user_consent(options, 1);

        sentry_init(options);
#endif
#endif
    }

    static void setUserConsent(bool granted)
    {
#if XOCEANUS_CRASH_REPORTING
        sentry_user_consent_give();
        if (!granted)
            sentry_user_consent_revoke();
#endif
        (void)granted;
    }

    static void shutdown()
    {
#if XOCEANUS_CRASH_REPORTING
        sentry_close();
#endif
    }
};

} // namespace xoceanus
```

- [ ] **Step 3: Commit**

```bash
git add CMakeLists.txt Source/Core/CrashReporter.h
git commit -m "feat: add opt-in Sentry crash reporting (gated behind XOCEANUS_BUILD_CRASH_REPORTING)"
```

---

### Task 7: Bootstrap Golden Audio Regression Baselines

**Files:**
- Modify: `Tests/RegressionTests/AudioRegressionTests.cpp`

**Prerequisites:** Must be run locally after a successful build. Cannot be done in CI because baselines must be captured from the actual DSP output.

- [ ] **Step 1: Build and run regression tests locally**

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build && ./XOceanusTests "[regression]" 2>&1
```

Expected: Three FAIL messages with actual RMS/peak values, like:
```
BOOTSTRAP NEEDED — paste into GOLDEN_OBRIX: { 0.0234, 0.1567 }
BOOTSTRAP NEEDED — paste into GOLDEN_ONSET: { 0.0891, 0.4523 }
BOOTSTRAP NEEDED — paste into GOLDEN_OUROBOROS: { 0.0456, 0.2134 }
```

- [ ] **Step 2: Paste the actual values into AudioRegressionTests.cpp**

Replace the three `-1.0` baselines with the actual captured values:

```cpp
// Replace these lines (around line 51-53):
static constexpr GoldenValues GOLDEN_OBRIX = {-1.0, -1.0};
static constexpr GoldenValues GOLDEN_ONSET = {-1.0, -1.0};
static constexpr GoldenValues GOLDEN_OUROBOROS = {-1.0, -1.0};

// With the actual values from step 1:
static constexpr GoldenValues GOLDEN_OBRIX = {/* RMS from output */, /* peak from output */};
static constexpr GoldenValues GOLDEN_ONSET = {/* RMS from output */, /* peak from output */};
static constexpr GoldenValues GOLDEN_OUROBOROS = {/* RMS from output */, /* peak from output */};
```

- [ ] **Step 3: Verify baselines pass**

```bash
cd build && cmake --build . && ./XOceanusTests "[regression]"
```

Expected: All 3 tests PASS

- [ ] **Step 4: Commit**

```bash
git add Tests/RegressionTests/AudioRegressionTests.cpp
git commit -m "test: bootstrap golden audio regression baselines for Obrix, Onset, Ouroboros"
```

---

## Execution Order

**Phase 1 — Quick Wins (Tasks 1-3):** ~15 minutes total, no prerequisites
1. Dependabot config
2. UBSan in sanitizers
3. Release workflow build gate

**Phase 2 — Medium Effort (Task 4):** ~20 minutes, no prerequisites
4. Coverage reporting workflow

**Phase 3 — Requires Credentials (Tasks 5-6):** Scaffold now, activate when secrets are configured
5. Code signing & notarization pipeline
6. Sentry crash reporting

**Phase 4 — Requires Local Build (Task 7):** Must be done by the developer locally
7. Golden audio baselines

## Post-Implementation Verification

After all tasks are committed, verify:
- [ ] `git diff main` shows only new/modified workflow files + CrashReporter.h + CMakeLists.txt changes
- [ ] Push to a feature branch and confirm build.yml passes
- [ ] Open a PR and confirm sanitizers.yml now runs asan + tsan + ubsan
- [ ] Verify Dependabot creates its first PR within 24 hours of merge
- [ ] Manually trigger coverage.yml and check the HTML report artifact
