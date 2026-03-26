# XOlokun — iOS Build Guide

Toolchain file: `cmake/ios-toolchain.cmake`

All commands must be run from the repo root (`~/Documents/GitHub/XO_OX-XOmnibus`).
Requires Xcode 14+ and the iOS SDK (install via `xcode-select --install` or the Mac App Store).

Replace `ABCDE12345` with your real 10-character Apple Developer Team ID.
Find it at https://developer.apple.com/account → Membership → Team ID.

---

## Physical Device Build (arm64)

```bash
cmake -B build-ios -G Xcode \
  -DCMAKE_TOOLCHAIN_FILE=cmake/ios-toolchain.cmake \
  -DIOS_PLATFORM=OS \
  -DIOS_DEPLOYMENT_TARGET=15.0 \
  -DDEVELOPMENT_TEAM=ABCDE12345 \
  -DCODE_SIGN_IDENTITY="iPhone Developer"

cmake --build build-ios --config Release
```

A signing certificate and provisioning profile are required for device builds.
See the Xcode Signing & Capabilities tab, or use automatic signing with a valid team ID.

---

## Simulator Build — Apple Silicon Mac (arm64)

Use this when your development Mac has an M1, M2, or M3 chip.

```bash
cmake -B build-ios-sim-arm64 -G Xcode \
  -DCMAKE_TOOLCHAIN_FILE=cmake/ios-toolchain.cmake \
  -DIOS_PLATFORM=SIMULATORARM64 \
  -DIOS_DEPLOYMENT_TARGET=15.0

cmake --build build-ios-sim-arm64 --config Debug
```

No provisioning profile needed. The toolchain sets `CODE_SIGN_IDENTITY="-"` (ad-hoc) automatically.

---

## Simulator Build — Intel Mac (x86_64)

Use this on a pre-M1 Mac.

```bash
cmake -B build-ios-sim-x64 -G Xcode \
  -DCMAKE_TOOLCHAIN_FILE=cmake/ios-toolchain.cmake \
  -DIOS_PLATFORM=SIMULATOR64 \
  -DIOS_DEPLOYMENT_TARGET=15.0

cmake --build build-ios-sim-x64 --config Debug
```

---

## Opening in Xcode

After CMake configure, open the generated project directly in Xcode:

```bash
open build-ios/XOlokun.xcodeproj
```

From Xcode you can select a device/simulator, set the active scheme (XOlokun_AUv3 or XOlokun_Standalone), and use Product → Run.

---

## Optional CMake Variables

| Variable | Default | Description |
|---|---|---|
| `IOS_PLATFORM` | `OS` | `OS`, `SIMULATORARM64`, or `SIMULATOR64` |
| `IOS_DEPLOYMENT_TARGET` | `15.0` | Minimum iOS version |
| `DEVELOPMENT_TEAM` | `XXXXXXXXXX` (placeholder) | 10-char Apple Team ID |
| `CODE_SIGN_IDENTITY` | `iPhone Developer` (device) or `-` (sim) | Signing certificate name |
| `PRODUCT_BUNDLE_IDENTIFIER` | `com.xo-ox.xolokun` | Bundle ID override |

---

## AU Validation (macOS only — not applicable to iOS builds)

```bash
auval -v aumu Xolk XoOx
```

iOS AUv3 validation is performed by the host (GarageBand, AUM, etc.) at load time.

---

## Notes

- Bitcode is disabled — Apple deprecated it in Xcode 14 and removed support in Xcode 15+.
- The toolchain sets `APPLICATION_EXTENSION_API_ONLY=YES` to catch forbidden APIs at link time (AUv3 is an app extension).
- `XO_MOBILE=1` and `XO_FORMATS=AUv3;Standalone` are set automatically by `CMakeLists.txt` when `CMAKE_SYSTEM_NAME=iOS`.
- iOS 15.0 is the minimum; JUCE 8 supports iOS 14.0+, so 15.0 is safe and covers Swift Concurrency infrastructure if ever needed.
