// FamilyWaveguideTest.cpp
// Standalone C++ test for FamilyWaveguide.h — no JUCE required.
//
// Build from XO_OX-XOlokun root:
//   mkdir -p Tests/build && cd Tests/build
//   cmake .. -DCMAKE_CXX_STANDARD=17 && make
//   ./FamilyWaveguideTest
//
// All 20 assertions must pass for SP1 quality gate.

#include "../Source/DSP/FamilyWaveguide.h"
#include <cstdio>
#include <cmath>

namespace {

static int sPassed = 0;
static int sFailed = 0;

void check (bool condition, const char* name, const char* msg)
{
    if (condition) {
        printf ("[PASS] %s\n", name);
        ++sPassed;
    } else {
        printf ("[FAIL] %s — %s\n", name, msg);
        ++sFailed;
    }
}

// Run a Karplus-Strong loop for numSamples. Returns peak and final amplitude.
struct KSResult { float peak; float finalAmp; bool noNaN; };

KSResult runKS (xolokun::FamilyDelayLine& dl, xolokun::FamilyDampingFilter& df,
                float delayLen, int numSamples, float damping)
{
    float peak = 0.0f, finalAmp = 0.0f;
    bool noNaN = true;
    for (int i = 0; i < numSamples; ++i)
    {
        float out = dl.read (delayLen);
        float fed = df.process (out, damping);
        dl.write (fed);
        if (!std::isfinite (out)) noNaN = false;
        float amp = std::abs (out);
        if (amp > peak) peak = amp;
        if (i == numSamples - 1) finalAmp = amp;
    }
    return { peak, finalAmp, noNaN };
}

} // anonymous namespace

