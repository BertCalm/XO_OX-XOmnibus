#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../XOmnibusProcessor.h"
#include "../Core/EngineRegistry.h"

namespace xomnibus {

//==============================================================================
// Gallery Model — color constants
namespace GalleryColors {
    constexpr uint32_t shellWhite = 0xFFF8F6F3;
    constexpr uint32_t xoGold     = 0xFFE9C46A;
    constexpr uint32_t textDark   = 0xFF1A1A1A;
    constexpr uint32_t textMid    = 0xFF777570;
    constexpr uint32_t borderGray = 0xFFDDDAD5;
    constexpr uint32_t slotBg     = 0xFFFCFBF9;
    constexpr uint32_t emptySlot  = 0xFFEAE8E4;

    inline juce::Colour get(uint32_t hex) { return juce::Colour(hex); }

    inline juce::Colour accentForEngine(const juce::String& id)
    {
        if (id == "Snap")      return juce::Colour(0xFFC8553D);
        if (id == "Morph")     return juce::Colour(0xFF2A9D8F);
        if (id == "Dub")       return juce::Colour(0xFF6B7B3A);
        if (id == "Drift")     return juce::Colour(0xFF7B2D8B);
        if (id == "Bob")       return juce::Colour(0xFFE9A84A);
        if (id == "Fat")       return juce::Colour(0xFFFF1493);
        if (id == "Onset")     return juce::Colour(0xFF0066FF);
        if (id == "Overworld") return juce::Colour(0xFF39FF14);
        if (id == "Opal")      return juce::Colour(0xFFA78BFA);
        return get(borderGray);
    }

    inline juce::String prefixForEngine(const juce::String& id)
    {
        return id.toLowerCase();
    }
}

//==============================================================================
// GalleryLookAndFeel — Gallery Model visual language
class GalleryLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GalleryLookAndFeel()
    {
        using namespace GalleryColors;
        setColour(juce::Slider::rotarySliderFillColourId,        get(xoGold));
        setColour(juce::Slider::rotarySliderOutlineColourId,     get(borderGray));
        setColour(juce::Slider::textBoxTextColourId,             get(textMid));
        setColour(juce::Slider::textBoxBackgroundColourId,       juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxOutlineColourId,          juce::Colours::transparentBlack);
        setColour(juce::TextButton::buttonColourId,              get(shellWhite));
        setColour(juce::TextButton::buttonOnColourId,            get(xoGold));
        setColour(juce::TextButton::textColourOffId,             get(textDark));
        setColour(juce::ComboBox::backgroundColourId,            get(shellWhite));
        setColour(juce::ComboBox::outlineColourId,               get(borderGray));
        setColour(juce::PopupMenu::backgroundColourId,           get(shellWhite));
        setColour(juce::PopupMenu::textColourId,                 get(textDark));
        setColour(juce::PopupMenu::highlightedBackgroundColourId,get(xoGold).withAlpha(0.25f));
        setColour(juce::ScrollBar::thumbColourId,                get(borderGray));
        setColour(juce::Label::textColourId,                     get(textDark));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(6.0f);
        float r  = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        float cx = bounds.getCentreX(), cy = bounds.getCentreY();
        float fillAngle = startAngle + sliderPos * (endAngle - startAngle);

        juce::Path track;
        track.addCentredArc(cx, cy, r - 2, r - 2, 0, startAngle, endAngle, true);
        g.setColour(GalleryColors::get(GalleryColors::borderGray));
        g.strokePath(track, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

        juce::Path fill;
        fill.addCentredArc(cx, cy, r - 2, r - 2, 0, startAngle, fillAngle, true);
        auto accent = slider.findColour(juce::Slider::rotarySliderFillColourId);
        g.setColour(accent);
        g.strokePath(fill, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

        float tx = cx + (r - 6.0f) * std::sin(fillAngle);
        float ty = cy - (r - 6.0f) * std::cos(fillAngle);
        g.fillEllipse(tx - 3.5f, ty - 3.5f, 7.0f, 7.0f);

        g.setColour(GalleryColors::get(GalleryColors::shellWhite));
        g.fillEllipse(cx - r * 0.46f, cy - r * 0.46f, r * 0.92f, r * 0.92f);
        g.setColour(GalleryColors::get(GalleryColors::borderGray).withAlpha(0.5f));
        g.drawEllipse(cx - r * 0.46f, cy - r * 0.46f, r * 0.92f, r * 0.92f, 1.0f);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                              const juce::Colour& bg, bool highlighted, bool down) override
    {
        auto b = btn.getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(down ? GalleryColors::get(GalleryColors::xoGold)
                         : highlighted ? bg.brighter(0.06f) : bg);
        g.fillRoundedRectangle(b, 5.0f);
        g.setColour(GalleryColors::get(GalleryColors::borderGray));
        g.drawRoundedRectangle(b, 5.0f, 1.0f);
    }
};

//==============================================================================
// ParameterGrid — auto-generates knobs for all params matching a prefix.
// Mounted inside a Viewport for scrolling.
class ParameterGrid : public juce::Component
{
public:
    ParameterGrid(XOmnibusProcessor& proc,
                  const juce::String& enginePrefix,
                  juce::Colour accentColour)
    {
        auto& apvts = proc.getAPVTS();
        auto& rawParams = proc.getParameters(); // juce::AudioProcessor::getParameters()
        juce::String pfx = enginePrefix + "_";

        for (auto* p : rawParams)
        {
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
            {
                juce::String pid = rp->getParameterID();
                if (!pid.startsWith(pfx))
                    continue;

                auto inner = pid.substring(pfx.length()); // e.g. "filterCutoff"
                juce::String shortLabel = makeShortLabel(inner);

                auto slider = std::make_unique<juce::Slider>();
                slider->setSliderStyle(juce::Slider::RotaryVerticalDrag);
                slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
                slider->setColour(juce::Slider::rotarySliderFillColourId, accentColour);
                slider->setTooltip(rp->getName(64));
                addAndMakeVisible(*slider);

                auto label = std::make_unique<juce::Label>();
                label->setText(shortLabel, juce::dontSendNotification);
                label->setFont(juce::Font(8.0f));
                label->setColour(juce::Label::textColourId,
                                 GalleryColors::get(GalleryColors::textMid));
                label->setJustificationType(juce::Justification::centred);
                label->setInterceptsMouseClicks(false, false);
                addAndMakeVisible(*label);

                attachments.push_back(
                    std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                        apvts, pid, *slider));

                sliders.push_back(std::move(slider));
                labels.push_back(std::move(label));
            }
        }
    }

