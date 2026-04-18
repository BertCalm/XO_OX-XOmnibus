// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// EnginePickerDrawer.h — Slide-from-left engine picker drawer for XOceanus Ocean View.
//
// A persistent dark panel (~380px wide, full parent height) that animates in/out
// from the left edge of the ocean area.  It mirrors the EnginePickerPopup feature
// set (search field + category filter pills + scrollable engine list) but lives
// as a permanent child component rather than a transient CallOutBox.
//
// Usage (from OceanView or any parent Component):
//
//   // Declare as member:
//   xoceanus::EnginePickerDrawer engineDrawer;
//
//   // Wire up callback BEFORE adding to parent:
//   engineDrawer.onEngineSelected = [this](const juce::String& id) {
//       loadEngineIntoSlot(id, activeSlot);
//   };
//
//   // Add as child — starts hidden (animProgress_ = 0, setVisible(false)):
//   addChildComponent(engineDrawer);
//
//   // In parent's resized():
//   //   Give the drawer its logical (fully-open) bounds.  The drawer moves
//   //   its own x-position each animation tick via setTopLeftPosition().
//   engineDrawer.setBounds(getLocalBounds()
//                              .withWidth(EnginePickerDrawer::kDrawerWidth));
//
//   // Toggle open/close on a button press:
//   engineDrawer.toggle();
//
// Z-order: addChildComponent the drawer AFTER other ocean children so it
// renders on top of them.
//
// Slide animation: 250 ms ease-out on open, linear on close. Timer ticks at
// ~16 ms (≈60 fps).  The drawer moves itself via setTopLeftPosition() — the
// parent component does not need to call resized() during animation.
//
// Thread safety: all methods must be called on the message thread.

