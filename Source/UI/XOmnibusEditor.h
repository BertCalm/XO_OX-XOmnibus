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
// OverviewPanel — right-side content when no engine is selected.
class OverviewPanel : public juce::Component
{
public:
    explicit OverviewPanel(XOmnibusProcessor& proc) : processor(proc) {}

    void refresh() { repaint(); }

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

        // "COUPLING ACTIVE" label if multiple engines
        if (active.size() >= 2)
        {
            g.setColour(get(xoGold).withAlpha(0.6f));
            g.setFont(juce::Font(8.5f).boldened());
            g.drawText("COUPLING ACTIVE",
                       b.withY(chainY + pillH * 0.5f + 10.0f).withHeight(14.0f).toNearestInt(),
                       juce::Justification::centred);
        }
    }

private:
    XOmnibusProcessor& processor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OverviewPanel)
};

//==============================================================================
// CompactEngineTile — slim tile in the left sidebar column.
// Shows engine identity. Click to select (or load engine if empty).
class CompactEngineTile : public juce::Component
{
public:
    std::function<void(int)> onSelect; // called with slot index when clicked

    CompactEngineTile(XOmnibusProcessor& proc, int slotIndex)
        : processor(proc), slot(slotIndex)
    {
        refresh();
    }

    void refresh()
    {
        auto* eng = processor.getEngine(slot);
        hasEngine = (eng != nullptr);
        engineId  = hasEngine ? eng->getEngineId() : juce::String{};
        accent    = hasEngine ? eng->getAccentColour()
                              : GalleryColors::get(GalleryColors::emptySlot);
        repaint();
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

        if (hasEngine)
        {
            // Accent dot
            g.setColour(accent);
            g.fillEllipse(b.getX() + 10.0f, b.getCentreY() - 5.0f, 10.0f, 10.0f);

            // Engine name
            g.setFont(juce::Font(11.0f).boldened());
            g.setColour(isSelected ? accent : get(textDark));
            g.drawText(engineId.toUpperCase(),
                       (int)b.getX() + 26, (int)b.getY(), (int)b.getWidth() - 30, (int)b.getHeight(),
                       juce::Justification::centredLeft);
        }
        else
        {
            g.setColour(get(textMid).withAlpha(0.3f));
            g.setFont(juce::Font(9.0f));
            g.drawText("SLOT " + juce::String(slot + 1) + " — empty",
                       b.toNearestInt(), juce::Justification::centred);
        }
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
        static const std::pair<const char*, juce::Colour> engines[] = {
            {"Snap",      juce::Colour(0xFFC8553D)},
            {"Morph",     juce::Colour(0xFF2A9D8F)},
            {"Dub",       juce::Colour(0xFF6B7B3A)},
            {"Drift",     juce::Colour(0xFF7B2D8B)},
            {"Bob",       juce::Colour(0xFFE9A84A)},
            {"Fat",       juce::Colour(0xFFFF1493)},
            {"Onset",     juce::Colour(0xFF0066FF)},
            {"Overworld", juce::Colour(0xFF39FF14)},
        };

        juce::PopupMenu menu;
        menu.addSectionHeader("LOAD INTO SLOT " + juce::String(slot + 1));
        menu.addSeparator();

        for (int i = 0; i < (int)std::size(engines); ++i)
        {
            if (EngineRegistry::instance().isRegistered(engines[i].first))
                menu.addColouredItem(i + 1, engines[i].first, engines[i].second,
                                     true, false);
        }

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
            [this](int result)
            {
                static const char* ids[] = {
                    "Snap","Morph","Dub","Drift","Bob","Fat","Onset","Overworld"
                };
                if (result >= 1 && result <= (int)std::size(ids))
                {
                    processor.loadEngine(slot, ids[result - 1]);
                    juce::Timer::callAfterDelay(120, [this]
                    {
                        refresh();
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
          macros(proc.getAPVTS())
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

        detail.setVisible(false);
        detail.setAlpha(0.0f);

        setSize(880, 490);
        startTimerHz(5);
    }

    ~XOmnibusEditor() override
    {
        stopTimer();
        setLookAndFeel(nullptr);
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

        g.setColour(get(xoGold));
        g.setFont(juce::Font(8.5f).boldened());
        g.drawText("XO_OX Designs",
                   juce::Rectangle<int>(16, kHeaderH - 18, 110, 12),
                   juce::Justification::centredLeft);

        g.setColour(get(textMid).withAlpha(0.5f));
        g.setFont(juce::Font(9.0f));
        g.drawText("7 Engines · 12 Coupling Types · 1000 Presets",
                   juce::Rectangle<int>(getWidth() - 310, 0, 298, kHeaderH - 6),
                   juce::Justification::centredRight);

        // Sidebar separator
        int sepX = kSidebarW;
        g.setColour(get(borderGray));
        g.drawVerticalLine(sepX, (float)kHeaderH, (float)(getHeight() - kMacroH));
    }

    void resized() override
    {
        auto area = getLocalBounds();
        area.removeFromTop(kHeaderH);
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

    static constexpr int kHeaderH  = 50;
    static constexpr int kMacroH   = 105;
    static constexpr int kSidebarW = 155;
    static constexpr int kFadeMs   = 150;

    XOmnibusProcessor& processor;
    std::unique_ptr<GalleryLookAndFeel> laf;

    std::array<std::unique_ptr<CompactEngineTile>, XOmnibusProcessor::MaxSlots> tiles;
    OverviewPanel   overview;
    EngineDetailPanel detail;
    MacroSection    macros;

    int selectedSlot = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XOmnibusEditor)
};

} // namespace xomnibus
