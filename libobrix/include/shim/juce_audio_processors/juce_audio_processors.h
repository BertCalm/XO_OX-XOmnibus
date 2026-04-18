// SPDX-License-Identifier: MIT
// Shim: intercepts #include <juce_audio_processors/juce_audio_processors.h>
// and redirects to juce_compat.h which provides all necessary JUCE type stubs.
#pragma once
#include "../../obrix/juce_compat.h"
