// JuceHarfBuzz.cc — Unity build for the HarfBuzz text shaping library bundled with JUCE 8.
// Required by juce_graphics for text layout/shaping on iOS.
// HAVE_CORETEXT must be defined for iOS so hb-coretext.cc compiles its implementation.
#if defined(__APPLE__)
 #define HAVE_CORETEXT 1
#endif
#include <juce_graphics/fonts/harfbuzz/harfbuzz.cc>
