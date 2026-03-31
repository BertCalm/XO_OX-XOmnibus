#pragma once
// EnginePickerPopup.h — Searchable engine selection component for XOlokun.
//
// Replaces the flat PopupMenu engine list with:
//   - Text search across engine name, category, and archetype description
//   - Category filter pills (ALL | SYNTH | PERC | BASS | PAD | STRING | ORGAN | VOCAL | FX | UTILITY)
//   - Results grouped by water-column depth zone (Sunlit / Twilight / Midnight)
//   - 8px accent color dot + engine name + category badge per row
//
// Designed to be shown in a juce::CallOutBox:
//   auto* popup = new xolokun::EnginePickerPopup();
//   popup->onEngineSelected = [this](const juce::String& id) { ... };
//   juce::CallOutBox::launchAsynchronously(std::unique_ptr<Component>(popup),
//                                          targetBounds, parentComp);
//
// Size: 340 x 420pt.

#include <juce_audio_processors/juce_audio_processors.h>
#include "../../Core/EngineRegistry.h"
#include "../GalleryColors.h"

namespace xolokun {

//==============================================================================
// Lightweight LookAndFeel override so category pill buttons render with a
// compact 8pt label font without requiring a full LookAndFeel subclass elsewhere.
struct PillButtonLookAndFeel : public juce::LookAndFeel_V4
{
    void drawButtonText(juce::Graphics& g, juce::TextButton& btn,
                        bool /*isMouseOver*/, bool /*isButtonDown*/) override
    {
        g.setFont(GalleryFonts::label(7.5f));
        g.setColour(btn.findColour(btn.getToggleState()
                    ? juce::TextButton::textColourOnId
                    : juce::TextButton::textColourOffId));
        g.drawFittedText(btn.getButtonText(),
                         btn.getLocalBounds().reduced(1, 0),
                         juce::Justification::centred, 1, 0.9f);
    }
};

//==============================================================================
// EnginePickerPopup — header-only implementation
class EnginePickerPopup : public juce::Component,
                          public juce::ListBoxModel,
                          public juce::TextEditor::Listener,
                          public juce::KeyListener
{
public:
    //==========================================================================
    // Callback invoked when the user selects an engine. Caller is responsible
    // for dismissing the owning CallOutBox (or call dismissPopup()).
    std::function<void(const juce::String& engineId)> onEngineSelected;

    //==========================================================================
    EnginePickerPopup()
    {
        A11y::setup(*this, "Engine Picker",
                    "Search and select an engine by name, category, or description");

        buildMetadataTable();
        collectRegisteredEngines();

        // ── Search field ─────────────────────────────────────────────────────
        searchField.setTextToShowWhenEmpty(
            "Search " + juce::String(allEngineIds.size()) + " engines...",
            GalleryColors::get(GalleryColors::t3()).withAlpha(0.80f));
        searchField.setColour(juce::TextEditor::backgroundColourId,
            GalleryColors::get(GalleryColors::surface()));
        searchField.setColour(juce::TextEditor::outlineColourId,
            GalleryColors::borderMd());
        searchField.setColour(juce::TextEditor::focusedOutlineColourId,
            juce::Colour(GalleryColors::xoGold));
        searchField.setColour(juce::TextEditor::textColourId,
            GalleryColors::get(GalleryColors::t1()));
        searchField.setFont(GalleryFonts::body(11.0f));
        searchField.setReturnKeyStartsNewLine(false);
        searchField.addListener(this);
        searchField.addKeyListener(this);
        addAndMakeVisible(searchField);

        // ── Category pill buttons ─────────────────────────────────────────────
        // Short readable labels — font is 7.5pt to keep all pills fitting in 340pt.
        static const char* kCatLabels[] = {
            "ALL", "Synth", "Perc", "Bass", "Pad", "String", "Organ", "Vocal", "FX", "Util"
        };
        static const char* kCatTooltips[] = {
            "All engines",
            "Synthesizer",
            "Percussion",
            "Bass",
            "Pad",
            "String",
            "Organ",
            "Vocal",
            "FX",
            "Utility",
        };
        for (int i = 0; i < kNumCategories; ++i)
        {
            catBtns[i].setButtonText(kCatLabels[i]);
            catBtns[i].setTooltip(kCatTooltips[i]);
            catBtns[i].setClickingTogglesState(false);
            catBtns[i].setLookAndFeel(&pillLnF);
            stylePillButton(catBtns[i], i == 0);
            const int catIdx = i;
            catBtns[i].onClick = [this, catIdx]
            {
                activeCategory = catIdx;
                for (int j = 0; j < kNumCategories; ++j)
                    stylePillButton(catBtns[j], j == catIdx);
                updateFilter();
            };
            addAndMakeVisible(catBtns[i]);
        }

        // ── Results list ──────────────────────────────────────────────────────
        listBox.setModel(this);
        listBox.setRowHeight(kRowHeight);
        listBox.setColour(juce::ListBox::backgroundColourId,
            GalleryColors::get(GalleryColors::elevated()));
        listBox.setColour(juce::ListBox::outlineColourId,
            GalleryColors::border());
        listBox.setOutlineThickness(1);
        listBox.addKeyListener(this);
        addAndMakeVisible(listBox);

        // ── Empty state label (shown when search returns no results) ──────────
        emptyLabel.setText("No engines match your search", juce::dontSendNotification);
        emptyLabel.setFont(GalleryFonts::body(11.0f));
        emptyLabel.setColour(juce::Label::textColourId,
            GalleryColors::get(GalleryColors::t3()));
        emptyLabel.setJustificationType(juce::Justification::centred);
        addChildComponent(emptyLabel); // hidden by default

        // ── Count label ───────────────────────────────────────────────────────
        countLabel.setFont(GalleryFonts::label(8.5f));
        countLabel.setColour(juce::Label::textColourId,
            GalleryColors::get(GalleryColors::textMid()).withAlpha(0.55f));
        countLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(countLabel);

        updateFilter();
        setSize(340, 420);

        // Focus the search field immediately so the user can start typing.
        // SafePointer guards against the rare case where the CallOutBox dismisses
        // this component before the 50ms timer fires (e.g. host window close).
        juce::Component::SafePointer<EnginePickerPopup> safeThis(this);
        juce::Timer::callAfterDelay(50, [safeThis] {
            if (safeThis != nullptr)
                safeThis->searchField.grabKeyboardFocus();
        });
    }

    ~EnginePickerPopup() override
    {
        // Remove key listeners explicitly to prevent dangling raw pointer reads
        // if JUCE's component destruction order tries to broadcast a final key event.
        searchField.removeKeyListener(this);
        listBox.removeKeyListener(this);
        // Detach custom LookAndFeel before it destructs (JUCE requirement).
        for (auto& btn : catBtns)
            btn.setLookAndFeel(nullptr);
    }

    //==========================================================================
    // juce::ListBoxModel interface
    //==========================================================================
    int getNumRows() override { return (int)flatRows.size(); }

    void paintListBoxItem(int row, juce::Graphics& g,
                          int w, int h, bool isSelected) override
    {
        if (row < 0 || row >= (int)flatRows.size())
            return;

        const auto& fr = flatRows[static_cast<size_t>(row)];
        using namespace GalleryColors;

        // ── Section header rows ───────────────────────────────────────────────
        if (fr.isSectionHeader)
        {
            // Subtle tinted header background — raised above the list bg
            g.fillAll(get(raised()).withAlpha(darkMode() ? 1.0f : 0.6f));

            // Left accent stripe
            g.setColour(fr.headerColor.withAlpha(0.70f));
            g.fillRect(0, 0, 3, h);

            // Header label — uppercase, T3 tonal color with zone hue blended in
            g.setFont(GalleryFonts::label(8.0f));
            g.setColour(get(t3()).interpolatedWith(fr.headerColor, 0.35f));
            g.drawText(fr.headerLabel.toUpperCase(), 10, 0, w - 14, h,
                       juce::Justification::centredLeft, true);
            return;
        }

        // ── Engine rows ───────────────────────────────────────────────────────
        if (isSelected)
        {
            g.fillAll(juce::Colour(0x0BFFFFFF));
            g.setColour(juce::Colour(get(xoGold)));
            g.fillRect(0, 0, 2, h);
        }
        else if (row % 2 == 0)
            g.fillAll(get(elevated()));
        else
            g.fillAll(get(surface()));

        // Accent dot
        const float dotR = 4.0f;
        const float dotCy = h * 0.5f;
        const float dotCx = 10.0f + dotR;
        g.setColour(fr.accent);
        g.fillEllipse(dotCx - dotR, dotCy - dotR, dotR * 2.0f, dotR * 2.0f);

        // Engine name — top half of row
        const int nameY = 3;
        const int nameH = 16;
        g.setFont(GalleryFonts::display(11.0f));
        g.setColour(isSelected ? fr.accent : get(t1()));
        g.drawText(fr.engineId.toUpperCase(),
                   24, nameY, w - 74, nameH,
                   juce::Justification::centredLeft, true);

        // Archetype subtitle — below engine name, muted T3 color
        if (!fr.archetype.isEmpty())
        {
            g.setFont(GalleryFonts::body(8.0f));
            g.setColour(get(t3()).withAlpha(0.70f));
            g.drawText(fr.archetype,
                       24, nameY + nameH, w - 74, 11,
                       juce::Justification::centredLeft, true);
        }

        // Category badge — right side, T4 muted mono (vertically centred in name row)
        g.setFont(GalleryFonts::value(8.0f));
        g.setColour(get(t4()));
        g.drawText(fr.category,
                   w - 66, nameY, 62, nameH,
                   juce::Justification::centredRight, true);
    }

    void listBoxItemClicked(int row, const juce::MouseEvent&) override
    {
        commitSelection(row);
    }

    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override
    {
        commitSelection(row);
    }

    void selectedRowsChanged(int lastRowSelected) override
    {
        // Selection change via keyboard navigation does not commit — only
        // explicit click (listBoxItemClicked) or Enter/Return commits.
        (void)lastRowSelected;
    }

    //==========================================================================
    // juce::TextEditor::Listener
    //==========================================================================
    void textEditorTextChanged(juce::TextEditor&) override { updateFilter(); }
    void textEditorReturnKeyPressed(juce::TextEditor&) override
    {
        commitSelection(listBox.getSelectedRow());
    }
    void textEditorEscapeKeyPressed(juce::TextEditor&) override
    {
        dismissPopup();
    }

    //==========================================================================
    // juce::KeyListener — arrow keys in search field navigate list
    //==========================================================================
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            dismissPopup();
            return true;
        }

