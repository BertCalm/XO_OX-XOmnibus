#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../XOlokunProcessor.h"
#include "../Core/EngineRegistry.h"
#include "EngineVocabulary.h"
// GalleryColors.h provides GalleryColors, GalleryFonts, A11y — must come before
// CouplingVisualizer.h and ExportDialog.h which depend on those namespaces.
#include "GalleryColors.h"
#include "CouplingVisualizer/CouplingVisualizer.h"
#include "ExportDialog/ExportDialog.h"
// PlaySurface.h has a minimal GalleryColors::get() stub guarded by this macro.
// Define before inclusion so the stub is skipped (full def is already above).
#define XOLOKUN_GALLERY_COLORS_DEFINED 1
#include "PlaySurface/PlaySurface.h"
// PresetBrowser.h opens its own namespace xolokun { } block — must be included
// at file scope (before our namespace xolokun { below) to avoid nesting.
#include "PresetBrowser/PresetBrowser.h"

namespace xolokun {

// GalleryColors, GalleryFonts, and A11y are provided by GalleryColors.h
// (included above).  Their definitions are canonical in that file; they are
// available here via the xolokun:: namespace that GalleryColors.h opens.

//==============================================================================
// GalleryLookAndFeel — Gallery Model visual language (WCAG 2.1 AA compliant)
class GalleryLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GalleryLookAndFeel() { applyTheme(); }

    // Re-apply all theme colors — call after toggling GalleryColors::darkMode().
    void applyTheme()
    {
        using namespace GalleryColors;
        setColour(juce::Slider::rotarySliderFillColourId,        get(xoGold));
        setColour(juce::Slider::rotarySliderOutlineColourId,     get(borderGray()));
        setColour(juce::Slider::textBoxTextColourId,             get(textMid()));
        setColour(juce::Slider::textBoxBackgroundColourId,       juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxOutlineColourId,          juce::Colours::transparentBlack);
        setColour(juce::TextButton::buttonColourId,              get(shellWhite()));
        setColour(juce::TextButton::buttonOnColourId,            get(xoGold));
        setColour(juce::TextButton::textColourOffId,             get(textDark()));
        setColour(juce::ComboBox::backgroundColourId,            get(shellWhite()));
        setColour(juce::ComboBox::outlineColourId,               get(borderGray()));
        setColour(juce::PopupMenu::backgroundColourId,           get(shellWhite()));
        setColour(juce::PopupMenu::textColourId,                 get(textDark()));
        setColour(juce::PopupMenu::highlightedBackgroundColourId,get(xoGold).withAlpha(0.25f));
        setColour(juce::ScrollBar::thumbColourId,                get(borderGray()));
        setColour(juce::Label::textColourId,                     get(textDark()));
    }

    // Ecological Instrument paradigm — zone arcs + setpoint marker + value arc.
    // Visual language mirrors the web PlaySurface:
    //   Track ring   : faint full-sweep baseline
    //   Zone arcs    : Sunlit/Twilight/Midnight depth bands at 0.22 opacity
    //   Value arc    : engine accent color, shows current parameter value
    //   Setpoint ▲   : XO Gold triangle pointing outward at current position
    //   Center disc  : warm white inner field (Gallery Model shell)
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(6.0f);
        float r     = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        float cx    = bounds.getCentreX();
        float cy    = bounds.getCentreY();
        float sweep = endAngle - startAngle;
        float fillAngle = startAngle + sliderPos * sweep;
        float arcR  = r - 2.0f;

        // Focus ring (WCAG 2.4.7)
        if (slider.hasKeyboardFocus(true))
            A11y::drawCircularFocusRing(g, cx, cy, r + 2.0f);

        // ── 1. Ecological depth zone arcs ────────────────────────────────
        // Wider than the track so they create a soft halo behind it.
        // Sunlit 0–55% | Twilight 55–80% | Midnight 80–100% of sweep.
        struct ZoneBand { float normStart, normEnd; uint32_t rgba; };
        constexpr ZoneBand kZones[3] = {
            { 0.00f, 0.55f, 0xFF48CAE4 },   // Sunlit  — tropical cyan
            { 0.55f, 0.80f, 0xFF0096C7 },   // Twilight — deep blue
            { 0.80f, 1.00f, 0xFF7B2FBE }    // Midnight — bioluminescent violet
        };
        for (const auto& z : kZones)
        {
            float a0 = startAngle + z.normStart * sweep;
            float a1 = startAngle + z.normEnd   * sweep;
            juce::Path zp;
            zp.addCentredArc(cx, cy, arcR, arcR, 0.0f, a0, a1, true);
            g.setColour(juce::Colour(z.rgba).withAlpha(0.22f));
            g.strokePath(zp, juce::PathStrokeType(4.5f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        }

        // ── 2. Track ring — full-sweep baseline ──────────────────────────
        {
            juce::Path track;
            track.addCentredArc(cx, cy, arcR, arcR, 0.0f, startAngle, endAngle, true);
            g.setColour(GalleryColors::get(GalleryColors::borderGray()).withAlpha(0.40f));
            g.strokePath(track, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        }

        // ── 3. Value arc — engine accent color ───────────────────────────
        if (sliderPos > 0.001f)
        {
            juce::Path fill;
            fill.addCentredArc(cx, cy, arcR, arcR, 0.0f, startAngle, fillAngle, true);
            auto accent = slider.findColour(juce::Slider::rotarySliderFillColourId);
            g.setColour(accent);
            g.strokePath(fill, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        }

        // ── 4. Center disc — Gallery Model warm white inner field ────────
        {
            float discR = r * 0.44f;
            g.setColour(GalleryColors::get(GalleryColors::shellWhite()));
            g.fillEllipse(cx - discR, cy - discR, discR * 2.0f, discR * 2.0f);
            g.setColour(GalleryColors::get(GalleryColors::borderGray()).withAlpha(0.35f));
            g.drawEllipse(cx - discR, cy - discR, discR * 2.0f, discR * 2.0f, 1.0f);

            // Live value readout during interaction — center disc becomes the face
            if (slider.isMouseButtonDown() || slider.isMouseOverOrDragging())
            {
                juce::String valStr;
                double val = slider.getValue();
                // Integers for whole-number ranges (e.g. semitones, MIDI CCs);
                // one decimal place for continuous parameters.
                if (slider.getInterval() >= 1.0 && slider.getMaximum() <= 127.0)
                    valStr = juce::String((int)val);
                else
                    valStr = juce::String(val, 1);

                float fontSize = juce::jmax(7.0f, discR * 0.7f);
                g.setFont(GalleryFonts::value(fontSize));
                g.setColour(GalleryColors::get(GalleryColors::textDark()).withAlpha(0.85f));
                g.drawText(valStr,
                           juce::Rectangle<float>(cx - discR, cy - discR, discR * 2.0f, discR * 2.0f),
                           juce::Justification::centred);
            }
        }

        // ── 5. Setpoint triangle ▲ — XO Gold marker at player's intent ──
        // Tip at the arc edge, base pointing inward. Screen coords: Y increases downward,
        // so point at angle θ = (cx + arcR·sin θ, cy − arcR·cos θ).
        {
            float tipR   = arcR + 1.5f;
            float tipX   = cx + tipR * std::sin(fillAngle);
            float tipY   = cy - tipR * std::cos(fillAngle);

            // Unit vectors: radial outward (rdx,rdy) and perpendicular (pdx,pdy)
            float rdx =  std::sin(fillAngle);
            float rdy = -std::cos(fillAngle);
            float pdx = -rdy;   // rotate radial 90° CCW in screen space
            float pdy =  rdx;

            constexpr float kTriH = 5.5f;   // height (depth into arc)
            constexpr float kTriW = 3.2f;   // half-width of base

            juce::Path tri;
            tri.startNewSubPath(tipX, tipY);
            tri.lineTo(tipX - rdx * kTriH + pdx * kTriW,
                       tipY - rdy * kTriH + pdy * kTriW);
            tri.lineTo(tipX - rdx * kTriH - pdx * kTriW,
                       tipY - rdy * kTriH - pdy * kTriW);
            tri.closeSubPath();

            g.setColour(juce::Colour(GalleryColors::xoGold));
            g.fillPath(tri);
        }
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                              const juce::Colour& bg, bool highlighted, bool down) override
    {
        auto b = btn.getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(down ? GalleryColors::get(GalleryColors::xoGold)
                         : highlighted ? bg.brighter(0.06f) : bg);
        g.fillRoundedRectangle(b, 5.0f);

        // Focus ring (WCAG 2.4.7) — use focus colour instead of border when focused
        if (btn.hasKeyboardFocus (true))
        {
            g.setColour (A11y::focusRingColour());
            g.drawRoundedRectangle (b, 5.0f, 2.0f);
        }
        else
        {
            g.setColour(GalleryColors::get(GalleryColors::borderGray()));
            g.drawRoundedRectangle(b, 5.0f, 1.0f);
        }
    }
};

//==============================================================================
// ParameterGrid — auto-generates knobs for all params matching a prefix.
// Mounted inside a Viewport for scrolling.
//
// Parameters are sorted into color-coded sections (OSC / FILTER / MOD / FX /
// MACRO / OTHER) based on keywords in the parameter inner name.  Each section
// gets a Space Grotesk Bold 10pt ALL-CAPS header with a color dot, and every
// knob cell receives a subtle 3px left-border bar in the section color.
class ParameterGrid : public juce::Component
{
public:
    // ── Section identity ────────────────────────────────────────────────────
    enum class Section { OSC, FILTER, MOD, FX, MACRO, OTHER };

    // Classify a parameter by its inner name (part after prefix_).
    static Section classifyParam(const juce::String& inner)
    {
        juce::String s = inner.toLowerCase();

        // MACRO checked first — "macroCharacter" must not fall through to OSC/OTHER
        if (s.contains("macro") || s.contains("character") ||
            s.contains("movement") || s.contains("coupling") ||
            s.contains("space"))
            return Section::MACRO;

        // OSC
        if (s.contains("osc") || s.contains("wave") || s.contains("tune") ||
            s.contains("pitch") || s.contains("detune") || s.contains("harm") ||
            s.contains("partial") || s.contains("src"))
            return Section::OSC;

        // FILTER
        if (s.contains("filter") || s.contains("cutoff") || s.contains("reso") ||
            s.contains("freq") || s.contains("filt") || s.contains("flt") ||
            s.contains("svf") || s.contains("q"))
            return Section::FILTER;

        // MOD
        if (s.contains("lfo") || s.contains("mod") || s.contains("env") ||
            s.contains("attack") || s.contains("decay") || s.contains("sustain") ||
            s.contains("release") || s.contains("rate") || s.contains("depth") ||
            s.contains("amount") || s.contains("adsr"))
            return Section::MOD;

        // FX
        if (s.contains("delay") || s.contains("reverb") || s.contains("chorus") ||
            s.contains("drive") || s.contains("sat") || s.contains("wet") ||
            s.contains("dry") || s.contains("mix") || s.contains("fx") ||
            s.contains("distort") || s.contains("compress"))
            return Section::FX;

        return Section::OTHER;
    }

    static juce::Colour sectionColour(Section s)
    {
        switch (s)
        {
            case Section::OSC:    return juce::Colour(0xFF48CAE4); // Sunlit blue
            case Section::FILTER: return juce::Colour(0xFFFF6B6B); // Warm red
            case Section::MOD:    return juce::Colour(0xFF00FF41); // Phosphor green
            case Section::FX:     return juce::Colour(0xFFBF40FF); // Prism violet
            case Section::MACRO:  return juce::Colour(0xFFE9C46A); // XO Gold
            default:              return juce::Colour(0xFF9E9B97); // text-muted
        }
    }

    static const char* sectionName(Section s)
    {
        switch (s)
        {
            case Section::OSC:    return "OSC";
            case Section::FILTER: return "FILTER";
            case Section::MOD:    return "MOD";
            case Section::FX:     return "FX";
            case Section::MACRO:  return "MACRO";
            default:              return "OTHER";
        }
    }

    // ── Constructor ─────────────────────────────────────────────────────────
    ParameterGrid(XOlokunProcessor& proc,
                  const juce::String& engId,
                  const juce::String& enginePrefix,
                  juce::Colour accentColour)
    {
        auto& apvts = proc.getAPVTS();
        auto& rawParams = proc.getParameters(); // juce::AudioProcessor::getParameters()
        juce::String pfx = enginePrefix + "_";

        // ── Collect params into per-section buckets ─────────────────────────
        struct ParamEntry { juce::String pid; juce::String shortLabel; Section sec; };
        // 6 buckets — index matches Section enum value (OSC=0 … OTHER=5)
        std::vector<ParamEntry> buckets[6];

        for (auto* p : rawParams)
        {
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
            {
                juce::String pid = rp->getParameterID();
                if (!pid.startsWith(pfx))
                    continue;

                auto inner = pid.substring(pfx.length()); // e.g. "filterCutoff"
                juce::String shortLabel = EngineVocabulary::labelFor(
                    engId, pid, makeShortLabel(inner));

                Section sec = classifyParam(inner);
                buckets[static_cast<int>(sec)].push_back({ pid, shortLabel, sec });
            }
        }

        // ── Flatten buckets → ordered param list; record section runs ───────
        // Display order: OSC, FILTER, MOD, FX, MACRO, OTHER
        constexpr Section kOrder[] = {
            Section::OSC, Section::FILTER, Section::MOD,
            Section::FX,  Section::MACRO,  Section::OTHER
        };

        for (auto orderedSec : kOrder)
        {
            auto& bucket = buckets[static_cast<int>(orderedSec)];
            if (bucket.empty()) continue;

            int startIdx = (int)sliders.size();

            for (auto& entry : bucket)
            {
                auto* rp = dynamic_cast<juce::RangedAudioParameter*>(
                    apvts.getParameter(entry.pid));
                if (!rp) continue;

                auto slider = std::make_unique<juce::Slider>();
                slider->setSliderStyle(juce::Slider::RotaryVerticalDrag);
                slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
                slider->setColour(juce::Slider::rotarySliderFillColourId, accentColour);
                slider->setTooltip(rp->getName(64));
                A11y::setup(*slider, rp->getName(64),
                            rp->getName(64) + " (" + entry.pid + ")");
                addAndMakeVisible(*slider);

                auto label = std::make_unique<juce::Label>();
                label->setText(entry.shortLabel, juce::dontSendNotification);
                label->setFont(GalleryFonts::label(8.0f));
                label->setColour(juce::Label::textColourId,
                                 GalleryColors::get(GalleryColors::textMid()));
                label->setJustificationType(juce::Justification::centred);
                label->setInterceptsMouseClicks(false, false);
                addAndMakeVisible(*label);

                attachments.push_back(
                    std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                        apvts, entry.pid, *slider));

                sliders.push_back(std::move(slider));
                labels.push_back(std::move(label));
            }

            sectionRuns.push_back({ orderedSec, startIdx, (int)sliders.size() - startIdx });
        }
    }

    // ── Height calculation — accounts for per-section header rows ───────────
    int getRequiredHeight(int availableWidth) const
    {
        int cols = juce::jmax(1, availableWidth / kCellW);
        int y = kPad;

        for (auto& run : sectionRuns)
        {
            y += kHeaderRowH; // section header strip
            int rows = (run.count + cols - 1) / cols;
            y += rows * kCellH;
        }

        return y + kPad;
    }

    void resized() override
    {
        int cols    = juce::jmax(1, getWidth() / kCellW);
        int offsetX = (getWidth() - cols * kCellW) / 2;
        int y       = kPad;

        knobBounds.resize(sliders.size());
        int flatIdx = 0;

        for (auto& run : sectionRuns)
        {
            y += kHeaderRowH; // skip over section header

            for (int i = 0; i < run.count; ++i, ++flatIdx)
            {
                int col = i % cols;
                int row = i / cols;
                int cx  = offsetX + col * kCellW;
                int cy  = y + row * kCellH;

                sliders[flatIdx]->setBounds(cx + 6, cy + 4, kCellW - 12, kCellW - 12);
                labels[flatIdx]->setBounds(cx, cy + kCellW - 4, kCellW, 14);
                knobBounds[flatIdx] = { cx, cy };
            }

            int rows = (run.count + cols - 1) / cols;
            y += rows * kCellH;
        }
    }

    // ── Paint — section headers + left-border bars on each knob cell ────────
    void paint(juce::Graphics& g) override
    {
        int cols      = juce::jmax(1, getWidth() / kCellW);
        int y         = kPad;
        int flatIdx   = 0;
        bool firstSec = true;

        for (auto& run : sectionRuns)
        {
            juce::Colour secCol  = sectionColour(run.sec);
            juce::String secText = sectionName(run.sec);

            // ── Separator line above every section except the first ─────────
            if (!firstSec)
            {
                g.setColour(secCol.withAlpha(0.20f));
                g.drawHorizontalLine(y, 4.0f, (float)(getWidth() - 4));
            }
            firstSec = false;

            // ── Section header background tint ─────────────────────────────
            g.setColour(secCol.withAlpha(0.06f));
            g.fillRect(0, y, getWidth(), kHeaderRowH);

            // ── Color dot (6×6 circle, vertically centered in header row) ──
            const int dotSize = 6;
            const int dotX    = 10;
            const int dotY    = y + (kHeaderRowH - dotSize) / 2;
            g.setColour(secCol);
            g.fillEllipse((float)dotX, (float)dotY, (float)dotSize, (float)dotSize);

            // ── Section name — Space Grotesk Bold 10pt, ALL CAPS ───────────
            g.setColour(secCol.brighter(0.15f));
            g.setFont(juce::Font(GalleryFonts::spaceGroteskBold()).withHeight(10.0f));
            g.drawText(secText,
                       dotX + dotSize + 6, y,
                       getWidth() - dotX - dotSize - 16, kHeaderRowH,
                       juce::Justification::centredLeft);

            y += kHeaderRowH;

            // ── 3px left-border bar on each knob cell ──────────────────────
            for (int i = 0; i < run.count; ++i, ++flatIdx)
            {
                if (flatIdx >= (int)knobBounds.size()) break;
                auto [cx, cy] = knobBounds[flatIdx];
                g.setColour(secCol.withAlpha(0.50f));
                g.fillRect(cx, cy + 2, 3, kCellH - 4);
            }

            int rows = (run.count + cols - 1) / cols;
            y += rows * kCellH;
        }
    }

private:
    // Section run descriptor — one entry per non-empty section, in display order
    struct SectionRun { Section sec; int startIdx; int count; };

    // "filterCutoff" → "CUTOFF", "level" → "LEVEL", "ampAttack" → "ATTACK"
    static juce::String makeShortLabel(const juce::String& inner)
    {
        for (int i = 1; i < inner.length(); ++i)
            if (juce::CharacterFunctions::isUpperCase(inner[i]))
                return inner.substring(i).toUpperCase();
        return inner.toUpperCase();
    }

    static constexpr int kCellW      = 82;
    static constexpr int kCellH      = 90;
    static constexpr int kPad        = 12;
    static constexpr int kHeaderRowH = 22; // height of each section header strip

    // Destruction order matters: attachments first, then sliders.
    // Members are destroyed in reverse declaration order, so declare
    // sliders/labels before attachments.
    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::Label>>  labels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;

    // Section layout metadata — populated during construction / resized()
    std::vector<SectionRun>         sectionRuns; // ordered section descriptors
    std::vector<std::pair<int,int>>  knobBounds;  // (cx, cy) per slider, filled in resized()

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterGrid)
};

//==============================================================================
// MacroHeroStrip — full-width "character portrait" panel showing 4 tall pillar
// sliders for an engine's macro parameters. Appears at the top of EngineDetailPanel
// when an engine with macro params is selected.
//
// Macro param discovery: iterates APVTS for params matching "{prefix}_macro*"
// (up to 4 collected in declaration order). If fewer than 4 are found the
// remaining pillars are hidden. If none are found the strip hides itself.
class MacroHeroStrip : public juce::Component
{
public:
    explicit MacroHeroStrip(XOlokunProcessor& proc) : processor(proc) {}

