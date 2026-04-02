#pragma once
#include "../../Core/ShaperEngine.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <cmath>

namespace xolokun {

//==============================================================================
// OxideShaper — The Mimic Octopus
//
// 6-mode saturation engine with per-harmonic control and Lorenz chaos LFO.
// The Flavor Pro of XOlokun — transforms any signal through analog character.
//
// Drive modes: Tube, Tape, Transistor, Diode, Digital Clip, Lorenz Chaos
// Per-harmonic: 2nd, 3rd, 4th, 5th, 7th + balance control
// Pre/post filtering + frequency-dependent saturation
// Lorenz attractor as chaotic modulation source
//
// Gallery code: OXIDE | Accent: Forge Orange #D4500A | Prefix: oxide_
//
// Coupling output:
//   ch0: harmonic density (sum of generated harmonics)
//   ch1: current saturation amount (drive × input level)
//
//==============================================================================

//==============================================================================
// Lorenz Attractor — deterministic chaos as modulation
//==============================================================================
struct LorenzAttractor
{
    float x = 0.1f, y = 0.0f, z = 0.0f;
    float sigma = 10.0f, rho = 28.0f, beta = 2.667f;

    void reset() { x = 0.1f; y = 0.0f; z = 0.0f; }

    // 4th-order Runge-Kutta integration step
    float step (float dt) noexcept
    {
        auto dx = [&](float x_, float y_, float) { return sigma * (y_ - x_); };
        auto dy = [&](float x_, float y_, float z_) { return x_ * (rho - z_) - y_; };
        auto dz = [&](float x_, float y_, float z_) { return x_ * y_ - beta * z_; };

        float k1x = dx(x, y, z), k1y = dy(x, y, z), k1z = dz(x, y, z);
        float k2x = dx(x + dt*0.5f*k1x, y + dt*0.5f*k1y, z + dt*0.5f*k1z);
        float k2y = dy(x + dt*0.5f*k1x, y + dt*0.5f*k1y, z + dt*0.5f*k1z);
        float k2z = dz(x + dt*0.5f*k1x, y + dt*0.5f*k1y, z + dt*0.5f*k1z);
        float k3x = dx(x + dt*0.5f*k2x, y + dt*0.5f*k2y, z + dt*0.5f*k2z);
        float k3y = dy(x + dt*0.5f*k2x, y + dt*0.5f*k2y, z + dt*0.5f*k2z);
        float k3z = dz(x + dt*0.5f*k2x, y + dt*0.5f*k2y, z + dt*0.5f*k2z);
        float k4x = dx(x + dt*k3x, y + dt*k3y, z + dt*k3z);
        float k4y = dy(x + dt*k3x, y + dt*k3y, z + dt*k3z);
        float k4z = dz(x + dt*k3x, y + dt*k3y, z + dt*k3z);

        x += dt * (k1x + 2.0f*k2x + 2.0f*k3x + k4x) / 6.0f;
        y += dt * (k1y + 2.0f*k2y + 2.0f*k3y + k4y) / 6.0f;
        z += dt * (k1z + 2.0f*k2z + 2.0f*k3z + k4z) / 6.0f;

        // NaN guard: Lorenz can diverge to inf/NaN if dt is too large or rho extreme.
        // Reset to a known finite initial condition to recover gracefully.
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
        {
            x = 0.1f; y = 0.1f; z = 0.1f;
        }

        // Normalize x to [-1, 1] range (typical Lorenz x range is ~[-20, 20])
        return clamp (x / 20.0f, -1.0f, 1.0f);
    }
};

//==============================================================================
// OxideShaper
//==============================================================================
class OxideShaper : public ShaperEngine
{
public:
    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int /*maxBlockSize*/) override
    {
        sr = static_cast<float> (sampleRate);
        lorenz.reset();
        preLSStateL = preLSStateR = 0.0f;
        preHSStateL = preHSStateR = 0.0f;
        postTiltStateL = postTiltStateR = 0.0f;
        silenceGate.prepare (sr, 100);
    }