#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace xoceanus
{

//==============================================================================
class EnginePickerDrawer : public juce::Component,
                           public juce::Timer
{
public:
    //==========================================================================
    // Public API
    //==========================================================================

    // Fired when the user clicks an engine row.  The drawer does NOT close
    // automatically — the caller decides whether to close it.
    std::function<void(const juce::String& engineId)> onEngineSelected;

    EnginePickerDrawer();
    ~EnginePickerDrawer() override;

    // Animate slide in from left
    void open();

    // Animate slide out to left
    void close();

    bool isOpen()  const noexcept { return animState_ != AnimState::Closed; }
    void toggle()        { isOpen() ? close() : open(); }

    // Mark which engines are currently loaded in the 4 engine slots.
    // Loaded engines receive a teal ring around their accent dot.
    void setLoadedEngines(const std::array<juce::String, 4>& slotEngines);

    // juce::Component overrides
    void paint    (juce::Graphics& g) override;
    void resized  () override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    // juce::Timer override
    void timerCallback() override;

    //==========================================================================
    // Constants
    //==========================================================================
    static constexpr int kDrawerWidth = 380;

private:
    //==========================================================================
    // Design constants
    //==========================================================================
    static constexpr float kAnimDurationMs  = 250.0f;
    static constexpr int   kTimerIntervalMs = 16;
    static constexpr int   kRowHeight       = 72;
    static constexpr int   kHeaderH         = 52;
    static constexpr int   kSearchH         = 40;
    static constexpr int   kPillBarH        = 36;
    static constexpr int   kNumCats         = 10;  // ALL + 9 role categories

    //==========================================================================
    // Color palette (Ocean design system — dark navy)
    //==========================================================================
    static juce::Colour colDrawerBg()     noexcept { return juce::Colour(0xFF1A2332); }
    static juce::Colour colTeal()         noexcept { return juce::Colour(0xFF7FDBCA); }
    static juce::Colour colSearchBg()     noexcept { return juce::Colour(0xFF243040); }
    static juce::Colour colSearchText()   noexcept { return juce::Colour(0xFF8899AA); }
    static juce::Colour colPillActiveBg() noexcept { return juce::Colour(0xFF2A5A50); }
    static juce::Colour colPillInactTx()  noexcept { return juce::Colour(0xFF667788); }
    static juce::Colour colEngineName()   noexcept { return juce::Colour(0xFFE0E8F0); }
    static juce::Colour colArchetype()    noexcept { return juce::Colour(0xFF667788); }
    static juce::Colour colRowHover()     noexcept { return juce::Colour(0xFF243444); }
    static juce::Colour colRowSelected()  noexcept { return juce::Colour(0xFF1E3A4A); }
    static juce::Colour colCloseBtn()     noexcept { return juce::Colour(0xFF667788); }
    static juce::Colour colBorder()       noexcept { return juce::Colour(0xFF243040); }

    //==========================================================================
    // Engine metadata (mirrors EnginePickerPopup::EngineInfo — keep in sync)
    //==========================================================================
    struct DrawerEngineInfo
    {
        const char* id;
        const char* category;   // "Synth"|"Percussion"|"Bass"|"Pad"|"String"|"Organ"|"Vocal"|"FX"|"Utility"
        const char* archetype;  // short description displayed under the engine name
        uint32_t    accentARGB;
        int         depthZone;  // 0=Sunlit 1=Twilight 2=Midnight
    };

    static const DrawerEngineInfo* engineMetadataTable() noexcept
    {
        // clang-format off
        static const DrawerEngineInfo kTable[] =
        {
            // ── Kitchen Collection — Organs (Chef Quad) ──────────────────────
            { "Oto",       "Organ",      "tonewheel drawbar organ",                  0xFFF5F0E8, 0 },
            { "Octave",    "Organ",      "Hammond tonewheel simulation",             0xFF8B6914, 0 },
            { "Oleg",      "Organ",      "theatre pipe organ",                       0xFFC0392B, 0 },
            { "Otis",      "Organ",      "gospel soul organ drive",                  0xFFD4A017, 0 },
            // ── Kitchen Collection — Pianos (Kitchen Quad) ───────────────────
            { "Oven",      "String",     "Steinway concert grand piano",             0xFF1C1C1C, 1 },
            { "Ochre",     "String",     "wooden resonator piano",                   0xFFCC7722, 1 },
            { "Obelisk",   "String",     "grand piano sympathetic resonance",        0xFFFFFFE0, 0 },
            { "Opaline",   "String",     "prepared piano rust and objects",          0xFFB7410E, 1 },
            // ── Kitchen Collection — Bass (Cellar Quad) ──────────────────────
            { "Ogre",      "Bass",       "sub bass synthesizer",                     0xFF0D0D0D, 2 },
            { "Olate",     "Bass",       "fretless bass guitar",                     0xFF5C3317, 1 },
            { "Oaken",     "Bass",       "upright double bass",                      0xFF9C6B30, 1 },
            { "Omega",     "Bass",       "analog synth bass",                        0xFF003366, 2 },
            // ── Kitchen Collection — Strings (Garden Quad) ───────────────────
            { "Orchard",   "String",     "orchestral strings bow pressure",          0xFFFFB7C5, 0 },
            { "Overgrow",  "String",     "overgrown string textures",                0xFF228B22, 1 },
            { "Osier",     "String",     "willow wind strings",                      0xFFC0C8C8, 0 },
            { "Oxalis",    "String",     "wood sorrel lilac strings",                0xFF9B59B6, 1 },
            // ── Kitchen Collection — Pads (Broth Quad) ───────────────────────
            { "Overwash",  "Pad",        "tide foam diffusion pad",                  0xFFF0F8FF, 0 },
            { "Overworn",  "Pad",        "worn felt texture pad",                    0xFF808080, 1 },
            { "Overflow",  "Pad",        "deep current flowing pad",                 0xFF1A3A5C, 2 },
            { "Overcast",  "Pad",        "cloud diffusion pad",                      0xFF778899, 1 },
            // ── Kitchen Collection — EPs (Fusion Quad) ───────────────────────
            { "Oasis",     "Synth",      "desert spring electric piano",             0xFF00827F, 0 },
            { "Oddfellow", "Synth",      "spectral fingerprint cache EP",            0xFFB87333, 1 },
            { "Onkolo",    "Synth",      "spectral amber resonant EP",               0xFFFFBF00, 1 },
            { "Opcode",    "Synth",      "dark turquoise code-driven EP",            0xFF5F9EA0, 1 },
            // ── Flagship + core synths ───────────────────────────────────────
            { "Obrix",     "Synth",      "modular brick reef synthesizer",           0xFF1E8B7E, 2 },
            { "Oxytocin",  "Synth",      "circuit love triangle synthesizer",        0xFF9B5DE5, 2 },
            { "Overbite",  "Synth",      "apex predator modal synthesizer",          0xFFF0EDE8, 2 },
            { "Overworld", "Synth",      "ERA triangle timbral crossfade",           0xFF39FF14, 0 },
            { "Ouroboros", "Synth",      "strange attractor chaotic synthesizer",    0xFFFF2D2D, 2 },
            { "Oracle",    "Synth",      "GENDY stochastic maqam synthesis",         0xFF4B0082, 2 },
            { "Orbital",   "Synth",      "group envelope synthesizer",               0xFFFF6B6B, 1 },
            { "Opal",      "Synth",      "granular cloud synthesizer",               0xFFA78BFA, 1 },
            { "Obsidian",  "Synth",      "crystal resonant synthesizer",             0xFFE8E0D8, 2 },
            { "Origami",   "Synth",      "fold-point waveshaping synthesizer",       0xFFE63946, 1 },
            { "Obscura",   "Synth",      "daguerreotype physical modeling",          0xFF8A9BA8, 1 },
            { "Oblique",   "Synth",      "prismatic bounce synth",                   0xFFBF40FF, 1 },
            { "Organism",  "Synth",      "cellular automata generative synth",       0xFFC6E377, 1 },
            { "Orbweave",  "Synth",      "topological knot coupling engine",         0xFF8E4585, 2 },
            { "Overtone",  "Synth",      "continued fraction spectral synth",        0xFFA8D8EA, 1 },
            { "Oxbow",     "Synth",      "entangled reverb synthesizer",             0xFF1A6B5A, 2 },
            { "Outlook",   "Synth",      "panoramic dual wavetable synth",           0xFF4169E1, 1 },
            { "Overlap",   "Synth",      "knot matrix FDN synthesizer",              0xFF00FFB4, 2 },
            { "Orca",      "Synth",      "apex predator wavetable echolocation",     0xFF1B2838, 2 },
            { "Octopus",   "Synth",      "decentralized alien intelligence synth",   0xFFE040FB, 2 },
            { "Ombre",     "Synth",      "dual narrative memory synthesizer",        0xFF7B6B8A, 1 },
            { "OpenSky",   "Synth",      "euphoric shimmer supersaw synth",          0xFFFF8C00, 0 },
            // ── Percussion ───────────────────────────────────────────────────
            { "Onset",     "Percussion", "cross-voice coupling percussion",          0xFF0066FF, 1 },
            { "Offering",  "Percussion", "psychology-driven boom bap drums",         0xFFE5B80B, 1 },
            { "Oware",     "Percussion", "Akan tuned mallet percussion",             0xFFB5883E, 1 },
            { "Ostinato",  "Percussion", "modal membrane world rhythm engine",       0xFFE8701A, 1 },
            // ── Vocal ────────────────────────────────────────────────────────
            { "Opera",     "Vocal",      "additive-vocal Kuramoto synchrony",        0xFFD4AF37, 1 },
            { "Obbligato", "Vocal",      "breath articulation vocal synth",          0xFFFF8A7A, 1 },
            // ── Bass synths ──────────────────────────────────────────────────
            { "Oblong",    "Bass",       "resonant bass synthesizer",                0xFFE9A84A, 1 },
            { "Obese",     "Bass",       "fat saturation bass synth",                0xFFFF1493, 1 },
            // ── Organ & wind ─────────────────────────────────────────────────
            { "Organon",   "Organ",      "variational metabolism organ synth",       0xFF00CED1, 1 },
            { "Ohm",       "Organ",      "sage analog organ synthesizer",            0xFF87AE73, 0 },
            { "Ottoni",    "Organ",      "patina brass organ synthesizer",           0xFF5B8A72, 1 },
            { "Ole",       "Organ",      "hibiscus flamenco organ synth",            0xFFC9377A, 1 },
            // ── String / physical modeling ───────────────────────────────────
            { "Orphica",   "String",     "siren seafoam plucked string",             0xFF7FDBCA, 1 },
            { "Osprey",    "String",     "shore coastline cultural synthesis",       0xFF1B4F8A, 1 },
            { "Osteria",   "String",     "porto wine shore string synth",            0xFF722F37, 1 },
            { "Owlfish",   "String",     "Mixtur-Trautonium string modeling",        0xFFB8860B, 2 },
            // ── Character synths ─────────────────────────────────────────────
            { "OddfeliX",  "Synth",      "neon tetra character synth",              0xFF00A6D6, 0 },
            { "OddOscar",  "Synth",      "axolotl character synth",                 0xFFE8839B, 0 },
            { "Odyssey",   "Synth",      "drift analog poly synthesizer",            0xFF7B2D8B, 1 },
            { "Overdub",   "Synth",      "spring reverb dub synthesizer",            0xFF6B7B3A, 1 },
            { "Oceanic",   "Synth",      "chromatophore phosphorescent synth",       0xFF00B4A0, 1 },
            { "Ocelot",    "Synth",      "biome crossfade ocelot synth",             0xFFC5832B, 1 },
            { "Osmosis",   "Synth",      "external audio membrane synth",            0xFFC0C0C0, 1 },
            // ── Utility / FX ─────────────────────────────────────────────────
            { "Optic",     "Utility",    "visual modulation zero-audio engine",      0xFF00FF41, 0 },
            { "Outwit",    "FX",         "chromatophore amber effect engine",        0xFFCC6600, 1 },
            // ── Additional engines ───────────────────────────────────────────
            { "OceanDeep", "Synth",      "hydrostatic deep ocean synthesizer",       0xFF2D0A4E, 2 },
            { "Ouie",      "Synth",      "duophonic hammerhead synthesizer",         0xFF708090, 2 },
            { "Obiont",    "Synth",      "cellular automata oscillator",             0xFFE8A030, 2 },
            { "Okeanos",   "String",     "Spice Route Rhodes electric piano",        0xFFC49B3F, 1 },
            { "Outflow",   "Synth",      "predictive spatial fluid-dynamics engine", 0xFF1A1A40, 2 },
            // Sentinel — must remain last
            { nullptr, nullptr, nullptr, 0, 0 },
        };
        // clang-format on
        return kTable;
    }

    //==========================================================================
    // Category filter labels and their corresponding category strings
    //==========================================================================
    static const char* kFilterLabel(int i) noexcept
    {
        static const char* kL[kNumCats] = {
            "ALL","Synth","Perc","Bass","Pad","String","Organ","Vocal","FX","Util"
        };
        return (i >= 0 && i < kNumCats) ? kL[i] : "ALL";
    }

    // Returns the raw category string that must match EngineRow::category.
    // Index 0 (ALL) returns "" — an empty string matches all categories.
    static const char* kFilterCategory(int i) noexcept
    {
        static const char* kC[kNumCats] = {
            "",           // ALL
            "Synth",      // 1
            "Percussion", // 2
            "Bass",       // 3
            "Pad",        // 4
            "String",     // 5
            "Organ",      // 6
            "Vocal",      // 7
            "FX",         // 8
            "Utility",    // 9
        };
        return (i >= 0 && i < kNumCats) ? kC[i] : "";
    }

    //==========================================================================
    // Per-row data for the filtered engine list
    //==========================================================================
    struct EngineRow
    {
        juce::String id;
        juce::String category;
        juce::String archetype;
        juce::Colour accent { 0xFF7FDBCA };
        int          depthZone = 1;
    };

    //==========================================================================
    // Animation state
    //==========================================================================
    enum class AnimState { Closed, Opening, Open, Closing };

    //==========================================================================
    // Pill button LookAndFeel
    //==========================================================================
    struct PillLookAndFeel : public juce::LookAndFeel_V4
    {
        void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                                  const juce::Colour& /*bg*/,
                                  bool /*hilighted*/, bool /*isDown*/) override
        {
            const bool active = btn.getToggleState();
            const auto b = btn.getLocalBounds().toFloat().reduced(1.0f, 1.0f);
            if (active)
            {
                g.setColour(colPillActiveBg());
                g.fillRoundedRectangle(b, 5.0f);
                g.setColour(colTeal().withAlpha(0.50f));
                g.drawRoundedRectangle(b, 5.0f, 1.0f);
            }
            // inactive: transparent background (no fill)
        }

        void drawButtonText(juce::Graphics& g, juce::TextButton& btn,
                            bool /*hilighted*/, bool /*isDown*/) override
        {
            const bool active = btn.getToggleState();
            g.setFont(GalleryFonts::label(10.5f));
            g.setColour(active ? colTeal() : colPillInactTx());
            g.drawFittedText(btn.getButtonText(),
                             btn.getLocalBounds().reduced(2, 0),
                             juce::Justification::centred, 1, 0.9f);
        }
    };

    //==========================================================================
    // Inner scrollable list component
    //==========================================================================
    // Declared as a nested class so it can call back into the drawer's private state.
    class EngineListComponent : public juce::Component
    {
    public:
        explicit EngineListComponent(EnginePickerDrawer& owner) : owner_(owner)
        {
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
        }

        void paint(juce::Graphics& g) override
        {
            const auto& rows  = owner_.filteredEngines_;
            const int   nRows = (int)rows.size();
            const int   w     = getWidth();

            if (nRows == 0)
            {
                g.setFont(GalleryFonts::body(13.0f));
                g.setColour(colArchetype());
                g.drawText("No engines match your search",
                           getLocalBounds(), juce::Justification::centred, false);
                return;
            }

            for (int i = 0; i < nRows; ++i)
            {
                const auto& row  = rows[(size_t)i];
                const int   rowY = i * kRowHeight;

                // ── Row background ────────────────────────────────────────────
                if (i == owner_.selectedRow_)
                    g.setColour(colRowSelected());
                else if (i == owner_.hoveredRow_)
                    g.setColour(colRowHover());
                else
                    g.setColour(colDrawerBg());
                g.fillRect(juce::Rectangle<int>(0, rowY, w, kRowHeight));

                // ── Separator ─────────────────────────────────────────────────
                g.setColour(colBorder().withAlpha(0.55f));
                g.drawHorizontalLine(rowY + kRowHeight - 1, 0.0f, (float)w);

                // ── Accent dot (right side, vertically centred) ───────────────
                const float dotR  = 4.0f;
                const float dotCx = (float)w - 14.0f;
                const float dotCy = (float)rowY + (float)kRowHeight * 0.5f;
                g.setColour(row.accent.withAlpha(0.88f));
                g.fillEllipse(dotCx - dotR, dotCy - dotR, dotR * 2.0f, dotR * 2.0f);

                // Teal ring if this engine is currently loaded in a slot
                if (owner_.loadedEngineIds_.count(row.id.toStdString()) > 0)
                {
                    g.setColour(colTeal().withAlpha(0.75f));
                    g.drawEllipse(dotCx - dotR - 2.0f, dotCy - dotR - 2.0f,
                                  (dotR + 2.0f) * 2.0f, (dotR + 2.0f) * 2.0f, 1.5f);
                }

                // ── Engine name (bold white) ──────────────────────────────────
                const int textX = 16;
                const int nameY = rowY + 14;
                const int textW = w - textX - 30; // leave room for accent dot
                g.setFont(GalleryFonts::display(14.0f));
                g.setColour(colEngineName());
                g.drawText(row.id, textX, nameY, textW, 18,
                           juce::Justification::centredLeft, true);

                // ── Archetype description (smaller, muted) ────────────────────
                if (row.archetype.isNotEmpty())
                {
                    g.setFont(GalleryFonts::body(11.5f));
                    g.setColour(colArchetype());
                    g.drawText(row.archetype, textX, nameY + 20, textW, 16,
                               juce::Justification::centredLeft, true);
                }
            }
        }

        void mouseMove(const juce::MouseEvent& e) override
        {
            const int row = e.y / kRowHeight;
            if (row != owner_.hoveredRow_)
            {
                owner_.hoveredRow_ = (row >= 0 && row < (int)owner_.filteredEngines_.size())
                                         ? row : -1;
                repaint();
            }
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            const int row = e.y / kRowHeight;
            if (row >= 0 && row < (int)owner_.filteredEngines_.size())
            {
                owner_.selectedRow_ = row;
                repaint();
                if (owner_.onEngineSelected)
                    owner_.onEngineSelected(owner_.filteredEngines_[(size_t)row].id);
            }
        }

        void mouseExit(const juce::MouseEvent& /*e*/) override
        {
            if (owner_.hoveredRow_ != -1)
            {
                owner_.hoveredRow_ = -1;
                repaint();
            }
        }

    private:
        EnginePickerDrawer& owner_;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineListComponent)
    };

    //==========================================================================
    // Private helper methods (declared here, defined below the class)
    //==========================================================================
    void buildMetadataLookup();
    void populateEngineList();
    void updateFilter();
    void applyAnimPosition();
    void styleAllPillButtons();

    //==========================================================================
    // Member variables
    //==========================================================================

    // Metadata lookup: lowercase engine id → index into kTable
    std::unordered_map<std::string, int> metaLookup_;

    // Full engine roster (sorted alphabetically)
    std::vector<EngineRow> allEngines_;

    // Filtered subset currently displayed in the list
    std::vector<EngineRow> filteredEngines_;

    // Engine IDs currently loaded in the 4 engine slots (for the indicator ring)
    std::unordered_set<std::string> loadedEngineIds_;

    // Animation
    AnimState animState_    = AnimState::Closed;
    float     animProgress_ = 0.0f;   // 0.0 = fully hidden, 1.0 = fully visible
    int64_t   animStartMs_  = 0;

    // Interaction state (row indices into filteredEngines_)
    int hoveredRow_  = -1;
    int selectedRow_ = -1;

    // Active category filter (0 = ALL)
    int activeCatIdx_ = 0;

    // Close button bounds (local component coords) + hover state
    juce::Rectangle<int> closeBtnBounds_;
    bool                 closeBtnHovered_ = false;

    // UI widgets
    juce::TextEditor    searchField_;
    juce::TextButton    catBtns_[kNumCats];
    juce::Viewport      listViewport_;
    EngineListComponent listComp_;     // must be declared AFTER filteredEngines_ / selectedRow_
    PillLookAndFeel     pillLnF_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnginePickerDrawer)
};

