#!/usr/bin/env bash
# ObrixReef — Android SDK setup (no Android Studio required)
#
# Run this once on your dev machine to install the Android SDK,
# NDK, and build tools needed to compile ObrixReef from the command line.
#
# Usage:
#   chmod +x setup-android-sdk.sh
#   ./setup-android-sdk.sh
#
# After setup, build with:
#   cd android/obrixreef
#   ./gradlew assembleDebug
#
# Install to connected device (USB debugging enabled):
#   ./gradlew installDebug
#
# Or directly:
#   adb install app/build/outputs/apk/debug/app-debug.apk

set -euo pipefail

ANDROID_SDK_DIR="${ANDROID_HOME:-$HOME/Android/Sdk}"
CMDLINE_TOOLS_URL="https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip"

# macOS detection
if [[ "$(uname)" == "Darwin" ]]; then
    CMDLINE_TOOLS_URL="https://dl.google.com/android/repository/commandlinetools-mac-11076708_latest.zip"
fi

echo "=== ObrixReef Android SDK Setup ==="
echo "SDK location: $ANDROID_SDK_DIR"
echo ""

# 1. Download command-line tools if needed
if [ ! -f "$ANDROID_SDK_DIR/cmdline-tools/latest/bin/sdkmanager" ]; then
    echo "Downloading Android command-line tools..."
    mkdir -p "$ANDROID_SDK_DIR/cmdline-tools"
    TMPZIP=$(mktemp /tmp/cmdline-tools-XXXXXX.zip)
    curl -L -o "$TMPZIP" "$CMDLINE_TOOLS_URL"
    unzip -q "$TMPZIP" -d "$ANDROID_SDK_DIR/cmdline-tools"
    mv "$ANDROID_SDK_DIR/cmdline-tools/cmdline-tools" "$ANDROID_SDK_DIR/cmdline-tools/latest"
    rm "$TMPZIP"
    echo "Command-line tools installed."
else
    echo "Command-line tools already present."
fi

export PATH="$ANDROID_SDK_DIR/cmdline-tools/latest/bin:$ANDROID_SDK_DIR/platform-tools:$PATH"
export ANDROID_HOME="$ANDROID_SDK_DIR"

# 2. Accept licenses
echo ""
echo "Accepting SDK licenses..."
yes | sdkmanager --licenses > /dev/null 2>&1 || true

# 3. Install required packages
echo ""
echo "Installing SDK packages..."
sdkmanager --install \
    "platform-tools" \
    "platforms;android-36" \
    "build-tools;36.0.0" \
    "ndk;27.0.12077973" \
    "cmake;3.22.1"

echo ""
echo "=== Setup complete ==="
echo ""
echo "Add to your shell profile (~/.bashrc or ~/.zshrc):"
echo ""
echo "  export ANDROID_HOME=\"$ANDROID_SDK_DIR\""
echo "  export PATH=\"\$ANDROID_HOME/cmdline-tools/latest/bin:\$ANDROID_HOME/platform-tools:\$ANDROID_HOME/ndk/27.0.12077973:\$PATH\""
echo ""
echo "Then build ObrixReef:"
echo ""
echo "  cd android/obrixreef"
echo "  ./gradlew assembleDebug"
echo ""
echo "Install to your Nothing Phone (USB debugging enabled):"
echo ""
echo "  ./gradlew installDebug"
echo "  # or: adb install app/build/outputs/apk/debug/app-debug.apk"
echo ""
echo "Enable USB debugging on Nothing Phone:"
echo "  Settings → About Phone → tap Build Number 7 times"
echo "  Settings → System → Developer Options → USB Debugging → ON"
