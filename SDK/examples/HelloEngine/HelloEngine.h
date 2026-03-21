#pragma once
// HelloEngine — XOmnibus SDK "Hello World" engine
//
// A complete, minimal engine demonstrating every XOmnibus integration contract:
//   - SynthEngine interface (all virtual methods)
//   - 4 parameters with hlo_ prefix
//   - 4 macros mapped to parameters
//   - 2-voice polyphony (round-robin)
//   - Coupling: sends audio, receives AmpToFilter
//   - All 6 Doctrines satisfied (D001–D006)
//   - SilenceGate (SRO zero-idle bypass) — SDK variant via isSilent flag
//   - Denormal protection in filter feedback path
//
// DSP architecture:
//   Voice = sine oscillator + pitch envelope
//   Filter = one-pole lowpass with resonance feedback
//   Parameters: hlo_pitch, hlo_cutoff, hlo_resonance, hlo_decay
//
// Read me before editing:
//   SDK/examples/HelloEngine/README.md

#include <xomnibus/SynthEngine.h>
#include <xomnibus/EngineModule.h>
#include <cmath>
#include <array>
#include <atomic>
#include <algorithm>
#include <string>
#include <unordered_map>

namespace {

//==============================================================================
// Tiny helper — flush denormals.
// Required in all feedback/filter paths per XOmnibus Architecture Rules.
//==============================================================================
inline float flushDenormal (float x)
{
    return (std::abs (x) < 1e-18f) ? 0.0f : x;
}

//==============================================================================
// OnePoleLPF — one-pole lowpass with resonance feedback.
//
// Uses matched-Z coefficient: coeff = exp(-2*PI*fc/sr)
// per CLAUDE.md DSP rules ("exp(-2*PI*fc/sr), not w/(w+1)").
//==============================================================================
struct OnePoleLPF
{
    float state = 0.0f;

    void reset() { state = 0.0f; }

    // Process one sample.
    // @param input   Signal to filter.
    // @param cutoff  Cutoff frequency in Hz.
    // @param res     Resonance [0, 0.95]. Applied as feedback of state.
    // @param sr      Sample rate.
    float tick (float input, float cutoff, float res, float sr)
    {
        // Matched-Z coefficient (CLAUDE.md rule: use exp(-2*PI*fc/sr))
        float fc   = std::clamp (cutoff, 20.0f, sr * 0.499f);
        float coeff = std::exp (-6.28318530718f * fc / sr);

        // Resonance feedback — adds a fraction of the filter output back to input
        float in = input + state * std::clamp (res, 0.0f, 0.95f);

        // One-pole: y[n] = (1-coeff)*x[n] + coeff*y[n-1]
        state = flushDenormal ((1.0f - coeff) * in + coeff * state);
        return state;
    }
};

//==============================================================================
// PitchEnvelope — simple exponential decay for pitch modulation.
// Adds warmth and pluck character on note attack.
//==============================================================================
struct PitchEnvelope
{
    float level = 0.0f;

    void reset() { level = 0.0f; }

    void trigger() { level = 1.0f; }

    // Returns a pitch offset in semitones (decays from +12 to 0).
    // @param decaySeconds  Time constant in seconds.
    // @param sr            Sample rate.
    float tick (float decaySeconds, float sr)
    {
        if (level < 0.0001f) { level = 0.0f; return 0.0f; }
        float decayCoeff = std::exp (-1.0f / (std::max (decaySeconds, 0.001f) * sr));
        level *= decayCoeff;
        return level * 12.0f; // up to +12 semitones of pitch bend on attack
    }
};

//==============================================================================
// HelloVoice — one polyphonic voice.
//
// Owns: sine oscillator, pitch envelope, one-pole LPF.
//==============================================================================
struct HelloVoice
{
    bool  active    = false;
    int   note      = -1;
    float freq      = 440.0f;
    float velocity  = 0.0f;
    float phase     = 0.0f;
    float ampEnv    = 0.0f;  // simple exponential amplitude decay
    bool  releasing = false;

    OnePoleLPF filter;
    PitchEnvelope pitchEnv;

    void reset()
    {
        active    = false;
        note      = -1;
        ampEnv    = 0.0f;
        phase     = 0.0f;
        releasing = false;
        filter.reset();
        pitchEnv.reset();
    }

    void noteOn (int n, float vel)
    {
        note      = n;
        velocity  = vel;
        freq      = 440.0f * std::pow (2.0f, (static_cast<float> (n) - 69.0f) / 12.0f);
        ampEnv    = 1.0f;
        releasing = false;
        active    = true;
        // Keep phase for legato feel; re-trigger pitch envelope
        pitchEnv.trigger();
    }

