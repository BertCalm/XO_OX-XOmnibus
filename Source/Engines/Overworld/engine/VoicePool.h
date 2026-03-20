#pragma once
// XOverworld VoicePool stub — build-only placeholder.
// The real DSP is in the XOverworld standalone repo.
// Replace with: git clone https://github.com/BertCalm/XOverworld ~/Documents/GitHub/XOverworld
#include <cstddef>
#include "Parameters.h"

namespace xoverworld {

class VoicePool {
public:
    void prepare(float) {}
    void allNotesOff() {}
    void reset() {}
    void noteOn(int, float) {}
    void noteOff(int) {}
    void applyParams(const ParamSnapshot&) {}
    float process(float, float, float, float, float) { return 0.0f; }
};

} // namespace xoverworld