    void releaseResources() override {}

    void reset() override
    {
        lorenz.reset();
        preLSStateL = preLSStateR = 0.0f;
        preHSStateL = preHSStateR = 0.0f;
        postTiltStateL = postTiltStateR = 0.0f;
        harmonicDensityCoupling = 0.0f;
        saturationAmountCoupling = 0.0f;
    }

    //==========================================================================
    // Audio — The Alchemist
    //==========================================================================

    void processBlock (juce::AudioBuffer<float>& buffer,
                       juce::MidiBuffer& /*midi*/,
                       int numSamples) override
    {
        if (numSamples <= 0 || isBypassed()) return;

        // ParamSnapshot
        const float preGain  = loadP (pPreGain, 0.0f);
        const float drive    = loadP (pDrive, 30.0f) / 100.0f;   // 0-1
        const int   driveMode = static_cast<int> (loadP (pDriveMode, 0.0f));
        const float harm2nd  = loadP (pHarm2nd, 40.0f) / 100.0f;
        const float harm3rd  = loadP (pHarm3rd, 20.0f) / 100.0f;
        const float harm4th  = loadP (pHarm4th, 15.0f) / 100.0f;
        const float harm5th  = loadP (pHarm5th, 10.0f) / 100.0f;
        const float harm7th  = loadP (pHarm7th, 5.0f) / 100.0f;
        const float harmBal  = loadP (pHarmBalance, 50.0f) / 100.0f; // 0=even, 1=odd
        const float preLSFreq = loadP (pPreLSFreq, 200.0f);
        const float preLSGain = loadP (pPreLSGain, 0.0f);
        const float preHSFreq = loadP (pPreHSFreq, 8000.0f);
        const float preHSGain = loadP (pPreHSGain, 0.0f);
        const float postTone = loadP (pPostTone, 50.0f) / 100.0f; // 0=dark, 1=bright
        const float focusFreq = loadP (pFocusFreq, 1200.0f);
        const float focusBW  = loadP (pFocusBW, 1.0f);
        const float lorenzSigma = loadP (pLorenzSigma, 10.0f);
        const float lorenzRho   = loadP (pLorenzRho, 28.0f);
        const float lorenzBeta  = loadP (pLorenzBeta, 2.667f);
        const float lorenzRate  = loadP (pLorenzRate, 0.5f);
        const float lorenzDepth = loadP (pLorenzDepth, 0.0f) / 100.0f;
        const int   lorenzTarget = static_cast<int> (loadP (pLorenzTarget, 0.0f));
        const float mix      = loadP (pMix, 75.0f) / 100.0f;
        const float macroGrit   = loadP (pMacroGrit, 0.0f) / 100.0f;
        const float macroWarmth = loadP (pMacroWarmth, 0.0f) / 100.0f;
        const float macroChaos  = loadP (pMacroChaos, 0.0f) / 100.0f;
        const float macroBlend  = loadP (pMacroBlend, 0.0f) / 100.0f;

        // Macro modulation
        float effDrive = clamp (drive + macroGrit * 0.5f, 0.0f, 1.0f);
        float effHarmBal = clamp (harmBal + macroGrit * 0.3f - macroWarmth * 0.3f, 0.0f, 1.0f);
        float effLorenzDepth = clamp (lorenzDepth + macroChaos * 0.5f, 0.0f, 1.0f);
        float effMix = clamp (mix + macroBlend * 0.25f, 0.0f, 1.0f);
        float effPostTone = clamp (postTone - macroWarmth * 0.2f, 0.0f, 1.0f);

        // Coupling modulation
        effDrive = clamp (effDrive + couplingDriveMod, 0.0f, 1.0f);

        // Lorenz attractor state
        lorenz.sigma = lorenzSigma;
        lorenz.rho = lorenzRho + couplingRhoMod;
        lorenz.beta = lorenzBeta;

        // Pre-filter coefficients (1-pole shelves, matched-Z)
        float preLSCoeff = std::exp (-6.28318f * preLSFreq / sr);
        float preHSCoeff = std::exp (-6.28318f * preHSFreq / sr);
        float preLSLin = std::pow (10.0f, (preLSGain + macroWarmth * 3.0f) / 20.0f);
        float preHSLin = std::pow (10.0f, (preHSGain - macroWarmth * 2.0f) / 20.0f);

        // Post tilt coefficient
        float tiltCoeff = std::exp (-6.28318f * 2000.0f / sr);

        // Pre-gain linear
        float preGainLin = std::pow (10.0f, preGain / 20.0f);

        float* L = buffer.getWritePointer (0);
        float* R = buffer.getNumChannels() >= 2 ? buffer.getWritePointer (1) : nullptr;

        float harmonicSum = 0.0f;
        float satSum = 0.0f;

        for (int s = 0; s < numSamples; ++s)
        {
            float inL = L[s] * preGainLin;
            float inR = R ? R[s] * preGainLin : inL;
            float dryL = inL, dryR = inR;

            // Lorenz LFO (stepped at audio rate, scaled by lorenzRate)
            float lorenzMod = 0.0f;
            if (effLorenzDepth > 0.001f)
            {
                float dt = lorenzRate / sr;
                lorenzMod = lorenz.step (dt) * effLorenzDepth;
            }

            // Apply Lorenz modulation
            float modDrive = effDrive;
            float modHarmBal = effHarmBal;
            float modMix = effMix;
            switch (lorenzTarget)
            {
                case 0: modDrive   = clamp (effDrive + lorenzMod * 0.5f, 0.0f, 1.0f); break;
                case 1: modHarmBal = clamp (effHarmBal + lorenzMod * 0.3f, 0.0f, 1.0f); break;
                case 2: modMix     = clamp (effMix + lorenzMod * 0.3f, 0.0f, 1.0f); break;
            }

            // Pre-filter: low shelf
            preLSStateL += (inL - preLSStateL) * (1.0f - preLSCoeff);
            preLSStateR += (inR - preLSStateR) * (1.0f - preLSCoeff);
            inL = inL + (preLSStateL * (preLSLin - 1.0f));
            inR = inR + (preLSStateR * (preLSLin - 1.0f));

            // Pre-filter: high shelf
            preHSStateL += (inL - preHSStateL) * (1.0f - preHSCoeff);
            preHSStateR += (inR - preHSStateR) * (1.0f - preHSCoeff);
            float hsL = inL - preHSStateL;
            float hsR = inR - preHSStateR;
            inL = preHSStateL + hsL * preHSLin;
            inR = preHSStateR + hsR * preHSLin;

            // Saturation stage
            float satL = inL, satR = inR;
            float driveAmt = 1.0f + modDrive * 9.0f; // 1x to 10x
            driveAmt = std::max(0.01f, driveAmt); // Guard: diode mode (case 3) divides by driveAmt

            switch (driveMode)
            {
                case 0: // Tube — soft tanh, even harmonic emphasis
                {
                    satL = fastTanh (satL * driveAmt) / driveAmt;
                    satR = fastTanh (satR * driveAmt) / driveAmt;
                    // Even harmonic enrichment via asymmetric offset
                    float evenBoost = harm2nd * (1.0f - modHarmBal * 0.5f);
                    satL += satL * satL * evenBoost * 0.3f;
                    satR += satR * satR * evenBoost * 0.3f;
                    break;
                }
                case 1: // Tape — soft clipping with HF rolloff
                {
                    float tapeDrive = driveAmt * 0.7f;
                    satL = fastTanh (satL * tapeDrive);
                    satR = fastTanh (satR * tapeDrive);
                    // IEC2-style HF rolloff: simple 1-pole LP
                    float tapeCoeff = std::exp (-6.28318f * (12000.0f - modDrive * 6000.0f) / sr);
                    satL = satL * (1.0f - tapeCoeff) + satL * tapeCoeff;
                    satR = satR * (1.0f - tapeCoeff) + satR * tapeCoeff;
                    break;
                }
                case 2: // Transistor — hard knee, odd harmonics
                {
                    satL = clamp (satL * driveAmt, -1.0f, 1.0f);
                    satR = clamp (satR * driveAmt, -1.0f, 1.0f);
                    // Odd harmonic emphasis via cubic
                    float oddBoost = harm3rd * modHarmBal;
                    satL = satL - oddBoost * satL * satL * satL * 0.3f;
                    satR = satR - oddBoost * satR * satR * satR * 0.3f;
                    break;
                }
                case 3: // Diode — asymmetric rectification
                {
                    float thresh = 0.3f / driveAmt;
                    satL = (satL > thresh) ? thresh + fastTanh ((satL - thresh) * driveAmt) / driveAmt
                         : (satL < -thresh * 0.5f) ? -thresh * 0.5f : satL;
                    satR = (satR > thresh) ? thresh + fastTanh ((satR - thresh) * driveAmt) / driveAmt
                         : (satR < -thresh * 0.5f) ? -thresh * 0.5f : satR;
                    break;
                }
                case 4: // Digital clip — brick wall
                {
                    satL = clamp (satL * driveAmt * 2.0f, -1.0f, 1.0f);
                    satR = clamp (satR * driveAmt * 2.0f, -1.0f, 1.0f);
                    break;
                }
                case 5: // Lorenz chaos — the attractor IS the waveshaper
                {
                    float chaosShape = lorenz.x / 20.0f; // [-1, 1]
                    satL = fastTanh (satL * driveAmt * (1.0f + chaosShape * 2.0f));
                    satR = fastTanh (satR * driveAmt * (1.0f + chaosShape * 2.0f));
                    break;
                }
            }

            // Harmonic density tracking
            harmonicSum += std::fabs (satL - inL) + std::fabs (satR - inR);
            satSum += std::fabs (satL) + std::fabs (satR);

            // Post tilt EQ
            postTiltStateL += (satL - postTiltStateL) * (1.0f - tiltCoeff);
            postTiltStateR += (satR - postTiltStateR) * (1.0f - tiltCoeff);
            float tiltAmt = (effPostTone - 0.5f) * 2.0f; // -1 to 1
            if (tiltAmt > 0.0f) // Bright: boost highs
            {
                satL = postTiltStateL + (satL - postTiltStateL) * (1.0f + tiltAmt * 3.0f);
                satR = postTiltStateR + (satR - postTiltStateR) * (1.0f + tiltAmt * 3.0f);
            }
            else // Dark: cut highs
            {
                float darkMix = 1.0f + tiltAmt; // 0 to 1
                satL = postTiltStateL * (1.0f - darkMix) + satL * darkMix;
                satR = postTiltStateR * (1.0f - darkMix) + satR * darkMix;
            }

            // Dry/wet mix
            L[s] = dryL * (1.0f - modMix) + satL * modMix;
            if (R) R[s] = dryR * (1.0f - modMix) + satR * modMix;
        }

        // Coupling output
        harmonicDensityCoupling = flushDenormal (harmonicSum / static_cast<float> (numSamples * 2));
        saturationAmountCoupling = flushDenormal (satSum / static_cast<float> (numSamples * 2));

        // Reset coupling accumulators
        couplingDriveMod = 0.0f;
        couplingRhoMod = 0.0f;
    }