    // Call after an engine slot is selected. Returns true if at least one
    // macro param was found and the strip should be shown.
    bool loadEngine(const juce::String& engId, const juce::String& paramPrefix,
                    juce::Colour accent)
    {
        engineName   = engId;
        accentColour = accent;

        // Clear previous attachments (must happen before slider rebuild)
        for (auto& att : attachments)
            att.reset();

        // Determine the full prefix used for param lookup (some prefixes already
        // contain a trailing underscore in the frozen table, normalise here).
        juce::String pfx = paramPrefix;
        if (!pfx.endsWithChar('_'))
            pfx += "_";
        juce::String macroPrefix = pfx + "macro"; // e.g. "oasis_macro"

        // Collect up to 4 macro param IDs from the APVTS in declaration order.
        auto& apvts = processor.getAPVTS();
        juce::StringArray foundIds, foundNames;

        for (auto* p : processor.getParameters())
        {
            if (foundIds.size() >= 4)
                break;
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
            {
                if (rp->getParameterID().startsWithIgnoreCase(macroPrefix))
                {
                    foundIds.add(rp->getParameterID());
                    // Derive short label: vocabulary override first, then
                    // everything after "_macro" → uppercase as fallback.
                    juce::String raw = rp->getParameterID().substring(macroPrefix.length());
                    juce::String defaultLabel = raw.isEmpty()
                        ? juce::String("M" + juce::String(foundIds.size()))
                        : raw.toUpperCase();
                    foundNames.add(EngineVocabulary::labelFor(
                        engineName, rp->getParameterID(), defaultLabel));
                }
            }
        }

        numMacros = foundIds.size();
        if (numMacros == 0)
        {
            setVisible(false);
            return false;
        }

        // Configure visible pillars and attach to found params
        for (int i = 0; i < 4; ++i)
        {
            bool active = (i < numMacros);
            pillars[i].setVisible(active);
            pillarLabels[i].setVisible(active);

            if (active)
            {
                pillars[i].setSliderStyle(juce::Slider::LinearVertical);
                pillars[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
                pillars[i].setColour(juce::Slider::trackColourId,
                                     accent.withAlpha(0.18f));
                pillars[i].setColour(juce::Slider::thumbColourId, accent);
                pillars[i].setColour(juce::Slider::backgroundColourId,
                                     GalleryColors::get(GalleryColors::borderGray()));
                pillars[i].setTooltip(foundNames[i]);
                A11y::setup(pillars[i], foundNames[i]);

                pillarLabels[i].setText(foundNames[i], juce::dontSendNotification);
                pillarLabels[i].setFont(GalleryFonts::heading(8.0f));
                pillarLabels[i].setColour(juce::Label::textColourId, accent.darker(0.2f));
                pillarLabels[i].setJustificationType(juce::Justification::centred);

                attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    apvts, foundIds[i], pillars[i]);
            }
        }

        setVisible(true);
        resized();
        repaint();
        return true;
    }

    void paint(juce::Graphics& g) override
    {
        if (numMacros == 0)
            return;

        using namespace GalleryColors;
        auto b = getLocalBounds().toFloat();

        // Background — slightly tinted with accent
        g.setColour(get(shellWhite()));
        g.fillRoundedRectangle(b, 4.0f);

        // Accent top strip
        g.setColour(accentColour);
        g.fillRect(b.removeFromTop(3.0f));

        // Engine name header line
        g.setColour(accentColour.darker(0.2f));
        g.setFont(GalleryFonts::display(11.0f));
        g.drawText(engineName.toUpperCase() + "  —  MACROS",
                   8, 3, getWidth() - 16, kHeaderH - 3,
                   juce::Justification::centredLeft);

        // Thin separator under header
        g.setColour(juce::Colour(GalleryColors::xoGold).withAlpha(0.45f));
        g.drawHorizontalLine(kHeaderH, 8.0f, (float)(getWidth() - 8));
    }

    void resized() override
    {
        if (numMacros == 0)
            return;

        const int labelH  = 14;
        const int sliderY = kHeaderH + 4;
        const int sliderH = getHeight() - sliderY - labelH - 4;
        const int colW    = getWidth() / 4;

        for (int i = 0; i < numMacros; ++i)
        {
            int x = i * colW;
            pillars[i].setBounds(x + 4, sliderY, colW - 8, juce::jmax(8, sliderH));
            pillarLabels[i].setBounds(x, getHeight() - labelH - 2, colW, labelH);
        }
    }

private:
    static constexpr int kHeaderH = 22;

    XOlokunProcessor& processor;
    juce::String       engineName;
    juce::Colour       accentColour { GalleryColors::get(GalleryColors::borderGray()) };
    int                numMacros = 0;

    std::array<juce::Slider, 4> pillars;
    std::array<juce::Label,  4> pillarLabels;
    // Destruction order: attachments must be destroyed before sliders.
    // Declared after sliders/labels so they are destroyed first (reverse order).
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> attachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroHeroStrip)
};

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

        // Rebuild parameter grid for this engine
        auto* newGrid = new ParameterGrid(processor, engineId, prefix, accentColour);
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

    XOlokunProcessor& processor;
    MacroHeroStrip     macroHero;
    juce::Viewport     viewport;
    juce::String       engineId  { "—" };
    juce::Colour       accentColour { GalleryColors::get(GalleryColors::borderGray()) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineDetailPanel)
};

//==============================================================================
// FieldMapPanel — sonic cartography of the current session.
//
// Every note played deposits a glowing point in a 2D canvas:
//   X = session time (recent = right, scrolling left continuously)
//   Y = pitch (C1=bottom, C7=top), with depth zone bands as background
//
// Points: radius proportional to velocity, color from engine accent,
// alpha = 1 - (age / 240s). Fades over 4 minutes.
// Background: warm dark wash with Sunlit/Twilight/Midnight zone bands.
// Live cursor: thin vertical line at right edge (current moment).
//
class FieldMapPanel : public juce::Component, private juce::Timer
{
public:
    struct NoteEvent {
        float sessionTimeS;     // seconds since session start
        float normalizedPitch;  // 0.0 (C1) to 1.0 (C7)
        float velocity;         // 0.0–1.0, drives dot radius
        juce::Colour engineColor;
    };

    FieldMapPanel()
    {
        sessionStart = juce::Time::getCurrentTime();
        startTimerHz(30);
    }

    ~FieldMapPanel() override { stopTimer(); }

    // Called from the message thread (timer drain) when a note fires.
    void addNote(int midiNote, float velocity, juce::Colour engineColor)
    {
        NoteEvent ev;
        ev.sessionTimeS    = static_cast<float>((juce::Time::getCurrentTime() - sessionStart).inSeconds());
        ev.normalizedPitch = juce::jmap(static_cast<float>(juce::jlimit(24, 96, midiNote)), 24.0f, 96.0f, 0.0f, 1.0f);
        ev.velocity        = velocity;
        ev.engineColor     = engineColor;

        // Append to ring buffer (overwrite oldest)
        events[headIdx % kMaxEvents] = ev;
        ++headIdx;
    }

    void timerCallback() override { repaint(); }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        float now = static_cast<float>((juce::Time::getCurrentTime() - sessionStart).inSeconds());
        const float kWindowS = 240.0f; // 4-minute display window

        // ── Background — dark wash with zone bands ───────────────────────────
        g.setColour(juce::Colour(0xFF0D0D1A));
        g.fillRect(b);

        // Zone bands (bottom=Midnight, top=Sunlit — matching XOlokun ecology)
        struct ZoneBand { float yNormStart, yNormEnd; juce::uint32 rgba; float alpha; };
        const ZoneBand bands[] = {
            { 0.80f, 1.00f, 0xFF48CAE4, 0.04f },  // Sunlit   — top 20%
            { 0.45f, 0.80f, 0xFF0096C7, 0.03f },  // Twilight — middle
            { 0.00f, 0.45f, 0xFF7B2FBE, 0.04f },  // Midnight — bottom
        };
        for (const auto& band : bands)
        {
            float yTop    = b.getBottom() - band.yNormEnd   * b.getHeight();
            float yBottom = b.getBottom() - band.yNormStart * b.getHeight();
            g.setColour(juce::Colour(band.rgba).withAlpha(band.alpha));
            g.fillRect(b.getX(), yTop, b.getWidth(), yBottom - yTop);
        }

        // ── Zone boundary lines ───────────────────────────────────────────────
        auto drawBand = [&](float yNorm, juce::uint32 col) {
            float y = b.getBottom() - yNorm * b.getHeight();
            g.setColour(juce::Colour(col).withAlpha(0.12f));
            g.drawHorizontalLine(static_cast<int>(y), b.getX(), b.getRight());
        };
        drawBand(0.80f, 0xFF48CAE4);
        drawBand(0.45f, 0xFF0096C7);

        // ── Note points ──────────────────────────────────────────────────────
        int total = static_cast<int>(std::min(headIdx, kMaxEvents));
        for (int i = 0; i < total; ++i)
        {
            const auto& ev = events[i % kMaxEvents];
            float age = now - ev.sessionTimeS;
            if (age > kWindowS || age < 0.0f) continue;

            float alpha = 1.0f - (age / kWindowS);
            alpha = alpha * alpha; // quadratic fade — lingers bright then drops off
            if (alpha < 0.02f) continue;

            // X: map session time within window to pixel X
            float xNorm = (ev.sessionTimeS - (now - kWindowS)) / kWindowS;
            float x     = b.getX() + xNorm * b.getWidth();
            // Y: pitch (0=bottom C1, 1=top C7)
            float y     = b.getBottom() - ev.normalizedPitch * b.getHeight();
            // Radius: 2.5 base + velocity * 4
            float r     = 2.5f + ev.velocity * 4.0f;

            // Glow pass (wider, dimmer)
            g.setColour(ev.engineColor.withAlpha(alpha * 0.15f));
            g.fillEllipse(x - r * 2.0f, y - r * 2.0f, r * 4.0f, r * 4.0f);
            // Core dot
            g.setColour(ev.engineColor.withAlpha(alpha * 0.80f));
            g.fillEllipse(x - r, y - r, r * 2.0f, r * 2.0f);
        }

        // ── Live time cursor ─────────────────────────────────────────────────
        float cursorX = b.getRight() - 1.0f;
        g.setColour(juce::Colour(0xFFE9C46A).withAlpha(0.40f)); // XO Gold
        g.drawVerticalLine(static_cast<int>(cursorX), b.getY(), b.getBottom());

        // ── Label ────────────────────────────────────────────────────────────
        g.setFont(juce::Font(juce::FontOptions(8.0f)));
        g.setColour(juce::Colours::white.withAlpha(0.20f));
        g.drawText("FIELD MAP", b.reduced(6.0f, 4.0f), juce::Justification::topLeft);
    }

private:
    static constexpr size_t kMaxEvents = 4096;
    std::array<NoteEvent, kMaxEvents> events {};
    size_t headIdx = 0;
    juce::Time sessionStart;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FieldMapPanel)
};

//==============================================================================
// Coupling type → short label, used by both OverviewPanel and CouplingPanel.
inline juce::String couplingTypeLabel(CouplingType t)
{
    switch (t) {
        case CouplingType::AmpToFilter:      return "Amp->F";
        case CouplingType::AmpToPitch:       return "Amp->P";
        case CouplingType::LFOToPitch:       return "LFO->P";
        case CouplingType::EnvToMorph:       return "Env->M";
        case CouplingType::AudioToFM:        return "Au->FM";
        case CouplingType::AudioToRing:      return "Ring";
        case CouplingType::FilterToFilter:   return "F->F";
        case CouplingType::AmpToChoke:       return "Choke";
        case CouplingType::RhythmToBlend:    return "R->B";
        case CouplingType::EnvToDecay:       return "Env->D";
        case CouplingType::PitchToPitch:     return "P->P";
        case CouplingType::AudioToWavetable: return "Au->W";
        default:                             return "?";
    }
}

//==============================================================================
// OverviewPanel — right-side content when no engine is selected.
class OverviewPanel : public juce::Component
{
public:
    explicit OverviewPanel(XOlokunProcessor& proc) : processor(proc) {}