//==============================================================================
// Inline method definitions
//==============================================================================

inline EnginePickerDrawer::EnginePickerDrawer()
    : listComp_(*this)
{
    buildMetadataLookup();
    populateEngineList();

    // ── Search field ──────────────────────────────────────────────────────────
    searchField_.setTextToShowWhenEmpty("Search engines...", colSearchText());
    searchField_.setColour(juce::TextEditor::backgroundColourId,      colSearchBg());
    searchField_.setColour(juce::TextEditor::outlineColourId,         colBorder());
    searchField_.setColour(juce::TextEditor::focusedOutlineColourId,  colTeal().withAlpha(0.55f));
    searchField_.setColour(juce::TextEditor::textColourId,            colEngineName());
    searchField_.setColour(juce::CaretComponent::caretColourId,       colTeal());
    searchField_.setFont(GalleryFonts::body(13.0f));
    searchField_.setReturnKeyStartsNewLine(false);
    searchField_.onTextChange = [this] { updateFilter(); };
    addAndMakeVisible(searchField_);

    // ── Category pill buttons ─────────────────────────────────────────────────
    for (int i = 0; i < kNumCats; ++i)
    {
        catBtns_[i].setButtonText(kFilterLabel(i));
        catBtns_[i].setClickingTogglesState(false);
        catBtns_[i].setLookAndFeel(&pillLnF_);
        const int idx = i;
        catBtns_[i].onClick = [this, idx]
        {
            activeCatIdx_ = idx;
            styleAllPillButtons();
            updateFilter();
        };
        addAndMakeVisible(catBtns_[i]);
    }
    styleAllPillButtons(); // set initial toggle states

    // ── Scrollable viewport + list component ──────────────────────────────────
    // The viewport does NOT own listComp_ (false = don't delete).
    // EnginePickerDrawer owns listComp_ directly as a member.
    listViewport_.setViewedComponent(&listComp_, false);
    listViewport_.setScrollBarsShown(true, false); // vertical only
    listViewport_.setScrollBarThickness(4);
    listViewport_.getVerticalScrollBar().setColour(
        juce::ScrollBar::thumbColourId, colTeal().withAlpha(0.35f));
    addAndMakeVisible(listViewport_);

    // ── Initial state ─────────────────────────────────────────────────────────
    updateFilter();
    setVisible(false);          // drawer starts hidden
    setSize(kDrawerWidth, 600); // nominal size; parent must call setBounds before open()
}

