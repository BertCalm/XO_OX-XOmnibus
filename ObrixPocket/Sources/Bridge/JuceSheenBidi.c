// JuceSheenBidi.c — Compiles the SheenBidi BiDi library as a single unity build.
// Required by juce_graphics on iOS (Bidi text layout).
// SB_CONFIG_UNITY causes SheenBidi.c to include all its .c sub-files.
#define SB_CONFIG_UNITY
#include <juce_graphics/unicode/sheenbidi/Source/SheenBidi.c>