    // Called by the editor when engine state or coupling routes change.
    // Avoids calling getRoutes() (which copies a vector) inside paint().
    void refresh()
    {
        cachedActiveEngines.clear();
        for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
        {
            auto* eng = processor.getEngine(i);
            if (eng) cachedActiveEngines.push_back({eng->getEngineId(), eng->getAccentColour()});
        }
        cachedRoutes = processor.getCouplingMatrix().getRoutes();
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        g.fillAll(get(slotBg()));

        auto b = getLocalBounds().toFloat();
        float w = b.getWidth();
        float h = b.getHeight();

        // Active engine chain (cached in refresh() — no allocation in paint())
        const auto& active = cachedActiveEngines;

        if (active.empty())
        {
            // ── Empty state: XO Gold logo mark + instruction ──────────────────
            g.setColour(get(xoGold).withAlpha(0.18f));
            float markR = 48.0f;
            float markX = b.getCentreX() - markR;
            float markY = h * 0.30f - markR;
            g.fillEllipse(markX, markY, markR * 2, markR * 2);
            g.setColour(get(xoGold).withAlpha(0.55f));
            g.drawEllipse(markX, markY, markR * 2, markR * 2, 2.0f);
            g.setFont(GalleryFonts::display(22.0f));
            g.drawText("XO", (int)markX, (int)markY, (int)markR * 2, (int)markR * 2,
                       juce::Justification::centred);

            g.setColour(get(textMid()));
            g.setFont(GalleryFonts::body(12.0f));
            float instrY = h * 0.30f + markR + 16.0f;
            g.drawText("Click an engine tile to edit parameters",
                       b.withY(instrY).withHeight(20.0f).toNearestInt(),
                       juce::Justification::centred);

            g.setColour(get(textMid()).withAlpha(0.55f));
            g.setFont(GalleryFonts::body(10.0f));
            g.drawText("No engines loaded — use the tiles on the left",
                       b.withY(h * 0.65f).withHeight(20.0f).toNearestInt(),
                       juce::Justification::centred);
            return;
        }

        // ── Character portrait (first active engine) ──────────────────────────
        const juce::String& engineId = active[0].first;
        const juce::Colour  accent   = active[0].second;

        // Diagonal gradient wash — accent colour left edge, transparent at 70%
        juce::ColourGradient wash(accent.withAlpha(0.12f), 0.0f, 0.0f,
                                  juce::Colours::transparentBlack, w * 0.70f, 0.0f, false);
        g.setGradientFill(wash);
        g.fillAll();

        // 4 px left accent stripe
        g.setColour(accent.withAlpha(0.60f));
        g.fillRect(0.0f, 0.0f, 4.0f, h);

        // Portrait zone: top 38% of the panel
        float portraitH = h * 0.38f;

        // Engine name — large display font, anchored to bottom of portrait zone
        g.setColour(accent);
        g.setFont(GalleryFonts::display(28.0f));
        g.drawFittedText(engineId.toUpperCase(),
                         juce::Rectangle<int>(12, 0, (int)w - 20, (int)(portraitH * 0.72f)),
                         juce::Justification::centredBottom, 1);

        // Engine archetype tagline lookup
        static const std::pair<const char*, const char*> kArchetypes[] = {
            {"Opera",     "Additive-vocal Kuramoto — autonomous dramatic arcs"},
            {"Offering",  "Psychology-as-DSP boom bap — Berlyne curiosity engine"},
            {"Oware",     "Mallet physics + sympathetic resonance — material continuum"},
            {"Oxbow",     "Entangled reverb — chiasmus FDN + phase erosion"},
            {"Overbite",  "Five-macro apex predator — fang white silence"},
            {"Oceandeep", "Hydrostatic compression + bioluminescent exciter"},
            {"Orbweave",  "Topological knot coupling — trefoil/figure-eight matrices"},
            {"Overtone",  "Continued fraction spectral — \xcf\x80, e, \xcf\x86, \xe2\x88\x9a\x32 timbres"},
            {"Organism",  "Cellular automata generative — coral colony growth"},
            {"Ostinato",  "Modal membrane synthesis — world rhythm engine"},
            {"Opensky",   "Euphoric shimmer + Shepard ascension geometry"},
            {"Ouie",      "Duophonic hammerhead — 8-algorithm STRIFE/LOVE axis"},
            {"Obrix",     "Modular brick synthesis — coral reef ecology"},
            {"Oracle",    "GENDY stochastic + Maqam microtonal synthesis"},
            {"Organon",   "Variational free energy — metabolism as modulation"},
            {"Ouroboros", "Strange attractor — chaotic feedback with leash"},
            {"Obsidian",  "Crystal-clear subtractive — precision cutting tool"},
            {"Origami",   "Fold-point waveshaping — Vermillion geometry engine"},
            {"Oceanic",   "Chromatophore modulator — bioluminescent sea texture"},
            {"Ocelot",    "Biome crossfade — adaptive timbral territory"},
            {"Oblique",   "Prismatic bounce — RTJ x Funk x Tame Impala"},
            {"Osprey",    "Shore-system cultural fusion — 5 coastline identities"},
            {"Osteria",   "Porto Wine resonance — warmth-saturated harmonic mesh"},
            {"Orbital",   "Group envelope system — 8-voice dynamic architecture"},
            {"Oblong",    "Resonant string model — warm acoustic character"},
            {"Obese",     "Saturated poly — Mojo analog/digital axis"},
            {"Orphica",   "Plucked string body — velocity-brightened resonance"},
            {"Obbligato", "Breath-driven formant — obligatory melodic voice"},
            {"Ottoni",    "Patinated brass — Patina spectral character"},
            {"Onset",     "XVC cross-voice coupling — multi-circuit percussion"},
            {"Ole",       "Hibiscus drama — flamenco attack articulation engine"},
            {"Ohm",       "Sage meddling — zen macro drift and calm oscillation"},
            {"Optic",     "Zero-audio identity — visual AutoPulse modulator"},
            {"Overworld", "ERA triangle — 2D Buchla/Schulze/Vangelis crossfade"},
            {"Overdub",   "Spring reverb core — Vangelis metallic depth tail"},
            {"Opal",      "Granular clouds — textural time-stretch synthesis"},
            {"Owlfish",   "Mixtur-Trautonium oscillator — abyssal depth tones"},
            {"Odyssey",   "Wavetable drift — evolving spectral morphology"},
            {"OddOscar",  "Axolotl regeneration — morphing algorithm crossfade"},
            {"OddfeliX",  "Neon tetra filter — quick-change timbral snap"},
            {"Osmosis",   "External audio membrane — permeability coupling source"},
            {"Ombre",     "Dual-narrative blend — memory meets perception"},
            {"Orca",      "Apex predator wavetable — echolocation + breach"},
            {"Octopus",   "Decentralized alien intelligence — 8-arm chromatophore"},
            {"XOverlap",  "KnotMatrix FDN — biorthogonal voice entanglement"},
            {"XOutwit",   "Chromatophore ambush — rapid-fire spectral surprise"},
        };
        juce::String tag;
        for (const auto& [id, desc] : kArchetypes)
            if (engineId.containsIgnoreCase(id)) { tag = desc; break; }

        if (tag.isNotEmpty())
        {
            g.setColour(get(textMid()));
            g.setFont(GalleryFonts::body(9.0f));
            g.drawFittedText(tag,
                             juce::Rectangle<int>(12, (int)(portraitH * 0.74f), (int)w - 20, 28),
                             juce::Justification::centredTop, 2);
        }

        // Subtle separator line below portrait zone
        g.setColour(accent.withAlpha(0.18f));
        g.drawHorizontalLine((int)portraitH, 12.0f, w - 12.0f);

        // ── Engine chain pills + coupling routes (below portrait) ─────────────
        // Draw engine chain pills with XO Gold connectors
        float chainY = portraitH + (h - portraitH) * 0.28f;
        float pillW  = 80.0f, pillH = 24.0f;
        float totalW = active.size() * pillW + (active.size() - 1) * 24.0f;
        float startX = (b.getWidth() - totalW) * 0.5f;

        for (int i = 0; i < (int)active.size(); ++i)
        {
            float px = startX + i * (pillW + 24.0f);
            juce::Rectangle<float> pill(px, chainY - pillH * 0.5f, pillW, pillH);

            g.setColour(active[i].second.withAlpha(0.15f));
            g.fillRoundedRectangle(pill, 12.0f);
            g.setColour(active[i].second);
            g.drawRoundedRectangle(pill, 12.0f, 1.5f);
            g.setFont(GalleryFonts::heading(9.5f));
            g.drawFittedText(active[i].first.toUpperCase(),
                             pill.toNearestInt(), juce::Justification::centred, 1);

            if (i < (int)active.size() - 1)
            {
                float lx1 = px + pillW + 3.0f;
                float lx2 = px + pillW + 21.0f;
                g.setColour(get(xoGold).withAlpha(0.5f));
                g.drawLine(lx1, chainY, lx2 - 5.0f, chainY, 1.5f);
                juce::Path head;
                head.addTriangle(lx2, chainY, lx2 - 6.0f, chainY - 4.0f, lx2 - 6.0f, chainY + 4.0f);
                g.fillPath(head);
            }
        }

        // ── Mini arc node diagram — replaces text-row coupling route list ────────
        {
            const auto& routes = cachedRoutes;
            float routeY = chainY + pillH * 0.5f + 12.0f;
            float padding = 16.0f;

            // Count active routes for the summary text below the diagram
            int numActive = (int)std::count_if(routes.begin(), routes.end(),
                                               [](const auto& r){ return r.active && r.amount >= 0.005f; });

            // 4 engine nodes at corners of an 80×60 sub-rect in the panel bottom
            auto nodeArea = juce::Rectangle<float>(padding, routeY, w - 2.0f * padding, 60.0f);
            juce::Point<float> nodePos[4] = {
                nodeArea.getTopLeft().translated(8.0f, 8.0f),
                nodeArea.getTopRight().translated(-8.0f, 8.0f),
                nodeArea.getBottomLeft().translated(8.0f, -8.0f),
                nodeArea.getBottomRight().translated(-8.0f, -8.0f),
            };

            // Draw Bézier arcs for each active coupling route
            for (const auto& route : routes)
            {
                if (!route.active || route.amount < 0.005f) continue;
                if (route.sourceSlot < 0 || route.sourceSlot >= 4) continue;
                if (route.destSlot   < 0 || route.destSlot   >= 4) continue;

                auto from = nodePos[route.sourceSlot];
                auto to   = nodePos[route.destSlot];
                float midX = (from.x + to.x) * 0.5f;
                float midY = (from.y + to.y) * 0.5f - 12.0f; // bow upward

                juce::Path arc;
                arc.startNewSubPath(from);
                arc.quadraticTo(juce::Point<float>(midX, midY), to);

                // Color by coupling type — mirrors CouplingArcOverlay palette
                juce::Colour arcCol;
                switch (route.type)
                {
                    case CouplingType::AudioToFM:
                    case CouplingType::AudioToRing:
                    case CouplingType::AudioToWavetable:
                    case CouplingType::AudioToBuffer:
                        arcCol = juce::Colour(0xFF0096C7); // Twilight Blue — audio-rate
                        break;
                    case CouplingType::KnotTopology:
                        arcCol = juce::Colour(0xFF7B2FBE); // Midnight Violet — bidirectional
                        break;
                    default:
                        arcCol = juce::Colour(0xFFE9C46A); // XO Gold — modulation
                        break;
                }

                g.setColour(arcCol.withAlpha(0.35f + route.amount * 0.45f));
                g.strokePath(arc, juce::PathStrokeType(1.5f));
            }

            // Draw engine node circles (4 corner positions)
            for (int i = 0; i < 4; ++i)
            {
                bool hasEng = (i < (int)cachedActiveEngines.size());
                juce::Colour nodeCol = hasEng ? cachedActiveEngines[static_cast<size_t>(i)].second
                                              : get(emptySlot());
                g.setColour(nodeCol.withAlpha(0.70f));
                g.fillEllipse(nodePos[i].x - 6.0f, nodePos[i].y - 6.0f, 12.0f, 12.0f);
                g.setColour(nodeCol);
                g.drawEllipse(nodePos[i].x - 6.0f, nodePos[i].y - 6.0f, 12.0f, 12.0f, 1.0f);

                // Slot number label
                g.setFont(juce::Font(juce::FontOptions(7.0f)));
                g.setColour(juce::Colours::white.withAlpha(0.70f));
                g.drawText(juce::String(i + 1),
                           (int)(nodePos[i].x - 6.0f), (int)(nodePos[i].y - 6.0f), 12, 12,
                           juce::Justification::centred);
            }

            // Route count summary line below the diagram
            float summaryY = nodeArea.getBottom() + 6.0f;
            g.setColour(get(textMid()).withAlpha(0.70f));
            g.setFont(GalleryFonts::body(9.0f));
            juce::String summaryText = numActive > 0
                ? juce::String(numActive) + " active route" + (numActive > 1 ? "s" : "")
                : "No active coupling routes";
            g.drawText(summaryText,
                       b.withY(summaryY).withHeight(14.0f).toNearestInt(),
                       juce::Justification::centred);
        }
    }

private:
    XOlokunProcessor& processor;
    // Cached state — updated in refresh(), never in paint()
    std::vector<std::pair<juce::String, juce::Colour>> cachedActiveEngines;
    std::vector<MegaCouplingMatrix::CouplingRoute> cachedRoutes;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OverviewPanel)
};

//==============================================================================
// CompactEngineTile — slim tile in the left sidebar column.
// Shows engine identity. Click to select (or load engine if empty).
class CompactEngineTile : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    std::function<void(int)> onSelect; // called with slot index when clicked

    CompactEngineTile(XOlokunProcessor& proc, int slotIndex)
        : processor(proc), slot(slotIndex)
    {
        A11y::setup (*this, "Engine Slot " + juce::String (slotIndex + 1),
                     "Click to select engine, right-click for options");
        setExplicitFocusOrder (slotIndex + 1);
        refresh();
        startTimerHz(10); // poll voice count at 10Hz (sufficient for visual feedback)
    }

    ~CompactEngineTile() override { stopTimer(); }

    void refresh()
    {
        auto* eng = processor.getEngine(slot);
        bool newHasEngine = (eng != nullptr);
        juce::String newId = newHasEngine ? eng->getEngineId() : juce::String{};

        // Only repaint when state actually changed — avoids idle repaint overhead.
        if (!isLoading && newHasEngine == hasEngine && newId == engineId)
            return;

        isLoading = false; // engine arrived — clear loading state
        hasEngine = newHasEngine;
        engineId  = newId;
        setTooltip(hasEngine ? "Click to edit parameters. Right-click for options."
                             : "Slot " + juce::String(slot + 1) + ": empty — click to load engine");
        accent    = hasEngine ? eng->getAccentColour()
                              : GalleryColors::get(GalleryColors::emptySlot());
        repaint();
    }

    void timerCallback() override
    {
        auto* eng = processor.getEngine(slot);
        int newCount = eng ? eng->getActiveVoiceCount() : 0;
        if (newCount != voiceCount)
        {
            voiceCount = newCount;
            repaint();
        }
    }

    // Ecological tile: porthole circle + accent strip + depth-zone gradient on select.
    // Voice-reactive: porthole ring brightens and strip glows when voices are playing.
    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        auto b = getLocalBounds().toFloat().reduced(3.0f, 2.0f);
        bool hovered = isMouseOver();

        // ── Tile background ───────────────────────────────────────────────
        if (isSelected && hasEngine)
        {
            // Depth-zone gradient: engine accent (sunlit) → midnight violet
            juce::ColourGradient grad(accent.withAlpha(0.10f), b.getX(), b.getCentreY(),
                                      juce::Colour(0xFF7B2FBE).withAlpha(0.04f),
                                      b.getRight(), b.getCentreY(), false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(b, 8.0f);
        }
        else
        {
            g.setColour(hovered ? get(slotBg()).brighter(0.025f) : get(slotBg()));
            g.fillRoundedRectangle(b, 8.0f);
        }

        // Border — accent when selected, subtle otherwise
        g.setColour(isSelected ? accent : (hovered ? accent.withAlpha(0.35f) : get(borderGray())));
        g.drawRoundedRectangle(b, 8.0f, isSelected ? 2.0f : 1.0f);

        if (isLoading)
        {
            g.setColour(get(xoGold).withAlpha(0.5f));
            g.setFont(GalleryFonts::body(9.0f));
            g.drawText("LOADING...", b.toNearestInt(), juce::Justification::centred);
            if (hasKeyboardFocus(true)) A11y::drawFocusRing(g, b, 8.0f);
            return;
        }

        if (hasEngine)
        {
            // Voice density: smooth sqrt ramp 0 (silent) → 1 (full polyphony)
            const float kMaxVoices = 8.0f;
            float voiceDensity = (voiceCount > 0)
                ? juce::jmin(1.0f, std::sqrt((float)voiceCount / kMaxVoices))
                : 0.0f;

            // Derived alphas / stroke — replace binary ternaries with smooth curves
            float fillAlpha  = 0.09f + voiceDensity * 0.19f;  // 0.09 (silent) → 0.28 (full)
            float ringAlpha  = 0.38f + voiceDensity * 0.52f;  // 0.38 → 0.90
            float ringStroke = 1.0f  + voiceDensity * 1.0f;   // 1.0  → 2.0
            float stripAlpha = 0.38f + voiceDensity * 0.50f;  // 0.38 → 0.88

            // ── Left accent strip — voice activity indicator ───────────────
            float stripX = b.getX() + 1.5f;
            float stripH = b.getHeight() * 0.55f;
            float stripY = b.getCentreY() - stripH * 0.5f;
            g.setColour(accent.withAlpha(stripAlpha));
            g.fillRoundedRectangle(stripX, stripY, 3.0f, stripH, 1.5f);

            // ── Porthole circle ────────────────────────────────────────────
            const float porW = 30.0f;
            float porCx = b.getX() + 20.0f + porW * 0.5f;
            float porCy = b.getCentreY();
            float porR  = porW * 0.5f;

            // Inner fill — brightest when voices active
            g.setColour(accent.withAlpha(fillAlpha));
            g.fillEllipse(porCx - porR, porCy - porR, porW, porW);

            // Porthole ring
            g.setColour(accent.withAlpha(ringAlpha));
            g.drawEllipse(porCx - porR, porCy - porR, porW, porW, ringStroke);

            // Glass highlight arc — top-left arc (porthole glass illusion)
            {
                float hR = porR - 2.0f;
                juce::Path hl;
                hl.addCentredArc(porCx, porCy, hR, hR, 0,
                                  -juce::MathConstants<float>::pi * 1.15f,
                                  -juce::MathConstants<float>::pi * 0.45f, true);
                g.setColour(juce::Colours::white.withAlpha(0.20f));
                g.strokePath(hl, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
            }

            // Slot number inside porthole
            g.setFont(GalleryFonts::value(9.0f));
            g.setColour(accent.withAlpha(0.38f + voiceDensity * 0.37f));  // 0.38 → 0.75
            g.drawText(juce::String(slot + 1),
                       (int)(porCx - porR), (int)(porCy - porR),
                       (int)porW, (int)porW, juce::Justification::centred);

            // ── Engine name ────────────────────────────────────────────────
            float nameX = porCx + porR + 7.0f;
            float nameW = b.getRight() - nameX - 18.0f;
            g.setFont(GalleryFonts::heading(11.0f));
            g.setColour(isSelected ? accent : get(textDark()));
            g.drawText(engineId.toUpperCase(),
                       (int)nameX, (int)b.getY(), (int)nameW, (int)b.getHeight(),
                       juce::Justification::centredLeft);

            // ── Voice activity dots — right edge ───────────────────────────
            if (voiceCount > 0)
            {
                const float dotR = 2.5f, dotSpacing = 5.5f;
                float dotX = b.getRight() - 7.0f;
                float dotY = b.getCentreY() - dotR;
                int maxDots = std::min(voiceCount, 4);
                for (int d = 0; d < maxDots; ++d)
                {
                    float alpha = d == 0 ? 0.88f : juce::jmax(0.30f, 0.7f - d * 0.2f);
                    g.setColour(accent.withAlpha(alpha));
                    g.fillEllipse(dotX - static_cast<float>(d) * dotSpacing,
                                  dotY, dotR * 2.0f, dotR * 2.0f);
                }
            }
        }
        else
        {
            // Empty slot — soft "+" affordance (warmer invite than a slot number)
            float cx = b.getCentreX(), cy = b.getCentreY();
            float armLen = 7.0f, armW = 1.5f;
            juce::Colour plusCol = get(textMid()).withAlpha(0.28f);
            g.setColour(plusCol);
            g.fillRoundedRectangle(cx - armLen, cy - armW * 0.5f, armLen * 2.0f, armW, armW * 0.5f);
            g.fillRoundedRectangle(cx - armW * 0.5f, cy - armLen, armW, armLen * 2.0f, armW * 0.5f);
        }

        // Focus ring (WCAG 2.4.7)
        if (hasKeyboardFocus(true))
            A11y::drawFocusRing(g, b, 8.0f);
    }

    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&)  override { repaint(); }
    void focusGained (juce::Component::FocusChangeType) override { repaint(); }
    void focusLost   (juce::Component::FocusChangeType) override { repaint(); }

    // Keyboard activation (WCAG 2.1.1 — all interactive elements operable via keyboard)
    bool keyPressed (const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey)
        {
            if (hasEngine)
            {
                if (onSelect) onSelect (slot);
            }
            else
                showLoadMenu();
            return true;
        }
        return false;
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (!e.mods.isPopupMenu() || !hasEngine)
            return;

        juce::PopupMenu menu;
        menu.addSectionHeader("SLOT " + juce::String(slot + 1) + ": " + engineId.toUpperCase());
        menu.addSeparator();
        menu.addItem(100, "Change Engine...");
        menu.addItem(101, "Remove Engine");
        menu.addSeparator();

        juce::PopupMenu moveMenu;
        for (int i = 0; i < 4; ++i)
        {
            if (i != slot)
                moveMenu.addItem(200 + i, "Move to Slot " + juce::String(i + 1));
        }
        menu.addSubMenu("Move to Slot", moveMenu);

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
            [this](int result)
            {
                if (result == 100)
                    showLoadMenu();
                else if (result == 101)
                {
                    processor.unloadEngine(slot);
                }
                else if (result >= 200 && result < 204)
                {
                    int targetSlot = result - 200;
                    // Get current engine ID before unloading
                    auto* eng = processor.getEngine(slot);
                    if (eng != nullptr)
                    {
                        auto currentId = eng->getEngineId().toStdString();
                        processor.loadEngine(targetSlot, currentId);
                        processor.unloadEngine(slot);
                    }
                }
            });
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (e.mouseWasDraggedSinceMouseDown())
            return;

        if (e.mods.isPopupMenu())
            return;  // right-click handled by mouseDown

        if (hasEngine)
        {
            if (onSelect) onSelect(slot);
        }
        else
        {
            showLoadMenu();
        }
    }

    void setSelected(bool sel) { isSelected = sel; repaint(); }

private:
    void showLoadMenu()
    {
        // Dynamically query the registry — no hardcoded ID list to keep in sync.
        auto registeredIds = EngineRegistry::instance().getRegisteredIds();

        juce::PopupMenu menu;
        menu.addSectionHeader("LOAD INTO SLOT " + juce::String(slot + 1));
        menu.addSeparator();

        for (int i = 0; i < (int)registeredIds.size(); ++i)
        {
            juce::String id(registeredIds[static_cast<size_t>(i)].c_str());
            auto colour = GalleryColors::accentForEngine(id);
            menu.addColouredItem(i + 1, id, colour, true, false);
        }

        if (hasEngine)
        {
            menu.addSeparator();
            menu.addItem(9999, "Remove Engine from Slot " + juce::String(slot + 1));
        }

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
            [this, registeredIds](int result)
            {
                if (result == 9999)
                {
                    processor.unloadEngine(slot);
                    return;
                }

                if (result >= 1 && result <= (int)registeredIds.size())
                {
                    isLoading = true;
                    repaint(); // show "LOADING..." immediately
                    processor.loadEngine(slot, registeredIds[static_cast<size_t>(result - 1)]);
                    // Refresh is driven by onEngineChanged (event-driven, via callAsync).
                    // Navigate to detail panel on the next message loop tick.
                    juce::Timer::callAfterDelay(0, [this]
                    {
                        if (onSelect) onSelect(slot);
                    });
                }
            });
    }

    XOlokunProcessor& processor;
    int slot;
    juce::String engineId;
    juce::Colour accent;
    bool hasEngine  = false;
    bool isSelected = false;
    bool isLoading  = false; // true between loadEngine() call and onEngineChanged callback
    int  voiceCount = 0;     // updated by timerCallback at 20Hz

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompactEngineTile)
};

