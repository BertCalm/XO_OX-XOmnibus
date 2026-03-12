# /build — Build Project & Verify Artifacts

Build the XOmnibus project for a target platform, verify expected artifacts exist, and report results.

## Usage

- `/build` — Build macOS (default)
- `/build ios` — Build iOS
- `/build clean` — Clean build directory first, then build macOS
- `/build all` — Build both macOS and iOS

## Process

### 1. Pre-flight checks
- Verify CMakeLists.txt exists at project root
- Verify JUCE is present at `Libs/JUCE/`
- Check for Ninja: `which ninja` (fall back to Unix Makefiles if missing)

### 2. Configure (if needed)
Only re-configure if `build/` directory doesn't exist or `clean` was requested.

**macOS:**
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

**iOS:**
```bash
cmake -B build-ios -G Xcode \
  -DCMAKE_TOOLCHAIN_FILE=ios-toolchain.cmake \
  -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=${DEVELOPMENT_TEAM_ID:-YOUR_TEAM_ID}
```

### 3. Build
```bash
cmake --build build --config Release
```

Capture stdout and stderr. Time the build.

### 4. Verify artifacts

**macOS — expect both:**
- `build/XOmnibus_artefacts/Release/AU/XOmnibus.component`
- `build/XOmnibus_artefacts/Release/Standalone/XOmnibus.app`

**iOS — expect both:**
- `build-ios/XOmnibus_artefacts/Release-iphoneos/AUv3/XOmnibus.appex`
- `build-ios/XOmnibus_artefacts/Release-iphoneos/Standalone/XOmnibus.app`

### 5. Report

```markdown
## Build Report
- **Platform:** macOS / iOS / both
- **Generator:** Ninja / Xcode
- **Duration:** {time}
- **Status:** SUCCESS / FAILED
- **Artifacts:**
  - ✓ AU component (path, size)
  - ✓ Standalone app (path, size)
- **Warnings:** {count} (list first 5 if any)
- **Errors:** {list if failed}
```

### 6. On failure
- Parse the error output for the first compilation error
- Identify the file and line number
- Read that file and surrounding context
- Suggest a fix or report the issue clearly

## Primitives Used
- `chain-pipeline` — configure → build → verify → report (gate at each step)

## Notes
- Never modify source files during build — this is a read-only verification skill
- If build directory exists and is recent (< 1 hour), skip configure step
- Always report artifact sizes — sudden size changes indicate problems
