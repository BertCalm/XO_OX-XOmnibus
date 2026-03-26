# =============================================================================
# ios-toolchain.cmake — XOlokun iOS Cross-Compilation Toolchain
# =============================================================================
#
# Supports:
#   - Physical device:              arm64 (iPhone 5s+, iPad Air+)
#   - Apple Silicon simulator:      arm64 (M1/M2/M3 Mac)
#   - Intel Mac simulator:          x86_64 (pre-M1 Mac)
#
# Usage (all three must be passed BEFORE project() is evaluated):
#
#   cmake -B build-ios -G Xcode \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/ios-toolchain.cmake \
#     [-DIOS_PLATFORM=OS|SIMULATOR64|SIMULATORARM64] \
#     [-DIOS_DEPLOYMENT_TARGET=15.0] \
#     [-DDEVELOPMENT_TEAM=YOUR10CHARID] \
#     [-DCODE_SIGN_IDENTITY="iPhone Developer"] \
#     [-DPRODUCT_BUNDLE_IDENTIFIER=com.xo-ox.xolokun]
#
# The default IOS_PLATFORM is OS (physical device, arm64).
# Switch to SIMULATORARM64 for Apple Silicon host, SIMULATOR64 for Intel host.
#
# CMake requires the toolchain file to be processed BEFORE project() so that
# CMAKE_SYSTEM_NAME is known when the compiler is first probed. Pass this file
# via -DCMAKE_TOOLCHAIN_FILE on the initial configure invocation only.
# =============================================================================

# Guard: prevent double-inclusion across CMake's two-pass toolchain loading.
if(_XO_IOS_TOOLCHAIN_LOADED)
    return()
endif()
set(_XO_IOS_TOOLCHAIN_LOADED TRUE)

# =============================================================================
# 1. Platform selection
#    IOS_PLATFORM values:
#      OS             — physical device (arm64)
#      SIMULATORARM64 — Apple Silicon Mac simulator (arm64)
#      SIMULATOR64    — Intel Mac simulator    (x86_64)
# =============================================================================

if(NOT DEFINED IOS_PLATFORM)
    set(IOS_PLATFORM "OS" CACHE STRING
        "iOS target platform: OS | SIMULATORARM64 | SIMULATOR64" FORCE)
endif()

set(_VALID_PLATFORMS OS SIMULATORARM64 SIMULATOR64)
if(NOT IOS_PLATFORM IN_LIST _VALID_PLATFORMS)
    message(FATAL_ERROR
        "[ios-toolchain] Unknown IOS_PLATFORM '${IOS_PLATFORM}'. "
        "Valid values: OS, SIMULATORARM64, SIMULATOR64")
endif()

# =============================================================================
# 2. CMAKE_SYSTEM_NAME — MUST be set before project() is processed.
#    CMake's Platform/iOS.cmake module uses this to configure compiler probing.
# =============================================================================

set(CMAKE_SYSTEM_NAME     "iOS"   CACHE STRING "" FORCE)
set(CMAKE_SYSTEM_VERSION  "15.0"  CACHE STRING "" FORCE)

# =============================================================================
# 3. Deployment target
# =============================================================================

if(NOT DEFINED IOS_DEPLOYMENT_TARGET)
    set(IOS_DEPLOYMENT_TARGET "15.0" CACHE STRING
        "Minimum iOS deployment target (JUCE 8 supports 14.0+)" FORCE)
endif()

# CMake uses CMAKE_OSX_DEPLOYMENT_TARGET for all Apple platforms including iOS.
set(CMAKE_OSX_DEPLOYMENT_TARGET "${IOS_DEPLOYMENT_TARGET}" CACHE STRING "" FORCE)

# =============================================================================
# 4. SDK and sysroot selection
# =============================================================================

if(IOS_PLATFORM STREQUAL "OS")
    # ── Physical device ───────────────────────────────────────────────────────
    set(_XO_IOS_SDK         "iphoneos")
    set(_XO_IOS_ARCHS       "arm64")
    set(_XO_IOS_SYSROOT_HINT
        "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs")
else()
    # ── Simulator ─────────────────────────────────────────────────────────────
    set(_XO_IOS_SDK         "iphonesimulator")
    set(_XO_IOS_SYSROOT_HINT
        "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs")
    if(IOS_PLATFORM STREQUAL "SIMULATORARM64")
        set(_XO_IOS_ARCHS   "arm64")
    else()  # SIMULATOR64
        set(_XO_IOS_ARCHS   "x86_64")
    endif()
endif()