inline EnginePickerDrawer::~EnginePickerDrawer()
{
    stopTimer();
    // Detach LookAndFeel before pill buttons destruct (JUCE requirement)
    for (auto& btn : catBtns_)
        btn.setLookAndFeel(nullptr);
    // Detach the viewport from listComp_ before either destructs
    listViewport_.setViewedComponent(nullptr, false);
}

//------------------------------------------------------------------------------
inline void EnginePickerDrawer::open()
{
    if (animState_ == AnimState::Open || animState_ == AnimState::Opening)
        return;

    setVisible(true);
    toFront(false);
    // If closing, reverse from current progress instead of jumping to 0
    if (animState_ != AnimState::Closing)
        animProgress_ = 0.0f;
    animState_   = AnimState::Opening;
    animStartMs_ = juce::Time::currentTimeMillis()
                   - (int64_t)(animProgress_ * kAnimDurationMs);
    startTimer(kTimerIntervalMs);
}

inline void EnginePickerDrawer::close()
{
    if (animState_ == AnimState::Closed || animState_ == AnimState::Closing)
        return;

    if (animState_ != AnimState::Opening)
        animProgress_ = 1.0f;
    animState_   = AnimState::Closing;
    animStartMs_ = juce::Time::currentTimeMillis()
                   - (int64_t)((1.0f - animProgress_) * kAnimDurationMs);
    startTimer(kTimerIntervalMs);
}

