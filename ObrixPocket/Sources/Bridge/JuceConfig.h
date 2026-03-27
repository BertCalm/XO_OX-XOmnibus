#pragma once

// JUCE configuration — included by every JuceModule_*.mm translation unit.
// Must appear before any JUCE header is pulled in.

#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#define JUCE_IOS 1
#define JUCE_MODULE_AVAILABLE_juce_audio_basics      1
#define JUCE_MODULE_AVAILABLE_juce_audio_devices     1
#define JUCE_MODULE_AVAILABLE_juce_audio_formats     1
#define JUCE_MODULE_AVAILABLE_juce_audio_processors  1
#define JUCE_MODULE_AVAILABLE_juce_audio_utils       1
#define JUCE_MODULE_AVAILABLE_juce_core              1
#define JUCE_MODULE_AVAILABLE_juce_data_structures   1
#define JUCE_MODULE_AVAILABLE_juce_dsp               1
#define JUCE_MODULE_AVAILABLE_juce_events            1
#define JUCE_DISPLAY_SPLASH_SCREEN    0
#define JUCE_USE_DARK_SPLASH_SCREEN   0
#define JUCE_STANDALONE_APPLICATION   0
#define JUCE_REPORT_APP_USAGE         0
#define JUCE_WEB_BROWSER              0
#define JUCE_USE_CURL                 0