        if (key == juce::KeyPress::returnKey)
        {
            commitSelection(listBox.getSelectedRow());
            return true;
        }

        const int numRows = (int)flatRows.size();
        if (numRows == 0)
            return false;

        if (key == juce::KeyPress::downKey)
        {
            int cur = listBox.getSelectedRow();
            int next = (cur < 0) ? 0 : juce::jmin(cur + 1, numRows - 1);
            // Skip section headers
            while (next < numRows && flatRows[static_cast<size_t>(next)].isSectionHeader)
                ++next;
            if (next < numRows)
            {
                listBox.selectRow(next);
                listBox.scrollToEnsureRowIsOnscreen(next);
            }
            return true;
        }

        if (key == juce::KeyPress::upKey)
        {
            int cur = listBox.getSelectedRow();
            int prev = (cur <= 0) ? 0 : cur - 1;
            // Skip section headers
            while (prev > 0 && flatRows[static_cast<size_t>(prev)].isSectionHeader)
                --prev;
            if (!flatRows[static_cast<size_t>(prev)].isSectionHeader)
            {
                listBox.selectRow(prev);
                listBox.scrollToEnsureRowIsOnscreen(prev);
            }
            return true;
        }

        (void)originatingComponent;
        return false;
    }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        // Use elevated surface so the popup reads as a raised card in both themes.
        g.fillAll(get(elevated()));
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(8, 6);

        // Search row (32pt): search field + count label right-anchored
        auto searchRow = b.removeFromTop(32);
        countLabel.setBounds(searchRow.removeFromRight(52));
        searchField.setBounds(searchRow.reduced(0, 2));

        b.removeFromTop(4);

        // Category pill row (28pt)
        auto pillRow = b.removeFromTop(28);
        const int totalW = pillRow.getWidth();
        // Distribute pills evenly — "ALL" is slightly wider
        const int pillW = totalW / kNumCategories;
        for (int i = 0; i < kNumCategories; ++i)
            catBtns[i].setBounds(pillRow.removeFromLeft(
                (i < kNumCategories - 1) ? pillW
                                          : pillRow.getWidth()) // last pill gets remainder
                .reduced(1, 0));

        b.removeFromTop(4);

        listBox.setBounds(b);
        emptyLabel.setBounds(listBox.getBounds());
    }

