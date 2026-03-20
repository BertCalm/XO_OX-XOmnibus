#pragma once
// XOverworld FIREcho stub — build-only placeholder.
namespace xoverworld {
class FIREcho {
public:
    void prepare(float) {}
    void setDelay(int) {}
    void setFeedback(float) {}
    void setMix(float) {}
    void setFIRCoefficients(const float[8]) {}
    float process(float s) { return s; }
};
} // namespace xoverworld
