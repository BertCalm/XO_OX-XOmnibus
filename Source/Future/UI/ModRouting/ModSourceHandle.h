// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
// Future feature — not yet wired into the UI. See GitHub issue #670.
// ModSourceHandle is the draggable mod-source circle used by DragDropModRouter.
// To activate: parent component must create ModSourceHandle instances per mod source.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
// GalleryColors.h lives at Source/UI/GalleryColors.h; use the Source/ root include
// path so this header is includable from any translation unit regardless of depth.
#include "UI/GalleryColors.h"

namespace xoceanus
{

//==============================================================================
// ModSourceId — canonical IDs for all drag-source modulation types.
//
// These map to the standard "per-voice" modulation sources available in every
// engine through the APVTS / expression pipeline.  DSP engines read the route
// table produced by ModRoutingModel and apply depth at render time.
//
enum class ModSourceId
{
    LFO1 = 0,       // Engine LFO 1 (sine by default)
    LFO2 = 1,       // Engine LFO 2 (triangle / free-run)
    Envelope = 2,   // Amplitude envelope follower output (ENV 1 / Amp)
    Velocity = 3,   // Note velocity (0–1, set at note-on, held)
    Aftertouch = 4, // Mono/poly aftertouch (0–1, continuous)
    ModWheel = 5,   // MIDI CC 1 mod wheel (0–1, continuous)
    // ── Extended sources added Wave5-A3 to match D9 F4 + G3 spec ──────────
    LFO3 = 6,           // Engine LFO 3 (free-running, bipolar)
    Envelope2 = 7,      // ENV 2 (auxiliary envelope, bipolar)
    MacroTone = 8,      // Macro knob: TONE (unipolar)
    MacroTide = 9,      // Macro knob: TIDE (unipolar)
    MacroCouple = 10,   // Macro knob: COUPLE (unipolar)
    MacroDepth = 11,    // Macro knob: DEPTH (unipolar)
    MidiCC = 12,        // Assignable MIDI CC (unipolar)
    MpePressure = 13,   // MPE per-note pressure (unipolar)
    MpeSlide = 14,      // MPE per-note slide / Y-axis (unipolar)
    SeqStepValue = 15,  // Sequencer step value output (bipolar)
    // Renamed from ChordToneIdx — actual chord tone index is pending C5 phase.
    // Currently returns getLiveGate() (0 or 1, unipolar) from the slot sequencer.
    // The integer ID (16) is stable and must not change (preset serialisation).
    LiveGate = 16,      // Sequencer gate state (0 or 1, unipolar)
    BeatPhase = 17,     // Beat phase ramp 0→1 per bar (bipolar)
    // ── Wave5-D3: XOuija pin source ─────────────────────────────────────────
    // A pinned XOuija position exposes two bipolar values:
    //   X-axis (circle-of-fifths)  → normalized [-1, +1] mapped from [0, 1]
    //   Y-axis (influence depth)   → normalized [-1, +1] mapped from [0, 1]
    // The ModRoutingModel routes these as a single source; the DSP engine reads
    // the X or Y component depending on the parameter's semantic (see D9 wiring).
    // Capture slots 0-3 are differentiated by the ModRoute's destParamId suffix.
    XouijaCell = 18,    // Pinned XOuija position (bipolar X+Y, 4 capture slots)
    // ── #1289: per-step pitch offset ModSource ───────────────────────────────
    // Bipolar -1..+1 mapped from ±12 semitones (0.0 on silent / rest steps).
    // Deferred from C5; depends on C3 per-step pitch data in PerEnginePatternSequencer.
    SeqStepPitch = 19,  // Per-step pitch offset bipolar -1..+1 (from ±12 semitones)
    // ── #1357: XY Surface position sources (W8B mount) ──────────────────────
    // XY surface X-axis value for each engine slot, bipolar [-1, +1] (centred on 0.5).
    // Read from XOceanusProcessor::xyX_[slot] atomics updated by XYSurface::onXYChanged.
    // Slot is determined by the ModRoute's destParamId suffix convention.
    XYX0 = 20,   // XY surface X-axis, slot 0 (bipolar)
    XYX1 = 21,   // XY surface X-axis, slot 1 (bipolar)
    XYX2 = 22,   // XY surface X-axis, slot 2 (bipolar)
    XYX3 = 23,   // XY surface X-axis, slot 3 (bipolar)
    // XY surface Y-axis value for each engine slot, bipolar [-1, +1].
    XYY0 = 24,   // XY surface Y-axis, slot 0 (bipolar)
    XYY1 = 25,   // XY surface Y-axis, slot 1 (bipolar)
    XYY2 = 26,   // XY surface Y-axis, slot 2 (bipolar)
    XYY3 = 27,   // XY surface Y-axis, slot 3 (bipolar)
    Count = 28
};

// Human-readable names used in tooltips and the route list panel.
inline juce::String modSourceName(ModSourceId id)
{
    switch (id)
    {
    case ModSourceId::LFO1:
        return "LFO 1";
    case ModSourceId::LFO2:
        return "LFO 2";
    case ModSourceId::Envelope:
        return "ENV 1 (Amp)";
    case ModSourceId::Velocity:
        return "Velocity";
    case ModSourceId::Aftertouch:
        return "Aftertouch";
    case ModSourceId::ModWheel:
        return "Mod Wheel";
    case ModSourceId::LFO3:
        return "LFO 3";
    case ModSourceId::Envelope2:
        return "ENV 2";
    case ModSourceId::MacroTone:
        return "Macro: TONE";
    case ModSourceId::MacroTide:
        return "Macro: TIDE";
    case ModSourceId::MacroCouple:
        return "Macro: COUPLE";
    case ModSourceId::MacroDepth:
        return "Macro: DEPTH";
    case ModSourceId::MidiCC:
        return "MIDI CC";
    case ModSourceId::MpePressure:
        return "MPE Pressure";
    case ModSourceId::MpeSlide:
        return "MPE Slide";
    case ModSourceId::SeqStepValue:
        return "Seq Step Value";
    case ModSourceId::LiveGate:
        return "Live Gate";
    case ModSourceId::BeatPhase:
        return "Beat Phase";
    case ModSourceId::XouijaCell:
        return "XOuija Pin";
    case ModSourceId::SeqStepPitch:
        return "Seq Step Pitch";
    // ── #1357: XY Surface sources ───────────────────────────────────────────
    case ModSourceId::XYX0: return "XY X (Slot 1)";
    case ModSourceId::XYX1: return "XY X (Slot 2)";
    case ModSourceId::XYX2: return "XY X (Slot 3)";
    case ModSourceId::XYX3: return "XY X (Slot 4)";
    case ModSourceId::XYY0: return "XY Y (Slot 1)";
    case ModSourceId::XYY1: return "XY Y (Slot 2)";
    case ModSourceId::XYY2: return "XY Y (Slot 3)";
    case ModSourceId::XYY3: return "XY Y (Slot 4)";
    default:
        return "?";
    }
}

// Per-source accent colors.
// - LFO1:       Bioluminescent Cyan  — continuous, oscillating
// - LFO2:       Spectral Ice (lighter cyan) — second oscillator
// - Envelope:   Firelight Orange     — dynamic, transient-following
// - Velocity:   Emergence Lime       — performance gesture
// - Aftertouch: Aria Gold / Pink     — expressive pressure
// - ModWheel:   Royal Blue           — performance controller
inline juce::Colour modSourceColour(ModSourceId id)
{
    switch (id)
    {
    case ModSourceId::LFO1:
        return juce::Colour(0xFF00CED1); // cyan
    case ModSourceId::LFO2:
        return juce::Colour(0xFFA8D8EA); // spectral ice
    case ModSourceId::Envelope:
        return juce::Colour(0xFFE8701A); // orange
    case ModSourceId::Velocity:
        return juce::Colour(0xFFC6E377); // lime green
    case ModSourceId::Aftertouch:
        return juce::Colour(0xFFFF8A7A); // soft coral/pink
    case ModSourceId::ModWheel:
        return juce::Colour(0xFF4169E1); // royal blue
    case ModSourceId::LFO3:
        return juce::Colour(0xFF7EC8E3); // lighter cyan (third LFO)
    case ModSourceId::Envelope2:
        return juce::Colour(0xFFFFAA55); // warm amber (second envelope)
    case ModSourceId::MacroTone:
        return juce::Colour(0xFFE9C46A); // sandy gold
    case ModSourceId::MacroTide:
        return juce::Colour(0xFF7FDBCA); // tide teal
    case ModSourceId::MacroCouple:
        return juce::Colour(0xFFFF8A65); // coral
    case ModSourceId::MacroDepth:
        return juce::Colour(0xFF9B89D4); // violet
    case ModSourceId::MidiCC:
        return juce::Colour(0xFF9898D0); // periwinkle
    case ModSourceId::MpePressure:
        return juce::Colour(0xFFFFD54F); // amber gold
    case ModSourceId::MpeSlide:
        return juce::Colour(0xFFFF7043); // deep orange
    case ModSourceId::SeqStepValue:
        return juce::Colour(0xFF81D4FA); // light sky blue
    case ModSourceId::LiveGate:
        return juce::Colour(0xFFF48FB1); // pink
    case ModSourceId::BeatPhase:
        return juce::Colour(0xFF80CBC4); // muted teal
    case ModSourceId::XouijaCell:
        return juce::Colour(0xFFE9C46A); // xo-gold — matches planchette accent
    case ModSourceId::SeqStepPitch:
        return juce::Colour(0xFF56CFB2); // seafoam — matches REEFS family (pitch-oriented)
    // ── #1357: XY Surface sources — ocean teal/blue gradient ───────────────
    case ModSourceId::XYX0:
    case ModSourceId::XYX1:
    case ModSourceId::XYX2:
    case ModSourceId::XYX3:
        return juce::Colour(0xFF3CB4AA); // xoceanus teal — X axis
    case ModSourceId::XYY0:
    case ModSourceId::XYY1:
    case ModSourceId::XYY2:
    case ModSourceId::XYY3:
        return juce::Colour(0xFF2A7FA5); // deep ocean blue — Y axis
    default:
        return juce::Colour(GalleryColors::xoGold);
    }
}

//==============================================================================
// DragPayload — serialised into the juce::var passed to startDragging().
//
// The ModSourceHandle encodes the source ID as an int var so that any
// DragAndDropTarget in the hierarchy can decode it without a header dependency.
// Use DragPayload::encode / decode as the single serialisation point.
//
struct DragPayload
{
    static constexpr int kMagic = 0x584F4D53; // 'XOMS' — XO Mod Source