    int getRequiredHeight(int availableWidth) const
    {
        int cols = juce::jmax(1, availableWidth / kCellW);
        int rows = ((int)sliders.size() + cols - 1) / cols;
        return rows * kCellH + kPad * 2;
    }

    void resized() override
    {
        int cols = juce::jmax(1, getWidth() / kCellW);
        int offsetX = (getWidth() - cols * kCellW) / 2;

        for (int i = 0; i < (int)sliders.size(); ++i)
        {
            int col = i % cols;
            int row = i / cols;
            int cx = offsetX + col * kCellW;
            int cy = kPad + row * kCellH;

            sliders[i]->setBounds(cx + 6, cy + 4, kCellW - 12, kCellW - 12);
            labels[i]->setBounds(cx, cy + kCellW - 4, kCellW, 14);
        }
    }

private:
    // "filterCutoff" → "CUTOFF", "level" → "LEVEL", "ampAttack" → "ATTACK"
    static juce::String makeShortLabel(const juce::String& inner)
    {
        for (int i = 1; i < inner.length(); ++i)
            if (juce::CharacterFunctions::isUpperCase(inner[i]))
                return inner.substring(i).toUpperCase();
        return inner.toUpperCase();
    }

    static constexpr int kCellW = 82;
    static constexpr int kCellH = 90;
    static constexpr int kPad   = 12;

    // Destruction order matters: attachments first, then sliders.
    // Members are destroyed in reverse declaration order, so declare
    // sliders/labels before attachments.
    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::Label>>  labels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterGrid)
};

//==============================================================================
// EngineDetailPanel — right-side parameter view for one engine slot.
// Contains a scrollable ParameterGrid plus header with engine identity.
class EngineDetailPanel : public juce::Component
{
public:
    explicit EngineDetailPanel(XOmnibusProcessor& proc) : processor(proc) {}

    // Called when the user selects an engine slot to inspect.
    // Returns false if the slot is empty.
    bool loadSlot(int slot)
    {
        auto* eng = processor.getEngine(slot);
        if (!eng) return false;

        engineId     = eng->getEngineId();
        accentColour = eng->getAccentColour();

        // Rebuild parameter grid for this engine
        auto prefix = GalleryColors::prefixForEngine(engineId);
        auto* newGrid = new ParameterGrid(processor, prefix, accentColour);
        viewport.setViewedComponent(newGrid, /*takeOwnership=*/true);

        int gridW = juce::jmax(200, viewport.getWidth()
                                    - viewport.getScrollBarThickness() - 4);
        newGrid->setSize(gridW, newGrid->getRequiredHeight(gridW));
        viewport.setViewPosition(0, 0);

        repaint();
        return true;
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        g.fillAll(get(slotBg));

        // Accent header bar
        g.setColour(accentColour);
        g.fillRect(0, 0, getWidth(), kHeaderH);

        // Engine name
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(16.0f).boldened());
        g.drawText(engineId.toUpperCase(),
                   kHeaderH + 8, 0, getWidth() - kHeaderH - 90, kHeaderH,
                   juce::Justification::centredLeft);

        // Slot label
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.setFont(juce::Font(9.0f));
        g.drawText("PARAMETERS", 0, 0, getWidth() - 16, kHeaderH,
                   juce::Justification::centredRight);