//------------------------------------------------------------------------------
inline void EnginePickerDrawer::setLoadedEngines(const std::array<juce::String, 4>& slotEngines)
{
    loadedEngineIds_.clear();
    for (const auto& id : slotEngines)
        if (id.isNotEmpty())
            loadedEngineIds_.insert(id.toStdString());
    listComp_.repaint();
}

//------------------------------------------------------------------------------
inline void EnginePickerDrawer::timerCallback()
{
    const int64_t now     = juce::Time::currentTimeMillis();
    const float   elapsed = (float)(now - animStartMs_);
    const float   t       = juce::jlimit(0.0f, 1.0f, elapsed / kAnimDurationMs);

    if (animState_ == AnimState::Opening)
    {
        animProgress_ = t;
        applyAnimPosition();

        if (t >= 1.0f)
        {
            animState_    = AnimState::Open;
            animProgress_ = 1.0f;
            stopTimer();

            // Focus the search field once the drawer is fully open
            juce::Component::SafePointer<EnginePickerDrawer> safeThis(this);
            juce::Timer::callAfterDelay(40, [safeThis]
            {
                if (safeThis != nullptr)
                    safeThis->searchField_.grabKeyboardFocus();
            });
        }
    }
    else if (animState_ == AnimState::Closing)
    {
        animProgress_ = 1.0f - t;
        applyAnimPosition();

        if (t >= 1.0f)
        {
            animState_    = AnimState::Closed;
            animProgress_ = 0.0f;
            stopTimer();
            setVisible(false);
        }
    }
}