//==============================================================================
// MacroSection — 4 XO Gold macro knobs + master volume, always visible
class MacroSection : public juce::Component
{
public:
    explicit MacroSection(juce::AudioProcessorValueTreeState& apvts)
    {
        struct Def { const char* id; const char* label; };
        static constexpr Def defs[4] = {
            {"macro1","CHARACTER"}, {"macro2","MOVEMENT"},
            {"macro3","COUPLING"},  {"macro4","SPACE"}
        };
        for (int i = 0; i < 4; ++i)
        {
            knobs[i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
            knobs[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            knobs[i].setColour(juce::Slider::rotarySliderFillColourId,
                               GalleryColors::get(GalleryColors::xoGold));
            knobs[i].setTooltip(juce::String("Macro ") + juce::String(i + 1) + ": " + defs[i].label);
            A11y::setup (knobs[i], juce::String ("Macro ") + juce::String (i + 1) + " " + defs[i].label);
            addAndMakeVisible(knobs[i]);
            attach[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, defs[i].id, knobs[i]);

            lbls[i].setText(defs[i].label, juce::dontSendNotification);
            lbls[i].setFont(GalleryFonts::heading(8.0f));
            lbls[i].setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::textMid()));
            lbls[i].setJustificationType(juce::Justification::centred);
            addAndMakeVisible(lbls[i]);
        }

        master.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        master.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        master.setColour(juce::Slider::rotarySliderFillColourId,
                         GalleryColors::get(GalleryColors::textMid()));
        master.setTooltip("Master output volume");
        A11y::setup (master, "Master Volume");
        addAndMakeVisible(master);
        masterAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "masterVolume", master);

        masterLbl.setText("MASTER", juce::dontSendNotification);
        masterLbl.setFont(GalleryFonts::heading(8.0f));
        masterLbl.setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::textMid()));
        masterLbl.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(masterLbl);
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        auto b = getLocalBounds().toFloat();
        g.setColour(get(shellWhite()));
        g.fillRoundedRectangle(b, 6.0f);
        g.setColour(get(borderGray()));
        g.drawRoundedRectangle(b.reduced(0.5f), 6.0f, 1.0f);

        // XO Gold top stripe — macros are brand constants, not ocean-depth values
        g.setColour(get(xoGold));
        g.fillRect(b.removeFromTop(3.0f));

        // MACROS label
        g.setColour(get(xoGoldText()));
        g.setFont(GalleryFonts::heading(8.0f));
        g.drawText("MACROS", getLocalBounds().removeFromTop(20), juce::Justification::centred);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(6, 2);
        b.removeFromTop(18);
        const int kh = 50, lh = 13;
        auto macroArea = b.removeFromLeft(b.getWidth() - 68);
        int cw = macroArea.getWidth() / 4;
        for (int i = 0; i < 4; ++i)
        {
            auto col = macroArea.removeFromLeft(cw);
            int kx = col.getCentreX() - kh / 2;
            knobs[i].setBounds(kx, col.getY(), kh, kh);
            lbls[i].setBounds(kx, col.getY() + kh + 1, kh, lh);
        }
        int mx = b.getCentreX() - 22;
        master.setBounds(mx, b.getY(), 44, 44);
        masterLbl.setBounds(mx, b.getY() + 45, 44, lh);
    }

    // Called by PresetBrowserStrip when a preset with custom macroLabels is loaded.
    void setLabels(const juce::StringArray& labels)
    {
        static const char* defaults[4] = {"CHARACTER","MOVEMENT","COUPLING","SPACE"};
        for (int i = 0; i < 4; ++i)
        {
            auto text = (i < labels.size() && labels[i].isNotEmpty())
                            ? labels[i] : juce::String(defaults[i]);
            lbls[i].setText(text, juce::dontSendNotification);
        }
    }

private:
    std::array<juce::Slider, 4> knobs;
    std::array<juce::Label,  4> lbls;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> attach;
    juce::Slider master;
    juce::Label  masterLbl;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroSection)
};

//==============================================================================
// AdvancedFXPanel — generic advanced parameter popup for any FX section.
// Takes an array of parameter ID/label pairs and displays them as rotary knobs.
class AdvancedFXPanel : public juce::Component
{
public:
    AdvancedFXPanel(juce::AudioProcessorValueTreeState& apvts,
                    const juce::String& title,
                    const std::vector<std::pair<juce::String, juce::String>>& paramDefs)
    {
        titleText = title;
        int count = juce::jmin(static_cast<int>(paramDefs.size()), kMaxKnobs);
        numKnobs = count;

        for (int i = 0; i < count; ++i)
        {
            knobs[i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
            knobs[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            knobs[i].setColour(juce::Slider::rotarySliderFillColourId,
                               GalleryColors::get(GalleryColors::textMid()).withAlpha(0.75f));
            addAndMakeVisible(knobs[i]);
            attach[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, paramDefs[static_cast<size_t>(i)].first, knobs[i]);

            lbls[i].setText(paramDefs[static_cast<size_t>(i)].second, juce::dontSendNotification);
            lbls[i].setFont(GalleryFonts::heading(8.5f));
            lbls[i].setColour(juce::Label::textColourId,
                              GalleryColors::get(GalleryColors::textMid()));
            lbls[i].setJustificationType(juce::Justification::centred);
            addAndMakeVisible(lbls[i]);
        }
        setSize(64 * count + 16, 96);
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        g.fillAll(get(shellWhite()));
        g.setColour(get(textMid()).withAlpha(0.40f));
        g.setFont(GalleryFonts::heading(8.0f));
        g.drawText(titleText, getLocalBounds().removeFromTop(14).reduced(8, 0),
                   juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(8, 4);
        b.removeFromTop(14);
        int cw = numKnobs > 0 ? b.getWidth() / numKnobs : b.getWidth();
        for (int i = 0; i < numKnobs; ++i)
        {
            auto col = b.removeFromLeft(cw);
            int kh = 44;
            int ky = col.getCentreY() - (kh + 13) / 2;
            knobs[i].setBounds(col.getCentreX() - kh / 2, ky, kh, kh);
            lbls[i].setBounds(col.getX(), ky + kh + 2, col.getWidth(), 12);
        }
    }

private:
    static constexpr int kMaxKnobs = 8;
    juce::String titleText;
    int numKnobs = 0;
    std::array<juce::Slider, kMaxKnobs> knobs;
    std::array<juce::Label, kMaxKnobs>  lbls;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, kMaxKnobs> attach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdvancedFXPanel)
};

//==============================================================================
// MasterFXSection — 6-section FX strip with primary knobs per section +
// ADV buttons for deeper parameters. Always visible at bottom of Gallery Model.
//
// Layout: SAT | DELAY | REVERB | MOD | COMP | SEQ
class MasterFXSection : public juce::Component
{
public:
    explicit MasterFXSection(juce::AudioProcessorValueTreeState& apvts) : myApvts(apvts)
    {
        // Primary knob definitions: { paramID, knobLabel, sectionLabel }
        struct Def { const char* id; const char* label; const char* section; };
        static constexpr Def defs[kNumPrimaryKnobs] = {
            {"master_satDrive",      "DRIVE",  "SAT"},
            {"master_delayMix",      "MIX",    "DELAY"},
            {"master_delayFeedback", "FB",     ""},
            {"master_reverbMix",     "MIX",    "REVERB"},
            {"master_modDepth",      "DEPTH",  "MOD"},
            {"master_compMix",       "GLUE",   "COMP"},
        };

        for (int i = 0; i < kNumPrimaryKnobs; ++i)
        {
            knobs[i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
            knobs[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            knobs[i].setColour(juce::Slider::rotarySliderFillColourId,
                               GalleryColors::get(GalleryColors::textMid()).withAlpha(0.7f));
            knobs[i].setTooltip(juce::String(defs[i].section) + " " + defs[i].label);
            A11y::setup(knobs[i], juce::String("Master FX ") + defs[i].section + " " + defs[i].label);
            addAndMakeVisible(knobs[i]);
            attach[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, defs[i].id, knobs[i]);

            lbls[i].setText(defs[i].label, juce::dontSendNotification);
            lbls[i].setFont(GalleryFonts::heading(7.5f));
            lbls[i].setColour(juce::Label::textColourId,
                              GalleryColors::get(GalleryColors::textMid()));
            lbls[i].setJustificationType(juce::Justification::centred);
            addAndMakeVisible(lbls[i]);
        }

        // Section header labels
        static const char* sectionNames[kNumSections] = {"SAT", "DELAY", "REVERB", "MOD", "COMP", "SEQ"};
        for (int i = 0; i < kNumSections; ++i)
        {
            sectionHdrs[i].setText(sectionNames[i], juce::dontSendNotification);
            sectionHdrs[i].setFont(GalleryFonts::label(7.0f));
            sectionHdrs[i].setColour(juce::Label::textColourId,
                                     GalleryColors::get(GalleryColors::textMid()).withAlpha(0.50f));
            sectionHdrs[i].setJustificationType(juce::Justification::centred);
            addAndMakeVisible(sectionHdrs[i]);
        }

        // ADV buttons for each section
        auto makeAdvBtn = [&](juce::TextButton& btn, const juce::String& tip, const juce::String& a11y) {
            btn.setButtonText("ADV");
            btn.setTooltip(tip);
            A11y::setup(btn, a11y, "Open advanced settings");
            btn.setColour(juce::TextButton::buttonColourId,
                          GalleryColors::get(GalleryColors::shellWhite()));
            btn.setColour(juce::TextButton::textColourOffId,
                          GalleryColors::get(GalleryColors::textMid()).withAlpha(0.6f));
            addAndMakeVisible(btn);
        };

        makeAdvBtn(advBtnDelay, "Delay Time, Ping Pong, Damping, Diffusion, Sync", "Advanced Delay");
        advBtnDelay.onClick = [this] {
            showAdvancedPanel(advBtnDelay, "DELAY ADVANCED", {
                {"master_delayTime",     "TIME"},
                {"master_delayPingPong", "P.PONG"},
                {"master_delayDamping",  "DAMP"},
                {"master_delayDiffusion","DIFF"},
                {"master_delaySync",     "SYNC"},
            });
        };

        makeAdvBtn(advBtnReverb, "Reverb Size", "Advanced Reverb");
        advBtnReverb.onClick = [this] {
            showAdvancedPanel(advBtnReverb, "REVERB ADVANCED", {
                {"master_reverbSize", "SIZE"},
            });
        };

        makeAdvBtn(advBtnMod, "Mod Rate, Mode, Feedback", "Advanced Modulation");
        advBtnMod.onClick = [this] {
            showAdvancedPanel(advBtnMod, "MOD ADVANCED", {
                {"master_modRate",     "RATE"},
                {"master_modMix",      "MIX"},
                {"master_modMode",     "MODE"},
                {"master_modFeedback", "FB"},
            });
        };

        makeAdvBtn(advBtnComp, "Comp Ratio, Attack, Release", "Advanced Compressor");
        advBtnComp.onClick = [this] {
            showAdvancedPanel(advBtnComp, "COMP ADVANCED", {
                {"master_compRatio",   "RATIO"},
                {"master_compAttack",  "ATTACK"},
                {"master_compRelease", "RELEASE"},
            });
        };

        // SEQ section: enable toggle + ADV
        seqToggle.setButtonText("SEQ");
        seqToggle.setTooltip("Enable Master FX Sequencer");
        A11y::setup(seqToggle, "Sequencer Enable", "Toggle the Master FX step sequencer");
        seqToggle.setColour(juce::TextButton::buttonColourId,
                            GalleryColors::get(GalleryColors::shellWhite()));
        seqToggle.setColour(juce::TextButton::buttonOnColourId,
                            GalleryColors::get(GalleryColors::xoGold));
        seqToggle.setColour(juce::TextButton::textColourOffId,
                            GalleryColors::get(GalleryColors::textMid()));
        seqToggle.setColour(juce::TextButton::textColourOnId,
                            GalleryColors::get(GalleryColors::textDark()));
        seqToggle.setClickingTogglesState(true);
        seqAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, "master_seqEnabled", seqToggle);
        addAndMakeVisible(seqToggle);

        makeAdvBtn(advBtnSeq, "Seq Rate, Steps, Depth, Smooth, Pattern, Targets", "Advanced Sequencer");
        advBtnSeq.onClick = [this] {
            showAdvancedPanel(advBtnSeq, "SEQUENCER", {
                {"master_seqRate",      "RATE"},
                {"master_seqSteps",     "STEPS"},
                {"master_seqDepth",     "DEPTH"},
                {"master_seqSmooth",    "SMOOTH"},
                {"master_seqPattern",   "PATTERN"},
                {"master_seqTarget1",   "TGT 1"},
                {"master_seqTarget2",   "TGT 2"},
                {"master_seqEnvAmount", "ENV"},
            });
        };
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        auto b = getLocalBounds().toFloat();
        g.setColour(get(shellWhite()).darker(0.03f));
        g.fillRoundedRectangle(b, 6.0f);
        g.setColour(get(borderGray()));
        g.drawRoundedRectangle(b.reduced(0.5f), 6.0f, 1.0f);

        // Dividers between sections
        g.setColour(get(borderGray()).withAlpha(0.5f));
        for (int i = 0; i < kNumSections - 1; ++i)
        {
            if (divX[i] > 0)
                g.drawLine(static_cast<float>(divX[i]), 8.0f,
                           static_cast<float>(divX[i]), static_cast<float>(getHeight()) - 8, 1.0f);
        }
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(4, 3);

        // Section widths: proportional based on knob count
        // SAT(1 knob) : DELAY(2) : REVERB(1) : MOD(1) : COMP(1) : SEQ(toggle+adv)
        // Weights:  2 : 3 : 2 : 2 : 2 : 2  = 13 total
        static constexpr int weights[kNumSections] = {2, 3, 2, 2, 2, 2};
        static constexpr int totalWeight = 13;
        int totalW = b.getWidth();

        int sectionX[kNumSections + 1];
        sectionX[0] = b.getX();
        for (int i = 0; i < kNumSections; ++i)
            sectionX[i + 1] = sectionX[i] + (totalW * weights[i]) / totalWeight;

        // Record divider positions
        for (int i = 0; i < kNumSections - 1; ++i)
            divX[i] = sectionX[i + 1];

        int topY = b.getY();
        int h = b.getHeight();

        // Helper: lay out knobs in a section
        auto layoutKnob = [&](int knobIdx, int sx, int sw) {
            int kh = 36, lh = 10;
            int ky = topY + 10 + (h - 10 - kh - lh) / 2;
            knobs[knobIdx].setBounds(sx + (sw - kh) / 2, ky, kh, kh);
            lbls[knobIdx].setBounds(sx, ky + kh + 1, sw, lh);
        };

        auto layoutAdvBtn = [&](juce::TextButton& btn, int sx, int sw, int topOff) {
            btn.setBounds(sx + (sw - 28) / 2, topY + h - 16 + topOff, 28, 13);
        };

        // Section 0: SAT — 1 knob (DRIVE)
        {
            int sx = sectionX[0], sw = sectionX[1] - sectionX[0];
            sectionHdrs[0].setBounds(sx, topY, sw, 10);
            layoutKnob(0, sx, sw);
        }

        // Section 1: DELAY — 2 knobs (MIX, FB) + ADV
        {
            int sx = sectionX[1], sw = sectionX[2] - sectionX[1];
            sectionHdrs[1].setBounds(sx, topY, sw, 10);
            int halfW = sw / 2;
            layoutKnob(1, sx, halfW);
            layoutKnob(2, sx + halfW, halfW);
            layoutAdvBtn(advBtnDelay, sx, sw, 0);
        }

        // Section 2: REVERB — 1 knob (MIX) + ADV
        {
            int sx = sectionX[2], sw = sectionX[3] - sectionX[2];
            sectionHdrs[2].setBounds(sx, topY, sw, 10);
            layoutKnob(3, sx, sw);
            layoutAdvBtn(advBtnReverb, sx, sw, 0);
        }

        // Section 3: MOD — 1 knob (DEPTH) + ADV
        {
            int sx = sectionX[3], sw = sectionX[4] - sectionX[3];
            sectionHdrs[3].setBounds(sx, topY, sw, 10);
            layoutKnob(4, sx, sw);
            layoutAdvBtn(advBtnMod, sx, sw, 0);
        }

        // Section 4: COMP — 1 knob (GLUE) + ADV
        {
            int sx = sectionX[4], sw = sectionX[5] - sectionX[4];
            sectionHdrs[4].setBounds(sx, topY, sw, 10);
            layoutKnob(5, sx, sw);
            layoutAdvBtn(advBtnComp, sx, sw, 0);
        }

        // Section 5: SEQ — toggle + ADV
        {
            int sx = sectionX[5], sw = sectionX[6] - sectionX[5];
            sectionHdrs[5].setBounds(sx, topY, sw, 10);
            int btnW = juce::jmin(sw - 4, 36);
            seqToggle.setBounds(sx + (sw - btnW) / 2, topY + 14, btnW, 22);
            layoutAdvBtn(advBtnSeq, sx, sw, 0);
        }
    }

private:
    void showAdvancedPanel(juce::TextButton& btn, const juce::String& title,
                           const std::vector<std::pair<juce::String, juce::String>>& params)
    {
        juce::CallOutBox::launchAsynchronously(
            std::make_unique<AdvancedFXPanel>(myApvts, title, params),
            btn.getScreenBounds(),
            getTopLevelComponent());
    }

    static constexpr int kNumPrimaryKnobs = 6;
    static constexpr int kNumSections = 6;

    juce::AudioProcessorValueTreeState& myApvts;

    // Primary knobs
    std::array<juce::Slider, kNumPrimaryKnobs> knobs;
    std::array<juce::Label, kNumPrimaryKnobs>  lbls;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, kNumPrimaryKnobs> attach;

    // Section headers
    std::array<juce::Label, kNumSections> sectionHdrs;

    // ADV buttons per section
    juce::TextButton advBtnDelay, advBtnReverb, advBtnMod, advBtnComp, advBtnSeq;

    // SEQ toggle
    juce::TextButton seqToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> seqAttach;

    // Divider X positions
    int divX[kNumSections - 1] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterFXSection)
};

//==============================================================================
// PresetBrowserPanel — full preset browser shown in a CallOutBox.
// Provides mood tabs + name search + scrollable list.
// Calls onPresetSelected callback when the user clicks a preset row.
class PresetBrowserPanel : public juce::Component,
                           public juce::ListBoxModel
{
public:
    PresetBrowserPanel(const PresetManager& pm,
                       std::function<void(const PresetData&)> onSelect)
        : presetManager(pm), onPresetSelected(std::move(onSelect))
    {
        // Search field
        searchField.setTextToShowWhenEmpty("Search presets\xe2\x80\xa6",
                                           GalleryColors::get(GalleryColors::textMid()).withAlpha(0.4f));
        searchField.setColour(juce::TextEditor::backgroundColourId,
                              GalleryColors::get(GalleryColors::slotBg()));
        searchField.setColour(juce::TextEditor::outlineColourId,
                              GalleryColors::get(GalleryColors::borderGray()));
        searchField.setColour(juce::TextEditor::textColourId,
                              GalleryColors::get(GalleryColors::textDark()));
        searchField.setFont(GalleryFonts::body(11.0f));
        searchField.onTextChange = [this] { updateFilter(); };
        addAndMakeVisible(searchField);

        // Mood filter buttons (ALL = index 0, then 15 moods)
        static const char* moodLabels[] = {
            "ALL", "Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family",
            "Submerged", "Coupling", "Crystalline", "Deep", "Ethereal", "Kinetic", "Luminous", "Organic"
        };
        for (int i = 0; i < kNumMoods; ++i)
        {
            moodBtns[i].setButtonText(moodLabels[i]);
            moodBtns[i].setClickingTogglesState(false);
            moodBtns[i].setColour(juce::TextButton::buttonColourId,
                                  GalleryColors::get(GalleryColors::shellWhite()));
            moodBtns[i].setColour(juce::TextButton::textColourOffId,
                                  GalleryColors::get(GalleryColors::textMid()));
            moodBtns[i].setColour(juce::TextButton::buttonOnColourId,
                                  GalleryColors::get(GalleryColors::xoGold).withAlpha(0.2f));
            moodBtns[i].onClick = [this, i]
            {
                activeMood = i;
                for (int j = 0; j < kNumMoods; ++j)
                    moodBtns[j].setToggleState(j == i, juce::dontSendNotification);
                updateFilter();
            };
            addAndMakeVisible(moodBtns[i]);
        }
        moodBtns[0].setToggleState(true, juce::dontSendNotification);

        // Preset list
        listBox.setModel(this);
        listBox.setRowHeight(24);
        listBox.setColour(juce::ListBox::backgroundColourId,
                          GalleryColors::get(GalleryColors::slotBg()));
        listBox.setColour(juce::ListBox::outlineColourId,
                          GalleryColors::get(GalleryColors::borderGray()));
        listBox.setOutlineThickness(1);
        addAndMakeVisible(listBox);

        // Count label
        countLabel.setFont(GalleryFonts::label(8.5f));
        countLabel.setColour(juce::Label::textColourId,
                             GalleryColors::get(GalleryColors::textMid()).withAlpha(0.55f));
        countLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(countLabel);

        updateFilter();
        setSize(380, 340);
    }

    // juce::ListBoxModel interface
    int getNumRows() override { return (int)filtered.size(); }

    void paintListBoxItem(int row, juce::Graphics& g,
                          int w, int h, bool selected) override
    {
        if (row < 0 || row >= (int)filtered.size())
            return;

        const auto& preset = filtered[static_cast<size_t>(row)];
        using namespace GalleryColors;

        if (selected)
            g.fillAll(get(xoGold).withAlpha(0.22f));
        else if (row % 2 == 0)
            g.fillAll(get(shellWhite()));
        else
            g.fillAll(get(slotBg()));

        // Mood accent dot
        static const juce::Colour moodColors[] = {
            juce::Colour(0xFF00A6D6), // Foundation  → OddfeliX/Neon Tetra Blue
            juce::Colour(0xFFE8839B), // Atmosphere  → OddOscar/Axolotl Gill Pink
            juce::Colour(0xFF7B2D8B), // Entangled   → Drift/Violet
            juce::Colour(0xFF0066FF), // Prism       → Onset/Blue
            juce::Colour(0xFFE9A84A), // Flux        → Bob/Amber
            juce::Colour(0xFFA78BFA), // Aether      → Opal/Lavender
            juce::Colour(0xFFFF8A7A), // Family      → Rascal Coral
            juce::Colour(0xFF2D0A4E), // Submerged   → Trench Violet
            juce::Colour(0xFF1A6B5A), // Coupling    → Oxbow Teal
            juce::Colour(0xFFA8D8EA), // Crystalline → Spectral Ice
            juce::Colour(0xFF003366), // Deep        → Synth Bass Blue
            juce::Colour(0xFF9B5DE5), // Ethereal    → Synapse Violet
            juce::Colour(0xFFE5B80B), // Kinetic     → Crate Wax Yellow
            juce::Colour(0xFFC6E377), // Luminous    → Emergence Lime
            juce::Colour(0xFF228B22), // Organic     → Forest Green
        };
        static const char* moodIds[] = {
            "Foundation", "Atmosphere", "Entangled",  "Prism",   "Flux",     "Aether",   "Family",  "Submerged",
            "Coupling",   "Crystalline","Deep",        "Ethereal","Kinetic",  "Luminous", "Organic"
        };
        juce::Colour dot = get(borderGray());
        for (int mi = 0; mi < 15; ++mi)
            if (preset.mood == moodIds[mi]) { dot = moodColors[mi]; break; }

        g.setColour(dot.withAlpha(0.7f));
        g.fillEllipse(8.0f, h * 0.5f - 3.5f, 7.0f, 7.0f);

        // Preset name
        g.setColour(get(selected ? textDark() : textMid()));
        g.setFont(GalleryFonts::body(10.5f));
        g.drawText(preset.name, 22, 0, w - 36, h,
                   juce::Justification::centredLeft, true);

        // Engine tag if multi-engine
        if (!preset.engines.isEmpty() && preset.engines[0].isNotEmpty())
        {
            auto tag = preset.engines[0].substring(0, juce::jmin(3, preset.engines[0].length())).toUpperCase();
            g.setColour(get(textMid()).withAlpha(0.50f));
            g.setFont(GalleryFonts::label(7.5f));
            g.drawText(tag, w - 28, 0, 26, h, juce::Justification::centredRight);
        }
    }

    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override
    {
        selectRow(row);
    }

    void listBoxItemClicked(int row, const juce::MouseEvent&) override
    {
        listBox.selectRow(row);
    }

    void selectedRowsChanged(int) override
    {
        int row = listBox.getSelectedRow();
        selectRow(row);
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        g.fillAll(get(shellWhite()));
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(8, 6);

        // Search field row
        auto searchRow = b.removeFromTop(28);
        countLabel.setBounds(searchRow.removeFromRight(56));
        searchField.setBounds(searchRow.reduced(0, 2));

        b.removeFromTop(4);

        // Mood tabs row
        auto moodRow = b.removeFromTop(24);
        int bw = moodRow.getWidth() / kNumMoods;
        for (int i = 0; i < kNumMoods; ++i)
            moodBtns[i].setBounds(moodRow.removeFromLeft(bw).reduced(1, 0));

        b.removeFromTop(4);

        // Preset list
        listBox.setBounds(b);
    }

private:
    void selectRow(int row)
    {
        if (row >= 0 && row < (int)filtered.size() && onPresetSelected)
            onPresetSelected(filtered[static_cast<size_t>(row)]);
    }

    void updateFilter()
    {
        static const char* moodNames[] = {
            "", "Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family",
            "Submerged", "Coupling", "Crystalline", "Deep", "Ethereal", "Kinetic", "Luminous", "Organic"
        };

        auto query = searchField.getText().trim();

        // Start with mood filter (index 0 = ALL = no filter)
        const auto& lib = presetManager.getLibrary();
        filtered.clear();

        for (const auto& p : lib)
        {
            bool moodMatch = (activeMood == 0) || (p.mood == moodNames[activeMood]);
            bool nameMatch = query.isEmpty() || p.name.containsIgnoreCase(query);
            if (moodMatch && nameMatch)
                filtered.push_back(p);
        }

        // Sort alphabetically within current filter
        std::sort(filtered.begin(), filtered.end(),
                  [](const PresetData& a, const PresetData& b) {
                      return a.name.compareIgnoreCase(b.name) < 0;
                  });

        listBox.updateContent();
        listBox.deselectAllRows();

        countLabel.setText(juce::String(filtered.size()) + " presets",
                           juce::dontSendNotification);
    }

    static constexpr int kNumMoods = 16; // ALL + 15 moods

    const PresetManager& presetManager;
    std::function<void(const PresetData&)> onPresetSelected;

    juce::TextEditor searchField;
    juce::TextButton moodBtns[kNumMoods];
    juce::ListBox listBox;
    juce::Label countLabel;
    std::vector<PresetData> filtered;
    int activeMood = 0; // 0 = ALL

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserPanel)
};

// PresetBrowser is included at the top of this file (before namespace xolokun {)
// to avoid the nested-namespace problem.  The class is available here as
// xolokun::PresetBrowser.

//==============================================================================
// PresetBrowserStrip — prev/next navigation + preset name display.
// Lives in the editor header. Calls processor.applyPreset() on navigation.
class PresetBrowserStrip : public juce::Component,
                           private PresetManager::Listener
{
public:
    PresetBrowserStrip(XOlokunProcessor& proc)
        : processor(proc)
    {
        prevBtn.setButtonText("<");
        nextBtn.setButtonText(">");
        browseBtn.setButtonText("\xe2\x8a\x9e"); // ⊞ grid icon (UTF-8)
        prevBtn.setTooltip("Previous preset");
        nextBtn.setTooltip("Next preset");
        browseBtn.setTooltip("Browse all presets by mood");
        A11y::setup (prevBtn, "Previous Preset");
        A11y::setup (nextBtn, "Next Preset");
        A11y::setup (browseBtn, "Browse Presets", "Open preset browser by mood category");

        for (auto* btn : {&prevBtn, &nextBtn, &browseBtn})
        {
            btn->setColour(juce::TextButton::buttonColourId,
                           GalleryColors::get(GalleryColors::shellWhite()));
            btn->setColour(juce::TextButton::textColourOffId,
                           GalleryColors::get(GalleryColors::textMid()));
            addAndMakeVisible(*btn);
        }

        nameLabel.setJustificationType(juce::Justification::centred);
        nameLabel.setFont(GalleryFonts::heading(10.5f));
        nameLabel.setColour(juce::Label::textColourId,
                            GalleryColors::get(GalleryColors::textDark()));
        nameLabel.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(nameLabel);

        prevBtn.onClick = [this]
        {
            auto& pm = processor.getPresetManager();
            pm.previousPreset();
            const auto& preset = pm.getCurrentPreset();
            processor.applyPreset(preset);
            if (macroSection && !preset.macroLabels.isEmpty())
                macroSection->setLabels(preset.macroLabels);
        };

        nextBtn.onClick = [this]
        {
            auto& pm = processor.getPresetManager();
            pm.nextPreset();
            const auto& preset = pm.getCurrentPreset();
            processor.applyPreset(preset);
            if (macroSection && !preset.macroLabels.isEmpty())
                macroSection->setLabels(preset.macroLabels);
        };

        browseBtn.onClick = [this] { openBrowser(); };

        processor.getPresetManager().addListener(this);
        updateDisplay();
    }

    ~PresetBrowserStrip() override
    {
        processor.getPresetManager().removeListener(this);
    }

    void updateDisplay()
    {
        auto& pm = processor.getPresetManager();
        int total = pm.getLibrarySize();
        bool hasPresets = total > 0;
        prevBtn.setEnabled(hasPresets);
        nextBtn.setEnabled(hasPresets);

        if (hasPresets)
        {
            auto name = pm.getCurrentPreset().name;
            nameLabel.setText(name.isEmpty() ? "—" : name,
                              juce::dontSendNotification);
        }
        else
        {
            nameLabel.setText("no presets", juce::dontSendNotification);
        }
    }

    void resized() override
    {
        auto b = getLocalBounds();
        prevBtn.setBounds(b.removeFromLeft(22));
        browseBtn.setBounds(b.removeFromRight(30));
        nextBtn.setBounds(b.removeFromRight(22));
        nameLabel.setBounds(b);
    }

    void setMacroSection(MacroSection* ms) { macroSection = ms; }

private:
    void presetLoaded(const PresetData& preset) override
    {
        nameLabel.setText(preset.name, juce::dontSendNotification);
    }

    void openBrowser()
    {
        auto& pm = processor.getPresetManager();

        // SafePointer becomes null if this component is destroyed while the
        // CallOutBox is still open (e.g., plugin editor closed during browse).
        juce::Component::SafePointer<PresetBrowserStrip> safeThis(this);

        // Build the full PresetBrowser (search + DNA + 48px rows).
        auto browser = std::make_unique<PresetBrowser>(pm);

        // Wire the onPresetSelected callback — same logic as the old panel.
        browser->onPresetSelected = [safeThis, &proc = processor](const PresetData& preset)
        {
            proc.getPresetManager().setCurrentPreset(preset);
            proc.applyPreset(preset);
            // Only touch UI members if the strip is still alive.
            if (safeThis != nullptr)
            {
                safeThis->nameLabel.setText(preset.name, juce::dontSendNotification);
                if (safeThis->macroSection && !preset.macroLabels.isEmpty())
                    safeThis->macroSection->setLabels(preset.macroLabels);
            }
        };

        // Size: 560 wide × 520 tall — accommodates 48px rows, search bar,
        // mood filter strip, and the "Find Similar" DNA button at the bottom.
        browser->setSize(560, 520);

        juce::CallOutBox::launchAsynchronously(
            std::move(browser),
            browseBtn.getScreenBounds(),
            getTopLevelComponent());
    }

    XOlokunProcessor& processor;
    juce::TextButton prevBtn, nextBtn, browseBtn;
    juce::Label nameLabel;
    MacroSection* macroSection = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserStrip)
};