    ModSourceId sourceId{ModSourceId::LFO1};

    juce::var encode() const
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("magic", kMagic);
        obj->setProperty("sourceId", static_cast<int>(sourceId));
        return juce::var(obj);
    }

    static bool isModSourceDrag(const juce::var& v)
    {
        if (auto* obj = v.getDynamicObject())
            return static_cast<int>(obj->getProperty("magic")) == kMagic;
        return false;
    }

    static DragPayload decode(const juce::var& v)
    {
        DragPayload p;
        if (auto* obj = v.getDynamicObject())
            p.sourceId = static_cast<ModSourceId>(static_cast<int>(obj->getProperty("sourceId")));
        return p;
    }
};

//==============================================================================
// ModSourceHandle — 16x16 draggable circle representing a single mod source.
//
// Visual design:
//   • Filled circle in source color at full opacity when idle
//   • 1px white inner rim (specular highlight, 9% alpha)
//   • 1.5px outer ring in source color at 40% alpha — "readiness halo"
//   • On hover: halo expands (outer ring 60% alpha, 2px)
//   • While dragging: fill fades to 60% — shows the source is "consumed"
//   • Focus ring follows A11y token (keyboard navigable)
//
// Drag behavior:
//   • mouseDown captures the drag via DragAndDropContainer::startDragging()
//   • The DragPayload is encoded as a juce::var — zero overhead on drop targets
//     that don't use it
//   • The parent DragAndDropContainer (editor root) must be present in the
//     component hierarchy for startDragging() to work
//
// Size: always 16x16 logical pixels.  The component should be embedded in a
// parent that can accommodate an 8-px clearance zone for the hover halo.
//
class ModSourceHandle : public juce::Component
{
public:
    static constexpr int kDiameter = 16;