//------------------------------------------------------------------------------
inline void EnginePickerDrawer::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds();

    // ── Main background ───────────────────────────────────────────────────────
    g.setColour(colDrawerBg());
    g.fillRect(bounds);

    // ── Right-edge shadow/border ──────────────────────────────────────────────
    g.setColour(colTeal().withAlpha(0.10f));
    g.drawVerticalLine(bounds.getRight() - 1, 0.0f, (float)bounds.getHeight());
    g.setColour(colBorder());
    g.drawVerticalLine(bounds.getRight() - 2, 0.0f, (float)bounds.getHeight());

    // ── Title bar ─────────────────────────────────────────────────────────────
    {
        const auto titleR = bounds.withHeight(kHeaderH).toFloat();

        // Subtle teal wash
        juce::ColourGradient titleGrad(colTeal().withAlpha(0.08f), 0.0f, 0.0f,
                                       juce::Colour(0x00000000), titleR.getWidth(), 0.0f, false);
        g.setGradientFill(titleGrad);
        g.fillRect(titleR);

        // Title text
        g.setFont(GalleryFonts::display(15.0f));
        g.setColour(colTeal());
        g.drawText("Engine Library",
                   bounds.withHeight(kHeaderH).withTrimmedLeft(16).withTrimmedRight(48),
                   juce::Justification::centredLeft, false);

        // Close button (×)
        g.setFont(GalleryFonts::body(17.0f));
        g.setColour(closeBtnHovered_ ? juce::Colour(0xFFFFFFFF) : colCloseBtn());
        g.drawText(juce::String::fromUTF8("\xC3\x97"),  // U+00D7 MULTIPLICATION SIGN
                   closeBtnBounds_, juce::Justification::centred, false);

        // Bottom separator
        g.setColour(colBorder());
        g.drawHorizontalLine(kHeaderH - 1, 0.0f, (float)bounds.getWidth());
    }

    // ── Bottom fade gradient ──────────────────────────────────────────────────
    {
        const int fadeH = 52;
        const int fadeY = bounds.getHeight() - fadeH;
        juce::ColourGradient fadeGrad(juce::Colour(0x00000000),       0.0f, (float)fadeY,
                                      colDrawerBg().withAlpha(0.94f), 0.0f, (float)bounds.getHeight(),
                                      false);
        g.setGradientFill(fadeGrad);
        g.fillRect(0, fadeY, bounds.getWidth(), fadeH);
    }
}