//==============================================================================
// ChordMachinePanel — visual interface for the Chord Machine.
//
// Shows: chord strip (4 slot cards), 16-step grid, control knobs.
// Polls ChordMachine state at 15Hz for real-time visualization.
//
class ChordMachinePanel : public juce::Component, private juce::Timer
{
public:
    explicit ChordMachinePanel (XOlokunProcessor& proc)
        : processor (proc)
    {
        setTitle ("Chord Machine");
        setDescription ("Generative chord sequencer with palette, voicing, and pattern controls");

        // ON/OFF toggle
        addAndMakeVisible (enableBtn);
        enableBtn.setButtonText ("OFF");
        enableBtn.setClickingTogglesState (true);
        A11y::setup (enableBtn, "Chord Machine Enable", "Toggle chord machine on or off");
        enableBtn.onClick = [this]
        {
            bool on = enableBtn.getToggleState();
            if (auto* p = processor.getAPVTS().getParameter ("cm_enabled"))
                p->setValueNotifyingHost (on ? 1.0f : 0.0f);
            enableBtn.setButtonText (on ? "ON" : "OFF");
        };

        // Sequencer play/stop
        addAndMakeVisible (seqBtn);
        seqBtn.setButtonText ("SEQ");
        seqBtn.setClickingTogglesState (true);
        seqBtn.onClick = [this]
        {
            bool on = seqBtn.getToggleState();
            if (auto* p = processor.getAPVTS().getParameter ("cm_seq_running"))
                p->setValueNotifyingHost (on ? 1.0f : 0.0f);
        };

        // Palette selector
        addAndMakeVisible (paletteBox);
        paletteBox.addItemList ({ "WARM", "BRIGHT", "TENSION", "OPEN",
                                   "DARK", "SWEET", "COMPLEX", "RAW" }, 1);
        paletteBox.setSelectedId (1, juce::dontSendNotification);
        paletteBox.onChange = [this]
        {
            if (auto* p = processor.getAPVTS().getParameter ("cm_palette"))
                p->setValueNotifyingHost (static_cast<float> (paletteBox.getSelectedItemIndex())
                                          / 7.0f);
        };

        // Voicing selector
        addAndMakeVisible (voicingBox);
        voicingBox.addItemList ({ "ROOT-SPREAD", "DROP-2", "QUARTAL",
                                   "UPPER STRUCT", "UNISON" }, 1);
        voicingBox.setSelectedId (1, juce::dontSendNotification);
        voicingBox.onChange = [this]
        {
            if (auto* p = processor.getAPVTS().getParameter ("cm_voicing"))
                p->setValueNotifyingHost (static_cast<float> (voicingBox.getSelectedItemIndex())
                                          / 4.0f);
        };

        // Pattern selector
        addAndMakeVisible (patternBox);
        patternBox.addItemList ({ "FOUR", "OFF-BEAT", "SYNCO", "STAB",
                                   "GATE", "PULSE", "BROKEN", "REST" }, 1);
        patternBox.setSelectedId (2, juce::dontSendNotification);
        patternBox.onChange = [this]
        {
            auto idx = patternBox.getSelectedItemIndex();
            processor.getChordMachine().applyPattern (static_cast<RhythmPattern> (idx));
            if (auto* p = processor.getAPVTS().getParameter ("cm_seq_pattern"))
                p->setValueNotifyingHost (static_cast<float> (idx) / 7.0f);
        };

        // Velocity curve selector
        addAndMakeVisible (velCurveBox);
        velCurveBox.addItemList ({ "EQUAL", "ROOT HEAVY", "TOP BRIGHT", "V-SHAPE" }, 1);
        velCurveBox.setSelectedId (2, juce::dontSendNotification);
        velCurveBox.onChange = [this]
        {
            if (auto* p = processor.getAPVTS().getParameter ("cm_vel_curve"))
                p->setValueNotifyingHost (static_cast<float> (velCurveBox.getSelectedItemIndex())
                                          / 3.0f);
        };

        // Knobs
        auto makeKnob = [this] (juce::Slider& knob, const juce::String& paramId)
        {
            knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 14);
            addAndMakeVisible (knob);
            attachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                processor.getAPVTS(), paramId, knob));
        };

        makeKnob (spreadKnob,    "cm_spread");
        makeKnob (bpmKnob,       "cm_seq_bpm");
        makeKnob (swingKnob,     "cm_seq_swing");
        makeKnob (gateKnob,      "cm_seq_gate");
        makeKnob (humanizeKnob,  "cm_humanize");
        makeKnob (duckKnob,      "cm_sidechain_duck");

        // ENO mode toggle
        addAndMakeVisible (enoBtn);
        enoBtn.setButtonText ("ENO");
        enoBtn.setClickingTogglesState (true);
        enoBtn.onClick = [this]
        {
            if (auto* p = processor.getAPVTS().getParameter ("cm_eno_mode"))
                p->setValueNotifyingHost (enoBtn.getToggleState() ? 1.0f : 0.0f);
        };

        startTimerHz (10);  // Reduced from 15Hz — sufficient for step highlighting
    }

    ~ChordMachinePanel() override { stopTimer(); }

    void paint (juce::Graphics& g) override
    {
        using namespace GalleryColors;
        auto& cm = processor.getChordMachine();
        auto assignment = cm.getCurrentAssignment();

        // ── Chord Strip (4 slot cards) ──
        auto stripArea = getLocalBounds().reduced (6, 0)
                             .withY (kTopBarH + 4).withHeight (kStripH);

        int cardW = (stripArea.getWidth() - 18) / 4;
        for (int i = 0; i < 4; ++i)
        {
            auto card = stripArea.withX (stripArea.getX() + i * (cardW + 6)).withWidth (cardW);

            // Card background
            g.setColour (get (slotBg()));
            g.fillRoundedRectangle (card.toFloat(), 4.0f);
            g.setColour (get (borderGray()));
            g.drawRoundedRectangle (card.toFloat(), 4.0f, 1.0f);

            // Engine accent bar at bottom
            auto* eng = processor.getEngine (i);
            juce::Colour accent = eng ? accentForEngine (eng->getEngineId())
                                      : get (emptySlot());
            g.setColour (accent);
            g.fillRect (card.removeFromBottom (4).toFloat());

            // Slot number
            g.setColour (get (textMid()));
            g.setFont (GalleryFonts::body(10.0f));
            g.drawText ("S" + juce::String (i + 1), card.removeFromTop (14),
                        juce::Justification::centred);

            // MIDI note name
            g.setColour (get (textDark()));
            g.setFont (GalleryFonts::display(16.0f));
            g.drawText (ChordMachine::midiNoteToName (assignment.midiNotes[i]),
                        card.removeFromTop (22), juce::Justification::centred);

            // Engine name
            g.setColour (accent);
            g.setFont (GalleryFonts::body(10.0f));
            juce::String eName = eng ? eng->getEngineId() : "—";
            g.drawText (eName, card.removeFromTop (14), juce::Justification::centred);
        }

        // ── Step Grid (16 steps) ──
        auto gridArea = getLocalBounds().reduced (6, 0)
                            .withY (kTopBarH + kStripH + 10).withHeight (kGridH);

        int stepW = (gridArea.getWidth() - 30) / 16;
        int curStep = cm.getCurrentStep();
        bool seqOn = cm.isSequencerRunning();

        for (int s = 0; s < 16; ++s)
        {
            auto stepR = gridArea.withX (gridArea.getX() + s * (stepW + 2))
                                 .withWidth (stepW);

            auto stepData = cm.getStep (s);
            bool isActive = stepData.active;
            bool isCurrent = seqOn && (s == curStep);

            // Step cell
            juce::Colour cellCol;
            if (isCurrent && isActive)
                cellCol = get (xoGold);
            else if (isCurrent)
                cellCol = get (xoGold).withAlpha (0.3f);
            else if (isActive)
                cellCol = get (textDark()).withAlpha (0.15f);
            else
                cellCol = get (slotBg());

            g.setColour (cellCol);
            g.fillRoundedRectangle (stepR.toFloat().withTrimmedBottom (16), 3.0f);
            g.setColour (get (borderGray()));
            g.drawRoundedRectangle (stepR.toFloat().withTrimmedBottom (16), 3.0f, 0.5f);

            // Beat marker (steps 0, 4, 8, 12)
            if ((s & 3) == 0)
            {
                g.setColour (get (textMid()).withAlpha (0.4f));
                g.fillRect (stepR.getX(), stepR.getY() - 3, stepW, 2);
            }

            // Root note label below
            if (stepData.rootNote >= 0)
            {
                g.setColour (get (textMid()));
                g.setFont (GalleryFonts::label(8.0f));
                g.drawText (ChordMachine::midiNoteToName (stepData.rootNote),
                            stepR.withY (stepR.getBottom() - 14).withHeight (14),
                            juce::Justification::centred);
            }
        }

        // ── Knob Labels ──
        auto knobArea = getLocalBounds().reduced (6, 0)
                            .withY (kTopBarH + kStripH + kGridH + 16)
                            .withHeight (kKnobH);

        static const char* knobLabels[] = { "SPREAD", "BPM", "SWING", "GATE", "HUMAN", "DUCK" };
        int knobW = (knobArea.getWidth() - 5 * 8) / 6;
        for (int i = 0; i < 6; ++i)
        {
            auto labelR = knobArea.withX (knobArea.getX() + i * (knobW + 8))
                                  .withWidth (knobW).removeFromTop (14);
            g.setColour (get (textMid()));
            g.setFont (GalleryFonts::body(9.0f));
            g.drawText (knobLabels[i], labelR, juce::Justification::centred);
        }

        // Spread label (dynamic)
        float curSpread = cm.getSpread();
        g.setColour (get (xoGold));
        g.setFont (GalleryFonts::label(8.0f).withStyle (juce::Font::bold));
        auto spreadLabelR = knobArea.withWidth (knobW).removeFromBottom (12);
        g.drawText (ChordMachine::spreadLabel (curSpread), spreadLabelR,
                    juce::Justification::centred);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (6, 0);

        // ── Top control bar ──
        auto top = area.removeFromTop (kTopBarH);
        enableBtn .setBounds (top.removeFromLeft (44).reduced (2));
        paletteBox.setBounds (top.removeFromLeft (90).reduced (2));
        voicingBox.setBounds (top.removeFromLeft (100).reduced (2));
        seqBtn    .setBounds (top.removeFromLeft (44).reduced (2));
        patternBox.setBounds (top.removeFromLeft (85).reduced (2));
        top.removeFromLeft (8);
        velCurveBox.setBounds (top.removeFromLeft (95).reduced (2));
        top.removeFromLeft (4);
        enoBtn.setBounds (top.removeFromLeft (44).reduced (2));

        // ── Knobs ──
        auto knobArea = getLocalBounds().reduced (6, 0)
                            .withY (kTopBarH + kStripH + kGridH + 16)
                            .withHeight (kKnobH);

        int knobW = (knobArea.getWidth() - 5 * 8) / 6;
        juce::Slider* knobs[] = { &spreadKnob, &bpmKnob, &swingKnob,
                                   &gateKnob, &humanizeKnob, &duckKnob };
        for (int i = 0; i < 6; ++i)
        {
            auto r = knobArea.withX (knobArea.getX() + i * (knobW + 8))
                             .withWidth (knobW);
            knobs[i]->setBounds (r.withTrimmedTop (14));
        }
    }

