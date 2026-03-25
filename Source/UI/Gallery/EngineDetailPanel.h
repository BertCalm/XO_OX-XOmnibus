#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOlokunProcessor.h"
#include "../GalleryColors.h"
#include "MacroHeroStrip.h"
#include "ParameterGrid.h"
#include "MidiLearnMouseListener.h"

namespace xolokun
{

//==============================================================================
// EngineDetailPanel — right-side parameter view for one engine slot.
// Contains a MacroHeroStrip (4 pillar sliders for engine macros) plus a
// scrollable ParameterGrid showing all remaining params.
class EngineDetailPanel : public juce::Component
{
public:
    explicit EngineDetailPanel(XOlokunProcessor& proc)
        : processor(proc), macroHero(proc)
    {
        addAndMakeVisible(macroHero);
        addAndMakeVisible(viewport);
    }

    // Optional: wire MIDI learn manager before the first loadSlot() call.
    void setMidiLearnManager(MIDILearnManager* mgr)
    {
        learnManager = mgr;
        macroHero.setMidiLearnManager(mgr);
    }

    // Called when the user selects an engine slot to inspect.
    // Returns false if the slot is empty.
    bool loadSlot(int slot)
    {
        auto* eng = processor.getEngine(slot);
        if (!eng) return false;

        engineId     = eng->getEngineId();
        accentColour = eng->getAccentColour();

        // Load macro hero strip — it shows/hides itself based on discovery
        auto prefix = GalleryColors::prefixForEngine(engineId);
        macroHero.loadEngine(engineId, prefix, accentColour);

        // Rebuild parameter grid for this engine (pass MIDI learn manager)
        auto* newGrid = new ParameterGrid(processor, engineId, prefix, accentColour, learnManager);
        viewport.setViewedComponent(newGrid, /*takeOwnership=*/true);

        int gridW = juce::jmax(200, viewport.getWidth()
                                    - viewport.getScrollBarThickness() - 4);
        newGrid->setSize(gridW, newGrid->getRequiredHeight(gridW));
        viewport.setViewPosition(0, 0);

        resized();
        repaint();
        return true;
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        g.fillAll(get(slotBg()));

        // ── Ecological header: accent → midnight depth gradient ────────────
        {
            juce::ColourGradient grad(accentColour, 0.0f, 0.0f,
                                      juce::Colour(0xFF1A0A2E), (float)getWidth(), 0.0f, false);
            g.setGradientFill(grad);
            g.fillRect(0, 0, getWidth(), kHeaderH);
        }

        // ── Zone depth bands — three 2px horizontal strips at header bottom ─
        // Visualizes Sunlit / Twilight / Midnight split across the header width
        {
            const int stripH = 2;
            const int stripY = kHeaderH - stripH;
            const float zoneW = getWidth() / 3.0f;
            const uint32_t zoneColors[3] = { 0xFF48CAE4, 0xFF0096C7, 0xFF7B2FBE };
            for (int z = 0; z < 3; ++z)
            {
                g.setColour(juce::Colour(zoneColors[z]).withAlpha(0.55f));
                g.fillRect((int)(z * zoneW), stripY, (int)zoneW, stripH);
            }
        }

        // ── Engine name ────────────────────────────────────────────────────
        g.setColour(juce::Colours::white);
        g.setFont(GalleryFonts::display(16.0f));
        g.drawText(engineId.toUpperCase(),
                   12, 0, getWidth() - 100, kHeaderH,
                   juce::Justification::centredLeft);

        // ── "PARAMETERS" right label ────────────────────────────────────────
        g.setColour(juce::Colours::white.withAlpha(0.45f));
        g.setFont(GalleryFonts::body(8.5f));
        g.drawText("PARAMETERS", 0, 0, getWidth() - 10, kHeaderH,
                   juce::Justification::centredRight);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        area.removeFromTop(kHeaderH);

        // Place macro hero strip below the header (120px tall if visible)
        if (macroHero.isVisible())
        {
            macroHero.setBounds(area.removeFromTop(kHeroH).reduced(4, 2));
        }
        else
        {
            macroHero.setBounds(0, 0, 0, 0);
        }

        viewport.setBounds(area);

        // Resize grid content if it exists
        if (auto* grid = viewport.getViewedComponent())
        {
            int gridW = juce::jmax(200, viewport.getWidth()
                                        - viewport.getScrollBarThickness() - 4);
            int gridH = static_cast<ParameterGrid*>(grid)->getRequiredHeight(gridW);
            grid->setSize(gridW, gridH);
        }
    }

private:
    static constexpr int kHeaderH = 38;
    static constexpr int kHeroH   = 120; // height of the macro hero strip

    XOlokunProcessor&  processor;
    MacroHeroStrip     macroHero;
    juce::Viewport     viewport;
    juce::String       engineId  { "—" };
    juce::Colour       accentColour { GalleryColors::get(GalleryColors::borderGray()) };
    MIDILearnManager*  learnManager = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineDetailPanel)
};

} // namespace xolokun