        // Subtle separator below header
        g.setColour(get(borderGray));
        g.drawHorizontalLine(kHeaderH, 0.0f, (float)getWidth());
    }

    void resized() override
    {
        viewport.setBounds(0, kHeaderH, getWidth(), getHeight() - kHeaderH);

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

    XOmnibusProcessor& processor;
    juce::Viewport     viewport;
    juce::String       engineId  { "—" };
    juce::Colour       accentColour { GalleryColors::get(GalleryColors::borderGray) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineDetailPanel)
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
    explicit OverviewPanel(XOmnibusProcessor& proc) : processor(proc) {}

    // Called by the editor when engine state or coupling routes change.
    // Avoids calling getRoutes() (which copies a vector) inside paint().
    void refresh()
    {
        cachedRoutes = processor.getCouplingMatrix().getRoutes();
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        g.fillAll(get(slotBg));

        // Centre content
        auto b = getLocalBounds().toFloat();

        // XO Gold logo mark
        g.setColour(get(xoGold).withAlpha(0.18f));
        float markR = 48.0f;
        float markX = b.getCentreX() - markR;
        float markY = b.getHeight() * 0.30f - markR;
        g.fillEllipse(markX, markY, markR * 2, markR * 2);
        g.setColour(get(xoGold).withAlpha(0.55f));
        g.drawEllipse(markX, markY, markR * 2, markR * 2, 2.0f);
        g.setFont(juce::Font(22.0f).boldened());
        g.drawText("XO", (int)markX, (int)markY, (int)markR * 2, (int)markR * 2,
                   juce::Justification::centred);

        // Instruction
        g.setColour(get(textMid));
        g.setFont(juce::Font(12.0f));
        float instrY = b.getHeight() * 0.30f + markR + 16.0f;
        g.drawText("Click an engine tile to edit parameters",
                   b.withY(instrY).withHeight(20.0f).toNearestInt(),
                   juce::Justification::centred);

        // Active engine chain
        std::vector<std::pair<juce::String, juce::Colour>> active;
        for (int i = 0; i < XOmnibusProcessor::MaxSlots; ++i)
        {
            auto* eng = processor.getEngine(i);
            if (eng) active.push_back({eng->getEngineId(), eng->getAccentColour()});
        }

        if (active.empty())
        {
            g.setColour(get(textMid).withAlpha(0.35f));
            g.setFont(juce::Font(10.0f));
            g.drawText("No engines loaded — use the tiles on the left",
                       b.withY(b.getHeight() * 0.65f).withHeight(20.0f).toNearestInt(),
                       juce::Justification::centred);
            return;
        }

        // Draw engine chain pills with XO Gold connectors
        float chainY = b.getHeight() * 0.62f;
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
            g.setFont(juce::Font(9.5f).boldened());
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

        // Coupling route visualization (uses cachedRoutes — no alloc inside paint)
        if (!cachedRoutes.empty())
        {
            const auto& routes = cachedRoutes;
            float matY = chainY + pillH * 0.5f + 18.0f;
            float cellW = 88.0f, cellH = 18.0f;
            int numActive = (int)std::count_if(routes.begin(), routes.end(),
                                               [](const auto& r){ return r.active; });
            if (numActive > 0)
            {
                g.setColour(get(xoGold).withAlpha(0.55f));
                g.setFont(juce::Font(8.0f).boldened());
                g.drawText("COUPLING ROUTES (" + juce::String(numActive) + ")",
                           b.withY(matY).withHeight(12.0f).toNearestInt(),
                           juce::Justification::centred);
                matY += 14.0f;

                for (const auto& route : routes)
                {
                    if (!route.active) continue;
                    auto* src = processor.getEngine(route.sourceSlot);
                    auto* dst = processor.getEngine(route.destSlot);
                    if (!src || !dst) continue;

                    juce::String rowText = src->getEngineId().substring(0, 4).toUpperCase()
                                          + " -> "
                                          + dst->getEngineId().substring(0, 4).toUpperCase()
                                          + "  " + couplingTypeLabel(route.type)
                                          + "  (" + juce::String(route.amount, 2) + ")";

                    auto rowBounds = b.withY(matY).withHeight(cellH).toNearestInt();
                    rowBounds = rowBounds.withLeft(rowBounds.getCentreX() - (int)(cellW * 1.4f))
                                        .withWidth((int)(cellW * 2.8f));

                    juce::Colour routeColor = route.isNormalled
                        ? get(borderGray).darker(0.1f)
                        : get(xoGold).withAlpha(0.8f);

                    g.setColour(routeColor.withAlpha(0.12f));
                    g.fillRoundedRectangle(rowBounds.toFloat(), 4.0f);
                    g.setColour(routeColor.withAlpha(0.45f));
                    g.drawRoundedRectangle(rowBounds.toFloat().reduced(0.5f), 4.0f, 1.0f);

                    g.setColour(routeColor);
                    g.setFont(juce::Font(9.0f));
                    g.drawText(rowText, rowBounds.reduced(6, 0),
                               juce::Justification::centredLeft, true);

                    matY += cellH + 2.0f;
                    if (matY + cellH > b.getBottom() - 20.0f)
                    {
                        // Count remaining routes and show overflow indicator
                        int remaining = 0;
                        for (const auto& r2 : routes)
                            if (r2.active && &r2 > &route) ++remaining;
                        if (remaining > 0)
                        {
                            g.setColour(get(textMid).withAlpha(0.35f));
                            g.setFont(juce::Font(8.5f));
                            g.drawText("+ " + juce::String(remaining) + " more route" + (remaining > 1 ? "s" : ""),
                                       b.withY(matY).withHeight(14.0f).toNearestInt(),
                                       juce::Justification::centred);
                        }
                        break;
                    }
                }
            }
            else
            {
                g.setColour(get(textMid).withAlpha(0.3f));
                g.setFont(juce::Font(9.0f));
                g.drawText("No active coupling routes",
                           b.withY(chainY + pillH * 0.5f + 10.0f).withHeight(16.0f).toNearestInt(),
                           juce::Justification::centred);
            }
        }
    }

private:
    XOmnibusProcessor& processor;
    // Cached coupling routes — updated in refresh(), never in paint()
    std::vector<MegaCouplingMatrix::CouplingRoute> cachedRoutes;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OverviewPanel)
};