private:
    void timerCallback() override
    {
        // Only repaint when sequencer step changes (avoids full redraw at 10Hz)
        int step = processor.getChordMachine().getCurrentStep();
        if (step != lastPaintedStep)
        {
            lastPaintedStep = step;
            repaint();
        }
    }
    int lastPaintedStep = -1;

    static constexpr int kTopBarH = 30;
    static constexpr int kStripH  = 80;
    static constexpr int kGridH   = 70;
    static constexpr int kKnobH   = 90;

    XOlokunProcessor& processor;

    juce::TextButton enableBtn, seqBtn, enoBtn;
    juce::ComboBox paletteBox, voicingBox, patternBox, velCurveBox;
    juce::Slider spreadKnob, bpmKnob, swingKnob, gateKnob, humanizeKnob, duckKnob;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordMachinePanel)
};

//==============================================================================
// PerformanceViewPanel — real-time coupling performance interface.
//
// Layout:
//   ┌──────────────────────────────────────────────────────┐
//   │  [P] PERFORMANCE VIEW                   [BAKE] [X]  │
//   ├───────────────────────┬──────────────────────────────┤
//   │                       │  Route Detail Panel          │
//   │  CouplingVisualizer   │  Route 1: [Src] → [Tgt]     │
//   │  (diamond graph viz)  │  Type: [dropdown]  Depth: ═  │
//   │                       │  Route 2 / 3 / 4 ...         │
//   ├───────────────────────┴──────────────────────────────┤
//   │  ◉ CHARACTER  ◉ MOVEMENT  ◉ COUPLING  ◉ SPACE       │
//   └──────────────────────────────────────────────────────┘
//
// Fills the same bounds as OverviewPanel / EngineDetailPanel / ChordMachinePanel.
// Toggle via "P" header button, using the same fade animation pattern.
//
class PerformanceViewPanel : public juce::Component
{
public:
    PerformanceViewPanel (XOlokunProcessor& proc)
        : processor (proc),
          apvts (proc.getAPVTS()),
          couplingVisualizer (proc.getCouplingMatrix(),
                              [&proc] (int slot) -> juce::String
                              {
                                  auto* eng = proc.getEngine (slot);
                                  return eng ? eng->getEngineId() : juce::String{};
                              },
                              [&proc] (int slot) -> juce::Colour
                              {
                                  auto* eng = proc.getEngine (slot);
                                  return eng ? eng->getAccentColour()
                                             : juce::Colour (0xFF555555);
                              })
    {
        setTitle ("Performance View");
        setDescription ("Real-time coupling performance interface with route controls and macro knobs");

        addAndMakeVisible (couplingVisualizer);
        couplingVisualizer.start();

        // ── BAKE button ──
        addAndMakeVisible (bakeBtn);
        bakeBtn.setButtonText ("BAKE");
        bakeBtn.setTooltip ("Save current coupling performance state as a reusable preset");
        bakeBtn.setEnabled (true);
        A11y::setup (bakeBtn, "Bake Coupling", "Save current coupling overlay as a coupling preset");
        bakeBtn.setColour (juce::TextButton::buttonColourId,
                           GalleryColors::get (GalleryColors::xoGold).withAlpha (0.15f));
        bakeBtn.setColour (juce::TextButton::textColourOffId,
                           GalleryColors::get (GalleryColors::xoGold));
        bakeBtn.onClick = [this] { handleBake(); };

        // ── CLEAR button — reset overlay to inactive ──
        addAndMakeVisible (clearBtn);
        clearBtn.setButtonText ("CLR");
        clearBtn.setTooltip ("Clear all performance coupling routes");
        A11y::setup (clearBtn, "Clear Coupling", "Deactivate all performance coupling routes");
        clearBtn.setColour (juce::TextButton::buttonColourId,
                            GalleryColors::get (GalleryColors::slotBg()));
        clearBtn.setColour (juce::TextButton::textColourOffId,
                            GalleryColors::get (GalleryColors::textMid()));
        clearBtn.onClick = [this] {
            processor.getCouplingPresetManager().clearOverlay();
            couplingVisualizer.refresh();
            repaint();
        };

        // ── Coupling preset selector dropdown ──
        addAndMakeVisible (couplingPresetBox);
        couplingPresetBox.setTextWhenNothingSelected ("Coupling Presets...");
        couplingPresetBox.setTooltip ("Load a saved coupling preset");
        A11y::setup (couplingPresetBox, "Coupling Preset Selector",
                     "Choose a saved coupling performance preset to load");
        couplingPresetBox.setColour (juce::ComboBox::backgroundColourId,
                                     GalleryColors::get (GalleryColors::shellWhite()));
        couplingPresetBox.setColour (juce::ComboBox::outlineColourId,
                                     GalleryColors::get (GalleryColors::borderGray()));
        couplingPresetBox.setColour (juce::ComboBox::textColourId,
                                     GalleryColors::get (GalleryColors::textDark()));
        couplingPresetBox.onChange = [this] { handleCouplingPresetSelected(); };
        refreshCouplingPresetList();

        // ── Route sections (4 routes) ──
        for (int r = 0; r < kNumRoutes; ++r)
        {
            auto& section = routes[r];
            juce::String prefix = "cp_r" + juce::String (r + 1) + "_";

            // Active toggle
            section.activeBtn.setButtonText ("ON");
            section.activeBtn.setClickingTogglesState (true);
            A11y::setup (section.activeBtn, "Route " + juce::String (r + 1) + " Enable");
            section.activeBtn.setColour (juce::TextButton::buttonColourId,
                                         GalleryColors::get (GalleryColors::slotBg()));
            section.activeBtn.setColour (juce::TextButton::buttonOnColourId,
                                         GalleryColors::get (GalleryColors::xoGold));
            section.activeBtn.setColour (juce::TextButton::textColourOffId,
                                         GalleryColors::get (GalleryColors::textMid()));
            section.activeBtn.setColour (juce::TextButton::textColourOnId,
                                         GalleryColors::get (GalleryColors::textDark()));
            section.activeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
                apvts, prefix + "active", section.activeBtn);
            addAndMakeVisible (section.activeBtn);

            // Coupling type combo (14 types from CouplingType enum)
            section.typeBox.addItemList ({
                "AmpToFilter", "AmpToPitch", "LFOToPitch", "EnvToMorph",
                "AudioToFM", "AudioToRing", "FilterToFilter", "AmpToChoke",
                "RhythmToBlend", "EnvToDecay", "PitchToPitch", "AudioToWavetable",
                "AudioToBuffer", "KnotTopology"
            }, 1);
            A11y::setup (section.typeBox, "Route " + juce::String (r + 1) + " Coupling Type");
            section.typeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
                apvts, prefix + "type", section.typeBox);
            addAndMakeVisible (section.typeBox);

            // Depth slider (bipolar -1.0 to 1.0)
            section.depthSlider.setSliderStyle (juce::Slider::LinearHorizontal);
            section.depthSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 16);
            section.depthSlider.setColour (juce::Slider::thumbColourId,
                                           GalleryColors::get (GalleryColors::xoGold));
            section.depthSlider.setColour (juce::Slider::trackColourId,
                                           GalleryColors::get (GalleryColors::borderGray()));
            section.depthSlider.setTooltip ("Route " + juce::String (r + 1) + " depth (bipolar)");
            A11y::setup (section.depthSlider, "Route " + juce::String (r + 1) + " Depth");
            section.depthAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                apvts, prefix + "amount", section.depthSlider);
            addAndMakeVisible (section.depthSlider);

            // Source slot selector
            section.sourceBox.addItemList ({ "Slot 1", "Slot 2", "Slot 3", "Slot 4" }, 1);
            A11y::setup (section.sourceBox, "Route " + juce::String (r + 1) + " Source Slot");
            section.sourceAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
                apvts, prefix + "source", section.sourceBox);
            addAndMakeVisible (section.sourceBox);

            // Target slot selector
            section.targetBox.addItemList ({ "Slot 1", "Slot 2", "Slot 3", "Slot 4" }, 1);
            A11y::setup (section.targetBox, "Route " + juce::String (r + 1) + " Target Slot");
            section.targetAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
                apvts, prefix + "target", section.targetBox);
            addAndMakeVisible (section.targetBox);

            // Route label
            section.label.setFont (GalleryFonts::heading (9.0f));
            section.label.setColour (juce::Label::textColourId,
                                     GalleryColors::get (GalleryColors::textDark()));
            section.label.setJustificationType (juce::Justification::centredLeft);
            addAndMakeVisible (section.label);
        }

        // ── Macro knobs (performance context duplicate) ──
        struct MacroDef { const char* id; const char* label; };
        static constexpr MacroDef macroDefs[4] = {
            { "macro1", "CHARACTER" }, { "macro2", "MOVEMENT" },
            { "macro3", "COUPLING" }, { "macro4", "SPACE" }
        };
        for (int i = 0; i < 4; ++i)
        {
            macroKnobs[i].setSliderStyle (juce::Slider::RotaryVerticalDrag);
            macroKnobs[i].setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            macroKnobs[i].setColour (juce::Slider::rotarySliderFillColourId,
                                     GalleryColors::get (GalleryColors::xoGold));
            macroKnobs[i].setTooltip (juce::String ("Macro ") + juce::String (i + 1)
                                      + ": " + macroDefs[i].label);
            A11y::setup (macroKnobs[i], juce::String ("Macro ") + juce::String (i + 1)
                                        + " " + macroDefs[i].label);
            addAndMakeVisible (macroKnobs[i]);
            macroAttach[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                apvts, macroDefs[i].id, macroKnobs[i]);

            macroLabels[i].setText (macroDefs[i].label, juce::dontSendNotification);
            macroLabels[i].setFont (GalleryFonts::heading (8.0f));
            macroLabels[i].setColour (juce::Label::textColourId,
                                      GalleryColors::get (GalleryColors::textMid()));
            macroLabels[i].setJustificationType (juce::Justification::centred);
            addAndMakeVisible (macroLabels[i]);
        }

        updateRouteLabels();
    }

    // Call from editor timer or on engine change to keep route labels current.
    void refresh()
    {
        couplingVisualizer.refresh();
        updateRouteLabels();
        repaint();
    }

    // Refresh the coupling preset dropdown from the CouplingPresetManager library.
    void refreshCouplingPresetList()
    {
        couplingPresetBox.clear (juce::dontSendNotification);
        auto& cpm = processor.getCouplingPresetManager();
        auto names = cpm.getPresetNames();
        if (names.isEmpty())
        {
            couplingPresetBox.setTextWhenNothingSelected ("No presets saved");
        }
        else
        {
            couplingPresetBox.setTextWhenNothingSelected ("Coupling Presets...");
            for (int i = 0; i < names.size(); ++i)
                couplingPresetBox.addItem (names[i], i + 1);
        }
    }

    void paint (juce::Graphics& g) override
    {
        using namespace GalleryColors;
        g.fillAll (get (slotBg()));

        // ── Header bar ──
        auto headerArea = getLocalBounds().removeFromTop (kHeaderH).toFloat();
        g.setColour (get (xoGold));
        g.fillRect (headerArea);

        g.setColour (juce::Colours::white);
        g.setFont (GalleryFonts::display (14.0f));
        g.drawText ("PERFORMANCE VIEW", headerArea.reduced (12, 0),
                     juce::Justification::centredLeft);

        // ── BAKE flash feedback ──
        if (bakeFlashAlpha > 0.0f)
        {
            g.setColour (get (xoGold).withAlpha (bakeFlashAlpha));
            g.fillRect (getLocalBounds().toFloat());
        }

        // ── Separator between coupling strip and route panel ──
        auto body = getLocalBounds().withTrimmedTop (kHeaderH).withTrimmedBottom (kMacroStripH);
        int sepX = body.getX() + (int) (body.getWidth() * kStripRatio);
        g.setColour (get (borderGray()));
        g.drawVerticalLine (sepX, (float) body.getY(), (float) body.getBottom());

        // ── Route section backgrounds ──
        auto routeArea = body.withTrimmedLeft ((int) (body.getWidth() * kStripRatio) + 1)
                             .reduced (6, 4);
        int routeH = routeArea.getHeight() / kNumRoutes;

        for (int r = 0; r < kNumRoutes; ++r)
        {
            auto sectionRect = routeArea.withY (routeArea.getY() + r * routeH)
                                        .withHeight (routeH).reduced (0, 2).toFloat();

            // Subtle rounded rect background per route
            g.setColour (get (shellWhite()).withAlpha (0.5f));
            g.fillRoundedRectangle (sectionRect, 4.0f);
            g.setColour (get (borderGray()).withAlpha (0.4f));
            g.drawRoundedRectangle (sectionRect, 4.0f, 0.5f);
        }

        // ── Macro strip separator ──
        auto macroStripArea = getLocalBounds().removeFromBottom (kMacroStripH).toFloat();
        g.setColour (get (borderGray()));
        g.drawHorizontalLine ((int) macroStripArea.getY(), 0.0f, (float) getWidth());

        // ── Macro strip background ──
        g.setColour (get (shellWhite()));
        g.fillRect (macroStripArea);

        // ── XO Gold accent at top of macro strip ──
        g.setColour (get (xoGold));
        g.fillRect (macroStripArea.removeFromTop (2.0f));

        // ── Macro strip header ──
        g.setColour (get (textMid()));
        g.setFont (GalleryFonts::heading (8.0f));
        g.drawText ("MACROS", getLocalBounds().removeFromBottom (kMacroStripH)
                                              .removeFromTop (14),
                     juce::Justification::centred);
    }

    void resized() override
    {
        auto area = getLocalBounds();

        // ── Header ──
        auto header = area.removeFromTop (kHeaderH);
        bakeBtn.setBounds (header.removeFromRight (60).reduced (8, 6));
        clearBtn.setBounds (header.removeFromRight (42).reduced (4, 6));
        header.removeFromRight (4);  // spacing
        couplingPresetBox.setBounds (header.removeFromRight (160).reduced (4, 6));

        // ── Macro strip at bottom ──
        auto macroArea = area.removeFromBottom (kMacroStripH).reduced (8, 4);
        macroArea.removeFromTop (14); // space for "MACROS" label
        int macroKnobW = macroArea.getWidth() / 4;
        for (int i = 0; i < 4; ++i)
        {
            auto col = macroArea.removeFromLeft (macroKnobW);
            int kh = 44;
            int kx = col.getCentreX() - kh / 2;
            macroKnobs[i].setBounds (kx, col.getY(), kh, kh);
            macroLabels[i].setBounds (kx, col.getY() + kh + 1, kh, 12);
        }

        // ── Body: coupling strip (left 40%) + route panel (right 60%) ──
        auto body = area;
        int stripW = (int) (body.getWidth() * kStripRatio);
        couplingVisualizer.setBounds (body.removeFromLeft (stripW));

        // ── Route sections ──
        auto routePanel = body.reduced (6, 4);
        int routeH = routePanel.getHeight() / kNumRoutes;

        for (int r = 0; r < kNumRoutes; ++r)
        {
            auto section = routePanel.withY (routePanel.getY() + r * routeH)
                                     .withHeight (routeH).reduced (4, 4);

            auto& rt = routes[r];

            // Row 1: label + active toggle
            auto row1 = section.removeFromTop (20);
            rt.activeBtn.setBounds (row1.removeFromRight (36).reduced (0, 1));
            rt.label.setBounds (row1);

            section.removeFromTop (2);

            // Row 2: source → target selectors
            auto row2 = section.removeFromTop (22);
            int halfW = (row2.getWidth() - 16) / 2;
            rt.sourceBox.setBounds (row2.removeFromLeft (halfW).reduced (0, 1));
            row2.removeFromLeft (16); // arrow spacing
            rt.targetBox.setBounds (row2.removeFromLeft (halfW).reduced (0, 1));

            section.removeFromTop (2);

            // Row 3: type dropdown + depth slider
            auto row3 = section.removeFromTop (22);
            rt.typeBox.setBounds (row3.removeFromLeft (row3.getWidth() * 2 / 5).reduced (0, 1));
            row3.removeFromLeft (4);
            rt.depthSlider.setBounds (row3.reduced (0, 1));
        }
    }

