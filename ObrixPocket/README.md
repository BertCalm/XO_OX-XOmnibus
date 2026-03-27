# OBRIX Pocket — Build Instructions

## Prerequisites
- Xcode 15+
- iOS 16+ deployment target
- XcodeGen (`brew install xcodegen`) — generates the .xcodeproj

## Build Steps

1. Install XcodeGen if you don't have it:
   ```bash
   brew install xcodegen
   ```

2. Generate the Xcode project:
   ```bash
   cd ObrixPocket
   xcodegen generate
   ```

3. Open in Xcode:
   ```bash
   open ObrixPocket.xcodeproj
   ```

4. Select your development team in Signing & Capabilities

5. Build and run on a physical iPhone (audio doesn't work in Simulator)

## Architecture
- **Swift/SwiftUI**: App lifecycle, tab navigation, settings
- **SpriteKit**: Reef grid rendering, creature animations, touch handling
- **JUCE (audio only)**: ObrixEngine hosting via AudioProcessorPlayer
- **ObjC++ bridge**: ObrixBridge.mm connects Swift <-> JUCE

## Project Structure
```
ObrixPocket/
├── project.yml          # XcodeGen spec
├── Sources/
│   ├── App/             # SwiftUI entry point, ContentView
│   ├── Audio/           # AudioEngineManager, ReefStore
│   ├── Bridge/          # ObrixBridge (ObjC++ <-> JUCE), JuceModules.mm
│   └── UI/              # ReefTab, ReefScene (SpriteKit)
└── Resources/
    ├── Info.plist        # App metadata + permission strings
    └── ObrixPocket.entitlements
```