# Resolve the sysroot via xcrun at configure time so the exact SDK version
# (e.g., iPhoneOS26.2.sdk) is always picked up without hard-coding it.
find_program(_XCRUN xcrun REQUIRED)
execute_process(
    COMMAND "${_XCRUN}" --sdk "${_XO_IOS_SDK}" --show-sdk-path
    OUTPUT_VARIABLE _XO_IOS_SYSROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _XCRUN_RESULT
)
if(_XCRUN_RESULT)
    # xcrun not available or SDK not installed — fall back to a known path.
    message(WARNING
        "[ios-toolchain] xcrun failed (exit ${_XCRUN_RESULT}). "
        "Attempting fallback sysroot under ${_XO_IOS_SYSROOT_HINT}. "
        "Install Xcode and the iOS SDK to fix this.")
    # Glob for the newest SDK directory under the platform hint.
    file(GLOB _SDK_DIRS "${_XO_IOS_SYSROOT_HINT}/*.sdk")
    if(_SDK_DIRS)
        list(SORT _SDK_DIRS)
        list(GET _SDK_DIRS -1 _XO_IOS_SYSROOT)   # pick the last (newest)
    else()
        message(FATAL_ERROR
            "[ios-toolchain] No iOS SDK found under ${_XO_IOS_SYSROOT_HINT}. "
            "Install Xcode from the Mac App Store.")
    endif()
endif()

set(CMAKE_OSX_SYSROOT "${_XO_IOS_SYSROOT}" CACHE STRING "" FORCE)

message(STATUS "[ios-toolchain] Platform  : ${IOS_PLATFORM}")
message(STATUS "[ios-toolchain] SDK       : ${_XO_IOS_SDK}")
message(STATUS "[ios-toolchain] Sysroot   : ${_XO_IOS_SYSROOT}")
message(STATUS "[ios-toolchain] Archs     : ${_XO_IOS_ARCHS}")
message(STATUS "[ios-toolchain] MinOS     : ${IOS_DEPLOYMENT_TARGET}")

# =============================================================================
# 5. Architecture
#    CMAKE_OSX_ARCHITECTURES is the canonical CMake variable on Apple targets.
# =============================================================================

set(CMAKE_OSX_ARCHITECTURES "${_XO_IOS_ARCHS}" CACHE STRING "" FORCE)

# =============================================================================
# 6. Compiler — Clang from the active Xcode toolchain
# =============================================================================

find_program(_XCRUN_CLANG clang
    HINTS "${_XO_IOS_SYSROOT}/../../../../../../Toolchains/XcodeDefault.xctoolchain/usr/bin"
    NO_DEFAULT_PATH)

if(NOT _XCRUN_CLANG)
    # Resolve via xcrun as a reliable fallback.
    execute_process(
        COMMAND "${_XCRUN}" --sdk "${_XO_IOS_SDK}" --find clang
        OUTPUT_VARIABLE _XCRUN_CLANG
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

if(_XCRUN_CLANG)
    set(CMAKE_C_COMPILER   "${_XCRUN_CLANG}" CACHE FILEPATH "" FORCE)
    # clang++ is always adjacent to clang.
    string(REPLACE "/clang" "/clang++" _XCRUN_CLANGXX "${_XCRUN_CLANG}")
    if(EXISTS "${_XCRUN_CLANGXX}")
        set(CMAKE_CXX_COMPILER "${_XCRUN_CLANGXX}" CACHE FILEPATH "" FORCE)
    endif()
endif()

# =============================================================================
# 7. Code signing — overridable from the command line.
#
#    Replace XXXXXXXXXX with your 10-character Apple Developer Team ID.
#    Find it at: https://developer.apple.com/account  →  Membership → Team ID
#    Or: security find-identity -v -p codesigning
#
#    DEVELOPMENT_TEAM  — required for device builds; optional for simulator.
#    CODE_SIGN_IDENTITY — "iPhone Developer" covers both dev certificates.
# =============================================================================

if(NOT DEFINED DEVELOPMENT_TEAM)
    set(DEVELOPMENT_TEAM "XXXXXXXXXX" CACHE STRING
        "Apple Developer Team ID (10-char). Override: -DDEVELOPMENT_TEAM=ABCDE12345")
endif()

if(NOT DEFINED CODE_SIGN_IDENTITY)
    if(IOS_PLATFORM STREQUAL "OS")
        set(CODE_SIGN_IDENTITY "iPhone Developer" CACHE STRING
            "Code signing identity for device builds")
    else()
        # Simulator builds do not require a signing identity; use the null
        # identity so CMake/Xcode does not prompt or fail when no provisioning
        # profile is installed.
        set(CODE_SIGN_IDENTITY "-" CACHE STRING
            "Code signing identity (simulator: '-' = ad-hoc/none)")
    endif()
endif()

# Propagate to Xcode build settings via CMAKE_XCODE_ATTRIBUTE_* variables.
# These apply to every target in the generated Xcode project.
set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM     "${DEVELOPMENT_TEAM}"    CACHE STRING "" FORCE)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY   "${CODE_SIGN_IDENTITY}"  CACHE STRING "" FORCE)