private:
    void updateRouteLabels()
    {
        for (int r = 0; r < kNumRoutes; ++r)
        {
            juce::String prefix = "cp_r" + juce::String (r + 1) + "_";

            // Read source/target slot from APVTS to display engine names
            int srcSlot = 0, tgtSlot = 1;
            if (auto* srcParam = apvts.getRawParameterValue (prefix + "source"))
                srcSlot = juce::roundToInt (srcParam->load());
            if (auto* tgtParam = apvts.getRawParameterValue (prefix + "target"))
                tgtSlot = juce::roundToInt (tgtParam->load());

            juce::String srcName = "\xe2\x80\x94";
            juce::String tgtName = "\xe2\x80\x94";
            if (auto* eng = processor.getEngine (srcSlot))
                srcName = eng->getEngineId().toUpperCase();
            if (auto* eng = processor.getEngine (tgtSlot))
                tgtName = eng->getEngineId().toUpperCase();

            routes[r].label.setText (
                "Route " + juce::String (r + 1) + ":  " + srcName + "  \xe2\x86\x92  " + tgtName,
                juce::dontSendNotification);
        }
    }

    //==========================================================================
    // BAKE handler — capture overlay, prompt for name, save to disk
    //==========================================================================
    void handleBake()
    {
        auto& cpm = processor.getCouplingPresetManager();

        // Quick check: is there anything to bake?
        auto snapshot = cpm.bakeCurrent ("Preview");
        if (!snapshot.hasActiveRoutes())
        {
            // Nothing to bake — flash the button red briefly as feedback
            bakeBtn.setColour (juce::TextButton::buttonColourId, juce::Colour (0x40FF4444));
            juce::Timer::callAfterDelay (400, [safeThis = juce::Component::SafePointer<PerformanceViewPanel> (this)] {
                if (safeThis)
                {
                    safeThis->bakeBtn.setColour (juce::TextButton::buttonColourId,
                        GalleryColors::get (GalleryColors::xoGold).withAlpha (0.15f));
                    safeThis->repaint();
                }
            });
            return;
        }

        // Show a name input dialog
        auto* alertWindow = new juce::AlertWindow (
            "Bake Coupling Preset",
            "Enter a name for this coupling configuration:",
            juce::MessageBoxIconType::NoIcon, this);
        alertWindow->addTextEditor ("name", "Untitled Coupling", "Preset Name:");
        alertWindow->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
        alertWindow->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        alertWindow->enterModalState (true, juce::ModalCallbackFunction::create (
            [safeThis = juce::Component::SafePointer<PerformanceViewPanel> (this), alertWindow] (int result)
            {
                if (result == 1 && safeThis)
                {
                    auto presetName = alertWindow->getTextEditorContents ("name").trim();
                    if (presetName.isEmpty())
                        presetName = "Untitled Coupling";

                    safeThis->performBake (presetName);
                }
                delete alertWindow;
            }), false);
    }

    void performBake (const juce::String& presetName)
    {
        auto& cpm = processor.getCouplingPresetManager();

        // Capture the current overlay state
        auto state = cpm.bakeCurrent (presetName);
        state.author = "User";

        // Determine file path
        auto dir = CouplingPresetManager::getDefaultDirectory();
        auto sanitizedName = presetName.replaceCharacters (" /\\:*?\"<>|", "___________");
        auto file = dir.getChildFile (sanitizedName + ".xocoupling");

        // Handle name collisions — append a number if needed
        int suffix = 1;
        while (file.existsAsFile() && suffix < 100)
        {
            file = dir.getChildFile (sanitizedName + "_" + juce::String (suffix) + ".xocoupling");
            ++suffix;
        }

        // Save to disk
        if (cpm.saveToFile (file, state))
        {
            // Re-scan the library to pick up the new preset
            cpm.scanDirectory (CouplingPresetManager::getDefaultDirectory());
            refreshCouplingPresetList();

            // Visual feedback — XO Gold flash
            triggerBakeFlash();
        }
    }

    //==========================================================================
    // Coupling preset recall handler
    //==========================================================================
    void handleCouplingPresetSelected()
    {
        int selectedId = couplingPresetBox.getSelectedId();
        if (selectedId <= 0)
            return;

        int index = selectedId - 1;
        auto& cpm = processor.getCouplingPresetManager();
        const auto* preset = cpm.getPreset (index);
        if (preset)
        {
            cpm.loadBakedCoupling (*preset);
            couplingVisualizer.refresh();
            updateRouteLabels();
            repaint();
        }
    }

    //==========================================================================
    // BAKE flash animation — brief XO Gold overlay that fades out
    //==========================================================================
    void triggerBakeFlash()
    {
        bakeFlashAlpha = 0.3f;
        repaint();

        // Fade out over 500ms using a timer callback chain
        fadeOutBakeFlash();
    }

    void fadeOutBakeFlash()
    {
        juce::Timer::callAfterDelay (50, [safeThis = juce::Component::SafePointer<PerformanceViewPanel> (this)] {
            if (!safeThis) return;

            safeThis->bakeFlashAlpha -= 0.05f;
            if (safeThis->bakeFlashAlpha <= 0.0f)
            {
                safeThis->bakeFlashAlpha = 0.0f;
                safeThis->repaint();
                return;
            }

            safeThis->repaint();
            safeThis->fadeOutBakeFlash();
        });
    }

    static constexpr int kHeaderH     = 34;
    static constexpr int kMacroStripH = 80;
    static constexpr int kNumRoutes   = 4;
    static constexpr float kStripRatio = 0.40f;

    XOlokunProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;

    // Left panel: full-graph coupling visualizer (replaces CouplingStripEditor)
    CouplingVisualizer couplingVisualizer;

    // Header controls
    juce::TextButton bakeBtn;
    juce::TextButton clearBtn;
    juce::ComboBox   couplingPresetBox;

    // BAKE flash animation state
    float bakeFlashAlpha = 0.0f;

    // Per-route controls
    struct RouteSection
    {
        juce::Label      label;
        juce::TextButton activeBtn;
        juce::ComboBox   typeBox;
        juce::Slider     depthSlider;
        juce::ComboBox   sourceBox;
        juce::ComboBox   targetBox;

        // APVTS attachments — must be destroyed before the controls they reference.
        // Declared after controls so they are destroyed first (reverse declaration order).
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   activeAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   depthAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> targetAttach;
    };
    std::array<RouteSection, kNumRoutes> routes;

    // Bottom macro strip
    std::array<juce::Slider, 4> macroKnobs;
    std::array<juce::Label, 4>  macroLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> macroAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PerformanceViewPanel)
};

//==============================================================================
// CouplingArcOverlay — transparent component drawn on top of the tile sidebar.
//
// Renders pulsing bioluminescent Bézier arcs between engine tile centres for
// every active coupling route in the MegaCouplingMatrix. The overlay is fully
// transparent to mouse events; clicks pass through to the tiles beneath.
//
// Usage:
//   - Construct with the XOlokunProcessor reference.
//   - Call setTileCenter(slot, centre) after tile bounds are set in resized().
//   - Add to the editor and set its bounds to cover the full editor area.
//   - The 30Hz timer drives continuous repaint; no other refresh needed.
//
// Arc geometry:
//   Control points bow 60px to the LEFT of the tile column so arcs never
//   overlap the right-side engine detail panel.
//
// Color coding by coupling type:
//   Audio routes  (FM / Ring / Wavetable / Buffer) → Twilight Blue   #0096C7
//   Modulation    (Amp / LFO / Env / Filter / Pitch / Rhythm)        → XO Gold #E9C46A
//   KnotTopology  (bidirectional entanglement)                        → Midnight Violet #7B2FBE
//
// Two-pass glow painting (Bézier cubic):
//   Pass 1: arcColor @ 0.08 alpha, 6px stroke — soft outer glow halo
//   Pass 2: arcColor @ glowAlpha, 2px stroke  — bright animated core
//   glowAlpha = 0.45 + 0.25 * sin(pulsePhase)  → range [0.20, 0.70]
//   pulsePhase advances 0.08 rad / timer tick (≈ 2.4 rad/s, ≈ 0.38 Hz)
//
class CouplingArcOverlay : public juce::Component, private juce::Timer
{
public:
    explicit CouplingArcOverlay(XOlokunProcessor& proc) : processor(proc)
    {
        setInterceptsMouseClicks(false, false); // pass-through to tiles beneath
        startTimerHz(30);
    }

    ~CouplingArcOverlay() override { stopTimer(); }

    // Called by XOlokunEditor::resized() once tile positions are finalised.
    // Centre is in the LOCAL coordinate space of this overlay component.
    void setTileCenter(int slot, juce::Point<float> centre)
    {
        if (slot >= 0 && slot < MegaCouplingMatrix::MaxSlots)
            tileCenters[static_cast<size_t>(slot)] = centre;
    }

    void timerCallback() override { repaint(); }

    void paint(juce::Graphics& g) override
    {
        // Snapshot coupling routes on the message thread — safe (getRoutes() uses atomic_load)
        const auto routes = processor.getCouplingMatrix().getRoutes();
        if (routes.empty())
            return;

        // Build index: key = (src << 2) | dst, value = max amount seen in this frame.
        // Collapse multiple routes on the same pair into one arc (visually cleaner).
        // We keep track of the dominant CouplingType for color selection.
        struct ArcInfo {
            float      amount = 0.0f;
            CouplingType type  = CouplingType::AmpToFilter;
        };
        std::array<ArcInfo, 12> arcMap {}; // 4 slots × 3 destination bits → 12 max pairs

        int arcCount = 0; // number of unique active pairs
        std::array<std::pair<int,int>, 12> arcPairs {};

        for (const auto& route : routes)
        {
            if (!route.active || route.amount < 0.001f)
                continue;
            if (route.sourceSlot < 0 || route.sourceSlot >= MegaCouplingMatrix::MaxSlots)
                continue;
            if (route.destSlot < 0 || route.destSlot >= MegaCouplingMatrix::MaxSlots)
                continue;

            // Canonical ordering: always store with lower slot first so we don't
            // draw two arcs between the same pair (forward + reverse KnotTopology).
            int lo = juce::jmin(route.sourceSlot, route.destSlot);
            int hi = juce::jmax(route.sourceSlot, route.destSlot);
            if (lo == hi) continue; // same slot — skip

            // Map (lo, hi) to a flat index.  4 slots → max 6 unique pairs.
            // We use a simple linear search over the live arcPairs array
            // because the count is always ≤ 6 — no hashmap overhead.
            int found = -1;
            for (int k = 0; k < arcCount; ++k)
                if (arcPairs[static_cast<size_t>(k)].first == lo && arcPairs[static_cast<size_t>(k)].second == hi)
                    { found = k; break; }

            if (found < 0 && arcCount < 12)
            {
                found = arcCount++;
                arcPairs[static_cast<size_t>(found)] = { lo, hi };
            }

            if (found >= 0)
            {
                auto& ai = arcMap[static_cast<size_t>(found)];
                if (route.amount > ai.amount)
                {
                    ai.amount = route.amount;
                    ai.type   = route.type;
                }
            }
        }

        if (arcCount == 0)
            return;

        for (int k = 0; k < arcCount; ++k)
        {
            const auto& [lo, hi] = arcPairs[static_cast<size_t>(k)];
            const auto& ai       = arcMap[static_cast<size_t>(k)];

            const auto from = tileCenters[static_cast<size_t>(lo)];
            const auto to   = tileCenters[static_cast<size_t>(hi)];

            // Skip degenerate arcs (tile center not yet set)
            if (from.isOrigin() && to.isOrigin())
                continue;

            // Choose arc color by coupling type category
            juce::Colour arcColor;
            switch (ai.type)
            {
                case CouplingType::AudioToFM:
                case CouplingType::AudioToRing:
                case CouplingType::AudioToWavetable:
                case CouplingType::AudioToBuffer:
                    arcColor = juce::Colour(0xFF0096C7); // Twilight Blue — audio-rate routes
                    break;
                case CouplingType::KnotTopology:
                    arcColor = juce::Colour(0xFF7B2FBE); // Midnight Violet — bidirectional entanglement
                    break;
                default:
                    arcColor = juce::Colour(0xFFE9C46A); // XO Gold — modulation routes
                    break;
            }

            // Bézier control points bow leftward (away from right panel)
            const float midX  = (from.x + to.x) * 0.5f - 60.0f;
            const juce::Point<float> cp1 (midX, from.y);
            const juce::Point<float> cp2 (midX, to.y);

            juce::Path arc;
            arc.startNewSubPath(from);
            arc.cubicTo(cp1, cp2, to);

            // Animate pulse — advance phase for this arc slot
            pulsePhase[static_cast<size_t>(k)] += 0.08f;
            if (pulsePhase[static_cast<size_t>(k)] > juce::MathConstants<float>::twoPi)
                pulsePhase[static_cast<size_t>(k)] -= juce::MathConstants<float>::twoPi;

            const float glowAlpha = 0.45f + 0.25f * std::sin(pulsePhase[static_cast<size_t>(k)]);

            // Pass 1: wide soft glow halo
            g.setColour(arcColor.withAlpha(0.08f));
            g.strokePath(arc, juce::PathStrokeType(6.0f));

            // Pass 2: bright animated core
            g.setColour(arcColor.withAlpha(glowAlpha));
            g.strokePath(arc, juce::PathStrokeType(2.0f));
        }
    }

private:
    XOlokunProcessor& processor;
    std::array<juce::Point<float>, MegaCouplingMatrix::MaxSlots> tileCenters {};
    float pulsePhase[12] {}; // one phase accumulator per unique arc pair (max 6 pairs, 12 for safety)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CouplingArcOverlay)
};