private:
    //==========================================================================
    // Internal flat row representation (engine rows + section headers)
    struct FlatRow
    {
        bool       isSectionHeader = false;

        // For engine rows
        juce::String engineId;
        juce::String category;
        juce::String archetype;
        juce::Colour accent;
        int          depthZone = 0; // 0=Sunlit 1=Twilight 2=Midnight

        // For section headers
        juce::String headerLabel;
        juce::Colour headerColor;
    };

    //==========================================================================
    // Static engine metadata table
    struct EngineInfo
    {
        const char* id;
        const char* category;   // "Synth" | "Percussion" | "Bass" | "Pad" | "String" | "Organ" | "Vocal" | "FX" | "Utility"
        const char* archetype;  // one-line description shown in search
        uint32_t    accentARGB;
        int         depthZone;  // 0=Sunlit 1=Twilight 2=Midnight
    };

    static const EngineInfo* engineMetadataTable()
    {
        // clang-format off
        static const EngineInfo kTable[] =
        {
            // Kitchen Collection — Organs (Chef Quad)
            { "Oto",        "Organ",       "tonewheel drawbar organ",                   0xFFF5F0E8, 0 },
            { "Octave",     "Organ",       "Hammond tonewheel simulation",              0xFF8B6914, 0 },
            { "Oleg",       "Organ",       "theatre pipe organ",                        0xFFC0392B, 0 },
            { "Otis",       "Organ",       "gospel soul organ drive",                   0xFFD4A017, 0 },
            // Kitchen Collection — Pianos (Kitchen Quad)
            { "Oven",       "String",      "Steinway concert grand piano",              0xFF1C1C1C, 1 },
            { "Ochre",      "String",      "wooden resonator piano",                    0xFFCC7722, 1 },
            { "Obelisk",    "String",      "grand piano sympathetic resonance",         0xFFFFFFE0, 0 },
            { "Opaline",    "String",      "prepared piano rust and objects",           0xFFB7410E, 1 },
            // Kitchen Collection — Bass (Cellar Quad)
            { "Ogre",       "Bass",        "sub bass synthesizer",                      0xFF0D0D0D, 2 },
            { "Olate",      "Bass",        "fretless bass guitar",                      0xFF5C3317, 1 },
            { "Oaken",      "Bass",        "upright double bass",                       0xFF9C6B30, 1 },
            { "Omega",      "Bass",        "analog synth bass",                         0xFF003366, 2 },
            // Kitchen Collection — Strings (Garden Quad)
            { "Orchard",    "String",      "orchestral strings bow pressure",           0xFFFFB7C5, 0 },
            { "Overgrow",   "String",      "overgrown string textures",                 0xFF228B22, 1 },
            { "Osier",      "String",      "willow wind strings",                       0xFFC0C8C8, 0 },
            { "Oxalis",     "String",      "wood sorrel lilac strings",                 0xFF9B59B6, 1 },
            // Kitchen Collection — Pads (Broth Quad)
            { "Overwash",   "Pad",         "tide foam diffusion pad",                   0xFFF0F8FF, 0 },
            { "Overworn",   "Pad",         "worn felt texture pad",                     0xFF808080, 1 },
            { "Overflow",   "Pad",         "deep current flowing pad",                  0xFF1A3A5C, 2 },
            { "Overcast",   "Pad",         "cloud diffusion pad",                       0xFF778899, 1 },
            // Kitchen Collection — EPs (Fusion Quad)
            { "Oasis",      "Synth",       "desert spring electric piano",              0xFF00827F, 0 },
            { "Oddfellow",  "Synth",       "spectral fingerprint cache EP",             0xFFB87333, 1 },
            { "Onkolo",     "Synth",       "spectral amber resonant EP",                0xFFFFBF00, 1 },
            { "Opcode",     "Synth",       "dark turquoise code-driven EP",             0xFF5F9EA0, 1 },
            // Flagship + core synths
            { "Obrix",      "Synth",       "modular brick reef synthesizer",            0xFF1E8B7E, 2 },
            { "Oxytocin",   "Synth",       "circuit love triangle synthesizer",         0xFF9B5DE5, 2 },
            { "Overbite",   "Synth",       "apex predator modal synthesizer",           0xFFF0EDE8, 2 },
            { "Overworld",  "Synth",       "ERA triangle timbral crossfade",            0xFF39FF14, 0 },
            { "Ouroboros",  "Synth",       "strange attractor chaotic synthesizer",     0xFFFF2D2D, 2 },
            { "Oracle",     "Synth",       "GENDY stochastic maqam synthesis",          0xFF4B0082, 2 },
            { "Orbital",    "Synth",       "group envelope synthesizer",                0xFFFF6B6B, 1 },
            { "Opal",       "Synth",       "granular cloud synthesizer",                0xFFA78BFA, 1 },
            { "Obsidian",   "Synth",       "crystal resonant synthesizer",              0xFFE8E0D8, 2 },
            { "Origami",    "Synth",       "fold-point waveshaping synthesizer",        0xFFE63946, 1 },
            { "Obscura",    "Synth",       "daguerreotype physical modeling",           0xFF8A9BA8, 1 },
            { "Oblique",    "Synth",       "prismatic bounce synth",                    0xFFBF40FF, 1 },
            { "Organism",   "Synth",       "cellular automata generative synth",        0xFFC6E377, 1 },
            { "Orbweave",   "Synth",       "topological knot coupling engine",          0xFF8E4585, 2 },
            { "Overtone",   "Synth",       "continued fraction spectral synth",         0xFFA8D8EA, 1 },
            { "Oxbow",      "Synth",       "entangled reverb synthesizer",              0xFF1A6B5A, 2 },
            { "Outlook",    "Synth",       "panoramic dual wavetable synth",            0xFF4169E1, 1 },
            { "Overlap",    "Synth",       "knot matrix FDN synthesizer",              0xFF00FFB4, 2 },
            { "Orca",       "Synth",       "apex predator wavetable echolocation",      0xFF1B2838, 2 },
            { "Octopus",    "Synth",       "decentralized alien intelligence synth",    0xFFE040FB, 2 },
            { "Ombre",      "Synth",       "dual narrative memory synthesizer",         0xFF7B6B8A, 1 },
            { "Opensky",    "Synth",       "euphoric shimmer supersaw synth",           0xFFFF8C00, 0 },
            // Percussion
            { "Onset",      "Percussion",  "cross-voice coupling percussion",           0xFF0066FF, 1 },
            { "Offering",   "Percussion",  "psychology-driven boom bap drums",          0xFFE5B80B, 1 },
            { "Oware",      "Percussion",  "Akan tuned mallet percussion",              0xFFB5883E, 1 },
            { "Ostinato",   "Percussion",  "modal membrane world rhythm engine",        0xFFE8701A, 1 },
            // Vocal
            { "Opera",      "Vocal",       "additive-vocal Kuramoto synchrony",         0xFFD4AF37, 1 },
            { "Obbligato",  "Vocal",       "breath articulation vocal synth",           0xFFFF8A7A, 1 },
            // Bass synths
            { "Oblong",     "Bass",        "resonant bass synthesizer",                 0xFFE9A84A, 1 },
            { "Obese",      "Bass",        "fat saturation bass synth",                 0xFFFF1493, 1 },
            // Organ & wind
            { "Organon",    "Organ",       "variational metabolism organ synth",        0xFF00CED1, 1 },
            { "Ohm",        "Organ",       "sage analog organ synthesizer",             0xFF87AE73, 0 },
            { "Ottoni",     "Organ",       "patina brass organ synthesizer",            0xFF5B8A72, 1 },
            { "Ole",        "Organ",       "hibiscus flamenco organ synth",             0xFFC9377A, 1 },
            // String / physical modeling
            { "Orphica",    "String",      "siren seafoam plucked string",              0xFF7FDBCA, 1 },
            { "Osprey",     "String",      "shore coastline cultural synthesis",        0xFF1B4F8A, 1 },
            { "Osteria",    "String",      "porto wine shore string synth",             0xFF722F37, 1 },
            { "Owlfish",    "String",      "Mixtur-Trautonium string modeling",         0xFFB8860B, 2 },
            // Character
            { "OddfeliX",   "Synth",       "neon tetra character synth",               0xFF00A6D6, 0 },
            { "OddOscar",   "Synth",       "axolotl character synth",                  0xFFE8839B, 0 },
            { "Odyssey",    "Synth",       "drift analog poly synthesizer",             0xFF7B2D8B, 1 },
            { "Overdub",    "Synth",       "spring reverb dub synthesizer",             0xFF6B7B3A, 1 },
            { "Oceanic",    "Synth",       "chromatophore phosphorescent synth",        0xFF00B4A0, 1 },
            { "Ocelot",     "Synth",       "biome crossfade ocelot synth",              0xFFC5832B, 1 },
            { "Osmosis",    "Synth",       "external audio membrane synth",             0xFFC0C0C0, 1 },
            // Utility
            { "Optic",      "Utility",     "visual modulation zero-audio engine",       0xFF00FF41, 0 },
            { "Outwit",     "FX",          "chromatophore amber effect engine",         0xFFCC6600, 1 },
            // Additional engines (alphabetically filled)
            { "Oceandeep",  "Synth",       "hydrostatic deep ocean synthesizer",        0xFF2D0A4E, 2 },
            { "Ouie",       "Synth",       "duophonic hammerhead synthesizer",          0xFF708090, 2 },
            { "Obiont",     "Synth",       "cellular automata oscillator",              0xFFE8A030, 2 },
            { "Okeanos",    "String",      "Spice Route Rhodes electric piano",         0xFFC49B3F, 1 },
            { "Outflow",    "Synth",       "predictive spatial fluid-dynamics engine",  0xFF1A1A40, 2 },
            // Sentinel — must remain last
            { nullptr, nullptr, nullptr, 0, 0 },
        };
        // clang-format on
        return kTable;
    }

    //==========================================================================
    // Build a lookup map from lowercased engine ID → metadata index.
    // Uses the sentinel entry (id == nullptr) to terminate — no hardcoded count.
    void buildMetadataTable()
    {
        const auto* table = engineMetadataTable();
        for (int i = 0; table[i].id != nullptr; ++i)
        {
            juce::String key = juce::String(table[i].id).toLowerCase();
            // First entry wins — duplicate IDs in the table are intentionally ignored.
            if (metaLookup.find(key.toStdString()) == metaLookup.end())
                metaLookup[key.toStdString()] = i;
        }
    }

    // Returns a pointer to the EngineInfo for an engine ID (case-insensitive).
    // Returns nullptr if not in the static table.
    const EngineInfo* metaFor(const juce::String& engineId) const
    {
        auto it = metaLookup.find(engineId.toLowerCase().toStdString());
        if (it != metaLookup.end())
            return engineMetadataTable() + it->second;
        return nullptr;
    }

    juce::String categoryOf(const juce::String& engineId) const
    {
        if (auto* m = metaFor(engineId)) return m->category;
        return "Synth";
    }

    juce::String archetypeOf(const juce::String& engineId) const
    {
        if (auto* m = metaFor(engineId)) return m->archetype;
        return "";
    }

    int depthZoneOf(const juce::String& engineId) const
    {
        if (auto* m = metaFor(engineId)) return m->depthZone;
        return 1; // default Twilight
    }

    //==========================================================================
    // Collect engine IDs from the live registry
    void collectRegisteredEngines()
    {
        allEngineIds.clear();
        const auto& reg = EngineRegistry::instance();
        for (const auto& stdId : reg.getRegisteredIds())
            allEngineIds.add(juce::String(stdId.c_str()));

        // Sort alphabetically for consistent initial display
        allEngineIds.sort(true);
    }

    //==========================================================================
    // Search predicate
    bool matchesQuery(const juce::String& engineId,
                      const juce::String& category,
                      const juce::String& archetype,
                      const juce::String& query) const
    {
        if (query.isEmpty()) return true;
        auto q = query.toLowerCase();
        if (engineId.toLowerCase().contains(q)) return true;
        if (category.toLowerCase().contains(q))  return true;
        if (archetype.toLowerCase().contains(q)) return true;
        return false;
    }

    //==========================================================================
    // Category filter — index matches catBtns[] order:
    //   0=ALL  1=SYNTH  2=PERC  3=BASS  4=PAD  5=STRING  6=ORGAN  7=VOCAL  8=FX  9=UTILITY
    bool matchesCategory(const juce::String& category, int catFilter) const
    {
        if (catFilter == 0) return true; // ALL
        static const char* kFilterToCategory[] = {
            "",          // 0 ALL
            "Synth",     // 1
            "Percussion",// 2
            "Bass",      // 3
            "Pad",       // 4
            "String",    // 5
            "Organ",     // 6
            "Vocal",     // 7
            "FX",        // 8
            "Utility",   // 9
        };
        return category == kFilterToCategory[catFilter];
    }

    //==========================================================================
    // Rebuild flatRows from current filter state and repopulate listBox
    void updateFilter()
    {
        flatRows.clear();

        auto query = searchField.getText().trim();

        // Depth zone colors
        static const juce::Colour kZoneColors[3] = {
            juce::Colour(0xFF48CAE4), // Sunlit  — cyan
            juce::Colour(0xFF0096C7), // Twilight — blue
            juce::Colour(0xFF7B2FBE), // Midnight — violet
        };
        static const char* kZoneLabels[3] = {
            "Sunlit Zone",
            "Twilight Zone",
            "Midnight Zone",
        };

        // Collect matching engines into per-zone buckets
        juce::StringArray zones[3];

        for (const auto& id : allEngineIds)
        {
            auto cat  = categoryOf(id);
            auto arch = archetypeOf(id);

            if (!matchesQuery(id, cat, arch, query))      continue;
            if (!matchesCategory(cat, activeCategory))    continue;

            int zone = depthZoneOf(id);
            zone = juce::jlimit(0, 2, zone);
            zones[zone].add(id);
        }

        // Build flat list: header + engines for each non-empty zone
        int totalEngines = 0;
        for (int z = 0; z < 3; ++z)
        {
            if (zones[z].isEmpty()) continue;

            // Section header row
            FlatRow hdr;
            hdr.isSectionHeader = true;
            hdr.headerLabel     = kZoneLabels[z];
            hdr.headerColor     = kZoneColors[z];
            flatRows.push_back(std::move(hdr));

            // Engine rows within this zone
            for (const auto& id : zones[z])
            {
                FlatRow er;
                er.isSectionHeader = false;
                er.engineId        = id;
                er.category        = categoryOf(id);
                er.archetype       = archetypeOf(id);
                er.accent          = GalleryColors::accentForEngine(id);
                er.depthZone       = z;
                flatRows.push_back(std::move(er));
                ++totalEngines;
            }
        }

        listBox.updateContent();
        listBox.deselectAllRows();

        // Auto-select first engine row so arrow-down works immediately
        for (int i = 0; i < (int)flatRows.size(); ++i)
        {
            if (!flatRows[static_cast<size_t>(i)].isSectionHeader)
            {
                listBox.selectRow(i);
                break;
            }
        }

        // Show empty state label when no engines match; hide list to avoid blank area
        const bool isEmpty = (totalEngines == 0);
        emptyLabel.setVisible(isEmpty);
        listBox.setVisible(!isEmpty);

        countLabel.setText(juce::String(totalEngines) + " engines",
                           juce::dontSendNotification);
    }

    //==========================================================================
    // Commit engine selection and fire callback
    void commitSelection(int row)
    {
        if (row < 0 || row >= (int)flatRows.size())
            return;

        const auto& fr = flatRows[static_cast<size_t>(row)];
        if (fr.isSectionHeader)
            return;

        if (onEngineSelected)
            onEngineSelected(fr.engineId);

        dismissPopup();
    }

    void dismissPopup()
    {
        if (auto* callOut = findParentComponentOfClass<juce::CallOutBox>())
            callOut->dismiss();
    }

    //==========================================================================
    // Pill button styling helper
    void stylePillButton(juce::TextButton& btn, bool active) const
    {
        using namespace GalleryColors;
        btn.setColour(juce::TextButton::buttonColourId,
                      active ? get(xoGold).withAlpha(0.14f)
                              : juce::Colour(0x00000000));
        btn.setColour(juce::TextButton::textColourOffId,
                      active ? get(xoGoldText())
                              : get(t3()));
        btn.setColour(juce::TextButton::textColourOnId,
                      get(xoGoldText()));
        btn.setColour(juce::TextButton::buttonOnColourId,
                      get(xoGold).withAlpha(0.14f));
        btn.setToggleState(active, juce::dontSendNotification);
    }

    //==========================================================================
    static constexpr int kNumCategories = 10;
    // Section headers share the same row height as engine rows — both rendered
    // inside paintListBoxItem, distinguished by FlatRow::isSectionHeader.
    static constexpr int kRowHeight = 34;

    // Engine ID list from registry
    juce::StringArray allEngineIds;

    // Metadata lookup: lowercase engine id → index in kTable
    std::unordered_map<std::string, int> metaLookup;

    // Flat display rows (headers interleaved with engine rows)
    std::vector<FlatRow> flatRows;

    // LookAndFeel for compact pill buttons (must outlive catBtns[])
    PillButtonLookAndFeel pillLnF;

    // UI
    juce::TextEditor  searchField;
    juce::TextButton  catBtns[kNumCategories];
    juce::ListBox     listBox;
    juce::Label       countLabel;
    juce::Label       emptyLabel;

    int activeCategory = 0; // 0 = ALL

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnginePickerPopup)
};

} // namespace xolokun