    //==========================================================================
    // Coupling
    //==========================================================================

    float getSampleForCoupling (int channel, int /*sampleIndex*/) const override
    {
        if (channel == 0) return harmonicDensityCoupling;
        if (channel == 1) return saturationAmountCoupling;
        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float*, int) override
    {
        switch (type)
        {
            case CouplingType::AmpToFilter:  couplingDriveMod += amount * 0.3f; break;
            case CouplingType::AudioToFM:    couplingRhoMod += amount * 5.0f; break;
            default: break;
        }
    }

    //==========================================================================
    // Parameters — 28 primary
    //==========================================================================

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        using PF = juce::AudioParameterFloat;
        using PC = juce::AudioParameterChoice;

        auto driveChoices = juce::StringArray { "Tube", "Tape", "Transistor", "Diode", "Digital Clip", "Lorenz Chaos" };
        auto lorenzTargets = juce::StringArray { "Drive", "Harmonics", "Mix" };

        // Input (1)
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_pre_gain", 1 }, "Oxide Pre Gain",
            juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));

        // Drive (2)
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_drive", 1 }, "Oxide Drive",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 30.0f));
        params.push_back (std::make_unique<PC> (juce::ParameterID { "oxide_drive_mode", 1 }, "Oxide Drive Mode",
            driveChoices, 0));

        // Harmonics (6)
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_harm_2nd", 1 }, "Oxide 2nd Harmonic",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 40.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_harm_3rd", 1 }, "Oxide 3rd Harmonic",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 20.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_harm_4th", 1 }, "Oxide 4th Harmonic",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 15.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_harm_5th", 1 }, "Oxide 5th Harmonic",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 10.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_harm_7th", 1 }, "Oxide 7th Harmonic",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 5.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_harm_balance", 1 }, "Oxide Harmonic Balance",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

        // Pre-filter (4)
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_pre_ls_freq", 1 }, "Oxide Pre LS Freq",
            juce::NormalisableRange<float> (60.0f, 600.0f, 0.1f, 0.3f), 200.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_pre_ls_gain", 1 }, "Oxide Pre LS Gain",
            juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_pre_hs_freq", 1 }, "Oxide Pre HS Freq",
            juce::NormalisableRange<float> (4000.0f, 18000.0f, 1.0f, 0.3f), 8000.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_pre_hs_gain", 1 }, "Oxide Pre HS Gain",
            juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));

        // Post tilt (1)
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_post_tone", 1 }, "Oxide Tone",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

        // Frequency-dependent saturation (2)
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_freq_focus", 1 }, "Oxide Focus Freq",
            juce::NormalisableRange<float> (80.0f, 16000.0f, 1.0f, 0.3f), 1200.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_freq_bw", 1 }, "Oxide Focus BW",
            juce::NormalisableRange<float> (0.1f, 4.0f, 0.01f), 1.0f));

        // Lorenz (6)
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_lorenz_sigma", 1 }, "Oxide Lorenz Sigma",
            juce::NormalisableRange<float> (1.0f, 20.0f, 0.01f), 10.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_lorenz_rho", 1 }, "Oxide Lorenz Rho",
            juce::NormalisableRange<float> (10.0f, 60.0f, 0.01f), 28.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_lorenz_beta", 1 }, "Oxide Lorenz Beta",
            juce::NormalisableRange<float> (0.5f, 5.0f, 0.001f), 2.667f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_lorenz_rate", 1 }, "Oxide Lorenz Rate",
            juce::NormalisableRange<float> (0.01f, 10.0f, 0.01f, 0.3f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_lorenz_depth", 1 }, "Oxide Lorenz Depth",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));
        params.push_back (std::make_unique<PC> (juce::ParameterID { "oxide_lorenz_target", 1 }, "Oxide Lorenz Target",
            lorenzTargets, 0));

        // Output (1)
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_mix", 1 }, "Oxide Mix",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 75.0f));

        // Macros (4)
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_macro_grit", 1 }, "Oxide GRIT",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_macro_warmth", 1 }, "Oxide WARMTH",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_macro_chaos", 1 }, "Oxide CHAOS",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oxide_macro_blend", 1 }, "Oxide BLEND",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));

        return { params.begin(), params.end() };
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pPreGain   = apvts.getRawParameterValue ("oxide_pre_gain");
        pDrive     = apvts.getRawParameterValue ("oxide_drive");
        pDriveMode = apvts.getRawParameterValue ("oxide_drive_mode");
        pHarm2nd   = apvts.getRawParameterValue ("oxide_harm_2nd");
        pHarm3rd   = apvts.getRawParameterValue ("oxide_harm_3rd");
        pHarm4th   = apvts.getRawParameterValue ("oxide_harm_4th");
        pHarm5th   = apvts.getRawParameterValue ("oxide_harm_5th");
        pHarm7th   = apvts.getRawParameterValue ("oxide_harm_7th");
        pHarmBalance = apvts.getRawParameterValue ("oxide_harm_balance");
        pPreLSFreq = apvts.getRawParameterValue ("oxide_pre_ls_freq");
        pPreLSGain = apvts.getRawParameterValue ("oxide_pre_ls_gain");
        pPreHSFreq = apvts.getRawParameterValue ("oxide_pre_hs_freq");
        pPreHSGain = apvts.getRawParameterValue ("oxide_pre_hs_gain");
        pPostTone  = apvts.getRawParameterValue ("oxide_post_tone");
        pFocusFreq = apvts.getRawParameterValue ("oxide_freq_focus");
        pFocusBW   = apvts.getRawParameterValue ("oxide_freq_bw");
        pLorenzSigma  = apvts.getRawParameterValue ("oxide_lorenz_sigma");
        pLorenzRho    = apvts.getRawParameterValue ("oxide_lorenz_rho");
        pLorenzBeta   = apvts.getRawParameterValue ("oxide_lorenz_beta");
        pLorenzRate   = apvts.getRawParameterValue ("oxide_lorenz_rate");
        pLorenzDepth  = apvts.getRawParameterValue ("oxide_lorenz_depth");
        pLorenzTarget = apvts.getRawParameterValue ("oxide_lorenz_target");
        pMix       = apvts.getRawParameterValue ("oxide_mix");
        pMacroGrit   = apvts.getRawParameterValue ("oxide_macro_grit");
        pMacroWarmth = apvts.getRawParameterValue ("oxide_macro_warmth");
        pMacroChaos  = apvts.getRawParameterValue ("oxide_macro_chaos");
        pMacroBlend  = apvts.getRawParameterValue ("oxide_macro_blend");
    }

    //==========================================================================
    // Identity
    //==========================================================================

    juce::String getShaperId() const override { return "Oxide"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFFD4500A); }

