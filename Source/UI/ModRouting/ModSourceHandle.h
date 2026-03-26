#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"

namespace xolokun {

//==============================================================================
// ModSourceId — canonical IDs for all drag-source modulation types.
//
// These map to the standard "per-voice" modulation sources available in every
// engine through the APVTS / expression pipeline.  DSP engines read the route
// table produced by ModRoutingModel and apply depth at render time.
//
enum class ModSourceId
{
    LFO1       = 0,   // Engine LFO 1 (sine by default)
    LFO2       = 1,   // Engine LFO 2 (triangle / free-run)
    Envelope   = 2,   // Amplitude envelope follower output
    Velocity   = 3,   // Note velocity (0–1, set at note-on, held)
    Aftertouch = 4,   // Mono/poly aftertouch (0–1, continuous)
    ModWheel   = 5,   // MIDI CC 1 mod wheel (0–1, continuous)
    Count      = 6
};

// Human-readable names used in tooltips and the route list panel.
inline juce::String modSourceName(ModSourceId id)
{
    switch (id)
    {
        case ModSourceId::LFO1:       return "LFO 1";
        case ModSourceId::LFO2:       return "LFO 2";
        case ModSourceId::Envelope:   return "Envelope";
        case ModSourceId::Velocity:   return "Velocity";
        case ModSourceId::Aftertouch: return "Aftertouch";
        case ModSourceId::ModWheel:   return "Mod Wheel";
        default:                       return "?";
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
        case ModSourceId::LFO1:       return juce::Colour(0xFF00CED1);   // cyan
        case ModSourceId::LFO2:       return juce::Colour(0xFFA8D8EA);   // spectral ice
        case ModSourceId::Envelope:   return juce::Colour(0xFFE8701A);   // orange
        case ModSourceId::Velocity:   return juce::Colour(0xFFC6E377);   // lime green
        case ModSourceId::Aftertouch: return juce::Colour(0xFFFF8A7A);   // soft coral/pink
        case ModSourceId::ModWheel:   return juce::Colour(0xFF4169E1);   // royal blue
        default:                       return juce::Colour(GalleryColors::xoGold);
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

    ModSourceId sourceId { ModSourceId::LFO1 };

    juce::var encode() const
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("magic",    kMagic);
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

    explicit ModSourceHandle(ModSourceId id)
        : sourceId(id)
        , sourceColour(modSourceColour(id))
    {
        setSize(kDiameter, kDiameter);
        setRepaintsOnMouseActivity(true);

        // Accessibility
        A11y::setup(*this,
                    modSourceName(id),
                    "Drag to any parameter knob to create a modulation route",
                    /* wantsKeyFocus = */ true);
    }

    ModSourceId getSourceId() const noexcept { return sourceId; }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        const float w  = static_cast<float>(getWidth());
        const float h  = static_cast<float>(getHeight());
        const float cx = w * 0.5f;
        const float cy = h * 0.5f;
        const float r  = static_cast<float>(kDiameter) * 0.5f - 1.0f;

        const bool over    = isMouseOver();
        const bool dragging = isDragging;

        // ── Outer halo ring — readiness indicator ──────────────────────────
        {
            float haloAlpha = over ? 0.60f : 0.30f;
            float haloW     = over ? 2.0f  : 1.5f;
            float haloR     = r + 2.5f;
            g.setColour(sourceColour.withAlpha(haloAlpha));
            g.drawEllipse(cx - haloR, cy - haloR, haloR * 2.0f, haloR * 2.0f, haloW);
        }

        // ── Filled circle body ────────────────────────────────────────────
        float fillAlpha = dragging ? 0.55f : 1.0f;
        g.setColour(sourceColour.withAlpha(fillAlpha));
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);

        // ── Specular inner rim (top-left) ─────────────────────────────────
        g.setColour(juce::Colour(0xFFFFFFFF).withAlpha(0.09f));
        g.drawEllipse(cx - r + 0.5f, cy - r + 0.5f,
                      r * 2.0f - 1.0f, r * 2.0f - 1.0f, 0.8f);

        // ── Source letter / icon ──────────────────────────────────────────
        // Single-character mnemonic rendered at the center of the handle.
        // Uses JetBrains Mono for legibility at small sizes.
        juce::String glyph;
        switch (sourceId)
        {
            case ModSourceId::LFO1:       glyph = "1"; break;
            case ModSourceId::LFO2:       glyph = "2"; break;
            case ModSourceId::Envelope:   glyph = "E"; break;
            case ModSourceId::Velocity:   glyph = "V"; break;
            case ModSourceId::Aftertouch: glyph = "A"; break;
            case ModSourceId::ModWheel:   glyph = "M"; break;
            default:                       glyph = "?"; break;
        }

        g.setFont(GalleryFonts::value(6.5f));

        // Choose a contrasting glyph colour: bright colours use dark text,
        // dark colours use light text — checked via perceived luminance.
        const float luma = 0.299f * sourceColour.getFloatRed()
                         + 0.587f * sourceColour.getFloatGreen()
                         + 0.114f * sourceColour.getFloatBlue();
        g.setColour(luma > 0.55f
                    ? juce::Colour(0xFF1A1A1A).withAlpha(0.85f)
                    : juce::Colour(0xFFFFFFFF).withAlpha(0.85f));

        g.drawText(glyph,
                   juce::Rectangle<float>(cx - r, cy - r, r * 2.0f, r * 2.0f),
                   juce::Justification::centred,
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
                    ig.drawEllipse(icx - ir + 0.5f, icy - ir + 0.5f,
                                   ir * 2.0f - 1.0f, ir * 2.0f - 1.0f, 0.8f);
                }

                container->startDragging(payload.encode(), this,
                                         juce::ScaledImage(img),
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
    ModSourceId  sourceId;
    juce::Colour sourceColour;
    bool         isDragging { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModSourceHandle)
};

} // namespace xolokun