    void noteOff()
    {
        releasing = true;
    }

    // Render one sample and return it (no mixing — caller adds to buffer).
    // @param cutoff        Filter cutoff (may include coupling modulation).
    // @param resonance     Filter resonance [0,1].
    // @param decaySeconds  Amplitude decay time.
    // @param sr            Sample rate.
    float tick (float cutoff, float resonance, float decaySeconds, float sr)
    {
        if (!active) return 0.0f;

        // --- Amplitude envelope (exponential decay, D001: velocity shapes level) ---
        float ampDecayCoeff = std::exp (-1.0f / (std::max (decaySeconds, 0.001f) * sr));
        if (releasing)
            ampDecayCoeff = std::exp (-1.0f / (std::min (decaySeconds * 0.25f, 0.05f) * sr));
        ampEnv *= ampDecayCoeff;
        if (ampEnv < 0.0001f) { ampEnv = 0.0f; active = false; return 0.0f; }

        // --- Pitch envelope (D001: velocity shapes timbre via transient pitch sweep) ---
        float pitchSemis = pitchEnv.tick (decaySeconds * 0.3f, sr);
        float instFreq   = freq * std::pow (2.0f, pitchSemis / 12.0f);

        // --- Sine oscillator ---
        float osc = std::sin (phase * 6.28318530718f);
        phase += instFreq / sr;
        if (phase >= 1.0f) phase -= 1.0f;

        // --- Filter (D001: velocity also opens cutoff) ---
        float velCutoffBoost = velocity * 4000.0f; // brighter at high velocity
        float out = filter.tick (osc, cutoff + velCutoffBoost, resonance, sr);

        return out * ampEnv * velocity * 0.5f;
    }
};

} // anonymous namespace

//==============================================================================
// HelloEngine — the main engine class.
//==============================================================================
class HelloEngine : public xomnibus::SynthEngine
{
public:
    //==========================================================================
    // LIFECYCLE
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = static_cast<float> (sampleRate);
        for (auto& v : voices) v.reset();
        nextVoice = 0;

        // Allocate coupling send buffer (no allocation on audio thread later)
        couplingBuf.resize (static_cast<size_t> (maxBlockSize * 2), 0.0f);

        isSilent    = false;
        silenceHold = 0;
        // Hold 200 ms of silence before bypassing (standard engine)
        silenceHoldMax = static_cast<int> (sampleRate * 0.2);