# Automatic signing: let Xcode manage provisioning profiles when a valid team
# ID is provided. Set to NO if you manage profiles manually.
if(DEVELOPMENT_TEAM STREQUAL "XXXXXXXXXX")
    # Placeholder: no real team → disable automatic signing to avoid Xcode
    # errors about missing provisioning profiles during CI/configure checks.
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual" CACHE STRING "" FORCE)
    message(STATUS
        "[ios-toolchain] Code signing DISABLED (placeholder Team ID). "
        "Pass -DDEVELOPMENT_TEAM=ABCDE12345 to enable.")
else()
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic" CACHE STRING "" FORCE)
endif()

# =============================================================================
# 8. Bundle identifier stub
#    Override: -DPRODUCT_BUNDLE_IDENTIFIER=com.yourcompany.xolokun
#    This is also set in CMakeLists.txt via BUNDLE_ID on juce_add_plugin(), so
#    the toolchain value serves as a fallback for ancillary targets (tests, etc).
# =============================================================================

if(NOT DEFINED PRODUCT_BUNDLE_IDENTIFIER)
    set(PRODUCT_BUNDLE_IDENTIFIER "com.xo-ox.xolokun" CACHE STRING
        "App/plugin bundle identifier. Override: -DPRODUCT_BUNDLE_IDENTIFIER=com.yourco.xolokun")
endif()

set(CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER
    "${PRODUCT_BUNDLE_IDENTIFIER}" CACHE STRING "" FORCE)

# =============================================================================
# 9. Bitcode — DISABLED
#    Apple deprecated bitcode in Xcode 14 (2022) and removed support in
#    Xcode 15+. Enabling it causes build warnings/errors with modern SDKs.
#    JUCE 8 also does not enable bitcode.
# =============================================================================

set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE "NO" CACHE STRING "" FORCE)

# =============================================================================
# 10. Simulator-specific: allow only-active-arch for faster iteration.
#     On device, always build the full arch set (just arm64 here).
# =============================================================================

if(IOS_PLATFORM STREQUAL "OS")
    set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "NO" CACHE STRING "" FORCE)
else()
    # Simulator iterative builds: only compile the host arch.
    set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES" CACHE STRING "" FORCE)
endif()

# =============================================================================
# 11. AUv3 / App Extension API only
#     AUv3 plugins are app extensions. They must not call UIApplication APIs
#     that are unavailable to extensions. Setting this attribute causes the
#     linker to error immediately if a forbidden API is referenced — catching
#     extension-incompatible code at link time rather than App Store review.
#
#     Note: only applies to the AUv3 extension target, not the Standalone app.
#     JUCE's juce_add_plugin() sets this per-target; this global attribute
#     provides a project-wide safety net.
# =============================================================================

set(CMAKE_XCODE_ATTRIBUTE_APPLICATION_EXTENSION_API_ONLY "YES" CACHE STRING "" FORCE)

# =============================================================================
# 12. Swift — disabled (XOlokun is pure C++/JUCE; no Swift sources)
# =============================================================================

set(CMAKE_XCODE_ATTRIBUTE_SWIFT_OPTIMIZATION_LEVEL "-Onone" CACHE STRING "" FORCE)

# =============================================================================
# 13. Search paths — tell CMake's find_* functions not to look in macOS
#     framework dirs or host system paths when cross-compiling for iOS.
# =============================================================================

set(CMAKE_FIND_ROOT_PATH "${_XO_IOS_SYSROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# =============================================================================
# 14. Skip host-only executables
#     CMake tries to link and run test executables when detecting platform
#     capabilities. This cannot work when cross-compiling for iOS.
# =============================================================================

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# =============================================================================
# 15. Suppress noisy legacy deprecation warnings from SDK headers
#     (iOS 26 SDK deprecates some Carbon-era CoreMIDI calls; JUCE still uses
#     them internally — suppress at the toolchain level so they don't bury
#     XOlokun diagnostics.)
# =============================================================================

set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_ABOUT_DEPRECATED_FUNCTIONS "NO" CACHE STRING "" FORCE)

# =============================================================================
# Summary banner (visible at configure time)
# =============================================================================

message(STATUS "──────────────────────────────────────────────────")
message(STATUS " XOlokun iOS Toolchain configured")
message(STATUS "  Platform  : ${IOS_PLATFORM} (${_XO_IOS_ARCHS})")
message(STATUS "  MinOS     : ${IOS_DEPLOYMENT_TARGET}")
message(STATUS "  SDK       : ${_XO_IOS_SYSROOT}")
message(STATUS "  Team ID   : ${DEVELOPMENT_TEAM}")
message(STATUS "  Sign ID   : ${CODE_SIGN_IDENTITY}")
message(STATUS "  Bundle    : ${PRODUCT_BUNDLE_IDENTIFIER}")
message(STATUS "  Bitcode   : DISABLED")
message(STATUS "──────────────────────────────────────────────────")