//------------------------------------------------------------------------------
inline void EnginePickerDrawer::resized()
{
    auto b = getLocalBounds();

    // ── Title bar ─────────────────────────────────────────────────────────────
    auto titleBar = b.removeFromTop(kHeaderH);
    // Close button: 36×36 centred in the right 44px
    closeBtnBounds_ = titleBar.removeFromRight(44).withSizeKeepingCentre(36, 36);

    // ── Search field ─────────────────────────────────────────────────────────
    b.removeFromTop(8);
    searchField_.setBounds(b.removeFromTop(kSearchH).reduced(12, 0));

    // ── Category pills ────────────────────────────────────────────────────────
    b.removeFromTop(6);
    auto pillBar = b.removeFromTop(kPillBarH).reduced(8, 0);
    const int pillW = pillBar.getWidth() / kNumCats;
    for (int i = 0; i < kNumCats; ++i)
    {
        if (i < kNumCats - 1)
            catBtns_[i].setBounds(pillBar.removeFromLeft(pillW));
        else
            catBtns_[i].setBounds(pillBar); // last pill gets remaining width
    }

    // ── List viewport (fills remaining height) ────────────────────────────────
    b.removeFromTop(4);
    listViewport_.setBounds(b);

    // Sync list component width to viewport content area
    const int totalH = juce::jmax((int)filteredEngines_.size() * kRowHeight, 1);
    listComp_.setSize(listViewport_.getMaximumVisibleWidth(), totalH);
}