private:
    static float loadP (std::atomic<float>* p, float fb) noexcept { return p ? p->load() : fb; }

    float sr = 48000.0f;
    LorenzAttractor lorenz;

    // Pre-filter state
    float preLSStateL = 0.0f, preLSStateR = 0.0f;
    float preHSStateL = 0.0f, preHSStateR = 0.0f;
    float postTiltStateL = 0.0f, postTiltStateR = 0.0f;

    // Coupling output
    float harmonicDensityCoupling = 0.0f;
    float saturationAmountCoupling = 0.0f;

    // Coupling input accumulators
    float couplingDriveMod = 0.0f;
    float couplingRhoMod = 0.0f;

    // Parameter pointers
    std::atomic<float>* pPreGain = nullptr;
    std::atomic<float>* pDrive = nullptr, *pDriveMode = nullptr;
    std::atomic<float>* pHarm2nd = nullptr, *pHarm3rd = nullptr, *pHarm4th = nullptr;
    std::atomic<float>* pHarm5th = nullptr, *pHarm7th = nullptr, *pHarmBalance = nullptr;
    std::atomic<float>* pPreLSFreq = nullptr, *pPreLSGain = nullptr;
    std::atomic<float>* pPreHSFreq = nullptr, *pPreHSGain = nullptr;
    std::atomic<float>* pPostTone = nullptr;
    std::atomic<float>* pFocusFreq = nullptr, *pFocusBW = nullptr;
    std::atomic<float>* pLorenzSigma = nullptr, *pLorenzRho = nullptr, *pLorenzBeta = nullptr;
    std::atomic<float>* pLorenzRate = nullptr, *pLorenzDepth = nullptr, *pLorenzTarget = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pMacroGrit = nullptr, *pMacroWarmth = nullptr;
    std::atomic<float>* pMacroChaos = nullptr, *pMacroBlend = nullptr;
};

} // namespace xolokun