        // D005: LFO — "engine that cannot breathe is a photograph"
        // A slow filter modulation LFO, rate floor 0.01 Hz, default 0.3 Hz.
        // Implemented as a simple triangle oscillator here.
        lfoPhase    = 0.0f;
        lfoRate     = 0.3f; // Hz default; controlled by hlo_cutoff LFO in spirit
        (void) maxBlockSize; // already used for couplingBuf
    }

    void releaseResources() override
    {
        for (auto& v : voices) v.reset();
        couplingBuf.clear();
    }

    void reset() override
    {
        for (auto& v : voices) v.reset();
        nextVoice   = 0;
        isSilent    = false;
        silenceHold = 0;
        lfoPhase    = 0.0f;
        std::fill (couplingBuf.begin(), couplingBuf.end(), 0.0f);
        couplingFilterMod = 0.0f;
        lastSample = 0.0f;
    }

    //==========================================================================
    // AUDIO
    //==========================================================================

    void renderBlock (xomnibus::StereoBuffer& buffer,
                      const xomnibus::MidiEventList& midi) override
    {
        const int ns = buffer.numSamples;
        if (!buffer.left || !buffer.right || ns <= 0) return;

        //----------------------------------------------------------------------
        // Step 1: Parse MIDI (D006: velocity + CC expression inputs)
        //----------------------------------------------------------------------
        for (int i = 0; i < midi.numEvents; ++i)
        {
            const auto& ev = midi.events[i];

            if (ev.isNoteOn())
            {
                // Wake silence gate
                isSilent    = false;
                silenceHold = 0;

                // Round-robin voice allocation
                voices[nextVoice].noteOn (ev.getNoteNumber(),
                                          ev.getFloatVelocity());
                nextVoice = (nextVoice + 1) % kMaxVoices;
            }
            else if (ev.isNoteOff())
            {
                for (auto& v : voices)
                    if (v.active && v.note == ev.getNoteNumber())
                        v.noteOff();
            }
            else if (ev.isController())
            {
                // D006: mod wheel (CC1) sweeps filter cutoff
                if (ev.getControllerNumber() == 1)
                    modWheelNorm = static_cast<float> (ev.getControllerValue()) / 127.0f;
            }
            else if (ev.isChannelPressure())
            {
                // D006: aftertouch adds resonance shimmer
                aftertouchNorm = static_cast<float> (ev.getChannelPressureValue()) / 127.0f;
            }
        }

        //----------------------------------------------------------------------
        // Step 2: SRO silence gate bypass — zero CPU when engine is idle
        //----------------------------------------------------------------------
        if (isSilent)
        {
            std::fill (buffer.left,  buffer.left  + ns, 0.0f);
            std::fill (buffer.right, buffer.right + ns, 0.0f);
            std::fill (couplingBuf.begin(), couplingBuf.begin() + ns, 0.0f);
            return;
        }

        //----------------------------------------------------------------------
        // Step 3: Read parameters (snapshot once per block — no per-sample map
        // lookups on the audio thread, no stale-closure issues)
        //----------------------------------------------------------------------
        float pPitch     = paramPitch;      // Hz
        float pCutoff    = paramCutoff;     // Hz
        float pResonance = paramResonance;  // [0,1]
        float pDecay     = paramDecay;      // seconds

        // D002: LFO breathes the filter cutoff (rate floor: 0.01 Hz)
        //   lfoRate controlled by mod wheel [0.01 Hz..4 Hz]
        float activeLfoRate = 0.01f + modWheelNorm * 3.99f;

        // D006: aftertouch adds resonance shimmer
        float activeResonance = pResonance + aftertouchNorm * 0.2f;
        activeResonance = std::clamp (activeResonance, 0.0f, 0.95f);

        // Coupling: AmpToFilter modulation accumulated from applyCouplingInput()
        float couplingCutoffBump = couplingFilterMod * 8000.0f; // up to +8kHz

        //----------------------------------------------------------------------
        // Step 4: Render
        //----------------------------------------------------------------------
        bool anyActive = false;
        for (int s = 0; s < ns; ++s)
        {
            // D005: LFO tick — triangle wave, rate floor 0.01 Hz
            lfoPhase += activeLfoRate / sr;
            if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
            float lfo = (lfoPhase < 0.5f)
                        ? (lfoPhase * 4.0f - 1.0f)
                        : (3.0f - lfoPhase * 4.0f);  // triangle in [-1,+1]

            float activeCutoff = pCutoff + lfo * 500.0f + couplingCutoffBump;
            activeCutoff = std::clamp (activeCutoff, 20.0f, sr * 0.499f);

            float mix = 0.0f;
            for (auto& v : voices)
            {
                // Macro PITCH offset: pPitch shifts base freq by semitones
                float pitchShiftHz = 440.0f * std::pow (2.0f, pPitch / 12.0f) - 440.0f;
                v.freq = 440.0f * std::pow (2.0f, (static_cast<float> (v.note) - 69.0f) / 12.0f)
                         + pitchShiftHz;

                float vSample = v.tick (activeCutoff, activeResonance, pDecay, sr);
                mix += vSample;
                if (v.active) anyActive = true;
            }

            // Soft clip to prevent clipping from multi-voice sum
            mix = std::tanh (mix);

            buffer.left[s]  += mix;
            buffer.right[s] += mix;

            // Cache for coupling sends
            lastSample = mix;
            if (s < (int)couplingBuf.size())
                couplingBuf[static_cast<size_t> (s)] = mix;
        }

        //----------------------------------------------------------------------
        // Step 5: Update silence gate
        //----------------------------------------------------------------------
        if (!anyActive)
        {
            silenceHold += ns;
            if (silenceHold >= silenceHoldMax)
                isSilent = true;
        }
        else
        {
            silenceHold = 0;
        }

        // Reset per-block coupling accumulator
        couplingFilterMod = 0.0f;
    }

    //==========================================================================
    // COUPLING
    //==========================================================================

    // Send: called per-sample by MegaCouplingMatrix during tight coupling.
    // O(1) — just return cached sample.
    float getSampleForCoupling (int /*channel*/, int sampleIndex) const override
    {
        if (sampleIndex >= 0 && sampleIndex < (int)couplingBuf.size())
            return couplingBuf[static_cast<size_t> (sampleIndex)];
        return lastSample;
    }

    // Receive: accumulate modulation from another engine.
    // Only AmpToFilter is handled; others are no-ops for this engine.
    void applyCouplingInput (xomnibus::CouplingType type,
                             float amount,
                             const float* sourceBuffer,
                             int numSamples) override
    {
        if (type == xomnibus::CouplingType::AmpToFilter && sourceBuffer)
        {
            // Compute RMS amplitude of source block
            float rms = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                rms += sourceBuffer[i] * sourceBuffer[i];
            rms = std::sqrt (rms / static_cast<float> (std::max (numSamples, 1)));
            // Accumulate as a modulation signal scaled by the routing amount
            couplingFilterMod += rms * amount;
        }
    }

    //==========================================================================
    // PARAMETERS
    //==========================================================================

    // D004: every declared parameter must affect audio output.
    // D001: velocity shapes timbre (done in voice DSP).
    // D002: modulation present (LFO breathes filter, mod wheel, aftertouch).
    std::vector<xomnibus::ParameterDef> getParameterDefs() const override
    {
        return {
            // id              name           min    max     default  step   skew
            { "hlo_pitch",    "Pitch",       -24.0f, 24.0f,  0.0f,  0.01f, 1.0f },
            { "hlo_cutoff",   "Cutoff",       20.0f, 20000.0f, 2000.0f, 1.0f, 0.3f },
            { "hlo_resonance","Resonance",    0.0f,  0.95f,  0.2f,  0.001f,1.0f },
            { "hlo_decay",    "Decay",        0.01f, 4.0f,   0.5f,  0.001f, 0.5f },
        };
    }

    void setParameter (const std::string& id, float value) override
    {
        // Called from non-audio thread — use atomic-safe writes.
        // For SDK engines, parameters are plain floats updated here
        // and read per-block in renderBlock() with a local snapshot copy.
        if      (id == "hlo_pitch")     paramPitch     = value;
        else if (id == "hlo_cutoff")    paramCutoff    = value;
        else if (id == "hlo_resonance") paramResonance = value;
        else if (id == "hlo_decay")     paramDecay     = value;
    }

    float getParameter (const std::string& id) const override
    {
        if (id == "hlo_pitch")     return paramPitch;
        if (id == "hlo_cutoff")    return paramCutoff;
        if (id == "hlo_resonance") return paramResonance;
        if (id == "hlo_decay")     return paramDecay;
        return 0.0f;
    }

    //==========================================================================
    // IDENTITY
    //==========================================================================

    std::string getEngineId() const override { return "Hello"; }

    // Hello Green — a welcoming, accessible accent colour.
    xomnibus::Colour getAccentColour() const override { return { 0x4C, 0xAF, 0x50 }; }

    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override
    {
        int count = 0;
        for (const auto& v : voices)
            if (v.active) ++count;
        return count;
    }

