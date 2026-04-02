#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "OscarAnimState.h"

#if XO_HAS_RIVE

// Rive C++ runtime
#include <rive/file.hpp>
#include <rive/artboard.hpp>
#include <rive/animation/state_machine_instance.hpp>
#include <rive/animation/state_machine_input_instance.hpp>

#include "OscarJuceRenderer.h"

#include <memory>

namespace xoceanus {

//==============================================================================
// OscarRiveComponent — Oscar the axolotl, animated via Rive state machine.
//
// Runs at 60 Hz on the UI thread. Reads OscarAnimState (lock-free atomics
// written by MorphEngine). Advances the Rive state machine and repaints.
//
// Accent: Axolotl Gill Pink  #E8839B
// State machine name in oscar.riv: "OscarBehavior"
//
// State machine inputs:
//   gillSpeed      (number)  — 0.0–1.0, drives gill oscillation rate
//   morphPosition  (number)  — 0.0–3.0, shifts body color/opacity
//   evolutionLevel (number)  — 0–5, blend state for age morphing
//   noteOn         (trigger) — fires on voiceActive rising edge
//   celebration    (trigger) — fires on celebrationTrigger rising edge
//   bossMode       (boolean) — held true during boss phase
//   levelPct       (number)  — 0.0–1.0, output level for body swell
//==============================================================================
class OscarRiveComponent : public juce::Component, private juce::Timer
{
public:

    //--------------------------------------------------------------------------
    // Construction
    //--------------------------------------------------------------------------

    OscarRiveComponent()
    {
        setOpaque (false);  // Oscar floats over the Gallery Model shell
        setTitle ("Oscar");
        setDescription ("Oscar the axolotl — animated companion for OddOscar engine. "
                        "Reacts to synthesis parameters and performance events.");
    }

    /// Load oscar.riv from memory (call once after construction).
    /// Pass the raw .riv file bytes and their length.
    /// Returns true on success.
    bool loadRiv (const void* data, size_t sizeBytes)
    {
        auto result = rive::File::import (
            { static_cast<const uint8_t*> (data), sizeBytes }, &factory);

        if (result.errorCode != rive::ImportResult::success)
            return false;

        riveFile = std::move (result.file);

        artboard = riveFile->artboardDefault();
        if (artboard == nullptr)
            return false;

        auto* sm = artboard->stateMachine ("OscarBehavior");
        if (sm == nullptr)
            return false;

        stateMachine = sm->createInstance (artboard.get());

        // Cache input pointers — avoids string lookup each frame
        inputGillSpeed      = stateMachine->getNumber  ("gillSpeed");
        inputMorphPosition  = stateMachine->getNumber  ("morphPosition");
        inputEvolutionLevel = stateMachine->getNumber  ("evolutionLevel");
        inputLevelPct       = stateMachine->getNumber  ("levelPct");
        inputNoteOn         = stateMachine->getTrigger ("noteOn");
        inputCelebration    = stateMachine->getTrigger ("celebration");
        inputBossMode       = stateMachine->getBool    ("bossMode");

        return true;
    }

    /// Wire to MorphEngine's anim state. Call from the main thread before start().
    void setAnimState (OscarAnimState* state) { animState = state; }

    /// Begin ticking at 60 Hz.
    void start() { startTimerHz (reducedMotion ? 10 : 60); }
    void stop()  { stopTimer(); }

    void visibilityChanged() override
    {
        if (isVisible()) start();
        else stop();
    }

    //--------------------------------------------------------------------------
    // WCAG 2.3.3 — reduced motion support
    //--------------------------------------------------------------------------

    void setReducedMotion (bool enabled)
    {
        reducedMotion = enabled;
        if (isTimerRunning())
        {
            stopTimer();
            startTimerHz (reducedMotion ? 10 : 60);
        }
    }

    //--------------------------------------------------------------------------
    // Rendering
    //--------------------------------------------------------------------------

