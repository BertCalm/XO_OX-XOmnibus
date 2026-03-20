#pragma once
// XOverworld SVFilter stub — build-only placeholder.
namespace xoverworld {
class SVFilter {
public:
    void prepare(float) {}
    void setMode(int) {}
    void setCutoff(float) {}
    void setResonance(float) {}
    float process(float s) { return s; }
};
} // namespace xoverworld
