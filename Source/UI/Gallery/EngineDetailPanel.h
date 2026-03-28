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

        // W11: Track whether the engine actually changed so we only reset scroll
        // position on a genuine engine switch (not on parameter-only refreshes).
        const bool engineChanged = (eng->getEngineId() != engineId);

        engineId     = eng->getEngineId();
        accentColour = eng->getAccentColour();

        // P30: cache toUpperCase() result to avoid String alloc in paint().
        cachedEngineName = engineId.toUpperCase();
        // P29: cache header gradient — rebuilt here and in resized(), not in paint().
        cachedHeaderGrad = juce::ColourGradient(accentColour.withAlpha(0.14f), 0.0f, 0.0f,
                                                GalleryColors::get(GalleryColors::shellWhite()),
                                                (float)getWidth(), 0.0f, false);

        // Load macro hero strip — it shows/hides itself based on discovery
        auto prefix = GalleryColors::prefixForEngine(engineId);
        macroHero.loadEngine(engineId, prefix, accentColour);

        // Configure waveform display for this slot
        waveformDisplay.setSlot(slot);
        waveformDisplay.setAccentColour(accentColour);
        waveformDisplay.setEngineId(engineId);

        // ── Reset ALL specialized components before recreating ────────────────
        fiveMacroDisplay.reset();
        drumGrid.reset();
        trianglePad.reset();
        conductorArc.reset();
        modeSelector.reset();
        axisBar.reset();

        // ── OVERBITE gets FiveMacroDisplay instead of standard MacroHeroStrip ─
        if (engineId.equalsIgnoreCase("Overbite"))
        {
            fiveMacroDisplay = std::make_unique<FiveMacroDisplay>(processor.getAPVTS(), learnManager);
            addAndMakeVisible(*fiveMacroDisplay);
            macroHero.setVisible(false);
        }
        else
        {
            macroHero.setVisible(true);
        }

        // ── Percussion engines get a DrumPadGrid above the ParameterGrid ─────
        bool isPercEngine = (engineId.equalsIgnoreCase("Onset") ||
                             engineId.equalsIgnoreCase("Offering"));
        if (isPercEngine)
        {
            int numVoices = 8; // both ONSET and OFFERING use 8 voices for now
            drumGrid = std::make_unique<DrumPadGrid>(processor, prefix, accentColour, numVoices);
            addAndMakeVisible(*drumGrid);
        }

        // ── TriangleXYPad — OVERWORLD (ERA triangle) and OXYTOCIN (love triangle)
        if (engineId.equalsIgnoreCase("Overworld"))
        {
            // Verify both params exist before creating (graceful degradation).
            auto* px = processor.getAPVTS().getParameter("ow_era");
            auto* py = processor.getAPVTS().getParameter("ow_eraY");
            if (px && py)
            {
                trianglePad = std::make_unique<TriangleXYPad>(
                    processor.getAPVTS(),
                    "ow_era",    // X param — ERA horizontal blend
                    "ow_eraY",   // Y param — ERA vertical blend
                    std::array<juce::String, 3>{{ "ANALOG", "DIGITAL", "HYBRID" }},
                    std::array<juce::Colour, 3>{{
                        juce::Colour(0xFF39FF14),  // Neon Green (OVERWORLD accent)
                        juce::Colour(0xFFBF40FF),  // Prism Violet
                        juce::Colour(GalleryColors::xoGold)   // XO Gold
                    }});
                addAndMakeVisible(*trianglePad);
            }
        }
        else if (engineId.equalsIgnoreCase("Oxytocin"))
        {
            // OXYTOCIN's love triangle uses oxy_intimacy (X) and oxy_passion (Y).
            // These are the two independent love-triangle axes present in OxytocinAdapter.
            auto* px = processor.getAPVTS().getParameter("oxy_intimacy");
            auto* py = processor.getAPVTS().getParameter("oxy_passion");
            if (px && py)
            {
                trianglePad = std::make_unique<TriangleXYPad>(
                    processor.getAPVTS(),
                    "oxy_intimacy",   // X param — Intimacy axis
                    "oxy_passion",    // Y param — Passion axis
                    std::array<juce::String, 3>{{ "RE-201", "MS-20", "MOOG" }},
                    std::array<juce::Colour, 3>{{
                        juce::Colour(0xFF9B5DE5),  // Synapse Violet (OXYTOCIN accent)
                        juce::Colour(0xFFFF6B6B),  // Warm Red
                        juce::Colour(GalleryColors::xoGold)   // XO Gold
                    }});
                addAndMakeVisible(*trianglePad);
            }
        }

        // ── ConductorArcDisplay — OPERA autonomous dramatic arc visualizer ────
        if (engineId.equalsIgnoreCase("Opera"))
        {
            conductorArc = std::make_unique<ConductorArcDisplay>(processor.getAPVTS());
            addAndMakeVisible(*conductorArc);
        }

        // ── NamedModeSelector — engines with meaningful choice parameter names ─
        // Note: ow_era is a float param (not a choice), so OVERWORLD's ERA
        // blending is handled entirely by the TriangleXYPad above.
        // NamedModeSelector requires an AudioParameterChoice to bind via
        // ComboBoxAttachment — only wire it to genuine choice params.
        if (engineId.equalsIgnoreCase("Ocelot"))
        {
            if (processor.getAPVTS().getParameter("ocelot_biome"))
            {
                modeSelector = std::make_unique<NamedModeSelector>(
                    processor.getAPVTS(), "ocelot_biome",
                    juce::StringArray{ "RAINFOREST", "SAVANNA", "TUNDRA", "REEF" },
                    static_cast<const std::vector<juce::Colour>*>(nullptr),
                    juce::Colour(0xFFC5832B)); // Ocelot Tawny
                addAndMakeVisible(*modeSelector);
            }
        }
        else if (engineId.equalsIgnoreCase("Oracle"))
        {
            auto* p = processor.getAPVTS().getParameter("oracle_maqam");
            if (p)
            {
                if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(p))
                {
                    modeSelector = std::make_unique<NamedModeSelector>(
                        processor.getAPVTS(), "oracle_maqam",
                        choice->choices,
                        static_cast<const std::vector<juce::Colour>*>(nullptr),
                        juce::Colour(0xFF4B0082)); // Prophecy Indigo
                    addAndMakeVisible(*modeSelector);
                }
            }
        }
        else if (engineId.equalsIgnoreCase("Orbweave"))
        {
            if (processor.getAPVTS().getParameter("weave_knotType"))
            {
                modeSelector = std::make_unique<NamedModeSelector>(
                    processor.getAPVTS(), "weave_knotType",
                    juce::StringArray{ "TREFOIL", "FIGURE-8", "TORUS", "SOLOMON" },
                    static_cast<const std::vector<juce::Colour>*>(nullptr),
                    juce::Colour(0xFF8E4585)); // Kelp Knot Purple (ORBWEAVE accent)
                addAndMakeVisible(*modeSelector);
            }
        }

        // ── BipolarAxisBar — engines with bipolar interaction axes ────────────
        if (engineId.equalsIgnoreCase("Ouie"))
        {
            if (processor.getAPVTS().getParameter("ouie_macroHammer"))
            {
                axisBar = std::make_unique<BipolarAxisBar>(
                    processor.getAPVTS(), "ouie_macroHammer",
                    "STRIFE", "LOVE",
                    juce::Colour(0xFFFF2D2D),  // Ouroboros Red (strife)
                    juce::Colour(0xFFFF69B4)); // Pink (love)
                addAndMakeVisible(*axisBar);
            }
        }
        else if (engineId.equalsIgnoreCase("Obese"))
        {
            // OBESE mojo axis: fat_mojo is the real parameter ID (not fat_satMojo).
            if (processor.getAPVTS().getParameter("fat_mojo"))
            {
                axisBar = std::make_unique<BipolarAxisBar>(
                    processor.getAPVTS(), "fat_mojo",
                    "ANALOG", "DIGITAL",
                    juce::Colour(0xFFE9A84A),  // Warm amber
                    juce::Colour(0xFFBF40FF)); // Prism Violet
                addAndMakeVisible(*axisBar);
            }
        }
        else if (engineId.equalsIgnoreCase("Oware"))
        {
            if (processor.getAPVTS().getParameter("owr_material"))
            {
                axisBar = std::make_unique<BipolarAxisBar>(
                    processor.getAPVTS(), "owr_material",
                    "SOFT", "HARD",
                    juce::Colour(0xFF8B6914),  // Warm wood
                    juce::Colour(0xFFC0C0C0)); // Metallic silver
                addAndMakeVisible(*axisBar);
            }
        }

        // ── OBRIX gets a custom panel instead of the generic ParameterGrid ────
        if (engineId.equalsIgnoreCase("Obrix"))
        {
            // Clear the viewport BEFORE resetting obrixPanel to avoid a dangling
            // pointer: if a previous OBRIX load transferred ownership to the
            // viewport via release(), the viewport holds the raw ptr. Clearing
            // first lets the viewport delete that component safely before we
            // create a new one.
            viewport.setViewedComponent(nullptr, false);
            obrixPanel.reset(); // safe: viewport no longer references it
            obrixPanel = std::make_unique<ObrixDetailPanel>(processor, learnManager);
            viewport.setViewedComponent(obrixPanel.release(), true);
        }
        else
        {
            auto* newGrid = new ParameterGrid(processor, engineId, prefix, accentColour, learnManager);
            newGrid->setParentViewport(&viewport);
            viewport.setViewedComponent(newGrid, true);
        }

        // W11: Only reset scroll to top when the engine actually changed.
        if (engineChanged)
            viewport.setViewPosition(0, 0);

        resized();
        repaint();
        return true;
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;

        // ── Clean header: engine name + thin accent line ──────────────────────
        // No heavy gradient, no zone bands — matches v05 mockup.
        {
            // Engine name — 16px Space Grotesk bold, accent color
            g.setFont(GalleryFonts::display(16.0f));
            g.setColour(accentColour);
            g.drawText(cachedEngineName,
                       juce::Rectangle<int>(12, 0, getWidth() - 24, kHeaderH),
                       juce::Justification::centredLeft);

            // Thin accent line under header (2px)
            g.setColour(accentColour.withAlpha(0.5f));
            g.fillRect(12, kHeaderH - 2, getWidth() - 24, 2);
        }

        // ── "OSCILLOSCOPE" label above waveform display ───────────────────────
        if (!oscLabelBounds.isEmpty())
        {
            g.setFont(GalleryFonts::value(9.0f));
            g.setColour(get(t3()));
            g.drawText("OSCILLOSCOPE",
                       oscLabelBounds.withTrimmedLeft(8),
                       juce::Justification::centredLeft,
                       false);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds();
        area.removeFromTop(kHeaderH);

        // P29: rebuild header gradient when width changes.
        cachedHeaderGrad = juce::ColourGradient(accentColour.withAlpha(0.14f), 0.0f, 0.0f,
                                                GalleryColors::get(GalleryColors::shellWhite()),
                                                (float)getWidth(), 0.0f, false);

        // MacroHeroStrip hidden — macros are in the header row.
        macroHero.setBounds(0, 0, 0, 0);
        macroHero.setVisible(false);
        if (fiveMacroDisplay)
        {
            fiveMacroDisplay->setBounds(0, 0, 0, 0);
            fiveMacroDisplay->setVisible(false);
        }

        // ── Bottom: "OSCILLOSCOPE" label (10px) + waveform display (70px) ────
        {
            int waveH     = 70;
            int labelH    = 12;
            auto waveArea = area.removeFromBottom(waveH + labelH);
            oscLabelBounds = waveArea.removeFromTop(labelH);  // painted in paint()
            waveformDisplay.setBounds(waveArea.reduced(4, 2));
        }

        // ── Specialized displays (engine-specific, above parameter grid) ─────
        if (drumGrid)
        {
            int drumH = drumGrid->getRequiredHeight(area.getWidth());
            drumGrid->setBounds(area.removeFromTop(juce::jmin(drumH, 160)));
        }
        if (trianglePad)
        {
            auto padArea = area.removeFromTop(144);
            trianglePad->setBounds(padArea.withSizeKeepingCentre(160, 140));
        }
        if (conductorArc)
        {
            auto arcArea = area.removeFromTop(64);
            conductorArc->setBounds(arcArea.withSizeKeepingCentre(200, 60));
        }
        if (modeSelector)
            modeSelector->setBounds(area.removeFromTop(36).reduced(4, 2));
        if (axisBar)
            axisBar->setBounds(area.removeFromTop(32).reduced(4, 2));

        // ── Main area: Viewport with ParameterGrid (fills remaining space) ───
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
    static constexpr int kHeroH   = 88; // height of the macro hero strip

    XOlokunProcessor&  processor;
    MacroHeroStrip     macroHero;
    WaveformDisplay    waveformDisplay;
    std::unique_ptr<ObrixDetailPanel>     obrixPanel;       // only created when OBRIX is loaded
    std::unique_ptr<DrumPadGrid>          drumGrid;         // created for ONSET, OFFERING
    std::unique_ptr<FiveMacroDisplay>     fiveMacroDisplay; // created for OVERBITE
    std::unique_ptr<TriangleXYPad>        trianglePad;      // created for OVERWORLD, OXYTOCIN
    std::unique_ptr<ConductorArcDisplay>  conductorArc;     // created for OPERA
    std::unique_ptr<NamedModeSelector>    modeSelector;     // created for OVERWORLD, OCELOT, ORACLE, ORBWEAVE
    std::unique_ptr<BipolarAxisBar>       axisBar;          // created for OUIE, OBESE, OWARE
    juce::Viewport     viewport;
    juce::String       engineId  { "—" };
    juce::Colour       accentColour { GalleryColors::get(GalleryColors::borderGray()) };
    MIDILearnManager*  learnManager = nullptr;
    // P29: cached header gradient — rebuilt in loadSlot() and resized().
    juce::ColourGradient cachedHeaderGrad;
    // P30: cached uppercase engine name — set in loadSlot(), used in paint().
    juce::String         cachedEngineName { "—" };
    // Bounds for the "OSCILLOSCOPE" label painted above the waveform display.
    juce::Rectangle<int> oscLabelBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineDetailPanel)
};

} // namespace xolokun