    void paint (juce::Graphics& g) override
    {
        if (artboard == nullptr)
        {
            paintProcedural (g);
            return;
        }

        auto bounds = getLocalBounds().toFloat();

        // Scale artboard to fit, preserving aspect ratio.
        // Oscar's artboard is designed at 200x200 px in the Rive editor.
        constexpr float kArtboardSize = 200.0f;
        float scale = std::min (bounds.getWidth(), bounds.getHeight()) / kArtboardSize;

        float offsetX = (bounds.getWidth()  - kArtboardSize * scale) * 0.5f;
        float offsetY = (bounds.getHeight() - kArtboardSize * scale) * 0.5f;

        OscarJuceRenderer renderer (g);
        renderer.save();
        renderer.transform (rive::Mat2D { scale, 0.0f, 0.0f, scale, offsetX, offsetY });

        artboard->draw (&renderer);

        renderer.restore();
    }

    void resized() override
    {
        // No child layout — Oscar is one Rive canvas.
    }

    //--------------------------------------------------------------------------
    // Timer — 60 Hz tick
    //--------------------------------------------------------------------------

    void timerCallback() override
    {
        frameCount++;

        // Snapshot state from engine (lock-free)
        if (animState != nullptr)
            lastSnapshot = animState->snapshot();

        if (artboard != nullptr && stateMachine != nullptr)
        {
            pushInputs (lastSnapshot);
            const float deltaSeconds = 1.0f / (reducedMotion ? 10.0f : 60.0f);
            stateMachine->advance (deltaSeconds);
            artboard->advance (deltaSeconds);
        }

        repaint();
    }

private:

    //--------------------------------------------------------------------------
    // Input mapping — translates Oscar's synthesis parameters into Rive
    // animation inputs. See mapGillSpeed() below for the key design notes.
    //--------------------------------------------------------------------------

    void pushInputs (const OscarAnimState::Snapshot& s)
    {
        // --- Gill speed ---
        // morph_drift (0–1) → gill oscillation rate (0–1).
        // This is the soul of the animation. How does drift map to gill energy?
        //
        // mapGillSpeed() blends driftAmount + outputLevel with a resting floor.
        // See implementation below for design rationale.
        if (inputGillSpeed != nullptr)
            inputGillSpeed->value (mapGillSpeed (s.gillSpeed, s.outputLevel));

        // --- Morph position → body tint ---
        // 0.0 = sine (soft pink, gills barely moving)
        // 3.0 = noise (translucent, reef-dissolved)
        if (inputMorphPosition != nullptr)
            inputMorphPosition->value (s.morphPosition);

        // --- Evolution ---
        if (inputEvolutionLevel != nullptr)
            inputEvolutionLevel->value (static_cast<float> (s.evolutionLevel));

        // --- Output level swell ---
        if (inputLevelPct != nullptr)
            inputLevelPct->value (s.outputLevel);

        // --- Note-on trigger (rising edge detection) ---
        if (inputNoteOn != nullptr && s.voiceActive && !wasVoiceActive)
            inputNoteOn->fire();
        wasVoiceActive = s.voiceActive;

        // --- Celebration trigger ---
        if (inputCelebration != nullptr && s.celebration)
        {
            inputCelebration->fire();
            if (animState != nullptr)
                animState->celebrationTrigger.store (false, std::memory_order_relaxed);
        }

        // --- Boss mode ---
        if (inputBossMode != nullptr)
            inputBossMode->value (s.bossMode);
    }

    //--------------------------------------------------------------------------
    // mapGillSpeed — YOUR contribution
    //
    // The gill oscillation rate is the single most expressive visual element.
    // Six bezier curves, oscillating independently — this function decides
    // how fast they move based on what the synthesis is doing.
    //
    // Inputs:
    //   driftAmount  — morph_drift parameter (0.0–1.0)
    //                  0 = imperceptible breathing, 1 = obvious wandering
    //   outputLevel  — instantaneous signal amplitude (0.0–1.0)
    //                  loud chords should make the gills react
    //
    // Returns: Rive input value "gillSpeed" (0.0–1.0)
    //   0.0 = dead still (Oscar isn't breathing, something is wrong)
    //   0.15 = resting idle (the floor — Oscar is always alive)
    //   0.5  = interested / active playing
    //   1.0  = excited (celebration, boss mode, high drift + loud output)
    //
    // Design question: is this linear? Exponential? Does loudness add to drift,
    // or multiply it? Does Oscar have a minimum resting gill rate even at
    // drift=0? (He should — gills stop for dead creatures, not living ones.)
    //--------------------------------------------------------------------------
    float mapGillSpeed (float driftAmount, float outputLevel) const
    {
        constexpr float kFloor = 0.12f;  // Oscar is always alive

        // Exponential curve: pow(x, 0.6) bows upward — small drift still
        // registers visually, but large drift doesn't feel 3x more than medium.
        // Biological systems respond this way: sensitive at rest, saturating
        // at excitation. pow(0.1, 0.6) ≈ 0.25, pow(0.5, 0.6) ≈ 0.66.
        float driftCurved = std::pow (driftAmount, 0.6f);

        // Loud chords stir the water around Oscar — sympathetic flutter.
        // Scaled so amplitude alone can't fully drive the gills; drift leads.
        float levelStir = outputLevel * 0.20f;

        float raw = kFloor + (1.0f - kFloor) * driftCurved + levelStir;
        return juce::jlimit (kFloor, 1.0f, raw);
    }