//------------------------------------------------------------------------------
inline void EnginePickerDrawer::mouseMove(const juce::MouseEvent& e)
{
    const bool nowHovered = closeBtnBounds_.contains(e.getPosition());
    if (nowHovered != closeBtnHovered_)
    {
        closeBtnHovered_ = nowHovered;
        repaint(closeBtnBounds_);
    }
}

inline void EnginePickerDrawer::mouseDown(const juce::MouseEvent& e)
{
    if (closeBtnBounds_.contains(e.getPosition()))
        close();
}

inline void EnginePickerDrawer::mouseExit(const juce::MouseEvent& /*e*/)
{
    if (closeBtnHovered_)
    {
        closeBtnHovered_ = false;
        repaint(closeBtnBounds_);
    }
}

//------------------------------------------------------------------------------
// Private helpers
//------------------------------------------------------------------------------

inline void EnginePickerDrawer::buildMetadataLookup()
{
    metaLookup_.clear();
    const auto* table = engineMetadataTable();
    for (int i = 0; table[i].id != nullptr; ++i)
    {
        std::string key = juce::String(table[i].id).toLowerCase().toStdString();
        // First entry wins — intentional; no duplicate IDs in the table
        if (metaLookup_.find(key) == metaLookup_.end())
            metaLookup_[key] = i;
    }
}

inline void EnginePickerDrawer::populateEngineList()
{
    allEngines_.clear();
    const auto* table = engineMetadataTable();
    for (int i = 0; table[i].id != nullptr; ++i)
    {
        EngineRow row;
        row.id        = table[i].id;
        row.category  = table[i].category;
        row.archetype = table[i].archetype;
        row.accent    = juce::Colour(table[i].accentARGB);
        row.depthZone = table[i].depthZone;
        allEngines_.push_back(std::move(row));
    }

    // Alphabetical sort for consistent display
    std::sort(allEngines_.begin(), allEngines_.end(),
              [](const EngineRow& a, const EngineRow& b)
              { return a.id.compareIgnoreCase(b.id) < 0; });
}

inline void EnginePickerDrawer::updateFilter()
{
    filteredEngines_.clear();

    const auto   query  = searchField_.getText().trim().toLowerCase();
    const auto   catStr = juce::String(kFilterCategory(activeCatIdx_));
    const bool   allCat = (activeCatIdx_ == 0); // ALL — skip category check

    for (const auto& row : allEngines_)
    {
        // ── Category filter ───────────────────────────────────────────────────
        if (!allCat && row.category != catStr)
            continue;

        // ── Search filter ─────────────────────────────────────────────────────
        if (query.isNotEmpty())
        {
            const bool nameHit  = row.id.toLowerCase().contains(query);
            const bool archHit  = row.archetype.toLowerCase().contains(query);
            const bool catHit   = row.category.toLowerCase().contains(query);
            if (!nameHit && !archHit && !catHit)
                continue;
        }

        filteredEngines_.push_back(row);
    }

    // ── Sync list component size ──────────────────────────────────────────────
    const int totalH = juce::jmax((int)filteredEngines_.size() * kRowHeight, 1);
    listComp_.setSize(juce::jmax(listViewport_.getMaximumVisibleWidth(), 1), totalH);
    listComp_.repaint();

    // ── Reset scroll and selection ───────────────────────────────────────────
    listViewport_.setViewPosition(0, 0);
    hoveredRow_  = -1;
    selectedRow_ = filteredEngines_.empty() ? -1 : 0;
}

inline void EnginePickerDrawer::applyAnimPosition()
{
    if (getParentComponent() == nullptr)
        return;

    // Fully open = x at 0 (flush left of parent).
    // Fully closed = x at -kDrawerWidth (completely off-screen left).
    const float t = juce::jlimit(0.0f, 1.0f, animProgress_);

    // Ease-out curve on open: pow(t, 0.4).  Linear on close.
    const float easedT = (animState_ == AnimState::Opening || animState_ == AnimState::Open)
                             ? std::pow(t, 0.4f)
                             : t;

    const int hiddenX = -kDrawerWidth;
    const int openX   = 0;
    const int newX    = juce::roundToInt((float)hiddenX + easedT * (float)(openX - hiddenX));
    setTopLeftPosition(newX, getY());
}

inline void EnginePickerDrawer::styleAllPillButtons()
{
    for (int i = 0; i < kNumCats; ++i)
        catBtns_[i].setToggleState(i == activeCatIdx_, juce::dontSendNotification);
}

} // namespace xoceanus