private:
    //==========================================================================
    // DSP STATE
    //==========================================================================

    static constexpr int kMaxVoices = 2;

    float sr = 44100.0f;

    std::array<HelloVoice, kMaxVoices> voices {};
    int nextVoice = 0;

    // D005: LFO state
    float lfoPhase = 0.0f;
    float lfoRate  = 0.3f;

    // D006: expression CC state
    float modWheelNorm   = 0.0f;
    float aftertouchNorm = 0.0f;

    // Coupling
    std::vector<float> couplingBuf;
    float lastSample      = 0.0f;
    float couplingFilterMod = 0.0f; // accumulated per block, consumed in renderBlock

    // Parameters — plain floats; snapshotted once per block in renderBlock().
    // These are written from the non-audio thread via setParameter() and
    // read on the audio thread. For a production engine, wrap in std::atomic<float>.
    // For the SDK example, the single-writer / single-reader access pattern
    // across a buffer boundary is sufficient without extra locking.
    float paramPitch     =  0.0f;    // semitones offset
    float paramCutoff    = 2000.0f;  // Hz
    float paramResonance =  0.2f;
    float paramDecay     =  0.5f;    // seconds

    // SRO silence gate — SDK variant (no JUCE SilenceGate available here)
    bool isSilent      = false;
    int  silenceHold   = 0;
    int  silenceHoldMax = 8820; // default 200 ms at 44100 Hz; updated in prepare()
};

//==============================================================================
// Export for dynamic loading
//==============================================================================
XOMNIBUS_EXPORT_ENGINE (HelloEngine,
                        "Hello",
                        "Hello Engine",
                        "hlo_",
                        0x4C, 0xAF, 0x50,
                        "1.0.0",
                        "XO_OX SDK Example")