    //--------------------------------------------------------------------------
    // Procedural Oscar — drawn entirely with JUCE paths.
    // No .riv file required. This IS Oscar, not a fallback.
    // Scales from 24 px (header indicator) to 200 px (full panel).
    //--------------------------------------------------------------------------

    void paintProcedural (juce::Graphics& g)
    {
        auto bounds  = getLocalBounds().toFloat();
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY() + bounds.getHeight() * 0.04f;
        const float R  = std::min (bounds.getWidth(), bounds.getHeight()) * 0.42f;

        if (R < 4.0f) return;

        const float time = static_cast<float> (frameCount) / 60.0f;

        // -- Color palette ---------------------------------------------------
        const float morphT   = lastSnapshot.morphPosition / 3.0f;
        const float bodyAlpha = 1.0f - morphT * 0.20f;
        const float bodySat   = 1.0f - morphT * 0.60f;

        const auto gillPink  = juce::Colour (0xFFE8839B);
        const auto bellyPink = juce::Colour (0xFFF2B3C1);
        const auto deepPink  = juce::Colour (0xFFC45E7A);
        const auto gillTipHi = juce::Colour (0xFFFFD6E0);
        const auto eyeDark   = juce::Colour (0xFF3D2B3D);
        const auto pupilBlk  = juce::Colour (0xFF1A1A2E);

        const auto bodyColor = gillPink
            .withMultipliedSaturation (bodySat)
            .withAlpha (bodyAlpha);

        // Body swell from output amplitude (max 4% expansion)
        const float swell = 1.0f + lastSnapshot.outputLevel * 0.04f;

        // -- Gills (drawn first — behind body) --------------------------------
        // 6 stalks: 3 left, 3 right. Each oscillates independently.
        struct GillDef { float phase; float rate; float side; int rank; };
        static constexpr GillDef kGills[6] = {
            { 0.0f, 1.00f, -1.0f, 0 },
            { 0.4f, 0.87f, -1.0f, 1 },
            { 1.1f, 1.13f, -1.0f, 2 },
            { 0.2f, 0.95f, +1.0f, 0 },
            { 0.8f, 1.07f, +1.0f, 1 },
            { 1.5f, 0.91f, +1.0f, 2 },
        };

        const float gillLen    = R * 0.60f;
        const float gillHz     = lastSnapshot.gillSpeed * 1.8f; // full oscillations per second
        const float swingRad   = juce::degreesToRadians (12.0f + lastSnapshot.gillSpeed * 16.0f);
        const float gillThick  = std::max (1.5f, R * 0.075f);

        for (auto& gd : kGills)
        {
            // Three stalks per side, spread slightly apart
            const float spread = (gd.rank - 1.0f) * R * 0.16f;
            const float baseX  = cx + gd.side * R * 0.50f + spread * gd.side;
            const float baseY  = cy - R * 0.32f;

            // Oscillation: sine with per-gill phase and rate offset
            const float swing = std::sin (time * gillHz * juce::MathConstants<float>::twoPi
                                          * gd.rate + gd.phase) * swingRad;

            // Base direction: angled upward and outward, then swing applied
            const float lean   = gd.side * 0.45f;              // lean outward ~26°
            const float angle  = -juce::MathConstants<float>::halfPi + lean + swing;

            const float tipX = baseX + std::cos (angle) * gillLen;
            const float tipY = baseY + std::sin (angle) * gillLen;

            // Quadratic bezier gives stalk a natural S-curve
            const float ctrlAngle = -juce::MathConstants<float>::halfPi + lean * 0.5f + swing * 0.3f;
            const float ctrlX = baseX + std::cos (ctrlAngle) * gillLen * 0.55f;
            const float ctrlY = baseY + std::sin (ctrlAngle) * gillLen * 0.55f;

            juce::Path stalk;
            stalk.startNewSubPath (baseX, baseY);
            stalk.quadraticTo (ctrlX, ctrlY, tipX, tipY);

            g.setGradientFill (juce::ColourGradient (
                bodyColor, baseX, baseY,
                gillTipHi.withAlpha (bodyAlpha * 0.8f), tipX, tipY, false));
            g.strokePath (stalk, juce::PathStrokeType (
                gillThick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Feathery branches at tip — only when large enough to see
            if (R > 14.0f)
            {
                const float bLen = gillLen * 0.28f;
                juce::Path branches;
                branches.startNewSubPath (tipX, tipY);
                branches.lineTo (tipX + std::cos (angle + 0.45f) * bLen,
                                 tipY + std::sin (angle + 0.45f) * bLen);
                branches.startNewSubPath (tipX, tipY);
                branches.lineTo (tipX + std::cos (angle - 0.45f) * bLen,
                                 tipY + std::sin (angle - 0.45f) * bLen);

                g.setColour (gillTipHi.withAlpha (bodyAlpha * 0.65f));
                g.strokePath (branches, juce::PathStrokeType (
                    gillThick * 0.45f, juce::PathStrokeType::curved,
                    juce::PathStrokeType::rounded));
            }
        }

        // -- Body -------------------------------------------------------------
        const float bw = R * 1.55f * swell;
        const float bh = R * 1.15f * swell;
        g.setColour (bodyColor);
        g.fillEllipse (cx - bw * 0.5f, cy - bh * 0.5f, bw, bh);

        // Outline
        g.setColour (deepPink.withAlpha (bodyAlpha * 0.35f));
        g.drawEllipse (cx - bw * 0.5f, cy - bh * 0.5f, bw, bh, 1.2f);

        // -- Belly highlight --------------------------------------------------
        const float bellyw = R * 0.85f * swell;
        const float bellyh = R * 0.60f * swell;
        g.setColour (bellyPink.withAlpha (bodyAlpha * 0.55f));
        g.fillEllipse (cx - bellyw * 0.5f,
                       cy - bellyh * 0.5f + R * 0.18f,
                       bellyw, bellyh);

        // -- Legs (only at ≥ 32 px effective radius) --------------------------
        if (R >= 32.0f)
        {
            const float legW = R * 0.22f;
            const float legH = R * 0.32f;
            const float legY = cy + R * 0.52f;

            g.setColour (bodyColor.darker (0.08f));
            g.fillRoundedRectangle (cx - R * 0.52f - legW * 0.5f, legY, legW, legH, legW * 0.4f);
            g.fillRoundedRectangle (cx + R * 0.52f - legW * 0.5f, legY, legW, legH, legW * 0.4f);
        }

        // -- Eyes -------------------------------------------------------------
        if (R >= 8.0f)
        {
            const float eyeR = R * 0.175f;
            const float eyeY = cy - R * 0.08f;
            const float eLx  = cx - R * 0.26f;
            const float eRx  = cx + R * 0.26f;

            // Iris
            g.setColour (eyeDark);
            g.fillEllipse (eLx - eyeR, eyeY - eyeR, eyeR * 2.0f, eyeR * 2.0f);
            g.fillEllipse (eRx - eyeR, eyeY - eyeR, eyeR * 2.0f, eyeR * 2.0f);

            // Pupils — shift slightly at high morph (Oscar notices the timbral change)
            const float pupilR  = eyeR * 0.55f;
            const float pupilOx = morphT * eyeR * 0.25f;

            g.setColour (pupilBlk);
            g.fillEllipse (eLx - pupilR + pupilOx, eyeY - pupilR, pupilR * 2.0f, pupilR * 2.0f);
            g.fillEllipse (eRx - pupilR + pupilOx, eyeY - pupilR, pupilR * 2.0f, pupilR * 2.0f);

            // Specular dot (makes eyes feel wet and alive)
            if (R >= 14.0f)
            {
                const float specR = pupilR * 0.3f;
                g.setColour (juce::Colours::white.withAlpha (0.7f));
                g.fillEllipse (eLx - pupilR * 0.3f, eyeY - pupilR * 0.3f, specR * 2.0f, specR * 2.0f);
                g.fillEllipse (eRx - pupilR * 0.3f, eyeY - pupilR * 0.3f, specR * 2.0f, specR * 2.0f);
            }
        }
    }

    //--------------------------------------------------------------------------
    // Rive runtime objects
    //--------------------------------------------------------------------------

    // Minimal Rive factory for JUCE — constructs RenderPath/RenderPaint via
    // OscarJuceRenderer's factory methods. The factory itself is stateless.
    struct JuceRiveFactory : public rive::Factory
    {
        rive::rcp<rive::RenderBuffer> makeRenderBuffer (
            rive::RenderBufferType, rive::RenderBufferFlags, size_t) override
        { return nullptr; }

        rive::rcp<rive::RenderShader> makeLinearGradient (
            float, float, float, float,
            const rive::ColorInt*, const float*, int) override
        { return nullptr; }

        rive::rcp<rive::RenderShader> makeRadialGradient (
            float, float, float,
            const rive::ColorInt*, const float*, int) override
        { return nullptr; }

        rive::rcp<rive::RenderPath> makeRenderPath (
            rive::RawPath& rawPath, rive::FillRule fill) override
        {
            auto p = rive::make_rcp<OscarJucePath>();
            // Convert RawPath to OscarJucePath
            for (auto [verb, pts] : rawPath)
            {
                switch (verb)
                {
                    case rive::PathVerb::move:  p->moveTo (pts[0].x, pts[0].y); break;
                    case rive::PathVerb::line:  p->lineTo (pts[0].x, pts[0].y); break;
                    case rive::PathVerb::cubic:
                        p->cubicTo (pts[0].x, pts[0].y, pts[1].x, pts[1].y, pts[2].x, pts[2].y);
                        break;
                    case rive::PathVerb::close: p->close(); break;
                    default: break;
                }
            }
            if (fill == rive::FillRule::evenOdd)
                p->fillRule (rive::FillRule::evenOdd);
            return p;
        }

        rive::rcp<rive::RenderPath> makeEmptyRenderPath() override
        { return rive::make_rcp<OscarJucePath>(); }

        rive::rcp<rive::RenderPaint> makeRenderPaint() override
        { return rive::make_rcp<OscarJucePaint>(); }

        rive::rcp<rive::RenderImage> decodeImage (rive::Span<const uint8_t>) override
        { return nullptr; }
    };

    JuceRiveFactory factory;

    std::unique_ptr<rive::File>                 riveFile;
    std::unique_ptr<rive::ArtboardInstance>     artboard;
    std::unique_ptr<rive::StateMachineInstance> stateMachine;

    // Cached state machine input pointers (no string lookup per frame)
    rive::SMINumber*  inputGillSpeed      = nullptr;
    rive::SMINumber*  inputMorphPosition  = nullptr;
    rive::SMINumber*  inputEvolutionLevel = nullptr;
    rive::SMINumber*  inputLevelPct       = nullptr;
    rive::SMITrigger* inputNoteOn         = nullptr;
    rive::SMITrigger* inputCelebration    = nullptr;
    rive::SMIBool*    inputBossMode       = nullptr;

    //--------------------------------------------------------------------------
    // State
    //--------------------------------------------------------------------------

    OscarAnimState*              animState      = nullptr;
    OscarAnimState::Snapshot     lastSnapshot;       // last read from animState
    bool                         reducedMotion  = false;
    bool                         wasVoiceActive = false;
    uint64_t                     frameCount     = 0;
};

} // namespace xoceanus

#else // !XO_HAS_RIVE — stub component (no animation, no Rive dependency)

namespace xoceanus {

class OscarRiveComponent : public juce::Component
{
public:
    OscarRiveComponent() { setOpaque (false); }
    bool loadRiv (const void*, size_t) { return false; }
    void setAnimState (OscarAnimState*) {}
    void start() {}
    void stop() {}
};

} // namespace xoceanus

#endif // XO_HAS_RIVE