//==============================================================================
// CompactEngineTile — slim tile in the left sidebar column.
// Shows engine identity. Click to select (or load engine if empty).
class CompactEngineTile : public juce::Component, private juce::Timer
{
public:
    std::function<void(int)> onSelect; // called with slot index when clicked

    CompactEngineTile(XOmnibusProcessor& proc, int slotIndex)
        : processor(proc), slot(slotIndex)
    {
        refresh();
        startTimerHz(20); // poll voice count at 20Hz
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
        accent    = hasEngine ? eng->getAccentColour()
                              : GalleryColors::get(GalleryColors::emptySlot);
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

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        auto b = getLocalBounds().toFloat().reduced(3.0f, 2.0f);

        bool hovered = isMouseOver();

        g.setColour(isSelected ? accent.withAlpha(0.12f)
                               : hovered ? get(slotBg).brighter(0.02f)
                               : get(slotBg));
        g.fillRoundedRectangle(b, 8.0f);

        g.setColour(isSelected ? accent : (hovered ? accent.withAlpha(0.4f) : get(borderGray)));
        g.drawRoundedRectangle(b, 8.0f, isSelected ? 2.5f : 1.0f);

        if (isLoading)
        {
            g.setColour(get(xoGold).withAlpha(0.5f));
            g.setFont(juce::Font(9.0f));
            g.drawText("LOADING...", b.toNearestInt(), juce::Justification::centred);
        }
        else if (hasEngine)
        {
            // Accent dot
            g.setColour(accent);
            g.fillEllipse(b.getX() + 10.0f, b.getCentreY() - 5.0f, 10.0f, 10.0f);

            // Engine name
            g.setFont(juce::Font(11.0f).boldened());
            g.setColour(isSelected ? accent : get(textDark));
            g.drawText(engineId.toUpperCase(),
                       (int)b.getX() + 26, (int)b.getY(), (int)b.getWidth() - 48, (int)b.getHeight(),
                       juce::Justification::centredLeft);

            // Voice count dots — right edge, one dot per active voice
            if (voiceCount > 0)
            {
                const float dotR = 3.0f, dotSpacing = 7.0f;
                float dotX = b.getRight() - 6.0f;
                float dotY = b.getCentreY() - dotR;
                int maxDots = std::min(voiceCount, 6);
                for (int d = 0; d < maxDots; ++d)
                {
                    g.setColour(accent.withAlpha(0.7f + 0.3f * (d == 0 ? 1.0f : 0.0f)));
                    g.fillEllipse(dotX - static_cast<float>(d) * dotSpacing, dotY, dotR * 2.0f, dotR * 2.0f);
                }
            }
        }
        else
        {
            g.setColour(get(textMid).withAlpha(0.3f));
            g.setFont(juce::Font(9.0f));
            g.drawText("SLOT " + juce::String(slot + 1) + " — empty",
                       b.toNearestInt(), juce::Justification::centred);
        }

        // Slot number badge — top-right corner, always visible for quick navigation
        g.setFont(juce::Font(8.0f).boldened());
        g.setColour(get(textMid).withAlpha(hasEngine ? 0.35f : 0.2f));
        g.drawText(juce::String(slot + 1),
                   (int)b.getRight() - 14, (int)b.getY() + 2, 12, 12,
                   juce::Justification::centredRight);
    }

    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&)  override { repaint(); }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (!e.mods.isLeftButtonDown() || e.mouseWasDraggedSinceMouseDown())
            return;

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

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
            [this, registeredIds](int result)
            {
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

    XOmnibusProcessor& processor;
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
            addAndMakeVisible(knobs[i]);
            attach[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, defs[i].id, knobs[i]);

            lbls[i].setText(defs[i].label, juce::dontSendNotification);
            lbls[i].setFont(juce::Font(8.0f).boldened());
            lbls[i].setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::textMid));
            lbls[i].setJustificationType(juce::Justification::centred);
            addAndMakeVisible(lbls[i]);
        }

        master.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        master.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        master.setColour(juce::Slider::rotarySliderFillColourId,
                         GalleryColors::get(GalleryColors::textMid));
        addAndMakeVisible(master);
        masterAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "masterVolume", master);

        masterLbl.setText("MASTER", juce::dontSendNotification);
        masterLbl.setFont(juce::Font(8.0f).boldened());
        masterLbl.setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::textMid));
        masterLbl.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(masterLbl);
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        auto b = getLocalBounds().toFloat();
        g.setColour(get(shellWhite));
        g.fillRoundedRectangle(b, 6.0f);
        g.setColour(get(xoGold));
        g.fillRoundedRectangle(b.removeFromTop(3.0f), 0.0f);
        g.setColour(get(borderGray));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 6.0f, 1.0f);

        g.setColour(get(textMid));
        g.setFont(juce::Font(8.0f).boldened());
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
// AdvancedFXPanel — shown in a CallOutBox for secondary FX parameters.
// Hosts 4 knobs: reverb size, comp ratio, comp attack, comp release.
class AdvancedFXPanel : public juce::Component
{
public:
    explicit AdvancedFXPanel(juce::AudioProcessorValueTreeState& apvts)
    {
        struct Def { const char* id; const char* label; };
        static constexpr Def defs[4] = {
            {"master_reverbSize",  "SIZE"},
            {"master_compRatio",   "RATIO"},
            {"master_compAttack",  "ATTACK"},
            {"master_compRelease", "RELEASE"},
        };
        for (int i = 0; i < 4; ++i)
        {
            knobs[i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
            knobs[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            knobs[i].setColour(juce::Slider::rotarySliderFillColourId,
                               GalleryColors::get(GalleryColors::textMid).withAlpha(0.75f));
            addAndMakeVisible(knobs[i]);
            attach[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, defs[i].id, knobs[i]);

            lbls[i].setText(defs[i].label, juce::dontSendNotification);
            lbls[i].setFont(juce::Font(8.5f).boldened());
            lbls[i].setColour(juce::Label::textColourId,
                              GalleryColors::get(GalleryColors::textMid));
            lbls[i].setJustificationType(juce::Justification::centred);
            addAndMakeVisible(lbls[i]);
        }
        setSize(256, 96);
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        g.fillAll(get(shellWhite));
        g.setColour(get(textMid).withAlpha(0.40f));
        g.setFont(juce::Font(8.0f).boldened());
        g.drawText("ADVANCED  ·  REVERB + COMP",
                   getLocalBounds().removeFromTop(14).reduced(8, 0),
                   juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(8, 4);
        b.removeFromTop(14);
        int cw = b.getWidth() / 4;
        for (int i = 0; i < 4; ++i)
        {
            auto col = b.removeFromLeft(cw);
            int kh = 44;
            int ky = col.getCentreY() - (kh + 13) / 2;
            knobs[i].setBounds(col.getCentreX() - kh / 2, ky, kh, kh);
            lbls[i].setBounds(col.getX(), ky + kh + 2, col.getWidth(), 12);
        }
    }

private:
    std::array<juce::Slider, 4> knobs;
    std::array<juce::Label, 4>  lbls;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> attach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdvancedFXPanel)
};

//==============================================================================
// MasterFXSection — 3 primary knobs (SAT DRIVE, REVERB MIX, COMP GLUE) +
// ADV button that opens a CallOutBox for the 4 secondary parameters.
// Always visible at the bottom of the Gallery Model.
class MasterFXSection : public juce::Component
{
public:
    explicit MasterFXSection(juce::AudioProcessorValueTreeState& apvts) : myApvts(apvts)
    {
        struct Def { const char* id; const char* label; const char* section; };
        static constexpr Def defs[3] = {
            {"master_satDrive",  "DRIVE", "SAT"},
            {"master_reverbMix", "MIX",   "REVERB"},
            {"master_compMix",   "GLUE",  "COMP"},
        };
        for (int i = 0; i < 3; ++i)
        {
            knobs[i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
            knobs[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            knobs[i].setColour(juce::Slider::rotarySliderFillColourId,
                               GalleryColors::get(GalleryColors::textMid).withAlpha(0.7f));
            addAndMakeVisible(knobs[i]);
            attach[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, defs[i].id, knobs[i]);

            lbls[i].setText(defs[i].label, juce::dontSendNotification);
            lbls[i].setFont(juce::Font(8.5f).boldened());
            lbls[i].setColour(juce::Label::textColourId,
                              GalleryColors::get(GalleryColors::textMid));
            lbls[i].setJustificationType(juce::Justification::centred);
            addAndMakeVisible(lbls[i]);

            sectionLbls[i].setText(defs[i].section, juce::dontSendNotification);
            sectionLbls[i].setFont(juce::Font(7.0f));
            sectionLbls[i].setColour(juce::Label::textColourId,
                                     GalleryColors::get(GalleryColors::textMid).withAlpha(0.45f));
            sectionLbls[i].setJustificationType(juce::Justification::centred);
            addAndMakeVisible(sectionLbls[i]);
        }

        advBtn.setButtonText("ADV");
        advBtn.setTooltip("Reverb Size \xc2\xb7 Comp Ratio, Attack, Release");
        advBtn.setColour(juce::TextButton::buttonColourId,
                         GalleryColors::get(GalleryColors::shellWhite));
        advBtn.setColour(juce::TextButton::textColourOffId,
                         GalleryColors::get(GalleryColors::textMid));
        advBtn.onClick = [this] { showAdvanced(); };
        addAndMakeVisible(advBtn);
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        auto b = getLocalBounds().toFloat();
        g.setColour(get(shellWhite).darker(0.03f));
        g.fillRoundedRectangle(b, 6.0f);
        g.setColour(get(borderGray));
        g.drawRoundedRectangle(b.reduced(0.5f), 6.0f, 1.0f);

        // Subtle dividers between the three sections (drawn only after first resized())
        g.setColour(get(borderGray).withAlpha(0.5f));
        if (divX[0] > 0 && divX[1] > 0)
        {
            g.drawLine((float)divX[0], 8.0f, (float)divX[0], (float)getHeight() - 8, 1.0f);
            g.drawLine((float)divX[1], 8.0f, (float)divX[1], (float)getHeight() - 8, 1.0f);
        }
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(6, 4);
        advBtn.setBounds(b.removeFromRight(38).withSizeKeepingCentre(36, 20));

        int cw = b.getWidth() / 3;
        divX[0] = b.getX() + cw;
        divX[1] = b.getX() + 2 * cw;

        for (int i = 0; i < 3; ++i)
        {
            auto col = b.removeFromLeft(cw);
            int kh = 44, lh = 12, sh = 10;
            int ky = col.getCentreY() - (kh + lh) / 2;
            knobs[i].setBounds(col.getCentreX() - kh / 2, ky, kh, kh);
            lbls[i].setBounds(col.getX(), ky + kh + 2, col.getWidth(), lh);
            sectionLbls[i].setBounds(col.getX(), col.getY() + 2, col.getWidth(), sh);
        }
    }

private:
    void showAdvanced()
    {
        juce::CallOutBox::launchAsynchronously(
            std::make_unique<AdvancedFXPanel>(myApvts),
            advBtn.getScreenBounds(),
            nullptr);
    }

    juce::AudioProcessorValueTreeState& myApvts;
    std::array<juce::Slider, 3> knobs;
    std::array<juce::Label, 3>  lbls;
    std::array<juce::Label, 3>  sectionLbls;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 3> attach;
    juce::TextButton advBtn;
    int divX[2] = {0, 0};

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
                                           GalleryColors::get(GalleryColors::textMid).withAlpha(0.4f));
        searchField.setColour(juce::TextEditor::backgroundColourId,
                              GalleryColors::get(GalleryColors::slotBg));
        searchField.setColour(juce::TextEditor::outlineColourId,
                              GalleryColors::get(GalleryColors::borderGray));
        searchField.setColour(juce::TextEditor::textColourId,
                              GalleryColors::get(GalleryColors::textDark));
        searchField.setFont(juce::Font(11.0f));
        searchField.onTextChange = [this] { updateFilter(); };
        addAndMakeVisible(searchField);

        // Mood filter buttons (ALL = index 0, then 6 moods)
        static const char* moodLabels[] = {
            "ALL", "Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether"
        };
        for (int i = 0; i < kNumMoods; ++i)
        {
            moodBtns[i].setButtonText(moodLabels[i]);
            moodBtns[i].setClickingTogglesState(false);
            moodBtns[i].setColour(juce::TextButton::buttonColourId,
                                  GalleryColors::get(GalleryColors::shellWhite));
            moodBtns[i].setColour(juce::TextButton::textColourOffId,
                                  GalleryColors::get(GalleryColors::textMid));
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
                          GalleryColors::get(GalleryColors::slotBg));
        listBox.setColour(juce::ListBox::outlineColourId,
                          GalleryColors::get(GalleryColors::borderGray));
        listBox.setOutlineThickness(1);
        addAndMakeVisible(listBox);

        // Count label
        countLabel.setFont(juce::Font(8.5f));
        countLabel.setColour(juce::Label::textColourId,
                             GalleryColors::get(GalleryColors::textMid).withAlpha(0.55f));
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
            g.fillAll(get(shellWhite));
        else
            g.fillAll(get(slotBg));

        // Mood accent dot
        static const juce::Colour moodColors[] = {
            juce::Colour(0xFFC8553D), // Foundation → Snap/Terracotta
            juce::Colour(0xFF2A9D8F), // Atmosphere → Morph/Teal
            juce::Colour(0xFF7B2D8B), // Entangled  → Drift/Violet
            juce::Colour(0xFF0066FF), // Prism      → Onset/Blue
            juce::Colour(0xFFE9A84A), // Flux       → Bob/Amber
            juce::Colour(0xFFA78BFA), // Aether     → Opal/Lavender
        };
        static const char* moodIds[] = {
            "Foundation","Atmosphere","Entangled","Prism","Flux","Aether"
        };
        juce::Colour dot = get(borderGray);
        for (int mi = 0; mi < 6; ++mi)
            if (preset.mood == moodIds[mi]) { dot = moodColors[mi]; break; }

        g.setColour(dot.withAlpha(0.7f));
        g.fillEllipse(8.0f, h * 0.5f - 3.5f, 7.0f, 7.0f);

        // Preset name
        g.setColour(get(selected ? textDark : textMid));
        g.setFont(juce::Font(10.5f));
        g.drawText(preset.name, 22, 0, w - 36, h,
                   juce::Justification::centredLeft, true);

        // Engine tag if multi-engine
        if (!preset.engines.isEmpty())
        {
            auto tag = preset.engines[0].substring(0, 3).toUpperCase();
            g.setColour(get(textMid).withAlpha(0.30f));
            g.setFont(juce::Font(7.5f));
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
        g.fillAll(get(shellWhite));
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
            "", "Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether"
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

    static constexpr int kNumMoods = 7;

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

//==============================================================================
// PresetBrowserStrip — prev/next navigation + preset name display.
// Lives in the editor header. Calls processor.applyPreset() on navigation.
class PresetBrowserStrip : public juce::Component,
                           private PresetManager::Listener
{
public:
    PresetBrowserStrip(XOmnibusProcessor& proc)
        : processor(proc)
    {
        prevBtn.setButtonText("<");
        nextBtn.setButtonText(">");
        browseBtn.setButtonText("\xe2\x8a\x9e"); // ⊞ grid icon (UTF-8)
        prevBtn.setTooltip("Previous preset");
        nextBtn.setTooltip("Next preset");
        browseBtn.setTooltip("Browse all presets by mood");

        for (auto* btn : {&prevBtn, &nextBtn, &browseBtn})
        {
            btn->setColour(juce::TextButton::buttonColourId,
                           GalleryColors::get(GalleryColors::shellWhite));
            btn->setColour(juce::TextButton::textColourOffId,
                           GalleryColors::get(GalleryColors::textMid));
            addAndMakeVisible(*btn);
        }

        nameLabel.setJustificationType(juce::Justification::centred);
        nameLabel.setFont(juce::Font(10.5f).boldened());
        nameLabel.setColour(juce::Label::textColourId,
                            GalleryColors::get(GalleryColors::textDark));
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

        auto onSelect = [safeThis, &proc = processor](const PresetData& preset)
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

        juce::CallOutBox::launchAsynchronously(
            std::make_unique<PresetBrowserPanel>(pm, std::move(onSelect)),
            browseBtn.getScreenBounds(),
            nullptr);
    }

    XOmnibusProcessor& processor;
    juce::TextButton prevBtn, nextBtn, browseBtn;
    juce::Label nameLabel;
    MacroSection* macroSection = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserStrip)
};

//==============================================================================
// XOmnibusEditor — Gallery Model plugin window.
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
class XOmnibusEditor : public juce::AudioProcessorEditor,
                       private juce::Timer
{
public:
    explicit XOmnibusEditor(XOmnibusProcessor& proc)
        : AudioProcessorEditor(proc),
          processor(proc),
          overview(proc),
          detail(proc),
          macros(proc.getAPVTS()),
          masterFXStrip(proc.getAPVTS()),
          presetBrowser(proc)
    {
        laf = std::make_unique<GalleryLookAndFeel>();
        setLookAndFeel(laf.get());

        for (int i = 0; i < XOmnibusProcessor::MaxSlots; ++i)
        {
            tiles[i] = std::make_unique<CompactEngineTile>(proc, i);
            tiles[i]->onSelect = [this, i](int slot) { selectSlot(slot); };
            addAndMakeVisible(*tiles[i]);
        }

        addAndMakeVisible(overview);
        addAndMakeVisible(detail);
        addAndMakeVisible(macros);
        addAndMakeVisible(masterFXStrip);
        addAndMakeVisible(presetBrowser);

        detail.setVisible(false);
        detail.setAlpha(0.0f);

        // Scan factory preset directory
        auto presetDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                             .getChildFile("Application Support/XO_OX/XOmnibus/Presets");
        if (presetDir.isDirectory())
            proc.getPresetManager().scanPresetDirectory(presetDir);
        presetBrowser.setMacroSection(&macros); // wire preset macroLabels → macro knob labels
        presetBrowser.updateDisplay();

        // Event-driven tile refresh: only repaint the affected tile and overview on engine change.
        proc.onEngineChanged = [this](int slot)
        {
            if (slot >= 0 && slot < XOmnibusProcessor::MaxSlots)
                tiles[slot]->refresh();
            overview.refresh();
        };

        setSize(880, 562);
        setWantsKeyboardFocus(true);
        startTimerHz(1); // Reduced from 5Hz — idle polling only as a fallback
    }

    ~XOmnibusEditor() override
    {
        stopTimer();
        processor.onEngineChanged = nullptr; // prevent callback after editor is destroyed
        setLookAndFeel(nullptr);
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        // Keys 1-4 jump directly to engine slots
        for (int i = 0; i < XOmnibusProcessor::MaxSlots; ++i)
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
        g.fillAll(get(shellWhite));

        // Header area
        auto header = getLocalBounds().removeFromTop(kHeaderH).toFloat();
        g.setColour(get(shellWhite));
        g.fillRect(header);

        // XO Gold stripe
        g.setColour(get(xoGold));
        g.fillRect(header.removeFromBottom(3.0f));

        g.setColour(get(textDark));
        g.setFont(juce::Font(19.0f).boldened());
        g.drawText("XOmnibus",
                   juce::Rectangle<int>(16, 0, 160, kHeaderH - 3),
                   juce::Justification::centredLeft);

        g.setColour(get(textDark).withAlpha(0.5f)); // xoGold (#E9C46A) fails WCAG AA on shellWhite
        g.setFont(juce::Font(8.5f).boldened());
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
                : "9 Engines · 12 Coupling Types · 1000 Presets";

            g.setColour(activeRoutes > 0 ? get(xoGold) : get(textMid).withAlpha(0.5f));
            g.setFont(juce::Font(9.0f));
            g.drawText(routeLabel,
                       juce::Rectangle<int>(getWidth() - 310, 0, 298, kHeaderH - 6),
                       juce::Justification::centredRight);
        }

        // Sidebar separator
        int sepX = kSidebarW;
        g.setColour(get(borderGray));
        g.drawVerticalLine(sepX, (float)kHeaderH, (float)(getHeight() - kMacroH));
    }

    void resized() override
    {
        auto area = getLocalBounds();

        // Header: reserve space for preset browser (right side)
        auto header = area.removeFromTop(kHeaderH);
        presetBrowser.setBounds(header.removeFromRight(220).reduced(4, 10));

        // Bottom strips (from bottom up)
        masterFXStrip.setBounds(area.removeFromBottom(kMasterFXH).reduced(6, 3));
        macros.setBounds(area.removeFromBottom(kMacroH).reduced(6, 4));

        // Left sidebar tiles
        auto sidebar = area.removeFromLeft(kSidebarW);
        int tileH = sidebar.getHeight() / XOmnibusProcessor::MaxSlots;
        for (int i = 0; i < XOmnibusProcessor::MaxSlots; ++i)
            tiles[i]->setBounds(sidebar.removeFromTop(tileH));

        // Right panels (stacked, only one visible at a time)
        overview.setBounds(area);
        detail.setBounds(area);
    }

private:
    void selectSlot(int slot)
    {
        // Deselect all tiles
        for (int i = 0; i < XOmnibusProcessor::MaxSlots; ++i)
            tiles[i]->setSelected(i == slot);

        if (slot == selectedSlot && detail.isVisible())
            return; // already showing this one

        selectedSlot = slot;

        auto& anim = juce::Desktop::getInstance().getAnimator();

        if (detail.isVisible())
        {
            // Cross-fade: fade out → swap → fade in
            anim.fadeOut(&detail, kFadeMs);
            juce::Timer::callAfterDelay(kFadeMs, [this, slot]
            {
                if (detail.loadSlot(slot))
                {
                    overview.setVisible(false);
                    detail.setAlpha(0.0f);
                    detail.setVisible(true);
                    juce::Desktop::getInstance().getAnimator().fadeIn(&detail, kFadeMs);
                }
                else
                {
                    showOverview();
                }
            });
        }
        else
        {
            // Fade out overview, fade in detail
            anim.fadeOut(&overview, kFadeMs);
            juce::Timer::callAfterDelay(kFadeMs, [this, slot]
            {
                if (detail.loadSlot(slot))
                {
                    overview.setVisible(false);
                    detail.setAlpha(0.0f);
                    detail.setVisible(true);
                    juce::Desktop::getInstance().getAnimator().fadeIn(&detail, kFadeMs);
                }
                else
                {
                    overview.setAlpha(1.0f);
                    overview.setVisible(true);
                }
            });
        }
    }

    void showOverview()
    {
        selectedSlot = -1;
        for (int i = 0; i < XOmnibusProcessor::MaxSlots; ++i)
            tiles[i]->setSelected(false);

        auto& anim = juce::Desktop::getInstance().getAnimator();
        if (detail.isVisible())
        {
            anim.fadeOut(&detail, kFadeMs);
            juce::Timer::callAfterDelay(kFadeMs, [this]
            {
                detail.setVisible(false);
                overview.setAlpha(0.0f);
                overview.setVisible(true);
                juce::Desktop::getInstance().getAnimator().fadeIn(&overview, kFadeMs);
            });
        }
    }

    void timerCallback() override
    {
        for (int i = 0; i < XOmnibusProcessor::MaxSlots; ++i)
            tiles[i]->refresh();
        if (!detail.isVisible())
            overview.refresh();
    }

    static constexpr int kHeaderH   = 50;
    static constexpr int kMacroH    = 105;
    static constexpr int kMasterFXH = 68;
    static constexpr int kSidebarW  = 155;
    static constexpr int kFadeMs    = 150;

    XOmnibusProcessor& processor;
    std::unique_ptr<GalleryLookAndFeel> laf;

    std::array<std::unique_ptr<CompactEngineTile>, XOmnibusProcessor::MaxSlots> tiles;
    OverviewPanel      overview;
    EngineDetailPanel  detail;
    MacroSection       macros;
    MasterFXSection    masterFXStrip;
    PresetBrowserStrip presetBrowser;

    int selectedSlot = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XOmnibusEditor)
};

} // namespace xomnibus
