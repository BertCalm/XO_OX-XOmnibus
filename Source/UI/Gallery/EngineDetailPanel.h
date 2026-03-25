#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOlokunProcessor.h"
#include "../GalleryColors.h"
#include "MacroHeroStrip.h"
#include "ParameterGrid.h"
#include "MidiLearnMouseListener.h"
#include "ObrixDetailPanel.h"
#include "WaveformDisplay.h"
#include "DrumPadGrid.h"
#include "SpecializedDisplays.h"
#include "SpecializedWidgets.h"

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
        : processor(proc), macroHero(proc), waveformDisplay(proc)
    {
        addAndMakeVisible(macroHero);
        addAndMakeVisible(viewport);
        addAndMakeVisible(waveformDisplay);
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

        // Configure waveform display for this slot
        waveformDisplay.setSlot(slot);
        waveformDisplay.setAccentColour(accentColour);
        waveformDisplay.setEngineId(engineId);

        // OVERBITE gets FiveMacroDisplay instead of standard MacroHeroStrip
        if (engineId.equalsIgnoreCase("Overbite"))
        {
            fiveMacroDisplay = std::make_unique<FiveMacroDisplay>(processor.getAPVTS(), learnManager);
            addAndMakeVisible(*fiveMacroDisplay);
            macroHero.setVisible(false);
        }
        else
        {
            fiveMacroDisplay.reset();
            macroHero.setVisible(true);
        }

        // Percussion engines get a DrumPadGrid above the ParameterGrid
        bool isPercEngine = (engineId.equalsIgnoreCase("Onset") ||
                             engineId.equalsIgnoreCase("Offering"));
        if (isPercEngine)
        {
            int numVoices = 8; // both ONSET and OFFERING use 8 voices for now
            drumGrid = std::make_unique<DrumPadGrid>(processor, prefix, accentColour, numVoices);
            addAndMakeVisible(*drumGrid);
        }
        else
        {
            drumGrid.reset();
        }

        // OBRIX gets a custom panel instead of the generic ParameterGrid
        if (engineId.equalsIgnoreCase("Obrix"))
        {
            obrixPanel = std::make_unique<ObrixDetailPanel>(processor, learnManager);
            viewport.setViewedComponent(obrixPanel.release(), true);
        }
        else
        {
            auto* newGrid = new ParameterGrid(processor, engineId, prefix, accentColour, learnManager);
            newGrid->setParentViewport(&viewport);
            viewport.setViewedComponent(newGrid, true);
        }

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

        // Place macro hero strip (or FiveMacroDisplay for OVERBITE) below the header
        if (fiveMacroDisplay && fiveMacroDisplay->isVisible())
        {
            fiveMacroDisplay->setBounds(area.removeFromTop(56).reduced(4, 2));
        }
        else if (macroHero.isVisible())
        {
            macroHero.setBounds(area.removeFromTop(kHeroH).reduced(4, 2));
        }
        else
        {
            macroHero.setBounds(0, 0, 0, 0);
        }

        // Waveform oscilloscope display (200x80pt, or less if narrow)
        {
            int waveH = 80;
            int waveW = juce::jmin(200, area.getWidth() - 16);
            auto waveArea = area.removeFromTop(waveH + 4);
            waveformDisplay.setBounds(waveArea.withSizeKeepingCentre(waveW, waveH));
        }

        // DrumPadGrid — placed between the waveform display and the viewport
        if (drumGrid)
        {
            int drumH = drumGrid->getRequiredHeight(area.getWidth());
            drumGrid->setBounds(area.removeFromTop(juce::jmin(drumH, 200)));
        }

        viewport.setBounds(area);

        // Resize viewport content: support both ParameterGrid and ObrixDetailPanel
        if (auto* grid = dynamic_cast<ParameterGrid*>(viewport.getViewedComponent()))
        {
            grid->setParentViewport(&viewport);
            int gridW = juce::jmax(200, viewport.getWidth() - viewport.getScrollBarThickness() - 4);
            int gridH = grid->getRequiredHeight(gridW);
            grid->setSize(gridW, gridH);
        }
        else if (auto* obrix = dynamic_cast<ObrixDetailPanel*>(viewport.getViewedComponent()))
        {
            int gridW = juce::jmax(200, viewport.getWidth() - viewport.getScrollBarThickness() - 4);
            int gridH = obrix->getRequiredHeight(gridW);
            obrix->setSize(gridW, gridH);
        }
    }

private:
    static constexpr int kHeaderH = 38;
    static constexpr int kHeroH   = 120; // height of the macro hero strip

    XOlokunProcessor&  processor;
    MacroHeroStrip     macroHero;
    WaveformDisplay    waveformDisplay;
    std::unique_ptr<ObrixDetailPanel>  obrixPanel;      // only created when OBRIX is loaded
    std::unique_ptr<DrumPadGrid>       drumGrid;        // created for ONSET, OFFERING
    std::unique_ptr<FiveMacroDisplay>  fiveMacroDisplay; // created for OVERBITE
    juce::Viewport     viewport;
    juce::String       engineId  { "—" };
    juce::Colour       accentColour { GalleryColors::get(GalleryColors::borderGray()) };
    MIDILearnManager*  learnManager = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineDetailPanel)
};

} // namespace xolokun
