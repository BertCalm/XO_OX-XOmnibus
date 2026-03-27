// Single translation unit for JUCE module implementations.
// This file compiles all JUCE audio modules needed by ObrixBridge.
// No JUCE GUI modules are included — UI is SwiftUI/SpriteKit.

#define JUCE_IOS 1
#define JUCE_MODULE_AVAILABLE_juce_audio_basics 1
#define JUCE_MODULE_AVAILABLE_juce_audio_devices 1
#define JUCE_MODULE_AVAILABLE_juce_audio_formats 1
#define JUCE_MODULE_AVAILABLE_juce_audio_processors 1
#define JUCE_MODULE_AVAILABLE_juce_audio_utils 1
#define JUCE_MODULE_AVAILABLE_juce_core 1
#define JUCE_MODULE_AVAILABLE_juce_data_structures 1
#define JUCE_MODULE_AVAILABLE_juce_dsp 1
#define JUCE_MODULE_AVAILABLE_juce_events 1
#define JUCE_MODULE_AVAILABLE_juce_graphics 1

// Required JUCE config
#define JUCE_DISPLAY_SPLASH_SCREEN 0
#define JUCE_USE_DARK_SPLASH_SCREEN 0
#define JUCE_STANDALONE_APPLICATION 1
#define JUCE_REPORT_APP_USAGE 0
#define JUCE_WEB_BROWSER 0
#define JUCE_USE_CURL 0

// Include JUCE module implementations
#include <juce_core/juce_core.mm>
#include <juce_data_structures/juce_data_structures.mm>
#include <juce_events/juce_events.mm>
#include <juce_graphics/juce_graphics.mm>
#include <juce_audio_basics/juce_audio_basics.mm>
#include <juce_audio_devices/juce_audio_devices.mm>
#include <juce_audio_formats/juce_audio_formats.mm>
#include <juce_audio_processors/juce_audio_processors.mm>
#include <juce_audio_utils/juce_audio_utils.mm>
#include <juce_dsp/juce_dsp.mm>