//==============================================================================
// XOlokunEditor — Gallery Model plugin window.
//
// Layout:
//   ┌─────────────────────────────────────┐
//   │  Header (title + tagline)           │
//   ├────────┬────────────────────────────┤
//   │ Tile 1 │                            │
//   │ Tile 2 │  Right panel               │
//   │ Tile 3 │  (OverviewPanel or         │
//   │ Tile 4 │   EngineDetailPanel)       │
//   ├────────┴────────────────────────────┤
//   │  MacroSection                       │
//   └─────────────────────────────────────┘
//
// Transition: 150ms opacity cross-fade via juce::ComponentAnimator
// when switching between overview and engine detail, or between engines.
//
class XOlokunEditor : public juce::AudioProcessorEditor,
                       private juce::Timer
{
public:
    explicit XOlokunEditor(XOlokunProcessor& proc)
        : AudioProcessorEditor(proc),
          processor(proc),
          overview(proc),
          detail(proc),
          chordPanel(proc),
          performancePanel(proc),
          macros(proc.getAPVTS()),
          masterFXStrip(proc.getAPVTS()),
          presetBrowser(proc)
    {
        laf = std::make_unique<GalleryLookAndFeel>();
        setLookAndFeel(laf.get());

        for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
        {
            tiles[i] = std::make_unique<CompactEngineTile>(proc, i);
            tiles[i]->onSelect = [this](int slot) { selectSlot(slot); };
            addAndMakeVisible(*tiles[i]);
        }

        addAndMakeVisible(fieldMap);
        addAndMakeVisible(overview);
        addAndMakeVisible(detail);
        addAndMakeVisible(chordPanel);
        addAndMakeVisible(performancePanel);
        addAndMakeVisible(macros);
        addAndMakeVisible(masterFXStrip);
        addAndMakeVisible(presetBrowser);

        // Coupling arc overlay: must be added AFTER tiles so it paints on top.
        // setInterceptsMouseClicks(false,false) means tiles still receive clicks.
        addAndMakeVisible(couplingArcs);

        detail.setVisible(false);
        detail.setAlpha(0.0f);
        chordPanel.setVisible(false);
        chordPanel.setAlpha(0.0f);
        performancePanel.setVisible(false);
        performancePanel.setAlpha(0.0f);

        // "CM" toggle button in header area
        addAndMakeVisible(cmToggleBtn);
        cmToggleBtn.setButtonText("CM");
        cmToggleBtn.setTooltip("Chord Machine — generative chord sequencer");
        A11y::setup (cmToggleBtn, "Chord Machine Toggle", "Toggle the chord machine sequencer panel");
        cmToggleBtn.setClickingTogglesState(true);
        cmToggleBtn.onClick = [this]
        {
            if (cmToggleBtn.getToggleState())
            {
                perfToggleBtn.setToggleState(false, juce::dontSendNotification);
                showChordMachine();
            }
            else
                showOverview();
        };

        // "P" toggle button in header area — Performance View
        addAndMakeVisible(perfToggleBtn);
        perfToggleBtn.setButtonText("P");
        perfToggleBtn.setTooltip("Performance View — real-time coupling control");
        A11y::setup (perfToggleBtn, "Performance View Toggle", "Toggle the coupling performance panel");
        perfToggleBtn.setClickingTogglesState(true);
        perfToggleBtn.onClick = [this]
        {
            if (perfToggleBtn.getToggleState())
            {
                cmToggleBtn.setToggleState(false, juce::dontSendNotification);
                showPerformanceView();
            }
            else
                showOverview();
        };

        // "PS" toggle button — PlaySurface (4-zone performance interface)
        addAndMakeVisible(surfaceToggleBtn);
        surfaceToggleBtn.setButtonText("PS");
        surfaceToggleBtn.setTooltip("PlaySurface — 4-zone performance interface (pads, orbit, strip, triggers)");
        A11y::setup (surfaceToggleBtn, "PlaySurface Toggle",
                     "Toggle the PlaySurface performance panel at the bottom of the editor");
        surfaceToggleBtn.setClickingTogglesState(true);
        surfaceToggleBtn.onClick = [this]
        {
            if (surfaceToggleBtn.getToggleState())
                showPlaySurface();
            else
                hidePlaySurface();
        };

        // PlaySurface — hidden by default; revealed via surfaceToggleBtn
        addAndMakeVisible(playSurface);
        playSurface.setVisible(false);
        playSurface.setAlpha(0.0f);

        // Wire MIDI: PlaySurface note events flow through the processor's
        // MidiMessageCollector, which is drained into processBlock each audio callback.
        playSurface.setMidiCollector(&proc.getMidiCollector(), 1);

        // Dark mode toggle — light is default (brand rule)
        addAndMakeVisible(themeToggleBtn);
        themeToggleBtn.setButtonText("D");
        themeToggleBtn.setTooltip("Toggle dark mode");
        A11y::setup (themeToggleBtn, "Dark Mode Toggle", "Switch between light and dark theme");
        themeToggleBtn.setClickingTogglesState(true);
        themeToggleBtn.onClick = [this]
        {
            GalleryColors::darkMode() = themeToggleBtn.getToggleState();
            laf->applyTheme();
            repaint();
            for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
                tiles[i]->repaint();
            overview.repaint();
            detail.repaint();
            chordPanel.repaint();
            performancePanel.repaint();
            macros.repaint();
            masterFXStrip.repaint();
            presetBrowser.repaint();
        };

        // Export button — launches ExportDialog as a CallOutBox
        addAndMakeVisible(exportBtn);
        exportBtn.setButtonText("XPN");
        exportBtn.setTooltip("Export presets as MPC-compatible XPN expansion pack");
        A11y::setup (exportBtn, "Export", "Open export dialog to build XPN expansion packs");
        exportBtn.onClick = [this]
        {
            juce::CallOutBox::launchAsynchronously(
                std::make_unique<ExportDialog>(
                    processor.getPresetManager(),
                    &processor.getAPVTS(),
                    &processor.getCouplingMatrix()),
                exportBtn.getScreenBounds(),
                getTopLevelComponent());
        };

        // Scan factory preset directory
        auto presetDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                             .getChildFile("Application Support/XO_OX/XOlokun/Presets");
        if (presetDir.isDirectory())
            proc.getPresetManager().scanPresetDirectory(presetDir);
        presetBrowser.setMacroSection(&macros); // wire preset macroLabels → macro knob labels
        presetBrowser.updateDisplay();

        // Event-driven tile refresh: only repaint the affected tile and overview on engine change.
        proc.onEngineChanged = [this](int slot)
        {
            if (slot >= 0 && slot < XOlokunProcessor::MaxSlots)
                tiles[slot]->refresh();
            overview.refresh();
            if (performancePanel.isVisible())
                performancePanel.refresh();
        };

        setSize(880, 562);
        setResizable(true, true);
        setResizeLimits(720, 460, 1400, 900);
        setWantsKeyboardFocus(true);
        setTitle ("XOlokun Synthesizer");
        setDescription ("Multi-engine synthesizer with cross-engine coupling. "
                        "Keys 1-4 select engine slots, Escape returns to overview.");
        startTimerHz(1); // Reduced from 5Hz — idle polling only as a fallback
    }

    ~XOlokunEditor() override
    {
        stopTimer();
        processor.onEngineChanged = nullptr; // prevent callback after editor is destroyed
        setLookAndFeel(nullptr);
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        // Keys 1-4 jump directly to engine slots
        for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
        {
            if (key == juce::KeyPress('1' + i))
            {
                if (processor.getEngine(i) != nullptr)
                    selectSlot(i);
                return true;
            }
        }
        // Escape returns to overview
        if (key == juce::KeyPress::escapeKey)
        {
            showOverview();
            return true;
        }
        return false;
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        g.fillAll(get(shellWhite()));

        // Header area
        auto header = getLocalBounds().removeFromTop(kHeaderH).toFloat();
        g.setColour(get(shellWhite()));
        g.fillRect(header);

        // XO Gold stripe
        g.setColour(get(xoGold));
        g.fillRect(header.removeFromBottom(3.0f));

        g.setColour(get(textDark()));
        g.setFont(GalleryFonts::display(19.0f));
        g.drawText("XOlokun",
                   juce::Rectangle<int>(16, 0, 160, kHeaderH - 3),
                   juce::Justification::centredLeft);

        g.setColour(get(xoGoldText())); // Darkened gold — meets WCAG AA on shellWhite
        g.setFont(GalleryFonts::heading(8.5f));
        g.drawText("XO_OX Designs",
                   juce::Rectangle<int>(16, kHeaderH - 18, 110, 12),
                   juce::Justification::centredLeft);

        // Active coupling route count in header
        {
            auto routes = processor.getCouplingMatrix().getRoutes();
            int activeRoutes = 0;
            for (const auto& r : routes) if (r.active && r.amount >= 0.001f) ++activeRoutes;

            juce::String routeLabel = (activeRoutes > 0)
                ? juce::String(activeRoutes) + " coupling route" + (activeRoutes > 1 ? "s" : "") + " active"
                : ([this]() -> juce::String {
                      int numEngines = (int)EngineRegistry::instance().getRegisteredIds().size();
                      int numCoupling = 14;
                      int numPresets = (int)processor.getPresetManager().getLibrary().size();
                      juce::String presetStr = (numPresets > 0)
                          ? juce::String(numPresets) + "+"
                          : "18000+";
                      return juce::String(numEngines) + " Engines \xc2\xb7 "
                           + juce::String(numCoupling) + " Coupling Types \xc2\xb7 "
                           + presetStr + " Presets";
                  }());

            g.setColour(activeRoutes > 0 ? get(xoGoldText()) : get(textMid()).withAlpha(0.5f));
            g.setFont(GalleryFonts::body(9.0f));
            g.drawText(routeLabel,
                       juce::Rectangle<int>(getWidth() - 310, 0, 298, kHeaderH - 6),
                       juce::Justification::centredRight);
        }

        // Sidebar separator
        int sepX = kSidebarW;
        g.setColour(get(borderGray()));
        g.drawVerticalLine(sepX, (float)kHeaderH, (float)(getHeight() - kMacroH));
    }

    void resized() override
    {
        auto area = getLocalBounds();

        // Header: reserve space for toggle buttons and preset browser (right side)
        auto header = area.removeFromTop(kHeaderH);
        presetBrowser.setBounds(header.removeFromRight(220).reduced(4, 10));
        exportBtn.setBounds(header.removeFromRight(46).reduced(4, 12));
        themeToggleBtn.setBounds(header.removeFromRight(32).reduced(2, 12));
        surfaceToggleBtn.setBounds(header.removeFromRight(36).reduced(2, 12));
        perfToggleBtn.setBounds(header.removeFromRight(32).reduced(2, 12));
        cmToggleBtn.setBounds(header.removeFromRight(42).reduced(4, 12));

        // PlaySurface — when visible it occupies ~40% of the content height at the
        // very bottom of the editor (below the macros + masterFX strips), spanning
        // the full editor width.  Its own dark palette (PS::kSurfaceBg 0xFF1A1A1A)
        // provides a clear visual break from the Gallery shell above.
        const int contentH = area.getHeight();
        const int surfaceH = (int)(contentH * 0.40f);  // 40% of content area
        if (playSurface.isVisible())
            playSurface.setBounds(area.removeFromBottom(surfaceH));
        else
            playSurface.setBounds(0, getHeight(), getWidth(), surfaceH); // parked below visible area

        // Bottom strips (from bottom up)
        masterFXStrip.setBounds(area.removeFromBottom(kMasterFXH).reduced(6, 3));
        macros.setBounds(area.removeFromBottom(kMacroH).reduced(6, 4));

        // Left sidebar tiles
        auto sidebar = area.removeFromLeft(kSidebarW);
        int tileH = sidebar.getHeight() / XOlokunProcessor::MaxSlots;
        for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
            tiles[i]->setBounds(sidebar.removeFromTop(tileH));

        // Field Map — fixed strip at the bottom of the right-panel area.
        // Always visible beneath the panel stack. Height matches ~2 octaves of screen.
        auto fieldMapArea = area.removeFromBottom(kFieldMapH);
        fieldMap.setBounds(fieldMapArea);

        // Right panels (stacked, only one visible at a time — above Field Map)
        overview.setBounds(area);
        detail.setBounds(area);
        chordPanel.setBounds(area);
        performancePanel.setBounds(area);

        // Coupling arc overlay covers the full editor so it can draw arcs across
        // the sidebar tile column.  Tile centres are expressed in local (overlay)
        // coordinates, which match editor-local coordinates because the overlay
        // is sized to the full editor bounds.
        couplingArcs.setBounds(getLocalBounds());
        {
            // Recompute tile centre positions in overlay-local coordinates.
            // Tiles were laid out above; retrieve their current bounds.
            for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
            {
                auto tileBounds = tiles[i]->getBounds(); // editor-local
                couplingArcs.setTileCenter(i, tileBounds.getCentre().toFloat());
            }
        }
    }

private:
    void selectSlot(int slot)
    {
        // Deselect all tiles
        for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
            tiles[i]->setSelected(i == slot);

        if (slot == selectedSlot && detail.isVisible())
            return; // already showing this one

        selectedSlot = slot;
        cmToggleBtn.setToggleState(false, juce::dontSendNotification);
        perfToggleBtn.setToggleState(false, juce::dontSendNotification);

        // Hide chord/performance panels if visible
        if (chordPanel.isVisible())
            chordPanel.setVisible(false);
        if (performancePanel.isVisible())
            performancePanel.setVisible(false);

        auto& anim = juce::Desktop::getInstance().getAnimator();

        // SafePointer prevents crash if editor is destroyed during animation
        juce::Component::SafePointer<XOlokunEditor> safeThis(this);

        if (detail.isVisible())
        {
            // Cross-fade: fade out → swap → fade in
            anim.fadeOut(&detail, kFadeMs);
            juce::Timer::callAfterDelay(kFadeMs, [safeThis, slot]
            {
                if (safeThis == nullptr) return;
                auto& self = *safeThis;
                if (self.detail.loadSlot(slot))
                {
                    self.overview.setVisible(false);
                    self.detail.setAlpha(0.0f);
                    self.detail.setVisible(true);
                    juce::Desktop::getInstance().getAnimator().fadeIn(&self.detail, kFadeMs);
                }
                else
                {
                    self.showOverview();
                }
            });
        }
        else
        {
            // Fade out overview, fade in detail
            anim.fadeOut(&overview, kFadeMs);
            juce::Timer::callAfterDelay(kFadeMs, [safeThis, slot]
            {
                if (safeThis == nullptr) return;
                auto& self = *safeThis;
                if (self.detail.loadSlot(slot))
                {
                    self.overview.setVisible(false);
                    self.detail.setAlpha(0.0f);
                    self.detail.setVisible(true);
                    juce::Desktop::getInstance().getAnimator().fadeIn(&self.detail, kFadeMs);
                }
                else
                {
                    self.overview.setAlpha(1.0f);
                    self.overview.setVisible(true);
                }
            });
        }
    }

    void showOverview()
    {
        selectedSlot = -1;
        for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
            tiles[i]->setSelected(false);
        cmToggleBtn.setToggleState(false, juce::dontSendNotification);
        perfToggleBtn.setToggleState(false, juce::dontSendNotification);

        auto& anim = juce::Desktop::getInstance().getAnimator();
        juce::Component* outgoing = detail.isVisible() ? static_cast<juce::Component*>(&detail)
                                  : chordPanel.isVisible() ? static_cast<juce::Component*>(&chordPanel)
                                  : performancePanel.isVisible() ? static_cast<juce::Component*>(&performancePanel)
                                  : nullptr;
        if (outgoing)
        {
            juce::Component::SafePointer<XOlokunEditor> safeThis(this);
            juce::Component::SafePointer<juce::Component> safeOutgoing(outgoing);
            anim.fadeOut(outgoing, kFadeMs);
            juce::Timer::callAfterDelay(kFadeMs, [safeThis, safeOutgoing]
            {
                if (safeThis == nullptr) return;
                if (safeOutgoing != nullptr) safeOutgoing->setVisible(false);
                safeThis->overview.setAlpha(0.0f);
                safeThis->overview.setVisible(true);
                juce::Desktop::getInstance().getAnimator().fadeIn(&safeThis->overview, kFadeMs);
            });
        }
    }

    void showChordMachine()
    {
        selectedSlot = -1;
        for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
            tiles[i]->setSelected(false);

        auto& anim = juce::Desktop::getInstance().getAnimator();
        juce::Component* outgoing = detail.isVisible() ? static_cast<juce::Component*>(&detail)
                                  : performancePanel.isVisible() ? static_cast<juce::Component*>(&performancePanel)
                                  : overview.isVisible() ? static_cast<juce::Component*>(&overview)
                                  : nullptr;
        if (outgoing)
        {
            juce::Component::SafePointer<XOlokunEditor> safeThis(this);
            juce::Component::SafePointer<juce::Component> safeOutgoing(outgoing);
            anim.fadeOut(outgoing, kFadeMs);
            juce::Timer::callAfterDelay(kFadeMs, [safeThis, safeOutgoing]
            {
                if (safeThis == nullptr) return;
                if (safeOutgoing != nullptr) safeOutgoing->setVisible(false);
                safeThis->chordPanel.setAlpha(0.0f);
                safeThis->chordPanel.setVisible(true);
                juce::Desktop::getInstance().getAnimator().fadeIn(&safeThis->chordPanel, kFadeMs);
            });
        }
        else
        {
            chordPanel.setAlpha(0.0f);
            chordPanel.setVisible(true);
            anim.fadeIn(&chordPanel, kFadeMs);
        }
    }

    void showPerformanceView()
    {
        selectedSlot = -1;
        for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
            tiles[i]->setSelected(false);

        performancePanel.refresh();

        auto& anim = juce::Desktop::getInstance().getAnimator();
        juce::Component* outgoing = detail.isVisible() ? static_cast<juce::Component*>(&detail)
                                  : chordPanel.isVisible() ? static_cast<juce::Component*>(&chordPanel)
                                  : overview.isVisible() ? static_cast<juce::Component*>(&overview)
                                  : nullptr;
        if (outgoing)
        {
            juce::Component::SafePointer<XOlokunEditor> safeThis(this);
            juce::Component::SafePointer<juce::Component> safeOutgoing(outgoing);
            anim.fadeOut(outgoing, kFadeMs);
            juce::Timer::callAfterDelay(kFadeMs, [safeThis, safeOutgoing]
            {
                if (safeThis == nullptr) return;
                if (safeOutgoing != nullptr) safeOutgoing->setVisible(false);
                safeThis->performancePanel.setAlpha(0.0f);
                safeThis->performancePanel.setVisible(true);
                juce::Desktop::getInstance().getAnimator().fadeIn(&safeThis->performancePanel, kFadeMs);
            });
        }
        else
        {
            performancePanel.setAlpha(0.0f);
            performancePanel.setVisible(true);
            anim.fadeIn(&performancePanel, kFadeMs);
        }
    }

    // ── PlaySurface show/hide ──────────────────────────────────────────────────
    // The PlaySurface overlays the bottom ~40% of the editor.  Showing it
    // triggers a resized() so the layout pushes up the macros/masterFX strips;
    // hiding it restores the original layout.  Both use the standard 150ms fade.

    void showPlaySurface()
    {
        auto& anim = juce::Desktop::getInstance().getAnimator();
        playSurface.setAlpha(0.0f);
        playSurface.setVisible(true);
        resized(); // recalculate layout so PlaySurface gets its bounds before fading in
        anim.fadeIn(&playSurface, kFadeMs);
    }

    void hidePlaySurface()
    {
        auto& anim = juce::Desktop::getInstance().getAnimator();
        juce::Component::SafePointer<XOlokunEditor> safeThis(this);
        anim.fadeOut(&playSurface, kFadeMs);
        juce::Timer::callAfterDelay(kFadeMs, [safeThis]
        {
            if (safeThis == nullptr) return;
            safeThis->playSurface.setVisible(false);
            safeThis->resized(); // restore layout once panel is hidden
        });
    }

    void timerCallback() override
    {
        for (int i = 0; i < XOlokunProcessor::MaxSlots; ++i)
            tiles[i]->refresh();
        if (!detail.isVisible())
            overview.refresh();
        if (performancePanel.isVisible())
            performancePanel.refresh();

        // Drain Field Map note events from the lock-free audio-thread queue.
        // Color is resolved here on the message thread (safe: getEngine / getAccentColour).
        // XO Gold is used as fallback when no engine occupies the slot.
        static const juce::Colour kXOGold = juce::Colour(0xFFE9C46A);
        processor.drainNoteEvents([&](const XOlokunProcessor::NoteMapEvent& ev)
        {
            juce::Colour colour = kXOGold;
            if (ev.slot >= 0 && ev.slot < XOlokunProcessor::MaxSlots)
            {
                if (auto* eng = processor.getEngine(ev.slot))
                    colour = eng->getAccentColour();
            }
            fieldMap.addNote(ev.midiNote, ev.velocity, colour);
        });
    }

    static constexpr int kHeaderH   = 50;
    static constexpr int kMacroH    = 105;
    static constexpr int kMasterFXH = 68;
    static constexpr int kSidebarW  = 155;
    static constexpr int kFadeMs    = 150;
    static constexpr int kFieldMapH = 110; // Field Map panel height (pixels)

    XOlokunProcessor& processor;
    std::unique_ptr<GalleryLookAndFeel> laf;

    std::array<std::unique_ptr<CompactEngineTile>, XOlokunProcessor::MaxSlots> tiles;
    FieldMapPanel          fieldMap;
    OverviewPanel          overview;
    EngineDetailPanel      detail;
    ChordMachinePanel      chordPanel;
    PerformanceViewPanel   performancePanel;
    MacroSection           macros;
    MasterFXSection        masterFXStrip;
    PresetBrowserStrip     presetBrowser;
    juce::TextButton       cmToggleBtn;
    juce::TextButton       perfToggleBtn;
    juce::TextButton       surfaceToggleBtn;
    juce::TextButton       themeToggleBtn;
    juce::TextButton       exportBtn;
    PlaySurface            playSurface;
    CouplingArcOverlay     couplingArcs { processor };

    int selectedSlot = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XOlokunEditor)
};

} // namespace xolokun