    explicit ModSourceHandle(ModSourceId id) : sourceId(id), sourceColour(modSourceColour(id))
    {
        setSize(kDiameter, kDiameter);
        setRepaintsOnMouseActivity(true);

        // Accessibility
        A11y::setup(*this, modSourceName(id), "Drag to any parameter knob to create a modulation route",
                    /* wantsKeyFocus = */ true);
    }

    ModSourceId getSourceId() const noexcept { return sourceId; }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        const float cx = w * 0.5f;
        const float cy = h * 0.5f;
        const float r = static_cast<float>(kDiameter) * 0.5f - 1.0f;

        const bool over = isMouseOver();
        const bool dragging = isDragging;

        // ── Outer halo ring — readiness indicator ──────────────────────────
        {
            float haloAlpha = over ? 0.60f : 0.30f;
            float haloW = over ? 2.0f : 1.5f;
            float haloR = r + 2.5f;
            g.setColour(sourceColour.withAlpha(haloAlpha));
            g.drawEllipse(cx - haloR, cy - haloR, haloR * 2.0f, haloR * 2.0f, haloW);
        }

        // ── Filled circle body ────────────────────────────────────────────
        float fillAlpha = dragging ? 0.55f : 1.0f;
        g.setColour(sourceColour.withAlpha(fillAlpha));
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);

        // ── Specular inner rim (top-left) ─────────────────────────────────
        g.setColour(juce::Colour(0xFFFFFFFF).withAlpha(0.09f));
        g.drawEllipse(cx - r + 0.5f, cy - r + 0.5f, r * 2.0f - 1.0f, r * 2.0f - 1.0f, 0.8f);

        // ── Source letter / icon ──────────────────────────────────────────
        // Single-character mnemonic rendered at the center of the handle.
        // Uses JetBrains Mono for legibility at small sizes.
        juce::String glyph;
        switch (sourceId)
        {
        case ModSourceId::LFO1:
            glyph = "1";
            break;
        case ModSourceId::LFO2:
            glyph = "2";
            break;
        case ModSourceId::Envelope:
            glyph = "E";
            break;
        case ModSourceId::Velocity:
            glyph = "V";
            break;
        case ModSourceId::Aftertouch:
            glyph = "A";
            break;
        case ModSourceId::ModWheel:
            glyph = "M";
            break;
        case ModSourceId::LFO3:
            glyph = "3";
            break;
        case ModSourceId::Envelope2:
            glyph = "F";  // "F" = second envelope (E already taken by ENV1)
            break;
        case ModSourceId::MacroTone:
            glyph = "T";
            break;
        case ModSourceId::MacroTide:
            glyph = "~";
            break;
        case ModSourceId::MacroCouple:
            glyph = "C";
            break;
        case ModSourceId::MacroDepth:
            glyph = "D";
            break;
        case ModSourceId::MidiCC:
            glyph = "C";
            break;
        case ModSourceId::MpePressure:
            glyph = "P";
            break;
        case ModSourceId::MpeSlide:
            glyph = "S";
            break;
        case ModSourceId::SeqStepValue:
            glyph = "Q";
            break;
        case ModSourceId::LiveGate:
            glyph = "G";
            break;
        case ModSourceId::BeatPhase:
            glyph = "B";
            break;
        case ModSourceId::XouijaCell:
            glyph = "Y"; // "Y" for ouiJa — avoids ambiguity with Q=seq, J=none
            break;
        default:
            glyph = "?";
            break;
        }

        g.setFont(GalleryFonts::value(6.5f));

        // Choose a contrasting glyph colour: bright colours use dark text,
        // dark colours use light text — checked via perceived luminance.
        const float luma = 0.299f * sourceColour.getFloatRed() + 0.587f * sourceColour.getFloatGreen() +
                           0.114f * sourceColour.getFloatBlue();
        g.setColour(luma > 0.55f ? juce::Colour(0xFF1A1A1A).withAlpha(0.85f)
                                 : juce::Colour(0xFFFFFFFF).withAlpha(0.85f));

        g.drawText(glyph, juce::Rectangle<float>(cx - r, cy - r, r * 2.0f, r * 2.0f), juce::Justification::centred,
                   /* useEllipsisIfTooBig = */ false);

        // ── Keyboard focus ring ────────────────────────────────────────────
        if (hasKeyboardFocus(true))
            A11y::drawCircularFocusRing(g, cx, cy, r + 2.5f);
    }

    //==========================================================================
    // Mouse — drag initiation
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isLeftButtonDown())
        {
            isDragging = true;
            repaint();

            if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
            {
                DragPayload payload;
                payload.sourceId = sourceId;

                // ScaledImage drag image: a slightly larger copy of the handle circle
                juce::Image img(juce::Image::ARGB, 20, 20, true);
                {
                    juce::Graphics ig(img);
                    const float icx = 10.0f, icy = 10.0f, ir = 7.0f;
                    ig.setColour(sourceColour);
                    ig.fillEllipse(icx - ir, icy - ir, ir * 2.0f, ir * 2.0f);
                    ig.setColour(juce::Colour(0xFFFFFFFF).withAlpha(0.09f));
                    ig.drawEllipse(icx - ir + 0.5f, icy - ir + 0.5f, ir * 2.0f - 1.0f, ir * 2.0f - 1.0f, 0.8f);
                }

                container->startDragging(payload.encode(), this, juce::ScaledImage(img),
                                         /* allowDraggingToOtherWindows = */ true);
            }
        }
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        isDragging = false;
        repaint();
    }

    //==========================================================================
    // Keyboard — space / enter to trigger a "click" interaction (accessibility)
    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::spaceKey || key.getKeyCode() == juce::KeyPress::returnKey)
        {
            // Accessibility action: announce the source name to screen readers.
            // A full keyboard-driven route dialog would be a Phase 2 feature.
            setDescription("Press Space to begin keyboard drag from " + modSourceName(sourceId));
            return true;
        }
        return false;
    }

private:
    ModSourceId sourceId;
    juce::Colour sourceColour;
    bool isDragging{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModSourceHandle)
};

} // namespace xoceanus