int main()
{
    const double SR       = 44100.0;
    const float  pitch    = 440.0f;
    const float  delayLen = static_cast<float> (SR) / pitch; // ~100.2 samples

    // -----------------------------------------------------------------------
    // 0. FamilyDelayLine Lagrange — interpolated read is continuous (no step at integer boundaries)
    // A correct Lagrange implementation produces smooth values as frac sweeps 0→1.
    // A wrong formula would produce large discontinuities at integer frac values.
    // -----------------------------------------------------------------------
    {
        xolokun::FamilyDelayLine dl;
        dl.prepare (512);
        // Write a simple ramp so we can predict expected values
        for (int i = 0; i < 200; ++i)
            dl.write (static_cast<float> (i) * 0.01f);

        // Read at 50.0, 50.25, 50.5, 50.75, 51.0 — values must be monotonic
        float v0 = dl.read (50.0f);
        float v1 = dl.read (50.25f);
        float v2 = dl.read (50.5f);
        float v3 = dl.read (50.75f);
        float v4 = dl.read (51.0f);
        bool monotonic = (v0 >= v1) && (v1 >= v2) && (v2 >= v3) && (v3 >= v4);
        // (ramp in buffer → shorter delay = more recent = higher value → monotonic decreasing)
        check (monotonic, "DelayLine_Lagrange_monotonic",
               "Lagrange read not smooth across fractional range — formula incorrect");
        check (std::isfinite(v0) && std::isfinite(v2) && std::isfinite(v4),
               "DelayLine_Lagrange_finite", "Lagrange read returned NaN/Inf");
    }

    // -----------------------------------------------------------------------
    // 1. FamilyDelayLine — basic read is finite
    // -----------------------------------------------------------------------
    {
        xolokun::FamilyDelayLine dl;
        dl.prepare (static_cast<int> (SR) + 8);
        dl.write (1.0f);
        for (int i = 0; i < 10; ++i) dl.write (0.0f);
        float r = dl.read (5.0f);
        check (std::isfinite (r), "DelayLine_read_finite", "Read returned NaN/Inf");
    }

    // -----------------------------------------------------------------------
    // 2. Karplus-Strong loop — no NaN
    // -----------------------------------------------------------------------
    {
        xolokun::FamilyDelayLine  dl;
        xolokun::FamilyDampingFilter df;
        dl.prepare (static_cast<int> (SR) + 8);
        df.prepare();
        // Seed with impulse
        for (int i = 0; i < static_cast<int> (delayLen) + 1; ++i)
            dl.write (i == 0 ? 0.5f : 0.0f);
        auto res = runKS (dl, df, delayLen, static_cast<int> (SR * 2.0f), 0.995f);
        check (res.noNaN,            "KS_no_NaN",      "NaN/Inf in KS loop output");
        check (res.peak > 0.01f,     "KS_peak_nonzero","KS loop peak near zero");
        check (res.finalAmp < res.peak * 0.5f, "KS_decays", "KS did not decay after 2s");
    }

    // -----------------------------------------------------------------------
    // 3. PluckExciter — fires, decays, goes silent
    // -----------------------------------------------------------------------
    {
        xolokun::PluckExciter pe;
        pe.prepare (SR);
        pe.trigger (2.0f);
        float maxOut = 0.0f;
        bool  anyNaN = false;
        for (int i = 0; i < 2000; ++i) {
            float s = pe.tick (0.5f);
            if (!std::isfinite (s)) anyNaN = true;
            maxOut = std::max (maxOut, std::abs (s));
        }
        check (!anyNaN,        "PluckExciter_no_NaN",    "PluckExciter NaN/Inf");
        check (maxOut > 1e-6f, "PluckExciter_nonzero",   "PluckExciter no output after trigger");
        // Past burst window — should be silent
        float late = 0.0f;
        for (int i = 0; i < 500; ++i) late += std::abs (pe.tick (0.5f));
        check (late < 1e-6f,   "PluckExciter_silent",    "PluckExciter still outputting past burst");
    }

    // -----------------------------------------------------------------------
    // 4. StrumExciter — multi-string staggered output
    // -----------------------------------------------------------------------
    {
        xolokun::StrumExciter se;
        se.prepare (SR);
        se.trigger (4, 5.0f, 1.0f);
        float maxOut = 0.0f;
        bool  anyNaN = false;
        for (int i = 0; i < 6000; ++i) {
            float s = se.tick (0.5f);
            if (!std::isfinite (s)) anyNaN = true;
            maxOut = std::max (maxOut, std::abs (s));
        }
        check (!anyNaN,        "StrumExciter_no_NaN",  "StrumExciter NaN/Inf");
        check (maxOut > 1e-6f, "StrumExciter_nonzero", "StrumExciter no output");
    }

    // -----------------------------------------------------------------------
    // 5. PickExciter — attack burst, non-zero
    // -----------------------------------------------------------------------
    {
        xolokun::PickExciter pe;
        pe.prepare (SR);
        pe.trigger (1.5f);
        float maxOut = 0.0f;
        bool  anyNaN = false;
        for (int i = 0; i < 1000; ++i) {
            float s = pe.tick (0.7f);
            if (!std::isfinite (s)) anyNaN = true;
            maxOut = std::max (maxOut, std::abs (s));
        }
        check (!anyNaN,        "PickExciter_no_NaN",  "PickExciter NaN/Inf");
        check (maxOut > 1e-6f, "PickExciter_nonzero", "PickExciter no output");
    }

    // -----------------------------------------------------------------------
    // 6. AirJetExciter — sound at full breath, near-silent at zero
    // -----------------------------------------------------------------------
    {
        xolokun::AirJetExciter ae;
        ae.prepare (SR);
        float maxOut = 0.0f;
        bool  anyNaN = false;
        for (int i = 0; i < 1000; ++i) {
            float s = ae.tick (1.0f, 440.0f);
            if (!std::isfinite (s)) anyNaN = true;
            maxOut = std::max (maxOut, std::abs (s));
        }
        check (!anyNaN,        "AirJetExciter_no_NaN",       "AirJetExciter NaN/Inf");
        check (maxOut > 1e-6f, "AirJetExciter_full_breath",  "AirJetExciter silent at full breath");
        ae.reset();
        float silentSum = 0.0f;
        for (int i = 0; i < 500; ++i) silentSum += std::abs (ae.tick (0.0f, 440.0f));
        check (silentSum < 0.05f, "AirJetExciter_zero_breath", "AirJetExciter too loud at zero breath");
    }

    // -----------------------------------------------------------------------
    // 7. ReedExciter — continuous output at breathPressure > 0
    // -----------------------------------------------------------------------
    {
        xolokun::ReedExciter re;
        re.prepare (SR);
        float maxOut = 0.0f;
        bool  anyNaN = false;
        for (int i = 0; i < 1000; ++i) {
            float s = re.tick (0.8f, 0.5f);
            if (!std::isfinite (s)) anyNaN = true;
            maxOut = std::max (maxOut, std::abs (s));
        }
        check (!anyNaN,        "ReedExciter_no_NaN",  "ReedExciter NaN/Inf");
        check (maxOut > 1e-6f, "ReedExciter_nonzero", "ReedExciter no output");
    }

    // -----------------------------------------------------------------------
    // 8. LipBuzzExciter — oscillates at given frequency
    // -----------------------------------------------------------------------
    {
        xolokun::LipBuzzExciter le;
        le.prepare (SR);
        float maxOut = 0.0f;
        bool  anyNaN = false;
        for (int i = 0; i < 2000; ++i) {
            float s = le.tick (220.0f, 0.5f, 0.5f);
            if (!std::isfinite (s)) anyNaN = true;
            maxOut = std::max (maxOut, std::abs (s));
        }
        check (!anyNaN,        "LipBuzzExciter_no_NaN",  "LipBuzzExciter NaN/Inf");
        check (maxOut > 1e-6f, "LipBuzzExciter_nonzero", "LipBuzzExciter no output");
    }

    // -----------------------------------------------------------------------
    // 9. BowExciter — non-zero with pressure, silent at zero pressure
    // -----------------------------------------------------------------------
    {
        xolokun::BowExciter be;
        be.prepare (SR);
        float maxOut = 0.0f;
        bool  anyNaN = false;
        for (int i = 0; i < 1000; ++i) {
            float s = be.tick (0.8f, 0.5f, 0.1f);
            if (!std::isfinite (s)) anyNaN = true;
            maxOut = std::max (maxOut, std::abs (s));
        }
        check (!anyNaN,        "BowExciter_no_NaN",       "BowExciter NaN/Inf");
        check (maxOut > 1e-6f, "BowExciter_with_pressure","BowExciter silent with bow on string");
        be.reset();
        float zeroSum = 0.0f;
        for (int i = 0; i < 100; ++i) zeroSum += std::abs (be.tick (0.0f, 0.5f, 0.0f));
        check (zeroSum < 1e-6f, "BowExciter_lifted",      "BowExciter not silent when bow lifted");
    }

    // -----------------------------------------------------------------------
    // 10. FamilyBodyResonance — resonates from impulse, no NaN
    // -----------------------------------------------------------------------
    {
        xolokun::FamilyBodyResonance br;
        br.prepare (SR);
        br.setParams (440.0f, 5.0f);
        float maxOut = 0.0f;
        bool  anyNaN = false;
        for (int i = 0; i < 2000; ++i) {
            float s = br.process (i == 0 ? 1.0f : 0.0f);
            if (!std::isfinite (s)) anyNaN = true;
            maxOut = std::max (maxOut, std::abs (s));
        }
        check (!anyNaN,        "BodyResonance_no_NaN",  "BodyResonance NaN/Inf");
        check (maxOut > 1e-6f, "BodyResonance_resonates","BodyResonance no output from impulse");
    }

    // -----------------------------------------------------------------------
    // 11. FamilySympatheticBank — sympathetic bloom after input
    // -----------------------------------------------------------------------
    {
        xolokun::FamilySympatheticBank sb;
        sb.prepare (SR, 512);
        sb.tune (440.0f);
        float maxOut = 0.0f;
        bool  anyNaN = false;
        for (int i = 0; i < 4000; ++i) {
            float input = (i < 100) ? 0.5f : 0.0f;
            float s = sb.process (input, 1.0f);
            if (!std::isfinite (s)) anyNaN = true;
            maxOut = std::max (maxOut, std::abs (s));
        }
        check (!anyNaN,         "SympatheticBank_no_NaN",  "SympatheticBank NaN/Inf");
        check (maxOut > 1e-8f,  "SympatheticBank_resonates","SympatheticBank no output after impulse");
    }

    // -----------------------------------------------------------------------
    // 12. FamilyOrganicDrift — output within ±1 semitone, non-zero
    // -----------------------------------------------------------------------
    {
        xolokun::FamilyOrganicDrift od;
        od.prepare (SR);
        float maxDrift = 0.0f;
        bool  anyNaN   = false;
        for (int i = 0; i < 44100; ++i) {
            float d = od.tick (0.2f, 5.0f); // 0.2 Hz rate, ±5 cents
            if (!std::isfinite (d)) anyNaN = true;
            maxDrift = std::max (maxDrift, std::abs (d));
        }
        check (!anyNaN,           "OrganicDrift_no_NaN",     "OrganicDrift NaN/Inf");
        check (maxDrift < 0.1f,   "OrganicDrift_in_range",   "OrganicDrift exceeds ±1 semitone");
        check (maxDrift > 1e-6f,  "OrganicDrift_nonzero",    "OrganicDrift always zero");
    }

    // -----------------------------------------------------------------------
    printf ("\n=== FamilyWaveguide Tests: %d passed, %d failed ===\n", sPassed, sFailed);
    return (sFailed == 0) ? 0 : 1;
}
